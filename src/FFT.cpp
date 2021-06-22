#include "FFT.hpp"
#include "window.hpp"

#include <cmath>


namespace {
    // Wrap an arbitrary phase in radians into the range ]-pi, pi]
    double wrap_phase(float32_t phase){
        return std::fmod(phase + M_PI, -2.0 * M_PI) + M_PI;
    }
}


void FFT::update(void)
{
    // get input block
    audio_block_t *input_block;
    input_block = receiveReadOnly();
    if (!input_block) return;

    // allocate output block
    audio_block_t *output_block;
    output_block = allocate();
    if (!output_block) {
        release(input_block);
        return;
    }

#if defined(__ARM_ARCH_7EM__)
    // get the input block data
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        // fill the input buffer from the back with a chunck of length HOP_SIZE, so we can easier overlap and add later
        const uint16_t index = i + m_offset + FRAME_OVERLAP;
        m_inputBuffer[index] = input_block->data[i];
    }

    // fill the output block
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        output_block->data[i] = m_outputBuffer[i + m_offset];
    }

    // send out the audio block
    transmit(output_block, 0);
    release(input_block);
    release(output_block);

    // check if we collected enough buffers for another fft
    m_offset += AUDIO_BLOCK_SAMPLES;
    if (m_offset < HOP_SIZE) {
        return;
    }
    m_offset = 0;

    // convert buffer to float
    arm_q15_to_float(m_inputBuffer, m_floatInBuffer, FRAME_SIZE);

    // move a chunck of length FRAME_OVERLAP by HOP_SIZE to the front of the input buffer for the next fft
    std::memmove(m_inputBuffer, m_inputBuffer + HOP_SIZE, sizeof(int16_t) * FRAME_OVERLAP);

    // apply window function
    arm_mult_f32(m_floatInBuffer, const_cast<float*>(HannWindow2048), m_floatInBuffer, FRAME_SIZE);

    // Serial.println("Converted to float: ");
    // for (int i = 0; i < FFT_LENGTH; i++)
    // {
    //     Serial.print(m_floatInBuffer[i]);
    //     Serial.print(", ");
    // }
    // Serial.println();

    // do the fft
    // the FFT output is complex and in the following format
    // {real(0), imag(0), real(1), imag(1), ...}
    // real[0] represents the DC offset, and imag[0] should be 0
#if defined(KINETISK)
    arm_rfft_f32(&m_fftInst, m_floatInBuffer, m_floatComplexBuffer);
#elif defined(__IMXRT1062__)
    arm_rfft_fast_f32(&m_fftInst, m_floatInBuffer, m_floatComplexBuffer, 0);
#endif
    

    // analyse the lower half of the signal (upper half is the same but mirrored)
    //Serial.println("Magnitues: ");
    for (int i = 0; i < HALF_FRAME_SIZE; i++) {
        // deinterlace the FFT result
        float32_t real = m_floatComplexBuffer[i * 2];
        float32_t imag = m_floatComplexBuffer[i * 2 + 1];

        // compute phase and magnitude
        float32_t magnitude = 2.0f * std::sqrt(real * real + imag * imag);
        float32_t phase = std::atan2(imag, real);
        //Serial.print(magnitude);
        //Serial.print(", ");

        // compute phase difference
        float32_t temp = phase - m_previousPhases[i];
        m_previousPhases[i] = phase;

        // subtract the expected phase difference
        temp -= static_cast<float32_t>(i) * m_omega;

        // map phase into the range ]-pi, pi]
        temp = wrap_phase(temp);

        // get deviation from bin frequency from the +/- Pi interval
        temp = OVERSAMPLING_FACTOR * temp / (2. * M_PI);

        // compute the k-th partials' true frequency
        temp = static_cast<float32_t>(i) * m_binFrequencyWidth + temp * m_binFrequencyWidth;

        // save magnitude and true frequency
        m_magnitudes[i] = magnitude;
        m_frequencies[i] = temp;
    }
    //Serial.println();

    // do the actual pitchshifting
    std::memset(m_synthesisMagnitudes, 0, sizeof m_synthesisMagnitudes);
    std::memset(m_synthesisFrequencies, 0, sizeof m_synthesisFrequencies);
    for (int i = 0; i < HALF_FRAME_SIZE; i++) {
        uint16_t index = i * m_pitchShiftFactor;
        if (index <= HALF_FRAME_SIZE) {
            m_synthesisMagnitudes[index] += m_magnitudes[i];
            m_synthesisFrequencies[index] = m_frequencies[i] * m_pitchShiftFactor;
        }
    }

    // synthesis the signal
    for (int i = 0; i < HALF_FRAME_SIZE; i++) {
        // get magnitude and true frquency from the synthesis array
        float32_t magnitude = m_synthesisMagnitudes[i];
        float32_t temp = m_synthesisFrequencies[i];

        // subtract bin mid frequency
        temp -= static_cast<float32_t>(i) * m_binFrequencyWidth;

        // get bin deviation from freq deviation
        temp /= m_binFrequencyWidth;

        // take oversamping into account
        temp = 2.0 * M_PI * temp / OVERSAMPLING_FACTOR;

        // add the overlap phase advance back in
        temp += static_cast<float32_t>(i) * m_omega;

        // accumulate delta phase to get bin phase
        m_phaseSum[i] += temp;
        temp = m_phaseSum[i];

        // compute real and imaginary part and re-interleave
        m_floatComplexBuffer[i * 2] = magnitude * std::cos(temp);
        m_floatComplexBuffer[i * 2 + 1] = magnitude * std::sin(temp);
    }

    // zero negative frequencies 
    for (int i = FRAME_SIZE /* + 2 only if <= */; i < 2 * FRAME_SIZE; i++)
        m_floatComplexBuffer[i] = 0.f;

    // do the ifft
#if defined(KINETISK)
    arm_rfft_f32(&m_ifftInst, m_floatComplexBuffer, m_floatOutBuffer);
#elif defined(__IMXRT1062__)
    arm_rfft_fast_f32(&m_fftInst, m_floatComplexBuffer, m_floatOutBuffer, 1);
#endif

    // apply window function again
    arm_mult_f32(m_floatOutBuffer, const_cast<float*>(HannWindow2048), m_floatOutBuffer, FRAME_SIZE);

    // Serial.println("IFFT result: ");
    // for (int i = 0; i < FFT_LENGTH; i++)
    // {
    //     Serial.print(m_floatOutBuffer[i]);
    //     Serial.print(", ");
    // }
    // Serial.println();
    // Serial.println();

    // convert floats back to int
    arm_float_to_q15(m_floatOutBuffer, m_outputBuffer, FRAME_SIZE);

    // add overlap of the previous output the new output
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // TODO: this could be optimized
        m_outputBuffer[i] += m_overlapBuffer[i];
    }

    // save the overlap for the next round
    std::memcpy(m_overlapBuffer, m_outputBuffer + HOP_SIZE, sizeof(int16_t) * FRAME_OVERLAP);    

#else
    release(input_block);
    release(output_block);
#endif
}
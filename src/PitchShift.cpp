//////////////////////////////////////////////////////////////////////
//
// Ultrasonic Hearing - Enabling you to extend your senses into 
// the ultrasonic range using a Teensy 4.1
// Copyright (C) 2021  Maximilian Wagenbach
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//////////////////////////////////////////////////////////////////////


#include <PitchShift.hpp>

#include <cmath>


namespace {
    // Wrap an arbitrary phase in radians into the range ]-pi, pi]
    double wrap_phase(float32_t phase){
        return std::fmod(phase + M_PI, -2.0 * M_PI) + M_PI;
    }
}


PitchShift::PitchShift(uint32_t sampleRate, float32_t pitchShiftFactor) :
    AudioStream(1, inputQueueArray),
    m_binFrequencyWidth{static_cast<float32_t>(sampleRate) / FRAME_SIZE},
    m_pitchShiftFactor(pitchShiftFactor),
    m_highPassCutoff(0.f),
    m_offset{0}
{
    // initialize FFT
#if defined(KINETISK)
    arm_status status = arm_rfft_init_f32(&m_fftInst, &m_fftComplexInst, FRAME_SIZE, 0, 1);
    if (status == ARM_MATH_SUCCESS)
        Serial.println("FFT initilized!");
    else if (status == ARM_MATH_ARGUMENT_ERROR)
        Serial.println("FFT size not supported");
    
    status = arm_rfft_init_f32(&m_ifftInst, &m_ifftComplexInst, FRAME_SIZE, 1, 1);
    if (status == ARM_MATH_SUCCESS)
        Serial.println("iFFT initilized!");
    else if (status == ARM_MATH_ARGUMENT_ERROR)
        Serial.println("iFFT size not supported");
#elif defined(__IMXRT1062__)
    arm_status status = arm_rfft_fast_init_f32(&m_fftInst, FRAME_SIZE);
    if (status == ARM_MATH_SUCCESS)
        Serial.println("FFT initilized!");
    else if (status == ARM_MATH_ARGUMENT_ERROR)
        Serial.println("FFT size not supported");
#endif
    // generate window
    generateWindow();

    // initialize buffers
    std::memset(m_inputBuffer, 0, sizeof m_inputBuffer);
    std::memset(m_outputBuffer, 0, sizeof m_outputBuffer);
    std::memset(m_overlapBuffer, 0, sizeof m_overlapBuffer);
    std::memset(m_previousPhases, 0, sizeof m_previousPhases);
    std::memset(m_magnitudes, 0, sizeof m_magnitudes);
    std::memset(m_frequencies, 0, sizeof m_frequencies);
    std::memset(m_synthesisMagnitudes, 0, sizeof m_synthesisMagnitudes);
    std::memset(m_synthesisFrequencies, 0, sizeof m_synthesisFrequencies);
    std::memset(m_phaseSum, 0, sizeof m_phaseSum);
}


void PitchShift::generateWindow()
{
    // generate a Hann window with 0 on both ends
    for(int i = 0; i < FRAME_SIZE; i++)
    {
        m_window[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * (static_cast<double>(i) / (FRAME_SIZE - 1))));
    }
}


void PitchShift::setHighPassCutoff(float cutoff)
{
    if (cutoff < 0.f)
        cutoff = 0.f;
    m_highPassCutoff = cutoff;
}


void PitchShift::update(void)
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

#if defined(__ARM_ARCH_7EM__)  // Armv7-M (Cortex-M4) with FPU
    // copy the input block data into the input buffer 
    // starting with an offset of FRAME_OVERLAP iteratively filling a chunck of length HOP_SIZE (making overlap & add easier later)
    std::memcpy(m_inputBuffer + m_offset + FRAME_OVERLAP, input_block->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);

    // fill the output block
    std::memcpy(output_block->data, m_outputBuffer + m_offset, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);

    // send out the output block
    transmit(output_block, 0);
    release(input_block);
    release(output_block);

    // check if we have collected enough new buffers for another FFT
    m_offset += AUDIO_BLOCK_SAMPLES;
    if (m_offset < HOP_SIZE) {
        return;
    }
    m_offset = 0;

    // convert buffer to float
    arm_q15_to_float(m_inputBuffer, m_floatInBuffer, FRAME_SIZE);

    // as preparation for the next FFT, move a chunck of length FRAME_OVERLAP by HOP_SIZE to the beginning of the input buffer
    std::memmove(m_inputBuffer, m_inputBuffer + HOP_SIZE, sizeof(int16_t) * FRAME_OVERLAP);

    // apply the window function
    arm_mult_f32(m_floatInBuffer, m_window, m_floatInBuffer, FRAME_SIZE);

    // do the FFT
    // the FFT output is complex and in the following format
    // {real(0), imag(0), real(1), imag(1), ...}
    // real[0] represents the DC offset, and imag[0] should be 0
#if defined(KINETISK)
    arm_rfft_f32(&m_fftInst, m_floatInBuffer, m_floatComplexBuffer);
#elif defined(__IMXRT1062__)
    arm_rfft_fast_f32(&m_fftInst, m_floatInBuffer, m_floatComplexBuffer, 0);
#endif

    // analyse the lower half of the signal (upper half is the same just mirrored)
    for (int i = 0; i < HALF_FRAME_SIZE; i++) {
        // deinterlace the FFT result
        float32_t real = m_floatComplexBuffer[i * 2];
        float32_t imag = m_floatComplexBuffer[i * 2 + 1];

        // compute phase and magnitude
        float32_t magnitude = 2.0f * std::sqrt(real * real + imag * imag);
        float32_t phase = std::atan2(imag, real);

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

    std::memset(m_synthesisMagnitudes, 0, sizeof m_synthesisMagnitudes);
    std::memset(m_synthesisFrequencies, 0, sizeof m_synthesisFrequencies);
    // apply a high pass filter by zeroing lower FFT bins
    const uint16_t startIndex = std::round(m_highPassCutoff / m_binFrequencyWidth);
    for (int i = startIndex; i < HALF_FRAME_SIZE; i++) {
        // do the actual pitchshifting
        uint16_t index = i * m_pitchShiftFactor;
        if (index <= HALF_FRAME_SIZE) {
            m_synthesisMagnitudes[index] += m_magnitudes[i];
            m_synthesisFrequencies[index] = m_frequencies[i] * m_pitchShiftFactor;
        }
    }

    // synthesize the signal
    for (int i = 0; i < HALF_FRAME_SIZE; i++) {
        // get new magnitude and true frequency from the synthesis array
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

        // compute new real and imaginary part and re-interleave
        m_floatComplexBuffer[i * 2] = magnitude * arm_cos_f32(temp);
        m_floatComplexBuffer[i * 2 + 1] = magnitude * arm_sin_f32(temp);
    }

    // zero negative frequencies
    std::memset(m_floatComplexBuffer + FRAME_SIZE, 0, sizeof(float32_t) * FRAME_SIZE);

    // do the iFFT
#if defined(KINETISK)
    arm_rfft_f32(&m_ifftInst, m_floatComplexBuffer, m_floatOutBuffer);
#elif defined(__IMXRT1062__)
    arm_rfft_fast_f32(&m_fftInst, m_floatComplexBuffer, m_floatOutBuffer, 1);
#endif

    // apply window function again (because we synthesized the signal from scratch)
    arm_mult_f32(m_floatOutBuffer, m_window, m_floatOutBuffer, FRAME_SIZE);

    // convert floats back to int
    arm_float_to_q15(m_floatOutBuffer, m_outputBuffer, FRAME_SIZE);

    // add overlap of the previous output to the new output
    arm_add_q15(m_outputBuffer, m_overlapBuffer, m_outputBuffer, FRAME_OVERLAP);

    // save the overlap for the next round
    std::memcpy(m_overlapBuffer, m_outputBuffer + HOP_SIZE, sizeof(int16_t) * FRAME_OVERLAP);    

#else
    release(input_block);
    release(output_block);
#endif
}
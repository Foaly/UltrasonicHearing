#include <Arduino.h>
#include "FFT.hpp"

#include "window.hpp"


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
    arm_rfft_f32(&m_fftInst, m_floatInBuffer, m_floatComplexBuffer);

    // compute magnitudes
    //Serial.println("Magnitues: ");
    for (int i=0; i < 2*FFT_LENGTH; i+=2) {
        float32_t real = m_floatComplexBuffer[i];
        float32_t imag = m_floatComplexBuffer[i + 1];
        float32_t result;

        arm_sqrt_f32(real * real + imag * imag, &result);

        //Serial.print(result);
        //Serial.print(", ");
    }
    //Serial.println();

    // do the ifft
    arm_rfft_f32(&m_ifftInst, m_floatComplexBuffer, m_floatOutBuffer);

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
#include <Arduino.h>
#include "FFT.hpp"


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
    switch (state) {
    case 0:
        // get the input block data
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            m_inputBuffer[i] = input_block->data[i];
        }

        // fill the output block
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            output_block->data[i] = m_outputBuffer[i];   // m_buffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
        }

        transmit(output_block, 0);
        release(input_block);
        release(output_block);

        state = 1;
        break;

    case 1:
        // get the input block data
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            m_inputBuffer[128 + i] = input_block->data[i];
        }

        // fill the output block
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            output_block->data[i] = m_outputBuffer[i + 128];   // m_buffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
        }

        transmit(output_block, 0);
        release(input_block);
        release(output_block);

        state = 2;
        break;

    case 2:
        // get the input block data
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            m_inputBuffer[256 + i] = input_block->data[i];
        }

        // fill the output block
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            output_block->data[i] = m_outputBuffer[i + 256];   // m_buffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
        }

        transmit(output_block, 0);
        release(input_block);
        release(output_block);
        
        state = 3;
        break;

    case 3:
        // get the input block data
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            m_inputBuffer[384 + i] = input_block->data[i];
        }

        // fill the output block
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            output_block->data[i] = m_outputBuffer[i + 384];   // m_buffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
        }

        transmit(output_block, 0);
        release(input_block);
        release(output_block);
        
        state = 0;

        // TODO: do the actual FFT processing

        // for now copy input to output
        for (int i = 0; i < 4 * AUDIO_BLOCK_SAMPLES; i++) {
            m_outputBuffer[i] = m_inputBuffer[i];
        }

        break;
    }

#else
    release(input_block);
    release(output_block);
#endif
}
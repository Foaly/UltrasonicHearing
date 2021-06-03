#ifndef FFT_HPP
#define FFT_HPP

#include "Arduino.h"
#include "AudioStream.h"

#include "arm_math.h"

class FFT : public AudioStream
{
public:
    FFT() : AudioStream(1, inputQueueArray)
    {
        // initialize buffers
        for(int i = 0; i < 512; i++) {
            m_inputBuffer[i] = 0;
            m_outputBuffer[i] = 0;
        }
    }

    virtual void update(void);
private:
    uint8_t state;
    audio_block_t *inputQueueArray[1];

    int16_t m_inputBuffer[512] __attribute__ ((aligned(4)));
    int16_t m_outputBuffer[512] __attribute__ ((aligned(4)));
};

#endif // FFT_HPP
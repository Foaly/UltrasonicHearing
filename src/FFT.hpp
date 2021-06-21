#ifndef FFT_HPP
#define FFT_HPP

#include "Arduino.h"
#include "AudioStream.h"

#include <arm_math.h>
#include <cstring>

class FFT : public AudioStream
{
public:
    FFT() : AudioStream(1, inputQueueArray),
            m_offset{0}
    {
        // initialize FFTs
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

        // initialize buffers
        std::memset(m_inputBuffer, 0, sizeof m_inputBuffer);
        std::memset(m_outputBuffer, 0, sizeof m_outputBuffer);
        std::memset(m_overlapBuffer, 0, sizeof m_overlapBuffer);
    }

    void update(void);

private:
    audio_block_t *inputQueueArray[1];
    static const uint16_t FRAME_SIZE = 2048; // FFT length, has to be one of 128, 512, 2048
    static const uint16_t OVERSAMPLING_FACTOR = 4; // has to be power of 2
    static const uint16_t HOP_SIZE = FRAME_SIZE / OVERSAMPLING_FACTOR;
    static const uint16_t FRAME_OVERLAP = FRAME_SIZE - HOP_SIZE;
    static const uint16_t HALF_FRAME_SIZE = FRAME_SIZE / 2;

    uint16_t m_offset;
    int16_t m_inputBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    int16_t m_outputBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatInBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatComplexBuffer[2 * FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatOutBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    int16_t m_overlapBuffer[FRAME_OVERLAP] __attribute__ ((aligned(4)));

    arm_rfft_instance_f32 m_fftInst;
    arm_cfft_radix4_instance_f32 m_fftComplexInst;
    arm_rfft_instance_f32 m_ifftInst;
    arm_cfft_radix4_instance_f32 m_ifftComplexInst;
};

#endif // FFT_HPP

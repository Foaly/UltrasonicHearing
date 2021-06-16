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
        arm_status status = arm_rfft_init_f32(&m_fftInst, &m_fftComplexInst, FFT_LENGTH, 0, 1);
        if (status == ARM_MATH_SUCCESS)
            Serial.println("FFT initilized!");
        else if (status == ARM_MATH_ARGUMENT_ERROR)
            Serial.println("FFT size not supported");
        
        status = arm_rfft_init_f32(&m_ifftInst, &m_ifftComplexInst, FFT_LENGTH, 1, 1);
        if (status == ARM_MATH_SUCCESS)
            Serial.println("iFFT initilized!");
        else if (status == ARM_MATH_ARGUMENT_ERROR)
            Serial.println("iFFT size not supported");

        // initialize buffers
        std::memset(m_inputBuffer, 0, sizeof m_inputBuffer);
        std::memset(m_outputBuffer, 0, sizeof m_outputBuffer);
        std::memset(m_addBuffer, 0, sizeof m_addBuffer);
    }

    void update(void);

private:
    audio_block_t *inputQueueArray[1];
    static const uint16_t FFT_LENGTH = 2048; // has to be one of 128, 512, 2048
    static const uint16_t HALF_FFT_LENGTH = FFT_LENGTH / 2;

    uint16_t m_offset;
    int16_t m_inputBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
    int16_t m_outputBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
    float32_t m_floatInBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
    float32_t m_floatComplexBuffer[2 * FFT_LENGTH] __attribute__ ((aligned(4)));
    float32_t m_floatOutBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
    int16_t m_addBuffer[HALF_FFT_LENGTH] __attribute__ ((aligned(4)));

    arm_rfft_instance_f32 m_fftInst;
    arm_cfft_radix4_instance_f32 m_fftComplexInst;
    arm_rfft_instance_f32 m_ifftInst;
    arm_cfft_radix4_instance_f32 m_ifftComplexInst;
};

#endif // FFT_HPP

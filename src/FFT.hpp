#ifndef FFT_HPP
#define FFT_HPP

#include "Arduino.h"
#include "AudioStream.h"

#include <arm_math.h>

class FFT : public AudioStream
{
public:
    FFT() : AudioStream(1, inputQueueArray)
    {
		arm_status status = arm_rfft_init_f32(&m_fftInst, &m_fftComplexInst, FFT_LENGTH, 0, 1);
		if (status == ARM_MATH_SUCCESS)
			Serial.println("FFT initilized!");
		else if (status == ARM_MATH_ARGUMENT_ERROR)
			Serial.println("FFT size not supported");

        // initialize buffers
        for(int i = 0; i < FFT_LENGTH; i++) {
            m_inputBuffer[i] = 0;
            m_outputBuffer[i] = 0;
        }
    }

    virtual void update(void);

private:
    uint8_t state;
    audio_block_t *inputQueueArray[1];
	static const uint16_t FFT_LENGTH = 512; // has to be power of 2

    int16_t m_inputBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
    int16_t m_outputBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
	float32_t m_floatInBuffer[FFT_LENGTH] __attribute__ ((aligned(4)));
	float32_t m_floatOutBuffer[2 * FFT_LENGTH] __attribute__ ((aligned(4)));


	arm_rfft_instance_f32 m_fftInst;
	arm_cfft_radix4_instance_f32 m_fftComplexInst;
};

#endif // FFT_HPP

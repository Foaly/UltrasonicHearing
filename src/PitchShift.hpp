#ifndef PITCHSHIFT_HPP
#define PITCHSHIFT_HPP

#include "Arduino.h"
#include "AudioStream.h"

#include <arm_math.h>
#include <cstring>

class PitchShift : public AudioStream
{
public:
    PitchShift(uint32_t sampleRate, float32_t pitchShiftFactor) :
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

    void update(void);
    // A high pass filter that is applied during the processing.
    // The cutoff is specified in Hertz in the range before the pitchshifting happens.
    void setHighPassCutoff(float cutoff);

private:
    void generateWindow();

    audio_block_t *inputQueueArray[1];
    static const uint16_t FRAME_SIZE = 1024; // FFT length, Teensy 3.x supports only 128, 512, 2048, but Teensy 4.x supports 32, 64, 128, 256, 512, 1024, 2048, 4096
    static const uint16_t OVERSAMPLING_FACTOR = 4; // has to be power of 2
    static const uint16_t HOP_SIZE = FRAME_SIZE / OVERSAMPLING_FACTOR;
    static const uint16_t FRAME_OVERLAP = FRAME_SIZE - HOP_SIZE;
    static const uint16_t HALF_FRAME_SIZE = FRAME_SIZE / 2;

    const float32_t m_omega = 2.0 * M_PI * static_cast<float32_t>(HOP_SIZE) / FRAME_SIZE;  // omega is the nominal (expected) phase increment for each FFT bin at the given analysis hop size
    const float32_t m_binFrequencyWidth;
    const float32_t m_pitchShiftFactor;
    float32_t m_highPassCutoff;

    uint16_t m_offset;
    int16_t m_inputBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    int16_t m_outputBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatInBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_window[FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_previousPhases[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_magnitudes[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_frequencies[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_synthesisMagnitudes[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_synthesisFrequencies[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_phaseSum[HALF_FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatComplexBuffer[2 * FRAME_SIZE] __attribute__ ((aligned(4)));
    float32_t m_floatOutBuffer[FRAME_SIZE] __attribute__ ((aligned(4)));
    int16_t m_overlapBuffer[FRAME_OVERLAP] __attribute__ ((aligned(4)));

#if defined(KINETISK)  // Teensy 3.x
    arm_rfft_instance_f32 m_fftInst;
    arm_cfft_radix4_instance_f32 m_fftComplexInst;
    arm_rfft_instance_f32 m_ifftInst;
    arm_cfft_radix4_instance_f32 m_ifftComplexInst;
#elif defined(__IMXRT1062__)  // Teensy 4.x
    arm_rfft_fast_instance_f32 m_fftInst;
#endif
};

#endif // PITCHSHIFT_HPP

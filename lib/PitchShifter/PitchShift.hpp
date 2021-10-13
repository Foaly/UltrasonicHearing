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


#ifndef PITCHSHIFT_HPP
#define PITCHSHIFT_HPP

#include <Arduino.h>
#include <AudioStream.h>

#include <arm_math.h>
#include <cstring>

class PitchShift : public AudioStream
{
public:
    PitchShift(uint32_t sampleRate, float32_t pitchShiftFactor);

    // processing loop called by the audio engine
    void update(void);

    // A high pass filter that is applied during the processing.
    // The cutoff is specified in Hertz in the range before the pitchshifting happens.
    void setHighPassCutoff(float cutoff);

private:
    void generateWindow();

    audio_block_t* inputQueueArray[1];
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

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


#include <unity.h>

#include <PitchShift.hpp>

#include <cmath>

uint16_t failedTestCount = 0;

const int sampleRate = 44100;
//const int sampleRate = 234000;
const uint16_t frameSize = 1024;  // has to be power of 2
const int16_t semitones = -12;  // shift in semitones
const float32_t pitchShiftFactor = std::pow(2., semitones / 12.);

const float amplitude = 32767 * 0.2f;
const float frequency = 440.f;
const float phase = 0.f;
const float deltaTime = 1.f / sampleRate;
float time = 0.f;

// this function generates the simulated input to the audio engine
int sineGenerator() {
    // simple sine wave
    float value = amplitude * std::sin(2.f * M_PI * frequency * time + phase);
    time += deltaTime;
    return static_cast<int>(value);
}


void test_pitchShifter(void) {
    PitchShift<frameSize> pitchShifter(sampleRate, pitchShiftFactor);
    pitchShifter.setInputGenerator(&sineGenerator);

    const uint16_t frameCount = 4 * frameSize / AUDIO_BLOCK_SAMPLES;

    for(uint16_t i = 0; i < frameCount; i++)
        pitchShifter.update();

    TEST_ASSERT_EQUAL(42, 42);
}


void setup() {
    // Wait for >2 secs if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_pitchShifter);
    failedTestCount = UNITY_END();
}


void loop() {
    if(failedTestCount > 0) {
        // at least one test failed, blink fast
        digitalWrite(13, HIGH);
        delay(100);
        digitalWrite(13, LOW);
        delay(100);
    }
    else {
        // all tests passed, blink slow
        digitalWrite(13, HIGH);
        delay(800);
        digitalWrite(13, LOW);
        delay(800);
    }
}

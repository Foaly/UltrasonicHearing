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


const int sampleRate = 192000;
const int16_t semitones = -36;  // shift in semitones
uint16_t failedTestCount = 0;


void test_pitchShifter(void) {
    PitchShift pitchShifter(sampleRate, semitones);

    pitchShifter.update();
    pitchShifter.update();
    pitchShifter.update();
    pitchShifter.update();

    TEST_ASSERT_EQUAL(42, 42);
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
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

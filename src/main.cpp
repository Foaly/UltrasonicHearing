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

//************************************************************
// Serial - set to true for debug output, false for no output
//************************************************************
// -->  Use DEBUG_SERIAL.println for debugging
#define DEBUGGING true  
#define DEBUG_SERIAL \
  if (DEBUGGING) Serial

const int BAUD = 9600;

#include <Arduino.h>
#include <elapsedMillis.h>
#include <cmath>

#include <Audio.h>
#include <Wire.h>

#include <PitchShift.hpp>
#include <DebugBlocks.hpp>
#include <WavFileWriter.hpp>
#include <utils.hpp>

// --> CONSTANTS in BIG_LETTERS
const int MIC_INPUT = AUDIO_INPUT_MIC;
//const int SAMPLE_RATE = 44100;
//const int SAMPLE_RATE = 96000;
const int SAMPLE_RATE = 192000;
//const int SAMPLE_RATE = 234000;

const int16_t SEMITONES = -36;  // shift in SEMITONES
const float32_t PITCH_SHIFT_FACTOR = std::pow(2., SEMITONES / 12.);

elapsedMillis performanceStatsClock;


AudioControlSGTL5000     audioShield;
AudioInputI2S            audioInput;
PitchShift               fft(SAMPLE_RATE, PITCH_SHIFT_FACTOR);
Counter                  counter;
Printer                  printer;
AudioOutputI2S           audioOutput;
AudioSynthWaveformSine   sine;
AudioRecordQueue         queue;
WavFileWriter            wavWriter(queue);

//AudioConnection    patchCord1(audioInput, 0, audioOutput, 0);
//AudioConnection    patchCord2(audioInput, 1, audioOutput, 1);

//AudioConnection    patchCord(counter, 0, fft, 0);
//AudioConnection    patchCord1(fft, 0, printer, 0);

//AudioConnection      sineToFFT(sine, 0, fft, 0);
AudioConnection      micToFFT(audioInput, 0, fft, 0);
AudioConnection      fftToOut(fft, 0, audioOutput, 0);
AudioConnection      fftToOut2(fft, 0, audioOutput, 1);
//AudioConnection      micToWAV(audioInput, 0, queue, 0);
AudioConnection      fftToWav(fft, 0, queue, 0);
//AudioConnection      fftToPrinter(fft, 0, printer, 0);


void printPerformanceData();


void setup() {
    delay(1000);
    
    Serial.begin(BAUD);
    // --> No magic numbers
    AudioMemory(200);
    audioShield.enable();
    audioShield.inputSelect(MIC_INPUT);
    // --> No magic numbers
    audioShield.micGain(45);  //0-63
    // --> No magic numbers
    audioShield.volume(0.6);  //0-1

    setI2SFreq(SAMPLE_RATE);
    Serial.print("Running at SAMPLE_RATE: ");
    Serial.println(SAMPLE_RATE);
    // --> No magic numbers
    fft.setHighPassCutoff(22000.f);
    // --> No magic numbers
    sine.frequency(200 * (AUDIO_SAMPLE_RATE_EXACT / SAMPLE_RATE));

    Serial.println("Done initializing! Starting now!");
}


void loop() {
    // --> No magic numbers
    if(performanceStatsClock > 500) {
        printPerformanceData();
        performanceStatsClock = 0;
    }

    if (Serial.available() > 0) {
        // read the incoming byte
        byte incomingByte = Serial.read();

        if ( incomingByte == 'r' ) {
            if (!wavWriter.isWriting()) {
                Serial.println("Recording started!");
                wavWriter.open("Ultra.wav", SAMPLE_RATE, 1);
            }
            else {
                Serial.println("Recording stopped!");
                wavWriter.close();
            }
        }
    }

    if (wavWriter.isWriting())
        wavWriter.update();
}


void printPerformanceData() {
    Serial.print("CPU: ");
    Serial.print("fft=");
    Serial.print(fft.processorUsage());
    Serial.print(",");
    Serial.print(fft.processorUsageMax());
    Serial.print("  ");
    Serial.print("all=");
    Serial.print(AudioProcessorUsage());
    Serial.print(",");
    Serial.print(AudioProcessorUsageMax()); // estimate of the maximum percentage of CPU time any audio update has ever used (0 to 100)
    Serial.print("    ");
    Serial.print("Audio Memory: ");
    Serial.print(AudioMemoryUsage()); // number of audio blocks currently in use
    Serial.print(",");
    Serial.print(AudioMemoryUsageMax()); // maximum number of audio blocks ever in use
    Serial.println();
}

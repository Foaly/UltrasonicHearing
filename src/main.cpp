#include <Arduino.h>
#include <elapsedMillis.h>
#include <cmath>

#include <Audio.h>
#include <Wire.h>

#include <PitchShift.hpp>
#include <DebugBlocks.hpp>
#include <WavFileWriter.hpp>
#include <utils.hpp>


const int micInput = AUDIO_INPUT_MIC;
//const int sampleRate = 44100;
//const int sampleRate = 96000;
const int sampleRate = 192000;

const int16_t semitones = -36;  // shift in semitones
const float32_t pitchShiftFactor = std::pow(2., semitones / 12.);

elapsedMillis performanceStatsClock;


AudioControlSGTL5000     audioShield;
AudioInputI2S            audioInput;
PitchShift               fft(sampleRate, pitchShiftFactor);
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
    
    Serial.begin(9600);
    AudioMemory(200);
    audioShield.enable();
    audioShield.inputSelect(micInput);
    audioShield.micGain(45);  //0-63
    audioShield.volume(0.6);  //0-1

    setI2SFreq(sampleRate);
    Serial.print("Running at samplerate: ");
    Serial.println(sampleRate);

    sine.frequency(200 * (AUDIO_SAMPLE_RATE_EXACT / sampleRate));

    Serial.println("Done initializing! Starting now!");
}


void loop() {
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
                wavWriter.open("Ultra.wav", sampleRate, 1);
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
    Serial.print(AudioProcessorUsageMax());
    Serial.print("    ");
    Serial.print("Audio Memory: ");
    Serial.print(AudioMemoryUsage()); // number of audio blocks currently in use
    Serial.print(",");
    Serial.print(AudioMemoryUsageMax()); // maximum number of audio blocks ever in use
    Serial.println();
}

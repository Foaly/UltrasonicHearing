#include <Arduino.h>

#include <Audio.h>
#include <Wire.h>

#include <FFT.hpp>
#include <DebugBlocks.hpp>


const int micInput = AUDIO_INPUT_MIC;

AudioControlSGTL5000     audioShield;
AudioInputI2S            audioInput;
FFT                      fft;
Counter                  counter;
Printer                  printer;
//AudioOutputI2S           audioOutput;
AudioSynthWaveformSine   sine;

//AudioConnection    patchCord(audioInput, 0, fft, 0);
//AudioConnection    patchCord1(audioInput, 0, audioOutput, 0);
//AudioConnection    patchCord2(audioInput, 1, audioOutput, 1);

//AudioConnection    patchCord(counter, 0, fft, 0);
//AudioConnection    patchCord1(fft, 0, printer, 0);

AudioConnection      sineToFFT(sine, 0, fft, 0);


void printPerformanceData();


void setup() {
    delay(1000);
    
    Serial.begin(9600);
    AudioMemory(200);
    audioShield.enable();
    audioShield.inputSelect(micInput);
    audioShield.micGain(35);  //0-63
    audioShield.volume(0.8);  //0-1

    sine.frequency(10000);

    Serial.println("Done initializing! Starting now!");
}


void loop() {
    printPerformanceData();
    delay(500);
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

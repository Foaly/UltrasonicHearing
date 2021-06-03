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

//AudioConnection    patchCord(audioInput, 0, fft, 0);
//AudioConnection    patchCord1(audioInput, 0, audioOutput, 0);
//AudioConnection    patchCord2(audioInput, 1, audioOutput, 1);

AudioConnection    patchCord(counter, 0, fft, 0);
AudioConnection    patchCord1(fft, 0, printer, 0);

void setup() {
    delay(1000);
    
    Serial.begin(9600);
    AudioMemory(200);
    audioShield.enable();
    audioShield.inputSelect(micInput);
    audioShield.micGain(35);  //0-63
    audioShield.volume(0.8);  //0-1

    Serial.println("Done initilizing! Starting now!");
}

void loop() {
}
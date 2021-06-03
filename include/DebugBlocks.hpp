#ifndef DEBUGBLOCKS_HPP
#define DEBUGBLOCKS_HPP

#include "Arduino.h"
#include "AudioStream.h"


class Counter : public AudioStream
{
public:
    Counter() : AudioStream(0, NULL),
    m_counter{0}
    {}
    
    void update(void){
        // allocate output block
        audio_block_t *output_block;
        output_block = allocate();
        if (!output_block) return;

        // fill buffer
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            output_block->data[i] = m_counter;
        }
        m_counter++;

        transmit(output_block, 0);
        release(output_block);
    }
private:
    uint16_t m_counter;
};


class Printer : public AudioStream
{
public:
    Printer() : AudioStream(1, inputQueueArray)
    {}
    
    void update(void){
        // get input block
        audio_block_t *input_block;
        input_block = receiveReadOnly();
        if (!input_block) return;

        Serial.println(input_block->data[0]);
        release(input_block);
    }
private:
    audio_block_t *inputQueueArray[1];
};

#endif // DEBUGBLOCKS_HPP
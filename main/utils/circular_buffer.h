#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint8_t* buffer;
    size_t memberSize;
    size_t count;
    int readPos;
    int writePos;
} circularBuffer_t;

#define platform_midi_buffer_empty(buf) (buf->read_pos == buf->write_pos)

void circularBufferInit(circularBuffer_t* buffer, size_t memberSize, size_t capacity);
void circularBufferDeinit(circularBuffer_t* buffer);
size_t circularBufferAvailable(const circularBuffer_t* buffer);
size_t circularBufferCapacity(const circularBuffer_t* buffer);
void circularBufferWrite(circularBuffer_t* buffer, void* src, size_t count);
size_t circularBufferRead(circularBuffer_t* buffer, void* dest, size_t maxCount);

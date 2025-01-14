#include "circular_buffer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void circularBufferInit(circularBuffer_t* buffer, size_t memberSize, size_t capacity)
{
    buffer->buffer     = malloc(memberSize * capacity);
    buffer->memberSize = memberSize;
    buffer->count      = capacity;
    buffer->readPos    = 0;
    buffer->writePos   = 0;
}

void circularBufferDeinit(circularBuffer_t* buffer)
{
    uint8_t* tmp   = buffer->buffer;
    buffer->count  = 0;
    buffer->buffer = NULL;
    free(tmp);
}

size_t circularBufferAvailable(const circularBuffer_t* buffer)
{
    if (!buffer || !buffer->buffer)
    {
        return 0;
    }

    if (buffer->readPos <= buffer->writePos)
    {
        return (buffer->writePos - buffer->readPos) / buffer->memberSize;
    }
    else
    {
        return (buffer->count - (buffer->readPos / buffer->memberSize)) + (buffer->writePos / buffer->memberSize);
    }
}

size_t circularBufferCapacity(const circularBuffer_t* buffer)
{
    if (!buffer || !buffer->buffer)
    {
        return 0;
    }
    return buffer->count - circularBufferAvailable(buffer);
}

void circularBufferWrite(circularBuffer_t* buffer, void* src, size_t count)
{
    bool wasFlipped = (buffer->writePos < buffer->readPos);
    if (buffer->writePos + (count * buffer->memberSize) >= buffer->count * buffer->memberSize)
    {
        // Write up to the end first
        memcpy(buffer->buffer + buffer->writePos, src, (buffer->count * buffer->memberSize) - buffer->writePos);
        // Then write to the start
        memcpy(buffer->buffer, ((char*)src) + ((buffer->count * buffer->memberSize) - buffer->writePos),
               count - (buffer->count * buffer->memberSize - buffer->writePos));
    }
    else
    {
        memcpy(buffer->buffer + buffer->writePos, src, count * buffer->memberSize);
    }

    buffer->writePos = (buffer->writePos + count * buffer->memberSize) % (buffer->count * buffer->memberSize);

    // Detect overwriting unread data and fix the readPos
    if (wasFlipped != (buffer->writePos < buffer->readPos))
    {
        buffer->readPos = (buffer->writePos + 1) % (buffer->count * buffer->memberSize);
    }
}

size_t circularBufferRead(circularBuffer_t* buffer, void* dest, size_t maxCount)
{
    size_t count = circularBufferAvailable(buffer);
    if (count > maxCount)
    {
        count = maxCount;
    }

    if (buffer->readPos + (count * buffer->memberSize) >= buffer->count * buffer->memberSize)
    {
        // Read up to the end first
        memcpy(dest, buffer->buffer + buffer->readPos, (buffer->count * buffer->memberSize) - buffer->readPos);
        // Then read from the start
        memcpy(((char*)dest) + (buffer->count * buffer->memberSize) - buffer->readPos, buffer->buffer,
               count - (buffer->count * buffer->memberSize - buffer->readPos));
    }
    else
    {
        // All at once
        memcpy(dest, buffer->buffer + buffer->readPos, count * buffer->memberSize);
    }

    buffer->readPos = (buffer->readPos + count * buffer->memberSize) % (buffer->count * buffer->memberSize);

    return count;
}

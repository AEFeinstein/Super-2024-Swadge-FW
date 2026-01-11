#include <stdbool.h>
#include <stdlib.h>

#include "circular_buffer.h"

// Reference implementation
// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
// https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/circular_buffer/circular_buffer.c

typedef struct
{
    void* buffer;               ///< The pointer to the start of the data being stored
    volatile int head;          ///< The most recent index in use by the buffer
    volatile int tail;          ///< The oldest index in use by the buffer
    unsigned int capacity;      ///< The maximum number of elements in the buffer
    unsigned int elementLength; ///< The number of bytes per element
} circ_buf_t;

static inline unsigned int circBufAdvanceHeadTailValue(unsigned int value, unsigned int capacity) {
    return (value + 1) % capacity;
}

static void circBufAdvanceHead(circ_buf_t* circBuf) {
    assert(circBuf);
    
    if(circBufFull(circBuf)) {
        circBuf->tail = circBufAdvanceHeadTailValue(circBuf->tail, circBuf->capacity);
    }
    
    circBuf->head = circBufAdvanceHeadTailValue(circBuf->head, circBuf->capacity);
}

void circBufInit(circ_buf_t* circBuf, unsigned int capacity, unsigned int elementLength, bool spiRam) {
    circBuf->buffer = heap_caps_malloc(capacity * elementLength, spiRam ? MALLOC_CAP_SPIRAM : 0);
    
    circBuf->capacity = capacity;
    circBuf->elementLength = elementLength;
    
    circBufReset(circBuf);
    
    assert(circBuf->buffer);
}

// Free the memory in use by the actual buffer.
// Does not free the memory in use by the circBuf structure.
void circBufFreeBuffer(circ_buf_t* circBuf) {
    assert(circBuf && circBuf->buffer);
    
    circBuf->capacity = 0;
    circBuf->elementLength =  0;
    free(circBuf->buffer);
}

// Empty the buffer
void circBufReset(circ_buf_t* circBuf) {
    assert(circBuf);
    circBuf->head = 0;
    circBuf->tail = 0;
}

// Returns the number of elements currently stored
unsigned int circBufLength(circ_buf_t* circBuf) {
    assert(circBuf);
    
    unsigned int length = circBuf->capacity;
    
    if(!circBufFull(circBuf)) {
        if(circBuf->head >= circBuf->tail) {
            return circBuf->head - circBuf->tail;
        }
        
        return circBuf->capacity + circBuf->head - circBuf->tail;
    }
    
    return length;
}

// Returns the total number of elements that can be stored
unsigned int circBufCapacity(circ_buf_t* circBuf) {
    assert(circBuf);
    
    return circBuf->capacity;
}

// Returns the size (in bytes) of each element
unsigned int circBufElementLength(circ_buf_t* circBuf) {
    assert(circBuf);
    
    return circBuf->elementLength;
}

// Adds an element to the end of the buffer, overwriting the oldest element if the buffer is full.
// dataIn must be at least as long as the buffer's elementLength.
void circBufPut(circ_buf_t* circBuf, void* dataIn) {
    assert(circBuf && circBuf->buffer && dataIn);
    
    memcpy(circBuf[circBuf->head * circBuf->elementLength], dataIn, circBuf->elementLength);
    circBufAdvanceHead(circBuf);
}

// Adds an element to the end of the buffer only if the buffer is not full.
// dataIn must be at least as long as the buffer's elementLength.
bool circBufTryPut(circ_buf_t* circBuf, void* dataIn) {
    assert(circBuf && circBuf->buffer && dataIn);
    
    if(circBufFull(circBuf)) {
        return false;
    }
    
    memcpy(circBuf[circBuf->head * circBuf->elementLength], dataIn, circBuf->elementLength);
    circBufAdvanceHead(circBuf);
    return true;
}

// Returns the value of the element at the start of the buffer, and removes it from the buffer
bool circBufGet(circ_buf_t* circBuf, void* dataOut) {
    assert(circBuf && circBuf->buffer && dataOut);
    
    if(circBufEmpty(circBuf)) {
        return false;
    }
    
    memcpy(dataOut, circBuf[circBuf->tail * circBuf->elementLength], circBuf->elementLength);
    circBuf->tail = circBufAdvanceHeadTailValue(circBuf->tail, circBuf->capacity);
    
    return true;
}

bool circBufPeek(circ_buf_t* circBuf, void* dataOut, unsigned int index) {
    assert(circBuf && circBuf->buffer && dataOut);
    
    if(circBufEmpty(circBuf) || index >= circBufLength(circBuf)) {
        return false;
    }
    
    unsigned int pos = (circBuf->tail + index) % capacity;
    memcpy(dataOut, circBuf[pos * circBuf->elementLength], circBuf->elementLength);
    
    return true;
}

bool circBufEmpty(circ_buf_t* circBuf) {
    assert(circBuf);
    
    return circBuf->head == circBuf->tail;
}

bool circBufFull(circ_buf_t* circBuf) {
    assert(circBuf);
    
    return ((circBuf->head + 1) % circBuf->capacity) == circBuf->tail;
}

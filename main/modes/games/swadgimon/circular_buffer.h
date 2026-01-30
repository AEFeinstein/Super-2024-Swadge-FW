#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include <stdbool.h>

typedef struct
{
    void* buffer;               ///< The pointer to the start of the data being stored
    volatile int head;          ///< The most recent index in use by the buffer
    volatile int tail;          ///< The oldest index in use by the buffer
    unsigned int capacity;      ///< The maximum number of elements in the buffer
    unsigned int elementLength; ///< The number of bytes per element
} circ_buf_t;

void circBufInit(circ_buf_t* circBuf, unsigned int capacity, unsigned int elementLength, bool spiRam);
void circBufFreeBuffer(circ_buf_t* circBuf);
void circBufReset(circ_buf_t* circBuf);
unsigned int circBufLength(const circ_buf_t* circBuf);
unsigned int circBufCapacity(const circ_buf_t* circBuf);
unsigned int circBufElementLength(const circ_buf_t* circBuf);
void circBufPut(circ_buf_t* circBuf, const void* data);
bool circBufTryPut(circ_buf_t* circBuf, const void* data);
bool circBufGet(circ_buf_t* circBuf, void* dataOut);
bool circBufPeek(const circ_buf_t* circBuf, void* dataOut, unsigned int index);
bool circBufEmpty(const circ_buf_t* circBuf);
bool circBufFull(const circ_buf_t* circBuf);

#endif

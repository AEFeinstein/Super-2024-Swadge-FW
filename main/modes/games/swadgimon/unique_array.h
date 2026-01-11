/*
 * Like a set, but with an order to its elements.
 * Specifically designed for small arrays of bytes
 */

#ifndef _UNIQUE_ARRAY_H_
#define _UNIQUE_ARRAY_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t* buffer;                   ///< The pointer to the start of the data being stored
    volatile unsigned int lengthInUse; ///< The most recent index in use by the array
    unsigned int capacity;             ///< The number of elements in the buffer
} uniq_arr_t;

void uniqArrInit(uniq_arr_t* uniqArr, unsigned int capacity, bool spiRam);
void uniqArrFreeBuffer(uniq_arr_t* uniqArr);
void uniqArrReset(uniq_arr_t* uniqArr);
unsigned int uniqArrLength(uniq_arr_t* uniqArr);
unsigned int uniqArrCapacity(uniq_arr_t* uniqArr);
bool uniqArrPut(uniq_arr_t* uniqArr, uint8_t data);
bool uniqArrGet(uniq_arr_t* uniqArr, uint8_t* dataOut, unsigned int idx);
bool uniqArrSearch(uniq_arr_t* uniqArr, unsigned int* idxOut, uint8_t data);
bool uniqArrRemoveIdx(uniq_arr_t* uniqArr, unsigned int idx);
bool uniqArrRemove(uniq_arr_t* uniqArr, uint8_t data);
bool uniqArrIntersection(uniq_arr_t* uniqArr1Dest, uniq_arr_t* uniqArr2);
bool uniqArrUnion(uniq_arr_t* uniqArr1Dest, uniq_arr_t* uniqArr2);
bool uniqArrDifference(uniq_arr_t* uniqArr1Dest, uniq_arr_t^ uniqArr2);
void uniqArrCopy(uniq_arr_t* dest, uniq_arr_t* src, bool spiRam);
bool uniqArrEmpty(uniq_arr_t* uniqArr);
bool uniqArrFull(uniq_arr_t* uniqArr);

#endif

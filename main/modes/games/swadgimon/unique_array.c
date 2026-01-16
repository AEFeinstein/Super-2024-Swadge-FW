/*
 * Like a set, but with an order to its elements.
 * Specifically designed for small arrays of bytes
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"

#include "unique_array.h"

// Runs in O(1)
static uint8_t* uniqArrInitNewBuffer(unsigned int capacity, bool spiRam) {
    uint8_t* buffer = heap_caps_malloc(capacity, spiRam ? MALLOC_CAP_SPIRAM : 0);
    assert(buffer);
    return buffer;
}

// Runs in O(1)
void uniqArrInit(uniq_arr_t* uniqArr, unsigned int capacity, bool spiRam) {
    assert(uniqArr);
    
    uniqArr->buffer = uniqArrInitNewBuffer(capacity, spiRam);
    uniqArr->capacity = capacity;
    uniqArr->lengthInUse = 0;
    uniqArr->spiRam = spiRam;
}

// Free the memory in use by the actual buffer.
// Does not free the memory in use by the uniqArr structure.
// Runs in O(1)
void uniqArrFreeBuffer(uniq_arr_t* uniqArr) {
    assert(uniqArr);
    
    if(uniqArr->buffer != NULL) {
        uniqArr->capacity = 0;
        free(uniqArr->buffer);
        uniqArr->buffer = NULL;
    }
}

// Runs in O(1)
void uniqArrReset(uniq_arr_t* uniqArr) {
    uniqArr->lengthInUse = 0;
}

// Returns the number of elements currently stored
// Runs in O(1)
unsigned int uniqArrLength(uniq_arr_t* uniqArr) {
    assert(uniqArr);
    
    return uniqArr->lengthInUse;
}

// Returns the total number of elements that can be stored
// Runs in O(1)
unsigned int uniqArrCapacity(uniq_arr_t* uniqArr) {
    assert(uniqArr);
    
    return uniqArr->capacity;
}

// Adds an element to the end of the buffer, overwriting the oldest element if the buffer is full.
// dataIn must be at least as long as the buffer's elementLength.
// Runs in O(n)
bool uniqArrPut(uniq_arr_t* uniqArr, uint8_t data) {
    assert(uniqArr && uniqArr->buffer);
    
    uniqArrRemove(uniqArr, data);
    
    // Add the element
    uniqArr->buffer[uniqArr->lengthInUse] = data;
    uniqArr->lengthInUse++;
    return true;
}

// Returns true if the index is within used bounds of the array, false otherwise.
// Sets dataOut to the value of the element at the given index, if it exists. Otherwise, does not change dataOut.
// Runs in O(1)
bool uniqArrGet(uniq_arr_t* uniqArr, uint8_t* dataOut, unsigned int idx) {
    assert(uniqArr && uniqArr->buffer && dataOut);
    
    if(idx >= uniqArr->lengthInUse) {
        return false;
    }
    
    *dataOut = uniqArr->buffer[idx];
    return true;
}

// Returns true if the given data exists in the array, false otherwise.
// Sets idxOut to the index of the data, if it exists. Otherwise, does not change idxOut.
// Runs in O(n)
bool uniqArrSearch(uniq_arr_t* uniqArr, unsigned int* idxOut, uint8_t data) {
    assert(uniqArr && uniqArr->buffer);
    
    // Search the buffer for the given data
    for(unsigned int i = 0; i < uniqArr->lengthInUse; i++) {
        if(uniqArr->buffer[i] == data) {
            // Only set idxOut if the pointer is not NULL
            if(idxOut != NULL) {
                *idxOut = i;
            }
            // Found a match
            return true;
        }
    }
    
    // No match
    return false;
}

// Runs in O(n)
bool uniqArrRemoveIdx(uniq_arr_t* uniqArr, unsigned int idx) {
    assert(uniqArr && uniqArr->buffer);
    
    // Cannot remove items past the end of the array
    if(idx >= uniqArr->lengthInUse) {
        return false;
    }
    
    // If we're removing the last element, there's no need to do this copy
    if(idx < uniqArr->lengthInUse - 1) {
        // Copy the array in-place, skipping the index to be removed
        for(int i = idx; i < uniqArr->lengthInUse - 1; i++) {
            uniqArr->buffer[i] = uniqArr->buffer[i + 1];
        }
    }
    
    uniqArr->lengthInUse--;
    return true;
}

// Runs in O(n)
bool uniqArrRemove(uniq_arr_t* uniqArr, uint8_t data) {
    // No need to do an assert here, since we don't use the pointers directly, just pass them to other functions with their own asserts
    
    unsigned int existingIdx = 0;
    
    if(!uniqArrSearch(uniqArr, &existingIdx, data)) {
        return false;
    }
    
    return uniqArrRemoveIdx(uniqArr, existingIdx);
}

// Runs in O(n^2)
bool uniqArrIntersection(uniq_arr_t* uniqArr1Dest, uniq_arr_t* uniqArr2) {
    assert(uniqArr1Dest && uniqArr2);
    
    for(int i = 0; i < uniqArr1Dest->lengthInUse; i++) {
        uint8_t data;
        
        if(!uniqArrGet(uniqArr1Dest, &data, i)) {
            return false;
        }
        
        if(!uniqArrSearch(uniqArr2, NULL, data)) {
            uniqArrRemoveIdx(uniqArr1Dest, i);
            i--;
        }
    }
    
    return true;
}

// Runs in O(n^2)
bool uniqArrUnion(uniq_arr_t* uniqArr1Dest, uniq_arr_t* uniqArr2) {
    assert(uniqArr1Dest && uniqArr2);
    
    for(int i = 0; i < uniqArr2->lengthInUse; i++) {
        uint8_t data;
        
        if(!uniqArrGet(uniqArr2, &data, i)) {
            return false;
        }
        
        uniqArrPut(uniqArr1Dest, data); //TODO: preserve arr1Dest's ordering
    }
    
    return true;
}

// Runs in O(n^2)
bool uniqArrDifference(uniq_arr_t* uniqArr1Dest, uniq_arr_t* uniqArr2) {
    assert(uniqArr1Dest && uniqArr2);
    
    for(int i = 0; i < uniqArr2->lengthInUse; i++) {
        uint8_t data;
        if(!uniqArrGet(uniqArr2, &data, i)) {
            return false;
        }
        
        uniqArrRemove(uniqArr1Dest, data);
    }
    
    return true;
}

// Runs in O(n)
// The dest capacity must first be set to 0, or uniqArrInit must first be called on the dest array
// Will reallocate the dest array's buffer if it's too small or if it's in the opposite RAM type from the given spiRam bool
void uniqArrCopy(uniq_arr_t* dest, uniq_arr_t* src, bool spiRam) {
    assert(dest && src);
    
    if(dest->capacity < src->lengthInUse || dest->spiRam != spiRam) {
        uniqArrFreeBuffer(dest);
        uniqArrInit(dest, src->capacity, spiRam);
    }
    
    assert(dest->buffer);
    
    memcpy(dest->buffer, src->buffer, src->lengthInUse);
    dest->lengthInUse = src->lengthInUse;
}

// Runs in O(1)
bool uniqArrEmpty(uniq_arr_t* uniqArr) {
    assert(uniqArr);
    
    return uniqArr->lengthInUse == 0;
}

// Runs in O(1)
bool uniqArrFull(uniq_arr_t* uniqArr) {
    assert(uniqArr);
    
    return uniqArr->lengthInUse == uniqArr->capacity;
}

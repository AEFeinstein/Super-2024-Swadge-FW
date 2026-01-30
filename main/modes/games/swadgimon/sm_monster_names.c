#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "macros.h"

#include "sm_constants.h"
#include "sm_monster_names.h"
#include "sm_names.h"



static char* monsterNamesGetCustom(names_header_t* monsterNamesWithHeader, uint16_t idx, uint8_t nameLength, uint16_t numNames);



// Initialize the monster names structure
names_header_t* monsterNamesInit(void) {
    return initNames(MAX_MONSTER_NAME_LEN, 0);
}

// Runs in O(1)
uint8_t monsterNamesNameLength(const names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    return monsterNamesWithHeader->nameLength;
}

// Runs in O(1)
uint16_t monsterNamesCapacity(const names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    return monsterNamesWithHeader->numNames;
}

// Return a pointer to the name at the given index, with the given name length and number of names.
// Bounds are only checked between parameters, not with the header!
// Runs in O(1)
// TODO: get rid of the empty name at index 0 behind the scenes, pretend index 0 is always empty, and shift everything over by 1. So monsterNamesGet(..., 4) would return the name at offset 3 in actual storage
static char* monsterNamesGetCustom(names_header_t* monsterNamesWithHeader, uint16_t idx, uint8_t nameLength, uint16_t numNames) {
    assert(monsterNamesWithHeader);
    
    if(idx >= numNames) {
        return NULL;
    }
    
    return &((char*) &monsterNamesWithHeader[1])[idx * nameLength];
}

// Return a pointer to the name at the given index, or NULL if the index is past the bounds of the array
// Runs in O(1)
char* monsterNamesGet(names_header_t* monsterNamesWithHeader, uint16_t idx) {
    assert(monsterNamesWithHeader);
    
    return monsterNamesGetCustom(monsterNamesWithHeader, idx, monsterNamesWithHeader->nameLength, monsterNamesWithHeader->numNames);
}

// Returns a pointer to the first unused name slot, if ptrOut is non-NULL.
// Returns the index of the first unused name slot, if idxOut is non-NULL.
// Runs in O(n)
bool monsterNamesGetFirstUnused(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut) {
    assert(monsterNamesWithHeader);
    
    // Skip index 0
    for(uint16_t i = 1; i < monsterNamesWithHeader->numNames; i++) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            return false;
        }
        
        // If the first character is a null terminator, return the pointer and index
        if(ptr[0] == 0) {
            if(ptrOut != NULL) {
                *ptrOut = ptr;
            }
            
            if(idxOut != NULL) {
                *idxOut = i;
            }
            
            return true;
        }
    }
    
    return false;
}

// Returns a pointer to the last used name slot, if ptrOut is non-NULL.
// Returns the index of the last used name slot, if idxOut is non-NULL.
// Runs in O(n)
bool monsterNamesGetLastUsed(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut) {
    assert(monsterNamesWithHeader);
    
    for(int32_t i = monsterNamesWithHeader->numNames; i >= 0; i--) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            return false;
        }
        
        // If the first character is non-null, return the pointer and index.
        // If no names are actually used, treat the 0th/"null" name as used.
        if(ptr[0] != 0 || i == 0) {
            if(ptrOut != NULL) {
                *ptrOut = ptr;
            }
            
            if(idxOut != NULL) {
                *idxOut = i;
            }
            
            return true;
        }
    }
    
    return false;
}

// Adds an element to the end of the buffer, overwriting the oldest element if the buffer is full.
// data must be at least as long as the monster names structure's nameLength.
// Returns a pointer to the name slot written to, if ptrOut is non-NULL.
// Returns the index of the name slot written to, if idxOut is non-NULL.
// Runs in O(n)
bool monsterNamesPut(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut, const char* data) {
    assert(monsterNamesWithHeader);
    
    char* ptr;
    uint16_t idx;
    if(!monsterNamesGetFirstUnused(monsterNamesWithHeader, &ptr, &idx)) {
        return false;
    }
    
    // Add the element
    strncpy(ptr, data, monsterNamesWithHeader->nameLength);
    
    // Add null terminator
    ptr[monsterNamesWithHeader->nameLength - 1] = 0;
    
    if(ptrOut != NULL) {
        *ptrOut = ptr;
    }
    
    if(idxOut != NULL) {
        *idxOut = idx;
    }
    
    return true;
}

// Returns true if the index is within used bounds of the array, false otherwise.
// Sets dataOut to the value of the element at the given index, if it exists. Otherwise, does not change dataOut.
// Runs in O(1)
bool monsterNamesStrncpy(names_header_t* monsterNamesWithHeader, char* dataOut, uint16_t idx) {
    assert(monsterNamesWithHeader && dataOut);
    
    // Bounds check is performed in monsterNamesGet, so no need to do it beforehand
    
    char* ptr = monsterNamesGet(monsterNamesWithHeader, idx);
    
    if(ptr == NULL) {
        return false;
    }
    
    strncpy(dataOut, ptr, monsterNamesWithHeader->nameLength);
    
    // Set null terminator
    dataOut[monsterNamesWithHeader->nameLength - 1] = 0;
    
    return true;
}

// Returns true if the given data exists in the array, false otherwise.
// Sets idxOut to the index of the data, if it exists. Otherwise, does not change idxOut.
// Runs in O(n)
bool monsterNamesSearch(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut, const char* data) {
    assert(monsterNamesWithHeader);
    
    // Search the buffer for the given data
    for(uint16_t i = 0; i < monsterNamesWithHeader->numNames; i++) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            return false;
        }
        
        if(strncmp(ptr, data, monsterNamesWithHeader->nameLength) == 0) {
            if(ptrOut != NULL) {
                *ptrOut = ptr;
            }
            
            if(idxOut != NULL) {
                *idxOut = i;
            }
            
            return true;
        }
    }
    
    // No match
    return false;
}

// Runs in O(n)
bool monsterNamesRemoveIdx(names_header_t* monsterNamesWithHeader, uint16_t idx) {
    assert(monsterNamesWithHeader);
    
    // Cannot remove items past the end of the array
    if(idx >= monsterNamesWithHeader->numNames) {
        return false;
    }
    
    // Bounds check is performed in monsterNamesGet, so no need to do it beforehand
    
    char* ptr = monsterNamesGet(monsterNamesWithHeader, idx);
    
    if(ptr == NULL) {
        return false;
    }
    
    // Set the first character to a null terminator
    ptr[0] = 0;
    
    return true;
}

// Runs in O(n)
bool monsterNamesRemove(names_header_t* monsterNamesWithHeader, const char* data) {
    // No need to do an assert here, since we don't use the pointers directly, just pass them to other functions with their own asserts
    
    uint16_t existingIdx = 0;
    
    if(!monsterNamesSearch(monsterNamesWithHeader, NULL, &existingIdx, data)) {
        return false;
    }
    
    return monsterNamesRemoveIdx(monsterNamesWithHeader, existingIdx);
}

// Runs in O(n)
bool monsterNamesCopy(names_header_t* dest, names_header_t* src) {
    assert(dest && src);
    
    // TODO: count in-use names instead of using src->numNames
    // TODO: count max stored name length instead of using src->nameLength
    //uint16_t numNamesUsedInSrc = monsterNamesCountUsed(src);
    //uint8_t longestNameLengthInSrc = monsterNamesCountLongestName(src);
    if(dest->numNames < src->numNames || dest->nameLength < src->nameLength) {
        return false;
    }
    
    uint8_t minLength = MIN(src->nameLength, dest->nameLength);
    uint8_t extraLength = dest->nameLength - minLength;
    
    for(uint16_t i = 0; i < src->numNames; i++) {
        char* srcPtr = monsterNamesGet(src, i);
        char* destPtr = monsterNamesGet(dest, i);
        
        if(srcPtr == NULL || destPtr == NULL) {
            return false;
        }
        
        strncpy(destPtr, srcPtr, minLength);
        
        if(extraLength > 0) {
            // Set the characters after the copied string to null terminators
            memset(&destPtr[minLength], 0, extraLength);
        }
        else {
            // Set the last character to a null terminator
            destPtr[dest->nameLength - 1] = 0;
        }
    }
    
    return true;
}

// Grow the capacity of the array by additionalNames, adding the extra space to the end
// Runs in O(n)
bool monsterNamesGrow(names_header_t** monsterNamesWithHeader, uint16_t additionalNames) {
    assert(monsterNamesWithHeader && *monsterNamesWithHeader);
    
    uint16_t newNumNames = (*monsterNamesWithHeader)->numNames + additionalNames;
    size_t newSize = sizeof(names_header_t) + (*monsterNamesWithHeader)->nameLength * newNumNames;
    
    // Request reallocation of memory
    names_header_t* newPtr = heap_caps_realloc(*monsterNamesWithHeader, newSize, MALLOC_CAP_SPIRAM);
    
    if(newPtr == NULL) {
        // Old memory is still allocated, so old pointer remains valid
        return false;
    }
    
    // Old memory is freed or old/new pointers are identical, so we update the pointer
    *monsterNamesWithHeader = newPtr;
    
    // Zero out the new memory
    for(int i = (*monsterNamesWithHeader)->numNames; i < newNumNames; i++) {
        char* ptr = monsterNamesGetCustom(*monsterNamesWithHeader, i, (*monsterNamesWithHeader)->nameLength, newNumNames);
        
        if(ptr == NULL) {
            // Transaction is non-atomic (more memory is allocated, but not zero'd).
            // But all IO accesses (through the functions in this c file) and all NVS writes use the header, so we can just tell the calling function that the operation failed, and nothing should touch the new memory
            return false;
        }
        
        memset(ptr, 0, (*monsterNamesWithHeader)->nameLength);
    }
    
    // New memory is zero'd and usable
    (*monsterNamesWithHeader)->numNames = newNumNames;
    
    return true;
}

/*
// Allocates new memory, copies the array, replaces the pointer, and frees old memory
// Runs in O(n)
bool monsterNamesRealloc(names_header_t** monsterNamesWithHeader, uint8_t newNameLength, uint16_t newNumNames) {
    assert(monsterNamesWithHeader && *monsterNamesWithHeader);
    
    names_header_t* newPtr = NULL;
    size_t newSize = sizeof(names_header_t) + newNameLength * newNumNames;
    size_t oldSize = sizeof(names_header_t) + monsterNamesWithHeader->nameLength * monsterNamesWithHeader->numNames;
    
    // if new size is smaller, copy old names backward, starting with first name and working forward
    if(newSize < oldSize) {
        for(uint16_t i = 0; i < src->numNames; i++) {
            char* srcPtr = monsterNamesGet(src, i);
            char* destPtr = monsterNamesGet(dest, i);
            
            if(srcPtr == NULL || destPtr == NULL) {
                return false;
            }
            
            // TODO
        }
    }
    
    if(newNameLength == (*monsterNamesWithHeader)->nameLength) {
        newPtr = heap_caps_realloc(*monsterNamesWithHeader, newSize, MALLOC_CAP_SPIRAM);
        
        if(newPtr == NULL) {
            return false;
        }
        
        *monsterNamesWithHeader = newPtr;
    }
    
    // if new size is larger, copy old names forward, starting with last name and working backward
    // TODO
    
    *monsterNamesWithHeader->nameLength = newNameLength;
    *monsterNamesWithHeader->numNames = newNumNames;
    return true;
}
*/

// Runs in O(n)
uint16_t monsterNamesCountUsed(names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    uint16_t used = 0;
    
    for(uint16_t i = 0; i < monsterNamesWithHeader->numNames; i ++) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            assert(0);
        }
        
        // If the first character is non-NULL, this name slot is in use
        if(ptr[0] != 0) {
            used++;
        }
    }
    
    return used;
}

// Returns the number of characters in the longest name currently stored in the array, including space for the null terminator.
// If the null terminator does not exist in the longest name, the function will return monsterNamesWithHeader->nameLength + 1.
// Runs in O(n)
uint8_t monsterNamesCountLongestName(names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    uint8_t longestLength = 0;
    
    for(uint16_t i = 0; i < monsterNamesWithHeader->numNames; i ++) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            assert(0);
        }
        
        uint8_t length = (uint8_t) strnlen(ptr, monsterNamesWithHeader->nameLength);
        if(length > longestLength) {
            longestLength = length;
        }
    }
    
    return longestLength + 1;
}

// Runs in O(n)
bool monsterNamesEmpty(names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    for(uint16_t i = 0; i < monsterNamesWithHeader->numNames; i++) {
        char* ptr = monsterNamesGet(monsterNamesWithHeader, i);
        
        if(ptr == NULL) {
            assert(0);
        }
        
        // If the first character is non-NULL, this name slot is in use, so the array is not empty
        if(ptr[0] != 0) {
            return false;
        }
    }
    
    return true;
}

// Runs in O(n)
bool monsterNamesFull(names_header_t* monsterNamesWithHeader) {
    assert(monsterNamesWithHeader);
    
    return !monsterNamesGetFirstUnused(monsterNamesWithHeader, NULL, NULL);
}

/**
 * @file cVectors.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Implements the C++ style vectors (dynamic arrays)
 * @version 1.0
 * @date 2025-01-01
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cVectors.h"
#include <stdlib.h>

//==============================================================================
// Defines
//==============================================================================

#define VEC_INITIAL_CAPACITY 4

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Resizes the vector when required
 *
 * @param vec Vector to resize
 * @param newCapacity How large the Vector needs to be
 */
void _vectorResize(cVector_t* vec, int newCapacity);

//==============================================================================
// Functions
//==============================================================================

void vectorInit(cVector_t* vec)
{
    vec->capacity = VEC_INITIAL_CAPACITY;
    vec->data     = malloc(sizeof(void*) * vec->capacity);
    vec->total    = 0;
}

int vectorTotal(cVector_t* vec)
{
    return vec->total;
}

void vectorAdd(cVector_t* vec, void* item)
{
    if (vec->capacity == vec->total)
    {
        _vectorResize(vec, vec->capacity * 2);
    }
    vec->data[vec->total++] = item;
}

void vectorSet(cVector_t* vec, int idx, void* item)
{
    if (idx >= 0 && idx < vec->total)
    {
        vec->data[idx] = item;
    }
}

void* vectorGet(cVector_t* vec, int idx)
{
    if (idx >= 0 && idx < vec->total)
    {
        return vec->data[idx];
    }
    return NULL;
}

void vectorRemove(cVector_t* vec, int idx)
{
    // Return if outside bounds
    if (idx < 0 || idx >= vectorTotal(vec))
    {
        return;
    }

    // Shift data down, overwriting starting on the main index
    for (int i = idx; i < vec->total - 1; i++)
    {
        vec->data[i] = vec->data[i + 1];
    }

    // Null last value
    vec->total--;
    vec->data[vec->total] = NULL;

    // Resize down if capacity is low enough (Divided by four to not constantly up and daown size)
    if (vec->total > 0 && vec->total == (vec->capacity >> 2))
    {
        _vectorResize(vec, vec->capacity >> 1);
    }
}

void vectorFree(cVector_t* vec)
{
    free(vec->data);
}

//==============================================================================
// Static Functions
//==============================================================================

void _vectorResize(cVector_t* vec, int newCapacity)
{
    void** data = realloc(vec->data, sizeof(void*) * newCapacity);
    if (data)
    {
        vec->data     = data;
        vec->capacity = newCapacity;
    }
}
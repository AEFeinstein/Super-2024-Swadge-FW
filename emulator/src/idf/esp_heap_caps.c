#include <stdio.h>
#include "esp_heap_caps.h"

typedef struct
{
    void* ptr;
    size_t size;
    int32_t caps;
} allocation_t;

#define A_TABLE_SIZE 4096
allocation_t aTable[A_TABLE_SIZE] = {0};
size_t usedMemory[2]              = {0};

/**
 * @brief Allocate a chunk of memory which has the given capabilities
 *
 * Equivalent semantics to libc malloc(), for capability-aware memory.
 *
 * In IDF, ``malloc(p)`` is equivalent to ``heap_caps_malloc(p, MALLOC_CAP_8BIT)``.
 *
 * @param size Size, in bytes, of the amount of memory to allocate
 * @param caps        Bitwise OR of MALLOC_CAP_* flags indicating the type
 *                    of memory to be returned
 *
 * @return A pointer to the memory allocated on success, NULL on failure
 */
void* heap_caps_malloc(size_t size, uint32_t caps __attribute__((unused)))
{
    int32_t idx = 0;
    while (NULL != aTable[idx].ptr)
    {
        idx++;
    }

    void* ptr = malloc(size);

    if (idx < A_TABLE_SIZE)
    {
        aTable[idx].ptr  = ptr;
        aTable[idx].size = size;
        aTable[idx].caps = caps;

        if(MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] += aTable[idx].size;
        }
        else
        {
            usedMemory[0] += aTable[idx].size;
        }
        printf("malloc: %7zu / %7zu\n", usedMemory[0], usedMemory[1]);
    }
    else
    {
        fprintf(stderr, "Allocation table out of space\n");
    }

    return aTable[idx].ptr;
}

/**
 * @brief Allocate a chunk of memory which has the given capabilities. The initialized value in the memory is set to
 * zero.
 *
 * Equivalent semantics to libc calloc(), for capability-aware memory.
 *
 * In IDF, ``calloc(p)`` is equivalent to ``heap_caps_calloc(p, MALLOC_CAP_8BIT)``.
 *
 * @param n    Number of continuing chunks of memory to allocate
 * @param size Size, in bytes, of a chunk of memory to allocate
 * @param caps        Bitwise OR of MALLOC_CAP_* flags indicating the type
 *                    of memory to be returned
 *
 * @return A pointer to the memory allocated on success, NULL on failure
 */
void* heap_caps_calloc(size_t n, size_t size, uint32_t caps)
{
    int32_t idx = 0;
    while (NULL != aTable[idx].ptr)
    {
        idx++;
    }

    void* ptr = calloc(n, size);

    if (idx < A_TABLE_SIZE)
    {
        aTable[idx].ptr  = ptr;
        aTable[idx].size = (n * size);
        aTable[idx].caps = caps;

        if(MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] += aTable[idx].size;
        }
        else
        {
            usedMemory[0] += aTable[idx].size;
        }
        printf("calloc: %7zu / %7zu\n", usedMemory[0], usedMemory[1]);
    }
    else
    {
        fprintf(stderr, "Allocation table out of space\n");
    }
    return aTable[idx].ptr;
}

void heap_caps_free(void* ptr)
{
    int32_t idx = 0;
    while (aTable[idx].ptr != ptr)
    {
        idx++;
    }
    if (idx < A_TABLE_SIZE)
    {
        if(MALLOC_CAP_SPIRAM & aTable[idx].caps)
        {
            usedMemory[1] -= aTable[idx].size;
        }
        else
        {
            usedMemory[0] -= aTable[idx].size;
        }
        aTable[idx].ptr  = NULL;
        aTable[idx].size = 0;
        printf("free  : %7zu / %7zu\n", usedMemory[0], usedMemory[1]);
    }
    free(ptr);
}
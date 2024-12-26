#include <stdio.h>
#include "esp_heap_caps.h"
#include <inttypes.h>

//#define MEMORY_DEBUG
#ifdef MEMORY_DEBUG

    #define SPIRAM_SIZE          2093904
    #define SPIRAM_LARGEST_BLOCK 2064384

    // #define MEMORY_DEBUG_PRINT
    #ifdef MEMORY_DEBUG_PRINT
        #define md_printf(...)     printf(__VA_ARGS__)
        #define md_fprintf(f, ...) fprintf(f, __VA_ARGS__)
    #else
        #define md_printf(...)
        #define md_fprintf(f, ...)
    #endif

    #define A_TABLE_SIZE 16384
typedef struct
{
    void* ptr;
    size_t size;
    int32_t caps;
} allocation_t;

allocation_t aTable[A_TABLE_SIZE] = {0};
size_t usedMemory[2]              = {0};

const char memDbgFmtStr[] = "%-6s at %s:%d\n  %7" PRIu32 " / %7" PRIu32 " (%p)\n";

#endif

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
void* heap_caps_malloc_dbg(size_t size, uint32_t caps, const char* file, const char* func, int32_t line)
{
#ifdef MEMORY_DEBUG
    if (size >= SPIRAM_LARGEST_BLOCK)
    {
        fprintf(stderr, memDbgFmtStr, "Too large alloc!", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1],
                NULL);
        exit(-1);
    }

    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && NULL != aTable[idx].ptr)
    {
        idx++;
    }

    void* ptr = malloc(size);

    if (idx < A_TABLE_SIZE)
    {
        aTable[idx].ptr  = ptr;
        aTable[idx].size = size;
        aTable[idx].caps = caps;

        if (MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] += aTable[idx].size;
        }
        else
        {
            usedMemory[0] += aTable[idx].size;
        }
        md_printf(memDbgFmtStr, "malloc", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1], ptr);

        if (usedMemory[1] >= SPIRAM_SIZE)
        {
            fprintf(stderr, memDbgFmtStr, "Out of SPIRAM!", file, line, (uint32_t)usedMemory[0],
                    (uint32_t)usedMemory[1], NULL);
            exit(-1);
        }
    }
    else
    {
        md_fprintf(stderr, "Allocation table out of space\n");
    }

    return ptr;
#else
    return malloc(size);
#endif
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
void* heap_caps_calloc_dbg(size_t n, size_t size, uint32_t caps, const char* file, const char* func, int32_t line)
{
#ifdef MEMORY_DEBUG

    if (n * size >= SPIRAM_LARGEST_BLOCK)
    {
        fprintf(stderr, memDbgFmtStr, "Too large alloc!", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1],
                NULL);
        exit(-1);
    }

    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && NULL != aTable[idx].ptr)
    {
        idx++;
    }

    void* ptr = calloc(n, size);

    if (idx < A_TABLE_SIZE)
    {
        aTable[idx].ptr  = ptr;
        aTable[idx].size = (n * size);
        aTable[idx].caps = caps;

        if (MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] += aTable[idx].size;
        }
        else
        {
            usedMemory[0] += aTable[idx].size;
        }
        md_printf(memDbgFmtStr, "calloc", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1], ptr);

        if (usedMemory[1] >= SPIRAM_SIZE)
        {
            fprintf(stderr, memDbgFmtStr, "Out of SPIRAM!", file, line, (uint32_t)usedMemory[0],
                    (uint32_t)usedMemory[1], NULL);
            exit(-1);
        }
    }
    else
    {
        md_fprintf(stderr, "Allocation table out of space\n");
    }
    return ptr;
#else
    return calloc(n, size);
#endif
}

void heap_caps_free_dbg(void* ptr, const char* file, const char* func, int32_t line)
{
#ifdef MEMORY_DEBUG
    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && aTable[idx].ptr != ptr)
    {
        idx++;
    }
    if (idx < A_TABLE_SIZE)
    {
        if (MALLOC_CAP_SPIRAM & aTable[idx].caps)
        {
            usedMemory[1] -= aTable[idx].size;
        }
        else
        {
            usedMemory[0] -= aTable[idx].size;
        }
        aTable[idx].ptr  = NULL;
        aTable[idx].size = 0;
        md_printf(memDbgFmtStr, "free", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1], ptr);
    }
    else
    {
        fprintf(stderr, memDbgFmtStr, "Probable double-free!", file, line, (uint32_t)usedMemory[0],
                (uint32_t)usedMemory[1], ptr);
    }
#endif
    free(ptr);
}

void* heap_caps_realloc_dbg(void* ptr, size_t size, uint32_t caps, const char* file, const char* func, int32_t line)
{
#ifdef MEMORY_DEBUG

    if (size >= SPIRAM_LARGEST_BLOCK)
    {
        fprintf(stderr, memDbgFmtStr, "Too large alloc!", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1],
                NULL);
        exit(-1);
    }

    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && ptr != aTable[idx].ptr)
    {
        idx++;
    }

    void* rPtr = realloc(ptr, size);

    if (idx < A_TABLE_SIZE)
    {
        // Remove old size
        if (MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] -= aTable[idx].size;
        }
        else
        {
            usedMemory[0] -= aTable[idx].size;
        }

        aTable[idx].ptr  = rPtr;
        aTable[idx].size = size;
        aTable[idx].caps = caps;

        // Add new size
        if (MALLOC_CAP_SPIRAM & caps)
        {
            usedMemory[1] += aTable[idx].size;
        }
        else
        {
            usedMemory[0] += aTable[idx].size;
        }
        md_printf(memDbgFmtStr, "realloc", file, line, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1], rPtr);

        if (usedMemory[1] >= SPIRAM_SIZE)
        {
            fprintf(stderr, memDbgFmtStr, "Out of SPIRAM!", file, line, (uint32_t)usedMemory[0],
                    (uint32_t)usedMemory[1], NULL);
            exit(-1);
        }
    }
    else
    {
        md_fprintf(stderr, "Allocation table out of space\n");
    }
    return rPtr;
#else
    return realloc(ptr, size);
#endif
}
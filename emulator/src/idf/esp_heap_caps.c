//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include "esp_heap_caps.h"

//==============================================================================
// Defines
//==============================================================================

#define MEMORY_DEBUG
#define MEMORY_DEBUG_PRINT

#define SPIRAM_SIZE          2093904
#define SPIRAM_LARGEST_BLOCK 2064384

#define A_TABLE_SIZE 16384

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    OP_MALLOC,
    OP_CALLOC,
    OP_REALLOC,
    OP_FREE,
} memOp_t;

typedef enum
{
    MEM_INTERNAL,
    MEM_SPIRAM,
    MAX_MEM_TYPES,
} memType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    void* ptr;
    size_t size;
    int32_t caps;
    const char* file;
    const char* func;
    uint32_t line;
    char tag[32];
} allocation_t;

//==============================================================================
// Variables
//==============================================================================

allocation_t aTable[A_TABLE_SIZE] = {0};
size_t usedMemory[MAX_MEM_TYPES]  = {0};

//==============================================================================
// Function declarations
//==============================================================================

static void printMemoryOperation(memOp_t op, allocation_t* al);
static void saveAllocation(memOp_t op, void* ptr, allocation_t* oldEntry, uint32_t size, uint32_t caps,
                           const char* file, const char* func, uint32_t line, const char* tag);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param op
 * @param al
 */
static void printMemoryOperation(memOp_t op, allocation_t* al)
{
#ifdef MEMORY_DEBUG_PRINT

    const char str_malloc[]  = "malloc";
    const char str_calloc[]  = "calloc";
    const char str_realloc[] = "realloc";
    const char str_free[]    = "free";
    const char* opStr        = NULL;
    switch (op)
    {
        case OP_MALLOC:
        {
            opStr = str_malloc;
            break;
        }
        case OP_CALLOC:
        {
            opStr = str_calloc;
            break;
        }
        case OP_REALLOC:
        {
            opStr = str_realloc;
            break;
        }
        case OP_FREE:
        {
            opStr = str_free;
            break;
        }
    }

    int32_t internalDiff = 0;
    int32_t spiRamDiff   = 0;
    if (al->caps & MALLOC_CAP_SPIRAM)
    {
        spiRamDiff = (OP_FREE == op) ? -al->size : al->size;
    }
    else
    {
        internalDiff = (OP_FREE == op) ? -al->size : al->size;
    }

    static bool headerPrinted = false;
    if (!headerPrinted)
    {
        printf("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", "Operation", "File", "Function", "Line", "Tag", "Pointer", "INT Diff",
               "SPI Diff", "INT Used", "SPI Used");
        headerPrinted = true;
    }

    // esp_timer_get_time() is impossible to use here for some reason
    printf("%s,%s,%s,%d,%s,%p,%d,%d,%d,%d\n", opStr, al->file, al->func, al->line, al->tag, al->ptr, internalDiff,
           spiRamDiff, (uint32_t)usedMemory[0], (uint32_t)usedMemory[1]);
#endif
}

/**
 * @brief TODO
 *
 * @param op
 * @param ptr
 * @param oldPtr
 * @param size
 * @param caps
 * @param file
 * @param func
 * @param line
 * @param tag
 */
static void saveAllocation(memOp_t op, void* ptr, allocation_t* oldEntry, uint32_t size, uint32_t caps,
                           const char* file, const char* func, uint32_t line, const char* tag)
{
    allocation_t* al = NULL;
    // Find the old entry in the table. if oldPtr is NULL, then an empty entry will be found
    if (NULL == oldEntry)
    {
        int32_t idx = 0;
        while (idx < A_TABLE_SIZE && NULL != aTable[idx].ptr)
        {
            idx++;
        }
        al = &aTable[idx];
    }
    else
    {
        al = oldEntry;
    }

    // If the index is valid
    if (NULL != al)
    {
        // Freeing works differently than allocating
        if (OP_FREE == op)
        {
            // Pick a variable to track overall size
            size_t* usedMem = (MALLOC_CAP_SPIRAM & al->caps) ? &usedMemory[1] : &usedMemory[0];

            // Overwrite with free source data for printing later
            al->file = file;
            al->func = func;
            al->line = line;
            // Don't overwrite tag

            // Decrement space
            if (al->size > *usedMem)
            {
                *usedMem = 0;
                fprintf(stderr, "!! Freeing more than allocated at %s:%d\n", file, line);
            }
            else
            {
                *usedMem -= al->size;
            }

            // Print the operation
            printMemoryOperation(op, al);

            // Erase the table entry
            memset(al, 0, sizeof(allocation_t));
        }
        else
        {
            if (size >= SPIRAM_LARGEST_BLOCK)
            {
                fprintf(stderr, "!! Too large alloc at %s:%d (%d)\n", file, line, size);
                exit(-1);
            }

            // Pick a variable to track overall size
            size_t* usedMem = (MALLOC_CAP_SPIRAM & caps) ? &usedMemory[1] : &usedMemory[0];

            // Save the old size for reallocs
            uint32_t oldSize = 0;
            if (OP_REALLOC == op)
            {
                oldSize = al->size;
            }

            // Save entry
            al->ptr  = ptr;
            al->size = size;
            al->caps = caps;
            al->file = file;
            al->func = func;
            al->line = line;
            if (tag)
            {
                snprintf(al->tag, sizeof(al->tag) - 1, "%s", tag);
            }
            else
            {
                memset(al->tag, 0, sizeof(al->tag));
            }

            // Adjust space
            *usedMem -= oldSize;
            *usedMem += al->size;

            // Print it
            printMemoryOperation(op, al);
        }

        if (usedMemory[1] >= SPIRAM_SIZE)
        {
            fprintf(stderr, "!! Out of SPIRAM at %s:%d (%d)\n", file, line, (uint32_t)usedMemory[1]);
            exit(-1);
        }
    }
    else if (OP_FREE == op)
    {
        // Trying to free an entry not in the table
        fprintf(stderr, "!! Probable double-free at %s:%d (%p)\n", file, line, ptr);
    }
    else
    {
        // Trying to add to the table, but there's no space
        fprintf(stderr, "!! Allocation table out of space\n");
    }
}

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
void* heap_caps_malloc_dbg(size_t size, uint32_t caps, const char* file, const char* func, int32_t line,
                           const char* tag)
{
#ifdef MEMORY_DEBUG
    void* ptr = malloc(size);
    saveAllocation(OP_MALLOC, ptr, NULL, size, caps, file, func, line, tag);
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
void* heap_caps_calloc_dbg(size_t n, size_t size, uint32_t caps, const char* file, const char* func, int32_t line,
                           const char* tag)
{
#ifdef MEMORY_DEBUG
    void* ptr = calloc(n, size);
    saveAllocation(OP_CALLOC, ptr, NULL, n * size, caps, file, func, line, tag);
    return ptr;
#else
    return calloc(n, size);
#endif
}

/**
 * @brief TODO
 *
 * @param ptr
 * @param size
 * @param caps
 * @param file
 * @param func
 * @param line
 * @param tag
 * @return void*
 */
void* heap_caps_realloc_dbg(void* ptr, size_t size, uint32_t caps, const char* file, const char* func, int32_t line,
                            const char* tag)
{
#ifdef MEMORY_DEBUG

    // Find the old entry in the table. if oldPtr is NULL, then an empty entry will be found
    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && ptr != aTable[idx].ptr)
    {
        idx++;
    }

    void* newPtr = realloc(ptr, size);
    saveAllocation(OP_REALLOC, newPtr, &aTable[idx], size, caps, file, func, line, tag);
    return newPtr;
#else
    return realloc(ptr, size);
#endif
}

/**
 * @brief TODO
 *
 * @param ptr
 * @param file
 * @param func
 * @param line
 */
void heap_caps_free_dbg(void* ptr, const char* file, const char* func, int32_t line, const char* tag)
{
#ifdef MEMORY_DEBUG

    // Find the old entry in the table. if oldPtr is NULL, then an empty entry will be found
    int32_t idx = 0;
    while (idx < A_TABLE_SIZE && ptr != aTable[idx].ptr)
    {
        idx++;
    }

    saveAllocation(OP_FREE, ptr, &aTable[idx], 0, 0, file, func, line, tag);
#endif
    free(ptr);
}

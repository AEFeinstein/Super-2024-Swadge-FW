#pragma once

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Flags to indicate the capabilities of the various memory systems
 */
#define MALLOC_CAP_EXEC   (1 << 0)  ///< Memory must be able to run executable code
#define MALLOC_CAP_32BIT  (1 << 1)  ///< Memory must allow for aligned 32-bit data accesses
#define MALLOC_CAP_8BIT   (1 << 2)  ///< Memory must allow for 8/16/...-bit data accesses
#define MALLOC_CAP_DMA    (1 << 3)  ///< Memory must be able to accessed by DMA
#define MALLOC_CAP_PID2   (1 << 4)  ///< Memory must be mapped to PID2 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID3   (1 << 5)  ///< Memory must be mapped to PID3 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID4   (1 << 6)  ///< Memory must be mapped to PID4 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID5   (1 << 7)  ///< Memory must be mapped to PID5 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID6   (1 << 8)  ///< Memory must be mapped to PID6 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID7   (1 << 9)  ///< Memory must be mapped to PID7 memory space (PIDs are not currently used)
#define MALLOC_CAP_SPIRAM (1 << 10) ///< Memory must be in SPI RAM
#define MALLOC_CAP_INTERNAL \
    (1 << 11) ///< Memory must be internal; specifically it should not disappear when flash/spiram cache is switched off
#define MALLOC_CAP_DEFAULT \
    (1 << 12) ///< Memory can be returned in a non-capability-specific memory allocation (e.g. malloc(), calloc()) call
#define MALLOC_CAP_IRAM_8BIT (1 << 13) ///< Memory must be in IRAM and allow unaligned access
#define MALLOC_CAP_RETENTION (1 << 14)
#define MALLOC_CAP_RTCRAM    (1 << 15) ///< Memory must be in RTC fast memory
#define MALLOC_CAP_INVALID   (1 << 31) ///< Memory can't be used / list end marker

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
#define heap_caps_malloc(s, c)    heap_caps_malloc_dbg(s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_malloc_tag(s, c, t)    heap_caps_malloc_dbg(s, c, __FILE__, __func__, __LINE__, t)

/** See above, but with function and line debugging */
void* heap_caps_malloc_dbg(size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);

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
#define heap_caps_calloc(n, s, c) heap_caps_calloc_dbg(n, s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_calloc_tag(n, s, c, t) heap_caps_calloc_dbg(n, s, c, __FILE__, __func__, __LINE__, t)

/** See above, but with function and line debugging */
void* heap_caps_calloc_dbg(size_t n, size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);


/**
 * @brief Reallocate memory previously allocated via heap_caps_malloc() or heap_caps_realloc().
 * 
 * Equivalent semantics to libc realloc(), for capability-aware memory.
 * 
 * In IDF, realloc(p, s) is equivalent to heap_caps_realloc(p, s, MALLOC_CAP_8BIT).
 * 
 * 'caps' parameter can be different to the capabilities that any original 'ptr' was allocated with. In this way, realloc can be used to "move" a buffer if necessary to ensure it meets a new set of capabilities.
 *
 * @param ptr Pointer to previously allocated memory, or NULL for a new allocation.
 * @param size Size of the new buffer requested, or 0 to free the buffer.
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory desired for the new allocation.
 *
 * @return Pointer to a new buffer of size 'size' with capabilities 'caps', or NULL if allocation failed.
 */
#define heap_caps_realloc(p, s, c) heap_caps_realloc_dbg(p, s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_realloc_tag(p, s, c, t) heap_caps_realloc_dbg(p, s, c, __FILE__, __func__, __LINE__, t)

/** See above, but with function and line debugging */
void *heap_caps_realloc_dbg(void *ptr, size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);

/**
 * @brief Custom free to track memory allocation
 * 
 * @param ptr 
 */
#define heap_caps_free(p)         heap_caps_free_dbg(p, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_free_tag(p)         heap_caps_free_dbg(p, __FILE__, __func__, __LINE__, t)

/** See above, but with function and line debugging */
void heap_caps_free_dbg(void* ptr, const char* file, const char* func, int32_t line, const char* tag);

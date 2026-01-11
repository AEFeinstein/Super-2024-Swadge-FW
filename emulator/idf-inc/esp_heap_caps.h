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


#define heap_caps_malloc(s, c)    heap_caps_malloc_dbg(s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_malloc_tag(s, c, t)    heap_caps_malloc_dbg(s, c, __FILE__, __func__, __LINE__, t)
void* heap_caps_malloc_dbg(size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);

#define heap_caps_calloc(n, s, c) heap_caps_calloc_dbg(n, s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_calloc_tag(n, s, c, t) heap_caps_calloc_dbg(n, s, c, __FILE__, __func__, __LINE__, t)
void* heap_caps_calloc_dbg(size_t n, size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);

#define heap_caps_realloc(p, s, c) heap_caps_realloc_dbg(p, s, c, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_realloc_tag(p, s, c, t) heap_caps_realloc_dbg(p, s, c, __FILE__, __func__, __LINE__, t)
void *heap_caps_realloc_dbg(void *ptr, size_t size, uint32_t caps, const char* file, const char* func, int32_t line, const char* tag);

#define heap_caps_free(p)         heap_caps_free_dbg(p, __FILE__, __func__, __LINE__, NULL)
#define heap_caps_free_tag(p)         heap_caps_free_dbg(p, __FILE__, __func__, __LINE__, t)
void heap_caps_free_dbg(void* ptr, const char* file, const char* func, int32_t line, const char* tag);

void dumpAllocTable(void);

/*! \file coreutil.h
 *
 * \section coreutil_utils_design Design Philosophy
 *
 * These are functions which can help profile code running on real hardware. They won't do anything useful in the
 * emulator.
 *
 * \section coreutil_utils_usage Usage
 *
 * Call the functions as necessary.
 *
 * \section coreutil_utils_example Example
 *
 * \code{.c}
 * // Get the number of cycles when starting
 * uint32_t start = getCycleCount();
 * // Run some function to profile
 * some_long_func();
 * // Get the total number of cycles elapsed
 * uint32_t total = getCycleCount() - start;
 * // Print the number of cycles
 * printf("%" PRIu32 "\n", total);
 * \endcode
 */

#ifndef _COREUTIL_H_
#define _COREUTIL_H_

/** @brief Silly redefinition to trick Doxygen into documenting a static function, which it otherwise ignores */
#define STATIC static

/**
 * @brief Get current cycle count of processor for profiling.  It is in 1/F_CPU units.
 *   This will actually compile down to be included in the code, itself, and
 *   "should" (does in all the tests I've run) execute in one clock cycle since
 *   there is no function call and rsr only takes one cycle to complete.
 *
 * Note, this will always return zero on for the emulator
 *
 * @return Number of processor cycles on actual hardware, or zero in the emulator.
 */
STATIC inline uint32_t getCycleCount(void)
{
#if defined(__XTENSA__)
    uint32_t ccount;
    asm volatile("rsr %0,ccount" : "=a"(ccount));
    return ccount;
#else
    return 0;
#endif
}

#endif

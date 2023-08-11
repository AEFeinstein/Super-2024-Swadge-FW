#ifndef _COREUTIL_H_
#define _COREUTIL_H_


/**
 * @brief Get current cycle count of processor for profiling.  It is in 1/F_CPU units.
 *   This will actually compile down to be included in the code, itself, and
 *   "should" (does in all the tests I've run) execute in one clock cycle since
 *   there is no function call and rsr only takes one cycle to complete. 
 *
 * @return Number of processor cycles.
 */
static inline uint32_t getCycleCount()
{
    uint32_t ccount;
    asm volatile("rsr %0,ccount":"=a" (ccount));
    return ccount;
}

#endif


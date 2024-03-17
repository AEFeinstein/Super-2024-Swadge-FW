//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include "hdw-dac.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

fnDacCallback_t dacCb = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the DAC
 *
 * @param cb
 */
void initDac(fnDacCallback_t cb)
{
    // TODO
    dacCb = cb;
}

/**
 * @brief TODO
 *
 */
void deinitDac(void)
{
}

/**
 * @brief TODO
 *
 */
void dacStart(void)
{
    // TODO
}

/**
 * @brief TODO
 *
 */
void dacStop(void)
{
    // TODO
}

/**
 * @brief Poll the queue to see if it needs to be filled with audio samples
 */
void dacPoll(void)
{
    // TODO
    uint8_t samps[2048];
    dacCb(samps, 2048);
}

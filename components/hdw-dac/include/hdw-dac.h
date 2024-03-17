/*! \file hdw-dac.h
 *
 * \section dac_design Design Philosophy
 *
 * TODO
 *
 * \section dac_usage Usage
 *
 * TODO
 *
 * \section dac_example Example
 *
 * TODO:
 * \code{.c}
 * TODO
 * \endcode
 */

#pragma once

#include <stdint.h>

#define AUDIO_SAMPLE_RATE_HZ 32768

/**
 * @brief TODO
 * @param len
 * @param sample
 */
typedef void (*fnDacCallback_t)(uint8_t* samples, int16_t len);

void initDac(fnDacCallback_t cb);
void deinitDac(void);
void dacPoll(void);
void dacStart(void);
void dacStop(void);

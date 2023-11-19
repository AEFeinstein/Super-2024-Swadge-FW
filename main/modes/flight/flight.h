/*
 * mode_flight.h
 *
 *  Created on: Sept 15th, 2020
 *      Author: <>< CNLohr
 */

#ifndef MODES_MODE_FLIGHT_H_
#define MODES_MODE_FLIGHT_H_

extern swadgeMode_t modeFlight;

#define NUM_FLIGHTSIM_TOP_SCORES   4
#define FLIGHT_HIGH_SCORE_NAME_LEN 4

typedef struct __attribute__((aligned(4)))
{
    // One set for any% one set for 100%
    char displayName[NUM_FLIGHTSIM_TOP_SCORES * 2][FLIGHT_HIGH_SCORE_NAME_LEN];
    uint32_t timeCentiseconds[NUM_FLIGHTSIM_TOP_SCORES * 2];
    // 0 = D-pad down to pitch up, 1 = D-pad up to pitch up. Opposite of most games' y-invert settings
    uint8_t flightInvertY;
    uint8_t flightEnableIMU;
    uint8_t reserved[16];
} flightSimSaveData_t;

extern swadgeMode_t flightMode;

#endif /* MODES_MODE_FLIGHT_H_ */

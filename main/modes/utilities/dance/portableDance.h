#ifndef _PORTABLEDANCE_H_
#define _PORTABLEDANCE_H_

#include "swadge2024.h"
#include "dance.h"

/// Speed scaling factor. Actual speed will be DANCE_SPEED_MULT / speed
#define DANCE_SPEED_MULT 8

typedef struct
{
    const ledDance_t* dance;
    bool enable;
} ledDanceOpt_t;

typedef struct
{
    // List of dances to loop through
    ledDanceOpt_t* dances;

    // Set when dance needs to be reset
    bool resetDance;

    uint8_t danceIndex;
    int32_t speed;

    // If non-NULL, the dance index and speed will be saved/loaded from this NVS namespace
    const char* nvsNs;
} portableDance_t;

portableDance_t* initPortableDance(const char* nvsNs);
void freePortableDance(portableDance_t* dance);
void portableDanceMainLoop(portableDance_t* dance, int64_t elapsedUs);
bool portableDanceSetByIndex(portableDance_t* dance, uint8_t danceIndex);
bool portableDanceSetByName(portableDance_t* dance, const char* danceName);
void portableDanceNext(portableDance_t* dance);
void portableDancePrev(portableDance_t* dance);
void portableDanceSetSpeed(portableDance_t* dance, int32_t speed);
bool portableDanceDisableDance(portableDance_t* dance, const char* danceName);
const char* portableDanceGetName(portableDance_t* dance);

#endif

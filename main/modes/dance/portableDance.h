#ifndef _PORTABLEDANCE_H_
#define _PORTABLEDANCE_H_

#include "swadge2024.h"
#include "dance.h"

typedef struct
{
    const ledDanceArg* dance;
    bool enable;
} ledDanceOpt_t;

typedef struct
{
    // List of dances to loop through
    ledDanceOpt_t* dances;

    // Set when dance needs to be reset
    bool resetDance;

    uint8_t danceIndex;

    // If non-NULL, the dance index will be saved/loaded from this nvs key
    const char* nvsKey;
} portableDance_t;

portableDance_t* initPortableDance(const char* nvsKey);
void freePortableDance(portableDance_t* dance);
void portableDanceMainLoop(portableDance_t* dance, int64_t elapsedUs);
bool portableDanceSetByName(portableDance_t* dance, const char* danceName);
void portableDanceNext(portableDance_t* dance);
void portableDancePrev(portableDance_t* dance);
bool portableDanceDisableDance(portableDance_t* dance, const char* danceName);
const char* portableDanceGetName(portableDance_t* dance);

#endif
#ifndef _PORTABLEDANCE_H_
#define _PORTABLEDANCE_H_

#include "swadge.h"
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

    // Used for the body if split is on. Used for everything if split is off.
    uint8_t danceIndex;
    int32_t speed;

    // Only used if split is on.
    uint8_t wingDanceIndex;
    int32_t wingSpeed;
    led_t bodyLeds[CONFIG_NUM_LEDS];
    led_t wingLeds[CONFIG_NUM_LEDS];

    // 0 for normal, 1 for different fairy body and wing patterns
    // It's an int not a bool because the menu structures use ints.
    uint8_t split;

    // If non-NULL, the dance index and speed will be saved/loaded from this NVS namespace
    const char* nvsNs;
} portableDance_t;

portableDance_t* initPortableDance(const char* nvsNs);
void freePortableDance(portableDance_t* dance);
void portableDanceMainLoop(portableDance_t* dance, int64_t elapsedUs);
void portableDanceSetSplit(portableDance_t* dance, uint8_t split);
bool portableDanceSetByIndex(portableDance_t* dance, uint8_t danceIndex, bool wings);
bool portableDanceSetByName(portableDance_t* dance, const char* danceName, bool wings);
void portableDanceNext(portableDance_t* dance);
void portableDancePrev(portableDance_t* dance);
void portableDanceSetSpeed(portableDance_t* dance, int32_t speed, bool wings);
bool portableDanceDisableDance(portableDance_t* dance, const char* danceName);
const char* portableDanceGetName(portableDance_t* dance);

#endif

#ifndef _BTN_H_
#define _BTN_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

typedef enum
{
    UP     = 0x01,
    DOWN   = 0x02,
    LEFT   = 0x04,
    RIGHT  = 0x08,
    BTN_A  = 0x10,
    BTN_B  = 0x20,
    START  = 0x40,
    SELECT = 0x80,
#if defined(EMU)
    EMU_P2_UP     = 0x0100,
    EMU_P2_DOWN   = 0x0200,
    EMU_P2_LEFT   = 0x0400,
    EMU_P2_RIGHT  = 0x0800,
    EMU_P2_BTN_A  = 0x1000,
    EMU_P2_BTN_B  = 0x2000,
    EMU_P2_START  = 0x4000,
    EMU_P2_SELECT = 0x8000,
#endif
} buttonBit_t;

typedef struct
{
    uint16_t state;
    buttonBit_t button;
    bool down;
} buttonEvt_t;

void initButtons(uint8_t numButtons, ...);
void deinitButtons(void);
bool checkButtonQueue(buttonEvt_t*);

#endif
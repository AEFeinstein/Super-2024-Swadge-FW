#ifndef _BTN_H_
#define _BTN_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/touch_pad.h"

typedef enum __attribute__((packed))
{
    UP     = 0x0001, //!< The up button's bit
    DOWN   = 0x0002, //!< The down button's bit
    LEFT   = 0x0004, //!< The left button's bit
    RIGHT  = 0x0008, //!< The right button's bit
    BTN_A  = 0x0010, //!< The A button's bit
    BTN_B  = 0x0020, //!< The B button's bit
    START  = 0x0040, //!< The start button's bit
    SELECT = 0x0080, //!< The select button's bit
    TP_0   = 0x0100, //!< Touch pad 0's button bit
    TP_1   = 0x0200, //!< Touch pad 1's button bit
    TP_2   = 0x0400, //!< Touch pad 2's button bit
    TP_3   = 0x0800, //!< Touch pad 3's button bit
    TP_4   = 0x1000, //!< Touch pad 4's button bit
} buttonBit_t;

typedef struct
{
    uint16_t state;     //!< A bitmask for the state of all buttons
    buttonBit_t button; //!< The button that caused this event
    bool down;          //!< True if the button was pressed, false if it was released
} buttonEvt_t;

void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchButtons, uint8_t numTouchButtons);
void deinitButtons(void);
bool checkButtonQueue(buttonEvt_t*);

int getTouchCentroid(int32_t* centerVal, int32_t* intensityVal);

#endif

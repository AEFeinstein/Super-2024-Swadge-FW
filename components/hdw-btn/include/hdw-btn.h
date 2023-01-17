#ifndef _BTN_H_
#define _BTN_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

typedef enum __attribute__((packed))
{
    UP     = 0x01, //!< The up button's bit
    DOWN   = 0x02, //!< The down button's bit
    LEFT   = 0x04, //!< The left button's bit
    RIGHT  = 0x08, //!< The right button's bit
    BTN_A  = 0x10, //!< The A button's bit
    BTN_B  = 0x20, //!< The B button's bit
    START  = 0x40, //!< The start button's bit
    SELECT = 0x80  //!< The select button's bit
} buttonBit_t;

typedef struct
{
    uint16_t state;     //!< A bitmask for the state of all buttons
    buttonBit_t button; //!< The button that caused this event
    bool down;          //!< True if the button was pressed, false if it was released
} buttonEvt_t;

void initButtons(uint8_t numButtons, ...);
void deinitButtons(void);
bool checkButtonQueue(buttonEvt_t*);

#endif

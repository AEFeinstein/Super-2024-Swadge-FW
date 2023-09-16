#pragma once

#include "tinyusb.h"

#define TUD_HID_REPORT_DESC_GAMEPAD(x)

#define TUD_CONFIG_DESC_LEN (9)

// Config number, interface count, string index, total length, attribute, power in mA
#define TUD_CONFIG_DESCRIPTOR(config_num, _itfcount, _stridx, _total_len, _attribute, _power_ma) \
    config_num, _itfcount, _stridx, _total_len, _attribute, _power_ma
#define TUD_HID_DESCRIPTOR(_itfnum, _stridx, _boot_protocol, _report_desc_len, _epin, _epsize, _ep_interval) \
    _itfnum, _stridx, _boot_protocol, _report_desc_len, _epin, _epsize, _ep_interval

#define CFG_TUD_HID      1
#define TUD_HID_DESC_LEN (9 + 9 + 7)

enum
{
    TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP = TU_BIT(5),
    TUSB_DESC_CONFIG_ATT_SELF_POWERED  = TU_BIT(6),
};

//--------------------------------------------------------------------+
// GAMEPAD
//--------------------------------------------------------------------+
/** \addtogroup ClassDriver_HID_Gamepad Gamepad
 *  @{ */

/* From https://www.kernel.org/doc/html/latest/input/gamepad.html
          ____________________________              __
         / [__ZL__]          [__ZR__] \               |
        / [__ TL __]        [__ TR __] \              | Front Triggers
     __/________________________________\__         __|
    /                                  _   \          |
   /      /\           __             (N)   \         |
  /       ||      __  |MO|  __     _       _ \        | Main Pad
 |    <===DP===> |SE|      |ST|   (W) -|- (E) |       |
  \       ||    ___          ___       _     /        |
  /\      \/   /   \        /   \     (S)   /\      __|
 /  \________ | LS  | ____ |  RS | ________/  \       |
|         /  \ \___/ /    \ \___/ /  \         |      | Control Sticks
|        /    \_____/      \_____/    \        |    __|
|       /                              \       |
 \_____/                                \_____/

     |________|______|    |______|___________|
       D-Pad    Left       Right   Action Pad
               Stick       Stick

                 |_____________|
                    Menu Pad

  Most gamepads have the following features:
  - Action-Pad 4 buttons in diamonds-shape (on the right side) NORTH, SOUTH, WEST and EAST.
  - D-Pad (Direction-pad) 4 buttons (on the left side) that point up, down, left and right.
  - Menu-Pad Different constellations, but most-times 2 buttons: SELECT - START.
  - Analog-Sticks provide freely moveable sticks to control directions, Analog-sticks may also
  provide a digital button if you press them.
  - Triggers are located on the upper-side of the pad in vertical direction. The upper buttons
  are normally named Left- and Right-Triggers, the lower buttons Z-Left and Z-Right.
  - Rumble Many devices provide force-feedback features. But are mostly just simple rumble motors.
 */

/// HID Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED
{
    int8_t x;         ///< Delta x  movement of left analog-stick
    int8_t y;         ///< Delta y  movement of left analog-stick
    int8_t z;         ///< Delta z  movement of right analog-joystick
    int8_t rz;        ///< Delta Rz movement of right analog-joystick
    int8_t rx;        ///< Delta Rx movement of analog left trigger
    int8_t ry;        ///< Delta Ry movement of analog right trigger
    uint8_t hat;      ///< Buttons mask for currently pressed buttons in the DPad/hat
    uint32_t buttons; ///< Buttons mask for currently pressed buttons
} hid_gamepad_report_t;

/// Standard Gamepad Buttons Bitmap
typedef enum
{
    GAMEPAD_BUTTON_0  = TU_BIT(0),
    GAMEPAD_BUTTON_1  = TU_BIT(1),
    GAMEPAD_BUTTON_2  = TU_BIT(2),
    GAMEPAD_BUTTON_3  = TU_BIT(3),
    GAMEPAD_BUTTON_4  = TU_BIT(4),
    GAMEPAD_BUTTON_5  = TU_BIT(5),
    GAMEPAD_BUTTON_6  = TU_BIT(6),
    GAMEPAD_BUTTON_7  = TU_BIT(7),
    GAMEPAD_BUTTON_8  = TU_BIT(8),
    GAMEPAD_BUTTON_9  = TU_BIT(9),
    GAMEPAD_BUTTON_10 = TU_BIT(10),
    GAMEPAD_BUTTON_11 = TU_BIT(11),
    GAMEPAD_BUTTON_12 = TU_BIT(12),
    GAMEPAD_BUTTON_13 = TU_BIT(13),
    GAMEPAD_BUTTON_14 = TU_BIT(14),
    GAMEPAD_BUTTON_15 = TU_BIT(15),
    GAMEPAD_BUTTON_16 = TU_BIT(16),
    GAMEPAD_BUTTON_17 = TU_BIT(17),
    GAMEPAD_BUTTON_18 = TU_BIT(18),
    GAMEPAD_BUTTON_19 = TU_BIT(19),
    GAMEPAD_BUTTON_20 = TU_BIT(20),
    GAMEPAD_BUTTON_21 = TU_BIT(21),
    GAMEPAD_BUTTON_22 = TU_BIT(22),
    GAMEPAD_BUTTON_23 = TU_BIT(23),
    GAMEPAD_BUTTON_24 = TU_BIT(24),
    GAMEPAD_BUTTON_25 = TU_BIT(25),
    GAMEPAD_BUTTON_26 = TU_BIT(26),
    GAMEPAD_BUTTON_27 = TU_BIT(27),
    GAMEPAD_BUTTON_28 = TU_BIT(28),
    GAMEPAD_BUTTON_29 = TU_BIT(29),
    GAMEPAD_BUTTON_30 = TU_BIT(30),
    GAMEPAD_BUTTON_31 = TU_BIT(31),
} hid_gamepad_button_bm_t;

/// Standard Gamepad Buttons Naming from Linux input event codes
/// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
#define GAMEPAD_BUTTON_A     GAMEPAD_BUTTON_0
#define GAMEPAD_BUTTON_SOUTH GAMEPAD_BUTTON_0

#define GAMEPAD_BUTTON_B    GAMEPAD_BUTTON_1
#define GAMEPAD_BUTTON_EAST GAMEPAD_BUTTON_1

#define GAMEPAD_BUTTON_C GAMEPAD_BUTTON_2

#define GAMEPAD_BUTTON_X     GAMEPAD_BUTTON_3
#define GAMEPAD_BUTTON_NORTH GAMEPAD_BUTTON_3

#define GAMEPAD_BUTTON_Y    GAMEPAD_BUTTON_4
#define GAMEPAD_BUTTON_WEST GAMEPAD_BUTTON_4

#define GAMEPAD_BUTTON_Z      GAMEPAD_BUTTON_5
#define GAMEPAD_BUTTON_TL     GAMEPAD_BUTTON_6
#define GAMEPAD_BUTTON_TR     GAMEPAD_BUTTON_7
#define GAMEPAD_BUTTON_TL2    GAMEPAD_BUTTON_8
#define GAMEPAD_BUTTON_TR2    GAMEPAD_BUTTON_9
#define GAMEPAD_BUTTON_SELECT GAMEPAD_BUTTON_10
#define GAMEPAD_BUTTON_START  GAMEPAD_BUTTON_11
#define GAMEPAD_BUTTON_MODE   GAMEPAD_BUTTON_12
#define GAMEPAD_BUTTON_THUMBL GAMEPAD_BUTTON_13
#define GAMEPAD_BUTTON_THUMBR GAMEPAD_BUTTON_14

/// Standard Gamepad HAT/DPAD Buttons (from Linux input event codes)
typedef enum
{
    GAMEPAD_HAT_CENTERED   = 0, ///< DPAD_CENTERED
    GAMEPAD_HAT_UP         = 1, ///< DPAD_UP
    GAMEPAD_HAT_UP_RIGHT   = 2, ///< DPAD_UP_RIGHT
    GAMEPAD_HAT_RIGHT      = 3, ///< DPAD_RIGHT
    GAMEPAD_HAT_DOWN_RIGHT = 4, ///< DPAD_DOWN_RIGHT
    GAMEPAD_HAT_DOWN       = 5, ///< DPAD_DOWN
    GAMEPAD_HAT_DOWN_LEFT  = 6, ///< DPAD_DOWN_LEFT
    GAMEPAD_HAT_LEFT       = 7, ///< DPAD_LEFT
    GAMEPAD_HAT_UP_LEFT    = 8, ///< DPAD_UP_LEFT
} hid_gamepad_hat_t;

/// @}
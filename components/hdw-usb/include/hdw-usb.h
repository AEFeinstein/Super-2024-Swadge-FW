/*! \file hdw-usb.h
 *
 * \section usb_design Design Philosophy
 *
 * The USB component uses Espressif's <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32s2/api-reference/peripherals/usb_device.html">USB
 * Device Driver</a>. It's based on the <a
 * href="https://github.com/espressif/esp-idf/tree/v5.2.1/examples/peripherals/usb/device/tusb_hid">TinyUSB
 * Human Interface Device Example</a>.
 *
 * The Swadge primarily functions as a USB gamepad.
 *
 * The Swadge can also exchange USB data with a host by setting a Swadge mode's ::swadgeMode_t.fnAdvancedUSB function
 * pointer. If that is set, then USB \c SET_REPORT and \c GET_REPORT messages with \c report_id == 173 will be sent to
 * that function.
 *
 * For development purposes, the Swadge has advanced USB capabilities to load and execute code from RAM. These
 * capabilites can be read about in advanced_usb_control.h.
 *
 * \section usb_usage Usage
 *
 * You don't need to call initUsb() or deinitUsb(). The system does this the appropriate time.
 *
 * sendUsbGamepadReport() should be called whenever there is an updated gamepad state to be sent to the host.
 *
 * usbSetSwadgeMode() should NOT be called, except during development.
 *
 * \section usb_example Example
 *
 * \code{.c}
 * static hid_gamepad_report_t report;
 * report.buttons = lastBtnState;
 * sendUsbGamepadReport(&report);
 * \endcode
 */

#ifndef _HDW_USB_
#define _HDW_USB_

//==============================================================================
// Includes
//==============================================================================

#include <class/hid/hid.h>
#include "tinyusb.h"

//==============================================================================
// Enums
//==============================================================================

/// Switch Gamepad Buttons Bitmap
typedef enum
{
    GAMEPAD_NS_BUTTON_Y       = 0x01,
    GAMEPAD_NS_BUTTON_B       = 0x02,
    GAMEPAD_NS_BUTTON_A       = 0x04,
    GAMEPAD_NS_BUTTON_X       = 0x08,
    GAMEPAD_NS_BUTTON_TL      = 0x10,
    GAMEPAD_NS_BUTTON_TR      = 0x20,
    GAMEPAD_NS_BUTTON_TL2     = 0x40,
    GAMEPAD_NS_BUTTON_TR2     = 0x80,
    GAMEPAD_NS_BUTTON_MINUS   = 0x100,
    GAMEPAD_NS_BUTTON_PLUS    = 0x200,
    GAMEPAD_NS_BUTTON_THUMBL  = 0x400,
    GAMEPAD_NS_BUTTON_THUMBR  = 0x800,
    GAMEPAD_NS_BUTTON_HOME    = 0x1000,
    GAMEPAD_NS_BUTTON_CAPTURE = 0x2000,
    GAMEPAD_NS_BUTTON_Z       = 0x4000, /// UNUSED?
} hid_gamepad_ns_button_bm_t;

/// Switch Gamepad HAT/DPAD Buttons (from Linux input event codes)
typedef enum
{
    GAMEPAD_NS_HAT_CENTERED   = 8, ///< DPAD_CENTERED
    GAMEPAD_NS_HAT_UP         = 0, ///< DPAD_UP
    GAMEPAD_NS_HAT_UP_RIGHT   = 1, ///< DPAD_UP_RIGHT
    GAMEPAD_NS_HAT_RIGHT      = 2, ///< DPAD_RIGHT
    GAMEPAD_NS_HAT_DOWN_RIGHT = 3, ///< DPAD_DOWN_RIGHT
    GAMEPAD_NS_HAT_DOWN       = 4, ///< DPAD_DOWN
    GAMEPAD_NS_HAT_DOWN_LEFT  = 5, ///< DPAD_DOWN_LEFT
    GAMEPAD_NS_HAT_LEFT       = 6, ///< DPAD_LEFT
    GAMEPAD_NS_HAT_UP_LEFT    = 7, ///< DPAD_UP_LEFT
} hid_gamepad_ns_hat_t;

//==============================================================================
// Structs
//==============================================================================

/// HID Switch Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED
{
    uint16_t buttons; ///< Buttons mask for currently pressed buttons
    uint8_t hat;      ///< Buttons mask for currently pressed buttons in the DPad/hat
    int8_t x;         ///< Delta x  movement of left analog-stick
    int8_t y;         ///< Delta y  movement of left analog-stick
    int8_t rx;        ///< Delta Rx movement of analog left trigger
    int8_t ry;        ///< Delta Ry movement of analog right trigger
    int8_t z;         ///< Delta z  movement of right analog-joystick
    int8_t rz;        ///< Delta Rz movement of right analog-joystick
} hid_gamepad_ns_report_t;

//==============================================================================
// Function typedefs
//==============================================================================

/**
 * @brief Function typedef for a callback which will send USB SET_REPORT and GET_REPORT messages to a Swadge mode
 *
 * @param buffer Pointer to full command
 * @param length Total length of the buffer (command ID included)
 * @param isGet 0 if this is a \c SET_REPORT, 1 if this is a \c GET_REPORT
 * @return The number of bytes returned to the host
 */
typedef int16_t (*fnAdvancedUsbHandler)(uint8_t* buffer, uint16_t length, uint8_t isGet);

/**
 * @brief Function typedef for a function which will switch the Swadge mode
 * @param mode A pointer to the new Swadge mode. Be very careful with this, it will execute whatever's at this address.
 */
typedef void (*fnSetSwadgeMode)(void* mode);

//==============================================================================
// Function Prototypes
//==============================================================================

void initUsb(fnSetSwadgeMode _setSwadgeMode, fnAdvancedUsbHandler _advancedUsbHandler, bool redirectPrintf);
void deinitUsb(void);
void sendUsbGamepadReport(hid_gamepad_report_t* report);
void usbSetSwadgeMode(void* newMode);
void initTusb(const tinyusb_config_t* tusb_cfg, const uint8_t* descriptor);
bool tud_hid_gamepad_report_ns(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                               uint8_t hat, uint16_t buttons);

#endif
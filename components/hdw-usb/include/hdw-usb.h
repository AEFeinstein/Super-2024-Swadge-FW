/*! \file hdw-usb.h
 *
 * \section usb_design Design Philosophy
 *
 * The USB component uses Espressif's <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/api-reference/peripherals/usb_device.html">USB
 * Device Driver</a>. It's based on the <a
 * href="https://github.com/espressif/esp-idf/tree/release/v5.0.1/examples/peripherals/usb/device/tusb_hid">TinyUSB
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
 * #include "hdw-usb.h"
 *
 * ...
 *
 * static hid_gamepad_report_t report;
 * report.buttons = lastBtnState;
 * sendUsbGamepadReport(&report);
 * \endcode
 */

#ifndef _HDW_USB_
#define _HDW_USB_

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"

/**
 * @brief Function typedef for a callback which will send USB SET_REPORT and GET_REPORT messages to a Swadge mode
 *
 * @param buffer Pointer to full command
 * @param length Total length of the buffer (command ID incldued)
 * @param isGet 0 if this is a \c SET_REPORT, 1 if this is a \c GET_REPORT
 * @return The number of bytes returned to the host
 */
typedef int16_t (*fnAdvancedUsbHandler)(uint8_t* buffer, uint16_t length, uint8_t isGet);
/**
 * @brief Function typedef for a function which will switch the Swadge mode
 * @param mode A pointer to the new Swadge mode. Be very careful with this, it will execute whatever's at this address.
 */
typedef void (*fnSetSwadgeMode)(void* mode);

void initUsb(fnSetSwadgeMode setSwadgeMode, fnAdvancedUsbHandler advancedUsbHandler);
void deinitUsb(void);
void sendUsbGamepadReport(hid_gamepad_report_t* report);
void usbSetSwadgeMode(void* newMode);

#endif
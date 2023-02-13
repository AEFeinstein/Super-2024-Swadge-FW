/*! \file hdw-usb.h
 *
 * \section usb_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section usb_usage Usage
 *
 * TODO doxygen
 *
 * \section usb_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _HDW_USB_
#define _HDW_USB_

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"

void initUsb(void);
void sendUsbGamepadReport(hid_gamepad_report_t* report);

#endif
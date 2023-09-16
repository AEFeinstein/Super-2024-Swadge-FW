//==============================================================================
// Includes
//==============================================================================

#include "hdw-usb.h"
#include "emu_main.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize USB HID device
 *
 * @param _setSwadgeMode A function that can be called from this component to set the Swadge mode
 * @param _advancedUsbHandler A function that can be called from this component to handle USB commands
 */
void initUsb(fnSetSwadgeMode _setSwadgeMode, fnAdvancedUsbHandler _advancedUsbHandler)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Uninitialize USB HID device
 * Note, this does nothing as tinyusb_driver_uninstall() doesn't exist
 */
void deinitUsb(void)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Send a USB gamepad report to the system
 *
 * @param report The report to send, the current state of all gamepad inputs
 */
void sendUsbGamepadReport(hid_gamepad_report_t* report)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Initialize TinyUSB
 *
 * @param tusb_cfg The TinyUSB configuration
 * @param descriptor The descriptor to use for this configuration
 */
void initTusb(const tinyusb_config_t* tusb_cfg, const uint8_t* descriptor)
{
    WARN_UNIMPLEMENTED();
}
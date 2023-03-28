//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <class/hid/hid_device.h>

#include "tinyusb.h"
#include "hdw-usb.h"
#include "advanced_usb_control.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* TAG = "USB";

/**
 * @brief HID report descriptor, just a gamepad
 */
static const uint8_t hid_report_descriptor[] = {TUD_HID_REPORT_DESC_GAMEPAD()};

/**
 * @brief String descriptor
 */
static const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "Magfest",              // 1: Manufacturer
    "Swadge Controller",    // 2: Product
    "123456",               // 3: Serials, should use chip ID
    "Swadge HID interface", // 4: HID
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1,                                                        // Configuration number
                          1,                                                        // interface count
                          0,                                                        // string index
                          (TUD_CONFIG_DESC_LEN + (CFG_TUD_HID * TUD_HID_DESC_LEN)), // total length
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,                       // attribute
                          100),                                                     // power in mA

    TUD_HID_DESCRIPTOR(0,                             // Interface number
                       4,                             // string index
                       false,                         // boot protocol
                       sizeof(hid_report_descriptor), // report descriptor len
                       0x81,                          // EP In address
                       16,                            // size
                       10),                           // polling interval
};

//==============================================================================
// Variables
//==============================================================================

static fnAdvancedUsbHandler advancedUsbHandler;
static fnSetSwadgeMode setSwadgeMode;

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
    ESP_LOGI(TAG, "USB initialization");

    // Save the function pointers
    advancedUsbHandler = _advancedUsbHandler;
    setSwadgeMode      = _setSwadgeMode;

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = hid_string_descriptor,
        .external_phy             = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Set the log to print with advanced_usb_write_log_printf()
    esp_log_set_vprintf(advanced_usb_write_log_printf);

    ESP_LOGI(TAG, "USB initialization DONE");
}

/**
 * @brief Deinitialize USB HID device
 * Note, this does nothing as tinyusb_driver_uninstall() doesn't exist
 */
void deinitUsb(void)
{
    return;
}

/**
 * @brief Send a USB gamepad report to the system
 *
 * @param report The report to send, the current state of all gamepad inputs
 */
void sendUsbGamepadReport(hid_gamepad_report_t* report)
{
    if (tud_ready())
    {
        tud_hid_gamepad_report(HID_ITF_PROTOCOL_NONE, report->x, report->y, report->z, report->rx, report->ry,
                               report->rz, report->hat, report->buttons);
    }
}

//==============================================================================
// TinyUSB HID callbacks
//==============================================================================

/**
 * Invoked when received GET HID REPORT DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 *
 * @param instance The index to get a descriptor report for
 * @return The descriptor report for the given index
 */
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance __attribute__((unused)))
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

/**
 * Invoked when received GET_REPORT control request
 * Application must fill buffer report's content and return its length.
 * Return zero will cause the stack to STALL request
 *
 * @param instance The endpoint index to handle a GET_REPORT
 * @param report_id The report ID
 * @param report_type Unused
 * @param buffer Pointer to a feature get request for the command set.
 * @param reqLen Number of bytes host is requesting from us.
 * @return The number of bytes returned to the host
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type __attribute__((unused)), uint8_t* buffer, uint16_t reqLen)
{
    if (report_id == 170 || report_id == 171)
    {
        return handle_advanced_usb_control_get(reqLen, buffer);
    }
    else if (report_id == 172)
    {
        return handle_advanced_usb_terminal_get(reqLen, buffer);
    }
    else if (report_id == 173 && advancedUsbHandler)
    {
        return advancedUsbHandler(buffer, reqLen, 1);
    }
    else
    {
        return reqLen;
    }
}

/**
 * Invoked when received SET_REPORT control request or
 * received data on OUT endpoint ( Report ID = 0, Type = 0 )
 *
 * @param instance The endpoint index to handle a SET_REPORT request
 * @param report_id The report ID
 * @param report_type unused
 * @param buffer Pointer to full command
 * @param bufsize Total length of the buffer (command ID included)
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type __attribute__((unused)),
                           uint8_t const* buffer, uint16_t bufsize)
{
    if (report_id >= 170 && report_id <= 171)
    {
        handle_advanced_usb_control_set(bufsize, buffer);
    }
    else if (report_id == 173 && advancedUsbHandler)
    {
        advancedUsbHandler((uint8_t*)buffer, bufsize, 0);
    }
}

/**
 * @brief Set the Swadge mode to the given pointer
 *
 * @param newMode A pointer to the new mode to set
 */
void usbSetSwadgeMode(void* newMode)
{
    setSwadgeMode(newMode);
}

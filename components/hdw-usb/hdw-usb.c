/*! \file hdw-usb.c
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

//==============================================================================
// Includes
//==============================================================================

#include "esp_log.h"
#include "tinyusb.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* TAG = "example";

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
// Functions
//==============================================================================

/**
 * @brief TODO doxygen
 *
 */
void initUsb(void)
{
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = hid_string_descriptor,
        .external_phy             = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
}

/**
 * @brief TODO doxygen
 *
 */
void sendUsbGamepadReport(hid_gamepad_report_t* report)
{
    tud_hid_gamepad_report(HID_ITF_PROTOCOL_NONE, report->x, report->y, report->z, report->rx, report->ry, report->rz,
                           report->hat, report->buttons);
}

//==============================================================================
// TinyUSB HID callbacks
//==============================================================================

/**
 * Invoked when received GET HID REPORT DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 *
 * TODO members
 * @param instance
 * @return uint8_t const*
 */
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    (void)instance;
    return hid_report_descriptor;
}

/**
 * Invoked when received GET_REPORT control request
 * Application must fill buffer report's content and return its length.
 * Return zero will cause the stack to STALL request
 *
 * TODO members
 * @param instance
 * @param report_id
 * @param report_type
 * @param buffer
 * @param reqlen
 * @return uint16_t
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

/**
 * Invoked when received SET_REPORT control request or
 * received data on OUT endpoint ( Report ID = 0, Type = 0 )
 *
 * TODO members
 * @param instance
 * @param report_id
 * @param report_type
 * @param buffer
 * @param bufsize
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

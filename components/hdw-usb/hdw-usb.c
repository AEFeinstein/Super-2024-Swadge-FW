//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <esp_mac.h>
#include <class/hid/hid_device.h>

#include "hdw-usb.h"
#include "advanced_usb_control.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* TAG = "USB";

// clang-format off
/**
 * @brief HID Gamepad Report Descriptor Template
 * with 32 buttons, 2 joysticks and 1 hat/dpad with following layout
 * | X | Y | Z | Rz | Rx | Ry (1 byte each) | hat/DPAD (1 byte) | Button Map (4 bytes) |
 */
#define TUD_HID_REPORT_DESC_GAMEPAD_SWADGE(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
	HID_REPORT_ID( /*REPORT_ID_GAMEPAD*/ 0x01 ) \
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min -127, max 127 ) */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN  ( 0x81                                   ) ,\
    HID_LOGICAL_MAX  ( 0x7f                                   ) ,\
    HID_REPORT_COUNT ( 6                                      ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN  ( 1                                      ) ,\
    HID_LOGICAL_MAX  ( 8                                      ) ,\
    HID_PHYSICAL_MIN ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                               ) ,\
    HID_REPORT_COUNT ( 1                                      ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 16 bit Button Map */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN    ( 1                                      ) ,\
    HID_USAGE_MAX    ( 16                                     ) ,\
    HID_LOGICAL_MIN  ( 0                                      ) ,\
    HID_LOGICAL_MAX  ( 1                                      ) ,\
    HID_REPORT_COUNT ( 16                                     ) ,\
    HID_REPORT_SIZE  ( 1                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* Allow for 0xaa (regular size), 0xab (jumbo sized) and 0xac mini feature reports; Windows needs specific id'd and size'd endpoints. */ \
    HID_REPORT_COUNT ( CFG_TUD_ENDPOINT0_SIZE                 ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_REPORT_ID    ( 0xaa                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( (255-1)         ) ,\
    HID_REPORT_ID    ( 0xab                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( 1                                      ) ,\
    HID_REPORT_ID    ( 0xac                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( (255-1)         ) ,\
    HID_REPORT_ID    ( 0xad                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
  HID_COLLECTION_END
// clang-format on

static const uint8_t hid_report_descriptor[] = {TUD_HID_REPORT_DESC_GAMEPAD_SWADGE()};

/**
 * @brief String descriptor
 */
static char* hid_string_descriptor[7] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "Magfest",              // 1: Manufacturer
    "Swadge Controller",    // 2: Product
    "123456",               // 3: Serials, overwritten with chip ID
    "Swadge HID interface", // 4: HID

    // Tricky keep these symbols, for sandboxing.  These are not used.  But, by keeping them here it makes them accessable via the sandbox.
    (char*)&tud_connect,
    (char*)&tud_disconnect
};

/**
 * @brief Unique serial number.
 */
static char serial_string[13];

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
// Function declarations
//==============================================================================

static bool tud_hid_n_gamepad_report_ns(uint8_t instance, uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz,
                                        int8_t rx, int8_t ry, uint8_t hat, uint16_t buttons);

//==============================================================================
// Variables
//==============================================================================

static fnAdvancedUsbHandler advancedUsbHandler;
static fnSetSwadgeMode setSwadgeMode;
static const uint8_t* c_descriptor;

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

    uint8_t mac[6];
    esp_err_t e = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (e == ESP_OK)
    {
        snprintf(serial_string, sizeof(serial_string), "%02x%02x%02x%02x%02x%02x", (int)mac[0], (int)mac[1], (int)mac[2],
            (int)mac[3], (int)mac[4], (int)mac[5]);
        hid_string_descriptor[3] = serial_string;
    }

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = hid_string_descriptor,
        .external_phy             = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    // Initialize TinyUSB with the default descriptor
    initTusb(&tusb_cfg, hid_report_descriptor);

    // Set the log to print with advanced_usb_write_log_printf()
    esp_log_set_vprintf(advanced_usb_write_log_printf);

    ESP_LOGI(TAG, "USB initialization DONE");
}

/**
 * @brief Initialize TinyUSB
 *
 * @param tusb_cfg The TinyUSB configuration
 * @param descriptor The descriptor to use for this configuration
 */
void initTusb(const tinyusb_config_t* tusb_cfg, const uint8_t* descriptor)
{
    c_descriptor = descriptor;
    ESP_ERROR_CHECK(tinyusb_driver_install(tusb_cfg));
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

/**
 * @brief Send a USB gamepad report to a Switch, with instance
 *
 * @param instance Should be 0
 * @param report_id Should be HID_ITF_PROTOCOL_NONE
 * @param x Unsure
 * @param y Unsure
 * @param z Unsure
 * @param rz Unsure
 * @param rx Unsure
 * @param ry Unsure
 * @param hat A bitmask of ::hid_gamepad_ns_hat_t
 * @param buttons A bitmask of ::hid_gamepad_ns_button_bm_t
 * @return Unsure
 */
static bool tud_hid_n_gamepad_report_ns(uint8_t instance, uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz,
                                        int8_t rx, int8_t ry, uint8_t hat, uint16_t buttons)
{
    hid_gamepad_ns_report_t report = {
        .buttons = buttons,
        .hat     = hat,
        .x       = x,
        .y       = y,
        .rx      = rx,
        .ry      = ry,
        .z       = z,
        .rz      = rz,
    };

    return tud_hid_n_report(instance, report_id, &report, sizeof(report));
}

/**
 * @brief Send a USB gamepad report to a Switch
 *
 * @param report_id Should be HID_ITF_PROTOCOL_NONE
 * @param x Unsure
 * @param y Unsure
 * @param z Unsure
 * @param rz Unsure
 * @param rx Unsure
 * @param ry Unsure
 * @param hat A bitmask of ::hid_gamepad_ns_hat_t
 * @param buttons A bitmask of ::hid_gamepad_ns_button_bm_t
 * @return Unsure
 */
bool tud_hid_gamepad_report_ns(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                               uint8_t hat, uint16_t buttons)
{
    return tud_hid_n_gamepad_report_ns(0, report_id, x, y, z, rz, rx, ry, hat, buttons);
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
    return c_descriptor;
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
        return handle_advanced_usb_control_get(buffer - 1, reqLen + 1);
    }
    else if (report_id == 172)
    {
        return handle_advanced_usb_terminal_get(buffer - 1, reqLen + 1);
    }
    else if (report_id == 173 && advancedUsbHandler)
    {
        return advancedUsbHandler(buffer - 1, reqLen + 1, 1);
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
        handle_advanced_usb_control_set(buffer - 1, bufsize + 1);
    }
    else if (report_id == 173 && advancedUsbHandler)
    {
        advancedUsbHandler((uint8_t*)buffer - 1, bufsize + 1, 0);
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

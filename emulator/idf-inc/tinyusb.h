#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

//--------------------------------------------------------------------+
// Macros Helper
//--------------------------------------------------------------------+
#define TU_ARRAY_SIZE(_arr) (sizeof(_arr) / sizeof(_arr[0]))
#define TU_MIN(_x, _y)      (((_x) < (_y)) ? (_x) : (_y))
#define TU_MAX(_x, _y)      (((_x) > (_y)) ? (_x) : (_y))

#define TU_U16(_high, _low) ((uint16_t)(((_high) << 8) | (_low)))
#define TU_U16_HIGH(_u16)   ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define TU_U16_LOW(_u16)    ((uint8_t)((_u16) & 0x00ff))
#define U16_TO_U8S_BE(_u16) TU_U16_HIGH(_u16), TU_U16_LOW(_u16)
#define U16_TO_U8S_LE(_u16) TU_U16_LOW(_u16), TU_U16_HIGH(_u16)

#define TU_U32_BYTE3(_u32) ((uint8_t)((((uint32_t)_u32) >> 24) & 0x000000ff)) // MSB
#define TU_U32_BYTE2(_u32) ((uint8_t)((((uint32_t)_u32) >> 16) & 0x000000ff))
#define TU_U32_BYTE1(_u32) ((uint8_t)((((uint32_t)_u32) >> 8) & 0x000000ff))
#define TU_U32_BYTE0(_u32) ((uint8_t)(((uint32_t)_u32) & 0x000000ff)) // LSB

#define U32_TO_U8S_BE(_u32) TU_U32_BYTE3(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE0(_u32)
#define U32_TO_U8S_LE(_u32) TU_U32_BYTE0(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE3(_u32)

#define TU_BIT(n)        (1UL << (n))
#define TU_GENMASK(h, l) ((UINT32_MAX << (l)) & (UINT32_MAX >> (31 - (h))))

#define TU_ATTR_PACKED __attribute__((packed))

/// USB Device Descriptor
typedef struct TU_ATTR_PACKED
{
    uint8_t bLength;         ///< Size of this descriptor in bytes.
    uint8_t bDescriptorType; ///< DEVICE Descriptor Type.
    uint16_t bcdUSB; ///< BUSB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H). This field
                     ///< identifies the release of the USB Specification with which the device and its descriptors are
                     ///< compliant.

    uint8_t
        bDeviceClass; ///< Class code (assigned by the USB-IF). \li If this field is reset to zero, each interface
                      ///< within a configuration specifies its own class information and the various interfaces operate
                      ///< independently. \li If this field is set to a value between 1 and FEH, the device supports
                      ///< different class specifications on different interfaces and the interfaces may not operate
                      ///< independently. This value identifies the class definition used for the aggregate interfaces.
                      ///< \li If this field is set to FFH, the device class is vendor-specific.
    uint8_t bDeviceSubClass; ///< Subclass code (assigned by the USB-IF). These codes are qualified by the value of the
                             ///< bDeviceClass field. \li If the bDeviceClass field is reset to zero, this field must
                             ///< also be reset to zero. \li If the bDeviceClass field is not set to FFH, all values are
                             ///< reserved for assignment by the USB-IF.
    uint8_t
        bDeviceProtocol; ///< Protocol code (assigned by the USB-IF). These codes are qualified by the value of the
                         ///< bDeviceClass and the bDeviceSubClass fields. If a device supports class-specific protocols
                         ///< on a device basis as opposed to an interface basis, this code identifies the protocols
                         ///< that the device uses as defined by the specification of the device class. \li If this
                         ///< field is reset to zero, the device does not use class-specific protocols on a device
                         ///< basis. However, it may use classspecific protocols on an interface basis. \li If this
                         ///< field is set to FFH, the device uses a vendor-specific protocol on a device basis.
    uint8_t bMaxPacketSize0; ///< Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid). For HS
                             ///< devices is fixed to 64.

    uint16_t idVendor;     ///< Vendor ID (assigned by the USB-IF).
    uint16_t idProduct;    ///< Product ID (assigned by the manufacturer).
    uint16_t bcdDevice;    ///< Device release number in binary-coded decimal.
    uint8_t iManufacturer; ///< Index of string descriptor describing manufacturer.
    uint8_t iProduct;      ///< Index of string descriptor describing product.
    uint8_t iSerialNumber; ///< Index of string descriptor describing the device's serial number.

    uint8_t bNumConfigurations; ///< Number of possible configurations.
} tusb_desc_device_t;

/**
 * @brief Configuration structure of the TinyUSB core
 *
 * USB specification mandates self-powered devices to monitor USB VBUS to detect connection/disconnection events.
 * If you want to use this feature, connected VBUS to any free GPIO through a voltage divider or voltage comparator.
 * The voltage divider output should be (0.75 * Vdd) if VBUS is 4.4V (lowest valid voltage at device port).
 * The comparator thresholds should be set with hysteresis: 4.35V (falling edge) and 4.75V (raising edge).
 */
typedef struct
{
    union
    {
        const tusb_desc_device_t*
            device_descriptor; /*!< Pointer to a device descriptor. If set to NULL, the TinyUSB device will use a
                                  default device descriptor whose values are set in Kconfig */
        const tusb_desc_device_t* descriptor
            __attribute__((deprecated)); /*!< Alias to `device_descriptor` for backward compatibility */
    };
    const char** string_descriptor; /*!< Pointer to array of string descriptors. If set to NULL, TinyUSB device will use
                                       a default string descriptors whose values are set in Kconfig */
    int string_descriptor_count;    /*!< Number of descriptors in above array */
    bool external_phy;              /*!< Should USB use an external PHY */
    const uint8_t*
        configuration_descriptor; /*!< Pointer to a configuration descriptor. If set to NULL, TinyUSB device will use a
                                     default configuration descriptor whose values are set in Kconfig */
    bool self_powered;            /*!< This is a self-powered USB device. USB VBUS must be monitored. */
    int vbus_monitor_io;          /*!< GPIO for VBUS monitoring. Ignored if not self_powered. */
} tinyusb_config_t;

/// HID Interface Protocol
typedef enum
{
    HID_ITF_PROTOCOL_NONE     = 0, ///< None
    HID_ITF_PROTOCOL_KEYBOARD = 1, ///< Keyboard
    HID_ITF_PROTOCOL_MOUSE    = 2  ///< Mouse
} hid_interface_protocol_enum_t;

esp_err_t tinyusb_driver_install(const tinyusb_config_t* config);
bool tud_ready(void);
bool tud_hid_gamepad_report(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                            uint8_t hat, uint32_t buttons);

#include "emu_main.h"
#include "tinyusb.h"

esp_err_t tinyusb_driver_install(const tinyusb_config_t* config)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

bool tud_ready(void)
{
    WARN_UNIMPLEMENTED();
    return true;
}

bool tud_hid_gamepad_report(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                            uint8_t hat, uint32_t buttons)
{
    WARN_UNIMPLEMENTED();
    return true;
}

bool tud_hid_gamepad_report_ns(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                               uint8_t hat, uint16_t buttons)
{
    WARN_UNIMPLEMENTED();
    return true;
}
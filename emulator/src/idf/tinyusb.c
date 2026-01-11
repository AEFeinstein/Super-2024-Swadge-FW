#include "emu_main.h"
#include "tinyusb.h"
#include "midi_device.h"
#include "midi_device_emu.h"

#include <esp_log.h>

#include <string.h>

bool using_midi = false;

esp_err_t tinyusb_driver_install(const tinyusb_config_t* config)
{
    if (!strcmp("Swadge Synthesizer", config->string_descriptor[2]))
    {
        using_midi = true;
        setMidiClientName(config->string_descriptor[2]);
        midid_init();
    }
    else
    {
        WARN_UNIMPLEMENTED();
    }
    return ESP_OK;
}

bool tud_ready(void)
{
    // ESP_LOGI("TinyUSB", "tud_ready() returning %s", using_midi ? "true" : "false");
    return using_midi;
}

bool tud_hid_gamepad_report(uint8_t report_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry,
                            uint8_t hat, uint32_t buttons)
{
    WARN_UNIMPLEMENTED();
    return true;
}

#include "esp_sleep.h"
#include "swadge2024.h"

esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void)
{
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us)
{
    return ESP_OK;
}

void esp_deep_sleep_start(void)
{
    // On the emulator, this will switch the Swadge mode without rebooting
    // On an actual Swadge, this function will reboot the system and the new Swadge mode will be used after reboot
    softSwitchToPendingSwadge();
    return;
}
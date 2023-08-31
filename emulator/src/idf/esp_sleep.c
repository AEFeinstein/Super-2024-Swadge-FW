#include <unistd.h>
#include "esp_sleep.h"
#include "esp_sleep_emu.h"
#include "swadge2024.h"

static uint64_t timeToLightSleep = 0;
static bool modeLocked           = false;
static bool overrideLock         = false;

esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void)
{
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us)
{
    timeToLightSleep = time_in_us;
    return ESP_OK;
}

esp_err_t esp_light_sleep_start(void)
{
    usleep(timeToLightSleep);
    return ESP_OK;
}

void esp_deep_sleep_start(void)
{
    if (modeLocked && !overrideLock)
    {
        // Un-turn-off the backlight from when the mode switch was attempted
        enableTFTBacklight();
    }
    else
    {
        overrideLock = false;

        // On the emulator, this will switch the Swadge mode without rebooting
        // On an actual Swadge, this function will reboot the system and the new Swadge mode will be used after reboot
        softSwitchToPendingSwadge();
        return;
    }
}

/**
 * @brief Forcibly switch the emulator into a different swadge mode, even if it is locked.
 *
 * @param mode The swadge mode to force-switch into
 */
void emulatorForceSwitchToSwadgeMode(swadgeMode_t* mode)
{
    // Switch the swadge mode normally
    switchToSwadgeMode(mode);

    // Allow the next mode change to complete
    overrideLock = true;
}

/**
 * @brief Set whether the swadge mode may be changed as normally via ::switchToSwadgeMode()
 *
 * @param locked Whether or not the swadge mode should be locked
 */
void emulatorSetSwadgeModeLocked(bool locked)
{
    modeLocked   = locked;
    overrideLock = false;
}

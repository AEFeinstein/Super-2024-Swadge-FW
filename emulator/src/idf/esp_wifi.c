#include <string.h>

#include <esp_wifi.h>
#include "emu_main.h"
#include "esp_random.h"

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6])
{
    // Randomly generate MAC
    static uint8_t randMac[6] = {0};
    static bool macRandomized = false;
    if (!macRandomized)
    {
        macRandomized = true;
        randMac[0]    = esp_random();
        randMac[1]    = esp_random();
        randMac[2]    = esp_random();
        randMac[3]    = esp_random();
        randMac[4]    = esp_random();
        randMac[5]    = esp_random();
    }
    memcpy(mac, randMac, sizeof(randMac));
    return ESP_OK;
}

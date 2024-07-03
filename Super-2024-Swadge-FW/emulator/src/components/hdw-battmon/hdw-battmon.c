#include "emu_main.h"
#include "hdw-battmon.h"

/**
 * @brief Initialize a oneshot analog-to-digital unit to read the battery voltage
 *
 * @param gpio The GPIO to initialize the ADC on
 */
void initBattmon(gpio_num_t gpio)
{
    // nothing to do
}

/**
 * @brief Deinitialize the analog-to-digital unit used to read the battery voltage
 */
void deinitBattmon(void)
{
    // nothing to do
}

/**
 * @brief Read and return the battery voltage
 *
 * @return The voltage if read, or -1 if there was an error
 */
int readBattmon(void)
{
    return -1;
}

//==============================================================================
// Includes
//==============================================================================

#include "hdw-temperature.h"
#include "emu_main.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the ESP's onboard temperature sensor
 */
void initTemperatureSensor(void)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief deinitialize the ESP's onboard temperature sensor
 */
void deinitTemperatureSensor(void)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief
 */
void powerDownTemperatureSensor(void)
{
    ; // TODO LPM
}

/**
 * @brief
 */
void powerUpTemperatureSensor(void)
{
    ; // TODO LPM
}

/**
 * @brief Get a temperature reading from the ESP's onboard temperature sensor
 *
 * @return A floating point temperature, or -274 if not read
 */
float readTemperatureSensor(void)
{
    WARN_UNIMPLEMENTED();
    // Impossible!
    return -274;
}

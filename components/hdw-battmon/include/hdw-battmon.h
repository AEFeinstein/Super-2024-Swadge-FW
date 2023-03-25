/*! \file hdw-battmon.h
 *
 * \section battmon_design Design Philosophy
 *
 * The battery monitor uses the <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/api-reference/peripherals/adc_oneshot.html">Analog
 * to Digital Converter (ADC) Oneshot Mode Driver</a>.
 *
 * The battery monitor code is based on the <a
 * href="https://github.com/espressif/esp-idf/tree/v5.0.1/examples/peripherals/adc/oneshot_read">ADC Single Read
 * Example</a>.
 *
 * \warning The battery monitor and microphone (hdw-mic.h) cannot be used at the same time! Each mode can either
 * continuously sample the microphone or measure the battery voltage, not both.
 *
 * \section battmon_usage Usage
 *
 * You don't need to call initBattmon() or deinitBattmon(). The system does at the appropriate times.
 *
 * readBattmon() should be called to read the instantaneous battery voltage.
 *
 * \section battmon_example Example
 *
 * \code{.c}
 * printf("%d\n", readBattmon());
 * \endcode
 */

#include <hal/gpio_types.h>

void initBattmon(gpio_num_t gpio);
void deinitBattmon(void);
int readBattmon(void);

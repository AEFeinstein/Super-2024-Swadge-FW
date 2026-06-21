//==============================================================================
// Includes
//==============================================================================
#include <esp_log.h>
#include <string.h>
#include "static_i2c.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "hdw-aw32001e-bms.h"

//==============================================================================
// Defines
//==============================================================================

//Do I need to do this again? This is done in hw_imu.c.
#define DSCL_OUTPUT                             \
    {                                           \
        GPIO.enable1_w1ts.val = 1 << (41 - 32); \
    }
#define DSCL_INPUT                              \
    {                                           \
        GPIO.enable1_w1tc.val = 1 << (41 - 32); \
    }
#define DSDA_OUTPUT                  \
    {                                \
        GPIO.enable_w1ts = 1 << (3); \
    }
#define DSDA_INPUT                   \
    {                                \
        GPIO.enable_w1tc = 1 << (3); \
    }
#define READ_DSDA ((GPIO.in >> 3) & 1)

// 14 counts (1MHz) works most of the time, but no hurries, let's slow it down to ~800k.
void i2c_delay(int x)
{
    int i;
    for (i = 0; i < 19 * x; i++)
        asm volatile("nop");
}
#define DELAY1 i2c_delay(1);
#define DELAY2 i2c_delay(2);

//END Do I need to do this again?

//Uncomment the BMS settings you want to use. If both are uncommented, conservative, "safe" values will be used. 
#define DEFAULT_BMS_SETTINGS
//#define SWADGE_BMS_SETTINGS

#define AW32001_ADDRESS 0x49

//==============================================================================
// Internal Functions
//==============================================================================

/**
 * @brief Set a specific register on the AW32001 BMS to a value.
 *
 * @param dev The 7-bit address of the device to set the register to.
 * @param reg The 8-bit register #
 * @param val The 8-bit value to set the register to.
 * @return ESP_OK if the operation was successful.
 */
static esp_err_t AW32001Set(int dev, int reg, uint8_t val)
{
    SendStart();
    SendByte(dev << 1);
    SendByte(reg);
    SendByte(val);
    SendStop();
    return ESP_OK;
}

/**
 * @brief Read a buffer back from a specific I2C device.
 *
 * @param device The 7-bit device address
 * @param reg The 8-bit register # to start at.
 * @param data The buffer to load the data into.
 * @param data_len Number of bytes to read.
 * @return positive number if operation was successful, or esp_err_t if failure.
 */
static int GeneralI2CGet(int device, int reg, uint8_t* data, int data_len)
{
    SendStart();
    SendByte(device << 1);
    SendByte(reg);
    SendStart();
    SendByte((device << 1) | 1);
    int i;
    for (i = 0; i < data_len; i++)
    {
        data[i] = GetByte(i == data_len - 1);
    }
    SendStop();
    return data_len;
}


/**
 * @brief Initialize the BMS
 *
 * @param sda The GPIO for the Serial Data line
 * @param scl The GPIO for the Serial CLock line
 * @param pullup Either \c GPIO_PULLUP_DISABLE if there are external pullup resistors on SDA and SCL or \c
 * GPIO_PULLUP_ENABLE if internal pull-ups should be used
 * @return true if the BMS initialized, or false if it did not
 */
bool initBMS(gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup)
{
    int i;
    int retry = 0;
    do_retry:
    gpio_config_t gsetup = {
        .pin_bit_mask = (1ULL << sda) | (1ULL << scl),
        .mode         = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };

    gpio_config(&gsetup);

    // This will "shake loose" any devices stuck on the bus.
    GPIO.enable_w1ts      = 1 << (3);
    GPIO.enable1_w1ts.val = 1 << (41 - 32);
    esp_rom_delay_us(10);
    GPIO.out_w1tc = 1 << (3);
    for (i = 0; i < 16; i++)
    {
        esp_rom_delay_us(10);
        GPIO.out1_w1ts.val = 1 << (41 - 32);
        esp_rom_delay_us(10);
        GPIO.out1_w1tc.val = 1 << (41 - 32);
    }
    esp_rom_delay_us(10);
    GPIO.out1_w1ts.val = 1 << (41 - 32);
    esp_rom_delay_us(10);
    GPIO.out_w1ts = 1 << (3);
    esp_rom_delay_us(10);
    GPIO.out1_w1ts.val = 1 << (41 - 32); // Send final stop

    // Prepare for normal open drain functionality.
    GPIO.enable1_w1tc.val = 1 << (41 - 32);
    GPIO.enable_w1tc      = 1 << (3);
    GPIO.out1_w1tc.val    = 1 << (41 - 32);
    GPIO.out_w1tc         = 1 << (3);



    uint8_t who = 0xaa;
    int r       = GeneralI2CGet(AW32001_ADDRESS, CHIP_ID, &who, 1);
    if (r != 1 || who != 0x49)
    {
        ESP_LOGW("BMS", "WHOAMI Failed (%02x), %d", who, r);
        if (retry++ < 10)
            goto do_retry;
        ESP_LOGE("BMS", "Init failed on 1");
        return ESP_FAIL;
    }
    ESP_LOGI("BMS", "Init Start");

    setBMS();

    for (i = 0; i < 2; i++)
    {
        vTaskDelay(1);
        int check = initBMS();
        if (check != ESP_OK)
        {
            ESP_LOGI("BMS", "Init Fault Retry");
            if (retry++ < 10)
                goto do_retry;
            ESP_LOGI("BMS", "Init failed on 2");
            return false;
        }
        ESP_LOGI("BMS", "Check %d", check);
    }

    ESP_LOGI("BMS", "Init Ok");
    return true;
}

/**
 * @brief Initialize the BMS
 * @return ESP_OK if the BMS parameters were set, or a non-zero value if they were not
 */

esp_err_t BMSSetRegistersAndReset(void)
{
    #ifdef DEFAULT_BMS_SETTINGS
    // default settings from datasheet
    #endif

    #ifdef SWADGE_BMS_SETTINGS
    // for later as we learn things
    #endif

    #if !defined(DEFAULT_BMS_SETTINGS) && !defined(SWADGE_BMS_SETTINGS)
    uint8_t incurrent = INPUTCURRENT_500mA;
    uint8_t current = FASTCHARGE_128;
    uint8_t discurrent = DISCHARGE_2000mA;
    uint8_t voltage = CHARGEVOLTAGE_4200mV;
    uint8_t uvlo = UVLO_3030mV;
    uint8_t tj = TJ_80C;
    uint8_t sys_voltage = SYSVOLTAGE_4600;
    uint8_t vin = VIN_DPM_4520mV;
    uint8_t timeout = 0;
    #endif

    int r = 0;
    uint8_t val = 0;

    for (int i = 0; i < 7; i++)
    {
        if (r != 0)
        {
            ESP_LOGE("BMS", "Failed to set BMS register 0x%02X", i);
            return ESP_FAIL;
            break;
        }

        ESP_LOGI("BMS", "SWADGE BMS REGISTER: 0x%02X", i);
        switch (i)
        {
            case INPUT_SRC:  
                //Bits 0-3 set IIN_LIM, bits 4-7 set VIN_DPM
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x8F;
                #else
                val = (incurrent & 0x0F) | ((vin & 0x0F) << 4);
                #endif
                break;
            case POWER_ON_CFG:  
                //Bits 0-2 set VBAT_UVLO, bit 3 sets charge enable, bit 4 sets HIZ mode, bit 5 sets battery disconnect interrupt time for reset, bits 6-7 sets power-on after reset delay time
                //default values for these are: UVLO_2760mV, charge disabled (1), HIZ disabled (0), 4s (1), 16 (2) 
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0xAC;
                #else
                val = (uvlo & 0x7) | (1 <<3) | (0 << 4) | (1 << 5) | (2 << 6);
                #endif
                break;
            case CHG_CURRENT: 
                //Bits 0-5 set fast charge current, bit 6 sets WD timer reset, bit 7 sets software reset
                //default values for these are: 128 (0x40), WD reset disabled (0), no software reset (0)
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x0F;
                #else
                val = (current & 0x3F) | (0 << 6) | (0 << 7);
                #endif
                break;
            case TERM_CURRENT: 
                //Bits 0-3 set pre-charge current, bits 4-7 set discharge current limit
                //default values for these are: pre-charge current 3mA (1), discharge current limit 2000mA (9). TODO settable pre-charge current
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x91;
                #else
                val = (1 & 0x3) | (discurrent << 4);
                #endif
                break;
            case CHG_VOLTAGE:
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0xA3;
                #else
                //Bit 0 sets battery recharge threshold below VBAT_REG, bit 1 sets VBAT precharge to fast charge threshold, bits 2-7 set battery regulated voltage
                //default values for these are: VRECH = 200mV (1), VBAT_PRE = 3.0V (1), VBAT_REG = 4.2V (40)
                val = (1 << 0) | (1 << 1) | (voltage << 2);
                #endif;
                break;
            case TIMER_WD:
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x7A;
                #else
                //Bit 0 sets termination timer enable, bits 1-2 set fast charge time, bit 3 sets safety timer enable, bit 4 sets termination enable, bit 5-6 sets the watchdog time, bit 7 sets watchdog control in discharge mode
                // default values for these are: termination timer enable (0), fast charge time 5hrs (1), safety timer enabled (1), termination enabled (1), watchdog time 160s (3), watchdog enabled in discharge mode (0)
                val = (0 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (3 << 5) | (0 << 7);
                #endif;
                break;
            case MAIN_CTRL: 
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0xC0;
                #else
                //Bit
                #endif;
                break;
            case SYS_CTRL:
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x38;
                #else
                //TODO 
                #endif;
                break; 
            default:
                break;
        }
        

    r |= AW32001Set(AW32001_ADDRESS, i, val);
    ESP_LOGI("BMS", "SWADGE BMS REGISTER: 0x%02X written with value: 0x%02X", i, val);
    val = 0;
        
    }

    //TODO reset

    return r == 0 ? ESP_OK : ESP_FAIL;

    
}
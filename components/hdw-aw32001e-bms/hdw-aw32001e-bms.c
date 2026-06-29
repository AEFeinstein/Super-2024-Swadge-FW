//==============================================================================
// Includes
//==============================================================================
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "hdw-aw32001e-bms.h"

//==============================================================================
// Defines
//==============================================================================

//Do I need to do this again? This is done in hw_imu.c. Maybe yes because the imu is not always enabled? The BMS init is called before IMU init in swadge.c
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
static void i2c_delay(int x)
{
    for (int i = 0; i < 19 * x; i++)
    {
        asm volatile("nop");
    }
}

#define DELAY1 i2c_delay(1);
#define DELAY2 i2c_delay(2);

// static_i2c.h uses this macro name to toggle static linkage.
#define I2CNOSTATIC
#include "static_i2c.h"
#undef I2CNOSTATIC

//END Do I need to do this again?

//Uncomment the BMS settings you want to use. If both are uncommented, conservative, "safe" values will be used. 
//#define DEFAULT_BMS_SETTINGS
#define SWADGE_BMS_SETTINGS

#define AW32001_ADDRESS 0x49

//==============================================================================
// Function Prototypes
//==============================================================================
  


//==============================================================================
// Internal Functions
//==============================================================================

/**
 * @brief Set a specific register on the AW32001 BMS to a value.
 *
 * @param reg The 8-bit register #
 * @param val The 8-bit value to set the register to.
 * @return ESP_OK if the operation was successful.
 */
esp_err_t AW32001Set(int reg, uint8_t val)
{
    SendStart();
    SendByte(AW32001_ADDRESS << 1);
    SendByte(reg);
    SendByte(val);
    SendStop();
    return ESP_OK;
}

/**
 * @brief Read the state of the BMS register
 *
 * @param data The buffer to write the BMS register data into.
 * @param reg The register to read.
 * @return ESP_OK if the operation was successful.
 */
esp_err_t AW32001Get(uint8_t* data, uint8_t reg)
{
    SendStart();
    SendByte(AW32001_ADDRESS << 1);
    SendByte(reg);
    SendStop();
    SendStart();
    SendByte((AW32001_ADDRESS << 1) | 1); //RW bit == 1 for read
    *data = GetByte(1); //send nak, we only want this register
    SendStop();

    ESP_LOGI("BMS", "Read BMS Register 0x%02X: 0x%02X", reg, *data);
    //checksum
    if (reg == CHIP_ID && *data != 0x49)
    {
        ESP_LOGE("BMS", "BMS Register 0x%02X returned unexpected value: 0x%02X", reg, *data);
        return ESP_FAIL;
    }
    else{
    return ESP_OK;
    }
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
        .pull_up_en   = pullup,
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
    int r       = AW32001Get(&who, CHIP_ID);
    if (r != ESP_OK || who != 0x49)
    {
        ESP_LOGW("BMS", "WHOAMI Failed (%02x), %d", who, r);
        if (retry++ < 10)
            goto do_retry;
        ESP_LOGE("BMS", "Init failed on 1");
        return ESP_FAIL;
    }
    ESP_LOGI("BMS", "Init Start");

    for (i = 0; i < 2; i++)
    {
        vTaskDelay(1);
        int check = BMSSetRegistersAndReset();
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
 * @brief Deinit the BMS (nothing to do)
 * @return ESP_OK
 */
esp_err_t deinitBMS(void)
{
    return ESP_OK;
}

/**
 * @brief Set Registers on the BMS
 * @return ESP_OK if the BMS parameters were set, or a non-zero value if they were not
 */
esp_err_t BMSSetRegistersAndReset(void)
{
    #ifdef DEFAULT_BMS_SETTINGS
    //do nothing
    #endif

    #ifdef SWADGE_BMS_SETTINGS
    uint8_t incurrent = INPUTCURRENT_500mA;
    uint8_t current = FASTCHARGE_512mA;
    uint8_t discurrent = DISCHARGE_600mA;
    uint8_t voltage = CHARGEVOLTAGE_4200mV;
    uint8_t uvlo = UVLO_3030mV;
    uint8_t tj = TJ_120C;
    uint8_t sys_voltage = SYSVOLTAGE_4600mV;
    uint8_t vin = VIN_DPM_4520mV;
    uint8_t timeout = 0;
    uint8_t enable_therm = 0;
    uint8_t shipmode = 0;
    //todo struct instead of this
    #endif

    #if !defined(DEFAULT_BMS_SETTINGS) && !defined(SWADGE_BMS_SETTINGS)
    uint8_t incurrent = INPUTCURRENT_500mA;
    uint8_t current = FASTCHARGE_128mA;
    uint8_t discurrent = DISCHARGE_600mA;
    uint8_t voltage = CHARGEVOLTAGE_4200mV;
    uint8_t uvlo = UVLO_3030mV;
    uint8_t tj = TJ_80C;
    uint8_t sys_voltage = SYSVOLTAGE_4600mV;
    uint8_t vin = VIN_DPM_4520mV;
    uint8_t timeout = 0;
    uint8_t enable_therm = 0;
    uint8_t shipmode = 0;
    //todo struct instead of this
    #endif

    int r = 0;
    uint8_t val = 0;

    for (int i = 0; i <= 7; i++)
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
                val = (uvlo & 0x7) | (0 <<3) | (0 << 4) | (1 << 5) | (2 << 6);
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
                #endif
                break;
            case TIMER_WD:
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x7A;
                #else
                //Bit 0 sets termination timer enable, bits 1-2 set fast charge time, bit 3 sets safety timer enable, bit 4 sets termination enable, bit 5-6 sets the watchdog time, bit 7 sets watchdog control in discharge mode
                // default values for these are: termination timer enable (0), fast charge time 8hrs (2), safety timer enabled (1), termination enabled (1), watchdog time 160s (3), watchdog enabled in discharge mode (0)
                val = (0 << 0) | (3 << 2) | (0 << 3) | (1 << 4) | (timeout << 5) | (0 << 7);
                #endif
                break;
            case MAIN_CTRL: 
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0xC0;
                #else
                //Bit 0 enables Battery OVP interrupt control, bit 1 enables ntc int control, bit 2 enables charge status interrupt control, bit 3 enables end of charge interrupt control, bit 4 enables power good interrupt control, bit 5 enables ship mode control, bit 6 enables extended safety timer for PPMF, bit 7 enables thermistor control
                //default values for these are: OVP int enabled (0), NTC int enabled (0), charge status int enabled (0), end of charge int enabled (0), power good int enabled (0), ship mode control disabled (0), extended safety timer for PPMF enabled (1), thermistor control enabled (1)
                val = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (shipmode << 5) | (1 << 6) | (enable_therm << 7);
                #endif
                break;
            case SYS_CTRL:
                #ifdef DEFAULT_BMS_SETTINGS
                val = 0x38;
                #else
                 //Bits 0-3 set VSYS_REG, Bits 4-5 set TJ_Reg, Bit 6 enables VIN_DPM loop, Bit 7 disables PCB thermistor control
                val = (sys_voltage << 0) | (tj << 4) | (0 << 6) | (!enable_therm << 7);
                #endif
                break; 
            default:
                break;
        }
        

    r |= AW32001Set(i, val);
    ESP_LOGI("BMS", "SWADGE BMS REGISTER: 0x%02X written with value: 0x%02X", i, val);
    val = 0;
        
    }
    

    return r == 0 ? ESP_OK : ESP_FAIL;
    
}

/**
 * @brief Get Charge State of Battery
 * @return TRUE if battery is charging, FALSE if the battery is not charging
 */
bool getChargeState(void)
{
    uint8_t val = 0;
    if (AW32001Get(&val, SYS_STATUS) != ESP_OK)
    {
        ESP_LOGE("BMS", "Failed to read SYS_STATUS register");
        return false;
    }
    //get bits 3-4 for charge status, if 00 or 11 then not charging
    uint8_t charge_status = (val >> 3) & 0x03;
    if (charge_status == NOT_CHARGING || charge_status == FULL_CHARGE) //if not charging or done charging
    {
        return false;
    }
    else if (charge_status == CHARGING || charge_status == PRE_CHARGE)
    {
        return true;
    }
    else
    {
        ESP_LOGE("BMS", "Unexpected charge status: %d", charge_status);
        return false;
    }

}

/**
 * @brief set ship mode after elecrow flashing
 * @return nothing
 */
void setShipMode(bool enable)
{

   if (!AW32001Set(0xC, 0x8) && !AW32001Set(MAIN_CTRL, 0x40))
   {
       ESP_LOGE("BMS", "Failed to set ship mode");
   }
}
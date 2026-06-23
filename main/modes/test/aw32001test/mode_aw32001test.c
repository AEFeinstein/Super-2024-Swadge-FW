//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "swadge.h"
#include "mode_aw32001test.h"
#include "mainMenu.h"
#include "hdw-aw32001e-bms.h"

#define REGS 11

//==============================================================================
// Functions Prototypes
//==============================================================================

void aw32001testEnterMode(void);
void aw32001testExitMode(void);
void aw32001testMainLoop(int64_t elapsedUs);
void aw32001testUpdateFramebuffers(void);


//==============================================================================
// Variables
//==============================================================================

typedef struct
{
    font_t* font;
    int64_t tElapsedUs;
    int mode;
    uint8_t data;
    uint8_t val;
    char buf[9];
    char buffer[64];
} aw32001test_t;

aw32001test_t* aw32001test;






const char aw32001testName[] = "aw32001test";

swadgeMode_t mode_aw32001test = {
    .modeName                 = aw32001testName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = aw32001testEnterMode,
    .fnExitMode               = aw32001testExitMode,
    .fnMainLoop               = aw32001testMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

  
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
 


//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the aw32001test mode, allocate and initialize memory
 */
void aw32001testEnterMode(void)
{
    // Allocate memory for this mode
    aw32001test = (aw32001test_t*)heap_caps_calloc(1, sizeof(aw32001test_t), MALLOC_CAP_8BIT);

    // Load a font
    font_t* aw32001testFont = (font_t*)heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_8BIT);
    loadFont(SONIC_FONT, aw32001testFont, false);

    // Load some fonts
    aw32001test->font = aw32001testFont;

    aw32001test->tElapsedUs         = 0;
    aw32001test->mode               = 0;

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    //Turn on LEDS full brightness because we're trying to stress the BMS
    for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = 255;
        leds[i].g = 255;
        leds[i].b = 255;
    }
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * Exit the aw32001test mode, free memory
 **/
void aw32001testExitMode(void)
{
    // Free the font
    freeFont(aw32001test->font);
    heap_caps_free(aw32001test->font);
    // Free memory for this mode
    heap_caps_free(aw32001test);
}

/**
 * Main aw32001test loop
 *
 * @param elapsedUs The time elapsed since the last call
 */
void aw32001testMainLoop(int64_t elapsedUs)
{
    // Clear first
    clearPxTft();

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_A:
                case PB_B:
                {
                    // Exit
                    switchToSwadgeMode(&mainMenuMode);
                    break;
                }
                case PB_UP:
                case PB_LEFT:
                {
                    

                    aw32001test->mode--;
                    if (aw32001test->mode < 0)
                    {
                        aw32001test->mode = REGS - 1;
                        
                    }
                    
                    
                    break;
                }
                case PB_DOWN:
                case PB_RIGHT:
                {
                    

                    aw32001test->mode++;
                    if (aw32001test->mode >= REGS)
                    {
                        aw32001test->mode = 0;
                    
                    }
                    
                    
                    break;
                }
                case PB_START:
                case PB_SELECT:
                    break;
            }
        }
    }

    aw32001test->tElapsedUs += elapsedUs;

 
    AW32001Get(&aw32001test->data, aw32001test->mode);
    
    for (int i = 7; i >= 0; i--) {
        if (aw32001test->data & (1 << (7-i)))
            aw32001test->buf[i] = '1';
        else
            aw32001test->buf[i] = '0';
    }
    aw32001test->buf[8] = '\0';

   

    
        sprintf(aw32001test->buffer, "REG: %d", aw32001test->mode);
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 70);
        sprintf(aw32001test->buffer, "VAL: %s", aw32001test->buf);
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 90);

        //expected values, this is verbose because Emily can't read otherwise

        switch (aw32001test->mode)
        {
            case INPUT_SRC:  
                //Bits 0-3 set IIN_LIM, bits 4-7 set VIN_DPM
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0x8F;
                #else
                aw32001test->val = (incurrent & 0x0F) | ((vin & 0x0F) << 4);
                #endif
                break;
            case POWER_ON_CFG:  
                //Bits 0-2 set VBAT_UVLO, bit 3 sets charge enable, bit 4 sets HIZ mode, bit 5 sets battery disconnect interrupt time for reset, bits 6-7 sets power-on after reset delay time
                //default values for these are: UVLO_2760mV, charge disabled (1), HIZ disabled (0), 4s (1), 16 (2) 
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0xAC;
                #else
                aw32001test->val = (uvlo & 0x7) | (0 <<3) | (0 << 4) | (1 << 5) | (2 << 6);
                #endif
                break;
            case CHG_CURRENT: 
                //Bits 0-5 set fast charge current, bit 6 sets WD timer reset, bit 7 sets software reset
                //default values for these are: 128 (0x40), WD reset disabled (0), no software reset (0)
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0x0F;
                #else
                aw32001test->val = (current & 0x3F) | (0 << 6) | (0 << 7);
                #endif
                break;
            case TERM_CURRENT: 
                //Bits 0-3 set pre-charge current, bits 4-7 set discharge current limit
                //default values for these are: pre-charge current 3mA (1), discharge current limit 2000mA (9). TODO settable pre-charge current
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0x91;
                #else
                aw32001test->val = (1 & 0x3) | (discurrent << 4);
                #endif
                break;
            case CHG_VOLTAGE:
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0xA3;
                #else
                //Bit 0 sets battery recharge threshold below VBAT_REG, bit 1 sets VBAT precharge to fast charge threshold, bits 2-7 set battery regulated voltage
                //default values for these are: VRECH = 200mV (1), VBAT_PRE = 3.0V (1), VBAT_REG = 4.2V (40)
                aw32001test->val = (1 << 0) | (1 << 1) | (voltage << 2);
                #endif
                break;
            case TIMER_WD:
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0x7A;
                #else
                //Bit 0 sets termination timer enable, bits 1-2 set fast charge time, bit 3 sets safety timer enable, bit 4 sets termination enable, bit 5-6 sets the watchdog time, bit 7 sets watchdog control in discharge mode
                // default values for these are: termination timer enable (0), fast charge time 5hrs (1), safety timer enabled (1), termination enabled (1), watchdog time 160s (3), watchdog enabled in discharge mode (0)
                aw32001test->val = (0 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (3 << 5) | (0 << 7);
                #endif
                break;
            case MAIN_CTRL: 
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0xC0;
                #else
                //Bit 0 enables Battery OVP interrupt control, bit 1 enables ntc int control, bit 2 enables charge status interrupt control, bit 3 enables end of charge interrupt control, bit 4 enables power good interrupt control, bit 5 enables ship mode control, bit 6 enables extended safety timer for PPMF, bit 7 enables thermistor control
                //default values for these are: OVP int enabled (0), NTC int enabled (0), charge status int enabled (0), end of charge int enabled (0), power good int enabled (0), ship mode control disabled (0), extended safety timer for PPMF enabled (1), thermistor control enabled (1)
                aw32001test->val = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (shipmode << 5) | (1 << 6) | (enable_therm << 7);
                #endif
                break;
            case SYS_CTRL:
                #ifdef DEFAULT_BMS_SETTINGS
                aw32001test->val = 0x38;
                #else
                 //Bits 0-3 set VSYS_REG, Bits 4-5 set TJ_Reg, Bit 6 enables VIN_DPM loop, Bit 7 disables PCB thermistor control
                aw32001test->val = (sys_voltage << 0) | (tj << 4) | (0 << 6) | (!enable_therm << 7);
                #endif
                break; 
            case SYS_STATUS:
                //Bit 0 indicates therm status, bit 1 indicates power good, bit 2 indicates PPM status, bit 3-4 indicates charge status, bit 5-6 is the revision, bit 7 indicates WD fault
                //default values for these are: bit 0 no regulation, bit 1 good (1), ch state 00 for not charging, 01 precharge (1), 10 for charging (2), 11 for charge done (3)
                aw32001test->val = (0 << 0) | (1 << 1) | (0 << 2) | (0 << 3) | (0 << 4) | (0 << 5) | (1 << 6) | (0 << 7);
                break;
            case FAULT_STATUS:
                //default values for these are: all faults cleared (0)
                aw32001test->val = 0x00;
                break;
            case CHIP_ID:
                aw32001test->val = 0x49;
                break;
            default:
                break;
        }

        for (int i = 7; i >= 0; i--) {
        if (aw32001test->val & (1 << (7-i)))
            aw32001test->buf[i] = '1';
        else
            aw32001test->buf[i] = '0';
    }
    aw32001test->buf[8] = '\0';
        sprintf(aw32001test->buffer, "EXP: %s", aw32001test->buf);
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 110);

        //charge status

        AW32001Get(&aw32001test->data, SYS_STATUS);
        
        for (int i = 7; i >= 0; i--) {
            if (aw32001test->data & (1 << (7-i)))
                aw32001test->buf[i] = '1';
            else
                aw32001test->buf[i] = '0';
        }
        aw32001test->buf[8] = '\0';

        sprintf(aw32001test->buffer, "Power Good: %c", aw32001test->buf[1]);
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 130);
        sprintf(aw32001test->buffer, "Charge Status: %c%c", aw32001test->buf[3], aw32001test->buf[4]);
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 150);
        switch ((aw32001test->buf[3] - '0') << 1 | (aw32001test->buf[4] - '0')) {
            case 0:
                sprintf(aw32001test->buffer, "Not Charging");
                break;
            case 1:
                sprintf(aw32001test->buffer, "Precharge");
                break;
            case 2:
                sprintf(aw32001test->buffer, "Charging");
                break;
            case 3:
                sprintf(aw32001test->buffer, "Charge Done");
                break;
        }
        drawText(aw32001test->font, 215, aw32001test->buffer, 2, 170);
    

    
    }

#pragma once

#include <soc/gpio_num.h>

// // Define hardware-specific GPIOs
// #if defined(CONFIG_HARDWARE_WAVEBIRD) || defined(CONFIG_HARDWARE_GUNSHIP)
//     #define GPIO_SAO_1 GPIO_NUM_17
//     #define GPIO_SAO_2 GPIO_NUM_18

//     #define GPIO_BTN_UP    GPIO_NUM_0
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_2
//     #define GPIO_BTN_RIGHT GPIO_NUM_1

// #elif defined(defined(CONFIG_HARDWARE_HOTDOG_PRODUCTION))
//     #define GPIO_SAO_1 GPIO_NUM_40
//     #define GPIO_SAO_2 GPIO_NUM_42

//     #define GPIO_BTN_UP    GPIO_NUM_0
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_2
//     #define GPIO_BTN_RIGHT GPIO_NUM_1

// #elif defined(defined(CONFIG_HARDWARE_HOTDOG_PROTO))
//     #define GPIO_SAO_1 GPIO_NUM_40
//     #define GPIO_SAO_2 GPIO_NUM_42

//     #define GPIO_BTN_UP    GPIO_NUM_1
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_0
//     #define GPIO_BTN_RIGHT GPIO_NUM_2

// #elif defined(defined(CONFIG_HARDWARE_PULSE))
//     #define GPIO_SAO_1 GPIO_NUM_42 // Flip SAO GPIOs relative to Hotdog
//     #define GPIO_SAO_2 GPIO_NUM_40

//     #define GPIO_BTN_UP    GPIO_NUM_0
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_2
//     #define GPIO_BTN_RIGHT GPIO_NUM_1
// #else
//     #error "Define what hardware is being built for"
// #endif

typedef enum
{
#ifdef CONFIG_HARDWARE_WAVEBIRD
    #warning TODO fill this in

#elif defined(CONFIG_HARDWARE_GUNSHIP)
    #warning TODO fill this in

#elif defined(CONFIG_HARDWARE_HOTDOG_PROTO)
    #warning TODO fill this in

#elif defined(CONFIG_HARDWARE_HOTDOG_PRODUCTION)
    #warning TODO fill this in

#elif defined(CONFIG_HARDWARE_PULSE)
    GPIO_BTN_UP    = GPIO_NUM_0,
    GPIO_BTN_RIGHT = GPIO_NUM_1,
    GPIO_BTN_LEFT  = GPIO_NUM_2,
    GPIO_I2C_SDA   = GPIO_NUM_3,
    GPIO_BTN_DOWN  = GPIO_NUM_4,
    GPIO_BTN_MENU  = GPIO_NUM_5,
    GPIO_VMON      = GPIO_NUM_6,
    GPIO_MIC       = GPIO_NUM_7,
    GPIO_BTN_PAUSE = GPIO_NUM_8,
    GPIO_TOUCH_1   = GPIO_NUM_9,
    GPIO_TOUCH_2   = GPIO_NUM_10,
    GPIO_TOUCH_3   = GPIO_NUM_11,
    GPIO_TOUCH_4   = GPIO_NUM_12,
    GPIO_TOUCH_5   = GPIO_NUM_13,
    GPIO_TOUCH_6   = GPIO_NUM_14,
    GPIO_BTN_B     = GPIO_NUM_15,
    GPIO_BTN_A     = GPIO_NUM_16,
    GPIO_SPK       = GPIO_NUM_17,
    GPIO_SPK_SHDN  = GPIO_NUM_18,
    GPIO_USB_DM    = GPIO_NUM_19,
    GPIO_USB_DP    = GPIO_NUM_20,
    GPIO_TFT_RS    = GPIO_NUM_21,
    // GPIO_NUM_22 - GPIO_NUM_32 are used internally
    GPIO_TFT_FMARK = GPIO_NUM_33,
    GPIO_TFT_CS    = GPIO_NUM_34,
    GPIO_TFT_ATP   = GPIO_NUM_35,
    GPIO_TFT_SCL   = GPIO_NUM_36,
    GPIO_TFT_SDA   = GPIO_NUM_37,
    GPIO_TFT_RESET = GPIO_NUM_38,
    GPIO_LED       = GPIO_NUM_39,
    GPIO_SAO_B     = GPIO_NUM_40,
    GPIO_I2C_SCL   = GPIO_NUM_41,
    GPIO_CH32_PROG = GPIO_NUM_42,
    GPIO_TX        = GPIO_NUM_43,
    GPIO_RX        = GPIO_NUM_44,
    // GPIO_NUM_45 and GPIO_NUM_46 are not connected
    // GPIO_NUM_47 and GPIO_NUM_48 do not exist on ESP32-S2
    GPIO_SAO_A = GPIO_NUM_NC, // Hardware conflict with GPIO_CH32_PROG

#elif defined(CONFIG_HARDWARE_FAIRY_PROTO)
    GPIO_BTN_UP    = GPIO_NUM_0,
    GPIO_TOUCH_1   = GPIO_NUM_1,
    GPIO_TOUCH_2   = GPIO_NUM_2,
    GPIO_TOUCH_3   = GPIO_NUM_3,
    GPIO_TOUCH_4   = GPIO_NUM_4,
    GPIO_TOUCH_5   = GPIO_NUM_5,
    GPIO_TOUCH_6   = GPIO_NUM_6,
    GPIO_TOUCH_7   = GPIO_NUM_7,
    GPIO_TOUCH_8   = GPIO_NUM_8,
    GPIO_TOUCH_9   = GPIO_NUM_9,
    GPIO_TFT_CS    = GPIO_NUM_10,
    GPIO_TFT_SDA   = GPIO_NUM_11,
    GPIO_TFT_SCL   = GPIO_NUM_12,
    GPIO_TFT_ATP   = GPIO_NUM_13,
    GPIO_TOUCH_14  = GPIO_NUM_14,
    GPIO_SAO_B     = GPIO_NUM_15,
    GPIO_SAO_A     = GPIO_NUM_16,
    GPIO_MIC       = GPIO_NUM_17,
    GPIO_TFT_RESET = GPIO_NUM_18,
    GPIO_USB_DM    = GPIO_NUM_19,
    GPIO_USB_DP    = GPIO_NUM_20,
    GPIO_SLEEP     = GPIO_NUM_21,
    // GPIO_NUM_22 - GPIO_NUM_37 are used internally
    GPIO_TFT_RS    = GPIO_NUM_38,
    GPIO_CH32_PROG = GPIO_NUM_39,
    GPIO_I2C_SDA   = GPIO_NUM_40,
    GPIO_I2C_SCL   = GPIO_NUM_41,
    GPIO_LED       = GPIO_NUM_42,
    GPIO_TX        = GPIO_NUM_43,
    GPIO_RX        = GPIO_NUM_44,
    // GPIO_NUM_45 and GPIO_NUM_46 are not connected
    GPIO_SPK      = GPIO_NUM_47,
    GPIO_SPK_SHDN = GPIO_NUM_48,
    GPIO_VMON     = GPIO_NUM_NC,
#endif
} swadgeGpio_t;
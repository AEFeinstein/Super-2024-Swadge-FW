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

// #elif defined(CONFIG_HARDWARE_HOTDOG_PRODUCTION)
//     #define GPIO_SAO_1 GPIO_NUM_40
//     #define GPIO_SAO_2 GPIO_NUM_42

//     #define GPIO_BTN_UP    GPIO_NUM_0
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_2
//     #define GPIO_BTN_RIGHT GPIO_NUM_1

// #elif defined(CONFIG_HARDWARE_HOTDOG_PROTO)
//     #define GPIO_SAO_1 GPIO_NUM_40
//     #define GPIO_SAO_2 GPIO_NUM_42

//     #define GPIO_BTN_UP    GPIO_NUM_1
//     #define GPIO_BTN_DOWN  GPIO_NUM_4
//     #define GPIO_BTN_LEFT  GPIO_NUM_0
//     #define GPIO_BTN_RIGHT GPIO_NUM_2

// #elif defined(CONFIG_HARDWARE_PULSE)
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
#if CONFIG_IDF_TARGET_ESP32S2
#warning TODO fill this in
#elif CONFIG_IDF_TARGET_ESP32S3
    GPIO_BTN_UP    = GPIO_NUM_0,
    GPIO_SAO_B     = GPIO_NUM_1,
    GPIO_SAO_A     = GPIO_NUM_2,
    GPIO_BTN_PAUSE = GPIO_NUM_3,
    GPIO_SPK_SHDN  = GPIO_NUM_4,
    GPIO_I2S_SDIN  = GPIO_NUM_5,
    GPIO_I2S_LRCK  = GPIO_NUM_6,
    GPIO_I2S_SDOUT = GPIO_NUM_7,
    GPIO_BTN_B     = GPIO_NUM_8,
    GPIO_TOUCH_4   = GPIO_NUM_9,
    GPIO_TOUCH_1   = GPIO_NUM_10,
    GPIO_TOUCH_3   = GPIO_NUM_11,
    GPIO_TOUCH_6   = GPIO_NUM_12,
    GPIO_TOUCH_2   = GPIO_NUM_13,
    GPIO_TOUCH_5   = GPIO_NUM_14,
    GPIO_I2S_BCLK  = GPIO_NUM_15,
    GPIO_I2S_MCLK  = GPIO_NUM_16,
    GPIO_LED       = GPIO_NUM_17,
    GPIO_BTN_A     = GPIO_NUM_18,
    GPIO_USB_DM    = GPIO_NUM_19,
    GPIO_USB_DP    = GPIO_NUM_20,
    GPIO_CH32_PROG = GPIO_NUM_21,
    // GPIO_NC_26 = GPIO_NUM_26,
    // GPIO_NC_27 = GPIO_NUM_27,
    // GPIO_NC_28 = GPIO_NUM_28,
    // GPIO_NC_29 = GPIO_NUM_29,
    // GPIO_NC_30 = GPIO_NUM_30,
    // GPIO_NC_31 = GPIO_NUM_31,
    // GPIO_NC_32 = GPIO_NUM_32,
    // GPIO_NC_33 = GPIO_NUM_33,
    // GPIO_NC_34 = GPIO_NUM_34,
    GPIO_BTN_DOWN  = GPIO_NUM_35,
    GPIO_BTN_LEFT  = GPIO_NUM_36,
    GPIO_TFT_ATP   = GPIO_NUM_37,
    GPIO_TFT_RESET = GPIO_NUM_38,
    GPIO_TFT_SDA   = GPIO_NUM_39,
    GPIO_TFT_SCL   = GPIO_NUM_40,
    GPIO_TFT_RS    = GPIO_NUM_41,
    GPIO_TFT_CS    = GPIO_NUM_42,
    GPIO_TX        = GPIO_NUM_43,
    GPIO_RX        = GPIO_NUM_44,
    GPIO_BTN_RIGHT = GPIO_NUM_45,
    GPIO_BTN_MENU  = GPIO_NUM_46,
    GPIO_I2C_SCL   = GPIO_NUM_47,
    GPIO_I2C_SDA   = GPIO_NUM_48,
#endif
} swadgeGpio_t;
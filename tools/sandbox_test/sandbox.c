#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "swadge2024.h"
#include "hal/gpio_types.h"
#include "esp_log.h"
#include "soc/efuse_reg.h"
#include "soc/soc.h"
#include "soc/system_reg.h"
#include "hdw-tft.h"
#include "mainMenu.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "coreutil.h"

int global_i = 100;
menu_t * menu;
const char * menu_Bootload = "Bootloader";

//#define REBOOT_TEST
//#define PROFILE_TEST
#define MODE7_TEST

#ifdef MODE7_TEST
int mode7timing;
#endif

// External functions defined in .S file for you assembly people.
void minimal_function();
uint32_t test_function( uint32_t x );
uint32_t asm_read_gpio();

wsg_t example_sprite;

static void mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if( label == mainMenuMode.modeName )
    {
        switchToSwadgeMode( &mainMenuMode );
    }
    else if( label == menu_Bootload )
    {
        // Uncomment this to reboot the chip into the bootloader.
        // This is to test to make sure we can call ROM functions.
        REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        void software_reset( uint32_t x );
        software_reset( 0 );
    }
}

void sandbox_main(void)
{
#if 1

#ifdef REBOOT_TEST
    // Uncomment this to reboot the chip into the bootloader.
    // This is to test to make sure we can call ROM functions.
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    void software_reset( uint32_t x );
    software_reset( 0 );
#endif

    ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

    REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

    menu = initMenu("USB Sandbox", mainMenuCb);
    addSingleItemToMenu(menu, mainMenuMode.modeName);
    addSingleItemToMenu(menu, menu_Bootload);

    loadWsg("kid0.wsg", &example_sprite, true);
#endif

    ESP_LOGI( "sandbox", "Loaded" );
}

void sandbox_exit()
{
//    ESP_LOGI( "sandbox", "Exit" );
#if 0
    if( menu )
    {
        deinitMenu(menu);
    }
    if( example_sprite.px )
    {
        freeWsg( &example_sprite ); 
    }
#endif
//    ESP_LOGI( "sandbox", "Exit" );
}

void sandbox_tick()
{
#if 1
    uint32_t start, end;
#ifdef PROFILE_TEST
    volatile uint32_t profiles[7];  // Use of volatile here to force compiler to order instructions and not cheat.
    // Profile function call into assembly land
    // Mostly used to understand function call overhead.
    start = getCycleCount();
    minimal_function();
    end = getCycleCount();
    profiles[0] = end-start-1;

    // Profile a nop (Should be 1, because profiling takes 1 cycle)
    start = getCycleCount();
    asm volatile( "nop" );
    end = getCycleCount();
    profiles[1] = end-start-1;

    // Profile reading a register (will be slow)
    start = getCycleCount();
    READ_PERI_REG( GPIO_ENABLE_W1TS_REG );
    end = getCycleCount();
    profiles[2] = end-start-1;

    // Profile writing a regsiter (will be fast)
    // The ESP32-S2 can "write" to memory and keep executing
    start = getCycleCount();
    WRITE_PERI_REG( GPIO_ENABLE_W1TS_REG, 0 );
    end = getCycleCount();
    profiles[3] = end-start-1;

    // Profile subsequent writes (will be slow)
    // The ESP32-S2 can only write once in a buffered write.
    start = getCycleCount();
    WRITE_PERI_REG( GPIO_ENABLE_W1TS_REG, 0 );
    WRITE_PERI_REG( GPIO_ENABLE_W1TS_REG, 0 );
    end = getCycleCount();
    profiles[4] = end-start-1;

    // Profile a more interesting assembly instruction
    start = getCycleCount();
    uint32_t tfret = test_function( 0xaaaa );
    end = getCycleCount();
    profiles[5] = end-start-1;

    // Profile a more interesting assembly instruction
    start = getCycleCount();
    uint32_t tfret2 = asm_read_gpio( );
    end = getCycleCount();
    profiles[6] = end-start-1;
    vTaskDelay(1);

    ESP_LOGI( "sandbox", "global_i: %d %d %d %d %d %d %d clock cycles; tf ret: %08x / %08x", profiles[0], profiles[1], profiles[2], profiles[3], profiles[4], profiles[5], profiles[6], tfret, tfret2 );
#else
//    ESP_LOGI( "sandbox", "global_i: %d", global_i++ );
    global_i++;
#endif

//    if( menu )
//        drawMenu(menu);

    start = getCycleCount();

    end = getCycleCount();
    ESP_LOGI( "sandbox", "SPROF: %lu / Mode7: %d", end-start, mode7timing );

    for( int mode = 0; mode < 8; mode++ )
    {
        drawWsg( &example_sprite, 50+mode*20, (global_i%20)-10, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, 50+mode*20, (global_i%20)+230, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, (global_i%20)-10, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, (global_i%20)+270, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
    }

    buttonEvt_t evt              = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        menu = menuButton(menu, evt);
    }
#endif

}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
    int i;

    uint32_t start = getCycleCount();
    fillDisplayArea(x, y, x+w, y+h, 0 );
    for( i = 0; i < 16; i++ )
        fillDisplayArea(i*16+8, y, i*16+16+8, y+16, up*16+i );
    mode7timing = getCycleCount() - start;
}


swadgeMode_t sandbox_mode = {
    .modeName                 = "sandbox",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = sandbox_main,
    .fnExitMode               = sandbox_exit,
    .fnMainLoop               = sandbox_tick,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = sandboxBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL
};



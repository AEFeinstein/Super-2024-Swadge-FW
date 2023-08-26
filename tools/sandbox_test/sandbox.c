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
#include "hdw-btn.h"

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

int getTouchJoystick( int32_t * phi, int32_t * r, int32_t * intensity )
{
	#define TOUCH_CENTER 2
	const uint8_t ringzones[] = { 3, 0, 1, 4, 5 };
	#define NUM_TZ_RING 5
	int32_t baseVals[6];
	int32_t ringIntensity = 0;
	int bc = getBaseTouchVals( baseVals, 6 );
	if( bc != 6 )
		return bc;


	int centerIntensity = baseVals[TOUCH_CENTER];

	// First, compute phi.

	// Find most pressed pad
    int peak    = -1;
    int peakBin = -1;
    for (int i = 0; i < NUM_TZ_RING; i++)
    {
        int32_t bv = baseVals[ringzones[i]];
        if (bv > peak)
        {
            peak    = bv;
            peakBin = i;
        }
    }

    if (peakBin < 0)
    {
        return 0;
    }
    // Arbitrary, but we use 1200 as the minimum peak value.
    if (peak < 1200 && centerIntensity < 1200 )
    {
        return 0;
    }
    // We know our peak bin, now we need to know the average and differential of the adjacent bins.
    int leftOfPeak  = (peakBin > 0) ? baseVals[ringzones[peakBin - 1]] : baseVals[ringzones[NUM_TZ_RING - 1]];
    int rightOfPeak = (peakBin < NUM_TZ_RING - 1) ? baseVals[ringzones[peakBin + 1]] : baseVals[ringzones[0]];

    int oPeak  = peak;
    int center = peakBin << 8;

    if (rightOfPeak >= leftOfPeak)
    {
        // We bend upward (or are neutral)
        rightOfPeak -= leftOfPeak;
        peak -= leftOfPeak;
        center += (rightOfPeak << 8) / (rightOfPeak + peak);

        ringIntensity = oPeak + rightOfPeak;
    }
    else
    {	
        // We bend downward
        leftOfPeak -= rightOfPeak;
        peak -= rightOfPeak;
        center -= (leftOfPeak << 8) / (leftOfPeak + peak);

        ringIntensity = oPeak + leftOfPeak;
    }
	int ringph = (center < 0)?(center + 1280):center;
	if( phi ) *phi = ringph;

	// Find ratio of ring to inner.
	int totalIntensity = centerIntensity + ringIntensity;
	int radius = (ringIntensity<<10) / totalIntensity;

	if( r ) *r = radius;

	if( intensity ) *intensity = totalIntensity;

	return 6;
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
    WR  ITE_PERI_REG( GPIO_ENABLE_W1TS_REG, 0 );
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

//    ESP_LOGI( "sandbox", "SPROF: %lu / Mode7: %d", end-start, mode7timing );

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
	int32_t phi = 0;
	int32_t r = 0;
	int32_t intensity = 0;
    start = getCycleCount();
	int tbv = getTouchJoystick( &phi, &r, &intensity );
    end = getCycleCount();

	if( tbv > 0 )
	{
		extern const int16_t sin1024[91];

		phi = phi * 360 / 1280 + 330;
		if( phi >= 360 ) phi -= 360;
		int32_t sY = -getSin1024( phi ) * r;
		phi += 90;
		if( phi >= 360 ) phi -= 360;
		int32_t sX = getSin1024( phi ) * r;

	    drawWsg( &example_sprite, 120-10 + (sX>>15), 140-20 + (sY>>15), 0, 0, 0 );
	}

	ESP_LOGI( "sandbox", "TBV [%lu] %ld %ld %ld %d", end-start, phi, r, intensity, tbv );


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



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
#include "hdw-ch32v003.h"
#include "mainMenu.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "soc/i2c_reg.h"
#include "soc/gpio_struct.h"
#include "coreutil.h"
#include "hdw-btn.h"
#include "hdw-imu.h"

#include "deep_sleep.h"
#include "matrix_drops.h"

int frameno;
int bQuit;

int global_i = 100;
menu_t * menu;
const char * menu_Bootload = "Bootloader";
const char * menu_matrix_drops = "Matrix Drops";
const char * menu_deep_sleep = "Deep Sleep";
menuMegaRenderer_t* menuMegaRenderer;

font_t logbook;

// External functions defined in .S file for you assembly people.
void minimal_function();
uint32_t test_function( uint32_t x );
uint32_t asm_read_gpio();

wsg_t example_sprite;

static bool mainMenuCb(const char* label, bool selected, unsigned long int settingVal)
{
	if( selected )
	{
		if( label == menu_matrix_drops )
		{
			ch32v003WriteFlash( matrix_drops, sizeof( matrix_drops ) );
			ch32v003Resume();
		}
		else if( label == menu_deep_sleep )
		{
			ch32v003WriteFlash( deep_sleep, sizeof( deep_sleep ) );
			ch32v003Resume();
		}
		else if( label == mainMenuMode.modeName )
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
	return true;
}

void sandbox_main(void)
{
	frameno = 0;
	bQuit = 0;

	ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

	REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

	menu = initMenu("USB Sandbox", mainMenuCb);
	addSingleItemToMenu(menu, menu_matrix_drops );
	addSingleItemToMenu(menu, menu_deep_sleep );
	addSingleItemToMenu(menu, mainMenuMode.modeName);
	addSingleItemToMenu(menu, menu_Bootload);
	static char menent[128];
	sprintf( menent, "%d\n", initCh32v003( 18) );
	addSingleItemToMenu(menu, menent);
    menuMegaRenderer = initMenuMegaRenderer(NULL, NULL, NULL);

	loadWsg(HP_BOTTOM_BIGMA_WSG, &example_sprite, true);

    loadFont(LOGBOOK_FONT, &logbook, false);

	accelSetRegistersAndReset();

	setFrameRateUs(5000);



	ESP_LOGI( "sandbox", "Loaded" );
}

void sandbox_exit()
{
	bQuit = 1;
}

void sandbox_tick()
{
	if( bQuit ) return;


	buttonEvt_t evt = {0};
	while (checkButtonQueueWrapper(&evt))
	{
		menu = menuButton(menu, evt);
	}
    drawMenuMega(menu, menuMegaRenderer, 1);

//	drawText( &logbook, 215, "test", 10, 20 );
}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
//	accelIntegrate();
	if( up == 2 || up == 8 || up == 13 ) 
		accelIntegrate();

	fillDisplayArea(x, y, x+w, y+h, 2 );
}


swadgeMode_t sandbox_mode = {
	.modeName				 = "sandbox",
	.wifiMode				 = NO_WIFI,
	.overrideUsb			  = false,
	.usesAccelerometer		= false,
	.usesThermometer		  = false,
	.fnEnterMode			  = sandbox_main,
	.fnExitMode			   = sandbox_exit,
	.fnMainLoop			   = sandbox_tick,
	.fnAudioCallback		  = NULL,
	.fnBackgroundDrawCallback = sandboxBackgroundDrawCallback,
	.fnEspNowRecvCb		   = NULL,
	.fnEspNowSendCb		   = NULL,
	.fnAdvancedUSB			= NULL
};



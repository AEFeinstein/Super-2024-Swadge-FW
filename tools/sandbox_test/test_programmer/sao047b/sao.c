#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#include "graphics.h"

#include "games.h"

void test() { }

int main()
{
	SystemInit();

	funGpioInitAll();
	funPinMode( PD0, GPIO_CFGLR_OUT_50Mhz_PP );
	funDigitalWrite( PD0, FUN_LOW );

	// button setup
	funPinMode( PA1, GPIO_CFGLR_IN_PUPD );
	funPinMode( PA2, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PA1, FUN_HIGH );
	funDigitalWrite( PA2, FUN_HIGH );

	ssd1306_rst();
	if(ssd1306_spi_init())
	{
		// Could not init OLED.  Cry?
	}
	ssd1306_init();

	ssd1306_cmd( SSD1306_SETMULTIPLEX );
	ssd1306_cmd( 39 );
	ssd1306_cmd( SSD1306_SETDISPLAYOFFSET );
	ssd1306_cmd( 0 );

	ssd1306_cmd( SSD1306_SETPRECHARGE ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETCONTRAST ); ssd1306_cmd( 0xf1 );
	ssd1306_cmd( SSD1306_SETVCOMDETECT ); ssd1306_cmd( 0x40 );
	ssd1306_cmd( 0xad ); ssd1306_cmd( 0x90 ); // Set Charge pump (set to 0x90 for extra bright)

	gameTimeUs = 0;
	gameMode = GameModeMainMenu;

	uint32_t modeStartTime = SysTick->CNT;
	int gameNumber = 0;

	while(1)
	{
		int ret = gameMode();
		uint32_t now = SysTick->CNT;

		int thisMask = (( GPIOA->INDR >> 1 ) & 3) ^ 3;
		buttonEventDown = thisMask & ~buttonMask;
		buttonEventUp = ~thisMask & buttonMask;
		buttonMask = thisMask;

		gameTimeUs = (now - modeStartTime) / 6; // 6 as a constant can div fast.
		int gameTimeUsFlip = (now - modeStartTime) < 0;
		if( gameTimeUsFlip ) gameTimeUs += 715827883;
		swapBuffer();

		frameno++;

		if( ret != 0 )
		{
			glyphdraw_invert = 0;
			glyphdraw_nomask = 0;
			gameTimeUs = 0;
			modeStartTime = now;
			frameno = 0;
			memset( gameData, 0, sizeof( gameData ) );

			gameNumber++;

			if( gameNumber == 10 )
			{
				gameMode = GameModeEnding;
			}
			else if( gameNumber == 11 )
			{
				gameNumber = 0;
				gameMode = GameModeMainMenu;
			}
			else
			{
				gameMode = gameModes[ SysTick->CNT % (sizeof(gameModes)/sizeof(gameModes[0])) ];
			}
		}
	}
	return 0;
}



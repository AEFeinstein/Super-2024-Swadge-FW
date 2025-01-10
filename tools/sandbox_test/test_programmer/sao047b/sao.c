#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#include "graphics.h"


int frameno;

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

	printf( "Setup Start\n" );

	ssd1306_rst();
	if(ssd1306_spi_init())
	{
		printf( "Could not connect to OLED\n" );
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

	int i;


	printf( "Setup Complete\n" );
	while(1)
	{

		glyphdraw_invert = 0;
		glyphdraw_nomask = 1;

		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "A GRAND" );

		swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", (frameno>>2) );

		swapBuffer();


		background(7);

		frameno++;
	}
}



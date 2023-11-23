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
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "soc/i2c_reg.h"
#include "soc/gpio_struct.h"
#include "coreutil.h"
#include "hdw-btn.h"
#include "hdw-imu.h"

int bQuit = 0;
int global_i = 100;

font_t ibm;


// External functions defined in .S file for you assembly people.
void minimal_function();
uint32_t test_function( uint32_t x );
uint32_t asm_read_gpio();

extern fnAdvancedUsbHandler advancedUsbHandler;
int16_t sandboxAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

static char* local_hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "Magfest",              // 1: Manufacturer
    "Swadge Controller",    // 2: Product
    "XXXXXX",               // 3: Serials, should use chip ID
    "Swadge HID interface", // 4: HID
};


// clang-format off
#define TUD_HID_REPORT_DESC_NIL(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Allow for 0xaa (regular size), 0xab (jumbo sized) and 0xac mini feature reports; Windows needs specific id'd and size'd endpoints. */ \
    HID_REPORT_COUNT ( CFG_TUD_ENDPOINT0_SIZE                 ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_REPORT_ID    ( 0xaa                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( (255-1)         ) ,\
    HID_REPORT_ID    ( 0xab                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( 1                                      ) ,\
    HID_REPORT_ID    ( 0xac                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
    HID_REPORT_COUNT ( (255-1)         ) ,\
    HID_REPORT_ID    ( 0xad                                   ) \
    HID_USAGE        ( HID_USAGE_DESKTOP_GAMEPAD              ) ,\
    HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
  HID_COLLECTION_END
// clang-format on

static const uint8_t local_hid_report_descriptor[] = {TUD_HID_REPORT_DESC_NIL()};

static const uint8_t local_hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1,                                                        // Configuration number
                          1,                                                        // interface count
                          0,                                                        // string index
                          (TUD_CONFIG_DESC_LEN + (CFG_TUD_HID * TUD_HID_DESC_LEN)), // total length
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,                       // attribute
                          100),                                                     // power in mA

    TUD_HID_DESCRIPTOR(0,                             // Interface number
                       4,                             // string index
                       false,                         // boot protocol
                       sizeof(local_hid_report_descriptor), // report descriptor len
                       0x81,                          // EP In address
                       64,                            // size
                       1),                           // polling interval
};



void sandbox_main(void)
{
	bQuit = 0;

	ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

	extern const tusb_desc_device_t descriptor_dev_kconfig;
	void tinyusb_set_descriptor( const tusb_desc_device_t * dev_descriptor, const char **str_desc, int str_desc_count, const uint8_t *cfg_desc);
	initTusb( (const tinyusb_config_t *)&descriptor_dev_kconfig, local_hid_report_descriptor );
    tinyusb_set_descriptor(&descriptor_dev_kconfig, local_hid_string_descriptor, 5, local_hid_configuration_descriptor);
	tud_disconnect();
    tusb_init();
	int i;	for( i = 0; i < 10000; i++ ) asm volatile( "nop" ); tud_connect();

	advancedUsbHandler = sandboxAdvancedUSB;

    loadFont("ibm_vga8.font", &ibm, false);

	fillDisplayArea(0, 0, 280, 240, 5 );

	setFrameRateUs(5000);

	ESP_LOGI( "sandbox", "Loaded" );
}

int16_t sandboxAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
	// The send/receive buffers are 64 bytes.
	if( isGet )
	{
		// When sending data back to the host, we don't need to prefix the report descriptor.
		buffer[0] = 0xaa;
		buffer[1] = 0x55;
		buffer[2] = 0xaa;
		buffer[3] = 0x55;
		return 64;
	}
	else
	{
		char st[128];
		// When getting data, buffer[0] is always 0xad, our report ID, so we only have 63 bytes actually available.
		switch( buffer[1] )
		{
			case 1: // Draw pixel
			{
				int x = buffer[2];
				int y = buffer[3];
				int len = buffer[4];  // Need length redundantly, because Windows can't send variable sized reports.
				int color = buffer[5];
				char st[64] = { 0 };
				if( len > length - 6 ) len = length - 6;
				if( len > 63 ) len = 63;
				int i;
				for( i = 0; i < len; i++ )
				{
					uint8_t c = buffer[5+i];
					if( c < 127 && c > 31 )
						st[i] = c;
				}
				st[len] = 0;
				drawText( &ibm, color, st, x, y );
				break;
			}
			default:
				sprintf( st, "RX From Host %d [%02x %02x %02x %02x...]", length, buffer[0], buffer[1], buffer[2], buffer[3] );
				drawText( &ibm, 215, st, 10, 20 );
				break;
		}
		return 0;
	}
}

void sandbox_exit()
{
	bQuit = 1;
}

void sandbox_tick()
{
	if( bQuit ) return;
}


swadgeMode_t sandbox_mode = {
	.modeName				= "hidsandbox",
	.wifiMode				= NO_WIFI,
	.overrideUsb			= true,
	.usesAccelerometer		= true,
	.usesThermometer		= false,
	.fnEnterMode			= sandbox_main,
	.fnExitMode				= sandbox_exit,
	.fnMainLoop				= sandbox_tick,
	.fnAudioCallback		= NULL,
	.fnBackgroundDrawCallback = NULL,
	.fnEspNowRecvCb			= NULL,
	.fnEspNowSendCb			= NULL,
	.fnAdvancedUSB			= sandboxAdvancedUSB,
};



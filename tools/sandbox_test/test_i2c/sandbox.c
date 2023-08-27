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

#define LSM6DSL_ADDRESS						0x6a
#define QMC6308_ADDRESS						0x2c

#define LSM6DSL_FUNC_CFG_ACCESS				0x01
#define LSM6DSL_SENSOR_SYNC_TIME_FRAME		0x04
#define LSM6DSL_FIFO_CTRL1					0x06
#define LSM6DSL_FIFO_CTRL2					0x07
#define LSM6DSL_FIFO_CTRL3					0x08
#define LSM6DSL_FIFO_CTRL4					0x09
#define LSM6DSL_FIFO_CTRL5					0x0a
#define LSM6DSL_ORIENT_CFG_G				0x0b
#define LSM6DSL_INT1_CTRL					0x0d
#define LSM6DSL_INT2_CTRL					0x0e
#define LMS6DS3_WHO_AM_I					0x0f
#define LSM6DSL_CTRL1_XL					0x10
#define LSM6DSL_CTRL2_G						0x11
#define LSM6DSL_CTRL3_C						0x12
#define LSM6DSL_CTRL4_C						0x13
#define LSM6DSL_CTRL5_C						0x14
#define LSM6DSL_CTRL6_C						0x15
#define LSM6DSL_CTRL7_G						0x16
#define LSM6DSL_CTRL8_XL					0x17
#define LSM6DSL_CTRL9_XL					0x18
#define LSM6DSL_CTRL10_C					0x19
#define LSM6DSL_MASTER_CONFIG				0x1a
#define LSM6DSL_WAKE_UP_SRC					0x1b
#define LSM6DSL_TAP_SRC						0x1c
#define LSM6DSL_D6D_SRC						0x1d
#define LSM6DSL_STATUS_REG					0x1e
#define LSM6DSL_OUT_TEMP_L					0x20
#define LSM6DSL_OUT_TEMP_H					0x21
#define LMS6DS3_OUTX_L_G					0x22
#define LMS6DS3_OUTX_H_G					0x23
#define LMS6DS3_OUTY_L_G					0x24
#define LMS6DS3_OUTY_H_G					0x25
#define LMS6DS3_OUTZ_L_G					0x26
#define LMS6DS3_OUTZ_H_G					0x27
#define LMS6DS3_OUTX_L_XL					0x28
#define LMS6DS3_OUTX_H_XL					0x29
#define LMS6DS3_OUTY_L_XL					0x2a
#define LMS6DS3_OUTY_H_XL					0x2b
#define LMS6DS3_OUTZ_L_XL					0x2c
#define LMS6DS3_OUTZ_H_XL					0x2d


struct LSM6DSLData
{
	uint16_t temp;
	int32_t gyroaccum[3];
	int16_t gyrolast[3];
} LSM6DSL;

static esp_err_t GeneralSet( int dev, int reg, int val )
{
    i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, dev << 1, false);
    i2c_master_write_byte(cmdHandle, reg, false);
    i2c_master_write_byte(cmdHandle, val, true);
    i2c_master_stop(cmdHandle);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);
	return err;
}

static esp_err_t LSM6DSLSet( int reg, int val )
{
	return GeneralSet( LSM6DSL_ADDRESS, reg, val );
}

static int GeneralI2CGet( int device, int reg, uint8_t * data, int data_len )
{
    i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, device << 1, false);
    i2c_master_write_byte(cmdHandle, reg, false);
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, device << 1 | I2C_MASTER_READ, false);
    i2c_master_read(cmdHandle, data, data_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmdHandle);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);
	if( err ) return err;
	else return data_len;
}


static int ReadLSM6DSL( uint8_t * data, int data_len )
{
    i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, LSM6DSL_ADDRESS << 1, false);
    i2c_master_write_byte(cmdHandle, 0x3A, false);
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, LSM6DSL_ADDRESS << 1 | I2C_MASTER_READ, false);
	uint32_t fifolen = 0;
    i2c_master_read(cmdHandle, (uint8_t*)&fifolen, 3, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmdHandle);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);
	if( err < 0 ) return err;

	fifolen &= 0x3ff;
	if( fifolen > data_len / 2 ) fifolen = data_len / 2;

	cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, LSM6DSL_ADDRESS << 1 | I2C_MASTER_READ, false);
    i2c_master_read(cmdHandle, data, fifolen * 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmdHandle);
    err = i2c_master_cmd_begin(I2C_NUM_0, cmdHandle, 100);

    i2c_cmd_link_delete(cmdHandle);
	if( err < 0 ) return err;

	return fifolen;
}

static void LSM6DSLIntegrate()
{
	struct LSM6DSLData * ld = &LSM6DSL;

	int16_t data[72];
	int r = GeneralI2CGet( LSM6DSL_ADDRESS, 0x20, (uint8_t*)data, 2 );
	if( r < 0 ) return;

	ld->temp = data[0];
	int readr = ReadLSM6DSL( (uint8_t*)data, sizeof( data ) );

	int samp;
	int16_t * cdata = data;

	for( samp = 0; samp < readr; samp+=12 )
	{
		ld->gyroaccum[0] += cdata[0];
		ld->gyroaccum[1] += cdata[1];
		ld->gyroaccum[2] += cdata[2];

		ld->gyrolast[0] = cdata[0];
		ld->gyrolast[1] = cdata[1];
		ld->gyrolast[2] = cdata[2];

		cdata += 6;
	}
}

static void LMS6DS3Setup()
{
	memset( &LSM6DSL, 0, sizeof(LSM6DSL) );
	// Enable access
	LSM6DSLSet( LSM6DSL_FUNC_CFG_ACCESS, 0x20 );
	LSM6DSLSet( LSM6DSL_CTRL3_C, 0x81 ); // Force reset
	vTaskDelay( 1 );
	LSM6DSLSet( LSM6DSL_CTRL3_C, 0x44 ); // unforce reset
	LSM6DSLSet( LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b110 ); // 208 Hz ODR
	LSM6DSLSet( LSM6DSL_FIFO_CTRL3, 0b00001001 ); // Put both devices in FIFO.
	LSM6DSLSet( LSM6DSL_CTRL1_XL, 0b01011001 ); // Setup accel (16 g's FS)
	LSM6DSLSet( LSM6DSL_CTRL2_G, 0b01010100 ); // Setup gyro, 500dps
	LSM6DSLSet( LSM6DSL_CTRL7_G, 0b10000000 ); // Setup gyro, not high performance mode.
	LSM6DSLSet( LSM6DSL_FIFO_CTRL2, 0b00000000 ); //Temp not in fifo  (Why no work?)

	uint8_t who = 0xaa;
	int r = GeneralI2CGet( LSM6DSL_ADDRESS, LMS6DS3_WHO_AM_I, &who, 1 );
	if( r != 1 || who != 0x6a )
	{
		ESP_LOGE( "LSM6DSL", "WHOAMI Failed (%02x), %d. Cannot start part.\n", who, r ); 
	}
}


int global_i = 100;
menu_t * menu;
const char * menu_Bootload = "Bootloader";

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

    ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

    REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

    menu = initMenu("USB Sandbox", mainMenuCb);
    addSingleItemToMenu(menu, mainMenuMode.modeName);
    addSingleItemToMenu(menu, menu_Bootload);

    loadWsg("kid0.wsg", &example_sprite, true);

	// Try to reinstall, just in case.
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = GPIO_NUM_3,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_io_num       = GPIO_NUM_41,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 1000000,
        .clk_flags        = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };

	//i2c_driver_delete( I2C_NUM_0 );
    ESP_LOGI( "sandbox", "i2c_param_config=%d", i2c_param_config(I2C_NUM_0, &conf) );
	ESP_LOGI( "sandbox", "i2c_driver_install=%d", i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0) );

	LMS6DS3Setup();
	GeneralSet( QMC6308_ADDRESS, 0x0b, 0x80 );
	GeneralSet( QMC6308_ADDRESS, 0x0b, 0x03 );
	GeneralSet( QMC6308_ADDRESS, 0x0a, 0x83 );

    ESP_LOGI( "sandbox", "Loaded" );
}

void sandbox_exit()
{
}

void sandbox_tick()
{
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

    int i;
    char ctsbuffer[1024];
    char *cts = ctsbuffer;

#if 0
	// 0x12 = QMA7981
	// 0x2c = QMC6308 
	// 0x6a = LSM6DSL
    for( i = 0; i < 128; i++ )
    {
        if( !(i & 0xf) )
        {
            cts+=sprintf( cts, "\n%02x: ", i );
        }

        i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
        i2c_master_start(cmdHandle);
        i2c_master_write_byte(cmdHandle, i << 1, false);
        i2c_master_write_byte(cmdHandle, 0, true);
        i2c_master_stop(cmdHandle);
        esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmdHandle, 100);
        i2c_cmd_link_delete(cmdHandle);

        cts+=sprintf( cts, "%2x ", (err>=0)?i:0 );
    }
    ESP_LOGI( "sandbox", "%s", ctsbuffer );
#endif

#if 0
	uint8_t rxbuf[128];
	cts = ctsbuffer;
	int r = GeneralI2CGet( 0x6a, 0, rxbuf, 96 );
	for( i = 0; i < 96; i++ )
	{
		if( ( i & 0x7 ) == 0 ) 		cts += sprintf( cts, "\n%02x: ", i );
		cts += sprintf( cts, " %02x", rxbuf[i] );
	}
	ESP_LOGI( "I2C", "%s\n", ctsbuffer );
#endif


#if 0

	uint8_t rxbuf[128];
	cts = ctsbuffer;

	int r = GeneralI2CGet( LSM6DSL_ADDRESS, 0x20, rxbuf, 2 );
	int temp = rxbuf[0] | rxbuf[1]<<8;

	uint16_t data[64];
	int readr = ReadLSM6DSL( data, sizeof( data ) );

	cts += sprintf( cts, "TEMP: %d %d ", temp, readr );

	for( i = 0; i < readr; i++ )
	{
		cts += sprintf( cts, " %04x", data[i] );
	}
#endif

	LSM6DSLIntegrate();

	cts += sprintf( cts, "%d / %5d %5d %5d / %ld %ld %ld", LSM6DSL.temp, 
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2], 
		LSM6DSL.gyroaccum[0], LSM6DSL.gyroaccum[1], LSM6DSL.gyroaccum[2] );

	ESP_LOGI( "I2C", "%s", ctsbuffer );
}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
    int i;

    //uint32_t start = getCycleCount();
    fillDisplayArea(x, y, x+w, y+h, 0 );
    for( i = 0; i < 16; i++ )
        fillDisplayArea(i*16+8, y, i*16+16+8, y+16, up*16+i );
    //mode7timing = getCycleCount() - start;
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



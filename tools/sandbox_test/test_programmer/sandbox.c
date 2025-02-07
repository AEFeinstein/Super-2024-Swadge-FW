#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "hal/gpio_types.h"
#include "esp_log.h"
#include "soc/efuse_reg.h"
#include "soc/soc.h"
#include "soc/system_reg.h"
#include "advanced_usb_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/dedic_gpio_reg.h"
#include "soc/dport_access.h"
#include "soc/gpio_sig_map.h"
#include "soc/rtc.h"
#include "freertos/portmacro.h"

// For clock output
#include "esp_system.h"
#include "hal/gpio_types.h"
#include "esp_log.h"
#include "soc/efuse_reg.h"
#include "soc/soc.h"
#include "soc/system_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "soc/i2s_reg.h"
#include "soc/periph_defs.h"
#include "rom/lldesc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include "soc/regi2c_apll.h"
#include "hal/regi2c_ctrl_ll.h"
#include "esp_private/periph_ctrl.h"
#include "esp_private/regi2c_ctrl.h"
#include "hal/clk_tree_ll.h"
#include "pindefs.h"

///SWADGE
#ifdef SWADGE
#include "swadge2024.h"
#include "hdw-tft.h"
#include "mainMenu.h"
font_t logbook;
int global_i;

#if 1
static const tusb_desc_device_t knsDescriptor = {
    .bLength            = 18U,
    .bDescriptorType    = 1,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x303a,
    .idProduct          = 0x4004,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

/// @brief PC string Descriptor
static const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "N/A",              // 1: Manufacturer
    "Swadge In Programmer Mode",    // 2: Product
    "s2-ch32xx-pgm-v0",               // 3: Serials (will be overridden)
    "Swadge HID interface", // 4: HID
};

/// @brief PC report Descriptor
static const uint8_t hid_report_descriptor[] = {TUD_HID_REPORT_DESC_GAMEPAD()};

/// @brief PC Config Descriptor
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1,                                                        // Configuration number
                          1,                                                        // interface count
                          0,                                                        // string index
                          (TUD_CONFIG_DESC_LEN + (CFG_TUD_HID * TUD_HID_DESC_LEN)), // total length
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,                       // attribute
                          100),                                                     // power in mA

    TUD_HID_DESCRIPTOR(0,                             // Interface number
                       4,                             // string index
                       false,                         // boot protocol
                       sizeof(hid_report_descriptor), // report descriptor len
                       0x81,                          // EP In address
                       16,                            // size
                       10),                           // polling interval
};

/// @brief PC tusb configuration
static const tinyusb_config_t programmer_tusb_cfg = {
    .device_descriptor        = &knsDescriptor,
    .string_descriptor        = hid_string_descriptor,
	.string_descriptor_count = sizeof(hid_string_descriptor)/sizeof(hid_string_descriptor[0]),
    .external_phy             = false,
    .configuration_descriptor = hid_configuration_descriptor,
};

extern char serial_string[13];

#endif

#endif

int frameno;

#define DisableISR()            do { XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL); portbenchmarkINTERRUPT_DISABLE(); } while (0)
#define EnableISR()             do { portbenchmarkINTERRUPT_RESTORE(0); XTOS_SET_INTLEVEL(0); } while (0)

#define MAX_IN_TIMEOUT 1000
#include "ch32v003_swio.h"

#include "updi_bitbang.h"

uint32_t pinmask;
uint32_t pinmaskpower;
uint32_t clockpin;
uint8_t retbuff[256];
uint8_t * retbuffptr = 0;
int retisready = 0;
int updi_clocks_per_bit = 0;
int programmer_mode = 0;

short int ch32v003_usb_feature_report( uint8_t * buffer, short unsigned int reqlen, unsigned char is_get );

struct SWIOState state;

void sandbox_main()
{

#ifdef SWADGE
	void tinyusb_set_descriptors( const tinyusb_config_t * dev_descriptor);
	initTusb( (const tinyusb_config_t *)&programmer_tusb_cfg, hid_report_descriptor );
    tinyusb_set_descriptors(&programmer_tusb_cfg);
	extern fnAdvancedUsbHandler advancedUsbHandler;
	advancedUsbHandler = ch32v003_usb_feature_report;
    tusb_init();
	tud_disconnect();
	int i;	for( i = 0; i < 10000; i++ ) asm volatile( "nop" );
	tud_connect();

#endif


	REG_WRITE( IO_MUX_REG(SWIO_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor. This is the actual SWIO.
	REG_WRITE( IO_MUX_REG(SWCLK_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor. This is the actual SWCLK.


#ifdef SWADGE
	// I have no idea why this is needed.
    gpio_config_t reset_gpios = {
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << SWIO_PIN) | (1ULL << SWCLK_PIN),
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&reset_gpios);

	REG_WRITE( GPIO_FUNC42_OUT_SEL_CFG_REG, SIG_GPIO_OUT_IDX );
	REG_WRITE( GPIO_FUNC40_OUT_SEL_CFG_REG, SIG_GPIO_OUT_IDX );
#endif

#ifdef VDD3V3_EN_PIN
	REG_WRITE( IO_MUX_REG(VDD3V3_EN_PIN), 1<<FUN_IE_S | 1<<FUN_PD_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.
#endif
#ifdef VDD5V_EN_PIN
	REG_WRITE( IO_MUX_REG(VDD5V_EN_PIN), 1<<FUN_IE_S | 1<<FUN_PD_S | 3<<FUN_DRV_S );  //5V for part 40mA drive.
#endif
#ifdef SWIO_PU_PIN
	REG_WRITE( IO_MUX_REG(SWIO_PU_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //SWPUC
	GPIO.out_w1ts = 1<<SWIO_PU_PIN;
	GPIO.enable_w1ts = 1<<SWIO_PU_PIN;
#endif
#ifdef SWCLK_PU_PIN
	REG_WRITE( IO_MUX_REG(SWCLK_PU_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //SWPUC
	GPIO.out_w1ts = 1<<SWCLK_PU_PIN;
	GPIO.enable_w1ts = 1<<SWCLK_PU_PIN;
#endif

	retbuffptr = retbuff;


	memset( &state, 0, sizeof( state ) );
#if SWIO_PIN > 31
	state.pinmaskD = 1<<(SWIO_PIN-32);
	// For UPDI
	pinmask = (1<<(SWIO_PIN-32));
#else
	state.pinmaskD = 1<<SWIO_PIN;
	// For UPDI
	pinmask = (1<<SWIO_PIN);
#endif

#if SWCLK_PIN > 31
	state.pinmaskC = 1<<(SWCLK_PIN-32);
#else
	state.pinmaskC = 1<<SWCLK_PIN;
#endif


#if defined( VDD5V_EN_PIN ) || defined( VDD3V3_EN_PIN )
	pinmaskpower = (1<<VDD3V3_EN_PIN) | (1<<VDD5V_EN_PIN);
	GPIO.out_w1ts = pinmaskpower;
	GPIO.enable_w1ts = pinmaskpower;
#endif
	GPIO_VAR_W1TS = state.pinmaskD;
	GPIO_VAR_ENABLE_W1TS = state.pinmaskD;

	esp_rom_delay_us(5000);

	rtc_cpu_freq_config_t m;
	rtc_clk_cpu_freq_get_config( &m );

	updi_clocks_per_bit = UPDIComputeClocksPerBit( m.freq_mhz, 115200 );

	switch( m.freq_mhz )
	{
	case 240:
		state.t1coeff = 5; // 9 or 10 is good.  5 is too low. 13 is sometimes too high. ***BUT*** WHEN RUNNING FROM SANDBOX NEED BE HIGHER
		break;
	default:
		state.t1coeff = 100; // Untested At Other Speeds
		break;
	}

	programmer_mode = 0;
	retbuffptr = retbuff;

#if 0
//	DoSongAndDanceToEnterPgmMode( t1coeff, pinmask );
	WriteReg32( t1coeff, pinmask, 0x7e, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
	WriteReg32( t1coeff, pinmask, 0x7d, 0x5aa50000 | (1<<10) ); // CFGR (1<<10 == Allow output from slave)

	uint32_t rval = 0;
	int r = ReadReg32( t1coeff, pinmask, 0x7c, &rval ); // Capability Register (CPBR)
	uprintf( "CPBR: %d - %08x %08x\n", r, rval, REG_READ( GPIO_IN_REG ) );
	
	#if 0
	WriteReg32( t1coeff, pinmask, CDMCONTROL, 0x80000001 ); // Make the debug module work properly.
	WriteReg32( t1coeff, pinmask, CDMCONTROL, 0x80000001 ); // Initiate a halt request.
	WriteReg32( t1coeff, pinmask, CDMCONTROL, 1 ); // Clear halt request bit.
	WriteReg32( t1coeff, pinmask, CDMCONTROL, 0x40000001 ); // Resume
	#endif
	
	r = ReadReg32( t1coeff, pinmask, 0x11, &rval ); // 
	uprintf( "DMSTATUS: %d - %08x %08x\n", r, rval, REG_READ( GPIO_IN_REG ) );
#endif
	frameno = 0;

#ifdef SWADGE
	ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );
	loadFont( "logbook.font", &logbook, false );
#endif

}

void teardown()
{
	// Power-Down
#if  SWIO_PIN < 32
	GPIO_VAR_W1TC = 1<<SWIO_PIN;
	GPIO_VAR_W1TS = 1<<SWIO_PIN;
#else
	GPIO_VAR_W1TC = 1<<(SWIO_PIN-32);
	GPIO_VAR_W1TS = 1<<(SWIO_PIN-32);
#endif

#ifdef VDD3V3_EN_PIN
	GPIO.out_w1tc = 1<<VDD3V3_EN_PIN;
#endif
#ifdef VDD5V_EN_PIN
	GPIO.out_w1tc = 1<<VDD5V_EN_PIN;	
#endif
#ifdef SWIO_PU_PIN
	GPIO.out_w1tc = 1<<SWIO_PU_PIN;
#endif
}

void sandbox_tick()
{
//	if( frameno & 8 )
//		GPIO_VAR_ENABLE_W1TS = state.pinmaskD;
//	else
//		GPIO_VAR_ENABLE_W1TC = state.pinmaskD;
//	GPIO_VAR_W1TC = state.pinmaskD;
	char cts[32];
	snprintf( cts, sizeof(cts)-1, "F: %d %x", frameno, GPIO_VAR_IN );
    int16_t tWidth            = textWidth(&logbook, cts);
	drawText(&logbook, c000, cts, (TFT_WIDTH - tWidth) / 2-2, (TFT_HEIGHT - logbook.height) / 2+2);
	drawText(&logbook, c555, cts, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - logbook.height) / 2);
	frameno++;
//	GPIO_VAR_W1TS = state.pinmaskD;
}

// Configures APLL = 480 / 4 = 120
// 40 * (SDM2 + SDM1/(2^8) + SDM0/(2^16) + 4) / ( 2 * (ODIV+2) );
// Datasheet recommends that numerator does not exceed 500MHz.
void local_rtc_clk_apll_enable(bool enable, uint32_t sdm0, uint32_t sdm1, uint32_t sdm2, uint32_t o_div)
{
	REG_SET_FIELD(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_PLLA_FORCE_PD, enable ? 0 : 1);
	REG_SET_FIELD(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_PLLA_FORCE_PU, enable ? 1 : 0);

	if (enable) {
		REGI2C_WRITE_MASK(I2C_APLL, I2C_APLL_DSDM2, sdm2);
		REGI2C_WRITE_MASK(I2C_APLL, I2C_APLL_DSDM0, sdm0);
		REGI2C_WRITE_MASK(I2C_APLL, I2C_APLL_DSDM1, sdm1);
		REGI2C_WRITE(I2C_APLL, I2C_APLL_SDM_STOP, CLK_LL_APLL_SDM_STOP_VAL_1);
		REGI2C_WRITE(I2C_APLL, I2C_APLL_SDM_STOP, CLK_LL_APLL_SDM_STOP_VAL_2_REV1);
		REGI2C_WRITE_MASK(I2C_APLL, I2C_APLL_OR_OUTPUT_DIV, o_div);
	}
}


void SwitchMode( uint8_t ** liptr, uint8_t ** lretbuffptr )
{
	programmer_mode = *(*liptr++);
	// Unknown Programmer
	(*lretbuffptr)[0] = 0;
	(*lretbuffptr)[1] = programmer_mode;
	(*lretbuffptr) += 2;
	uprintf( "Changing programming mode to %d\n", programmer_mode );
}

short int ch32v003_usb_feature_report( uint8_t * buffer, short unsigned int reqlen, unsigned char is_get )
{
	if( is_get )
	{
		if( !retisready ) { buffer[0] = 0xff; return reqlen; }
		retisready = 0;
		int len = retbuffptr - retbuff;
		buffer[0] = len;
		if( len > reqlen-1 ) len = reqlen-1;
		memcpy( buffer+1, retbuff, len );
		retbuffptr = retbuff;
		return reqlen;
	}

	// Is send.
	// buffer[0] is the request ID.
	uint8_t * iptr = &buffer[1];
	while( iptr - buffer < reqlen )	
	{
		uint8_t cmd = *(iptr++);
		int remain = reqlen - (iptr - buffer);
		// Make sure there is plenty of space.
		if( (sizeof(retbuff)-(retbuffptr - retbuff)) < 6 ) break;

		uprintf( "CMD: %02x\n", cmd );
		if( programmer_mode == 0 )
		{
			if( cmd == 0xfe ) // We will never write to 0x7f.
			{
				cmd = *(iptr++);
				uprintf( "SUBMCD: %02x\n", cmd );

				switch( cmd )
				{
				case 0xfd:
					SwitchMode( &iptr, &retbuffptr );
					break;
				case 0x01:
					//DoSongAndDanceToEnterPgmMode( &state );
					// This was determind not to be needed.
					REG_WRITE( IO_MUX_REG(SWCLK_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor. This is the actual SWCLK.
#ifdef SWCLK_PU_PIN
					REG_WRITE( IO_MUX_REG(SWCLK_PU_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //SWPUC
#endif
					REG_WRITE( IO_MUX_REG(SWIO_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor. This is the actual SWIO.
#ifdef SWIO_PU_PIN
					REG_WRITE( IO_MUX_REG(SWIO_PU_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //SWPUC
#endif
					InitializeSWDSWIO( &state );
					break;
				case 0x02: // Power-down 
					uprintf( "Power down\n" );
					// Make sure clock is disabled.
#ifdef MULTI2_PIN
					gpio_matrix_out( GPIO_NUM(MULTI2_PIN), 254, 1, 0 );
#endif
					GPIO_VAR_W1TC = state.pinmaskD;
					GPIO_VAR_W1TC = state.pinmaskC;
					GPIO_VAR_ENABLE_W1TS = state.pinmaskD;
					GPIO_VAR_ENABLE_W1TS = state.pinmaskC;
#if defined( VDD5V_EN_PIN ) || defined( VDD3V3_EN_PIN )
					GPIO.enable_w1tc = pinmaskpower;
					GPIO.out_w1tc = pinmaskpower;
#endif
					break;
				case 0x03: // Power-up
					uprintf( "Power Up\n" );
#if defined( VDD5V_EN_PIN ) || defined( VDD3V3_EN_PIN )
					GPIO.out_w1ts = pinmaskpower;
					GPIO.enable_w1ts = pinmaskpower;
#endif
					GPIO_VAR_ENABLE_W1TS = state.pinmaskD;
					GPIO_VAR_ENABLE_W1TS = state.pinmaskC;
					GPIO_VAR_W1TS = state.pinmaskD;
					//gpio_matrix_out( GPIO_NUM(SWCLK_PIN), CLK_I2S_MUX_IDX, 1, 0 );
					break;
				case 0x04: // Delay( uint16_t us )
					esp_rom_delay_us(iptr[0] | (iptr[1]<<8) );
					iptr += 2;
					break;
				case 0x05: // Void High Level State
					ResetInternalProgrammingState( &state );
					break;
				case 0x06: // Wait-for-flash-op.
					*(retbuffptr++) = WaitForFlash( &state );
					break;
				case 0x07: // Wait-for-done-op.
					*(retbuffptr++) = WaitForDoneOp( &state );
					break;
				case 0x08: // Write Data32.
				{
					if( remain >= 9 )
					{
						int r = WriteWord( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24),  iptr[4] | (iptr[5]<<8) | (iptr[6]<<16) | (iptr[7]<<24) );
						iptr += 8;
						*(retbuffptr++) = r;
					}
					break;
				}
				case 0x09: // Read Data32.
				{
					if( remain >= 5 )
					{
						int r = ReadWord( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24), (uint32_t*)&retbuffptr[1] );
						iptr += 4;
						retbuffptr[0] = r;
						if( r < 0 )
							*((uint32_t*)&retbuffptr[1]) = 0;
						retbuffptr += 5;
					}
					break;
				}
				case 0x0a:
					ResetInternalProgrammingState( &state );
					break;
				case 0x0b:
					if( remain >= 68 )
					{
						int r = Write64Block( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24), (uint8_t*)&iptr[4] );
						iptr += 68;
						*(retbuffptr++) = r;
					}
					break;
				case 0x0c:
					if( remain >= 8 )
					{
						// Output clock on P2.

						// Maximize the drive strength.
#ifdef MULTI2_PIN
						gpio_set_drive_capability( GPIO_NUM(MULTI2_PIN), GPIO_DRIVE_CAP_2 );

						// Use the IO matrix to create the inverse of TX on pin 17.
						gpio_matrix_out( GPIO_NUM(MULTI2_PIN), CLK_I2S_MUX_IDX, 1, 0 );

						periph_module_enable(PERIPH_I2S0_MODULE);

						int use_apll = *(iptr++);  // try 1
						int sdm0 = *(iptr++);      // try 0
						int sdm1 = *(iptr++);      // try 0
						int sdm2 = *(iptr++);      // try 8
						int odiv = *(iptr++);      // try 0
						iptr +=3 ; // reserved.

						local_rtc_clk_apll_enable( use_apll, sdm0, sdm1, sdm2, odiv );

						if( use_apll )
						{
							WRITE_PERI_REG( I2S_CLKM_CONF_REG(0), (1<<I2S_CLK_SEL_S) | (1<<I2S_CLK_EN_S) | (0<<I2S_CLKM_DIV_A_S) | (0<<I2S_CLKM_DIV_B_S) | (2<<I2S_CLKM_DIV_NUM_S) );
						}
						else
						{
							// fI2S = fCLK / ( N + B/A )
							// DIV_NUM = N
							// Note I2S_CLKM_DIV_NUM minimum = 2 by datasheet.  Less than that and it will ignoreeee you.
							WRITE_PERI_REG( I2S_CLKM_CONF_REG(0), (2<<I2S_CLK_SEL_S) | (1<<I2S_CLK_EN_S) | (0<<I2S_CLKM_DIV_A_S) | (0<<I2S_CLKM_DIV_B_S) | (1<<I2S_CLKM_DIV_NUM_S) );  // Minimum reduction, 2:1
						}
#endif
					}
					break;
				case 0x0d:  // Do a terminal log through.
				{
					int tries = 100;
					if( remain >= 8 )
					{
						int r;
						uint32_t leavevalA = iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24);
						iptr += 4;
						uint32_t leavevalB = iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24);
						iptr += 4;
						uint8_t * origretbuf = (retbuffptr++);
						int canrx = (sizeof(retbuff)-(retbuffptr - retbuff)) - 8;
						while( canrx > 8 )
						{
							r = PollTerminal( &state, retbuffptr, canrx, leavevalA, leavevalB );
							origretbuf[0] = r;
							if( r >= 0 )
							{
								retbuffptr += r;
								if( tries-- <= 0 ) break; // ran out of time?
							}
							else
							{
								break;
							}
							canrx = (sizeof(retbuff)-(retbuffptr - retbuff)) -8;
							// Otherwise all is well.  If we aren't signaling try to poll for more data.
							if( leavevalA != 0 || leavevalB != 0 ) break;
						}
					}
					break;
				}
				// Done

				}
			} else if( cmd == 0xff )
			{
				break;
			}
			else
			{
				// Otherwise it's a regular command.
				// 7-bit-cmd .. 1-bit read(0) or write(1) 
				// if command lines up to a normal QingKeV2 debug command, treat it as that command.
				uprintf( "OTHER CMD: %02x %d\n", cmd, remain );
				if( cmd & 1 )
				{
					if( remain >= 4 )
					{
						uprintf( "MCFWritereg( %02x: %08x\n", cmd>>1, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24) );
						MCFWriteReg32( &state, cmd>>1, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24) );
						iptr += 4;
					}
				}
				else
				{
					if( remain >= 1 && (sizeof(retbuff)-(retbuffptr - retbuff)) >= 4 )
					{
						int r = MCFReadReg32( &state, cmd>>1, (uint32_t*)&retbuffptr[1] );
						uprintf( "MCFReadreg( %02x: %08x ) = %04x\n", cmd>>1, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24), *(uint32_t*)&retbuffptr[1] );
						retbuffptr[0] = r;
						if( r < 0 )
							*((uint32_t*)&retbuffptr[1]) = 0;
						retbuffptr += 5;
					}
				}
			}
		}
		else if( programmer_mode == 1 )
		{
			if( cmd == 0xff ) break;

			switch( cmd )
			{
			case 0xfe:
			{
				cmd = *(iptr++);
				if( cmd == 0xfd )
				{
					SwitchMode( &iptr, &retbuffptr );
				}
				break;
			}
			case 0x90:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.
				rtc_cpu_freq_config_t m;
				rtc_clk_cpu_freq_get_config( &m );

				if( (sizeof(retbuff)-(retbuffptr - retbuff)) >= 18 && remain >= 2 )
				{
					int baudrate = *(iptr++);
					baudrate |= (*(iptr++))<<8;

					updi_clocks_per_bit = UPDIComputeClocksPerBit( m.freq_mhz, baudrate*32 );


					UPDIPowerOn( pinmask, pinmaskpower );
					uint8_t sib[17] = { 0 };
					int r = UPDISetup( pinmask, m.freq_mhz, updi_clocks_per_bit, sib );
					uprintf( "UPDISetup() = %d -> %s\n", r, sib );

					retbuffptr[0] = r;
					memcpy( retbuffptr + 1, sib, 17 );
					retbuffptr += 18;
				}
				break;
			}
			case 0x91:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.
				UPDIPowerOn( pinmask, pinmaskpower );
				break;
			}
			case 0x92:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				UPDIPowerOff( pinmask, pinmaskpower );
				break;
			}
			case 0x93: // Flash 64-byte block.
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				if( remain >= 2+64 )
				{
					int addytowrite = *(iptr++);
					addytowrite |= (*(iptr++))<<8;
					int r;
					r = UPDIFlash( pinmask, updi_clocks_per_bit, addytowrite, iptr, 64, 0);
					uprintf( "Flash Response: %d\n", r );
					iptr += 64;

					*(retbuffptr++) = r;
				}
				break;
			}
			case 0x94:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				if( remain >= 3 )
				{
					int addytorx = *(iptr++);
					addytorx |= (*(iptr++))<<8;
					int bytestorx = *(iptr++);

					if( (sizeof(retbuff)-(retbuffptr - retbuff)) >= bytestorx + 1 )
					{
						retbuffptr[0] = UPDIReadMemoryArea( pinmask, updi_clocks_per_bit, addytorx, (uint8_t*)&retbuffptr[1], bytestorx );
						retbuffptr += bytestorx + 1;
					}
				}
				break;
			}
			case 0x95:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				*(retbuffptr++) = UPDIErase( pinmask, updi_clocks_per_bit );
				break;
			}
			case 0x96:
			{
				*(retbuffptr++) = UPDIReset( pinmask, updi_clocks_per_bit );

				break;
			}
			default:
				*(retbuffptr++) = 0xfe;
				*(retbuffptr++) = cmd;
				break;
			}
		}
		else
		{
			// Unknown Programmer
			*(retbuffptr++) = 0;
			*(retbuffptr++) = programmer_mode;

			break;
		}
	}

	retisready = 1;

	return 0;
}

#ifdef SWADGE

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
#if 1
    int i;

    fillDisplayArea(x, y, x+w, y+h, 0 );
    for( i = 0; i < 16; i++ )
        fillDisplayArea(i*16+8, y, i*16+16+8, y+16, up*16+i );
#endif
}


swadgeMode_t sandbox_mode =
{
    .wifiMode                 = NO_WIFI,
    .modeName                 = "programmer",
    .overrideUsb              = true,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
	.fnEnterMode = sandbox_main,
	.fnMainLoop = sandbox_tick,
	.fnExitMode = teardown,
	.fnAdvancedUSB = ch32v003_usb_feature_report,
    .fnBackgroundDrawCallback = sandboxBackgroundDrawCallback,
};

#else

struct SandboxStruct sandbox_mode =
{
	.fnIdle = sandbox_tick,
	.fnDecom = teardown,
	.fnAdvancedUSB = ch32v003_usb_feature_report
};


#endif



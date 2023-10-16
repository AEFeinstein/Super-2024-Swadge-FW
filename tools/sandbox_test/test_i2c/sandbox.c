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
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "soc/i2c_reg.h"
#include "soc/gpio_struct.h"
#include "coreutil.h"
#include "hdw-btn.h"

#include "bunny.h"

#include "hdw-imu.h"

int16_t bunny_verts_out[ sizeof(bunny_verts)/3/2*3 ];

int frameno;
int bQuit;

#define LSM6DSL_ADDRESS						0x6a

typedef enum __attribute__((packed))
{
	LSM6DSL_FUNC_CFG_ACCESS				= 0x01,
	LSM6DSL_SENSOR_SYNC_TIME_FRAME		= 0x04,
	LSM6DSL_FIFO_CTRL1					= 0x06,
	LSM6DSL_FIFO_CTRL2					= 0x07,
	LSM6DSL_FIFO_CTRL3					= 0x08,
	LSM6DSL_FIFO_CTRL4					= 0x09,
	LSM6DSL_FIFO_CTRL5					= 0x0a,
	LSM6DSL_ORIENT_CFG_G				= 0x0b,
	LSM6DSL_INT1_CTRL					= 0x0d,
	LSM6DSL_INT2_CTRL					= 0x0e,
	LMS6DS3_WHO_AM_I					= 0x0f,
	LSM6DSL_CTRL1_XL					= 0x10,
	LSM6DSL_CTRL2_G						= 0x11,
	LSM6DSL_CTRL3_C						= 0x12,
	LSM6DSL_CTRL4_C						= 0x13,
	LSM6DSL_CTRL5_C						= 0x14,
	LSM6DSL_CTRL6_C						= 0x15,
	LSM6DSL_CTRL7_G						= 0x16,
	LSM6DSL_CTRL8_XL					= 0x17,
	LSM6DSL_CTRL9_XL					= 0x18,
	LSM6DSL_CTRL10_C					= 0x19,
	LSM6DSL_MASTER_CONFIG				= 0x1a,
	LSM6DSL_WAKE_UP_SRC					= 0x1b,
	LSM6DSL_TAP_SRC						= 0x1c,
	LSM6DSL_D6D_SRC						= 0x1d,
	LSM6DSL_STATUS_REG					= 0x1e,
	LSM6DSL_OUT_TEMP_L					= 0x20,
	LSM6DSL_OUT_TEMP_H					= 0x21,
	LMS6DS3_OUTX_L_G					= 0x22,
	LMS6DS3_OUTX_H_G					= 0x23,
	LMS6DS3_OUTY_L_G					= 0x24,
	LMS6DS3_OUTY_H_G					= 0x25,
	LMS6DS3_OUTZ_L_G					= 0x26,
	LMS6DS3_OUTZ_H_G					= 0x27,
	LMS6DS3_OUTX_L_XL					= 0x28,
	LMS6DS3_OUTX_H_XL					= 0x29,
	LMS6DS3_OUTY_L_XL					= 0x2a,
	LMS6DS3_OUTY_H_XL					= 0x2b,
	LMS6DS3_OUTZ_L_XL					= 0x2c,
	LMS6DS3_OUTZ_H_XL					= 0x2d,
} lsm6dslReg_t;
float rsqrtf(float x);
float mathsqrtf(float x);
void mathEulerToQuat(float * q, const float * euler);
void mathQuatApply(float * qout, const float * q1, const float * q2);
void mathQuatNormalize(float * qout, const float * qin );
void mathCrossProduct(float * p, const float * a, const float * b);
void mathRotateVectorByInverseOfQuaternion(float * pout, const float * q, const float * p );
void mathRotateVectorByQuaternion(float * pout, const float * q, const float * p);
esp_err_t LSM6DSLSet( int reg, int val );
int GeneralI2CGet( int device, int reg, uint8_t * data, int data_len );
int ReadLSM6DSL( uint8_t * data, int data_len );

#if 1

// For main add 	ESP_LOGI( "test", "%p %p %p %p %p\n", &i2c_driver_delete, &LSM6DSLSet, &rsqrtf, &mathCrossProduct, &accelIntegrate );

static void LSM6DSLIntegrate()
{
	LSM6DSLData * ld = &LSM6DSL;

	int16_t data[6*16];

	// Get temperature sensor (in case we ever want to use it)
	int r = GeneralI2CGet( LSM6DSL_ADDRESS, 0x20, (uint8_t*)data, 2 );
	if( r < 0 ) return;
	if( r == 2 ) ld->temp = data[0];
	int readr = ReadLSM6DSL( (uint8_t*)data, sizeof( data ) );

	if( readr < 0 ) return;
	int samp;
	int16_t * cdata = data;

    uint32_t start = getCycleCount();

	// STEP 0:  Decide your coordinate frame.

	// [0] = +X axis coming out right of controller.
	// [1] = +Y axis, pointing straight up out of controller, out where the USB port is.
	// [2] = +Z axis, pointing up from the face of the controller.

	ld->lastreadr = readr;

	for( samp = 0; samp < readr; samp+=6 )
	{
		// Extract data from IMU
		int16_t * euler_deltas = cdata; // Euler angles, from gyro.
		int16_t * accel_data   = cdata + 3;

		// Manual cal, used only for Steps 2..8
		//	euler_deltas[0] -= 12;
		//	euler_deltas[1] += 22;
		//	euler_deltas[2] += 4;

		// We can sum rotations to understand the amount of counts in a full circle.
		// Note: this is actually more of a debug mechanism.
		ld->gyroaccum[0] += euler_deltas[0];
		ld->gyroaccum[1] += euler_deltas[1];
		ld->gyroaccum[2] += euler_deltas[2];

		// STEP 1:  Visually inspect the gyro values.
		// STEP 2:  Integrate the gyro values, verify they are correct.

		// 2000 dps full-scale
		// 32768 is full-scale
		// 208 SPS
		// convert to radians. ( 2000.0f / 32768.0f / 208.0f * 2.0 * 3.14159f / 180.0f );  
		// Measured = 560,000 counts per scale (Measured by looking at sum)
		// Testing -> 3.14159 * 2.0 / 566000;
		float fFudge = 0.5 * 1.15; //XXX TODO: Investigate.
		float fScale = ( 2000.0f / 32768.0f / 208.0f * 2.0 * 3.14159f / 180.0f ) * fFudge;

		// STEP 3:  Integrate gyro values into a quaternion.
		// This step is validated by working with just one axis at a time
		// then apply a coordinate frame to ld->fqQuat and validate that it is
		// correct.
		float fEulerScales[3] = {
			-fScale,
			 fScale,
			-fScale };

		float fEulers[3] = {
			euler_deltas[0] * fEulerScales[0],
			euler_deltas[1] * fEulerScales[1],
			euler_deltas[2] * fEulerScales[2] };

		// Used for calibration
		if( ld->performCal )
		{
			float diff[3] = {
				fEulers[0] - ld->fvAverage[0],
				fEulers[1] - ld->fvAverage[1],
				fEulers[2] - ld->fvAverage[2] };

			float diffsq[3] = {
				(diff[0]<0)?-diff[0]:diff[0],
				(diff[1]<0)?-diff[1]:diff[1],
				(diff[2]<0)?-diff[2]:diff[2] };

			diffsq[0] *= 1000.0;
			diffsq[1] *= 1000.0;
			diffsq[2] *= 1000.0;

			ld->fvDeviation[0] -= 0.004;
			ld->fvDeviation[1] -= 0.004;
			ld->fvDeviation[2] -= 0.004;

			if( ld->fvDeviation[0] < diffsq[0] ) ld->fvDeviation[0] = diffsq[0];
			if( ld->fvDeviation[1] < diffsq[1] ) ld->fvDeviation[1] = diffsq[1];
			if( ld->fvDeviation[2] < diffsq[2] ) ld->fvDeviation[2] = diffsq[2];

			if( ld->fvDeviation[0] > 0.8 ) ld->fvDeviation[0] = 0.8;
			if( ld->fvDeviation[1] > 0.8 ) ld->fvDeviation[1] = 0.8;
			if( ld->fvDeviation[2] > 0.8 ) ld->fvDeviation[2] = 0.8;

			diff[0] *= mathsqrtf( ld->fvDeviation[0] ) * 0.5;
			diff[1] *= mathsqrtf( ld->fvDeviation[1] ) * 0.5;
			diff[2] *= mathsqrtf( ld->fvDeviation[2] ) * 0.5;

			ld->fvAverage[0] += diff[0];
			ld->fvAverage[1] += diff[1];
			ld->fvAverage[2] += diff[2];

			// Compute the running RMS error.
			float fvEuler = (
				ld->fvDeviation[0] * ld->fvDeviation[0] +
				ld->fvDeviation[1] * ld->fvDeviation[1] +
				ld->fvDeviation[2] * ld->fvDeviation[2] );

			if( fvEuler < 0.00015f )
			{
				ld->fvBias[0] = -ld->fvAverage[0];
				ld->fvBias[1] = -ld->fvAverage[1];
				ld->fvBias[2] = -ld->fvAverage[2];
				writeNvs32( "gyrocalx", *(int32_t*)(&ld->fvBias[0]) );
				writeNvs32( "gyrocaly", *(int32_t*)(&ld->fvBias[1]) );
				writeNvs32( "gyrocalz", *(int32_t*)(&ld->fvBias[2]) );
				ld->performCal = 0;
			}
		}

		fEulers[0] += ld->fvBias[0];
		fEulers[1] += ld->fvBias[1];
		fEulers[2] += ld->fvBias[2];
		mathEulerToQuat( ld->fqQuatLast, fEulers );
		mathQuatApply( ld->fqQuat, ld->fqQuat, ld->fqQuatLast );

		// STEP 4: Validate yor values by doing 4 90 degree turns
		//  across multiple axes.
		// i.e. rotate controller down, clockwise from top, up, counter-clockwise.
		// while investigating quaternion.  It should return to identity.

		// STEP 6: Determine our "error" based on accelerometer.
		// NOTE: This step could be done on the inner loop if you want, and done over
		// every accelerometer cycle, or it can be done on the outside, every few cycles.
		// all that realy matters is that it is done periodically.

		// STEP 6A: Examine vectors.  Generally speaking, we want an "up" vector, not a gravity vector.
		// this is "up" in the controller's point of view.
		float accel_up[3] = { 
			-accel_data[0],
			 accel_data[1],
			-accel_data[2]
		};

		float accel_inverse_mag = rsqrtf( accel_up[0] * accel_up[0] + accel_up[1] * accel_up[1] + accel_up[2] * accel_up[2] );
		accel_up[0] *= accel_inverse_mag;
		accel_up[1] *= accel_inverse_mag;
		accel_up[2] *= accel_inverse_mag;


		// Step 6B: Next, compute what we think "up" should be from our point of view.  We will use +Y Up.
		float what_we_think_is_up[3] = { 0, 1, 0 };
		mathRotateVectorByInverseOfQuaternion( what_we_think_is_up, LSM6DSL.fqQuat, what_we_think_is_up );

		// Step 6C: Next, we determine how far off we are.  This will tell us our error.
		float corrective_quaternion[4];

		// TRICKY: The ouput of this is actually the axis of rotation, which is ironically
		// in vector-form the same as a quaternion.  So we can write directly into the quat.
		mathCrossProduct( corrective_quaternion + 1, accel_up, what_we_think_is_up );

		// Now, we apply this in step 7.

		// First, we can compute what the drift values of our axes are, to anti-drift them.
		// If you do only this, you will always end up in an unstable oscillation. 
		memcpy( ld->fCorrectLast, corrective_quaternion+1, 12 );
		// XXX TODO: We need to multiply by amount the accelerometer gives us assurance.
		//ld->fvBias[0] += mathsqrtf(corrective_quaternion[1]) * 0.0000002;
		//ld->fvBias[1] += mathsqrtf(corrective_quaternion[2]) * 0.0000002;
		//ld->fvBias[2] += mathsqrtf(corrective_quaternion[3]) * 0.0000002;

		float corrective_force = (ld->sampCount++ == 0) ? 0.5f : 0.0005f;

		// Second, we can apply a very small corrective tug.  This helps prevent oscillation
		// about the correct answer.  This acts sort of like a P term to a PID loop.
		// This is actually the **primary**, or fastest responding thing.
		corrective_quaternion[1] *= corrective_force;
		corrective_quaternion[2] *= corrective_force;
		corrective_quaternion[3] *= corrective_force;

		// x^2+y^2+z^2+q^2 -> ALGEBRA! -> sqrt( 1-x^2-y^2-z^2 ) = w
		corrective_quaternion[0] = mathsqrtf( 1 
			- corrective_quaternion[1]*corrective_quaternion[1]
			- corrective_quaternion[2]*corrective_quaternion[2]
			- corrective_quaternion[3]*corrective_quaternion[3] );

		mathQuatApply( ld->fqQuat, ld->fqQuat, corrective_quaternion );

		// Magnitude of correction angle = inverse_sin( magntiude( axis_of_correction ) );
		// We want to significantly reduce that. To mute any effect.

		cdata += 6;
	}

	// Now we move onto the outer loop.
	// STEP 5: We now want to normalize the quat periodically.  Don't do this too
	// soon, otherwise you won't notice math errors.  Realistically, this should
	// only need to be done every hundreds of thousands of samples.
	//
	// Also, don't do this too often, otherwise you will reduce accuracy, 
	// unnecessarily.
	float * qRot = ld->fqQuat;
	float qmagsq = qRot[0] * qRot[0] + qRot[1] * qRot[1] + qRot[2] * qRot[2] + qRot[3] * qRot[3];
	if( qmagsq > 1.05 || qmagsq < 0.95 )
	{
		// This normalizes everything.
		qmagsq = rsqrtf( qmagsq );
		qRot[0] = qRot[0] * qmagsq;
		qRot[1] = qRot[1] * qmagsq;
		qRot[2] = qRot[2] * qmagsq;
		qRot[3] = qRot[3] * qmagsq;
	}


	if( samp )
	{
		ld->gyrolast[0] = cdata[-6];
		ld->gyrolast[1] = cdata[-5];
		ld->gyrolast[2] = cdata[-4];
		ld->accellast[0] = cdata[-3];
		ld->accellast[1] = cdata[-2];
		ld->accellast[2] = cdata[-1];
	}

    ld->computetime = getCycleCount() - start;
}
#endif



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
	frameno = 0;
	bQuit = 0;

    ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

    REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

    menu = initMenu("USB Sandbox", mainMenuCb);
    addSingleItemToMenu(menu, mainMenuMode.modeName);
    addSingleItemToMenu(menu, menu_Bootload);

    loadWsg("kid0.wsg", &example_sprite, true);

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
/*
    for( int mode = 0; mode < 8; mode++ )
    {
        drawWsg( &example_sprite, 50+mode*20, (global_i%20)-10, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, 50+mode*20, (global_i%20)+230, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, (global_i%20)-10, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsg( &example_sprite, (global_i%20)+270, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
    }
*/

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        menu = menuButton(menu, evt);
    }


    char ctsbuffer[1024];
    char *cts = ctsbuffer;

#if 0
    int i;
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
/*
	cts += sprintf( cts, "%ld %ld / %5d %5d %5d / %5d %5d %5d / %ld %ld %ld / %f %f %f %f",
		LSM6DSL.computetime, LSM6DSL.temp, 
		LSM6DSL.accellast[0], LSM6DSL.accellast[1], LSM6DSL.accellast[2],
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2], 
		LSM6DSL.fqQuat[0], LSM6DSL.fqQuat[1], LSM6DSL.fqQuat[2], LSM6DSL.fqQuat[3]  );
*/

	float plusy[3] = { 0, 1, 0 };

	// Produce a model matrix from a quaternion.
	float plusx_out[3] = { 1, 0, 0 };
	float plusy_out[3] = { 0, 1, 0 };
	float plusz_out[3] = { 0, 0, 1 };
	mathRotateVectorByQuaternion( plusy, LSM6DSL.fqQuat, plusy );
	mathRotateVectorByQuaternion( plusy_out, LSM6DSL.fqQuat, plusy_out );
	mathRotateVectorByQuaternion( plusx_out, LSM6DSL.fqQuat, plusx_out );
	mathRotateVectorByQuaternion( plusz_out, LSM6DSL.fqQuat, plusz_out );


    uint32_t cycStart = getCycleCount();

	int i, vertices = 0;
	for( i = 0; i < sizeof(bunny_verts)/2; i+= 3 )
	{
		// Performingthe transform this way is about 700us.
		float bx = bunny_verts[i+2];
		float by = bunny_verts[i+1];
		float bz =-bunny_verts[i+0];
		float bunnyvert[3] = {
			bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2],
			bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2],
			bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2] };
		bunny_verts_out[vertices*3+0] = bunnyvert[0]/250 + 280/2; 
		bunny_verts_out[vertices*3+1] = -bunnyvert[1]/250 + 240/2;  // Convert from right-handed to left-handed coordinate frame.
		bunny_verts_out[vertices*3+2] = bunnyvert[2];
		vertices++;
	}

	int lines = 0;
	for( i = 0; i < sizeof(bunny_lines); i+= 2 )
	{
		int v1 = bunny_lines[i]*3;
		int v2 = bunny_lines[i+1]*3;
		float col = bunny_verts_out[v1+2]/2000 + 8;
		if( col > 5 ) col = 5;
		else if( col < 0 ) continue;
		drawLineFast(bunny_verts_out[v1], bunny_verts_out[v1+1],bunny_verts_out[v2], bunny_verts_out[v2+1], col);
		lines++;
	}


    uint32_t renderTime = getCycleCount() - cycStart;


	ESP_LOGI( "I2C", "%d %d  %f %f %f   %f %f %f   %f %f %f",
		(int)LSM6DSL.sampCount, (int)LSM6DSL.performCal,
		LSM6DSL.fvBias[0], LSM6DSL.fvBias[1], LSM6DSL.fvBias[2], 
		LSM6DSL.fvDeviation[0], LSM6DSL.fvDeviation[1], LSM6DSL.fvDeviation[2],
		LSM6DSL.fvAverage[0], LSM6DSL.fvAverage[1], LSM6DSL.fvAverage[2] );

/*
	cts += sprintf( cts, "%ld %ld %d %f %f %f / %f %f %f / %3d %3d %3d / %4d %4d %4d",
		LSM6DSL.computetime, renderTime, LSM6DSL.lastreadr,
		LSM6DSL.fCorrectLast[0], LSM6DSL.fCorrectLast[1], LSM6DSL.fCorrectLast[2],
		LSM6DSL.fvBias[0],		LSM6DSL.fvBias[1],		LSM6DSL.fvBias[2],
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2],
		LSM6DSL.accellast[0], LSM6DSL.accellast[1], LSM6DSL.accellast[2] );

	ESP_LOGI( "I2C", "%s", ctsbuffer );
*/
}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
//	accelIntegrate();
	if( up + 1 == upNum ) 
		LSM6DSLIntegrate();

    fillDisplayArea(x, y, x+w, y+h, 0 );
}


swadgeMode_t sandbox_mode = {
    .modeName                 = "sandbox",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
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



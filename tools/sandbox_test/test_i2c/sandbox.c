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


// GPIO_NUM_3,  // SDA
// GPIO_NUM_41, // SCL

#define DSCL_OUTPUT	{ GPIO.enable1_w1ts.val = 1<<(41-32); }
#define DSCL_INPUT	{ GPIO.enable1_w1tc.val = 1<<(41-32); }
#define DSDA_OUTPUT	{ GPIO.enable_w1ts = 1<<(3); }
#define DSDA_INPUT	{ GPIO.enable_w1tc = 1<<(3); }
#define READ_DSDA	  ( ( GPIO.in >> 3 ) & 1 )

// 14 counts (1MHz) works most of the time, but no hurries, let's slow it down to ~800k.
void i2c_delay( int x ) { int i; for( i = 0; i < 19*x; i++ ) asm volatile( "nop" ); }
#define DELAY1 i2c_delay(1);
#define DELAY2 i2c_delay(2);

#include "static_i2c.h"

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

	LSM6DSL_FIFO_STATUS1                = 0x3A,
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


static int _accelIntegrate();

#if 1

// For main add 	ESP_LOGI( "test", "%p %p %p %p %p\n", &i2c_driver_delete, &LSM6DSLSet, &rsqrtf, &mathCrossProduct, &accelIntegrate );



float rsqrtf(float x)
{
    typedef union
    {
        int32_t i;
        float f;
    } fiunion;
    const float xhalf = 0.5f * x;
    fiunion i         = {.f = x};
    i.i               = 0x5f375a86 - (i.i >> 1);
    x                 = i.f;
    x                 = x * (1.5f - xhalf * x * x);
    x                 = x * (1.5f - xhalf * x * x);
    return x;
}

void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

esp_err_t _GeneralSet(int dev, int reg, int val)
{
	SendStart();
	SendByte( dev << 1 );
	SendByte( reg );
	SendByte( val );
	SendStop();
	return ESP_OK;
}

esp_err_t _LSM6DSLSet(int reg, int val)
{
	return _GeneralSet(LSM6DSL_ADDRESS, reg, val);
}


int _GeneralI2CGet(int device, int reg, uint8_t* data, int data_len)
{
	SendStart();
	SendByte( device << 1 );
	SendByte( reg );
	SendStart();
	SendByte( ( device << 1 ) | 1 );
	int i;
	for( i = 0; i < data_len; i++ )
	{
		data[i] = GetByte( i == data_len - 1 );
	}
	SendStop();
	return data_len;
}

/**
 * @brief Read the FIFO out of the LSM6DSL
 *
 * @param data The buffer to write the FIFO data into.
 * @param data_len The maximum size (in words) to read.
 * @return positive number if operation was successful, or esp_err_t if failure.
 */
int _ReadLSM6DSL(uint8_t* data, int data_len)
{
	uint32_t fifolen = 0;
//	_GeneralI2CGet( LSM6DSL_ADDRESS, 0x3A, (uint8_t*)&fifolen, 3 );

	SendStart();
	SendByte( LSM6DSL_ADDRESS << 1 );
	SendByte( LSM6DSL_FIFO_STATUS1 );
	SendStart();
	SendByte( ( LSM6DSL_ADDRESS << 1 ) | 1 );
	int i;
	for( i = 0; i < 3; i++ )
	{
		((uint8_t*)&fifolen)[i] = GetByte( 0 );
	}

	// Is fifo overflow.
	if (fifolen & 0x4000)
	{
		// reset fifo.
		// If we overflow, and we don't do this, bad things happen.
		GetByte( 1 );
		SendStop();
		_LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b000); // Disable fifo
		_LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b110); // 208 Hz ODR
		LSM6DSL.sampCount = 0;
		return 0;
	}

	fifolen &= 0x7ff;

	if (fifolen > data_len / 2)
		fifolen = data_len / 2;

	int read_len = fifolen / 6 * 12;

	if (read_len == 0)
	{
		GetByte( 1 );
		SendStop();
		return 0;
	}

	GetByte( 0 ); // Ignoring FIFO Status 4
	for( i = 0; i < read_len; i++ )
	{
		data[i] = GetByte( i == read_len - 1 );
	}
	SendStop();
	return fifolen;
}


static int _accelIntegrate()
{
	LSM6DSLData * ld = &LSM6DSL;

	int16_t data[6*32];

	// Get temperature sensor (in case we ever want to use it)
	//int r = _GeneralI2CGet( LSM6DSL_ADDRESS, 0x20, (uint8_t*)data, 2 );
	//if( r < 0 ) return -1;
	//if( r == 2 ) ld->temp = data[0];

	int readr = _ReadLSM6DSL( (uint8_t*)data, sizeof( data ) );

	if( readr < 0 ) return -2;
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

	return ESP_OK;
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

void _accelSetRegistersAndReset(void)
{
	_LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b000); // Reset FIFO
	_LSM6DSLSet(
		LSM6DSL_FIFO_CTRL5,
		(0b0101 << 3)
			| 0b110); // 208 Hz ODR, Continuous mode. If the FIFO is full, the new sample overwrites the older one.
	_LSM6DSLSet(LSM6DSL_FIFO_CTRL3, 0b00001001); // Put both devices (Accel + Gyro) in FIFO.
	_LSM6DSLSet(LSM6DSL_CTRL1_XL, 0b01011001);   // Setup accel (16 g's FS)
	_LSM6DSLSet(LSM6DSL_CTRL2_G, 0b01011100);	// Setup gyro, 2000dps
	_LSM6DSLSet(LSM6DSL_CTRL4_C, 0x00);		  // Disable all filtering.
	_LSM6DSLSet(LSM6DSL_CTRL7_G, 0b00000000);	// Setup gyro, not high performance mode = 0x80.  High perf = 0x00
	_LSM6DSLSet(LSM6DSL_FIFO_CTRL2, 0b00000000); // Temp not in fifo  (Why no work?)

	memset(&LSM6DSL, 0, sizeof(LSM6DSL));
	LSM6DSL.fqQuat[0]	 = 1;
	LSM6DSL.fqQuatLast[0] = 1;
	LSM6DSL.sampCount	 = 0;
	if (!readNvs32("gyrocalx", (int32_t*)&LSM6DSL.fvBias[0]))
	{
		LSM6DSL.performCal = 1;
		LSM6DSL.fvBias[0]  = 0;
	}
	if (!readNvs32("gyrocaly", (int32_t*)&LSM6DSL.fvBias[1]))
	{
		LSM6DSL.performCal = 1;
		LSM6DSL.fvBias[1]  = 0;
	}
	if (!readNvs32("gyrocalz", (int32_t*)&LSM6DSL.fvBias[2]))
	{
		LSM6DSL.performCal = 1;
		LSM6DSL.fvBias[2]  = 0;
	}
}

esp_err_t _initAccelerometer(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz)
{
	int i;
	int retry = 0;
	esp_err_t ret_val;
	i2c_driver_delete(_i2c_port);
do_retry:

	gpio_config_t gsetup = {
		.pin_bit_mask = (1ULL<<sda) | (1ULL<<scl),
		.mode = GPIO_MODE_INPUT_OUTPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
	};

	ret_val = gpio_config( &gsetup );
	ESP_LOGE( "sandbox", "RET: %d", ret_val );

	// This will "shake loose" any devices stuck on the bus.
	GPIO.enable_w1ts = 1<<(3);
	GPIO.enable1_w1ts.val = 1<<(41-32);
	esp_rom_delay_us(10);
	GPIO.out_w1tc = 1<<(3);
	for (i = 0; i < 16; i++)
	{
		esp_rom_delay_us(10);
		GPIO.out1_w1ts.val = 1<<(41-32);
		esp_rom_delay_us(10);
		GPIO.out1_w1tc.val = 1<<(41-32);
	}
	esp_rom_delay_us(10);
	GPIO.out1_w1ts.val = 1<<(41-32);
	esp_rom_delay_us(10);
	GPIO.out_w1ts = 1<<(3);
	esp_rom_delay_us(10);
	GPIO.out1_w1ts.val = 1<<(41-32);  // Send final stop

	ret_val = ESP_OK;

	// Prepare for normal open drain functionality.
	GPIO.enable1_w1tc.val = 1<<(41-32);
	GPIO.enable_w1tc = 1<<(3);
	GPIO.out1_w1tc.val = 1<<(41-32);
	GPIO.out_w1tc = 1<<(3);


	// Enable access
	_LSM6DSLSet(LSM6DSL_FUNC_CFG_ACCESS, 0x20);
	_LSM6DSLSet(LSM6DSL_CTRL3_C, 0x81); // Force reset
	esp_rom_delay_us(100);
	_LSM6DSLSet(LSM6DSL_CTRL3_C, 0x44); // unforce reset

	uint8_t who = 0xaa;
	int r	   = _GeneralI2CGet(LSM6DSL_ADDRESS, LMS6DS3_WHO_AM_I, &who, 1);
	if (r != 1 || who != 0x6a)
	{
		ESP_LOGW("accel", "WHOAMI Failed (%02x), %d", who, r);
		if (retry++ < 10)
			goto do_retry;
		ESP_LOGE("accel", "Init failed on 1");
		return ESP_FAIL;
	}
	ESP_LOGI("accel", "Init Start");

	_accelSetRegistersAndReset();

	for (i = 0; i < 2; i++)
	{
		vTaskDelay(1);
		int check = _accelIntegrate();
		if (check != ESP_OK)
		{
			ESP_LOGI("accel", "Init Fault Retry");
			if (retry++ < 10)
				goto do_retry;
			ESP_LOGI("accel", "Init failed on 2");
			return ESP_FAIL;
		}
		ESP_LOGI("accel", "Check %d", check);
	}

	ESP_LOGI("accel", "Init Ok");
	return ESP_OK;
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


    _initAccelerometer(I2C_NUM_0,
                      GPIO_NUM_3,  // SDA
                      GPIO_NUM_41, // SCL
                      GPIO_PULLUP_ENABLE, 1000000);

	_accelSetRegistersAndReset();

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
	//if( up + 1 == upNum || up == 0 || up == 8 ) 
	_accelIntegrate();

	fillDisplayArea(x, y, x+w, y+h, 0 );
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



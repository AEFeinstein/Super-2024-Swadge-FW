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

#include "bunny.h"

int16_t bunny_verts_out[ sizeof(bunny_verts)/3/2*3 ];


int frameno;

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
	int32_t temp;
	uint32_t caltime;

	// Quats are wxyz.
	// You can take a vector, in controller space, rotate by this quat, and you get it in world space.
	float fqQuatLast[4];
	float fqQuat[4];  // Quats are wxyz

	// Bias for all of the euler angles.
	float fvBias[3];

	// For debug
	int lastreadr;
	int32_t gyroaccum[3];
	uint32_t gyrocount;
	int16_t gyrolast[3];
	int16_t accellast[3];
	float fCorrectLast[3];

} LSM6DSL;

#include <math.h>

/* Coordinate frame:
	OpenGL / OpenVR / Godot / Etc...

	+X goes right.
	+Y comes out top of controller.
	+Z comes toward user (Into User's Eyes)
*/

float rsqrtf ( float x )
{
	typedef union { int32_t i; float f; } fiunion; 
    const float xhalf = 0.5f * x;
    fiunion i = { .f = x };

    i.i = 0x5f375a86 - ( i.i >> 1 );
    x = i.f;
    x = x * ( 1.5f - xhalf * x * x );
    x = x * ( 1.5f - xhalf * x * x );

    return x;
}

float mathsqrtf( float x )
{
	// Trick to do approximate, fast square roots.
	int sign = x < 0;
	if( sign ) x = -x;
	float o = x;
	o = (o+x/o)/2;
	o = (o+x/o)/2;
	o = (o+x/o)/2;
	o = (o+x/o)/2;
	if( sign )
		return -o;
	else
		return o;
}

void mathEulerToQuat( float * q, const float * euler )
{
	float pitch = euler[0];
	float yaw = euler[1];
	float roll = euler[2];
    float cr = cosf(pitch * 0.5);
    float sr = sinf(pitch * 0.5); // Pitch: About X
    float cp = cosf(yaw * 0.5); 
    float sp = sinf(yaw * 0.5);   // Yaw:   About Y
    float cy = cosf(roll * 0.5);
    float sy = sinf(roll * 0.5);  // Roll:  About Z
    q[0] = cr * cp * cy + sr * sp * sy;
    q[1] = sr * cp * cy - cr * sp * sy;
    q[2] = cr * sp * cy + sr * cp * sy;
    q[3] = cr * cp * sy - sr * sp * cy;
}

void mathQuatApply(float * qout, const float * q1, const float * q2) {
	// NOTE: Does not normalize
	float tmpw, tmpx, tmpy;
	tmpw = (q1[0] * q2[0]) - (q1[1] * q2[1]) - (q1[2] * q2[2]) - (q1[3] * q2[3]);
	tmpx = (q1[0] * q2[1]) + (q1[1] * q2[0]) + (q1[2] * q2[3]) - (q1[3] * q2[2]);
	tmpy = (q1[0] * q2[2]) - (q1[1] * q2[3]) + (q1[2] * q2[0]) + (q1[3] * q2[1]);
	qout[3] = (q1[0] * q2[3]) + (q1[1] * q2[2]) - (q1[2] * q2[1]) + (q1[3] * q2[0]);
	qout[2] = tmpy;
	qout[1] = tmpx;
	qout[0] = tmpw;
}

void mathQuatNormalize(float * qout, const float * qin )
{
	float qmag = qin[0] * qin[0] + qin[1] * qin[1] + qin[2] * qin[2] + qin[3] * qin[3];
	qmag = rsqrtf( qmag );
	qout[0] = qin[0] * qmag;
	qout[1] = qin[1] * qmag;
	qout[2] = qin[2] * qmag;
	qout[3] = qin[3] * qmag;
}

void mathCrossProduct(float * p, const float * a, const float * b)
{
	float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2] = a[0] * b[1] - a[1] * b[0];
	p[1] = ty;
	p[0] = tx;
}

void mathRotateVectorByQuaternion(float * pout, const float * q, const float * p )
{
	// return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
	float iqo[3];
	mathCrossProduct( iqo, q + 1 /*.xyz*/, p );
	iqo[0] += q[0] * p[0];
	iqo[1] += q[0] * p[1];
	iqo[2] += q[0] * p[2];
	float ret[3];
	mathCrossProduct( ret, q + 1 /*.xyz*/, iqo );
	pout[0] = ret[0] * 2.0 + p[0];
	pout[1] = ret[1] * 2.0 + p[1];
	pout[2] = ret[2] * 2.0 + p[2];
}

void mathRotateVectorByInverseOfQuaternion(float * pout, const float * q, const float * p )
{
	// return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
	float iqo[3];
	mathCrossProduct( iqo, p, q + 1 /*.xyz*/ );
	iqo[0] += q[0] * p[0];
	iqo[1] += q[0] * p[1];
	iqo[2] += q[0] * p[2];
	float ret[3];
	mathCrossProduct( ret, iqo, q + 1 /*.xyz*/ );
	pout[0] = ret[0] * 2.0 + p[0];
	pout[1] = ret[1] * 2.0 + p[1];
	pout[2] = ret[2] * 2.0 + p[2];
}

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

	int16_t data[6*16];
	int r = GeneralI2CGet( LSM6DSL_ADDRESS, 0x20, (uint8_t*)data, 2 );
	if( r < 0 ) return;
	if( r == 2 ) ld->temp = data[0];
	int readr = ReadLSM6DSL( (uint8_t*)data, sizeof( data ) );

	int samp;
	int16_t * cdata = data;

    uint32_t start = getCycleCount();

	// STEP 0:  Decide your coordinate frame.

	// [0] = +X axis coming out right of controller.
	// [1] = +Y axis, pointing straight up out of controller, out where the USB port is.
	// [2] = +Z axis, pointing up from the face of the controller.

	ld->lastreadr = readr;

	for( samp = 0; samp < readr; samp+=12 )
	{
		// Extract data from IMU
		int16_t * euler_deltas = cdata; // Euler angles, from gyro.
		int16_t * accel_data   = cdata + 3;

		// Manual cal, used only for Steps 2..8
	//	euler_deltas[0] -= 12;
	//	euler_deltas[1] += 22;
	//	euler_deltas[2] += 4;

		// We can sum rotations to understand the amount of counts in a full circle.
		ld->gyroaccum[0] += euler_deltas[0];
		ld->gyroaccum[1] += euler_deltas[1];
		ld->gyroaccum[2] += euler_deltas[2];
		ld->gyrocount++;

		// STEP 1:  Visually inspect the gyro values.
		// STEP 2:  Integrate the gyro values, verify they are correct.

		// 2000 dps full-scale
		// 32768 is full-scale
		// 208 SPS
		// convert to radians. ( 2000.0f / 32768.0f / 208.0f * 2.0 * 3.14159f / 180.0f );  
		// Measured = 560,000 counts per scale (Measured by looking at sum)
		// Testing -> 3.14159 * 2.0 / 566000;
		float fFudge = 1.1;
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
			euler_deltas[0] * fEulerScales[0] + ld->fvBias[0],
			euler_deltas[1] * fEulerScales[1] + ld->fvBias[1],
			euler_deltas[2] * fEulerScales[2] + ld->fvBias[2] };

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

		//ESP_LOGI( "SB", "%ld %ld %ld", raw_up[0], raw_up[1], raw_up[2] );

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
		ld->fvBias[0] += mathsqrtf(corrective_quaternion[1]) * 0.0000002;
		ld->fvBias[1] += mathsqrtf(corrective_quaternion[2]) * 0.0000002;
		ld->fvBias[2] += mathsqrtf(corrective_quaternion[3]) * 0.0000002;


		// Second, we can apply a very small corrective tug.  This helps prevent oscillation
		// about the correct answer.  This acts sort of like a P term to a PID loop.
		// This is actually the **primary**, or fastest responding thing.
		const float corrective_force = 0.005f;
		corrective_quaternion[1] *= corrective_force;
		corrective_quaternion[2] *= corrective_force;
		corrective_quaternion[3] *= corrective_force;

		// x^2+y^2+z^2+q^2 -> ALGEBRA! -> sqrt( 1-x^2-y^2-z^2 ) = w
		corrective_quaternion[0] = mathsqrtf( 1 
			- corrective_quaternion[1]*corrective_quaternion[1]
			- corrective_quaternion[2]*corrective_quaternion[2]
			- corrective_quaternion[3]*corrective_quaternion[3] );
//		ESP_LOGI( "x", "%f %f %f %f\n", corrective_quaternion[0], corrective_quaternion[1], corrective_quaternion[2], corrective_quaternion[3] );
		mathQuatApply( ld->fqQuat, ld->fqQuat, corrective_quaternion );

		// Magnitude of correction angle = inverse_sin( magntiude( axis_of_correction ) );
		// We want to significantly reduce that. To mute any effect.


		// TODO which directon of frame of reference?  Up relative to controller? OR controller relative to world?
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

    ld->caltime = getCycleCount() - start;
}

static void LMS6DS3Setup()
{
	memset( &LSM6DSL, 0, sizeof(LSM6DSL) );
	LSM6DSL.fqQuat[0] = 1;

	// Enable access
	LSM6DSLSet( LSM6DSL_FUNC_CFG_ACCESS, 0x20 );
	LSM6DSLSet( LSM6DSL_CTRL3_C, 0x81 ); // Force reset
	vTaskDelay( 1 );
	LSM6DSLSet( LSM6DSL_CTRL3_C, 0x44 ); // unforce reset
	LSM6DSLSet( LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b110 ); // 208 Hz ODR
	LSM6DSLSet( LSM6DSL_FIFO_CTRL3, 0b00001001 ); // Put both devices in FIFO.
	LSM6DSLSet( LSM6DSL_CTRL1_XL, 0b01011001 ); // Setup accel (16 g's FS)
	LSM6DSLSet( LSM6DSL_CTRL2_G, 0b01011100 ); // Setup gyro, 2000dps
	LSM6DSLSet( LSM6DSL_CTRL4_C, 0x00 ); // Disable all filtering.
	LSM6DSLSet( LSM6DSL_CTRL7_G, 0b00000000 ); // Setup gyro, not high performance mode = 0x80.  High perf = 0x00
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
	frameno = 0;

    ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

    REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

    menu = initMenu("USB Sandbox", mainMenuCb);
    addSingleItemToMenu(menu, mainMenuMode.modeName);
    addSingleItemToMenu(menu, menu_Bootload);

    loadWsg("kid0.wsg", &example_sprite, true);
/*
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

//	i2c_driver_delete( I2C_NUM_0 );
    ESP_LOGI( "sandbox", "i2c_param_config=%d", i2c_param_config(I2C_NUM_0, &conf) );
	ESP_LOGI( "sandbox", "i2c_driver_install=%d", i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0) );
*/
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

	LSM6DSLIntegrate();
/*
	cts += sprintf( cts, "%ld %ld / %5d %5d %5d / %5d %5d %5d / %ld %ld %ld / %f %f %f %f",
		LSM6DSL.caltime, LSM6DSL.temp, 
		LSM6DSL.accellast[0], LSM6DSL.accellast[1], LSM6DSL.accellast[2],
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2], 
		LSM6DSL.fqQuat[0], LSM6DSL.fqQuat[1], LSM6DSL.fqQuat[2], LSM6DSL.fqQuat[3]  );
*/

	float plusy[3] = { 0, 1, 0 };
	float plusy_out[3] = { 0, 1, 0 };
	mathRotateVectorByQuaternion( plusy, LSM6DSL.fqQuat, plusy );
//	mathRotateVectorByInverseOfQuaternion( plusy_out, LSM6DSL.fqQuat, plusy_out );
	mathRotateVectorByQuaternion( plusy_out, LSM6DSL.fqQuat, plusy_out );

	float plusx_out[3] = { 1, 0, 0 };
	mathRotateVectorByQuaternion( plusx_out, LSM6DSL.fqQuat, plusx_out );

	float plusz_out[3] = { 0, 0, 1 };
	mathRotateVectorByQuaternion( plusz_out, LSM6DSL.fqQuat, plusz_out );

	int i, vertices = 0;
	for( i = 0; i < sizeof(bunny_verts)/2; i+= 3 )
	{
		float bz = -bunny_verts[i+0];
		float by = bunny_verts[i+1];
		float bx = bunny_verts[i+2];

		float box = bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2];
		float boy = bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2];
		float boz = bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2];

		bunny_verts_out[vertices*3+0] = box/250 + 280/2; 
		bunny_verts_out[vertices*3+1] = -boy/250 + 240/2;  // Convert from right-handed to left-handed coordinate frame.
		bunny_verts_out[vertices*3+2] = boz;

		vertices++;
	}
/*
	int centerX = 280/2;
	int centerY = 240/2;
	float xcomp = -plusy_out[0];
	float ycomp = plusy_out[1];
	int v0x = xcomp * 90;
	int v0y = ycomp * 90;
	drawLineFast(centerX-v0x, centerY-v0y, centerX+v0x, centerY+v0y, 215 );
*/

#if 1
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
#endif

	cts += sprintf( cts, "%ld %d %f %f %f / %f %f %f / %3d %3d %3d / %4d %4d %4d / %d %d", LSM6DSL.caltime, LSM6DSL.lastreadr,
		LSM6DSL.fCorrectLast[0], LSM6DSL.fCorrectLast[1], LSM6DSL.fCorrectLast[2],
		LSM6DSL.fvBias[0],		LSM6DSL.fvBias[1],		LSM6DSL.fvBias[2],
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2],
		LSM6DSL.accellast[0], LSM6DSL.accellast[1], LSM6DSL.accellast[2],
		 vertices, lines );

	ESP_LOGI( "I2C", "%s", ctsbuffer );
}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
    int i;

    fillDisplayArea(x, y, x+w, y+h, 0 );
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



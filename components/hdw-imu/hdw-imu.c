//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <math.h>
#include <string.h>
#include "hdw-imu.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "soc/gpio_struct.h"
#include "hdw-nvs.h"

#define DSCL_OUTPUT                             \
    {                                           \
        GPIO.enable1_w1ts.val = 1 << (41 - 32); \
    }
#define DSCL_INPUT                              \
    {                                           \
        GPIO.enable1_w1tc.val = 1 << (41 - 32); \
    }
#define DSDA_OUTPUT                  \
    {                                \
        GPIO.enable_w1ts = 1 << (3); \
    }
#define DSDA_INPUT                   \
    {                                \
        GPIO.enable_w1tc = 1 << (3); \
    }
#define READ_DSDA ((GPIO.in >> 3) & 1)

// 14 counts (1MHz) works most of the time, but no hurries, let's slow it down to ~800k.
void i2c_delay(int x)
{
    int i;
    for (i = 0; i < 19 * x; i++)
        asm volatile("nop");
}
#define DELAY1 i2c_delay(1);
#define DELAY2 i2c_delay(2);

#include "static_i2c.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    LSM6DSL_FUNC_CFG_ACCESS        = 0x01,
    LSM6DSL_SENSOR_SYNC_TIME_FRAME = 0x04,
    LSM6DSL_FIFO_CTRL1             = 0x06,
    LSM6DSL_FIFO_CTRL2             = 0x07,
    LSM6DSL_FIFO_CTRL3             = 0x08,
    LSM6DSL_FIFO_CTRL4             = 0x09,
    LSM6DSL_FIFO_CTRL5             = 0x0a,
    LSM6DSL_ORIENT_CFG_G           = 0x0b,
    LSM6DSL_INT1_CTRL              = 0x0d,
    LSM6DSL_INT2_CTRL              = 0x0e,
    LMS6DS3_WHO_AM_I               = 0x0f,
    LSM6DSL_CTRL1_XL               = 0x10,
    LSM6DSL_CTRL2_G                = 0x11,
    LSM6DSL_CTRL3_C                = 0x12,
    LSM6DSL_CTRL4_C                = 0x13,
    LSM6DSL_CTRL5_C                = 0x14,
    LSM6DSL_CTRL6_C                = 0x15,
    LSM6DSL_CTRL7_G                = 0x16,
    LSM6DSL_CTRL8_XL               = 0x17,
    LSM6DSL_CTRL9_XL               = 0x18,
    LSM6DSL_CTRL10_C               = 0x19,
    LSM6DSL_MASTER_CONFIG          = 0x1a,
    LSM6DSL_WAKE_UP_SRC            = 0x1b,
    LSM6DSL_TAP_SRC                = 0x1c,
    LSM6DSL_D6D_SRC                = 0x1d,
    LSM6DSL_STATUS_REG             = 0x1e,
    LSM6DSL_OUT_TEMP_L             = 0x20,
    LSM6DSL_OUT_TEMP_H             = 0x21,
    LSM6DSL_FIFO_STATUS1           = 0x3A,
} lsm6dslReg_t;

//==============================================================================
// Defines
//==============================================================================

#define LSM6DSL_ADDRESS 0x6a
#define QMC6308_ADDRESS 0x2c

//==============================================================================
// Variables
//==============================================================================

LSM6DSLData LSM6DSL;

//==============================================================================
// Utility Prototypes
//==============================================================================

static inline uint32_t getCycleCount();

esp_err_t GeneralSet(int dev, int reg, int val);
esp_err_t LSM6DSLSet(int reg, int val);
int GeneralI2CGet(int device, int reg, uint8_t* data, int data_len);
int ReadLSM6DSL(uint8_t* data, int data_len);

//==============================================================================
// Utility Functions
//==============================================================================

/**
 * @brief Perform a fast, approximate reciprocal square root
 *
 * @param x The number to take a recriprocal square root of.
 * @return approximately 1/sqrt(x)
 */
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

/**
 * @brief Perform a fast, approximate square root
 *
 * @param x The number to take a square root of.
 * @return approximately sqrt(x) (but is much faster)
 */
float mathsqrtf(float x)
{
    // Trick to do approximate, fast square roots. (Though it is surprisingly fast)
    int sign = x < 0;
    if (sign)
        x = -x;
    if (x < 0.0000001)
        return 0.0001;
    float o = x;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    if (sign)
        return -o;
    else
        return o;
}

/**
 * @brief convert euler angles (in radians) to a quaternion.
 *
 * @param q Pointer to the wxyz quat (float[4]) to be written.
 * @param euler Pointer to a float[3] of euler angles.
 */
void mathEulerToQuat(float* q, const float* euler)
{
    float pitch = euler[0];
    float yaw   = euler[1];
    float roll  = euler[2];
    float cr    = cosf(pitch * 0.5);
    float sr    = sinf(pitch * 0.5); // Pitch: About X
    float cp    = cosf(yaw * 0.5);
    float sp    = sinf(yaw * 0.5); // Yaw:   About Y
    float cy    = cosf(roll * 0.5);
    float sy    = sinf(roll * 0.5); // Roll:  About Z
    q[0]        = cr * cp * cy + sr * sp * sy;
    q[1]        = sr * cp * cy - cr * sp * sy;
    q[2]        = cr * sp * cy + sr * cp * sy;
    q[3]        = cr * cp * sy - sr * sp * cy;
}

/**
 * @brief Rotate one quaternion by another (and do not normalize)
 *
 * @param qout Pointer to the wxyz quat (float[4]) to be written.
 * @param q1 First quaternion to be rotated.
 * @param q2 Quaternion to rotate q1 by.
 */
void mathQuatApply(float* qout, const float* q1, const float* q2)
{
    // NOTE: Does not normalize - you will need to normalize eventually.
    float tmpw, tmpx, tmpy;
    tmpw    = (q1[0] * q2[0]) - (q1[1] * q2[1]) - (q1[2] * q2[2]) - (q1[3] * q2[3]);
    tmpx    = (q1[0] * q2[1]) + (q1[1] * q2[0]) + (q1[2] * q2[3]) - (q1[3] * q2[2]);
    tmpy    = (q1[0] * q2[2]) - (q1[1] * q2[3]) + (q1[2] * q2[0]) + (q1[3] * q2[1]);
    qout[3] = (q1[0] * q2[3]) + (q1[1] * q2[2]) - (q1[2] * q2[1]) + (q1[3] * q2[0]);
    qout[2] = tmpy;
    qout[1] = tmpx;
    qout[0] = tmpw;
}

/**
 * @brief Normalize a quaternion
 *
 * @param qout Pointer to the wxyz quat (float[4]) to be written.
 * @param qin Pointer to the quaterion to normalize.
 */
void mathQuatNormalize(float* qout, const float* qin)
{
    float qmag = qin[0] * qin[0] + qin[1] * qin[1] + qin[2] * qin[2] + qin[3] * qin[3];
    qmag       = rsqrtf(qmag);
    qout[0]    = qin[0] * qmag;
    qout[1]    = qin[1] * qmag;
    qout[2]    = qin[2] * qmag;
    qout[3]    = qin[3] * qmag;
}

/**
 * @brief Perform a 3D cross product
 *
 * @param p Pointer to the float[3] output of the cross product (p = a x b)
 * @param a Pointer to the float[3] of the cross product a vector.
 * @param b Pointer to the float[3] of the cross product b vector.
 */
void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

/**
 * @brief Rotate a 3D vector by a quaternion
 *
 * @param pout Pointer to the float[3] output of the rotation
 * @param q Pointer to the wxyz quaternion (float[4]) of the rotation.
 * @param p Pointer to the float[3] of the vector to rotates.
 */
void mathRotateVectorByQuaternion(float* pout, const float* q, const float* p)
{
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, q + 1 /*.xyz*/, p);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, q + 1 /*.xyz*/, iqo);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

/**
 * @brief Rotate a 3D vector by the inverse of a quaternion
 *
 * @param pout Pointer to the float[3] output of the antirotation.
 * @param q Pointer to the wxyz quaternion (float[4]) opposite of the rotation.
 * @param p Pointer to the float[3] of the vector to antirotates.
 */
void mathRotateVectorByInverseOfQuaternion(float* pout, const float* q, const float* p)
{
    // General note: Performing a transform this way can be about 20-30% slower than a well formed 3x3 matrix.
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, p, q + 1 /*.xyz*/);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, iqo, q + 1 /*.xyz*/);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

static inline uint32_t getCycleCount()
{
    uint32_t ccount;
    asm volatile("rsr %0,ccount" : "=a"(ccount));
    return ccount;
}

//==============================================================================
// Internal Functions
//==============================================================================

/**
 * @brief Set a specific register on the IMU to a value.
 *
 * @param dev The 7-bit address of the device to set the register to.
 * @param reg The 8-bit register #
 * @param val The 8-bit value to set the register to.
 * @return ESP_OK if the operation was successful.
 */
esp_err_t GeneralSet(int dev, int reg, int val)
{
    SendStart();
    SendByte(dev << 1);
    SendByte(reg);
    SendByte(val);
    SendStop();
    return ESP_OK;
}

/**
 * @brief Set a specific register on the LSM6DSL to a value.
 *
 * @param reg The 8-bit register #
 * @param val The 8-bit value to set the register to.
 * @return ESP_OK if the operation was successful.
 */
esp_err_t LSM6DSLSet(int reg, int val)
{
    return GeneralSet(LSM6DSL_ADDRESS, reg, val);
}

/**
 * @brief Read a buffer back from a specific I2C device.
 *
 * @param device The 7-bit device address
 * @param reg The 8-bit register # to start at.
 * @param data The buffer to load the data into.
 * @param data_len Number of bytes to read.
 * @return positive number if operation was successful, or esp_err_t if failure.
 */
int GeneralI2CGet(int device, int reg, uint8_t* data, int data_len)
{
    SendStart();
    SendByte(device << 1);
    SendByte(reg);
    SendStart();
    SendByte((device << 1) | 1);
    int i;
    for (i = 0; i < data_len; i++)
    {
        data[i] = GetByte(i == data_len - 1);
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
int ReadLSM6DSL(uint8_t* data, int data_len)
{
    uint32_t fifolen = 0;
    SendStart();
    SendByte(LSM6DSL_ADDRESS << 1);
    SendByte(LSM6DSL_FIFO_STATUS1);
    SendStart();
    SendByte((LSM6DSL_ADDRESS << 1) | 1);
    int i;

    // Read first 3 bytes, the 4th byte might be a nak.
    for (i = 0; i < 3; i++)
    {
        ((uint8_t*)&fifolen)[i] = GetByte(0);
    }

    // Is fifo overflow.
    if (fifolen & 0x4000)
    {
        // reset fifo.
        // If we overflow, and we don't do this, bad things happen.
        GetByte(1);
        SendStop();
        LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b000); // Disable fifo
        LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b110); // 208 Hz ODR
        LSM6DSL.sampCount = 0;
        return 0;
    }

    fifolen &= 0x7ff;

    if (fifolen > data_len / 2)
        fifolen = data_len / 2;

    // Make sure we only read out full data segments.
    int read_len = fifolen / 6 * 12;

    if (read_len == 0)
    {
        // No bytes? nak!
        GetByte(1);
        SendStop();
        return 0;
    }

    GetByte(0); // Ignoring FIFO Status 4

    // Read out all bytes.
    for (i = 0; i < read_len; i++)
    {
        data[i] = GetByte(i == read_len - 1);
    }
    SendStop();
    return fifolen;
}

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the IMU
 *
 * @param sda The GPIO for the Serial DAta line
 * @param scl The GPIO for the Serial CLock line
 * @param pullup Either \c GPIO_PULLUP_DISABLE if there are external pullup resistors on SDA and SCL or \c
 * GPIO_PULLUP_ENABLE if internal pull-ups should be used
 * @return ESP_OK if the accelerometer initialized, or a non-zero value if it did not
 */
esp_err_t initAccelerometer(gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup)
{
    int i;
    int retry = 0;
do_retry:

    gpio_config_t gsetup = {
        .pin_bit_mask = (1ULL << sda) | (1ULL << scl),
        .mode         = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };

    gpio_config(&gsetup);

    // This will "shake loose" any devices stuck on the bus.
    GPIO.enable_w1ts      = 1 << (3);
    GPIO.enable1_w1ts.val = 1 << (41 - 32);
    esp_rom_delay_us(10);
    GPIO.out_w1tc = 1 << (3);
    for (i = 0; i < 16; i++)
    {
        esp_rom_delay_us(10);
        GPIO.out1_w1ts.val = 1 << (41 - 32);
        esp_rom_delay_us(10);
        GPIO.out1_w1tc.val = 1 << (41 - 32);
    }
    esp_rom_delay_us(10);
    GPIO.out1_w1ts.val = 1 << (41 - 32);
    esp_rom_delay_us(10);
    GPIO.out_w1ts = 1 << (3);
    esp_rom_delay_us(10);
    GPIO.out1_w1ts.val = 1 << (41 - 32); // Send final stop

    // Prepare for normal open drain functionality.
    GPIO.enable1_w1tc.val = 1 << (41 - 32);
    GPIO.enable_w1tc      = 1 << (3);
    GPIO.out1_w1tc.val    = 1 << (41 - 32);
    GPIO.out_w1tc         = 1 << (3);

    // Enable access
    LSM6DSLSet(LSM6DSL_FUNC_CFG_ACCESS, 0x20);
    LSM6DSLSet(LSM6DSL_CTRL3_C, 0x81); // Force reset
    esp_rom_delay_us(100);
    LSM6DSLSet(LSM6DSL_CTRL3_C, 0x44); // unforce reset

    uint8_t who = 0xaa;
    int r       = GeneralI2CGet(LSM6DSL_ADDRESS, LMS6DS3_WHO_AM_I, &who, 1);
    if (r != 1 || who != 0x6a)
    {
        ESP_LOGW("accel", "WHOAMI Failed (%02x), %d", who, r);
        if (retry++ < 10)
            goto do_retry;
        ESP_LOGE("accel", "Init failed on 1");
        return ESP_FAIL;
    }
    ESP_LOGI("accel", "Init Start");

    accelSetRegistersAndReset();

    for (i = 0; i < 2; i++)
    {
        vTaskDelay(1);
        int check = accelIntegrate();
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

/**
 * @brief Deinit the accelerometer (nothing to do)
 *
 * @return ESP_OK
 */
esp_err_t deInitAccelerometer(void)
{
    return ESP_OK;
}

/**
 * @brief Read all pending samples in IMU and perform a sensor fusion pass
 *
 * @return ESP_OK if successful, or nonzero if error.
 */
esp_err_t accelIntegrate()
{
    LSM6DSLData* ld = &LSM6DSL;

    int16_t data[6 * 16];

    // Get temperature sensor (in case we ever want to use it)
    // int r = GeneralI2CGet(LSM6DSL_ADDRESS, 0x20, (uint8_t*)data, 2);
    // if (r < 0)
    //    return r;
    // if (r == 2)
    //    ld->temp = data[0];
    int readr = ReadLSM6DSL((uint8_t*)data, sizeof(data));
    if (readr < 0)
        return readr;
    int samp;
    int16_t* cdata = data;

    uint32_t start = getCycleCount();

    // STEP 0:  Decide your coordinate frame.

    // [0] = +X axis coming out right of controller.
    // [1] = +Y axis, pointing straight up out of controller, out where the USB port is.
    // [2] = +Z axis, pointing up from the face of the controller.

    ld->lastreadr = readr;

    for (samp = 0; samp < readr; samp += 6)
    {
        // Extract data from IMU
        int16_t* euler_deltas = cdata; // Euler angles, from gyro.
        int16_t* accel_data   = cdata + 3;

        // Manual cal, used only for Steps 2..8
        //    euler_deltas[0] -= 12;
        //    euler_deltas[1] += 22;
        //    euler_deltas[2] += 4;

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
        float fFudge = 0.5625; // XXX TODO: Investigate.
        float fScale = (2000.0f / 32768.0f / 208.0f * 2.0 * 3.14159f / 180.0f) * fFudge;

        // STEP 3:  Integrate gyro values into a quaternion.
        // This step is validated by working with just one axis at a time
        // then apply a coordinate frame to ld->fqQuat and validate that it is
        // correct.
        float fEulerScales[3] = {-fScale, fScale, -fScale};

        float fEulers[3]
            = {euler_deltas[0] * fEulerScales[0], euler_deltas[1] * fEulerScales[1], euler_deltas[2] * fEulerScales[2]};

        // Used for calibration
        if (ld->performCal)
        {
            float diff[3]
                = {fEulers[0] - ld->fvAverage[0], fEulers[1] - ld->fvAverage[1], fEulers[2] - ld->fvAverage[2]};

            float diffsq[3] = {(diff[0] < 0) ? -diff[0] : diff[0], (diff[1] < 0) ? -diff[1] : diff[1],
                               (diff[2] < 0) ? -diff[2] : diff[2]};

            diffsq[0] *= 1000.0;
            diffsq[1] *= 1000.0;
            diffsq[2] *= 1000.0;

            ld->fvDeviation[0] -= 0.004;
            ld->fvDeviation[1] -= 0.004;
            ld->fvDeviation[2] -= 0.004;

            if (ld->fvDeviation[0] < diffsq[0])
                ld->fvDeviation[0] = diffsq[0];
            if (ld->fvDeviation[1] < diffsq[1])
                ld->fvDeviation[1] = diffsq[1];
            if (ld->fvDeviation[2] < diffsq[2])
                ld->fvDeviation[2] = diffsq[2];

            if (ld->fvDeviation[0] > 0.8)
                ld->fvDeviation[0] = 0.8;
            if (ld->fvDeviation[1] > 0.8)
                ld->fvDeviation[1] = 0.8;
            if (ld->fvDeviation[2] > 0.8)
                ld->fvDeviation[2] = 0.8;

            diff[0] *= mathsqrtf(ld->fvDeviation[0]) * 0.5;
            diff[1] *= mathsqrtf(ld->fvDeviation[1]) * 0.5;
            diff[2] *= mathsqrtf(ld->fvDeviation[2]) * 0.5;

            ld->fvAverage[0] += diff[0];
            ld->fvAverage[1] += diff[1];
            ld->fvAverage[2] += diff[2];

            // Compute the running RMS error.
            float fvEuler = accelGetStdDevInCal();

            if (fvEuler < 0.00015f)
            {
                ld->fvBias[0] = -ld->fvAverage[0];
                ld->fvBias[1] = -ld->fvAverage[1];
                ld->fvBias[2] = -ld->fvAverage[2];
                struct fiunion
                {
                    union
                    {
                        int32_t i;
                        float f;
                    } u;
                };
                struct fiunion x, y, z;
                x.u.f = ld->fvBias[0];
                y.u.f = ld->fvBias[1];
                z.u.f = ld->fvBias[2];
                writeNvs32("gyrocalx", x.u.i);
                writeNvs32("gyrocaly", y.u.i);
                writeNvs32("gyrocalz", z.u.i);

                ld->performCal     = 0;
                ld->fvDeviation[0] = 0;
                ld->fvDeviation[0] = 1;
                ld->fvDeviation[0] = 2;
            }
        }

        fEulers[0] += ld->fvBias[0];
        fEulers[1] += ld->fvBias[1];
        fEulers[2] += ld->fvBias[2];

        mathEulerToQuat(ld->fqQuatLast, fEulers);
        mathQuatApply(ld->fqQuat, ld->fqQuat, ld->fqQuatLast);

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
        float accel_up[3] = {-accel_data[0], accel_data[1], -accel_data[2]};

        float accel_inverse_mag
            = rsqrtf(accel_up[0] * accel_up[0] + accel_up[1] * accel_up[1] + accel_up[2] * accel_up[2]);
        accel_up[0] *= accel_inverse_mag;
        accel_up[1] *= accel_inverse_mag;
        accel_up[2] *= accel_inverse_mag;

        ld->fvLastAccelRaw[0] = accel_up[0];
        ld->fvLastAccelRaw[1] = accel_up[1];
        ld->fvLastAccelRaw[2] = accel_up[2];

        if (ld->sampCount++ == 0)
        {
            // set fqQuat to be the rotation to go from our "up" from the
            // accelerometer to the nominal "up"
            float ideal_up[3] = {0, 1, 0};

            float half[3]     = {accel_up[0] + ideal_up[0], accel_up[1] + ideal_up[1], accel_up[2] + ideal_up[2]};
            float halfnormreq = rsqrtf(half[0] * half[0] + half[1] * half[1] + half[2] * half[2]);
            half[0] *= halfnormreq;
            half[1] *= halfnormreq;
            half[2] *= halfnormreq;

            float* q = ld->fqQuat;
            mathCrossProduct(q + 1, accel_up, half);
            float dotdiff = accel_up[0] * half[0] + accel_up[1] * half[1] + accel_up[2] * half[2];
            q[0]          = dotdiff;
        }
        else
        {
            // Step 6B: Next, compute what we think "up" should be from our point of view.  We will use +Y Up.
            float what_we_think_is_up[3] = {0, 1, 0};
            mathRotateVectorByInverseOfQuaternion(what_we_think_is_up, LSM6DSL.fqQuat, what_we_think_is_up);

            // Step 6C: Next, we determine how far off we are.  This will tell us our error.
            float corrective_quaternion[4];

            // TRICKY: The ouput of this is actually the axis of rotation, which is ironically
            // in vector-form the same as a quaternion.  So we can write directly into the quat.
            mathCrossProduct(corrective_quaternion + 1, accel_up, what_we_think_is_up);

            // Now, we apply this in step 7.

            // First, we can compute what the drift values of our axes are, to anti-drift them.
            // If you do only this, you will always end up in an unstable oscillation.
            memcpy(ld->fCorrectLast, corrective_quaternion + 1, 12);

            // XXX TODO: We need to multiply by amount the accelerometer gives us assurance.
            ld->fvBias[0] += mathsqrtf(corrective_quaternion[1]) * 0.0000002;
            ld->fvBias[1] += mathsqrtf(corrective_quaternion[2]) * 0.0000002;
            ld->fvBias[2] += mathsqrtf(corrective_quaternion[3]) * 0.0000002;

            float corrective_force = 0.0005f;

            // Second, we can apply a very small corrective tug.  This helps prevent oscillation
            // about the correct answer.  This acts sort of like a P term to a PID loop.
            // This is actually the **primary**, or fastest responding thing.
            corrective_quaternion[1] *= corrective_force;
            corrective_quaternion[2] *= corrective_force;
            corrective_quaternion[3] *= corrective_force;

            // x^2+y^2+z^2+q^2 -> ALGEBRA! -> sqrt( 1-x^2-y^2-z^2 ) = w
            corrective_quaternion[0] = mathsqrtf(1 - corrective_quaternion[1] * corrective_quaternion[1]
                                                 - corrective_quaternion[2] * corrective_quaternion[2]
                                                 - corrective_quaternion[3] * corrective_quaternion[3]);

            mathQuatApply(ld->fqQuat, ld->fqQuat, corrective_quaternion);
        }
        cdata += 6;
    }

    // Now we move onto the outer loop.
    // STEP 5: We now want to normalize the quat periodically.  Don't do this too
    // soon, otherwise you won't notice math errors.  Realistically, this should
    // only need to be done every hundreds of thousands of samples.
    //
    // Also, don't do this too often, otherwise you will reduce accuracy,
    // unnecessarily.
    float* qRot  = ld->fqQuat;
    float qmagsq = qRot[0] * qRot[0] + qRot[1] * qRot[1] + qRot[2] * qRot[2] + qRot[3] * qRot[3];
    if (qmagsq > 1.05 || qmagsq < 0.95)
    {
        // This normalizes everything.
        qmagsq  = rsqrtf(qmagsq);
        qRot[0] = qRot[0] * qmagsq;
        qRot[1] = qRot[1] * qmagsq;
        qRot[2] = qRot[2] * qmagsq;
        qRot[3] = qRot[3] * qmagsq;
    }

    if (samp)
    {
        ld->gyrolast[0]  = cdata[-6];
        ld->gyrolast[1]  = cdata[-5];
        ld->gyrolast[2]  = cdata[-4];
        ld->accellast[0] = cdata[-3];
        ld->accellast[1] = cdata[-2];
        ld->accellast[2] = cdata[-1];
    }

    ld->computetime = getCycleCount() - start;

    return ESP_OK;
}

/**
 * @brief Get the RAW accelerometer "up" vector from device point of view.
 *
 * @return ESP_OK
 */
esp_err_t accelGetAccelVecRaw(int16_t* x, int16_t* y, int16_t* z)
{
    *x = LSM6DSL.fvLastAccelRaw[0] * 256;
    *y = LSM6DSL.fvLastAccelRaw[1] * 256;
    *z = LSM6DSL.fvLastAccelRaw[2] * 256;
    return ESP_OK;
}

/**
 * @brief Get the current orientation "up" vector from device point of view.
 *
 * @return ESP_OK
 */
esp_err_t accelGetOrientVec(int16_t* x, int16_t* y, int16_t* z)
{
    float plusy[3] = {0, 1, 0};
    mathRotateVectorByInverseOfQuaternion(plusy, LSM6DSL.fqQuat, plusy);
    *x = plusy[0] * 256;
    *y = plusy[1] * 256;
    *z = plusy[2] * 256;
    return ESP_OK;
}

/**
 * @brief Get the current estimated quaternion of the IMU.
 *
 * @return ESP_OK
 */
esp_err_t accelGetQuaternion(float* q)
{
    float* fq = LSM6DSL.fqQuat;
    q[0]      = fq[0];
    q[1]      = fq[1];
    q[2]      = fq[2];
    q[3]      = fq[3];
    return ESP_OK;
}

/**
 * @brief Initialize calibration of IMU.
 *
 * @return ESP_OK If cal start was OK.
 */
esp_err_t accelPerformCal()
{
    LSM6DSL.performCal     = 1;
    LSM6DSL.fvDeviation[0] = 1;
    LSM6DSL.fvDeviation[1] = 1;
    LSM6DSL.fvDeviation[2] = 1;
    LSM6DSL.fvAverage[0]   = 0;
    LSM6DSL.fvAverage[1]   = 0;
    LSM6DSL.fvAverage[2]   = 0;

    // Ignore failures here.
    eraseNvsKey("gyrocalx");
    eraseNvsKey("gyrocaly");
    eraseNvsKey("gyrocalz");

    return ESP_OK;
}

/**
 * @brief Get current divergence as the cal algorithm attempts to converge on a good cal.
 *
 * @return If in cal mode, will return current motion deviation. If out of cal mode, will return 0.0f
 */
float accelGetStdDevInCal()
{
    if (LSM6DSL.performCal == 0)
        return 0.0f;
    float* d = LSM6DSL.fvDeviation;
    return (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

/**
 * @brief Reset the IMU's core registers. Should be done any time you are entering a mode with the IMU.
 *
 */
void accelSetRegistersAndReset(void)
{
    LSM6DSLSet(LSM6DSL_FIFO_CTRL5, (0b0101 << 3) | 0b000); // Reset FIFO
    LSM6DSLSet(
        LSM6DSL_FIFO_CTRL5,
        (0b0101 << 3)
            | 0b110); // 208 Hz ODR, Continuous mode. If the FIFO is full, the new sample overwrites the older one.
    LSM6DSLSet(LSM6DSL_FIFO_CTRL3, 0b00001001); // Put both devices (Accel + Gyro) in FIFO.
    LSM6DSLSet(LSM6DSL_CTRL1_XL, 0b01011001);   // Setup accel (16 g's FS)
    LSM6DSLSet(LSM6DSL_CTRL2_G, 0b01011100);    // Setup gyro, 2000dps
    LSM6DSLSet(LSM6DSL_CTRL4_C, 0x00);          // Disable all filtering.
    LSM6DSLSet(LSM6DSL_CTRL7_G, 0b00000000);    // Setup gyro, not high performance mode = 0x80.  High perf = 0x00
    LSM6DSLSet(LSM6DSL_FIFO_CTRL2, 0b00000000); // Temp not in fifo  (Why no work?)

    memset(&LSM6DSL, 0, sizeof(LSM6DSL));
    LSM6DSL.fqQuat[0]     = 1;
    LSM6DSL.fqQuatLast[0] = 1;
    LSM6DSL.sampCount     = 0;
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

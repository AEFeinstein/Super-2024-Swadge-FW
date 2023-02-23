//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>

#include "hdw-accel.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    QMA7981_REG_CHIP_ID     = 0x00,
    QMA7981_REG_DX_L        = 0x01,
    QMA7981_REG_DX_H        = 0x02,
    QMA7981_REG_DY_L        = 0x03,
    QMA7981_REG_DY_H        = 0x04,
    QMA7981_REG_DZ_L        = 0x05,
    QMA7981_REG_DZ_H        = 0x06,
    QMA7981_REG_STEP_L      = 0x07,
    QMA7981_REG_STEP_H      = 0x08,
    QMA7981_REG_INT_STAT_0  = 0x0A,
    QMA7981_REG_INT_STAT_1  = 0x0B,
    QMA7981_REG_INT_STAT_4  = 0x0D,
    QMA7981_REG_RANGE       = 0x0F,
    QMA7981_REG_BAND_WIDTH  = 0x10,
    QMA7981_REG_PWR_MANAGE  = 0x11,
    QMA7981_REG_STEP_CONF_0 = 0x12,
    QMA7981_REG_STEP_CONF_1 = 0x13,
    QMA7981_REG_STEP_CONF_2 = 0x14,
    QMA7981_REG_STEP_CONF_3 = 0x15,
    QMA7981_REG_INT_EN_0    = 0x16,
    QMA7981_REG_INT_EN_1    = 0x17,
    QMA7981_REG_INT_MAP_0   = 0x19,
    QMA7981_REG_INT_MAP_1   = 0x1A,
    QMA7981_REG_INT_MAP_2   = 0x1B,
    QMA7981_REG_INT_MAP_3   = 0x1C,
    QMA7981_REG_SIG_STEP_TH = 0x1D,
    QMA7981_REG_STEP        = 0x1F
} qmaReg_t;

//==============================================================================
// Defines
//==============================================================================

#define QMA7981_ADDR 0x12

//==============================================================================
// Variables
//==============================================================================

static qma_range_t qma_range = QMA_RANGE_2G;
static i2c_port_t i2c_port;

//==============================================================================
// Function Prototypes
//==============================================================================

static esp_err_t qma7981_read_byte(qmaReg_t reg_addr, uint8_t* data);
static esp_err_t qma7981_write_byte(qmaReg_t reg_addr, uint8_t data);
static esp_err_t qma7981_read_bytes(qmaReg_t reg_addr, size_t data_len, uint8_t* data);
static int16_t signExtend10bit(uint16_t in);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the accelerometer
 *
 * @param _i2c_port The i2c port to use for the accelerometer
 * @param sda The GPIO for the Serial DAta line
 * @param scl The GPIO for the Serial CLock line
 * @param pullup Either \c GPIO_PULLUP_DISABLE if there are external pullup resistors on SDA and SCL or \c
 * GPIO_PULLUP_ENABLE if internal pullups should be used
 * @param clkHz The frequency of the I2C clock
 * @param range The range to measure, between ::QMA_RANGE_2G and ::QMA_RANGE_32G
 * @param bandwidth The bandwidth to measure at, between ::QMA_BANDWIDTH_128_HZ and ::QMA_BANDWIDTH_1024_HZ
 * @return ESP_OK if the accelerometer initialized, or a non-zero value if it did not
 */
esp_err_t initAccelerometer(i2c_port_t _i2c_port, gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup, uint32_t clkHz,
                            qma_range_t range, qma_bandwidth_t bandwidth)
{
    i2c_port          = _i2c_port;
    esp_err_t ret_val = ESP_OK;

    /* Install i2c driver */
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = sda,
        .sda_pullup_en    = pullup,
        .scl_io_num       = scl,
        .scl_pullup_en    = pullup,
        .master.clk_speed = clkHz,
        .clk_flags        = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };
    ret_val |= i2c_param_config(i2c_port, &conf);
    ret_val |= i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);

    /* Exit sleep mode*/
    ret_val |= qma7981_write_byte(QMA7981_REG_PWR_MANAGE, 0xC0);
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Set range */
    ret_val |= qma7981_write_byte(QMA7981_REG_RANGE, range);
    /* Set bandwidth */
    ret_val |= qma7981_write_byte(QMA7981_REG_BAND_WIDTH, bandwidth);

    return ret_val;
}

/**
 * @brief Deinit the accelerometer (nothting to do)
 *
 * @return ESP_OK
 */
esp_err_t deInitAccelerometer(void)
{
    return ESP_OK;
}

/**
 * @brief Read a single byte from the accelerometer
 *
 * @param reg_addr The register to read from
 * @param data The byte which was read is written here
 * @return ESP_OK if the byte was read, or a non-zero value if it was not
 */
static esp_err_t qma7981_read_byte(qmaReg_t reg_addr, uint8_t* data)
{
    return qma7981_read_bytes(reg_addr, 1, data);
}

/**
 * @brief Read multiple bytes from the accelerometer
 *
 * @param reg_addr The register to read from
 * @param data_len The number of bytes to read
 * @param data The bytes which were read are written here
 * @return ESP_OK if the bytes were read, or a non-zero value if they were not
 */
static esp_err_t qma7981_read_bytes(qmaReg_t reg_addr, size_t data_len, uint8_t* data)
{
    // Write the register to read from
    i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, QMA7981_ADDR << 1 | I2C_MASTER_WRITE, false);
    i2c_master_write_byte(cmdHandle, reg_addr, false);
    i2c_master_stop(cmdHandle);
    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);

    if (ESP_OK != err)
    {
        return err;
    }

    // Read from the register
    cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);
    i2c_master_write_byte(cmdHandle, QMA7981_ADDR << 1 | I2C_MASTER_READ, false);
    i2c_master_read(cmdHandle, data, data_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmdHandle);
    err = i2c_master_cmd_begin(i2c_port, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);

    return err;
}

/**
 * @brief Write a single byte to the accelerometer
 *
 * @param reg_addr The register address to write to
 * @param data The byte to write
 * @return ESP_OK if the byte was written, or a non-zero value if it was not
 */
static esp_err_t qma7981_write_byte(qmaReg_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmdHandle = i2c_cmd_link_create();
    i2c_master_start(cmdHandle);

    i2c_master_write_byte(cmdHandle, QMA7981_ADDR << 1, false);
    i2c_master_write_byte(cmdHandle, reg_addr, false);
    i2c_master_write_byte(cmdHandle, data, false);

    i2c_master_stop(cmdHandle);
    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmdHandle, 100);
    i2c_cmd_link_delete(cmdHandle);
    return err;
}

/**
 * @brief Read and return the 16-bit step counter
 *
 * Note that this can be configured with ::QMA7981_REG_STEP_CONF_0 through ::QMA7981_REG_STEP_CONF_3
 *
 * @param data The step counter value is written here
 * @return ESP_OK if the step count was read, or a non-zero value if it was not
 */
esp_err_t accelGetStep(uint16_t* data)
{
    esp_err_t ret_val = ESP_OK;
    uint8_t step_h = 0, step_l = 0;

    if (NULL == data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret_val |= qma7981_read_byte(QMA7981_REG_STEP_L, &step_l);
    ret_val |= qma7981_read_byte(QMA7981_REG_STEP_H, &step_h);

    *data = (step_h << 8) + step_l;

    return ret_val;
}

/**
 * @brief Set the accelerometer's measurement range
 *
 * @param range The range to measure, from ::QMA_RANGE_2G to ::QMA_RANGE_32G
 * @return ESP_OK if the range was set, or a non-zero value if it was not
 */
esp_err_t accelSetRange(qma_range_t range)
{
    esp_err_t ret_val = qma7981_write_byte(QMA7981_REG_RANGE, range);
    qma_range         = range;

    return ret_val;
}

/**
 * @brief Read the current acceleration vector from the accelerometer and return
 * the vector through arguments. If the read fails, the last known values are
 * returned instead.
 *
 * @param x The X component of the acceleration vector is written here
 * @param y The Y component of the acceleration vector is written here
 * @param z The Z component of the acceleration vector is written here
 * @return ESP_OK if the acceleration was read, or a non-zero value if it was not
 */
esp_err_t accelGetAccelVec(int16_t* x, int16_t* y, int16_t* z)
{
    static int16_t lastX = 0;
    static int16_t lastY = 0;
    static int16_t lastZ = 0;

    // Read 6 bytes of data(0x00)
    uint8_t raw_data[6];
    // Do the read
    esp_err_t ret_val = qma7981_read_bytes(QMA7981_REG_DX_L, 6, raw_data);

    // If the read was successsful
    if (ESP_OK == ret_val)
    {
        // Sign extend the 10 bit value to 16 bits and save it as the last known value
        // TODO The datasheet mentions this is a 14 bit reading, not a 10 bit one?
        lastX = signExtend10bit(((raw_data[0] >> 6) | (raw_data[1]) << 2) & 0x03FF);
        lastY = -signExtend10bit(((raw_data[2] >> 6) | (raw_data[3]) << 2) & 0x03FF);
        lastZ = -signExtend10bit(((raw_data[4] >> 6) | (raw_data[5]) << 2) & 0x03FF);
    }

    // Copy out the acceleration value
    *x = lastX;
    *y = lastY;
    *z = lastZ;

    return ret_val;
}

/**
 * @brief Helper function to sign-extend a 10 bit two's complement number to 16 bit
 *
 * @param in The two's compliment number to sign-extend
 * @return The sign-extended two's compliment number
 */
static int16_t signExtend10bit(uint16_t in)
{
    if (in & 0x200)
    {
        return (in | 0xFC00); // extend the sign bit all the way out
    }
    else
    {
        return (in & 0x01FF); // make sure the sign bits are cleared
    }
}

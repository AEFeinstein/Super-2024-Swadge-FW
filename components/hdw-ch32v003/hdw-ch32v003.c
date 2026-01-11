//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/rtc.h>
#include <driver/dedic_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Needed for ch32v003_swio.h
#define MAX_IN_TIMEOUT 1000
#define DisableISR()                         \
    do                                       \
    {                                        \
        XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL); \
        portbenchmarkINTERRUPT_DISABLE();    \
    } while (0)
#define EnableISR()                        \
    do                                     \
    {                                      \
        portbenchmarkINTERRUPT_RESTORE(0); \
        XTOS_SET_INTLEVEL(0);              \
    } while (0)
int uprintf(const char* fmt, ...);
// #define uprintf(x...) ESP_LOGE( "ch32v003programmer", x)

#define GPIO_VAR_W1TC        (*GPIO_VAR_W1TC_R)
#define GPIO_VAR_W1TS        (*GPIO_VAR_W1TS_R)
#define GPIO_VAR_ENABLE_W1TC (*GPIO_VAR_ENABLE_W1TC_R)
#define GPIO_VAR_ENABLE_W1TS (*GPIO_VAR_ENABLE_W1TS_R)
#define GPIO_VAR_IN          (*GPIO_VAR_IN_R)

static uint32_t SWIO_PIN;
static uint32_t SWCLK_PIN = 64;

static volatile uint32_t* GPIO_VAR_W1TC_R        = &GPIO.out_w1tc;
static volatile uint32_t* GPIO_VAR_W1TS_R        = &GPIO.out_w1ts;
static volatile uint32_t* GPIO_VAR_ENABLE_W1TC_R = &GPIO.enable_w1tc;
static volatile uint32_t* GPIO_VAR_ENABLE_W1TS_R = &GPIO.enable_w1ts;
static volatile uint32_t* GPIO_VAR_IN_R          = &GPIO.in;

#define IO_MUX_REG(x)  XIO_MUX_REG(x)
#define XIO_MUX_REG(x) (IO_MUX_GPIO0_REG + (x * 4))

#define GPIO_NUM(x)  XGPIO_NUM(x)
#define XGPIO_NUM(x) (GPIO_NUM_0 + (x * 4))

#include "hdw-ch32v003.h"
#include "ch32v003_swio.h"

//==============================================================================
// External overrides
//==============================================================================
const uint8_t* cnfsGetFile(int index, size_t* size);

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

// Coordinate map is used any time you want to do custom work with the 003's framebuffer.
const uint16_t Coordmap[8 * 16] = {
    0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0xffff, 0xffff, 0x0002, 0x0102, 0x0202, 0x0302, 0x0402, 0x0502,
    0xffff, 0xffff, 0x0001, 0x0101, 0x0201, 0x0301, 0x0401, 0x0501, 0xffff, 0xffff, 0x0008, 0x0108, 0x0208, 0x0308,
    0x0408, 0x0508, 0xffff, 0xffff, 0x0007, 0x0107, 0x0207, 0x0307, 0x0407, 0x0507, 0xffff, 0xffff, 0x0006, 0x0106,
    0x0206, 0x0306, 0x0406, 0x0506, 0xffff, 0xffff, 0x0005, 0x0205, 0x0405, 0x0605, 0x0600, 0x0700, 0xffff, 0xffff,
    0x0004, 0x0204, 0x0404, 0x0604, 0x0602, 0x0702, 0xffff, 0xffff, 0x0003, 0x0203, 0x0403, 0x0603, 0x0601, 0x0701,
    0xffff, 0xffff, 0x0105, 0x0305, 0x0505, 0x0705, 0x0608, 0x0708, 0xffff, 0xffff, 0x0104, 0x0304, 0x0504, 0x0704,
    0x0607, 0x0707, 0xffff, 0xffff, 0x0103, 0x0303, 0x0503, 0x0703, 0x0606, 0x0706, 0xffff, 0xffff,
};

// Keep available outside this file just in case someone wants to access
// the hardware directly.
struct SWIOState swioContext;

//==============================================================================
// Prototypes
//==============================================================================

static int ch32v003Check();

//==============================================================================
// Functions
//==============================================================================

int initCh32v003(int swdio_pin)
{
    SWIO_PIN  = swdio_pin;
    SWCLK_PIN = 64; // Unused.

    if (SWIO_PIN < 32)
    {
        GPIO_VAR_W1TC_R        = &GPIO.out_w1tc;
        GPIO_VAR_W1TS_R        = &GPIO.out_w1ts;
        GPIO_VAR_ENABLE_W1TC_R = &GPIO.enable_w1tc;
        GPIO_VAR_ENABLE_W1TS_R = &GPIO.enable_w1ts;
        GPIO_VAR_IN_R          = &GPIO.in;
    }
    else
    {
        GPIO_VAR_W1TC_R        = &GPIO.out1_w1tc.val;
        GPIO_VAR_W1TS_R        = &GPIO.out1_w1ts.val;
        GPIO_VAR_ENABLE_W1TC_R = &GPIO.enable1_w1tc.val;
        GPIO_VAR_ENABLE_W1TS_R = &GPIO.enable1_w1ts.val;
        GPIO_VAR_IN_R          = &GPIO.in1.val;
    }
    return 0;
}

/**
 * @brief Write to memory on the ch32v003
 *
 * @param binary A binary image to be loaded into a location in the ch32v003.
 * @param length The size of the binary image.
 * @param offset The offset of the binary image.s
 */
int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t offset)
{
    if (ch32v003Check())
        return -1;

    ResetInternalProgrammingState(&swioContext);

    int offset_end = offset + length;

    if ((offset & 0x3f) == 0)
    {
        for (; offset + 63 < offset_end; offset += 64)
        {
            if (Write64Block(&swioContext, offset, binary))
            {
                ESP_LOGE("ch32v003", "Failed to Write64Block");
                ch32v003Teardown();
                return -4;
            }
            binary += 64;
        }
    }

    if ((offset & 0x3) == 0)
    {
        if (offset < 0x10000000)
            EraseFlash(&swioContext, offset, 64, 0);

        for (; offset + 3 < offset_end; offset += 4)
        {
            if (WriteWord(&swioContext, offset, *((const uint32_t*)binary)))
            {
                ESP_LOGE("ch32v003", "Failed to WriteWord");
                ch32v003Teardown();
                return -5;
            }
            // uprintf("Write4 %08x %08x\n", (unsigned)offset, *(const uint32_t*)binary);
            binary += 4;
        }
    }

    for (; offset < offset_end; offset++)
    {
        // Write the rest, one byte at-a-time.
        uint32_t wordBase       = offset & (~0x3);
        uint32_t wordOffsetBits = (offset & 3) * 8;
        uint32_t word           = 0;
        if (ReadWord(&swioContext, wordBase, &word))
        {
            ESP_LOGE("ch32v003", "Failed to ReadWord for byte writing.");
            ch32v003Teardown();
            return -6;
        }
        uint8_t byte = *binary;
        binary++;
        word = (word & ~(0xff << wordOffsetBits)) | (byte << wordOffsetBits);
        if (WriteWord(&swioContext, wordBase, word))
        {
            ESP_LOGE("ch32v003", "Failed to WriteWord for byte writing.");
            ch32v003Teardown();
            return -7;
        }
    }
    return 0;
}

/**
 * @brief Write data into the ch32v003's flash.
 *
 * @param buf A pointer to a binary image in the master processor
 * @param sz The size of the binary image to be written
 * @return 0 if OK, nonzero in error condition.
 */
int ch32v003WriteFlash(const uint8_t* buf, int sz)
{
    if (ch32v003Check())
        return -1;

    ResetInternalProgrammingState(&swioContext);

    ch32v003SetReg(DMCONTROL, 0x80000001); // Request halt
    ch32v003SetReg(DMCONTROL, 0x80000001); // Really request halt.
    ch32v003SetReg(DMCONTROL, 0x00000001); // Clear halt request.

    uint8_t block[64];
    int i;
    for (i = 0; i < sz; i += 64)
    {
        int rem = sz - i;
        if (rem >= 64)
        {
            rem = 64;
            memset(block + rem, 0, 64 - rem);
        }
        memcpy(block, buf + i, rem);

        if (Write64Block(&swioContext, i + 0x08000000, block))
        {
            ESP_LOGE("ch32v003", "Failed to write word to flash.");
            ch32v003Teardown();
            return -6;
        }
    }

    int flashok = 1;
    int j;
    for (j = 0; j < sz / 4; j++)
    {
        unsigned addy = 0x08000000 + j * 4;
        uint32_t by   = 0;
        int ra        = ch32v003ReadMemory((uint8_t*)&by, 4, addy);
        if (ra)
        {
            uprintf("Failed to read %08x\n", addy);
            flashok = 0;
        }
        if (by != ((uint32_t*)buf)[j])
        {
            uprintf("@%d %02x=%02x\n", j, by, ((uint32_t*)buf)[j]);
            flashok = 0;
        }
    }

    return !flashok;
}

/**
 * @brief Cause the 003 to reboot and execute the program loaded into it.
 *
 * @return 0 if OK, nonzero in error condition.
 */
int ch32v003Resume()
{
    if (ch32v003Check())
        return -1;

    ResetInternalProgrammingState(&swioContext);

    ch32v003SetReg(DMCONTROL, 0x80000003); // Halt
    ch32v003SetReg(DMCONTROL, 0x80000003); // Halt + Reset
    ch32v003SetReg(DMCONTROL, 0x00000001); // Clear halt request.
    ch32v003SetReg(DMCONTROL, 0x40000001); // Resume
    ch32v003SetReg(DMCONTROL, 0x40000001); // Resume

    return 0;
}

/**
 * @brief Read data from the ch32v003's memory space into host processor memory space
 *
 * @param binary A pointer to a buffer in the host processor
 * @param length The number of bytes to read
 * @param offset The location of memory to read in the ch32v003
 * @return 0 if OK, nonzero if error.
 */
int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t offset)
{
    if (ch32v003Check())
        return -1;

    int i;
    for (i = 0; i < length; i += 4)
    {
        uint32_t word = 0;
        if (ReadWord(&swioContext, i + offset, &word))
        {
            ESP_LOGE("ch32v003", "Failed to ReadWord for byte reading.");
            ch32v003Teardown();
            return -6;
        }

        int btor = length - i;
        if (btor > 4)
            btor = 4;

        memcpy(binary + i, &word, btor);
    }
    return 0;
}

/**
 * @brief Read ch32v003 debug module registers
 *
 * @param regno The debug module register number.
 * @param value A pointer to the uint32_t to receive that register's value.
 * @return 0 if OK, nonzero if error.
 */
int ch32v003GetReg(int regno, uint32_t* value)
{
    if (ch32v003Check())
        return -1;

    return MCFReadReg32(&swioContext, regno, value);
}

/**
 * @brief Write ch32v003 debug module registers
 *
 * @param regno The debug module register number.
 * @param regValue The value to write into that debug module regsiter
 * @return 0 if OK, nonzero if error.
 */
int ch32v003SetReg(int regno, uint32_t regValue)
{
    if (ch32v003Check())
        return -1;

    MCFWriteReg32(&swioContext, regno, regValue);

    return 0;
}

/**
 * @brief See if the ch32v003 has any pending printf to print on host processor.
 *
 */
void ch32v003CheckTerminal()
{
    char buffer[257];
    int r = PollTerminal(&swioContext, (uint8_t*)buffer, sizeof(buffer), 0, 0);
    if (r > 0 && r < sizeof(buffer) - 1)
    {
        buffer[r] = 0;
        // ESP_LOGI("ch32v003", "%s", buffer);
        uprintf("%s", buffer);
    }
}

/**
 * @brief Disables ch32v003 communications
 *
 */
void ch32v003Teardown()
{
    // Power-Down
    if (SWIO_PIN < 32)
    {
        GPIO_VAR_W1TC = 1 << SWIO_PIN;
        GPIO_VAR_W1TS = 1 << SWIO_PIN;
    }
    else
    {
        GPIO_VAR_W1TC = 1 << (SWIO_PIN - 32);
        GPIO_VAR_W1TS = 1 << (SWIO_PIN - 32);
    }

    memset(&swioContext, 0, sizeof(swioContext));
}

/**
 * @brief Halt the processor, and write to RAM a greyscale asset image.
 *
 * @param slot Must be less than \ref CH32V003_MAX_IMAGE_SLOTS.
 * @param asset_idx Is a .gs.png asset, for instance EYES_DEFAULT_GS (From eyes_default.gs.png).  This image must be
 * ::EYE_LED_W x ::EYE_LED_H pixels in size.
 * @return 0 if OK, nonzero if error.
 */
int ch32v003WriteBitmapAsset(int slot, int asset_idx)
{
    size_t sz          = 0;
    const uint8_t* buf = cnfsGetFile(asset_idx, &sz);
    if (sz < 4)
    {
        printf("Error: Asset wrong size (%d) bytes.\n", (int)sz);
        return -1;
    }
    if (((const uint16_t*)buf)[0] != EYE_LED_W || ((const uint16_t*)buf)[1] != EYE_LED_H)
    {
        printf("Error: Asset wrong dimensions (%d x %d) needs (%d, %d).\n", ((const uint16_t*)buf)[0],
               ((const uint16_t*)buf)[1], EYE_LED_W, EYE_LED_H);
        return -1;
    }

    struct PixelMap
    {
        uint8_t buffer[EYE_LED_H][EYE_LED_W];
    };
    struct PixelMap* pm = (struct PixelMap*)(buf + 4);

    return ch32v003WriteBitmap(slot, pm->buffer);
}

/**
 * @brief Write a ::EYE_LED_W x ::EYE_LED_H pixel greyscale image into a RAM slot on the ch32v003.
 *
 * @param slot Must be less than \ref CH32V003_MAX_IMAGE_SLOTS.
 * @param pixels are a greyscale image to be written out.
 * @return 0 if OK, nonzero if error.
 */
int ch32v003WriteBitmap(int slot, const uint8_t pixels[EYE_LED_H][EYE_LED_W])
{
    if (slot >= CH32V003_MAX_IMAGE_SLOTS)
    {
        ESP_LOGE("ch32v003", "Failed: requested slot too big.\n");
        return -1;
    }

    uint8_t rkbuffer[128];

    int i, x, y;

    for (y = 0; y < EYE_LED_H; y++)
        for (x = 0; x < EYE_LED_W; x++)
        {
            int intensity = pixels[y][x];
            int coord     = Coordmap[x * 8 + (5 - y)];

            int ox = coord & 0xff;
            int oy = coord >> 8;

            int ofs       = ox;
            uint8_t* ledo = &rkbuffer[ofs];
            int imask     = ~(1 << oy);
            int mask      = ~imask;

            for (i = 0; i < 8; i++)
            {
                if (intensity & (1 << i))
                    *ledo |= mask;
                else
                    *ledo &= imask;
                ledo += 9;
            }
        }

    // Make sure processor is halted if we're using it in framebuffer mode.
    ch32v003SetReg(DMCONTROL, 0x80000001); // Request halt
    ch32v003SetReg(DMCONTROL, 0x80000001); // Really request halt.
    ch32v003SetReg(DMCONTROL, 0x00000001); // Clear halt request.

    return ch32v003WriteMemory(rkbuffer, sizeof(rkbuffer), 0x20000200 + slot * 72);
}

/**
 * @brief Override the DMA pointer on the 003 to point at a pre-loaded image in RAM.
 *
 * @param slot Must be less than \ref CH32V003_MAX_IMAGE_SLOTS.
 * @return 0 if OK, nonzero if error.
 */
int ch32v003SelectBitmap(int slot)
{
    if (slot >= CH32V003_MAX_IMAGE_SLOTS)
    {
        ESP_LOGE("ch32v003", "Failed: requested slot too big.\n");
        return -1;
    }
    uint32_t ledPointer = 0x20000200 + slot * 72;
    uint32_t gpioOff    = 0x00000000;
    uint32_t gpioOn     = 0x22222222;
    // Overwrite DMA1_Channel5->MADDR, Assume we've been configured.
    ch32v003WriteMemory((const uint8_t*)&gpioOff, 4, 0x40011000);
    ch32v003WriteMemory((uint8_t*)&ledPointer, 4, 0x40020064);
    return ch32v003WriteMemory((const uint8_t*)&gpioOn, 4, 0x40011000);
}

static int ch32v003Check()
{
    if (!swioContext.t1coeff)
    {
        // https://github.com/cnlohr/Super-2024-Swadge-FW/blob/ch32v003programmer/tools/sandbox_test/test_programmer/sandbox.c
        REG_WRITE(IO_MUX_REG(SWIO_PIN),
                  1 << FUN_IE_S | 1 << FUN_PU_S | 1 << FUN_DRV_S); // Additional pull-up, 10mA drive.  Optional: 10k
                                                                   // pull-up resistor. This is the actual SWIO.
        // REG_WRITE( IO_MUX_REG(SWCLK_PIN), 1<<FUN_IE_S | 1<<FUN_PU_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA
        // drive.  Optional: 10k pull-up resistor. This is the actual SWCLK.

        // I have no idea why this is needed.
        gpio_config_t reset_gpios = {
            .mode         = GPIO_MODE_INPUT,
            .pin_bit_mask = (1ULL << SWIO_PIN), // | (1ULL << SWCLK_PIN),
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&reset_gpios);
        REG_WRITE(/*GPIO_FUNC18_OUT_SEL_CFG_REG*/ GPIO_FUNC0_OUT_SEL_CFG_REG + SWIO_PIN * 4, SIG_GPIO_OUT_IDX);
        // REG_WRITE( GPIO_FUNC40_OUT_SEL_CFG_REG, SIG_GPIO_OUT_IDX );

        memset(&swioContext, 0, sizeof(swioContext));
        if (SWIO_PIN > 31)
        {
            swioContext.pinmaskD = 1 << (SWIO_PIN - 32);
        }
        else
        {
            swioContext.pinmaskD = 1 << SWIO_PIN;
        }

        GPIO_VAR_W1TS        = swioContext.pinmaskD;
        GPIO_VAR_ENABLE_W1TS = swioContext.pinmaskD;

        esp_rom_delay_us(5000);

        rtc_cpu_freq_config_t m;
        rtc_clk_cpu_freq_get_config(&m);

        switch (m.freq_mhz)
        {
            case 240:
                swioContext.t1coeff = 9; // 9 or 10 is good.  5 is too low. 13 is sometimes too high. ***BUT*** WHEN
                                         // RUNNING FROM SANDBOX NEED BE HIGHER
                break;
            default:
                swioContext.t1coeff = 100; // Untested At Other Speeds
                break;
        }

        int r = InitializeSWDSWIO(&swioContext);
        uprintf("InitializeSWDSWIO = %d / m: %d\n", r, swioContext.t1coeff);
        if (r)
        {
            ESP_LOGE("ch32v003", "Failed to initialize");
            ch32v003Teardown();
            return -1;
        }
    }
    return 0;
}

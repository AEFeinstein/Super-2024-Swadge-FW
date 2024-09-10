//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>

#include <esp_flash.h>
#include <rom/rtc.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_log.h>
#include <driver/uart.h>

#include "tinyusb.h"

#include "hdw-usb.h"
#include "advanced_usb_control.h"

//==============================================================================
// Defines
//==============================================================================

// Uncomment the ESP_LOGI to activate logging for this file.
// Logging can cause issues in operation, so by default it should remain off.
#define ULOG(x...) // ESP_LOGI( "advanced_usb_control", x )

// Size of printf buffer.
#define AUPB_SIZE 4096

//==============================================================================
// Variables
//==============================================================================

static uint32_t* advanced_usb_scratch_buffer_data;
static uint32_t advanced_usb_scratch_buffer_data_size;
static uint32_t advanced_usb_scratch_immediate[SCRATCH_IMMEDIATE_DWORDS];
static uint8_t* advanced_usb_printf_buffer = NULL;
static int advanced_usb_printf_head;
static int advanced_usb_printf_tail;

static uint32_t* advanced_usb_read_offset;
static uint8_t did_init_flash_function;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Accept a "get" feature report command from a USB host and write
 *         back whatever is needed to send back.
 *
 * @param reqLen Number of bytes host is requesting from us.
 * @param data Pointer to a feature get request for the command set.
 * @return Number of bytes that will be returned.
 */
int handle_advanced_usb_control_get(uint8_t* data, int reqLen)
{
    if (advanced_usb_read_offset == 0)
    {
        return 0;
    }
    memcpy(data, advanced_usb_read_offset, reqLen);
    return reqLen;
}

/**
 * @brief Internal function for writing log data to the ring buffer.
 *
 * @param cookie *unused*
 * @param data Pointer to text that needs to be logged.
 * @return size Number of bytes in data that need to be logged.
 */
static int advanced_usb_write_log(void* cookie __attribute__((unused)), const char* data, int size)
{
    if (NULL == advanced_usb_printf_buffer)
    {
        advanced_usb_printf_buffer = heap_caps_calloc(AUPB_SIZE, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    }

    int next = (advanced_usb_printf_head + 1) % AUPB_SIZE;
    int idx  = 0;
    // Drop extra characters on the floor.
    while (next != advanced_usb_printf_tail && idx < size)
    {
        advanced_usb_printf_buffer[advanced_usb_printf_head] = data[idx++];
        advanced_usb_printf_head                             = next;
        next                                                 = (advanced_usb_printf_head + 1) % AUPB_SIZE;
    }

    // Write out to UART as well.
    // This currently crashes
    // uart_write_bytes(0, (const char*)data, size);

    return size;
}

/**
 * @brief vaprintf stand-in for USB logging.
 *
 * @param fmt vaprintf format
 * @param args vaprintf args
 * @return size Number of characters that were written.
 */
int advanced_usb_write_log_printf(const char* fmt, va_list args)
{
    char buffer[512];
    int l = vsnprintf(buffer, 511, fmt, args);
    advanced_usb_write_log(0, buffer, l);
    return l;
}

/**
 * @brief vaprintf stand-in for USB logging.
 *
 * @param fmt vaprintf format
 * @param ... vaprintf args
 * @return size Number of characters that were written.
 */
int uprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int r = advanced_usb_write_log_printf(fmt, args);
    va_end(args);
    return r;
}

/**
 * @brief USB request to get text in buffer
 *
 * @param reqLen The number of bytes the host is requesting from us.
 * @param data The data that we will write back into
 * @return size Number of bytes to be returned to the host.
 */
int handle_advanced_usb_terminal_get(uint8_t* data, int reqLen)
{
    if (NULL == advanced_usb_printf_buffer)
    {
        advanced_usb_printf_buffer = heap_caps_calloc(AUPB_SIZE, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    }

    int togo = (advanced_usb_printf_head - advanced_usb_printf_tail + AUPB_SIZE) % AUPB_SIZE;

    data[0] = 171;

    int mark = 1;
    if (togo)
    {
        if (togo > reqLen - 3)
        {
            togo = reqLen - 3;
        }
        while (mark <= togo)
        {
            data[++mark] = advanced_usb_printf_buffer[advanced_usb_printf_tail++];
            if (advanced_usb_printf_tail == AUPB_SIZE)
            {
                advanced_usb_printf_tail = 0;
            }
        }
    }
    return mark + 1;
}

/**
 * @brief Accept a "send" feature report command from a USB host and interpret it.
 *         executing whatever needs to be executed.
 *
 * @param datalen Total length of the buffer (command ID included)
 * @param data Pointer to full command
 */
void IRAM_ATTR handle_advanced_usb_control_set(const uint8_t* data, int datalen)
{
    if (datalen < 6)
    {
        return;
    }
    intptr_t value = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
    switch (data[1])
    {
        case AUSB_CMD_REBOOT:
        {
            // This is mentioned a few places, but seems unnecessary.
            // void chip_usb_set_persist_flags( uint32_t x );
            // chip_usb_set_persist_flags( USBDC_PERSIST_ENA );

            // Decide to reboot into bootloader or not.
            REG_WRITE(RTC_CNTL_OPTION1_REG, value ? RTC_CNTL_FORCE_DOWNLOAD_BOOT : 0);
            software_reset();
            break;
        }
        case AUSB_CMD_WRITE_RAM:
        {
            // Write into scratch.
            if (datalen < 8)
            {
                return;
            }
            intptr_t length = data[6] | (data[7] << 8);
            ULOG("Writing %lu into %p", (uint32_t)length, (void*)value);
            memcpy((void*)value, data + 8, length);
            break;
        }
        case AUSB_CMD_READ_RAM:
        {
            // Configure read.
            advanced_usb_read_offset = (uint32_t*)value;
            break;
        }
        case AUSB_CMD_EXEC_RAM:
        {
            // Execute scratch
            void (*scratchFn)() = (void (*)())(value);
            ULOG("Executing %p (%p) // base %08x/%p", (void*)value, scratchFn, 0, advanced_usb_scratch_buffer_data);
            scratchFn();
            break;
        }
        case AUSB_CMD_SWITCH_MODE:
        {
            // Switch Swadge mode
            ULOG("SwadgeMode Value: 0x%08x", value);
            usbSetSwadgeMode((void*)value);
            break;
        }
        case AUSB_CMD_ALLOC_SCRATCH:
        {
            // (re) allocate the primary scratch buffer.
            // value = -1 will just cause a report.
            // value = 0 will clear it.
            // value < 0 just read current data.
            ULOG("Allocating to %lu (Current: %p / %lu)", (uint32_t)value, advanced_usb_scratch_buffer_data,
                 (uint32_t)advanced_usb_scratch_buffer_data_size);
            if (!(value & 0x80000000))
            {
                if (value > advanced_usb_scratch_buffer_data_size)
                {
                    if (advanced_usb_scratch_buffer_data)
                    {
                        free(advanced_usb_scratch_buffer_data);
                    }
                    advanced_usb_scratch_buffer_data      = calloc(1, value);
                    advanced_usb_scratch_buffer_data_size = value;
                }
                if (value == 0)
                {
                    if (advanced_usb_scratch_buffer_data)
                    {
                        free(advanced_usb_scratch_buffer_data);
                    }
                    advanced_usb_scratch_buffer_data_size = 0;
                }
            }
            advanced_usb_scratch_immediate[0] = (intptr_t)advanced_usb_scratch_buffer_data;
            advanced_usb_scratch_immediate[1] = advanced_usb_scratch_buffer_data_size;
            advanced_usb_read_offset          = (uint32_t*)(&advanced_usb_scratch_immediate[0]);
            ULOG("New: %p / %lu", advanced_usb_scratch_buffer_data, advanced_usb_scratch_buffer_data_size);
            break;
        }
        case ACMD_CMD_MEMSET:
        {
            if (datalen < 11)
            {
                return;
            }
            intptr_t length = data[6] | (data[7] << 8) | (data[8] << 16) | (data[9] << 24);
            ULOG("Memset %d into %p", (int)length, (void*)value);
            memset((void*)value, data[10], length);
            break;
        }
        case ACMD_CMD_GET_VER:
        {
            // TODO: This is terrible.  It should be improved.
            void app_main(void);
            advanced_usb_scratch_immediate[0] = (uint32_t)&app_main;
            advanced_usb_scratch_immediate[1] = (uint32_t)&advanced_usb_scratch_buffer_data;
            advanced_usb_scratch_immediate[2] = (uint32_t)&handle_advanced_usb_control_set;
            advanced_usb_scratch_immediate[3] = (uint32_t)&handle_advanced_usb_terminal_get;
            advanced_usb_read_offset          = advanced_usb_scratch_immediate;
            break;
        }
        case AUSB_CMD_FLASH_ERASE: // Flash erase region
        {
            if (datalen < 10)
            {
                return;
            }

            intptr_t length = data[6] | (data[7] << 8) | (data[8] << 16) | (data[9] << 24);
            if (!did_init_flash_function)
            {
                esp_flash_init(0);
            }

            if ((length & 0x80000000) && value == 0)
            {
                esp_flash_erase_chip(0);
            }
            else
            {
                esp_flash_erase_region(0, value, length);
            }
            break;
        }
        case AUSB_CMD_FLASH_WRITE: // Flash write region
        {
            if (datalen < 8)
            {
                return;
            }
            intptr_t length = data[6] | (data[7] << 8);
            esp_flash_write(0, data + 8, value, length);
            break;
        }
        case AUSB_CMD_FLASH_READ: // Flash read region
        {
            if (datalen < 8)
            {
                return;
            }
            intptr_t length = data[6] | (data[7] << 8);
            if (length > sizeof(advanced_usb_scratch_immediate))
            {
                length = sizeof(advanced_usb_scratch_immediate);
            }
            esp_flash_read(0, advanced_usb_scratch_immediate, value, length);
            advanced_usb_read_offset = advanced_usb_scratch_immediate;
            break;
        }
    }
}

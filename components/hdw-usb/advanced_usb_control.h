/*! \file advanced_usb_control.h
 *
 * \section ausb Advanced USB Control System
 *
 * This system allows one to communicate with the ESP at runtime and issue a series of commands from userspace
 * applications using HID API. This can be concurrent to using the interface as any type of HID device, mouse, keyboard
 * etc.
 *
 * This is a convenient way of writing to flash, IRAM, etc. and testing out code without needing to reflash the whole
 * chip.
 *
 * Commands are in the form:
 * \code
 * [0xaa (or report ID = 0xaa)] [1-byte COMMAND] [4-byte parameter] [Data or parameters ...]
 * \endcode
 *
 * \note The first byte is the report ID. In some systems this is separate. In others it is the first byte of the
 * message. You will need to consult examples to determine which way this is.
 *
 * \par
 * \note Windows and Linux behave differently. In Windows, you always must transfer exactly \c REPORT_SIZE+1 bytes (With
 * first byte being the report ID (0xAA). Linux allows variations.
 *
 * \par
 * \note On Windows and Linux you can transfer arbitrary lengths of data for FEATURE_GET commands.
 * \note IF YOU KNOW HOW TO SEND ARBITRARY DATA LENGTHS FOR FEATURE_SET ON WINDOWS, PLEASE CONTACT ME (cnlohr)
 *
 * The commands that are supported are defined in ::ausb_cmd_t.
 */

#ifndef _ADVANCED_USB_CONTROL_H
#define _ADVANCED_USB_CONTROL_H

#include <stdint.h>
#include <stdarg.h>

/// The number of 32 bit variables in the RAM scratch space
#define SCRATCH_IMMEDIATE_DWORDS 64

/**
 * The command types that may be sent over USB
 *
 * \note parameters are 4-bytes, LSB first byte.
 */
typedef enum __attribute__((packed))
{
    /**
     * \code
     * AUSB_CMD_REBOOT: 0x03
     *     Parameter 1:
     *         Zero: Just reboot.
     *         Nonzero: Reboot into bootloader.
     * \endcode
     */
    AUSB_CMD_REBOOT = 0x03,
    /**
     * \code
     * AUSB_CMD_WRITE_RAM: 0x04
     *     Parameter 0: Address to start writing to.
     *     Parameter 1 [2-bytes]: Length of data to write
     *     Data to write...
     * \endcode     */
    AUSB_CMD_WRITE_RAM = 0x04,
    /**
     * \code
     * AUSB_CMD_READ_RAM: 0x05
     *     Parameter 0: Address to start reading from.
     *
     * NOTE: Must use "get report" to read data
     * \endcode
     */
    AUSB_CMD_READ_RAM = 0x05,
    /**
     * \code
     * AUSB_CMD_EXEC_RAM: 0x06
     *     Parameter 0: Address to "call"
     * \endcode
     */
    AUSB_CMD_EXEC_RAM = 0x06,
    /**
     * \code
     * AUSB_CMD_SWITCH_MODE: 0x07
     *     Parameter 0: Pointer to Swadge mode (will change)
     *         If 0, will go to main mode without reboot.
     * \endcode
     */
    AUSB_CMD_SWITCH_MODE = 0x07,
    /**
     * \code
     * AUSB_CMD_ALLOC_SCRATCH: 0x08
     *     Parameter 0: Size of scrach requested.
     *         If -1, simply report.
     *         If 0, deallocate scratch.
     *
     * NOTE: Address and size of scratch will be available to "get reports" as two 4-byte parameters.
     * \endcode
     */
    AUSB_CMD_ALLOC_SCRATCH = 0x08,
    /**
     * \code
     * ACMD_CMD_MEMSET: 0x09
     *     Parameter 0: Start of memory to memset
     *     Parameter 1: Byte to write.
     * \endcode
     */
    ACMD_CMD_MEMSET = 0x09,
    /**
     * \code
     * ACMD_CMD_GETVER: 0x0a
     *     Writes a 16-byte version identifier in the scratch, which can be read. Format TBD.
     * \endcode
     */
    ACMD_CMD_GETVER = 0x0a,
    /**
     * \code
     * AUSB_CMD_FLASH_ERASE: 0x10
     *     Parameter 0: Start address of flash to erase.
     *     Parameter 1 [4 bytes]: Size of flash to erase.
     *
     * NOTE: For flash erase commands, you MUST use sector-aligned values!
     * \endcode
     */
    AUSB_CMD_FLASH_ERASE = 0x10,
    /**
     * \code
     * AUSB_CMD_FLASH_WRITE: 0x11
     *     Parameter 0: Start address of flash to write.
     *     Parameter 1 [2 bytes]: Length of data to write
     *     Payload: Data to write.
     * \endcode
     */
    AUSB_CMD_FLASH_WRITE = 0x11,
    /**
     * \code
     * AUSB_CMD_FLASH_READ: 0x12
     *     Parameter 0: Start address of flash to read.
     *     Parameter 1 [2 bytes]: Quantity of flash to read (Cannot exceed SCRATCH_IMMEDIATE_DWORDS)
     *
     * NOTE: The data is written to a scratch buffer
     * \endcode
     */
    AUSB_CMD_FLASH_READ = 0x12
} ausb_cmd_t;

int handle_advanced_usb_control_get(int reqlen, uint8_t* data);
int handle_advanced_usb_terminal_get(int reqlen, uint8_t* data);
void handle_advanced_usb_control_set(int datalen, const uint8_t* data);
int advanced_usb_write_log_printf(const char* fmt, va_list args);
int uprintf(const char* fmt, ...);

#endif

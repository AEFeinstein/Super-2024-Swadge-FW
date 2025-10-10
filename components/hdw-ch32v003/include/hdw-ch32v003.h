/*! \file hdw-ch32v003.h
 *
 * This file contains the header to control the semihosted ch32v003 via the swio programming pin.
 *
 */

#ifndef _HDW_CH32V003_H_
#define _HDW_CH32V003_H_

#include <stdbool.h>
#include <stdint.h>
#include <driver/gpio.h>

int initCh32v003(int swdio_pin);

#define CH32V003_MAX_IMAGE_SLOTS 20

/**
 * @brief Load and run a binary image from the assets folder on the ch32v003. Included as a `#define` because we can't
 * include main functionality in a module.
 *
 * @param asset cnfsFileIdx_t of a binary image to be loaded into a location in the ch32v003. This must be of type
 * cnfsFileIdx_t
 * @return 0 if OK, nonzero in error condition.
 */
#define ch32v003RunBinaryAsset(asset)                     \
    ({                                                    \
        size_t sz;                                        \
        const uint8_t* buf = cnfsGetFile(asset, &sz);     \
        int r              = ch32v003WriteFlash(buf, sz); \
        ch32v003Resume();                                 \
        r;                                                \
    })

// More nitty gritty functions

int ch32v003WriteFlash(const uint8_t* buf, int sz);
int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003GetReg(int regno, uint32_t* value);
int ch32v003SetReg(int regno, uint32_t regValue);
int ch32v003Resume();
void ch32v003CheckTerminal();
void ch32v003Teardown();

int ch32v003WriteBitmapAsset(int slot, int asset_idx);
int ch32v003WriteBitmap(int slot, const uint8_t pixels[6][12]);
int ch32v003SelectBitmap(int slot);

#endif

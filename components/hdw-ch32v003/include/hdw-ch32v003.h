/*! \file hdw-ch32v003.h
 *
 * TODO
 *
 */

#ifndef _HDW_CH32V003_H_
#define _HDW_CH32V003_H_

#include <stdbool.h>
#include <stdint.h>
#include <driver/gpio.h>

int initCh32v003(int swdio_pin);

int ch32v003WriteFlash(const uint8_t* buf, int sz);
int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003GetReg(int regno, uint32_t* value);
int ch32v003SetReg(int regno, uint32_t regValue);
int ch32v003Resume();
void ch32v003CheckTerminal();
void ch32v003Teardown();

// Only available on the emulator
void ch32v003EmuDraw(int window_w, int window_h);

#endif

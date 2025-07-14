// Stub
#include <stdint.h>

int initCh32v003(int swdio_pin)
{
	return -1;
}

int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address)
{
    return -1;
}

int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address)
{
    return -1;
}

int ch32v003GetReg(int regno, uint32_t* value)
{
    return -1;
}

int ch32v003SetReg(int regno, uint32_t regValue)
{
    return -1;
}

void ch32v003CheckTerminal()
{
}

void ch32v003Teardown()
{
}

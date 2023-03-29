#ifndef _CCCONFIG_H
#define _CCCONFIG_H

#include <stdint.h>

#define HPABUFFSIZE 512

#define CCEMBEDDED
#define DFREQ 8000

// We are not enabling these for the ESP8266 port.
#define LIN_WRAPAROUND 0
#define SORT_NOTES     0

#endif

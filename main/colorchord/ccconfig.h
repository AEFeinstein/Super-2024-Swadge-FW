#ifndef _CC_CONFIG_H
#define _CC_CONFIG_H

#include <stdint.h>

#define HPA_BUF_SIZE 512

#define CC_EMBEDDED
#define D_FREQ 8000

// We are not enabling these for the ESP8266 port.
#define LIN_WRAPAROUND 0
#define SORT_NOTES     0

#endif

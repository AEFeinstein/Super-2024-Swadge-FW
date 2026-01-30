#ifndef _SM_NAMES_H_
#define _SM_NAMES_H_

#include <stdint.h>

typedef struct {
    // Do not change the order or size of members in this struct
    uint8_t saveFormat;
    uint8_t nameLength;
    uint16_t numNames;
} names_header_t;

names_header_t* initNames(uint8_t nameLength, uint16_t numNames);

#endif


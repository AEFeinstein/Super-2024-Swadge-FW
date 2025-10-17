#ifndef _LEVELDEF_H_
#define _LEVELDEF_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    cnfsFileIdx_t filename;
    uint16_t timeLimit;
    uint16_t defaultWsgSetIndex;
    uint8_t mainBgmIndex;
    uint8_t bossBgmIndex;
} mgLeveldef_t;

#endif

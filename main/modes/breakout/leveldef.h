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
    char filename[16];
    const char* hintTextPtr;
    uint16_t bgmIndex;
} leveldef_t;

#endif

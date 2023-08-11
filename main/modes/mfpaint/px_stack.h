#ifndef _PX_STACK_H_
#define _PX_STACK_H_

#include <stddef.h>

#include "display.h"

#define PIXEL_STACK_MIN_SIZE 2


/// @brief Represents the value of a pixel and its screen coordinates
typedef struct
{
    uint16_t x, y;
    paletteColor_t col;
} pxVal_t;


/// @brief A structure for storing an unbounded number of pixels and their color.
typedef struct
{
    // Pointer to the first of pixel coordinates/values, heap-allocated
    pxVal_t* data;

    // The number of pxVal_t entries currently allocated for the stack
    size_t size;

    // The index of the value on the top of the stack
    int32_t index;
} pxStack_t;


bool initPxStack(pxStack_t* pxStack);
void freePxStack(pxStack_t* pxStack);
bool maybeGrowPxStack(pxStack_t* pxStack, size_t count);
// void maybeShrinkPxStack(pxStack_t* pxStack);
bool pushPx(pxStack_t* pxStack, display_t* disp, uint16_t x, uint16_t y);
bool popPx(pxStack_t* pxStack, display_t* disp);
bool peekPx(const pxStack_t* pxStack, pxVal_t* dest);
bool getPx(const pxStack_t* pxStack, size_t pos, pxVal_t* dest);
bool dropPx(pxStack_t* pxStack);
size_t pxStackSize(const pxStack_t* pxStack);
bool pushPxScaled(pxStack_t* pxStack, display_t* disp, int x, int y, int xTr, int yTr, int xScale, int yScale);
bool popPxScaled(pxStack_t* pxStack, display_t* disp, int xScale, int yScale);

#endif

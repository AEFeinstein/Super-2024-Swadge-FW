#include <stdio.h>
#include "hdw-led-matrices.h"

void matrixLEDInit()
{
    // Set up pin
    // load some code into the CH32 if required
    // If required, send default expressions
}

void matrixLEDSendExpression(ledMatrix_t* mat)
{
    // Sets the expression to the brightnesses in the matrix
}

void matrixLEDSetExpression(defaultExpressions_t expr)
{
    // Sets a default expression
}

void matrixLEDSetBrightness(uint8_t brightness)
{
    // Sets the max brightness that the eyes can be set to. 0 is off, 7 is max
}

void matrixLEDDeinit(void)
{
    // Teardown
}
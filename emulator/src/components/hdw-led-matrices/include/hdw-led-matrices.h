#pragma once

/* Current assumptions:
 * - LED Matrix is 6x6x2
 * - We are writing expressions to RAM and can switch between them on the fly
 * - Some default expressions 
 */

#define MATRIX_UART_PIN 42
#define MATRIX_LED_COUNT 72

typedef enum
{
    LEDM_BLANK,
    LEDM_SPINNY,
    LEDM_CLOSED,
    LEDM_OPEN,
} defaultExpressions_t;

typedef struct 
{
    uint8_t matrix[MATRIX_LED_COUNT]; ///< 72 LEDs, brightness 
} ledMatrix_t;

/**
 * @brief Initializes the LED Matrix. Should only be called once
 * 
 */
void matrixLEDInit(void);

/**
 * @brief Sends a custom expression to the matrix
 * 
 * @param mat Matrix to write
 */
void matrixLEDSendExpression(ledMatrix_t* mat);

/**
 * @brief Sets one of the pre-loaded expressions
 * 
 * @param expr The index of the expression to set
 */
void matrixLEDSetExpression(defaultExpressions_t expr);

/**
 * @brief Sets the max brightness for the LEDs. 0-255
 * 
 * @param brightness 
 */
void matrixLEDSetBrightness(uint8_t brightness);

/**
 * @brief Tear down whatever needs torn down
 * 
 */
void matrixLEDDeinit(void);
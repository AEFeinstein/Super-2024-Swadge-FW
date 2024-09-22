/**
 * @file cg_Field.h
 * @author your name (you@domain.com)
 * @brief The field of play for the main garden area
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "cg_Typedef.h"

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Sets up the field
 *
 * @param cg Game Object
 */
void cgInitField(cGrove_t* cg);

/**
 * @brief Moves the view of the field byt eh provided x and y
 *
 * @param cg Game Object
 * @param xChange Distance to move horizontally. Negative is right, positive if left.
 * @param yChange Distance to move vertically. Negative is up, positive s down
 */
void cgMoveCamera(cGrove_t* cg, int16_t xChange, int16_t yChange);

/**
 * @brief Draws the field
 *
 * @param cg Game Object
 */
void cgDrawField(cGrove_t* cg);
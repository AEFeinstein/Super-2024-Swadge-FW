/**
 * @file jerkChicken.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode about jerking the chicken. No, not like the food. Not that way either.
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "jerkChicken.h"

//==============================================================================
// Consts
//==============================================================================

const char chickenModeName[] = "Jerk Chicken";

//==============================================================================
// Structs
//==============================================================================

typedef struct 
{

} chickenData_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void enterChicken(void);
static void exitChicken(void);
static void chickenLoop(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t chickenMode = {
    .modeName    = chickenModeName,
    .fnEnterMode = enterChicken,
    .fnExitMode  = exitChicken,
    .fnMainLoop  = chickenLoop,
};

chickenData_t* cd;

//==============================================================================
// Functions
//==============================================================================

static void enterChicken(void)
{
    cd = (chickenData_t*)heap_caps_calloc(1, sizeof(chickenData_t), MALLOC_CAP_8BIT);
}

static void exitChicken(void)
{
    heap_caps_free(cd);
}

static void chickenLoop(int64_t elapsedUs)
{
}
/**
 * @file renderer3dTest.h
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief Tests the 3D capabilities of the renderer
 * @version 0.1
 * @date 2024-12-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

//==============================================================================
// Includes
//==============================================================================

#include "renderer3dTest.h"

//==============================================================================
// Consts
//==============================================================================

static const char modeName[] = "3D Renderer Test";

//==============================================================================
// Structs
//==============================================================================

typedef struct
{

} ren3_t;

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Required mode funcs
 * 
 */
static void enterMode(void);
static void exitMode(void);
static void mainLoop(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t renderer3dTestMode = {
    .modeName                 = modeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = enterMode,
    .fnExitMode               = exitMode,
    .fnMainLoop               = mainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

ren3_t* render;

//==============================================================================
// Function Definitions
//==============================================================================

static void enterMode(void)
{
    // Initialize memory
    // Slower RAM (MIDI may squeeze this out)
    // render = (ren3_t*)heap_caps_calloc(1, sizeof(ren3_t), MALLOC_CAP_8BIT);
    // Faster RAM
    render = calloc(1, sizeof(ren3_t));
}

static void exitMode(void)
{

}

static void mainLoop(int64_t elapsedUs)
{

}
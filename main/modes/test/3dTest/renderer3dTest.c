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
#include "cVectors.h"

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

// Mode functions
static void enterMode(void);
static void exitMode(void);
static void mainLoop(int64_t elapsedUs);

// Individual tests

/**
 * @brief Tests C++ style vectors
 *
 * @return int Error codes
 */
static void vecTest(void);

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
    // Free mode
    // Slower
    // heap_caps_free(render);
    // Faster
    free(render);
}

static void mainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            // Vec Test
            vecTest();
        }
    }
}

static void vecTest()
{
    // Initialize
    cVector_t v;
    int i = 1;
    int j = 5;
    int k = 19;
    int l = 42;
    int m = 109;
    int n = 207;

    // Init Vector
    initVector(&v);
    if (v.total != 0)
    {
        ESP_LOGI("VecTest", "Error - Total doesn't match after initialization.");
        freeVector(&v);
        return;
    }
    else if (v.capacity != 4)
    {
        ESP_LOGI("VecTest", "Error - Capacity does not match after initialization.");
        freeVector(&v);
        return;
    }

    // Get total
    if (vectorTotal(&v) != 0)
    {
        ESP_LOGI("VecTest", "Error - Getting total via 'vectorTotal' resulted in wrong value.");
        freeVector(&v);
        return;
    }

    // Add item
    vectorAdd(&v, &i);
    if (vectorTotal(&v) != 1)
    {
        ESP_LOGI("VecTest", "Error - First item added, total not updated");
        freeVector(&v);
        return;
    }
    if (v.data[0] != &i)
    {
        ESP_LOGI("VecTest", "Error - First item added, pointer is incorrect.");
        freeVector(&v);
        return;
    }

    // Add multiple items
    vectorAdd(&v, &j);
    vectorAdd(&v, &k);
    vectorAdd(&v, &l);
    vectorAdd(&v, &m);
    if (vectorTotal(&v) != 5)
    {
        ESP_LOGI("VecTest", "Error - Several items added, total not updated");
        freeVector(&v);
        return;
    }
    if (v.data[2] != &k)
    {
        ESP_LOGI("VecTest", "Error - Several items added, index 3 not correct pointer.");
        freeVector(&v);
        return;
    }
    if (v.capacity != 8)
    {
        ESP_LOGI("VecTest", "Error - Several items added, capacity hasn't updated.");
        freeVector(&v);
        return;
    }

    // Vector Set
    vectorSet(&v, 1, &n);
    vectorSet(&v, 6, &l);  // FIXME: Catch error if bounds check fails
    vectorSet(&v, -1, &l); // FIXME: Catch error if bounds check fails
    if (vectorTotal(&v) != 5)
    {
        ESP_LOGI("VecTest", "Error - Vector Set, Total not correct");
        freeVector(&v);
        return;
    }
    if (v.data[1] != &n)
    {
        ESP_LOGI("VecTest", "Error - Set data incorrect");
        freeVector(&v);
        return;
    }

    // Vector Get
    if (vectorGet(&v, 4) != &m)
    {
        ESP_LOGI("VecTest", "Error - Get grabbed the wrong value");
        freeVector(&v);
        return;
    }

    // Vector Remove
    vectorRm(&v, 1);
    if(vectorGet(&v, 3) != &m)
    {
        ESP_LOGI("VecTest", "Error - RM failed");
        freeVector(&v);
        return;
    }
    
    // Free and exit
    freeVector(&v);

    // Exit
    ESP_LOGI("VecTest", "C++ Vector tests okay");
}
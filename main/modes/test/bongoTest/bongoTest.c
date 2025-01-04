/**
 * @file bongoTest.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Tests your beatboxing skills
 * @version 1.0
 * @date 2025-01-04
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "bongoTest.h"
#include "wsgPalette.h"

//==============================================================================
// Defines
//==============================================================================

#define HIT_DURATION 50000
#define HEAT_CUTOFF  10
#define DECAY_RATE   200000

//==============================================================================
// Consts
//==============================================================================

const char bongoModeName[] = "Bongo Bongo Bongo";

const char* bongoWsgs[] = {"bongoDown.wsg", "bongoLeft.wsg", "bongoRight.wsg", "bongoUp.wsg", "bongoTable.wsg"};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Assets
    wsg_t* sprs;

    // Bongo
    bool hits[2];

    // Rainbows
    uint8_t bgColor;
    uint8_t heat;
    int64_t heatDecay;
    wsgPalette_t pal;
} bongoTest_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Entrance into the mode
 *
 */
static void enterTheBongo(void);

/**
 * @brief Mode cleanup
 *
 */
static void abandonBongo(void);

/**
 * @brief Main Loop
 *
 * @param elapsedUs Amount of time since last called
 */
static void playWithBongo(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t bongoTest = {
    .modeName                 = bongoModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = enterTheBongo,
    .fnExitMode               = abandonBongo,
    .fnMainLoop               = playWithBongo,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

bongoTest_t* bt;

//==============================================================================
// Functions
//==============================================================================

static void enterTheBongo(void)
{
    bt       = (bongoTest_t*)heap_caps_calloc(1, sizeof(bongoTest_t), MALLOC_CAP_8BIT);
    bt->sprs = heap_caps_calloc(ARRAY_SIZE(bongoWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(bongoWsgs); tIdx++)
    {
        loadWsg(bongoWsgs[tIdx], &bt->sprs[tIdx], true);
    }
    wsgPaletteReset(&bt->pal);
}

static void abandonBongo(void)
{
    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(bongoWsgs); tIdx++)
    {
        freeWsg(&bt->sprs[tIdx]);
    }
    heap_caps_free(bt->sprs);
    free(bt);
}

static void playWithBongo(int64_t elapsedUs)
{
    // TODO: LEDs

    // Decay
    bt->heatDecay += elapsedUs;
    if (bt->heatDecay >= DECAY_RATE)
    {
        bt->heatDecay = 0;
        bt->heat--;
    }

    // Input me harder
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A)
            {
                bt->hits[0] = true;
                // TODO: Play sound
            }
            else if (evt.button & PB_B)
            {
                bt->hits[1] = true;
                // TODO: Play sound
            }
            bt->bgColor++;
            if (bt->heat <= 254)
            {
                bt->heat++;
            }
        }
        else if (!evt.down)
        {
            if (evt.button & PB_A)
            {
                bt->hits[0] = false;
            }
            else if (evt.button & PB_B)
            {
                bt->hits[1] = false;
            }
        }
    }
    if (bt->heat >= HEAT_CUTOFF)
    {
        wsgPaletteSet(&bt->pal, c555, paletteHsvToHex((bt->bgColor + 128) % 255, 255, 255));
    }
    else
    {
        wsgPaletteSet(&bt->pal, c555, c555);
    }

    // Draw me like a french poodle
    // BG
    if (bt->heat >= HEAT_CUTOFF)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT - 64, paletteHsvToHex(bt->bgColor, 255, 255));
    }
    else
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT - 64, c000);
    }

    // Table
    drawWsgPaletteSimple(&bt->sprs[4], 0, 0, &bt->pal);

    // Cat
    if (!bt->hits[0] && !bt->hits[1])
    {
        drawWsgSimple(&bt->sprs[3], 0, 0);
    }
    else if (!bt->hits[0] && bt->hits[1])
    {
        drawWsgSimple(&bt->sprs[2], 0, 0);
    }
    else if (bt->hits[0] && !bt->hits[1])
    {
        drawWsgSimple(&bt->sprs[1], 0, 0);
    }
    else if (bt->hits[0] && bt->hits[1])
    {
        drawWsgSimple(&bt->sprs[0], 0, 0);
    }
}
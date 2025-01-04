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
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

#define HEAT_CUTOFF 10
#define DECAY_RATE  200000
#define UNSITCK_VAL 250000

//==============================================================================
// Consts
//==============================================================================

const char bongoModeName[] = "Bongo Bongo Bongo";

const char* const bongoWsgs[]
    = {"bongoDown.wsg", "bongoLeft.wsg", "bongoRight.wsg", "bongoUp.wsg", "bongoTable.wsg", "bongoMeow.wsg"};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Assets
    wsg_t* sprs;

    // Bongo
    bool hits[6];
    int32_t unstickTimer;

    // Rainbows
    uint8_t bgColor;
    uint8_t heat;
    int64_t heatDecay;
    wsgPalette_t pal;

    // LEDs
    led_t leds[CONFIG_NUM_LEDS];

    // MIDI
    midiPlayer_t* player;
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

static bongoTest_t* bt;

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
    initGlobalMidiPlayer();
    bt->player                    = globalMidiPlayerGet(MIDI_BGM);
    bt->player->mode              = MIDI_STREAMING;
    bt->player->streamingCallback = NULL;
    midiGmOn(bt->player);
    midiPause(bt->player, false);
    wsgPaletteReset(&bt->pal);
    bt->heat = 0;
}

static void abandonBongo(void)
{
    deinitGlobalMidiPlayer();
    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(bongoWsgs); tIdx++)
    {
        freeWsg(&bt->sprs[tIdx]);
    }
    heap_caps_free(bt->sprs);
    heap_caps_free(bt);
}

static void playWithBongo(int64_t elapsedUs)
{
    // Decay
    bt->heatDecay += elapsedUs;
    if (bt->heatDecay >= DECAY_RATE && bt->heat > 0)
    {
        bt->heatDecay = 0;
        bt->heat--;
    }

    // LEDs
    for (int idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        if (bt->leds[idx].r > 2)
        {
            bt->leds[idx].r -= 3;
        }
        if (bt->leds[idx].g > 2)
        {
            bt->leds[idx].g -= 3;
        }
        if (bt->leds[idx].b > 2)
        {
            bt->leds[idx].b -= 3;
        }
    }
    setLeds(bt->leds, CONFIG_NUM_LEDS);

    bt->unstickTimer += elapsedUs;
    // Input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            bt->unstickTimer = 0;
            if (evt.button & PB_A)
            {
                bt->hits[0] = true;
                midiNoteOn(bt->player, 9, ELECTRIC_SNARE_OR_RIMSHOT, 0x7F);
            }
            else if (evt.button & PB_B)
            {
                bt->hits[1] = true;
                midiNoteOn(bt->player, 9, CLOSED_HI_HAT, 0x7F);
            }
            else if (evt.button & PB_UP)
            {
                bt->hits[2] = true;
                midiNoteOn(bt->player, 0, 72, 0x7F);
            }
            else if (evt.button & PB_DOWN)
            {
                bt->hits[3] = true;
                midiNoteOn(bt->player, 0, 71, 0x7F);
            }
            else if (evt.button & PB_LEFT)
            {
                bt->hits[4] = true;
                midiNoteOn(bt->player, 0, 74, 0x7F);
            }
            else if (evt.button & PB_RIGHT)
            {
                bt->hits[5] = true;
                midiNoteOn(bt->player, 0, 76, 0x7F);
            }
            bt->bgColor++;
            bt->leds[esp_random() % CONFIG_NUM_LEDS] = LedEHSVtoHEXhelper(bt->bgColor, 255, 200, true);
            if (bt->heat <= 254)
            {
                bt->heat++;
                bt->heatDecay = 0; // Avoids
            }
        }
        else if (!evt.down)
        {
            bt->unstickTimer = 0;
            if (evt.button & PB_A)
            {
                bt->hits[0] = false;
            }
            else if (evt.button & PB_B)
            {
                bt->hits[1] = false;
            }
            else if (evt.button & PB_UP)
            {
                bt->hits[2] = false;
                midiNoteOff(bt->player, 0, 72, 0x7F);
            }
            else if (evt.button & PB_DOWN)
            {
                bt->hits[3] = false;
                midiNoteOff(bt->player, 0, 71, 0x7F);
            }
            else if (evt.button & PB_LEFT)
            {
                bt->hits[4] = false;
                midiNoteOff(bt->player, 0, 74, 0x7F);
            }
            else if (evt.button & PB_RIGHT)
            {
                bt->hits[5] = false;
                midiNoteOff(bt->player, 0, 76, 0x7F);
            }
        }
    }
    if (bt->unstickTimer >= UNSITCK_VAL)
    {
        bt->unstickTimer = 0;
        midiPlayerReset(bt->player);
        bt->player->mode              = MIDI_STREAMING;
        bt->player->streamingCallback = NULL;
        midiGmOn(bt->player);
        midiPause(bt->player, false);
        for (int idx = 0; idx < 6; idx++)
        {
            bt->hits[idx] = false;
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

    // Draw
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

    if (bt->hits[2] || bt->hits[3] || bt->hits[4] || bt->hits[5])
    {
        drawWsgSimple(&bt->sprs[5], 108, 92);
    }
}
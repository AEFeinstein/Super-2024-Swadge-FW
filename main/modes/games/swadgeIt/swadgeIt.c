//==============================================================================
// Includes
//==============================================================================

#include "swadgeIt.h"
#include "mainMenu.h"
#include "heatshrink_helper.h"
#include "touchUtils.h"

//==============================================================================
// Defines
//==============================================================================

#define SWADGE_IT_FPS 40

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SI_MENU,
    SI_REACTION,
    SI_MEMORY,
    SI_HIGH_SCORES,
} swadgeItScreen_t;

typedef enum
{
    EVT_PRESS_IT,
    EVT_SHAKE_IT,
    EVT_YELL_IT,
    EVT_SWIRL_IT,
    MAX_NUM_EVTs,
} swadgeItEvt_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} vec3d_t;

typedef struct
{
    uint8_t* samples;
    uint32_t len;
} rawSample_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    swadgeItScreen_t screen;

    rawSample_t sfx[MAX_NUM_EVTs];

    uint32_t sampleIdx;
    bool pendingSwitchToMic;

    uint32_t timeToNextEvent;
    uint32_t nextEvtTimer;
    uint32_t currentEvt;
    bool evtSuccess;
    int32_t score;

    vec3d_t lastOrientation;

    uint32_t micSamplesProcessed;
    uint32_t micEnergy;
    uint32_t micFrameEnergy;

    touchSpinState_t touchSpinState;
} swadgeIt_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void swadgeItEnterMode(void);
static void swadgeItExitMode(void);
static void swadgeItMainLoop(int64_t elapsedUs);
static void swadgeItBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void swadgeItDacCallback(uint8_t* samples, int16_t len);
static void swadgeItAudioCallback(uint16_t* samples, uint32_t sampleCnt);

static void swadgeItMenuCb(const char* label, bool selected, uint32_t value);

static void swadgeItInput(swadgeItEvt_t evt);
static bool swadgeItCheckForShake(void);
static void swadgeItGameOver(void);

//==============================================================================
// Const data
//==============================================================================

static const char swadgeItStrName[]       = "Swadge It!";
static const char swadgeItStrReaction[]   = "Reaction";
static const char swadgeItStrMemory[]     = "Memory";
static const char swadgeItStrHighScores[] = "High Scores";
static const char swadgeItStrExit[]       = "Exit";

/** Must match order of swadgeItEvt_t */
const char* swadgeItSfxFiles[MAX_NUM_EVTs] = {
    "bopit.raw",
    "flickit.raw",
    "pullit.raw",
    "spinit.raw",
};

swadgeMode_t swadgeItMode = {
    .modeName                 = swadgeItStrName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = swadgeItEnterMode,
    .fnExitMode               = swadgeItExitMode,
    .fnMainLoop               = swadgeItMainLoop,
    .fnAudioCallback          = swadgeItAudioCallback,
    .fnBackgroundDrawCallback = swadgeItBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = swadgeItDacCallback,
};

//==============================================================================
// Variables
//==============================================================================

static swadgeIt_t* si;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
static void swadgeItEnterMode(void)
{
    // Set to 40 fps (25ms per frame)
    setFrameRateUs(1000000 / SWADGE_IT_FPS);

    // Allocate mode memory
    si = heap_caps_calloc(1, sizeof(swadgeIt_t), MALLOC_CAP_8BIT);

    // Allocate menu
    si->menu = initMenu(swadgeItStrName, swadgeItMenuCb);
    addSingleItemToMenu(si->menu, swadgeItStrReaction);
    addSingleItemToMenu(si->menu, swadgeItStrMemory);
    addSingleItemToMenu(si->menu, swadgeItStrHighScores);
    addSingleItemToMenu(si->menu, swadgeItStrExit);
    si->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    for (uint8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        si->sfx[i].samples = readHeatshrinkFile(swadgeItSfxFiles[i], &si->sfx[i].len, true);
    }

    si->currentEvt = MAX_NUM_EVTs;
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
static void swadgeItExitMode(void)
{
    // Free menu
    deinitMenuManiaRenderer(si->menuRenderer);
    deinitMenu(si->menu);

    for (uint8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        heap_caps_free(si->sfx[i].samples);
    }

    // Free mode memory
    heap_caps_free(si);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void swadgeItMainLoop(int64_t elapsedUs)
{
    if (si->pendingSwitchToMic)
    {
        // Then switch back to the microphone
        switchToMicrophone();

        // Reset mic values
        si->micSamplesProcessed = 0;
        si->micFrameEnergy      = 0;
        si->micEnergy           = 0;

        // Lower flag
        si->pendingSwitchToMic = false;
    }

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        switch (si->screen)
        {
            default:
            case SI_MENU:
            {
                si->menu = menuButton(si->menu, evt);
                break;
            }
            case SI_REACTION:
            case SI_MEMORY:
            {
                // Check for pushbutton events
                if (evt.down)
                {
                    switch (evt.button)
                    {
                        case PB_A:
                        case PB_B:
                        case PB_UP:
                        case PB_DOWN:
                        case PB_LEFT:
                        case PB_RIGHT:
                        {
                            // These buttons are events
                            swadgeItInput(EVT_PRESS_IT);
                            break;
                        }
                        default:
                        {
                            // All other buttons quit
                            si->screen = SI_MENU;
                            break;
                        }
                    }
                    break;
                }
                break;
            }
            case SI_HIGH_SCORES:
            {
                if (evt.down)
                {
                    si->screen = SI_MENU;
                }
                break;
            }
        }
    }

    switch (si->screen)
    {
        default:
        case SI_MENU:
        {
            drawMenuMania(si->menu, si->menuRenderer, elapsedUs);
            break;
        }
        case SI_REACTION:
        {
            // Draw current score
            font_t* font = si->menuRenderer->menuFont;
            char scoreStr[32];
            snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, si->score);
            int16_t tWidth = textWidth(font, scoreStr);
            drawText(font, c555, scoreStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - font->height) / 2);

            // Check for motion events
            if (swadgeItCheckForShake())
            {
                swadgeItInput(EVT_SHAKE_IT);
            }

            // Check for touchpad spin events
            int32_t phi       = 0;
            int32_t r         = 0;
            int32_t intensity = 0;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                getTouchSpins(&si->touchSpinState, phi, intensity);
                if (si->touchSpinState.spins)
                {
                    swadgeItInput(EVT_SWIRL_IT);
                    si->touchSpinState.spins = 0;
                }
            }
            else
            {
                si->touchSpinState.startSet = false;
            }

            // Check the gameplay timer
            RUN_TIMER_EVERY(si->nextEvtTimer, si->timeToNextEvent, elapsedUs, {
                // If there was successful event input
                if (si->evtSuccess)
                {
                    // Enable speaker for a new verbal command
                    switchToSpeaker();

                    // Pick a new event and reset the sample count
                    si->currentEvt = esp_random() % MAX_NUM_EVTs;
                    si->sampleIdx  = 0;

                    // Reset success flag
                    si->evtSuccess = false;

                    // Decrement the time between events
                    // if (si->timeToNextEvent > 500000)
                    // {
                    //     si->timeToNextEvent -= 100000;
                    // }
                }
                else
                {
                    // Input not received in time
                    swadgeItGameOver();
                }
            });
            break;
        }
        case SI_MEMORY:
        {
            // TODO gameplay logic
            break;
        }
        case SI_HIGH_SCORES:
        {
            // TODO high score rendering
            break;
        }
    }
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
static void swadgeItBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (si->screen)
    {
        default:
        case SI_MENU:
        {
            break;
        }
        case SI_REACTION:
        {
            // TODO gameplay rendering
            fillDisplayArea(x, y, x + w, y + h, c123);
            break;
        }
        case SI_MEMORY:
        {
            // TODO gameplay rendering
            fillDisplayArea(x, y, x + w, y + h, c321);
            break;
        }
        case SI_HIGH_SCORES:
        {
            // TODO high score rendering
            fillDisplayArea(x, y, x + w, y + h, c132);
            break;
        }
    }
}

/**
 * @brief This function is called to fill sample buffers for the DAC. If this is NULL, then
 * globalMidiPlayerFillBuffer() will be used instead to fill sample buffers
 *
 * @param samples The sample buffer to fill
 * @param len The number of samples to fill
 */
static void swadgeItDacCallback(uint8_t* samples, int16_t len)
{
    if (!si->pendingSwitchToMic && si->currentEvt < MAX_NUM_EVTs)
    {
        const rawSample_t* rs = &si->sfx[si->currentEvt];
        if (rs->samples)
        {
            // Make sure we don't read out of bounds
            int16_t cpLen = len;
            if (si->sampleIdx + len > rs->len)
            {
                cpLen = (rs->len - si->sampleIdx);
            }

            // Copy samples out
            memcpy(samples, &rs->samples[si->sampleIdx], cpLen);
            si->sampleIdx += cpLen;

            // If something was copied and the sample is now over
            if (cpLen && si->sampleIdx >= rs->len)
            {
                // Raise flag to switch back to mic mode in the main loop
                si->pendingSwitchToMic = true;
            }

            // Increment samples and decrement len
            samples += cpLen;
            len -= cpLen;
        }
    }

    // If there's anything else to write
    if (len)
    {
        // Write blanks
        memset(samples, 127, len);
    }
}

/**
 * @brief Process Swadge It inputs (button, shake, touch spin, yells)
 *
 * @param evt The event that occurred
 */
static void swadgeItInput(swadgeItEvt_t evt)
{
    if (evt == si->currentEvt)
    {
        if (SI_REACTION == si->screen)
        {
            if (!si->evtSuccess)
            {
                // Correct input, raise flag for next event
                si->evtSuccess = true;
                si->score++;
            }
        }
        else if (SI_MEMORY == si->screen)
        {
            // TODO correct input, move to next event
        }
    }
    else if ((SI_REACTION == si->screen) || (SI_MEMORY == si->screen))
    {
        // Input event doesn't match
        swadgeItGameOver();
    }

#if 0
    switch (evt)
    {
        case EVT_PRESS_IT:
        {
            printf("PRESS IT\n");
            break;
        }
        case EVT_SHAKE_IT:
        {
            printf("SHAKE IT\n");
            break;
        }
        case EVT_SWIRL_IT:
        {
            printf("SWIRL IT\n");
            break;
        }
        case EVT_YELL_IT:
        {
            printf("YELL IT\n");
            break;
        }
    }
#endif
}

/**
 * @brief This function is called whenever audio samples are read from the microphone (ADC) and are ready for
 * processing. Samples are read at 8KHz.
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
static void swadgeItAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    while (sampleCnt--)
    {
        // Get and process the sample
        int16_t samp = *(samples++);
        samp /= 2048;
        // Sum the absolute values
        si->micEnergy += ABS(samp);
        si->micSamplesProcessed++;
        // If we've captured a visual frame's worth of samples
        if ((ADC_SAMPLE_RATE_HZ / SWADGE_IT_FPS) == si->micSamplesProcessed)
        {
            // Save to micFrameEnergy and reset
            si->micSamplesProcessed = 0;
            si->micFrameEnergy      = si->micEnergy;
            si->micEnergy           = 0;

            // TODO hysteresis?
            if (si->micFrameEnergy > 500)
            {
                swadgeItInput(EVT_YELL_IT);
            }
        }
    }
}

/**
 * @brief Check if a shake was detected
 *
 * @return true if a shake was detected, false otherwise
 */
static bool swadgeItCheckForShake(void)
{
    if (ESP_OK == accelIntegrate())
    {
        vec3d_t orientation;
        if (ESP_OK == accelGetAccelVecRaw(&orientation.x, &orientation.y, &orientation.z))
        {
            vec3d_t delta = {
                .x = ABS(si->lastOrientation.x - orientation.x),
                .y = ABS(si->lastOrientation.y - orientation.y),
                .z = ABS(si->lastOrientation.z - orientation.z),
            };
            si->lastOrientation = orientation;
            uint32_t tDelta     = delta.x + delta.y + delta.z;
            if (tDelta > 300)
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Game over, called when there's incorrect input or no input
 */
static void swadgeItGameOver(void)
{
    // TODO
    si->screen = SI_MENU;
}

/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 */
static void swadgeItMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (swadgeItStrReaction == label)
        {
            si->screen = SI_REACTION;

            si->timeToNextEvent = 2000000;
            si->nextEvtTimer    = 0;
            si->currentEvt      = MAX_NUM_EVTs;

            // Start with this raised for the first loop
            si->evtSuccess = true;

            si->score = 0;
        }
        else if (swadgeItStrMemory == label)
        {
            si->screen = SI_MEMORY;
        }
        else if (swadgeItStrHighScores == label)
        {
            si->screen = SI_HIGH_SCORES;
        }
        else if (swadgeItStrExit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}

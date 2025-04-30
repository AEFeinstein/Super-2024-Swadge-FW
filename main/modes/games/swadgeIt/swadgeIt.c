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

// TODO tune values
#define MIC_ENERGY_THRESHOLD  1000
#define MIC_ENERGY_HYSTERESIS 10

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SI_MENU,
    SI_REACTION,
    SI_MEMORY,
    SI_HIGH_SCORES,
    SI_GAME_OVER,
} swadgeItScreen_t;

typedef enum
{
    EVT_PRESS_IT,
    EVT_SHAKE_IT,
    EVT_YELL_IT,
    EVT_SWIRL_IT,
    MAX_NUM_EVTS,
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
    const char* label;
    const char* sfx_fname;
    const paletteColor_t bgColor;
    const paletteColor_t txColor;
    const led_t ledColor;
} swadgeItEvtData_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    swadgeItScreen_t screen;

    rawSample_t sfx[MAX_NUM_EVTS];

    int32_t sampleIdx;
    bool pendingSwitchToMic;

    int32_t timeToNextEvent;
    int32_t nextEvtTimer;
    list_t inputQueue;
    list_t memoryQueue;
    list_t speechQueue;
    int32_t speechDelayUs;
    int32_t score;
    const swadgeItEvtData_t* dispEvt;

    vec3d_t lastOrientation;

    int32_t micSamplesProcessed;
    int32_t micEnergy;
    list_t micFrameEnergyHistory;
    bool isYelling;

    touchSpinState_t touchSpinState;

    int32_t memoryHighScore;
    int32_t reactionHighScore;

    int32_t gameOverTimer;
    bool newHighScore;
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
static void swadgeItUpdateDisplay(void);

//==============================================================================
// Const data
//==============================================================================

static const char swadgeItStrName[]       = "Swadge It!";
static const char swadgeItStrReaction[]   = "Reaction";
static const char swadgeItStrMemory[]     = "Memory";
static const char swadgeItStrHighScores[] = "High Scores";
static const char swadgeItStrExit[]       = "Exit";

static const char SI_REACTION_HS_KEY[] = "si_r_hs";
static const char SI_MEMORY_HS_KEY[]   = "si_m_hs";

/** Must match order of swadgeItEvt_t */
const swadgeItEvtData_t siEvtData[] = {
    {
        .sfx_fname = "bopit.raw",
        .label     = "Press it!",
        .bgColor   = c531,
        .txColor   = c555,
        .ledColor  = {.r = 0xFF, .g = 0x99, .b = 0x33},
    },
    {
        .sfx_fname = "flickit.raw",
        .label     = "Shake it!",
        .bgColor   = c325,
        .txColor   = c555,
        .ledColor  = {.r = 0x99, .g = 0x66, .b = 0xFF},
    },
    {
        .sfx_fname = "pullit.raw",
        .label     = "Yell it!",
        .bgColor   = c222,
        .txColor   = c555,
        .ledColor  = {.r = 0x66, .g = 0x66, .b = 0x66},
    },
    {
        .sfx_fname = "spinit.raw",
        .label     = "Swirl it!",
        .bgColor   = c412,
        .txColor   = c555,
        .ledColor  = {.r = 0xCC, .g = 0x33, .b = 0x66},
    },
};

const swadgeItEvtData_t siGoodData = {
    .sfx_fname = NULL,
    .label     = "Good!",
    .bgColor   = c453,
    .txColor   = c000,
    .ledColor  = {.r = 0xCC, .g = 0xFF, .b = 0x99},
};

const swadgeItEvtData_t siWaitData = {
    .sfx_fname = NULL,
    .label     = "Wait",
    .bgColor   = c000,
    .txColor   = c555,
    .ledColor  = {.r = 0x00, .g = 0x00, .b = 0x00},
};

const swadgeItEvtData_t siGoData = {
    .sfx_fname = NULL,
    .label     = "Go!",
    .bgColor   = c555,
    .txColor   = c000,
    .ledColor  = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
};

//==============================================================================
// Variables
//==============================================================================

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

    for (int8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        si->sfx[i].samples = readHeatshrinkFile(siEvtData[i].sfx_fname, &si->sfx[i].len, true);
    }

    clear(&si->inputQueue);
    clear(&si->memoryQueue);
    clear(&si->speechQueue);

    // Read high scores from NVS
    if (!readNvs32(SI_REACTION_HS_KEY, &si->reactionHighScore))
    {
        writeNvs32(SI_REACTION_HS_KEY, 0);
        si->reactionHighScore = 0;
    }
    if (!readNvs32(SI_MEMORY_HS_KEY, &si->memoryHighScore))
    {
        writeNvs32(SI_MEMORY_HS_KEY, 0);
        si->memoryHighScore = 0;
    }
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
static void swadgeItExitMode(void)
{
    // Free menu
    deinitMenuManiaRenderer(si->menuRenderer);
    deinitMenu(si->menu);

    for (int8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
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
    // Check button input
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
                    // Every button is input
                    swadgeItInput(EVT_PRESS_IT);
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
            case SI_GAME_OVER:
            {
                if (si->gameOverTimer <= 0 && evt.down)
                {
                    si->screen       = SI_MENU;
                    si->score        = 0;
                    si->newHighScore = false;
                }
                break;
            }
        }
    }

    // Main game logic and drawing
    switch (si->screen)
    {
        default:
        case SI_MENU:
        {
            drawMenuMania(si->menu, si->menuRenderer, elapsedUs);
            break;
        }
        case SI_REACTION:
        case SI_MEMORY:
        {
            // Check if the flag is raised to switch from speaker to microphone
            bool dequeueSpeech = false;
            if (si->pendingSwitchToMic)
            {
                // Then switch back to the microphone
                switchToMicrophone();

                // Reset mic values
                si->micSamplesProcessed = 0;
                clear(&si->micFrameEnergyHistory);
                si->micEnergy = 0;
                si->isYelling = false;

                // Lower flag
                si->pendingSwitchToMic = false;

                // Dequeue the speech that raised the flag
                dequeueSpeech = true;
            }
            // If the timer between verbal actions is running
            else if (si->speechDelayUs > 0)
            {
                // Decrement it
                si->speechDelayUs -= elapsedUs;

                // If the timer elapsed
                if (si->speechDelayUs <= 0)
                {
                    dequeueSpeech = true;
                }
            }

            if (dequeueSpeech)
            {
                // Remove from the speech queue
                shift(&si->speechQueue);
                // Reset sample index
                si->sampleIdx = 0;

                // Set new LEDs
                swadgeItUpdateDisplay();
            }

            // Check for motion events
            if (swadgeItCheckForShake())
            {
                // TODO hysteresis?
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

            // Check for yells
            if (si->micFrameEnergyHistory.length)
            {
                if (false == si->isYelling && (intptr_t)si->micFrameEnergyHistory.last->val > MIC_ENERGY_THRESHOLD)
                {
                    swadgeItInput(EVT_YELL_IT);
                    si->isYelling = true;
                }
                else if (si->isYelling)
                {
                    int32_t quietSamples = 0;
                    node_t* micNode      = si->micFrameEnergyHistory.first;
                    while (micNode)
                    {
                        if ((intptr_t)micNode->val > MIC_ENERGY_THRESHOLD)
                        {
                            // Still yelling
                            break;
                        }
                        else
                        {
                            quietSamples++;
                        }
                        micNode = micNode->next;
                    }

                    if (quietSamples >= MIC_ENERGY_HYSTERESIS)
                    {
                        clear(&si->micFrameEnergyHistory);
                        si->isYelling = false;
                    }
                }
            }

            // Run game specific logic
            if (SI_REACTION == si->screen)
            {
                // For reaction mode, check the gameplay timer
                RUN_TIMER_EVERY(si->nextEvtTimer, si->timeToNextEvent, elapsedUs, {
                    // If there was successful event input (i.e. the queue was emptied)
                    if (0 == si->inputQueue.length)
                    {
                        // Enable speaker for a new verbal command and reset sample count
                        switchToSpeaker();
                        si->sampleIdx = 0;

                        // Pick a new event and enqueue it
                        swadgeItEvt_t newEvt = esp_random() % MAX_NUM_EVTS;
                        push(&si->inputQueue, (void*)newEvt);
                        push(&si->speechQueue, (void*)newEvt);

                        // Set the LEDs for the new event
                        swadgeItUpdateDisplay();

                        // Decrement the time between events
                        // TODO tune gameplay
                        if (si->timeToNextEvent > 500000)
                        {
                            si->timeToNextEvent -= 100000;
                        }
                    }
                    else
                    {
                        // Input not received in time
                        swadgeItGameOver();
                    }
                });
            }
            else if (SI_MEMORY == si->screen)
            {
                // Run intro timer
                if (si->timeToNextEvent > 0)
                {
                    si->timeToNextEvent -= elapsedUs;
                }
                // If the queue was cleared
                else if (0 == si->inputQueue.length)
                {
                    // Enable speaker for a new verbal command and reset sample count
                    switchToSpeaker();
                    si->sampleIdx = 0;

                    // Add a new event to the memory queue
                    swadgeItEvt_t newEvt = esp_random() % MAX_NUM_EVTS;
                    push(&si->memoryQueue, (void*)newEvt);

                    // Copy the memory queue to the event and speech queues
                    node_t* evtNode = si->memoryQueue.first;
                    while (evtNode)
                    {
                        push(&si->inputQueue, evtNode->val);
                        push(&si->speechQueue, evtNode->val);
                        evtNode = evtNode->next;
                    }

                    // Set new LEDs
                    swadgeItUpdateDisplay();
                }
            }

            font_t* font = si->menuRenderer->menuFont;

            // Draw command
            int16_t tWidth = textWidth(font, si->dispEvt->label);
            drawText(font, si->dispEvt->txColor, si->dispEvt->label, (TFT_WIDTH - tWidth) / 2,
                     TFT_HEIGHT / 2 - font->height - 2);

            // Draw current score
            char scoreStr[32];
            snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, si->score);
            tWidth = textWidth(font, scoreStr);
            drawText(font, si->dispEvt->txColor, scoreStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2) + 2);

            break;
        }
        case SI_HIGH_SCORES:
        {
            // Draw high scores
            font_t* font = si->menuRenderer->menuFont;
            char hsString[64];

            // Draw reaction string
            snprintf(hsString, sizeof(hsString) - 1, "%s: %" PRId32, swadgeItStrReaction, si->reactionHighScore);
            int16_t tWidth = textWidth(font, hsString);
            drawText(font, c555, hsString, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2) - font->height);

            // Draw memory string
            snprintf(hsString, sizeof(hsString) - 1, "%s: %" PRId32, swadgeItStrMemory, si->memoryHighScore);
            tWidth = textWidth(font, hsString);
            drawText(font, c555, hsString, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2));

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, ARRAY_SIZE(leds));

            break;
        }
        case SI_GAME_OVER:
        {
            // Run timer to not exit high score too early
            if (si->gameOverTimer > 0)
            {
                si->gameOverTimer -= elapsedUs;
            }

            // Draw round score
            font_t* font = si->menuRenderer->menuFont;
            char gameOverStr[64];
            snprintf(gameOverStr, sizeof(gameOverStr) - 1, "Game Over: %" PRId32, si->score);
            int16_t tWidth = textWidth(font, gameOverStr);
            drawText(font, c555, gameOverStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - font->height) / 2);

            // Draw extra if it's a new high score
            if (si->newHighScore)
            {
                const char newHighScoreStr[] = "New High Score!";
                tWidth                       = textWidth(font, newHighScoreStr);
                drawText(font, c555, newHighScoreStr, (TFT_WIDTH - tWidth) / 2,
                         ((TFT_HEIGHT - font->height) / 2) + 2 + font->height);
            }

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, ARRAY_SIZE(leds));

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
        case SI_MEMORY:
        {
            // Draw background color for current command
            fillDisplayArea(x, y, x + w, y + h, si->dispEvt->bgColor);
            break;
        }
        case SI_HIGH_SCORES:
        {
            fillDisplayArea(x, y, x + w, y + h, c000);
            break;
        }
        case SI_GAME_OVER:
        {
            fillDisplayArea(x, y, x + w, y + h, c000);
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
    if (si->speechDelayUs <= 0 && !si->pendingSwitchToMic && si->speechQueue.length)
    {
        const rawSample_t* rs = &si->sfx[(swadgeItEvt_t)si->speechQueue.first->val];
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
                // If this is the last queued verbal events
                if (1 == si->speechQueue.length)
                {
                    // Raise flag to switch back to mic mode in the main loop
                    si->pendingSwitchToMic = true;
                }
                else
                {
                    si->speechDelayUs = 1000000;
                }
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
    // Event already cleared, return
    if (0 == si->inputQueue.length)
    {
        return;
    }

    // Ignore game input if not in a game mode
    if (!(SI_MEMORY == si->screen || SI_REACTION == si->screen))
    {
        return;
    }

    // If the input matches the current event
    if (evt == (swadgeItEvt_t)si->inputQueue.first->val)
    {
        // Remove first element
        shift(&si->inputQueue);

        // If the queue is empty
        if (0 == si->inputQueue.length)
        {
            // Increment the score
            si->score++;
        }

        // Set LEDs for new event
        swadgeItUpdateDisplay();
    }
    else if ((SI_REACTION == si->screen) || (SI_MEMORY == si->screen))
    {
        // Input event doesn't match
        swadgeItGameOver();
    }
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
            // Save to micFrameEnergyHistory and reset
            si->micSamplesProcessed = 0;
            push(&si->micFrameEnergyHistory, (void*)((intptr_t)si->micEnergy));
            while (si->micFrameEnergyHistory.length > MIC_ENERGY_HYSTERESIS)
            {
                shift(&si->micFrameEnergyHistory);
            }
            si->micEnergy = 0;
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
            int32_t tDelta      = delta.x + delta.y + delta.z;
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
    // Record high score
    si->newHighScore = false;
    if (SI_REACTION == si->screen && si->score > si->reactionHighScore)
    {
        si->reactionHighScore = si->score;
        writeNvs32(SI_REACTION_HS_KEY, si->score);
        si->newHighScore = true;
    }
    else if (SI_MEMORY == si->screen && si->score > si->memoryHighScore)
    {
        si->memoryHighScore = si->score;
        writeNvs32(SI_MEMORY_HS_KEY, si->score);
        si->newHighScore = true;
    }

    // Display round score
    si->screen        = SI_GAME_OVER;
    si->gameOverTimer = 1000000;
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
        if ((swadgeItStrReaction == label) || (swadgeItStrMemory == label))
        {
            si->screen = (swadgeItStrReaction == label) ? SI_REACTION : SI_MEMORY;

            si->dispEvt         = &siWaitData;
            si->timeToNextEvent = 2000000;
            si->nextEvtTimer    = 0;
            clear(&si->inputQueue);
            clear(&si->memoryQueue);
            clear(&si->speechQueue);
            swadgeItUpdateDisplay();
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

/**
 * @brief Set the LEDs according to the current event
 */
static void swadgeItUpdateDisplay(void)
{
    // If there's a speech event
    if (si->speechQueue.length)
    {
        // Display what is spoken
        si->dispEvt = &siEvtData[(swadgeItEvt_t)si->speechQueue.first->val];
    }
    // Otherwise if there's an input event
    else if (si->inputQueue.length)
    {
        if (SI_REACTION == si->screen)
        {
            // Reaction mode, show the current event
            si->dispEvt = &siEvtData[(swadgeItEvt_t)si->inputQueue.first->val];
        }
        else
        {
            // Memory mode, just show "Go"
            si->dispEvt = &siGoData;
        }
    }
    // Otherwise all inputs are finished and the score isn't zero
    else if (si->score)
    {
        si->dispEvt = &siGoodData;
    }

    // Copy to all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    for (int i = 0; i < ARRAY_SIZE(leds); i++)
    {
        memcpy(&leds[i], &si->dispEvt->ledColor, sizeof(led_t));
    }

    // Set LEDs
    setLeds(leds, ARRAY_SIZE(leds));
}

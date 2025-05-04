//==============================================================================
// Includes
//==============================================================================

#include "swadgeIt.h"
#include "mainMenu.h"
#include "heatshrink_helper.h"
#include "touchUtils.h"
#include "embeddedOut.h"

//==============================================================================
// Defines
//==============================================================================

#define SWADGE_IT_FPS 40

// TODO tune values
#define MIC_ENERGY_THRESHOLD  100000
#define MIC_ENERGY_HYSTERESIS 20

#define SHAKE_THRESHOLD  300
#define SHAKE_HYSTERESIS 10

#define TEXT_Y_SPACING 4

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
    // The current screen being displayed
    swadgeItScreen_t screen;

    // Menu and UI
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

    // Game over UI variables
    int32_t gameOverTimer;
    bool newHighScore;

    // Sound effects
    rawSample_t sfx[MAX_NUM_EVTS];
    int32_t sampleIdx;

    // Flag to switch from speaker to mic mode
    bool pendingSwitchToMic;

    // Gameplay variables
    int32_t timeToNextEvent;
    int32_t nextEvtTimer;
    list_t inputQueue;     // A queue of required inputs
    list_t memoryQueue;    // A growing queue of commands to memorize
    list_t speechQueue;    // A queue of verbal commands
    int32_t speechDelayUs; // Timer to pause between verbal commands
    int32_t score;
    const swadgeItEvtData_t* dispEvt; // The current event
    int32_t inputIdx;                 // The index of the current event, for memory mode

    // IMU Variables
    vec3d_t lastOrientation;
    list_t shakeHistory;
    bool isShook;

    // Microphone variables
    int32_t micSamplesProcessed;
    list_t micFrameEnergyHistory;
    bool isYelling;
    bool yellInput;
    dft32_data dd;       // Colorchord is used for spectral analysis
    embeddedNf_data end; // Colorchord is used for spectral analysis

    // Touchpad variables
    touchSpinState_t touchSpinState;

    // High scores from NVS
    int32_t memoryHighScore;
    int32_t reactionHighScore;
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

static void swadgeItCheckSpeech(int64_t elapsedUs);
static void swadgeItCheckInputs(void);
static void swadgeItGameplayLogic(int64_t elapsedUs);
static void swadgeItGameplayRender(void);

static void swadgeItHighScoreRender(void);
static void swadgeItGameOverRender(int64_t elapsedUs);

static void swadgeItSwitchToScreen(swadgeItScreen_t newScreen);
static void swadgeItClearMicState(void);

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

    // Load all SFX samples
    for (int8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        si->sfx[i].samples = readHeatshrinkFile(siEvtData[i].sfx_fname, &si->sfx[i].len, true);
    }

    // For yell detection
    InitColorChord(&si->end, &si->dd);

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

    // Free SFX
    for (int8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        heap_caps_free(si->sfx[i].samples);
    }

    // Clear queues
    clear(&si->inputQueue);
    clear(&si->memoryQueue);
    clear(&si->speechQueue);
    clear(&si->micFrameEnergyHistory);
    clear(&si->shakeHistory);

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
                // A button returns to the main menu
                if (evt.down)
                {
                    swadgeItSwitchToScreen(SI_MENU);
                }
                break;
            }
            case SI_GAME_OVER:
            {
                // A button returns to the main menu after the timer
                if (si->gameOverTimer <= 0 && evt.down)
                {
                    swadgeItSwitchToScreen(SI_MENU);
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
            swadgeItCheckSpeech(elapsedUs);
            swadgeItCheckInputs();
            swadgeItGameplayLogic(elapsedUs);
            swadgeItGameplayRender();
            break;
        }
        case SI_HIGH_SCORES:
        {
            swadgeItHighScoreRender();
            break;
        }
        case SI_GAME_OVER:
        {
            swadgeItGameOverRender(elapsedUs);
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
        case SI_GAME_OVER:
        {
            // Black background for these
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
    if (si->speechDelayUs <= 0 &&  // If the delay between verbal commands isn't running and
        !si->pendingSwitchToMic && // we aren't about to switch to the mic and
        si->speechQueue.length)    // There is something to say
    {
        // Get the raw samples
        const rawSample_t* rs = &si->sfx[(swadgeItEvt_t)si->speechQueue.first->val];
        if (rs->samples)
        {
            // Make sure we don't read out of bounds
            int16_t cpLen = len;
            if (si->sampleIdx + len > rs->len)
            {
                cpLen = (rs->len - si->sampleIdx);
            }

            // Copy samples out to the DAC
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
                    // Otherwise set a timer to pause between verbal commands
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
 * @brief This function manages verbal commands during gameplay.
 * It runs the timer between commands, deques commands when over, and switches from speaker to mic modes.
 *
 * @param elapsedUs The time elapsed since the last time this function was called
 */
static void swadgeItCheckSpeech(int64_t elapsedUs)
{
    // Check if the flag is raised to switch from speaker to microphone
    bool dequeueSpeech = false;
    if (si->pendingSwitchToMic)
    {
        // Then switch back to the microphone
        switchToMicrophone();

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

    // If speech should be dequeued now
    if (dequeueSpeech)
    {
        // Remove from the speech queue
        shift(&si->speechQueue);
        // Reset sample index
        si->sampleIdx = 0;

        // Set new LEDs
        swadgeItUpdateDisplay();
    }
}

/**
 * @brief Check for various Swadge It inputs including shaking, spinning, and yelling.
 * This doesn't check button inputs, which are handled in swadgeItMainLoop()
 */
static void swadgeItCheckInputs(void)
{
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

    // Check for yell input, done here rather than in swadgeItAudioCallback()
    if (si->yellInput)
    {
        si->yellInput = false;
        swadgeItInput(EVT_YELL_IT);
    }
}

/**
 * @brief Run gameplay logic for Swadge It. This includes both reaction and memory modes
 *
 * @param elapsedUs The time elapsed since the last time this function was called
 */
static void swadgeItGameplayLogic(int64_t elapsedUs)
{
    // Run game specific logic
    if (SI_REACTION == si->screen)
    {
        // For reaction mode, check if input was received before the gameplay timer elapsed
        RUN_TIMER_EVERY(si->nextEvtTimer, si->timeToNextEvent, elapsedUs, {
            // If there was successful event input (i.e. the queue was emptied)
            if (0 == si->inputQueue.length)
            {
                // Enable speaker for a new verbal command and reset sample count
                switchToSpeaker();
                si->sampleIdx = 0;

                // Reset mic values
                swadgeItClearMicState();

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

            // Reset mic values
            swadgeItClearMicState();

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
}

/**
 * @brief Render the TFT during gameplay. This draws the command, action count, and score.
 */
static void swadgeItGameplayRender(void)
{
    // Get a font, currently borrowed from the menu renderer
    font_t* font = si->menuRenderer->menuFont;

    // Center text vertically
    int numLines = 2;
    if (SI_MEMORY == si->screen && 0 != si->inputIdx)
    {
        numLines = 3;
    }
    int16_t yOff = (TFT_HEIGHT - (numLines * font->height + (numLines - 1) * TEXT_Y_SPACING)) / 2;

    // Draw command
    int16_t tWidth = textWidth(font, si->dispEvt->label);
    drawText(font, si->dispEvt->txColor, si->dispEvt->label, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += font->height + TEXT_Y_SPACING;

    // Draw action index for memory mode when there is one
    if (SI_MEMORY == si->screen && 0 != si->inputIdx)
    {
        char idxStr[32];
        snprintf(idxStr, sizeof(idxStr) - 1, "Action %" PRId32, si->inputIdx);
        tWidth = textWidth(font, idxStr);
        drawText(font, si->dispEvt->txColor, idxStr, (TFT_WIDTH - tWidth) / 2, yOff);
        yOff += font->height + TEXT_Y_SPACING;
    }

    // Draw current score
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "Score %" PRId32, si->score);
    tWidth = textWidth(font, scoreStr);
    drawText(font, si->dispEvt->txColor, scoreStr, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += font->height + TEXT_Y_SPACING;
}

/**
 * @brief Render the TFT during high score display. This draws the two high scores
 */
static void swadgeItHighScoreRender(void)
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
}

/**
 * @brief Render the TFT during game over. This draws the round's score
 *
 * @param elapsedUs The time elapsed since the last time this function was called
 */
static void swadgeItGameOverRender(int64_t elapsedUs)
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

        // Push to colorchord
        PushSample32(&si->dd, samp);

        // If enough samples have been processed
        si->micSamplesProcessed++;
        if (128 == si->micSamplesProcessed)
        {
            // Handle the frame
            si->micSamplesProcessed = 0;
            HandleFrameInfo(&si->end, &si->dd);

            // Sum total energy
            int32_t totalEnergy = 0;
            for (uint16_t i = 0; i < FIX_BINS; i++)
            {
                totalEnergy += si->end.fuzzed_bins[i];
            }

            // Add total energy to queue
            // TODO more efficient ringbuf?
            push(&si->micFrameEnergyHistory, (void*)((intptr_t)totalEnergy));
            if (si->micFrameEnergyHistory.length > MIC_ENERGY_HYSTERESIS)
            {
                shift(&si->micFrameEnergyHistory);
            }

            // Check for yelling and not yelling
            if (!si->isYelling)
            {
                // One sample is enough to yell
                if (totalEnergy > MIC_ENERGY_THRESHOLD)
                {
                    si->isYelling = true;
                    // Process the input on the main loop
                    si->yellInput = true;
                }
            }
            else // Is yelling, check for return to quiet
            {
                // Returning to quiet takes a few samples
                node_t* energyNode = si->micFrameEnergyHistory.first;
                while (energyNode)
                {
                    if ((intptr_t)energyNode->val > MIC_ENERGY_THRESHOLD)
                    {
                        // Still yelling
                        return;
                    }
                    energyNode = energyNode->next;
                }

                // Looped without returning, must not be yelling
                si->isYelling = false;
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
    // Get the raw IMU value
    if (ESP_OK == accelIntegrate())
    {
        vec3d_t orientation;
        if (ESP_OK == accelGetAccelVecRaw(&orientation.x, &orientation.y, &orientation.z))
        {
            // Get the difference between the last frame and now
            vec3d_t delta = {
                .x = ABS(si->lastOrientation.x - orientation.x),
                .y = ABS(si->lastOrientation.y - orientation.y),
                .z = ABS(si->lastOrientation.z - orientation.z),
            };
            si->lastOrientation = orientation;

            // Sum the deltas
            int32_t tDelta = delta.x + delta.y + delta.z;

            // Add the value to the history list
            push(&si->shakeHistory, (void*)((intptr_t)tDelta));
            if (si->shakeHistory.length > SHAKE_HYSTERESIS)
            {
                shift(&si->shakeHistory);
            }

            // If it's not shaking and over the threshold
            if (!si->isShook && tDelta > SHAKE_THRESHOLD)
            {
                // Mark it as shaking
                si->isShook = true;
                return true;
            }
            else if (si->isShook)
            {
                // If it's shaking, check for stability
                node_t* shakeNode = si->shakeHistory.first;
                while (shakeNode)
                {
                    if ((intptr_t)shakeNode->val > SHAKE_THRESHOLD)
                    {
                        // Shake in the history, return
                        return false;
                    }
                    shakeNode = shakeNode->next;
                }

                // No shake in the history, mark it as not shaking
                si->isShook = false;
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
    int32_t roundScore = si->score;
    swadgeItSwitchToScreen(SI_GAME_OVER);
    si->score         = roundScore;
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
            swadgeItSwitchToScreen((swadgeItStrReaction == label) ? SI_REACTION : SI_MEMORY);
            swadgeItUpdateDisplay();
        }
        else if (swadgeItStrHighScores == label)
        {
            swadgeItSwitchToScreen(SI_HIGH_SCORES);
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
    // Assume no input index
    si->inputIdx = 0;

    // If there's a speech event
    if (si->speechQueue.length)
    {
        // Display what is spoken
        si->dispEvt = &siEvtData[(swadgeItEvt_t)si->speechQueue.first->val];

        // For memory, get the current index
        if (SI_MEMORY == si->screen)
        {
            si->inputIdx = si->inputQueue.length - si->speechQueue.length + 1;
        }
    }
    // Otherwise if there's an input event
    else if (si->inputQueue.length)
    {
        if (SI_REACTION == si->screen)
        {
            // Reaction mode, show the current event
            si->dispEvt = &siEvtData[(swadgeItEvt_t)si->inputQueue.first->val];
        }
        else if (SI_MEMORY == si->screen)
        {
            // Memory mode, show "Go" and input index
            si->dispEvt  = &siGoData;
            si->inputIdx = si->memoryQueue.length - si->inputQueue.length + 1;
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

/**
 * @brief Clear transient internal state and switch to displaying a new screen
 *
 * @param newScreen The new screen to display
 */
static void swadgeItSwitchToScreen(swadgeItScreen_t newScreen)
{
    // Clear game over UI variables
    si->gameOverTimer = 0;
    si->newHighScore  = false;

    // Clear SFX & SPK variables
    si->sampleIdx          = 0;
    si->pendingSwitchToMic = false;

    // Clear gameplay variables
    si->timeToNextEvent = 2000000;
    si->nextEvtTimer    = 0;
    clear(&si->inputQueue);
    clear(&si->memoryQueue);
    clear(&si->speechQueue);
    si->speechDelayUs = 0;
    si->score         = 0;
    si->dispEvt       = &siWaitData;
    si->inputIdx      = 0;

    // Clear IMU variables
    memset(&si->lastOrientation, 0, sizeof(vec3d_t));
    clear(&si->shakeHistory);
    si->isShook = false;

    // Clear microphone variables
    swadgeItClearMicState();

    // Clear touchpad variables
    memset(&si->touchSpinState, 0, sizeof(touchSpinState_t));

    // Set the new screen
    si->screen = newScreen;
}

/**
 * @brief Helper function to clear microphone state
 */
static void swadgeItClearMicState(void)
{
    si->micSamplesProcessed = 0;
    clear(&si->micFrameEnergyHistory);
    si->isYelling = false;
    si->yellInput = false;
    InitColorChord(&si->end, &si->dd);
}

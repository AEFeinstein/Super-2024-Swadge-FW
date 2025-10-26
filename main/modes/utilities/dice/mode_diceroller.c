//==============================================================================
// Includes
//==============================================================================

#include "mode_diceroller.h"
#include "portableDance.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define DR_MAX_HIST 6
#define MAX_DICE    6

#define EYES_SLOT_DEAD   3
#define EYES_SLOT_SWIRL  4 ///< Starting slot for 4-frame swirl animation
#define EYES_SLOT_DIGITS 10

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    DR_STARTUP   = 0,
    DR_SHOW_ROLL = 1,
    DR_ROLLING   = 2
} dr_stateVals;

typedef enum
{
    DR_INPUT_DICE,
    DR_INPUT_COUNT,
    DR_INPUT_KEEP,
    DR_INPUT_MAX
} dr_inputSel;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t x;
    int32_t y;
} vector_t;

typedef struct
{
    int numFaces;  ///< The number of faces for this die
    int polyEdges; ///< The number of polygon edges to draw for this die
} die_t;

typedef struct
{
    int total; ///< The sum of these rolls
    die_t die; ///< The die used for these rolls
    int count; ///< The number of die rolled
    int keep;  ///< The number of die kept
} rollHistoryEntry_t;

typedef struct
{
    uint8_t pixels[36];
} eyeDigit_t;

typedef struct
{
    // UI variables
    font_t* ibm_vga8;
    wsg_t woodTexture;
    wsg_t cursor;
    wsg_t corner;

    // State machine variable
    int state;

    // Input selection
    int requestCount;
    int requestDieIdx;
    int requestKeep;
    dr_inputSel inputSelection;

    // Animation Timers
    int32_t rollRotAnimTimerUs;
    int32_t rollNumAnimTimerUs;

    // Current roll data
    rollHistoryEntry_t cRoll;
    int* rolls;
    int* fakeVals;

    // History of rolls
    list_t history;

    // IMU variables
    vec3d_t lastOrientation;
    list_t shakeHistory;
    bool isShook;
    int32_t noShakeTimer;

    // LED Variables
    portableDance_t* pDance;

    // Eye variables
    eyeDigit_t eyeDigits[10];
    uint8_t rollEyeFrame;
    int32_t rollEyeAnimTimerUs;
    bool initialEyes;

    // DAC variables
    int32_t cScalePeriodSamples[8];
    int32_t dacPeriodSample;
    int32_t dacPeriodSampleChangeTimer;
    int32_t sampleCount;
    bool dacLow;
} diceRoller_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void diceEnterMode(void);
void diceExitMode(void);
void diceMainLoop(int64_t elapsedUs);
void diceButtonCb(buttonEvt_t* evt);
void diceDacCallback(uint8_t* samples, int16_t len);

void getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius, vector_t* vertices);
void drawRegularPolygon(int xCenter, int yCenter, int8_t sides, float rotDeg, int16_t radius, paletteColor_t col,
                        int dashWidth);
void changeInputSelection(int change);
void changeDiceCountRequest(int change);
void changeDiceSidesRequest(int change);
void changeDiceKeepRequest(int change);
void doRoll(int count, const die_t* die, int keep);
void doStateMachine(int64_t elapsedUs);

void drawSelectionText(void);
void drawDiceText(int* diceVals);
void drawDiceBackground(int rotationOffsetDeg);
void genFakeVals(void);

void drawCurrentTotal(void);

void drawHistory(void);
void addTotalToHistory(void);

void drawPanel(int x0, int y0, int x1, int y1);
void drawHistoryPanel(void);

void drawBackgroundTable(void);

float cosDeg(float degrees);
float sinDeg(float degrees);
int intComparator(const void* a, const void* b);

void diceTrophyEval(void);

//==============================================================================
// Const Variables
//==============================================================================

static const die_t dice[] = {
    {.numFaces = 2, .polyEdges = 10}, {.numFaces = 4, .polyEdges = 3},   {.numFaces = 6, .polyEdges = 4},
    {.numFaces = 8, .polyEdges = 6},  {.numFaces = 10, .polyEdges = 4},  {.numFaces = 12, .polyEdges = 10},
    {.numFaces = 20, .polyEdges = 6}, {.numFaces = 100, .polyEdges = 6},
};

static const int32_t rollAnimationPeriod = 1000000; // 1 Second Spin
static const int32_t fakeValRerollPeriod = 100000;  // Change numbers every 0.1s
static const int32_t eyeFramePeriod      = 250000;  // Change swirl eye frame every 0.25s

static const char DR_NAMESTRING[] = "Dice Roller";

static const paletteColor_t diceBackgroundColor = c112;
static const paletteColor_t diceTextColor       = c550;
static const paletteColor_t diceTextColorNoKeep = c220;
static const paletteColor_t selectionTextColor  = c555;
static const paletteColor_t diceOutlineColor    = c223;
static const paletteColor_t totalTextColor      = c555;
static const paletteColor_t histTextColor       = c444;
// Panel colors
static const paletteColor_t outerGold  = c550;
static const paletteColor_t innerGold  = c540;
static const paletteColor_t panelColor = c400;
static const paletteColor_t textColor  = c555;

static const int xGridOffset    = TFT_WIDTH / 8;
static const int xGridMargin    = TFT_WIDTH / 5;
static const int yGridMargin    = TFT_HEIGHT / 7;
static const int xGridOffsets[] = {
    TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 + xGridOffset,               //
    TFT_WIDTH / 2 + xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 + xGridOffset,               //
    TFT_WIDTH / 2 + xGridMargin + xGridOffset,
};
static const int yGridOffsets[] = {
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin,
};

static float cScaleFrequencies[] = {
    261.63f, 293.66f, 329.63f, 349.23f, 392.0f, 440.0f, 493.88f, 523.25f,
};

//==============================================================================
// Variables
//==============================================================================

const trophyData_t diceTrophyList[] = {
    {
        .title       = "Nice.",
        .description = "Get the funny number",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
        .hidden      = true,
    },
    {
        .title       = "With advantage!",
        .description = "Have less keeps than rolls",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = false,
    },
    {
        .title       = "Where's the die jail?",
        .description = "Get two natural 1's in a row on a d20",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
        .hidden      = false,
    },
    {
        .title       = "Roll for initiative",
        .description = "Roll a single D20",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = false,
    },
    {
        .title       = "AAAAAAAHHhhhhHHHHhhhHHH!",
        .description = "Roll by shaking the swadge",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = false,
    },
    {
        .title       = "The Marketplace has real dice, you know?",
        .description = "Roll 2000 virtual dice",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 2000,
        .hidden      = false,
    },
    {
        .title       = "Yahtzee!",
        .description = "Get five of a kind when rolling 5d6",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
        .hidden      = false,
    },
};

const trophySettings_t diceTrophySettings = {
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 6,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = DR_NAMESTRING,
};

const trophyDataList_t diceTrophyData = {
    .settings = &diceTrophySettings,
    .list     = diceTrophyList,
    .length   = ARRAY_SIZE(diceTrophyList),
};

swadgeMode_t modeDiceRoller = {
    .modeName                 = DR_NAMESTRING,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = diceEnterMode,
    .fnExitMode               = diceExitMode,
    .fnMainLoop               = diceMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = diceDacCallback,
    .trophyData               = &diceTrophyData,
};

diceRoller_t* diceRoller;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter and set up the dice roller
 */
void diceEnterMode(void)
{
    diceRoller = heap_caps_calloc(1, sizeof(diceRoller_t), MALLOC_CAP_8BIT);

    diceRoller->ibm_vga8 = getSysFont();
    loadWsg(WOOD_TEXTURE_64_WSG, &diceRoller->woodTexture, false);
    loadWsg(UP_CURSOR_8_WSG, &diceRoller->cursor, false);
    loadWsg(GOLD_CORNER_TR_WSG, &diceRoller->corner, false);

    cnfsFileIdx_t digitGsFiles[] = {DICE_0_GS, DICE_1_GS, DICE_2_GS, DICE_3_GS, DICE_4_GS,
                                    DICE_5_GS, DICE_6_GS, DICE_7_GS, DICE_8_GS, DICE_9_GS};
    for (int i = 0; i < 10; i++)
    {
        size_t size        = 0;
        const uint8_t* buf = cnfsGetFile(digitGsFiles[i], &size);
        if (size != 40) // 4-byte header + 6x6
        {
            ESP_LOGW("DICE", "Eye digit asset %d wrong size (%d) bytes.\n", i, (int)size);
        }
        else
        {
            memcpy(&diceRoller->eyeDigits[i], buf + 4, ARRAY_SIZE(diceRoller->eyeDigits[i].pixels));
        }
    }

    ch32v003WriteBitmapAsset(EYES_SLOT_DEAD, EYES_DEAD_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 0, EYES_SWIRL_0_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 1, EYES_SWIRL_1_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 2, EYES_SWIRL_2_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 3, EYES_SWIRL_3_GS);

    memset(&diceRoller->cRoll, 0, sizeof(rollHistoryEntry_t));

    diceRoller->requestCount  = 1;
    diceRoller->requestDieIdx = 6;
    diceRoller->requestKeep   = diceRoller->requestCount;

    diceRoller->inputSelection = DR_INPUT_COUNT;

    diceRoller->state = DR_STARTUP;

    clear(&diceRoller->history);

    // Init LED animations
    diceRoller->pDance = initPortableDance(NULL);
    portableDanceSetByName(diceRoller->pDance, "Rainbow Fast");

    // Turn LEDs off
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Set timer to not accept motion right after start
    diceRoller->noShakeTimer = 1000000;

    // Calculate sample periods
    for (int i = 0; i < ARRAY_SIZE(diceRoller->cScalePeriodSamples); i++)
    {
        diceRoller->cScalePeriodSamples[i] = (uint32_t)roundf(DAC_SAMPLE_RATE_HZ / cScaleFrequencies[i]);
    }
    diceRoller->dacPeriodSample = diceRoller->cScalePeriodSamples[0];
}

/**
 * @brief Free dice roller memory and exit
 */
void diceExitMode(void)
{
    freeWsg(&diceRoller->woodTexture);
    freeWsg(&diceRoller->cursor);
    freeWsg(&diceRoller->corner);

    if (diceRoller->rolls != NULL)
    {
        heap_caps_free(diceRoller->rolls);
    }
    if (diceRoller->fakeVals != NULL)
    {
        heap_caps_free(diceRoller->fakeVals);
    }

    // Clear history
    while (diceRoller->history.length)
    {
        heap_caps_free(pop(&diceRoller->history));
    }

    freePortableDance(diceRoller->pDance);

    heap_caps_free(diceRoller);
}

/**
 * @brief Dice roller main loop, handle buttons, state machine, and animation
 *
 * @param elapsedUs The time since this function was last called
 */
void diceMainLoop(int64_t elapsedUs)
{
    // Handle buttons
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        diceButtonCb(&evt);
    }

    // Timer to not accept shakes immediately after start
    if (diceRoller->noShakeTimer > 0)
    {
        diceRoller->noShakeTimer -= elapsedUs;
    }

    // Handle motion
    if (checkForShake(&diceRoller->lastOrientation, &diceRoller->shakeHistory, &diceRoller->isShook))
    {
        if ((diceRoller->noShakeTimer <= 0) && (diceRoller->isShook))
        {
            doRoll(diceRoller->requestCount, &dice[diceRoller->requestDieIdx], diceRoller->requestKeep);
            trophyUpdate(&diceTrophyList[4], 1, true);
        }
    }

    // Handle logic
    doStateMachine(elapsedUs);
}

/**
 * @brief Handle a dice roller button event. Moves cursor and rolls dice
 *
 * @param evt The button event
 */
void diceButtonCb(buttonEvt_t* evt)
{
    switch (evt->button)
    {
        case PB_A:
        {
            if (evt->down)
            {
                doRoll(diceRoller->requestCount, &dice[diceRoller->requestDieIdx], diceRoller->requestKeep);
            }
            break;
        }
        case PB_UP:
        case PB_DOWN:
        {
            int dir = (evt->button == PB_UP) ? 1 : -1;
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                switch (diceRoller->inputSelection)
                {
                    case DR_INPUT_DICE:
                    default:
                    {
                        changeDiceSidesRequest(dir);
                        break;
                    }
                    case DR_INPUT_COUNT:
                    {
                        changeDiceCountRequest(dir);
                        break;
                    }
                    case DR_INPUT_KEEP:
                    {
                        changeDiceKeepRequest(dir);
                        break;
                    }
                }
            }
            break;
        }
        case PB_LEFT:
        case PB_RIGHT:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                changeInputSelection((evt->button == PB_LEFT) ? 1 : -1);
            }
            break;
        }
        case PB_B:
        {
            switchToSwadgeMode(&mainMenuMode);
        }
        default:
        {
            // Don't roll unless you press A
        }
    }
}

/**
 * @brief Draws a tiled wood texture on the entire background
 *
 * This could be a background callback, but it works fine as-is and this mode doesn't need to be super performant
 */
void drawBackgroundTable(void)
{
    for (int x = 0; x < TFT_WIDTH; x += diceRoller->woodTexture.w)
    {
        for (int y = 0; y < TFT_HEIGHT; y += diceRoller->woodTexture.h)
        {
            drawWsgTile(&diceRoller->woodTexture, x, y);
        }
    }
}

/**
 * @brief Main state machine for the dice roller. Handles rolling, drawing, and animation
 *
 * @param elapsedUs The time since this function was last called
 */
void doStateMachine(int64_t elapsedUs)
{
    // Always draw the background table first
    drawBackgroundTable();

    // Always draw the selection text
    drawSelectionText();

    switch (diceRoller->state)
    {
        case DR_STARTUP:
        default:
        {
            // Draw the mode name
            drawText(diceRoller->ibm_vga8, textColor, DR_NAMESTRING,
                     TFT_WIDTH / 2 - textWidth(diceRoller->ibm_vga8, DR_NAMESTRING) / 2, TFT_HEIGHT / 2);
            if (!diceRoller->initialEyes)
            {
                diceRoller->initialEyes = true;
                ch32v003SelectBitmap(EYES_SLOT_SWIRL + 3);
            }
            break;
        }
        case DR_SHOW_ROLL:
        {
            // Draw everything
            drawDiceBackground(0);
            drawDiceText(diceRoller->rolls);
            drawCurrentTotal();
            drawHistoryPanel();
            drawHistory();
            break;
        }
        case DR_ROLLING:
        {
            // Animate LEDs when rolling
            portableDanceMainLoop(diceRoller->pDance, elapsedUs);

            // Run animation timer
            diceRoller->rollRotAnimTimerUs += elapsedUs;

            // Run a timer to generate 'fake' numbers
            RUN_TIMER_EVERY(diceRoller->rollNumAnimTimerUs, fakeValRerollPeriod, elapsedUs, { genFakeVals(); });

            // Run timer for SFX
            RUN_TIMER_EVERY(diceRoller->dacPeriodSampleChangeTimer, rollAnimationPeriod / 6, elapsedUs, {
                // Pick a new, random value from cScalePeriodSamples
                int32_t newPeriod = diceRoller->dacPeriodSample;
                while (newPeriod == diceRoller->dacPeriodSample)
                {
                    newPeriod
                        = diceRoller->cScalePeriodSamples[esp_random() % ARRAY_SIZE(diceRoller->cScalePeriodSamples)];
                }
                diceRoller->dacPeriodSample = newPeriod;
            });

            // Run timer for eye animation
            RUN_TIMER_EVERY(diceRoller->rollEyeAnimTimerUs, eyeFramePeriod, elapsedUs, {
                diceRoller->rollEyeFrame = (diceRoller->rollEyeFrame + 1) % 4;
                ch32v003SelectBitmap(EYES_SLOT_SWIRL + diceRoller->rollEyeFrame);
            });

            // Draw everything
            drawDiceBackground((diceRoller->rollRotAnimTimerUs * 360) / rollAnimationPeriod);
            drawDiceText(diceRoller->fakeVals);
            drawHistoryPanel();
            drawHistory();

            // If the roll elapsed
            if (!diceRoller->isShook && diceRoller->rollRotAnimTimerUs > rollAnimationPeriod)
            {
                // Add to history and change the state
                addTotalToHistory();
                diceRoller->state = DR_SHOW_ROLL;

                // Turn LEDs off
                led_t leds[CONFIG_NUM_LEDS] = {0};
                setLeds(leds, CONFIG_NUM_LEDS);

                // Set eyes
                if (diceRoller->cRoll.total < 100)
                {
                    uint8_t bitmap[6][12] = {0};
                    eyeDigit_t* digits[2] = {&diceRoller->eyeDigits[diceRoller->cRoll.total / 10],
                                             &diceRoller->eyeDigits[diceRoller->cRoll.total % 10]};
                    for (int i = 0; i < 2; i++)
                    {
                        for (int x = 0; x < 6; x++)
                        {
                            for (int y = 0; y < 6; y++)
                            {
                                bitmap[y][x + 6 * i] = digits[i]->pixels[x + y * 6];
                            }
                        }
                    }
                    ch32v003WriteBitmap(EYES_SLOT_DIGITS, bitmap);
                    ch32v003SelectBitmap(EYES_SLOT_DIGITS);
                }
                else
                {
                    ch32v003SelectBitmap(EYES_SLOT_DEAD);
                }
            }
            break;
        }
    }
}

/**
 * @brief Draw a panel with a filled background and border
 *
 * @param x0 The X offset to start the panel at
 * @param y0 The Y offset to start the panel at
 * @param x1 The X offset to end the panel at
 * @param y1 The Y offset to end the panel at
 */
void drawPanel(int x0, int y0, int x1, int y1)
{
    // Draw the panel
    drawRect(x0, y0, x1, y1, outerGold);
    drawRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, innerGold);
    oddEvenFill(x0, y0, x1, y1, innerGold, panelColor);

    // Draw corners around the panel
    int cornerEdge = 8;
    drawWsg(&diceRoller->corner, x0, y0, true, false, 0);                           // Draw TopLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y0, false, false, 0);             // Draw TopRight
    drawWsg(&diceRoller->corner, x0, y1 - cornerEdge, true, true, 0);               // Draw BotLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y1 - cornerEdge, false, true, 0); // Draw BotRight
}

/**
 * @brief Draw the panel for dice history
 */
void drawHistoryPanel(void)
{
    // Calculate margins
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;
    int xMargin          = 45;
    int yMargin          = 10;

    // Draw the panel
    drawPanel(histX - xMargin, //
              histY - yMargin, //
              histX + xMargin, //
              histY + (DR_MAX_HIST + 1) * histYEntryOffset + yMargin);
}

/**
 * @brief Draw dice history to the TFT
 */
void drawHistory(void)
{
    // Calculate offsets
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;

    // Draw the header
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "History");
    drawText(diceRoller->ibm_vga8, totalTextColor, totalStr,        //
             histX - textWidth(diceRoller->ibm_vga8, totalStr) / 2, //
             histY);

    // For all the history
    node_t* histNode = diceRoller->history.first;
    int i            = 0;
    while (histNode)
    {
        rollHistoryEntry_t* entry = histNode->val;

        // Draw this history entry centered around the colon
        snprintf(totalStr, sizeof(totalStr), "%dd%dk%d:", entry->count, entry->die.numFaces, entry->keep);
        drawText(diceRoller->ibm_vga8, histTextColor, totalStr, histX + 14 - textWidth(diceRoller->ibm_vga8, totalStr),
                 histY + (i + 1) * histYEntryOffset);
        snprintf(totalStr, sizeof(totalStr), "%d", entry->total);
        drawText(diceRoller->ibm_vga8, c555, totalStr, histX + 17, histY + (i + 1) * histYEntryOffset);

        // Iterate
        histNode = histNode->next;
        i++;
    }
}

/**
 * @brief Add the last dice roll to the history
 */
void addTotalToHistory(void)
{
    // Make a new entry, copy current stats, and add it to history
    rollHistoryEntry_t* entry = heap_caps_calloc(1, sizeof(rollHistoryEntry_t), MALLOC_CAP_8BIT);
    memcpy(entry, &diceRoller->cRoll, sizeof(rollHistoryEntry_t));
    unshift(&diceRoller->history, entry);

    // If history is too long, remove old elements
    while (diceRoller->history.length > DR_MAX_HIST)
    {
        heap_caps_free(pop(&diceRoller->history));
    }
}

/**
 * @brief Draw the current dice roll total
 */
void drawCurrentTotal(void)
{
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "Total: %d", diceRoller->cRoll.total);
    drawText(diceRoller->ibm_vga8, totalTextColor, totalStr,
             TFT_WIDTH / 2 - textWidth(diceRoller->ibm_vga8, totalStr) / 2, TFT_HEIGHT * 7 / 8);
}

/**
 * @brief Change the cursor selection between number of dice and number of faces
 *
 * @param change +1 to go forward or -1 to go backwards
 */
void changeInputSelection(int change)
{
    diceRoller->inputSelection = (diceRoller->inputSelection + change + DR_INPUT_MAX) % DR_INPUT_MAX;
}

/**
 * @brief Change the number of dice to roll
 *
 * Never less than 1 except at mode start, never greater than MAX_DICE
 *
 * @param change +1 to go forward or -1 to go backwards
 */
void changeDiceCountRequest(int change)
{
    bool countEqualsKeep = (diceRoller->requestCount == diceRoller->requestKeep);

    // Change the request
    diceRoller->requestCount = (diceRoller->requestCount - 1 + change + MAX_DICE) % MAX_DICE + 1;

    // Make sure we're not keeping more dice than rolled
    if (diceRoller->requestKeep > diceRoller->requestCount)
    {
        diceRoller->requestKeep = diceRoller->requestCount;
    }

    // If the keep was equal to the request before, keep it synced
    if (countEqualsKeep)
    {
        diceRoller->requestKeep = diceRoller->requestCount;
    }
}

/**
 * @brief Change the number of dice to keep
 *
 * Never less than 1 except at mode start, never greater than diceRoller->requestCount
 *
 * @param change +1 to go forward or -1 to go backwards
 */
void changeDiceKeepRequest(int change)
{
    diceRoller->requestKeep
        = ((diceRoller->requestKeep - 1 + change + diceRoller->requestCount) % diceRoller->requestCount) + 1;
}

/**
 * @brief Change the number of faces of the dice to roll
 *
 * @param change +1 to go forward or -1 to go backwards
 */
void changeDiceSidesRequest(int change)
{
    diceRoller->requestDieIdx = (diceRoller->requestDieIdx + change + ARRAY_SIZE(dice)) % ARRAY_SIZE(dice);
}

/**
 * @brief Compare two ints
 *
 * @param a A pointer to an int
 * @param b A pointer to another int
 * @return Greater than zero if b>a, less than zero if b<a, or zero if b==a
 */
int intComparator(const void* a, const void* b)
{
    return *((const int*)b) - *((const int*)a);
}

/**
 * @brief Roll the dice
 *
 * @param count The number of dice to roll
 * @param die The type of die to roll
 * @param keep The number of die to keep
 */
void doRoll(int count, const die_t* die, int keep)
{
    // Check if it's already rolling
    if (diceRoller->state == DR_ROLLING)
    {
        return;
    }

    // Validate input
    if (0 >= count || NULL == die)
    {
        return;
    }

    // Reallocate rolls because the number of rolls may have changed
    if (diceRoller->rolls)
    {
        heap_caps_free(diceRoller->rolls);
    }
    diceRoller->rolls = (int*)heap_caps_calloc(count, sizeof(int), MALLOC_CAP_8BIT);

    // Make space for fake rolls too
    if (diceRoller->fakeVals)
    {
        heap_caps_free(diceRoller->fakeVals);
    }
    diceRoller->fakeVals = (int*)heap_caps_calloc(count, sizeof(int), MALLOC_CAP_8BIT);

    // Start timers fresh
    diceRoller->rollRotAnimTimerUs = 0;
    diceRoller->rollNumAnimTimerUs = 0;
    diceRoller->rollEyeAnimTimerUs = 0;

    // Roll the dice!
    for (int m = 0; m < count; m++)
    {
        int curVal           = (esp_random() % die->numFaces) + 1;
        diceRoller->rolls[m] = curVal;
    }

    // Sort the die rolls
    qsort(diceRoller->rolls, count, sizeof(int), intComparator);

    // Calculate the total of kept die
    int total = 0;
    for (int m = 0; m < keep; m++)
    {
        total += diceRoller->rolls[m];
    }

    // Save the roll
    diceRoller->cRoll.count = count;
    diceRoller->cRoll.die   = *die;
    diceRoller->cRoll.total = total;
    diceRoller->cRoll.keep  = keep;

    // Generate fake values to start
    genFakeVals();

    // Set the state to rolling
    diceRoller->state = DR_ROLLING;

    // Reset SFX timer
    diceRoller->dacPeriodSampleChangeTimer = 0;
    diceRoller->sampleCount                = 0;
    diceRoller->dacLow                     = false;

    // Set swirly eyes
    ch32v003SelectBitmap(EYES_SLOT_SWIRL);

    // Check trophies
    diceTrophyEval();
}

/**
 * @brief Helper function to get cosine in degrees
 *
 * @param degrees The number, in degrees, to get the cosine of
 * @return The cosine of the input
 */
float cosDeg(float degrees)
{
    return cosf(degrees / 360.0f * 2 * M_PI);
}

/**
 * @brief Helper function to get sine in degrees
 *
 * @param degrees The number, in degrees, to get the sine of
 * @return The sine of the input
 */
float sinDeg(float degrees)
{
    return sinf(degrees / 360.0f * 2 * M_PI);
}

/**
 * @brief Get the Regular Polygon Vertices object. Used in drawRegularPolygon(). The vector array returned
 * by this function must be freed to prevent memory leaks.
 *
 * @param sides Number of sides of regular polygon
 * @param rotDeg rotation of regular polygon clockwise in degrees (first point is draw pointing to the right)
 * @param radius Radius in pixels on which vertices will be placed.
 * @param vertices [OUT] where the vertices are written, an array of (x,y) coordinates in pixels centered at (0,0) of
 * length sides.
 */
void getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius, vector_t* vertices)
{
    float increment = 360.0f / sides;
    for (int k = 0; k < sides; k++)
    {
        vertices[k].x = round(radius * cosDeg(increment * k + rotDeg));
        vertices[k].y = round(radius * sinDeg(increment * k + rotDeg));
    }
}

/**
 * @brief Draw a regular polygon given a center point, number of sides, rotation, radius, outline color, and dash width.
 * WARNING: Lines are not guaranteed to be exactly one pixel thick due to behavior of the plotLine function.
 *
 * @param xCenter x axis center of polygon
 * @param yCenter y axis center of polygon
 * @param sides number of sides of polygon
 * @param rotDeg clockwise degrees of rotation
 * @param radius radius on which vertices will be placed from center
 * @param col color of outline
 * @param dashWidth dotted line behavior. 0 for solid line.
 */
void drawRegularPolygon(int xCenter, int yCenter, int8_t sides, float rotDeg, int16_t radius, paletteColor_t col,
                        int dashWidth)
{
    // For each vertex in the polygon
    vector_t vertices[sides];
    getRegularPolygonVertices(sides, rotDeg, radius, vertices);
    for (int vertInd = 0; vertInd < sides; vertInd++)
    {
        // Find the next vertex, may wrap around
        int8_t endInd = (vertInd + 1) % sides;

        // Draw a line
        drawLine(xCenter + vertices[vertInd].x, yCenter + vertices[vertInd].y, xCenter + vertices[endInd].x,
                 yCenter + vertices[endInd].y, col, dashWidth);
    }
}

/**
 * @brief Draw text for what the current selection is. This also draws the cursor.
 */
void drawSelectionText(void)
{
    font_t* font = diceRoller->ibm_vga8;

    // Create the whole string, measuring as we go
    char rollStr[32] = "Next roll is ";
    char tmpStr[32];

    int countStart = textWidth(font, rollStr);

    snprintf(tmpStr, sizeof(tmpStr) - 1, "%d", diceRoller->requestCount);
    strncat(rollStr, tmpStr, sizeof(rollStr) - strlen(rollStr) - 1);

    int diceStart = textWidth(font, rollStr);

    snprintf(tmpStr, sizeof(tmpStr) - 1, "d%d", dice[diceRoller->requestDieIdx].numFaces);
    strncat(rollStr, tmpStr, sizeof(rollStr) - strlen(rollStr) - 1);

    int keepStart = textWidth(font, rollStr);

    snprintf(tmpStr, sizeof(tmpStr) - 1, "k%d", diceRoller->requestKeep);
    strncat(rollStr, tmpStr, sizeof(rollStr) - strlen(rollStr) - 1);

    int keepEnd = textWidth(font, rollStr);

    // Draw the string it to the screen
    int xOffset = (TFT_WIDTH - textWidth(font, rollStr)) / 2;
    int yOffset = TFT_HEIGHT / 8;
    drawText(font, selectionTextColor, rollStr, xOffset, yOffset);

    // Figure out where to draw the cursor
    switch (diceRoller->inputSelection)
    {
        default:
        case DR_INPUT_COUNT:
        {
            xOffset += countStart + (diceStart - countStart) / 2;
            break;
        }
        case DR_INPUT_DICE:
        {
            xOffset += diceStart + (keepStart - diceStart) / 2;
            break;
        }
        case DR_INPUT_KEEP:
        {
            xOffset += keepStart + (keepEnd - keepStart) / 2;
            break;
        }
    }
    // Center the cursor
    xOffset -= (diceRoller->cursor.w / 2);

    // Draw the cursor
    drawWsgSimple(&diceRoller->cursor, xOffset, yOffset + font->height + 4);
}

/**
 * @brief Draw dice backgrounds, which are rotated, outlined, filled polygons
 *
 * @param rotationOffsetDeg The rotation to apply, in degrees
 */
void drawDiceBackground(int rotationOffsetDeg)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->cRoll.count; m++)
    {
        // Draw the polygon outline
        drawRegularPolygon(xGridOffsets[m], yGridOffsets[m] + 5, diceRoller->cRoll.die.polyEdges,
                           -90 + rotationOffsetDeg, 20, diceOutlineColor, 0);

        // Fill the polygon
        int oERadius = 23;
        oddEvenFill(xGridOffsets[m] - oERadius, yGridOffsets[m] - oERadius + 5, xGridOffsets[m] + oERadius,
                    yGridOffsets[m] + oERadius + 5, diceOutlineColor, diceBackgroundColor);
    }
}

/**
 * @brief Draw the text for the dice. This should be done after drawDiceBackground()
 */
void drawDiceText(int* diceVals)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->cRoll.count; m++)
    {
        // Convert integer to string
        char rollOutcome[32];
        snprintf(rollOutcome, sizeof(rollOutcome), "%d", diceVals[m]);

        paletteColor_t color = diceTextColor;
        if (m >= diceRoller->cRoll.keep && diceRoller->state != DR_ROLLING)
        {
            color = diceTextColorNoKeep;
        }
        // Draw the text
        drawText(diceRoller->ibm_vga8, color, rollOutcome,
                 xGridOffsets[m] - textWidth(diceRoller->ibm_vga8, rollOutcome) / 2, yGridOffsets[m]);
    }
}

/**
 * @brief Generate 'fake' values to display on die while rollingZ
 */
void genFakeVals(void)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->cRoll.count; m++)
    {
        // Pick a random number
        diceRoller->fakeVals[m] = esp_random() % diceRoller->cRoll.die.numFaces + 1;
    }
}

/**
 * @brief A callback which requests DAC samples from the application
 *
 * @param samples A buffer to fill with 8 bit unsigned DAC samples
 * @param len The length of the buffer to fill
 */
void diceDacCallback(uint8_t* samples, int16_t len)
{
    // If the dice are rolling
    if (diceRoller->state == DR_ROLLING)
    {
        // Output a square wave
        while (len--)
        {
            // Write a square wave sample
            *samples = diceRoller->dacLow ? (128 - 64) : (128 + 64);
            samples++;

            // Increment sample count and check if the wave should flip
            diceRoller->sampleCount++;
            if (!diceRoller->dacLow && diceRoller->sampleCount >= diceRoller->dacPeriodSample / 2)
            {
                // Halfway finished, set the wave low
                diceRoller->dacLow = true;
            }
            else if (diceRoller->sampleCount >= diceRoller->dacPeriodSample)
            {
                // All done, set the wave high
                diceRoller->dacLow      = false;
                diceRoller->sampleCount = 0;
            }
        }
    }
    else
    {
        // Not rolling, zero the output
        memset(samples, 128, len);
    }
}

void diceTrophyEval(void)
{
    // Check if the player got 69
    if (diceRoller->cRoll.total == 69)
    {
        trophyUpdate(&diceTrophyList[0], 1, true);
    }

    // Check if 'keeps' mechanic is being used
    if (diceRoller->cRoll.keep < diceRoller->cRoll.count)
    {
        trophyUpdate(&diceTrophyList[1], 1, true);
    }

    // If two nat 1s in a row
    if (diceRoller->cRoll.die.numFaces == 20 && diceRoller->cRoll.count == 1 && diceRoller->cRoll.total == 1)
    {
        node_t* histNode = diceRoller->history.first;
        if (histNode != NULL)
        {
            rollHistoryEntry_t* entry = histNode->val;
            if (entry->total == 1)
            {
                trophyUpdate(&diceTrophyList[2], 1, true);
            }
        }
    }

    // If a single D20 for the first time
    if (diceRoller->cRoll.die.numFaces == 20 && diceRoller->cRoll.count == 1)
    {
        trophyUpdate(&diceTrophyList[3], 1, true);
    }

    // If yahtzee
    if (diceRoller->cRoll.count == 5 && diceRoller->cRoll.die.numFaces == 6)
    {
        // Check if all rolls are equal
        int val   = diceRoller->rolls[0];
        bool same = true;
        for (int i = 1; i < diceRoller->cRoll.count; i++)
        {
            if (val != diceRoller->rolls[i])
            {
                same = false;
                break;
            }
        }
        if (same)
        {
            trophyUpdate(&diceTrophyList[6], 1, true);
        }
    }

    // Save rolled dice
    int totalDiceRolled = trophyGetSavedValue(&diceTrophyList[5]);
    totalDiceRolled += diceRoller->cRoll.count;
    trophyUpdateMilestone(&diceTrophyList[5], totalDiceRolled, 5);
}
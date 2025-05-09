/**
 * @file trophyTest.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @author GEMESIS
 * @brief Tests the functionality of the trophies.
 * @version 0.1
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "trophyTest.h"
#include "menu.h"

//==============================================================================
// Includes
//==============================================================================

#define SECOND_US 1000000

//==============================================================================
// Consts
//==============================================================================

static const char trophyModeName[] = "Trophy Test - Long name, short results!";

static const char* const textBlobs[] = {
    "Press A once",
    "Press B ten times",
    "Press up for eight seconds",
    "Press left, right and down at least once each",
    "Press pause to go to menu",
    "Trophy Test",
    "Clear progress",
    "Trophy Status",
    "Go back",
};

// Trophy Data example
const trophyData_t testTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .imageString = "kid0.wsg",
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing",
        .description = "Pressed B ten times!",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Trophy",
        .description = "Hold down the up button for eight seconds",
        .imageString = "",
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification, but like has a very long description",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
    {
        .title       = "Placeholder",
        .description = "If our eyes aren't real, why can't bees fly?",
        .imageString = "",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 12000000,
    },
    {
        .title       = "Placeholder 2",
        .description = "",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TROPHY_TEST_TESTING,
    TROPHY_TEST_MENU,
    TROPHY_TEST_DISPLAYING,
} trophyTestStateEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Trophy Vars
    // A presses
    int32_t aPresses;

    // B presses
    int32_t bPresses;

    // Up held duration
    int64_t heldTimer;
    int32_t upTime; // HeldTimer / SECOND_US
    bool timing;

    // Checklist
    int32_t checklistFlags;

    // Menu
    trophyTestStateEnum_t state;
    menu_t* menu;
    menuManiaRenderer_t* rndr;
} trophyTest_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Entrance into the mode
 *
 */
static void enterTrophy(void);

/**
 * @brief Mode cleanup
 *
 */
static void exitTrophy(void);

/**
 * @brief Main Loop
 *
 * @param elapsedUs Amount of time since last called
 */
static void runTrophy(int64_t elapsedUs);

/**
 * @brief Callback for options menu
 *
 * @param label Used to locate option
 * @param selected If option was selected or just highlighted
 * @param settingVal Value when a setting is changed
 */
static void trophyMenuCb(const char* label, bool selected, uint32_t settingVal);

/**
 * @brief Checks an individual bit flag out of a int32
 *
 * @param flags int32 containing the flag to check
 * @param idx Index of the bit
 * @return true If bit is set
 * @return false If bit is not set
 */
static bool checkBitFlag(int32_t flags, int8_t idx);

//==============================================================================
// Variables
//==============================================================================

trophySettings_t tSettings = {.animated           = false,
                              .drawFromBottom     = false,
                              .silent             = false,
                              .drawMaxDurationUs  = 500000,
                              .slideMaxDurationUs = 2000000};

trophyDataList_t trophyTestData = {.settings = &tSettings, .list = testTrophies};

swadgeMode_t trophyTestMode = {.modeName                 = trophyModeName,
                               .wifiMode                 = NO_WIFI,
                               .overrideUsb              = false,
                               .usesAccelerometer        = false,
                               .usesThermometer          = false,
                               .overrideSelectBtn        = false,
                               .fnEnterMode              = enterTrophy,
                               .fnExitMode               = exitTrophy,
                               .fnMainLoop               = runTrophy,
                               .fnAudioCallback          = NULL,
                               .fnBackgroundDrawCallback = NULL,
                               .fnEspNowRecvCb           = NULL,
                               .fnEspNowSendCb           = NULL,
                               .fnAdvancedUSB            = NULL,
                               .trophyData               = &trophyTestData};

static trophyTest_t* tt;

//==============================================================================
// Functions
//==============================================================================

static void enterTrophy()
{
    // Allocate memory
    tt = heap_caps_calloc(sizeof(trophyTest_t), 1, MALLOC_CAP_8BIT);

    // Initialize vars from disk
    tt->aPresses       = trophyGetSavedValue(testTrophies[0]);
    tt->bPresses       = trophyGetSavedValue(testTrophies[1]);
    tt->upTime         = trophyGetSavedValue(testTrophies[2]);
    tt->checklistFlags = trophyGetSavedValue(testTrophies[3]);

    tt->heldTimer = 0;

    // Menu
    tt->menu = initMenu(textBlobs[5], trophyMenuCb);
    tt->rndr = initMenuManiaRenderer(NULL, NULL, NULL);
    addSingleItemToMenu(tt->menu, textBlobs[6]);
    addSingleItemToMenu(tt->menu, textBlobs[7]);
    addSingleItemToMenu(tt->menu, textBlobs[8]);

    tt->state = TROPHY_TEST_TESTING;
}

static void exitTrophy()
{
    deinitMenuManiaRenderer(tt->rndr);
    deinitMenu(tt->menu);
    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (tt->state)
    {
        case TROPHY_TEST_MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                tt->menu = menuButton(tt->menu, evt);
            }
            drawMenuMania(tt->menu, tt->rndr, elapsedUs);
            break;
        }
        case TROPHY_TEST_DISPLAYING:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    tt->state = TROPHY_TEST_MENU;
                }
            }
            // TODO: Draw the current status screen
            break;
        }
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button == PB_A)
                    {
                        tt->aPresses++;
                        trophyUpdate(testTrophies[0], 1, true);
                    }
                    else if (evt.button == PB_B)
                    {
                        tt->bPresses++;
                        trophyUpdate(testTrophies[1], tt->bPresses, true);
                    }
                    else if (evt.button == PB_UP)
                    {
                        // Check how long held down
                        tt->timing    = true;
                        tt->heldTimer = 0; // Reset each time;
                    }
                    else if (evt.button == PB_DOWN)
                    {
                        tt->checklistFlags |= 1 << 0;
                        trophyUpdate(testTrophies[3], tt->checklistFlags, true);
                    }
                    else if (evt.button == PB_LEFT)
                    {
                        tt->checklistFlags |= 1 << 1;
                        trophyUpdate(testTrophies[3], tt->checklistFlags, true);
                    }
                    else if (evt.button == PB_RIGHT)
                    {
                        tt->checklistFlags |= 1 << 2;
                        trophyUpdate(testTrophies[3], tt->checklistFlags, true);
                    }
                    else if (evt.button == PB_START)
                    {
                        tt->state = TROPHY_TEST_MENU;
                    }
                }
                else if (evt.button & PB_UP)
                {
                    // Stop count
                    tt->timing = false;
                    // Save to NVS
                    if (tt->upTime < (tt->heldTimer / SECOND_US))
                    {
                        tt->upTime = (tt->heldTimer / SECOND_US);
                    }
                    trophyUpdate(testTrophies[2], tt->upTime, true);
                }
            }
            // Draw instructions
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

            char buffer[64];

            // Trigger
            drawText(getSysFont(), c555, textBlobs[0], 32, 32);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %" PRId32, tt->aPresses);
            drawText(getSysFont(), c500, buffer, 32, 48);

            // Additive
            drawText(getSysFont(), c555, textBlobs[1], 32, 64);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %" PRId32, tt->bPresses);
            drawText(getSysFont(), c050, buffer, 32, 80);

            // Progress
            drawText(getSysFont(), c555, textBlobs[2], 32, 96);
            snprintf(buffer, sizeof(buffer) - 1, "Previous Best: %" PRId32, tt->upTime);
            drawText(getSysFont(), c005, buffer, 32, 112);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %f", (tt->heldTimer * 1.0f) / SECOND_US);
            drawText(getSysFont(), c005, buffer, 32, 128);

            // Checklist
            int16_t startX = 32;
            int16_t startY = 146;
            drawTextWordWrap(getSysFont(), c555, textBlobs[3], &startX, &startY, TFT_WIDTH - 32, TFT_HEIGHT);
            paletteColor_t c = checkBitFlag(tt->checklistFlags, 0) ? c550 : c333;
            drawText(getSysFont(), c, "Down", 70, 170);
            c = checkBitFlag(tt->checklistFlags, 1) ? c505 : c333;
            drawText(getSysFont(), c, "Left", 32, 170);
            c = checkBitFlag(tt->checklistFlags, 2) ? c055 : c333;
            drawText(getSysFont(), c, "Right", 108, 170);

            // Go to menu
            drawText(getSysFont(), c555, textBlobs[4], 32, TFT_HEIGHT - 32);
            break;
        }
    }

    // Timer
    if (tt->timing)
    {
        tt->heldTimer += elapsedUs;
    }
}

static void trophyMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == textBlobs[6])
        {
            for (int idx = 0; idx < ARRAY_SIZE(testTrophies); idx++)
            {
                trophyClear(testTrophies[idx]);
            }
            tt->aPresses       = 0;
            tt->bPresses       = 0;
            tt->upTime         = 0;
            tt->heldTimer      = 0;
            tt->checklistFlags = 0;
        }
        else if (label == textBlobs[7])
        {
            tt->state = TROPHY_TEST_DISPLAYING;
        }
        else
        {
            tt->state = TROPHY_TEST_TESTING;
        }
    }
}

static bool checkBitFlag(int32_t flags, int8_t idx)
{
    return ((flags & (1 << idx)) != 0);
}
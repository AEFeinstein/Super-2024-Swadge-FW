//==============================================================================
// Includes
//==============================================================================

#include "swadgedoku.h"

#include "sudoku_data.h"
#include "sudoku_game.h"
#include "sudoku_solver.h"
#include "sudoku_ui.h"

#include "menu.h"
#include "wheel_menu.h"
#include "mainMenu.h"
#include "dialogBox.h"

//==============================================================================
// Defines
//==============================================================================

#define SUDOKU_PUZ_MIN SUDOKU_PUZ_000_BSP
#define SUDOKU_PUZ_MAX SUDOKU_PUZ_049_BSP

#define SUDOKU_SLN_MIN SUDOKU_SLN_000_BSP
#define SUDOKU_SLN_MAX SUDOKU_SLN_049_BSP

#define ENABLE_CUSTOM false
#define ENABLE_JIGSAW false

#define ONE_SECOND_IN_US (1000000)

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SWADGEDOKU_MAIN_MENU = 0,
    SWADGEDOKU_GAME      = 1,
    SWADGEDOKU_WIN       = 2,
    SWADGEDOKU_PAUSE     = 3,
} sudokuScreen_t;

typedef enum
{
    SD_BEGINNER = 0,
    SD_EASY     = 1,
    SD_MEDIUM   = 2,
    SD_HARD     = 3,
    SD_EXPERT   = 4,
    SD_HARDEST  = 5,
} sudokuDifficulty_t;

typedef enum
{
    HINT_POS_TOP,
    HINT_POS_CENTER,
    HINT_POS_BOTTOM,
} hintPosition_t;

//==============================================================================
// Structs
//==============================================================================

// grid functions: setValue(x, y, val); eraseValue(x, y, val); setNotes()

typedef struct
{
    /// @brief Holds font/image data needed to draw the UI
    sudokuDrawContext_t drawCtx;

    sudokuScreen_t screen;
    sudokuGrid_t game;
    int64_t playTimer;

    /// @brief The last (or next) level to play, the default to select
    int lastLevel;

    /// @brief The maximum unlocked level
    int maxLevel;

    int randState;

    sudokuPlayer_t player;

    menu_t* menu;
    menuMegaRenderer_t* menuRenderer;

    menu_t* emptyMenu;
    char emptyMenuTitle[32];

    menuItem_t* customModeMenuItem;
    menuItem_t* customSizeMenuItem;
    menuItem_t* customDifficultyMenuItem;

    bool jigsawForced;
    sudokuMode_t prevMode;
    bool squareForced;
    int prevSize;

    // Difficulty of the current game, or -1 if N/A
    int currentDifficulty;
    int currentMode;
    bool playingContinuation;
    int currentLevelNumber;
    int hintsUsed;

    menu_t* numberWheel;
    wheelMenuRenderer_t* numberWheelRenderer;
    rectangle_t wheelMenuTextBox;
    bool touchState;

    bool showingHint;
    sudokuMoveDesc_t hint;
    dialogBox_t* hintDialogBox;
    char hintText[256];
    hintPosition_t hintPos;

    menu_t* pauseMenu;

    // Settings cache!
    sudokuSettings_t settings;

    solverCache_t solverCache;

    sudokuGrid_t solution;
    bool useSolution;

    // whether or not the cursor is being dragged
    bool dragging;
    bool dragSet;
} swadgedoku_t;

typedef struct
{
    int timeLimit;
    int difficulty;
    int mode;
} swadgedokuTrophyTrigger_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void swadgedokuEnterMode(void);
static void swadgedokuExitMode(void);
static void swadgedokuMainLoop(int64_t elapsedUs);
static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void swadgedokuSetupMenu(void);
static bool swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value);
static bool swadgedokuPauseMenuCb(const char* label, bool selected, uint32_t value);
static void swadgedokuSetupNumberWheel(int base, uint16_t disableMask);
static bool numberWheelCb(const char* label, bool selected, uint32_t value);

static void swadgedokuDoWinCheck(void);
static void swadgedokuCheckTrophyTriggers(void);
static void swadgedokuPlayerSetDigit(uint8_t digit, bool isForce, bool forceSet);
static void swadgedokuShowHint(void);
static void swadgedokuGameButton(buttonEvt_t evt);
static void swadgedokuHintDialogCb(const char* label);

static sudokuDifficulty_t getLevelDifficulty(int level);

//==============================================================================
// Const data
//==============================================================================

// Mode name
static const char swadgedokuModeName[] = "Swadgedoku";

// Main menu
static const char menuItemContinue[]    = "Continue";
static const char menuItemLevelSelect[] = "Select Puzzle";
static const char menuItemPlayJigsaw[]  = "Jigsaw";
static const char menuItemPlayCustom[]  = "Infinite";
static const char menuItemStartCustom[] = "Start";
static const char menuItemCustomMode[]  = "Mode: ";
static const char menuItemCustomSize[]  = "Size: ";
static const char menuItemDifficulty[]  = "Rating: ";

// Settings menu
static const char menuItemSettings[]               = "Settings";
static const char menuItemWriteOnSelect[]          = "Write Number on Selection: ";
static const char menuItemAutoAnnotate[]           = "Auto-Annotate: ";
static const char menuItemHighlightPossibilities[] = "Highlight Possibilities: ";
static const char menuItemHighlightOnlyOptions[]   = "Highlight Only-Possible: ";
static const char menuItemMarkMistakes[]           = "Mark Mistakes: ";

// Pause menu
static const char strPaused[]             = "Paused";
static const char menuItemResume[]        = "Resume";
static const char menuItemHint[]          = "Hint";
static const char menuItemExitPuzzle[]    = "Save and Exit";
static const char menuItemResetPuzzle[]   = "Reset Puzzle";
static const char menuItemAbandonPuzzle[] = "Abandon Puzzle";

// Dialog box
static const char hintDialogTitle[]    = "Hint";
static const char dialogOptionOk[]     = "OK";
static const char dialogOptionCancel[] = "Cancel";

// Number wheel title
static const char strSelectDigit[] = "Select Digit";
static const char strYouWin[]      = "You Win!";

static const int32_t menuOptValsCustomSize[] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
};

static const char* const menuOptLabelsCustomSize[] = {
    "2x2",   "3x3",   "4x4",   "5x5",   "6x6",   "7x7",   "8x8",   "9x9",
    "10x10", "11x11", "12x12", "13x13", "14x14", "15x15", "16x16",
};

static const int32_t menuOptValsCustomMode[] = {
    SM_CLASSIC,
    SM_JIGSAW,
    SM_X_GRID,
};

static const char* const menuOptLabelsCustomMode[] = {
    "Classic",
    "Jigsaw",
    "X",
};

static const int32_t menuOptValsDifficulty[] = {
    SD_BEGINNER, SD_EASY, SD_MEDIUM, SD_HARD, SD_EXPERT, SD_HARDEST,
};

static const char* const menuOptLabelsDifficulty[] = {
    "Beginner", "Easy", "Medium", "Hard", "Expert", "Pain",
};

static const char* digitLabels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "A",
    "B",
    "C",
    "D",
    "E"
    "F",
    "0",
};

// Settings Data
/// @brief The NVS key to store the most-recently-completed level ID in
static const char settingKeyLastLevel[] = "sdku_lastlevel";

/// @brief The NVS key to store the maximum unlocked level ID
static const char settingKeyMaxLevel[] = "sdku_maxlevel";

/// @brief The NVS key to store the saved puzzle blob in
static const char settingKeyProgress[] = "sdku_progress";

/// @brief The NVS key to store the level number of the saved puzzle in
static const char settingKeyProgressId[] = "sdku_progid";

/// @brief The NVS key to store the elapsed time of the saved puzzle in
static const char settingKeyProgressTime[] = "sdku_progtime";

/// @brief Bounds for the full level select range (not the unlocked levels)
static const settingParam_t settingLevelSelectBounds
    = {.min = 1, .max = (SUDOKU_PUZ_MAX - SUDOKU_PUZ_MIN) + 1, .def = 1, .key = settingKeyLastLevel};

/// @brief Bounds for the custom 'grid size' setting
static const settingParam_t settingCustomSizeBounds
    = {.min = SUDOKU_MIN_BASE, .max = SUDOKU_MAX_BASE, .def = 9, .key = NULL};

/// @brief Bounds for the custom 'mode' setting
static const settingParam_t settingCustomModeBounds = {.min = 0, .max = 2, .def = 0, .key = NULL};

/// @brief Bounds for the difficulty setting
static const settingParam_t settingDifficultyBounds = {.min = SD_BEGINNER, .max = SD_HARDEST, .def = 0, .key = NULL};

/// @brief Generic bounds for true/false values, with a default of true
static const settingParam_t settingBoolDefaultOnBounds = {
    .min = 0,
    .max = 1,
    .def = 1,
    .key = NULL,
};

/// @brief Generic bounds for true/false values, with a default of false
static const settingParam_t settingBoolDefaultOffBounds = {
    .min = 0,
    .max = 1,
    .def = 0,
    .key = NULL,
};

/// @brief Strings for showing a boolean menu item as No/Yes
static const char* noYesOptions[] = {
    "No",
    "Yes",
};

/// @brief Boolean menu item values
static int32_t noYesValues[] = {
    0,
    1,
};

/// @brief A default light-mode display theme
static const sudokuTheme_t lightTheme = {.bgColor        = c333,
                                         .fillColor      = c555,
                                         .voidColor      = c333,
                                         .borderColor    = c000,
                                         .gridColor      = c222,
                                         .boxBorderColor = c000,
                                         .inkColor       = c000,
                                         .pencilColor    = c222,
                                         .uiTextColor    = c000,
                                         .cursorColor    = c505,
                                         .cursorShape    = OVERLAY_CIRCLE};

//==============================================================================
// Trophy Data
//==============================================================================

/// @brief Matches any puzzle, regardless of mode, difficulty, or solve time
const swadgedokuTrophyTrigger_t anyPuzzleTrigger = {
    .difficulty = -1,
    .timeLimit  = -1,
    .mode       = -1,
};

/// @brief Matches a puzzle that was solved in under ten minutes
const swadgedokuTrophyTrigger_t tenMinsTrigger = {
    .difficulty = -1,
    .timeLimit  = 600,
    .mode       = -1,
};

/// @brief Matches a puzzle that was solved in under five minutes
const swadgedokuTrophyTrigger_t fiveMinsTrigger = {
    .difficulty = -1,
    .timeLimit  = 300,
    .mode       = -1,
};

const trophyData_t swadgedokuTrophies[] = {{
                                               .title       = "It's a numbers game",
                                               .description = "Solve any Sudoku puzzle",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_TRIGGER,
                                               .difficulty  = TROPHY_DIFF_EASY,
                                               .maxVal      = 1,
                                               .identifier  = &anyPuzzleTrigger,
                                           },
                                           {
                                               .title       = "Fast fingers",
                                               .description = "Solve a Sudoku in under 10 minutes",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_TRIGGER,
                                               .difficulty  = TROPHY_DIFF_MEDIUM,
                                               .maxVal      = 1,
                                               .identifier  = &tenMinsTrigger,
                                           },
                                           {
                                               .title       = "Even faster fingers",
                                               .description = "Solve a Sudoku in under 5 minutes",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_TRIGGER,
                                               .difficulty  = TROPHY_DIFF_HARD,
                                               .maxVal      = 1,
                                               .identifier  = &fiveMinsTrigger,
                                           },
                                           {
                                               .title       = "Number cruncher",
                                               .description = "Solve all sudoku difficulties",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_PROGRESS,
                                               .difficulty  = TROPHY_DIFF_EXTREME,
                                               .maxVal      = (int)SD_HARDEST + 1,
                                               .identifier  = &anyPuzzleTrigger,
                                           },
                                           {
                                               .title       = "Revised rules",
                                               .description = "Play each Sudoku variant",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_CHECKLIST,
                                               .difficulty  = TROPHY_DIFF_HARD,
                                               .maxVal      = (1 << ((int)SM_X_GRID + 1)) - 1,
                                               .identifier  = &anyPuzzleTrigger,
                                           },
                                           {
                                               .title       = "A little help",
                                               .description = "Use a hint",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_TRIGGER,
                                               .difficulty  = TROPHY_DIFF_EASY,
                                               .maxVal      = 1,
                                               .identifier  = NULL,
                                           },
                                           {
                                               .title       = "All by myself",
                                               .description = "Solve a puzzle without any hints",
                                               .image       = NO_IMAGE_SET,
                                               .type        = TROPHY_TYPE_TRIGGER,
                                               .difficulty  = TROPHY_DIFF_MEDIUM,
                                               .maxVal      = 1,
                                               .identifier  = NULL,
                                           }};

// Individual mode settings
const trophySettings_t swadgedokuTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = swadgedokuModeName,
};

// This is passed to the swadgeMode_t
const trophyDataList_t swadgedokuTrophyData = {
    .settings = &swadgedokuTrophySettings,
    .list     = swadgedokuTrophies,
    .length   = ARRAY_SIZE(swadgedokuTrophies),
};

// Aliases for trophies
const trophyData_t* trophySolveAny = &swadgedokuTrophies[0];
const trophyData_t* trophyTenMins  = &swadgedokuTrophies[1];
const trophyData_t* trophyFiveMins = &swadgedokuTrophies[2];
const trophyData_t* trophyUseHint  = &swadgedokuTrophies[5];
const trophyData_t* trophyNoHints  = &swadgedokuTrophies[6];

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t swadgedokuMode = {
    .modeName                 = swadgedokuModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = swadgedokuEnterMode,
    .fnExitMode               = swadgedokuExitMode,
    .fnMainLoop               = swadgedokuMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = swadgedokuBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .trophyData               = &swadgedokuTrophyData,
};

static swadgedoku_t* sd = NULL;

//==============================================================================
// Functions
//==============================================================================

static void swadgedokuEnterMode(void)
{
    sd = heap_caps_calloc(1, sizeof(swadgedoku_t), MALLOC_CAP_8BIT);
    if (!sd)
    {
        // make cppcheck happy
        switchToSwadgeMode(&mainMenuMode);
        return;
    }

    sd->screen = SWADGEDOKU_MAIN_MENU;

    loadFont(RADIOSTARS_FONT, &sd->drawCtx.gridFont, true);
    loadFont(TINY_NUMBERS_FONT, &sd->drawCtx.noteFont, true);
    loadFont(SONIC_FONT, &sd->drawCtx.uiFont, true);

    loadWsg(SUDOKU_NOTES_WSG, &sd->drawCtx.noteTakingIcon, true);

    int32_t nvsVal;
    if (!readNvs32(settingKeyMaxLevel, &nvsVal))
    {
        nvsVal = 1;
    }
    sd->maxLevel = nvsVal;

    if (!readNvs32(settingKeyLastLevel, &nvsVal))
    {
        nvsVal = sd->maxLevel;
    }
    sd->lastLevel = nvsVal;

    swadgedokuLoadSettings(&sd->settings);

    sd->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);
    sd->emptyMenu    = initMenu(sd->emptyMenuTitle, NULL);

    sd->pauseMenu = initMenu(strPaused, swadgedokuPauseMenuCb);

    addSingleItemToMenu(sd->pauseMenu, menuItemResume);
    addSingleItemToMenu(sd->pauseMenu, menuItemHint);
    addSingleItemToMenu(sd->pauseMenu, menuItemResetPuzzle);
    addSingleItemToMenu(sd->pauseMenu, menuItemAbandonPuzzle);
    addSingleItemToMenu(sd->pauseMenu, menuItemExitPuzzle);

    initSolverCache(&sd->solverCache, 9, 9);

    swadgedokuSetupMenu();
}

static void swadgedokuExitMode(void)
{
    deinitSolverCache(&sd->solverCache);
    deinitSudokuGame(&sd->game);
    deinitSudokuGame(&sd->solution);

    void* val = NULL;
    while (NULL != (val = pop(&sd->player.overlay.shapes)))
    {
        heap_caps_free(val);
    }

    freeFont(&sd->drawCtx.gridFont);
    freeFont(&sd->drawCtx.noteFont);
    freeFont(&sd->drawCtx.uiFont);

    freeWsg(&sd->drawCtx.noteTakingIcon);

    heap_caps_free(sd->player.notes);
    heap_caps_free(sd->player.overlay.gridOpts);

    if (sd->numberWheelRenderer != NULL)
    {
        deinitWheelMenu(sd->numberWheelRenderer);
        sd->numberWheelRenderer = NULL;
    }
    if (sd->numberWheel != NULL)
    {
        deinitMenu(sd->numberWheel);
        sd->numberWheel = NULL;
    }

    deinitMenuMegaRenderer(sd->menuRenderer);
    deinitMenu(sd->menu);
    deinitMenu(sd->emptyMenu);
    deinitMenu(sd->pauseMenu);
    heap_caps_free(sd);
    sd = NULL;
}

static void swadgedokuMainLoop(int64_t elapsedUs)
{
    switch (sd->screen)
    {
        case SWADGEDOKU_MAIN_MENU:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                sd->menu = menuButton(sd->menu, evt);
            }

            drawMenuMega(sd->menu, sd->menuRenderer, elapsedUs);
            break;
        }

        case SWADGEDOKU_GAME:
        {
            bool inWheelMenu = false;
            if (!sd->showingHint)
            {
                int32_t phi, r, intensity;
                if (getTouchJoystick(&phi, &r, &intensity))
                {
                    wheelMenuTouch(sd->numberWheel, sd->numberWheelRenderer, phi, r);
                    sd->touchState = true;
                }
                else if (sd->touchState)
                {
                    wheelMenuTouchRelease(sd->numberWheel, sd->numberWheelRenderer);
                    sd->touchState = false;
                }

                // Now that touches have been handled, check if the wheel is active
                inWheelMenu = wheelMenuActive(sd->numberWheel, sd->numberWheelRenderer);

                if (!inWheelMenu)
                {
                    sd->playTimer += elapsedUs;
                }
                // TODO I started to put an else here but I forgot what was supposed to go in it
            }

            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (inWheelMenu && !sd->showingHint)
                {
                    wheelMenuButton(sd->numberWheel, sd->numberWheelRenderer, &evt);
                }
                else
                {
                    swadgedokuGameButton(evt);
                }
            }

            uint16_t fullNotes[sd->game.size * sd->game.size];
            if (sd->settings.autoAnnotate)
            {
                for (int n = 0; n < sd->game.size * sd->game.size; n++)
                {
                    fullNotes[n] = sd->game.notes[n] ^ sd->player.notes[n];
                }
            }
            else
            {
                memcpy(fullNotes, sd->player.notes, sd->game.size * sd->game.size * sizeof(uint16_t));
            }

            sudokuOverlayOpt_t optMask = OVERLAY_ALL;
            sudokuShapeTag_t tagMask   = ST_ALL;
            if (sd->showingHint)
            {
                tagMask = ST_HINT;
                optMask &= ~(OVERLAY_QUESTION | OVERLAY_CHECK | OVERLAY_HIGHLIGHT_A | OVERLAY_HIGHLIGHT_B
                             | OVERLAY_HIGHLIGHT_C | OVERLAY_HIGHLIGHT_D);
            }
            swadgedokuDrawGame(&sd->game, fullNotes, &sd->player.overlay, &lightTheme, &sd->drawCtx, tagMask, optMask);

            if (sd->showingHint)
            {
                // Calculate where on the screen the hint square is, so we can avoid covering it with the dialog box
                uint16_t dialogY = DIALOG_CENTER;

                switch (sd->hintPos)
                {
                    case HINT_POS_TOP:
                    {
                        dialogY = 0;
                        break;
                    }

                    case HINT_POS_CENTER:
                    {
                        dialogY = DIALOG_CENTER;
                        break;
                    }

                    case HINT_POS_BOTTOM:
                    {
                        dialogY = TFT_HEIGHT * 2 / 3;
                        break;
                    }
                }

                drawDialogBox(sd->hintDialogBox, getSysFont(), getSysFont(), DIALOG_CENTER, dialogY, DIALOG_AUTO,
                              DIALOG_AUTO, 3);
            }
            else
            {
                if (sd->player.noteTaking)
                {
                    // draw like, a pencil
                    drawWsgSimple(&sd->drawCtx.noteTakingIcon, TFT_WIDTH - 5 - sd->drawCtx.noteTakingIcon.w,
                                  (TFT_HEIGHT - sd->drawCtx.gridFont.height) / 2 - 5 - sd->drawCtx.noteTakingIcon.h);
                }

                if (sd->player.selectedDigit)
                {
                    char curDigitStr[16];
                    snprintf(curDigitStr, sizeof(curDigitStr), "%" PRIX8, sd->player.selectedDigit);
                    int textW = textWidth(&sd->drawCtx.gridFont, curDigitStr);

                    drawText(&sd->drawCtx.gridFont, lightTheme.uiTextColor, curDigitStr, TFT_WIDTH - 5 - textW,
                             (TFT_HEIGHT - sd->drawCtx.gridFont.height) / 2);
                }

                if (inWheelMenu)
                {
                    shadeDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, 2, c111);
                    drawRoundedRect(sd->wheelMenuTextBox.pos.x - 2, sd->wheelMenuTextBox.pos.y - 2,
                                    sd->wheelMenuTextBox.pos.x + sd->wheelMenuTextBox.width + 2,
                                    sd->wheelMenuTextBox.pos.y + sd->wheelMenuTextBox.height + 2, 5, c555, c000);
                    drawWheelMenu(sd->numberWheel, sd->numberWheelRenderer, elapsedUs);
                }
            }
            break;
        }

        case SWADGEDOKU_WIN:
        {
            strncpy(sd->emptyMenuTitle, strYouWin, sizeof(sd->emptyMenuTitle));
            drawMenuMega(sd->emptyMenu, sd->menuRenderer, elapsedUs);

            char playTime[64];
            int totalTime = sd->playTimer / ONE_SECOND_IN_US;

            int hours = totalTime / 3600;
            int mins  = (totalTime % 3600) / 60;
            int secs  = totalTime % 60;
            if (hours > 0)
            {
                snprintf(playTime, sizeof(playTime), "%d:%02d:%02d", hours, mins, secs);
            }
            else
            {
                snprintf(playTime, sizeof(playTime), "%02d:%02d", mins, secs);
            }

            const char* strCompletionTime = "Completed in";
            drawText(&sd->drawCtx.uiFont, c000, strCompletionTime,
                     (TFT_WIDTH - textWidth(&sd->drawCtx.uiFont, strCompletionTime)) / 2,
                     TFT_HEIGHT / 2 - sd->drawCtx.uiFont.height - 1);
            drawText(&sd->drawCtx.uiFont, c000, playTime, (TFT_WIDTH - textWidth(&sd->drawCtx.uiFont, playTime)) / 2,
                     TFT_HEIGHT / 2);

            buttonEvt_t evt = {0};
            if (checkButtonQueueWrapper(&evt) && evt.down)
            {
                // Reset the menu, return to main menu
                swadgedokuSetupMenu();
                sd->screen = SWADGEDOKU_MAIN_MENU;
            }
            break;
        }

        case SWADGEDOKU_PAUSE:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button == PB_B || evt.button == PB_START))
                {
                    sd->screen = SWADGEDOKU_GAME;
                    break;
                }

                sd->pauseMenu = menuButton(sd->pauseMenu, evt);
            }

            drawMenuMega(sd->pauseMenu, sd->menuRenderer, elapsedUs);
            break;
        }
    }
}

static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
}

static void swadgedokuSetupMenu(void)
{
    //// Main Menu
    bool hasProgress  = false;
    size_t progLength = 0;
    if (readNvsBlob(settingKeyProgress, NULL, &progLength))
    {
        hasProgress = true;
    }

    if (sd->menu != NULL)
    {
        deinitMenu(sd->menu);
        sd->menu = NULL;
    }

    sd->menu = initMenu(swadgedokuModeName, swadgedokuMainMenuCb);

    if (hasProgress)
    {
        addSingleItemToMenu(sd->menu, menuItemContinue);
    }
    ESP_LOGE("Swadgedoku", "Last level is %d", sd->lastLevel);
    if (sd->lastLevel > sd->maxLevel)
    {
        sd->lastLevel = sd->maxLevel;
    }

    // Kinda clunky...
    settingParam_t constrainedLevelSelectBounds = {
        .def = sd->lastLevel,
        .min = 1,
        .max = sd->maxLevel,
    };

    addSettingsItemToMenu(sd->menu, menuItemLevelSelect, &constrainedLevelSelectBounds, sd->lastLevel);

    if (ENABLE_JIGSAW)
    {
        addSingleItemToMenu(sd->menu, menuItemPlayJigsaw);
    }

    if (ENABLE_CUSTOM)
    {
        sd->menu = startSubMenu(sd->menu, menuItemPlayCustom);
        addSingleItemToMenu(sd->menu, menuItemStartCustom);
        sd->customModeMenuItem
            = addSettingsOptionsItemToMenu(sd->menu, menuItemCustomMode, menuOptLabelsCustomMode, menuOptValsCustomMode,
                                           ARRAY_SIZE(menuOptValsCustomMode), &settingCustomModeBounds, 0);
        sd->customSizeMenuItem
            = addSettingsOptionsItemToMenu(sd->menu, menuItemCustomSize, menuOptLabelsCustomSize, menuOptValsCustomSize,
                                           ARRAY_SIZE(menuOptValsCustomSize), &settingCustomSizeBounds, 9);
        sd->customDifficultyMenuItem
            = addSettingsOptionsItemToMenu(sd->menu, menuItemDifficulty, menuOptLabelsDifficulty, menuOptValsDifficulty,
                                           ARRAY_SIZE(menuOptValsDifficulty), &settingDifficultyBounds, 0);
    }

    sd->menu = endSubMenu(sd->menu);

    sd->menu = startSubMenu(sd->menu, menuItemSettings);
    addSettingsOptionsItemToMenu(sd->menu, menuItemWriteOnSelect, noYesOptions, noYesValues, ARRAY_SIZE(noYesOptions),
                                 &settingBoolDefaultOnBounds, sd->settings.writeOnSelect ? 1 : 0);
    addSettingsOptionsItemToMenu(sd->menu, menuItemAutoAnnotate, noYesOptions, noYesValues, ARRAY_SIZE(noYesOptions),
                                 &settingBoolDefaultOffBounds, sd->settings.autoAnnotate ? 1 : 0);
    addSettingsOptionsItemToMenu(sd->menu, menuItemHighlightPossibilities, noYesOptions, noYesValues,
                                 ARRAY_SIZE(noYesOptions), &settingBoolDefaultOnBounds,
                                 sd->settings.highlightOnlyOptions ? 1 : 0);
    addSettingsOptionsItemToMenu(sd->menu, menuItemHighlightOnlyOptions, noYesOptions, noYesValues,
                                 ARRAY_SIZE(noYesOptions), &settingBoolDefaultOffBounds,
                                 sd->settings.highlightPossibilities ? 1 : 0);
    addSettingsOptionsItemToMenu(sd->menu, menuItemMarkMistakes, noYesOptions, noYesValues, ARRAY_SIZE(noYesOptions),
                                 &settingBoolDefaultOffBounds, sd->settings.markMistakes ? 1 : 0);
    sd->menu = endSubMenu(sd->menu);
}

static void swadgedokuSetupNumberWheel(int base, uint16_t disableMask)
{
    //// Number Wheel
    if (sd->numberWheelRenderer != NULL)
    {
        deinitWheelMenu(sd->numberWheelRenderer);
        sd->numberWheelRenderer = NULL;
    }

    if (sd->numberWheel != NULL)
    {
        deinitMenu(sd->numberWheel);
        sd->numberWheel = NULL;
    }

    sd->wheelMenuTextBox.pos.x  = 24;
    sd->wheelMenuTextBox.pos.y  = TFT_HEIGHT - sd->drawCtx.uiFont.height - 3;
    sd->wheelMenuTextBox.width  = TFT_WIDTH - 48;
    sd->wheelMenuTextBox.height = sd->drawCtx.uiFont.height;

    sd->numberWheelRenderer = initWheelMenu(&sd->drawCtx.uiFont, 90, &sd->wheelMenuTextBox);
    sd->numberWheel         = initMenu(strSelectDigit, numberWheelCb);

    for (int i = 0; i < base && i < ARRAY_SIZE(digitLabels); i++)
    {
        uint8_t pos = (base - i) % base;
        addSingleItemToMenu(sd->numberWheel, digitLabels[i]);
        wheelMenuSetItemInfo(sd->numberWheelRenderer, digitLabels[i], NULL, pos, NO_SCROLL);
        wheelMenuSetItemTextIcon(sd->numberWheelRenderer, digitLabels[i], digitLabels[i]);

        if (disableMask & (1 << i))
        {
            wheelMenuSetItemColor(sd->numberWheelRenderer, digitLabels[i], c333, c444);
        }
    }
}

static bool swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (menuItemContinue == label)
        {
            size_t progLength = 0;
            if (readNvsBlob(settingKeyProgress, NULL, &progLength))
            {
                uint8_t buffer[progLength];
                if (readNvsBlob(settingKeyProgress, buffer, &progLength))
                {
                    if (loadSudokuData(buffer, progLength, &sd->game))
                    {
                        // Check if the progress is a "campaign" level
                        int32_t curLevel;
                        if (readNvs32(settingKeyProgressId, &curLevel))
                        {
                            sd->currentLevelNumber = curLevel;
                            sd->currentDifficulty  = getLevelDifficulty(curLevel);
                        }
                        else
                        {
                            sd->currentLevelNumber = -1;
                        }

                        int32_t progressTime = 0;
                        if (readNvs32(settingKeyProgressTime, &progressTime))
                        {
                            // we save in millis but the timer is in micros
                            sd->playTimer = progressTime;
                            sd->playTimer *= 1000;
                        }
                        else
                        {
                            sd->playTimer = 0;
                        }

                        if (sd->currentLevelNumber >= 0)
                        {
                            ESP_LOGI("Swadgedoku", "Loading level %d", sd->currentLevelNumber);
                            cnfsFileIdx_t solutionFile = sd->currentLevelNumber - 1 + SUDOKU_SLN_MIN;
                            if (SUDOKU_SLN_MIN <= solutionFile && solutionFile <= SUDOKU_SLN_MAX)
                            {
                                size_t solutionLen;
                                const uint8_t* solutionData = cnfsGetFile(solutionFile, &solutionLen);
                                sd->useSolution             = loadSudokuData(solutionData, solutionLen, &sd->solution);
                                if (sd->useSolution)
                                {
                                    ESP_LOGI("Swadgedoku", "Successfully loaded solution");
                                }
                                else
                                {
                                    ESP_LOGI("Swadgedoku", "Solution loading failed");
                                }
                            }
                        }

                        sd->hintsUsed           = 0;
                        sd->playingContinuation = true;
                        setupSudokuPlayer(&sd->player, &sd->game);
                        memcpy(sd->player.notes, sd->game.notes, sd->game.size * sizeof(uint16_t));
                        sudokuGetNotes(sd->game.notes, &sd->game, 0);
                        sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                                       sd->useSolution ? &sd->solution : NULL);
                        swadgedokuSetupNumberWheel(sd->game.base, 0);
                        resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
                        sd->solverCache.solution = sd->useSolution ? sd->solution.grid : NULL;
                        sd->screen               = SWADGEDOKU_GAME;
                    }
                    else
                    {
                        ESP_LOGE("Swadgedoku", "Can't load saved game: bad data");
                    }
                }
            }
        }
        else if (menuItemLevelSelect == label)
        {
            /*if (!setupSudokuGame(&sd->game, SM_CLASSIC, 9, 9))
            {
                ESP_LOGE("Swadgedoku", "Couldn't setup game???");
                return;
            }*/
            cnfsFileIdx_t file = value - 1 + SUDOKU_PUZ_MIN;
            if (file < SUDOKU_PUZ_MIN || file > SUDOKU_PUZ_MAX)
            {
                ESP_LOGE("Swadgedoku", "Tried to load illegal game file %" PRIu32, value);
                return false;
            }
            size_t fileLen;
            const uint8_t* sudokuData = cnfsGetFile(file, &fileLen);
            if (!loadSudokuData(sudokuData, fileLen, &sd->game))
            {
                ESP_LOGE("Swadgedoku", "Couldn't load game file");
                return false;
            }

            sd->hintsUsed           = 0;
            sd->playTimer           = 0;
            sd->playingContinuation = false;
            sd->currentLevelNumber  = value;
            sd->currentDifficulty   = getLevelDifficulty(value);

            cnfsFileIdx_t solutionFile = value - 1 + SUDOKU_SLN_MIN;
            if (SUDOKU_SLN_MIN <= solutionFile && solutionFile <= SUDOKU_SLN_MAX)
            {
                size_t solutionLen;
                const uint8_t* solutionData = cnfsGetFile(solutionFile, &solutionLen);
                sd->useSolution             = loadSudokuData(solutionData, solutionLen, &sd->solution);
            }

            setupSudokuPlayer(&sd->player, &sd->game);

            sudokuGetNotes(sd->game.notes, &sd->game, 0);
            sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                           sd->useSolution ? &sd->solution : NULL);
            resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
            sd->solverCache.solution = sd->useSolution ? sd->solution.grid : NULL;

            swadgedokuSetupNumberWheel(sd->game.base, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemStartCustom == label)
        {
            if (!setupSudokuGame(&sd->game, sd->customModeMenuItem->currentSetting,
                                 sd->customSizeMenuItem->currentSetting, sd->customSizeMenuItem->currentSetting))
            {
                ESP_LOGE("Swadgedoku", "Couldn't setup custom game???");
                return false;
            }

            sd->currentMode       = sd->customModeMenuItem->currentSetting;
            sd->currentDifficulty = sd->customDifficultyMenuItem->currentSetting;

            sd->hintsUsed           = 0;
            sd->playTimer           = 0;
            sd->playingContinuation = false;
            sd->currentLevelNumber  = -1;
            setupSudokuPlayer(&sd->player, &sd->game);
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
            sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                           sd->useSolution ? &sd->solution : NULL);
            resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
            swadgedokuSetupNumberWheel(sd->game.base, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemPlayJigsaw == label)
        {
            if (!setupSudokuGame(&sd->game, SM_JIGSAW, 9, 9))
            {
                return false;
            }

            sd->hintsUsed           = 0;
            sd->playTimer           = 0;
            sd->playingContinuation = false;
            sd->currentLevelNumber  = -1;
            setupSudokuPlayer(&sd->player, &sd->game);
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
            sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                           sd->useSolution ? &sd->solution : NULL);
            resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
            swadgedokuSetupNumberWheel(sd->game.base, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
    }
    else
    {
        if (menuItemLevelSelect == label)
        {
            sd->lastLevel = value;
        }
        else if (menuItemCustomSize == label)
        {
            // If the selected size is non-square, automatically switch to jigsaw mode
            bool forceJigsaw = false;
            switch (value)
            {
                case 1:
                case 4:
                case 9:
                case 16:
                    break;

                default:
                    forceJigsaw = true;
                    break;
            }
            if (forceJigsaw && sd->customModeMenuItem->currentOpt != SM_JIGSAW)
            {
                sd->prevMode                           = sd->customModeMenuItem->currentOpt;
                sd->customModeMenuItem->currentOpt     = SM_JIGSAW;
                sd->customModeMenuItem->currentSetting = SM_JIGSAW;
                sd->jigsawForced                       = true;
            }
            else if (!forceJigsaw && sd->customModeMenuItem->currentOpt == SM_JIGSAW && sd->jigsawForced)
            {
                // And when they switch away from a jigsaw-only size, return to the previous mode
                sd->customModeMenuItem->currentSetting = sd->prevMode;
                sd->customModeMenuItem->currentOpt     = sd->prevMode;
                sd->jigsawForced                       = false;
            }
        }
        else if (menuItemCustomMode == label)
        {
            // If the selected mode is changed to classic, switch away from incompatible non-square sizes

            bool forceSquare = false;
            switch (sd->customSizeMenuItem->currentSetting)
            {
                case 1:
                case 4:
                case 9:
                case 16:
                    break;

                default:
                    forceSquare = true;
                    break;
            }
            if (forceSquare && value == SM_CLASSIC)
            {
                sd->prevSize                           = sd->customSizeMenuItem->currentSetting;
                sd->customSizeMenuItem->currentSetting = 9;
                sd->customSizeMenuItem->currentOpt     = 7;
                sd->squareForced                       = true;
            }
            else if (!forceSquare && value != SM_CLASSIC && sd->squareForced)
            {
                // And when they switch away from the incompatible size, restore the previous selection
                sd->customSizeMenuItem->currentSetting = sd->prevSize;
                sd->customSizeMenuItem->currentOpt     = sd->prevSize - 2;
                sd->squareForced                       = false;
            }
        }
        else if (menuItemWriteOnSelect == label)
        {
            bool newVal = value ? true : false;
            if (newVal != sd->settings.writeOnSelect)
            {
                sd->settings.writeOnSelect = newVal;
                swadgedokuSaveSettings(&sd->settings);
            }
        }
        else if (menuItemAutoAnnotate == label)
        {
            bool newVal = value ? true : false;
            if (newVal != sd->settings.autoAnnotate)
            {
                sd->settings.autoAnnotate = newVal;
                swadgedokuSaveSettings(&sd->settings);
            }
        }
        else if (menuItemHighlightPossibilities == label)
        {
            bool newVal = value ? true : false;
            if (newVal != sd->settings.highlightPossibilities)
            {
                sd->settings.highlightPossibilities = newVal;
                swadgedokuSaveSettings(&sd->settings);
            }
        }
        else if (menuItemHighlightOnlyOptions == label)
        {
            bool newVal = value ? true : false;
            if (newVal != sd->settings.highlightOnlyOptions)
            {
                sd->settings.highlightOnlyOptions = newVal;
                swadgedokuSaveSettings(&sd->settings);
            }
        }
        else if (menuItemMarkMistakes == label)
        {
            bool newVal = value ? true : false;
            if (newVal != sd->settings.markMistakes)
            {
                sd->settings.markMistakes = newVal;
                swadgedokuSaveSettings(&sd->settings);
            }
        }
    }
    return false;
}

static bool swadgedokuPauseMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        ESP_LOGE("Swadgedoku", "Pause menu %s selected", label);
        if (menuItemResume == label)
        {
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemHint == label)
        {
            swadgedokuShowHint();
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemResetPuzzle == label)
        {
            for (int n = 0; n < sd->game.size * sd->game.size; n++)
            {
                // Just clear all non-player-set digits
                if (!(sd->game.flags[n] & (SF_LOCKED | SF_VOID)))
                {
                    sd->game.grid[n]    = 0;
                    sd->player.notes[n] = 0;
                }
            }
            sd->playTimer = 0;
            sd->hintsUsed = 0;
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
        }
        else if (menuItemAbandonPuzzle == label)
        {
            swadgedokuSetupMenu();
            sd->screen = SWADGEDOKU_MAIN_MENU;
        }
        else if (menuItemExitPuzzle == label)
        {
            size_t size = getSudokuSaveSize(&sd->game, NULL, NULL, NULL, NULL);
            ESP_LOGE("Swadgedoku", "Writing %d bytes of swadgedoku save to NVS", (int)size);
            uint16_t* tmpNotes = sd->game.notes;
            sd->game.notes     = sd->player.notes;
            uint8_t data[size];
            // Write the data to the buffer
            writeSudokuData(data, &sd->game);
            sd->game.notes = tmpNotes;

            // Write the buffer to NVS
            if (writeNvsBlob(settingKeyProgress, data, size))
            {
                ESP_LOGI("Swadgedoku", "Saved progress to NVS!");
                if (sd->currentLevelNumber != -1)
                {
                    if (!writeNvs32(settingKeyProgressId, sd->currentLevelNumber))
                    {
                        ESP_LOGE("Swadgedoku", "Failed to save progress level ID");
                    }

                    int32_t millis = sd->playTimer / 1000;
                    if (!writeNvs32(settingKeyProgressTime, millis))
                    {
                        ESP_LOGE("Swadgedoku", "Failed to save progress timer");
                    }
                }
                swadgedokuSetupMenu();
                sd->screen = SWADGEDOKU_MAIN_MENU;
            }
            else
            {
                ESP_LOGE("Swadgedoku", "Could not save progress to NVS!!!");
            }
        }
    }
    return false;
}

static bool numberWheelCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        for (int i = 0; i < ARRAY_SIZE(digitLabels); i++)
        {
            if (label == digitLabels[i])
            {
                sd->player.selectedDigit = i + 1;
                if (sd->settings.writeOnSelect)
                {
                    swadgedokuPlayerSetDigit(sd->player.selectedDigit, false, false);
                    sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                                   sd->useSolution ? &sd->solution : NULL);
                }
            }
        }
    }
    return false;
}

static void swadgedokuDoWinCheck(void)
{
    switch (swadgedokuCheckWin(&sd->game))
    {
        case SUDOKU_INVALID:
            ESP_LOGD("Swadgedoku", "Invalid sudoku board!");
            break;

        case SUDOKU_INCOMPLETE:
            ESP_LOGD("Swadgedoku", "Sudoku OK but incomplete");
            break;

        case SUDOKU_COMPLETE:
        {
            swadgedokuCheckTrophyTriggers();
            if (sd->playingContinuation)
            {
                // Clear saved game since we just finished it
                if (!eraseNvsKey(settingKeyProgress))
                {
                    ESP_LOGE("Swadgedoku", "Couldn't erase game progress from NVS!");
                }

                if (!eraseNvsKey(settingKeyProgressId))
                {
                    ESP_LOGE("Swadgedoku", "Couldn't erase progress level ID value");
                }

                if (!eraseNvsKey(settingKeyProgressTime))
                {
                    ESP_LOGE("Swadgedoku", "Couldn't erase progress timer value");
                }
            }

            if (sd->currentLevelNumber != -1)
            {
                // Update the max level if needed
                if (sd->currentLevelNumber >= sd->maxLevel && sd->maxLevel < settingLevelSelectBounds.max)
                {
                    ESP_LOGI("Swadgedoku", "Updating max completed level from %d to %d", sd->maxLevel,
                             sd->currentLevelNumber + 1);
                    sd->maxLevel = sd->currentLevelNumber + 1;
                    if (!writeNvs32(settingKeyMaxLevel, sd->maxLevel))
                    {
                        ESP_LOGE("Swadgedoku", "Couldn't write updated max level to NVS");
                    }
                }

                if (sd->lastLevel < sd->maxLevel && sd->currentLevelNumber < sd->maxLevel)
                {
                    ESP_LOGI("Swadgedoku", "Updating last completed level from %d to %d", sd->lastLevel,
                             sd->currentLevelNumber + 1);
                    sd->lastLevel = sd->currentLevelNumber + 1;
                    if (!writeNvs32(settingKeyLastLevel, sd->lastLevel))
                    {
                        ESP_LOGE("Swadgedoku", "Couldn't write updated last level to NVS");
                    }
                }
                sd->currentLevelNumber = -1;
            }
            sd->screen = SWADGEDOKU_WIN;
            break;
        }
    }
}

static void swadgedokuCheckTrophyTriggers(void)
{
    for (const trophyData_t* trophy = swadgedokuTrophyData.list;
         trophy < swadgedokuTrophyData.list + swadgedokuTrophyData.length; trophy++)
    {
        if (NULL != trophy->identifier)
        {
            const swadgedokuTrophyTrigger_t* trigger = (const swadgedokuTrophyTrigger_t*)trophy->identifier;

            if (trigger->timeLimit != -1 && (sd->playTimer / ONE_SECOND_IN_US) > trigger->timeLimit)
            {
                // Over time limit for trigger, skip to next
                continue;
            }

            if (trigger->difficulty != -1 && sd->currentDifficulty < trigger->difficulty)
            {
                // Difficulty doesn't match trigger, skip to next
                continue;
            }

            if (trigger->mode != -1 && sd->currentMode != trigger->mode)
            {
                // Mode doesn't match trigger, skip to next
                continue;
            }

            // All the trigger conditions must have passed, trigger the trophy!
            switch (trophy->type)
            {
                case TROPHY_TYPE_TRIGGER:
                    trophyUpdate(trophy, 1, true);
                    break;

                case TROPHY_TYPE_PROGRESS:
                    trophyUpdate(trophy, sd->currentDifficulty + 1, true);
                    break;

                case TROPHY_TYPE_ADDITIVE:
                    // TODO: This would probably be the overall 'score'
                    // So come up with a way to calculate a score
                    break;

                case TROPHY_TYPE_CHECKLIST:
                    // Checklist will probably be for the mode
                    trophySetChecklistTask(trophy, 1 << sd->currentMode, false, true);
                    break;
            }
        }
    }

    if (sd->hintsUsed == 0)
    {
        trophyUpdate(trophyNoHints, 1, true);
    }
}

static void swadgedokuPlayerSetDigit(uint8_t digit, bool isForce, bool forceSet)
{
    if (sd->player.noteTaking)
    {
        if (!(sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX] & (SF_LOCKED | SF_VOID)))
        {
            uint16_t bit   = 1 << (digit - 1);
            uint16_t* note = &sd->player.notes[sd->player.curY * sd->game.size + sd->player.curX];

            bool unset = (*note & bit);

            if (isForce)
            {
                unset = !forceSet;
            }

            if (unset)
            {
                *note &= (~bit);
            }
            else
            {
                *note |= bit;
            }
        }
    }
    else
    {
        if (setDigit(&sd->game, digit, sd->player.curX, sd->player.curY))
        {
            swadgedokuDoWinCheck();
        }
    }
}

static void swadgedokuShowHint(void)
{
    if (sudokuNextMove(&sd->solverCache, &sd->game))
    {
        hintBufDebug(sd->solverCache.hintBuf, sd->solverCache.hintbufLen);
        hintToOverlay(&sd->player.overlay, &sd->game, -1, sd->solverCache.hintBuf, sd->solverCache.hintbufLen);
        writeStepDescription(sd->hintText, sizeof(sd->hintText), sd->solverCache.hintBuf, sd->solverCache.hintbufLen,
                             -1);

        ESP_LOGI("Swadgedoku", "Got hint!");
        if (sd->hintDialogBox == NULL)
        {
            sd->hintDialogBox = initDialogBox(hintDialogTitle, sd->hintText, NULL, swadgedokuHintDialogCb);
            dialogBoxAddOption(sd->hintDialogBox, dialogOptionOk, NULL, OPTHINT_OK | OPTHINT_DEFAULT);
            dialogBoxAddOption(sd->hintDialogBox, dialogOptionCancel, NULL, OPTHINT_CANCEL);
        }
        else
        {
            sd->hintDialogBox->title          = hintDialogTitle;
            sd->hintDialogBox->detail         = sd->hintText;
            sd->hintDialogBox->selectedOption = sd->hintDialogBox->options.first;
        }

        ESP_LOGI("Swadgedoku", "Move: %s", sd->hintText);
        sd->showingHint = true;

        if (sd->hint.stepCount != 0)
        {
            if (sd->hint.eliminationSteps[0].eliminationCount > 0)
            {
                ESP_LOGI("Swadgedoku", "Some things were eliminated");
            }
        }
    }
    else
    {
        ESP_LOGI("Swadgedoku", "No hint :(");
    }
}

static void swadgedokuGameButton(buttonEvt_t evt)
{
    if (sd->showingHint)
    {
        if (evt.down && (evt.button == PB_UP || evt.button == PB_DOWN))
        {
            bool up = (evt.button == PB_UP);
            switch (sd->hintPos)
            {
                case HINT_POS_TOP:
                {
                    sd->hintPos = up ? HINT_POS_BOTTOM : HINT_POS_CENTER;
                    break;
                }

                case HINT_POS_CENTER:
                {
                    sd->hintPos = up ? HINT_POS_TOP : HINT_POS_BOTTOM;
                    break;
                }

                case HINT_POS_BOTTOM:
                {
                    sd->hintPos = up ? HINT_POS_CENTER : HINT_POS_TOP;
                    break;
                }
            }
        }
        else
        {
            dialogBoxButton(sd->hintDialogBox, &evt);
        }
        return;
    }

    if (evt.down)
    {
        bool moved      = false;
        bool reAnnotate = false;
        bool drag       = false;
        switch (evt.button)
        {
            case PB_A:
            {
                // A sets
                int digit = (evt.button == PB_A) ? sd->player.selectedDigit : 0;

                if (!((SF_LOCKED | SF_VOID) & sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                {
                    // Not locked or void, proceed setting the digit
                    if (!digit || digit == sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX])
                    {
                        // Unset number
                        setDigit(&sd->game, 0, sd->player.curX, sd->player.curY);
                        sd->dragSet  = false;
                        sd->dragging = true;
                        ESP_LOGI("Sudoku", "Drag - first digit was unset! (!digit)");
                    }
                    else
                    {
                        // Set number, might toggle
                        if (sd->player.noteTaking)
                        {
                            uint16_t bit = (1 << (digit - 1));
                            if (!sd->game.grid[sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX]])
                            {
                                sd->dragSet
                                    = 0
                                      == (bit
                                          & (sd->game.notes[sd->player.curY * sd->game.size + sd->player.curX]
                                             ^ sd->player.notes[sd->player.curY * sd->game.size + sd->player.curX]));
                                sd->dragging = true;
                            }
                            ESP_LOGI("Sudoku", "Drag - first digit was %sset! (noteTaking)", sd->dragSet ? "" : "un");
                        }
                        else
                        {
                            sd->dragSet  = true;
                            sd->dragging = true;
                            ESP_LOGI("Sudoku", "Drag - first digit was set! (else)");
                        }
                        swadgedokuPlayerSetDigit(sd->player.selectedDigit, false, false);
                    }

                    reAnnotate = true;
                }

                break;
            }

            case PB_B:
            {
                sd->player.noteTaking = !sd->player.noteTaking;
                break;
            }

            case PB_SELECT:
            {
                break;
            }

            case PB_START:
            {
                sd->screen = SWADGEDOKU_PAUSE;
                break;
            }

            case PB_UP:
            {
                moved     = true;
                int skips = 0;
                do
                {
                    if (sd->player.curY == 0)
                    {
                        sd->player.curY = sd->game.size - 1;
                    }
                    else
                    {
                        sd->player.curY--;
                    }
                } while (((SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                          || (sd->dragging && sd->player.noteTaking
                              && sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX]))
                         && skips++ < sd->game.size);

                if (sd->dragging)
                {
                    drag = true;
                }
                break;
            }

            case PB_DOWN:
            {
                moved     = true;
                int skips = 0;
                do
                {
                    if (sd->player.curY >= sd->game.size - 1)
                    {
                        sd->player.curY = 0;
                    }
                    else
                    {
                        sd->player.curY++;
                    }
                } while (((SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                          || (sd->dragging && sd->player.noteTaking
                              && sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX]))
                         && skips++ < sd->game.size);

                if (sd->dragging)
                {
                    drag = true;
                }
                break;
            }

            case PB_LEFT:
            {
                moved     = true;
                int skips = 0;
                do
                {
                    if (sd->player.curX == 0)
                    {
                        sd->player.curX = sd->game.size - 1;
                    }
                    else
                    {
                        sd->player.curX--;
                    }
                } while (((SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                          || (sd->dragging && sd->player.noteTaking
                              && sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX]))
                         && skips++ < sd->game.size);

                if (sd->dragging)
                {
                    drag = true;
                }
                break;
            }

            case PB_RIGHT:
            {
                moved     = true;
                int skips = 0;
                do
                {
                    if (sd->player.curX >= sd->game.size - 1)
                    {
                        sd->player.curX = 0;
                    }
                    else
                    {
                        sd->player.curX++;
                    }
                } while (((SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                          || (sd->dragging && sd->player.noteTaking
                              && sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX]))
                         && skips++ < sd->game.size);

                if (sd->dragging)
                {
                    drag = true;
                }
                break;
            }
        }

        if (drag)
        {
            int digit = sd->player.selectedDigit;

            if (!((SF_LOCKED | SF_VOID) & sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
            {
                if (digit)
                {
                    ESP_LOGI("Sudoku", "Dragging to %sset", sd->dragSet ? "" : "un");
                    swadgedokuPlayerSetDigit(digit, true, sd->dragSet);
                }
                else
                {
                    setDigit(&sd->game, 0, sd->player.curX, sd->player.curY);
                }
                reAnnotate = true;
            }
        }

        if (moved && sd->player.cursorShape)
        {
            switch (sd->player.cursorShape->type)
            {
                case OVERLAY_RECT:
                {
                    getOverlayPos(&sd->player.cursorShape->rectangle.pos.x, &sd->player.cursorShape->rectangle.pos.y,
                                  sd->player.curY, sd->player.curX, SUBPOS_NW);
                    break;
                }

                case OVERLAY_CIRCLE:
                {
                    getOverlayPos(&sd->player.cursorShape->circle.pos.x, &sd->player.cursorShape->circle.pos.y,
                                  sd->player.curY, sd->player.curX, SUBPOS_CENTER);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }

        if (moved || reAnnotate)
        {
            sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game, &sd->settings,
                           sd->useSolution ? &sd->solution : NULL);
        }
    }
    else if (evt.button & PB_A)
    {
        sd->dragging = false;
    }
}

static void swadgedokuHintDialogCb(const char* label)
{
    if (label == dialogOptionOk)
    {
        // Apply the hint
        sd->hintsUsed++;
        // setDigit(&sd->game, sd->hint.digit, sd->hint.pos % sd->game.size, sd->hint.pos / sd->game.size);
        applyHint(&sd->game, sd->player.notes, sd->solverCache.hintBuf, sd->solverCache.hintbufLen);
        swadgedokuDoWinCheck();
        resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
        sd->solverCache.solution = sd->useSolution ? sd->solution.grid : NULL;

        // Apply the trophy for using a hint
        trophyUpdate(trophyUseHint, 1, true);
    }
    else if (label == dialogOptionCancel)
    {
        sd->showingHint = false;
        resetSolverCache(&sd->solverCache, sd->game.size, sd->game.base);
        sd->solverCache.solution = sd->useSolution ? sd->solution.grid : NULL;
    }

    node_t* node = sd->player.overlay.shapes.first;
    while (node != NULL)
    {
        sudokuOverlayShape_t* shape = (sudokuOverlayShape_t*)node->val;
        if (shape->tag == ST_HINT)
        {
            node_t* tmp = node;
            node        = node->next;
            removeEntry(&sd->player.overlay.shapes, tmp);
            free(shape);
        }
        else
        {
            node = node->next;
        }
    }

    for (int n = 0; n < sd->game.size * sd->game.size; n++)
    {
        if (sd->player.overlay.gridOpts[n] & OVERLAY_SKIP)
        {
            sd->player.overlay.gridOpts[n] &= ~OVERLAY_SKIP;
        }
    }
    sd->showingHint = false;
}

static sudokuDifficulty_t getLevelDifficulty(int level)
{
    // TODO: Maybe just include the difficulty in the sudoku data?
    if (level < 10)
    {
        return SD_BEGINNER;
    }
    else if (level < 25)
    {
        return SD_EASY;
    }
    else if (level < 40)
    {
        return SD_MEDIUM;
    }
    else
    {
        return SD_HARD;
    }
}

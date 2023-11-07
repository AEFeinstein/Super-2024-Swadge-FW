/**
 * @file breakout.c
 * @author J.Vega (JVeg199X)
 * @brief It's Galactic Brickdown.
 * @date 2023-07-01
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "esp_random.h"
#include "breakout.h"

#include "gameData.h"
#include "tilemap.h"
#include "hdw-nvs.h"
#include "soundManager.h"
#include "entityManager.h"

#include "leveldef.h"
#include "mainMenu.h"

#include "fill.h"
#include "starfield.h"

#include <esp_log.h>

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
typedef void (*gameUpdateFunction_t)(breakout_t* self, int64_t elapsedUs);
struct breakout_t
{
    menu_t* menu;                     ///< The menu structure
    menuLogbookRenderer_t* mRenderer; ///< The menu renderer
    font_t logbook;                   ///< The font used in the menu and game
    font_t ibm_vga8;

    menuItem_t* levelSelectMenuItem;

    gameData_t gameData;
    tilemap_t tilemap;
    entityManager_t entityManager;

    uint16_t btnState;
    uint16_t prevBtnState;

    int32_t frameTimer;

    soundManager_t soundManager;
    starfield_t starfield;

    gameUpdateFunction_t update;

    breakoutHighScores_t highScores;
    uint8_t menuState;
    uint8_t menuSelection;

    breakoutUnlockables_t unlockables;
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void breakoutMainLoop(int64_t elapsedUs);
static void breakoutEnterMode(void);
static void breakoutExitMode(void);

static void breakoutMenuCb(const char* label, bool selected, uint32_t settingVal);
static void breakoutGameLoop(breakout_t* self, int64_t elapsedUs);

static void breakoutBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void drawBreakoutHud(font_t* font, gameData_t* gameData);
static void breakoutUpdateMainMenu(breakout_t* self, int64_t elapsedUs);
static void breakoutChangeStateReadyScreen(breakout_t* self);
static void breakoutUpdateReadyScreen(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawReadyScreen(font_t* logbook, font_t* ibm_vga8, gameData_t* gameData);
static void breakoutChangeStateGame(breakout_t* self);
static void breakoutDetectGameStateChange(breakout_t* self);
static void breakoutDetectBgmChange(breakout_t* self);
static void breakoutChangeStateDead(breakout_t* self);
static void breakoutUpdateDead(breakout_t* self, int64_t elapsedUs);
static void breakoutChangeStateGameOver(breakout_t* self);
static void breakoutUpdateGameOver(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawGameOver(font_t* logbook, font_t* ibm_vga8, gameData_t* gameData);
static void breakoutChangeStateMainMenu(breakout_t* self);
static void breakoutChangeStateLevelClear(breakout_t* self);
static void breakoutUpdateLevelClear(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawLevelClear(font_t* font, gameData_t* gameData);
static void breakoutChangeStateGameClear(breakout_t* self);
static void breakoutUpdateGameClear(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawGameClear(font_t* ibm_vga8, font_t* logbook, gameData_t* gameData);
static void breakoutChangeStateTitleScreen(breakout_t* self);
static void breakoutUpdateTitleScreen(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawTitleScreen(font_t* font, gameData_t* gameData);

static void breakoutInitializeHighScores(breakout_t* self);
static void breakoutLoadHighScores(breakout_t* self);
static void breakoutSaveHighScores(breakout_t* self);

static void breakoutInitializeUnlockables(breakout_t* self);
static void breakoutLoadUnlockables(breakout_t* self);
static void breakoutSaveUnlockables(breakout_t* self);

static void breakoutDrawHighScores(font_t* font, breakoutHighScores_t* highScores, gameData_t* gameData);
uint8_t breakoutGetHighScoreRank(breakoutHighScores_t* highScores, uint32_t newScore);
static void breakoutInsertScoreIntoHighScores(breakoutHighScores_t* highScores, uint32_t newScore, char newInitials[],
                                              uint8_t rank);

static void breakoutChangeStateNameEntry(breakout_t* self);
static void breakoutUpdateNameEntry(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawNameEntry(font_t* font, gameData_t* gameData, uint8_t currentInitial);
static void breakoutChangeStateShowHighScores(breakout_t* self);
static void breakoutUpdateShowHighScores(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawShowHighScores(font_t* font, uint8_t menuState);
static void breakoutChangeStatePause(breakout_t* self);
static void breakoutUpdatePause(breakout_t* self, int64_t elapsedUs);
static void breakoutDrawPause(font_t* font);
uint16_t breakoutGetLevelIndex(uint8_t world, uint8_t level);

void breakoutBuildMainMenu(breakout_t* self);

//==============================================================================
// Level Definitions
//==============================================================================

#define NUM_LEVELS 41

// The index into leveldef[] where the actual game levels start
// As opposed to utility levels like titlescreen, debug, etc.
#define GAME_LEVEL_START_INDEX 1

static const leveldef_t leveldef[NUM_LEVELS]
    = {{.filename = "titlescreen.bin", .timeLimit = 180},
       {.filename = "intro.bin", .timeLimit = 180},
       {.filename = "rightside.bin", .timeLimit = 180},
       {.filename = "upsidedown.bin", .timeLimit = 180},
       {.filename = "leftside.bin", .timeLimit = 180},
       {.filename = "split.bin", .timeLimit = 180},
       {.filename = "mag01.bin", .timeLimit = 180},
       {.filename = "mag02.bin", .timeLimit = 180},
       {.filename = "m_attack.bin", .timeLimit = 180},
       {.filename = "flower.bin", .timeLimit = 180},
       {.filename = "brkLvlChar1.bin", .timeLimit = 180},
       {.filename = "gaylordlogo.bin", .timeLimit = 180},
       {.filename = "trifecta.bin", .timeLimit = 180},
       {.filename = "xmarks.bin", .timeLimit = 180},
       {.filename = "bombtest.bin", .timeLimit = 180},
       {.filename = "devito.bin", .timeLimit = 180},
       {.filename = "lumberjacks.bin", .timeLimit = 180},
       {.filename = "halloween.bin", .timeLimit = 180},
       {.filename = "snake.bin", .timeLimit = 180},
       {.filename = "flipflop.bin", .timeLimit = 180},
       {.filename = "ponglike.bin", .timeLimit = 180},
       {.filename = "tinyhuge.bin", .timeLimit = 180},
       {.filename = "superhard.bin", .timeLimit = 180},
       {.filename = "firework.bin", .timeLimit = 180},
       {.filename = "starlite.bin", .timeLimit = 180},
       {.filename  = "jailbreak.bin", /*Starting here, the levels are NOT in order!*/
        .timeLimit = 180},
       {.filename = "getMorGet.bin", .timeLimit = 180},
       {.filename = "outtaMyWay.bin", .timeLimit = 180},
       {.filename = "bombrings.bin", .timeLimit = 180},
       {.filename = "angles.bin", .timeLimit = 180},
       {.filename = "themaze.bin", .timeLimit = 180},
       {.filename = "intersection.bin", .timeLimit = 180},
       {.filename = "foosball.bin", .timeLimit = 180},
       {.filename = "paddles.bin", .timeLimit = 180},
       {.filename = "corner.bin", .timeLimit = 180},
       {.filename = "b.bin", .timeLimit = 180},
       {.filename = "wtf.bin", .timeLimit = 180},
       {.filename = "mag03.bin", .timeLimit = 180},
       {.filename = "wallball.bin", .timeLimit = 180},
       {.filename = "introenemy.bin", .timeLimit = 180},
       {.filename = "stormcastle.bin", .timeLimit = 180}};

//==============================================================================
// Look Up Tables
//==============================================================================

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};
static const paletteColor_t redColors[4]               = {c510, c440, c050, c440};
static const paletteColor_t greenColors[4]             = {c555, c051, c030, c051};
static const paletteColor_t purpleColors[4]            = {c405, c440, c055, c440};

static const int16_t cheatCode[9] = {PB_UP, PB_B, PB_DOWN, PB_B, PB_LEFT, PB_B, PB_RIGHT, PB_B, PB_START};

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char breakoutName[]           = "Galactic Brickdown";
static const char breakoutTitleGalactic[]  = "Galactic";
static const char breakoutTitleBrickdown[] = "Brickdown";
static const char breakoutPressStart[]     = "Press Start";

static const char breakoutNewGame[]       = "New Game";
static const char breakoutContinue[]      = "Continue - Lv";
static const char breakoutHighScores[]    = "High Scores";
static const char breakoutResetScores[]   = "Reset Scores";
static const char breakoutResetProgress[] = "Reset Progress";
static const char breakoutExit[]          = "Exit";
static const char breakoutSaveAndExit[]   = "Save & Exit";

static const char breakoutReady[]    = "Get Ready!";
static const char breakoutGameOver[] = "Game Over!";

static const char breakoutLevelClear[] = "Cleared!";
static const char breakoutPause[]      = "Paused";

static const char breakoutHighScoreDisplayTitle[] = "Cosmic Scores";
static const char breakoutNameEntryTitle[]        = "Enter your initials!";
static const char breakoutNameEnteredTitle[]      = "Name registrated.";

static const char breakoutNvsKey_scores[]  = "brk_scores";
static const char breakoutNvsKey_unlocks[] = "brk_unlocks";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Pong
swadgeMode_t breakoutMode = {
    .modeName                 = breakoutName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = breakoutEnterMode,
    .fnExitMode               = breakoutExitMode,
    .fnMainLoop               = breakoutMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = breakoutBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Pong mode. This whole struct is calloc()'d and free()'d so that Pong is only
/// using memory while it is being played
breakout_t* breakout = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Pong mode, allocate required memory, and initialize required variables
 *
 */
static void breakoutEnterMode(void)
{
    breakout = calloc(1, sizeof(breakout_t));

    // Load a font
    loadFont("logbook.font", &breakout->logbook, false);
    loadFont("ibm_vga8.font", &breakout->ibm_vga8, false);

    breakout->mRenderer = initMenuLogbookRenderer(&breakout->logbook);

    initializeGameData(&(breakout->gameData), &(breakout->soundManager));
    initializeTileMap(&(breakout->tilemap));
    initializeSoundManager(&(breakout->soundManager));
    initializeEntityManager(&(breakout->entityManager), &(breakout->tilemap), &(breakout->gameData),
                            &(breakout->soundManager));
    initializeStarfield(&(breakout->starfield), false);

    breakout->tilemap.entityManager       = &(breakout->entityManager);
    breakout->tilemap.executeTileSpawnAll = true;
    breakout->tilemap.mapOffsetX          = 0;

    // loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);

    breakoutLoadHighScores(breakout);
    breakoutLoadUnlockables(breakout);

    // Set frame rate to 60 FPS
    setFrameRateUs(16666);

    breakout->menu = NULL;
    breakoutChangeStateTitleScreen(breakout);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void breakoutExitMode(void)
{
    // deinitMenu can't set menu pointer to NULL,
    // so this is the only way to know that the menu has not been previously freed.
    if (breakout->update == &breakoutUpdateMainMenu)
    {
        // Deinitialize the menu.
        // This will also free the "level select" menu item.
        deinitMenu(breakout->menu);
    }
    deinitMenuLogbookRenderer(breakout->mRenderer);

    // Free the fonts
    freeFont(&breakout->logbook);
    freeFont(&breakout->ibm_vga8);

    freeTilemap(&breakout->tilemap);
    freeSoundManager(&breakout->soundManager);
    freeEntityManager(&breakout->entityManager);

    // Free everything else
    free(breakout);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void breakoutMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == breakoutNewGame)
        {
            initializeGameDataFromTitleScreen(&(breakout->gameData));
            deactivateAllEntities(&(breakout->entityManager), false, false, false);
            breakout->gameData.level = 1;
            loadMapFromFile(&(breakout->tilemap), leveldef[GAME_LEVEL_START_INDEX].filename);
            breakout->gameData.countdown = leveldef[GAME_LEVEL_START_INDEX].timeLimit;
            breakoutChangeStateReadyScreen(breakout);
            deinitMenu(breakout->menu);
        }
        else if (label == breakoutContinue)
        {
            initializeGameDataFromTitleScreen(&(breakout->gameData));
            deactivateAllEntities(&(breakout->entityManager), false, false, false);
            breakout->gameData.level = settingVal;
            loadMapFromFile(&(breakout->tilemap), leveldef[breakout->gameData.level].filename);
            breakout->gameData.countdown = leveldef[breakout->gameData.level].timeLimit;
            breakoutChangeStateReadyScreen(breakout);
            deinitMenu(breakout->menu);
        }
        else if (label == breakoutHighScores)
        {
            breakoutChangeStateShowHighScores(breakout);
            breakout->gameData.btnState = 0;
            deinitMenu(breakout->menu);
        }
        else if (label == breakoutResetScores)
        {
            breakoutInitializeHighScores(breakout);
            bzrPlaySfx(&(breakout->soundManager.detonate), BZR_STEREO);
        }
        else if (label == breakoutResetProgress)
        {
            breakoutInitializeHighScores(breakout);
            bzrPlaySfx(&(breakout->soundManager.die), BZR_STEREO);
        }
        else if (label == breakoutSaveAndExit)
        {
            breakoutSaveHighScores(breakout);
            breakoutSaveUnlockables(breakout);
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == breakoutExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        bzrPlaySfx(&(breakout->soundManager.hit3), BZR_STEREO);
    }
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void breakoutMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        breakout->btnState          = evt.state;
        breakout->gameData.btnState = evt.state;

        if (breakout->update == &breakoutUpdateMainMenu)
        {
            // Pass button events to the menu
            breakout->menu = menuButton(breakout->menu, evt);
        }
    }

    breakout->update(breakout, elapsedUs);

    breakout->prevBtnState          = breakout->btnState;
    breakout->gameData.prevBtnState = breakout->prevBtnState;
}

void breakoutChangeStateMainMenu(breakout_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &breakoutUpdateMainMenu;
    breakoutBuildMainMenu(breakout);
}

static void breakoutUpdateMainMenu(breakout_t* self, int64_t elapsedUs)
{
    // Draw the menu
    drawMenuLogbook(breakout->menu, breakout->mRenderer, elapsedUs);
}

static void breakoutChangeStateReadyScreen(breakout_t* self)
{
    self->gameData.frameCount = 0;

    // Set up Cho Intro
    deactivateAllEntities(&(self->entityManager), false, true, true);
    self->gameData.ballsInPlay = 0;
    forceTileSpawnEntitiesWithinView(&(self->tilemap));

    self->entityManager.playerEntity = NULL;
    self->entityManager.playerEntity = createEntity(&(self->entityManager), ENTITY_CHO_INTRO, 0, 0);

    self->update = &breakoutUpdateReadyScreen;
}

static void breakoutUpdateReadyScreen(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    updateLedsInGame(&(self->gameData));
    drawStarfield(&(self->starfield));

    if (self->entityManager.playerEntity != NULL && self->entityManager.playerEntity->active)
    {
        self->entityManager.playerEntity->updateFunction(self->entityManager.playerEntity);

        if (self->gameData.targetBlocksBroken > 0)
        {
            drawTileMap(&(self->tilemap));
            updateStarfield(&(self->starfield), 1);
        }
    }
    else
    {
        if (!(self->gameData.frameCount % 60))
        {
            breakoutChangeStateGame(self);
            setLevelBgm(&(self->soundManager), BRK_BGM_SKILL);
            bzrPlayBgm(&(self->soundManager.levelBgm), BZR_STEREO);
        }

        if (self->gameData.targetBlocksBroken > 0 || !(self->gameData.frameCount % 2))
        {
            drawTileMap(&(self->tilemap));
        }

        updateStarfield(&(self->starfield), 1);
    }

    drawEntities(&(self->entityManager));
    breakoutDrawReadyScreen(&(self->logbook), &(self->ibm_vga8), &(self->gameData));
}

static void breakoutDrawReadyScreen(font_t* logbook, font_t* ibm_vga8, gameData_t* gameData)
{
    drawBreakoutHud(ibm_vga8, gameData);
    drawText(logbook, c555, breakoutReady, (TFT_WIDTH - textWidth(logbook, breakoutReady)) >> 1, 128);
}

static void breakoutChangeStateGame(breakout_t* self)
{
    self->gameData.frameCount             = 0;
    self->gameData.playerTimeBombsCount   = 0;
    self->gameData.playerRemoteBombPlaced = false;
    // deactivateAllEntities(&(self->entityManager), false, true);
    // self->tilemap.executeTileSpawnAll = true;

    // TODO: State change functions should probably always reset this.
    self->gameData.changeState = 0;
    self->gameData.gameState   = ST_GAME;
    self->update               = &breakoutGameLoop;
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void breakoutGameLoop(breakout_t* self, int64_t elapsedUs)
{
    updateTouchInput(&(self->gameData));

    if (((breakout->gameData.btnState & PB_START) && !(breakout->gameData.prevBtnState & PB_START)))
    {
        breakout->gameData.changeState = ST_PAUSE;
    }

    updateLedsInGame(&(self->gameData));
    breakoutDetectGameStateChange(self);
    updateEntities(&(self->entityManager));

    updateStarfield(&(self->starfield), 5);

    // Draw the field
    drawStarfield(&(self->starfield));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(breakout->gameData));

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;

        if (self->gameData.countdown > 0)
        {
            self->gameData.countdown--;
        }

        self->gameData.inGameTimer++;
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void breakoutBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c000);
}

void breakoutDetectGameStateChange(breakout_t* self)
{
    if (!self->gameData.changeState)
    {
        return;
    }

    switch (self->gameData.changeState)
    {
        case ST_GAME:
            breakoutChangeStateGame(self);
            return;

        case ST_DEAD:
            breakoutChangeStateDead(self);
            break;

        case ST_READY_SCREEN:
            breakoutChangeStateReadyScreen(self);
            break;

        case ST_LEVEL_CLEAR:
            breakoutChangeStateLevelClear(self);
            break;

        case ST_PAUSE:
            breakoutChangeStatePause(self);
            break;

        default:
            break;
    }

    self->gameData.changeState = 0;
}

void breakoutChangeStateDead(breakout_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    // self->gameData.levelDeaths++;
    self->gameData.combo = 0;
    // self->gameData.initialHp = 1;

    // buzzer_stop();
    // buzzer_play_bgm(&sndDie);

    self->update = &breakoutUpdateDead;
}

void breakoutUpdateDead(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        resetGameDataLeds(&(self->gameData));
        if (self->gameData.lives > 0)
        {
            breakoutChangeStateReadyScreen(self);
            return;
        }
        else
        {
            breakoutChangeStateGameOver(self);
            return;
        }
    }

    updateLedsInGame(&(self->gameData));
    updateEntities(&(self->entityManager));

    updateStarfield(&(self->starfield), 5);
    drawStarfield(&(self->starfield));

    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));

    /*if(self->gameData.countdown < 0){
        drawText(self->disp, &(self->radiostars), c555, str_time_up, (self->disp->w - textWidth(&(self->radiostars),
    str_time_up)) / 2, 128);
    }*/
}

void breakoutChangeStateGameOver(breakout_t* self)
{
    self->gameData.frameCount = 0;
    resetGameDataLeds(&(self->gameData));
    // buzzer_play_bgm(&bgmGameOver);
    self->update = &breakoutUpdateGameOver;
}

void breakoutUpdateGameOver(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        /*//Handle unlockables

        if(self->gameData.score >= BIG_SCORE) {
            self->unlockables.bigScore = true;
        }

        if(self->gameData.score >= BIGGER_SCORE) {
            self->unlockables.biggerScore = true;
        }

        if(!self->gameData.debugMode){
            savePlatformerUnlockables(self);
        }
        */
        deactivateAllEntities(&(self->entityManager), false, false, false);
        breakoutChangeStateNameEntry(self);
    }

    breakoutDrawGameOver(&(self->logbook), &(self->ibm_vga8), &(self->gameData));
    // updateLedsGameOver(&(self->gameData));
}

void breakoutDrawGameOver(font_t* logbook, font_t* ibm_vga8, gameData_t* gameData)
{
    drawBreakoutHud(ibm_vga8, gameData);
    drawText(logbook, c555, breakoutGameOver, (TFT_WIDTH - textWidth(logbook, breakoutGameOver)) / 2, 128);
}

static void drawBreakoutHud(font_t* font, gameData_t* gameData)
{
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%06" PRIu32, gameData->score);

    char levelStr[15];
    snprintf(levelStr, sizeof(levelStr) - 1, "Level %d", gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%d", gameData->lives);

    // char timeStr[10];
    // snprintf(timeStr, sizeof(timeStr) - 1, "T:%03d", gameData->countdown);

    if (gameData->frameCount > 29)
    {
        drawText(font, c500, "1UP", 24, 2);
    }

    drawText(font, c555, livesStr, 48, 2);
    // drawText(font, c555, coinStr, 160, 16);
    drawText(font, c555, scoreStr, 80, 2);
    drawText(font, c555, levelStr, 184, 2);
    // drawText(d, font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 220,
    // 16);

    drawText(font, c555, "B", 271, 32);
    drawText(font, c555, "O", 271, 44);
    drawText(font, c555, "N", 271, 56);
    drawText(font, c555, "U", 271, 68);
    drawText(font, c555, "S", 271, 80);
    drawRect(271, 96, 279, 96 + (gameData->countdown >> 1), c555);

    // if(gameData->comboTimer == 0){
    //     return;
    // }

    // snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    snprintf(scoreStr, sizeof(scoreStr) - 1, "x%d", gameData->combo);
    drawText(font, /*(gameData->comboTimer < 60) ? c030:*/ greenColors[(breakout->gameData.frameCount >> 3) % 4],
             scoreStr, 144, 2);

    // Draw centering lines, for paddle control debug
    // drawLine(TFT_WIDTH >> 1, 0, TFT_WIDTH >> 1, TFT_HEIGHT, c500, 0);
    // drawLine(0, (TFT_HEIGHT >> 1)+8, TFT_WIDTH, (TFT_HEIGHT >> 1)+8, c005, 0);
}

void breakoutChangeStateLevelClear(breakout_t* self)
{
    self->gameData.frameCount = 0;
    resetGameDataLeds(&(self->gameData));
    self->update = &breakoutUpdateLevelClear;
    bzrPlaySfx(&(self->soundManager.levelClear), BZR_STEREO);
}

void breakoutUpdateLevelClear(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;
    self->gameData.targetBlocksBroken = 0;

    if (self->gameData.frameCount > 60)
    {
        if (self->gameData.countdown > 0)
        {
            self->gameData.countdown--;

            if (self->gameData.countdown % 2)
            {
                bzrPlayBgm(&(self->soundManager.tally), BZR_STEREO);
            }

            scorePoints(&(self->gameData), 80, -1);
        }
        else if (self->gameData.frameCount % 60 == 0)
        {
            // Hey look, it's a frame rule!
            deactivateAllEntities(&(self->entityManager), false, false, false);

            uint16_t levelIndex = self->gameData.level;

            if (levelIndex >= NUM_LEVELS - 1)
            {
                // Game Cleared!

                // if(!self->gameData.debugMode){
                // Determine achievements
                /*self->unlockables.gameCleared = true;

                if(!self->gameData.continuesUsed){
                    self->unlockables.oneCreditCleared = true;

                    if(self->gameData.inGameTimer < FAST_TIME) {
                        self->unlockables.fastTime = true;
                    }
                }

                if(self->gameData.score >= BIG_SCORE) {
                    self->unlockables.bigScore = true;
                }

                if(self->gameData.score >= BIGGER_SCORE) {
                    self->unlockables.biggerScore = true;
                }
            }*/

                breakoutChangeStateGameClear(self);
                return;
            }
            else
            {
                // Advance to the next level
                self->gameData.level++;

                // Unlock the next level
                levelIndex++;
                if (levelIndex > self->unlockables.maxLevelIndexUnlocked)
                {
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }
                loadMapFromFile(&(breakout->tilemap), leveldef[levelIndex].filename);
                breakout->gameData.countdown = leveldef[levelIndex].timeLimit;
                breakoutChangeStateReadyScreen(self);
                if (!self->gameData.debugMode)
                {
                    breakoutSaveUnlockables(self);
                }

                return;
            }
        }
    }
    else if (self->gameData.frameCount < 30 || !(self->gameData.frameCount % 2))
    {
        drawTileMap(&(self->tilemap));
    }

    updateEntities(&(self->entityManager));

    drawStarfield(&(self->starfield));
    // drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawLevelClear(&(self->logbook), &(self->gameData));
    updateLedsLevelClear(&(self->gameData));
}

void breakoutDrawLevelClear(font_t* font, gameData_t* gameData)
{
    drawText(font, c555, breakoutLevelClear, (TFT_WIDTH - textWidth(font, breakoutLevelClear)) / 2, 128);
}

void breakoutChangeStateGameClear(breakout_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &breakoutUpdateGameClear;
    resetGameDataLeds(&(self->gameData));
    // buzzer_play_bgm(&bgmSmooth);
}

void breakoutUpdateGameClear(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if (self->gameData.frameCount > 450)
    {
        if (self->gameData.lives > 0)
        {
            if (self->gameData.frameCount % 60 == 0)
            {
                self->gameData.lives--;
                self->gameData.score += 200000;
                // buzzer_play_sfx(&snd1up);
            }
        }
        else if (self->gameData.frameCount % 960 == 0)
        {
            breakoutChangeStateGameOver(self);
        }
    }

    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawGameClear(&(self->ibm_vga8), &(self->logbook), &(self->gameData));
    updateLedsGameClear(&(self->gameData));
}

void breakoutDrawGameClear(font_t* ibm_vga8, font_t* logbook, gameData_t* gameData)
{
    drawBreakoutHud(ibm_vga8, gameData);

    drawText(logbook, c555, "Thanks for playing!", 16, 48);

    if (gameData->frameCount > 300)
    {
        drawText(logbook, c555, "See you next", 8, 112);
        drawText(logbook, c555, "debug mission!", 8, 160);
    }
}

void breakoutChangeStatePause(breakout_t* self)
{
    // buzzer_play_bgm(&sndPause);
    self->gameData.btnState = 0;
    self->update            = &breakoutUpdatePause;
}

void breakoutUpdatePause(breakout_t* self, int64_t elapsedUs)
{
    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        // buzzer_play_sfx(&sndPause);
        // self->gameData.changeBgm = self->gameData.currentBgm;
        // self->gameData.currentBgm = BGM_NULL;
        self->gameData.btnState = 0;
        self->update            = &breakoutGameLoop;
    }

    drawStarfield(&(self->starfield));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawPause(&(self->logbook));
}

void breakoutDrawPause(font_t* font)
{
    drawText(font, c555, breakoutPause, (TFT_WIDTH - textWidth(font, breakoutPause)) / 2, 128);
}

uint16_t breakoutGetLevelIndex(uint8_t world, uint8_t level)
{
    return (world - 1) * 4 + (level - 1);
}

static void breakoutChangeStateTitleScreen(breakout_t* self)
{
    self->gameData.frameCount = 0;

    self->update = &breakoutUpdateTitleScreen;

    if (self->gameData.gameState != ST_TITLE_SCREEN)
    {
        deactivateAllEntities(&(self->entityManager), false, false, false);
        loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);
        createEntity(&(self->entityManager), ENTITY_CAPTIVE_BALL, 24 + esp_random() % 232, 24 + esp_random() % 216);
        createEntity(&(self->entityManager), ENTITY_CAPTIVE_BALL, 24 + esp_random() % 232, 24 + esp_random() % 216);
        createEntity(&(self->entityManager), ENTITY_CAPTIVE_BALL, 24 + esp_random() % 232, 24 + esp_random() % 216);
    }

    self->gameData.gameState = ST_TITLE_SCREEN;
}

static void breakoutUpdateTitleScreen(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if (self->gameData.frameCount > 600)
    {
        // resetGameDataLeds(&(self->gameData));
        breakout->menuSelection = 0;
        breakoutChangeStateShowHighScores(self);

        return;
    }

    if ((self->gameData.btnState & cheatCode[breakout->menuSelection])
        && !(self->gameData.prevBtnState & cheatCode[breakout->menuSelection]))
    {
        breakout->menuSelection++;

        if (breakout->menuSelection > 8)
        {
            breakout->menuSelection      = 0;
            breakout->menuState          = 1;
            breakout->gameData.debugMode = true;
            bzrPlaySfx(&(breakout->soundManager.snd1up), BZR_STEREO);
        }
        else
        {
            bzrPlaySfx(&(breakout->soundManager.hit3), BZR_STEREO);
        }
    }

    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        self->gameData.btnState = 0;
        breakout->menuSelection = 0;

        if (!breakout->gameData.debugMode)
        {
            bzrPlaySfx(&(breakout->soundManager.launch), BZR_STEREO);
        }

        breakoutChangeStateMainMenu(self);
        return;
    }

    /*
    if(!(self->gameData.frameCount % 180)) {
         createEntity(&(self->entityManager), ENTITY_CAPTIVE_BALL, 24 + esp_random() % 232, 24 + esp_random() % 216);
    }
    */

    updateStarfield(&(self->starfield), 8);
    updateEntities(&(self->entityManager));
    drawStarfield(&(self->starfield));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    breakoutDrawTitleScreen(&(self->logbook), &(self->gameData));
    updateLedsTitleScreen(&(self->gameData));
}

static void breakoutDrawTitleScreen(font_t* font, gameData_t* gameData)
{
    drawText(font, c000, breakoutTitleGalactic, 52, 100);
    drawText(font, c000, breakoutTitleBrickdown, 100, 124);
    drawText(font, purpleColors[(breakout->gameData.frameCount >> 3) % 4], breakoutTitleGalactic, 48, 96);
    drawText(font, redColors[(breakout->gameData.frameCount >> 3) % 4], breakoutTitleBrickdown, 96, 120);

    if ((gameData->frameCount % 60) < 30)
    {
        drawText(font, c555, breakoutPressStart, (TFT_WIDTH - textWidth(font, breakoutPressStart)) >> 1, 192);
    }
}

void breakoutInitializeHighScores(breakout_t* self)
{
    self->highScores.scores[0] = 100000;
    self->highScores.scores[1] = 80000;
    self->highScores.scores[2] = 40000;
    self->highScores.scores[3] = 20000;
    self->highScores.scores[4] = 10000;

    for (uint8_t i = 0; i < NUM_BREAKOUT_HIGH_SCORES; i++)
    {
        self->highScores.initials[i][0] = 'J' + i;
        self->highScores.initials[i][1] = 'P' - i;
        self->highScores.initials[i][2] = 'V' + i;
    }
}

void breakoutLoadHighScores(breakout_t* self)
{
    size_t size = sizeof(breakoutHighScores_t);
    // Try reading the value
    if (false == readNvsBlob(breakoutNvsKey_scores, &(self->highScores), &(size)))
    {
        // Value didn't exist, so write the default
        breakoutInitializeHighScores(self);
    }
}

void breakoutSaveHighScores(breakout_t* self)
{
    size_t size = sizeof(breakoutHighScores_t);
    writeNvsBlob(breakoutNvsKey_scores, &(self->highScores), size);
}

void breakoutDrawHighScores(font_t* font, breakoutHighScores_t* highScores, gameData_t* gameData)
{
    drawText(font, redColors[(gameData->frameCount >> 3) % 4], "Rank Score  Name", 10, 88);
    for (uint8_t i = 0; i < NUM_BREAKOUT_HIGH_SCORES; i++)
    {
        char rowStr[32];
        snprintf(rowStr, sizeof(rowStr) - 1, " %d   %06" PRIu32 " %c%c%c", i + 1, highScores->scores[i],
                 highScores->initials[i][0], highScores->initials[i][1], highScores->initials[i][2]);
        drawText(font, (gameData->rank == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 16, 120 + i * 22);
    }
}

uint8_t breakoutGetHighScoreRank(breakoutHighScores_t* highScores, uint32_t newScore)
{
    uint8_t i;
    for (i = 0; i < NUM_BREAKOUT_HIGH_SCORES; i++)
    {
        if (highScores->scores[i] < newScore)
        {
            break;
        }
    }

    return i;
}

void breakoutInsertScoreIntoHighScores(breakoutHighScores_t* highScores, uint32_t newScore, char newInitials[],
                                       uint8_t rank)
{
    if (rank >= NUM_BREAKOUT_HIGH_SCORES)
    {
        return;
    }

    for (uint8_t i = NUM_BREAKOUT_HIGH_SCORES - 1; i > rank; i--)
    {
        highScores->scores[i]      = highScores->scores[i - 1];
        highScores->initials[i][0] = highScores->initials[i - 1][0];
        highScores->initials[i][1] = highScores->initials[i - 1][1];
        highScores->initials[i][2] = highScores->initials[i - 1][2];
    }

    highScores->scores[rank]      = newScore;
    highScores->initials[rank][0] = newInitials[0];
    highScores->initials[rank][1] = newInitials[1];
    highScores->initials[rank][2] = newInitials[2];
}

//--------------

void breakoutChangeStateShowHighScores(breakout_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &breakoutUpdateShowHighScores;
}

void breakoutUpdateShowHighScores(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if ((self->gameData.frameCount > 300)
        || (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
            || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))))
    {
        self->menuState     = 0;
        self->menuSelection = 0;
        bzrStop(true);
        breakoutChangeStateTitleScreen(self);
    }

    updateStarfield(&(self->starfield), 8);
    drawStarfield(&(self->starfield));

    breakoutDrawShowHighScores(&(self->logbook), self->menuState);
    breakoutDrawHighScores(&(self->logbook), &(self->highScores), &(self->gameData));

    updateLedsTitleScreen(&(self->gameData));
    // updateLedsShowHighScores(&(self->gameData));
}

void breakoutDrawShowHighScores(font_t* font, uint8_t menuState)
{
    /*if(platformer->easterEgg){
        drawText(font, highScoreNewEntryColors[(platformer->gameData.frameCount >> 3) % 4], str_hbd, (TFT_WIDTH -
    textWidth(font, str_hbd)) / 2, 32); } else*/
    if (menuState == 3)
    {
        drawText(font, redColors[(breakout->gameData.frameCount >> 3) % 4], breakoutNameEnteredTitle,
                 (TFT_WIDTH - textWidth(font, breakoutNameEnteredTitle)) / 2, 32);
    }
    else
    {
        drawText(font, purpleColors[(breakout->gameData.frameCount >> 3) % 4], breakoutHighScoreDisplayTitle,
                 (TFT_WIDTH - textWidth(font, breakoutHighScoreDisplayTitle)) / 2, 32);
    }
}

void breakoutChangeStateNameEntry(breakout_t* self)
{
    self->gameData.frameCount = 0;
    uint8_t rank              = breakoutGetHighScoreRank(&(self->highScores), self->gameData.score);
    self->gameData.rank       = rank;
    self->menuState           = 0;

    resetGameDataLeds(&(self->gameData));

    if (rank >= NUM_BREAKOUT_HIGH_SCORES || self->gameData.debugMode)
    {
        self->menuSelection = 0;
        self->gameData.rank = NUM_BREAKOUT_HIGH_SCORES;
        breakoutChangeStateShowHighScores(self);
        return;
    }

    // bzrPlayBgm(&(self->soundManager.bgmNameEntry), BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update        = &breakoutUpdateNameEntry;
}

void breakoutUpdateNameEntry(breakout_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if (self->gameData.btnState & PB_LEFT && !(self->gameData.prevBtnState & PB_LEFT))
    {
        self->menuSelection--;

        if (self->menuSelection < 32)
        {
            self->menuSelection = 90;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        bzrPlaySfx(&(self->soundManager.hit3), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_RIGHT && !(self->gameData.prevBtnState & PB_RIGHT))
    {
        self->menuSelection++;

        if (self->menuSelection > 90)
        {
            self->menuSelection = 32;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        bzrPlaySfx(&(self->soundManager.hit3), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_B && !(self->gameData.prevBtnState & PB_B))
    {
        if (self->menuState > 0)
        {
            self->menuState--;
            self->menuSelection = self->gameData.initials[self->menuState];
            bzrPlaySfx(&(self->soundManager.hit3), BZR_STEREO);
        }
        else
        {
            bzrPlaySfx(&(self->soundManager.hit2), BZR_STEREO);
        }
    }
    else if (self->gameData.btnState & PB_A && !(self->gameData.prevBtnState & PB_A))
    {
        self->menuState++;

        if (self->menuState > 2)
        {
            breakoutInsertScoreIntoHighScores(&(self->highScores), self->gameData.score, self->gameData.initials,
                                              self->gameData.rank);
            breakoutSaveHighScores(self);
            breakoutChangeStateShowHighScores(self);
            bzrPlaySfx(&(self->soundManager.snd1up), BZR_STEREO);
            return;
        }
        else
        {
            self->menuSelection = self->gameData.initials[self->menuState];
            bzrPlaySfx(&(self->soundManager.hit3), BZR_STEREO);
        }
    }

    updateStarfield(&(self->starfield), 8);
    drawStarfield(&(self->starfield));
    breakoutDrawNameEntry(&(self->logbook), &(self->gameData), self->menuState);
    updateLedsShowHighScores(&(self->gameData));
}

void breakoutDrawNameEntry(font_t* font, gameData_t* gameData, uint8_t currentInitial)
{
    drawText(font, greenColors[(breakout->gameData.frameCount >> 3) % 4], breakoutNameEntryTitle,
             (TFT_WIDTH - textWidth(font, breakoutNameEntryTitle)) / 2, 64);

    char rowStr[32];
    snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32, gameData->rank + 1, gameData->score);
    drawText(font, c555, rowStr, 16, 128);

    for (uint8_t i = 0; i < 3; i++)
    {
        snprintf(rowStr, sizeof(rowStr) - 1, "%c", gameData->initials[i]);
        drawText(font, (currentInitial == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 200 + 16 * i, 128);
    }
}

void breakoutInitializeUnlockables(breakout_t* self)
{
    self->unlockables.maxLevelIndexUnlocked = 1;
    self->unlockables.gameCleared           = false;
    /*self->unlockables.oneCreditCleared = false;
    self->unlockables.bigScore = false;
    self->unlockables.fastTime = false;
    self->unlockables.biggerScore = false;*/
}

void breakoutLoadUnlockables(breakout_t* self)
{
    size_t size = sizeof(breakoutUnlockables_t);
    // Try reading the value
    if (false == readNvsBlob(breakoutNvsKey_unlocks, &(self->unlockables), &(size)))
    {
        // Value didn't exist, so write the default
        breakoutInitializeUnlockables(self);
    }
}

void breakoutSaveUnlockables(breakout_t* self)
{
    size_t size = sizeof(breakoutUnlockables_t);
    writeNvsBlob(breakoutNvsKey_unlocks, &(self->unlockables), size);
}

void breakoutBuildMainMenu(breakout_t* self)
{
    // Initialize the menu
    breakout->menu = initMenu(breakoutName, breakoutMenuCb);
    addSingleItemToMenu(breakout->menu, breakoutNewGame);

    /*
        Manually allocate and build "level select" menu item
        because the max setting will have to change as levels are unlocked
    */
    if (breakout->unlockables.maxLevelIndexUnlocked > 0 || breakout->gameData.debugMode)
    {
        breakout->levelSelectMenuItem             = calloc(1, sizeof(menuItem_t));
        breakout->levelSelectMenuItem->label      = breakoutContinue;
        breakout->levelSelectMenuItem->minSetting = 1;
        breakout->levelSelectMenuItem->maxSetting
            = (breakout->gameData.debugMode) ? NUM_LEVELS - 1 : breakout->unlockables.maxLevelIndexUnlocked;
        breakout->levelSelectMenuItem->currentSetting
            = (breakout->gameData.level == 0) ? breakout->levelSelectMenuItem->maxSetting : breakout->gameData.level;
        breakout->levelSelectMenuItem->options = NULL;
        breakout->levelSelectMenuItem->subMenu = NULL;

        push(breakout->menu->items, breakout->levelSelectMenuItem);
    }

    addSingleItemToMenu(breakout->menu, breakoutHighScores);

    if (breakout->gameData.debugMode)
    {
        addSingleItemToMenu(breakout->menu, breakoutResetScores);
        addSingleItemToMenu(breakout->menu, breakoutResetProgress);
        addSingleItemToMenu(breakout->menu, breakoutSaveAndExit);
    }
    else
    {
        addSingleItemToMenu(breakout->menu, breakoutExit);
    }
}
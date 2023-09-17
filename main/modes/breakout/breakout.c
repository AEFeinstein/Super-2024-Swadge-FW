/**
 * @file breakout.c
 * @author J.Vega (JVeg199X)
 * @brief It's Galactic Breakdown.
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
#include "soundManager.h"
#include "entityManager.h"

#include "leveldef.h"
#include "mainMenu.h"

#include <esp_log.h>

//==============================================================================
// Defines
//==============================================================================

/// Physics math is done with fixed point numbers where the bottom four bits are the fractional part. It's like Q28.4
#define DECIMAL_BITS 4

#define BALL_RADIUS   (5 << DECIMAL_BITS)
#define PADDLE_WIDTH  (8 << DECIMAL_BITS)
#define PADDLE_HEIGHT (40 << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)

#define SPEED_LIMIT (30 << DECIMAL_BITS)

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum of screens that may be shown in breakout mode
 */
typedef enum
{
    BREAKOUT_MENU,
    BREAKOUT_GAME,
} breakoutScreen_t;

//==============================================================================
// Structs
//==============================================================================
typedef void (*gameUpdateFunction_t)(void *self, int64_t elapsedUs);
typedef struct 
{
    menu_t* menu; ///< The menu structure
    menuLogbookRenderer_t* mRenderer; ///< The menu renderer
    font_t logbook;   ///< The font used in the menu and game
    font_t ibm_vga8;

    menuItem_t* levelSelectMenuItem;
    
    gameData_t gameData;
    tilemap_t tilemap;
    entityManager_t entityManager;

    uint16_t btnState;
    uint16_t prevBtnState;

    int32_t frameTimer;

    soundManager_t soundManager;

    gameUpdateFunction_t update;
} breakout_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void breakoutMainLoop(int64_t elapsedUs);
static void breakoutEnterMode(void);
static void breakoutExitMode(void);

static void breakoutMenuCb(const char* label, bool selected, uint32_t settingVal);
static void breakoutGameLoop(breakout_t *self, int64_t elapsedUs);

static void breakoutBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void drawBreakoutHud(font_t *font, gameData_t *gameData);
static void breakoutUpdateTitleScreen(breakout_t *self, int64_t elapsedUs);
static void drawBreakoutTitleScreen(font_t *font, gameData_t *gameData);
static void breakoutChangeStateReadyScreen(breakout_t *self);
static void breakoutUpdateReadyScreen(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawReadyScreen(font_t *logbook, font_t *ibm_vga8, gameData_t *gameData);
static void breakoutChangeStateGame(breakout_t *self);
static void breakoutDetectGameStateChange(breakout_t *self);
static void breakoutDetectBgmChange(breakout_t *self);
static void breakoutChangeStateDead(breakout_t *self);
static void breakoutUpdateDead(breakout_t *self, int64_t elapsedUs);
static void breakoutChangeStateGameOver(breakout_t *self);
static void breakoutUpdateGameOver(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawGameOver(font_t *logbook, font_t *ibm_vga8, gameData_t *gameData);
static void breakoutChangeStateTitleScreen(breakout_t *self);
static void breakoutChangeStateLevelClear(breakout_t *self);
static void breakoutUpdateLevelClear(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawLevelClear(font_t *font, gameData_t *gameData);
static void breakoutChangeStateGameClear(breakout_t *self);
static void breakoutUpdateGameClear(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawGameClear(font_t *ibm_vga8, font_t *logbook, gameData_t *gameData);
static void breakoutInitializeBreakoutHighScores(breakout_t* self);
static void breakoutLoadBreakoutHighScores(breakout_t* self);
static void breakoutSaveBreakoutHighScores(breakout_t* self);
static void breakoutInitializeBreakoutUnlockables(breakout_t* self);
static void breakoutLoadBreakoutUnlockables(breakout_t* self);
static void breakoutSaveBreakoutUnlockables(breakout_t* self);

/*
static void drawBreakoutHighScores(font_t *font, breakoutHighScores_t *highScores, gameData_t *gameData);
uint8_t getHighScoreRank(breakoutHighScores_t *highScores, uint32_t newScore);
static void insertScoreIntoHighScores(breakoutHighScores_t *highScores, uint32_t newScore, char newInitials[], uint8_t rank);
*/

static void breakoutChangeStateNameEntry(breakout_t *self);
static void breakoutUpdateNameEntry(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawNameEntry(font_t *font, gameData_t *gameData, uint8_t currentInitial);
static void breakoutChangeStateShowHighScores(breakout_t *self);
static void breakoutUpdateShowHighScores(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawShowHighScores(font_t *font, uint8_t menuState);
static void breakoutChangeStatePause(breakout_t *self);
static void breakoutUpdatePause(breakout_t *self, int64_t elapsedUs);
static void breakoutDrawPause(font_t *font);
uint16_t breakoutGetLevelIndex(uint8_t world, uint8_t level);

//==============================================================================
// Level Definitions
//==============================================================================

#define NUM_LEVELS 11

static const leveldef_t leveldef[NUM_LEVELS] = {
    {.filename = "intro.bin",
     .timeLimit = 180},
    {.filename = "rightside.bin",
     .timeLimit = 180},
    {.filename = "upsidedown.bin",
     .timeLimit = 180},
    {.filename = "leftside.bin",
     .timeLimit = 180},
    {.filename = "split.bin",
     .timeLimit = 180},
    {.filename = "mag01.bin",
     .timeLimit = 180},
    {.filename = "mag02.bin",
     .timeLimit = 180},
    {.filename = "brkLvlChar1.bin",
     .timeLimit = 180},
    {.filename = "bombtest.bin",
     .timeLimit = 180},
    {.filename = "ponglike.bin",
     .timeLimit = 180},
    {.filename = "starlite.bin",
     .timeLimit = 180},
     };
    
//==============================================================================
// Look Up Tables
//==============================================================================

static const paletteColor_t greenColors[4] = {c555, c051, c030, c051};

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char breakoutName[] = "Galactic Breakdown";

static const char breakoutNewGame[] = "New Game";
static const char breakoutContinue[] = "Continue - Lv";
static const char breakoutExit[] = "Exit";

static const char breakoutReady[] = "Get Ready!";
static const char breakoutGameOver[] = "Game Over!";

static const char breakoutLevelClear[] = "Cleared!";
static const char breakoutPause[] = "Paused";

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

    // Initialize the menu
    breakout->menu = initMenu(breakoutName, breakoutMenuCb);
    breakout->mRenderer = initMenuLogbookRenderer(&breakout->logbook);

    initializeGameData(&(breakout->gameData));
    initializeTileMap(&(breakout->tilemap));
    initializeSoundManager(&(breakout->soundManager));
    initializeEntityManager(&(breakout->entityManager), &(breakout->tilemap), &(breakout->gameData), &(breakout->soundManager));
    
    breakout->tilemap.entityManager = &(breakout->entityManager);
    breakout->tilemap.executeTileSpawnAll = true;
    breakout->tilemap.mapOffsetX = -4;

    loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);

    addSingleItemToMenu(breakout->menu, breakoutNewGame);

    /*
        Manually allocate and build "level select" menu item
        because the max setting will have to change as levels are unlocked
    */

    breakout->levelSelectMenuItem = calloc(1,sizeof(menuItem_t));
    breakout->levelSelectMenuItem->label = breakoutContinue;
    breakout->levelSelectMenuItem->minSetting = 1;
    breakout->levelSelectMenuItem->maxSetting = NUM_LEVELS;
    breakout->levelSelectMenuItem->currentSetting = 1;
    breakout->levelSelectMenuItem->options = NULL;
    breakout->levelSelectMenuItem->subMenu = NULL;

    push(breakout->menu->items, breakout->levelSelectMenuItem);

    addSingleItemToMenu(breakout->menu, breakoutExit);

    //Set frame rate to 60 FPS
    setFrameRateUs(16666);

    // Set the mode to menu mode
    breakout->update = &breakoutUpdateTitleScreen;
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void breakoutExitMode(void)
{
    // Deinitialize the menu.
    // This will also free the "level select" menu item.
    deinitMenu(breakout->menu);
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
            breakout->gameData.level = 1;
            loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);
            breakout->gameData.countdown = leveldef[0].timeLimit;
            breakoutChangeStateReadyScreen(breakout);   
        } else if (label == breakoutContinue)
        {
            initializeGameDataFromTitleScreen(&(breakout->gameData));
            breakout->gameData.level = settingVal;
            loadMapFromFile(&(breakout->tilemap), leveldef[breakout->gameData.level - 1].filename);
            breakout->gameData.countdown = leveldef[breakout->gameData.level -1].timeLimit;
            breakoutChangeStateReadyScreen(breakout);   
        }
        else if (label == breakoutExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
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
    breakout->update(breakout, elapsedUs);
}

void breakoutChangeStateTitleScreen(breakout_t *self){
    self->gameData.frameCount = 0;
    self->update=&breakoutUpdateTitleScreen;
}

static void breakoutUpdateTitleScreen(breakout_t *self, int64_t elapsedUs){
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Pass button events to the menu
        breakout->menu = menuButton(breakout->menu, evt);
    }

    // Draw the menu
    drawMenuLogbook(breakout->menu, breakout->mRenderer, elapsedUs);
}

static void breakoutChangeStateReadyScreen(breakout_t *self){
    self->gameData.frameCount = 0;
    self->update = &breakoutUpdateReadyScreen;
}

static void breakoutUpdateReadyScreen(breakout_t *self, int64_t elapsedUs){    
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        breakoutChangeStateGame(self);
    }

    updateLedsInGame(&(self->gameData));
    breakoutDrawReadyScreen(&(self->logbook), &(self->ibm_vga8), &(self->gameData));
}

static void breakoutDrawReadyScreen(font_t *logbook, font_t *ibm_vga8, gameData_t *gameData){
    drawBreakoutHud(ibm_vga8, gameData);
    drawText(logbook, c555, breakoutReady, (TFT_WIDTH - textWidth(logbook, breakoutReady)) >> 1, 128);
}

static void breakoutChangeStateGame(breakout_t *self){
    self->gameData.frameCount = 0;
    self->gameData.playerBombsCount = 0;
    deactivateAllEntities(&(self->entityManager), false, true);
    self->tilemap.executeTileSpawnAll = true;
    self->update = &breakoutGameLoop;
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void breakoutGameLoop(breakout_t *self, int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        self->gameData.btnState = evt.state;
    }
    updateTouchInput(&(self->gameData));

    updateLedsInGame(&(self->gameData));
    breakoutDetectGameStateChange(self);
    updateEntities(&(self->entityManager));

    // Draw the field
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(breakout->gameData));

    self->gameData.frameCount++;
    if(self->gameData.frameCount > 59){
        self->gameData.frameCount = 0;

        if(self->gameData.countdown > 0){
            self->gameData.countdown--;
        }
        
        self->gameData.inGameTimer++;
    }

    self->gameData.prevBtnState = self->gameData.btnState;
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
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((0 == xp % 40) || (0 == yp % 40))
            {
                TURBO_SET_PIXEL(xp, yp, c110);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c001);
            }
        }
    }
}

void breakoutDetectGameStateChange(breakout_t *self){
    if(!self->gameData.changeState){
        return;
    }

    switch (self->gameData.changeState)
    {
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

void breakoutChangeStateDead(breakout_t *self){
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    //self->gameData.levelDeaths++;
    self->gameData.combo = 0;
    //self->gameData.initialHp = 1;

    //buzzer_stop();
    //buzzer_play_bgm(&sndDie);

    self->update=&breakoutUpdateDead;
}


void breakoutUpdateDead(breakout_t *self, int64_t elapsedUs){
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        resetGameDataLeds(&(self->gameData));
        if(self->gameData.lives > 0){
            breakoutChangeStateReadyScreen(self);
        } else {
            breakoutChangeStateGameOver(self);
        }
    }

    updateLedsInGame(&(self->gameData));
    updateEntities(&(self->entityManager));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));

    /*if(self->gameData.countdown < 0){
        drawText(self->disp, &(self->radiostars), c555, str_time_up, (self->disp->w - textWidth(&(self->radiostars), str_time_up)) / 2, 128);
    }*/
}

void breakoutChangeStateGameOver(breakout_t *self){
    self->gameData.frameCount = 0;
    resetGameDataLeds(&(self->gameData)); 
    //buzzer_play_bgm(&bgmGameOver);
    self->update=&breakoutUpdateGameOver;
}

void breakoutUpdateGameOver(breakout_t *self, int64_t elapsedUs){
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
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

        changeStateNameEntry(self);*/
        deactivateAllEntities(&(self->entityManager), false, false);
        breakoutChangeStateTitleScreen(self);
    }

    breakoutDrawGameOver(&(self->logbook), &(self->ibm_vga8), &(self->gameData));
    //updateLedsGameOver(&(self->gameData));
}

void breakoutDrawGameOver(font_t *logbook, font_t *ibm_vga8, gameData_t *gameData){
    drawBreakoutHud(ibm_vga8, gameData);
    drawText(logbook, c555, breakoutGameOver, (TFT_WIDTH - textWidth(logbook, breakoutGameOver)) / 2, 128);
}

static void drawBreakoutHud(font_t *font, gameData_t *gameData){
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%06" PRIu32, gameData->score);

    char levelStr[15];
    snprintf(levelStr, sizeof(levelStr) - 1, "Level %d", gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%d", gameData->lives);

    //char timeStr[10];
    //snprintf(timeStr, sizeof(timeStr) - 1, "T:%03d", gameData->countdown);

    if(gameData->frameCount > 29) {
        drawText(font, c500, "1UP", 24, 2);
    }
    
    drawText(font, c555, livesStr, 48, 2);
    //drawText(font, c555, coinStr, 160, 16);
    drawText(font, c555, scoreStr, 80, 2);
    drawText(font, c555, levelStr, 184, 2);
    //drawText(d, font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 220, 16);

    drawText(font, c555, "B", 271, 32);
    drawText(font, c555, "O", 271, 44);
    drawText(font, c555, "N", 271, 56);
    drawText(font, c555, "U", 271, 68);
    drawText(font, c555, "S", 271, 80);
    drawRect(271,96,279,96+(gameData->countdown >> 1), c555);

    //if(gameData->comboTimer == 0){
    //    return;
    //}

    //snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    snprintf(scoreStr, sizeof(scoreStr) - 1, "x%d", gameData->combo);
    drawText(font, /*(gameData->comboTimer < 60) ? c030:*/ greenColors[(breakout->gameData.frameCount >> 3) % 4], scoreStr, 144, 2);
}

void breakoutChangeStateLevelClear(breakout_t *self){
    self->gameData.frameCount = 0;
    resetGameDataLeds(&(self->gameData));
    self->update=&breakoutUpdateLevelClear;
}

void breakoutUpdateLevelClear(breakout_t *self, int64_t elapsedUs){ 
    self->gameData.frameCount++;
    self->gameData.targetBlocksBroken = 0;

    if(self->gameData.frameCount > 60){
        if(self->gameData.countdown > 0){
            self->gameData.countdown--;
            
            if(self->gameData.countdown % 2){
                bzrPlayBgm(&(self->soundManager.tally), BZR_STEREO);
            }

            uint16_t comboPoints = 20 * self->gameData.combo;

            self->gameData.score += comboPoints;
            self->gameData.comboScore = comboPoints;

            if(self->gameData.combo > 1){
                self->gameData.combo--;
            }
        } else if(self->gameData.frameCount % 60 == 0) {
            //Hey look, it's a frame rule!
            deactivateAllEntities(&(self->entityManager), false, false);

            uint16_t levelIndex = self->gameData.level;
            
            if(levelIndex >= NUM_LEVELS - 1){
                //Game Cleared!

                //if(!self->gameData.debugMode){
                    //Determine achievements
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
            } else {
                 //Advance to the next level
                self->gameData.level++;

                //Unlock the next level
                levelIndex++;
                /*if(levelIndex > self->unlockables.maxLevelIndexUnlocked){
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }*/
                loadMapFromFile(&(breakout->tilemap), leveldef[levelIndex].filename);
                breakout->gameData.countdown = leveldef[levelIndex].timeLimit;
                breakoutChangeStateReadyScreen(self);
                return;
            }

            /*if(!self->gameData.debugMode){
                savePlatformerUnlockables(self);
            }*/
        }
    }

    //updateEntities(&(self->entityManager));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawLevelClear( &(self->logbook), &(self->gameData));
    updateLedsLevelClear(&(self->gameData));
}

void breakoutDrawLevelClear(font_t *font, gameData_t *gameData){
    drawText(font, c555, breakoutLevelClear, (TFT_WIDTH - textWidth(font, breakoutLevelClear)) / 2, 128);
}

void breakoutChangeStateGameClear(breakout_t *self){
    self->gameData.frameCount = 0;
    self->update=&breakoutUpdateGameClear;
    resetGameDataLeds(&(self->gameData));
    //buzzer_play_bgm(&bgmSmooth);
}

void breakoutUpdateGameClear(breakout_t *self, int64_t elapsedUs){    
    self->gameData.frameCount++;

    if(self->gameData.frameCount > 450){
        if(self->gameData.lives > 0){
            if(self->gameData.frameCount % 60 == 0){
                self->gameData.lives--;
                self->gameData.score += 200000;
                //buzzer_play_sfx(&snd1up);
            }
        } else if(self->gameData.frameCount % 960 == 0) {
            breakoutChangeStateGameOver(self);
        }
    }

    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawGameClear(&(self->ibm_vga8), &(self->logbook), &(self->gameData));
    updateLedsGameClear(&(self->gameData));
}

void breakoutDrawGameClear(font_t *ibm_vga8, font_t *logbook, gameData_t *gameData){
    drawBreakoutHud(ibm_vga8, gameData);

    drawText(logbook, c555, "Thanks for playing!", 16, 48);
    
    if(gameData->frameCount > 300){
        drawText(logbook, c555, "See you next", 8, 112);
        drawText(logbook, c555, "debug mission!", 8, 160);
    }

}

void breakoutChangeStatePause(breakout_t *self){
    //buzzer_play_bgm(&sndPause);
    self->gameData.btnState = 0;
    self->update=&breakoutUpdatePause;
}

void breakoutUpdatePause(breakout_t *self, int64_t elapsedUs){
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        breakout->gameData.btnState = evt.state;
    }

    if((
        (self->gameData.btnState & PB_START)
        &&
        !(self->gameData.prevBtnState & PB_START)
    )){
        //buzzer_play_sfx(&sndPause);
        //self->gameData.changeBgm = self->gameData.currentBgm;
        //self->gameData.currentBgm = BGM_NULL;
        self->gameData.btnState = 0;
        self->update=&breakoutGameLoop;
    }

    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawPause(&(self->logbook));

    self->gameData.prevBtnState = self->prevBtnState;
}

void breakoutDrawPause(font_t *font){
    drawText(font, c555, breakoutPause, (TFT_WIDTH - textWidth(font, breakoutPause)) / 2, 128);
}

uint16_t breakoutGetLevelIndex(uint8_t world, uint8_t level){
    return (world-1) * 4 + (level-1);
}
/**
 * @file mode_platformer.c
 * @author J.Vega (JVeg199X)
 * @brief Super Swadge Land (port to Swadge 2024)
 * @date 2023-09-22
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "mode_platformer.h"
#include "esp_random.h"

#include "platformer_typedef.h"
#include "plTilemap.h"
#include "plGameData.h"
#include "plEntityManager.h"
#include "plLeveldef.h"

#include "hdw-led.h"
#include "palette.h"
#include "hdw-nvs.h"
#include "plSoundManager.h"
#include <inttypes.h>
#include "mainMenu.h"
#include "fill.h"

//==============================================================================
// Constants
//==============================================================================
#define BIG_SCORE 4000000UL
#define BIGGER_SCORE 10000000UL
#define FAST_TIME 1500 //25 minutes

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};

static const paletteColor_t redColors[4] = {c501, c540, c550, c540};
static const paletteColor_t yellowColors[4] = {c550, c331, c550, c555};
static const paletteColor_t greenColors[4] = {c555, c051, c030, c051};
static const paletteColor_t cyanColors[4] = {c055, c455, c055, c033};
static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};
static const paletteColor_t rgbColors[4] = {c500, c050, c005, c050};

static const int16_t cheatCode[11] = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};

//==============================================================================
// Functions Prototypes
//==============================================================================

void platformerEnterMode(void);
void platformerExitMode(void);
void platformerMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)(platformer_t *self);
struct platformer_t
{
    font_t radiostars;

    plTilemap_t tilemap;
    plEntityManager_t entityManager;
    plGameData_t gameData;

    plSoundManager_t soundManager;

    uint8_t menuState;
    uint8_t menuSelection;
    uint8_t cheatCodeIdx;

    int16_t btnState;
    int16_t prevBtnState;

    int32_t frameTimer;

    platformerHighScores_t highScores;
    platformerUnlockables_t unlockables;
    bool easterEgg;

    gameUpdateFuncton_t update;
};

//==============================================================================
// Function Prototypes
//==============================================================================
void drawPlatformerHud(font_t *font, plGameData_t *gameData);
void drawPlatformerTitleScreen(font_t *font, plGameData_t *gameData);
void changeStateReadyScreen(platformer_t *self);
void updateReadyScreen(platformer_t *self);
void drawReadyScreen(font_t *font, plGameData_t *gameData);
void changeStateGame(platformer_t *self);
void detectGameStateChange(platformer_t *self);
void detectBgmChange(platformer_t *self);
void changeStateDead(platformer_t *self);
void updateDead(platformer_t *self);
void changeStateGameOver(platformer_t *self);
void updateGameOver(platformer_t *self);
void drawGameOver(font_t *font, plGameData_t *gameData);
void changeStateTitleScreen(platformer_t *self);
void changeStateLevelClear(platformer_t *self);
void updateLevelClear(platformer_t *self);
void drawLevelClear(font_t *font, plGameData_t *gameData);
void changeStateGameClear(platformer_t *self);
void updateGameClear(platformer_t *self);
void drawGameClear(font_t *font, plGameData_t *gameData);
void initializePlatformerHighScores(platformer_t* self);
void loadPlatformerHighScores(platformer_t* self);
void savePlatformerHighScores(platformer_t* self);
void initializePlatformerUnlockables(platformer_t* self);
void loadPlatformerUnlockables(platformer_t* self);
void savePlatformerUnlockables(platformer_t* self);
void drawPlatformerHighScores(font_t *font, platformerHighScores_t *highScores, plGameData_t *gameData);
uint8_t getHighScoreRank(platformerHighScores_t *highScores, uint32_t newScore);
void insertScoreIntoHighScores(platformerHighScores_t *highScores, uint32_t newScore, char newInitials[], uint8_t rank);
void changeStateNameEntry(platformer_t *self);
void updateNameEntry(platformer_t *self);
void drawNameEntry(font_t *font, plGameData_t *gameData, uint8_t currentInitial);
void changeStateShowHighScores(platformer_t *self);
void updateShowHighScores(platformer_t *self);
void drawShowHighScores(font_t *font, uint8_t menuState);
void changeStatePause(platformer_t *self);
void updatePause(platformer_t *self);
void drawPause(font_t *font);
uint16_t getLevelIndex(uint8_t world, uint8_t level);

//==============================================================================
// Variables
//==============================================================================

platformer_t *platformer = NULL;

swadgeMode_t modePlatformer = {
        .modeName = "Swadge Land",
        .wifiMode = NO_WIFI,
        .overrideUsb = false,
        .usesAccelerometer        = false,
        .usesThermometer          = false,
        .fnEnterMode = platformerEnterMode,
        .fnExitMode = platformerExitMode,
        .fnMainLoop = platformerMainLoop,
        .fnAudioCallback          = NULL,
        .fnBackgroundDrawCallback = NULL,
        .fnEspNowRecvCb = NULL,
        .fnEspNowSendCb = NULL
};

#define NUM_LEVELS 16

static const plLeveldef_t leveldef[17] = {
    {.filename = "level1-1.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "dac01.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level1-3.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level1-4.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level2-1.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "dac03.bin",
     .timeLimit = 220,
     .checkpointTimeLimit = 90},
    {.filename = "level2-3.bin",
     .timeLimit = 200,
     .checkpointTimeLimit = 90},
    {.filename = "level2-4.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level3-1.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "dac02.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level3-3.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90},
    {.filename = "level3-4.bin",
     .timeLimit = 220,
     .checkpointTimeLimit = 110},
    {.filename = "level4-1.bin",
     .timeLimit = 270,
     .checkpointTimeLimit = 90},
    {.filename = "level4-2.bin",
     .timeLimit = 240,
     .checkpointTimeLimit = 90},
    {.filename = "level4-3.bin",
     .timeLimit = 240,
     .checkpointTimeLimit = 90},
    {.filename = "level4-4.bin",
     .timeLimit = 240,
     .checkpointTimeLimit = 90},
    {.filename = "debug.bin",
     .timeLimit = 180,
     .checkpointTimeLimit = 90}};

led_t platLeds[CONFIG_NUM_LEDS];

static const char str_get_ready[] = "Get Ready!";
static const char str_time_up[] = "-Time Up!-";
static const char str_game_over[] = "Game Over";
static const char str_well_done[] = "Well done!";
static const char str_congrats[] = "Congratulations!";
static const char str_initials[] = "Enter your initials!";
static const char str_hbd[] = "Happy Birthday, Evelyn!";
static const char str_registrated[] = "Your name registrated.";
static const char str_do_your_best[] = "Do your best!";
static const char str_pause[] = "-Pause-";

static const char KEY_SCORES[] = "pf_scores";
static const char KEY_UNLOCKS[] = "pf_unlocks";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void platformerEnterMode(void)
{
    // Allocate memory for this mode
    platformer = (platformer_t *)calloc(1, sizeof(platformer_t));
    memset(platformer, 0, sizeof(platformer_t));

    platformer->menuState = 0;
    platformer->menuSelection = 0;
    platformer->btnState = 0;
    platformer->prevBtnState = 0;

    loadPlatformerHighScores(platformer);
    loadPlatformerUnlockables(platformer);
    if(platformer->highScores.initials[0][0] == 'E' && platformer->highScores.initials[0][1] == 'F' && platformer->highScores.initials[0][2] == 'V'){
        platformer->easterEgg = true;
    }

    loadFont("radiostars.font", &platformer->radiostars, false);

    pl_initializeTileMap(&(platformer->tilemap));
    pl_loadMapFromFile(&(platformer->tilemap), leveldef[0].filename);

    pl_initializeSoundManager(&(platformer->soundManager));

    pl_initializeGameData(&(platformer->gameData), &(platformer->soundManager));
    pl_initializeEntityManager(&(platformer->entityManager), &(platformer->tilemap), &(platformer->gameData), &(platformer->soundManager));

    platformer->tilemap.entityManager = &(platformer->entityManager);
    platformer->tilemap.tileSpawnEnabled = true;

    setFrameRateUs(16666);

    platformer->update = &updateTitleScreen;
}

/**
 * @brief TODO
 *
 */
void platformerExitMode(void)
{
    freeFont(&platformer->radiostars);
    pl_freeTilemap(&(platformer->tilemap));
    pl_freeSoundManager(&(platformer->soundManager));
    pl_freeEntityManager(&(platformer->entityManager));
    free(platformer);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
void platformerMainLoop(int64_t elapsedUs)
{
    //Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        platformer->btnState = evt.state;
        platformer->gameData.btnState = evt.state;
    }

    platformer->update(platformer);

    platformer->prevBtnState = platformer->btnState;
    platformer->gameData.prevBtnState = platformer->prevBtnState;
}

/**
 * @brief TODO
 *
 * @param opt
 */
// void platformerCb(const char *opt)
// {
//     ESP_LOGI("MNU", "%s", opt);
// }

void updateGame(platformer_t *self)
{    
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    pl_updateEntities(&(self->entityManager));

    pl_drawTileMap(&(self->tilemap));
    pl_drawEntities(&(self->entityManager));
    detectGameStateChange(self);
    detectBgmChange(self);
    drawPlatformerHud(&(self->radiostars), &(self->gameData));

    self->gameData.frameCount++;
    if(self->gameData.frameCount > 59){
        self->gameData.frameCount = 0;
        self->gameData.countdown--;
        self->gameData.inGameTimer++;

        if(self->gameData.countdown < 10){
            bzrPlayBgm(&(self->soundManager.sndOuttaTime), BZR_STEREO);
        }

        if(self->gameData.countdown < 0){
            killPlayer(self->entityManager.playerEntity);
        }
    }

    updateComboTimer(&(self->gameData));
}

void drawPlatformerHud(font_t *font, plGameData_t *gameData)
{
    char coinStr[8];
    snprintf(coinStr, sizeof(coinStr) - 1, "C:%02d", gameData->coins);

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%06" PRIu32, gameData->score);

    char levelStr[15];
    snprintf(levelStr, sizeof(levelStr) - 1, "Level %d-%d", gameData->world, gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%d", gameData->lives);

    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr) - 1, "T:%03d", gameData->countdown);

    if(gameData->frameCount > 29) {
        drawText(font, c500, "1UP", 24, 2);
    }
    
    drawText(font, c555, livesStr, 56, 2);
    drawText(font, c555, coinStr, 160, 16);
    drawText(font, c555, scoreStr, 8, 16);
    drawText(font, c555, levelStr, 152, 2);
    drawText(font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 220, 16);

    if(gameData->comboTimer == 0){
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    drawText(font, (gameData->comboTimer < 60) ? c030: greenColors[(platformer->gameData.frameCount >> 3) % 4], scoreStr, 8, 30);
}

void updateTitleScreen(platformer_t *self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;
   
    // Handle inputs
    switch(platformer->menuState){
        case 0:{ 
            if(self->gameData.frameCount > 600){
                pl_resetGameDataLeds(&(self->gameData));
                changeStateShowHighScores(self);
            }

            if (
                (self->gameData.btnState & cheatCode[platformer->cheatCodeIdx])
                &&
                !(self->gameData.prevBtnState & cheatCode[platformer->cheatCodeIdx])
            ) {
                platformer->cheatCodeIdx++;

                if(platformer->cheatCodeIdx > 10){
                    platformer->cheatCodeIdx = 0;
                    platformer->menuState = 1;
                    platformer->gameData.debugMode = true;
                    bzrPlaySfx(&(platformer->soundManager.sndLevelClearS), BZR_STEREO);
                    break;
                } else {
                    bzrPlaySfx(&(platformer->soundManager.sndMenuSelect), BZR_STEREO);
                    break;
                }
            }
            else
            {
                if( !(self->gameData.frameCount % 150 ) ){
                    platformer->cheatCodeIdx = 0;
                }
            }

            if (
                (
                    (self->gameData.btnState & PB_START)
                    &&
                    !(self->gameData.prevBtnState & PB_START)
                )
                    ||
                (
                    (self->gameData.btnState & PB_A)
                    &&
                    !(self->gameData.prevBtnState & PB_A)
                )
            )
            {
                bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                platformer->menuState = 1;
                platformer->menuSelection = 0;
            }

            break;
        }
        case 1:{
            if (
                (
                    (self->gameData.btnState & PB_START)
                    &&
                    !(self->gameData.prevBtnState & PB_START)
                )
                    ||
                (
                    (self->gameData.btnState & PB_A)
                    &&
                    !(self->gameData.prevBtnState & PB_A)
                )
            )
            {
                switch(self->menuSelection){
                    case 0 ... 1: {
                        uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);
                        if(
                            (levelIndex >= NUM_LEVELS)
                            ||
                            (!self->gameData.debugMode && levelIndex > self->unlockables.maxLevelIndexUnlocked)
                        ){
                            bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                            break;
                        }

                        /*if(self->menuSelection == 0){
                            self->gameData.world = 1;
                            self->gameData.level = 1;
                        }*/

                        pl_initializeGameDataFromTitleScreen(&(self->gameData));
                        changeStateReadyScreen(self);
                        break;
                    }
                    case 2: {
                        if(self->gameData.debugMode){
                            //Reset Progress
                            initializePlatformerUnlockables(self);
                            bzrPlaySfx(&(self->soundManager.sndBreak), BZR_STEREO);
                        } else {
                            //Show High Scores
                            self->menuSelection = 0;
                            self->menuState = 0;

                            changeStateShowHighScores(self);
                            bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                        }
                        break;
                    }
                    case 3: {
                        if(self->gameData.debugMode){
                            //Reset High Scores
                            initializePlatformerHighScores(self);
                            bzrPlaySfx(&(self->soundManager.sndBreak), BZR_STEREO);
                        } else {
                            //Show Achievements
                            self->menuSelection = 0;
                            self->menuState = 2;
                            bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                        }
                        break;
                    }
                    case 4: {
                        if(self->gameData.debugMode){
                            //Save & Quit
                            savePlatformerHighScores(self);
                            savePlatformerUnlockables(self);
                            bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                            switchToSwadgeMode(&mainMenuMode);
                        } else {
                            bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                            switchToSwadgeMode(&mainMenuMode);
                        }
                        break;
                    }
                    default: {
                        bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                        self->menuSelection = 0;
                    }
                }
            } else if (
                    (
                    self->gameData.btnState & PB_UP
                    &&
                    !(self->gameData.prevBtnState & PB_UP)
                )
            )
            {
                if(platformer->menuSelection > 0){
                    platformer->menuSelection--;

                    if(!self->gameData.debugMode && platformer->menuSelection == 1 && self->unlockables.maxLevelIndexUnlocked == 0){
                        platformer->menuSelection--;
                    }

                    bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                }
            } else if (
                    (
                    self->gameData.btnState & PB_DOWN
                    &&
                    !(self->gameData.prevBtnState & PB_DOWN)
                )
            )
            {
                if(platformer->menuSelection < 4){
                    platformer->menuSelection++;

                    if(!self->gameData.debugMode && platformer->menuSelection == 1 && self->unlockables.maxLevelIndexUnlocked == 0){
                        platformer->menuSelection++;
                    }

                    bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                } else {
                    bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                }
            } else if (
                (
                    self->gameData.btnState & PB_LEFT
                    &&
                    !(self->gameData.prevBtnState & PB_LEFT)
                )
            )
            {
                if(platformer->menuSelection == 1){
                    if(platformer->gameData.level == 1 && platformer->gameData.world == 1){
                        bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                    } else {
                        platformer->gameData.level--;
                        if(platformer->gameData.level < 1){
                            platformer->gameData.level = 4;
                            if(platformer->gameData.world > 1){
                                platformer->gameData.world--;
                            }
                        }
                        bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                    }
                }
            } else if (
                    (
                    self->gameData.btnState & PB_RIGHT
                    &&
                    !(self->gameData.prevBtnState & PB_RIGHT)
                )
            )
            {
                if(platformer->menuSelection == 1){
                    if( 
                        (platformer->gameData.level == 4 && platformer->gameData.world == 4) 
                        || 
                        (!platformer->gameData.debugMode && getLevelIndex(platformer->gameData.world, platformer->gameData.level + 1) > platformer->unlockables.maxLevelIndexUnlocked )
                    )
                    {
                        bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                    } else {
                        platformer->gameData.level++;
                        if(platformer->gameData.level > 4){
                            platformer->gameData.level = 1;
                            if(platformer->gameData.world < 8){
                                platformer->gameData.world++;
                            }
                        }
                        bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                    }
                    
                }
            } else if (
                    (
                    self->gameData.btnState & PB_B
                    &&
                    !(self->gameData.prevBtnState & PB_B)
                )
            )
            {
                self->gameData.frameCount = 0;
                platformer->menuState = 0;
                bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
            }
            break;
        }
        case 2:{
            if (
                    (
                    self->gameData.btnState & PB_B
                    &&
                    !(self->gameData.prevBtnState & PB_B)
                )
            )
            {
                self->gameData.frameCount = 0;
                platformer->menuState = 1;
                bzrPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
            }
            break;
        }
        default:
            platformer->menuState = 0;
            bzrPlaySfx(&(platformer->soundManager.sndMenuDeny), BZR_STEREO);
            break;
    }


    pl_scrollTileMap(&(platformer->tilemap), 1, 0);
    if(self->tilemap.mapOffsetX >= self->tilemap.maxMapOffsetX && self->gameData.frameCount > 58){
        self->tilemap.mapOffsetX = 0;
    }
    
    drawPlatformerTitleScreen(&(self->radiostars), &(self->gameData));

    if(( (self->gameData.frameCount) % 10) == 0){
        for (int32_t i = 0; i < 8; i++)
        {
        
            //self->gameData.leds[i].r = (( (self->gameData.frameCount >> 4) % NUM_LEDS) == i) ? 0xFF : 0x00;

            platLeds[i].r += (esp_random() % 1);
            platLeds[i].g += (esp_random() % 8);
            platLeds[i].b += (esp_random() % 8);
        }
    }
    setLeds(platLeds, CONFIG_NUM_LEDS);
}

void drawPlatformerTitleScreen(font_t *font, plGameData_t *gameData)
{
    pl_drawTileMap(&(platformer->tilemap));

    drawText(font, c555, "Super Swadge Land", 40, 32);

    if(platformer->gameData.debugMode){
        drawText(font, c555, "Debug Mode", 80, 48);
    } 

    switch(platformer->menuState){
        case 0: {
            if ((gameData->frameCount % 60 ) < 30)
            {
                drawText(font, c555, "- Press START button -", 20, 128);
            }
            break;
        }
         
        case 1: {
            drawText(font, c555, "Start Game", 48, 128);

            if(platformer->gameData.debugMode || platformer->unlockables.maxLevelIndexUnlocked > 0){
                char levelStr[24];
                snprintf(levelStr, sizeof(levelStr) - 1, "Level Select: %d-%d", gameData->world, gameData->level);
                drawText(font, c555, levelStr, 48, 144);
            }

            if(platformer->gameData.debugMode){
                drawText(font, c555, "Reset Progress", 48, 160);
                drawText(font, c555, "Reset High Scores", 48, 176);
                drawText(font, c555, "Save & Exit to Menu", 48, 192);
            } else {
                drawText(font, c555, "High Scores", 48, 160);
                drawText(font, c555, "Achievements", 48, 176);
                drawText(font, c555, "Exit to Menu", 48, 192);
            }

            drawText(font, c555, "->", 32, 128 + platformer->menuSelection * 16);

            break;
        }

        case 2: {
            if(platformer->unlockables.gameCleared){
                drawText(font, redColors[(gameData->frameCount >> 3) % 4], "Beat the game!", 48, 80);
            }

            if(platformer->unlockables.oneCreditCleared){
                drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "1 Credit Clear!", 48, 96);
            }

            if(platformer->unlockables.bigScore){
                drawText(font, greenColors[(gameData->frameCount >> 3) % 4], "Got 4 million points!", 48, 112);
            }

            if(platformer->unlockables.biggerScore){
                drawText(font, cyanColors[(gameData->frameCount >> 3) % 4], "Got 10 million points!", 48, 128);
            }

            if(platformer->unlockables.fastTime){
                drawText(font, purpleColors[(gameData->frameCount >> 3) % 4], "Beat within 25 min!", 48, 144);
            }

            if(platformer->unlockables.gameCleared && platformer->unlockables.oneCreditCleared && platformer->unlockables.bigScore && platformer->unlockables.biggerScore && platformer->unlockables.fastTime){
                drawText(font, rgbColors[(gameData->frameCount >> 3) % 4], "100% 100% 100%", 48, 160);
            }

            drawText(font, c555, "Press B to Return", 48, 192);
            break;
        }
        
        default:
            break;
    }
}

void changeStateReadyScreen(platformer_t *self){
    self->gameData.frameCount = 0;
    
    bzrPlayBgm(&(self->soundManager.bgmIntro), BZR_STEREO);
    
    pl_resetGameDataLeds(&(self->gameData));
    
    self->update=&updateReadyScreen;
}

void updateReadyScreen(platformer_t *self){
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        bzrStop(true);
        changeStateGame(self);
    }

    drawReadyScreen(&(self->radiostars), &(self->gameData));
}

void drawReadyScreen(font_t *font, plGameData_t *gameData){
    drawPlatformerHud(font, gameData);
    int16_t xOff = (TFT_WIDTH - textWidth(font, str_get_ready)) / 2;
    drawText(font, c555, str_get_ready, xOff, 128);

    if(getLevelIndex(gameData->world, gameData->level) == 0)
    {
        drawText(font, c555, "A: Jump", xOff, 128 + (font->height + 3) * 3);
        drawText(font, c555, "B: Run / Fire", xOff, 128 + (font->height + 3) * 4);
    }
}

void changeStateGame(platformer_t *self){
    self->gameData.frameCount = 0;
    self->gameData.currentBgm = 0;
    pl_resetGameDataLeds(&(self->gameData));

    pl_deactivateAllEntities(&(self->entityManager), false);

    uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);
    pl_loadMapFromFile(&(platformer->tilemap), leveldef[levelIndex].filename);
    self->gameData.countdown = leveldef[levelIndex].timeLimit;

    plEntityManager_t * entityManager = &(self->entityManager);
    entityManager->viewEntity = pl_createPlayer(entityManager, entityManager->tilemap->warps[self->gameData.checkpoint].x * 16, entityManager->tilemap->warps[self->gameData.checkpoint].y * 16);
    entityManager->playerEntity = entityManager->viewEntity;
    entityManager->playerEntity->hp = self->gameData.initialHp;
    pl_viewFollowEntity(&(self->tilemap),entityManager->playerEntity);


    pl_updateLedsHpMeter(&(self->entityManager),&(self->gameData));

    self->tilemap.executeTileSpawnAll = true;

    self->update = &updateGame;
}

void detectGameStateChange(platformer_t *self){
    if(!self->gameData.changeState){
        return;
    }

    switch (self->gameData.changeState)
    {
        case PL_ST_DEAD:
            changeStateDead(self);
            break;

        case PL_ST_READY_SCREEN:
            changeStateReadyScreen(self);
            break;
        
        case PL_ST_LEVEL_CLEAR:
            changeStateLevelClear(self);
            break;

        case PL_ST_PAUSE:
            changeStatePause(self);
            break;

        default:
            break;
    }

    self->gameData.changeState = 0;
}

void detectBgmChange(platformer_t *self){
    if(!self->gameData.changeBgm){
        return;
    }

    switch (self->gameData.changeBgm)
    {
        case PL_BGM_NULL:
            if(self->gameData.currentBgm != PL_BGM_NULL){
                bzrStop(true);
            }
            break;

        case PL_BGM_MAIN:
            if(self->gameData.currentBgm != PL_BGM_MAIN){
                bzrPlayBgm(&(self->soundManager.bgmDemagio), BZR_STEREO);
            }
            break;
        
        case PL_BGM_ATHLETIC:
            if(self->gameData.currentBgm != PL_BGM_ATHLETIC){
                bzrPlayBgm(&(self->soundManager.bgmSmooth), BZR_STEREO);
            }
            break;

        case PL_BGM_UNDERGROUND:
            if(self->gameData.currentBgm != PL_BGM_UNDERGROUND){
                bzrPlayBgm(&(self->soundManager.bgmUnderground), BZR_STEREO);
            }
            break;

        case PL_BGM_FORTRESS:
            if(self->gameData.currentBgm != PL_BGM_FORTRESS){
                bzrPlayBgm(&(self->soundManager.bgmCastle), BZR_STEREO);
            }
            break;

        default:
            break;
    }

    self->gameData.currentBgm = self->gameData.changeBgm;
    self->gameData.changeBgm = 0;
}

void changeStateDead(platformer_t *self){
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    self->gameData.levelDeaths++;
    self->gameData.combo = 0;
    self->gameData.comboTimer = 0;
    self->gameData.initialHp = 1;

    bzrStop(true);
    bzrPlayBgm(&(self->soundManager.sndDie), BZR_STEREO);

    self->update=&updateDead;
}

void updateDead(platformer_t *self){
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);
    
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        if(self->gameData.lives > 0){
            changeStateReadyScreen(self);
        } else {
            changeStateGameOver(self);
        }
    }

    pl_updateEntities(&(self->entityManager));
    pl_drawTileMap(&(self->tilemap));
    pl_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->radiostars), &(self->gameData));

    if(self->gameData.countdown < 0){
        drawText(&(self->radiostars), c555, str_time_up, (TFT_WIDTH - textWidth(&(self->radiostars), str_time_up)) / 2, 128);
    }
}


void updateGameOver(platformer_t *self){
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        //Handle unlockables

        if(self->gameData.score >= BIG_SCORE) {
            self->unlockables.bigScore = true;
        }

        if(self->gameData.score >= BIGGER_SCORE) {
            self->unlockables.biggerScore = true;
        }

        if(!self->gameData.debugMode){
            savePlatformerUnlockables(self);
        }

        changeStateNameEntry(self);
    }

    drawGameOver(&(self->radiostars), &(self->gameData));
    pl_updateLedsGameOver(&(self->gameData));
}

void changeStateGameOver(platformer_t *self){
    self->gameData.frameCount = 0;
    pl_resetGameDataLeds(&(self->gameData)); 
    bzrPlayBgm(&(self->soundManager.bgmGameOver), BZR_STEREO);
    self->update=&updateGameOver;   
    
}

void drawGameOver(font_t *font, plGameData_t *gameData){
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
}

void changeStateTitleScreen(platformer_t *self){
    self->gameData.frameCount = 0;
    self->update=&updateTitleScreen;
}

void changeStateLevelClear(platformer_t *self){
    self->gameData.frameCount = 0;
    self->gameData.checkpoint = 0;
    self->gameData.levelDeaths = 0;
    self->gameData.initialHp = self->entityManager.playerEntity->hp;
    self->gameData.extraLifeCollected = false;
    pl_resetGameDataLeds(&(self->gameData));
    self->update=&updateLevelClear;
}

void updateLevelClear(platformer_t *self){
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);
    
    self->gameData.frameCount++;

    if(self->gameData.frameCount > 60){
        if(self->gameData.countdown > 0){
            self->gameData.countdown--;
            
            if(self->gameData.countdown % 2){
                bzrPlayBgm(&(self->soundManager.sndTally), BZR_STEREO);
            }

            uint16_t comboPoints = 50 * self->gameData.combo;

            self->gameData.score += comboPoints;
            self->gameData.comboScore = comboPoints;

            if(self->gameData.combo > 1){
                self->gameData.combo--;
            }
        } else if(self->gameData.frameCount % 60 == 0) {
            //Hey look, it's a frame rule!
            
            uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);
            
            if(levelIndex >= NUM_LEVELS - 1){
                //Game Cleared!

                if(!self->gameData.debugMode){
                    //Determine achievements
                    self->unlockables.gameCleared = true;
                    
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
                }

                changeStateGameClear(self);
            } else {
                 //Advance to the next level
                self->gameData.level++;
                if(self->gameData.level > 4){
                    self->gameData.world++;
                    self->gameData.level = 1;
                }

                //Unlock the next level
                levelIndex++;
                if(levelIndex > self->unlockables.maxLevelIndexUnlocked){
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }

                changeStateReadyScreen(self);
            }

            if(!self->gameData.debugMode){
                savePlatformerUnlockables(self);
            }
        }
    }

    pl_updateEntities(&(self->entityManager));
    pl_drawTileMap(&(self->tilemap));
    pl_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->radiostars), &(self->gameData));
    drawLevelClear(&(self->radiostars), &(self->gameData));
    pl_updateLedsLevelClear(&(self->gameData));
}

void drawLevelClear(font_t *font, plGameData_t *gameData){
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) / 2, 128);
}

void changeStateGameClear(platformer_t *self){
    self->gameData.frameCount = 0;
    self->update=&updateGameClear;
    pl_resetGameDataLeds(&(self->gameData));
    bzrPlayBgm(&(self->soundManager.bgmSmooth), BZR_STEREO);
}

void updateGameClear(platformer_t *self){
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    
    self->gameData.frameCount++;

    if(self->gameData.frameCount > 450){
        if(self->gameData.lives > 0){
            if(self->gameData.frameCount % 60 == 0){
                self->gameData.lives--;
                self->gameData.score += 200000;
                bzrPlaySfx(&(self->soundManager.snd1up), BZR_STEREO);
            }
        } else if(self->gameData.frameCount % 960 == 0) {
            changeStateGameOver(self);
        }
    }

    drawPlatformerHud(&(self->radiostars), &(self->gameData));
    drawGameClear(&(self->radiostars), &(self->gameData));
    pl_updateLedsGameClear(&(self->gameData));
}

void drawGameClear( font_t *font, plGameData_t *gameData){
    drawPlatformerHud(font, gameData);

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr) - 1, "in %06" PRIu32 " seconds!", gameData->inGameTimer);

    drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], str_congrats, (TFT_WIDTH - textWidth(font, str_congrats)) / 2, 48);

    if(gameData->frameCount > 120){
        drawText(font, c555, "You've completed your", 8, 80);
        drawText(font, c555, "trip across Swadge Land", 8, 96);
    }
    
    if(gameData->frameCount > 180){
        drawText(font, (gameData->inGameTimer < FAST_TIME) ? cyanColors[(gameData->frameCount >> 3) % 4] : c555, timeStr, (TFT_WIDTH - textWidth(font, timeStr)) / 2, 112);
    }

    if(gameData->frameCount > 300){
        drawText(font, c555, "The Swadge staff", 8, 144);
        drawText(font, c555, "thanks you for playing!", 8, 160);
    }

    if(gameData->frameCount > 420){
        drawText(font, (gameData->lives > 0) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, "Bonus 200000pts per life!", (TFT_WIDTH - textWidth(font, "Bonus 100000pts per life!")) / 2, 192);
    }

    /*
    drawText(font, c555, "Thanks for playing.", 24, 48);
    drawText(font, c555, "Many more battle scenes", 8, 96);
    drawText(font, c555, "will soon be available!", 8, 112);
    drawText(font, c555, "Bonus 100000pts per life!", 8, 160);
    */
}

void initializePlatformerHighScores(platformer_t* self){
    self->highScores.scores[0] = 100000;
    self->highScores.scores[1] = 80000;
    self->highScores.scores[2] = 40000;
    self->highScores.scores[3] = 20000;
    self->highScores.scores[4] = 10000;

    for(uint8_t i=0; i<NUM_PLATFORMER_HIGH_SCORES; i++){
        self->highScores.initials[i][0] = 'J' + i;
        self->highScores.initials[i][1] = 'P' - i;
        self->highScores.initials[i][2] = 'V' + i;
    }
}

void loadPlatformerHighScores(platformer_t* self)
{
    size_t size = sizeof(platformerHighScores_t);
    // Try reading the value
    if(false == readNvsBlob(KEY_SCORES, &(self->highScores), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerHighScores(self);
    }
}

void savePlatformerHighScores(platformer_t* self){
    size_t size = sizeof(platformerHighScores_t);
    writeNvsBlob( KEY_SCORES, &(self->highScores), size);
}

void initializePlatformerUnlockables(platformer_t* self){
    self->unlockables.maxLevelIndexUnlocked = 0;
    self->unlockables.gameCleared = false;
    self->unlockables.oneCreditCleared = false;
    self->unlockables.bigScore = false;
    self->unlockables.fastTime = false;
    self->unlockables.biggerScore = false;
}

void loadPlatformerUnlockables(platformer_t* self){
    size_t size = sizeof(platformerUnlockables_t);
    // Try reading the value
    if(false == readNvsBlob(KEY_UNLOCKS, &(self->unlockables), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerUnlockables(self);
    }
};

void savePlatformerUnlockables(platformer_t* self){
    size_t size = sizeof(platformerUnlockables_t);
    writeNvsBlob(KEY_UNLOCKS, &(self->unlockables), size);
};

void drawPlatformerHighScores(font_t *font, platformerHighScores_t *highScores, plGameData_t *gameData){
    drawText(font, c555, "RANK  SCORE  NAME", 48, 96);
    for(uint8_t i=0; i<NUM_PLATFORMER_HIGH_SCORES; i++){
        char rowStr[32];
        snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32 "   %c%c%c", i+1, highScores->scores[i], highScores->initials[i][0], highScores->initials[i][1], highScores->initials[i][2]);
        drawText(font, (gameData->rank == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr, 60, 128 + i*16);
    }
}

uint8_t getHighScoreRank(platformerHighScores_t *highScores, uint32_t newScore){
    uint8_t i;
    for(i=0; i<NUM_PLATFORMER_HIGH_SCORES; i++){
        if(highScores->scores[i] < newScore){
            break;
        }
    }

    return i;
}

void insertScoreIntoHighScores(platformerHighScores_t *highScores, uint32_t newScore, char newInitials[], uint8_t rank){

    if(rank >= NUM_PLATFORMER_HIGH_SCORES){
        return;
    }

    for(uint8_t i=NUM_PLATFORMER_HIGH_SCORES - 1; i>rank; i--){
        highScores->scores[i] = highScores->scores[i-1];
        highScores->initials[i][0] = highScores->initials[i-1][0];
        highScores->initials[i][1] = highScores->initials[i-1][1];
        highScores->initials[i][2] = highScores->initials[i-1][2];
    }

    highScores->scores[rank] = newScore;
    highScores->initials[rank][0] = newInitials[0];
    highScores->initials[rank][1] = newInitials[1];
    highScores->initials[rank][2] = newInitials[2];

}

void changeStateNameEntry(platformer_t *self){
    self->gameData.frameCount = 0;
    uint8_t rank = getHighScoreRank(&(self->highScores),self->gameData.score);
    self->gameData.rank = rank;
    self->menuState = 0;

    pl_resetGameDataLeds(&(self->gameData));

    if(rank >= NUM_PLATFORMER_HIGH_SCORES || self->gameData.debugMode){
        self->menuSelection = 0;
        self->gameData.rank = NUM_PLATFORMER_HIGH_SCORES;
        changeStateShowHighScores(self);
        return;
    }

    bzrPlayBgm(&(self->soundManager.bgmNameEntry), BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update=&updateNameEntry;
}

void updateNameEntry(platformer_t *self){
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if(
        self->gameData.btnState & PB_LEFT
        &&
        !(self->gameData.prevBtnState & PB_LEFT)
    ) {
        self->menuSelection--;

        if(self->menuSelection < 32){
            self->menuSelection = 90;
        }

        self->gameData.initials[self->menuState]=self->menuSelection;
        bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    } else if(
        self->gameData.btnState & PB_RIGHT
        &&
        !(self->gameData.prevBtnState & PB_RIGHT)
    ) {
        self->menuSelection++;

        if(self->menuSelection > 90){
            self->menuSelection = 32;
        }

         self->gameData.initials[self->menuState]=self->menuSelection;
         bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    } else if(
        self->gameData.btnState & PB_B
        &&
        !(self->gameData.prevBtnState & PB_B)
    ) {
        if(self->menuState > 0){
            self->menuState--;
            self->menuSelection = self->gameData.initials[self->menuState];
            bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        } else {
            bzrPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
        }
    } else if(
        self->gameData.btnState & PB_A
        &&
        !(self->gameData.prevBtnState & PB_A)
    ) {
        self->menuState++;
        
        if(self->menuState >2){
            insertScoreIntoHighScores(&(self->highScores), self->gameData.score, self->gameData.initials, self->gameData.rank);
            savePlatformerHighScores(self);
            changeStateShowHighScores(self);
            bzrPlaySfx(&(self->soundManager.sndPowerUp), BZR_STEREO);
        } else {
            self->menuSelection = self->gameData.initials[self->menuState];
            bzrPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
    }
    
    drawNameEntry(&(self->radiostars), &(self->gameData), self->menuState);
    pl_updateLedsShowHighScores(&(self->gameData));
}

void drawNameEntry(font_t *font, plGameData_t *gameData, uint8_t currentInitial){
    drawText(font, greenColors[(platformer->gameData.frameCount >> 3) % 4], str_initials, (TFT_WIDTH - textWidth(font, str_initials)) / 2, 64);

    char rowStr[32];
    snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32, gameData->rank+1, gameData->score);
    drawText(font, c555, rowStr, 64, 128);

    for(uint8_t i=0; i<3; i++){
        snprintf(rowStr, sizeof(rowStr) - 1, "%c", gameData->initials[i]);
        drawText(font, (currentInitial == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr, 192+16*i, 128);
    }
}

void changeStateShowHighScores(platformer_t *self){
    self->gameData.frameCount = 0;
    self->update=&updateShowHighScores;
}

void updateShowHighScores(platformer_t *self){
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    
    self->gameData.frameCount++;

    if((self->gameData.frameCount > 300) || (
        (
            (self->gameData.btnState & PB_START)
            &&
            !(self->gameData.prevBtnState & PB_START)
        )
            ||
        (
            (self->gameData.btnState & PB_A)
            &&
            !(self->gameData.prevBtnState & PB_A)
        )
    )){
        self->menuState = 0;
        self->menuSelection = 0;
        bzrStop(true);
        changeStateTitleScreen(self);
    }

    drawShowHighScores(&(self->radiostars), self->menuState);
    drawPlatformerHighScores(&(self->radiostars), &(self->highScores), &(self->gameData));

    pl_updateLedsShowHighScores(&(self->gameData));
}

void drawShowHighScores(font_t *font, uint8_t menuState){
    if(platformer->easterEgg){
        drawText(font, highScoreNewEntryColors[(platformer->gameData.frameCount >> 3) % 4], str_hbd, (TFT_WIDTH - textWidth(font, str_hbd)) / 2, 32);
    } else if(menuState == 3){
        drawText(font, redColors[(platformer->gameData.frameCount >> 3) % 4], str_registrated, (TFT_WIDTH - textWidth(font, str_registrated)) / 2, 32);
    } else {
        drawText(font, c555, str_do_your_best, (TFT_WIDTH - textWidth(font, str_do_your_best)) / 2, 32);
    }
}

void changeStatePause(platformer_t *self){
    bzrStop(true);
    bzrPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
    self->update=&updatePause;
}

void updatePause(platformer_t *self){
    if((
        (self->gameData.btnState & PB_START)
        &&
        !(self->gameData.prevBtnState & PB_START)
    )){
        bzrPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
        self->gameData.changeBgm = self->gameData.currentBgm;
        self->gameData.currentBgm = PL_BGM_NULL;
        self->update=&updateGame;
    }

    pl_drawTileMap(&(self->tilemap));
    pl_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->radiostars), &(self->gameData));
    drawPause(&(self->radiostars));
}

void drawPause(font_t *font){
    drawText(font, c555, str_pause, (TFT_WIDTH - textWidth(font, str_pause)) / 2, 128);
}

uint16_t getLevelIndex(uint8_t world, uint8_t level){
    return (world-1) * 4 + (level-1);
}
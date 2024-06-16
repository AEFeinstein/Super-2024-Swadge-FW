/**
 * @file pango.c
 * @author J.Vega (JVeg199X)
 * @brief Pango
 * @date 2024-05-04
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "pango.h"
#include "esp_random.h"

#include "pango_typedef.h"
#include "paTilemap.h"
#include "paGameData.h"
#include "paEntityManager.h"
#include "paLeveldef.h"

#include "hdw-led.h"
#include "palette.h"
#include "hdw-nvs.h"
#include "paSoundManager.h"
#include <inttypes.h>
#include "mainMenu.h"
#include "fill.h"

//==============================================================================
// Constants
//==============================================================================
#define BIG_SCORE    4000000UL
#define BIGGER_SCORE 10000000UL
#define FAST_TIME    1500 // 25 minutes

const char pangoName[] = "Pango";

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};

static const paletteColor_t redColors[4]    = {c501, c540, c550, c540};
static const paletteColor_t yellowColors[4] = {c550, c331, c550, c555};
static const paletteColor_t greenColors[4]  = {c555, c051, c030, c051};
static const paletteColor_t cyanColors[4]   = {c055, c455, c055, c033};
static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};
static const paletteColor_t rgbColors[4]    = {c500, c050, c005, c050};

static const int16_t cheatCode[11]
    = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};


#define DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH 8
#define DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH 2
static const uint8_t defaultEnemySpawnLocations[DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH * DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH] = {
    1,1,
    15,1,
    1,13,
    15,13,
    9,1,
    15,7,
    9,13,
    1,7//,
    //3,1
};

//==============================================================================
// Functions Prototypes
//==============================================================================

void pangoEnterMode(void);
void pangoExitMode(void);
void pangoMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)(pango_t* self);
struct pango_t
{
    font_t radiostars;

    paTilemap_t tilemap;
    paEntityManager_t entityManager;
    paGameData_t gameData;

    paSoundManager_t soundManager;

    uint8_t menuState;
    uint8_t menuSelection;
    uint8_t cheatCodeIdx;

    int16_t btnState;
    int16_t prevBtnState;

    int32_t frameTimer;

    pangoHighScores_t highScores;
    pangoUnlockables_t unlockables;
    bool easterEgg;

    gameUpdateFuncton_t update;
};

//==============================================================================
// Function Prototypes
//==============================================================================
void drawPangoHud(font_t* font, paGameData_t* gameData);
void drawPangoTitleScreen(font_t* font, paGameData_t* gameData);
void changeStateReadyScreen(pango_t* self);
void updateReadyScreen(pango_t* self);
void drawReadyScreen(font_t* font, paGameData_t* gameData);
void changeStateGame(pango_t* self);
void detectGameStateChange(pango_t* self);
void detectBgmChange(pango_t* self);
void changeStateDead(pango_t* self);
void updateDead(pango_t* self);
void changeStateGameOver(pango_t* self);
void updateGameOver(pango_t* self);
void drawGameOver(font_t* font, paGameData_t* gameData);
void changeStateTitleScreen(pango_t* self);
void changeStateLevelClear(pango_t* self);
void updateLevelClear(pango_t* self);
void drawLevelClear(font_t* font, paGameData_t* gameData);
void changeStateGameClear(pango_t* self);
void updateGameClear(pango_t* self);
void drawGameClear(font_t* font, paGameData_t* gameData);
void initializePangoHighScores(pango_t* self);
void loadPangoHighScores(pango_t* self);
void savePangoHighScores(pango_t* self);
void initializePangoUnlockables(pango_t* self);
void loadPangoUnlockables(pango_t* self);
void savePangoUnlockables(pango_t* self);
void drawPangoHighScores(font_t* font, pangoHighScores_t* highScores, paGameData_t* gameData);
uint8_t getHighScoreRank(pangoHighScores_t* highScores, uint32_t newScore);
void insertScoreIntoHighScores(pangoHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank);
void changeStateNameEntry(pango_t* self);
void updateNameEntry(pango_t* self);
void drawNameEntry(font_t* font, paGameData_t* gameData, uint8_t currentInitial);
void changeStateShowHighScores(pango_t* self);
void updateShowHighScores(pango_t* self);
void drawShowHighScores(font_t* font, uint8_t menuState);
void changeStatePause(pango_t* self);
void updatePause(pango_t* self);
void drawPause(font_t* font);
uint16_t getLevelIndex(uint8_t world, uint8_t level);

//==============================================================================
// Variables
//==============================================================================

pango_t* pango = NULL;

swadgeMode_t pangoMode = {.modeName                 = pangoName,
                               .wifiMode                 = NO_WIFI,
                               .overrideUsb              = false,
                               .usesAccelerometer        = false,
                               .usesThermometer          = false,
                               .fnEnterMode              = pangoEnterMode,
                               .fnExitMode               = pangoExitMode,
                               .fnMainLoop               = pangoMainLoop,
                               .fnAudioCallback          = NULL,
                               .fnBackgroundDrawCallback = NULL,
                               .fnEspNowRecvCb           = NULL,
                               .fnEspNowSendCb           = NULL};

#define NUM_LEVELS 16

static const paLeveldef_t leveldef[17] = {{.filename = "level1-1.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "dac01.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level1-3.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level1-4.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level2-1.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "dac03.bin", .timeLimit = 220, .checkpointTimeLimit = 90},
                                          {.filename = "level2-3.bin", .timeLimit = 200, .checkpointTimeLimit = 90},
                                          {.filename = "level2-4.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level3-1.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "dac02.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level3-3.bin", .timeLimit = 180, .checkpointTimeLimit = 90},
                                          {.filename = "level3-4.bin", .timeLimit = 220, .checkpointTimeLimit = 110},
                                          {.filename = "level4-1.bin", .timeLimit = 270, .checkpointTimeLimit = 90},
                                          {.filename = "level4-2.bin", .timeLimit = 240, .checkpointTimeLimit = 90},
                                          {.filename = "level4-3.bin", .timeLimit = 240, .checkpointTimeLimit = 90},
                                          {.filename = "level4-4.bin", .timeLimit = 240, .checkpointTimeLimit = 90},
                                          {.filename = "debug.bin", .timeLimit = 180, .checkpointTimeLimit = 90}};

led_t platLeds[CONFIG_NUM_LEDS];

static const char str_get_ready[]    = "Get Ready!";
static const char str_time_up[]      = "-Time Up!-";
static const char str_game_over[]    = "Game Over";
static const char str_well_done[]    = "Well done!";
static const char str_congrats[]     = "Congratulations!";
static const char str_initials[]     = "Enter your initials!";
static const char str_hbd[]          = "Happy Birthday, Evelyn!";
static const char str_registrated[]  = "Your name registrated.";
static const char str_do_your_best[] = "Do your best!";
static const char str_pause[]        = "-Pause-";

static const char KEY_SCORES[]  = "pf_scores";
static const char KEY_UNLOCKS[] = "pf_unlocks";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void pangoEnterMode(void)
{
    // Allocate memory for this mode
    pango = (pango_t*)calloc(1, sizeof(pango_t));
    memset(pango, 0, sizeof(pango_t));

    pango->menuState     = 0;
    pango->menuSelection = 0;
    pango->btnState      = 0;
    pango->prevBtnState  = 0;

    loadPangoHighScores(pango);
    loadPangoUnlockables(pango);
    if (pango->highScores.initials[0][0] == 'E' && pango->highScores.initials[0][1] == 'F'
        && pango->highScores.initials[0][2] == 'V')
    {
        pango->easterEgg = true;
    }

    loadFont("radiostars.font", &pango->radiostars, false);

    pa_initializeTileMap(&(pango->tilemap));
    pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
    pa_generateMaze(&(pango->tilemap));
    pango->tilemap.mapOffsetX = -4;

    pa_initializeSoundManager(&(pango->soundManager));

    pa_initializeGameData(&(pango->gameData), &(pango->soundManager));
    pa_initializeEntityManager(&(pango->entityManager), &(pango->tilemap), &(pango->gameData),
                               &(pango->soundManager));

    pango->tilemap.entityManager    = &(pango->entityManager);
    pango->tilemap.tileSpawnEnabled = true;

    setFrameRateUs(16666);

    pango->update = &updateTitleScreen;
}

/**
 * @brief TODO
 *
 */
void pangoExitMode(void)
{
    freeFont(&pango->radiostars);
    pa_freeTilemap(&(pango->tilemap));
    pa_freeSoundManager(&(pango->soundManager));
    pa_freeEntityManager(&(pango->entityManager));
    free(pango);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
void pangoMainLoop(int64_t elapsedUs)
{
    // Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        pango->btnState          = evt.state;
        pango->gameData.btnState = evt.state;
    }

    pango->update(pango);

    pango->prevBtnState          = pango->btnState;
    pango->gameData.prevBtnState = pango->prevBtnState;
}

/**
 * @brief TODO
 *
 * @param opt
 */
// void pangoCb(const char *opt)
// {
//     ESP_LOGI("MNU", "%s", opt);
// }

void updateGame(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    pa_updateEntities(&(self->entityManager));

    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));

    //drawEntityTargetTile(self->entityManager.playerEntity);

    detectGameStateChange(self);
    detectBgmChange(self);

    self->gameData.coins = self->entityManager.remainingEnemies;
    drawPangoHud(&(self->radiostars), &(self->gameData));

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
        self->gameData.countdown--;
        self->gameData.inGameTimer++;

        if (self->gameData.countdown < 10)
        {
            soundPlayBgm(&(self->soundManager.sndOuttaTime), BZR_STEREO);
        }

        if (self->gameData.countdown < 0)
        {
            killPlayer(self->entityManager.playerEntity);
        }

        pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
    }

    updateComboTimer(&(self->gameData));
}

void drawPangoHud(font_t* font, paGameData_t* gameData)
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

    if (gameData->frameCount > 29)
    {
        drawText(font, c500, "1UP", 24, 2);
    }

    drawText(font, c555, livesStr, 56, 2);
    drawText(font, c555, coinStr, 160, 16);
    drawText(font, c555, scoreStr, 8, 16);
    drawText(font, c555, levelStr, 152, 2);
    drawText(font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 220, 16);

    if (gameData->comboTimer == 0)
    {
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    drawText(font, (gameData->comboTimer < 60) ? c030 : greenColors[(pango->gameData.frameCount >> 3) % 4],
             scoreStr, 8, 30);
}

void updateTitleScreen(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    // Handle inputs
    switch (pango->menuState)
    {
        case 0:
        {
            if (self->gameData.frameCount > 600)
            {
                pa_resetGameDataLeds(&(self->gameData));
                changeStateShowHighScores(self);
            }

            if ((self->gameData.btnState & cheatCode[pango->cheatCodeIdx])
                && !(self->gameData.prevBtnState & cheatCode[pango->cheatCodeIdx]))
            {
                pango->cheatCodeIdx++;

                if (pango->cheatCodeIdx > 10)
                {
                    pango->cheatCodeIdx       = 0;
                    pango->menuState          = 1;
                    pango->gameData.debugMode = true;
                    soundPlaySfx(&(pango->soundManager.sndLevelClearS), BZR_STEREO);
                    break;
                }
                else
                {
                    soundPlaySfx(&(pango->soundManager.sndMenuSelect), BZR_STEREO);
                    break;
                }
            }
            else
            {
                if (!(self->gameData.frameCount % 150))
                {
                    pango->cheatCodeIdx = 0;
                }
            }

            if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
                || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A)))
            {
                soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                pango->menuState     = 1;
                pango->menuSelection = 0;
            }

            break;
        }
        case 1:
        {
            if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
                || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A)))
            {
                switch (self->menuSelection)
                {
                    case 0 ... 1:
                    {
                        uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);
                        if ((levelIndex >= NUM_LEVELS)
                            || (!self->gameData.debugMode && levelIndex > self->unlockables.maxLevelIndexUnlocked))
                        {
                            soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                            break;
                        }

                        /*if(self->menuSelection == 0){
                            self->gameData.world = 1;
                            self->gameData.level = 1;
                        }*/

                        pa_initializeGameDataFromTitleScreen(&(self->gameData));
                        self->entityManager.activeEnemies = 0;
                        pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
                        pa_generateMaze(&(pango->tilemap));
                        pa_placeEnemySpawns(&(pango->tilemap));

                        changeStateReadyScreen(self);
                        break;
                    }
                    case 2:
                    {
                        if (self->gameData.debugMode)
                        {
                            // Reset Progress
                            initializePangoUnlockables(self);
                            soundPlaySfx(&(self->soundManager.sndBreak), BZR_STEREO);
                        }
                        else
                        {
                            // Show High Scores
                            self->menuSelection = 0;
                            self->menuState     = 0;

                            changeStateShowHighScores(self);
                            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                        }
                        break;
                    }
                    case 3:
                    {
                        if (self->gameData.debugMode)
                        {
                            // Reset High Scores
                            initializePangoHighScores(self);
                            soundPlaySfx(&(self->soundManager.sndBreak), BZR_STEREO);
                        }
                        else
                        {
                            // Show Achievements
                            self->menuSelection = 0;
                            self->menuState     = 2;
                            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                        }
                        break;
                    }
                    case 4:
                    {
                        if (self->gameData.debugMode)
                        {
                            // Save & Quit
                            savePangoHighScores(self);
                            savePangoUnlockables(self);
                            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                            switchToSwadgeMode(&mainMenuMode);
                        }
                        else
                        {
                            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
                            switchToSwadgeMode(&mainMenuMode);
                        }
                        break;
                    }
                    default:
                    {
                        soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                        self->menuSelection = 0;
                    }
                }
            }
            else if ((self->gameData.btnState & PB_UP && !(self->gameData.prevBtnState & PB_UP)))
            {
                if (pango->menuSelection > 0)
                {
                    pango->menuSelection--;

                    if (!self->gameData.debugMode && pango->menuSelection == 1
                        && self->unlockables.maxLevelIndexUnlocked == 0)
                    {
                        pango->menuSelection--;
                    }

                    soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                }
            }
            else if ((self->gameData.btnState & PB_DOWN && !(self->gameData.prevBtnState & PB_DOWN)))
            {
                if (pango->menuSelection < 4)
                {
                    pango->menuSelection++;

                    if (!self->gameData.debugMode && pango->menuSelection == 1
                        && self->unlockables.maxLevelIndexUnlocked == 0)
                    {
                        pango->menuSelection++;
                    }

                    soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                }
                else
                {
                    soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                }
            }
            else if ((self->gameData.btnState & PB_LEFT && !(self->gameData.prevBtnState & PB_LEFT)))
            {
                if (pango->menuSelection == 1)
                {
                    if (pango->gameData.level == 1 && pango->gameData.world == 1)
                    {
                        soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                    }
                    else
                    {
                        pango->gameData.level--;
                        if (pango->gameData.level < 1)
                        {
                            pango->gameData.level = 4;
                            if (pango->gameData.world > 1)
                            {
                                pango->gameData.world--;
                            }
                        }
                        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                    }
                }
            }
            else if ((self->gameData.btnState & PB_RIGHT && !(self->gameData.prevBtnState & PB_RIGHT)))
            {
                if (pango->menuSelection == 1)
                {
                    if ((pango->gameData.level == 4 && pango->gameData.world == 4)
                        || (!pango->gameData.debugMode
                            && getLevelIndex(pango->gameData.world, pango->gameData.level + 1)
                                   > pango->unlockables.maxLevelIndexUnlocked))
                    {
                        soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
                    }
                    else
                    {
                        pango->gameData.level++;
                        if (pango->gameData.level > 4)
                        {
                            pango->gameData.level = 1;
                            if (pango->gameData.world < 8)
                            {
                                pango->gameData.world++;
                            }
                        }
                        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
                    }
                }
            }
            else if ((self->gameData.btnState & PB_B && !(self->gameData.prevBtnState & PB_B)))
            {
                self->gameData.frameCount = 0;
                pango->menuState     = 0;
                soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
            }
            break;
        }
        case 2:
        {
            if ((self->gameData.btnState & PB_B && !(self->gameData.prevBtnState & PB_B)))
            {
                self->gameData.frameCount = 0;
                pango->menuState     = 1;
                soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
            }
            break;
        }
        default:
            pango->menuState = 0;
            soundPlaySfx(&(pango->soundManager.sndMenuDeny), BZR_STEREO);
            break;
    }

    //pa_scrollTileMap(&(pango->tilemap), 1, 0);
    //if (self->tilemap.mapOffsetX >= self->tilemap.maxMapOffsetX && self->gameData.frameCount > 58)
    //{
    //    self->tilemap.mapOffsetX = 0;
    //}

    drawPangoTitleScreen(&(self->radiostars), &(self->gameData));

    if (((self->gameData.frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // self->gameData.leds[i].r = (( (self->gameData.frameCount >> 4) % NUM_LEDS) == i) ? 0xFF : 0x00;

            platLeds[i].r += (esp_random() % 1);
            platLeds[i].g += (esp_random() % 8);
            platLeds[i].b += (esp_random() % 8);
        }
    }
    setLeds(platLeds, CONFIG_NUM_LEDS);
}

void drawPangoTitleScreen(font_t* font, paGameData_t* gameData)
{
    pa_drawTileMap(&(pango->tilemap));

    drawText(font, c555, "<insert title here>", 40, 32);

    if (pango->gameData.debugMode)
    {
        drawText(font, c555, "Debug Mode", 80, 48);
    }

    switch (pango->menuState)
    {
        case 0:
        {
            if ((gameData->frameCount % 60) < 30)
            {
                drawText(font, c555, "- Press START button -", 20, 128);
            }
            break;
        }

        case 1:
        {
            drawText(font, c555, "Start Game", 48, 128);

            if (pango->gameData.debugMode || pango->unlockables.maxLevelIndexUnlocked > 0)
            {
                char levelStr[24];
                snprintf(levelStr, sizeof(levelStr) - 1, "Level Select: %d-%d", gameData->world, gameData->level);
                drawText(font, c555, levelStr, 48, 144);
            }

            if (pango->gameData.debugMode)
            {
                drawText(font, c555, "Reset Progress", 48, 160);
                drawText(font, c555, "Reset High Scores", 48, 176);
                drawText(font, c555, "Save & Exit to Menu", 48, 192);
            }
            else
            {
                drawText(font, c555, "High Scores", 48, 160);
                drawText(font, c555, "Achievements", 48, 176);
                drawText(font, c555, "Exit to Menu", 48, 192);
            }

            drawText(font, c555, "->", 32, 128 + pango->menuSelection * 16);

            break;
        }

        case 2:
        {
            if (pango->unlockables.gameCleared)
            {
                drawText(font, redColors[(gameData->frameCount >> 3) % 4], "Beat the game!", 48, 80);
            }

            if (pango->unlockables.oneCreditCleared)
            {
                drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "1 Credit Clear!", 48, 96);
            }

            if (pango->unlockables.bigScore)
            {
                drawText(font, greenColors[(gameData->frameCount >> 3) % 4], "Got 4 million points!", 48, 112);
            }

            if (pango->unlockables.biggerScore)
            {
                drawText(font, cyanColors[(gameData->frameCount >> 3) % 4], "Got 10 million points!", 48, 128);
            }

            if (pango->unlockables.fastTime)
            {
                drawText(font, purpleColors[(gameData->frameCount >> 3) % 4], "Beat within 25 min!", 48, 144);
            }

            if (pango->unlockables.gameCleared && pango->unlockables.oneCreditCleared
                && pango->unlockables.bigScore && pango->unlockables.biggerScore
                && pango->unlockables.fastTime)
            {
                drawText(font, rgbColors[(gameData->frameCount >> 3) % 4], "100% 100% 100%", 48, 160);
            }

            drawText(font, c555, "Press B to Return", 48, 192);
            break;
        }

        default:
            break;
    }
}

void changeStateReadyScreen(pango_t* self)
{
    self->gameData.frameCount = 0;

    soundPlayBgm(&(self->soundManager.bgmIntro), BZR_STEREO);

    pa_resetGameDataLeds(&(self->gameData));

    self->update = &updateReadyScreen;
}

void updateReadyScreen(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        soundStop(true);
        changeStateGame(self);
    }

    drawReadyScreen(&(self->radiostars), &(self->gameData));
}

void drawReadyScreen(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);
    int16_t xOff = (TFT_WIDTH - textWidth(font, str_get_ready)) / 2;
    drawText(font, c555, str_get_ready, xOff, 128);

    /*if (getLevelIndex(gameData->world, gameData->level) == 0)
    {
        drawText(font, c555, "A: Jump", xOff, 128 + (font->height + 3) * 3);
        drawText(font, c555, "B: Run / Fire", xOff, 128 + (font->height + 3) * 4);
    }*/
}

void changeStateGame(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.currentBgm = 0;
    pa_resetGameDataLeds(&(self->gameData));

    pa_deactivateAllEntities(&(self->entityManager), false);

    uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);
    // pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
    // pa_generateMaze(&(pango->tilemap));
    // pa_placeEnemySpawns(&(pango->tilemap));

    self->gameData.countdown = leveldef[levelIndex].timeLimit;

    paEntityManager_t* entityManager = &(self->entityManager);
    entityManager->viewEntity
        = pa_createPlayer(entityManager, (9 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                          (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    entityManager->playerEntity     = entityManager->viewEntity;
    entityManager->playerEntity->hp = self->gameData.initialHp;

    if(entityManager->activeEnemies == 0){
        for(uint16_t i = 0; i<self->entityManager.maxEnemies; i++){
            pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
        }
    } else {
        for(uint16_t i = 0; i<entityManager->activeEnemies; i++){
            if(i >= DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH){
                entityManager->activeEnemies--;
                continue;
            }

            createTestObject(&(self->entityManager), (defaultEnemySpawnLocations[i*2] << PA_TILE_SIZE_IN_POWERS_OF_2) + 8, (defaultEnemySpawnLocations[i*2+1] << PA_TILE_SIZE_IN_POWERS_OF_2) + 8);
        }

    }


    
    //pa_viewFollowEntity(&(self->tilemap), entityManager->playerEntity);

    pa_updateLedsHpMeter(&(self->entityManager), &(self->gameData));

    self->tilemap.executeTileSpawnAll = true;

    self->update = &updateGame;
}

void detectGameStateChange(pango_t* self)
{
    if (!self->gameData.changeState)
    {
        return;
    }

    switch (self->gameData.changeState)
    {
        case PA_ST_DEAD:
            changeStateDead(self);
            break;

        case PA_ST_READY_SCREEN:
            changeStateReadyScreen(self);
            break;

        case PA_ST_LEVEL_CLEAR:
            changeStateLevelClear(self);
            break;

        case PA_ST_PAUSE:
            changeStatePause(self);
            break;

        default:
            break;
    }

    self->gameData.changeState = 0;
}

void detectBgmChange(pango_t* self)
{
    if (!self->gameData.changeBgm)
    {
        return;
    }

    switch (self->gameData.changeBgm)
    {
        case PA_BGM_NULL:
            if (self->gameData.currentBgm != PA_BGM_NULL)
            {
                soundStop(true);
            }
            break;

        case PA_BGM_MAIN:
            if (self->gameData.currentBgm != PA_BGM_MAIN)
            {
                soundPlayBgm(&(self->soundManager.bgmDemagio), BZR_STEREO);
            }
            break;

        case PA_BGM_ATHLETIC:
            if (self->gameData.currentBgm != PA_BGM_ATHLETIC)
            {
                soundPlayBgm(&(self->soundManager.bgmSmooth), BZR_STEREO);
            }
            break;

        case PA_BGM_UNDERGROUND:
            if (self->gameData.currentBgm != PA_BGM_UNDERGROUND)
            {
                soundPlayBgm(&(self->soundManager.bgmUnderground), BZR_STEREO);
            }
            break;

        case PA_BGM_FORTRESS:
            if (self->gameData.currentBgm != PA_BGM_FORTRESS)
            {
                soundPlayBgm(&(self->soundManager.bgmCastle), BZR_STEREO);
            }
            break;

        default:
            break;
    }

    self->gameData.currentBgm = self->gameData.changeBgm;
    self->gameData.changeBgm  = 0;
}

void changeStateDead(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    self->gameData.levelDeaths++;
    self->gameData.combo      = 0;
    self->gameData.comboTimer = 0;
    self->gameData.initialHp  = 1;

    soundStop(true);
    soundPlayBgm(&(self->soundManager.sndDie), BZR_STEREO);

    self->update = &updateDead;
}

void updateDead(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        if (self->gameData.lives > 0)
        {
            changeStateReadyScreen(self);
        }
        else
        {
            changeStateGameOver(self);
        }
    }

    pa_updateEntities(&(self->entityManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawPangoHud(&(self->radiostars), &(self->gameData));

    if (self->gameData.countdown < 0)
    {
        drawText(&(self->radiostars), c555, str_time_up, (TFT_WIDTH - textWidth(&(self->radiostars), str_time_up)) / 2,
                 128);
    }
}

void updateGameOver(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        // Handle unlockables

        if (self->gameData.score >= BIG_SCORE)
        {
            self->unlockables.bigScore = true;
        }

        if (self->gameData.score >= BIGGER_SCORE)
        {
            self->unlockables.biggerScore = true;
        }

        if (!self->gameData.debugMode)
        {
            savePangoUnlockables(self);
        }

        changeStateNameEntry(self);
    }

    drawGameOver(&(self->radiostars), &(self->gameData));
    pa_updateLedsGameOver(&(self->gameData));
}

void changeStateGameOver(pango_t* self)
{
    self->gameData.frameCount = 0;
    pa_resetGameDataLeds(&(self->gameData));
    soundPlayBgm(&(self->soundManager.bgmGameOver), BZR_STEREO);
    self->update = &updateGameOver;
}

void drawGameOver(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);
    drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
}

void changeStateTitleScreen(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateTitleScreen;
}

void changeStateLevelClear(pango_t* self)
{
    self->gameData.frameCount         = 0;
    self->gameData.checkpoint         = 0;
    self->gameData.levelDeaths        = 0;
    self->gameData.initialHp          = self->entityManager.playerEntity->hp;
    self->gameData.extraLifeCollected = false;
    pa_resetGameDataLeds(&(self->gameData));
    self->update = &updateLevelClear;
}

void updateLevelClear(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if (self->gameData.frameCount > 60)
    {
        if (self->gameData.countdown > 0)
        {
            self->gameData.countdown--;

            if (self->gameData.countdown % 2)
            {
                soundPlayBgm(&(self->soundManager.sndTally), BZR_STEREO);
            }

            uint16_t comboPoints = 50 * self->gameData.combo;

            self->gameData.score += comboPoints;
            self->gameData.comboScore = comboPoints;

            if (self->gameData.combo > 1)
            {
                self->gameData.combo--;
            }
        }
        else if (self->gameData.frameCount % 60 == 0)
        {
            // Hey look, it's a frame rule!

            uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level);

            if (levelIndex >= NUM_LEVELS - 1)
            {
                // Game Cleared!

                if (!self->gameData.debugMode)
                {
                    // Determine achievements
                    self->unlockables.gameCleared = true;

                    if (!self->gameData.continuesUsed)
                    {
                        self->unlockables.oneCreditCleared = true;

                        if (self->gameData.inGameTimer < FAST_TIME)
                        {
                            self->unlockables.fastTime = true;
                        }
                    }

                    if (self->gameData.score >= BIG_SCORE)
                    {
                        self->unlockables.bigScore = true;
                    }

                    if (self->gameData.score >= BIGGER_SCORE)
                    {
                        self->unlockables.biggerScore = true;
                    }
                }

                changeStateGameClear(self);
            }
            else
            {
                // Advance to the next level
                self->gameData.level++;
                if (self->gameData.level > 4)
                {
                    self->gameData.world++;
                    self->gameData.level = 1;
                }

                // Unlock the next level
                levelIndex++;
                if (levelIndex > self->unlockables.maxLevelIndexUnlocked)
                {
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }

                pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
                pa_generateMaze(&(pango->tilemap));
                pa_placeEnemySpawns(&(pango->tilemap));
                self->entityManager.activeEnemies = 0;

                changeStateReadyScreen(self);
            }

            if (!self->gameData.debugMode)
            {
                savePangoUnlockables(self);
            }
        }
    }

    pa_updateEntities(&(self->entityManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawPangoHud(&(self->radiostars), &(self->gameData));
    drawLevelClear(&(self->radiostars), &(self->gameData));
    pa_updateLedsLevelClear(&(self->gameData));
}

void drawLevelClear(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);
    drawText(font, c555, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) / 2, 128);
}

void changeStateGameClear(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateGameClear;
    pa_resetGameDataLeds(&(self->gameData));
    soundPlayBgm(&(self->soundManager.bgmSmooth), BZR_STEREO);
}

void updateGameClear(pango_t* self)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if (self->gameData.frameCount > 450)
    {
        if (self->gameData.lives > 0)
        {
            if (self->gameData.frameCount % 60 == 0)
            {
                self->gameData.lives--;
                self->gameData.score += 200000;
                soundPlaySfx(&(self->soundManager.snd1up), BZR_STEREO);
            }
        }
        else if (self->gameData.frameCount % 960 == 0)
        {
            changeStateGameOver(self);
        }
    }

    drawPangoHud(&(self->radiostars), &(self->gameData));
    drawGameClear(&(self->radiostars), &(self->gameData));
    pa_updateLedsGameClear(&(self->gameData));
}

void drawGameClear(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr) - 1, "in %06" PRIu32 " seconds!", gameData->inGameTimer);

    drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], str_congrats,
             (TFT_WIDTH - textWidth(font, str_congrats)) / 2, 48);

    if (gameData->frameCount > 120)
    {
        drawText(font, c555, "You've completed your", 8, 80);
        drawText(font, c555, "trip across Swadge Land", 8, 96);
    }

    if (gameData->frameCount > 180)
    {
        drawText(font, (gameData->inGameTimer < FAST_TIME) ? cyanColors[(gameData->frameCount >> 3) % 4] : c555,
                 timeStr, (TFT_WIDTH - textWidth(font, timeStr)) / 2, 112);
    }

    if (gameData->frameCount > 300)
    {
        drawText(font, c555, "The Swadge staff", 8, 144);
        drawText(font, c555, "thanks you for playing!", 8, 160);
    }

    if (gameData->frameCount > 420)
    {
        drawText(font, (gameData->lives > 0) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555,
                 "Bonus 200000pts per life!", (TFT_WIDTH - textWidth(font, "Bonus 100000pts per life!")) / 2, 192);
    }

    /*
    drawText(font, c555, "Thanks for playing.", 24, 48);
    drawText(font, c555, "Many more battle scenes", 8, 96);
    drawText(font, c555, "will soon be available!", 8, 112);
    drawText(font, c555, "Bonus 100000pts per life!", 8, 160);
    */
}

void initializePangoHighScores(pango_t* self)
{
    self->highScores.scores[0] = 100000;
    self->highScores.scores[1] = 80000;
    self->highScores.scores[2] = 40000;
    self->highScores.scores[3] = 20000;
    self->highScores.scores[4] = 10000;

    for (uint8_t i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    {
        self->highScores.initials[i][0] = 'J' + i;
        self->highScores.initials[i][1] = 'P' - i;
        self->highScores.initials[i][2] = 'V' + i;
    }
}

void loadPangoHighScores(pango_t* self)
{
    size_t size = sizeof(pangoHighScores_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_SCORES, &(self->highScores), &(size)))
    {
        // Value didn't exist, so write the default
        initializePangoHighScores(self);
    }
}

void savePangoHighScores(pango_t* self)
{
    size_t size = sizeof(pangoHighScores_t);
    writeNvsBlob(KEY_SCORES, &(self->highScores), size);
}

void initializePangoUnlockables(pango_t* self)
{
    self->unlockables.maxLevelIndexUnlocked = 0;
    self->unlockables.gameCleared           = false;
    self->unlockables.oneCreditCleared      = false;
    self->unlockables.bigScore              = false;
    self->unlockables.fastTime              = false;
    self->unlockables.biggerScore           = false;
}

void loadPangoUnlockables(pango_t* self)
{
    size_t size = sizeof(pangoUnlockables_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_UNLOCKS, &(self->unlockables), &(size)))
    {
        // Value didn't exist, so write the default
        initializePangoUnlockables(self);
    }
}

void savePangoUnlockables(pango_t* self)
{
    size_t size = sizeof(pangoUnlockables_t);
    writeNvsBlob(KEY_UNLOCKS, &(self->unlockables), size);
}

void drawPangoHighScores(font_t* font, pangoHighScores_t* highScores, paGameData_t* gameData)
{
    drawText(font, c555, "RANK  SCORE  NAME", 48, 96);
    for (uint8_t i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    {
        char rowStr[32];
        snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32 "   %c%c%c", i + 1, highScores->scores[i],
                 highScores->initials[i][0], highScores->initials[i][1], highScores->initials[i][2]);
        drawText(font, (gameData->rank == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 60, 128 + i * 16);
    }
}

uint8_t getHighScoreRank(pangoHighScores_t* highScores, uint32_t newScore)
{
    uint8_t i;
    for (i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    {
        if (highScores->scores[i] < newScore)
        {
            break;
        }
    }

    return i;
}

void insertScoreIntoHighScores(pangoHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank)
{
    if (rank >= NUM_PLATFORMER_HIGH_SCORES)
    {
        return;
    }

    for (uint8_t i = NUM_PLATFORMER_HIGH_SCORES - 1; i > rank; i--)
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

void changeStateNameEntry(pango_t* self)
{
    self->gameData.frameCount = 0;
    uint8_t rank              = getHighScoreRank(&(self->highScores), self->gameData.score);
    self->gameData.rank       = rank;
    self->menuState           = 0;

    pa_resetGameDataLeds(&(self->gameData));

    if (rank >= NUM_PLATFORMER_HIGH_SCORES || self->gameData.debugMode)
    {
        self->menuSelection = 0;
        self->gameData.rank = NUM_PLATFORMER_HIGH_SCORES;
        changeStateShowHighScores(self);
        return;
    }

    soundPlayBgm(&(self->soundManager.bgmNameEntry), BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update        = &updateNameEntry;
}

void updateNameEntry(pango_t* self)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if (self->gameData.btnState & PB_LEFT && !(self->gameData.prevBtnState & PB_LEFT))
    {
        self->menuSelection--;

        if (self->menuSelection < 32)
        {
            self->menuSelection = 90;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_RIGHT && !(self->gameData.prevBtnState & PB_RIGHT))
    {
        self->menuSelection++;

        if (self->menuSelection > 90)
        {
            self->menuSelection = 32;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_B && !(self->gameData.prevBtnState & PB_B))
    {
        if (self->menuState > 0)
        {
            self->menuState--;
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
        else
        {
            soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
        }
    }
    else if (self->gameData.btnState & PB_A && !(self->gameData.prevBtnState & PB_A))
    {
        self->menuState++;

        if (self->menuState > 2)
        {
            insertScoreIntoHighScores(&(self->highScores), self->gameData.score, self->gameData.initials,
                                      self->gameData.rank);
            savePangoHighScores(self);
            changeStateShowHighScores(self);
            soundPlaySfx(&(self->soundManager.sndPowerUp), BZR_STEREO);
        }
        else
        {
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
    }

    drawNameEntry(&(self->radiostars), &(self->gameData), self->menuState);
    pa_updateLedsShowHighScores(&(self->gameData));
}

void drawNameEntry(font_t* font, paGameData_t* gameData, uint8_t currentInitial)
{
    drawText(font, greenColors[(pango->gameData.frameCount >> 3) % 4], str_initials,
             (TFT_WIDTH - textWidth(font, str_initials)) / 2, 64);

    char rowStr[32];
    snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32, gameData->rank + 1, gameData->score);
    drawText(font, c555, rowStr, 64, 128);

    for (uint8_t i = 0; i < 3; i++)
    {
        snprintf(rowStr, sizeof(rowStr) - 1, "%c", gameData->initials[i]);
        drawText(font, (currentInitial == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 192 + 16 * i, 128);
    }
}

void changeStateShowHighScores(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateShowHighScores;
}

void updateShowHighScores(pango_t* self)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if ((self->gameData.frameCount > 300)
        || (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
            || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))))
    {
        self->menuState     = 0;
        self->menuSelection = 0;
        soundStop(true);
        changeStateTitleScreen(self);
    }

    drawShowHighScores(&(self->radiostars), self->menuState);
    drawPangoHighScores(&(self->radiostars), &(self->highScores), &(self->gameData));

    pa_updateLedsShowHighScores(&(self->gameData));
}

void drawShowHighScores(font_t* font, uint8_t menuState)
{
    if (pango->easterEgg)
    {
        drawText(font, highScoreNewEntryColors[(pango->gameData.frameCount >> 3) % 4], str_hbd,
                 (TFT_WIDTH - textWidth(font, str_hbd)) / 2, 32);
    }
    else if (menuState == 3)
    {
        drawText(font, redColors[(pango->gameData.frameCount >> 3) % 4], str_registrated,
                 (TFT_WIDTH - textWidth(font, str_registrated)) / 2, 32);
    }
    else
    {
        drawText(font, c555, str_do_your_best, (TFT_WIDTH - textWidth(font, str_do_your_best)) / 2, 32);
    }
}

void changeStatePause(pango_t* self)
{
    soundStop(true);
    soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
    self->update = &updatePause;
}

void updatePause(pango_t* self)
{
    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
        self->gameData.changeBgm  = self->gameData.currentBgm;
        self->gameData.currentBgm = PA_BGM_NULL;
        self->update              = &updateGame;
    }

    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawPangoHud(&(self->radiostars), &(self->gameData));
    drawPause(&(self->radiostars));
}

void drawPause(font_t* font)
{
    drawText(font, c555, str_pause, (TFT_WIDTH - textWidth(font, str_pause)) / 2, 128);
}

uint16_t getLevelIndex(uint8_t world, uint8_t level)
{
    return (world - 1) * 4 + (level - 1);
}
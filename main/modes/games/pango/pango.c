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
#include "paWsgManager.h"
#include "paTilemap.h"
#include "paGameData.h"
#include "paEntityManager.h"

#include "hdw-led.h"
#include "palette.h"
#include "hdw-nvs.h"
#include "paSoundManager.h"
#include <inttypes.h>
#include "mainMenu.h"
#include "fill.h"
#include "paTables.h"

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
//static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};
//static const paletteColor_t rgbColors[4]    = {c500, c050, c005, c050};

static const int16_t cheatCode[11]
    = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};

//==============================================================================
// Functions Prototypes
//==============================================================================

void pangoEnterMode(void);
void pangoExitMode(void);
void pangoMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef void (*pa_gameUpdateFuncton_t)(pango_t* self, int64_t elapsedUs);
struct pango_t
{
    font_t font;

    paWsgManager_t wsgManager;
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

    pa_gameUpdateFuncton_t update;

    menuManiaRenderer_t* menuRenderer;
    menu_t* menu;
    menuItem_t* levelSelectMenuItem;
};

//==============================================================================
// Function Prototypes
//==============================================================================
void drawPangoHud(font_t* font, paGameData_t* gameData);
void drawPangoTitleScreen(font_t* font, paGameData_t* gameData);
void pangoBuildMainMenu(pango_t* self);
static void pangoUpdateMainMenu(pango_t* self, int64_t elapsedUs);
void changeStateReadyScreen(pango_t* self);
void updateReadyScreen(pango_t* self, int64_t elapsedUs);
void drawReadyScreen(font_t* font, paGameData_t* gameData);
void changeStateGame(pango_t* self);
void detectGameStateChange(pango_t* self);
void detectBgmChange(pango_t* self);
void changeStateDead(pango_t* self);
void updateDead(pango_t* self, int64_t elapsedUs);
void changeStateGameOver(pango_t* self);
void updateGameOver(pango_t* self, int64_t elapsedUs);
void drawGameOver(font_t* font, paGameData_t* gameData);
void changeStateTitleScreen(pango_t* self);
void changeStateLevelClear(pango_t* self);
void updateLevelClear(pango_t* self, int64_t elapsedUs);
void drawLevelClear(font_t* font, paGameData_t* gameData);
void changeStateGameClear(pango_t* self);
void updateGameClear(pango_t* self, int64_t elapsedUs);
void drawGameClear(font_t* font, paGameData_t* gameData);
void pangoInitializeHighScores(pango_t* self);
void loadPangoHighScores(pango_t* self);
void pangoSaveHighScores(pango_t* self);
void pangoInitializeUnlockables(pango_t* self);
void loadPangoUnlockables(pango_t* self);
void pangoSaveUnlockables(pango_t* self);
void drawPangoHighScores(font_t* font, pangoHighScores_t* highScores, paGameData_t* gameData);
uint8_t getHighScoreRank(pangoHighScores_t* highScores, uint32_t newScore);
void insertScoreIntoHighScores(pangoHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank);
void changeStateNameEntry(pango_t* self);
void updateNameEntry(pango_t* self, int64_t elapsedUs);
void drawNameEntry(font_t* font, paGameData_t* gameData, uint8_t currentInitial);
void pangoChangeStateShowHighScores(pango_t* self);
void updateShowHighScores(pango_t* self, int64_t elapsedUs);
void drawShowHighScores(font_t* font, uint8_t menuState);
void changeStatePause(pango_t* self);
void updatePause(pango_t* self, int64_t elapsedUs);
void drawPause(font_t* font);
uint16_t getLevelIndex(uint8_t world, uint8_t level);
void pangoChangeStateMainMenu(pango_t* self);

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

led_t paLeds[CONFIG_NUM_LEDS];

static const char str_ready[]        = "Ready?";
static const char str_set[]          = "Set...";
static const char str_pango[]        = "PANGO!";
static const char str_time_up[]      = "-Time Up!-";
static const char str_game_over[]    = "Game Over";
static const char str_well_done[]    = "Nice Clear!";
static const char str_congrats[]     = "Congratulations!";
static const char str_initials[]     = "Enter your initials!";
static const char str_hbd[]          = "Happy Birthday, Evelyn!";
static const char str_registrated[]  = "Your name registrated.";
static const char str_do_your_best[] = "Do your best!";
static const char str_pause[]        = "-Pause-";

static const char pangoMenuNewGame[]       = "New Game";
static const char pangoMenuContinue[]      = "Continue - Lv";
static const char pangoMenuCharacter[]     = "Character";
static const char pangoMenuHighScores[]    = "High Scores";
static const char pangoMenuResetScores[]   = "Reset Scores";
static const char pangoMenuResetProgress[] = "Reset Progress";
static const char pangoMenuExit[]          = "Exit";
static const char pangoMenuSaveAndExit[]   = "Save & Exit";

static const char KEY_SCORES[]  = "pa_scores";
static const char KEY_UNLOCKS[] = "pa_unlocks";

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

    loadFont("pango-fw.font", &pango->font, false);
    pango->menuRenderer = initMenuManiaRenderer(&pango->font, &pango->font, &pango->font);

    pa_initializeWsgManager(&(pango->wsgManager));

    pa_initializeTileMap(&(pango->tilemap), &(pango->wsgManager));
    pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
    pa_generateMaze(&(pango->tilemap));
    pango->tilemap.mapOffsetX = -4;

    pa_initializeSoundManager(&(pango->soundManager));

    pa_initializeGameData(&(pango->gameData), &(pango->soundManager));
    loadFont("tiny_numbers.font", &(pango->gameData.scoreFont), false);
    pa_initializeEntityManager(&(pango->entityManager), &(pango->wsgManager), &(pango->tilemap), &(pango->gameData),
                               &(pango->soundManager));

    pango->tilemap.entityManager    = &(pango->entityManager);
    pango->tilemap.tileSpawnEnabled = true;

    setFrameRateUs(16666);

    pango->menu = NULL;
    changeStateTitleScreen(pango);
}

/**
 * @brief TODO
 *
 */
void pangoExitMode(void)
{
    // deinitMenu can't set menu pointer to NULL,
    // so this is the only way to know that the menu has not been previously freed.
    if (pango->update == &pangoUpdateMainMenu)
    {
        // Deinitialize the menu.
        // This will also free the "level select" menu item.
        deinitMenu(pango->menu);
    }
    deinitMenuManiaRenderer(pango->menuRenderer);

    freeFont(&pango->font);
    pa_freeWsgManager(&(pango->wsgManager));
    pa_freeTilemap(&(pango->tilemap));
    pa_freeSoundManager(&(pango->soundManager));
    pa_freeEntityManager(&(pango->entityManager));
    free(pango);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void pangoMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == pangoMenuNewGame)
        {
            pango->gameData.world              = 1;
            pango->gameData.level              = 1;
            pango->entityManager.activeEnemies = 0;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData), 0);
            pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
            pa_generateMaze(&(pango->tilemap));
            pa_placeEnemySpawns(&(pango->tilemap));

            changeStateReadyScreen(pango);
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuContinue)
        {
            pango->gameData.world = 1;
            pango->gameData.level = settingVal;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData), settingVal);
            pango->entityManager.activeEnemies = 0;
            pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
            pa_generateMaze(&(pango->tilemap));
            pa_placeEnemySpawns(&(pango->tilemap));

            changeStateReadyScreen(pango);
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuCharacter)
        {
            pa_remapPlayerCharacter(&(pango->wsgManager), 16 * settingVal);
            soundPlaySfx(&(pango->soundManager.sndMenuConfirm), BZR_STEREO);
        }
        else if (label == pangoMenuHighScores)
        {
            pangoChangeStateShowHighScores(pango);
            pango->gameData.btnState = 0;
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuResetScores)
        {
            pangoInitializeHighScores(pango);
            // soundPlaySfx(&(pango->soundManager.detonate), BZR_STEREO);
        }
        else if (label == pangoMenuResetProgress)
        {
            pangoInitializeUnlockables(pango);
            // soundPlaySfx(&(pango->soundManager.die), BZR_STEREO);
        }
        else if (label == pangoMenuSaveAndExit)
        {
            pangoSaveHighScores(pango);
            pangoSaveUnlockables(pango);
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == pangoMenuExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        // soundPlaySfx(&(pango->soundManager.hit3), BZR_STEREO);
    }
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

        if (pango->update == &pangoUpdateMainMenu)
        {
            // Pass button events to the menu
            pango->menu = menuButton(pango->menu, evt);
        }
    }

    pango->update(pango, elapsedUs);

    pango->prevBtnState          = pango->btnState;
    pango->gameData.prevBtnState = pango->prevBtnState;
}

void pangoChangeStateMainMenu(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &pangoUpdateMainMenu;
    pangoBuildMainMenu(pango);
}

void pangoBuildMainMenu(pango_t* self)
{
    // Initialize the menu
    pango->menu = initMenu(pangoName, pangoMenuCb);
    addSingleItemToMenu(pango->menu, pangoMenuNewGame);

    /*
        Manually allocate and build "level select" menu item
        because the max setting will have to change as levels are unlocked
    */
    if (pango->unlockables.maxLevelIndexUnlocked > 1 || pango->gameData.debugMode)
    {
        pango->levelSelectMenuItem             = calloc(1, sizeof(menuItem_t));
        pango->levelSelectMenuItem->label      = pangoMenuContinue;
        pango->levelSelectMenuItem->minSetting = 1;
        pango->levelSelectMenuItem->maxSetting
            = (pango->gameData.debugMode) ? NUM_LEVELS - 1 : pango->unlockables.maxLevelIndexUnlocked;
        pango->levelSelectMenuItem->currentSetting
            = (pango->gameData.level == 1) ? pango->levelSelectMenuItem->maxSetting : pango->gameData.level;
        pango->levelSelectMenuItem->options = NULL;
        pango->levelSelectMenuItem->subMenu = NULL;

        push(pango->menu->items, pango->levelSelectMenuItem);
    }

    settingParam_t characterSettingBounds = {
        .def = 0,
        .min = 0,
        .max = 3,
        .key = NULL,
    };
    addSettingsItemToMenu(pango->menu, pangoMenuCharacter, &characterSettingBounds, 0);

    addSingleItemToMenu(pango->menu, pangoMenuHighScores);

    if (pango->gameData.debugMode)
    {
        addSingleItemToMenu(pango->menu, pangoMenuResetProgress);
        addSingleItemToMenu(pango->menu, pangoMenuResetScores);
        addSingleItemToMenu(pango->menu, pangoMenuSaveAndExit);
    }
    else
    {
        addSingleItemToMenu(pango->menu, pangoMenuExit);
    }
}

static void pangoUpdateMainMenu(pango_t* self, int64_t elapsedUs)
{
    // Draw the menu
    drawMenuMania(pango->menu, pango->menuRenderer, elapsedUs);
}

void updateGame(pango_t* self, int64_t elapsedUs)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    pa_updateEntities(&(self->entityManager));

    pa_animateTiles(&(self->wsgManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));

    // drawEntityTargetTile(self->entityManager.playerEntity);

    detectGameStateChange(self);
    detectBgmChange(self);

    // self->gameData.coins = self->gameData.remainingEnemies;
    self->gameData.coins = self->entityManager.aggroEnemies;
    drawPangoHud(&(self->font), &(self->gameData));

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

    drawText(font, c553, livesStr, 56, 2);
    // drawText(font, c553, coinStr, 160, 224);
    drawText(font, c553, scoreStr, 112, 2);
    drawText(font, c553, levelStr, 32, 226);
    drawText(font, (gameData->countdown > 30) ? c553 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 200, 226);

    if (gameData->comboTimer == 0)
    {
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 /*" (x%d)"*/, gameData->comboScore /*, gameData->combo*/);
    drawText(font, (gameData->comboTimer < 60) ? c030 : greenColors[(pango->gameData.frameCount >> 3) % 4], scoreStr,
             190, 2);
}

void updateTitleScreen(pango_t* self, int64_t elapsedUs)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if (self->gameData.frameCount > 600)
    {
        // resetGameDataLeds(&(self->gameData));
        pango->menuSelection = 0;
        pangoChangeStateShowHighScores(self);

        return;
    }

    if ((self->gameData.btnState & cheatCode[pango->menuSelection])
        && !(self->gameData.prevBtnState & cheatCode[pango->menuSelection]))
    {
        pango->menuSelection++;

        if (pango->menuSelection > 8)
        {
            pango->menuSelection      = 0;
            pango->menuState          = 1;
            pango->gameData.debugMode = true;
            // soundPlaySfx(&(pango->soundManager.levelClear), BZR_STEREO);
        }
        else
        {
            // soundPlaySfx(&(pango->soundManager.hit3), BZR_STEREO);
        }
    }

    if ((((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
         || ((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))))
    {
        self->gameData.btnState = 0;
        pango->menuSelection    = 0;

        if (!pango->gameData.debugMode)
        {
            // soundPlaySfx(&(pango->soundManager.launch), BZR_STEREO);
        }

        pangoChangeStateMainMenu(self);
        return;
    }

    drawPangoTitleScreen(&(self->font), &(self->gameData));

    if (((self->gameData.frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // self->gameData.leds[i].r = (( (self->gameData.frameCount >> 4) % NUM_LEDS) == i) ? 0xFF : 0x00;

            paLeds[i].r += (esp_random() % 1);
            paLeds[i].g += (esp_random() % 8);
            paLeds[i].b += (esp_random() % 8);
        }
    }
    setLeds(paLeds, CONFIG_NUM_LEDS);
}

void drawPangoTitleScreen(font_t* font, paGameData_t* gameData)
{
    // pa_drawTileMap(&(pango->tilemap));

    drawText(font, c555, "P A N G O", 96, 32);

    if (pango->gameData.debugMode)
    {
        drawText(font, c555, "Debug Mode", 80, 48);
    }

    if ((gameData->frameCount % 60) < 30)
    {
        drawText(font, c555, "- Press START button -", 20, 128);
    }

}

void changeStateReadyScreen(pango_t* self)
{
    self->gameData.frameCount = 0;

    soundPlayBgm(&(self->soundManager.bgmIntro), BZR_STEREO);

    pa_resetGameDataLeds(&(self->gameData));

    self->update = &updateReadyScreen;
}

void updateReadyScreen(pango_t* self, int64_t elapsedUs)
{
    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        soundStop(true);
        changeStateGame(self);
    }

    drawReadyScreen(&(self->font), &(self->gameData));
}

void drawReadyScreen(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);

    drawText(font, c555, str_ready, 80, 96);

    if (gameData->frameCount > 60)
    {
        drawText(font, c555, str_set, 112, 128);
    }

    if (gameData->frameCount > 120)
    {
        drawText(font, c555, str_pango, 144, 160);
    }

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

    self->gameData.countdown = 60;

    paEntityManager_t* entityManager = &(self->entityManager);
    entityManager->viewEntity   = pa_createPlayer(entityManager, (9 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                                                  (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    entityManager->playerEntity = entityManager->viewEntity;
    entityManager->playerEntity->hp = self->gameData.initialHp;

    if (entityManager->activeEnemies == 0)
    {
        for (uint16_t i = 0; i < self->gameData.maxActiveEnemies; i++)
        {
            pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
        }
    }
    else
    {
        // uint16_t randomAggroEnemy = esp_random() % self->gameData.maxActiveEnemies;

        int16_t skippedEnemyRespawnCount = 0;

        for (uint16_t i = 0; i < entityManager->activeEnemies; i++)
        {
            if (i >= DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH)
            {
                skippedEnemyRespawnCount++;
                continue;
            }

            uint8_t spawnTx     = defaultEnemySpawnLocations[i * DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH
                                                         + DEFAULT_ENEMY_SPAWN_LOCATION_TX_LOOKUP_OFFSET];
            uint8_t spawnTy     = defaultEnemySpawnLocations[i * DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH
                                                         + DEFAULT_ENEMY_SPAWN_LOCATION_TY_LOOKUP_OFFSET];
            uint8_t tileAtSpawn = pa_getTile(&(self->tilemap), spawnTx, spawnTy);

            switch (tileAtSpawn)
            {
                default:
                    break;
                case PA_TILE_BLOCK:
                    pa_setTile(&(self->tilemap), spawnTx, spawnTy, PA_TILE_EMPTY);
                    break;
                case PA_TILE_SPAWN_BLOCK_0:
                    skippedEnemyRespawnCount++;
                    continue;
                    break;
            }

            /*paEntity_t* newEnemy = */ createCrabdozer(&(self->entityManager),
                                                        (spawnTx << PA_TILE_SIZE_IN_POWERS_OF_2) + 8,
                                                        (spawnTy << PA_TILE_SIZE_IN_POWERS_OF_2) + 8);

            /*if(newEnemy != NULL && i == randomAggroEnemy){
                newEnemy->stateFlag = true;
                newEnemy->state = PA_EN_ST_STUN;
                newEnemy->stateTimer = 1;
            }*/
        }

        entityManager->activeEnemies -= skippedEnemyRespawnCount;
    }

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

void updateDead(pango_t* self, int64_t elapsedUs)
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
    drawPangoHud(&(self->font), &(self->gameData));

    if (self->gameData.countdown < 0)
    {
        drawText(&(self->font), c555, str_time_up, (TFT_WIDTH - textWidth(&(self->font), str_time_up)) / 2,
                 128);
    }
}

void updateGameOver(pango_t* self, int64_t elapsedUs)
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
            pangoSaveUnlockables(self);
        }

        changeStateNameEntry(self);
    }

    drawGameOver(&(self->font), &(self->gameData));
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
    self->gameData.gameState  = PA_ST_TITLE_SCREEN;
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

void updateLevelClear(pango_t* self, int64_t elapsedUs)
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

                pa_setDifficultyLevel(&(pango->gameData), getLevelIndex(self->gameData.world, self->gameData.level));
                pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
                pa_generateMaze(&(pango->tilemap));
                pa_placeEnemySpawns(&(pango->tilemap));

                self->entityManager.activeEnemies = 0;

                changeStateReadyScreen(self);
            }

            if (!self->gameData.debugMode)
            {
                pangoSaveUnlockables(self);
            }

            return;
        }
    }

    pa_updateEntities(&(self->entityManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawPangoHud(&(self->font), &(self->gameData));
    drawLevelClear(&(self->font), &(self->gameData));
    pa_updateLedsLevelClear(&(self->gameData));
}

void drawLevelClear(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);
    drawText(font, c000, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done) + 1) >> 1, 129);
    drawText(font, c553, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) >> 1, 128);
}

void changeStateGameClear(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateGameClear;
    pa_resetGameDataLeds(&(self->gameData));
    soundPlayBgm(&(self->soundManager.bgmSmooth), BZR_STEREO);
}

void updateGameClear(pango_t* self, int64_t elapsedUs)
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

    drawPangoHud(&(self->font), &(self->gameData));
    drawGameClear(&(self->font), &(self->gameData));
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
        drawText(font, c555, "Many more battle scenes", 8, 80);
        drawText(font, c555, "will soon be available", 8, 96);
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

void pangoInitializeHighScores(pango_t* self)
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
        pangoInitializeHighScores(self);
    }
}

void pangoSaveHighScores(pango_t* self)
{
    size_t size = sizeof(pangoHighScores_t);
    writeNvsBlob(KEY_SCORES, &(self->highScores), size);
}

void pangoInitializeUnlockables(pango_t* self)
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
        pangoInitializeHighScores(self);
    }
}

void pangoSaveUnlockables(pango_t* self)
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
        pangoChangeStateShowHighScores(self);
        return;
    }

    soundPlayBgm(&(self->soundManager.bgmNameEntry), BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update        = &updateNameEntry;
}

void updateNameEntry(pango_t* self, int64_t elapsedUs)
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
            pangoSaveHighScores(self);
            pangoChangeStateShowHighScores(self);
            soundPlaySfx(&(self->soundManager.sndPowerUp), BZR_STEREO);
        }
        else
        {
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
    }

    drawNameEntry(&(self->font), &(self->gameData), self->menuState);
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

void pangoChangeStateShowHighScores(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateShowHighScores;
}

void updateShowHighScores(pango_t* self, int64_t elapsedUs)
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

    drawShowHighScores(&(self->font), self->menuState);
    drawPangoHighScores(&(self->font), &(self->highScores), &(self->gameData));

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

void updatePause(pango_t* self, int64_t elapsedUs)
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
    drawPangoHud(&(self->font), &(self->gameData));
    drawPause(&(self->font));
}

void drawPause(font_t* font)
{
    drawText(font, c000, str_pause, (TFT_WIDTH - textWidth(font, str_pause) + 2) >> 1, 129);
    drawText(font, c553, str_pause, (TFT_WIDTH - textWidth(font, str_pause)) >> 1, 128);
}

uint16_t getLevelIndex(uint8_t world, uint8_t level)
{
    return (world - 1) * 4 + (level - 1);
}
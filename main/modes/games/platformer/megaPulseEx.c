/**
 * @file megaPulseEx.c
 * @author J.Vega (JVeg199X)
 * @brief Mega Pulse EX (JPV-MGSW-004-MG)
 * @date 2025-04-27
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "megaPulseEx.h"
#include "esp_random.h"
#include <esp_heap_caps.h>

#include "mega_pulse_ex_typedef.h"
#include "mgWsgManager.h"
#include "mgTilemap.h"
#include "mgGameData.h"
#include "mgEntityManager.h"
#include "mgLeveldef.h"

#include "hdw-led.h"
#include "palette.h"
#include "hdw-nvs.h"
#include "mgSoundManager.h"
#include <inttypes.h>
#include "mainMenu.h"
#include "fill.h"

//==============================================================================
// Constants
//==============================================================================
#define BIG_SCORE    4000000UL
#define BIGGER_SCORE 10000000UL
#define FAST_TIME    1500 // 25 minutes

const char platformerName[] = "Mega Pulse EX";

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};

static const paletteColor_t redColors[4]    = {c501, c540, c550, c540};
static const paletteColor_t yellowColors[4] = {c550, c331, c550, c555};
static const paletteColor_t greenColors[4]  = {c555, c051, c030, c051};
static const paletteColor_t cyanColors[4]   = {c055, c455, c055, c033};
static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};
static const paletteColor_t rgbColors[4]    = {c500, c050, c005, c050};

static const int16_t cheatCode[11]
    = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};

//==============================================================================
// Functions Prototypes
//==============================================================================

void platformerEnterMode(void);
void platformerExitMode(void);
void platformerMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)(platformer_t* self);
struct platformer_t
{
    font_t font;
    mgWsgManager_t wsgManager;

    mgTilemap_t tilemap;
    mgEntityManager_t entityManager;
    mgGameData_t gameData;

    mgSoundManager_t soundManager;

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

    menuMegaRenderer_t* menuRenderer;
    menu_t* menu;
};

//==============================================================================
// Function Prototypes
//==============================================================================
void drawPlatformerHud(font_t* font, mgGameData_t* gameData);
void drawPlatformerTitleScreen(font_t* font, mgGameData_t* gameData);
void changeStateReadyScreen(platformer_t* self);
void updateReadyScreen(platformer_t* self);
void drawReadyScreen(font_t* font, mgGameData_t* gameData);
void changeStateGame(platformer_t* self);
void detectGameStateChange(platformer_t* self);
void detectBgmChange(platformer_t* self);
void changeStateDead(platformer_t* self);
void updateDead(platformer_t* self);
void changeStateGameOver(platformer_t* self);
void updateGameOver(platformer_t* self);
void drawGameOver(font_t* font, mgGameData_t* gameData);
void changeStateTitleScreen(platformer_t* self);
void changeStateLevelClear(platformer_t* self);
void updateLevelClear(platformer_t* self);
void drawLevelClear(font_t* font, mgGameData_t* gameData);
void changeStateGameClear(platformer_t* self);
void updateGameClear(platformer_t* self);
void drawGameClear(font_t* font, mgGameData_t* gameData);
void initializePlatformerHighScores(platformer_t* self);
void loadPlatformerHighScores(platformer_t* self);
void savePlatformerHighScores(platformer_t* self);
void initializePlatformerUnlockables(platformer_t* self);
void loadPlatformerUnlockables(platformer_t* self);
void savePlatformerUnlockables(platformer_t* self);
void drawPlatformerHighScores(font_t* font, platformerHighScores_t* highScores, mgGameData_t* gameData);
uint8_t getHighScoreRank(platformerHighScores_t* highScores, uint32_t newScore);
void insertScoreIntoHighScores(platformerHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank);
void changeStateNameEntry(platformer_t* self);
void updateNameEntry(platformer_t* self);
void drawNameEntry(font_t* font, mgGameData_t* gameData, uint8_t currentInitial);
void changeStateShowHighScores(platformer_t* self);
void updateShowHighScores(platformer_t* self);
void drawShowHighScores(font_t* font, uint8_t menuState);
void changeStatePause(platformer_t* self);
void updatePause(platformer_t* self);
void drawPause(font_t* font);
uint16_t getLevelIndex(uint8_t world, uint8_t level);
void changeStateMainMenu(platformer_t* self);
void mgBuildMainMenu(platformer_t* self);
void mgUpdateMainMenu(platformer_t * self);
static void mg_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void changeStateLevelSelect(platformer_t* self);
void updateLevelSelect(platformer_t* self);
void drawLevelSelect(platformer_t* self);

//==============================================================================
// Variables
//==============================================================================

platformer_t* platformer = NULL;

swadgeMode_t modePlatformer = {.modeName                 = platformerName,
                               .wifiMode                 = NO_WIFI,
                               .overrideUsb              = false,
                               .usesAccelerometer        = false,
                               .usesThermometer          = false,
                               .fnEnterMode              = platformerEnterMode,
                               .fnExitMode               = platformerExitMode,
                               .fnMainLoop               = platformerMainLoop,
                               .fnAudioCallback          = NULL,
                               .fnBackgroundDrawCallback = mg_backgroundDrawCallback,
                               .fnEspNowRecvCb           = NULL,
                               .fnEspNowSendCb           = NULL};

#define NUM_LEVELS 16

led_t platLeds[CONFIG_NUM_LEDS];

static const char str_get_ready[]    = "LET'S GOOOOOO!";
static const char str_get_ready_2[]  = "WE'RE SO BACK!";
static const char str_time_up[]      = "-Time Up!-";
static const char str_game_over[]    = "IT'S SO OVER...";
static const char str_well_done[]    = "Well done!";
static const char str_congrats[]     = "Congratulations!";
static const char str_initials[]     = "Enter your initials!";
static const char str_hbd[]          = "Happy Birthday, Evelyn!";
static const char str_registrated[]  = "Your name registrated.";
static const char str_do_your_best[] = "Do your best!";
static const char str_pause[]        = "-Pause-";

static const char KEY_SCORES[]  = "mg_scores";
static const char KEY_UNLOCKS[] = "mg_unlocks";

static const char mgMenuNewGame[]       = "New Game";
static const char mgMenuPlaceholder[]   = "-------------";
static const char mgMenuContinue[]      = "Continue";
static const char mgMenuHighScores[]    = "High Scores";
static const char mgMenuResetScores[]   = "Reset Scores";
static const char mgMenuResetProgress[] = "Reset Progress";
static const char mgMenuExit[]          = "Exit";
static const char mgMenuSaveAndExit[]   = "Save & Exit";

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
    platformer = (platformer_t*)heap_caps_calloc(1, sizeof(platformer_t), MALLOC_CAP_8BIT);
    memset(platformer, 0, sizeof(platformer_t));

    platformer->menuState     = 0;
    platformer->menuSelection = 0;
    platformer->btnState      = 0;
    platformer->prevBtnState  = 0;

    loadPlatformerHighScores(platformer);
    loadPlatformerUnlockables(platformer);
    if (platformer->highScores.initials[0][0] == 'E' && platformer->highScores.initials[0][1] == 'F'
        && platformer->highScores.initials[0][2] == 'V')
    {
        platformer->easterEgg = true;
    }

    loadFont(MEGAMAX_JONES_FONT, &platformer->font, false);
    platformer->menuRenderer = initMenuMegaRenderer(NULL,NULL,NULL);

    mg_initializeWsgManager(&(platformer->wsgManager));

    mg_initializeTileMap(&(platformer->tilemap), &(platformer->wsgManager));

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[1].defaultWsgSetIndex);
    mg_loadMapFromFile(&(platformer->tilemap), leveldef[1].filename);

    mg_initializeSoundManager(&(platformer->soundManager));

    mg_initializeGameData(&(platformer->gameData), &(platformer->soundManager));
    mg_initializeEntityManager(&(platformer->entityManager), &(platformer->wsgManager), &(platformer->tilemap), &(platformer->gameData),
                               &(platformer->soundManager));

    platformer->tilemap.entityManager    = &(platformer->entityManager);
    platformer->tilemap.tileSpawnEnabled = true;

    setFrameRateUs(16666);

    changeStateMainMenu(platformer);
}

/**
 * @brief TODO
 *
 */
void platformerExitMode(void)
{
    freeFont(&platformer->font);
    mg_freeWsgManager(&(platformer->wsgManager));
    mg_freeTilemap(&(platformer->tilemap));
    mg_freeSoundManager(&(platformer->soundManager));
    mg_freeEntityManager(&(platformer->entityManager));
    heap_caps_free(platformer);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void mgMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == mgMenuNewGame)
        {
            uint16_t levelIndex = getLevelIndex(platformer->gameData.world, platformer->gameData.level);
            if ((levelIndex >= NUM_LEVELS)
                || (!platformer->gameData.debugMode && levelIndex > platformer->unlockables.maxLevelIndexUnlocked))
            {
                soundPlaySfx(&(platformer->soundManager.sndMenuDeny), BZR_STEREO);
                return;
            }

            /*if(self->menuSelection == 0){
                self->gameData.world = 1;
                self->gameData.level = 1;
            }*/

            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));
            changeStateLevelSelect(platformer);
        }
        else if (label == mgMenuContinue)
        {
            /*pango->gameData.level       = settingVal;
            pango->gameData.caravanMode = false;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData));
            pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), settingVal);
            pango->entityManager.activeEnemies = 0;
            pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
            pa_generateMaze(&(pango->tilemap));
            pa_placeEnemySpawns(&(pango->tilemap));

            changeStateReadyScreen(pango);
            deinitMenu(pango->menu);*/
        }
        else if (label == mgMenuHighScores)
        {
            changeStateShowHighScores(platformer);
            platformer->gameData.btnState = 0;
            deinitMenu(platformer->menu);
        }
        else if (label == mgMenuResetScores)
        {
            initializePlatformerHighScores(platformer);
            soundPlaySfx(&(platformer->soundManager.sndSquish), MIDI_SFX);
        }
        else if (label == mgMenuResetProgress)
        {
            initializePlatformerUnlockables(platformer);
            soundPlaySfx(&(platformer->soundManager.sndDie), MIDI_SFX);
        }
        else if (label == mgMenuSaveAndExit)
        {
            savePlatformerHighScores(platformer);
            savePlatformerUnlockables(platformer);
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == mgMenuExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == mgMenuPlaceholder)
        {
            soundPlaySfx(&(platformer->soundManager.sndMenuDeny), MIDI_SFX);
        }
    }
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
void platformerMainLoop(int64_t elapsedUs)
{
    // Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        platformer->btnState          = evt.state;
        platformer->gameData.btnState = evt.state;

        if (platformer->update == &mgUpdateMainMenu)
        {
            // Pass button events to the menu
            platformer->menu = menuButton(platformer->menu, evt);
        }
    }

    platformer->update(platformer);

    platformer->prevBtnState          = platformer->btnState;
    platformer->gameData.prevBtnState = platformer->prevBtnState;
}

void changeStateMainMenu(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.changeState = 0;
    self->update = &mgUpdateMainMenu;
    mgBuildMainMenu(self);
}

void mgBuildMainMenu(platformer_t* self)
{
    // Initialize the menu
    self->menu = initMenu(platformerName, mgMenuCb);
    addSingleItemToMenu(self->menu, mgMenuNewGame);

    addSingleItemToMenu(self->menu, mgMenuPlaceholder);

    addSingleItemToMenu(self->menu, mgMenuHighScores);

    if (self->gameData.debugMode)
    {
        addSingleItemToMenu(self->menu, mgMenuResetProgress);
        addSingleItemToMenu(self->menu, mgMenuResetScores);
        addSingleItemToMenu(self->menu, mgMenuSaveAndExit);
    }
    else
    {
        addSingleItemToMenu(self->menu, mgMenuExit);
    }
}

void mgUpdateMainMenu(platformer_t* self)
{
    // Draw the menu
    drawMenuMega(self->menu, self->menuRenderer, 16666);
}



void updateGame(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    mg_updateEntities(&(self->entityManager));

    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    detectGameStateChange(self);
    detectBgmChange(self);
    drawPlatformerHud(&(self->font), &(self->gameData));

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
    }

    updateComboTimer(&(self->gameData));
}

void drawPlatformerHud(font_t* font, mgGameData_t* gameData)
{
    //char coinStr[8];
    //snprintf(coinStr, sizeof(coinStr) - 1, "C:%02d", gameData->coins);

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%06" PRIu32, gameData->score);

    //char levelStr[15];
    //snprintf(levelStr, sizeof(levelStr) - 1, "Level %d-%d", gameData->world, gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%02d", gameData->lives);

    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr) - 1, "T:%03d", gameData->countdown);

    /*if (gameData->frameCount > 29)
    {
        drawText(font, c500, "1UP", 24, 2);
    }*/

    //drawText(font, c555, coinStr, 160, 16);
    drawText(font, c000, scoreStr, 34, 4);
    drawText(font, c555, scoreStr, 32, 2);
    //drawText(font, c555, levelStr, 152, 2);
    drawText(font, c000, timeStr, 214, 4);
    drawText(font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 212, 2);
    
    if(platformer->entityManager.playerEntity != NULL){
        drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOTTOM_ALPHA], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION);
        
        int8_t hp = platformer->entityManager.playerEntity->hp;

        for(uint8_t i = 0; i < 4; i++){
            if(hp > 6) {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_MIDDLE_6], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (i*16));
            } else {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_MIDDLE_0 + hp], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (i*16));
            }

            hp -= 6;

            if(hp < 0) {hp = 0;}
        }

        if(hp == 6) {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_TOP_6], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (4*16));
        } else {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_TOP_0 + hp], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (4*16));
        }
    }
    
    drawText(font, c000, livesStr, 6, 166);
    drawText(font, c555, livesStr, 4, 164);
    
    

    if (gameData->comboTimer == 0)
    {
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    drawText(font, (gameData->comboTimer < 60) ? c030 : greenColors[(platformer->gameData.frameCount >> 3) % 4],
             scoreStr, 32, 16);

    
}

void updateTitleScreen(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if ((self->gameData.btnState & cheatCode[self->menuSelection])
        && !(self->gameData.prevBtnState & cheatCode[self->menuSelection]))
    {
        self->menuSelection++;

        if (self->menuSelection > 8)
        {
            self->menuSelection      = 0;
            self->menuState          = 1;
            self->gameData.debugMode = true;
            soundPlaySfx(&(self->soundManager.sndLevelClearS), MIDI_SFX);
        }
    }

    if ((((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
         || ((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))))
    {
        self->gameData.btnState = 0;
        self->menuSelection    = 0;

        if (!self->gameData.debugMode)
        {
            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
        }

        changeStateMainMenu(self);
        return;
    }

    drawPlatformerTitleScreen(&(self->font), &(self->gameData));

    //leds
}

void drawPlatformerTitleScreen(font_t* font, mgGameData_t* gameData)
{
    mg_drawTileMap(&(platformer->tilemap));

    drawText(font, c555, "Mega Pulse EX", 40, 32);

    if (platformer->gameData.debugMode)
    {
        drawText(font, c555, "Debug Mode", 80, 48);
    }

    switch (platformer->menuState)
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

            if (platformer->gameData.debugMode || platformer->unlockables.maxLevelIndexUnlocked > 0)
            {
                char levelStr[24];
                snprintf(levelStr, sizeof(levelStr) - 1, "Level Select: %d-%d", gameData->world, gameData->level);
                drawText(font, c555, levelStr, 48, 144);
            }

            if (platformer->gameData.debugMode)
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

            drawText(font, c555, "->", 32, 128 + platformer->menuSelection * 16);

            break;
        }

        case 2:
        {
            if (platformer->unlockables.gameCleared)
            {
                drawText(font, redColors[(gameData->frameCount >> 3) % 4], "Beat the game!", 48, 80);
            }

            if (platformer->unlockables.oneCreditCleared)
            {
                drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "1 Credit Clear!", 48, 96);
            }

            if (platformer->unlockables.bigScore)
            {
                drawText(font, greenColors[(gameData->frameCount >> 3) % 4], "Got 4 million points!", 48, 112);
            }

            if (platformer->unlockables.biggerScore)
            {
                drawText(font, cyanColors[(gameData->frameCount >> 3) % 4], "Got 10 million points!", 48, 128);
            }

            if (platformer->unlockables.fastTime)
            {
                drawText(font, purpleColors[(gameData->frameCount >> 3) % 4], "Beat within 25 min!", 48, 144);
            }

            if (platformer->unlockables.gameCleared && platformer->unlockables.oneCreditCleared
                && platformer->unlockables.bigScore && platformer->unlockables.biggerScore
                && platformer->unlockables.fastTime)
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

void changeStateReadyScreen(platformer_t* self)
{
    self->gameData.frameCount = 0;

    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    soundPlayBgm(&(self->soundManager.bgmIntro), BZR_STEREO);

    mg_resetGameDataLeds(&(self->gameData));

    self->update = &updateReadyScreen;
}

void updateReadyScreen(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        soundStop(true);
        changeStateGame(self);
    }

    mg_drawTileMap(&(self->tilemap));
    drawReadyScreen(&(self->font), &(self->gameData));
}

void drawReadyScreen(font_t* font, mgGameData_t* gameData)
{
    //drawPlatformerHud(font, gameData);
    int16_t xOff = (TFT_WIDTH - textWidth(font, str_get_ready)) / 2;

    if(gameData->frameCount & 0b1111){
        drawText(font, c000, ((gameData->levelDeaths > 0) ? str_get_ready_2 : str_get_ready), xOff+2, 130);
        drawText(font, c555, ((gameData->levelDeaths > 0) ? str_get_ready_2 : str_get_ready), xOff, 128);
    }
    

    /*if (getLevelIndex(gameData->world, gameData->level) == 0)
    {
        drawText(font, c555, "A: Jump", xOff, 128 + (font->height + 3) * 3);
        drawText(font, c555, "B: Run / Fire", xOff, 128 + (font->height + 3) * 4);
    }*/
}

void changeStateGame(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.currentBgm = 0;
    mg_resetGameDataLeds(&(self->gameData));

    mg_deactivateAllEntities(&(self->entityManager), false);

    uint16_t levelIndex = getLevelIndex(self->gameData.world, self->gameData.level) + 1;

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[levelIndex].defaultWsgSetIndex);
    mg_loadMapFromFile(&(platformer->tilemap), leveldef[levelIndex].filename);
    self->gameData.countdown = leveldef[levelIndex].timeLimit;

    mgEntityManager_t* entityManager = &(self->entityManager);

    if(platformer->tilemap.defaultPlayerSpawn != NULL){
        entityManager->viewEntity
            = mg_createPlayer(entityManager, entityManager->tilemap->defaultPlayerSpawn->tx * 16 + entityManager->tilemap->defaultPlayerSpawn->xOffsetInPixels,
                            entityManager->tilemap->defaultPlayerSpawn->ty * 16 + entityManager->tilemap->defaultPlayerSpawn->yOffsetInPixels);
        entityManager->playerEntity     = entityManager->viewEntity;
        //entityManager->playerEntity->hp = self->gameData.initialHp;
        mg_viewFollowEntity(&(self->tilemap), entityManager->playerEntity);
    }

    mg_updateLedsHpMeter(&(self->entityManager), &(self->gameData));

    self->tilemap.executeTileSpawnAll = true;

    self->gameData.changeBgm = MG_BGM_MAIN;

    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);

    self->update = &updateGame;
}

static void mg_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c102 + ( (y >> 6) % 6) );
}


void detectGameStateChange(platformer_t* self)
{
    if (!self->gameData.changeState)
    {
        return;
    }

    switch (self->gameData.changeState)
    {
        case MG_ST_DEAD:
            changeStateDead(self);
            break;

        case MG_ST_READY_SCREEN:
            changeStateReadyScreen(self);
            break;

        case MG_ST_LEVEL_CLEAR:
            changeStateLevelClear(self);
            break;

        case MG_ST_PAUSE:
            changeStatePause(self);
            break;

        default:
            break;
    }

    self->gameData.changeState = 0;
}

void detectBgmChange(platformer_t* self)
{
    if (self->gameData.changeBgm == MG_BGM_NO_CHANGE)
    {
        return;
    }

    if(mg_setBgm(&(self->soundManager), self->gameData.changeBgm)){
        soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    }

    self->gameData.currentBgm = self->gameData.changeBgm;
    self->gameData.changeBgm  = MG_BGM_NO_CHANGE;
}

void changeStateDead(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    self->gameData.levelDeaths++;
    self->gameData.combo      = 0;
    self->gameData.comboTimer = 0;
    self->gameData.initialHp  = 1;

    soundStop(true);
    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    soundPlayBgm(&(self->soundManager.sndDie), BZR_STEREO);

    self->update = &updateDead;
}

void updateDead(platformer_t* self)
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

    mg_updateEntities(&(self->entityManager));
    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));

    if (self->gameData.countdown < 0)
    {
        drawText(&(self->font), c555, str_time_up, (TFT_WIDTH - textWidth(&(self->font), str_time_up)) / 2,
                 128);
    }
}

void updateGameOver(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

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
            savePlatformerUnlockables(self);
        }

        changeStateNameEntry(self);
    }

    drawGameOver(&(self->font), &(self->gameData));
    mg_updateLedsGameOver(&(self->gameData));
}

void changeStateGameOver(platformer_t* self)
{
    self->gameData.frameCount = 0;
    mg_resetGameDataLeds(&(self->gameData));
    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    soundPlayBgm(&(self->soundManager.bgmGameOver), BZR_STEREO);
    self->update = &updateGameOver;
}

void drawGameOver(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
}

void changeStateTitleScreen(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.gameState = MG_ST_TITLE_SCREEN;
    self->update              = &updateTitleScreen;
}

void changeStateLevelClear(platformer_t* self)
{
    self->gameData.frameCount         = 0;
    self->gameData.checkpoint         = 0;
    self->gameData.levelDeaths        = 0;
    self->gameData.initialHp          = self->entityManager.playerEntity->hp;
    self->gameData.extraLifeCollected = false;
    mg_resetGameDataLeds(&(self->gameData));
    self->update = &updateLevelClear;
}

void updateLevelClear(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if (self->gameData.frameCount > 60)
    {
        if (self->gameData.countdown > 0)
        {
            self->gameData.countdown--;

            if (self->gameData.countdown % 2)
            {
                globalMidiPlayerGet(MIDI_BGM)->loop = false;
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

                changeStateReadyScreen(self);
            }

            if (!self->gameData.debugMode)
            {
                savePlatformerUnlockables(self);
            }
        }
    }

    mg_updateEntities(&(self->entityManager));
    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));
    drawLevelClear(&(self->font), &(self->gameData));
    mg_updateLedsLevelClear(&(self->gameData));
}

void drawLevelClear(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) / 2, 128);
}

void changeStateGameClear(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateGameClear;
    mg_resetGameDataLeds(&(self->gameData));
    mg_setBgm(&(self->soundManager), MG_BGM_ATHLETIC);
    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
}

void updateGameClear(platformer_t* self)
{
    // Clear the display
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

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

    drawPlatformerHud(&(self->font), &(self->gameData));
    drawGameClear(&(self->font), &(self->gameData));
    mg_updateLedsGameClear(&(self->gameData));
}

void drawGameClear(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);

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

void initializePlatformerHighScores(platformer_t* self)
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

void loadPlatformerHighScores(platformer_t* self)
{
    size_t size = sizeof(platformerHighScores_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_SCORES, &(self->highScores), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerHighScores(self);
    }
}

void savePlatformerHighScores(platformer_t* self)
{
    size_t size = sizeof(platformerHighScores_t);
    writeNvsBlob(KEY_SCORES, &(self->highScores), size);
}

void initializePlatformerUnlockables(platformer_t* self)
{
    self->unlockables.maxLevelIndexUnlocked = 0;
    self->unlockables.gameCleared           = false;
    self->unlockables.oneCreditCleared      = false;
    self->unlockables.bigScore              = false;
    self->unlockables.fastTime              = false;
    self->unlockables.biggerScore           = false;
}

void loadPlatformerUnlockables(platformer_t* self)
{
    size_t size = sizeof(platformerUnlockables_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_UNLOCKS, &(self->unlockables), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerUnlockables(self);
    }
}

void savePlatformerUnlockables(platformer_t* self)
{
    size_t size = sizeof(platformerUnlockables_t);
    writeNvsBlob(KEY_UNLOCKS, &(self->unlockables), size);
}

void drawPlatformerHighScores(font_t* font, platformerHighScores_t* highScores, mgGameData_t* gameData)
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

uint8_t getHighScoreRank(platformerHighScores_t* highScores, uint32_t newScore)
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

void insertScoreIntoHighScores(platformerHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank)
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

void changeStateNameEntry(platformer_t* self)
{
    self->gameData.frameCount = 0;
    uint8_t rank              = getHighScoreRank(&(self->highScores), self->gameData.score);
    self->gameData.rank       = rank;
    self->menuState           = 0;

    mg_resetGameDataLeds(&(self->gameData));

    if (rank >= NUM_PLATFORMER_HIGH_SCORES || self->gameData.debugMode)
    {
        self->menuSelection = 0;
        self->gameData.rank = NUM_PLATFORMER_HIGH_SCORES;
        changeStateShowHighScores(self);
        return;
    }

    mg_setBgm(&(self->soundManager), MG_BGM_NAME_ENTRY);
    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update        = &updateNameEntry;
}

void updateNameEntry(platformer_t* self)
{
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

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
            savePlatformerHighScores(self);
            changeStateShowHighScores(self);
            soundPlaySfx(&(self->soundManager.sndPowerUp), BZR_STEREO);
        }
        else
        {
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
    }

    drawNameEntry(&(self->font), &(self->gameData), self->menuState);
    mg_updateLedsShowHighScores(&(self->gameData));
}

void drawNameEntry(font_t* font, mgGameData_t* gameData, uint8_t currentInitial)
{
    drawText(font, greenColors[(platformer->gameData.frameCount >> 3) % 4], str_initials,
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

void changeStateShowHighScores(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateShowHighScores;
}

void updateShowHighScores(platformer_t* self)
{
    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

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
    drawPlatformerHighScores(&(self->font), &(self->highScores), &(self->gameData));

    mg_updateLedsShowHighScores(&(self->gameData));
}

void drawShowHighScores(font_t* font, uint8_t menuState)
{
    if (platformer->easterEgg)
    {
        drawText(font, highScoreNewEntryColors[(platformer->gameData.frameCount >> 3) % 4], str_hbd,
                 (TFT_WIDTH - textWidth(font, str_hbd)) / 2, 32);
    }
    else if (menuState == 3)
    {
        drawText(font, redColors[(platformer->gameData.frameCount >> 3) % 4], str_registrated,
                 (TFT_WIDTH - textWidth(font, str_registrated)) / 2, 32);
    }
    else
    {
        drawText(font, c555, str_do_your_best, (TFT_WIDTH - textWidth(font, str_do_your_best)) / 2, 32);
    }
}

void changeStatePause(platformer_t* self)
{
    soundPause();
    soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
    self->update = &updatePause;
}

void updatePause(platformer_t* self)
{
    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        //soundResume();
        soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
        self->update              = &updateGame;
    }

    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));
    drawPause(&(self->font));
}

void drawPause(font_t* font)
{
    drawText(font, c555, str_pause, (TFT_WIDTH - textWidth(font, str_pause)) / 2, 128);
}

uint16_t getLevelIndex(uint8_t world, uint8_t level)
{
    return (world - 1) * 4 + (level - 1);
}

void changeStateLevelSelect(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->menuState     = 0;
    self->menuSelection = 0;

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[0].defaultWsgSetIndex);
    mg_loadMapFromFile(&(platformer->tilemap), leveldef[0].filename);
    self->tilemap.mapOffsetX = 12;
    
    self->update = &updateLevelSelect;
}

void updateLevelSelect(platformer_t* self)
{
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 59){
        self->gameData.frameCount = 0;
    }

    if(self->menuState < 2 && ((self->gameData.btnState & PB_RIGHT) && !(self->gameData.prevBtnState & PB_RIGHT))){
        self->menuState++;
    } else if(self->menuState > 0 && ((self->gameData.btnState & PB_LEFT) && !(self->gameData.prevBtnState & PB_LEFT))){
        self->menuState--;
    }

    if(self->menuSelection < 2 && ((self->gameData.btnState & PB_DOWN) && !(self->gameData.prevBtnState & PB_DOWN))){
        self->menuSelection++;
    } else if(self->menuSelection > 0 && ((self->gameData.btnState & PB_UP) && !(self->gameData.prevBtnState & PB_UP))){
        self->menuSelection--;
    }

    if( (self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
    {
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;

        mg_loadWsgSet(&(platformer->wsgManager), leveldef[self->gameData.level].defaultWsgSetIndex);
        mg_loadMapFromFile(&(platformer->tilemap), leveldef[self->gameData.level].filename);

        changeStateReadyScreen(self);
        return;
    }

    drawLevelSelect(self);
}

void drawLevelSelect(platformer_t* self)
{
    mg_drawTileMap(&(self->tilemap));

    drawText(&self->font, c555, "STAGE SELECT", 90, 8);

    drawRect(
        (64 + self->menuState * 64) - self->tilemap.mapOffsetX + ((self->gameData.frameCount >> 2) & 0b0111), 
        (48 + self->menuSelection * 64) - self->tilemap.mapOffsetY + ((self->gameData.frameCount >> 2) & 0b0111), 
        (64 + 48 + self->menuState * 64) - self->tilemap.mapOffsetX - ((self->gameData.frameCount >> 2) & 0b0111), 
        (48 + 48 + self->menuSelection * 64) - self->tilemap.mapOffsetY - ((self->gameData.frameCount >> 2) & 0b0111), 
        highScoreNewEntryColors[self->gameData.frameCount % 4]
    );
}
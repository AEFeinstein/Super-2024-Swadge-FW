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
#include "shapes.h"

//==============================================================================
// Constants
//==============================================================================
const char pangoName[] = "Pango";

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};

static const paletteColor_t redColors[4]    = {c501, c540, c550, c540};
static const paletteColor_t tsRedColors[4]  = {c535, c534, c514, c534};
static const paletteColor_t yellowColors[4] = {c550, c331, c550, c555};

static const paletteColor_t cyanColors[4]   = {c055, c455, c055, c033};
static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};

static const int16_t cheatCode[11]
    = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};

const char* characterSelectOptions[] = {"Pango", "Poe", "Pixel", "Piper"};
const int32_t characterSelectOptionValues[]
    = {PA_PLAYER_CHARACTER_PANGO, PA_PLAYER_CHARACTER_PO, PA_PLAYER_CHARACTER_PIXEL, PA_PLAYER_CHARACTER_GIRL};
#define NUM_CHARACTERS 4

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

    uint16_t btnState;
    uint16_t prevBtnState;

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
void drawGameClear(font_t* font, paWsgManager_t* wsgManager, paGameData_t* gameData, uint8_t page);
void pangoInitializeHighScores(pango_t* self);
void loadPangoHighScores(pango_t* self);
void pangoSaveHighScores(pango_t* self);
void pangoInitializeUnlockables(pango_t* self);
void loadPangoUnlockables(pango_t* self);
void pangoSaveUnlockables(pango_t* self);
void drawPangoHighScores(font_t* font, pangoHighScores_t* highScores, paGameData_t* gameData);
uint8_t getHighScoreRank(pangoHighScores_t* highScores, uint32_t newScore);
void insertScoreIntoHighScores(pangoHighScores_t* highScores, uint32_t newScore, char newInitials[],
                               uint8_t newCharacter, uint8_t rank);
void changeStateNameEntry(pango_t* self);
void updateNameEntry(pango_t* self, int64_t elapsedUs);
void drawNameEntry(font_t* font, paGameData_t* gameData, uint8_t currentInitial);
void pangoChangeStateShowHighScores(pango_t* self);
void updateShowHighScores(pango_t* self, int64_t elapsedUs);
void drawShowHighScores(font_t* font, uint8_t menuState);
void changeStatePause(pango_t* self);
void updatePause(pango_t* self, int64_t elapsedUs);
void drawPause(font_t* font);
void pangoChangeStateMainMenu(pango_t* self);
static void pa_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void drawPangoLogo(font_t* font, int16_t x, int16_t y);
uint16_t pa_getLevelClearBonus(int16_t elapsedTime);
void pa_setDifficultyLevel(paWsgManager_t* wsgManager, paGameData_t* gameData, uint16_t levelIndex);
void pa_advanceToNextLevelOrGameClear(pango_t* self);
void pa_updateLedsTitleScreen(pango_t* self);

void changeStateDemoControls(pango_t* self);
void updateDemoControls(pango_t* self, int64_t elapsedUs);
void drawDemoControls(uint16_t btnState);
void changeStateDemoScoring(pango_t* self);
void updateDemoScoring(pango_t* self, int64_t elapsedUs);
void changeStateAttractMode(pango_t* self);
void updateAttractMode(pango_t* self, int64_t elapsedUs);
void updateBtnStateAttractMode(pango_t* self);
uint16_t getAttractPlayerRandomDirection(void);

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
                          .fnBackgroundDrawCallback = pa_backgroundDrawCallback,
                          .fnEspNowRecvCb           = NULL,
                          .fnEspNowSendCb           = NULL};

// #define NUM_LEVELS 16

led_t paLeds[CONFIG_NUM_LEDS];

static const char str_ready[]        = "Ready?";
static const char str_set[]          = "Set...";
static const char str_pango[]        = "PANGO!";
static const char str_game_over[]    = "Game Over";
static const char str_well_done[]    = "Nice Clear!";
static const char str_initials[]     = "Enter your initials!";
static const char str_hbd[]          = "Happy Birthday, Evelyn!";
static const char str_registrated[]  = "Your name registrated.";
static const char str_do_your_best[] = "Hotdog Heroes";
static const char str_pause[]        = "-Pause-";

static const char pangoMenuNewGame[]       = "New Game";
static const char pangoMenuContinue[]      = "Continue - Rd";
static const char pangoMenuCaravan[]       = "Caravan Mode";
static const char pangoMenuCharacter[]     = "Character: ";
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
    pango = (pango_t*)heap_caps_calloc(1, sizeof(pango_t), MALLOC_CAP_8BIT);
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

    led_t ledColor = {.r = 0xf0, .g = 0xf0, .b = 0x00};
    recolorMenuManiaRenderer(pango->menuRenderer, //
                             c200, c555, c555,    // Title colors (bg, text, outline)
                             c000,                // Background
                             c110, c100,          // Rings
                             c003, c555,          // Rows
                             highScoreNewEntryColors, ARRAY_SIZE(highScoreNewEntryColors), ledColor);
    setManiaLedsOn(pango->menuRenderer, true);

    pa_initializeWsgManager(&(pango->wsgManager));

    pa_initializeTileMap(&(pango->tilemap), &(pango->wsgManager));

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
    freeFont(&pango->gameData.scoreFont);
    pa_freeWsgManager(&(pango->wsgManager));
    pa_freeTilemap(&(pango->tilemap));
    pa_freeSoundManager(&(pango->soundManager));
    pa_freeEntityManager(&(pango->entityManager));
    heap_caps_free(pango);
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
            pango->gameData.level              = 1;
            pango->entityManager.activeEnemies = 0;
            pango->gameData.caravanMode        = false;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData));
            pango->gameData.gameState = PA_ST_READY_SCREEN;
            changeStateDemoControls(pango);
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuContinue)
        {
            pango->gameData.level = settingVal;
            pango->gameData.caravanMode = false;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData));
            pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), settingVal);
            pango->entityManager.activeEnemies = 0;
            pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
            pa_generateMaze(&(pango->tilemap));
            pa_placeEnemySpawns(&(pango->tilemap));

            changeStateReadyScreen(pango);
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuCaravan)
        {
            pango->gameData.level              = 1;
            pango->entityManager.activeEnemies = 0;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData));
            pango->gameData.gameState = PA_ST_READY_SCREEN;
            pango->gameData.caravanMode        = true;
            pango->gameData.caravanTimer       = 300;
            changeStateDemoControls(pango);
            deinitMenu(pango->menu);
        }
        else if (label == pangoMenuCharacter)
        {
            pango->gameData.playerCharacter = settingVal;
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
            soundPlaySfx(&(pango->soundManager.sndSquish), MIDI_SFX);
        }
        else if (label == pangoMenuResetProgress)
        {
            pangoInitializeUnlockables(pango);
            soundPlaySfx(&(pango->soundManager.sndDie), MIDI_SFX);
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
        if (label == pangoMenuCharacter)
        {
            pango->gameData.playerCharacter = settingVal;
            pa_remapPlayerCharacter(&(pango->wsgManager), 16 * settingVal);
        }

        soundPlaySfx(&(pango->soundManager.sndMenuConfirm), BZR_STEREO);
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

    if (pango->gameData.debugMode)
    {
        DRAW_FPS_COUNTER(pango->gameData.scoreFont);
    }

    pango->prevBtnState          = pango->btnState;
    pango->gameData.prevBtnState = pango->gameData.btnState;
}

void pangoChangeStateMainMenu(pango_t* self)
{
    self->gameData.frameCount  = 0;
    self->gameData.changeState = 0;
    self->update               = &pangoUpdateMainMenu;
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
        pango->levelSelectMenuItem             = heap_caps_calloc(1, sizeof(menuItem_t), MALLOC_CAP_8BIT);
        pango->levelSelectMenuItem->label      = pangoMenuContinue;
        pango->levelSelectMenuItem->minSetting = 1;
        pango->levelSelectMenuItem->maxSetting
            = (pango->gameData.debugMode) ? MASTER_DIFFICULTY_TABLE_LENGTH : pango->unlockables.maxLevelIndexUnlocked;
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
    addSettingsOptionsItemToMenu(pango->menu, pangoMenuCharacter, characterSelectOptions, characterSelectOptionValues,
                                 NUM_CHARACTERS, &characterSettingBounds, pango->gameData.playerCharacter);
    addSingleItemToMenu(pango->menu, pangoMenuCaravan);
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
    drawWsgSimple(pango->wsgManager.sprites[PA_SP_PLAYER_ICON].wsg, 224, 136);
}

void updateGame(pango_t* self, int64_t elapsedUs)
{
    pa_updateEntities(&(self->entityManager));

    pa_animateTiles(&(self->wsgManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));

    if (((self->btnState & PB_START) && !(self->prevBtnState & PB_START)))
    {
        self->gameData.changeState = PA_ST_PAUSE;
    }

    detectGameStateChange(self);
    drawPangoHud(&(self->font), &(self->gameData));

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
        self->gameData.levelTime++;

        if(self->gameData.caravanMode){
            self->gameData.caravanTimer--;
            if(self->gameData.caravanTimer < 0){
                changeStateGameOver(self);
            }
        }

        if (self->gameData.remainingBlocks <= 0)
        {
            killPlayer(self->entityManager.playerEntity);
        }

        if (!self->gameData.firstBonusItemDispensed
            && (self->gameData.remainingBlocks < self->gameData.firstBonusItemDispenseThreshold))
        {
            pa_createBonusItem(&(self->entityManager),
                               ((1 + esp_random() % 15) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                               ((1 + esp_random() % 13) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
            self->gameData.firstBonusItemDispensed = true;
            self->gameData.leds[ledRemap[0]].b     = 0xFF;
        }

        if (!self->gameData.secondBonusItemDispensed
            && (self->gameData.remainingBlocks < self->gameData.secondBonusItemDispenseThreshold))
        {
            pa_createBonusItem(&(self->entityManager),
                               ((1 + esp_random() % 15) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                               ((1 + esp_random() % 13) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
            self->gameData.secondBonusItemDispensed = true;
            self->gameData.leds[ledRemap[0]].b      = 0xFF;
        }

        pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
    }

    pa_updateLedsInGame(&(self->gameData));
}

void drawPangoHud(font_t* font, paGameData_t* gameData)
{
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%7.6" PRIu32, gameData->score);

    char levelStr[15];
    snprintf(levelStr, sizeof(levelStr) - 1, "R:%02d", gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%d", gameData->lives);

    char timeStr[10];

    if (gameData->frameCount > 29)
    {
        drawText(font, c500, "1UP", 24, 2);
    }

    drawText(font, c553, scoreStr, 57, 2);
    
    if(!gameData->caravanMode)
    {
        snprintf(scoreStr, sizeof(scoreStr) - 1, "HI%7.6" PRIu32, pango->highScores.scores[0]);
        drawText(font, c553, scoreStr, 157, 2);
    } 
    else 
    {
         snprintf(timeStr, sizeof(timeStr) - 1, "Left:%03d", gameData->caravanTimer);
         drawText(font, (gameData->caravanTimer > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 157, 2);
    }

    for (uint8_t i = 0; i < gameData->lives; i++)
    {
        drawWsgSimple(pango->wsgManager.sprites[PA_SP_PLAYER_ICON].wsg, 32 + i * 16, 224);
    }

    drawText(font, c553, levelStr, 145, 226);

    snprintf(timeStr, sizeof(timeStr) - 1, "T:%02d", gameData->levelTime);
    drawText(font, c553, timeStr, 200, 226);

}

void updateTitleScreen(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if (self->gameData.frameCount > 600)
    {
        switch (pango->menuSelection)
        {
            case 0:
            default:
                changeStateDemoControls(self);
                break;
            case 1:
                changeStateDemoScoring(self);
                break;
            case 2:
                changeStateAttractMode(self);
                break;
        }

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
            soundPlaySfx(&(pango->soundManager.bgmLevelClear), MIDI_SFX);
        }
    }

    if ((((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
         || ((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))))
    {
        self->gameData.btnState = 0;
        pango->menuSelection    = 0;

        if (!pango->gameData.debugMode)
        {
            soundPlaySfx(&(pango->soundManager.sndSlide), BZR_STEREO);
        }

        pangoChangeStateMainMenu(self);
        return;
    }

    drawPangoTitleScreen(&(self->font), &(self->gameData));

    pa_updateLedsTitleScreen(self);
}

void pa_updateLedsTitleScreen(pango_t* self)
{
    if (((self->gameData.frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (!(esp_random() % 8))
            {
                switch (esp_random() % 3)
                {
                    case 0:
                    default:
                        paLeds[i].r = 255;
                        paLeds[i].g = 0;
                        paLeds[i].b = 0;
                        break;
                    case 1:
                        paLeds[i].r = 255;
                        paLeds[i].g = 255;
                        paLeds[i].b = 0;
                        break;
                    case 2:
                        paLeds[i].r = 0;
                        paLeds[i].g = 0;
                        paLeds[i].b = 255;
                        break;
                }
            }

            if (paLeds[i].r >= 16)
            {
                paLeds[i].r -= 16;
            }
            if (paLeds[i].g >= 16)
            {
                paLeds[i].g -= 16;
            }
            if (paLeds[i].b >= 16)
            {
                paLeds[i].b -= 16;
            }
        }
    }
    setLeds(paLeds, CONFIG_NUM_LEDS);
}

void drawPangoTitleScreen(font_t* font, paGameData_t* gameData)
{
    pa_drawTileMap(&(pango->tilemap));

    drawPangoLogo(font, 100, 32);

    if (pango->gameData.debugMode)
    {
        drawText(font, c555, "Debug Mode", 88, 96);
    }

    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_PIPER_WIN]), 128, 116);
    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_PO_PUSH_SIDE_1]), 144, 122);
    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_PIXEL_WIN]), 120, 126);
    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_PANGO_PUSH_SOUTH_2]), 136, 132);
    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_ENEMY_SIDE_2]), 64, 120);
    drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_ENEMY_DRILL_SIDE_1]), 52, 128);
    drawWsg(&(pango->wsgManager.wsgs[PA_WSG_ENEMY_DRILL_SIDE_2]), 200, 120, true, false, 0);
    drawWsg(&(pango->wsgManager.wsgs[PA_WSG_ENEMY_SIDE_1]), 212, 128, true, false, 0);

    if ((gameData->frameCount % 60) < 30)
    {
        drawText(font, c555, "- Press PAUSE button -", 20, 208);
    }
}

void drawPangoLogo(font_t* font, int16_t x, int16_t y)
{
    drawTriangleOutlined(x, y + 23, x + 63, y, x + 71, y + 47, c003, cyanColors[(pango->gameData.frameCount >> 2) % 4]);
    drawTriangleOutlined(x, y, x + 79, y + 15, x + 23, y + 47, c220,
                         yellowColors[(pango->gameData.frameCount >> 3) % 4]);
    drawText(font, c500, "PANGO", x + 12, y + 16);
    drawText(font, tsRedColors[(pango->gameData.frameCount >> 4) % 4], "PANGO", x + 11, y + 15);
}

void changeStateReadyScreen(pango_t* self)
{
    self->gameData.frameCount = 0;

    bool songChanged = pa_setBgm(&(self->soundManager), 1 + (self->gameData.level >> 2) % 4);

    switch (self->gameData.gameState)
    {
        case PA_ST_DEAD:
        default:
            soundPlayBgm(&(self->soundManager.currentBgm), MIDI_BGM);
            break;
        case PA_ST_LEVEL_CLEAR:
            if (songChanged)
            {
                soundPlayBgm(&(self->soundManager.currentBgm), MIDI_BGM);
            }
            break;
    }

    self->update = &updateReadyScreen;
}

void updateReadyScreen(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;
    if (self->gameData.frameCount > 159)
    {
        // soundStop(true);
        changeStateGame(self);
    }

    drawReadyScreen(&(self->font), &(self->gameData));
    pa_fadeLeds(&(self->gameData));
}

void drawReadyScreen(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "Extra Life at:%7.0" PRIu32, gameData->extraLifeScore);

    drawText(font, c555, str_ready, 80, 64);

    if (gameData->frameCount > 60)
    {
        drawText(font, c555, str_set, 112, 96);
    }

    if (gameData->frameCount > 120)
    {
        drawText(font, c555, str_pango, 144, 128);
    }

    drawText(font, c555, scoreStr, 24, 192);
}

void changeStateGame(pango_t* self)
{
    self->gameData.frameCount = 0;
    pa_resetGameDataLeds(&(self->gameData));

    pa_deactivateAllEntities(&(self->entityManager), false);

    paEntityManager_t* entityManager = &(self->entityManager);
    entityManager->viewEntity   = pa_createPlayer(entityManager, (9 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                                                  (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    entityManager->playerEntity = entityManager->viewEntity;

    if (entityManager->activeEnemies == 0)
    {
        for (uint16_t i = 0; i < self->gameData.maxActiveEnemies; i++)
        {
            pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
        }
    }
    else
    {
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
                    self->gameData.remainingBlocks--;
                    break;
                case PA_TILE_SPAWN_BLOCK_0:
                    skippedEnemyRespawnCount++;
                    continue;
                    break;
            }

            createCrabdozer(&(self->entityManager), (spawnTx << PA_TILE_SIZE_IN_POWERS_OF_2) + 8,
                            (spawnTy << PA_TILE_SIZE_IN_POWERS_OF_2) + 8);
        }

        entityManager->activeEnemies -= skippedEnemyRespawnCount;
    }

    self->tilemap.executeTileSpawnAll = true;
    self->gameData.gameState          = PA_ST_GAME;
    self->update                      = &updateGame;
}

static void pa_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c000);
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

void changeStateDead(pango_t* self)
{
    self->gameData.lives--;

    soundStop(true);
    soundPlaySfx(&(self->soundManager.sndDie), BZR_STEREO);

    self->gameData.gameState = PA_ST_DEAD;
    self->update             = &updateDead;
}

void updateDead(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if ((self->gameData.frameCount % 60) == 0)
    {
        // Keep counting time as a penalty
        self->gameData.levelTime++;

        if(self->gameData.caravanMode){
            self->gameData.caravanTimer--;
            if(self->gameData.caravanTimer < 0){
                changeStateGameOver(self);
                return;
            }
        }
    }

    if (self->gameData.frameCount > 179)
    {
        if (self->gameData.lives > 0)
        {
            if (self->gameData.remainingBlocks > 0)
            {
                changeStateReadyScreen(self);
            }
            else
            {
                pa_advanceToNextLevelOrGameClear(self);
                return;
            }
        }
        else
        {
            changeStateGameOver(self);
        }
    }

    pa_updateEntities(&(self->entityManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    pa_animateTiles(&(self->wsgManager));
    drawPangoHud(&(self->font), &(self->gameData));

    if (self->gameData.remainingBlocks <= 0)
    {
        drawText(&(self->font), c555, "Out of blocks!", 63, 128);
    }

    pa_fadeLeds(&(self->gameData));
}

void updateGameOver(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;
    
    if (!self->gameData.caravanMode){
        if (self->gameData.frameCount > 179)
        {
            // Handle unlockables
            if (!self->gameData.debugMode)
            {
                pangoSaveUnlockables(self);
            }

            changeStateNameEntry(self);
        }
        pa_updateLedsGameOver(&(self->gameData));
    } else {
        if (self->gameData.frameCount > 480)
        {
            if (self->gameData.btnState & PB_START && !(self->gameData.prevBtnState & PB_START))
            {
                changeStateTitleScreen(self);
                return;
            }
        }
        pa_updateLedsGameClear(&(self->gameData));
    }

    drawGameOver(&(self->font), &(self->gameData));
}

void changeStateGameOver(pango_t* self)
{
    self->gameData.frameCount = 0;
    pa_resetGameDataLeds(&(self->gameData));

    if(self->gameData.lives == 0)
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        player->loop         = false;
        soundPlayBgm(&(self->soundManager.bgmGameOver), BZR_STEREO);
    } 
    else 
    {
        pa_setBgm(&(self->soundManager), PA_BGM_HIGH_SCORE);
        soundPlayBgm(&(self->soundManager.currentBgm), MIDI_BGM);
    }
    self->update = &updateGameOver;
}

void drawGameOver(font_t* font, paGameData_t* gameData)
{
    if(!gameData->caravanMode)
    {
        drawPangoHud(font, gameData);
        drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
    } 
    else 
    {
        if (gameData->frameCount < 180){
            drawPangoHud(font, gameData);

            if(gameData->lives > 0) 
            {
                drawText(font, c555, "Time up!!", (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
            } 
            else 
            {
                drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
            }
        } else {
            char scoreStr[32];
            snprintf(scoreStr, sizeof(scoreStr) - 1, "%7.6" PRIu32, gameData->score);

            if(gameData->lives > 0) 
            {
                drawWsgSimpleScaled(pango->wsgManager.sprites[PA_SP_PLAYER_WIN].wsg, 120, 32, 2, 2);
            } 
            else 
            {
                drawWsgSimpleScaled(pango->wsgManager.sprites[PA_SP_PLAYER_HURT].wsg, 120, 32, 2, 2);
            }

            drawText(font, c555, "Caravan Score:", 64, 104);
            drawText(font, c555, scoreStr, 92, 116);

            drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "Thanks for playing!", 36, 144);

            if (gameData->frameCount > 480){
                drawText(font, c555, "Press Pause to exit.", 30, 200);
            }
        }
    }
}

void changeStateTitleScreen(pango_t* self)
{
    pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
    pa_generateMaze(&(pango->tilemap));

    self->gameData.frameCount = 0;

    self->gameData.gameState = PA_ST_TITLE_SCREEN;
    pa_remapBlockTile(&(pango->wsgManager), PA_WSG_BLOCK_TITLESCREEN);

    self->update = &updateTitleScreen;
}

void changeStateLevelClear(pango_t* self)
{
    self->gameData.frameCount = 0;
    pa_resetGameDataLeds(&(self->gameData));

    self->gameData.bonusScore = pa_getLevelClearBonus(self->gameData.levelTime);

    if (self->soundManager.currentBgmIndex != (1 + ((self->gameData.level + 1) >> 2) % 4))
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        player->loop         = false;
        soundPlayBgm(&(self->soundManager.bgmLevelClear), BZR_STEREO);
    }

    self->gameData.gameState = PA_ST_LEVEL_CLEAR;
    self->update             = &updateLevelClear;
}

void updateLevelClear(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    if (self->gameData.frameCount % 30 == 0)
    {
        self->gameData.leds[ledRemap[0]].g = (esp_random() % 2) ? 0xD0 : 0;
        self->gameData.leds[ledRemap[0]].b = (esp_random() % 2) ? 0xD0 : 0;
    }

    if (self->gameData.frameCount > 100)
    {
        if (self->gameData.bonusScore > 0)
        {
            pa_scorePoints(&(pango->gameData), 100);
            self->gameData.bonusScore -= 100;

            if (self->gameData.frameCount % 2)
            {
                soundPlaySfx(&(self->soundManager.sndTally), 0);
            }
        }
        else if (self->gameData.frameCount > 180 && (self->gameData.frameCount % 120 == 0))
        {
            // Hey look, it's a frame rule!
            pa_advanceToNextLevelOrGameClear(self);

            return;
        }
    }
    else if (self->gameData.frameCount < 30 || !(self->gameData.frameCount % 2))
    {
        pa_drawTileMap(&(self->tilemap));
    }

    pa_updateEntities(&(self->entityManager));
    pa_drawEntities(&(self->entityManager));
    drawPangoHud(&(self->font), &(self->gameData));
    drawLevelClear(&(self->font), &(self->gameData));
    pa_updateLedsInGame(&(self->gameData));
}

void drawLevelClear(font_t* font, paGameData_t* gameData)
{
    drawPangoHud(font, gameData);
    drawText(font, c000, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done) + 1) >> 1, 48);
    drawText(font, c553, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) >> 1, 48);

    if (gameData->frameCount > 30)
    {
        drawText(font, c555, "Time Bonus", 80, 80);
        drawText(font, (gameData->levelTime < 20) ? yellowColors[(pango->gameData.frameCount >> 3) % 4] : c555,
                 "00 - 19s ... 5000", 44, 100);
        drawText(font,
                 (gameData->levelTime >= 20 && gameData->levelTime < 30)
                     ? greenColors[(pango->gameData.frameCount >> 3) % 4]
                     : c555,
                 "20 - 29s ... 2000", 44, 112);
        drawText(font,
                 (gameData->levelTime >= 30 && gameData->levelTime < 40)
                     ? cyanColors[(pango->gameData.frameCount >> 3) % 4]
                     : c555,
                 "30 - 39s ... 1000", 44, 124);
        drawText(font,
                 (gameData->levelTime >= 40 && gameData->levelTime < 50)
                     ? purpleColors[(pango->gameData.frameCount >> 3) % 4]
                     : c555,
                 "40 - 49s ...  500", 44, 136);
        drawText(font,
                 (gameData->levelTime >= 50 && gameData->levelTime < 60)
                     ? redColors[(pango->gameData.frameCount >> 3) % 4]
                     : c555,
                 "50 - 59s ...  100", 44, 148);
        drawText(font, (gameData->levelTime > 59) ? c500 : c555, ">59s ....... None", 44, 160);
    }
}

void changeStateGameClear(pango_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.gameState  = PA_ST_GAME_CLEAR;
    self->menuState           = 0;
    self->update              = &updateGameClear;
    pa_resetGameDataLeds(&(self->gameData));
    soundStop(true);
}

void updateGameClear(pango_t* self, int64_t elapsedUs)
{
    self->gameData.frameCount++;

    switch (self->menuState)
    {
        case 0:
            if (self->gameData.frameCount == 360)
            {
                pa_setBgm(&(self->soundManager), PA_BGM_HIGH_SCORE);
                soundPlayBgm(&(self->soundManager.currentBgm), MIDI_BGM);
            }

            if (self->gameData.frameCount > 540)
            {
                if (self->gameData.lives > 0)
                {
                    if (self->gameData.frameCount % 60 == 0)
                    {
                        self->gameData.lives--;
                        self->gameData.score += 10000;
                        soundPlaySfx(&(self->soundManager.snd1up), MIDI_SFX);
                    }
                }
                else if (self->gameData.frameCount % 960 == 0)
                {
                    self->menuState           = 1;
                    self->gameData.frameCount = 0;
                }
            }

            pa_updateLedsGameOver(&(self->gameData));
            break;
        case 1:
        default:
            if (self->gameData.frameCount % 1320 == 0)
            {
                changeStateGameOver(self);
            }
            pa_updateLedsGameClear(&(self->gameData));
            break;
    }

    drawPangoHud(&(self->font), &(self->gameData));
    drawGameClear(&(self->font), &(self->wsgManager), &(self->gameData), self->menuState);
}

void drawGameClear(font_t* font, paWsgManager_t* wsgManager, paGameData_t* gameData, uint8_t page)
{
    drawPangoHud(font, gameData);

    switch (page)
    {
        case 0:
            drawText(font, c555, "You've reached the", 36, 48);
            drawText(font, redColors[(gameData->frameCount >> 3) % 4], "KILL SCREEN!", 72, 60);

            if (gameData->frameCount > 60 && gameData->frameCount < 300)
            {
                drawWsg(&wsgManager->wsgs[PA_WSG_PANGO_SOUTH], 132, 128, false, false, 0);
            }

            if (gameData->frameCount > 120)
            {
                drawText(font, c555, "PANGO.EXE will now", 36, 84);
                drawText(font, c555, "take your lives...", 36, 96);
            }

            if (gameData->frameCount > 240)
            {
                drawWsgSimpleScaled(&wsgManager->wsgs[PA_WSG_PANGO_ICON], 96, 112, 5, 5);
            }

            if (gameData->frameCount > 300)
            {
                drawText(font, (gameData->lives > 0) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555,
                         "for 10000pts each!", 36, 192);
            }

            break;

        case 1:
        default:

            if (gameData->frameCount > 60)
            {
                drawWsgSimpleScaled(&wsgManager->wsgs[PA_WSG_PANGO_WIN], 76, 44, 2, 2);
            }

            if (gameData->frameCount > 120)
            {
                drawWsgSimpleScaled(&wsgManager->wsgs[PA_WSG_PO_WIN], 108, 44, 2, 2);
            }

            if (gameData->frameCount > 180)
            {
                drawWsgSimpleScaled(&wsgManager->wsgs[PA_WSG_PIXEL_WIN], 140, 44, 2, 2);
            }

            if (gameData->frameCount > 240)
            {
                drawWsgSimpleScaled(&wsgManager->wsgs[PA_WSG_PIPER_WIN], 172, 44, 2, 2);
            }

            if (gameData->frameCount > 360)
            {
                drawText(font, c555, "We can't believe", 48, 108);
                drawText(font, c555, "you made it this far!", 24, 120);
            }

            if (gameData->frameCount > 540)
            {
                drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "Thanks for playing!", 36, 144);
            }

            if (gameData->frameCount > 680)
            {
                drawText(font, c555, "-Pango & Co.", 112, 194);
            }

            break;
    }
}

void pangoInitializeHighScores(pango_t* self)
{
    self->highScores.scores[0] = 100000;
    self->highScores.scores[1] = 80000;
    self->highScores.scores[2] = 40000;
    self->highScores.scores[3] = 20000;
    self->highScores.scores[4] = 10000;

    self->highScores.character[0] = 0;
    self->highScores.character[1] = 1;
    self->highScores.character[2] = 2;
    self->highScores.character[3] = 3;
    self->highScores.character[4] = 0;

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
        drawWsgSimple(&(pango->wsgManager.wsgs[PA_WSG_PANGO_ICON + 16 * highScores->character[i]]), 75, 126 + i * 16);
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

void insertScoreIntoHighScores(pangoHighScores_t* highScores, uint32_t newScore, char newInitials[],
                               uint8_t newCharacter, uint8_t rank)
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
        highScores->character[i]   = highScores->character[i - 1];
    }

    highScores->scores[rank]      = newScore;
    highScores->initials[rank][0] = newInitials[0];
    highScores->initials[rank][1] = newInitials[1];
    highScores->initials[rank][2] = newInitials[2];
    highScores->character[rank]   = newCharacter;
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

    pa_setBgm(&(self->soundManager), PA_BGM_HIGH_SCORE);
    soundPlayBgm(&(self->soundManager.currentBgm), MIDI_BGM);

    self->menuSelection = self->gameData.initials[0];
    self->update        = &updateNameEntry;
}

void updateNameEntry(pango_t* self, int64_t elapsedUs)
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
                                      self->gameData.playerCharacter, self->gameData.rank);
            pangoSaveHighScores(self);
            pangoChangeStateShowHighScores(self);
            soundPlaySfx(&(self->soundManager.snd1up), BZR_STEREO);
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
    self->gameData.frameCount++;

    if ((self->gameData.frameCount > 300)
        || (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
            || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))))
    {
        self->menuState = 0;
        // self->menuSelection = 0;
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
    globalMidiPlayerPauseAll();
    soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
    self->update = &updatePause;
}

void updatePause(pango_t* self, int64_t elapsedUs)
{
    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        globalMidiPlayerResumeAll();
        soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
        self->update = &updateGame;
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

uint16_t pa_getLevelClearBonus(int16_t elapsedTime)
{
    switch (elapsedTime)
    {
        case 0 ... 19:
            return 5000;
        case 20 ... 29:
            return 2000;
        case 30 ... 39:
            return 1000;
        case 40 ... 49:
            return 500;
        case 50 ... 59:
            return 100;
        default:
            return 0;
    }
}

void pa_setDifficultyLevel(paWsgManager_t* wsgManager, paGameData_t* gameData, uint16_t levelIndex)
{
    gameData->remainingEnemies
        = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH) + TOTAL_ENEMIES_LOOKUP_OFFSET];
    gameData->maxActiveEnemies
        = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH) + MAX_ACTIVE_ENEMIES_LOOKUP_OFFSET];
    gameData->enemyInitialSpeed
        = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH) + ENEMY_INITIAL_SPEED_LOOKUP_OFFSET];
    gameData->minAggroTime    = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH)
                                              + ENEMY_MINIMUM_AGGRESSIVE_TIME_LOOKUP_OFFSET];
    gameData->maxAggroTime    = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH)
                                              + ENEMY_MAXIMUM_AGGRESSIVE_TIME_LOOKUP_OFFSET];
    gameData->minAggroEnemies = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH)
                                                 + ENEMY_MINIMUM_AGGRESSIVE_COUNT_LOOKUP_OFFSET];
    gameData->maxAggroEnemies = masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH)
                                                 + ENEMY_MAXIMUM_AGGRESSIVE_COUNT_LOOKUP_OFFSET];

    pa_remapBlockTile(
        wsgManager,
        masterDifficulty[((levelIndex - 1) * MASTER_DIFFICULTY_TABLE_ROW_LENGTH) + BLOCK_WSG_LOOKUP_OFFSET]);
}

void pa_advanceToNextLevelOrGameClear(pango_t* self)
{
    self->gameData.levelTime                = 0;
    self->gameData.firstBonusItemDispensed  = false;
    self->gameData.secondBonusItemDispensed = false;

    if (self->gameData.level >= MASTER_DIFFICULTY_TABLE_LENGTH)
    {
        // Game Cleared!

        if (!self->gameData.debugMode)
        {
            // Determine achievements
            self->unlockables.gameCleared = true;

            if (!self->gameData.continuesUsed)
            {
                self->unlockables.oneCreditCleared = true;
            }
        }

        changeStateGameClear(self);
    }
    else
    {
        // Advance to the next level
        self->gameData.level++;

        // Unlock the next level
        if (self->gameData.level > self->unlockables.maxLevelIndexUnlocked)
        {
            self->unlockables.maxLevelIndexUnlocked = self->gameData.level;
        }

        pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), self->gameData.level);
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
}

void updateBtnStateAttractMode(pango_t* self)
{
    uint8_t t = pa_getTile(&(self->tilemap), self->entityManager.playerEntity->targetTileX,
                           self->entityManager.playerEntity->targetTileY);
    if (t == PA_TILE_BLOCK || t == PA_TILE_SPAWN_BLOCK_0)
    {
        self->gameData.btnState |= PB_A;
    }
    else
    {
        self->gameData.btnState &= ~PB_A;
    }

    if (!(self->gameData.frameCount % 16))
    {
        if ((t && t <= PA_TILE_WALL_7) || (esp_random() % 10) > 6)
        {
            self->gameData.btnState &= ~15;
            self->gameData.btnState |= getAttractPlayerRandomDirection();
        }
    }
}

uint16_t getAttractPlayerRandomDirection(void)
{
    switch (esp_random() % 8)
    {
        case 0:
        default:
            return PA_DIRECTION_NORTH;
        case 1:
            return PA_DIRECTION_SOUTH;
        case 2:
            return PA_DIRECTION_WEST;
        case 3:
            return PA_DIRECTION_NORTHWEST;
        case 4:
            return PA_DIRECTION_SOUTHWEST;
        case 5:
            return PA_DIRECTION_EAST;
        case 6:
            return PA_DIRECTION_NORTHEAST;
        case 7:
            return PA_DIRECTION_SOUTHEAST;
    }
}

void changeStateDemoControls(pango_t* self)
{
    pa_deactivateAllEntities(&(self->entityManager), false);
    pango->entityManager.activeEnemies = 0;
    pa_loadMapFromFile(&(pango->tilemap), "demo01.bin");
    pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), 2);
    self->entityManager.playerEntity
        = pa_createPlayer(&(self->entityManager), (0 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                          (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    self->menuState     = 0;
    self->frameTimer    = 0;
    self->menuSelection = 1;

    pango->update = &updateDemoControls;
}

void updateDemoControls(pango_t* self, int64_t elapsedUs)
{
    if (self->btnState & PB_START)
    {
        switch (self->gameData.gameState)
        {
            case PA_ST_TITLE_SCREEN:
            default:
                pangoChangeStateMainMenu(self);
                break;
            case PA_ST_READY_SCREEN:
                pa_initializeGameDataFromTitleScreen(&(self->gameData));
                self->entityManager.activeEnemies = 0;
                pa_setDifficultyLevel(&(self->wsgManager), &(self->gameData), self->gameData.level);
                pa_loadMapFromFile(&(self->tilemap), "preset.bin");
                pa_generateMaze(&(self->tilemap));
                pa_placeEnemySpawns(&(self->tilemap));

                changeStateReadyScreen(self);
                return;
        }
    }

    self->gameData.btnState = demoControlsScript[self->menuState * DEMO_CONTROLS_SCRIPT_TABLE_ROW_LENGTH
                                                 + DEMO_CONTROLS_BTN_STATE_LOOKUP_OFFSET];

    self->frameTimer++;
    if (self->frameTimer > demoControlsScript[self->menuState * DEMO_CONTROLS_SCRIPT_TABLE_ROW_LENGTH
                                              + DEMO_CONTROLS_DURATION_LOOKUP_OFFSET])
    {
        self->menuState++;

        if (self->menuState == 5 || self->menuState == 7 || self->menuState == 9)
        {
            createCrabdozer(&(self->entityManager), (0 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                            (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
        }

        if (self->menuState >= DEMO_CONTROLS_SCRIPT_TABLE_LENGTH)
        {
            switch (self->gameData.gameState)
            {
                case PA_ST_TITLE_SCREEN:
                default:
                    pangoChangeStateShowHighScores(self);
                    break;
                case PA_ST_READY_SCREEN:
                    pa_initializeGameDataFromTitleScreen(&(self->gameData));
                    self->entityManager.activeEnemies = 0;
                    pa_setDifficultyLevel(&(self->wsgManager), &(self->gameData), self->gameData.level);
                    pa_generateMaze(&(self->tilemap));
                    pa_loadMapFromFile(&(self->tilemap), "preset.bin");
                    pa_generateMaze(&(self->tilemap));
                    pa_placeEnemySpawns(&(self->tilemap));

                    changeStateReadyScreen(self);
                    return;
            }

            self->gameData.btnState = 0;
            return;
        }

        self->frameTimer = 0;
    }

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;

        if (self->menuState > 9)
        {
            pa_spawnEnemyFromSpawnBlock(&(self->entityManager));
        }
    }

    pa_updateEntities(&(self->entityManager));

    pa_animateTiles(&(self->wsgManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawDemoControls(self->gameData.btnState);
    drawText(&(self->font), c555, "Demonstration", 70, 16);

    if ((self->gameData.frameCount % 60) < 30)
    {
        switch (self->gameData.gameState)
        {
            case PA_ST_TITLE_SCREEN:
            default:
                drawText(&(self->font), c555, "- Press PAUSE button -", 20, 208);
                break;
            case PA_ST_READY_SCREEN:
                drawText(&(self->font), c555, "- Press PAUSE to skip -", 10, 208);
                break;
        }
    }

    if (self->gameData.gameState == PA_ST_TITLE_SCREEN)
    {
        pa_updateLedsTitleScreen(self);
    }
}

void drawDemoControls(uint16_t btnState)
{
    if (btnState & PB_LEFT)
    {
        drawCircleFilled(40, 168, 8, c555);
    }
    else
    {
        drawCircle(40, 168, 8, c555);
    }

    if (btnState & PB_UP)
    {
        drawCircleFilled(56, 152, 8, c555);
    }
    else
    {
        drawCircle(56, 152, 8, c555);
    }

    if (btnState & PB_RIGHT)
    {
        drawCircleFilled(72, 168, 8, c555);
    }
    else
    {
        drawCircle(72, 168, 8, c555);
    }

    if (btnState & PB_DOWN)
    {
        drawCircleFilled(56, 184, 8, c555);
    }
    else
    {
        drawCircle(56, 184, 8, c555);
    }

    if (btnState & PB_A)
    {
        drawCircleFilled(232, 164, 8, c555);
    }
    else
    {
        drawCircle(232, 164, 8, c555);
    }

    if (btnState & PB_B)
    {
        drawCircleFilled(208, 170, 8, c555);
    }
    else
    {
        drawCircle(208, 170, 8, c555);
    }
}

void changeStateDemoScoring(pango_t* self)
{
    pa_deactivateAllEntities(&(self->entityManager), false);
    pango->entityManager.activeEnemies = 0;
    pa_loadMapFromFile(&(pango->tilemap), "demo02.bin");
    pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), 6);
    self->entityManager.playerEntity
        = pa_createPlayer(&(self->entityManager), (0 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                          (3 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    self->menuState     = 0;
    self->frameTimer    = 0;
    self->menuSelection = 2;

    pango->update = &updateDemoScoring;
}

void updateDemoScoring(pango_t* self, int64_t elapsedUs)
{
    if (self->btnState & PB_START)
    {
        pangoChangeStateMainMenu(self);
    }

    self->gameData.btnState = demoScoringScript[self->menuState * DEMO_SCORING_SCRIPT_TABLE_ROW_LENGTH
                                                + DEMO_SCORING_BTN_STATE_LOOKUP_OFFSET];

    self->frameTimer++;
    if (self->frameTimer > demoScoringScript[self->menuState * DEMO_SCORING_SCRIPT_TABLE_ROW_LENGTH
                                             + DEMO_SCORING_DURATION_LOOKUP_OFFSET])
    {
        self->menuState++;

        if (self->menuState >= DEMO_SCORING_SCRIPT_TABLE_LENGTH)
        {
            pangoChangeStateShowHighScores(self);
            self->gameData.btnState = 0;
            return;
        }

        self->frameTimer = 0;
    }

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
    }

    pa_updateEntities(&(self->entityManager));

    pa_animateTiles(&(self->wsgManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));
    drawDemoControls(self->gameData.btnState);
    drawText(&(self->font), c555, "Demonstration", 70, 16);

    if ((self->gameData.frameCount % 60) < 30)
    {
        drawText(&(self->font), c555, "- Press PAUSE button -", 20, 208);
    }

    pa_updateLedsTitleScreen(self);
}

void changeStateAttractMode(pango_t* self)
{
    pa_initializeGameDataFromTitleScreen(&(self->gameData));
    pa_deactivateAllEntities(&(self->entityManager), false);
    pango->entityManager.activeEnemies = 0;
    pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), 2);
    pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
    pa_generateMaze(&(pango->tilemap));
    pa_placeEnemySpawns(&(pango->tilemap));
    self->entityManager.playerEntity
        = pa_createPlayer(&(self->entityManager), (9 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                          (7 << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
    self->menuState     = 0;
    self->frameTimer    = 0;
    self->menuSelection = 0;

    pango->update = &updateAttractMode;
}

void updateAttractMode(pango_t* self, int64_t elapsedUs)
{
    updateBtnStateAttractMode(self);

    pa_updateEntities(&(self->entityManager));

    pa_animateTiles(&(self->wsgManager));
    pa_drawTileMap(&(self->tilemap));
    pa_drawEntities(&(self->entityManager));

    if (((self->btnState & PB_START) && !(self->prevBtnState & PB_START)))
    {
        pangoChangeStateMainMenu(self);
        return;
    }

    drawPangoHud(&(self->font), &(self->gameData));

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
        self->gameData.levelTime++;

        if (self->gameData.remainingBlocks <= 0)
        {
            killPlayer(self->entityManager.playerEntity);
        }

        if (!self->gameData.firstBonusItemDispensed
            && (self->gameData.remainingBlocks < self->gameData.firstBonusItemDispenseThreshold))
        {
            pa_createBonusItem(&(self->entityManager),
                               ((1 + esp_random() % 15) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                               ((1 + esp_random() % 13) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
            self->gameData.firstBonusItemDispensed = true;
        }

        if (!self->gameData.secondBonusItemDispensed
            && (self->gameData.remainingBlocks < self->gameData.secondBonusItemDispenseThreshold))
        {
            pa_createBonusItem(&(self->entityManager),
                               ((1 + esp_random() % 15) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE,
                               ((1 + esp_random() % 13) << PA_TILE_SIZE_IN_POWERS_OF_2) + PA_HALF_TILE_SIZE);
            self->gameData.secondBonusItemDispensed = true;
        }

        pa_spawnEnemyFromSpawnBlock(&(self->entityManager));

        switch (self->gameData.changeState)
        {
            case PA_ST_NULL:
                break;
            case PA_ST_DEAD:
                if (!self->entityManager.playerEntity->active)
                {
                    pangoChangeStateShowHighScores(self);
                    self->gameData.playerCharacter = (self->gameData.playerCharacter + 1) % NUM_CHARACTERS;
                    pa_remapPlayerCharacter(&(self->wsgManager), 16 * self->gameData.playerCharacter);
                    self->gameData.changeState = 0;
                }
                break;
            default:
                pangoChangeStateShowHighScores(self);
                self->gameData.changeState = 0;
                break;
        }
    }

    drawText(&(self->font), c000, "Demonstration", 71, 17);
    drawText(&(self->font), c555, "Demonstration", 70, 16);

    if ((self->gameData.frameCount % 60) < 30)
    {
        drawText(&(self->font), c000, "- Press PAUSE button -", 21, 209);
        drawText(&(self->font), c555, "- Press PAUSE button -", 20, 208);
    }

    pa_updateLedsTitleScreen(self);
}
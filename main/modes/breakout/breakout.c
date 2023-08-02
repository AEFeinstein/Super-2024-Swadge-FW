/**
 * @file breakout.c
 * @author J.Vega (JVeg199X)
 * @brief It's breakout.
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
#include "entityManager.h"
#include "leveldef.h"

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

/**
 * @brief Enum of control schemes for a breakout game
 */
typedef enum
{
    BREAKOUT_BUTTON,
    BREAKOUT_TOUCH,
    BREAKOUT_TILT,
} breakoutControl_t;

/**
 * @brief Enum of CPU difficulties
 */
typedef enum
{
    BREAKOUT_EASY,
    BREAKOUT_MEDIUM,
    BREAKOUT_HARD,
} breakoutDifficulty_t;

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
    
    gameData_t gameData;
    tilemap_t tilemap;
    entityManager_t entityManager;

    breakoutScreen_t screen; ///< The screen being displayed

    breakoutControl_t control;       ///< The selected control scheme
    breakoutDifficulty_t difficulty; ///< The selected CPU difficulty

    uint16_t btnState;
    uint16_t prevBtnState;

    int32_t frameTimer;

    wsg_t paddleWsg; ///< A graphic for the paddle
    wsg_t ballWsg;   ///< A graphic for the ball

    song_t bgm;  ///< Background music
    song_t hit1; ///< Sound effect for one paddle's hit
    song_t hit2; ///< Sound effect for the other paddle's hit

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs

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

static void breakoutFadeLeds(int64_t elapsedUs);

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
// Level Definitons
//==============================================================================

#define NUM_LEVELS 2

static const leveldef_t leveldef[2] = {
    {.filename = "brkLevel1.bin",
     .timeLimit = 180},
    {.filename = "brkLvlChar1.bin",
     .timeLimit = 180}
     };

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char breakoutName[] = "Breakout";

static const char breakoutStartGame[] = "Start Game";

static const char breakoutCtrlButton[] = "Button Control";
static const char breakoutCtrlTouch[]  = "Touch Control";
static const char breakoutCtrlTilt[]   = "Tilt Control";

//static const char breakoutDiffEasy[]   = "Easy";
static const char breakoutDiffMedium[] = "Medium";
static const char breakoutDiffHard[]   = "Impossible";

//static const char breakoutPaused[] = "Paused";

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

    // Load graphics
    loadWsg("pball.wsg", &breakout->ballWsg, false);
    loadWsg("ppaddle.wsg", &breakout->paddleWsg, false);

    // Load SFX
    loadSong("block1.sng", &breakout->hit1, false);
    loadSong("block2.sng", &breakout->hit2, false);
    loadSong("gmcc.sng", &breakout->bgm, false);
    breakout->bgm.shouldLoop = true;

    // Initialize the menu
    breakout->menu = initMenu(breakoutName, breakoutMenuCb);
    breakout->mRenderer = initMenuLogbookRenderer(&breakout->logbook);

    initializeGameData(&(breakout->gameData));
    initializeTileMap(&(breakout->tilemap));
    initializeEntityManager(&(breakout->entityManager), &(breakout->tilemap), &(breakout->gameData));
    
    breakout->tilemap.entityManager = &(breakout->entityManager);
    breakout->tilemap.executeTileSpawnAll = true;

    loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);

    // These are the possible control schemes
    const char* controlSchemes[] = {
        breakoutCtrlButton,
        breakoutCtrlTouch,
        breakoutCtrlTilt,
    };

    addSingleItemToMenu(breakout->menu, breakoutStartGame);
    // Add each control scheme to the menu. Each control scheme has a submenu to select difficulty
    //for (uint8_t i = 0; i < ARRAY_SIZE(controlSchemes); i++)
    //{
        // Add a control scheme to the menu. This opens a submenu with difficulties
        //breakout->menu = startSubMenu(breakout->menu, controlSchemes[i]);
        // Add difficulties to the submenu
        //addSingleItemToMenu(breakout->menu, breakoutStartGame);
        //addSingleItemToMenu(breakout->menu, breakoutDiffMedium);
        //addSingleItemToMenu(breakout->menu, breakoutDiffHard);
        // End the submenu
        //breakout->menu = endSubMenu(breakout->menu);
    //}

    // Set the mode to menu mode
    breakout->screen = BREAKOUT_MENU;
    breakout->update = &breakoutUpdateTitleScreen;
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void breakoutExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(breakout->menu);
    deinitMenuLogbookRenderer(breakout->mRenderer);
    // Free the font
    freeFont(&breakout->logbook);
    freeFont(&breakout->ibm_vga8);
    // Free graphics
    freeWsg(&breakout->ballWsg);
    freeWsg(&breakout->paddleWsg);
    // Free the songs
    freeSong(&breakout->bgm);
    freeSong(&breakout->hit1);
    freeSong(&breakout->hit2);

    freeTilemap(&breakout->tilemap);
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
    // Only care about selected items, not scrolled-to items.
    // The same callback is called from the menu and submenu with no indication of which menu it was called from
    // Note that the label arg will be one of the strings used in startSubMenu() or addSingleItemToMenu()
    if (selected)
    {
        // Save what control scheme is selected (first-level menu)
        if (label == breakoutCtrlButton)
        {
            breakout->control = BREAKOUT_BUTTON;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == breakoutCtrlTouch)
        {
            breakout->control = BREAKOUT_TOUCH;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == breakoutCtrlTilt)
        {
            breakout->control = BREAKOUT_TILT;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == breakoutStartGame)
        {
            breakout->difficulty = BREAKOUT_EASY;
            breakout->screen = BREAKOUT_GAME;
            initializeGameDataFromTitleScreen(&(breakout->gameData));
            loadMapFromFile(&(breakout->tilemap), leveldef[0].filename);
            breakoutChangeStateReadyScreen(breakout);   
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == breakoutDiffMedium)
        {
            breakout->difficulty = BREAKOUT_MEDIUM;
            breakout->screen = BREAKOUT_GAME;
            breakoutChangeStateReadyScreen(breakout);
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == breakoutDiffHard)
        {
            breakout->difficulty = BREAKOUT_HARD;
            breakout->screen = BREAKOUT_GAME;
            breakoutChangeStateReadyScreen(breakout);
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

    breakoutDrawReadyScreen(&(self->logbook), &(self->ibm_vga8), &(self->gameData));
}

static void breakoutDrawReadyScreen(font_t *logbook, font_t *ibm_vga8, gameData_t *gameData){
    drawBreakoutHud(ibm_vga8, gameData);
    drawText(logbook, c555, breakoutReady, (TFT_WIDTH - textWidth(logbook, breakoutReady)) >> 1, 128);
}

static void breakoutChangeStateGame(breakout_t *self){
    self->gameData.frameCount = 0;
    deactivateAllEntities(&(self->entityManager), false);
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
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        self->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            // breakout->isPaused = !breakout->isPaused;
        }
    }

    // If the game is paused
    /*if (breakout->isPaused)
    {
        // Just draw and return
        breakoutDrawField();
        return;
    }

    // While the restart timer is active
    while (breakout->restartTimerUs > 0)
    {
        // Decrement the timer and draw the field, but don't run game logic
        breakout->restartTimerUs -= elapsedUs;
        breakoutDrawField();
        return;
    }*/

    // Do update each loop
    breakoutFadeLeds(elapsedUs);
    // breakoutControlPlayerPaddle();
    // breakoutControlCpuPaddle();
    // breakoutUpdatePhysics(elapsedUs);

    updateEntities(&(self->entityManager));
    breakoutDetectGameStateChange(self);

    // Draw the field
    drawEntities(&(self->entityManager));
    drawTileMap(&(self->tilemap));
    drawBreakoutHud(&(self->ibm_vga8), &(breakout->gameData));

    self->gameData.frameCount++;
    if(self->gameData.frameCount > 59){
        self->gameData.frameCount = 0;
    }

}

/**
 * @brief Fade the LEDs at a consistent rate over time
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void breakoutFadeLeds(int64_t elapsedUs)
{
    // This timer fades out LEDs. The fade is checked every 10ms
    // The pattern of incrementing a variable by elapsedUs, then decrementing it when it accumulates
    breakout->ledFadeTimer += elapsedUs;
    while (breakout->ledFadeTimer >= 10000)
    {
        breakout->ledFadeTimer -= 10000;

        // Fade left LED channels independently
        if (breakout->ledL.r)
        {
            breakout->ledL.r--;
        }
        if (breakout->ledL.g)
        {
            breakout->ledL.g--;
        }
        if (breakout->ledL.b)
        {
            breakout->ledL.b--;
        }

        // Fade right LEDs channels independently
        if (breakout->ledR.r)
        {
            breakout->ledR.r--;
        }
        if (breakout->ledR.g)
        {
            breakout->ledR.g--;
        }
        if (breakout->ledR.b)
        {
            breakout->ledR.b--;
        }
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
    self->gameData.levelDeaths++;
    self->gameData.combo = 0;
    self->gameData.comboTimer = 0;
    self->gameData.initialHp = 1;

    //buzzer_stop();
    //buzzer_play_bgm(&sndDie);

    self->update=&breakoutUpdateDead;
}


void breakoutUpdateDead(breakout_t *self, int64_t elapsedUs){
    self->gameData.frameCount++;
    if(self->gameData.frameCount > 179){
        if(self->gameData.lives > 0){
            breakoutChangeStateReadyScreen(self);
        } else {
            breakoutChangeStateGameOver(self);
        }
    }

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
    
    drawText(font, c555, livesStr, 48, 2);
    //drawText(font, c555, coinStr, 160, 16);
    drawText(font, c555, scoreStr, 80, 2);
    drawText(font, c555, levelStr, 184, 2);
    //drawText(d, font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 220, 16);

    if(gameData->comboTimer == 0){
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    //drawText(d, font, (gameData->comboTimer < 60) ? c030: greenColors[(platformer->gameData.frameCount >> 3) % 4], scoreStr, 8, 30);
}

void breakoutChangeStateLevelClear(breakout_t *self){
    self->gameData.frameCount = 0;
    resetGameDataLeds(&(self->gameData));
    self->update=&breakoutUpdateLevelClear;
}

void breakoutUpdateLevelClear(breakout_t *self, int64_t elapsedUs){ 
    self->gameData.frameCount++;

    if(self->gameData.frameCount > 60){
        if(self->gameData.countdown > 0){
            self->gameData.countdown--;
            
            if(self->gameData.countdown % 2){
                //buzzer_play_bgm(&sndTally);
            }

            uint16_t comboPoints = 50 * self->gameData.combo;

            self->gameData.score += comboPoints;
            self->gameData.comboScore = comboPoints;

            if(self->gameData.combo > 1){
                self->gameData.combo--;
            }
        } else if(self->gameData.frameCount % 60 == 0) {
            //Hey look, it's a frame rule!
            
            uint16_t levelIndex = breakoutGetLevelIndex(self->gameData.world, self->gameData.level);
            
            if(levelIndex >= NUM_LEVELS - 1){
                //Game Cleared!

                if(!self->gameData.debugMode){
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
            } else {
                 //Advance to the next level
                self->gameData.level++;
                if(self->gameData.level > 4){
                    self->gameData.world++;
                    self->gameData.level = 1;
                }

                //Unlock the next level
                levelIndex++;
                /*if(levelIndex > self->unlockables.maxLevelIndexUnlocked){
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }*/
                loadMapFromFile(&(breakout->tilemap), leveldef[levelIndex].filename);
                breakoutChangeStateReadyScreen(self);
            }

            /*if(!self->gameData.debugMode){
                savePlatformerUnlockables(self);
            }*/
        }
    }

    updateEntities(&(self->entityManager));
    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawLevelClear( &(self->logbook), &(self->gameData));
    updateLedsLevelClear(&(self->gameData));
}
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

    drawText(logbook, c555, "Thanks for playing!", 24, 48);
    
    if(gameData->frameCount > 300){
        drawText(logbook, c555, "See you", 8, 112);
        drawText(logbook, c555, "next mission!", 8, 160);
    }

}

void breakoutChangeStatePause(breakout_t *self){
    //buzzer_play_bgm(&sndPause);
    self->update=&breakoutUpdatePause;
}

void breakoutUpdatePause(breakout_t *self, int64_t elapsedUs){
    if((
        (self->gameData.btnState & PB_START)
        &&
        !(self->gameData.prevBtnState & PB_START)
    )){
        //buzzer_play_sfx(&sndPause);
        //self->gameData.changeBgm = self->gameData.currentBgm;
        //self->gameData.currentBgm = BGM_NULL;
        self->update=&breakoutGameLoop;
    }

    drawTileMap(&(self->tilemap));
    drawEntities(&(self->entityManager));
    drawBreakoutHud(&(self->ibm_vga8), &(self->gameData));
    breakoutDrawPause(&(self->logbook));

    self->prevBtnState = self->btnState;
    self->gameData.prevBtnState = self->prevBtnState;
}

void breakoutDrawPause(font_t *font){
    drawText(font, c555, breakoutPause, (TFT_WIDTH - textWidth(font, breakoutPause)) / 2, 128);
}

uint16_t breakoutGetLevelIndex(uint8_t world, uint8_t level){
    return (world-1) * 4 + (level-1);
}
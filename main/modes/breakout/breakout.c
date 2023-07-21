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

typedef struct
{
    menu_t* menu; ///< The menu structure
    menuLogbookRenderer_t* mRenderer; ///< The menu renderer
    font_t logbook;   ///< The font used in the menu and game
    
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
} breakout_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void breakoutMainLoop(int64_t elapsedUs);
static void breakoutEnterMode(void);
static void breakoutExitMode(void);

static void breakoutMenuCb(const char* label, bool selected, uint32_t settingVal);
static void breakoutGameLoop(int64_t elapsedUs);

static void breakoutResetGame(bool isInit, uint8_t whoWon);
static void breakoutFadeLeds(int64_t elapsedUs);

static void breakoutBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char breakoutName[] = "Breakout";

static const char breakoutCtrlButton[] = "Button Control";
static const char breakoutCtrlTouch[]  = "Touch Control";
static const char breakoutCtrlTilt[]   = "Tilt Control";

static const char breakoutDiffEasy[]   = "Easy";
static const char breakoutDiffMedium[] = "Medium";
static const char breakoutDiffHard[]   = "Impossible";

//static const char breakoutPaused[] = "Paused";

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

    loadMapFromFile(&(breakout->tilemap), "level1.bin");

    // These are the possible control schemes
    const char* controlSchemes[] = {
        breakoutCtrlButton,
        breakoutCtrlTouch,
        breakoutCtrlTilt,
    };

    // Add each control scheme to the menu. Each control scheme has a submenu to select difficulty
    for (uint8_t i = 0; i < ARRAY_SIZE(controlSchemes); i++)
    {
        // Add a control scheme to the menu. This opens a submenu with difficulties
        breakout->menu = startSubMenu(breakout->menu, controlSchemes[i]);
        // Add difficulties to the submenu
        addSingleItemToMenu(breakout->menu, breakoutDiffEasy);
        addSingleItemToMenu(breakout->menu, breakoutDiffMedium);
        addSingleItemToMenu(breakout->menu, breakoutDiffHard);
        // End the submenu
        breakout->menu = endSubMenu(breakout->menu);
    }

    // Set the mode to menu mode
    breakout->screen = BREAKOUT_MENU;
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
        else if (label == breakoutDiffEasy)
        {
            breakout->difficulty = BREAKOUT_EASY;
            breakoutResetGame(true, 0);
            breakout->screen = BREAKOUT_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == breakoutDiffMedium)
        {
            breakout->difficulty = BREAKOUT_MEDIUM;
            breakoutResetGame(true, 0);
            breakout->screen = BREAKOUT_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == breakoutDiffHard)
        {
            breakout->difficulty = BREAKOUT_HARD;
            breakoutResetGame(true, 0);
            breakout->screen = BREAKOUT_GAME;
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
    // Pick what runs and draws depending on the screen being displayed
    switch (breakout->screen)
    {
        case BREAKOUT_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                breakout->menu = menuButton(breakout->menu, evt);
            }

            // Draw the menu
            drawMenuLogbook(breakout->menu, breakout->mRenderer, elapsedUs);
            break;
        }
        case BREAKOUT_GAME:
        {
            // Run the main game loop. This will also process button events
            breakoutGameLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void breakoutGameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        breakout->btnState = evt.state;

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

    updateEntities(&(breakout->entityManager));

    // Draw the field
    drawEntities(&(breakout->entityManager));
    drawTileMap(&(breakout->tilemap));
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

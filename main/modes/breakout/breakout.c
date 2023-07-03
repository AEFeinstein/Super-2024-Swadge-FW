/**
 * @file breakout.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief An example Pong game
 * @date 2023-03-25
 *
 * TODO networking
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
    font_t ibm;   ///< The font used in the menu and game
    
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
static void breakoutEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                 int8_t rssi);
static void breakoutEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void breakoutMenuCb(const char* label, bool selected, uint32_t settingVal);
static void breakoutGameLoop(int64_t elapsedUs);

static void breakoutResetGame(bool isInit, uint8_t whoWon);
static void breakoutFadeLeds(int64_t elapsedUs);
static void breakoutControlPlayerPaddle(void);
static void breakoutControlCpuPaddle(void);
static void breakoutUpdatePhysics(int64_t elapsedUs);
static void breakoutIncreaseBallVelocity(int16_t magnitude);

static void breakoutBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void breakoutDrawField(void);

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
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .fnEnterMode              = breakoutEnterMode,
    .fnExitMode               = breakoutExitMode,
    .fnMainLoop               = breakoutMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = breakoutBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL, //breakoutEspNowRecvCb,
    .fnEspNowSendCb           = NULL, //breakoutEspNowSendCb,
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
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    breakout = calloc(1, sizeof(breakout_t));

    // Below, various assets are loaded from the SPIFFS file system to RAM. How did they get there?
    // The source assets are found in the /assets/breakout/ directory. Each asset is processed and packed into the
    // SPIFFS file system at compile time The following transformations are made:
    // * pball.png   -> pball.wsg
    // * ppaddle.png -> ppaddle.wsg
    // * block1.mid  -> block1.sng
    // * block2.mid  -> block2.sng
    // * gmcc.mid    -> gmcc.sng
    //
    // In addition, a common font is found in /assets/fonts/ and is transformed like so:
    // * ibm_vga8.font.png -> ibm_vga8.font
    //
    // If you'd like to learn more about how assets are processed and packed, see
    // /tools/spiffs_file_preprocessor/README.md
    //
    // If you'd like to learn more about how assets are loaded, see
    // /components/hdw-spiffs/include/hdw-spiffs.h

    // Load a font
    loadFont("ibm_vga8.font", &breakout->ibm, false);

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
    // Free the font
    freeFont(&breakout->ibm);
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
            drawMenu(breakout->menu, &breakout->ibm);
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
    breakoutDrawField();
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
 * @brief Move the player's paddle according to the chosen control scheme
 */
/*static void breakoutControlPlayerPaddle(void)
{
    // Move the paddle depending on the chosen control scheme
    switch (breakout->control)
    {
        default:
        case BREAKOUT_BUTTON:
        {
            // Move the player paddle if a button is currently down
            if (breakout->btnState & PB_UP)
            {
                breakout->paddleL.y = MAX(breakout->paddleL.y - (5 << DECIMAL_BITS), 0);
            }
            else if (breakout->btnState & PB_DOWN)
            {
                breakout->paddleL.y = MIN(breakout->paddleL.y + (5 << DECIMAL_BITS), FIELD_HEIGHT -
breakout->paddleL.height);
            }
            break;
        }
        case BREAKOUT_TOUCH:
        {
            // Check if the touch area is touched
            int32_t centerVal, intensityVal;
            if (getTouchCentroid(&centerVal, &intensityVal))
            {
                // If there is a touch, move the paddle to that location of the touch
                breakout->paddleL.y = (centerVal * (FIELD_HEIGHT - breakout->paddleL.height)) / 1024;
            }
            break;
        }
        case BREAKOUT_TILT:
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelGetAccelVec(&a_x, &a_y, &a_z))
            {
                // Move the paddle to the current tilt location
                breakout->paddleL.y = CLAMP(((a_x + 100) * (FIELD_HEIGHT - breakout->paddleL.height)) / 350, 0,
                                        (FIELD_HEIGHT - breakout->paddleL.height));
            }
            break;
        }
    }
}*/

/**
 * @brief Move the CPU's paddle according to the chosen difficulty
 */
/*static void breakoutControlCpuPaddle(void)
{
    // Move the computer paddle
    switch (breakout->difficulty)
    {
        default:
        case BREAKOUT_EASY:
        {
            // Blindly move up and down
            if (breakout->paddleRMovingUp)
            {
                breakout->paddleR.y = MAX(breakout->paddleR.y - (4 << DECIMAL_BITS), 0);
                // If the top boundary was hit
                if (0 == breakout->paddleR.y)
                {
                    // Start moving down
                    breakout->paddleRMovingUp = false;
                }
            }
            else
            {
                breakout->paddleR.y = MIN(breakout->paddleR.y + (4 << DECIMAL_BITS), FIELD_HEIGHT -
breakout->paddleR.height);
                // If the bottom boundary was hit
                if ((FIELD_HEIGHT - breakout->paddleR.height) == breakout->paddleR.y)
                {
                    // Start moving up
                    breakout->paddleRMovingUp = true;
                }
            }
            break;
        }
        case BREAKOUT_MEDIUM:
        {
            // Move towards the ball, slowly
            if (breakout->paddleR.y + (breakout->paddleR.height / 2) < breakout->ball.y)
            {
                breakout->paddleR.y = MIN(breakout->paddleR.y + (2 << DECIMAL_BITS), FIELD_HEIGHT -
breakout->paddleR.height);
            }
            else
            {
                breakout->paddleR.y = MAX(breakout->paddleR.y - (2 << DECIMAL_BITS), 0);
            }
            break;
        }
        case BREAKOUT_HARD:
        {
            // Never miss
            breakout->paddleR.y = CLAMP(breakout->ball.y - breakout->paddleR.height / 2, 0, FIELD_HEIGHT -
breakout->paddleR.height); break;
        }
    }
}*/

/**
 * @brief Update the breakout physics including ball position and collisions
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
/*static void breakoutUpdatePhysics(int64_t elapsedUs)
{
    // Update the ball's position
    breakout->ball.x += (breakout->ballVel.x * elapsedUs) / 100000;
    breakout->ball.y += (breakout->ballVel.y * elapsedUs) / 100000;

    // Check for goals
    if (breakout->ball.x < 0 || breakout->ball.x > FIELD_WIDTH)
    {
        // Reset the game. This keeps score.
        breakoutResetGame(false, breakout->ball.x < 0 ? 1 : 0);
    }
    else
    {
        // Checks for top and bottom wall collisions
        if (((breakout->ball.y - breakout->ball.radius) < 0 && breakout->ballVel.y < 0)
            || ((breakout->ball.y + breakout->ball.radius) > FIELD_HEIGHT && breakout->ballVel.y > 0))
        {
            // Reverse direction
            breakout->ballVel.y = -breakout->ballVel.y;
        }

        // If a goal was not scored,c heck for left paddle collision
        if ((breakout->ballVel.x < 0) && circleRectIntersection(breakout->ball, breakout->paddleL))
        {
            // Reverse direction
            breakout->ballVel.x = -breakout->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = breakout->ball.y - (breakout->paddleL.y + breakout->paddleL.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            breakout->ballVel = rotateVec2d(breakout->ballVel, (45 * diff) / (breakout->paddleL.height / 2));

            // Increase velocity
            breakoutIncreaseBallVelocity(1 << DECIMAL_BITS);

            // Play SFX
            bzrPlaySfx(&breakout->hit1);

            // Set an LED
            breakout->ledL.r = 0xFF;
            breakout->ledL.g = 0x80;
            breakout->ledL.b = 0x40;
        }
        // Check for right paddle collision
        else if ((breakout->ballVel.x > 0) && circleRectIntersection(breakout->ball, breakout->paddleR))
        {
            // Reverse direction
            breakout->ballVel.x = -breakout->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = breakout->ball.y - (breakout->paddleR.y + breakout->paddleR.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            breakout->ballVel = rotateVec2d(breakout->ballVel, -(45 * diff) / (breakout->paddleR.height / 2));

            // Increase velocity
            breakoutIncreaseBallVelocity(1 << DECIMAL_BITS);

            // Play SFX
            bzrPlaySfx(&breakout->hit2);

            // Set an LED
            breakout->ledR.r = 0x40;
            breakout->ledR.g = 0x80;
            breakout->ledR.b = 0xFF;
        }
    }
}*/

/**
 * @brief Reset the breakout game variables
 *
 * @param isInit True if this is the first time this is being called, false if it is reset after a player scores
 * @param whoWon The player who scored a point, either 0 or 1
 */
static void breakoutResetGame(bool isInit, uint8_t whoWon)
{
    // Set different variables based on initialization
    /*+if (isInit)
    {
        // Set up the left paddle
        breakout->paddleL.x      = 0;
        breakout->paddleL.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        breakout->paddleL.width  = PADDLE_WIDTH;
        breakout->paddleL.height = PADDLE_HEIGHT;

        // Set up the right paddle
        breakout->paddleR.x      = FIELD_WIDTH - PADDLE_WIDTH;
        breakout->paddleR.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        breakout->paddleR.width  = PADDLE_WIDTH;
        breakout->paddleR.height = PADDLE_HEIGHT;

        // Start playing music
        bzrPlayBgm(&breakout->bgm);
    }
    else
    {
        // Tally the score
        breakout->score[whoWon]++;
    }

    // Set the restart timer
    breakout->restartTimerUs = 2000000;

    // Set the ball variables
    breakout->ball.x      = (FIELD_WIDTH) / 2;
    breakout->ball.y      = (FIELD_HEIGHT) / 2;
    breakout->ball.radius = BALL_RADIUS;

    // Give the ball initial velocity to the top right
    breakout->ballVel.x = (4 << DECIMAL_BITS);
    breakout->ballVel.y = -(4 << DECIMAL_BITS);

    // Rotate up to 90 degrees, for some randomness
    breakout->ballVel = rotateVec2d(breakout->ballVel, esp_random() % 90);

    // Determine the direction of the serve based on who scored last
    if (whoWon)
    {
        breakout->ballVel.x = -breakout->ballVel.x;
    }*/
}

/**
 * @brief Increase the ball's velocity by a fixed amount
 *
 * @param magnitude The magnitude velocity to add to the ball
 */
/*static void breakoutIncreaseBallVelocity(int16_t magnitude)
{
    if (sqMagVec2d(breakout->ballVel) < (SPEED_LIMIT * SPEED_LIMIT))
    {
        // Create a vector in the same direction as breakout->ballVel with the given magnitude
        int32_t denom  = ABS(breakout->ballVel.x) + ABS(breakout->ballVel.y);
        vec_t velBoost = {
            .x = (magnitude * breakout->ballVel.x) / denom,
            .y = (magnitude * breakout->ballVel.y) / denom,
        };

        // Add the vectors together
        breakout->ballVel = addVec2d(breakout->ballVel, velBoost);
    }
}*/

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

/**
 * @brief Draw the Pong field to the TFT
 */
static void breakoutDrawField(void)
{
    /*// Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Copy the LED colors for left and right to the whole array
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
    {
        leds[i]                         = breakout->ledL;
        leds[i + (CONFIG_NUM_LEDS / 2)] = breakout->ledR;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);

    // No need to clear the display before drawing because it's redrawn by breakoutBackgroundDrawCallback() each time

#ifdef DRAW_SHAPES
    // This will draw the game with geometric shapes, not sprites

    // Bitshift the ball's location and radius from math coordinates to screen coordinates, then draw it
    drawCircleFilled((breakout->ball.x) >> DECIMAL_BITS, (breakout->ball.y) >> DECIMAL_BITS,
                     (breakout->ball.radius) >> DECIMAL_BITS, c555);

    // Bitshift the left paddle's location and radius from math coordinates to screen coordinates, then draw it
    drawRect((breakout->paddleL.x >> DECIMAL_BITS), (breakout->paddleL.y >> DECIMAL_BITS),
             ((breakout->paddleL.x + breakout->paddleL.width) >> DECIMAL_BITS),
             ((breakout->paddleL.y + breakout->paddleL.height) >> DECIMAL_BITS), c333);
    // Bitshift the right paddle's location and radius from math coordinates to screen coordinates, then draw it

    drawRect((breakout->paddleR.x >> DECIMAL_BITS), (breakout->paddleR.y >> DECIMAL_BITS),
             ((breakout->paddleR.x + breakout->paddleR.width) >> DECIMAL_BITS),
             ((breakout->paddleR.y + breakout->paddleR.height) >> DECIMAL_BITS), c333);
#else
    // This will draw the game with sprites, not geometric shapes

    // Draw the ball
    drawWsgSimple(&breakout->ballWsg, (breakout->ball.x - breakout->ball.radius) >> DECIMAL_BITS,
                  (breakout->ball.y - breakout->ball.radius) >> DECIMAL_BITS);
    // Draw one paddle
    drawWsg(&breakout->paddleWsg, breakout->paddleL.x >> DECIMAL_BITS, breakout->paddleL.y >> DECIMAL_BITS, false,
            false, 0);
    // Draw the other paddle, flipped
    drawWsg(&breakout->paddleWsg, breakout->paddleR.x >> DECIMAL_BITS, breakout->paddleR.y >> DECIMAL_BITS, true, false,
            0);

#endif

    // Set up variables to draw text
    char scoreStr[16] = {0};
    int16_t tWidth;

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, breakout->score[0]);
    // Measure the width of the score string
    tWidth = textWidth(&breakout->ibm, scoreStr);
    // Draw the score string to the display, centered at (TFT_WIDTH / 4)
    drawText(&breakout->ibm, c555, scoreStr, (TFT_WIDTH / 4) - (tWidth / 2), 0);

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, breakout->score[1]);
    // Measure the width of the score string
    tWidth = textWidth(&breakout->ibm, scoreStr);
    // Draw the score string to the display, centered at ((3 * TFT_WIDTH) / 4)
    drawText(&breakout->ibm, c555, scoreStr, ((3 * TFT_WIDTH) / 4) - (tWidth / 2), 0);

    // If the restart timer is active, draw it
    if (breakout->isPaused)
    {
        // Measure the width of the time string
        tWidth = textWidth(&breakout->ibm, breakoutPaused);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&breakout->ibm, c555, breakoutPaused, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else if (breakout->restartTimerUs > 0)
    {
        // Render the time to a string
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%01" PRId32 ".%03" PRId32, breakout->restartTimerUs / 1000000,
                 (breakout->restartTimerUs / 1000) % 1000);
        // Measure the width of the time string
        tWidth = textWidth(&breakout->ibm, scoreStr);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&breakout->ibm, c555, scoreStr, ((TFT_WIDTH - tWidth) / 2), 0);
    }*/
}


/**
 * This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data A pointer to the data received
 * @param len The length of the data received
 * @param rssi The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
/*static void breakoutEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&breakout->p2p, esp_now_info->src_addr, data, len, rssi);
}*/

/**
 * This function is called whenever an ESP-NOW packet is sent.
 * It is just a status callback whether or not the packet was actually sent.
 * This will be called after calling espNowSend()
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status The status of the transmission
 */
/*static void breakoutEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&breakout->p2p, mac_addr, status);
}*/

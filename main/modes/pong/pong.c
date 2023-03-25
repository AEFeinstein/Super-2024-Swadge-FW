/**
 * @file pong.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief An example Pong game
 * @date 2023-03-25
 * 
 * TODO WSGs
 * TODO sound
 * TODO LEDs
 * TODO networking
 * TODO background graphics
 */

//==============================================================================
// Includes
//==============================================================================

#include "esp_random.h"
#include "pong.h"

//==============================================================================
// Defines
//==============================================================================

/// Physics math is done with fixed point numbers where the bottom four bits are the fractional part. It's like Q28.4
#define DECIMAL_BITS 4

#define BALL_RADIUS   (8 << DECIMAL_BITS)
#define PADDLE_WIDTH  (8 << DECIMAL_BITS)
#define PADDLE_HEIGHT (40 << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)

#define SPEED_LIMIT (30 << DECIMAL_BITS)

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum of screens that may be shown in pong mode
 */
typedef enum
{
    PONG_MENU,
    PONG_GAME,
} pongScreen_t;

/**
 * @brief Enum of control schemes for a pong game
 */
typedef enum
{
    PONG_BUTTON,
    PONG_TOUCH,
    PONG_TILT,
} pongControl_t;

/**
 * @brief Enum of CPU difficulties
 */
typedef enum
{
    PONG_EASY,
    PONG_MEDIUM,
    PONG_HARD,
} pongDifficulty_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;        ///< The menu structure
    font_t ibm;          ///< The font used in the menu and game
    p2pInfo p2p;         ///< Peer to peer connectivity info, currently unused
    pongScreen_t screen; ///< The screen being displayed

    pongControl_t control;       ///< The selected control scheme
    pongDifficulty_t difficulty; ///< The selected CPU difficulty
    uint8_t score[2];            ///< The score for the game

    rectangle_t paddleL; ///< The left paddle
    rectangle_t paddleR; ///< The right paddle
    circle_t ball;       ///< The ball
    vec_t ballVel;       ///< The ball's velocity

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for paddle control
    bool paddleRMovingUp;   ///< The CPU's paddle direction on easy mode
    bool isPaused;          ///< true if the game is paused, false if it is running
} pong_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pongMainLoop(int64_t elapsedUs);
static void pongEnterMode(void);
static void pongExitMode(void);
static void pongEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void pongEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void pongMenuCb(const char* label, bool selected);
static void pongGameLoop(int64_t elapsedUs);

static void pongResetGame(bool isInit, uint8_t whoWon);
static void IncreaseBallVelocity(int16_t velAdd);
static void DrawField(void);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char pongName[] = "Pong";

static const char pongCtrlButton[] = "Button Control";
static const char pongCtrlTouch[]  = "Touch Control";
static const char pongCtrlTilt[]   = "Tilt Control";

static const char pongDiffEasy[]   = "Easy";
static const char pongDiffMedium[] = "Medium";
static const char pongDiffHard[]   = "Impossible";

static const char pongPaused[] = "Paused";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Pong
swadgeMode_t pongMode = {
    .modeName                 = pongName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .fnEnterMode              = pongEnterMode,
    .fnExitMode               = pongExitMode,
    .fnMainLoop               = pongMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = pongEspNowRecvCb,
    .fnEspNowSendCb           = pongEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Pong mode. This whole struct is calloc()'d and free()'d so that Pong is only
/// using memory while it is being played
pong_t* pong = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Pong mode, allocate required memory, and initialize required variables
 *
 */
static void pongEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    pong = calloc(1, sizeof(pong_t));

    // Load a font
    loadFont("ibm_vga8.font", &pong->ibm, false);

    // Initialize the menu
    pong->menu = initMenu(pongName, &pong->ibm, pongMenuCb);

    // These are the possible control schemes
    const char* controlSchemes[] = {
        pongCtrlButton,
        pongCtrlTouch,
        pongCtrlTilt,
    };

    // Add each control scheme to the menu. Each control scheme has a submenu to select difficulty
    for (uint8_t i = 0; i < ARRAY_SIZE(controlSchemes); i++)
    {
        // Add a control scheme to the menu. This opens a submenu with difficulties
        pong->menu = startSubMenu(pong->menu, controlSchemes[i]);
        // Add difficulties to the submenu
        addSingleItemToMenu(pong->menu, pongDiffEasy);
        addSingleItemToMenu(pong->menu, pongDiffMedium);
        addSingleItemToMenu(pong->menu, pongDiffHard);
        // End the submenu
        pong->menu = endSubMenu(pong->menu);
    }

    // Set the mode to menu mode
    pong->screen = PONG_MENU;
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void pongExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(pong->menu);
    // Free the font
    freeFont(&pong->ibm);
    // Free everything else
    free(pong);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 */
static void pongMenuCb(const char* label, bool selected)
{
    // Only care about selected items, not scrolled-to items.
    // The same callback is called from the menu and submenu with no indication of which menu it was called from
    // Note that the label arg will be one of the strings used in startSubMenu() or addSingleItemToMenu()
    if (selected)
    {
        // Save what control scheme is selected (first-level menu)
        if (label == pongCtrlButton)
        {
            pong->control = PONG_BUTTON;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == pongCtrlTouch)
        {
            pong->control = PONG_TOUCH;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == pongCtrlTilt)
        {
            pong->control = PONG_TILT;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == pongDiffEasy)
        {
            pong->difficulty = PONG_EASY;
            pongResetGame(true, 0);
            pong->screen = PONG_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == pongDiffMedium)
        {
            pong->difficulty = PONG_MEDIUM;
            pongResetGame(true, 0);
            pong->screen = PONG_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == pongDiffHard)
        {
            pong->difficulty = PONG_HARD;
            pongResetGame(true, 0);
            pong->screen = PONG_GAME;
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void pongMainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (pong->screen)
    {
        case PONG_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                pong->menu = menuButton(pong->menu, evt);
            }

            // Draw the menu
            drawMenu(pong->menu);
            break;
        }
        case PONG_GAME:
        {
            // Run the main game loop. This will also process button events
            pongGameLoop(elapsedUs);
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
static void pongGameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        pong->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            pong->isPaused = !pong->isPaused;
        }
    }

    // If the game is paused
    if (pong->isPaused)
    {
        // Just draw and return
        DrawField();
        return;
    }

    // While the restart timer is active
    while (pong->restartTimerUs > 0)
    {
        // Decrement the timer and draw the field, but don't run game logic
        pong->restartTimerUs -= elapsedUs;
        DrawField();
        return;
    }

    // Move the paddle depending on the chosen control scheme
    switch (pong->control)
    {
        default:
        case PONG_BUTTON:
        {
            // Move the player paddle if a button is currently down
            if (pong->btnState & PB_UP)
            {
                pong->paddleL.y = MAX(pong->paddleL.y - (5 << DECIMAL_BITS), 0);
            }
            else if (pong->btnState & PB_DOWN)
            {
                pong->paddleL.y = MIN(pong->paddleL.y + (5 << DECIMAL_BITS), FIELD_HEIGHT - pong->paddleL.height);
            }
            break;
        }
        case PONG_TOUCH:
        {
            // Check if the touch area is touched
            int32_t centerVal, intensityVal;
            if (getTouchCentroid(&centerVal, &intensityVal))
            {
                // If there is a touch, move the paddle to that location of the touch
                pong->paddleL.y = (centerVal * (FIELD_HEIGHT - pong->paddleL.height)) / 1024;
            }
            break;
        }
        case PONG_TILT:
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelGetAccelVec(&a_x, &a_y, &a_z))
            {
                // Move the paddle to the current tilt location
                pong->paddleL.y = CLAMP(((a_x + 100) * (FIELD_HEIGHT - pong->paddleL.height)) / 350, 0,
                                        (FIELD_HEIGHT - pong->paddleL.height));
            }
            break;
        }
    }

    // Move the computer paddle
    switch (pong->difficulty)
    {
        default:
        case PONG_EASY:
        {
            // Blindly move up and down
            if (pong->paddleRMovingUp)
            {
                pong->paddleR.y = MAX(pong->paddleR.y - (4 << DECIMAL_BITS), 0);
                // If the top boundary was hit
                if (0 == pong->paddleR.y)
                {
                    // Start moving down
                    pong->paddleRMovingUp = false;
                }
            }
            else
            {
                pong->paddleR.y = MIN(pong->paddleR.y + (4 << DECIMAL_BITS), FIELD_HEIGHT - pong->paddleR.height);
                // If the bottom boundary was hit
                if ((FIELD_HEIGHT - pong->paddleR.height) == pong->paddleR.y)
                {
                    // Start moving up
                    pong->paddleRMovingUp = true;
                }
            }
            break;
        }
        case PONG_MEDIUM:
        {
            // Move towards the ball, slowly
            if (pong->paddleR.y + (pong->paddleR.height / 2) < pong->ball.y)
            {
                pong->paddleR.y = MIN(pong->paddleR.y + (2 << DECIMAL_BITS), FIELD_HEIGHT - pong->paddleR.height);
            }
            else
            {
                pong->paddleR.y = MAX(pong->paddleR.y - (2 << DECIMAL_BITS), 0);
            }
            break;
        }
        case PONG_HARD:
        {
            // Never miss
            pong->paddleR.y = CLAMP(pong->ball.y - pong->paddleR.height / 2, 0, FIELD_HEIGHT - pong->paddleR.height);
            break;
        }
    }

    // Update the ball's position
    pong->ball.x += (pong->ballVel.x * elapsedUs) / 100000;
    pong->ball.y += (pong->ballVel.y * elapsedUs) / 100000;

    // Check for goals
    if (pong->ball.x < 0 || pong->ball.x > FIELD_WIDTH)
    {
        // Reset the game. This keeps score.
        pongResetGame(false, pong->ball.x < 0 ? 1 : 0);
    }
    else
    {
        // Checks for top and bottom wall collisions
        if (((pong->ball.y - pong->ball.radius) < 0 && pong->ballVel.y < 0)
            || ((pong->ball.y + pong->ball.radius) > FIELD_HEIGHT && pong->ballVel.y > 0))
        {
            // Reverse direction
            pong->ballVel.y = -pong->ballVel.y;
        }

        // If a goal was not scored,c heck for left paddle collision
        if ((pong->ballVel.x < 0) && circleRectIntersection(pong->ball, pong->paddleL))
        {
            // Reverse direction
            pong->ballVel.x = -pong->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = pong->ball.y - (pong->paddleL.y + pong->paddleL.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            pong->ballVel = rotateVec2d(pong->ballVel, (45 * diff) / (pong->paddleL.height / 2));

            // Increase velocity
            IncreaseBallVelocity(1 << DECIMAL_BITS);
        }
        // Check for right paddle collision
        else if ((pong->ballVel.x > 0) && circleRectIntersection(pong->ball, pong->paddleR))
        {
            // Reverse direction
            pong->ballVel.x = -pong->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = pong->ball.y - (pong->paddleR.y + pong->paddleR.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            pong->ballVel = rotateVec2d(pong->ballVel, -(45 * diff) / (pong->paddleR.height / 2));

            // Increase velocity
            IncreaseBallVelocity(1 << DECIMAL_BITS);
        }
    }

    // Draw the field
    DrawField();
}

/**
 * @brief Reset the pong game variables
 *
 * @param isInit True if this is the first time this is being called, false if it is reset after a player scores
 * @param whoWon The player who scored a point, either 0 or 1
 */
static void pongResetGame(bool isInit, uint8_t whoWon)
{
    // Set different variables based on initialization
    if (isInit)
    {
        // Set up the left paddle
        pong->paddleL.x      = 0;
        pong->paddleL.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        pong->paddleL.width  = PADDLE_WIDTH;
        pong->paddleL.height = PADDLE_HEIGHT;

        // Set up the right paddle
        pong->paddleR.x      = FIELD_WIDTH - PADDLE_WIDTH;
        pong->paddleR.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        pong->paddleR.width  = PADDLE_WIDTH;
        pong->paddleR.height = PADDLE_HEIGHT;
    }
    else
    {
        // Tally the score
        pong->score[whoWon]++;
    }

    // Set the restart timer
    pong->restartTimerUs = 2000000;

    // Set the ball variables
    pong->ball.x      = (FIELD_WIDTH) / 2;
    pong->ball.y      = (FIELD_HEIGHT) / 2;
    pong->ball.radius = BALL_RADIUS;

    // Give the ball initial velocity to the top right
    pong->ballVel.x = (4 << DECIMAL_BITS);
    pong->ballVel.y = -(4 << DECIMAL_BITS);

    // Rotate up to 90 degrees, for some randomness
    pong->ballVel = rotateVec2d(pong->ballVel, esp_random() % 90);

    // Determine the direction of the serve based on who scored last
    if (whoWon)
    {
        pong->ballVel.x = -pong->ballVel.x;
    }
}

/**
 * @brief Increase the ball's velocity by a fixed amount
 *
 * @param magnitude The magnitude velocity to add to the ball
 */
static void IncreaseBallVelocity(int16_t magnitude)
{
    if (sqMagVec2d(pong->ballVel) < (SPEED_LIMIT * SPEED_LIMIT))
    {
        // Create a vector in the same direction as pong->ballVel with the given magnitude
        int32_t denom  = ABS(pong->ballVel.x) + ABS(pong->ballVel.y);
        vec_t velBoost = {
            .x = (magnitude * pong->ballVel.x) / denom,
            .y = (magnitude * pong->ballVel.y) / denom,
        };

        // Add the vectors together
        pong->ballVel = addVec2d(pong->ballVel, velBoost);
    }
}

/**
 * @brief Draw the Pong field to the TFT
 */
static void DrawField(void)
{
    // Clear the display
    clearPxTft();

    // Bitshift the ball's location and radius from math coordinates to screen coordinates, then draw it
    drawCircleFilled((pong->ball.x) >> DECIMAL_BITS, (pong->ball.y) >> DECIMAL_BITS,
                     (pong->ball.radius) >> DECIMAL_BITS, c555);

    // Bitshift the left paddle's location and radius from math coordinates to screen coordinates, then draw it
    drawRect((pong->paddleL.x >> DECIMAL_BITS), (pong->paddleL.y >> DECIMAL_BITS),
             ((pong->paddleL.x + pong->paddleL.width) >> DECIMAL_BITS),
             ((pong->paddleL.y + pong->paddleL.height) >> DECIMAL_BITS), c050);

    // Bitshift the right paddle's location and radius from math coordinates to screen coordinates, then draw it
    drawRect((pong->paddleR.x >> DECIMAL_BITS), (pong->paddleR.y >> DECIMAL_BITS),
             ((pong->paddleR.x + pong->paddleR.width) >> DECIMAL_BITS),
             ((pong->paddleR.y + pong->paddleR.height) >> DECIMAL_BITS), c050);

    // Set up variables to draw text
    char scoreStr[16] = {0};
    int16_t tWidth;

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, pong->score[0]);
    // Measure the width of the score string
    tWidth = textWidth(&pong->ibm, scoreStr);
    // Draw the score string to the display, centered at (TFT_WIDTH / 4)
    drawText(&pong->ibm, c555, scoreStr, (TFT_WIDTH / 4) - (tWidth / 2), 0);

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, pong->score[1]);
    // Measure the width of the score string
    tWidth = textWidth(&pong->ibm, scoreStr);
    // Draw the score string to the display, centered at ((3 * TFT_WIDTH) / 4)
    drawText(&pong->ibm, c555, scoreStr, ((3 * TFT_WIDTH) / 4) - (tWidth / 2), 0);

    // If the restart timer is active, draw it
    if (pong->isPaused)
    {
        // Measure the width of the time string
        tWidth = textWidth(&pong->ibm, pongPaused);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&pong->ibm, c555, pongPaused, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else if (pong->restartTimerUs > 0)
    {
        // Render the time to a string
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%01" PRId32 ".%03" PRId32, pong->restartTimerUs / 1000000,
                 (pong->restartTimerUs / 1000) % 1000);
        // Measure the width of the time string
        tWidth = textWidth(&pong->ibm, scoreStr);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&pong->ibm, c555, scoreStr, ((TFT_WIDTH - tWidth) / 2), 0);
    }
}

/**
 * This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data A pointer to the data received
 * @param len The length of the data received
 * @param rssi The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
static void pongEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&pong->p2p, esp_now_info->src_addr, data, len, rssi);
}

/**
 * This function is called whenever an ESP-NOW packet is sent.
 * It is just a status callback whether or not the packet was actually sent.
 * This will be called after calling espNowSend()
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status The status of the transmission
 */
static void pongEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&pong->p2p, mac_addr, status);
}

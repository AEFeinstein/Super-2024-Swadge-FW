//==============================================================================
// Includes
//==============================================================================

#include "esp_random.h"
#include "pong.h"

//==============================================================================
// Defines
//==============================================================================

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

typedef enum
{
    PONG_MENU,
    PONG_GAME,
} pongScreen_t;

typedef enum
{
    PONG_BUTTON,
    PONG_TOUCH,
    PONG_TILT,
} pongControl_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t x;
    int32_t y;
} vec_t;

typedef struct
{
    menu_t* menu;
    font_t ibm;
    p2pInfo p2p;
    vec_t ballLoc;
    vec_t ballVel;
    pongScreen_t screen;
    pongControl_t control;
    int32_t paddleLocL;
    int32_t paddleLocR;
    uint32_t restartTimerUs;
    uint16_t btnState;
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

static void ResetGame(bool isInit, uint8_t whoWon);
static void RotateBall(int16_t degree);
static void IncreaseSpeed(int16_t speedM);
static void DrawField(void);
static bool checkCircleRectCollision(int32_t cx, int32_t cy, int32_t radius, int32_t rx, int32_t ry, int32_t rw,
                                     int32_t rh);

//==============================================================================
// Variables
//==============================================================================

/// @brief The name for Pong mode, a const char
const char pongName[] = "Pong";

static const char pongCtrlButton[] = "Button Control";
static const char pongCtrlTouch[]  = "Touch Control";
static const char pongCtrlTilt[]   = "Tilt Control";

/// @brief The Swadge mode for Pong
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

/// @brief All state information for the Pong mode. This whole struct is calloc()'d and free()'d so that Pong is only
/// using memory while it is being played
pong_t* pong = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void pongEnterMode(void)
{
    // Allocate and clear all memory for this mode
    pong = calloc(1, sizeof(pong_t));

    // Load a font
    loadFont("ibm_vga8.font", &pong->ibm, false);

    // Allocate the menu
    pong->menu = initMenu(pongName, &pong->ibm, pongMenuCb);

    // Add control schemes to the menu
    addSingleItemToMenu(pong->menu, pongCtrlButton);
    addSingleItemToMenu(pong->menu, pongCtrlTouch);
    addSingleItemToMenu(pong->menu, pongCtrlTilt);

    // Set the mode to menu mode
    pong->screen = PONG_MENU;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void pongExitMode(void)
{
    free(pong);
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 */
static void pongMenuCb(const char* label, bool selected)
{
    if (selected)
    {
        if (label == pongCtrlButton)
        {
            ResetGame(true, 0);
            pong->screen  = PONG_GAME;
            pong->control = PONG_BUTTON;
        }
        else if (label == pongCtrlTouch)
        {
            ResetGame(true, 0);
            pong->screen  = PONG_GAME;
            pong->control = PONG_TOUCH;
        }
        else if (label == pongCtrlTilt)
        {
            ResetGame(true, 0);
            pong->screen  = PONG_GAME;
            pong->control = PONG_TILT;
        }
    }
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void pongMainLoop(int64_t elapsedUs)
{
    // Run the main loop depending on the screen being displayed
    switch (pong->screen)
    {
        case PONG_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                pong->menu = menuButton(pong->menu, evt);
            }

            // Draw the menu
            drawMenu(pong->menu);
            break;
        }
        case PONG_GAME:
        {
            // Draw the game
            pongGameLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void pongGameLoop(int64_t elapsedUs)
{
    // Always process button events, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        pong->btnState = evt.state;
    }

    // TODO count-in timer

    // Move the paddle depending on the chosen control scheme
    switch (pong->control)
    {
        default:
        case PONG_BUTTON:
        {
            // Move the player paddle
            if (pong->btnState & PB_UP)
            {
                pong->paddleLocL = MAX(pong->paddleLocL - (5 << DECIMAL_BITS), 0);
            }
            else if (pong->btnState & PB_DOWN)
            {
                pong->paddleLocL = MIN(pong->paddleLocL + (5 << DECIMAL_BITS), FIELD_HEIGHT - PADDLE_HEIGHT);
            }
            break;
        }
        case PONG_TOUCH:
        {
            // Check if the touch area is touched
            int32_t centerVal, intensityVal;
            if (getTouchCentroid(&centerVal, &intensityVal))
            {
                // If there is a touch, move the paddle to that location
                pong->paddleLocL = (centerVal * (FIELD_HEIGHT - PADDLE_HEIGHT)) / 1024;
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
                pong->paddleLocL
                    = CLAMP(((a_x + 100) * (FIELD_HEIGHT - PADDLE_HEIGHT)) / 350, 0, (FIELD_HEIGHT - PADDLE_HEIGHT));
            }
            break;
        }
    }

    // TODO Move the computer paddle
    pong->paddleLocR = CLAMP(pong->ballLoc.y - PADDLE_HEIGHT / 2, 0, FIELD_HEIGHT - PADDLE_HEIGHT);

    // Update the ball's position
    pong->ballLoc.x += (pong->ballVel.x * elapsedUs) / 100000;
    pong->ballLoc.y += (pong->ballVel.y * elapsedUs) / 100000;

    // TODO victory check
    // TODO score

    // Checks for top and bottom wall collisons
    if (((pong->ballLoc.y - BALL_RADIUS) < 0 && pong->ballVel.y < 0)
        || ((pong->ballLoc.y + BALL_RADIUS) > FIELD_HEIGHT && pong->ballVel.y > 0))
    {
        // Reverse direction
        pong->ballVel.y = -pong->ballVel.y;
    }

    // check for game over
    if (pong->ballLoc.x < 0 || pong->ballLoc.x > FIELD_WIDTH)
    {
        ResetGame(false, pong->ballLoc.x < 0 ? 0 : 1);
        DrawField();
        return;
    }

    // Check for left paddle collision
    if ((pong->ballVel.x < 0)
        && checkCircleRectCollision(pong->ballLoc.x, pong->ballLoc.y, BALL_RADIUS, 0, pong->paddleLocL, PADDLE_WIDTH,
                                    PADDLE_HEIGHT))
    {
        // Reverse direction
        pong->ballVel.x = -pong->ballVel.x;

        // Apply extra rotation depending on part of the paddle hit
        // range of diff is -PADDLE_SIZE/2 to PADDLE_SIZE/2
        int16_t diff = pong->ballLoc.y - (pong->paddleLocL + PADDLE_HEIGHT / 2);
        // rotate 45deg at edge of paddle, 0deg in middle, linear in between
        RotateBall((45 * diff) / (PADDLE_HEIGHT / 2));

        // Increase speed
        IncreaseSpeed(1 << DECIMAL_BITS);
    }
    // Check for right paddle collision
    else if ((pong->ballVel.x > 0)
             && checkCircleRectCollision(pong->ballLoc.x, pong->ballLoc.y, BALL_RADIUS, FIELD_WIDTH - PADDLE_WIDTH,
                                         pong->paddleLocR, PADDLE_WIDTH, PADDLE_HEIGHT))
    {
        // Reverse direction
        pong->ballVel.x = -pong->ballVel.x;

        // Apply extra rotation depending on part of the paddle hit
        // range of diff is -PADDLE_SIZE/2 to PADDLE_SIZE/2
        int16_t diff = pong->ballLoc.y - (pong->paddleLocR + PADDLE_HEIGHT / 2);
        // rotate 45deg at edge of paddle, 0deg in middle, linear in between
        RotateBall((45 * diff) / (PADDLE_HEIGHT / 2));

        // Increase speed
        IncreaseSpeed(1 << DECIMAL_BITS);
    }

    DrawField();
}

/**
 * @brief CIRCLE/RECTANGLE
 *
 * @param cx
 * @param cy
 * @param radius
 * @param rx
 * @param ry
 * @param rw
 * @param rh
 * @return true
 * @return false
 */
static bool checkCircleRectCollision(int32_t cx, int32_t cy, int32_t radius, int32_t rx, int32_t ry, int32_t rw,
                                     int32_t rh)
{
    // temporary variables to set edges for testing
    int32_t testX = cx;
    int32_t testY = cy;

    // which edge is closest?
    if (cx < rx)
    {
        // test left edge
        testX = rx;
    }
    else if (cx > rx + rw)
    {
        // right edge
        testX = rx + rw;
    }

    if (cy < ry)
    {
        // top edge
        testY = ry;
    }
    else if (cy > ry + rh)
    {
        // bottom edge
        testY = ry + rh;
    }

    // get distance from closest edges
    int32_t distX    = cx - testX;
    int32_t distY    = cy - testY;
    int32_t distance = (distX * distX) + (distY * distY);

    // if the distance is less than the radius, collision!
    if (distance <= radius * radius)
    {
        return true;
    }
    return false;
}

/**
 * @brief TODO
 *
 * @param isInit
 * @param whoWon
 */
static void ResetGame(bool isInit, uint8_t whoWon)
{
    if (isInit)
    {
        pong->paddleLocL = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        pong->paddleLocR = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
    }
    pong->restartTimerUs = 1000000;
    pong->ballLoc.x      = (FIELD_WIDTH) / 2;
    pong->ballLoc.y      = (FIELD_HEIGHT) / 2;
    if (whoWon)
    {
        pong->ballVel.x = (5 << DECIMAL_BITS);
    }
    else
    {
        pong->ballVel.x = -(5 << DECIMAL_BITS);
    }
    uint8_t initY = esp_random() % 8;
    if (initY == 4)
    {
        initY++;
    }
    pong->ballVel.y = (initY - 4) << DECIMAL_BITS;
    DrawField();
}

/**
 * @brief Rotate the ball in degrees (-360 -> 359)
 *
 * @param degree
 */
static void RotateBall(int16_t degree)
{
    // printf("rotate %" PRId16 ": ", degree);
    while (degree < 0)
    {
        degree += 360;
    }
    while (degree > 359)
    {
        degree -= 360;
    }

    int16_t sin  = getSin1024(degree);
    int16_t cos  = getCos1024(degree);
    int32_t oldX = pong->ballVel.x;
    int32_t oldY = pong->ballVel.y;

    // printf("[%3d %3d] -> ", pong->ballVel.x, pong->ballVel.y);
    pong->ballVel.x = ((oldX * cos) - (oldY * sin)) / 1024;
    pong->ballVel.y = ((oldX * sin) + (oldY * cos)) / 1024;
    // printf("[%3d %3d]\n", pong->ballVel.x, pong->ballVel.y);
}

/**
 * @brief Apply a multiplier to the velocity.
 * TODO make this additive instead?
 *
 * @param speedM
 */
static void IncreaseSpeed(int16_t speedM)
{
    int32_t denom      = ABS(pong->ballVel.x) + ABS(pong->ballVel.y);
    int32_t xComponent = (speedM * pong->ballVel.x) / denom;
    int32_t yComponent = (speedM * pong->ballVel.y) / denom;

    // printf("Speedup [%3d %3d] + [%3d %3d] = ", xComponent, yComponent, pong->ballVel.x, pong->ballVel.y);
    pong->ballVel.x = CLAMP((pong->ballVel.x + xComponent), -SPEED_LIMIT, SPEED_LIMIT);
    pong->ballVel.y = CLAMP((pong->ballVel.y + yComponent), -SPEED_LIMIT, SPEED_LIMIT);
    // printf("[%3d %3d]\n", pong->ballVel.x, pong->ballVel.y);
}

/**
 * @brief Draw the Pong field to the TFT
 */
static void DrawField(void)
{
    clearPxTft();
    drawCircleFilled((pong->ballLoc.x) >> DECIMAL_BITS, (pong->ballLoc.y) >> DECIMAL_BITS, BALL_RADIUS >> DECIMAL_BITS,
                     c555);
    drawRect(0, (pong->paddleLocL >> DECIMAL_BITS), PADDLE_WIDTH >> DECIMAL_BITS,
             (pong->paddleLocL + PADDLE_HEIGHT) >> DECIMAL_BITS, c050);
    drawRect((FIELD_WIDTH - PADDLE_WIDTH) >> DECIMAL_BITS, (pong->paddleLocR) >> DECIMAL_BITS,
             FIELD_WIDTH >> DECIMAL_BITS, (pong->paddleLocR + PADDLE_HEIGHT) >> DECIMAL_BITS, c005);
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

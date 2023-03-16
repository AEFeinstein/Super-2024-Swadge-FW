#include "esp_random.h"
#include "pong.h"

#define DECIMAL_BITS 4

#define BALL_RADIUS   (8 << DECIMAL_BITS)
#define PADDLE_WIDTH  (8 << DECIMAL_BITS)
#define PADDLE_HEIGHT (40 << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)

#define SPEED_LIMIT (30 << DECIMAL_BITS)

static void pongMainLoop(int64_t elapsedUs);
static void pongEnterMode(void);
static void pongExitMode(void);
static void pongEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void pongEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

void ResetGame(bool isInit, uint8_t whoWon);
void RotateBall(int16_t degree);
void IncreaseSpeed(int16_t speedM);
void DrawField(void);

const char pongName[] = "Pong";

swadgeMode_t pongMode = {
    .modeName                 = pongName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
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

typedef struct
{
    int32_t x;
    int32_t y;
} vec_t;

typedef struct
{
    p2pInfo p2p;
    vec_t ballLoc;
    vec_t ballVel;
    int32_t paddleLocL;
    int32_t paddleLocR;
    uint32_t restartTimerUs;
    uint16_t btnState;
} pong_t;

pong_t* pong = NULL;

/**************************
 * *
 * Pong!! *
 * *
 ***************************/

/**
 * @brief TODO
 *
 */
static void pongEnterMode(void)
{
    pong = calloc(1, sizeof(pong_t));
    ResetGame(true, 0);
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void pongExitMode(void)
{
    free(pong);
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
static bool circleRect(int32_t cx, int32_t cy, int32_t radius, int32_t rx, int32_t ry, int32_t rw, int32_t rh)
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
 * @param elapsedUs
 */
static void pongMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        pong->btnState = evt.state;
    }

    // TODO count-in timer

    // Move the player paddle
    if (pong->btnState & PB_UP)
    {
        pong->paddleLocL = MAX(pong->paddleLocL - (5 << DECIMAL_BITS), 0);
    }
    else if (pong->btnState & PB_DOWN)
    {
        pong->paddleLocL = MIN(pong->paddleLocL + (5 << DECIMAL_BITS), FIELD_HEIGHT - PADDLE_HEIGHT);
    }

    // TODO Move the computer paddle
    pong->paddleLocR = CLAMP(pong->ballLoc.y - PADDLE_HEIGHT / 2, 0, FIELD_HEIGHT - PADDLE_HEIGHT);

    // Update the ball's position
    pong->ballLoc.x += (pong->ballVel.x * elapsedUs) / 100000;
    pong->ballLoc.y += (pong->ballVel.y * elapsedUs) / 100000;

    // TODO victory check

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
        && circleRect(pong->ballLoc.x, pong->ballLoc.y, BALL_RADIUS, 0, pong->paddleLocL, PADDLE_WIDTH, PADDLE_HEIGHT))
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
             && circleRect(pong->ballLoc.x, pong->ballLoc.y, BALL_RADIUS, FIELD_WIDTH - PADDLE_WIDTH, pong->paddleLocR,
                           PADDLE_WIDTH, PADDLE_HEIGHT))
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
 * @brief TODO
 *
 * @param isInit
 * @param whoWon
 */
void ResetGame(bool isInit, uint8_t whoWon)
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
void RotateBall(int16_t degree)
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
void IncreaseSpeed(int16_t speedM)
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
void DrawField(void)
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

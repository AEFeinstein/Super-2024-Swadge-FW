#include "esp_random.h"
#include "pong.h"

// #define BOARD_SIZE 240 /* In pixels */
#define IRQ_HZ 32

// #define S_M                    IRQ_HZ          /* Position Multiplier */
#define V_M IRQ_HZ /* Velocity Multiplier */
// #define PADDLE_SIZE            (4 * V_M * S_M) /* in sim size */
// #define TIME_STEP              (S_M / IRQ_HZ)  /* (1/32) seconds * 32 multiplier */
// #define EXTRA_ROTATION_ON_EDGE 15              /* degrees */
#define SPEED_LIMIT 1200

#define X 0
#define Y 1

#define BALL_RADIUS   16
#define PADDLE_WIDTH  8
#define PADDLE_HEIGHT 40

#define FIELD_HEIGHT TFT_HEIGHT
#define FIELD_WIDTH  TFT_WIDTH

#define DEAD_ZONE 15

static void pongMainLoop(int64_t elapsedUs);
static void pongEnterMode(void);
static void pongExitMode(void);
static void pongEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void pongEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

void ResetGame(uint8_t isInit, uint8_t whoWon);
void RotateBall(int16_t degree);
void IncreaseSpeed(int16_t speedM);
void DrawField(void);
void ProcessInput(__attribute__((unused)) int32_t p1ax, int32_t p1ay, __attribute__((unused)) int8_t p1bl,
                  __attribute__((unused)) int8_t p1br, __attribute__((unused)) int8_t p1bu,
                  __attribute__((unused)) int8_t p1bd, __attribute__((unused)) int32_t p2ax, int32_t p2ay,
                  __attribute__((unused)) int8_t p2bl, __attribute__((unused)) int8_t p2br,
                  __attribute__((unused)) int8_t p2bu, __attribute__((unused)) int8_t p2bd);

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
    p2pInfo p2p;
    int32_t ballLoc[2];
    int32_t ballVel[2];
    int32_t paddleLocL;
    int32_t paddleLocR;
    uint16_t restartTimer;
} pong_t;

pong_t* pong = NULL;

/**************************
 *                         *
 *         Pong!!          *
 *                         *
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
    // Physics!
    // Update the ball's position
    pong->ballLoc[X] += (pong->ballVel[X] * elapsedUs) / 100000;
    pong->ballLoc[Y] += (pong->ballVel[Y] * elapsedUs) / 100000;

    // Checks for top and bottom wall collisons
    if (((pong->ballLoc[Y] - BALL_RADIUS) < 0 && pong->ballVel[Y] < 0)
        || ((pong->ballLoc[Y] + BALL_RADIUS) > FIELD_HEIGHT && pong->ballVel[Y] > 0))
    {
        // Reverse direction
        pong->ballVel[Y] = -pong->ballVel[Y];
    }

    if (((pong->ballLoc[X] - BALL_RADIUS) < 0 && pong->ballVel[X] < 0)
        || ((pong->ballLoc[X] + BALL_RADIUS) > FIELD_WIDTH && pong->ballVel[X] > 0))
    {
        // Reverse direction
        pong->ballVel[X] = -pong->ballVel[X];
    }

    if ((pong->ballVel[X] < 0)
        && circleRect(pong->ballLoc[X], pong->ballLoc[Y], BALL_RADIUS, 0, pong->paddleLocL, PADDLE_WIDTH,
                      PADDLE_HEIGHT))
    {
        // Reverse direction
        pong->ballVel[X] = -pong->ballVel[X];
        // TODO increase speed
        // TODO angle collision
    }
    else if ((pong->ballVel[X] > 0)
             && circleRect(pong->ballLoc[X], pong->ballLoc[Y], BALL_RADIUS, FIELD_WIDTH - PADDLE_WIDTH,
                           pong->paddleLocR, PADDLE_WIDTH, PADDLE_HEIGHT))
    {
        // Reverse direction
        pong->ballVel[X] = -pong->ballVel[X];
        // TODO increase speed
        // TODO angle collision
    }

    // TODO check for win condition
    // TODO count-in timer

    DrawField();

    // int16_t rotation;
    // if (pong->restartTimer > 0)
    // {
    //     pong->restartTimer--;
    // }
    // else
    // {
    //     // Physics!
    //     // Update the ball's position
    //     pong->ballLoc[X] += (pong->ballVel[X] * elapsedUs);
    //     pong->ballLoc[Y] += (pong->ballVel[Y] * elapsedUs);

    //     // Check for collisions, win conditions. Nudge the ball if necessary
    //     // Top wall collision
    //     if (pong->ballLoc[Y] < 0 && pong->ballVel[Y] < 0)
    //     {
    //         pong->ballVel[Y] = -pong->ballVel[Y];
    //     }
    //     // Bottom wall collision
    //     else if (pong->ballLoc[Y] >= FIELD_HEIGHT && pong->ballVel[Y] > 0)
    //     {
    //         pong->ballVel[Y] = -pong->ballVel[Y];
    //     }
    //     // left paddle collision
    //     else if (pong->ballLoc[X] < 1 * S_M * V_M && pong->ballVel[X] < 0
    //              && (pong->paddleLocL <= pong->ballLoc[Y] && pong->ballLoc[Y] < pong->paddleLocL + PADDLE_SIZE))
    //     {
    //         pong->ballVel[X] = -pong->ballVel[X];
    //         pong->ballLoc[X] = 1 * S_M * V_M; // right on the edge of the paddle

    //         // Increase speed on collision
    //         IncreaseSpeed(37); // remember, divided by V_M

    //         // Apply extra rotation depending on part of the paddle hit
    //         // range of diff is -PADDLE_SIZE/2 to PADDLE_SIZE/2 (+/- 8192)
    //         int16_t diff = pong->ballLoc[Y] - (pong->paddleLocL + PADDLE_SIZE / 2);
    //         // rotate 45deg at edge of paddle, 0deg in middle, linear in between
    //         rotation = (EXTRA_ROTATION_ON_EDGE * diff) / (PADDLE_SIZE / 2);
    //         RotateBall(rotation);
    //     }
    //     // right paddle collision
    //     else if (pong->ballLoc[X] >= (BOARD_SIZE - 1) * S_M * V_M && pong->ballVel[X] > 0
    //              && (pong->paddleLocR <= pong->ballLoc[Y] && pong->ballLoc[Y] < pong->paddleLocR + PADDLE_SIZE))
    //     {
    //         pong->ballVel[X] = -pong->ballVel[X];
    //         pong->ballLoc[X] = (BOARD_SIZE - 1) * S_M * V_M - 1; // right on the edge of the paddle

    //         // Increase speed on collision
    //         IncreaseSpeed(37); // remember, divided by V_M

    //         // Apply extra rotation depending on part of the paddle hit
    //         // range of diff is -PADDLE_SIZE/2 to PADDLE_SIZE/2 (+/- 8192)
    //         int16_t diff = pong->ballLoc[Y] - (pong->paddleLocR + PADDLE_SIZE / 2);
    //         // rotate 45deg at edge of paddle, 0deg in middle, linear in between
    //         rotation = (EXTRA_ROTATION_ON_EDGE * diff) / (PADDLE_SIZE / 2);
    //         RotateBall(-rotation);
    //     }
    //     // left wall win
    //     else if (pong->ballLoc[X] < 0 && pong->ballVel[X] < 0)
    //     {
    //         ResetGame(0, 1);
    //     }
    //     // right wall win
    //     else if (pong->ballLoc[X] >= BOARD_SIZE * S_M * V_M && pong->ballVel[X] > 0)
    //     {
    //         ResetGame(0, 0);
    //     }
    // }
    // DrawField();
}

/**
 * @brief TODO
 *
 * @param isInit
 * @param whoWon
 */
void ResetGame(uint8_t isInit, uint8_t whoWon)
{
    if (isInit)
    {
        pong->paddleLocL = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        pong->paddleLocR = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
    }
    pong->restartTimer = IRQ_HZ * 3;
    pong->ballLoc[X]   = (FIELD_WIDTH) / 2;
    pong->ballLoc[Y]   = (FIELD_HEIGHT) / 2;
    if (whoWon)
    {
        pong->ballVel[X] = 5;
    }
    else
    {
        pong->ballVel[X] = 5;
    }
    uint8_t initY = esp_random() % 8;
    if (initY == 4)
    {
        initY++;
    }
    pong->ballVel[Y] = (initY - 4) * V_M;
    DrawField();
}

/**
 * @brief Rotate the ball in degrees (-360 -> 359)
 *
 * @param degree
 */
void RotateBall(int16_t degree)
{
    if (degree < 0)
    {
        degree += 360;
    }
    if (degree > 359)
    {
        degree -= 360;
    }

    int16_t sin = getSin1024(degree);
    int16_t cos = getCos1024(degree);

    pong->ballVel[X] = (pong->ballVel[X] * (int32_t)cos - pong->ballVel[Y] * (int32_t)sin) / 32;
    pong->ballVel[Y] = (pong->ballVel[X] * (int32_t)sin + pong->ballVel[Y] * (int32_t)cos) / 32;
}

/**
 * @brief Apply a multiplier to the velocity. make this additive instead?
 *
 * @param speedM
 */
void IncreaseSpeed(int16_t speedM)
{
    pong->ballVel[X] = (pong->ballVel[X] * speedM) / V_M;
    if (pong->ballVel[X] > SPEED_LIMIT)
    {
        pong->ballVel[X] = SPEED_LIMIT;
    }
    else if (pong->ballVel[X] < -SPEED_LIMIT)
    {
        pong->ballVel[X] = -SPEED_LIMIT;
    }

    pong->ballVel[Y] = (pong->ballVel[Y] * speedM) / V_M;
    if (pong->ballVel[Y] > SPEED_LIMIT)
    {
        pong->ballVel[Y] = SPEED_LIMIT;
    }
    else if (pong->ballVel[Y] < -SPEED_LIMIT)
    {
        pong->ballVel[Y] = -SPEED_LIMIT;
    }
}

/**
 * @brief TODO
 */
void DrawField(void)
{
    printf("%d, %d\n", pong->ballLoc[X], pong->ballLoc[Y]);

    clearPxTft();
    drawCircleFilled(pong->ballLoc[X], pong->ballLoc[Y], BALL_RADIUS, c555);
    drawRect(0, pong->paddleLocL, PADDLE_WIDTH, pong->paddleLocL + PADDLE_HEIGHT, c050);
    drawRect(TFT_WIDTH - PADDLE_WIDTH, pong->paddleLocR, TFT_WIDTH, pong->paddleLocR + PADDLE_HEIGHT, c005);
}

/**
 * @brief TODO
 *
 * @param p1ay
 * @param p2ay
 */
void ProcessInput(__attribute__((unused)) int32_t p1ax, int32_t p1ay, __attribute__((unused)) int8_t p1bl,
                  __attribute__((unused)) int8_t p1br, __attribute__((unused)) int8_t p1bu,
                  __attribute__((unused)) int8_t p1bd, __attribute__((unused)) int32_t p2ax, int32_t p2ay,
                  __attribute__((unused)) int8_t p2bl, __attribute__((unused)) int8_t p2br,
                  __attribute__((unused)) int8_t p2bu, __attribute__((unused)) int8_t p2bd)
{
    if (p1ay < 512 - DEAD_ZONE || 512 + DEAD_ZONE < p1ay)
    {
        pong->paddleLocL += ((512 - p1ay) * 3);
        if (pong->paddleLocL < 0)
        {
            pong->paddleLocL = 0;
        }
        else if (pong->paddleLocL > 12288)
        {
            pong->paddleLocL = 12288;
        }
    }

    if (p2ay < 512 - DEAD_ZONE || 512 + DEAD_ZONE < p2ay)
    {
        pong->paddleLocR += ((512 - p2ay) * 3);
        if (pong->paddleLocR < 0)
        {
            pong->paddleLocR = 0;
        }
        else if (pong->paddleLocR > 12288)
        {
            pong->paddleLocR = 12288;
        }
    }
}

/**
 * This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
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
 * @param status   The status of the transmission
 */
static void pongEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&pong->p2p, mac_addr, status);
}

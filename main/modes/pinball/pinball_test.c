//==============================================================================
// Includes
//==============================================================================

#include "pinball_test.h"
#include "pinball_zones.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Create balls with random positions and velocities
 *
 * @param p The pinball state
 * @param numBalls The number of balls to create
 */
void createRandomBalls(pinball_t* p, int32_t numBalls)
{
    // Don't overflow
    if (numBalls > MAX_NUM_BALLS)
    {
        numBalls = MAX_NUM_BALLS;
    }
    p->numBalls = 0;

    // Make some balls
    for (int32_t i = 0; i < numBalls; i++)
    {
        pbCircle_t* ball = &p->balls[p->numBalls++];
#define RAD 5
        ball->radius = TO_FX(RAD);
        ball->pos.x  = TO_FX((RAD + 1) + (esp_random() % (TFT_WIDTH - 2 * (RAD + 1))));
        ball->pos.y  = TO_FX((RAD + 1) + (esp_random() % (TFT_HEIGHT - 2 * (RAD + 1))));
#define MAX_VEL 128
        int32_t velX = ((-MAX_VEL / 2) + (esp_random() % MAX_VEL));
        ball->vel.x  = TO_FX_FRAC(velX * PIN_US_PER_FRAME, 1000000);
        int32_t velY = ((-MAX_VEL / 2) + (esp_random() % MAX_VEL));
        ball->vel.y  = TO_FX_FRAC(velY * PIN_US_PER_FRAME, 2000000);
        ball->color  = c500;
        ball->filled = true;
    }
}

/**
 * @brief Create random static walls
 *
 * @param p The pinballs state
 * @param numWalls The number of walls to create
 */
void createRandomWalls(pinball_t* p, int32_t numWalls)
{
    // Don't overflow
    if (numWalls > MAX_NUM_WALLS - 4)
    {
        numWalls = MAX_NUM_WALLS - 4;
    }
    p->numWalls = 0;

    // Always Create a boundary
    line_t corners[] = {
        {
            .p1 = {.x = TO_FX(0), .y = TO_FX(0)},
            .p2 = {.x = TO_FX(TFT_WIDTH - 1), .y = TO_FX(0)},
        },
        {
            .p1 = {.x = TO_FX(TFT_WIDTH - 1), .y = TO_FX(0)},
            .p2 = {.x = TO_FX(TFT_WIDTH - 1), .y = TO_FX(TFT_HEIGHT - 1)},
        },
        {
            .p1 = {.x = TO_FX(TFT_WIDTH - 1), .y = TO_FX(TFT_HEIGHT - 1)},
            .p2 = {.x = TO_FX(0), .y = TO_FX(TFT_HEIGHT - 1)},
        },
        {
            .p1 = {.x = TO_FX(0), .y = TO_FX(TFT_HEIGHT - 1)},
            .p2 = {.x = TO_FX(0), .y = TO_FX(0)},
        },
    };

    for (int32_t i = 0; i < ARRAY_SIZE(corners); i++)
    {
        pbLine_t* pbl = &p->walls[p->numWalls++];
        pbl->p1.x     = corners[i].p1.x;
        pbl->p1.y     = corners[i].p1.y;
        pbl->p2.x     = corners[i].p2.x;
        pbl->p2.y     = corners[i].p2.y;
        pbl->color    = c555;
        pbl->zoneMask = pinZoneLine(p, *pbl);
    }

    // Make a bunch of random lines
    for (int32_t nl = 0; nl < numWalls; nl++)
    {
        pbLine_t* pbl = &p->walls[p->numWalls++];

#define L_LEN 12

        pbl->p1.x  = TO_FX(L_LEN + (esp_random() % (TFT_WIDTH - (L_LEN * 2))));
        pbl->p1.y  = TO_FX(L_LEN + (esp_random() % (TFT_HEIGHT - (L_LEN * 2))));
        pbl->p2.x  = ADD_FX(pbl->p1.x, TO_FX((esp_random() % (L_LEN * 2)) - L_LEN));
        pbl->p2.y  = ADD_FX(pbl->p1.y, TO_FX((esp_random() % (L_LEN * 2)) - L_LEN));
        pbl->color = esp_random() % cTransparent;

        if (pbl->p1.x == pbl->p2.x && pbl->p1.y == pbl->p2.y)
        {
            if (esp_random() % 2)
            {
                pbl->p2.x = ADD_FX(pbl->p2.x, TO_FX(1));
            }
            else
            {
                pbl->p2.y = ADD_FX(pbl->p2.y, TO_FX(1));
            }
        }

        pbl->zoneMask = pinZoneLine(p, *pbl);
    }
}

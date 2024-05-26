//==============================================================================
// Includes
//==============================================================================

#include "pinball_test.h"
#include "pinball_zones.h"
#include "pinball_physics.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param p
 * @param x
 * @param y
 */
void pbCreateBall(pinball_t* p, q24_8 x, q24_8 y)
{
    pbCircle_t* ball = &p->balls[p->numBalls++];
#define BALL_RAD 5
    ball->radius = TO_FX(BALL_RAD);
    ball->pos.x  = x;
    ball->pos.y  = y;
#define MAX_VEL 128
    ball->vel.x   = 0;
    ball->vel.y   = 0;
    ball->accel_16.x = 0;
    ball->accel_16.y = 0x4000;
    ball->color   = c500;
    ball->filled  = true;
}

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
#define BALL_RAD 5
        ball->radius = TO_FX(BALL_RAD);
        ball->pos.x  = TO_FX((BALL_RAD + 1) + (esp_random() % (TFT_WIDTH - 2 * (BALL_RAD + 1))));
        ball->pos.y  = TO_FX((BALL_RAD + 1) + (esp_random() % (TFT_HEIGHT - 2 * (BALL_RAD + 1))));
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
 * @brief TODO
 *
 * @param p
 * @param numBumpers
 */
void createRandomBumpers(pinball_t* p, int32_t numBumpers)
{
    // Don't overflow
    if (numBumpers > MAX_NUM_BUMPERS)
    {
        numBumpers = MAX_NUM_BUMPERS;
    }
    p->numBumpers = 0;

    // Make some balls
    while (numBumpers > p->numBumpers)
    {
        pbCircle_t bumper = {0};
#define BUMPER_RAD 10
        bumper.radius   = TO_FX(BUMPER_RAD);
        bumper.pos.x    = TO_FX((BUMPER_RAD + 1) + (esp_random() % (TFT_WIDTH - 2 * (BUMPER_RAD + 1))));
        bumper.pos.y    = TO_FX((BUMPER_RAD + 1) + (esp_random() % (TFT_HEIGHT - 2 * (BUMPER_RAD + 1))));
        bumper.color    = c050;
        bumper.filled   = false;
        bumper.zoneMask = pinZoneCircle(p, bumper);

        bool intersection = false;
        for (int32_t ol = 0; ol < p->numWalls; ol++)
        {
            vec_t cv;
            if (circleLineIntersection(intCircle(bumper), intLine(p->walls[ol]), &cv))
            {
                intersection = true;
                break;
            }
        }

        for (int32_t ob = 0; ob < p->numBumpers; ob++)
        {
            if (circleCircleIntersection(intCircle(bumper), intCircle(p->bumpers[ob]), NULL))
            {
                intersection = true;
                break;
            }
        }

        if (!intersection)
        {
            memcpy(&p->bumpers[p->numBumpers], &bumper, sizeof(pbCircle_t));
            p->numBumpers++;
        }
    }
}

/**
 * @brief Create random static walls
 *
 * @param p The pinball state
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
        {
            .p1 = {.x = TO_FX(TFT_WIDTH/2), .y = TO_FX(40)},
            .p2 = {.x = TO_FX(TFT_WIDTH-1), .y = TO_FX(40 + TFT_HEIGHT/2)},
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
    while (numWalls > p->numWalls)
    {
        pbLine_t pbl = {0}; // = &p->walls[p->numWalls++];

#define L_LEN 12

        pbl.p1.x  = TO_FX(L_LEN + (esp_random() % (TFT_WIDTH - (L_LEN * 2))));
        pbl.p1.y  = TO_FX(L_LEN + (esp_random() % (TFT_HEIGHT - (L_LEN * 2))));
        pbl.p2.x  = ADD_FX(pbl.p1.x, TO_FX((esp_random() % (L_LEN * 2)) - L_LEN));
        pbl.p2.y  = ADD_FX(pbl.p1.y, TO_FX((esp_random() % (L_LEN * 2)) - L_LEN));
        pbl.color = c005; // esp_random() % cTransparent;

        if (pbl.p1.x == pbl.p2.x && pbl.p1.y == pbl.p2.y)
        {
            if (esp_random() % 2)
            {
                pbl.p2.x = ADD_FX(pbl.p2.x, TO_FX(1));
            }
            else
            {
                pbl.p2.y = ADD_FX(pbl.p2.y, TO_FX(1));
            }
        }

        pbl.zoneMask = pinZoneLine(p, pbl);

        bool intersection = false;
        for (int32_t ol = 0; ol < p->numWalls; ol++)
        {
            if (lineLineIntersection(intLine(pbl), intLine(p->walls[ol])))
            {
                intersection = true;
            }
        }

        for (int32_t ob = 0; ob < p->numBumpers; ob++)
        {
            vec_t cv;
            if (circleLineIntersection(intCircle(p->bumpers[ob]), intLine(pbl), &cv))
            {
                intersection = true;
            }
        }

        if (!intersection)
        {
            memcpy(&p->walls[p->numWalls], &pbl, sizeof(pbLine_t));
            p->numWalls++;
        }
    }
}

/**
 * @brief Create a Flipper
 *
 * @param p The pinball state
 * @param pivot_x
 * @param pivot_y
 * @param facingRight
 */
void createFlipper(pinball_t* p, int32_t pivot_x, int32_t pivot_y, bool facingRight)
{
    pbFlipper_t* f = &p->flippers[p->numFlippers];

    f->color         = c555;
    f->cPivot.pos.x  = pivot_x;
    f->cPivot.pos.y  = pivot_y;
    f->cPivot.radius = 10;
    f->length        = 40;
    f->cTip.radius   = 5;
    f->facingRight   = facingRight;
    // Update zone after setting position
    f->zoneMask = pinZoneFlipper(p, f);

    // Update angle and position after setting zone
    if (f->facingRight)
    {
        f->angle = 90 + FLIPPER_DOWN_ANGLE;
    }
    else
    {
        f->angle = 270 - FLIPPER_DOWN_ANGLE;
    }
    updateFlipperPos(f);

    // Update flipper count
    p->numFlippers++;
}

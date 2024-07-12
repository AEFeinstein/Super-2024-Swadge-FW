//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "pinball_test.h"
#include "pinball_zones.h"
#include "pinball_physics.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    circleFl_t cPivot;
    float tRadius;
    float len;
    bool facingRight;
} flipperFl_t;

//==============================================================================
// Const variables
//==============================================================================

static const lineFl_t constWalls[] = {
    {.p1 = {.x = 95, .y = 190}, .p2 = {.x = 0, .y = 116}},  {.p1 = {.x = 184, .y = 190}, .p2 = {.x = 279, .y = 116}},
    {.p1 = {.x = 0, .y = 0}, .p2 = {.x = 279, .y = 0}},     {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 0, .y = 0}},
    {.p1 = {.x = 279, .y = 0}, .p2 = {.x = 279, .y = 239}}, {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 279, .y = 239}},
    {.p1 = {.x = 0, .y = 116}, .p2 = {.x = 3, .y = 97}},    {.p1 = {.x = 3, .y = 97}, .p2 = {.x = 9, .y = 79}},
    {.p1 = {.x = 9, .y = 79}, .p2 = {.x = 20, .y = 58}},    {.p1 = {.x = 20, .y = 58}, .p2 = {.x = 37, .y = 37}},
    {.p1 = {.x = 37, .y = 37}, .p2 = {.x = 62, .y = 18}},   {.p1 = {.x = 62, .y = 18}, .p2 = {.x = 78, .y = 11}},
    {.p1 = {.x = 78, .y = 11}, .p2 = {.x = 96, .y = 5}},    {.p1 = {.x = 96, .y = 5}, .p2 = {.x = 116, .y = 1}},
    {.p1 = {.x = 116, .y = 1}, .p2 = {.x = 140, .y = 0}},   {.p1 = {.x = 140, .y = 0}, .p2 = {.x = 163, .y = 1}},
    {.p1 = {.x = 163, .y = 1}, .p2 = {.x = 183, .y = 5}},   {.p1 = {.x = 183, .y = 5}, .p2 = {.x = 201, .y = 11}},
    {.p1 = {.x = 201, .y = 11}, .p2 = {.x = 217, .y = 18}}, {.p1 = {.x = 217, .y = 18}, .p2 = {.x = 242, .y = 37}},
    {.p1 = {.x = 242, .y = 37}, .p2 = {.x = 259, .y = 58}}, {.p1 = {.x = 259, .y = 58}, .p2 = {.x = 270, .y = 79}},
    {.p1 = {.x = 270, .y = 79}, .p2 = {.x = 276, .y = 97}}, {.p1 = {.x = 276, .y = 97}, .p2 = {.x = 279, .y = 116}},
};

static const circleFl_t constBumpers[] = {
    {.pos = {.x = 140, .y = 62}, .radius = 19},
    {.pos = {.x = 182, .y = 105}, .radius = 19},
    {.pos = {.x = 97, .y = 105}, .radius = 19},
};

static const flipperFl_t constFlippers[] = {
    {.cPivot = {.pos = {.x = 90, .y = 200}, .radius = 10}, .tRadius = 5, .len = 35, .facingRight = true},
    {.cPivot = {.pos = {.x = 189, .y = 200}, .radius = 10}, .tRadius = 5, .len = 35, .facingRight = false},
};

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
void pbCreateBall(pinball_t* p, float x, float y)
{
    pbCircle_t* ball = &p->balls[p->numBalls++];
#define BALL_RAD 5
    ball->c.radius  = (BALL_RAD);
    ball->c.pos.x   = x;
    ball->c.pos.y   = y;
    ball->lastPos.x = x;
    ball->lastPos.y = y;
    // #define MAX_VEL 128
    ball->vel.x   = 0;
    ball->vel.y   = 0;
    ball->accel.x = 0;
    ball->accel.y = PINBALL_GRAVITY;
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
        ball->c.radius  = (BALL_RAD);
        ball->c.pos.x   = ((BALL_RAD + 1) + (esp_random() % (TFT_WIDTH - 2 * (BALL_RAD + 1))));
        ball->c.pos.y   = ((BALL_RAD + 1) + (esp_random() % (TFT_HEIGHT - 2 * (BALL_RAD + 1))));
        ball->lastPos.x = ball->c.pos.x;
        ball->lastPos.y = ball->c.pos.x;
        ball->vel.x     = 0;
        ball->vel.y     = 5 / 60.0f;
        ball->accel.x   = 0;
        ball->accel.y   = PINBALL_GRAVITY;
        ball->color     = esp_random() % cTransparent;
        ball->filled    = true;
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
    int fixedBumpersPlaced = 0;
    circleFl_t bumpers[]  = {};

    numBumpers += ARRAY_SIZE(bumpers);

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
        bumper.c.radius = (BUMPER_RAD);
        if (fixedBumpersPlaced < ARRAY_SIZE(bumpers))
        {
            bumper.c = bumpers[fixedBumpersPlaced];
            fixedBumpersPlaced++;
        }
        else
        {
            bumper.c.pos.x = ((BUMPER_RAD + 1) + (esp_random() % (TFT_WIDTH - 2 * (BUMPER_RAD + 1))));
            bumper.c.pos.y = ((BUMPER_RAD + 1) + (esp_random() % (TFT_HEIGHT - 2 * (BUMPER_RAD + 1))));
        }
        bumper.color    = c050;
        bumper.filled   = false;
        bumper.zoneMask = pinZoneCircle(p, bumper);

        bool intersection = false;
        for (int32_t ol = 0; ol < p->numWalls; ol++)
        {
            if (circleLineFlIntersection(bumper.c, p->walls[ol].l, true, NULL, NULL))
            {
                intersection = true;
                break;
            }
        }

        for (int32_t ob = 0; ob < p->numBumpers; ob++)
        {
            if (circleCircleFlIntersection(bumper.c, p->bumpers[ob].c, NULL, NULL))
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
    // Always Create a boundary
    lineFl_t walls[] = {
        {.p1 = {.x = 0, .y = 0}, .p2 = {.x = 279, .y = 0}},
        {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 0, .y = 0}},
        {.p1 = {.x = 279, .y = 0}, .p2 = {.x = 279, .y = 239}},
        {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 279, .y = 239}},
    };

    // Don't overflow
    if (numWalls > MAX_NUM_WALLS - ARRAY_SIZE(walls))
    {
        numWalls = MAX_NUM_WALLS - ARRAY_SIZE(walls);
    }
    p->numWalls = 0;

    for (int32_t i = 0; i < ARRAY_SIZE(walls); i++)
    {
        createWall(p, walls[i].p1.x, walls[i].p1.y, walls[i].p2.x, walls[i].p2.y);
    }

    // Make a bunch of random lines
    while (numWalls > p->numWalls)
    {
        pbLine_t pbl = {0}; // = &p->walls[p->numWalls++];

#define L_LEN 12

        pbl.l.p1.x    = (L_LEN + (esp_random() % (TFT_WIDTH - (L_LEN * 2))));
        pbl.l.p1.y    = (L_LEN + (esp_random() % (TFT_HEIGHT - (L_LEN * 2))));
        pbl.l.p2.x    = pbl.l.p1.x + ((esp_random() % (L_LEN * 2)) - L_LEN);
        pbl.l.p2.y    = pbl.l.p1.y + ((esp_random() % (L_LEN * 2)) - L_LEN);
        vecFl_t delta = {
            .x = pbl.l.p2.x - pbl.l.p1.x,
            .y = pbl.l.p2.y - pbl.l.p1.y,
        };
        pbl.length = magVecFl2d(delta);
        pbl.color  = c005; // esp_random() % cTransparent;

        if (pbl.l.p1.x == pbl.l.p2.x && pbl.l.p1.y == pbl.l.p2.y)
        {
            if (esp_random() % 2)
            {
                pbl.l.p2.x = (pbl.l.p2.x) + ((1));
            }
            else
            {
                pbl.l.p2.y = (pbl.l.p2.y) + ((1));
            }
        }

        pbl.zoneMask = pinZoneLine(p, pbl);

        bool intersection = false;
        for (int32_t ol = 0; ol < p->numWalls; ol++)
        {
            if (lineLineFlIntersection(pbl.l, p->walls[ol].l))
            {
                intersection = true;
            }
        }

        for (int32_t ob = 0; ob < p->numBumpers; ob++)
        {
            if (circleLineFlIntersection(p->bumpers[ob].c, pbl.l, true, NULL, NULL))
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
 * @brief Create a Wall
 *
 * @param p
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 */
void createWall(pinball_t* p, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    // Don't overflow
    if (p->numWalls + 1 < MAX_NUM_WALLS)
    {
        pbLine_t* pbl = &p->walls[p->numWalls++];
        pbl->l.p1.x   = x1;
        pbl->l.p1.y   = y1;
        pbl->l.p2.x   = x2;
        pbl->l.p2.y   = y2;
        vecFl_t delta = {
            .x = pbl->l.p2.x - pbl->l.p1.x,
            .y = pbl->l.p2.y - pbl->l.p1.y,
        };
        pbl->length   = magVecFl2d(delta);
        pbl->color    = c555;
        pbl->zoneMask = pinZoneLine(p, *pbl);
    }
}

/**
 * @brief Create a Bumper
 *
 * @param p
 * @param x
 * @param y
 * @param r
 */
void createBumper(pinball_t* p, int32_t x, int32_t y, int32_t r)
{
    // Don't overflow
    if (p->numBumpers + 1 < MAX_NUM_BUMPERS)
    {
        pbCircle_t* b = &p->bumpers[p->numBumpers];

        b->c.radius = r;
        b->c.pos.x  = x;
        b->c.pos.y  = y;
        b->color    = c050;
        b->filled   = false;
        b->zoneMask = pinZoneCircle(p, *b);

        p->numBumpers++;
    }
}

/**
 * @brief Create a Flipper
 *
 * @param p The pinball state
 * @param pivot_x
 * @param pivot_y
 * @param pivot_r
 * @param tip_r
 * @param len
 * @param facingRight
 */
void createFlipper(pinball_t* p, int32_t pivot_x, int32_t pivot_y, int32_t pivot_r, int32_t tip_r, int32_t len,
                   bool facingRight)
{
    // Don't overflow
    if (p->numFlippers + 1 < MAX_NUM_FLIPPERS)
    {
        pbFlipper_t* f = &p->flippers[p->numFlippers];

        f->cPivot.color = c505;
        f->cTip.color   = c505;
        f->sideL.color  = c505;
        f->sideR.color  = c505;

        f->cPivot.c.pos.x  = pivot_x;
        f->cPivot.c.pos.y  = pivot_y;
        f->cPivot.c.radius = pivot_r;
        f->length          = len;
        f->cTip.c.radius   = tip_r;
        f->facingRight     = facingRight;

        f->zoneMask = pinZoneFlipper(p, f);

        // Update angle and position after setting zone
        if (f->facingRight)
        {
            f->angle = M_PI_2 + FLIPPER_DOWN_ANGLE;
        }
        else
        {
            f->angle = M_PI + M_PI_2 - FLIPPER_DOWN_ANGLE;
        }
        updateFlipperPos(f);

        // Update flipper count
        p->numFlippers++;
    }
}

/**
 * @brief Load a pinball table from const data
 *
 * @param p The game state to load into
 */
void loadConstPbTable(pinball_t* p)
{
    for (int32_t idx = 0; idx < ARRAY_SIZE(constWalls); idx++)
    {
        const lineFl_t* w = &constWalls[idx];
        createWall(p, w->p1.x, w->p1.y, w->p2.x, w->p2.y);
    }

    for (int32_t idx = 0; idx < ARRAY_SIZE(constBumpers); idx++)
    {
        const circleFl_t* b = &constBumpers[idx];
        createBumper(p, b->pos.x, b->pos.y, b->radius);
    }

    for (int32_t idx = 0; idx < ARRAY_SIZE(constFlippers); idx++)
    {
        const flipperFl_t* f = &constFlippers[idx];
        createFlipper(p, f->cPivot.pos.x, f->cPivot.pos.y, f->cPivot.radius, f->tRadius, f->len, f->facingRight);
    }
}

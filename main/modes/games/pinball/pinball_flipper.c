#include <math.h>

#include "macros.h"
#include "shapes.h"

#include "pinball_flipper.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readFlipperFromFile(uint8_t* tableData, jsScene_t* scene)
{
    jsFlipper_t* flipper = &scene->flippers[scene->numFlippers++];
    uint32_t dIdx        = 0;

    flipper->pos.x       = readInt16(tableData, &dIdx);
    flipper->pos.y       = readInt16(tableData, &dIdx);
    flipper->radius      = readInt8(tableData, &dIdx);
    flipper->length      = readInt8(tableData, &dIdx);
    flipper->facingRight = readInt8(tableData, &dIdx) != 0;

    flipper->maxRotation     = 1.0f;
    flipper->restAngle       = 0.5f;
    flipper->angularVelocity = 10.0f;

    if (!flipper->facingRight)
    {
        flipper->restAngle   = M_PI - flipper->restAngle;
        flipper->maxRotation = -flipper->maxRotation;
    }
    flipper->sign        = (flipper->maxRotation >= 0) ? -1 : 1;
    flipper->maxRotation = ABS(flipper->maxRotation);

    // changing
    flipper->rotation               = 0;
    flipper->currentAngularVelocity = 0;
    flipper->buttonHeld             = false;

    return dIdx;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @param dt
 */
void jsFlipperSimulate(jsFlipper_t* flipper, float dt)
{
    float prevRotation = flipper->rotation;

    if (flipper->buttonHeld)
    {
        flipper->rotation = flipper->rotation + dt * flipper->angularVelocity;
        if (flipper->rotation > flipper->maxRotation)
        {
            flipper->rotation = flipper->maxRotation;
        }
    }
    else
    {
        flipper->rotation = flipper->rotation - dt * flipper->angularVelocity;
        if (flipper->rotation < 0)
        {
            flipper->rotation = 0;
        }
    }
    flipper->currentAngularVelocity = flipper->sign * (flipper->rotation - prevRotation) / dt;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @return vecFl_t
 */
vecFl_t jsFlipperGetTip(jsFlipper_t* flipper)
{
    float angle = flipper->restAngle + flipper->sign * flipper->rotation;
    vecFl_t dir = {.x = cosf(angle), .y = sinf(angle)};
    return addVecFl2d(flipper->pos, mulVecFl2d(dir, flipper->length));
}

/**
 * @brief TODO doc
 *
 * @param flipper
 */
void pinballDrawFlipper(jsFlipper_t* flipper, vec_t* cameraOffset)
{
    vecFl_t pos = {
        .x = flipper->pos.x - cameraOffset->x,
        .y = flipper->pos.y - cameraOffset->y,
    };
    drawCircleFilled(pos.x, pos.y, flipper->radius, c115);
    vecFl_t tip = jsFlipperGetTip(flipper);
    tip.x -= cameraOffset->x;
    tip.y -= cameraOffset->y;
    drawCircleFilled(tip.x, tip.y, flipper->radius, c115);
    drawLine(pos.x, pos.y + flipper->radius, tip.x, tip.y + flipper->radius, c115, 0);
    drawLine(pos.x, pos.y - flipper->radius, tip.x, tip.y - flipper->radius, c115, 0);
}

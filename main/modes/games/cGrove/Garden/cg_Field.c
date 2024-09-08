/**
 * @file cg_Field.c
 * @author your name (you@domain.com)
 * @brief The field of play for the main garden area
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "cg_Field.h"
#include "geometry.h"

//==============================================================================
// Static Functions
//==============================================================================

/**
 * @brief Draws the background grass color.
 * 
 * @param grassColor Color of the grass. Possible to change for seasons?
 */
static void _cgDrawLawn(paletteColor_t grassColor)
{
    // Draw green background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, grassColor);
    // TODO: Draw random blades of grass
}

/**
 * @brief Initialized a static background object
 * 
 * @param cg Game Object
 * @param spr sprite of the object
 * @param x x coordinate
 * @param y y coordinate
 * @param idx Index of object in array
 */
static void _cgInitObject(cGrove_t* cg, wsg_t spr, int16_t x, int16_t y, int8_t idx)
{
    cg->garden.field.staticObjects[idx].spr        = spr;
    cg->garden.field.staticObjects[idx].aabb.pos.x = x;
    cg->garden.field.staticObjects[idx].aabb.pos.y = y;
    cg->garden.field.staticObjects[idx].aabb.height = spr.h;
    cg->garden.field.staticObjects[idx].aabb.width = spr.w;
}

/**
 * @brief Draws a background object
 * 
 * @param cg Game Object
 * @param obj index of object
 */
static void _cgDrawObject(cGrove_t* cg, int8_t obj)
{
    int16_t xOffset = cg->garden.field.staticObjects[obj].aabb.pos.x - cg->garden.field.cam.aabb.pos.x;
    int16_t yOffset = cg->garden.field.staticObjects[obj].aabb.pos.y - cg->garden.field.cam.aabb.pos.y;
    drawWsgSimple(&cg->garden.field.staticObjects[obj].spr, xOffset, yOffset);
}

//==============================================================================
// Functions
//==============================================================================

void cgInitField(cGrove_t* cg)
{
    // Init camera
    cg->garden.field.cam.aabb.height = TFT_HEIGHT;
    cg->garden.field.cam.aabb.width  = TFT_WIDTH;
    cg->garden.field.cam.aabb.pos.x  = 32;
    cg->garden.field.cam.aabb.pos.y  = 32;

    // Init objects
    _cgInitObject(cg, cg->gardenSpr[0], 108, 88, 0);
    _cgInitObject(cg, cg->gardenSpr[0], 280, 240, 1);
}

void cgMoveCamera(cGrove_t* cg, int16_t xChange, int16_t yChange)
{
    cg->garden.field.cam.aabb.pos.x += xChange;
    cg->garden.field.cam.aabb.pos.y += yChange;
    if (cg->garden.field.cam.aabb.pos.x < 0)
    {
        cg->garden.field.cam.aabb.pos.x = 0;
    } else if (cg->garden.field.cam.aabb.pos.x > CG_FIELD_WIDTH - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.width))
    {
        cg->garden.field.cam.aabb.pos.x = CG_FIELD_WIDTH - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.width);
    }
    if (cg->garden.field.cam.aabb.pos.y < 0)
    {
        cg->garden.field.cam.aabb.pos.y = 0;
    } else if (cg->garden.field.cam.aabb.pos.y > CG_FIELD_HEIGHT - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.height))
    {
        cg->garden.field.cam.aabb.pos.x = CG_FIELD_HEIGHT - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.height);
    }
}

void cgDrawField(cGrove_t* cg)
{
    rectangle_t* camAABB = &cg->garden.field.cam.aabb;
    _cgDrawLawn(c031);

    // Draw pond
    // TODO: Make pond
    
    // Draw static objects
    vec_t temp;
    for (int8_t obj = 0; obj < CG_OBJ_COUNT; obj++)
    {
        if (rectRectIntersection(*camAABB, cg->garden.field.staticObjects[obj].aabb, &temp))
            _cgDrawObject(cg, obj);
    }

    // Draw boundary for debug
    // FIXME: Make so it only draws in debug mode
    drawRect(CG_FIELD_BOUNDARY, CG_FIELD_BOUNDARY, TFT_WIDTH - CG_FIELD_BOUNDARY, TFT_HEIGHT - CG_FIELD_BOUNDARY, c500);
}

/**
 * @file cg_GardenItems.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Interactive items inside of the main garden
 * @version 0.1
 * @date 2024-09-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Items.h"

//==============================================================================
// Functions
//==============================================================================

void cgInitItem(cGrove_t* cg, int8_t idx, char* name, wsg_t spr, vec_t pos)
{
    strcpy(cg->garden.items[idx].name, name);
    cg->garden.items[idx].spr = spr;
    cg->garden.items[idx].aabb.pos = pos;
    cg->garden.items[idx].aabb.height = spr.h;
    cg->garden.items[idx].aabb.width = spr.w;
    cg->garden.items[idx].active = true;
}

void cgDeactivateItem(cGrove_t* cg, int8_t idx)
{
    cg->garden.items[idx].active = false;
}

void cgDrawItem(cGrove_t* cg, int8_t idx)
{
    int16_t xOffset = cg->garden.items[idx].aabb.pos.x - cg->garden.field.cam.aabb.pos.x;
    int16_t yOffset = cg->garden.items[idx].aabb.pos.y - cg->garden.field.cam.aabb.pos.y;
    drawWsgSimple(&cg->garden.items[idx].spr, xOffset, yOffset);
    drawText(&cg->menuFont, c555, cg->garden.items[idx].name, xOffset, yOffset - 16);
}
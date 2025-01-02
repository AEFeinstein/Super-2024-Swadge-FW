/**
 * @file cg_GardenItems.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Interactive items inside of the main garden
 * @version 1.0
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
    strcpy(cg->grove.items[idx].name, name);
    cg->grove.items[idx].spr         = spr;
    cg->grove.items[idx].aabb.pos    = pos;
    cg->grove.items[idx].aabb.height = spr.h;
    cg->grove.items[idx].aabb.width  = spr.w;
    cg->grove.items[idx].active      = true;
}

void cgDeactivateItem(cGrove_t* cg, int8_t idx)
{
    cg->grove.items[idx].active = false;
}
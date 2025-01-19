//==============================================================================
// Includes
//==============================================================================

#include "mode_bigbug.h"
#include "tilemap_bigbug.h"
// #include "entityManager_bigbug.h"
// #include "entity_bigbug.h"
#include "worldGen_bigbug.h"
#include "random_bigbug.h"
#include "typedef_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
void bb_generateWorld(bb_tilemap_t* tilemap, int8_t* oldBoosterYs)
{
    // There are 6 handcrafted levels that get chosen randomly.
    uint8_t level = bb_randomInt(0, 5);
    wsg_t levelWsg; ///< A graphic representing the level data where tiles are pixels.

    char wsg_name[13];
    snprintf(wsg_name, sizeof(wsg_name), "level%d.wsg", level);
    loadWsgInplace(wsg_name, &levelWsg, true, bb_decodeSpace,
                   bb_hsd); // levelWsg only needed for this brief scope.

    int8_t midgroundHealthValues[] = {1, 4, 10};

    // keep track of the washing machine spawns. Don't allow them at the same x position or at positions 34, 37, 40
    // (booster positions).
    uint8_t washingMachineXPositions[35] = {0};

    // Set all the tiles
    for (int i = 0; i < TILE_FIELD_WIDTH; i++)
    {
        for (int j = 0; j < TILE_FIELD_HEIGHT; j++)
        {
            tilemap->fgTiles[i][j].pos    = i | (j << 7) | (1 << 15);
            tilemap->mgTiles[i][j].pos    = i | (j << 7); // z is implicitly zero
            tilemap->fgTiles[i][j].embed  = NOTHING_EMBED;
            tilemap->fgTiles[i][j].entity = NULL;
            tilemap->fgTiles[i][j].gCost  = 0;
            tilemap->mgTiles[i][j].gCost  = 0;
            tilemap->fgTiles[i][j].hCost  = 0;
            tilemap->mgTiles[i][j].hCost  = 0;

            uint32_t rgbCol = paletteToRGB(levelWsg.px[(j * levelWsg.w) + i]);

            // red value used for foreground tiles
            switch ((rgbCol >> 16) & 255)
            {
                case 102:
                {
                    tilemap->fgTiles[i][j].health = 1;
                    break;
                }
                case 153:
                {
                    tilemap->fgTiles[i][j].health = 4;
                    break;
                }
                case 204:
                {
                    tilemap->fgTiles[i][j].health = 10;
                    break;
                }
                case 255:
                {
                    tilemap->fgTiles[i][j].health = 100;
                    break;
                }
                default: // case 0
                {
                    tilemap->fgTiles[i][j].health = 0;
                    // blue value used for washing machines, cars, swadges/donuts
                    switch (rgbCol & 255)
                    {
                        case 51:
                        {
                            tilemap->fgTiles[i][j].embed
                                = bb_randomInt(0, 1) == 0 ? BB_CAR_WITH_DONUT_EMBED : BB_CAR_WITH_SWADGE_EMBED;
                            break;
                        }
                        case 102:
                        {
                            tilemap->fgTiles[i][j].embed = bb_randomInt(0, 1) == 0 ? BB_FOOD_CART_WITH_DONUT_EMBED
                                                                                   : BB_FOOD_CART_WITH_SWADGE_EMBED;
                            break;
                        }
                        case 153:
                        {
                            if (i != 34 && i != 37 && i != 40)
                            {
                                for (int lookupIdx = 0; lookupIdx < 35; lookupIdx++)
                                {
                                    if (washingMachineXPositions[lookupIdx] == i)
                                    {
                                        break;
                                    }
                                    else if (washingMachineXPositions[lookupIdx] == 0)
                                    {
                                        washingMachineXPositions[lookupIdx] = i;
                                        tilemap->fgTiles[i][j].embed        = WASHING_MACHINE_EMBED;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                    break;
                }
            }

            // green value used for midground tiles and doors and final boss
            switch ((rgbCol >> 8) & 255)
            {
                case 51:
                {
                    tilemap->mgTiles[i][j].health = tilemap->fgTiles[i][j].health == 0
                                                        ? midgroundHealthValues[bb_randomInt(0, 2)]
                                                        : tilemap->fgTiles[i][j].health;
                    break;
                }
                case 102:
                {
                    tilemap->mgTiles[i][j].health = 0;
                    tilemap->fgTiles[i][j].embed  = DOOR_EMBED;
                    break;
                }
                case 153:
                {
                    tilemap->mgTiles[i][j].health = 0;
                    tilemap->fgTiles[i][j].embed  = FINAL_BOSS_EMBED;
                    break;
                }
                default:
                {
                    tilemap->mgTiles[i][j].health = 0;
                    break;
                }
            }

            // blue channel is also for enemy density where there are foreground tiles.
            if (tilemap->fgTiles[i][j].health > 0)
            {
                if (bb_randomInt(0, 999) < 15 && i > 20 && i < 52)
                {
                    tilemap->fgTiles[i][j].embed = SKELETON_EMBED;
                }
                else if (bb_randomInt(0, 99) < (((rgbCol & 255) / 51) * 15))
                {
                    tilemap->fgTiles[i][j].embed = EGG_EMBED;
                }
            }
        }
    }

    tilemap->fgTiles[TILE_FIELD_WIDTH / 2 - 7][0].embed = EGG_EMBED; // tutorial egg

    if (level == 3)
    {
        tilemap->fgTiles[53][27].embed = BRICK_TUTORIAL_EMBED;
    }
    else if (level == 4)
    {
        tilemap->fgTiles[20][8].embed = BRICK_TUTORIAL_EMBED;
    }

    // carve out three tiles where any old boosters are buried
    for (int booster = 0; booster < 3; booster++)
    {
        if (oldBoosterYs[booster] > -1)
        {
            for (int carveY = oldBoosterYs[booster] - 1; carveY <= oldBoosterYs[booster] + 1; carveY++)
            {
                if (carveY >= 0)
                {
                    tilemap->fgTiles[34 + booster * 3][carveY].health = 0;
                    tilemap->fgTiles[35 + booster * 3][carveY].embed  = NOTHING_EMBED;
                }
            }
        }
    }

    freeWsg(&levelWsg);
}
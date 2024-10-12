//==============================================================================
// Includes
//==============================================================================
#include "tilemap_bigbug.h"
#include "entityManager_bigbug.h"
#include "entity_bigbug.h"
#include "worldGen_bigbug.h"
#include "random_bigbug.h"
#include "typedef_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEggs(bb_entityManager_t* entityManager, bb_tilemap_t* tilemap)
{
    printf("initializing eggs\n");

    for (int x = 0; x < TILE_FIELD_WIDTH; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            if (bb_randomInt(0, 1) == 0) //%50 chance
            {
                printf("egg at i:%d j:%d\n", x, y);
                tilemap->fgTiles[x][y].embed = EGG_EMBED;
                
                // bb_entity_t* eggLeaves = bb_createEntity(entityManager, NO_ANIMATION, true, EGG_LEAVES, 1,
                //                                          x * TILE_SIZE + HALF_TILE, y * TILE_SIZE + HALF_TILE);
                // if(eggLeaves == NULL)
                // {
                //     //no more room in the EntityManager. :(
                //     return;
                // }
                // ((bb_eggLeavesData_t*)eggLeaves->data)->egg = bb_createEntity(
                //     entityManager, NO_ANIMATION, true, EGG, 1, x * TILE_SIZE + HALF_TILE, y * TILE_SIZE + HALF_TILE);
                // if(((bb_eggLeavesData_t*)eggLeaves->data)->egg == NULL)
                // {
                //     bb_destroyEntity(eggLeaves, false);
                // }
            }
        
            else
            {
                tilemap->fgTiles[x][y].embed = NOTHING_EMBED;
            }
        }
    }
}
//==============================================================================
// Includes
//==============================================================================
#include "tilemap_bigbug.h"
#include "entityManager_bigbug.h"
#include "entity_bigbug.h"
#include "esp_random.h"
#include "worldGen_bigbug.h"

//==============================================================================
// Functions
//==============================================================================

int bb_randomInt(int lowerBound, int upperBound)
{
    return esp_random() % (upperBound - lowerBound + 1) + lowerBound;
}

void bb_initializeEggs(bb_entityManager_t* entityManager, bb_tilemap_t* tilemap)
{
    printf("initializing eggs\n");

    for (int x = 0; x < TILE_FIELD_WIDTH; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            if (bb_randomInt(0, 9) == 0) //%10 chance
            {
                printf("egg at i:%d j:%d\n", x, y);
                bb_entity_t* eggLeaves = bb_createEntity(entityManager, NO_ANIMATION, true, EGG_LEAVES, 1,
                                                         x * TILE_SIZE + HALF_TILE, y * TILE_SIZE + HALF_TILE);
                ((bb_eggLeavesData_t*)eggLeaves->data)->egg = bb_createEntity(
                    entityManager, NO_ANIMATION, true, EGG, 1, x * TILE_SIZE + HALF_TILE, y * TILE_SIZE + HALF_TILE);
            }
        }
    }
}
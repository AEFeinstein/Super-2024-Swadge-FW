//==============================================================================
// Includes
//==============================================================================
#include <color_utils.h>
#include "mode_bigbug.h"
#include "typedef_bigbug.h"
#include "tilemap_bigbug.h"
#include "random_bigbug.h"
#include "entity_bigbug.h"
#include "lighting_bigbug.h"

//==============================================================================
// Function Prototypes
//==============================================================================

//==============================================================================
// Functions
//==============================================================================
// static inline function to get bits 0-6 of pos
static inline uint8_t getX(const bb_midgroundTileInfo_t* tile)
{
    return tile->pos & 0x7F;
}

// static inline function to get bits 7-14 of pos
static inline uint8_t getY(const bb_midgroundTileInfo_t* tile)
{
    return (tile->pos >> 7) & 0xFF;
}

static inline uint16_t fCost(const bb_midgroundTileInfo_t* tile)
{
    return tile->gCost + tile->hCost;
}

void bb_loadWsgs(bb_tilemap_t* tilemap)
{
    if (false == tilemap->wsgsLoaded)
    {
        loadWsgInplace("headlampLookup.wsg", &tilemap->headlampWsg, true, bb_decodeSpace, bb_hsd); // 122 x 107 pixels

        loadWsgInplace("baked_Landfill2.wsg", &tilemap->surface1Wsg, true, bb_decodeSpace, bb_hsd);
        loadWsgInplace("baked_Landfill3.wsg", &tilemap->surface2Wsg, true, bb_decodeSpace, bb_hsd);
        loadWsgInplace("landfill_gradient.wsg", &tilemap->landfillGradient, true, bb_decodeSpace, bb_hsd);

        // TILE MAP shenanigans explained:
        // neigbhbors in LURD order (Left, Up, Down, Right) 1 if dirt, 0 if not
        // bin  dec  wsg
        // LURD
        // 0010 2    0
        // 1010 10   1
        // 1000 8    2
        // 0000 0    3

        // 0011 3    4
        // 1011 11   5
        // 1001 9    6
        // 0001 1    7

        // 0111 7    8
        // 1111 15   9
        // 1101 13   10
        // 0101 5    11

        // 0110 6    12
        // 1110 14   13
        // 1100 12   14
        // 0100 4    15

        // The index of bigbug->fore_s_Wsg is the LURD neighbor info.
        // The value within is the wsg graphic.
        // [3,7,0,4,15,11,12,8,2,6,1,5,14,10,13,9]

        // Midground
        for (int16_t i = 0; i < 120; i++)
        {
            char filename[20];
            snprintf(filename, sizeof(filename), "mid_s_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->mid_s_Wsg[i], true, bb_decodeSpace, bb_hsd);

            snprintf(filename, sizeof(filename), "mid_m_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->mid_m_Wsg[i], true, bb_decodeSpace, bb_hsd);

            snprintf(filename, sizeof(filename), "mid_h_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->mid_h_Wsg[i], true, bb_decodeSpace, bb_hsd);
        }

        // Foreground
        for (int16_t i = 0; i < 240; i++)
        {
            char filename[20];
            snprintf(filename, sizeof(filename), "fore_s_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->fore_s_Wsg[i], true, bb_decodeSpace, bb_hsd);

            snprintf(filename, sizeof(filename), "fore_m_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->fore_m_Wsg[i], true, bb_decodeSpace, bb_hsd);

            snprintf(filename, sizeof(filename), "fore_h_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->fore_h_Wsg[i], true, bb_decodeSpace, bb_hsd);

            snprintf(filename, sizeof(filename), "fore_b_%d.wsg", i);
            loadWsgInplace(filename, &tilemap->fore_b_Wsg[i], true, bb_decodeSpace, bb_hsd);
        }

        tilemap->wsgsLoaded = true;
    }
}

void bb_freeWsgs(bb_tilemap_t* tilemap)
{
    if (true == tilemap->wsgsLoaded)
    {
        freeWsg(&tilemap->headlampWsg); // 122 x 107 pixels

        freeWsg(&tilemap->surface1Wsg);
        freeWsg(&tilemap->surface2Wsg);
        freeWsg(&tilemap->landfillGradient);

        // Midground
        for (int16_t i = 0; i < 120; i++)
        {
            freeWsg(&tilemap->mid_s_Wsg[i]);
            freeWsg(&tilemap->mid_m_Wsg[i]);
            freeWsg(&tilemap->mid_h_Wsg[i]);
        }

        // Foreground
        for (int16_t i = 0; i < 240; i++)
        {
            freeWsg(&tilemap->fore_s_Wsg[i]);
            freeWsg(&tilemap->fore_m_Wsg[i]);
            freeWsg(&tilemap->fore_h_Wsg[i]);
            freeWsg(&tilemap->fore_b_Wsg[i]);
        }
        tilemap->wsgsLoaded = false;
    }
}

// flags neighbors to check for structural support
void flagNeighbors(const bb_midgroundTileInfo_t* tile, bb_gameData_t* gameData)
{
    uint8_t* left = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    left[0]       = getX(tile) - 1;
    left[1]       = getY(tile);
    left[2]       = 1;
    push(&gameData->pleaseCheck, (void*)left);

    if (getY(tile) > 0)
    {
        uint8_t* up = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
        up[0]       = getX(tile);
        up[1]       = getY(tile) - 1;
        up[2]       = 1;
        push(&gameData->pleaseCheck, (void*)up);
    }

    uint8_t* right = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    right[0]       = getX(tile) + 1;
    right[1]       = getY(tile);
    right[2]       = 1;
    push(&gameData->pleaseCheck, (void*)right);

    if (getY(tile) < TILE_FIELD_HEIGHT)
    {
        uint8_t* down = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
        down[0]       = getX(tile);
        down[1]       = getY(tile) + 1;
        down[2]       = 1;
        push(&gameData->pleaseCheck, (void*)down);
    }

    uint8_t* midground = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    midground[0]       = getX(tile);
    midground[1]       = getY(tile);
    midground[2]       = 0;
    push(&gameData->pleaseCheck, (void*)midground);
}

void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera, vec_t* garbotnikDrawPos, vec_t* garbotnikRotation,
                    bb_entityManager_t* entityManager)
{
    // setting up variables to draw midground & foreground
    // ESP_LOGD(BB_TAG,"camera x: %d\n", (bigbug->camera.pos.x >> DECIMAL_BITS));
    // ESP_LOGD(BB_TAG,"width: %d\n", FIELD_WIDTH);
    int16_t iStart = camera->pos.x / TILE_SIZE;
    int16_t iEnd   = iStart + TFT_WIDTH / TILE_SIZE + 1;
    int16_t jStart = camera->pos.y / TILE_SIZE;
    int16_t jEnd   = jStart + TFT_HEIGHT / TILE_SIZE + 1;

    iStart -= (camera->pos.x < 0);
    iEnd -= (camera->pos.x < 0);
    if (camera->pos.x + FIELD_WIDTH < 0)
    {
        iEnd = -1;
    }

    jStart -= (camera->pos.y < 0);
    jEnd -= (camera->pos.y < 0);
    if (camera->pos.y + FIELD_HEIGHT < 0)
    {
        jEnd = -1;
    }

    if (iEnd >= 0 && iStart < TILE_FIELD_WIDTH && jEnd >= 0 && jStart < TILE_FIELD_HEIGHT)
    {
        if (0 > iStart)
        {
            iStart = 0;
        }
        if (TILE_FIELD_WIDTH - 1 < iEnd)
        {
            iEnd = TILE_FIELD_WIDTH - 1;
        }
        if (0 > jStart)
        {
            jStart = 0;
        }
        if (TILE_FIELD_HEIGHT - 1 < jEnd)
        {
            jEnd = TILE_FIELD_HEIGHT - 1;
        }

        // ESP_LOGD(BB_TAG,"i: %d-%d j:%d-%d\n", iStart, iEnd, jStart, jEnd);
        // ESP_LOGD(BB_TAG,"x tile count: %d y tile count: %d\n", iEnd-iStart, jEnd-jStart);

        int32_t brightness;

        for (int32_t i = iStart; i <= iEnd; i++)
        {
            for (int32_t j = jStart; j <= jEnd; j++)
            {
                // Hijacking this i j double for loop to load entities within the camera bounds before drawing tiles.
                if (tilemap->fgTiles[i][j].embed != NOTHING_EMBED && tilemap->fgTiles[i][j].entity == NULL)
                {
                    switch (tilemap->fgTiles[i][j].embed)
                    {
                        case EGG_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 2);
                            bb_entity_t* eggLeaves
                                = bb_createEntity(entityManager, NO_ANIMATION, true, EGG_LEAVES, 1,
                                                  i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false);
                            if (eggLeaves != NULL)
                            {
                                ((bb_eggLeavesData_t*)eggLeaves->data)->egg = bb_createEntity(
                                    entityManager, NO_ANIMATION, true, EGG, 1, i * TILE_SIZE + HALF_TILE,
                                    j * TILE_SIZE + HALF_TILE, false, false);
                                if (((bb_eggLeavesData_t*)eggLeaves->data)->egg == NULL)
                                {
                                    bb_destroyEntity(eggLeaves, false, true);
                                }
                                else
                                {
                                    tilemap->fgTiles[i][j].entity = eggLeaves;
                                }
                            }
                            break;
                        }
                        case SKELETON_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            bb_entity_t* skeleton
                                = bb_createEntity(entityManager, NO_ANIMATION, true, BB_SKELETON, 1,
                                                  i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false);
                            if (skeleton != NULL)
                            {
                                tilemap->fgTiles[i][j].entity = skeleton;
                            }
                            break;
                        }
                        case WASHING_MACHINE_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            if (bb_createEntity(entityManager, NO_ANIMATION, true, BB_WASHING_MACHINE, 1,
                                                i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false)
                                != NULL)
                            {
                                tilemap->fgTiles[i][j].embed = NOTHING_EMBED;
                            }
                            break;
                        }
                        case BB_CAR_WITH_DONUT_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            bb_entity_t* car = bb_createEntity(entityManager, ONESHOT_ANIMATION, true, BB_CAR, 6,
                                                               i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE + 2,
                                                               false, false);
                            if (car != NULL)
                            {
                                ((bb_carData_t*)car->data)->reward = BB_DONUT;
                                car->currentAnimationFrame         = 1;
                                tilemap->fgTiles[i][j].embed       = NOTHING_EMBED;
                            }
                            break;
                        }
                        case BB_CAR_WITH_SWADGE_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            bb_entity_t* car = bb_createEntity(entityManager, ONESHOT_ANIMATION, true, BB_CAR, 6,
                                                               i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE + 2,
                                                               false, false);
                            if (car != NULL)
                            {
                                ((bb_carData_t*)car->data)->reward = BB_SWADGE;
                                car->currentAnimationFrame         = 1;
                                tilemap->fgTiles[i][j].embed       = NOTHING_EMBED;
                            }
                            break;
                        }
                        case BB_FOOD_CART_WITH_DONUT_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 2);
                            // background piece, also with collision on the umbrella
                            bb_entity_t* foodCartBG = bb_createEntity(entityManager, NO_ANIMATION, true, BB_FOOD_CART,
                                                                      1, i * TILE_SIZE + HALF_TILE + 1,
                                                                      j * TILE_SIZE + HALF_TILE - 58, false, false);
                            if (foodCartBG != NULL)
                            {
                                // main piece
                                bb_entity_t* foodCart = bb_createEntity(entityManager, NO_ANIMATION, true, BB_FOOD_CART,
                                                                        1, i * TILE_SIZE + HALF_TILE,
                                                                        j * TILE_SIZE + HALF_TILE - 13, false, false);
                                if (foodCart != NULL)
                                {
                                    ((bb_foodCartData_t*)foodCart->data)->partner   = foodCartBG;
                                    ((bb_foodCartData_t*)foodCartBG->data)->partner = foodCart;

                                    ((bb_foodCartData_t*)foodCart->data)->reward = BB_DONUT;
                                    foodCart->currentAnimationFrame
                                        = 20; // Also used as health for the food cart. It takes 10 hits to destroy.
                                    tilemap->fgTiles[i][j].embed = NOTHING_EMBED;
                                }
                                else
                                {
                                    bb_destroyEntity(foodCartBG, false, true);
                                }
                            }
                            break;
                        }
                        case BB_FOOD_CART_WITH_SWADGE_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 2);
                            // background piece, also with collision on the umbrella
                            bb_entity_t* foodCartBG = bb_createEntity(entityManager, NO_ANIMATION, true, BB_FOOD_CART,
                                                                      1, i * TILE_SIZE + HALF_TILE + 1,
                                                                      j * TILE_SIZE + HALF_TILE - 58, false, false);
                            if (foodCartBG != NULL)
                            {
                                // main piece
                                bb_entity_t* foodCart = bb_createEntity(entityManager, NO_ANIMATION, true, BB_FOOD_CART,
                                                                        1, i * TILE_SIZE + HALF_TILE,
                                                                        j * TILE_SIZE + HALF_TILE - 13, false, false);
                                if (foodCart != NULL)
                                {
                                    ((bb_foodCartData_t*)foodCart->data)->partner   = foodCartBG;
                                    ((bb_foodCartData_t*)foodCartBG->data)->partner = foodCart;

                                    ((bb_foodCartData_t*)foodCart->data)->reward = BB_SWADGE;
                                    foodCart->currentAnimationFrame = 20; // functions as health for the food cart
                                    tilemap->fgTiles[i][j].embed    = NOTHING_EMBED;
                                }
                                else
                                {
                                    bb_destroyEntity(foodCartBG, false, true);
                                }
                            }
                            break;
                        }
                        case DOOR_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            if (bb_createEntity(entityManager, NO_ANIMATION, true, BB_DOOR, 1,
                                                i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false)
                                != NULL)
                            {
                                tilemap->fgTiles[i][j].embed = NOTHING_EMBED;
                            }
                            break;
                        }
                        case BRICK_TUTORIAL_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            if (bb_createEntity(entityManager, NO_ANIMATION, true, BB_BRICK_TUTORIAL, 1,
                                                i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false)
                                != NULL)
                            {
                                tilemap->fgTiles[i][j].embed = NOTHING_EMBED;
                            }
                            break;
                        }
                        case FINAL_BOSS_EMBED:
                        {
                            bb_ensureEntitySpace(entityManager, 1);
                            if (bb_createEntity(entityManager, LOOPING_ANIMATION, false, BB_FINAL_BOSS, 4,
                                                i * TILE_SIZE + HALF_TILE, j * TILE_SIZE + HALF_TILE, false, false)
                                != NULL)
                            {
                                tilemap->fgTiles[i][j].embed = NOTHING_EMBED;
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }

                vec_t tilePos = {.x = i * TILE_SIZE - camera->pos.x, .y = j * TILE_SIZE - camera->pos.y};

                // Figure out which midground tile quadrants are worth drawing
                // to cut down on overdraw
                if (tilemap->mgTiles[i][j].health > 0)
                {
                    // Just don't draw it if it is on the very edge, because I am lazy to handle those cases.
                    if (i && (i != TILE_FIELD_WIDTH - 1) && j && (j != TILE_FIELD_HEIGHT - 1))
                    {
                        // drawMidground variable explained
                        // 0b00001000 means draw top    left  mg
                        // 0b00000100 means draw top    right mg
                        // 0b00000010 means draw bottom left  mg
                        // 0b00000001 means draw bottom right mg
                        uint8_t drawMidground = 0;
                        if (tilemap->fgTiles[i][j].health > 0) // if there is fg
                        {
                            // top left stuff
                            drawMidground |= ((
                                                  // mg going left & fg isn't
                                                  ((tilemap->mgTiles[i - 1][j].health > 0)
                                                   && (tilemap->fgTiles[i - 1][j].health == 0))
                                                  ||
                                                  // mg going up & fg isn't
                                                  ((tilemap->mgTiles[i][j - 1].health > 0)
                                                   && (tilemap->fgTiles[i][j - 1].health == 0))
                                                  ||
                                                  // mg going diagonal and fg isn't at diagonal
                                                  ((tilemap->mgTiles[i - 1][j].health > 0)
                                                   && (tilemap->mgTiles[i][j - 1].health > 0)
                                                   && (tilemap->mgTiles[i - 1][j - 1].health > 0)
                                                   && (tilemap->fgTiles[i - 1][j - 1].health == 0)))
                                              << 3); // set the 3rd bit

                            // top right stuff
                            drawMidground |= ((
                                                  // mg going right & fg isn't
                                                  ((tilemap->mgTiles[i + 1][j].health > 0)
                                                   && (tilemap->fgTiles[i + 1][j].health == 0))
                                                  ||
                                                  // mg going up & fg isn't
                                                  ((tilemap->mgTiles[i][j - 1].health > 0)
                                                   && (tilemap->fgTiles[i][j - 1].health == 0))
                                                  ||
                                                  // mg going diagonal and fg isn't at diagonal
                                                  ((tilemap->mgTiles[i + 1][j].health > 0)
                                                   && (tilemap->mgTiles[i][j - 1].health > 0)
                                                   && (tilemap->mgTiles[i + 1][j - 1].health > 0)
                                                   && (tilemap->fgTiles[i + 1][j - 1].health == 0)))
                                              << 2); // set the 2nd bit

                            // bottom left stuff
                            drawMidground |= ((
                                                  // mg going left & fg isn't
                                                  ((tilemap->mgTiles[i - 1][j].health > 0)
                                                   && (tilemap->fgTiles[i - 1][j].health == 0))
                                                  ||
                                                  // mg going down & fg isn't
                                                  ((tilemap->mgTiles[i][j + 1].health > 0)
                                                   && (tilemap->fgTiles[i][j + 1].health == 0))
                                                  ||
                                                  // mg going diagonal and fg isn't at diagonal
                                                  ((tilemap->mgTiles[i - 1][j].health > 0)
                                                   && (tilemap->mgTiles[i][j + 1].health > 0)
                                                   && (tilemap->mgTiles[i - 1][j + 1].health > 0)
                                                   && (tilemap->fgTiles[i - 1][j + 1].health == 0)))
                                              << 1); // set the 1st bit

                            // bottom right stuff
                            drawMidground |= ((
                                                  // mg going right & fg isn't
                                                  ((tilemap->mgTiles[i + 1][j].health > 0)
                                                   && (tilemap->fgTiles[i + 1][j].health == 0))
                                                  ||
                                                  // mg going down & fg isn't
                                                  ((tilemap->mgTiles[i][j + 1].health > 0)
                                                   && (tilemap->fgTiles[i][j + 1].health == 0))
                                                  ||
                                                  // mg going diagonal and fg isn't at diagonal
                                                  ((tilemap->mgTiles[i + 1][j].health > 0)
                                                   && (tilemap->mgTiles[i][j + 1].health > 0)
                                                   && (tilemap->mgTiles[i + 1][j + 1].health > 0)
                                                   && (tilemap->fgTiles[i + 1][j + 1].health == 0)))
                                              << 0); // set the 0th bit
                        }
                        else
                        {
                            drawMidground = 0b00001111;
                        }

                        // Draw midground tiles
                        wsg_t(*wsgMidgroundArrayPtr)[120] = bb_GetMidgroundWsgArrForCoord(tilemap, i, j);

                        // sprite_idx LURD order.
                        int8_t sprite_idx
                            = 8 * ((i - 1 < 0) ? 0 : (tilemap->mgTiles[i - 1][j].health > 0))
                              + 4 * ((j - 1 < 0) ? 0 : (tilemap->mgTiles[i][j - 1].health > 0))
                              + 2 * ((i + 1 > TILE_FIELD_WIDTH - 1) ? 0 : tilemap->mgTiles[i + 1][j].health > 0)
                              + 1 * ((j + 1 > TILE_FIELD_HEIGHT - 1) ? 0 : tilemap->mgTiles[i][j + 1].health > 0);
                        // corner_info represents up_left, up_right, down_left, down_right dirt presence (remember >0 is
                        // dirt).
                        int8_t corner_info
                            = 8
                                  * ((i - 1 < 0)   ? 0
                                     : (j - 1 < 0) ? 0
                                                   : (tilemap->mgTiles[i - 1][j - 1].health > 0))
                              + 4
                                    * ((i + 1 > TILE_FIELD_WIDTH - 1) ? 0
                                       : (j - 1 < 0)                  ? 0
                                                                      : tilemap->mgTiles[i + 1][j - 1].health > 0)
                              + 2
                                    * ((i - 1 < 0)                       ? 0
                                       : (j + 1 > TILE_FIELD_HEIGHT - 1) ? 0
                                                                         : tilemap->mgTiles[i - 1][j + 1].health > 0)
                              + 1
                                    * ((i + 1 > TILE_FIELD_WIDTH - 1)    ? 0
                                       : (j + 1 > TILE_FIELD_HEIGHT - 1) ? 0
                                                                         : tilemap->mgTiles[i + 1][j + 1].health > 0);

                        vec_t lookup = {tilePos.x + 8 - (garbotnikDrawPos->x + 18) + tilemap->headlampWsg.w,
                                        tilePos.y + 8 - (garbotnikDrawPos->y + 17) + tilemap->headlampWsg.h};
                        lookup       = divVec2d(lookup, 2);

                        if (drawMidground & 0b00001000)
                        {
                            // Top Left
                            // 0 11xx 1xxx
                            // 4 10xx xxxx
                            // 8 01xx xxxx
                            // 12 00xx xxxx
                            // 16 11xx 0xxx
                            // vec_t lookup = {tilePos.x + 8 - garbotnikDrawPos->x + 16 + tilemap->headlampWsg.w / 2,
                            //                 tilePos.y + 8 - garbotnikDrawPos->y + 17 + tilemap->headlampWsg.h / 2};
                            // ESP_LOGD(BB_TAG,"lookup: %d\n",lookup.x);
                            brightness = 5 - (j > 25 ? 25 : j) / 5;
                            if (entityManager->playerEntity != NULL
                                && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                            {
                                brightness = bb_midgroundLighting(&(tilemap->headlampWsg), &lookup,
                                                                  &(garbotnikRotation->x), brightness);
                            }

                            switch (sprite_idx & 0b1100)
                            {
                                case 0b1100: // 0 16
                                {
                                    switch (corner_info & 0b1000)
                                    {
                                        case 0b1000: // 0
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 0], tilePos.x,
                                                          tilePos.y);
                                            break;
                                        default: // 0b0000 16
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 16], tilePos.x,
                                                          tilePos.y);
                                            break;
                                    }
                                    break;
                                }
                                case 0b1000: // 4
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 4], tilePos.x, tilePos.y);
                                    break;
                                }
                                case 0b0100: // 8
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 8], tilePos.x, tilePos.y);
                                    break;
                                }
                                default: // 0b0000:12
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 12], tilePos.x, tilePos.y);
                                    break;
                                }
                            }
                        }

                        lookup.x += 8;

                        if (drawMidground & 0b000000100)
                        {
                            // Top Right
                            // 1 x11x x1xx
                            // 5 x01x xxxx
                            // 9 x10x xxxx
                            // 13 x00x xxxx
                            // 17 x11x x0xx
                            brightness = 5 - (j > 25 ? 25 : j) / 5;
                            if (entityManager->playerEntity != NULL
                                && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                            {
                                brightness = bb_midgroundLighting(&(tilemap->headlampWsg), &lookup,
                                                                  &(garbotnikRotation->x), brightness);
                            }

                            switch (sprite_idx & 0b110)
                            {
                                case 0b110: // 1 17
                                {
                                    switch (corner_info & 0b0100)
                                    {
                                        case 0b0100: // 1
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 1],
                                                          tilePos.x + HALF_TILE, tilePos.y);
                                            break;
                                        }
                                        default: // 0b0000 17
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 17],
                                                          tilePos.x + HALF_TILE, tilePos.y);
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case 0b010: // 5
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 5], tilePos.x + HALF_TILE,
                                                  tilePos.y);
                                    break;
                                }
                                case 0b100: // 9
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 9], tilePos.x + HALF_TILE,
                                                  tilePos.y);
                                    break;
                                }
                                default: // 0b0000:13
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 13], tilePos.x + HALF_TILE,
                                                  tilePos.y);
                                    break;
                                }
                            }
                        }

                        lookup.x -= 8;
                        lookup.y += 8;

                        if (drawMidground & 0b00000010)
                        {
                            // Bottom Left
                            // 2 1xx1 xx1x
                            // 6 1xx0 xxxx
                            // 10 0xx1 xxxx
                            // 14 0xx0 xxxx
                            // 18 1xx1 xx0x

                            brightness = 5 - (j > 25 ? 25 : j) / 5;
                            if (entityManager->playerEntity != NULL
                                && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                            {
                                brightness = bb_midgroundLighting(&(tilemap->headlampWsg), &lookup,
                                                                  &(garbotnikRotation->x), brightness);
                            }

                            switch (sprite_idx & 0b1001)
                            {
                                case 0b1001: // 2 18
                                {
                                    switch (corner_info & 0b0010)
                                    {
                                        case 0b0010: // 2
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 2], tilePos.x,
                                                          tilePos.y + HALF_TILE);
                                            break;
                                        }
                                        default: // 0b0000 18
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 18], tilePos.x,
                                                          tilePos.y + HALF_TILE);
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case 0b1000: // 6
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 6], tilePos.x,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                                case 0b0001: // 10
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 10], tilePos.x,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                                default: // 0b0000:14
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 14], tilePos.x,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                            }
                        }

                        lookup.x += 8;
                        if (drawMidground & 0b00000001)
                        {
                            // Bottom Right
                            // 3 xx11 xxx1
                            // 7 xx10 xxxx
                            // 11 xx01 xxxx
                            // 15 xx00 xxxx
                            // 19 xx11 xxx0
                            brightness = 5 - (j > 25 ? 25 : j) / 5;
                            if (entityManager->playerEntity != NULL
                                && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                            {
                                brightness = bb_midgroundLighting(&(tilemap->headlampWsg), &lookup,
                                                                  &(garbotnikRotation->x), brightness);
                            }

                            switch (sprite_idx & 0b0011)
                            {
                                case 0b11: // 3 19
                                {
                                    switch (corner_info & 0b1)
                                    {
                                        case 0b1: // 3
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 3],
                                                          tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                            break;
                                        }
                                        default: // 0b0000 19
                                        {
                                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 19],
                                                          tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case 0b10: // 7
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 7], tilePos.x + HALF_TILE,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                                case 0b01: // 11
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 11], tilePos.x + HALF_TILE,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                                default: // 0b0000:15
                                {
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20 * brightness + 15], tilePos.x + HALF_TILE,
                                                  tilePos.y + HALF_TILE);
                                    break;
                                }
                            }
                        }
                    }
                }

                // Draw foreground tiles
                if (tilemap->fgTiles[i][j].health >= 1)
                {
                    wsg_t(*wsgForegroundArrayPtr)[240] = bb_GetForegroundWsgArrForCoord(tilemap, i, j);

                    // sprite_idx LURD order.
                    uint8_t sprite_idx
                        = 8 * ((i - 1 < 0) ? 0 : (tilemap->fgTiles[i - 1][j].health > 0))
                          + 4 * ((j - 1 < 0) ? 0 : (tilemap->fgTiles[i][j - 1].health > 0))
                          + 2 * ((i + 1 > TILE_FIELD_WIDTH - 1) ? 0 : (tilemap->fgTiles[i + 1][j].health > 0))
                          + 1 * ((j + 1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->fgTiles[i][j + 1]).health > 0);
                    // corner_info represents up_left, up_right, down_left, down_right dirt presence (remember >0 is
                    // dirt).
                    uint8_t corner_info
                        = 8
                              * ((i - 1 < 0)   ? 0
                                 : (j - 1 < 0) ? 0
                                               : (tilemap->fgTiles[i - 1][j - 1].health > 0))
                          + 4
                                * ((i + 1 > TILE_FIELD_WIDTH - 1) ? 0
                                   : (j - 1 < 0)                  ? 0
                                                                  : (tilemap->fgTiles[i + 1][j - 1].health > 0))
                          + 2
                                * ((i - 1 < 0)                       ? 0
                                   : (j + 1 > TILE_FIELD_HEIGHT - 1) ? 0
                                                                     : (tilemap->fgTiles[i - 1][j + 1].health > 0))
                          + 1
                                * ((i + 1 > TILE_FIELD_WIDTH - 1)    ? 0
                                   : (j + 1 > TILE_FIELD_HEIGHT - 1) ? 0
                                                                     : (tilemap->fgTiles[i + 1][j + 1]).health > 0);

                    vec_t lookup = {tilePos.x + 8 - (garbotnikDrawPos->x + 18) + tilemap->headlampWsg.w,
                                    tilePos.y + 8 - (garbotnikDrawPos->y + 17) + tilemap->headlampWsg.h};
                    lookup       = divVec2d(lookup, 2);

                    if (entityManager->playerEntity != NULL
                        && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                    {
                        brightness = bb_foregroundLighting(&(tilemap->headlampWsg), &lookup, &(garbotnikRotation->x));
                    }
                    else
                    {
                        brightness = 0;
                    }

                    // Top Left      V
                    // 00RD ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                    // 1100 1...   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                    // 10RD ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                    // 01RD ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                    // 1110 1...   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                    // 1101 1...   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                    // 11RD 0...   (19,0), (17,1), (18,2), (16,3),#concave corners
                    // 1111 10..   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                    // 1111 1.0.   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                    // 1111 1110   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                    uint8_t num = (sprite_idx << 4) + corner_info; // 8-bit number
                    if ((num & 0b11000000) == 0b00000000)
                    {
                        // Case 1: 00.. ....
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 0], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11111000) == 0b11001000)
                    {
                        // Case 2: 1100 1...
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 4], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11000000) == 0b10000000)
                    {
                        // Case 3: 10.. ....
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 8], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11000000) == 0b01000000)
                    {
                        // Case 4: 01.. ....
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 12], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11111000) == 0b11101000)
                    {
                        // Case 5: 1110 1...
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 16], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11111000) == 0b11011000)
                    {
                        // Case 6: 1101 1...
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 20], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11001000) == 0b11000000)
                    {
                        // Case 7: 11.. 0...
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 24], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11111100) == 0b11111000)
                    {
                        // Case 8: 1111 10..
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 28], tilePos.x, tilePos.y);
                    }
                    else if ((num & 0b11111010) == 0b11111000)
                    {
                        // Case 9: 1111 1.0.
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 32], tilePos.x, tilePos.y);
                    }
                    else if (num == 0b11111110)
                    {
                        // Case 10: 1111 1110
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 36], tilePos.x, tilePos.y);
                    }
                    else
                    {
                        // Case 11: 1111 1111
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[39], tilePos.x, tilePos.y);
                    }

                    lookup.x += 8;
                    if (entityManager->playerEntity != NULL
                        && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                    {
                        brightness = bb_foregroundLighting(&(tilemap->headlampWsg), &lookup, &(garbotnikRotation->x));
                    }
                    else
                    {
                        brightness = 0;
                    }

                    // Top Right             V
                    // L00D ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                    // 0110 .1..   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                    // L01D ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                    // L10D ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                    // 1110 .1..   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                    // 0111 .1..   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                    // L11D .0...  (19,0), (17,1), (18,2), (16,3),#concave corners
                    // 1111 .1.0   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                    // 1111 01..   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                    // 1111 1101   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                    if (brightness > 5)
                    {
                        brightness = 5;
                    }
                    if ((num & 0b01100000) == 0b00000000)
                    {
                        // L00D ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 1], tilePos.x + HALF_TILE, tilePos.y);
                    }
                    else if ((num & 0b11110100) == 0b01100100)
                    {
                        // 0110 .1..   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 5], tilePos.x + HALF_TILE, tilePos.y);
                    }
                    else if ((num & 0b01100000) == 0b00100000)
                    {
                        // L01D ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 9], tilePos.x + HALF_TILE, tilePos.y);
                    }
                    else if ((num & 0b01100000) == 0b01000000)
                    {
                        // L10D ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 13], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if ((num & 0b11110100) == 0b11100100)
                    {
                        // 1110 .1..   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 17], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if ((num & 0b11110100) == 0b01110100)
                    {
                        // 0111 .1..   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 21], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if ((num & 0b01100100) == 0b01100000)
                    {
                        // L11D .0..  (19,0), (17,1), (18,2), (16,3),#concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 25], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if ((num & 0b11110101) == 0b11110100)
                    {
                        // 1111 .1.0   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 29], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if ((num & 0b11111100) == 0b11110100)
                    {
                        // 1111 01..   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 33], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else if (num == 0b11111101)
                    {
                        // 1111 1101   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 37], tilePos.x + HALF_TILE,
                                      tilePos.y);
                    }
                    else
                    {
                        // Case 11: 1111 1111
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[39], tilePos.x + HALF_TILE, tilePos.y);
                    }

                    lookup.x -= 8;
                    lookup.y += 8;
                    if (entityManager->playerEntity != NULL
                        && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                    {
                        brightness = bb_foregroundLighting(&(tilemap->headlampWsg), &lookup, &(garbotnikRotation->x));
                    }
                    else
                    {
                        brightness = 0;
                    }
                    // Bottom Left                   V
                    // 0UR0 ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                    // 0110 ..1.   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                    // 1UR0 ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                    // 0UR1 ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                    // 1011 ..1.   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                    // 1101 ..1.   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                    // 1UR1 ..0.   (19,0), (17,1), (18,2), (16,3),#concave corners
                    // 1111 0.1.   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                    // 1111 ..10   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                    // 1111 1011   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                    if (brightness > 5)
                    {
                        brightness = 5;
                    }
                    if ((num & 0b10010000) == 0b00000000)
                    {
                        // 0UR0 ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 2], tilePos.x, tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110110) == 0b10010010)
                    {
                        // 0110 ..1.   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 6], tilePos.x, tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b10010000) == 0b10000000)
                    {
                        // 1UR0 ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 10], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b10010000) == 0b00010000)
                    {
                        // 0UR1 ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 14], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110010) == 0b10110010)
                    {
                        // 1011 ..1.   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 18], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110010) == 0b11010010)
                    {
                        // 1101 ..1.   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 22], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b10010010) == 0b10010000)
                    {
                        // 1UR1 ..0.   (19,0), (17,1), (18,2), (16,3),#concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 26], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11111010) == 0b11110010)
                    {
                        // 1111 0.1.   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 30], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110011) == 0b11110010)
                    {
                        // 1111 ..10   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 34], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else if (num == 0b11111011)
                    {
                        // 1111 1011   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 38], tilePos.x,
                                      tilePos.y + HALF_TILE);
                    }
                    else
                    {
                        // Case 11: 1111 1111
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[39], tilePos.x, tilePos.y + HALF_TILE);
                    }

                    lookup.x += 8;
                    if (entityManager->playerEntity != NULL
                        && entityManager->playerEntity->updateFunction == bb_updateGarbotnikFlying)
                    {
                        brightness = bb_foregroundLighting(&(tilemap->headlampWsg), &lookup, &(garbotnikRotation->x));
                    }
                    else
                    {
                        brightness = 0;
                    }
                    // Bottom Right                          V
                    if ((num & 0b00110000) == 0b00000000)
                    {
                        // LU00 ....   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 3], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110001) == 0b00110001)
                    {
                        // 0011 ...1   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 7], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b00110000) == 0b00100000)
                    {
                        // LU10 ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 11], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b00110000) == 0b00010000)
                    {
                        // LU01 ....   (11,0), (11,1), (11,2), (11,3),#vertical light
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 15], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110001) == 0b10110001)
                    {
                        // 1011 ...1   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 19], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110001) == 0b01110001)
                    {
                        // 0111 ...1   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 23], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b00110001) == 0b00110000)
                    {
                        // LU11 ...0   (19,0), (17,1), (18,2), (16,3),#concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 27], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110011) == 0b11110001)
                    {
                        // 1111 ..01   (17,0), (16,1), (19,2), (18,3),#left of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 31], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if ((num & 0b11110101) == 0b11110001)
                    {
                        // 1111 .0.1   (18,0), (19,1), (16,2), (17,3),#right of concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 35], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else if (num == 0b11110111)
                    {
                        // 1111 0111   (16,0), (18,1), (17,2), (19,3) #opposite concave corners
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[40 * brightness + 39], tilePos.x + HALF_TILE,
                                      tilePos.y + HALF_TILE);
                    }
                    else
                    {
                        drawWsgSimple(&(*wsgForegroundArrayPtr)[39], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                    }

                    // char snum[4];
                    // sprintf(snum, "%d", brightness);
                    // drawText(&ibm, c555, snum, tilePos.x, tilePos.y);
                }
            }
        }
    }
    // freeFont(&ibm);
}

void bb_collisionCheck(bb_tilemap_t* tilemap, bb_entity_t* ent, vec_t* previousPos, bb_hitInfo_t* hitInfo)
{
    // Look up nearest tiles for collision checks
    // a tile's width is 16 pixels << 4 = 512. half width is 256.
    int32_t xIdx = (ent->pos.x - ent->halfWidth) / BITSHIFT_TILE_SIZE - (ent->pos.x < 0);  // the x index
    int32_t yIdx = (ent->pos.y - ent->halfHeight) / BITSHIFT_TILE_SIZE - (ent->pos.y < 0); // the y index

    int32_t closestSqDist = 131072 + ent->cSquared; //((16<<4)^2+(16<<4)^2+entity's cSquared)if it's further than this,
                                                    // there's no way it's a collision.
    closestSqDist += 150000;                        // Why do I have to do this? I don't know.............
    int32_t right_i = (ent->halfWidth * 2) / BITSHIFT_TILE_SIZE;
    right_i         = right_i ? right_i : 1;
    right_i += xIdx + 1;
    int32_t bottom_j = (ent->halfHeight * 2) / BITSHIFT_TILE_SIZE;
    bottom_j         = bottom_j ? bottom_j : 1;
    bottom_j += yIdx + 1;
    for (int32_t i = xIdx; i <= right_i; i++)
    {
        for (int32_t j = yIdx; j <= bottom_j; j++)
        {
            if (i >= 0 && i < TILE_FIELD_WIDTH && j >= 0 && j < TILE_FIELD_HEIGHT)
            {
                if (ent->gameData->tilemap.fgTiles[i][j].health >= 1)
                {
                    // Initial circle check for preselecting the closest dirt tile
                    int32_t sqDist
                        = sqMagVec2d(subVec2d(ent->pos, (vec_t){i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE,
                                                                j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE}));
                    if (sqDist < closestSqDist)
                    {
                        // Good candidate found!
                        vec_t tilePos = {i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE,
                                         j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE};
                        // AABB-AABB collision detection begins here
                        // https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
                        if (ent->pos.x - (int32_t)ent->halfWidth < tilePos.x + (int32_t)BITSHIFT_HALF_TILE
                            && ent->pos.x + (int32_t)ent->halfWidth > tilePos.x - (int32_t)BITSHIFT_HALF_TILE
                            && ent->pos.y - (int32_t)ent->halfHeight < tilePos.y + (int32_t)BITSHIFT_HALF_TILE
                            && ent->pos.y + (int32_t)ent->halfHeight > tilePos.y - (int32_t)BITSHIFT_HALF_TILE)
                        {
                            /////////////////////////
                            // Collision detected! //
                            /////////////////////////
                            hitInfo->hit    = true;
                            closestSqDist   = sqDist;
                            hitInfo->tile_i = i;
                            hitInfo->tile_j = j;

                            if (previousPos != NULL)
                            {
                                // More accurate collision resolution if previousPos provided.
                                // Used by entities that need to bounce around or move quickly.

                                // generate hitInfo based on position from previous frame.
                                hitInfo->normal = subVec2d(*previousPos, tilePos);
                            }
                            else
                            {
                                // Worse collision resolution
                                // for entities that don't care to store their previousPos.
                                hitInfo->normal = subVec2d(ent->pos, tilePos);
                            }
                            // Snap the offset to an orthogonal direction.
                            if ((hitInfo->normal.x < 0 ? -hitInfo->normal.x : hitInfo->normal.x)
                                > (hitInfo->normal.y < 0 ? -hitInfo->normal.y : hitInfo->normal.y))
                            {
                                if (hitInfo->normal.x > 0)
                                {
                                    hitInfo->normal.x = 1;
                                    hitInfo->normal.y = 0;
                                    hitInfo->pos.x    = tilePos.x + BITSHIFT_HALF_TILE;
                                    hitInfo->pos.y    = ent->pos.y;
                                }
                                else
                                {
                                    hitInfo->normal.x = -1;
                                    hitInfo->normal.y = 0;
                                    hitInfo->pos.x    = tilePos.x - BITSHIFT_HALF_TILE;
                                    hitInfo->pos.y    = ent->pos.y;
                                }
                            }
                            else
                            {
                                if (hitInfo->normal.y > 0)
                                {
                                    hitInfo->normal.x = 0;
                                    hitInfo->normal.y = 1;
                                    hitInfo->pos.x    = ent->pos.x;
                                    hitInfo->pos.y    = tilePos.y + BITSHIFT_HALF_TILE;
                                }
                                else
                                {
                                    hitInfo->normal.x = 0;
                                    hitInfo->normal.y = -1;
                                    hitInfo->pos.x    = ent->pos.x;
                                    hitInfo->pos.y    = tilePos.y - BITSHIFT_HALF_TILE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

wsg_t (*bb_GetMidgroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[120]
{
    if (tilemap->mgTiles[i][j].health > 4)
    {
        return &tilemap->mid_h_Wsg;
    }
    else if (tilemap->mgTiles[i][j].health > 1)
    {
        return &tilemap->mid_m_Wsg;
    }
    return &tilemap->mid_s_Wsg;
}

wsg_t (*bb_GetForegroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[240]
{
    if (tilemap->fgTiles[i][j].health > 10)
    {
        return &tilemap->fore_b_Wsg;
    }
    else if (tilemap->fgTiles[i][j].health > 4)
    {
        return &tilemap->fore_h_Wsg;
    }
    else if (tilemap->fgTiles[i][j].health > 1)
    {
        return &tilemap->fore_m_Wsg;
    }
    return &tilemap->fore_s_Wsg;
}
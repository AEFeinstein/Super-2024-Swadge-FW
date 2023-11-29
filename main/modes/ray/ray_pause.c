//==============================================================================
// Includes
//==============================================================================

#include "ray_pause.h"
#include "ray_tex_manager.h"

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum for the different corners in a map box, used for drawing warp lines
 */
typedef enum
{
    TOP_RIGHT,       ///< The top right corner of a map box
    TOP_LEFT,        ///< The top left corner of a map box
    BOTTOM_LEFT,     ///< The bottom left corner of a map box
    NUM_MAP_CORNERS, ///< The number of warp points in a map
} rayWorldMapCorner_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief The information necessary to draw a line between two corners in two map boxes
 */
typedef struct
{
    int16_t map1;                ///< The map to draw a warp line from
    int16_t map2;                ///< The map to draw a warp line to
    rayWorldMapCorner_t corner1; ///< The corner to draw a warp line from
    rayWorldMapCorner_t corner2; ///< The corner to draw a warp line to
} rayWorldMapLine_t;

//==============================================================================
// Const data
//==============================================================================

/// @brief A collection of all the possible warp lines to draw
const rayWorldMapLine_t warpLines[] = {
    // From map 0
    {
        .map1    = 0,
        .corner1 = TOP_LEFT,
        .map2    = 4,
        .corner2 = TOP_LEFT,
    },
    {
        .map1    = 0,
        .corner1 = TOP_RIGHT,
        .map2    = 1,
        .corner2 = BOTTOM_LEFT,
    },
    {
        .map1    = 0,
        .corner1 = BOTTOM_LEFT,
        .map2    = 3,
        .corner2 = TOP_LEFT,
    },
    // From map 1
    {
        .map1    = 1,
        .corner1 = TOP_LEFT,
        .map2    = 5,
        .corner2 = TOP_LEFT,
    },
    {
        .map1    = 1,
        .corner1 = TOP_RIGHT,
        .map2    = 2,
        .corner2 = BOTTOM_LEFT,
    },
    // From map 2
    {
        .map1    = 2,
        .corner1 = TOP_LEFT,
        .map2    = 5,
        .corner2 = TOP_RIGHT,
    },
    {
        .map1    = 2,
        .corner1 = TOP_RIGHT,
        .map2    = 3,
        .corner2 = BOTTOM_LEFT,
    },
    // From map 3
    {
        .map1    = 3,
        .corner1 = TOP_RIGHT,
        .map2    = 4,
        .corner2 = BOTTOM_LEFT,
    },
    // From map 4
    {
        .map1    = 4,
        .corner1 = TOP_RIGHT,
        .map2    = 5,
        .corner2 = BOTTOM_LEFT,
    },
};

//==============================================================================
// Function Declarations
//==============================================================================

static void rayPauseRenderLocalMap(ray_t* ray, uint32_t elapsedUs);
static void rayPauseRenderWorldMap(ray_t* ray, uint32_t elapsedUs);
static void drawPlayerIndicator(ray_t* ray, int16_t cX, int16_t cY);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Show the pause menu
 *
 * @param ray The whole game state
 */
void rayShowPause(ray_t* ray)
{
    raySwitchToScreen(RAY_PAUSE);
    bzrPause();
}

/**
 * @brief Check buttons in the pause menu
 *
 * @param ray The whole game state
 */
void rayPauseCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // If A was pressed
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_UP:
                case PB_DOWN:
                case PB_LEFT:
                case PB_RIGHT:
                case PB_A:
                case PB_B:
                {
                    // Switch between local map and world map
                    ray->pauseScreen = (ray->pauseScreen + 1) % RP_NUM_SCREENS;
                    break;
                }
                case PB_START:
                {
                    // Pause over, return to game
                    raySwitchToScreen(RAY_GAME);
                    bzrResume();
                    break;
                }
                default:
                case PB_SELECT:
                {
                    break;
                }
            }
        }
    }
}

/**
 * @brief Render the map on a pause menu
 *
 * @param ray The whole game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPauseRender(ray_t* ray, uint32_t elapsedUs)
{
    // Clear to black first
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Render based on the displayed screen
    switch (ray->pauseScreen)
    {
        default:
        case RP_LOCAL_MAP:
        {
            rayPauseRenderLocalMap(ray, elapsedUs);
            break;
        }
        case RP_WORLD_MAP:
        {
            rayPauseRenderWorldMap(ray, elapsedUs);
            break;
        }
    }

    if (ray->blink)
    {
#define TRIANGLE_OFFSET_X 20
#define TRIANGLE_OFFSET_Y 0
        drawTriangleOutlined(TFT_WIDTH - TRIANGLE_OFFSET_X - 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 4,
                             TFT_WIDTH - TRIANGLE_OFFSET_X - 4, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 10,
                             TFT_WIDTH - TRIANGLE_OFFSET_X - 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 16, c100, c542);

        drawTriangleOutlined(TRIANGLE_OFFSET_X + 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 4, TRIANGLE_OFFSET_X + 4,
                             TFT_HEIGHT - TRIANGLE_OFFSET_Y - 10, TRIANGLE_OFFSET_X + 16,
                             TFT_HEIGHT - TRIANGLE_OFFSET_Y - 16, c100, c542);
    }
}

/**
 * @brief Render the local map, a collection of tiles which are revealed as the player moves through the map
 *
 * @param ray The whole game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void rayPauseRenderLocalMap(ray_t* ray, uint32_t elapsedUs)
{
    int16_t tWidth = textWidth(&ray->ibm, rayMapNames[ray->p.mapId]);
    drawText(&ray->ibm, rayMapColors[ray->p.mapId], rayMapNames[ray->p.mapId], (TFT_WIDTH - tWidth) / 2, 2);

    // Figure out the largest cell size to draw the whole map centered on the screen
    int16_t cellSizeW = TFT_WIDTH / ray->map.w;
    int16_t cellSizeH = (TFT_HEIGHT - (ray->ibm.height + 4)) / ray->map.h;
    int16_t cellSize  = MIN(cellSizeW, cellSizeH);
    int16_t cellOffX  = (TFT_WIDTH - (ray->map.w * cellSize)) / 2;
    int16_t cellOffY  = (ray->ibm.height + 4) + (((TFT_HEIGHT - (ray->ibm.height + 4)) - (ray->map.h * cellSize)) / 2);

    // For each cell
    for (int16_t y = 0; y < ray->map.h; y++)
    {
        for (int16_t x = 0; x < ray->map.w; x++)
        {
            // If this cell was visited
            if (ray->map.visitedTiles[(y * ray->map.w) + x] > NOT_VISITED)
            {
                // Get the cell type and pick a color depending on the type
                rayMapCellType_t type = ray->map.tiles[x][y].type;
                paletteColor_t color  = c000;
                if (CELL_IS_TYPE(type, BG | WALL))
                {
                    // All walls are the same
                    color = c001;
                }
                else if (CELL_IS_TYPE(type, BG | DOOR))
                {
                    // Doors are colored based on the type of door
                    switch (type)
                    {
                        case BG_DOOR:
                        {
                            color = c444;
                            break;
                        }
                        case BG_DOOR_CHARGE:
                        {
                            color = c404;
                            break;
                        }
                        case BG_DOOR_MISSILE:
                        {
                            color = c500;
                            break;
                        }
                        case BG_DOOR_ICE:
                        {
                            color = c005;
                            break;
                        }
                        case BG_DOOR_XRAY:
                        {
                            // Hide XRAY doors until the player gets the xrayLoadOut
                            if (ray->p.i.xrayLoadOut)
                            {
                                color = c050;
                            }
                            else
                            {
                                // Wall color
                                color = c001;
                            }
                            break;
                        }
                        case BG_DOOR_SCRIPT:
                        {
                            color = c222;
                            break;
                        }
                        case BG_DOOR_KEY_A:
                        {
                            color = c541;
                            break;
                        }
                        case BG_DOOR_KEY_B:
                        {
                            color = c423;
                            break;
                        }
                        case BG_DOOR_KEY_C:
                        {
                            color = c234;
                            break;
                        }
                        case BG_DOOR_ARTIFACT:
                        {
                            color = c245;
                            break;
                        }
                        default:
                        {
                            // Can't reach here
                            break;
                        }
                    }
                }
                else if (CELL_IS_TYPE(type, BG | FLOOR))
                {
                    switch (type)
                    {
                        case BG_FLOOR:
                        {
                            color = c111;
                            break;
                        }
                        case BG_FLOOR_LAVA:
                        {
                            color = c311;
                            break;
                        }
                        case BG_FLOOR_WATER:
                        {
                            color = c113;
                            break;
                        }
                        case BG_FLOOR_HEAL:
                        {
                            color = c030;
                            break;
                        }
                        default:
                        {
                            // Can't reach here
                            break;
                        }
                    }
                }

                // Draw a rectangle for this map cell
                fillDisplayArea(cellOffX + (x * cellSize),       //
                                cellOffY + (y * cellSize),       //
                                cellOffX + ((x + 1) * cellSize), //
                                cellOffY + ((y + 1) * cellSize), //
                                color);
            }
        }
    }

    // Look through scenery for warp points
    node_t* currentNode = ray->scenery.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayObjCommon_t* obj = ((rayObjCommon_t*)currentNode->val);

        // Look for portals
        if (OBJ_SCENERY_PORTAL == obj->type)
        {
            // Found a portal, look for corresponding script
            node_t* scriptNode = ray->scripts[TOUCH].first;
            while (NULL != scriptNode)
            {
                rayScript_t* scr = scriptNode->val;
                // If this is the right script for this object
                if ((TOUCH == scr->ifOp) && (WARP == scr->thenOp) && (scr->ifArgs.idList.ids[0] == obj->id))
                {
                    // And the player has visited the other end of the warp
                    if (ray->p.mapsVisited[scr->thenArgs.warpDest.mapId])
                    {
                        // Draw a number indicating the warp destination
                        char num[8];
                        snprintf(num, sizeof(num) - 1, "%1d", scr->thenArgs.warpDest.mapId + 1);
                        tWidth = textWidth(&ray->ibm, num);
                        drawText(&ray->ibm, c555, num,
                                 cellOffX + (cellSize * FROM_FX(obj->posX)) + (cellSize - tWidth) / 2,
                                 cellOffY + (cellSize * FROM_FX(obj->posY)) + (cellSize - ray->ibm.height) / 2);
                    }
                    break;
                }
                scriptNode = scriptNode->next;
            }
        }

        // Iterate to the next node
        currentNode = currentNode->next;
    }

    // The player's location blinks, so draw it when appropriate
    int16_t cX = cellOffX + FROM_FX(cellSize * ray->p.posX);
    int16_t cY = cellOffY + FROM_FX(cellSize * ray->p.posY);
    drawPlayerIndicator(ray, cX, cY);
}

/**
 * @brief Draw the blinking player indicator on the pause screen
 *
 * @param ray The entire game state
 * @param cX The X pixel to center on
 * @param cY The Y pixel to center on
 */
static void drawPlayerIndicator(ray_t* ray, int16_t cX, int16_t cY)
{
    if (ray->blink)
    {
        // Draw a circle for the player
        int16_t cR = 6;
        drawCircle(cX, cY, cR, c145);

        // Draw a line for the player's direction
        int16_t lineEndX = cX + FROM_FX(cR * ray->p.dirX);
        int16_t lineEndY = cY + FROM_FX(cR * ray->p.dirY);
        drawLine(cX, cY, lineEndX, lineEndY, c552, 0);
    }
}

/**
 * @brief Render the world map, a collection of map boxes with warp lines between then. Map boxes are only
 * representative of actual maps, since only one map can be loaded at a time.
 *
 * @param ray The whole game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void rayPauseRenderWorldMap(ray_t* ray, uint32_t elapsedUs)
{
#define MAP_X_MARGIN  16
#define MAP_Y_MARGIN  36
#define MAP_Y_MID_GAP 16
#define MAP_SIZE      72

#define MAP_ROWS 2
#define MAP_COLS 3

#define WARP_POINT_INDENT 4

    // Save coordinates for where to draw warp lines
    int16_t warpCoords[NUM_MAPS][NUM_MAP_CORNERS][2];

    // A texture to draw after artifacts are acquired
    wsg_t* artifactWsg = getTexByType(ray, OBJ_ITEM_ARTIFACT);
    int16_t aAOffX     = (MAP_SIZE - artifactWsg->w) / 2;
    int16_t aAOffY     = (MAP_SIZE - artifactWsg->h) / 2;

    // For all six maps in a 3x2 grid
    for (int16_t mapY = 0; mapY < MAP_ROWS; mapY++)
    {
        for (int16_t mapX = 0; mapX < MAP_COLS; mapX++)
        {
            // Get the map ID
            int16_t mapId = (mapY * MAP_COLS) + mapX;

            // Only draw it if it's been visited
            if (ray->p.mapsVisited[mapId])
            {
                // Find the box coordinates
                int16_t startX = MAP_X_MARGIN + mapX * (MAP_SIZE + MAP_X_MARGIN);
                int16_t startY = MAP_Y_MARGIN + mapY * (MAP_SIZE + MAP_Y_MID_GAP);
                int16_t endX   = startX + MAP_SIZE;
                int16_t endY   = startY + MAP_SIZE;

                // Draw the dark red box, outlined
                fillDisplayArea(startX + 1, startY + 1, endX - 1, endY - 1, c100);
                drawRect(startX, startY, endX, endY, rayMapColors[mapId]);

                // Draw an artifact if was acquired here
                if (ray->p.i.artifacts[mapId])
                {
                    drawWsgSimple(artifactWsg, startX + aAOffX, startY + aAOffY);
                }

                // Draw the name, either above or below the box
                int16_t tWidth = textWidth(&ray->ibm, rayMapNames[mapId]);
                if (0 == mapY)
                {
                    drawText(&ray->ibm, rayMapColors[mapId], rayMapNames[mapId], //
                             startX + ((MAP_SIZE - tWidth) / 2), startY - ray->ibm.height - 4);
                }
                else
                {
                    drawText(&ray->ibm, rayMapColors[mapId], rayMapNames[mapId], //
                             startX + ((MAP_SIZE - tWidth) / 2), endY + 4);
                }

                // Save warp coordinates for this map
                warpCoords[mapId][TOP_LEFT][0] = startX + WARP_POINT_INDENT;
                warpCoords[mapId][TOP_LEFT][1] = startY + WARP_POINT_INDENT;

                warpCoords[mapId][TOP_RIGHT][0] = endX - WARP_POINT_INDENT - 1;
                warpCoords[mapId][TOP_RIGHT][1] = startY + WARP_POINT_INDENT;

                warpCoords[mapId][BOTTOM_LEFT][0] = startX + WARP_POINT_INDENT;
                warpCoords[mapId][BOTTOM_LEFT][1] = endY - WARP_POINT_INDENT - 1;

                if (ray->p.mapId == mapId)
                {
                    int16_t pPosX = startX + (MAP_SIZE * ray->p.posX) / TO_FX(ray->map.w);
                    int16_t pPosY = startY + (MAP_SIZE * ray->p.posY) / TO_FX(ray->map.h);

                    drawPlayerIndicator(ray, pPosX, pPosY);
                }
            }
        }
    }

    // For each warp line
    for (int16_t wIdx = 0; wIdx < ARRAY_SIZE(warpLines); wIdx++)
    {
        // If both maps were visited
        if (ray->p.mapsVisited[warpLines[wIdx].map1] && ray->p.mapsVisited[warpLines[wIdx].map2])
        {
            // Draw one color line
            drawLine(warpCoords[warpLines[wIdx].map1][warpLines[wIdx].corner1][0],
                     warpCoords[warpLines[wIdx].map1][warpLines[wIdx].corner1][1],
                     warpCoords[warpLines[wIdx].map2][warpLines[wIdx].corner2][0],
                     warpCoords[warpLines[wIdx].map2][warpLines[wIdx].corner2][1], rayMapColors[warpLines[wIdx].map1],
                     0);
            // Dash the other color on top of the first
            drawLine(warpCoords[warpLines[wIdx].map1][warpLines[wIdx].corner1][0],
                     warpCoords[warpLines[wIdx].map1][warpLines[wIdx].corner1][1],
                     warpCoords[warpLines[wIdx].map2][warpLines[wIdx].corner2][0],
                     warpCoords[warpLines[wIdx].map2][warpLines[wIdx].corner2][1], rayMapColors[warpLines[wIdx].map2],
                     6);
        }
    }

    // Draw the string
    char collectionStr[32] = {0};
    snprintf(collectionStr, sizeof(collectionStr) - 1, "Percentage Complete: %" PRId32 "%%", getItemCompletePct(ray));
    int16_t tWidth = textWidth(&ray->ibm, collectionStr);
    drawText(&ray->ibm, c555, collectionStr, (TFT_WIDTH - tWidth) / 2, TFT_HEIGHT - ray->ibm.height - 10);
}

/**
 * @brief Get the percentage of items collected, 0 to 100
 *
 * @return The percentage of items collected
 */
int32_t getItemCompletePct(ray_t* ray)
{
    // Count all the possible items in the game
    int32_t numTotalItems = 6 +             // energy tanks
                            ((2 * 6) + 1) + // Missile expansions;
                            6 +             // Artifacts
                            2 +             // Suits
                            5;              // Beams

    // Start counting what the player has
    int32_t numAcquiredItems = 0;

    // For each map
    for (int32_t mIdx = 0; mIdx < NUM_MAPS; mIdx++)
    {
        // Count energy tanks
        for (int32_t eIdx = 0; eIdx < E_TANKS_PER_MAP; eIdx++)
        {
            if (-1 != ray->p.i.healthPickUps[mIdx][eIdx])
            {
                numAcquiredItems++;
            }
        }

        // Count missile upgrades
        for (int32_t sIdx = 0; sIdx < MISSILE_UPGRADES_PER_MAP; sIdx++)
        {
            if (-1 != ray->p.i.missilesPickUps[mIdx][sIdx])
            {
                numAcquiredItems++;
            }
        }

        // Count artifacts
        if (ray->p.i.artifacts[mIdx])
        {
            numAcquiredItems++;
        }
    }

    // Count beams
    numAcquiredItems += (ray->p.i.beamLoadOut ? 1 : 0);
    numAcquiredItems += (ray->p.i.chargePowerUp ? 1 : 0);
    numAcquiredItems += (ray->p.i.missileLoadOut ? 1 : 0);
    numAcquiredItems += (ray->p.i.iceLoadOut ? 1 : 0);
    numAcquiredItems += (ray->p.i.xrayLoadOut ? 1 : 0);

    // Count suits
    numAcquiredItems += (ray->p.i.lavaSuit ? 1 : 0);
    numAcquiredItems += (ray->p.i.waterSuit ? 1 : 0);

    return (numAcquiredItems * 100) / numTotalItems;
}
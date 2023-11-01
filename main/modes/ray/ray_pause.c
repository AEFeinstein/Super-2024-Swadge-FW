//==============================================================================
// Includes
//==============================================================================

#include "ray_pause.h"

//==============================================================================
// Defines
//==============================================================================

#define PAUSE_BLINK_US 500000

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
    ray->screen = RAY_PAUSE;
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
            if (PB_START == evt.button)
            {
                // Pause over, return to game
                ray->screen = RAY_GAME;
            }
            if ((PB_A == evt.button) || (PB_B == evt.button))
            {
                // Switch between local map and world map
                ray->pauseScreen = (ray->pauseScreen + 1) % RP_NUM_SCREENS;
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

    // Run a timer to blink things
    ray->pauseBlinkTimer += elapsedUs;
    if (ray->pauseBlinkTimer > PAUSE_BLINK_US)
    {
        ray->pauseBlinkTimer -= PAUSE_BLINK_US;
        ray->pauseBlink = !ray->pauseBlink;
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
    // Figure out the largest cell size to draw the whole map centered on the screen
    int16_t cellSizeW = TFT_WIDTH / ray->map.w;
    int16_t cellSizeH = TFT_HEIGHT / ray->map.h;
    int16_t cellSize  = MIN(cellSizeW, cellSizeH);
    int16_t cellOffX  = (TFT_WIDTH - (ray->map.w * cellSize)) / 2;
    int16_t cellOffY  = (TFT_HEIGHT - (ray->map.h * cellSize)) / 2;

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
                            color = c234;
                            break;
                        }
                        case BG_DOOR_MISSILE:
                        {
                            color = c400;
                            break;
                        }
                        case BG_DOOR_ICE:
                        {
                            color = c004;
                            break;
                        }
                        case BG_DOOR_XRAY:
                        {
                            // Hide XRAY doors until the player gets the xrayLoadOut
                            if (ray->p.i.xrayLoadOut)
                            {
                                color = c020;
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
                        case BG_DOOR_KEY_B:
                        case BG_DOOR_KEY_C:
                        {
                            color = c440;
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

    // The player's location blinks, so draw it when appropriate
    if (ray->pauseBlink)
    {
        // Draw a circle for the player
        int16_t cX = cellOffX + FROM_FX(cellSize * ray->p.posX);
        int16_t cY = cellOffY + FROM_FX(cellSize * ray->p.posY);
        int16_t cR = cellSize / 2;
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
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

#define MAP_X_MARGIN 16
#define MAP_Y_MARGIN 36
#define MAP_Y_MIDGAP 24
#define MAP_SIZE     72

#define MAP_ROWS 2
#define MAP_COLS 3

#define WARP_POINT_INDENT 4

    // Save coordinates for where to draw warp lines
    int16_t warpCoords[NUM_MAPS][NUM_MAP_CORNERS][2];

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
                int16_t startY = MAP_Y_MARGIN + mapY * (MAP_SIZE + MAP_Y_MIDGAP);
                int16_t endX   = startX + MAP_SIZE;
                int16_t endY   = startY + MAP_SIZE;

                // Draw the black box, outlined
                fillDisplayArea(startX + 1, startY + 1, endX - 1, endY - 1, c000);
                drawRect(startX, startY, endX, endY, rayMapColors[mapId]);

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
}

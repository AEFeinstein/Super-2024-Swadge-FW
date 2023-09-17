//==============================================================================
// Includes
//==============================================================================

#include "ray_pause.h"

//==============================================================================
// Defines
//==============================================================================

#define PAUSE_BLINK_US 500000

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
        if (PB_START == evt.button && evt.down)
        {
            // Pause over, return to game
            ray->screen = RAY_GAME;
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
            if (ray->map.visitedTiles[(y * ray->map.w) + x])
            {
                // Get the cell type and pick a color depending on the type
                rayMapCellType_t type = ray->map.tiles[x][y].type;
                paletteColor_t color  = c000;
                if (CELL_IS_TYPE(type, BG | WALL))
                {
                    // All walls are the same
                    color = c212;
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
                            color = c123;
                            break;
                        }
                        case BG_DOOR_MISSILE:
                        {
                            color = c400;
                            break;
                        }
                        case BG_DOOR_ICE:
                        {
                            color = c040;
                            break;
                        }
                        case BG_DOOR_XRAY:
                        {
                            // TODO hide XRAY doors on the map?
                            color = c004;
                            break;
                        }
                        case BG_DOOR_SCRIPT:
                        {
                            color = c222;
                            break;
                        }
                        case BG_DOOR_KEY:
                        {
                            color = c330;
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
                    // All floors are the same
                    color = c111;
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
        int16_t cX = cellOffX + FROM_FX(cellSize * ray->posX);
        int16_t cY = cellOffY + FROM_FX(cellSize * ray->posY);
        int16_t cR = cellSize / 2;
        drawCircle(cX, cY, cR, c145);

        // Draw a line for the player's direction
        int16_t lineEndX = cX + FROM_FX(cR * ray->dirX);
        int16_t lineEndY = cY + FROM_FX(cR * ray->dirY);
        drawLine(cX, cY, lineEndX, lineEndY, c552, 0);
    }

    // Run a timer to blink things
    ray->pauseBlinkTimer += elapsedUs;
    if (ray->pauseBlinkTimer > PAUSE_BLINK_US)
    {
        ray->pauseBlinkTimer -= PAUSE_BLINK_US;
        ray->pauseBlink = !ray->pauseBlink;
    }
}

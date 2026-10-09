//==============================================================================
// Includes
//==============================================================================

#include "ray_pause.h"
#include "ray_tex_manager.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void rayPauseRenderLocalMap(ray_t* ray, uint32_t elapsedUs);
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
    globalMidiPlayerPauseAll();
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
                    globalMidiPlayerResumeAll();
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
    }

    //     if (ray->blink)
    //     {
    // #define TRIANGLE_OFFSET_X 20
    // #define TRIANGLE_OFFSET_Y 0
    //         drawTriangleOutlined(TFT_WIDTH - TRIANGLE_OFFSET_X - 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 4,
    //                              TFT_WIDTH - TRIANGLE_OFFSET_X - 4, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 10,
    //                              TFT_WIDTH - TRIANGLE_OFFSET_X - 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 16, c100,
    //                              c542);

    //         drawTriangleOutlined(TRIANGLE_OFFSET_X + 16, TFT_HEIGHT - TRIANGLE_OFFSET_Y - 4, TRIANGLE_OFFSET_X + 4,
    //                              TFT_HEIGHT - TRIANGLE_OFFSET_Y - 10, TRIANGLE_OFFSET_X + 16,
    //                              TFT_HEIGHT - TRIANGLE_OFFSET_Y - 16, c100, c542);
    //     }
}

/**
 * @brief Render the local map, a collection of tiles which are revealed as the player moves through the map
 *
 * @param ray The whole game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void rayPauseRenderLocalMap(ray_t* ray, uint32_t elapsedUs)
{
    const char* name = getRayMapMetadata(ray->p.mapId)->name;
    int16_t tWidth   = textWidth(&ray->ibm, name);
    drawText(&ray->ibm, c555, name, (TFT_WIDTH - tWidth) / 2, 2);

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
                    color = c444;
                }
                else if (CELL_IS_TYPE(type, BG | FLOOR))
                {
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

#include "soko_game.h"
#include "soko.h"

void gameLoop(int64_t elapsedUs)
{
    //logic

    //draw level
    drawTiles(soko->currentLevel);
}

//draw the background tiles of the level.
void drawTiles(sokoLevel_t* level)
{
    for (size_t x = 0; x < level->width; x++)
    {
        for (size_t y = 0; y < level->height; y++)
        {
            paletteColor_t color = c555;
            switch (level->tiles[x][y])
            {
                case SK_EMPTY:
                    color = c000;
                    break;
                case SK_WALL:
                    color = c111;
                    break;
                case SK_GOAL:
                    color = c141;
                    break;
                default:
                    break;
            }
            drawRect(x*10,y*10,x*10+10,y*10+10,color);
        }
    }
}
#include "soko_game.h"
#include "soko.h"
#include "soko_input.h"

void sokoTryPlayerMovement(soko_t*);

void gameLoop(soko_t* soko, int64_t elapsedUs)
{
    //logic
    
    sokoTryPlayerMovement(soko);

    //draw level
    drawTiles(&soko->currentLevel);
}


//Gameplay Logic
void sokoTryPlayerMovement(soko_t* soko)
{
    sokoEntity_t* player = &soko->currentLevel.entities[soko->currentLevel.playerIndex];
    if(soko->input.playerInputDeltaX != 0)
    {
        player->x += soko->input.playerInputDeltaX;
    }
    if(soko->input.playerInputDeltaY != 0)
    {
        player->y = player->y+soko->input.playerInputDeltaY;
    }
    printf("player: (%i, %i)\n",player->x,player->y);
}

//draw the background tiles of the level.
void drawTiles(sokoLevel_t* level)
{
    SETUP_FOR_TURBO();
    uint16_t s = 15;//scale
    uint16_t ox = (TFT_WIDTH/2)-((level->width)*s/2);
    uint16_t oy = (TFT_HEIGHT/2)-((level->height)*s/2);

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
            //Draw a square.
            //none of this matters it's all getting replaced with drawwsg later.
            for (size_t xd = ox+x*s; xd < ox+x*s+s; xd++)
            {
                for (size_t yd = oy+y*s; yd < oy+y*s+s; yd++)
                {
                    TURBO_SET_PIXEL(xd, yd, color);
                }
            }
            //draw outline around the square.
            //drawRect(ox+x*s,oy+y*s,ox+x*s+s,oy+y*s+s,color);
        }
    }

    for (size_t i = 0; i < level->entityCount; i++)
    {
        switch (level->entities[i].type)
        {
            case SKE_PLAYER:
                drawCircleFilled(ox+level->entities[i].x*s+s/2,oy+level->entities[i].y*s+s/2,s/2-1,c411);
                break;
            default:
                break;
        }
    }
    
}
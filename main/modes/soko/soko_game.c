#include "soko_game.h"
#include "soko.h"
#include "soko_gamerules.h"

/*
void sokoTryPlayerMovement(void);
sokoTile_t sokoGetTile(int, int);
bool sokoTryMoveEntityInDirection(sokoEntity_t*, int, int,uint16_t);
bool allCratesOnGoal(void);

*/
// sokoDirection_t sokoDirectionFromDelta(int, int);

// soko_t* s;
// sokoEntity_t* player;

soko_abs_t* soko_s;

void sokoInitGameBin(soko_abs_t* soko)
{
    printf("init sokoban game binary");

    soko_s = soko;
    soko_s->soko_player = &soko_s->currentLevel.entities[soko_s->currentLevel.playerIndex];

    soko->camX = soko_s->soko_player->x;
    soko->camY = soko_s->soko_player->y;

    sokoInitInput(&soko_s->input);

    soko->state = SKS_GAMEPLAY;

    sokoConfigGamemode(soko, soko->currentLevel.gameMode);
}

void sokoInitGame(soko_abs_t* soko)
{
    printf("init sokobon game.\n");

    // Configure conveninence pointers.
    soko_s              = soko;
    soko_s->soko_player = &soko_s->currentLevel.entities[soko_s->currentLevel.playerIndex];

    // reset camera
    soko->camX = soko_s->soko_player->x;
    soko->camY = soko_s->soko_player->y;

    sokoInitInput(&soko_s->input);

    soko->state = SKS_GAMEPLAY;

    sokoConfigGamemode(soko, SOKO_OVERWORLD);

    // sokoConfigGamemode(soko,SOKO_EULER);
}

void sokoInitNewLevel(soko_abs_t* soko, soko_var_t variant)
{
    printf("Init New Level.\n");

    soko_s              = soko;
    soko_s->soko_player = &soko_s->currentLevel.entities[soko_s->currentLevel.playerIndex];
    sokoInitInput(&soko_s->input);

    // set gameplay settings from default settings, if we want powerups or whatever that adjusts them, or have a state
    // machine.
    soko_s->maxPush = 0; // set to 1 for "traditional" sokoban.

    soko->state = SKS_GAMEPLAY;

    sokoConfigGamemode(soko, variant);
}

/*
void gameLoop(int64_t elapsedUs)
{
    if(s->state == SKS_GAMEPLAY)
    {
        //logic
        sokoTryPlayerMovement();

        //victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        s->allCratesOnGoal = allCratesOnGoal();
        if(s->allCratesOnGoal){
            s->state = SKS_VICTORY;
        }
        //draw level
        drawTiles(&s->currentLevel);

    }else if(s->state == SKS_VICTORY)
    {
        //check for input for exit/next level.
        drawTiles(&s->currentLevel);
    }


    //DEBUG PLACEHOLDER:
            // Render the time to a string
            char str[16] = {0};
            int16_t tWidth;
            if(!s->allCratesOnGoal)
            {
                snprintf(str, sizeof(str) - 1, "sokoban");
                // Measure the width of the time string
                tWidth = textWidth(&s->ibm, str);
                // Draw the time string to the display, centered at (TFT_WIDTH / 2)
                drawText(&s->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
            }else
            {
                snprintf(str, sizeof(str) - 1, "sokasuccess");
                // Measure the width of the time string
                tWidth = textWidth(&s->ibm, str);
                // Draw the time string to the display, centered at (TFT_WIDTH / 2)
                drawText(&s->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
            }

}


//Gameplay Logic
void sokoTryPlayerMovement()
{

    if(s->input.playerInputDeltaX == 0 && s->input.playerInputDeltaY == 0)
    {
        return;
    }

    sokoTryMoveEntityInDirection(player,s->input.playerInputDeltaX,s->input.playerInputDeltaY,0);
}


bool sokoTryMoveEntityInDirection(sokoEntity_t* entity, int dx, int dy, uint16_t push)
{
    //prevent infitnite loop where you push yourself nowhere.
    if(dx == 0 && dy == 0 )
    {
        return false;
    }

    //maxiumum number of crates we can push. Traditional sokoban has a limit of one. I prefer infinite for challenges.
    if(s->maxPush != 0 && push>s->maxPush)
    {
        return false;
    }

    int px = entity->x+dx;
    int py = entity->y+dy;
    sokoTile_t nextTile = sokoGetTile(px,py);

    if(nextTile == SKT_FLOOR || nextTile == SKT_GOAL || nextTile == SKT_EMPTY)
    {
        //Is there an entity at this position?
        for (size_t i = 0; i < s->currentLevel.entityCount; i++)
        {
            //is pushable.
            if(s->currentLevel.entities[i].type == SKE_CRATE)
            {
                if(s->currentLevel.entities[i].x == px && s->currentLevel.entities[i].y == py)
                {
                    if(sokoTryMoveEntityInDirection(&s->currentLevel.entities[i],dx,dy,push+1))
                    {
                        entity->x += dx;
                        entity->y += dy;
                        entity->facing = sokoDirectionFromDelta(dx,dy);
                        return true;
                    }else{
                        //can't push? can't move.
                        return false;
                    }

                }
            }

        }

        //No wall in front of us and nothing to push, we can move.
        entity->x += dx;
        entity->y += dy;
        entity->facing = sokoDirectionFromDelta(dx,dy);
        return true;
    }

    return false;
}

//draw the tiles (and entities, for now) of the level.
void drawTiles(sokoLevel_t* level)
{
    SETUP_FOR_TURBO();
    uint16_t scale = level->levelScale;
    uint16_t ox = (TFT_WIDTH/2)-((level->width)*scale/2);
    uint16_t oy = (TFT_HEIGHT/2)-((level->height)*scale/2);

    for (size_t x = 0; x < level->width; x++)
    {
        for (size_t y = 0; y < level->height; y++)
        {
            paletteColor_t color = cTransparent;
            switch (level->tiles[x][y])
            {
                case SKT_FLOOR:
                    color = c444;
                    break;
                case SKT_WALL:
                    color = c111;
                    break;
                case SKT_GOAL:
                    color = c141;
                    break;
                case SKT_EMPTY:
                    color = cTransparent;
                default:
                    break;
            }

            //Draw a square.
            //none of this matters it's all getting replaced with drawwsg later.
            if(color != cTransparent){
                for (size_t xd = ox+x*scale; xd < ox+x*scale+scale; xd++)
                {
                    for (size_t yd = oy+y*scale; yd < oy+y*scale+scale; yd++)
                    {
                        TURBO_SET_PIXEL(xd, yd, color);
                    }
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
                switch(level->entities[i].facing){
                    case SKD_UP:
                        drawWsg(&s->playerUpWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
                        break;
                    case SKD_RIGHT:
                        drawWsg(&s->playerRightWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
                        break;
                    case SKD_LEFT:
                        drawWsg(&s->playerLeftWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
                        break;
                    case SKD_DOWN:
                    default:
                        drawWsg(&s->playerDownWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
                        break;
                }

                break;
            case SKE_CRATE:
                 drawWsg(&s->crateWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
            case SKE_NONE:
            default:
                break;
        }
    }

}

bool allCratesOnGoal()
{
    for (size_t i = 0; i < s->currentLevel.entityCount; i++)
    {
        if(s->currentLevel.entities[i].type == SKE_CRATE)
        {
            if(s->currentLevel.tiles[s->currentLevel.entities[i].x][s->currentLevel.entities[i].y] != SKT_GOAL)
            {
                return false;
            }
        }
    }

    return true;
}


sokoDirection_t sokoDirectionFromDelta(int dx,int dy)
{
    if(dx > 0 && dy == 0)
    {
        return SKD_RIGHT;
    }else  if(dx < 0 && dy == 0)
    {
        return SKD_LEFT;
    }else  if(dx == 0 && dy < 0)
    {
        return SKD_UP;
    }else  if(dx == 0 && dy > 0)
    {
        return SKD_DOWN;
    }

    return SKD_NONE;
}
sokoTile_t sokoGetTile(int x, int y)
{
    if(x<0 || x >= s->currentLevel.width)
    {
        return SKT_WALL;
    }
    if(y<0 || y >= s->currentLevel.height)
    {
        return SKT_WALL;
    }

    return s->currentLevel.tiles[x][y];
}
*/
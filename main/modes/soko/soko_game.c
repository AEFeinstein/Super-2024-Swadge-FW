#include "soko_game.h"
#include "soko.h"

void sokoTryPlayerMovement(void);
sokoTile_t sokoGetTile(int, int);
bool sokoTryMoveEntityInDirection(sokoEntity_t*, int, int,uint16_t);
bool allCratesOnGoal(void);

soko_t* s;
sokoEntity_t* player;

void sokoInitGame(soko_t* soko)
{
    printf("init sokobon game.");
    //set gameplay settings from default settings, if we want powerups or whatever.
    s = soko;
    s->maxPush = 0;//set to 1 for "traditional" sokoban.
    player = &s->currentLevel.entities[s->currentLevel.playerIndex];
}

void gameLoop(int64_t elapsedUs)
{
    //logic
    sokoTryPlayerMovement();

    s->allCratesOnGoal = allCratesOnGoal();

    //draw level
    drawTiles(&s->currentLevel);

    //debug allcrates 

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

    //maxiumum number of crates we can push. Traditional sokoban has a limit of one. I prefer infinite.
    if(s->maxPush != 0 && push>s->maxPush)
    {
        return false;
    }

    int px = entity->x+dx;
    int py = entity->y+dy;
    sokoTile_t nextTile = sokoGetTile(px,py);

    if(nextTile == SK_EMPTY || nextTile == SK_GOAL)
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
                        return true;
                    }else{
                        //can't push? can't move.
                        return false;
                    }
                   
                }
            }
            
        }
        
        //nothing to push, then we can move.
        entity->x += dx;
        entity->y += dy;
        return true;
    }
    
    return false;
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
            case SKE_CRATE:
                drawCircleFilled(ox+level->entities[i].x*s+s/2,oy+level->entities[i].y*s+s/2,s/2-1,c441);
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
            if(s->currentLevel.tiles[s->currentLevel.entities[i].x][s->currentLevel.entities[i].y] != SK_GOAL)
            {
                return false;
            }
        }
    }

    return true;
}

sokoTile_t sokoGetTile(int x, int y)
{
    if(x<0 || x >= s->currentLevel.width)
    {
        return SK_WALL;
    }
    if(y<0 || y >= s->currentLevel.height)
    {
        return SK_WALL;
    }
     
    return s->currentLevel.tiles[x][y];
}
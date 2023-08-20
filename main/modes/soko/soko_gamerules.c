#include "soko_game.h"
#include "soko.h"
#include "soko_gamerules.h"



void sokoConfigGamemode(soko_abs_t* gamestate, soko_var_t variant) //This should be called when you reload a level to make sure game rules are correct
{
    if(1) //standard gamemode. Check 'variant' variable
    {
        gamestate->gameLoopFunc = absSokoGameLoop;
        gamestate->sokoTryPlayerMovementFunc = absSokoTryPlayerMovement;
        gamestate->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        gamestate->drawTilesFunc = absSokoDrawTiles;
        gamestate->allCratesOnGoalFunc = absSokoAllCratesOnGoal;
        gamestate->sokoGetTileFunc = absSokoGetTile;
    }
    //add conditional for alternative variants
}

void absSokoGameLoop(soko_abs_t *self, int64_t elapsedUs)
{
    if(self->state == SKS_GAMEPLAY)
    {
        //logic
        self->sokoTryPlayerMovementFunc(self);

        //victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        self->allCratesOnGoal = self->allCratesOnGoalFunc(self);
        if(self->allCratesOnGoal){
            self->state = SKS_VICTORY;
        }
        //draw level
        self->drawTilesFunc(self, &self->currentLevel);

    }else if(self->state == SKS_VICTORY)
    {
        //check for input for exit/next level.
        self->drawTilesFunc(self, &self->currentLevel);
    }


    //DEBUG PLACEHOLDER:
            // Render the time to a string
            char str[16] = {0};
            int16_t tWidth;
            if(!self->allCratesOnGoal)
            {
                snprintf(str, sizeof(str) - 1, "sokoban");
                // Measure the width of the time string
                tWidth = textWidth(&self->ibm, str);
                // Draw the time string to the display, centered at (TFT_WIDTH / 2)
                drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
            }else
            {
                snprintf(str, sizeof(str) - 1, "sokasuccess");
                // Measure the width of the time string
                tWidth = textWidth(&self->ibm, str);
                // Draw the time string to the display, centered at (TFT_WIDTH / 2)
                drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
            }
        
}


//Gameplay Logic
void absSokoTryPlayerMovement(soko_abs_t *self)
{
    
    if(self->input.playerInputDeltaX == 0 && self->input.playerInputDeltaY == 0)
    {
        return;
    }

    self->sokoTryMoveEntityInDirectionFunc(self, self->soko_player,self->input.playerInputDeltaX,self->input.playerInputDeltaY,0);
}


bool absSokoTryMoveEntityInDirection(soko_abs_t *self, sokoEntity_t* entity, int dx, int dy, uint16_t push)
{
    //prevent infitnite loop where you push yourself nowhere.
    if(dx == 0 && dy == 0 )
    {
        return false;
    }

    //maxiumum number of crates we can push. Traditional sokoban has a limit of one. I prefer infinite for challenges.
    if(self->maxPush != 0 && push>self->maxPush)
    {
        return false;
    }

    int px = entity->x+dx;
    int py = entity->y+dy;
    sokoTile_t nextTile = self->sokoGetTileFunc(self,px,py);

    if(nextTile == SKT_FLOOR || nextTile == SKT_GOAL || nextTile == SKT_EMPTY)
    {
        //Is there an entity at this position?
        for (size_t i = 0; i < self->currentLevel.entityCount; i++)
        {
            //is pushable.
            if(self->currentLevel.entities[i].type == SKE_CRATE)
            {
                if(self->currentLevel.entities[i].x == px && self->currentLevel.entities[i].y == py)
                {
                    if(self->sokoTryMoveEntityInDirectionFunc(self, &self->currentLevel.entities[i],dx,dy,push+1))
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
        
        //No wall in front of us and nothing to push, we can move.
        entity->x += dx;
        entity->y += dy;
        return true;
    }
    
    return false;
}

//draw the tiles (and entities, for now) of the level.
void absSokoDrawTiles(soko_abs_t *self, sokoLevel_t* level)
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
                // drawCircleFilled(ox+level->entities[i].x*scale+scale/2,oy+level->entities[i].y*scale+scale/2,scale/2-1,c411);
                drawWsg(&self->playerWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
                break;
            case SKE_CRATE:
                //drawCircleFilled(ox+level->entities[i].x*scale+scale/2,oy+level->entities[i].y*scale+scale/2,scale/2-1,c441);
                 drawWsg(&self->crateWSG,ox+level->entities[i].x*scale,oy+level->entities[i].y*scale,false,false,0);
            case SKE_NONE:
            default:
                break;
        }
    }
    
}

bool absSokoAllCratesOnGoal(soko_abs_t *self)
{
    for (size_t i = 0; i < self->currentLevel.entityCount; i++)
    {
        if(self->currentLevel.entities[i].type == SKE_CRATE)
        {
            if(self->currentLevel.tiles[self->currentLevel.entities[i].x][self->currentLevel.entities[i].y] != SKT_GOAL)
            {
                return false;
            }
        }
    }

    return true;
}

sokoTile_t absSokoGetTile(soko_abs_t *self, int x, int y)
{
    if(x<0 || x >= self->currentLevel.width)
    {
        return SKT_WALL;
    }
    if(y<0 || y >= self->currentLevel.height)
    {
        return SKT_WALL;
    }
     
    return self->currentLevel.tiles[x][y];
}
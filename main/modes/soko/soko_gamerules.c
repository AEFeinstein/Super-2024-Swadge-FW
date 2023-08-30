#include "soko_game.h"
#include "soko.h"
#include "soko_gamerules.h"

#include "shapes.h"

// clang-format off
// True if the entity CANNOT go on the tile
bool sokoEntityTileCollision[5][8] = {
    // Empty, //floor  //wall   //goal    //portal  //l-emit  //l-receive //walked
    {true, false, true, false, false, false, false, false}, // SKE_NONE
    {true, false, true, false, false, false, false, true},  // PLAYER
    {true, false, true, false, false, false, false, false}, // CRATE
    {true, false, true, false, false, false, false, false}, // LASER
    {true, false, true, false, false, false, false, false}, // STICKY CRATE
};
// clang-format on

uint64_t victoryDanceTimer;

void sokoConfigGamemode(
    soko_abs_t* gamestate,
    soko_var_t variant) // This should be called when you reload a level to make sure game rules are correct
{
    gamestate->currentTheme = &gamestate->sokoDefaultTheme;
    if (variant == SOKO_CLASSIC) // standard gamemode. Check 'variant' variable
    {
        printf("Config Soko to Classic\n");
        gamestate->maxPush                          = 1; // set to 1 for "traditional" sokoban.
        gamestate->gameLoopFunc                     = absSokoGameLoop;
        gamestate->sokoTryPlayerMovementFunc        = absSokoTryPlayerMovement;
        gamestate->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        gamestate->drawTilesFunc                    = absSokoDrawTiles;
        gamestate->isVictoryConditionFunc           = absSokoAllCratesOnGoal;
        gamestate->sokoGetTileFunc                  = absSokoGetTile;
    }
    else if (variant == SOKO_EULER) // standard gamemode. Check 'variant' variable
    {
        printf("Config Soko to Euler\n");
        gamestate->maxPush                          = 0; // set to 0 for infinite push.
        gamestate->gameLoopFunc                     = absSokoGameLoop;
        gamestate->sokoTryPlayerMovementFunc        = eulerSokoTryPlayerMovement;
        gamestate->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        gamestate->drawTilesFunc                    = absSokoDrawTiles;
        gamestate->isVictoryConditionFunc           = eulerNoUnwalkedFloors;
        gamestate->sokoGetTileFunc                  = absSokoGetTile;
    }
    else if (variant == SOKO_OVERWORLD)
    {
        printf("Config Soko to Overworld\n");
        gamestate->maxPush                          = 0; // set to 0 for infinite push.
        gamestate->gameLoopFunc                     = overworldSokoGameLoop;
        gamestate->sokoTryPlayerMovementFunc        = absSokoTryPlayerMovement;
        gamestate->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        gamestate->drawTilesFunc                    = absSokoDrawTiles;
        gamestate->isVictoryConditionFunc           = overworldPortalEntered;
        gamestate->sokoGetTileFunc                  = absSokoGetTile;

        gamestate->currentTheme = &gamestate->overworldTheme;

    }
    else if (variant == SOKO_LASERBOUNCE)
    {
        printf("Config Soko to Laser Bounce\n");
        gamestate->maxPush                          = 0; // set to 0 for infinite push.
        gamestate->gameLoopFunc                     = laserBounceSokoGameLoop;
        gamestate->sokoTryPlayerMovementFunc        = absSokoTryPlayerMovement;
        gamestate->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        gamestate->drawTilesFunc                    = absSokoDrawTiles;
        gamestate->isVictoryConditionFunc           = absSokoAllCratesOnGoal;
        gamestate->sokoGetTileFunc                  = absSokoGetTile;
    }

    // add conditional for alternative variants
}

void laserBounceSokoGameLoop(soko_abs_t* self, int64_t elapsedUs)
{
    if (self->state == SKS_GAMEPLAY)
    {
        // logic
        self->sokoTryPlayerMovementFunc(self);

        // victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        // todo: rename to isVictory or such.
        self->allCratesOnGoal = self->isVictoryConditionFunc(self);
        if (self->allCratesOnGoal)
        {
            self->state       = SKS_VICTORY;
            victoryDanceTimer = 0;
        }
        // draw level
        self->drawTilesFunc(self, &self->currentLevel);
        drawLaserFromEntity(self, self->soko_player);
    }
    else if (self->state == SKS_VICTORY)
    {
        // check for input for exit/next level.
        self->drawTilesFunc(self, &self->currentLevel);
        victoryDanceTimer += elapsedUs;
        if (victoryDanceTimer > SOKO_VICTORY_TIMER_US)
        {
            self->loadNewLevelIndex = 0;
            self->loadNewLevelFlag  = true;
            self->screen            = SOKO_LOADNEWLEVEL;
        }
    }

    // DEBUG PLACEHOLDER:
    //  Render the time to a string
    char str[16] = {0};
    int16_t tWidth;
    if (!self->allCratesOnGoal)
    {
        snprintf(str, sizeof(str) - 1, "sokoban");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "sokasuccess");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    sharedGameLoop(self);
}

void absSokoGameLoop(soko_abs_t* self, int64_t elapsedUs)
{
    if (self->state == SKS_GAMEPLAY)
    {
        // logic
        self->sokoTryPlayerMovementFunc(self);

        // victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        // todo: rename to isVictory or such.
        self->allCratesOnGoal = self->isVictoryConditionFunc(self);
        if (self->allCratesOnGoal)
        {
            self->state       = SKS_VICTORY;
            victoryDanceTimer = 0;
        }
        // draw level
        self->drawTilesFunc(self, &self->currentLevel);
    }
    else if (self->state == SKS_VICTORY)
    {
        // check for input for exit/next level.
        self->drawTilesFunc(self, &self->currentLevel);
        victoryDanceTimer += elapsedUs;
        if (victoryDanceTimer > SOKO_VICTORY_TIMER_US)
        {
            self->loadNewLevelIndex = 0;
            self->loadNewLevelFlag  = true;
            self->screen            = SOKO_LOADNEWLEVEL;
        }
    }

    // DEBUG PLACEHOLDER:
    char str[16] = {0};
    int16_t tWidth;
    if (!self->allCratesOnGoal)
    {
        snprintf(str, sizeof(str) - 1, "sokoban");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "sokasuccess");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    sharedGameLoop(self);
}

void sharedGameLoop(soko_abs_t* self)
{
    if (self->input.restartLevel)
    {
        restartCurrentLevel(self);
    }
    else if (self->input.exitToOverworld)
    {
        exitToOverworld(self);
    }
}

// Gameplay Logic
void absSokoTryPlayerMovement(soko_abs_t* self)
{
    if (self->input.playerInputDeltaX == 0 && self->input.playerInputDeltaY == 0)
    {
        return;
    }

    self->sokoTryMoveEntityInDirectionFunc(self, self->soko_player, self->input.playerInputDeltaX,
                                           self->input.playerInputDeltaY, 0);
}

bool absSokoTryMoveEntityInDirection(soko_abs_t* self, sokoEntity_t* entity, int dx, int dy, uint16_t push)
{
    // prevent infitnite loop where you push yourself nowhere.
    if (dx == 0 && dy == 0)
    {
        return false;
    }

    // maxiumum number of crates we can push. Traditional sokoban has a limit of one. I prefer infinite for challenges.
    if (self->maxPush != 0 && push > self->maxPush)
    {
        return false;
    }

    int px              = entity->x + dx;
    int py              = entity->y + dy;
    sokoTile_t nextTile = self->sokoGetTileFunc(self, px, py);

    // when this is false, we CAN move. True for Collision.
    if (!sokoEntityTileCollision[entity->type][nextTile])
    {
        // Is there an entity at this position?
        for (size_t i = 0; i < self->currentLevel.entityCount; i++)
        {
            // is pushable.
            if (self->currentLevel.entities[i].type == SKE_CRATE
                || self->currentLevel.entities[i].type == SKE_STICKY_CRATE)
            {
                if (self->currentLevel.entities[i].x == px && self->currentLevel.entities[i].y == py)
                {
                    if (self->sokoTryMoveEntityInDirectionFunc(self, &self->currentLevel.entities[i], dx, dy, push + 1))
                    {
                        printf("pushing entity");
                        entity->x += dx;
                        entity->y += dy;
                        entity->facing = sokoDirectionFromDelta(dx, dy);
                        return true;//if entities overlap, we should not break here?
                    }
                    else
                    {
                        // can't push? can't move.
                        return false;
                    }
                }
            }
        }

        // No wall in front of us and nothing to push, we can move.
        entity->x += dx;
        entity->y += dy;
        entity->facing = sokoDirectionFromDelta(dx, dy);
        return true;
    } // all other floor types invalid. Be careful when we add tile types in different rule sets.

    return false;
}

// draw the tiles (and entities, for now) of the level.
void absSokoDrawTiles(soko_abs_t* self, sokoLevel_t* level)
{
    uint16_t scale = level->levelScale;
    //These are in level space (not pixels) and must be within bounds of currentLevel.tiles.
    int16_t screenMinX,screenMaxX,screenMinY,screenMaxY;
    //offsets.
    uint16_t ox, oy;

    // Recalculate Camera Position
    // todo: extract to a function if we end up with different draw functions. Part of future pointer refactor.
    if (self->camEnabled)
    {
        //calculate camera position. Shift if needed. Cam position was initiated to player position.
        if (self->soko_player->x > self->camX + self->camPadExtentX)
        {
            self->camX = self->soko_player->x - self->camPadExtentX;
        }
        else if (self->soko_player->x < self->camX - self->camPadExtentX)
        {
            self->camX = self->soko_player->x + self->camPadExtentX;
        }
        else if (self->soko_player->y > self->camY + self->camPadExtentY)
        {
            self->camY = self->soko_player->y - self->camPadExtentY;
        }
        else if (self->soko_player->y < self->camY - self->camPadExtentY)
        {
            self->camY = self->soko_player->y + self->camPadExtentY;
        }

        //calculate offsets
        ox = -self->camX * scale + (TFT_WIDTH / 2);
        oy = -self->camY * scale + (TFT_HEIGHT / 2);

        //calculate out of bounds draws. todo: make tenery operators.
        screenMinX = self->camX - self->camWidth/2 - 1;
        if(screenMinX < 0){
            screenMinX = 0;
        }
        screenMaxX = self->camX + self->camWidth/2 + 1;
        if(screenMaxX > level->width){
            screenMaxX = level->width;
        }
        screenMinY = self->camY - self->camHeight/2 - 1;
        if(screenMinY < 0){
            screenMinY = 0;
        }
        screenMaxY = self->camY + self->camHeight/2 + 1;
        if(screenMaxY > level->height){
            screenMaxY = level->height;
        }
    } else
    {//no camera
        //calculate offsets to center the level.
        ox = (TFT_WIDTH / 2) - ((level->width) * scale / 2);
        oy = (TFT_HEIGHT / 2) - ((level->height) * scale / 2);

        //bounds are just the level.
        screenMinX = 0; 
        screenMaxX = level->width;
        screenMinY = 0;
        screenMaxY = level->height;
    }

    SETUP_FOR_TURBO();
    
   // uint16_t DEBUG_DRAW_COUNT=0;

    for (size_t x = screenMinX; x < screenMaxX; x++)
    {
        for (size_t y = screenMinY; y < screenMaxY; y++)
        {
            paletteColor_t color = cTransparent;
            switch (level->tiles[x][y])
            {
                case SKT_FLOOR:
                    color = self->currentTheme->floorColor;
                    break;
                case SKT_WALL:
                    color = self->currentTheme->wallColor;
                    break;
                case SKT_GOAL:
                    color = c141;
                    break;
                case SKT_FLOOR_WALKED:
                    color = c334;
                    break;
                case SKT_EMPTY:
                    color = cTransparent;
                    break;
                case SKT_PORTAL:
                    color = c440;
                    break;
                default:
                    break;
            }

            // Draw a square.
            // none of this matters it's all getting replaced with drawwsg later.
            if (color != cTransparent)
            {
                for (size_t xd = ox + x * scale; xd < ox + x * scale + scale; xd++)
                {
                    for (size_t yd = oy + y * scale; yd < oy + y * scale + scale; yd++)
                    {
                        TURBO_SET_PIXEL(xd, yd, color);
                    }
                }
            }
            //DEBUG_DRAW_COUNT++;
            // draw outline around the square.
            // drawRect(ox+x*s,oy+y*s,ox+x*s+s,oy+y*s+s,color);
        }
    }

    for (size_t i = 0; i < level->entityCount; i++)
    {
        //don't bother drawing off screen
        if(level->entities[i].x >= screenMinX && level->entities[i].x <= screenMaxX && level->entities[i].y >= screenMinY && level->entities[i].y <= screenMaxY)
        {
            switch (level->entities[i].type)
            {
                case SKE_PLAYER:
                    switch (level->entities[i].facing)
                    {
                        case SKD_UP:
                            drawWsg(&self->currentTheme->playerUpWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        case SKD_RIGHT:
                            drawWsg(&self->currentTheme->playerRightWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        case SKD_LEFT:
                            drawWsg(&self->currentTheme->playerLeftWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        case SKD_DOWN:
                        default:
                            drawWsg(&self->currentTheme->playerDownWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                    }

                    break;
                case SKE_CRATE:
                    drawWsg(&self->currentTheme->crateWSG, ox + level->entities[i].x * scale, oy + level->entities[i].y * scale, false,
                            false, 0);
                    break;
                case SKE_STICKY_CRATE:
                    drawWsg(&self->currentTheme->stickyCrateWSG, ox + level->entities[i].x * scale, oy + level->entities[i].y * scale,
                            false, false, 0);
                    break;
                case SKE_NONE:
                default:
                    break;
            }
           // DEBUG_DRAW_COUNT++;
        }
    }
}

/*
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
                //
drawCircleFilled(ox+level->entities[i].x*scale+scale/2,oy+level->entities[i].y*scale+scale/2,scale/2-1,c411);
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
*/
bool absSokoAllCratesOnGoal(soko_abs_t* self)
{
    //printf("Victory Enter\n");
    for (size_t i = 0; i < self->currentLevel.entityCount; i++)
    {
        //printf("Loop Enter ");
        if (self->currentLevel.entities[i].type == SKE_CRATE)
        {
            //printf("Crate Found ");
            if (self->currentLevel.tiles[self->currentLevel.entities[i].x][self->currentLevel.entities[i].y]
                != SKT_GOAL)
            {
                //printf("Crate Off Goal");
                return false;
            }
        }
        //printf("\n");
    }
    //printf("Victory True\n");
    return true;
}

sokoTile_t absSokoGetTile(soko_abs_t* self, int x, int y)
{
    if (x < 0 || x >= self->currentLevel.width)
    {
        return SKT_WALL;
    }
    if (y < 0 || y >= self->currentLevel.height)
    {
        return SKT_WALL;
    }

    return self->currentLevel.tiles[x][y];
}

sokoDirection_t sokoDirectionFromDelta(int dx, int dy)
{
    if (dx > 0 && dy == 0)
    {
        return SKD_RIGHT;
    }
    else if (dx < 0 && dy == 0)
    {
        return SKD_LEFT;
    }
    else if (dx == 0 && dy < 0)
    {
        return SKD_UP;
    }
    else if (dx == 0 && dy > 0)
    {
        return SKD_DOWN;
    }

    return SKD_NONE;
}

sokoVec_t sokoGridToPix(soko_abs_t* self, sokoVec_t grid) // Convert grid position to screen pixel position
{
    sokoVec_t retVec;
    SETUP_FOR_TURBO();
    uint16_t scale
        = self->currentLevel
              .levelScale; //@todo These should be in constants, but too lazy to change all references at the moment.
    uint16_t ox = (TFT_WIDTH / 2) - ((self->currentLevel.width) * scale / 2);
    uint16_t oy = (TFT_HEIGHT / 2) - ((self->currentLevel.height) * scale / 2);
    retVec.x    = ox + scale * grid.x + scale / 2;
    retVec.y    = oy + scale * grid.y + scale / 2;
    return retVec;
}

void drawLaserFromEntity(soko_abs_t* self, sokoEntity_t* emitter)
{
    sokoCollision_t impactSpot = sokoBeamImpact(self, self->soko_player);
    // printf("Player Pos: x:%d,y:%d Facing:%d Impact Result: x:%d,y:%d, Flag:%d
    // Index:%d\n",self->soko_player->x,self->soko_player->y,self->soko_player->facing,impactSpot.x,impactSpot.y,impactSpot.entityFlag,impactSpot.entityIndex);
    sokoVec_t playerGrid, impactGrid;
    playerGrid.x        = emitter->x;
    playerGrid.y        = emitter->y;
    impactGrid.x        = impactSpot.x;
    impactGrid.y        = impactSpot.y;
    sokoVec_t playerPix = sokoGridToPix(self, playerGrid);
    sokoVec_t impactPix = sokoGridToPix(self, impactGrid);
    drawLine(playerPix.x, playerPix.y, impactPix.x, impactPix.y, c500, 0);
}

int sokoBeamImpactRecursive(soko_abs_t* self,int emitter_x, int emitter_y, sokoDirection_t emitterDir, sokoEntity_t* rootEmitter);

void sokoDoBeam(soko_abs_t* self)
{
    bool receiverImpact;
    for(int entInd = 0; entInd < self->currentLevel.entityCount; entInd++)
    {
        if(self->currentLevel.entities[entInd].type == SKE_LASER_EMIT_UP)
        {
            self->currentLevel.entities[entInd].properties->targetCount = 0;
            receiverImpact = sokoBeamImpactRecursive(self, self->currentLevel.entities[entInd].x, self->currentLevel.entities[entInd].y, self->currentLevel.entities[entInd].type, &self->currentLevel.entities[entInd]);
        }
    }
}

bool sokoLaserTileCollision(sokoTile_t testTile)
{
    switch(testTile)
    {
        case SKT_EMPTY:
            return false;
        case SKT_FLOOR:
            return false;
        case SKT_WALL:
            return true;
        case SKT_GOAL:
            return false;
        case SKT_PORTAL:
            return false;
        case SKT_FLOOR_WALKED:
            return false;
        case SKT_NO_WALK:
            return false;
        default:
            return false;
    }
}

bool sokoLaserEntityCollision(sokoEntityType_t testEntity)
{
    switch(testEntity) //Anything that doesn't unconditionally pass should return true
    {
        case SKE_NONE:
            return false;
        case SKE_PLAYER:
            return false;
        case SKE_CRATE:
            return true;
        case SKE_LASER_90:
            return true;
        case SKE_STICKY_CRATE:
            return true;
        case SKE_WARP:
            return false;
        case SKE_BUTTON:
            return false;
        case SKE_LASER_EMIT_UP:
            return true;
        case SKE_LASER_RECEIVE_OMNI:
            return true;
        case SKE_LASER_RECEIVE:
            return true;
        case SKE_GHOST:
            return true;
        default:
            return false;
    }
}

sokoDirection_t sokoRedirectDir(sokoDirection_t emitterDir, bool inverted)
{
    switch(emitterDir)
    {
        case SKD_UP:
            return inverted ? SKD_LEFT : SKD_RIGHT;
        case SKD_DOWN:
            return inverted ? SKD_RIGHT : SKD_LEFT;
        case SKD_RIGHT:
            return inverted ? SKD_DOWN : SKD_UP;
        case SKD_LEFT:
            return inverted ? SKD_UP : SKD_DOWN;
        default:
            return SKD_NONE;
    }
}

int sokoBeamImpactRecursive(soko_abs_t* self,int emitter_x, int emitter_y, sokoDirection_t emitterDir, sokoEntity_t* rootEmitter)
{
    sokoDirection_t dir = emitterDir;
    sokoVec_t projVec   = {0, 0};
    sokoVec_t emitVec   = {emitter_x, emitter_y};
    switch (dir)
    {
        case SKD_DOWN:
            projVec.y = 1;
            break;
        case SKD_UP:
            projVec.y = -1;
            break;
        case SKD_LEFT:
            projVec.x = -1;
            break;
        case SKD_RIGHT:
            projVec.x = 1;
            break;
        default:
            projVec.y = -1;
            break;
            // return base entity position
    }

    // Iterate over tiles in ray to edge of level
    sokoVec_t testPos = sokoAddCoord(emitVec, projVec);
    int entityCount   = self->currentLevel.entityCount;
    // todo: make first pass pack a statically allocated array with only the entities in the path of the laser.


    int16_t possibleSquares = 0;
    if (dir == SKD_RIGHT) // move these checks into the switch statement
    {
        possibleSquares = self->currentLevel.width - emitVec.x; // Up to and including far wall
    }
    if (dir == SKD_LEFT)
    {
        possibleSquares = emitVec.x + 1;
    }
    if (dir == SKD_UP)
    {
        possibleSquares = emitVec.y + 1;
    }
    if (dir == SKD_DOWN)
    {
        possibleSquares = self->currentLevel.height - emitVec.y;
    }

    int tileCollFlag, entCollFlag, entCollInd;
    tileCollFlag = entCollFlag = entCollInd = 0;

    bool retVal;
    // printf("emitVec(%d,%d)",emitVec.x,emitVec.y);
    // printf("projVec:(%d,%d) possibleSquares:%d ",projVec.x,projVec.y,possibleSquares);

    for (int n = 0; n < possibleSquares; n++)
    {
        sokoTile_t posTile = absSokoGetTile(self, testPos.x, testPos.y);
        // printf("|n:%d,posTile:(%d,%d):%d|",n,testPos.x,testPos.y,posTile);
        if (sokoLaserTileCollision(posTile))
        {
            tileCollFlag = 1;
            break;
        }
        for (int m = 0; m < entityCount; m++) // iterate over tiles/entities to check for laser collision. First pass
                                              // finds everything in the path of the
        {
            sokoEntity_t candidateEntity = self->currentLevel.entities[m];
            // printf("|m:%d;CE:(%d,%d)%d",m,candidateEntity.x,candidateEntity.y,candidateEntity.type);
            if (candidateEntity.x == testPos.x && candidateEntity.y == testPos.y)
            {
                // printf(";POSMATCH;Coll:%d",entityCollision[candidateEntity.type]);
                if (sokoLaserEntityCollision(candidateEntity.type))
                {
                    entCollFlag = 1;
                    entCollInd  = m;
                    // printf("|");
                    break;
                }
            }
            // printf("|");
        }
        sokoEntityProperties_t* entProps = rootEmitter->properties;
        if (tileCollFlag)
        {
            entProps->targetX[entProps->targetCount] = testPos.x; //Pack target properties with every impacted position.
            entProps->targetY[entProps->targetCount] = testPos.y; 
            entProps->targetCount++;
        }
        if (entCollFlag)
        {
            sokoEntityType_t entType = self->currentLevel.entities[entCollInd].type;
            
            entProps->targetX[entProps->targetCount] = testPos.x; //Pack target properties with every impacted entity.
            entProps->targetY[entProps->targetCount] = testPos.y; //If there's a redirect, it will be added after this one.
            entProps->targetCount++;
            if(entType == SKE_LASER_90)
            {
                sokoDirection_t redirectDir = sokoRedirectDir(emitterDir, self->currentLevel.entities[entCollInd].facing); //SKD_UP or SKD_DOWN
                sokoBeamImpactRecursive(self,testPos.x,testPos.y,redirectDir,rootEmitter);
            }
            
            break;
        }
        testPos = sokoAddCoord(testPos, projVec);
    }
    retVal = self->currentLevel.entities[entCollInd].properties->targetCount;
    // printf("\n");
    //retVal.x           = testPos.x;
    //retVal.y           = testPos.y;
    //retVal.entityIndex = entCollInd;
    //retVal.entityFlag  = entCollFlag;
    // printf("impactPoint:(%d,%d)\n",testPos.x,testPos.y);
    return retVal;    

}


sokoCollision_t sokoBeamImpact(soko_abs_t* self, sokoEntity_t* emitter)
{
    sokoDirection_t dir = emitter->facing;
    sokoVec_t projVec   = {0, 0};
    sokoVec_t emitVec   = {emitter->x, emitter->y};
    switch (dir)
    {
        case SKD_DOWN:
            projVec.y = 1;
            break;
        case SKD_UP:
            projVec.y = -1;
            break;
        case SKD_LEFT:
            projVec.x = -1;
            break;
        case SKD_RIGHT:
            projVec.x = 1;
            break;
        default:
            // return base entity position
    }

    // Iterate over tiles in ray to edge of level
    sokoVec_t testPos = sokoAddCoord(emitVec, projVec);
    int entityCount   = self->currentLevel.entityCount;
    // todo: make first pass pack a statically allocated array with only the entities in the path of the laser.

    uint8_t tileCollision[]
        = {0, 0, 1, 0, 0, 1, 1}; // There should be a pointer internal to the game state so this can vary with game mode
    uint8_t entityCollision[] = {0, 0, 1, 1};

    int16_t possibleSquares = 0;
    if (dir == SKD_RIGHT) // move these checks into the switch statement
    {
        possibleSquares = self->currentLevel.width - emitVec.x; // Up to and including far wall
    }
    if (dir == SKD_LEFT)
    {
        possibleSquares = emitVec.x + 1;
    }
    if (dir == SKD_UP)
    {
        possibleSquares = emitVec.y + 1;
    }
    if (dir == SKD_DOWN)
    {
        possibleSquares = self->currentLevel.height - emitVec.y;
    }

    int tileCollFlag, entCollFlag, entCollInd;
    tileCollFlag = entCollFlag = entCollInd = 0;

    sokoCollision_t retVal;
    // printf("emitVec(%d,%d)",emitVec.x,emitVec.y);
    // printf("projVec:(%d,%d) possibleSquares:%d ",projVec.x,projVec.y,possibleSquares);

    for (int n = 0; n < possibleSquares; n++)
    {
        sokoTile_t posTile = absSokoGetTile(self, testPos.x, testPos.y);
        // printf("|n:%d,posTile:(%d,%d):%d|",n,testPos.x,testPos.y,posTile);
        if (tileCollision[posTile])
        {
            tileCollFlag = 1;
            break;
        }
        for (int m = 0; m < entityCount; m++) // iterate over tiles/entities to check for laser collision. First pass
                                              // finds everything in the path of the
        {
            sokoEntity_t candidateEntity = self->currentLevel.entities[m];
            // printf("|m:%d;CE:(%d,%d)%d",m,candidateEntity.x,candidateEntity.y,candidateEntity.type);
            if (candidateEntity.x == testPos.x && candidateEntity.y == testPos.y)
            {
                // printf(";POSMATCH;Coll:%d",entityCollision[candidateEntity.type]);
                if (entityCollision[candidateEntity.type])
                {
                    entCollFlag = 1;
                    entCollInd  = m;
                    // printf("|");
                    break;
                }
            }
            // printf("|");
        }

        if (entCollFlag)
        {
            break;
        }
        testPos = sokoAddCoord(testPos, projVec);
    }
    // printf("\n");
    retVal.x           = testPos.x;
    retVal.y           = testPos.y;
    retVal.entityIndex = entCollInd;
    retVal.entityFlag  = entCollFlag;
    // printf("impactPoint:(%d,%d)\n",testPos.x,testPos.y);
    return retVal;
}

sokoVec_t sokoAddCoord(sokoVec_t op1, sokoVec_t op2)
{
    sokoVec_t retVal;
    retVal.x = op1.x + op2.x;
    retVal.y = op1.y + op2.y;
    return retVal;
}

// Euler Game Modes
void eulerSokoTryPlayerMovement(soko_abs_t* self)
{
    if (self->input.playerInputDeltaX == 0 && self->input.playerInputDeltaY == 0)
    {
        return;
    }

    uint16_t x = self->soko_player->x;
    uint16_t y = self->soko_player->y;
    bool moved = self->sokoTryMoveEntityInDirectionFunc(self, self->soko_player, self->input.playerInputDeltaX,
                                                        self->input.playerInputDeltaY, 0);

    if (moved)
    {
        // Paint Floor

        // previous
        if (self->currentLevel.tiles[x][y] == SKT_FLOOR)
        {
            self->currentLevel.tiles[x][y] = SKT_FLOOR_WALKED;
        }
        // current
        if (self->currentLevel.tiles[self->soko_player->x][self->soko_player->y] == SKT_FLOOR)
        {
            self->currentLevel.tiles[self->soko_player->x][self->soko_player->y] = SKT_FLOOR_WALKED;
        }

        // Try Sticky Blocks
        // Loop through all entities is probably not really slower than sampling? We usually have <5 entities.
        for (size_t i = 0; i < self->currentLevel.entityCount; i++)
        {
            if (self->currentLevel.entities[i].type == SKE_STICKY_CRATE)
            {
                if (self->currentLevel.entities[i].x == x && self->currentLevel.entities[i].y == y + 1)
                {
                    absSokoTryMoveEntityInDirection(self, &self->currentLevel.entities[i],
                                                    self->input.playerInputDeltaX, self->input.playerInputDeltaY, 0);
                }
                else if (self->currentLevel.entities[i].x == x && self->currentLevel.entities[i].y == y - 1)
                {
                    absSokoTryMoveEntityInDirection(self, &self->currentLevel.entities[i],
                                                    self->input.playerInputDeltaX, self->input.playerInputDeltaY, 0);
                }
                else if (self->currentLevel.entities[i].y == y && self->currentLevel.entities[i].x == x + 1)
                {
                    absSokoTryMoveEntityInDirection(self, &self->currentLevel.entities[i],
                                                    self->input.playerInputDeltaX, self->input.playerInputDeltaY, 0);
                }
                else if (self->currentLevel.entities[i].y == y && self->currentLevel.entities[i].x == x - 1)
                {
                    absSokoTryMoveEntityInDirection(self, &self->currentLevel.entities[i],
                                                    self->input.playerInputDeltaX, self->input.playerInputDeltaY, 0);
                }
            }
        }
    }
}

bool eulerNoUnwalkedFloors(soko_abs_t* self)
{
    for (size_t x = 0; x < self->currentLevel.width; x++)
    {
        for (size_t y = 0; y < self->currentLevel.height; y++)
        {
            if (self->currentLevel.tiles[x][y] == SKT_FLOOR)
            {
                return false;
            }
        }
    }

    return true;
}

void overworldSokoGameLoop(soko_abs_t* self, int64_t elapsedUs)
{
    if (self->state == SKS_GAMEPLAY)
    {
        // logic
        self->sokoTryPlayerMovementFunc(self);

        // victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        // todo: rename to isVictory or such.
        self->allCratesOnGoal = self->isVictoryConditionFunc(self);
        if (self->allCratesOnGoal)
        {
            self->state = SKS_VICTORY;
            printf("Player at %d,%d\n", self->soko_player->x, self->soko_player->y);
            victoryDanceTimer = 0;
        }
        // draw level
        self->drawTilesFunc(self, &self->currentLevel);
    }
    else if (self->state == SKS_VICTORY)
    {
        self->drawTilesFunc(self, &self->currentLevel);

        // check for input for exit/next level.
        uint8_t targetWorldIndex = 0;
        for (int i = 0; i < self->portalCount; i++)
        {
            if (self->soko_player->x == self->portals[i].x && self->soko_player->y == self->portals[i].y)
            {
                targetWorldIndex = i + 1;
                break;
            }
        }
        printf("Player at %d,%d\n", self->soko_player->x, self->soko_player->y);
        self->loadNewLevelIndex = targetWorldIndex;
        self->loadNewLevelFlag  = true;
        self->screen            = SOKO_LOADNEWLEVEL;
    }

    // DEBUG PLACEHOLDER:
    //  Render the time to a string
    char str[16] = {0};
    int16_t tWidth;
    if (!self->allCratesOnGoal)
    {
        snprintf(str, sizeof(str) - 1, "sokoban");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "sokasuccess");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 0);
    }
}

bool overworldPortalEntered(soko_abs_t* self)
{
    for (uint8_t i = 0; i < self->portalCount; i++)
    {
        if (self->soko_player->x == self->portals[i].x && self->soko_player->y == self->portals[i].y)
        {
            return true;
        }
    }
    return false;
}

void restartCurrentLevel(soko_abs_t* self)
{
    // assumed this is set already?
    // self->loadNewLevelIndex = self->loadNewLevelIndex;

    // todo: what can we do about screen flash when restarting?
    self->loadNewLevelFlag = true;
    self->screen           = SOKO_LOADNEWLEVEL;
}

void exitToOverworld(soko_abs_t* self)
{
    self->loadNewLevelIndex = 0;
    self->loadNewLevelFlag  = true;
    // self->state = SKS_GAMEPLAY;
    self->screen = SOKO_LOADNEWLEVEL;
}

#include "soko_game.h"
#include "soko.h"
#include "soko_gamerules.h"
#include "soko_save.h"
#include "shapes.h"
#include "soko_undo.h"

// clang-format off
// True if the entity CANNOT go on the tile
bool sokoEntityTileCollision[6][9] = {
    // Empty, //floor//wall//goal//noWalk//portal  //l-emit  //l-receive //walked
    {true, false, true, false, true,false,  false, false, false}, // SKE_NONE
    {true, false, true, false, true,false,  false, false, true},  // PLAYER
    {true, false, true, false,  true,false, false, false, false}, // CRATE
    {true, false, true, false, true,false,  false, false, false}, // LASER
    {true, false, true, false,  true,false, false, false, false}, // STICKY CRATE
    {true, false, true, false,  true,false, false, false, true} // STICKY_TRAIL_CRATE
};

static const char *victorym[] ={"nice", "good job", "well done", "solved it", "high five!","hooray","brilliant","victory","incredible", "awesome!","superb work","success!"};
const size_t victorym_count = sizeof(victorym) / sizeof(victorym[0]);
// clang-format on

uint64_t victoryDanceTimer;

void sokoConfigGamemode(soko_abs_t* soko, soko_var_t variant) // This should be called when you reload a level to make sure game rules are correct
{
    soko->currentTheme = &soko->sokoDefaultTheme;
    soko->background   = SKBG_GRID;

    if (variant == SOKO_CLASSIC) // standard gamemode. Check 'variant' variable
    {
        printf("Config Soko to Classic\n");
        soko->maxPush                          = 1; // set to 1 for "traditional" sokoban.
        soko->gameLoopFunc                     = absSokoGameLoop;
        soko->sokoTryPlayerMovementFunc        = absSokoTryPlayerMovement;
        soko->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        soko->drawTilesFunc                    = absSokoDrawTiles;
        soko->isVictoryConditionFunc           = absSokoAllCratesOnGoal;
        soko->sokoGetTileFunc                  = absSokoGetTile;
    }
    else if (variant == SOKO_EULER) // standard gamemode. Check 'variant' variable
    {
        printf("Config Soko to Euler\n");
        soko->maxPush                          = 0; // set to 0 for infinite push.
        soko->gameLoopFunc                     = absSokoGameLoop;
        soko->sokoTryPlayerMovementFunc        = eulerSokoTryPlayerMovement;
        soko->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        soko->drawTilesFunc                    = absSokoDrawTiles;
        soko->isVictoryConditionFunc           = eulerNoUnwalkedFloors;
        soko->currentTheme                     = &soko->eulerTheme;
        soko->background                       = SKBG_BLACK;

        // Initialze spaces below the player and sticky.
        for (size_t i = 0; i < soko->currentLevel.entityCount; i++)
        {
            if (soko->currentLevel.entities[i].type == SKE_PLAYER
                || soko->currentLevel.entities[i].type == SKE_STICKY_TRAIL_CRATE)
            {
                soko->currentLevel.tiles[soko->currentLevel.entities[i].x][soko->currentLevel.entities[i].y]
                    = SKT_FLOOR_WALKED;
            }
        }
    }
    else if (variant == SOKO_OVERWORLD)
    {
        printf("Config Soko to Overworld\n");
        soko->maxPush                          = 0; // set to 0 for infinite push.
        soko->gameLoopFunc                     = overworldSokoGameLoop;
        soko->sokoTryPlayerMovementFunc        = absSokoTryPlayerMovement;
        soko->sokoTryMoveEntityInDirectionFunc = absSokoTryMoveEntityInDirection;
        soko->drawTilesFunc                    = absSokoDrawTiles;
        soko->isVictoryConditionFunc           = overworldPortalEntered;
        soko->sokoGetTileFunc                  = absSokoGetTile;
        soko->currentTheme                     = &soko->overworldTheme;
        // set position to previous overworld positon when re-entering the overworld
        // but like... not an infinite loop?
        
        soko->soko_player->x = soko->overworld_playerX;
        soko->soko_player->y = soko->overworld_playerY;
        soko->background     = SKBG_BLACK;

        for (size_t i = 0; i < soko->portalCount; i++)
        {
            if (soko->portals[i].index < SOKO_LEVEL_COUNT)
            {
                soko->portals[i].levelCompleted = soko->levelSolved[soko->portals[i].index];
            }
        }
    }
    else
    {
        printf("invalid gamemode.");
    }

    //save overworld position.
    uint32_t overworldData =  (soko->overworld_playerY << 16) | soko->overworld_playerX;
    writeNvs32("sk_overworldPos", overworldData);

    // add conditional for alternative variants
    sokoInitHistory(soko);

    //pick a single random victory message. We can't pick during the loop or it will be random every frame.
    soko->chosen_victory_message = rand() % victorym_count;
}

void absSokoGameLoop(soko_abs_t* soko, int64_t elapsedUs)
{
    if (soko->state == SKS_GAMEPLAY)
    {
        // logic
        soko->sokoTryPlayerMovementFunc(soko);

        // undo check
        if (soko->input.undo)
        {
            sokoUndo(soko);
        }

        // victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        // todo: rename to isVictory or such.
        soko->allCratesOnGoal = soko->isVictoryConditionFunc(soko);
        if (soko->allCratesOnGoal)
        {
            soko->state       = SKS_VICTORY;
            victoryDanceTimer = 0;
        }
        // draw level
        soko->drawTilesFunc(soko, &soko->currentLevel);
    }
    else if (soko->state == SKS_VICTORY)
    {
        // check for input for exit/next level.
        soko->drawTilesFunc(soko, &soko->currentLevel);
        victoryDanceTimer += elapsedUs;
        if (victoryDanceTimer > SOKO_VICTORY_TIMER_US)
        {
            sokoSolveCurrentLevel(soko);
            soko->loadNewLevelIndex = 0;
            soko->loadNewLevelFlag  = true;
            soko->screen            = SOKO_LOADNEWLEVEL;
        }
    }

    char str[20] = {0};
    int16_t tWidth;
    if (!soko->allCratesOnGoal)
    {
        snprintf(str, sizeof(str) - 1, "%s", soko->levelTitles[soko->currentLevelIndex]);
        // Measure the width of the time string
        tWidth = textWidth(&soko->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&soko->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 2);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "%s", victorym[soko->chosen_victory_message]);
        // Measure the width of the time string
        tWidth = textWidth(&soko->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&soko->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 2);
    }
    sharedGameLoop(soko);
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
void absSokoTryPlayerMovement(soko_abs_t* soko)
{
    if (soko->input.playerInputDeltaX == 0 && soko->input.playerInputDeltaY == 0)
    {
        return;
    }

    bool b = soko->sokoTryMoveEntityInDirectionFunc(soko, soko->soko_player, soko->input.playerInputDeltaX,
                                                    soko->input.playerInputDeltaY, 0);
    sokoHistoryTurnOver(soko);
    if (b)
    {
        soko->moveCount++;
    }
}

bool absSokoTryMoveEntityInDirection(soko_abs_t* self, sokoEntity_t* entity, int dx, int dy, uint16_t push)
{
    // prevent infitnite loop where you push yourself nowhere.
    if (dx == 0 && dy == 0)
    {
        return false;
    }

    // maxiumum number of crates we can push. Traditional sokoban has a limit of one. Euler is infinity.
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
                        sokoAddEntityMoveToHistory(self, entity, entity->x, entity->y, entity->facing);
                        entity->x += dx;
                        entity->y += dy;
                        entity->facing = sokoDirectionFromDelta(dx, dy);
                        return true; // if entities overlap, we should not break here?
                    }
                    else
                    {
                        // can't push? can't move.
                        return false;
                    }
                }
            }
            else if (self->currentLevel.entities[i].type == SKE_STICKY_TRAIL_CRATE)
            {
                // previous
                // for euler. todo: make EulerTryMoveEntityInDirection instead of an if statement.
                if (self->currentLevel.entities[i].x == px && self->currentLevel.entities[i].y == py)
                {
                    if (self->sokoTryMoveEntityInDirectionFunc(self, &self->currentLevel.entities[i], dx, dy, push + 1))
                    {
                        sokoAddEntityMoveToHistory(self, entity, entity->x, entity->y, entity->facing);
                        entity->x += dx;
                        entity->y += dy;
                        entity->facing = sokoDirectionFromDelta(dx, dy);
                        return true;
                    }
                    else
                    {
                        // can't push? can't move.
                        return false;
                    }
                }
            }
        }

        // todo: this is a hack, we should have separate absSokoTryMoveEntityInDirection functions.
        if (self->currentLevel.gameMode == SOKO_EULER && entity->propFlag && entity->properties.trail)
        {
            if (self->currentLevel.tiles[entity->x + dx][entity->y + dy] == SKT_FLOOR)
            {
                sokoAddTileMoveToHistory(self, entity->x + dx, entity->y + dy, SKT_FLOOR);
                self->currentLevel.tiles[entity->x + dx][entity->y + dy] = SKT_FLOOR_WALKED;
            }

            if (self->currentLevel.tiles[entity->x][entity->y] == SKT_FLOOR)
            {
                sokoAddTileMoveToHistory(self, entity->x, entity->y, SKT_FLOOR);
                self->currentLevel.tiles[entity->x][entity->y] = SKT_FLOOR_WALKED;
            }
        }
        // No wall in front of us and nothing to push, we can move.
        // we assume the player never gets pushed for undo here, so if it's the player moving, thats a new move.
        sokoAddEntityMoveToHistory(self, entity, entity->x, entity->y, entity->facing);
        entity->x += dx;
        entity->y += dy;
        entity->facing = sokoDirectionFromDelta(dx, dy);
        return true;
    }
    // all other floor types invalid. Be careful when we add tile types in different rule sets.

    return false;
}
// draw the tiles (and entities, for now) of the level.
void absSokoDrawTiles(soko_abs_t* self, sokoLevel_t* level)
{
    uint16_t scale = level->levelScale;
    // These are in level space (not pixels) and must be within bounds of currentLevel.tiles.
    int16_t screenMinX, screenMaxX, screenMinY, screenMaxY;
    // offsets.
    uint16_t ox, oy;

    // Recalculate Camera Position
    // todo: extract to a function if we end up with different draw functions. Part of future pointer refactor.
    if (self->camEnabled)
    {
        // calculate camera position. Shift if needed. Cam position was initiated to player position.
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

        // calculate offsets
        ox = -self->camX * scale + (TFT_WIDTH / 2);
        oy = -self->camY * scale + (TFT_HEIGHT / 2);

        // calculate out of bounds draws. todo: make tenery operators.
        screenMinX = self->camX - self->camWidth / 2 - 1;
        if (screenMinX < 0)
        {
            screenMinX = 0;
        }
        screenMaxX = self->camX + self->camWidth / 2 + 1;
        if (screenMaxX > level->width)
        {
            screenMaxX = level->width;
        }
        
        screenMinY = self->camY - self->camHeight / 2 - 1;
        if (screenMinY < 0)
        {
            screenMinY = 0;
        }
        screenMaxY = self->camY + self->camHeight / 2 + 1;
        if (screenMaxY > level->height)
        {
            screenMaxY = level->height;
        }
    }
    else
    { // no camera
        // calculate offsets to center the level.
        ox = (TFT_WIDTH / 2) - ((level->width) * scale / 2);
        oy = (TFT_HEIGHT / 2) - ((level->height) * scale / 2);

        // bounds are just the level.
        screenMinX = 0;
        screenMaxX = level->width;
        screenMinY = 0;
        screenMaxY = level->height;
    }

    SETUP_FOR_TURBO();

    // Tile Drawing (bg layer)
    for (size_t x = screenMinX; x < screenMaxX; x++)
    {
        for (size_t y = screenMinY; y < screenMaxY; y++)
        {
            paletteColor_t color = cTransparent;
            switch (level->tiles[x][y])
            {
                case SKT_FLOOR:
                {
                    color = self->currentTheme->floorColor;
                    break;
                }
                case SKT_WALL:
                {
                    color = self->currentTheme->wallColor;
                    break;
                }
                case SKT_GOAL:
                {
                    color = self->currentTheme->floorColor;
                    break;
                }
                case SKT_FLOOR_WALKED:
                {
                    color = self->currentTheme->altFloorColor;
                    break;
                }
                case SKT_EMPTY:
                {
                    color = cTransparent;
                    break;
                }
                case SKT_PORTAL:
                { // todo: draw completed or not completed.
                    color = c441;
                    // color = self->currentTheme->floorColor;
                    break;
                }
                default:
                {
                    break;
                }
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

            if (level->tiles[x][y] == SKT_GOAL)
            {
                drawWsg(&self->currentTheme->goalWSG, ox + x * scale, oy + y * scale, false, false, 0);
            }

            // DEBUG_DRAW_COUNT++;
            //  draw outline around the square.
            //  drawRect(ox+x*s,oy+y*s,ox+x*s+s,oy+y*s+s,color);
        }
    }

    // draw portal in overworld before entities.
    // hypothetically, we can get rid of the overworld check, and there just won't be other portals? but there could be?
    // sprint("a\n");
    if (self->currentLevel.gameMode == SOKO_OVERWORLD)
    {
        self->allSolved=true;
        for (int i = 0; i < self->portalCount; i++)
        {
            if (self->portals[i].x >= screenMinX && self->portals[i].x <= screenMaxX && self->portals[i].y >= screenMinY
                && self->portals[i].y <= screenMaxY)
            {
                if (self->portals[i].levelCompleted)
                {
                    drawWsg(&self->currentTheme->portal_completeWSG, ox + self->portals[i].x * scale,
                            oy + self->portals[i].y * scale, false, false, 0);
                }
                else
                {
                    drawWsg(&self->currentTheme->portal_incompleteWSG, ox + self->portals[i].x * scale,
                            oy + self->portals[i].y * scale, false, false, 0);
                    self->allSolved = false;
                }
            }
        }
    }

    // draw entities
    for (size_t i = 0; i < level->entityCount; i++)
    {
        // don't bother drawing off screen
        if (level->entities[i].x >= screenMinX && level->entities[i].x <= screenMaxX
            && level->entities[i].y >= screenMinY && level->entities[i].y <= screenMaxY)
        {
            switch (level->entities[i].type)
            {
                case SKE_PLAYER:
                {
                    // for (size_t xd = ox + level->entities[i].x * scale; xd < ox + level->entities[i].x * scale + scale; xd++)
                    // {
                    //     for (size_t yd = oy +level->entities[i]. y * scale; yd < oy + level->entities[i].y * scale + scale; yd++)
                    //     {
                    //         TURBO_SET_PIXEL(xd, yd, c211);
                    //     }
                    // }
                    // continue;
                    switch (level->entities[i].facing)
                    {
                        case SKD_UP:
                        {
                            drawWsg(&self->currentTheme->playerUpWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        }
                        case SKD_RIGHT:
                        {
                            drawWsg(&self->currentTheme->playerRightWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        }
                        case SKD_LEFT:
                        {
                            drawWsg(&self->currentTheme->playerLeftWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        }
                        case SKD_DOWN:
                        default:
                        {
                            drawWsg(&self->currentTheme->playerDownWSG, ox + level->entities[i].x * scale,
                                    oy + level->entities[i].y * scale, false, false, 0);
                            break;
                        }
                    }

                    break;
                }
                case SKE_CRATE:
                {
                    if (self->currentLevel.tiles[self->currentLevel.entities[i].x][self->currentLevel.entities[i].y]
                        == SKT_GOAL)
                    {
                        drawWsg(&self->currentTheme->crateOnGoalWSG, ox + level->entities[i].x * scale,
                                oy + level->entities[i].y * scale, false, false, 0);
                    }
                    else
                    {
                        drawWsg(&self->currentTheme->crateWSG, ox + level->entities[i].x * scale,
                                oy + level->entities[i].y * scale, false, false, 0);
                    }
                    break;
                }
                case SKE_STICKY_CRATE:
                {
                    drawWsg(&self->currentTheme->stickyCrateWSG, ox + level->entities[i].x * scale,
                            oy + level->entities[i].y * scale, false, false, 0);
                    break;
                }
                case SKE_STICKY_TRAIL_CRATE:
                {
                    drawWsg(&self->currentTheme->crateOnGoalWSG, ox + level->entities[i].x * scale,
                            oy + level->entities[i].y * scale, false, false, 0);
                    break;
                }
                case SKE_NONE:
                default:
                {
                    break;
                }
            }
        }
    }
}

bool absSokoAllCratesOnGoal(soko_abs_t* soko)
{
    for (size_t i = 0; i < soko->currentLevel.entityCount; i++)
    {
        if (soko->currentLevel.entities[i].type == SKE_CRATE)
        {
            if (soko->currentLevel.tiles[soko->currentLevel.entities[i].x][soko->currentLevel.entities[i].y]
                != SKT_GOAL)
            {
                return false;
            }
        }
    }
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
    uint16_t scale
        = self->currentLevel
              .levelScale; //@todo These should be in constants, but too lazy to change all references at the moment.
    uint16_t ox = (TFT_WIDTH / 2) - ((self->currentLevel.width) * scale / 2);
    uint16_t oy = (TFT_HEIGHT / 2) - ((self->currentLevel.height) * scale / 2);
    retVec.x    = ox + scale * grid.x + scale / 2;
    retVec.y    = oy + scale * grid.y + scale / 2;
    return retVec;
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
            sokoAddTileMoveToHistory(self, x, y, SKT_FLOOR);
            self->currentLevel.tiles[x][y] = SKT_FLOOR_WALKED;
        }
        if (self->currentLevel.tiles[self->soko_player->x][self->soko_player->y] == SKT_FLOOR)
        {
            sokoAddTileMoveToHistory(self, self->soko_player->x, self->soko_player->y, SKT_FLOOR);
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
        sokoHistoryTurnOver(self);
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

        // by saving this before we move, we lag by one position. The final movement onto a portal doesn't get saved, as
        // this loopin't entered again then we return to the position we were at before the last loop.
        self->overworld_playerX = self->soko_player->x;
        self->overworld_playerY = self->soko_player->y;

        self->sokoTryPlayerMovementFunc(self);

        // victory status. stored separate from gamestate because of future gameplay ideas/remixes.
        // todo: rename 'allCrates' to isVictory or such.
        self->allCratesOnGoal = self->isVictoryConditionFunc(self);
        if (self->allCratesOnGoal)
        {
            self->state = SKS_VICTORY;
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
                targetWorldIndex = self->portals[i].index;
                break;
            }
        }

        //int32_t data = self->overworld_playerY << 16;
        //data = data | self->overworld_playerX;
       // writeNvs32("sk_over_pos",data);

        self->loadNewLevelIndex = targetWorldIndex;
        self->loadNewLevelFlag  = false; // load saved data.
        self->screen            = SOKO_LOADNEWLEVEL;
    }

    // DEBUG PLACEHOLDER:
    //  Render the time to a string
    char str[31] = {0};
    int16_t tWidth;
    if (!self->allSolved)
    {
        snprintf(str, sizeof(str) - 1, "Hunter's Block Puzzles");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 3);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "'oh, nice, You won' - Hunter");
        // Measure the width of the time string
        tWidth = textWidth(&self->ibm, str);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&self->ibm, c555, str, ((TFT_WIDTH - tWidth) / 2), 3);
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

void exitToOverworld(soko_abs_t* soko)
{
    printf("Exit to Overworld\n");
    // save. todo: skip if victory.
    if (soko->currentLevel.gameMode == SOKO_EULER)
    {
        // sokoSaveEulerTiles(soko);
    }
    // sokoSaveCurrentLevelEntities(soko);

    soko->loadNewLevelIndex = 0;
    soko->loadNewLevelFlag  = true;
    // self->state = SKS_GAMEPLAY;
    soko->screen = SOKO_LOADNEWLEVEL;
}

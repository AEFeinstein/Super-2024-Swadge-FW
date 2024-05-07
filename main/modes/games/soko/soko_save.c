#include "soko.h"
#include "soko_save.h"

static void sokoSaveCurrentLevelEntities(soko_abs_t* soko);
static void sokoLoadCurrentLevelEntities(soko_abs_t* soko);
static void sokoSetLevelSolvedState(soko_abs_t* soko, uint16_t levelIndex, bool solved);
static void sokoLoadBinTiles(soko_abs_t* soko, int byteCount);

/// @brief Called on 'resume' from the menu.
/// @param soko 
void sokoLoadGameplay(soko_abs_t* soko){
    //load current level
    int current = 0;
    sokoLoadBinLevel(soko, current);

    //current level entity positions
    sokoLoadCurrentLevelEntities(soko);    
}

void sokoLoadLevelSolvedState(soko_abs_t* soko){
    //todo: automatically split for >32, >64 levels using 2 loops.

    int lvs = 0;
    readNvs32("sklv1",&lvs);
    //i<32...
    for (size_t i = 0; i < SOKO_LEVEL_COUNT; i++)
    {
        soko->levelSolved[i] = (1 & lvs>>i) == 1;
    }
    //now the next 32 bytes!
    // readNvs32("sklv2",&lvs);
    // for (size_t i = 32; i < SOKO_LEVEL_COUNT || i < 64; i++)
    // {
    //     soko->levelSolved[i] = (1 & lvs>>i) == 1;
    // }

    //etc. Probably won't bother cleaning it into nested loop until over 32*4 levels...
    //so .. never?
}




void sokoSetLevelSolvedState(soko_abs_t* soko, uint16_t levelIndex, bool solved){
    printf("save level solved status %d\n", levelIndex);
    //todo: changes a single levels bool in the sokoSolved array,
    soko->levelSolved[levelIndex] = true;

    int section = levelIndex/32;
    int index = levelIndex;
    int lvs = 0;

    if (section == 0)
    {
        readNvs32("sklv1",&lvs);
    }else if(section == 1){
        readNvs32("sklv2",&lvs);
        index -= 32;
    }//else, 64, 

    //write the bit.
    if(solved){
        //set bit
        lvs = lvs | (1 << index);
    }else{
        //clear bit
        lvs = lvs & ~(1 << index);
    }

    //write the bit out to data.
    if (section == 0)
    {
        writeNvs32("sklv1",lvs);
    }else if(section == 1){
        writeNvs32("sklv2",lvs);
    }
}

void sokoSolveCurrentLevel(soko_abs_t* soko){
    if(soko->currentLevelIndex == 0){
        //overworld level.
        return;
    }else{
        sokoSetLevelSolvedState(soko,soko->currentLevelIndex,true);
    }
}

//Saving Progress
//soko->overworldX
//soko->overworldY
//current level? or just stick on overworld?

//current level progress (all entitity positions/data, entities array. non-entities comes from file.)
//euler encoding? (do like picross level?)

void sokoSaveCurrentLevelEntities(soko_abs_t* soko){
    //todo: the overworld will have >max entities... and they never need to be serialized...
    //so maybe just make a separate array for portals that is entities of size maxLevelCount...
    //and then treat it completely separately in the game loops.

    //sort of feels like we should do something similar to the blob packing of the levels.
    //Then write a function that's like "get entity from bytes" where we pass it an array-slice of bytes, and get back some entity object.
    //except, we have to include x,y data here... so it would be different...

    //instead, we can have our own binary encoding. Some entities never move, and can be loaded from the disk.
    //after they are loaded, we save "Index, X, Y, Extra" binary sets, and replace the values for the entities at the index position.
    //I think it will work such that, for a level, entities will always have the same index position in the entities array...
    //this is ONLY true if we never actually 'destroy' or 'CREATE' entities, but just flip some 'dead' flag.

    //if each entity is 4 bytes, then we can save (adjust) all entities as a single blob, always, since it's a pre-allocated array.
    char entities[SOKO_MAX_ENTITY_COUNT*4];
    size_t size = sizeof(entities);

    for(int i = 0;i<soko->currentLevel.entityCount;i++){
        entities[i*4] = i;
        //todo: facing...
        //sokoentityproperties? will these ever change at runtime? there is an "hp" that was made for laserbounce...
        //do we need the propflag?
        entities[i*4+1] = soko->currentLevel.entities[i].x;
        entities[i*4+2] = soko->currentLevel.entities[i].y;
        entities[i*4+3] = soko->currentLevel.entities[i].propFlag == true ? 1:0;
    }
    writeNvsBlob("sk_current_entities",&entities,size);
}
//todo: there is no clean place to return to the main menu right now, so gotta write that function/flow so this can get called.

/// @brief After loading the level into currentLevel, this updates the entity array with saved
/// @param soko 
void sokoLoadCurrentLevelEntities(soko_abs_t* soko){
    char entities[SOKO_MAX_ENTITY_COUNT*4];
    size_t size = sizeof(entities);
    readNvsBlob("sk_current_entities",&entities,&size);
    for(int i = 0;i<soko->currentLevel.entityCount;i++){
        //todo: wait, if all entities are the same length, we don't actually need to save the index...
        soko->currentLevel.entities[i].x = entities[i*4+1];
        soko->currentLevel.entities[i].x = entities[i*4+2];
        soko->currentLevel.entities[i].propFlag = entities[i*4+3] == 1;
    }
}

//Level loading
void sokoLoadBinLevel(soko_abs_t* soko, uint16_t levelIndex)
{
    printf("load bin level %d, %s\n", levelIndex, soko->levelNames[levelIndex]);
    soko->state = SKS_INIT;
    size_t fileSize;
    soko->levelBinaryData = spiffsReadFile(soko->levelNames[levelIndex], &fileSize, true); // Heap CAPS malloc/calloc allocation for SPI RAM

    // The pointer returned by spiffsReadFile can be freed with free() with no additional steps.
    soko->currentLevel.width = soko->levelBinaryData[0];  // first two bytes of a level's data always describe the
                                                          // bounding width and height of the tilemap.
    soko->currentLevel.height = soko->levelBinaryData[1]; // Max Theoretical Level Bounding Box Size is 255x255, though
                                                          // you'll likely run into issues with entities first.
    soko->currentLevel.gameMode = (soko_var_t)soko->levelBinaryData[2];
    // for(int i = 0; i < fileSize; i++)
    //{
    //     printf("%d, ",soko->levelBinaryData[i]);
    // }
    // printf("\n");
    soko->currentLevelIndex = levelIndex;
    soko->currentLevel.levelScale = 16;
    soko->camWidth                = TFT_WIDTH / (soko->currentLevel.levelScale);
    soko->camHeight               = TFT_HEIGHT / (soko->currentLevel.levelScale);
    soko->camEnabled    = soko->camWidth < soko->currentLevel.width || soko->camHeight < soko->currentLevel.height;
    soko->camPadExtentX = soko->camWidth * 0.6 * 0.5;
    soko->camPadExtentY = soko->camHeight * 0.6 * 0.5;

    //incremented by loadBinTiles.
    soko->currentLevel.entityCount = 0;
    soko->portalCount = 0;

    sokoLoadBinTiles(soko, (int)fileSize);
    // for(int k = 0; k < soko->currentLevel.entityCount; k++)
    //{
    //     printf("Ent%d:%d
    //     (%d,%d)",k,soko->currentLevel.entities[k].type,soko->currentLevel.entities[k].x,soko->currentLevel.entities[k].y);
    // }
    // printf("\n");

    if(levelIndex == 0){
        if(soko->overworld_playerX == 0 && soko->overworld_playerY == 0){
            printf("resetting player position from loaded entity\n");
            soko->overworld_playerX = soko->soko_player->x;
            soko->overworld_playerY = soko->soko_player->y;
        }
    }

    printf("Loaded level w: %i, h %i, entities: %i\n",soko->currentLevel.width,soko->currentLevel.height,soko->currentLevel.entityCount);
}

//todo: rename self to soko
void sokoLoadBinTiles(soko_abs_t* self, int byteCount)
{
    const int HEADER_BYTE_OFFSET   = 3;//width,height,mode
    int totalTiles                 = self->currentLevel.width * self->currentLevel.height;
    int tileIndex                  = 0;
    self->currentLevel.entityCount = 0;
    self->goalCount                = 0;

    printf("reading tiles+entities:\n");
    for (int i = HEADER_BYTE_OFFSET; i < byteCount; i++)
    {
        // Objects in level data should be of the form 
        // SKB_OBJSTART, SKB_[Object Type], [Data Bytes] , SKB_OBJEND
        if (self->levelBinaryData[i] == SKB_OBJSTART){
            int objX = (tileIndex - 1) % (self->currentLevel.width); // Look at the previous
            int objY = (tileIndex - 1) / (self->currentLevel.width);
            uint8_t flagByte, direction;
            bool players, crates, sticky, trail, inverted;
            int hp, targetX, targetY;
            printf("reading object byte after start: %i,%i:%i\n",objX,objY,self->levelBinaryData[i+1]);

            switch (self->levelBinaryData[i + 1]) // On creating entities, index should be advanced to the SKB_OBJEND
                                                  // byte so the post-increment moves to the next tile.
            {
                case SKB_COMPRESS:
                    i += 2;
                    break; // Not yet implemented
                case SKB_PLAYER:
                    //moved gamemode to bit 3 of level data in header.
                    //self->currentLevel.gameMode                                      = self->levelBinaryData[i + 2];
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_PLAYER;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->soko_player              = &self->currentLevel.entities[self->currentLevel.playerIndex];
                    self->currentLevel.playerIndex = self->currentLevel.entityCount;
                    self->currentLevel.entityCount += 1;
                    i += 2;//start, player, end.
                    break;
                case SKB_CRATE:
                    flagByte = self->levelBinaryData[i + 2];
                    sticky   = !!(flagByte & (0x1 << 0));
                    trail    = !!(flagByte & (0x1 << 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type
                        = sticky ? SKE_STICKY_CRATE : SKE_CRATE;
                    self->currentLevel.entities[self->currentLevel.entityCount].x = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag           = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky = sticky;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->trail  = trail;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_WARPINTERNAL: //[type][flags][hp][destx][desty]
                    flagByte = self->levelBinaryData[i + 2];
                    crates   = !!(flagByte & (0x1 << 0));
                    hp       = self->levelBinaryData[i + 3];
                    targetX  = self->levelBinaryData[i + 4];
                    targetY  = self->levelBinaryData[i + 5];

                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_WARP;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag           = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->hp     = hp;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX
                        = malloc(sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY
                        = malloc(sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = 1;
                    self->currentLevel.entityCount += 1;
                    i += 6;
                    break;
                case SKB_WARPINTERNALEXIT:
                    flagByte = self->levelBinaryData[i + 2];

                    i += 2;            // No data or properties in this object.
                    break;             // Can be used later on for verifying valid warps from save files.
                case SKB_WARPEXTERNAL: //[typep][flags][index]
                    // todo implement extraction of index value and which values should be used for auto-indexed portals
                    self->currentLevel.tiles[objX][objY] = SKT_PORTAL;
                    self->portals[self->portalCount].index
                        = self->portalCount + 1; // For basic test, 1 indexed with levels, but multi-room overworld
                                                 // needs more sophistication to keep indices correct.
                    self->portals[self->portalCount].x = objX;
                    self->portals[self->portalCount].y = objY;
                    printf("Portal %d at %d,%d\n", self->portals[self->portalCount].index,
                           self->portals[self->portalCount].x, self->portals[self->portalCount].y);
                    self->portalCount += 1;
                    i += 4;
                    break;
                case SKB_BUTTON: //[type][flag][numTargets][targetx][targety]...
                    flagByte = self->levelBinaryData[i + 2];
                    crates   = !!(flagByte & (0x1 << 0));
                    players  = !!(flagByte & (0x1 << 1));
                    inverted = !!(flagByte & (0x1 << 2));
                    sticky   = !!(flagByte & (0x1 << 3));
                    hp       = self->levelBinaryData[i + 3];

                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_BUTTON;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX
                        = malloc(sizeof(uint8_t) * hp);
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY
                        = malloc(sizeof(uint8_t) * hp);
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates      = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players     = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->inverted    = inverted;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky      = sticky;
                    for (int j = 0; j < hp; j++)
                    {
                        self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX[j]
                            = self->levelBinaryData[3 + 2 * j + 1];
                        self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY[j]
                            = self->levelBinaryData[3 + 2 * (j + 1)];
                    }
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = hp;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players     = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates      = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->inverted    = inverted;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky      = sticky;
                    self->currentLevel.entityCount += 1;
                    i += (4 + 2 * hp);
                    break;
                case SKB_LASEREMITTER: //[type][flag]
                    flagByte  = self->levelBinaryData[i + 2];
                    direction = (flagByte & (0x3 << 6)) >> 6; // flagbyte stores direction in 0bDD0000P0 Where D is
                                                              // direction bits and P is player push
                    players = !!(flagByte & (0x1 < 1));

                    self->currentLevel.entities[self->currentLevel.entityCount].type   = SKE_LASER_EMIT_UP;
                    self->currentLevel.entities[self->currentLevel.entityCount].x      = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y      = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag            = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX
                        = calloc(SOKO_MAX_ENTITY_COUNT, sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX
                        = calloc(SOKO_MAX_ENTITY_COUNT, sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = 0;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_LASERRECEIVEROMNI:
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_LASER_RECEIVE_OMNI;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entityCount += 1;
                    i += 2;
                    break;
                case SKB_LASERRECEIVER:
                    flagByte  = self->levelBinaryData[i + 2];
                    direction = (flagByte & (0x3 << 6)) >> 6; // flagbyte stores direction in 0bDD0000P0 Where D is
                                                              // direction bits and P is player push
                    players = !!(flagByte & (0x1 < 1));

                    self->currentLevel.entities[self->currentLevel.entityCount].type   = SKE_LASER_RECEIVE;
                    self->currentLevel.entities[self->currentLevel.entityCount].x      = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y      = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_LASER90ROTATE:
                    flagByte  = self->levelBinaryData[i + 2];
                    direction = !!(flagByte & (0x1 < 0));
                    players   = !!(flagByte & (0x1 < 1));

                    self->currentLevel.entities[self->currentLevel.entityCount].type   = SKE_LASER_90;
                    self->currentLevel.entities[self->currentLevel.entityCount].x      = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y      = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag            = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_GHOSTBLOCK:
                    flagByte = self->levelBinaryData[i + 2];
                    inverted = !!(flagByte & (0x1 < 2));
                    players  = !!(flagByte & (0x1 < 1));

                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_GHOST;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties
                        = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag            = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_OBJEND:
                    i += 1;
                    break;
                default: // Make the best of an undefined object type and try to skip it by finding its end byte
                    bool objEndFound          = false;
                    int undefinedObjectLength = 0;
                    while (!objEndFound)
                    {
                        undefinedObjectLength += 1;
                        if (self->levelBinaryData[i + undefinedObjectLength] == SKB_OBJEND)
                        {
                            objEndFound = true;
                        }
                    }
                    i += undefinedObjectLength; // Move to the completion byte of an undefined object type and hope it
                                                // doesn't have two end bytes.
            }
        }
        else
        {
            int tileX = (tileIndex) % (self->currentLevel.width);
            int tileY = (tileIndex) / (self->currentLevel.width);
            // self->currentLevel.tiles[tileX][tileY] = self->levelBinaryData[i];
            int tileType = 0;
            switch (self->levelBinaryData[i]) // This is a bit easier to read than two arrays
            {
                case SKB_EMPTY:
                    tileType = SKT_EMPTY;
                    break;
                case SKB_WALL:
                    tileType = SKT_WALL;
                    break;
                case SKB_FLOOR:
                    tileType = SKT_FLOOR;
                    break;
                case SKB_NO_WALK:
                    tileType = SKT_FLOOR; //@todo Add No-Walk floors that can only accept crates or pass lasers
                    break;
                case SKB_GOAL:
                    tileType = SKT_GOAL;
                    self->goals[self->goalCount].x = tileX;
                    self->goals[self->goalCount].y = tileY;
                    self->goalCount++;
                    break;
                default:
                    tileType = SKT_EMPTY;
                    break;
            }
            self->currentLevel.tiles[tileX][tileY] = tileType;
            // printf("BinData@%d: %d Tile: %d at (%d,%d)
            // index:%d\n",i,self->levelBinaryData[i],tileType,tileX,tileY,tileIndex);
            tileIndex++;
        }
    }
}
#include <string.h>

#include "soko.h"
#include "soko_game.h"
#include "soko_gamerules.h"
#include "soko_save.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoLoadBinLevel(uint16_t levelIndex);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static sokoTile_t sokoGetTileFromColor(paletteColor_t);
static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t);
static int sokoFindIndex(soko_abs_t* self, int targetIndex);
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self);

// strings
static const char sokoModeName[]        = "Sokobanabokabon";
static const char sokoResumeGameLabel[] = "returnitytoit";
static const char sokoNewGameLabel[]    = "startsitfresh";

// create the mode
swadgeMode_t sokoMode = {
    .modeName                 = sokoModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = sokoEnterMode,
    .fnExitMode               = sokoExitMode,
    .fnMainLoop               = sokoMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = sokoBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

// soko_t* soko=NULL;
soko_abs_t* soko = NULL;

// extern const char* sokoLevelNames[]
//     = {"sk_overworld1.wsg", "sk_sticky_test.wsg", "sk_test1.wsg", "sk_test2.wsg", "sk_test3.wsg"};

extern const soko_var_t sokoLevelVariants[]
    = {SOKO_OVERWORLD, SOKO_EULER, SOKO_CLASSIC, SOKO_CLASSIC, SOKO_LASERBOUNCE};

//@TODO: Remove this when all levels do binary loading and dynamic name loading.
// extern const char* sokoBinLevelNames[] = {
//     "sk_binOverworld.bin", "warehouse.bin", "sk_sticky_test.bin", "sk_test1.bin", "sk_test3.bin",
// };

static void sokoEnterMode(void)
{
    soko = calloc(1, sizeof(soko_abs_t));
    // Load a font
    loadFont("ibm_vga8.font", &soko->ibm, false);

    // todo: move to convenience function for loading level data. Preferrebly in it's own file so contributors can futz
    // with it with fewer git merge cases.
    soko->levels[0] = "sk_testpuzzle.wsg";

    // free a wsg that we never loaded... is bad.
    loadWsg(soko->levels[0], &soko->levelWSG, true); // spiRAM cus only used during loading, not gameplay.

    // load sprite assets
    soko->currentTheme = &soko->sokoDefaultTheme;

    // Default Theme
    loadWsg("sk_pango_fwd1.wsg", &soko->sokoDefaultTheme.playerDownWSG, false);
    loadWsg("sk_pango_back1.wsg", &soko->sokoDefaultTheme.playerUpWSG, false);
    loadWsg("sk_pango_side1.wsg", &soko->sokoDefaultTheme.playerLeftWSG, false);
    loadWsg("sk_pango_side2.wsg", &soko->sokoDefaultTheme.playerRightWSG, false);
    loadWsg("sk_crate.wsg", &soko->sokoDefaultTheme.crateWSG, false);
    loadWsg("sk_crate_ongoal.wsg",&soko->sokoDefaultTheme.crateOnGoalWSG,false);
    loadWsg("sk_sticky_crate.wsg", &soko->sokoDefaultTheme.stickyCrateWSG, false);

    soko->sokoDefaultTheme.wallColor  = c111;
    soko->sokoDefaultTheme.floorColor = c444;

    //we check against 0,0 as an invalid start location, and use file location instead.
    soko->overworld_playerX = 0;
    soko->overworld_playerY = 0;

    // Overworld Theme
    soko->overworldTheme.playerDownWSG  = soko->sokoDefaultTheme.playerDownWSG;
    soko->overworldTheme.playerUpWSG    = soko->sokoDefaultTheme.playerUpWSG;
    soko->overworldTheme.playerLeftWSG  = soko->sokoDefaultTheme.playerLeftWSG;
    soko->overworldTheme.playerRightWSG = soko->sokoDefaultTheme.playerRightWSG;
    soko->overworldTheme.crateWSG       = soko->sokoDefaultTheme.crateWSG;
    soko->overworldTheme.crateOnGoalWSG = soko-> sokoDefaultTheme.crateOnGoalWSG;
    soko->overworldTheme.stickyCrateWSG = soko->sokoDefaultTheme.stickyCrateWSG;

    soko->overworldTheme.wallColor  = c121;
    soko->overworldTheme.floorColor = c454;

    // Initialize the menu
    soko->menu                = initMenu(sokoModeName, sokoMenuCb);
    soko->menuLogbookRenderer = initMenuLogbookRenderer(&soko->ibm);

    addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoNewGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
    soko->state  = SKS_INIT;

    //load up the level list.
    soko->levelFileText = loadTxt("SK_LEVEL_LIST.txt", true);
    sokoExtractLevelNamesAndIndices(soko);

    //load level solved state.
    sokoLoadLevelSolvedState(&soko);
}

static void sokoExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenuLogbookRenderer(soko->menuLogbookRenderer);

    // Free the font
    freeFont(&soko->ibm);

    // free the level name file
    freeTxt(soko->levelFileText);

    // free the level
    freeWsg(&soko->levelWSG);

    // free sprites
    freeWsg(&soko->sokoDefaultTheme.playerUpWSG);
    freeWsg(&soko->sokoDefaultTheme.playerDownWSG);
    freeWsg(&soko->sokoDefaultTheme.playerLeftWSG);
    freeWsg(&soko->sokoDefaultTheme.playerRightWSG);
    freeWsg(&soko->sokoDefaultTheme.crateWSG);
    freeWsg(&soko->sokoDefaultTheme.crateOnGoalWSG);
    freeWsg(&soko->sokoDefaultTheme.stickyCrateWSG);

    // Free everything else
    free(soko);
}

static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        // placeholder.
        if (label == sokoResumeGameLabel)
        {
            sokoLoadGameplay(soko);
            sokoLoadBinLevel(0);
            sokoInitGameBin(soko);
            soko->screen = SOKO_LEVELPLAY;
        }
        else if (label == sokoNewGameLabel)
        {
            // load level.
            //we probably shouldn't have a new game option. User should be able to quit at any time.
            sokoLoadBinLevel(0);
            sokoInitGameBin(soko);
            soko->screen = SOKO_LEVELPLAY;
        }
    }
}

static void sokoMainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (soko->screen)
    {
        case SOKO_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                soko->menu = menuButton(soko->menu, evt);
            }

            // Draw the menu
            drawMenuLogbook(soko->menu, soko->menuLogbookRenderer, elapsedUs);
            break;
        }
        case SOKO_LEVELPLAY:
        {
            // pass along to other gameplay, in other file
            //  Always process button events, regardless of control scheme, so the main menu button can be captured
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Save the button state
                soko->input.btnState = evt.state;
            }

            // process input functions in input.
            // Input will turn state into function calls into the game code, and handle complexities.
            sokoPreProcessInput(&soko->input, elapsedUs);
            // background had been drawn, input has been processed and functions called. Now do followup logic and draw
            // level. gameplay loop
            soko->gameLoopFunc(soko, elapsedUs);
            break;
        }
        case SOKO_LOADNEWLEVEL:
        {
            sokoLoadBinLevel(soko->loadNewLevelIndex);
            printf("init new level");
            sokoInitNewLevel(soko, sokoLevelVariants[soko->loadNewLevelIndex]);
            printf("go to gameplay");
            soko->screen = SOKO_LEVELPLAY;
        }
    }
}

void freeEntity(soko_abs_t* self, sokoEntity_t* entity) // Free internal entity structures
{
    if (entity->propFlag)
    {
        if (entity->properties->targetCount)
        {
            free(entity->properties->targetX);
            free(entity->properties->targetY);
        }
        free(entity->properties);
        entity->propFlag = false;
    }
    self->currentLevel.entityCount -= 1;
}

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
                    soko->portalCount += 1;
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

static void sokoLoadBinLevel(uint16_t levelIndex)
{
    const int HEADER_BYTE_OFFSET = 3; // Number of bytes before tile data begins

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

// placeholder.
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((0 == xp % 20) || (0 == yp % 20))
            {
                TURBO_SET_PIXEL(xp, yp, c002);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c001);
            }
        }
    }
}

static int sokoFindIndex(soko_abs_t* self, int targetIndex)
{
    // Filenames are formatted like '1:sk_level.bin:'
    int retVal = -1;
    for (int i = 0; i < targetIndex; i++)
    {
        if (self->levelIndices[i] == targetIndex)
        {
            retVal = i;
        }
    }
    return retVal;
}

static void sokoExtractLevelNamesAndIndices(soko_abs_t* self)
{
    printf("Loading Level List...!\n");
    printf("%s\n", self->levelFileText);
    printf("%d\n", (int)strlen(self->levelFileText));
    // char* a = strstr(self->levelFileText,":");
    // char* b = strstr(a,".bin:");
    // printf("%d",(int)((int)b-(int)a));
    // char* stringPtrs[30];
    // memset(stringPtrs,0,30*sizeof(char*));
    char** stringPtrs = soko->levelNames;
    memset(stringPtrs, 0, SOKO_LEVEL_COUNT * sizeof(char*));
    int* levelInds = soko->levelIndices;
    memset(levelInds, 0, SOKO_LEVEL_COUNT * sizeof(int));
    int intInd       = 0;
    int ind          = 0;
    char* storageStr = strtok(self->levelFileText, ":");
    while (storageStr != NULL)
    {
        //strtol(storageStr, NULL, 10) && 
        if (!(strstr(storageStr, ".bin"))) // Make sure you're not accidentally reading a number from a filename
        {
            levelInds[intInd] = (int)strtol(storageStr, NULL, 10);
            // printf("NumberThing: %s :: %d\n",storageStr,(int)strtol(storageStr,NULL,10));
            intInd++;
        }
        else
        {
            if (!strpbrk(storageStr, "\n\t\r ") && (strstr(storageStr, ".bin")))
            {
                int tokLen    = strlen(storageStr);
                char* tempPtr = calloc((tokLen + 1), sizeof(char)); // Length plus null teminator
                // strcpy(tempPtr,storageStr);
                // stringPtrs[ind] = tempPtr;
                stringPtrs[ind] = storageStr;
                // printf("%s\n",storageStr);
                ind++;
            }
        }
        // printf("This guy!\n");
        storageStr = strtok(NULL, ":");
    }
    printf("Strings: %d, Ints: %d\n", ind, intInd);
    printf("Levels and indices:\n");
    for (int i = ind - 1; i > -1; i--)
    {
        printf("Index: %d : %d : %s\n", i, levelInds[i], stringPtrs[i]);
    }
}

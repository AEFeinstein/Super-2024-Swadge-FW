#include <string.h>

#include "soko.h"
#include "soko_game.h"
#include "soko_gamerules.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoLoadBinLevel(uint16_t levelIndex);
static void sokoLoadLevel(uint16_t);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static sokoTile_t sokoGetTileFromColor(paletteColor_t);
static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t);
static int sokoFindIndex(soko_abs_t* self, int targetIndex);
static void sokoExtractLevelNamesAndIndeces(soko_abs_t* self);

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

extern const char* sokoLevelNames[]
    = {"sk_overworld1.wsg", "sk_sticky_test.wsg", "sk_test1.wsg", "sk_test2.wsg", "sk_test3.wsg"};

extern const soko_var_t sokoLevelVariants[]
    = {SOKO_OVERWORLD, SOKO_EULER, SOKO_CLASSIC, SOKO_CLASSIC, SOKO_LASERBOUNCE};

extern const char* sokoBinLevelNames[] = //@TODO: Remove this when all levels do binary loading and dynamic name loading.
{
    "sk_binOverworld.bin",
    "warehouse.bin",
    "sk_sticky_test.bin",
    "sk_test1.bin",
    "sk_test3.bin"
};


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

//Default Theme
    loadWsg("sk_player_down.wsg", &soko->sokoDefaultTheme.playerDownWSG, false);
    loadWsg("sk_player_up.wsg", &soko->sokoDefaultTheme.playerUpWSG, false);
    loadWsg("sk_player_left.wsg", &soko->sokoDefaultTheme.playerLeftWSG, false);
    loadWsg("sk_player_right.wsg", &soko->sokoDefaultTheme.playerRightWSG, false);
    loadWsg("sk_crate.wsg", &soko->sokoDefaultTheme.crateWSG, false);
    loadWsg("sk_sticky_crate.wsg", &soko->sokoDefaultTheme.stickyCrateWSG, false);

    soko->sokoDefaultTheme.wallColor = c111;
    soko->sokoDefaultTheme.floorColor = c444;

//Overworld Theme
    soko->overworldTheme.playerDownWSG = soko->sokoDefaultTheme.playerDownWSG;
    soko->overworldTheme.playerUpWSG = soko->sokoDefaultTheme.playerUpWSG;
    soko->overworldTheme.playerLeftWSG = soko->sokoDefaultTheme.playerLeftWSG;
    soko->overworldTheme.playerRightWSG = soko->sokoDefaultTheme.playerRightWSG;
    soko->overworldTheme.crateWSG = soko->sokoDefaultTheme.crateWSG;
    soko->overworldTheme.stickyCrateWSG = soko->sokoDefaultTheme.stickyCrateWSG;

    soko->overworldTheme.wallColor = c121;
    soko->overworldTheme.floorColor = c454;

    // Initialize the menu
    soko->menu                = initMenu(sokoModeName, sokoMenuCb);
    soko->menuLogbookRenderer = initMenuLogbookRenderer(&soko->ibm);

    addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoNewGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
    soko->state  = SKS_INIT;
}

static void sokoExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenuLogbookRenderer(soko->menuLogbookRenderer);

    // Free the font
    freeFont(&soko->ibm);

    //free the level name file
    freeTxt(soko->levelFileText);

    // free the level
    freeWsg(&soko->levelWSG);

    // free sprites
    freeWsg(&soko->sokoDefaultTheme.playerUpWSG);
    freeWsg(&soko->sokoDefaultTheme.playerDownWSG);
    freeWsg(&soko->sokoDefaultTheme.playerLeftWSG);
    freeWsg(&soko->sokoDefaultTheme.playerRightWSG);
    freeWsg(&soko->sokoDefaultTheme.crateWSG);
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
            // load level.
            //sokoLoadLevel(0);
            soko->levelFileText = loadTxt("SK_LEVEL_LIST.txt",true);
            sokoExtractLevelNamesAndIndeces(soko);
            /*
            for(int i = 0; i < 20; i++)
            {
                int ind = findIndex(soko,i);
                
                if(ind != -1)
                {
                    printf("Found %d at %d:%s\n",i,ind,soko->levelNames[ind]);
                }
                else
                {
                    printf("%d Not Found\n",i);
                }
            }
            */
            sokoLoadBinLevel(0);
            sokoInitGameBin(soko);
            soko->screen = SOKO_LEVELPLAY;
        }
        else if (label == sokoNewGameLabel)
        {
            // load level.
            sokoLoadLevel(0);
            sokoInitGame(soko);
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
            sokoLoadLevel(soko->loadNewLevelIndex);
            sokoInitNewLevel(soko, sokoLevelVariants[soko->loadNewLevelIndex]);

            soko->screen = SOKO_LEVELPLAY;
        }
    }
}



void freeEntity(soko_abs_t* self, sokoEntity_t* entity) //Free internal entity structures
{
    if(entity->propFlag)
    {
        if(entity->properties->targetCount)
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
    const int HEADER_BYTE_OFFSET = 2;
    int totalTiles = self->currentLevel.width * self->currentLevel.height;
    int tileIndex = 0;
    self->currentLevel.entityCount = 0;
    self->goalCount = 0;
    for(int i = HEADER_BYTE_OFFSET; i < byteCount; i++)
    {
        if(self->levelBinaryData[i] == SKB_OBJSTART) //Objects in level data should be of the form SKB_OBJSTART, SKB_[Object Type], [Data Bytes] , SKB_OBJEND
        {
            int objX = (tileIndex-1) % (self->currentLevel.width); //Look at the previous
            int objY = (tileIndex-1) / (self->currentLevel.width);
            uint8_t flagByte, direction;
            bool players,crates,sticky,trail,inverted;
            int hp, targetX, targetY;
            switch(self->levelBinaryData[i+1]) //On creating entities, index should be advanced to the SKB_OBJEND byte so the post-increment moves to the next tile.
            {
                case SKB_COMPRESS:
                    i += 2;
                    break; //Not yet implemented
                case SKB_PLAYER:
                    self->currentLevel.gameMode = self->levelBinaryData[i+2];
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_PLAYER;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->soko_player = &self->currentLevel.entities[self->currentLevel.playerIndex];
                    self->currentLevel.playerIndex = self->currentLevel.entityCount;
                    self->currentLevel.entityCount+=1;
                    i += 3; 
                    break;
                case SKB_CRATE:
                    flagByte = self->levelBinaryData[i+2];
                    sticky = !!(flagByte & (0x1 << 0));
                    trail = !!(flagByte & (0x1 << 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type = sticky ? SKE_STICKY_CRATE : SKE_CRATE;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky = sticky;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->trail = trail;
                    self->currentLevel.entityCount += 1;
                    i += 3;
                    break;
                case SKB_WARPINTERNAL: //[type][flags][hp][destx][desty]
                    flagByte = self->levelBinaryData[i+2];
                    crates = !!(flagByte & (0x1 << 0));
                    hp = self->levelBinaryData[i+3];
                    targetX = self->levelBinaryData[i+4];
                    targetY = self->levelBinaryData[i+5];
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_WARP;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->hp = hp;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX = malloc(sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY = malloc(sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = 1;
                    self->currentLevel.entityCount+=1;
                    i += 6;
                    break;
                case SKB_WARPINTERNALEXIT:
                    flagByte = self->levelBinaryData[i+2];

                    i += 2; //No data or properties in this object.
                    break; //Can be used later on for verifying valid warps from save files.
                case SKB_WARPEXTERNAL: //[typep][flags][index]
                    //todo implement extraction of index value and which values should be used for auto-indexed portals
                    self->currentLevel.tiles[objX][objY] = SKT_PORTAL;
                    self->portals[self->portalCount].index
                    = self->portalCount + 1; // For basic test, 1 indexed with levels, but multi-room overworld needs
                                             // more sophistication to keep indeces correct.
                    self->portals[self->portalCount].x = objX;
                    self->portals[self->portalCount].y = objY;
                    printf("Portal %d at %d,%d\n", self->portals[self->portalCount].index,
                        self->portals[self->portalCount].x, self->portals[self->portalCount].y);
                    soko->portalCount += 1;
                    i += 4;
                    break;
                case SKB_BUTTON: //[type][flag][numTargets][targetx][targety]...
                    flagByte = self->levelBinaryData[i+2];
                    crates = !!(flagByte & (0x1 << 0));
                    players = !!(flagByte & (0x1 << 1));
                    inverted = !!(flagByte & (0x1 << 2));
                    sticky = !!(flagByte & (0x1 << 3));
                    hp = self->levelBinaryData[i+3];
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_BUTTON;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX = malloc(sizeof(uint8_t) * hp);
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY = malloc(sizeof(uint8_t) * hp);
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->inverted = inverted;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky = sticky;
                    for(int j = 0; j < hp; j++)
                    {
                        self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX[j] = self->levelBinaryData[3 + 2*j + 1];
                        self->currentLevel.entities[self->currentLevel.entityCount].properties->targetY[j] = self->levelBinaryData[3 + 2*(j + 1)];
                    }
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = hp;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->crates = crates;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->inverted = inverted;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->sticky = sticky;
                    self->currentLevel.entityCount+=1;
                    i += (4 + 2*hp);
                    break;
                case SKB_LASEREMITTER: //[type][flag]
                    flagByte = self->levelBinaryData[i+2];
                    direction = (flagByte & (0x3 << 6)) >> 6; // flagbyte stores direction in 0bDD0000P0 Where D is direction bits and P is player push
                    players = !!(flagByte & (0x1 < 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_LASER_EMIT_UP;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX = calloc(SOKO_MAX_ENTITY_COUNT,sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetX = calloc(SOKO_MAX_ENTITY_COUNT,sizeof(uint8_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->targetCount = 0;
                    self->currentLevel.entityCount+=1;
                    i += 3;
                    break;
                case SKB_LASERRECEIVEROMNI:
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_LASER_RECEIVE_OMNI;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entityCount+=1;
                    i += 2;
                    break;
                case SKB_LASERRECEIVER:
                    flagByte = self->levelBinaryData[i+2];
                    direction = (flagByte & (0x3 << 6)) >> 6; // flagbyte stores direction in 0bDD0000P0 Where D is direction bits and P is player push
                    players = !!(flagByte & (0x1 < 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_LASER_RECEIVE;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entityCount+=1;
                    i += 3;
                    break;
                case SKB_LASER90ROTATE:
                    flagByte = self->levelBinaryData[i+2];
                    direction = !!(flagByte & (0x1 < 0));
                    players = !!(flagByte & (0x1 < 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_LASER_90;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].facing = direction;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entityCount+=1;
                    i += 3;
                    break;
                case SKB_GHOSTBLOCK:
                    flagByte = self->levelBinaryData[i+2];
                    inverted = !!(flagByte & (0x1 < 2));
                    players = !!(flagByte & (0x1 < 1));
                    self->currentLevel.entities[self->currentLevel.entityCount].type = SKE_GHOST;
                    self->currentLevel.entities[self->currentLevel.entityCount].x    = objX;
                    self->currentLevel.entities[self->currentLevel.entityCount].y    = objY;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties = malloc(sizeof(sokoEntityProperties_t));
                    self->currentLevel.entities[self->currentLevel.entityCount].propFlag = true;
                    self->currentLevel.entities[self->currentLevel.entityCount].properties->players = players;
                    self->currentLevel.entityCount+=1;
                    i += 3;
                    break;
                case SKB_OBJEND:
                    i += 1;
                    break;
                default: //Make the best of an undefined object type and try to skip it by finding its end byte
                    bool objEndFound = false;
                    int undefinedObjectLength = 0;
                    while(!objEndFound)
                    {
                        undefinedObjectLength += 1;
                        if(self->levelBinaryData[i+undefinedObjectLength] == SKB_OBJEND)
                        {
                            objEndFound = true;
                        }
                        
                    }
                    i += undefinedObjectLength; //Move to the completion byte of an undefined object type and hope it doesn't have two end bytes.
            }
        }
        else
        {
            int tileX = (tileIndex) % (self->currentLevel.width);
            int tileY = (tileIndex) / (self->currentLevel.width);
            //self->currentLevel.tiles[tileX][tileY] = self->levelBinaryData[i];
            int tileType = 0;
            switch(self->levelBinaryData[i]) //This is a bit easier to read than two arrays
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
            //printf("BinData@%d: %d Tile: %d at (%d,%d) index:%d\n",i,self->levelBinaryData[i],tileType,tileX,tileY,tileIndex);
            tileIndex++;
        }
        
    }
}

static void sokoLoadBinLevel(uint16_t levelIndex)
{
    const int HEADER_BYTE_OFFSET = 2; //Number of bytes before tile data begins

    printf("load level %d\n", levelIndex);
    soko->state = SKS_INIT;
    size_t fileSize;
    soko->levelBinaryData = spiffsReadFile(sokoBinLevelNames[levelIndex], &fileSize, true); //Heap CAPS malloc/calloc allocation for SPI RAM
    //The pointer returned by spiffsReadFile can be freed with free() with no additional steps.
    soko->currentLevel.width = soko->levelBinaryData[0]; //first two bytes of a level's data always describe the bounding width and height of the tilemap.
    soko->currentLevel.height = soko->levelBinaryData[1]; //Max Theoretical Level Bounding Box Size is 255x255, though you'll likely run into issues with entities first.
    //for(int i = 0; i < fileSize; i++)
    //{
    //    printf("%d, ",soko->levelBinaryData[i]);
    //}
    //printf("\n");
    soko->currentLevel.levelScale = 16;
    soko->camWidth  = TFT_WIDTH / (soko->currentLevel.levelScale);
    soko->camHeight = TFT_HEIGHT / (soko->currentLevel.levelScale);
    soko->camEnabled = soko->camWidth<soko->currentLevel.width || soko->camHeight< soko->currentLevel.height;
    soko->camPadExtentX = soko->camWidth * 0.6 * 0.5;
    soko->camPadExtentY = soko->camHeight * 0.6 * 0.5;

    soko->currentLevel.entityCount = 0;

    soko->portalCount = 0;

    sokoLoadBinTiles(soko,(int)fileSize);
    //for(int k = 0; k < soko->currentLevel.entityCount; k++)
    //{
    //    printf("Ent%d:%d (%d,%d)",k,soko->currentLevel.entities[k].type,soko->currentLevel.entities[k].x,soko->currentLevel.entities[k].y);
    //}
    //printf("\n");
}

static void sokoLoadLevel(uint16_t levelIndex)
{
    printf("load level %d\n", levelIndex);
    soko->state = SKS_INIT;
    // get image file from selected index
    loadWsg(sokoLevelNames[levelIndex], &soko->levelWSG, false);

    // populate background array
    // populate entities array

    soko->currentLevel.width  = soko->levelWSG.w;
    soko->currentLevel.height = soko->levelWSG.h;

    // player and crate wsg's are 16px right now.
    // In picross I wrote a drawWSGScaled for the main screen when i could get away with it on level select screen. but
    // here I think just commit to something.
    //  Maybe 24? How big are levels going to get?
    soko->currentLevel.levelScale = 16;

    // how many tiles can fit horizontally and vertically. This doesn't change, and here is we we figure out scale.
    // floor
    soko->camWidth  = TFT_WIDTH / (soko->currentLevel.levelScale);
    soko->camHeight = TFT_HEIGHT / (soko->currentLevel.levelScale);

    // enable the camera only if the levelwidth or the levelHeight is greater than camWidth or camHeight.
    // should we enable it independently for x/y if the level is thin?
    soko->camEnabled = soko->camWidth<soko->currentLevel.width || soko->camHeight< soko->currentLevel.height;

    // percentage of screen to let the player move around in. Small for testing.
    //these are half extents. so .7*.5 is 70% of the screen for the movement box. The extent had to be smaller or = than half the camsize.
    soko->camPadExtentX = soko->camWidth * 0.6 * 0.5;
    soko->camPadExtentY = soko->camHeight * 0.6 * 0.5;

    soko->currentLevel.entityCount = 0;
    paletteColor_t sampleColor;
    soko->portalCount = 0;

    for (size_t x = 0; x < soko->currentLevel.width; x++)
    {
        for (size_t y = 0; y < soko->currentLevel.height; y++)
        {
            sampleColor                    = soko->levelWSG.px[y * soko->levelWSG.w + x];
            soko->currentLevel.tiles[x][y] = sokoGetTileFromColor(sampleColor);
            if (soko->currentLevel.tiles[x][y] == SKT_PORTAL)
            {
                soko->portals[soko->portalCount].index
                    = soko->portalCount + 1; // For basic test, 1 indexed with levels, but multi-room overworld needs
                                             // more sophistication to keep indeces correct.
                soko->portals[soko->portalCount].x = x;
                soko->portals[soko->portalCount].y = y;
                printf("Portal %d at %d,%d\n", soko->portals[soko->portalCount].index,
                       soko->portals[soko->portalCount].x, soko->portals[soko->portalCount].y);
                soko->portalCount += 1;
            }
            sokoEntityType_t e = sokoGetEntityFromColor(sampleColor);
            if (e != SKE_NONE)
            {
                soko->currentLevel.entities[soko->currentLevel.entityCount].type = e;
                soko->currentLevel.entities[soko->currentLevel.entityCount].x    = x;
                soko->currentLevel.entities[soko->currentLevel.entityCount].y    = y;
                if (e == SKE_PLAYER)
                {
                    soko->currentLevel.playerIndex = soko->currentLevel.entityCount;
                }
                soko->currentLevel.entityCount = soko->currentLevel.entityCount + 1;
            }
        }
    }
}

static sokoTile_t sokoGetTileFromColor(paletteColor_t col)
{
    // even if player (c005) or crate (c500) is here, they stand on floor. 505 is player and crate, invalid.
    if (col == c555 || col == c005 || col == c500 || col == c101)
    {
        return SKT_FLOOR;
    }
    else if (col == c000)
    {
        return SKT_WALL;
    }
    else if (col == c050 || col == c550 || col == c055)
    { // goal is c050, crate and goal is c550, player and goal is c055
        return SKT_GOAL;
    }
    else if (col == c440) // remember, web safe colors are {0,1,2,3,4,5} cRGB or {0,51,102,153,204,255} decimal.
                          // Increments of 0x33 or 51. cABC = 0x(0x33*A)(0x33*B)(0x33*C)
    {
        return SKT_PORTAL;
    }
    // transparent or invalid is empty. Todo: can catch transparent and report error otherwise... once comitted to
    // encoding scheme.
    return SKT_EMPTY;
}

static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t col)
{
    // todo: get actual rgb value from the paletteColors array and check if the rgb values > 0.
    if (col == c500 || col == c550)
    {
        return SKE_CRATE;
    }
    else if (col == c005 || col == c055)
    { // has green. r and b used for entity. g for tile.
        return SKE_PLAYER;
    }
    else if (col == c101)
    {
        return SKE_STICKY_CRATE;
    }

    return SKE_NONE;
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
    //Filenames are formatted like '1:sk_level.bin:'
    int retVal = -1;
    for(int i = 0; i < targetIndex; i++)
    {
        if(self->levelIndeces[i] == targetIndex)
        {
            retVal = i;
        }
    }
    return retVal;
}

static void sokoExtractLevelNamesAndIndeces(soko_abs_t* self)
{
    printf("Loading Level List...!\n");
    printf("%s\n",self->levelFileText);
    printf("%d\n",(int)strlen(self->levelFileText));
    //char* a = strstr(self->levelFileText,":");
    //char* b = strstr(a,".bin:");
    //printf("%d",(int)((int)b-(int)a));
    //char* stringPtrs[30];
    //memset(stringPtrs,0,30*sizeof(char*));
    char** stringPtrs = soko->levelNames;
    memset(stringPtrs,0,SOKO_LEVEL_COUNT*sizeof(char*));
    int* levelInds = soko->levelIndeces;
    memset(levelInds,0,SOKO_LEVEL_COUNT*sizeof(int));
    int intInd = 0;
    int ind = 0;
    char* storageStr = strtok(self->levelFileText,":");
    while(storageStr != NULL)
    {
        if(strtol(storageStr,NULL,10) && !(strstr(storageStr,".bin"))) //Make sure you're not accidentally reading a number from a filename
        {
            levelInds[intInd] = (int)strtol(storageStr,NULL,10);
            //printf("NumberThing: %s :: %d\n",storageStr,(int)strtol(storageStr,NULL,10));
            intInd++;
        }
        else{
            if(!strpbrk(storageStr,"\n\t\r ") && (strstr(storageStr,".bin")))
            {
                int tokLen = strlen(storageStr);
                char* tempPtr = calloc((tokLen+1),sizeof(char)); //Length plus null teminator
                //strcpy(tempPtr,storageStr);
                //stringPtrs[ind] = tempPtr;
                stringPtrs[ind] = storageStr;
                //printf("%s\n",storageStr);
            ind++;
            }
        }
        //printf("This guy!\n");
        storageStr = strtok(NULL,":");
    }
    printf("Strings: %d, Ints: %d\n", ind, intInd);
    printf("Levels and Indeces:\n");
    for(int i = ind-1; i > -1; i--)
    {
        printf("Index: %d : %d : %s\n",i,levelInds[i],stringPtrs[i]);
    }

}

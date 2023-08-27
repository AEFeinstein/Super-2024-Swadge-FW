#include "soko.h"
#include "soko_game.h"
#include "soko_gamerules.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoLoadLevel(uint16_t);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static sokoTile_t sokoGetTileFromColor(paletteColor_t);
static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t);

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
    loadWsg("sk_player_down.wsg", &soko->playerDownWSG, false);
    loadWsg("sk_player_up.wsg", &soko->playerUpWSG, false);
    loadWsg("sk_player_left.wsg", &soko->playerLeftWSG, false);
    loadWsg("sk_player_right.wsg", &soko->playerRightWSG, false);
    loadWsg("sk_crate.wsg", &soko->crateWSG, false);
    loadWsg("sk_sticky_crate.wsg", &soko->stickyCrateWSG, false);

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

    // free the level
    freeWsg(&soko->levelWSG);

    // free sprites
    freeWsg(&soko->playerUpWSG);
    freeWsg(&soko->playerDownWSG);
    freeWsg(&soko->playerLeftWSG);
    freeWsg(&soko->playerRightWSG);
    freeWsg(&soko->crateWSG);
    freeWsg(&soko->stickyCrateWSG);

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
            sokoLoadLevel(0);
            sokoInitGame(soko);
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
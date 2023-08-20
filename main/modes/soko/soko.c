#include "soko.h"
#include "soko_game.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoLoadLevel(uint16_t);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static sokoTile_t sokoGetTileFromColor(paletteColor_t);
static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t);


//strings
static const char sokoModeName[] = "Sokobanabokabon";
static const char sokoResumeGameLabel[] = "returnitytoit";
static const char sokoNewGameLabel[] = "startsitfresh";

//create the mode
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

//soko_t* soko=NULL;
soko_abs_t* soko=NULL;
static void sokoEnterMode(void)
{
    soko = calloc(1, sizeof(soko_abs_t));
    // Load a font
    loadFont("ibm_vga8.font", &soko->ibm, false);
    
    //todo: move to convenience function for loading level data. Preferrebly in it's own file so contributors can futz with it with fewer git merge cases.
    soko->levels[0]="sk_testpuzzle.wsg";
    
    //free a wsg that we never loaded... is bad.
    loadWsg(soko->levels[0],&soko->levelWSG,true);//spiRAM cus only used during loading, not gameplay.

    //load sprite assets
    loadWsg("sk_player_down.wsg",&soko->playerDownWSG,false);
    loadWsg("sk_player_up.wsg",&soko->playerUpWSG,false);
    loadWsg("sk_player_left.wsg",&soko->playerLeftWSG,false);
    loadWsg("sk_player_right.wsg",&soko->playerRightWSG,false);
    loadWsg("sk_crate.wsg",&soko->crateWSG,false);

    // Initialize the menu
    soko->menu                = initMenu(sokoModeName, sokoMenuCb);
    soko->menuLogbookRenderer = initMenuLogbookRenderer(&soko->ibm);

    addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoNewGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
    soko->state = SKS_INIT;

}

static void sokoExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenuLogbookRenderer(soko->menuLogbookRenderer);

    // Free the font
    freeFont(&soko->ibm);

    //free the level
    freeWsg(&soko->levelWSG);

    //free sprites
    freeWsg(&soko->playerUpWSG);
    freeWsg(&soko->playerDownWSG);
    freeWsg(&soko->playerLeftWSG);
    freeWsg(&soko->playerRightWSG);
    freeWsg(&soko->crateWSG);

    // Free everything else
    free(soko);
}


static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
       //placeholder.
       if(label == sokoResumeGameLabel)
       {
            //load level.
            sokoLoadLevel(0);
            sokoInitGame(soko);
            soko->screen = SOKO_LEVELPLAY;
       }else if(label == sokoNewGameLabel)
       {
            //load level.
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
            //pass along to other gameplay, in other file
             // Always process button events, regardless of control scheme, so the main menu button can be captured
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Save the button state
                soko->input.btnState = evt.state;
            }

            //process input functions in input.
            //Input will turn state into function calls into the game code, and handle complexities.
            sokoPreProcessInput(&soko->input,elapsedUs);
            //background had been drawn, input has been processed and functions called. Now do followup logic and draw level.
            //gameplay loop
            soko->gameLoopFunc(soko, elapsedUs);
        }
    }
}

static void sokoLoadLevel(uint16_t levelIndex)
{
    printf("load level\n");
    soko->state = SKS_INIT;
    //get image file from selected index
    loadWsg(soko->levels[levelIndex],&soko->levelWSG,false);

    //populate background array
    //populate entities array

    soko->currentLevel.width = soko->levelWSG.w;
    soko->currentLevel.height = soko->levelWSG.h;
    
    
    //player and crate wsg's are 16px right now.
    //In picross I wrote a drawWSGScaled for the main screen when i could get away with it on level select screen. but here I think just commit to something.
    // Maybe 24? How big are levels going to get?
    soko->currentLevel.levelScale = 16;

    soko->currentLevel.entityCount = 0;
    paletteColor_t sampleColor;

    for (size_t x = 0; x < soko->currentLevel.width; x++)
    {
        for (size_t y = 0; y < soko->currentLevel.height; y++)
        {
            sampleColor = soko->levelWSG.px[y * soko->levelWSG.w + x];
            soko->currentLevel.tiles[x][y] = sokoGetTileFromColor(sampleColor);
            sokoEntityType_t e = sokoGetEntityFromColor(sampleColor);
            if(e != SKE_NONE){
                soko->currentLevel.entities[soko->currentLevel.entityCount].type = e;
                soko->currentLevel.entities[soko->currentLevel.entityCount].x = x;
                soko->currentLevel.entities[soko->currentLevel.entityCount].y = y;
                if(e == SKE_PLAYER)
                {
                    soko->currentLevel.playerIndex = soko->currentLevel.entityCount;
                }
                soko->currentLevel.entityCount = soko->currentLevel.entityCount+1;
            }
        }
    }
}

static sokoTile_t sokoGetTileFromColor(paletteColor_t col)
{
    //even if player (c005) or crate (c500) is here, they stand on floor. 505 is player and crate, invalid.
    if(col== c555 || col == c005 || col == c500){
        return SKT_FLOOR;
    }else if(col == c000)
    {
        return SKT_WALL;
    }else if(col == c050 || col == c550 || col == c055){//goal is c050, crate and goal is c550, player and goal is c055
        return SKT_GOAL;
    }
    //transparent or invalid is empty. Todo: can catch transparent and report error otherwise... once comitted to encoding scheme.
    return SKT_EMPTY;
}

static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t col)
{
    //todo: get actual rgb value from the paletteColors array and check if the rgb values > 0.
    if(col == c500 || col == c550)
    {
        return SKE_CRATE;
    }else if(col == c005 || col == c055){//has green. r and b used for entity. g for tile.
        return SKE_PLAYER;
    }

    return SKE_NONE;
}

//placeholder.
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
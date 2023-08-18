#include "soko.h"
#include "soko_game.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoLoadLevel(uint16_t);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

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

soko_t* soko=NULL;
static void sokoEnterMode(void)
{
    soko = calloc(1, sizeof(soko_t));
    // Load a font
    loadFont("ibm_vga8.font", &soko->ibm, false);

    // Initialize the menu
    soko->menu                = initMenu(sokoModeName, sokoMenuCb);
    soko->menuLogbookRenderer = initMenuLogbookRenderer(&soko->ibm);

    addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoNewGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
}

static void sokoExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenuLogbookRenderer(soko->menuLogbookRenderer);
    // Free the font
    freeFont(&soko->ibm);

    //free the level

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
            sokoPreProcessInput(&soko->input);
            //background had been drawn, input has been processed and functions called. Now do followup logic and draw level.
            //gameplay loop
            gameLoop(elapsedUs);
        }
    }
}

static void sokoLoadLevel(uint16_t levelIndex)
{
    //get image file from selected index
    //populate background array
    //populate entities array

    //here we learn how big the level is from the image.

    soko->currentLevel.width = 7;
    soko->currentLevel.height = 7;
    
    for (size_t x = 0; x < soko->currentLevel.width; x++)
    {
        for (size_t y = 0; y < soko->currentLevel.height; y++)
        {
            if(x == 0 || y == 0)
            {
                soko->currentLevel.tiles[x][y] = SK_WALL;
            }else{
                soko->currentLevel.tiles[x][y] = SK_EMPTY;
            }
        }
    }
    
    soko->currentLevel.tiles[4][4] = SK_WALL;
    soko->currentLevel.tiles[5][2] = SK_GOAL;
    soko->currentLevel.tiles[5][3] = SK_GOAL;


    printf("create entities");
    soko->currentLevel.playerIndex = 0;
    soko->currentLevel.entityCount = 3;

    soko->currentLevel.entities[soko->currentLevel.playerIndex].type = SKE_PLAYER;
    soko->currentLevel.entities[soko->currentLevel.playerIndex].x = 1;
    soko->currentLevel.entities[soko->currentLevel.playerIndex].y = 1;

    soko->currentLevel.entities[1].type = SKE_CRATE;
    soko->currentLevel.entities[1].x = 2;
    soko->currentLevel.entities[1].y = 2;

    soko->currentLevel.entities[2].type = SKE_CRATE;
    soko->currentLevel.entities[2].x = 3;
    soko->currentLevel.entities[2].y = 3;
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
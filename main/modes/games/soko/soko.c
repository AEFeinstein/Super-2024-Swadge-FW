#include <string.h>

#include "soko.h"
#include "soko_game.h"
#include "soko_gamerules.h"
#include "soko_save.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static sokoTile_t sokoGetTileFromColor(paletteColor_t);
static sokoEntityType_t sokoGetEntityFromColor(paletteColor_t);
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self);
static void sokoLoadBinTiles(soko_abs_t* self, int byteCount);

// strings
static const char sokoModeName[]        = "Sokobanabokabon";
static const char sokoResumeGameLabel[] = "returnitytoit";
static const char sokoNewGameLabel[]    = "startsyfreshy";

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

static void sokoEnterMode(void)
{
    soko = calloc(1, sizeof(soko_abs_t));
    // Load a font
    loadFont("ibm_vga8.font", &soko->ibm, false);

    // load sprite assets
    //set pointer
    soko->currentTheme = &soko->sokoDefaultTheme;
    soko->sokoDefaultTheme.wallColor  = c111;
    soko->sokoDefaultTheme.floorColor = c444;
    soko->background = SKBG_GRID;
    //load or set themes...
    // Default Theme
    loadWsg("sk_pixel_front.wsg", &soko->sokoDefaultTheme.playerDownWSG, false);
    loadWsg("sk_pixel_back.wsg", &soko->sokoDefaultTheme.playerUpWSG, false);
    loadWsg("sk_pixel_left.wsg", &soko->sokoDefaultTheme.playerLeftWSG, false);
    loadWsg("sk_pixel_right.wsg", &soko->sokoDefaultTheme.playerRightWSG, false);
    loadWsg("sk_crate_2.wsg", &soko->sokoDefaultTheme.crateWSG, false);
    loadWsg("sk_crate_ongoal.wsg", &soko->sokoDefaultTheme.crateOnGoalWSG,false);
    loadWsg("sk_sticky_crate.wsg", &soko->sokoDefaultTheme.stickyCrateWSG, false);
    loadWsg("sk_portal_complete.wsg", &soko->sokoDefaultTheme.portal_completeWSG, false);
    loadWsg("sk_portal_incomplete.wsg", &soko->sokoDefaultTheme.portal_incompleteWSG, false);
    loadWsg("sk_goal.wsg", &soko->sokoDefaultTheme.goalWSG,false);

    //we check against 0,0 as an invalid start location, and use file location instead.
    soko->overworld_playerX = 0;
    soko->overworld_playerY = 0;

    // Overworld Theme
    soko->overworldTheme.playerDownWSG  = soko->sokoDefaultTheme.playerDownWSG;
    soko->overworldTheme.playerUpWSG    = soko->sokoDefaultTheme.playerUpWSG;
    soko->overworldTheme.playerLeftWSG  = soko->sokoDefaultTheme.playerLeftWSG;
    soko->overworldTheme.playerRightWSG = soko->sokoDefaultTheme.playerRightWSG;
    soko->overworldTheme.crateWSG       = soko->sokoDefaultTheme.crateWSG;
    soko->overworldTheme.goalWSG        = soko->sokoDefaultTheme.goalWSG;
    soko->overworldTheme.crateOnGoalWSG = soko-> sokoDefaultTheme.crateOnGoalWSG;
    soko->overworldTheme.stickyCrateWSG = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->overworldTheme.portal_completeWSG = soko->sokoDefaultTheme.portal_completeWSG;
    soko->overworldTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->overworldTheme.wallColor  = c111;
    soko->overworldTheme.floorColor = c444;


    // Euler Theme
    soko->eulerTheme.playerDownWSG  = soko->sokoDefaultTheme.playerDownWSG;
    soko->eulerTheme.playerUpWSG    = soko->sokoDefaultTheme.playerUpWSG;
    soko->eulerTheme.playerLeftWSG  = soko->sokoDefaultTheme.playerLeftWSG;
    soko->eulerTheme.playerRightWSG = soko->sokoDefaultTheme.playerRightWSG;
    soko->eulerTheme.goalWSG        = soko->sokoDefaultTheme.goalWSG;

    loadWsg("sk_e_crate.wsg", &soko->eulerTheme.crateWSG, false);
    loadWsg("sk_sticky_trail_crate.wsg", &soko->eulerTheme.crateOnGoalWSG, false);
    soko->eulerTheme.stickyCrateWSG = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->eulerTheme.portal_completeWSG = soko->sokoDefaultTheme.portal_completeWSG;
    soko->eulerTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->eulerTheme.wallColor  = c000;
    soko->eulerTheme.floorColor = c555;

    // Initialize the menu
    soko->menu                = initMenu(sokoModeName, sokoMenuCb);
    soko->menuManiaRenderer = initMenuManiaRenderer(&soko->ibm, NULL, NULL);

    addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoNewGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
    soko->state  = SKS_INIT;

    //load up the level list.
    soko->levelFileText = loadTxt("SK_LEVEL_LIST.txt", true);
    sokoExtractLevelNamesAndIndices(soko);

    //load level solved state.
    sokoLoadLevelSolvedState(soko);
}

static void sokoExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenuManiaRenderer(soko->menuManiaRenderer);

    // Free the font
    freeFont(&soko->ibm);

    // free the level name file
    freeTxt(soko->levelFileText);

    // free sprites
    //  default
    freeWsg(&soko->sokoDefaultTheme.playerUpWSG);
    freeWsg(&soko->sokoDefaultTheme.playerDownWSG);
    freeWsg(&soko->sokoDefaultTheme.playerLeftWSG);
    freeWsg(&soko->sokoDefaultTheme.playerRightWSG);
    freeWsg(&soko->sokoDefaultTheme.crateWSG);
    freeWsg(&soko->sokoDefaultTheme.crateOnGoalWSG);
    freeWsg(&soko->sokoDefaultTheme.stickyCrateWSG);
    freeWsg(&soko->sokoDefaultTheme.portal_completeWSG);
    freeWsg(&soko->sokoDefaultTheme.portal_incompleteWSG);
    freeWsg(&soko->sokoDefaultTheme.goalWSG);
    //  euler
    freeWsg(&soko->eulerTheme.crateWSG);
    freeWsg(&soko->eulerTheme.crateOnGoalWSG);
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
            int32_t data;
            readNvs32("sk_data",&data);
            //bitshift, etc, as needed.
            uint16_t lastSaved = (uint16_t)data;
            sokoLoadGameplay(soko,lastSaved,false);
            sokoInitGameBin(soko);
            soko->screen = SOKO_LEVELPLAY;
        }
        else if (label == sokoNewGameLabel)
        {
            // load level.
            //we probably shouldn't have a new game option; just an overworld option.
            sokoLoadGameplay(soko,0,true);    
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
            drawMenuMania(soko->menu, soko->menuManiaRenderer, elapsedUs);
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
            sokoLoadGameplay(soko, soko->loadNewLevelIndex,soko->loadNewLevelFlag);
            sokoInitNewLevel(soko, soko->currentLevel.gameMode);
            printf("Go to gameplay\n");
            soko->loadNewLevelFlag = false;//reset flag.
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

// placeholder.
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    if(soko->background == SKBG_GRID){
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
    }else{//SKBG_BLACK
        for (int16_t yp = y; yp < y + h; yp++)
        {
            for (int16_t xp = x; xp < x + w; xp++)
            {

                TURBO_SET_PIXEL(xp, yp, c000);
            }
        }
    }
}
//todo: move to soko_save
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self)
{
    printf("Loading Level List...!\n");
    //printf("%s\n", self->levelFileText);
    //printf("%d\n", (int)strlen(self->levelFileText));
    // char* a = strstr(self->levelFileText,":");
    // char* b = strstr(a,".bin:");
    // printf("%d",(int)((int)b-(int)a));
    // char* stringPtrs[30];
    // memset(stringPtrs,0,30*sizeof(char*));
    char** stringPtrs = soko->levelNames;
    memset(stringPtrs, 0, SOKO_LEVEL_COUNT * sizeof(char*));
    memset(soko->levelIndices, 0, SOKO_LEVEL_COUNT * sizeof(int));
    int intInd       = 0;
    int ind          = 0;
    char* storageStr = strtok(self->levelFileText, ":");
    while (storageStr != NULL)
    {
        //strtol(storageStr, NULL, 10) && 
        if (!(strstr(storageStr, ".bin"))) // Make sure you're not accidentally reading a number from a filename
        {
            soko->levelIndices[intInd] = (int)strtol(storageStr, NULL, 10);
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
        printf("Index: %d : %d : %s\n", i, soko->levelIndices[i], stringPtrs[i]);
    }
}

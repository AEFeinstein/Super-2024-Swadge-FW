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
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self);

// strings
static const char sokoModeName[]        = "Hunter's Block Puzzles";
static const char sokoPlayGameLabel[] = "Play";

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

    soko->allSolved = false;

    // load sprite assets
    // set pointer
    soko->currentTheme                   = &soko->sokoDefaultTheme;
    soko->sokoDefaultTheme.wallColor     = c111;
    soko->sokoDefaultTheme.floorColor    = c444;
    soko->sokoDefaultTheme.altFloorColor = c444;
    soko->background                     = SKBG_BLACK;
    // load or set themes...
    //  Default Theme
    loadWsg("sk_player1.wsg", &soko->sokoDefaultTheme.playerDownWSG, false);
    loadWsg("sk_player2.wsg", &soko->sokoDefaultTheme.playerUpWSG, false);
    loadWsg("sk_player3.wsg", &soko->sokoDefaultTheme.playerLeftWSG, false);
    loadWsg("sk_player4.wsg", &soko->sokoDefaultTheme.playerRightWSG, false);
    loadWsg("sk_crate_2.wsg", &soko->sokoDefaultTheme.crateWSG, false);
    loadWsg("sk_crate_ongoal.wsg", &soko->sokoDefaultTheme.crateOnGoalWSG, false);
    loadWsg("sk_sticky_crate.wsg", &soko->sokoDefaultTheme.stickyCrateWSG, false);
    loadWsg("sk_portal_complete.wsg", &soko->sokoDefaultTheme.portal_completeWSG, false);
    loadWsg("sk_portal_incomplete.wsg", &soko->sokoDefaultTheme.portal_incompleteWSG, false);
    loadWsg("sk_goal.wsg", &soko->sokoDefaultTheme.goalWSG, false);

    // we check against 0,0 as an invalid start location, and use file location instead.
    soko->overworld_playerX = 0;
    soko->overworld_playerY = 0;

    // Overworld Theme
    soko->overworldTheme.playerDownWSG        = soko->sokoDefaultTheme.playerDownWSG;
    soko->overworldTheme.playerUpWSG          = soko->sokoDefaultTheme.playerUpWSG;
    soko->overworldTheme.playerLeftWSG        = soko->sokoDefaultTheme.playerLeftWSG;
    soko->overworldTheme.playerRightWSG       = soko->sokoDefaultTheme.playerRightWSG;
    soko->overworldTheme.crateWSG             = soko->sokoDefaultTheme.crateWSG;
    soko->overworldTheme.goalWSG              = soko->sokoDefaultTheme.goalWSG;
    soko->overworldTheme.crateOnGoalWSG       = soko->sokoDefaultTheme.crateOnGoalWSG;
    soko->overworldTheme.stickyCrateWSG       = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->overworldTheme.portal_completeWSG   = soko->sokoDefaultTheme.portal_completeWSG;
    soko->overworldTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->overworldTheme.wallColor            = c111;
    soko->overworldTheme.floorColor           = c444;

    // Euler Theme
    soko->eulerTheme.playerDownWSG  = soko->sokoDefaultTheme.playerDownWSG;
    soko->eulerTheme.playerUpWSG    = soko->sokoDefaultTheme.playerUpWSG;
    soko->eulerTheme.playerLeftWSG  = soko->sokoDefaultTheme.playerLeftWSG;
    soko->eulerTheme.playerRightWSG = soko->sokoDefaultTheme.playerRightWSG;
    soko->eulerTheme.goalWSG        = soko->sokoDefaultTheme.goalWSG;

    loadWsg("sk_e_crate.wsg", &soko->eulerTheme.crateWSG, false);
    loadWsg("sk_sticky_trail_crate.wsg", &soko->eulerTheme.crateOnGoalWSG, false);
    soko->eulerTheme.stickyCrateWSG       = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->eulerTheme.portal_completeWSG   = soko->sokoDefaultTheme.portal_completeWSG;
    soko->eulerTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->eulerTheme.wallColor            = c000;
    soko->eulerTheme.floorColor           = c555;
    soko->eulerTheme.altFloorColor        = c433; // painted tiles color.

    // Initialize the menu
    soko->menu              = initMenu(sokoModeName, sokoMenuCb);
    soko->menuManiaRenderer = initMenuManiaRenderer(&soko->ibm, NULL, NULL);

    //addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoPlayGameLabel);

    // Set the mode to menu mode
    soko->screen = SOKO_MENU;
    soko->state  = SKS_INIT;

    // load up the level list.
    soko->levelFileText = loadTxt("SK_LEVEL_LIST.txt", true);
    sokoExtractLevelNamesAndIndices(soko);

    // load level solved state.
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
    free(soko->levelBinaryData); // TODO is this the best place to free?
    // Free everything else
    free(soko);
}

static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        // placeholder.
        if (label == sokoPlayGameLabel)
        {
            int32_t overworld_player;
            if(readNvs32("sk_overworldPos", &overworld_player))
            {
                soko->overworld_playerX = (uint16_t)(overworld_player & 0b1111111111111111);//okay so the cast to uint16 just does this right?
                soko->overworld_playerY = (uint16_t)(overworld_player >> 16);
            }else{
                soko->overworld_playerX = 0;
                soko->overworld_playerY = 0;
            }
            printf("Load Overworld: %d,%d - {%d}\n", soko->overworld_playerX, soko->overworld_playerY, overworld_player);

            //if x == 0 && y == 0, then put the player somewhere else.

            sokoLoadGameplay(soko, 0, false);
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
            sokoLoadGameplay(soko, soko->loadNewLevelIndex, soko->loadNewLevelFlag);
            sokoInitNewLevel(soko, soko->currentLevel.gameMode);
            printf("Go to gameplay\n");
            soko->loadNewLevelFlag = false; // reset flag.
            soko->screen           = SOKO_LEVELPLAY;
        }
    }
}

static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();
    uint16_t shiftReg = 0xACE1u;
    uint16_t bit      = 0;
    switch (soko->background)
    {
        case SKBG_GRID:
        {
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
            break;
        }
        case SKBG_BLACK:
        {
            for (int16_t yp = y; yp < y + h; yp++)
            {
                for (int16_t xp = x; xp < x + w; xp++)
                {
                    TURBO_SET_PIXEL(xp, yp, c000);
                }
            }
            break;
        }
        case SKBG_FORREST:
        {
            for (int16_t yp = y; yp < y + h; yp += 8)
            {
                for (int16_t xp = x; xp < x + w; xp += 8)
                {
                    // not random enough but im going to leave it as is.
                    // LFSR
                    bit      = ((shiftReg >> 0) ^ (shiftReg >> 2) ^ (shiftReg >> 3) ^ (shiftReg >> 5)) & 1u;
                    shiftReg = (shiftReg >> 1) | (bit << 15);
                    shiftReg = shiftReg + yp + xp * 3 + 1;

                    for (int16_t ypp = yp; ypp < yp + 8; ypp++)
                    {
                        for (int16_t xpp = xp; xpp < xp + 8; xpp++)
                        {
                            if ((shiftReg & 3) == 0)
                            {
                                TURBO_SET_PIXEL(xpp, ypp, c020);
                            }
                            else
                            {
                                TURBO_SET_PIXEL(xpp, ypp, c121);
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
// todo: move to soko_save
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self)
{
   
    char** stringPtrs = soko->levelNames;
    memset(stringPtrs, 0, sizeof(soko->levelNames));
    memset(soko->levelTitles, 0, sizeof(soko->levelTitles));
    memset(soko->levelIndices, 0, sizeof(soko->levelIndices));
    int intInd       = 0;
    int ind          = 0;
    char* storageStr = strtok(self->levelFileText, ":");
    while (storageStr != NULL)
    {
        // strtol(storageStr, NULL, 10) &&
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
                int tokLen = strlen(storageStr);
                // char* tempPtr = calloc((tokLen + 1), sizeof(char)); // Length plus null teminator
                // strcpy(tempPtr,storageStr);
                // stringPtrs[ind] = tempPtr;

                //remove the sk_e_ and .bin from the filename and copy to title.
                stringPtrs[ind] = storageStr;
                char* title = malloc(tokLen-9);
                strncpy(title, storageStr+5,tokLen-9);
                
                //change _ to spaces
                for(int i = 0; i < strlen(title); i++)
                {
                    if(title[i] == '_'){
                        title[i] = ' ';
                    }
                }
                //set title
                soko->levelTitles[ind] = title;

                ind++;
            }
        }
        // printf("This guy!\n");
        storageStr = strtok(NULL, ":");
    }

}

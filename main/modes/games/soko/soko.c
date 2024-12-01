#include <string.h>

#include "soko.h"
#include "soko_game.h"
#include "soko_gamerules.h"
#include "soko_save.h"
#include "sokoHelp.h"
#include "mainMenu.h"

static void sokoMainLoop(int64_t elapsedUs);
static void sokoEnterMode(void);
static void sokoExitMode(void);
static void sokoMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sokoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void sokoExtractLevelNamesAndIndices(soko_abs_t* self);

// strings
const char sokoModeName[]             = "Hunter's Puzzles";
static const char sokoPlayGameLabel[] = "Play";
static const char sokoHelpLabel[]     = "Instructions";
static const char sokoExitLabel[]     = "Exit";

const char SOKO_TAG[] = "SB";

extern const char key_sk_overworldPos[];

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
    soko = heap_caps_calloc(1, sizeof(soko_abs_t), MALLOC_CAP_SPIRAM);
    // Load a font
    loadFont("ibm_vga8.font", &soko->ibm, true);
    loadFont("rodin_eb.font", &soko->font_rodin, true);
    makeOutlineFont(&soko->font_rodin, &soko->font_rodin_outline, true);
    loadFont("righteous_150.font", &soko->font_righteous, true);
    makeOutlineFont(&soko->font_righteous, &soko->font_righteous_outline, true);

    soko->allSolved = false;

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
    loadWsg("sk_player5.wsg", &soko->sokoDefaultTheme.playerCenterWSG, false);
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
    soko->overworldTheme.playerCenterWSG      = soko->sokoDefaultTheme.playerCenterWSG;
    soko->overworldTheme.crateWSG             = soko->sokoDefaultTheme.crateWSG;
    soko->overworldTheme.goalWSG              = soko->sokoDefaultTheme.goalWSG;
    soko->overworldTheme.crateOnGoalWSG       = soko->sokoDefaultTheme.crateOnGoalWSG;
    soko->overworldTheme.stickyCrateWSG       = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->overworldTheme.portal_completeWSG   = soko->sokoDefaultTheme.portal_completeWSG;
    soko->overworldTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->overworldTheme.wallColor            = c111;
    soko->overworldTheme.floorColor           = c444;

    // Euler Theme
    soko->eulerTheme.playerDownWSG   = soko->sokoDefaultTheme.playerDownWSG;
    soko->eulerTheme.playerUpWSG     = soko->sokoDefaultTheme.playerUpWSG;
    soko->eulerTheme.playerLeftWSG   = soko->sokoDefaultTheme.playerLeftWSG;
    soko->eulerTheme.playerRightWSG  = soko->sokoDefaultTheme.playerRightWSG;
    soko->eulerTheme.playerCenterWSG = soko->sokoDefaultTheme.playerCenterWSG;
    soko->eulerTheme.goalWSG         = soko->sokoDefaultTheme.goalWSG;

    loadWsg("sk_e_crate.wsg", &soko->eulerTheme.crateWSG, false);
    loadWsg("sk_sticky_trail_crate.wsg", &soko->eulerTheme.crateOnGoalWSG, false);
    soko->eulerTheme.stickyCrateWSG       = soko->sokoDefaultTheme.stickyCrateWSG;
    soko->eulerTheme.portal_completeWSG   = soko->sokoDefaultTheme.portal_completeWSG;
    soko->eulerTheme.portal_incompleteWSG = soko->sokoDefaultTheme.portal_incompleteWSG;
    soko->eulerTheme.wallColor            = c000;
    soko->eulerTheme.floorColor           = c555;
    soko->eulerTheme.altFloorColor        = c433; // painted tiles color.

    // Initialize the menu
    soko->menu   = initMenu(sokoModeName, sokoMenuCb);
    soko->bgMenu = initMenu(sokoModeName, NULL);
    soko->menuManiaRenderer
        = initMenuManiaRenderer(&soko->font_righteous, &soko->font_righteous_outline, &soko->font_rodin);

    // addSingleItemToMenu(soko->menu, sokoResumeGameLabel);
    addSingleItemToMenu(soko->menu, sokoPlayGameLabel);
    addSingleItemToMenu(soko->menu, sokoHelpLabel);
    addSingleItemToMenu(soko->menu, sokoExitLabel);

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
    for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(soko->levelTitles); lIdx++)
    {
        if (soko->levelTitles[lIdx])
        {
            free(soko->levelTitles[lIdx]);
        }
    }
    // Deinitialize the menu
    deinitMenu(soko->menu);
    deinitMenu(soko->bgMenu);
    deinitMenuManiaRenderer(soko->menuManiaRenderer);

    // Free the font
    freeFont(&soko->ibm);
    freeFont(&soko->font_rodin);
    freeFont(&soko->font_rodin_outline);
    freeFont(&soko->font_righteous);
    freeFont(&soko->font_righteous_outline);

    // free the level name file
    freeTxt(soko->levelFileText);

    // free sprites
    //  default
    freeWsg(&soko->sokoDefaultTheme.playerUpWSG);
    freeWsg(&soko->sokoDefaultTheme.playerDownWSG);
    freeWsg(&soko->sokoDefaultTheme.playerLeftWSG);
    freeWsg(&soko->sokoDefaultTheme.playerRightWSG);
    freeWsg(&soko->sokoDefaultTheme.playerCenterWSG);
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
            if (readNvs32(key_sk_overworldPos, &overworld_player))
            {
                soko->overworld_playerX
                    = (uint16_t)(overworld_player
                                 & 0b1111111111111111); // okay so the cast to uint16 just does this right?
                soko->overworld_playerY = (uint16_t)(overworld_player >> 16);
            }
            else
            {
                soko->overworld_playerX = 0;
                soko->overworld_playerY = 0;
            }
            ESP_LOGD(SOKO_TAG, "Load Overworld: %" PRIu16 ",%" PRIu16 " - {%" PRId32 "}", soko->overworld_playerX,
                     soko->overworld_playerY, overworld_player);

            // if x == 0 && y == 0, then put the player somewhere else.

            sokoLoadGameplay(soko, 0, false);
            sokoInitGameBin(soko);
            soko->screen = SOKO_LEVELPLAY;
        }
        else if (sokoHelpLabel == label)
        {
            soko->helpIdx = 0;
            soko->screen  = SOKO_HELP;
        }
        else if (sokoExitLabel == label)
        {
            switchToSwadgeMode(&mainMenuMode);
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
            ESP_LOGD(SOKO_TAG, "Go to gameplay");
            soko->loadNewLevelFlag = false; // reset flag.
            soko->screen           = SOKO_LEVELPLAY;
            break;
        }
        case SOKO_HELP:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                buttonSokoHelp(soko, &evt);
            }
            drawSokoHelp(soko, elapsedUs);
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
            // ESP_LOGD(SOKO_TAG, "NumberThing: %s :: %d",storageStr,(int)strtol(storageStr,NULL,10));
            intInd++;
        }
        else
        {
            if (!strpbrk(storageStr, "\n\t\r ") && (strstr(storageStr, ".bin")))
            {
                int tokLen = strlen(storageStr);
                // char* tempPtr = heap_caps_malloc((tokLen + 1), sizeof(char), MALLOC_CAP_SPIRAM) ; // Length plus null
                // teminator strcpy(tempPtr,storageStr); stringPtrs[ind] = tempPtr;

                // remove the sk_e_ and .bin from the filename and copy to title.
                stringPtrs[ind] = storageStr;
                char* title     = heap_caps_calloc(tokLen - 8, sizeof(char), MALLOC_CAP_SPIRAM);
                strncpy(title, storageStr + 5, tokLen - 9);

                // change _ to spaces
                for (int i = 0; i < strlen(title); i++)
                {
                    if (title[i] == '_')
                    {
                        title[i] = ' ';
                    }
                }
                // set title
                soko->levelTitles[ind] = title;

                ind++;
            }
        }
        // ESP_LOGD(SOKO_TAG, "This guy!");
        storageStr = strtok(NULL, ":");
    }
}

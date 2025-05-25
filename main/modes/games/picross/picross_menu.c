//==============================================================================
// Functions
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "swadge2024.h"

#include "picross_menu.h"
#include "mode_picross.h"
#include "picross_select.h"
#include "picross_tutorial.h"
#include "picross_consts.h"
#include "picross_music.h"
//==============================================================================
// Enums & Structs
//==============================================================================

typedef enum
{
    PICROSS_MENU,
    PICROSS_LEVELSELECT,
    PICROSS_OPTIONS,
    PICROSS_GAME,
    PICROSS_TUTORIAL
} picrossScreen_t;

typedef struct
{
    font_t mmFont;
    meleeMenu_t* menu;
    picrossLevelDef_t levels[PICROSS_LEVEL_COUNT];
    picrossScreen_t screen;
    int32_t savedIndex;
    uint16_t mainMenuPos;
    uint16_t settingsPos;
    int32_t options; // bit 0: hints
} picrossMenu_t;
//==============================================================================
// Function Prototypes
//==============================================================================

void picrossEnterMode();
void picrossExitMode(void);
void picrossMainLoop(int64_t elapsedUs);
void picrossButtonCb(buttonEvt_t* evt);
void picrossTouchCb(touch_event_t* evt);
void loadLevels(void);
void picrossMainMenuCb(const char* opt);
void picrossMenuOptionsCb(const char* opt);

//==============================================================================
// Variables
//==============================================================================

// Key values for persistent save data.
const char picrossCurrentPuzzleIndexKey[] = "pic_cur_ind";
const char picrossSavedOptionsKey[]       = "pic_opts";
const char picrossCompletedLevelData[]    = "pic_victs"; // todo: rename to Key suffix
const char picrossProgressData[]          = "pic_prog";  // todo: rename to key suffix
const char picrossMarksData[]             = "pic_marks";

// Main menu strings
static char str_picrossTitle[]      = "\x7f-cross"; // \x7f is interpreted as the pi char
static const char str_continue[]    = "Continue";
static const char str_levelSelect[] = "Puzzle Select";
static const char str_howtoplay[]   = "How To Play";
static const char str_exit[]        = "Exit";

// Options Menu
static const char str_options[] = "Options";

static const char str_back[]          = "Back";
static const char str_HintsOn[]       = "Mistake Alert: On";
static const char str_HintsOff[]      = "Mistake Alert: Off";
static const char str_GuidesOn[]      = "Guides: On";
static const char str_GuidesOff[]     = "Guides: Off";
static const char str_MarkX[]         = "Empty Marks: X";
static const char str_MarkSolid[]     = "Empty Marks: Solid";
static const char str_AnimateBGOn[]   = "BG Animate: On";
static const char str_AnimateBGOff[]  = "BG Animate: Off";
static const char str_eraseProgress[] = "Reset Current";

swadgeMode_t modePicross = {
    .modeName                = str_picrossTitle,
    .fnEnterMode             = picrossEnterMode,
    .fnExitMode              = picrossExitMode,
    .fnMainLoop              = picrossMainLoop,
    .fnButtonCallback        = picrossButtonCb,
    .fnTouchCallback         = picrossTouchCb,
    .wifiMode                = NO_WIFI,
    .fnEspNowRecvCb          = NULL,
    .fnEspNowSendCb          = NULL,
    .fnAccelerometerCallback = NULL,
    .fnAudioCallback         = NULL,
    .fnTemperatureCallback   = NULL,
    .overrideUsb             = false,
};

picrossMenu_t* pm;

//==============================================================================
// Functions
//==============================================================================
/**
 * @brief Enter the picross mode by displaying the top level menu
 *
 * @param disp The display to draw to
 *
 */
void picrossEnterMode()
{
    // Allocate and zero memory
    pm = calloc(1, sizeof(picrossMenu_t));

    loadFont("mm.font", &(pm->mmFont));

    pm->menu = initMeleeMenu(str_picrossTitle, &(pm->mmFont), picrossMainMenuCb);

    pm->screen = PICROSS_MENU;

    setPicrossMainMenu(true);

    loadLevels();

    // set default options
    if (false == readNvs32(picrossSavedOptionsKey, &pm->options))
    {
        writeNvs32(picrossSavedOptionsKey,
                   14); // 1110 = x's on, bg on, guide on, hintwarning off. On the fence on guides on or off.
    }
}

void picrossExitMode(void)
{
    switch (pm->screen)
    {
        case PICROSS_MENU:
        case PICROSS_OPTIONS:
        {
            // Nothing extra
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossExitLevelSelect();
            break;
        }
        case PICROSS_GAME:
        {
            picrossExitGame();
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossExitTutorial();
            break;
        }
    }

    // Free WSG's
    for (int i = 0; i < PICROSS_LEVEL_COUNT; i++)
    {
        freeWsg(&pm->levels[i].levelWSG);
        freeWsg(&pm->levels[i].completedWSG);
    }
    picrossExitLevelSelect(); // this doesnt actually get called as we go in and out of levelselect (because it breaks
                              // everything), so lets call it now
    deinitMeleeMenu(pm->menu);
    // p2pDeinit(&jm->p2p);
    freeFont(&(pm->mmFont));
    free(pm);
}

/**
 * @brief Loads in all of the level files. Each level is made up of a black/white data file, and a completed color
 * version. The black.white version can be white or transparent for empty spaces, and any other pixel value for filled
 * spaces. Clues and level size is determined by the images, but only 10x10 is currently supported. It will completely
 * break if it's not 10x10 right now. I would like to support any size that is <=15, including non-square puzzles. First
 * is getting them to render and not write out of bounds, second is scaling and placing the puzzle on screen correctly.
 *
 */
void loadLevels()
{
    // LEVEL SETUP AREA
    // DACVAK JUST SEARCH FOR "DACVAK" TO FIND THIS

    // Todo: we can cut our memory use down by about 2/3 if we use a naming convention and the titles to pull the wsg
    // names. snprint into loadWsg, i think. Make sure it all works with, say, apostophes and spaces.

    // pm->levels[0].title = "Test";
    // loadWsg("TEMP.wsg", &pm->levels[0].levelWSG, false);
    // loadWsg("TEMP.wsg", &pm->levels[0].completedWSG, false);

    // any entry with lowercase names is testing data. CamelCase names are good to go. This is not convention, just
    // nature of dac sending me files vs. my testing ones.
    pm->levels[0].title = "Pi";
    loadWsg(PI_PZL_WSG, &pm->levels[0].levelWSG, false); // 5x5
    loadWsg(PI_SLV_WSG, &pm->levels[0].completedWSG, false);

    pm->levels[1].title = "Penguin";
    loadWsg(PENGUIN_PZL_WSG, &pm->levels[1].levelWSG, false); // 5x5
    loadWsg(PENGUIN_SLV_WSG, &pm->levels[1].completedWSG, false);

    pm->levels[2].title = "Twenty Years";
    loadWsg(TWENTY_PZL_WSG, &pm->levels[2].levelWSG, false); // 5x7
    loadWsg(TWENTY_SLV_WSG, &pm->levels[2].completedWSG, false);

    pm->levels[3].title = "A Lie";
    loadWsg(CAKE_PZL_WSG, &pm->levels[3].levelWSG, false); // 5x10
    loadWsg(CAKE_SLV_WSG, &pm->levels[3].completedWSG, false);

    pm->levels[4].title = "XP Bliss";
    loadWsg(BLISS_WSG, &pm->levels[4].levelWSG, false); // 5x10
    loadWsg(BLISS_C_WSG, &pm->levels[4].completedWSG, false);

    pm->levels[5].title = "Discord Notification";
    loadWsg(DISCORD_PZL_WSG, &pm->levels[5].levelWSG, false); // 10x10
    loadWsg(DISCORD_SLV_WSG, &pm->levels[5].completedWSG, false);

    pm->levels[6].title = "Snare Drum";
    loadWsg(SNARE_DRUM_PZL_WSG, &pm->levels[6].levelWSG, false); // 10x10
    loadWsg(SNARE_DRUM_SLV_WSG, &pm->levels[6].completedWSG, false);

    pm->levels[7].title = "Sus";                       //"Sus" or just "Among Us"
    loadWsg(AMONG_PZL_WSG, &pm->levels[7].levelWSG, false); // 5x5
    loadWsg(AMONG_SLV_WSG, &pm->levels[7].completedWSG, false);

    pm->levels[8].title = "Danny";
    loadWsg(DANNY_PZL_WSG, &pm->levels[8].levelWSG, false); // 10x10
    loadWsg(DANNY_SLV_WSG, &pm->levels[8].completedWSG, false);

    pm->levels[9].title = "Controller";
    loadWsg(CONTROLLER_PZL_WSG, &pm->levels[9].levelWSG, false); // 10x10
    loadWsg(CONTROLLER_SLV_WSG, &pm->levels[9].completedWSG, false);

    pm->levels[10].title = "Cat";
    loadWsg(CAT_PZL_WSG, &pm->levels[10].levelWSG, false); // 10x10
    loadWsg(CAT_SLV_WSG, &pm->levels[10].completedWSG, false);

    pm->levels[11].title = "Pear";                     // todo: move this lower, it can be tricky ish.
    loadWsg(PEAR_PZL_WSG, &pm->levels[11].levelWSG, false); // 10x10
    loadWsg(PEAR_SLV_WSG, &pm->levels[11].completedWSG, false);

    pm->levels[12].title = "Cherry";
    loadWsg(CHERRY_PZL_WSG, &pm->levels[12].levelWSG, false); // 10x10
    loadWsg(CHERRY_SLV_WSG, &pm->levels[12].completedWSG, false);

    pm->levels[13].title = "Magnet";
    loadWsg(MAGNET_PZL_WSG, &pm->levels[13].levelWSG, false); // 10x10
    loadWsg(MAGNET_SLV_WSG, &pm->levels[13].completedWSG, false);

    pm->levels[14].title = "Strawberry";
    loadWsg(STRAWBERRY_PZL_WSG, &pm->levels[14].levelWSG, false); // 10x10
    loadWsg(STRAWBERRY_SLV_WSG, &pm->levels[14].completedWSG, false);

    pm->levels[15].title = "Frog";
    loadWsg(FROG_PZL_WSG, &pm->levels[15].levelWSG, false);
    loadWsg(FROG_SLV_WSG, &pm->levels[15].completedWSG, false);

    pm->levels[16].title = "Galaga Bug";
    loadWsg(GALAGA_PZL_WSG, &pm->levels[16].levelWSG, false); // 10x10
    loadWsg(GALAGA_SLV_WSG, &pm->levels[16].completedWSG, false);

    pm->levels[17].title = "Green Shell";
    loadWsg(GREEN_SHELL_PZL_WSG, &pm->levels[17].levelWSG, false); // 10x10
    loadWsg(GREEN_SHELL_SLV_WSG, &pm->levels[17].completedWSG, false);

    pm->levels[18].title = "Zelda";
    loadWsg(LINK_PZL_WSG, &pm->levels[18].levelWSG, false); // 10x10
    loadWsg(LINK_SLV_WSG, &pm->levels[18].completedWSG, false);

    pm->levels[19].title = "Lil' B";
    loadWsg(LIL_B_PZL_WSG, &pm->levels[19].levelWSG, false); // 15x15
    loadWsg(LIL_B_SLV_WSG, &pm->levels[19].completedWSG, false);

    pm->levels[20].title = "Goomba";
    loadWsg(GOOMBA_PZL_WSG, &pm->levels[20].levelWSG, false); // 15x15
    loadWsg(GOOMBA_SLV_WSG, &pm->levels[20].completedWSG, false);

    pm->levels[21].title = "Mouse";
    loadWsg(MOUSE_PZL_WSG, &pm->levels[21].levelWSG, false); // 15x15
    loadWsg(MOUSE_SLV_WSG, &pm->levels[21].completedWSG, false);

    pm->levels[22].title = "Note";
    loadWsg(NOTE_PZL_WSG, &pm->levels[22].levelWSG, false); // 15x15
    loadWsg(NOTE_SLV_WSG, &pm->levels[22].completedWSG, false);

    pm->levels[23].title = "Big Mouth Billy";
    loadWsg(BASS_PZL_WSG, &pm->levels[23].levelWSG, false); // 15x15
    loadWsg(BASS_SLV_WSG, &pm->levels[23].completedWSG, false);

    pm->levels[24].title = "Fountain Pen";
    loadWsg(FOUNTAIN_PEN_PZL_WSG, &pm->levels[24].levelWSG, false); // 15x15
    loadWsg(FOUNTAIN_PEN_SLV_WSG, &pm->levels[24].completedWSG, false);

    pm->levels[25].title = "Power Plug";
    loadWsg(PLUG_PZL_WSG, &pm->levels[25].levelWSG, false); // 15x15 - This one is on the harder side of things.
    loadWsg(PLUG_SLV_WSG, &pm->levels[25].completedWSG, false);

    pm->levels[26].title = "Blender";
    loadWsg(BLENDER_PZL_WSG, &pm->levels[26].levelWSG, false); // 15x15
    loadWsg(BLENDER_SLV_WSG, &pm->levels[26].completedWSG, false);

    pm->levels[27].title = "Nintendo 64";
    loadWsg(_N64_PZL_WSG, &pm->levels[27].levelWSG, false); // 15x15
    loadWsg(_N64_PZL_WSG, &pm->levels[27].completedWSG, false);

    pm->levels[28].title = "Rocket League";
    loadWsg(ROCKET_LEAGUE_PZL_WSG, &pm->levels[28].levelWSG, false); // 15x15 - This one is on the harder side of things.
    loadWsg(ROCKET_LEAGUE_SLV_WSG, &pm->levels[28].completedWSG, false);

    // this has to be the last puzzle.
    pm->levels[29].title = "Never Gonna";            // give you up, but title too long for single line.
    loadWsg(RR_PZL_WSG, &pm->levels[29].levelWSG, false); // 15/15
    loadWsg(RR_SLV_WSG, &pm->levels[29].completedWSG, false);

    // dont forget to update PICROSS_LEVEL_COUNT (in #define in picross_consts.h) when adding levels.

    // set indices. Used to correctly set save data. levels are loaded without context of the levels array, so they
    // carry the index info with them so we can save victories.
    for (int i = 0; i < PICROSS_LEVEL_COUNT; i++) // 8 should = number of levels and that should = levelCount.
    {
        pm->levels[i].index = i;
    }
}

/**
 * Call the appropriate main loop function for the screen being displayed
 *
 * @param elapsedUd Time.deltaTime
 */
void picrossMainLoop(int64_t elapsedUs)
{
    switch (pm->screen)
    {
        case PICROSS_OPTIONS:
        case PICROSS_MENU:
        {
            drawMeleeMenu(pm->menu);
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossLevelSelectLoop(elapsedUs);
            break;
        }
        case PICROSS_GAME:
        {
            picrossGameLoop(elapsedUs);
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossTutorialLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief Call the appropriate button functions for the screen being displayed
 *
 * @param evt
 */
void picrossButtonCb(buttonEvt_t* evt)
{
    switch (pm->screen)
    {
        case PICROSS_OPTIONS:
        case PICROSS_MENU:
        {
            // Pass button events from the Swadge mode to the menu
            if (evt->down)
            {
                if (pm->screen == PICROSS_OPTIONS && evt->button == PB_B)
                {
                    // Simulate selecting "Back" on B press
                    picrossMainMenuCb(str_back);
                }
                else
                {
                    switch (evt->button)
                    {
                        case PB_UP:
                        case PB_DOWN:
                        case PB_A:
                        case PB_B:
                        case PB_START:
                        case PB_SELECT:
                        {
                            meleeMenuButton(pm->menu, evt->button);
                            break;
                        }
                        case PB_LEFT:
                        case PB_RIGHT:
                        {
                            if ((str_HintsOn == pm->menu->rows[pm->menu->selectedRow])
                                || (str_HintsOff == pm->menu->rows[pm->menu->selectedRow])
                                || (str_GuidesOn == pm->menu->rows[pm->menu->selectedRow])
                                || (str_GuidesOff == pm->menu->rows[pm->menu->selectedRow])
                                || (str_MarkX == pm->menu->rows[pm->menu->selectedRow])
                                || (str_MarkSolid == pm->menu->rows[pm->menu->selectedRow])
                                || (str_AnimateBGOn == pm->menu->rows[pm->menu->selectedRow])
                                || (str_AnimateBGOff == pm->menu->rows[pm->menu->selectedRow]))
                            {
                                picrossMainMenuCb(pm->menu->rows[pm->menu->selectedRow]);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossLevelSelectButtonCb(evt);
            break;
        }
        case PICROSS_GAME:
        {
            picrossGameButtonCb(evt);
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossTutorialButtonCb(evt);
            break;
        }
            // No wifi mode stuff
    }
}

void picrossTouchCb(touch_event_t* evt)
{
    if (pm->screen == PICROSS_GAME)
    {
        picrossGameTouchCb(evt);
    }
}

/////////////////////////

/**
 * @brief Sets up the top level menu for Picross
 */
void setPicrossMainMenu(bool resetPos)
{
    // override the main menu with the options menu instead.

    if (pm->screen == PICROSS_OPTIONS)
    {
        // Draw the options menu
        resetMeleeMenu(pm->menu, str_options, picrossMainMenuCb);

        if (picrossGetSaveFlag(
                1)) // are guides on? (getHints loaded from perma, we can skip doing that again and just read local)
        {
            addRowToMeleeMenu(pm->menu, str_GuidesOn);
        }
        else
        {
            addRowToMeleeMenu(pm->menu, str_GuidesOff);
        }

        if (picrossGetLoadedSaveFlag(2)) // Bg animate on?
        {
            addRowToMeleeMenu(pm->menu, str_AnimateBGOn);
        }
        else
        {
            addRowToMeleeMenu(pm->menu, str_AnimateBGOff);
        }

        if (picrossGetLoadedSaveFlag(3)) // Choose X
        {
            addRowToMeleeMenu(pm->menu, str_MarkX);
        }
        else
        {
            addRowToMeleeMenu(pm->menu, str_MarkSolid);
        }

        if (picrossGetLoadedSaveFlag(0)) // are hints on?
        {
            addRowToMeleeMenu(pm->menu, str_HintsOn);
        }
        else
        {
            addRowToMeleeMenu(pm->menu, str_HintsOff);
        }

        addRowToMeleeMenu(pm->menu, str_eraseProgress);
        addRowToMeleeMenu(pm->menu, str_back);

        // Set the position
        if (resetPos)
        {
            pm->settingsPos = 0;
        }
        pm->menu->selectedRow = pm->settingsPos;
    }
    else
    {
        // otherwise, if we are in ANY game state, this should default (and set) to picross_menu
        resetMeleeMenu(pm->menu, str_picrossTitle, picrossMainMenuCb);

        // only display continue button if we have a game in progress.

        // attempt to read the saved index. If it doesnt exist, set it to -1.
        if ((false == readNvs32(picrossCurrentPuzzleIndexKey, &pm->savedIndex)))
        {
            writeNvs32(picrossCurrentPuzzleIndexKey,
                       -1); // if we enter this screen and instantly exist, also set it to -1 and we can just load that.
            pm->savedIndex = -1; // there is no current index.
        }

        // only show continue button if we have saved data.
        if (pm->savedIndex >= 0)
        {
            addRowToMeleeMenu(pm->menu, str_continue);
        }

        addRowToMeleeMenu(pm->menu, str_levelSelect);
        addRowToMeleeMenu(pm->menu, str_howtoplay);
        addRowToMeleeMenu(pm->menu, str_options);
        addRowToMeleeMenu(pm->menu, str_exit);
        pm->screen = PICROSS_MENU;

        // Set the position
        if (resetPos)
        {
            pm->mainMenuPos = 0;
        }
        pm->menu->selectedRow = pm->mainMenuPos;
    }
}

/**
 * @brief Frees level select menu and returns to the picross menu. Should not be called by not-the-level-select-menu.
 *
 */
void returnToPicrossMenu(void)
{
    picrossExitLevelSelect(); // free data
    setPicrossMainMenu(false);
}
/**
 * @brief Frees level select menu and returns to the picross menu, except skipping past the level select menu.
 *
 */
void returnToPicrossMenuFromGame(void)
{
    setPicrossMainMenu(false);
}

/**
 * @brief Exits the tutorial mode.
 *
 */
void exitTutorial(void)
{
    pm->screen = PICROSS_MENU;
    setPicrossMainMenu(false);
}

void continueGame()
{
    // get the current level index
    int32_t currentIndex = 0; // just load 0 if its 0.
    readNvs32(picrossCurrentPuzzleIndexKey, &currentIndex);

    // load in the level we selected.
    // uh. read the currentLevelIndex and get the value from
    picrossStartGame(&pm->mmFont, &pm->levels[currentIndex], true);
    pm->screen = PICROSS_GAME;
}

// menu button & options menu callbacks. Set the screen and call the appropriate start functions
void picrossMainMenuCb(const char* opt)
{
    if (str_picrossTitle == pm->menu->title)
    {
        pm->mainMenuPos = pm->menu->selectedRow;
    }
    else if (str_options == pm->menu->title)
    {
        pm->settingsPos = pm->menu->selectedRow;
    }

    if (opt == str_continue)
    {
        continueGame();

        // Turn off LEDs
        led_t leds[CONFIG_NUM_LEDS] = {0};
        setLeds(leds, CONFIG_NUM_LEDS);

        return;
    }
    if (opt == str_howtoplay)
    {
        pm->screen = PICROSS_TUTORIAL;
        picrossStartTutorial(&pm->mmFont);

        // Turn off LEDs
        led_t leds[CONFIG_NUM_LEDS] = {0};
        setLeds(leds, CONFIG_NUM_LEDS);

        return;
    }
    if (opt == str_levelSelect)
    {
        pm->screen = PICROSS_LEVELSELECT;
        picrossStartLevelSelect(&pm->mmFont, pm->levels);

        // Turn off LEDs
        led_t leds[CONFIG_NUM_LEDS] = {0};
        setLeds(leds, CONFIG_NUM_LEDS);

        return;
    }
    if (opt == str_exit)
    {
        // Exit to main menu
        switchToSwadgeMode(&modeMainMenu);
        return;
    }
    else if (opt == str_back)
    {
        pm->screen = PICROSS_MENU;
        setPicrossMainMenu(false);
    }
    else if (opt == str_options)
    {
        pm->screen = PICROSS_OPTIONS;
        setPicrossMainMenu(true);
    }
    else if (opt == str_HintsOff)
    {
        // turn hints back on
        picrossSetSaveFlag(0, true);
        setPicrossMainMenu(false); // re-setup menu with new text, but dont change cursor position
    }
    else if (opt == str_HintsOn)
    {
        // turn hints off
        picrossSetSaveFlag(0, false);
        setPicrossMainMenu(false);
    }
    else if (opt == str_GuidesOff)
    {
        // turn guides back on
        picrossSetSaveFlag(1, true);
        setPicrossMainMenu(false); // re-setup menu with new text, but dont change cursor position
    }
    else if (opt == str_GuidesOn)
    {
        // turn guides off
        picrossSetSaveFlag(1, false);
        setPicrossMainMenu(false);
    }
    else if (opt == str_AnimateBGOff)
    {
        // turn bgAnimate back on
        picrossSetSaveFlag(2, true);
        setPicrossMainMenu(false); // re-setup menu with new text, but dont change cursor position
    }
    else if (opt == str_AnimateBGOn)
    {
        // turn bgAnimate off
        picrossSetSaveFlag(2, false);
        setPicrossMainMenu(false);
    }
    else if (opt == str_MarkX)
    {
        // turn choose X back off
        picrossSetSaveFlag(3, false);
        setPicrossMainMenu(false); // re-setup menu with new text, but dont change cursor position
    }
    else if (opt == str_MarkSolid)
    {
        // turn chooseX on
        picrossSetSaveFlag(3, true);
        setPicrossMainMenu(false);
    }
    else if (opt == str_eraseProgress)
    {
        // todo: still need to do a confirmation on these, probably just by changing the text.
        // Commented out
        //  //Next time we load a puzzle it will re-save and zero-out the data, we just have to tell the mennu not to
        //  show the 'continue' option.
        writeNvs32(picrossCurrentPuzzleIndexKey, -1);

        // see comment above as to why this isn't needed.
        //  size_t size = sizeof(picrossProgressData_t);
        //  picrossProgressData_t* progData = calloc(1,size);//zero out = reset.
        //  writeNvsBlob(picrossProgressData,progData,size);
        //  free(progData);

        // the code to erase ALL (victory) progress. Still want to put this... somewhere
        //  size_t size = sizeof(picrossVictoryData_t);
        //  picrossVictoryData_t* victData = calloc(1,size);//zero out = reset.
        //  writeNvsBlob(picrossCompletedLevelData,victData,size);
        //  free(victData);
    }
}

/**
 * @brief Selects a picross level then starts the game. Called by level select, and sends us into mode_picross.c Doesn't
 * free memory.
 *
 * @param selectedLevel Pointer to the a level struct.
 */
void selectPicrossLevel(picrossLevelDef_t* selectedLevel)
{
    // picrossExitLevelSelect();//we do this BEFORE we enter startGame.
    pm->screen = PICROSS_GAME;
    picrossStartGame(&pm->mmFont, selectedLevel, false);
}

// void returnToLevelSelect()//todo: rename
// {
//     //todo: abstract this to function
//     pm->screen = PICROSS_LEVELSELECT;
//     //todo: fix below warning
//     picrossStartLevelSelect(&pm->mmFont,pm->levels);
// }

/**
 * @brief Save data is stored in a single integer, using the register as 32 flags.
 * Hints on/off is pos 0.
 * Guide on/off is pos 1
 * BG Animation is pos 2
 *
 * @param pos Pos (<32) which flag to get.
 * @return true
 * @return false
 */
bool picrossGetSaveFlag(int pos)
{
    if (false == readNvs32(picrossSavedOptionsKey, &pm->options))
    {
        writeNvs32(picrossSavedOptionsKey, 0);
    }

    return picrossGetLoadedSaveFlag(pos);
}

/**
 * @brief once you have called picrossGetSaveFlag for any position, you don't need to call it again until an option
 * changes. As we often read a bunch of values in a row, this function can be used after the first one to save on
 * uneccesary read calls.
 *
 * @param pos
 * @return true
 * @return false
 */
bool picrossGetLoadedSaveFlag(int pos)
{
    int val = (pm->options & (1 << pos)) >> pos;
    return val == 1;
}

// also sets pm->options
/**
 * @brief Sets a flag. Sets pm->options, which is actually used by the game without necessarily having to (repeatedly)
 * call picrossGetSaveFlag. Of course, it also also saves it to perma.
 *
 * @param pos index to get. 0<pos<32.
 * @param on save vale.
 */
void picrossSetSaveFlag(int pos, bool on)
{
    if (on)
    {
        pm->options = pm->options | (1 << (pos));
    }
    else
    {
        pm->options = pm->options & ~(1 << (pos));
    }
    writeNvs32(picrossSavedOptionsKey, pm->options);
}
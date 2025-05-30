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

#include "mainMenu.h"

//==============================================================================
// Enums & Structs
//==============================================================================

typedef enum
{
    PICROSS_MENU,
    PICROSS_LEVELSELECT,
    PICROSS_GAME,
    PICROSS_TUTORIAL
} picrossScreen_t;

typedef struct
{
    font_t mmFont;
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    picrossLevelDef_t levels[PICROSS_LEVEL_COUNT];
    picrossScreen_t screen;
    int32_t savedIndex;
    int32_t options; // bit 0: hints
} picrossMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void picrossEnterMode(void);
void picrossExitMode(void);
void picrossMainLoop(int64_t elapsedUs);
void picrossButtonCb(buttonEvt_t* evt);
void picrossTouchCb(bool touched);
void loadLevels(void);
void picrossMainMenuCb(const char* label, bool selected, uint32_t value);
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
static char str_picrossTitle[]      = "Pi-cross"; // \x7f is interpreted as the pi char
static const char str_continue[]    = "Continue";
static const char str_levelSelect[] = "Puzzle Select";
static const char str_howtoplay[]   = "How To Play";
static const char str_exit[]        = "Exit";

// Options Menu
static const char str_options[] = "Options";

static const char str_On[]       = "On";
static const char str_Off[]      = "Off";
static const char* strs_on_off[] = {str_Off, str_On};

static const char str_X[]         = "X";
static const char str_Solid[]     = "Solid";
static const char* strs_x_solid[] = {str_X, str_Solid};

static const char str_Hints[]     = "Mistake Alert: ";
static const char str_Guides[]    = "Guides: ";
static const char str_Mark[]      = "Empty Marks: ";
static const char str_AnimateBG[] = "BG Animate: ";

static const int32_t trueFalseVals[] = {
    false,
    true,
};

static const char str_eraseProgress[] = "Reset Current";

swadgeMode_t modePicross = {
    .modeName                 = str_picrossTitle,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = picrossEnterMode,
    .fnExitMode               = picrossExitMode,
    .fnMainLoop               = picrossMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = NULL,
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
void picrossEnterMode(void)
{
    // Allocate and zero memory
    pm = calloc(1, sizeof(picrossMenu_t));

    loadFont(MM_FONT, &(pm->mmFont), false);

    pm->menu     = initMenu(str_picrossTitle, picrossMainMenuCb);
    pm->renderer = initMenuManiaRenderer(NULL, NULL, NULL);

    pm->screen = PICROSS_MENU;

    addSingleItemToMenu(pm->menu, str_levelSelect);
    addSingleItemToMenu(pm->menu, str_howtoplay);

    pm->menu = startSubMenu(pm->menu, str_options);

    settingParam_t sp_tf = {
        .min = trueFalseVals[0],
        .max = trueFalseVals[ARRAY_SIZE(trueFalseVals) - 1],
        .def = trueFalseVals[0],
    };
    addSettingsOptionsItemToMenu(pm->menu, str_Guides, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 picrossGetSaveFlag(PO_SHOW_GUIDES));
    addSettingsOptionsItemToMenu(pm->menu, str_AnimateBG, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 picrossGetLoadedSaveFlag(PO_ANIMATE_BG));
    addSettingsOptionsItemToMenu(pm->menu, str_Mark, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_x_solid), &sp_tf,
                                 picrossGetLoadedSaveFlag(PO_MARK_X));
    addSettingsOptionsItemToMenu(pm->menu, str_Hints, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 picrossGetLoadedSaveFlag(PO_SHOW_HINTS));
    pm->menu = endSubMenu(pm->menu);

    addSingleItemToMenu(pm->menu, str_exit);

    loadLevels();
}

void picrossExitMode(void)
{
    switch (pm->screen)
    {
        case PICROSS_MENU:
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
    deinitMenu(pm->menu);
    deinitMenuManiaRenderer(pm->renderer);
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

    pm->levels[7].title = "Sus";                            //"Sus" or just "Among Us"
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

    pm->levels[11].title = "Pear";                          // todo: move this lower, it can be tricky ish.
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
    loadWsg(ROCKET_LEAGUE_PZL_WSG, &pm->levels[28].levelWSG,
            false); // 15x15 - This one is on the harder side of things.
    loadWsg(ROCKET_LEAGUE_SLV_WSG, &pm->levels[28].completedWSG, false);

    // this has to be the last puzzle.
    pm->levels[29].title = "Never Gonna";                 // give you up, but title too long for single line.
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
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        picrossButtonCb(&evt);
    }

    // Check for touchpad touches
    int32_t phi       = 0;
    int32_t r         = 0;
    int32_t intensity = 0;
    picrossTouchCb(getTouchJoystick(&phi, &r, &intensity));

    switch (pm->screen)
    {
        case PICROSS_MENU:
        {
            drawMenuMania(pm->menu, pm->renderer, elapsedUs);
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
        case PICROSS_MENU:
        {
            pm->menu = menuButton(pm->menu, *evt);
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
    }
}

void picrossTouchCb(bool touched)
{
    if (pm->screen == PICROSS_GAME)
    {
        picrossGameTouchCb(touched);
    }
}

/////////////////////////

/**
 * @brief Frees level select menu and returns to the picross menu. Should not be called by not-the-level-select-menu.
 *
 */
void returnToPicrossMenu(void)
{
    picrossExitLevelSelect(); // free data
    pm->screen = PICROSS_MENU;
}
/**
 * @brief Frees level select menu and returns to the picross menu, except skipping past the level select menu.
 *
 */
void returnToPicrossMenuFromGame(void)
{
    pm->screen = PICROSS_MENU;
}

/**
 * @brief Exits the tutorial mode.
 *
 */
void exitTutorial(void)
{
    pm->screen = PICROSS_MENU;
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
void picrossMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == str_continue)
        {
            continueGame();

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);

            return;
        }
        if (label == str_howtoplay)
        {
            pm->screen = PICROSS_TUTORIAL;
            picrossStartTutorial(&pm->mmFont);

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);

            return;
        }
        if (label == str_levelSelect)
        {
            pm->screen = PICROSS_LEVELSELECT;
            picrossStartLevelSelect(&pm->mmFont, pm->levels);

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);

            return;
        }
        if (label == str_exit)
        {
            // Exit to main menu
            switchToSwadgeMode(&mainMenuMode);
            return;
        }
        else if (label == str_eraseProgress)
        {
            // todo: still need to do a confirmation on these, probably just by changing the text.
            // Commented out
            //  //Next time we load a puzzle it will re-save and zero-out the data, we just have to tell the mennu not
            //  to show the 'continue' option.
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
    else
    {
        // Not selected
        if (str_Guides == label)
        {
            picrossSetSaveFlag(PO_SHOW_GUIDES, value);
        }
        else if (str_AnimateBG == label)
        {
            picrossSetSaveFlag(PO_ANIMATE_BG, value);
        }
        else if (str_Hints == label)
        {
            picrossSetSaveFlag(PO_SHOW_HINTS, value);
        }
        else if (str_Mark == label)
        {
            picrossSetSaveFlag(PO_MARK_X, value);
        }
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
bool picrossGetSaveFlag(picrossOption_t pos)
{
    if (false == readNvs32(picrossSavedOptionsKey, &pm->options))
    {
        // set default options
        // x's on, bg on, guide on, hintwarning off. On the fence on guides on or off.
        int32_t defaults = (1 << PO_ANIMATE_BG) | (1 << PO_SHOW_GUIDES) | (1 << PO_MARK_X);
        writeNvs32(picrossSavedOptionsKey, defaults);
        pm->options = defaults;
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
bool picrossGetLoadedSaveFlag(picrossOption_t pos)
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
void picrossSetSaveFlag(picrossOption_t pos, bool on)
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
//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_menu.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define HS_STR_LEN 32

//==============================================================================
// Function Declarations
//==============================================================================

static void shMenuCb(const char*, bool selected, uint32_t settingVal);

//==============================================================================
// Const Variables
//==============================================================================

static const shSong_t shSongList[] = {
    {
        .name = "GShip Credits",
        .midi = "credits.mid",
        .easy = "credits_e.cch",
        .med  = "credits_m.cch",
        .hard = "credits_h.cch",
    },
};

static const char strSongSelect[] = "Song Select";
static const char strEasy[]       = "Easy";
static const char strMedium[]     = "Medium";
static const char strHard[]       = "Hard";
static const char strHighScores[] = "High Scores";
static const char strSettings[]   = "Settings";
static const char strExit[]       = "Exit";

const char* shs_fail_key      = "shs_fail";
const char* shs_fail_label    = "Song Fail: ";
const char* shs_fail_opts[]   = {"On", "Off"};
const int32_t shs_fail_vals[] = {true, false};

const char* shs_speed_key      = "shs_speed";
const char* shs_speed_label    = "Scroll: ";
const char* shs_speed_opts[]   = {"Slow", "Normal", "Fast", "Turbo"};
const int32_t shs_speed_vals[] = {4000000, 3000000, 2000000, 1000000};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param sh
 */
void shSetupMenu(shVars_t* sh)
{
    // Allocate the menu
    sh->menu     = initMenu(swadgeHeroMode.modeName, shMenuCb);
    sh->renderer = initMenuManiaRenderer(&sh->righteous, NULL, &sh->rodin);
    setManiaLedsOn(sh->renderer, true);

    // Add songs to play
    sh->menu = startSubMenu(sh->menu, strSongSelect);
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(shSongList); sIdx++)
    {
        // Add difficulties for each song
        sh->menu = startSubMenu(sh->menu, shSongList[sIdx].name);
        addSingleItemToMenu(sh->menu, strEasy);
        addSingleItemToMenu(sh->menu, strMedium);
        addSingleItemToMenu(sh->menu, strHard);
        sh->menu = endSubMenu(sh->menu);
    }
    sh->menu = endSubMenu(sh->menu);

    // Add high score entries per-song
    sh->menu = startSubMenu(sh->menu, strHighScores);
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(shSongList); sIdx++)
    {
        sh->menu = startSubMenu(sh->menu, shSongList[sIdx].name);

        // Allocate and print high score strings
        // Save them all to a linked list to free later

        // TODO get high scores from NVS
        int32_t maxScore = 0x7FFFFFFF;

        char* easyStr = heap_caps_calloc(1, sizeof(char) * HS_STR_LEN, MALLOC_CAP_SPIRAM);
        snprintf(easyStr, HS_STR_LEN - 1, "Easy: %" PRId32, maxScore);
        addSingleItemToMenu(sh->menu, easyStr);
        push(&sh->hsStrs, easyStr);

        char* mediumStr = heap_caps_calloc(1, sizeof(char) * HS_STR_LEN, MALLOC_CAP_SPIRAM);
        snprintf(mediumStr, HS_STR_LEN - 1, "Medium: %" PRId32, maxScore);
        addSingleItemToMenu(sh->menu, mediumStr);
        push(&sh->hsStrs, mediumStr);

        char* hardStr = heap_caps_calloc(1, sizeof(char) * HS_STR_LEN, MALLOC_CAP_SPIRAM);
        snprintf(hardStr, HS_STR_LEN - 1, "Hard: %" PRId32, maxScore);
        addSingleItemToMenu(sh->menu, hardStr);
        push(&sh->hsStrs, hardStr);

        sh->menu = endSubMenu(sh->menu);
    }
    sh->menu = endSubMenu(sh->menu);

    // Add settings
    sh->menu = startSubMenu(sh->menu, strSettings);

    // Setting for song fail
    settingParam_t failBounds = {
        .min = shs_fail_vals[0],
        .max = shs_fail_vals[ARRAY_SIZE(shs_fail_vals) - 1],
    };
    addSettingsOptionsItemToMenu(sh->menu, shs_fail_label, shs_fail_opts, shs_fail_vals, ARRAY_SIZE(shs_fail_vals),
                                 &failBounds, shGetSettingFail());

    // Setting for note speed
    settingParam_t speedBounds = {
        .min = shs_speed_vals[0],
        .max = shs_speed_vals[ARRAY_SIZE(shs_speed_vals) - 1],
    };
    addSettingsOptionsItemToMenu(sh->menu, shs_speed_label, shs_speed_opts, shs_speed_vals, ARRAY_SIZE(shs_speed_vals),
                                 &speedBounds, shGetSettingSpeed());

    // End settings
    sh->menu = endSubMenu(sh->menu);

    // Add exit
    addSingleItemToMenu(sh->menu, strExit);
}

/**
 * @brief TODO
 *
 * @param sh
 */
void shTeardownMenu(shVars_t* sh)
{
    void* toFree;
    while ((toFree = pop(&sh->hsStrs)))
    {
        free(toFree);
    }
    setManiaLedsOn(sh->renderer, false);
    deinitMenuManiaRenderer(sh->renderer);
    deinitMenu(sh->menu);
}

/**
 * @brief TODO
 *
 * @param sh
 * @param btn
 */
void shMenuInput(shVars_t* sh, buttonEvt_t* btn)
{
    sh->menu = menuButton(sh->menu, *btn);
}

/**
 * @brief TODO
 *
 * @param sh
 */
void shMenuDraw(shVars_t* sh, int32_t elapsedUs)
{
    drawMenuMania(sh->menu, sh->renderer, elapsedUs);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void shMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    // Get a reference to the game state because it isn't a callback arg
    shVars_t* sh = getShVars();

    if (selected)
    {
        if (strSongSelect == label)
        {
            // Note we're on the song select submenu
            sh->submenu = strSongSelect;
        }
        else if (strHighScores == label)
        {
            // Note we're on the high score submenu
            sh->submenu = strHighScores;
        }
        else if (strSettings == label)
        {
            // Note we're on the settings submenu
            sh->submenu = strSettings;
        }
        else if (strExit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (strEasy == label)
        {
            printf("Play %s (Easy)\n", sh->menuSong->midi);
            sh->difficulty = SH_EASY;
            shChangeScreen(sh, SH_GAME);
        }
        else if (strMedium == label)
        {
            printf("Play %s (Medium)\n", sh->menuSong->midi);
            sh->difficulty = SH_MEDIUM;
            shChangeScreen(sh, SH_GAME);
        }
        else if (strHard == label)
        {
            printf("Play %s (Hard)\n", sh->menuSong->midi);
            sh->difficulty = SH_HARD;
            shChangeScreen(sh, SH_GAME);
        }
        else if (sh->submenu == strSongSelect || sh->submenu == strHighScores)
        {
            // These submenus have lists of songs. Find the song by the label
            for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(shSongList); sIdx++)
            {
                // If the name matches
                if (label == shSongList[sIdx].name)
                {
                    // Save the song
                    sh->menuSong = &shSongList[sIdx];

                    if (sh->submenu == strHighScores)
                    {
                        // Show high scores
                        printf("HS %s\n", shSongList[sIdx].midi);
                    }
                    // If playing a song, difficulty select is next

                    // Found the song, so break
                    break;
                }
            }
        }
    }
    else
    {
        if (label == shs_fail_label)
        {
            writeNvs32(shs_fail_key, settingVal);
        }
        if (label == shs_speed_label)
        {
            writeNvs32(shs_speed_key, settingVal);
        }
    }
}

/**
 * @brief TODO
 *
 * TODO use this somewhere
 *
 * @return true
 * @return false
 */
bool shGetSettingFail(void)
{
    int32_t failSetting;
    if (readNvs32(shs_fail_key, &failSetting))
    {
        return failSetting;
    }
    return true;
}

/**
 * @brief TODO
 *
 * TODO use this somewhere
 *
 * @return true
 * @return false
 */
int32_t shGetSettingSpeed(void)
{
    int32_t speedSetting;
    if (readNvs32(shs_speed_key, &speedSetting))
    {
        return speedSetting;
    }
    return 0;
}
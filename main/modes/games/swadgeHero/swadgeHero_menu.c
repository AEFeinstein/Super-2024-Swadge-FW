//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_menu.h"
#include "swadgeHero_game.h"
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

static const char easyStr_noscore[]   = "Easy:";
static const char mediumStr_noscore[] = "Medium:";
static const char hardStr_noscore[]   = "Hard:";

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
 * @brief TODO doc
 *
 * @param sh
 */
void shSetupMenu(shVars_t* sh)
{
    // Allocate the menu
    sh->menu     = initMenu(swadgeHeroMode.modeName, shMenuCb);
    sh->renderer = initMenuManiaRenderer(&sh->righteous, NULL, &sh->rodin);

    static const paletteColor_t shadowColors[] = {c110, c210, c220, c320, c330, c430, c330, c320, c220, c210};
    led_t ledColor                             = {.r = 0x80, .g = 0x00, .b = 0x00};
    recolorMenuManiaRenderer(sh->renderer,     //
                             c431, c100, c100, // Title colors (bg, text, outline)
                             c111,             // Background
                             c200, c210,       // Rings
                             c000, c444,       // Rows
                             shadowColors, ARRAY_SIZE(shadowColors), ledColor);
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

        // Helpers for building the high score menu
        const shDifficulty_t difficulties[] = {SH_EASY, SH_MEDIUM, SH_HARD};
        const char* labels[]                = {easyStr_noscore, mediumStr_noscore, hardStr_noscore};

        for (int32_t i = 0; i < ARRAY_SIZE(difficulties); i++)
        {
            char nvsKey[16];
            shGetNvsKey(shSongList[sIdx].midi, difficulties[i], nvsKey);

            int32_t tmpScore;
            if (readNvs32(nvsKey, &tmpScore))
            {
                int32_t gradeIdx = (tmpScore >> 28) & 0x0F;
                tmpScore &= 0x0FFFFFFF;

                // Allocate and print high score strings
                char* hsStr = heap_caps_calloc(1, sizeof(char) * HS_STR_LEN, MALLOC_CAP_SPIRAM);
                snprintf(hsStr, HS_STR_LEN - 1, "%s %" PRId32 " %s", labels[i], tmpScore, getLetterGrade(gradeIdx));
                addSingleItemToMenu(sh->menu, hsStr);

                // Save them all to a linked list to free later
                push(&sh->hsStrs, hsStr);
            }
            else
            {
                addSingleItemToMenu(sh->menu, labels[i]);
            }
        }

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
 * @brief TODO doc
 *
 * @param sh
 */
void shTeardownMenu(shVars_t* sh)
{
    void* toFree;
    while ((toFree = pop(&sh->hsStrs)))
    {
        heap_caps_free(toFree);
    }
    setManiaLedsOn(sh->renderer, false);
    deinitMenuManiaRenderer(sh->renderer);
    deinitMenu(sh->menu);
}

/**
 * @brief TODO doc
 *
 * @param sh
 * @param btn
 */
void shMenuInput(shVars_t* sh, buttonEvt_t* btn)
{
    sh->menu = menuButton(sh->menu, *btn);
}

/**
 * @brief TODO doc
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
 * @brief TODO doc
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
 * @brief TODO doc
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
    return shs_speed_vals[1];
}
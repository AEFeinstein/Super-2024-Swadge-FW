/**
 * @file artillery_help.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief Display help pages for Vector Tanks
 * @date 2025-11-26
 */

//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_help.h"
#include "artillery_phys.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const helpPage_t helpPages1[] = {
    {
        .title = artilleryModeName,
        .text  = "Welcome to Vector Tanks! You'll take turns driving your tank, adjusting your shot, and firing "
                 "to score the most points!",
    },
    {
        .title = artilleryModeName,
        .text  = "You get points for landing shots on your opponent. Watch out, friendly fire is turned on too!",
    },
    {
        .title = artilleryModeName,
        .text  = "Each match is seven rounds long and whoever has the most points at the end wins all the bragging "
                 "rights, until the next match.",
    },
    {
        .title = str_wirelessConnect,
        .text  = "In Wireless PvP, play a match with two Swadges and a friend. Hold Swadges close to pair them. No "
                 "screen peeking!",
    },
    {
        .title = str_passAndPlay,
        .text = "In Pass and Play, play a match with one Swadge and a friend by passing the Swadge around. Remember to "
                "wash hands!",
    },
    {
        .title = str_cpuPractice,
        .text  = "In CPU Practice, play a practice match against a CPU opponent. It's not your friend. Show no mercy.",
    },
    {
        .title = str_paintSelect,
        .text  = "Select your tank's colors in the Paint Shop. Which one is the most you?",
    },
    {
        .title = artilleryModeName,
        .text  = "When you're playing a match you can take a few actions each turn.",
    },
    {
        .title = str_load_ammo,
        .text  = "Start a turn by loading ammo. Each ammo has special properties and can only be used once each match. "
                 "Choose wisely.",
    },
    {
        .title = str_look_around,
        .text  = "Then you can look around the field to see terrain and where your opponent is before driving or "
                 "adjusting your shot.",
    },
    {
        .title = str_drive,
        .text  = "Each turn you have a little fuel to drive around. Tanks are not very efficient movers. Take cover or "
                 "get a clearer shot!",
    },
    {
        .title = str_adjust,
        .text  = "Adjust the angle and power of the shot before firing. Spinning the touchpad adjusts the angle too.",
    },
    {
        .title = str_fire,
        .text  = "Once you're in position with loaded ammo and an adjusted shot, fire away!",
    },
    {
        .title = str_load_ammo,
        .text  = "Let's go over the ammo's special properties.",
    },
};

// Ammo pages inserted here during initialization

static const helpPage_t helpPages2[] = {
    {
        .title = artilleryModeName,
        .text  = "That's all you need to know. Get out there and be the best tank captain ever!",
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the help pages. This combines the help pages written here and ammo notes
 *
 * @param ad All the artillery mode data
 */
void artilleryHelpInit(artilleryData_t* ad)
{
    // Get ammo attributes
    uint16_t numAttributes          = 0;
    const artilleryAmmoAttrib_t* aa = getAmmoAttributes(&numAttributes);

    // Figure out the total number of help pages
    int32_t totalPages = ARRAY_SIZE(helpPages1) + numAttributes + ARRAY_SIZE(helpPages2);

    // Allocate space for help pages
    ad->helpPages = heap_caps_calloc(totalPages, sizeof(helpPage_t), MALLOC_CAP_SPIRAM);
    if (ad->helpPages)
    {
        // Copy first set of help pages
        memcpy(ad->helpPages, helpPages1, sizeof(helpPages1));

        // Build help pages for ammo
        for (uint16_t aIdx = 0; aIdx < numAttributes; aIdx++)
        {
            ad->helpPages[ARRAY_SIZE(helpPages1) + aIdx].title = aa[aIdx].name;
            ad->helpPages[ARRAY_SIZE(helpPages1) + aIdx].text  = aa[aIdx].help;
        }

        // Copy second set of help pages
        memcpy(&ad->helpPages[ARRAY_SIZE(helpPages1) + numAttributes], helpPages2, sizeof(helpPages2));

        // Initialize help
        ad->help = initHelpScreen(ad->blankMenu, ad->mRenderer, ad->helpPages, totalPages);
    }
    else
    {
        // Return to the menu
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief Free data for the help pages
 *
 * @param ad All the artillery mode data
 */
void artilleryHelpDeinit(artilleryData_t* ad)
{
    if (ad->help)
    {
        deinitHelpScreen(ad->help);
        ad->help = NULL;
    }

    if (ad->helpPages)
    {
        heap_caps_free(ad->helpPages);
        ad->helpPages = NULL;
    }
}

/**
 * @brief Process input for the help pages
 *
 * @param ad All the artillery mode data
 * @param evt The button event to process
 */
void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    // Scrolls left and right
    if (buttonHelp(ad->help, evt))
    {
        // Exit the help menu
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief Draw the help pages
 *
 * @param ad All the artillery mode data
 * @param elapsedUs The time since this function was last called
 */
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    drawHelp(ad->help, elapsedUs);
}

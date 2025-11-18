//==============================================================================
// Includes
//==============================================================================

#include "artillery_help.h"
#include "artillery_phys.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const helpPage_t helpPages1[] = {
    {
        .title = artilleryModeName,
        .text  = "This is the first page of help text! Welcome!",
    },
    {
        .title = str_passAndPlay,
        .text  = "TODO str_passAndPlay note",
    },
    {
        .title = str_wirelessConnect,
        .text  = "TODO str_wirelessConnect note",
    },
    {
        .title = str_cpuPractice,
        .text  = "TODO str_cpuPractice note",
    },
    {
        .title = str_paintSelect,
        .text  = "TODO str_paintSelect note",
    },
    {
        .title = str_help,
        .text  = "TODO str_help note",
    },
    {
        .title = str_exit,
        .text  = "TODO str_exit note",
    },

    {
        .title = str_load_ammo,
        .text  = "TODO str_load_ammo note",
    },
    {
        .title = str_drive,
        .text  = "TODO str_drive note",
    },
    {
        .title = str_look_around,
        .text  = "TODO str_look_around note",
    },
    {
        .title = str_adjust,
        .text  = "TODO str_adjust note",
    },
    {
        .title = str_fire,
        .text  = "TODO str_fire note",
    },
};

static const helpPage_t helpPages2[] = {
    {
        .title = artilleryModeName,
        .text  = "TODO Good luck!",
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ad
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
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryHelpDeinit(artilleryData_t* ad)
{
    deinitHelpScreen(ad->help);
    if (ad->helpPages)
    {
        heap_caps_free(ad->helpPages);
        ad->helpPages = NULL;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param evt
 */
void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    if (buttonHelp(ad->help, evt))
    {
        // Exit the help menu
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param elapsedUs
 */
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    drawHelp(ad->help, elapsedUs);
}

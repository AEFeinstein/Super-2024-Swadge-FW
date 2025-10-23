/*! \file helpPages.h
 *
 * \section help_pages_design Design Philosophy
 *
 * Help pages can be used to draw multiple pages of multi-line text. This is useful for drawing a help manual for a
 * Swadge mode in a consistent manner. The pages are themed using menuMegaRenderer_t.
 *
 * \section help_pages_usage Usage
 *
 * initHelpScreen() must be called to initialize the screen before handling buttons and drawing. deinitHelpScreen() must
 * be called to free memory when it's no longer needed.
 *
 * buttonHelp() must be called to handle button input. It will return a boolean indicating if the help screen should be
 * exited.
 *
 * drawHelp() must be called to draw the screen.
 *
 * \section help_pages_example Example
 *
 * \code{.c}
 *
// Declare constant help pages
static const helpPage_t helpPages[] = {
    {
        .title = "Page 1",
        .text  = "This is the first page of help text! Welcome!",
    },
    {
        .title = "Page 2",
        .text  = "This is the second page of help text. It probably has more details or something.",
    },
};

// Set up the help screen when entering the mode
// The menu must be empty
menu_t* bgMenu                   = initMenu("Mode Name", NULL);
menuMegaRenderer_t* menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);
helpPageVars_t* help             = initHelpScreen(bgMenu, menuRenderer, helpPages, ARRAY_SIZE(helpPages));


// Call this in the mode's fnMainLoop function when checkButtonQueueWrapper() emits an event
if(buttonHelp(helpPageVars_t* help, buttonEvt_t* evt))
{
    // Exit the help menu
}


// Call this in the mode's fnMainLoop function to draw
drawHelp(help, elapsedUs);


// Deinitialize when exiting the mode
deinitHelpScreen(help);
 * \endcode
 */

#pragma once

#include <stdint.h>
#include "hdw-btn.h"
#include "menu.h"
#include "menuMegaRenderer.h"

/**
 * @brief A help page consisting of a title and text
 */
typedef struct
{
    const char* title; ///< The title for an individual help page
    const char* text;  ///< The text to be displayed on a help page
} helpPage_t;

/**
 * @brief All the variables required for a help screen
 */
typedef struct
{
    menu_t* bgMenu;                   ///< A menu to render behind the help text. Must not have any entries
    menuMegaRenderer_t* menuRenderer; ///< A renderer to render the menu with
    const helpPage_t* pages;          ///< All the help pages
    int32_t numPages;                 ///< The number of help pages in helpPageVars_t.pages
    int32_t helpIdx;                  ///< The index of the page currently being displayed
    int32_t arrowBlinkTimer;          ///< A timer used to blink arrows indicating more pages
} helpPageVars_t;

helpPageVars_t* initHelpScreen(menu_t* bgMenu, menuMegaRenderer_t* menuRenderer, const helpPage_t* pages,
                               int32_t numPages);
void deinitHelpScreen(helpPageVars_t* help);
bool buttonHelp(helpPageVars_t* help, buttonEvt_t* evt);
void drawHelp(helpPageVars_t* help, int32_t elapsedUs);

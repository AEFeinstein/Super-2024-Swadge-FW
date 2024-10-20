//==============================================================================
// Includes
//==============================================================================

#include "hdw-tft.h"
#include "sequencerHelp.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_MARGIN_L 18
#define TEXT_MARGIN_R 13

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* title;
    const char* text;
} seqHelpPage_t;

//==============================================================================
// Const data
//==============================================================================

const seqHelpPage_t helpPages[] = {
    {
        .title = sequencerName,
        .text  = "Welcome to the Sequencer. Let's learn how to make some music!",
    },
    {
        .title = sequencerName,
        .text  = "First off, pressing the Pause button always switches between viewing the menu and the grid.",
    },
    {
        .title = str_file,
        .text  = "In the menu, there are four \"File\" options which manage song data.",
    },
    {
        .title = str_file,
        .text  = "\"Save\" will save the currently loaded song.",
    },
    {
        .title = str_file,
        .text
        = "The song is auto saved when exiting the mode with the \"Exit\" option or the Menu, but NOT if you turn off "
          "the Swadge!",
    },
    {
        .title = str_file,
        .text  = "\"Save As\" will save the song in the slot of your choice. There are four slots.",
    },
    {
        .title = str_file,
        .text  = "\"Load\" will load a song from a slot with data in it.",
    },
    {
        .title = str_file,
        .text  = "\"Reset This Song\" will reset the grid to empty. This does not affect saved data.",
    },
    {
        .title = str_songOptions,
        .text = "In the menu, there are five \"Song Options\" that configure how the grid is shown and how the song is "
                "played.",
    },
    {
        .title = str_songOptions,
        .text  = "\"Tempo\" changes how fast the song is played, from slow (60 bpm) to fast (300 bpm).",
    },
    {
        .title = str_songOptions,
        .text = "\"Grid\" changes what notes the grid is drawn at and where the cursor snaps, from wide (whole notes), "
                "to narrow (sixteenth notes).",
    },
    {
        .title = str_songOptions,
        .text  = "\"Signature\" changes how many quarter notes are in a bar, from two (2/4) to seven (7/4).",
    },
    {
        .title = str_songOptions,
        .text = "\"Loop\" changes if the song starts playing from the beginning again when finished (On) or not (Off).",
    },
    {
        .title = str_songOptions,
        .text  = "\"End Song Here\" sets the song's ending where the cursor currently is. When playing, the song will "
                 "either stop or loop at this point.",
    },
    {
        .title = str_grid,
        .text  = "On the grid you can write and play back a song.",
    },
    {
        .title = str_grid,
        .text  = "The D-Pad moves the cursor.",
    },
    {
        .title = str_grid,
        .text  = "The A button adds a note to empty space or removes a note if one is in the cursor.",
    },
    {
        .title = str_grid,
        .text  = "The B button jumps to the beginning of the song, starts playing, and stops playing.",
    },
    {
        .title = str_grid,
        .text  = "The touchpad is a wheel menu that adjusts the note settings.",
    },
    {
        .title = str_grid,
        .text  = "The touchpad changes the length of the note (from sixteenth to whole) and the instrument.",
    },
    {
        .title = sequencerName,
        .text  = "Now go make a masterpiece!",
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param sv
 * @param tElapsed
 */
void drawSequencerHelp(sequencerVars_t* sv, int32_t tElapsed)
{
    // Set the title
    sv->bgMenu->title = helpPages[sv->helpIdx].title;

    // Draw background, without animation
    drawMenuMania(sv->bgMenu, sv->menuRenderer, 0);

    // Draw text
    int16_t xOff = TEXT_MARGIN_L;
    int16_t yOff = MANIA_TITLE_HEIGHT + 8;
    drawTextWordWrap(&sv->font_rodin, c000, helpPages[sv->helpIdx].text, &xOff, &yOff, TFT_WIDTH - TEXT_MARGIN_R,
                     TFT_HEIGHT);

    //  TODO page numbers?
    // TODO indicator arrows?
}

/**
 * @brief TODO
 *
 * @param sv
 * @param evt
 */
void buttonSequencerHelp(sequencerVars_t* sv, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_LEFT:
            {
                if (sv->helpIdx > 0)
                {
                    sv->helpIdx--;
                }
                else
                {
                    sv->screen = SEQUENCER_MENU;
                }
                break;
            }
            case PB_RIGHT:
            {
                if (sv->helpIdx < ARRAY_SIZE(helpPages) - 1)
                {
                    sv->helpIdx++;
                }
                else
                {
                    sv->screen = SEQUENCER_MENU;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
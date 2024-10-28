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

#define ARROW_BLINK_PERIOD 1000000

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
        .text  = "The song is auto saved when exiting the mode with the \"Exit\" option or the Menu button, but NOT if "
                 "you turn off "
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
        .text  = "\"End Song Here\" sets the song's ending where the cursor currently is. The song will either stop or "
                 "loop here.",
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
        .text  = "Up on the touchpad changes the instrument being placed.",
    },
    {
        .title = str_grid,
        .text  = "Down on the touchpad changes the length of the note from sixteenth note to whole note.",
    },
    {
        .title = str_grid,
        .text  = "The drums don't have hits for every note, so play around and find the good ones.",
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
 * @brief Draw the help menu
 *
 * @param sv The entire sequencer state
 * @param elapsedUs The time elapsed since the last function call
 */
void drawSequencerHelp(sequencerVars_t* sv, int32_t elapsedUs)
{
    // Set the title
    sv->bgMenu->title = helpPages[sv->helpIdx].title;

    // Draw background, without animation
    drawMenuMania(sv->bgMenu, sv->menuRenderer, 0);

    // Draw text
    paletteColor_t textColor    = c555;
    paletteColor_t outlineColor = c000;
    int16_t xOff                = TEXT_MARGIN_L;
    int16_t yOff                = MANIA_TITLE_HEIGHT + 8;
    drawTextWordWrap(&sv->font_rodin, textColor, helpPages[sv->helpIdx].text, &xOff, &yOff, TFT_WIDTH - TEXT_MARGIN_R,
                     TFT_HEIGHT);
    xOff = TEXT_MARGIN_L;
    yOff = MANIA_TITLE_HEIGHT + 8;
    drawTextWordWrap(&sv->font_rodin_outline, outlineColor, helpPages[sv->helpIdx].text, &xOff, &yOff,
                     TFT_WIDTH - TEXT_MARGIN_R, TFT_HEIGHT);

    // Draw page numbers
    char pageText[32];
    snprintf(pageText, sizeof(pageText) - 1, "%" PRId32 "/%" PRId32 "", 1 + sv->helpIdx,
             (int32_t)ARRAY_SIZE(helpPages));

    int16_t tWidth = textWidth(&sv->font_rodin, pageText);
    drawText(&sv->font_rodin, textColor, pageText, TFT_WIDTH - 30 - tWidth, TFT_HEIGHT - sv->font_rodin.height + 2);
    drawText(&sv->font_rodin_outline, outlineColor, pageText, TFT_WIDTH - 30 - tWidth,
             TFT_HEIGHT - sv->font_rodin_outline.height + 2);

    // Blink the arrows
    sv->arrowBlinkTimer += elapsedUs;
    while (sv->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        sv->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (sv->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        // Draw arrows to indicate this can be scrolled
        if (0 != sv->helpIdx)
        {
            // Draw left arrow if not on the first page
            drawText(&sv->font_rodin, textColor, "<", 0, (TFT_HEIGHT - sv->font_rodin.height) / 2);
            drawText(&sv->font_rodin_outline, outlineColor, "<", 0, (TFT_HEIGHT - sv->font_rodin_outline.height) / 2);
        }

        if ((ARRAY_SIZE(helpPages) - 1) != sv->helpIdx)
        {
            // Draw right arrow if not on the last page
            drawText(&sv->font_rodin, textColor, ">", TFT_WIDTH - textWidth(&sv->font_rodin, ">"),
                     (TFT_HEIGHT - sv->font_rodin.height) / 2);
            drawText(&sv->font_rodin_outline, outlineColor, ">", TFT_WIDTH - textWidth(&sv->font_rodin, ">"),
                     (TFT_HEIGHT - sv->font_rodin_outline.height) / 2);
        }
    }
}

/**
 * @brief Handle a button event on the help screen
 *
 * @param sv The entire sequencer state
 * @param evt The button event to handle
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
                    setSequencerScreen(SEQUENCER_MENU);
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
                    setSequencerScreen(SEQUENCER_MENU);
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
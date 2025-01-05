//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_gameEnd.h"
#include "swadgeHero_game.h"

//==============================================================================
// Defines
//==============================================================================

// Space between lines
#define TEXT_Y_SPACING 9
// Spacing for the fail chart
#define Y_MARGIN   8
#define Y_HEIGHT   50
#define X_MARGIN   ((TFT_WIDTH - NUM_FAIL_METER_SAMPLES) / 2)
#define BOX_MARGIN 2

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle input for the game end screen
 *
 * @param sh The Swadge Hero game state
 * @param evt The button event
 */
void shGameEndInput(shVars_t* sh, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
                // TODO add a timer to not allow this for a second or two after switching
                shChangeScreen(sh, SH_MENU);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief Draw the game end screen with statistics
 *
 * @param sh The Swadge Hero game state
 * @param elapsedUs The time elapsed since the last time this function was called.
 */
void shGameEndDraw(shVars_t* sh, int32_t elapsedUs)
{
    // Draw background for the body
    fillDisplayArea(0, 2 * Y_MARGIN + sh->rodin.height, TFT_WIDTH, TFT_HEIGHT, c111);

    // Start here
    int32_t yOff = Y_MARGIN;

    // Draw the name
    int16_t tWidth = textWidth(&sh->rodin, sh->songName);
    drawText(&sh->rodin, c555, sh->songName, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += sh->rodin.height + Y_MARGIN;

    // This is the text area between the title and fail chart
    int32_t textAreaTop    = yOff;
    int32_t textAreaBottom = TFT_HEIGHT - Y_MARGIN - Y_HEIGHT - BOX_MARGIN;

    // Vertically center the six timings
    yOff = textAreaTop
           + ((textAreaBottom - textAreaTop) - (NUM_NOTE_TIMINGS * sh->ibm.height)
              - ((NUM_NOTE_TIMINGS - 1) * TEXT_Y_SPACING))
                 / 2;

    // Draw all note count labels
    int32_t yCounts = yOff;
    int32_t maxXoff = 0;

    const paletteColor_t timingColors[NUM_NOTE_TIMINGS] = {c051, c141, c231, c321, c411, c501};
    for (int32_t i = 0; i < NUM_NOTE_TIMINGS; i++)
    {
        int32_t xOff = drawText(&sh->ibm, timingColors[i], timings[i].label, TEXT_Y_SPACING, yOff);
        if (xOff > maxXoff)
        {
            maxXoff = xOff;
        }
        yOff += sh->ibm.height + TEXT_Y_SPACING;
    }

    // Draw all note counts
    yOff          = yCounts;
    int32_t xVals = maxXoff + TEXT_Y_SPACING;
    for (int32_t i = 0; i < NUM_NOTE_TIMINGS; i++)
    {
        char histText[32];
        snprintf(histText, sizeof(histText), "%" PRId32, sh->noteHistogram[i]);
        int32_t xOff = drawText(&sh->ibm, c555, histText, xVals, yOff);
        if (xOff > maxXoff)
        {
            maxXoff = xOff;
        }
        yOff += sh->ibm.height + TEXT_Y_SPACING;
    }

    // Draw a line between hits and misses
    int16_t lineY = yOff - (sh->ibm.height + TEXT_Y_SPACING) - ((TEXT_Y_SPACING + 1) / 2);
    drawLineFast(TEXT_Y_SPACING, lineY, maxXoff, lineY, timingColors[NUM_NOTE_TIMINGS - 1]);

    // Vertically center the three score parts
    yOff = textAreaTop + ((textAreaBottom - textAreaTop) - (3 * sh->rodin.height) - (2 * TEXT_Y_SPACING)) / 2;

    // Draw letter
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%s", sh->grade);
    tWidth                     = textWidth(&sh->rodin, scoreStr);
    paletteColor_t letterColor = 'S' == sh->grade[0] ? c225 : timingColors[sh->grade[0] - 'A'];
    drawText(&sh->rodin, letterColor, scoreStr, maxXoff + (TFT_WIDTH - maxXoff - tWidth) / 2, yOff);
    yOff += sh->rodin.height + TEXT_Y_SPACING;

    // Draw the score
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, sh->score);
    tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, maxXoff + (TFT_WIDTH - maxXoff - tWidth) / 2, yOff);
    yOff += sh->rodin.height + TEXT_Y_SPACING;

    // Draw max combo
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32 " Combo", sh->maxCombo);
    tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, maxXoff + (TFT_WIDTH - maxXoff - tWidth) / 2, yOff);
    yOff += sh->rodin.height + TEXT_Y_SPACING;

    // Draw a graph of the fail meter
    if (sh->failOn)
    {
        drawRect(X_MARGIN - BOX_MARGIN, TFT_HEIGHT - Y_MARGIN - Y_HEIGHT - BOX_MARGIN, //
                 TFT_WIDTH - X_MARGIN + BOX_MARGIN, TFT_HEIGHT - Y_MARGIN + BOX_MARGIN, c220);

        // Setup the first point
        int32_t xOff    = (TFT_WIDTH - NUM_FAIL_METER_SAMPLES) / 2;
        vec_t lastPoint = {
            .x = xOff,
            .y = TFT_HEIGHT - Y_MARGIN - 25,
        };

        // Iterate through all points
        node_t* failNode = sh->failSamples.first;
        while (failNode)
        {
            // Draw a line from the last point to this one
            xOff++;
            vec_t cPoint = {
                .x = xOff,
                .y = TFT_HEIGHT - Y_MARGIN - ((intptr_t)failNode->val) / 2,
            };
            drawLineFast(lastPoint.x, lastPoint.y, cPoint.x, cPoint.y, c440);

            // Increment to the next
            lastPoint = cPoint;
            failNode  = failNode->next;
        }
    }
}

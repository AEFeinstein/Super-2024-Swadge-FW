//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_gameEnd.h"
#include "swadgeHero_game.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Const Variables
//==============================================================================

//==============================================================================
// Function Declarations
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param sh
 * @param evt
 */
void shGameEndInput(shVars_t* sh, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            {
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
 * @brief TODO doc
 *
 * @param sh
 * @param elapsedUs
 */
void shGameEndDraw(shVars_t* sh, int32_t elapsedUs)
{
// Space between lines
#define TEXT_Y_SPACING 8
// Spacing for the fail chart
#define Y_MARGIN   14
#define Y_HEIGHT   50
#define X_MARGIN   ((TFT_WIDTH - NUM_FAIL_METER_SAMPLES) / 2)
#define BOX_MARGIN 2

    clearPxTft();

    int32_t yOff = Y_MARGIN;

    // Draw the name
    int16_t tWidth = textWidth(&sh->rodin, sh->songName);
    drawText(&sh->rodin, c555, sh->songName, (TFT_WIDTH - tWidth) / 2, yOff);
    yOff += sh->rodin.height;

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
    for (int32_t i = 0; i < NUM_NOTE_TIMINGS; i++)
    {
        int32_t xOff = drawText(&sh->ibm, c555, timings[i].label, TEXT_Y_SPACING, yOff);
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

    // Vertically center the three score parts
    yOff = textAreaTop + ((textAreaBottom - textAreaTop) - (3 * sh->rodin.height) - (2 * TEXT_Y_SPACING)) / 2;

    // Draw letter
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%s", sh->grade);
    tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, maxXoff + (TFT_WIDTH - maxXoff - tWidth) / 2, yOff);
    yOff += sh->rodin.height + TEXT_Y_SPACING;

    // Draw the score
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, sh->score);
    tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, maxXoff + (TFT_WIDTH - maxXoff - tWidth) / 2, yOff);
    yOff += sh->rodin.height + TEXT_Y_SPACING;

    // Draw max combo
    snprintf(scoreStr, sizeof(scoreStr) - 1, "Combo: %" PRId32, sh->maxCombo);
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

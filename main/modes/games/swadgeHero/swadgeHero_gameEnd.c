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
    clearPxTft();

    // Draw the score
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRId32, sh->score);
    int16_t tWidth = textWidth(&sh->rodin, scoreStr);
    drawText(&sh->rodin, c555, scoreStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - sh->rodin.height) / 2);

    // Draw a graph of the fail meter
    if (sh->failOn)
    {
#define Y_MARGIN   20
#define Y_HEIGHT   50
#define X_MARGIN   ((TFT_WIDTH - NUM_FAIL_METER_SAMPLES) / 2)
#define BOX_MARGIN 2

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

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "swadge2024.h"

#include "picross_menu.h"
#include "picross_select.h"
#include "picross_menu.h"

//====
// globals, basically
//====
picrossLevelSelect_t* ls;

static const char str_win[] = "You Are Win!";

//===
// Function Prototypes
//
void levelSelectInput(void);
void drawLevelSelectScreen(font_t* font);
void drawPicrossLevelWSG(wsg_t* wsg, int16_t xOff, int16_t yOff, bool highlight);
void drawPicrossPreviewWindow(wsg_t* wsg);
//====
// Functions
//====

// Initiation
void picrossStartLevelSelect(font_t* bigFont, picrossLevelDef_t levels[])
{
    ls            = heap_caps_calloc(1, sizeof(picrossLevelSelect_t), MALLOC_CAP_8BIT);
    ls->game_font = bigFont;
    // ls->smallFont = smallFont;
    loadFont(EARLY_GAMEBOY_FONT, &(ls->smallFont), false);
    loadWsg(UNKNOWN_PUZZLE_WSG, &ls->unknownPuzzle, false);

    size_t size                    = sizeof(picrossVictoryData_t);
    picrossVictoryData_t* victData = heap_caps_calloc(
        1, size, MALLOC_CAP_8BIT); // zero out. if data doesnt exist, then its been correctly initialized to all 0s.
    readNvsBlob(picrossCompletedLevelData, victData, &size);
    ls->currentIndex = -1; // set to impossible index so we don't continue level 0 when we haven't started level 0
    readNvs32(picrossCurrentPuzzleIndexKey, &ls->currentIndex);
    ls->allLevelsComplete = true;
    for (int i = 0; i < PICROSS_LEVEL_COUNT; i++)
    {
        // set completed data from save data.
        if (victData->victories[i] == true)
        {
            levels[i].completed = true;
        }
        else
        {
            levels[i].completed   = false;
            ls->allLevelsComplete = false;
        }

        ls->levels[i] = levels[i];
    }

    ls->hoverX          = 0;
    ls->hoverY          = 0;
    ls->hoverLevelIndex = 0;
    ls->topVisibleRow   = 0; // todo: move to hold.
    ls->prevBtnState    = PB_SELECT | PB_START | PB_A | PB_B | PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT;

    ls->btnState = 0;

    // visual settings
    ls->cols        = 5;
    ls->rows        = 6;
    ls->totalRows   = (PICROSS_LEVEL_COUNT + (ls->cols - 1)) / ls->cols;
    ls->paddingLeft = 10;
    ls->paddingTop  = 20;
    ls->gap         = 5;
    ls->gridScale   = 30; // PICROSS_MAX_SIZE * 2

    heap_caps_free(victData);
}

void picrossLevelSelectLoop(int64_t elapsedUs)
{
    // Draw The Screen
    drawLevelSelectScreen(ls->game_font);

    // Handle Input
    // has to happen last so we can free up on exit.
    // todo: make a (free/exit) bool flag.
    levelSelectInput();
}

void levelSelectInput()
{
    // todo: quit with both start+select

    if (ls->btnState & (PB_START | PB_B) && !(ls->prevBtnState & PB_START) && !(ls->btnState & PB_A))
    {
        // exit to main menu
        returnToPicrossMenu(); // from level select.
        return;
    }
    // Choosing a Level
    if (ls->btnState & PB_A && !(ls->prevBtnState & PB_A) && !(ls->btnState & PB_START))
    {
        if (ls->hoverLevelIndex == ls->currentIndex)
        {
            continueGame();
            picrossExitLevelSelect();
            return;
        }
        if (ls->hoverLevelIndex < PICROSS_LEVEL_COUNT)
        {
            ls->chosenLevel = &ls->levels[ls->hoverLevelIndex];
            writeNvs32(picrossCurrentPuzzleIndexKey,
                       ls->hoverLevelIndex); // save the selected level before we lose context of the index.

            size_t size                     = sizeof(picrossProgressData_t);
            picrossProgressData_t* progress = heap_caps_calloc(
                1, size,
                MALLOC_CAP_8BIT); // zero out. if data doesnt exist, then its been correctly initialized to all 0s.
            readNvsBlob(picrossCompletedLevelData, progress, &size);

            selectPicrossLevel(ls->chosenLevel);
            picrossExitLevelSelect();
            heap_caps_free(progress);
        }
        return;
    }

    int xBound = ls->cols * (ls->hoverY + ls->topVisibleRow + 1);
    if (xBound > PICROSS_LEVEL_COUNT)
    {
        xBound = PICROSS_LEVEL_COUNT % ls->cols;
    }
    else
    {
        xBound = ls->cols;
    }

    // Input Movement checks
    if (ls->btnState & PB_RIGHT && !(ls->prevBtnState & PB_RIGHT))
    {
        ls->hoverX++;
        if (ls->hoverX >= xBound)
        {
            ls->hoverX = 0;
        }
    }
    else if (ls->btnState & PB_LEFT && !(ls->prevBtnState & PB_LEFT))
    {
        ls->hoverX--;
        if (ls->hoverX < 0)
        {
            ls->hoverX = xBound - 1;
        }
    }
    else if (ls->btnState & PB_DOWN && !(ls->prevBtnState & PB_DOWN))
    {
        ls->hoverY++;
        if (ls->hoverY >= ls->rows)
        {
            if (ls->topVisibleRow > ls->totalRows - ls->rows - 1)
            {
                // cycle to top
                ls->hoverY        = 0;
                ls->topVisibleRow = 0;
            }
            else
            {
                // scroll down
                ls->topVisibleRow++;
                ls->hoverY--; // instead of moving cursor down.
            }
        }
    }
    else if (ls->btnState & PB_UP && !(ls->prevBtnState & PB_UP))
    {
        ls->hoverY--;
        if (ls->hoverY < 0)
        {
            if (ls->topVisibleRow == 0)
            {
                // at the complete top. cycle all the way to the bottom.
                ls->hoverY        = ls->rows - 1;
                ls->topVisibleRow = ls->totalRows - ls->rows;
            }
            else
            {
                // scroll up instead of moving curser up.
                ls->topVisibleRow--;
                ls->hoverY++;
            }
        }
    }

    // Recalc xBound after potentially changing rows
    xBound = ls->cols * (ls->hoverY + ls->topVisibleRow + 1);
    if (xBound > PICROSS_LEVEL_COUNT)
    {
        xBound = PICROSS_LEVEL_COUNT % ls->cols;
    }
    else
    {
        xBound = ls->cols;
    }

    if (ls->hoverX >= xBound)
    {
        ls->hoverX = xBound - 1;
    }

    ls->hoverLevelIndex = ls->hoverY * ls->cols + (ls->topVisibleRow * ls->cols) + ls->hoverX;

    // hack for when levels aren't a multiple of 5, to not selet into empty space. Run the input again recursively until
    // selection wraps back over into a valid index.
    if (ls->hoverLevelIndex >= PICROSS_LEVEL_COUNT)
    {
        levelSelectInput();
    }

    ls->prevBtnState = ls->btnState;
}

void drawLevelSelectScreen(font_t* font)
{
    clearPxTft();
    uint8_t s = ls->gridScale; // scale
    uint8_t x;
    uint8_t y;
    char textBuffer[64];

    // todo: Draw Choose Level Text.
    drawText(font, c555, "Puzzle", 190, 30);
    drawText(font, c555, "Select", 190, 60);
    snprintf(textBuffer, sizeof(textBuffer) - 1, "%d/%d", (int)ls->hoverLevelIndex + 1, (int)PICROSS_LEVEL_COUNT);
    int16_t t = textWidth(&ls->smallFont, textBuffer) / 2;
    drawText(&ls->smallFont, c555, textBuffer, TFT_WIDTH - 54 - t, 90);

    int start = ls->topVisibleRow * ls->cols;
    int end   = ls->cols * ls->rows;
    // max against total level count.
    // end = end > PICROSS_LEVEL_COUNT ? end : PICROSS_LEVEL_COUNT - (PICROSS_LEVEL_COUNT%ls->cols);
    if (ls->topVisibleRow + ls->rows < ls->totalRows)
    {
        // draw ... at the bottom of the screen to indicate more puzzles.
        end += ls->cols;
    }

    // draw the top to indicate more puzzles.
    if (ls->topVisibleRow > 0)
    {
        for (int i = 0; i < ls->cols; i++)
        {
            x = 0;
            if (i != 0)
            {
                x = (i % ls->cols);
            }
            x      = x * s + ls->paddingLeft + ls->gap * x;
            int ty = -s + ls->paddingTop - ls->gap;
            if (ls->levels[start - ls->cols + i].completed)
            {
                drawPicrossLevelWSG(&ls->levels[start - ls->cols + i].completedWSG, x, ty, false);
            }
            else
            {
                // Draw ? sprite
                drawPicrossLevelWSG(&ls->unknownPuzzle, x, ty, ((start - ls->cols + i) == ls->currentIndex));
            }
        }
    }

    for (int i = 0; i < end; i++)
    {
        if (start + i >= PICROSS_LEVEL_COUNT)
        {
            break;
        }

        y = i / ls->cols;
        x = 0;
        if (i != 0)
        {
            x = (i % ls->cols);
        }
        x = x * s + ls->paddingLeft + ls->gap * x;
        y = y * s + ls->paddingTop + ls->gap * y;
        if (ls->levels[start + i].completed)
        {
            drawPicrossLevelWSG(&ls->levels[start + i].completedWSG, x, y, false);
        }
        else
        {
            // Draw ? sprite
            drawPicrossLevelWSG(&ls->unknownPuzzle, x, y, ((start + i) == ls->currentIndex));
        }
    }

    if (ls->hoverLevelIndex < PICROSS_LEVEL_COUNT)
    {
        // Draw the current level difficulty at the bottom left.
        //(debug)
        snprintf(textBuffer, sizeof(textBuffer) - 1, "%" PRIu16 "x%" PRIu16,
                 (int)ls->levels[ls->hoverLevelIndex].levelWSG.w, (int)ls->levels[ls->hoverLevelIndex].levelWSG.h);
        t = textWidth(&ls->smallFont, textBuffer) / 2;
        drawText(&ls->smallFont, c555, textBuffer, TFT_WIDTH - 54 - t, TFT_HEIGHT - 28);
    }

    //
    // draw level choose input
    x = ls->hoverX;
    y = ls->hoverY;

    box_t inputBox = {
        .x0 = (x * s) + ls->paddingLeft + ls->gap * x,
        .y0 = (y * s) + ls->paddingTop + ls->gap * y,
        .x1 = (x * s) + s + ls->paddingLeft + ls->gap * x,
        .y1 = (y * s) + s + ls->paddingTop + ls->gap * y,
    };
    // draw preview window
    if (ls->levels[ls->hoverLevelIndex].completed)
    {
        // if completed, show victory image and green hover
        // only do error bounds checking on the window to prevent crashing. Cursor showing out of bounds is less
        // confusing than cursor vanishing
        if (ls->hoverLevelIndex < PICROSS_LEVEL_COUNT)
        { // This doesnt actually work because the index goes off into pointer-land
            drawPicrossPreviewWindow(&ls->levels[ls->hoverLevelIndex].completedWSG);
        }
        drawBox(inputBox, c151, false, 0);
    }
    else
    {
        // if incomplete, show ? image and red hvoer
        drawPicrossPreviewWindow(&ls->unknownPuzzle);
        drawBox(inputBox, c511, false, 0);
        inputBox.x0++;
        inputBox.x1--;
        inputBox.y0++;
        inputBox.y1--;
        drawBox(inputBox, c511, false, 0);
    }

    if (ls->allLevelsComplete)
    {
        drawText(ls->game_font, c000, str_win, 53, 103);
        drawText(ls->game_font, c555, str_win, 50, 100);
    }
}

void picrossLevelSelectButtonCb(buttonEvt_t* evt)
{
    ls->btnState = evt->state;
}

void picrossExitLevelSelect()
{
    if (NULL != ls)
    {
        freeWsg(&ls->unknownPuzzle);
        freeFont(&(ls->smallFont));

        // freeFont((ls->game_font));
        // heap_caps_free(&ls->chosenLevel->title);
        // heap_caps_free(&ls->chosenLevel);
        heap_caps_free(ls);
        ls = NULL;
    }
}

// Little replacement of drawWsg to draw the 2px for each px. Sprite scaling doesnt appear to be a thing yet, so this is
// my hack. we could do 4x and have plenty of room?
void drawPicrossLevelWSG(wsg_t* wsg, int16_t xOff, int16_t yOff, bool highlight)
{
    if (NULL == wsg->px)
    {
        return;
    }

    // Draw the WSG into a 30 pixel square area. thats PICROSSMAXLEVELSIZE*2. This is also gridScale.
    // gridScale works because 5,10, and 15 will divide into 30 evenly. Those are the only levels we plan on really
    // supporting. if we just pick whatever gridscale, then this function isn't would be ... wonky BUT if we do
    // something else, uh... it'll maybe not be the right size but... should look close to centered.
    uint16_t pixelPerPixel = ls->gridScale / ((wsg->w > wsg->h) ? wsg->w : wsg->h);

    // level draw size is 2 pixels for each square when drawing 15x15 squares.
    // 3 pixels for 10x10
    // 5 pixels for 5x5

    if (wsg->w != wsg->h)
    {
        if (wsg->h < wsg->w)
        {
            yOff = yOff + (((PICROSS_MAX_LEVELSIZE * 2) - wsg->h * pixelPerPixel)) / 2;
        }
        else
        {
            // calculate new x offset to center
            xOff = xOff + (((PICROSS_MAX_LEVELSIZE * 2) - wsg->w * pixelPerPixel) / 2);
        }
    }

    // Draw the image's pixels
    for (int16_t srcY = 0; srcY < wsg->h; srcY++)
    {
        for (int16_t srcX = 0; srcX < wsg->w; srcX++)
        {
            // Draw if not transparent
            if (cTransparent != wsg->px[(srcY * wsg->w) + srcX])
            {
                // Transform this pixel's draw location as necessary
                int16_t dstX = srcX * pixelPerPixel + xOff;
                int16_t dstY = srcY * pixelPerPixel + yOff;

                // Check bounds
                if (0 <= dstX && dstX < TFT_WIDTH && 0 <= dstY && dstY <= TFT_HEIGHT)
                {
                    // //root pixel

                    // // Draw the pixel
                    for (int i = 0; i < pixelPerPixel; i++)
                    {
                        for (int j = 0; j < pixelPerPixel; j++)
                        {
                            setPxTft(dstX + i, dstY + j, wsg->px[(srcY * wsg->w) + srcX]);
                        }
                    }
                }
            }
        }
    }
    // draw square around the puzzle
    if (highlight)
    {
        //-1 and +2 is to go around the image, not through
        xOff       = xOff - 1;
        yOff       = yOff - 1;
        uint16_t w = pixelPerPixel * wsg->w + 2;
        uint16_t h = pixelPerPixel * wsg->h + 2;

        drawLine(xOff, yOff, xOff + w, yOff, c550, 3);         // top
        drawLine(xOff, yOff + h, xOff + w, yOff + h, c550, 3); // bottom
        drawLine(xOff, yOff, xOff, yOff + h, c550, 3);         // left
        drawLine(xOff + w, yOff, xOff + w, yOff + h, c550, 3); // right
    }
}

/// @brief Draws the picross preview of the level. Really, it just draws a wsg at a very specific location using drawBox
/// calls.It can likely be optimized (by pulling the actual drawing code from drawBox into here?)
/// @param d display
/// @param wsg image to draw. on hover, it will be told to draw the image of the level we are hoving over - a '?' image
/// or the completed one, as appopriate.
void drawPicrossPreviewWindow(wsg_t* wsg)
{
    uint8_t s = 3 * ls->gridScale / ((wsg->w > wsg->h) ? wsg->w : wsg->h);

    for (int i = 0; i < wsg->w; i++)
    {
        for (int j = 0; j < wsg->h; j++)
        {
            box_t box = {
                .x0 = (i * s) + ls->paddingLeft + 176,
                .y0 = (j * s) + ls->paddingTop + 90,
                .x1 = (i * s) + s + ls->paddingLeft + 176,
                .y1 = (j * s) + s + ls->paddingTop + 90,
            };
            drawBox(box, wsg->px[(j * wsg->w) + i], true, 0);
        }
    }
}

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "swadge2024.h"

#include "mode_picross.h"
#include "picross_menu.h"
#include "picross_select.h"

//==============================================================================
// Function Prototypes
//==============================================================================
void picrossUserInput(int64_t elapsedUs);
void picrossCheckLevel(void);
void picrossSetupPuzzle(bool cont);
void picrossCalculateHoverHint(void);
void setCompleteLevelFromWSG(wsg_t* puzz);
void drawSinglePixelFromWSG(int x, int y, wsg_t* image, int offsetX, int offsetY);
// bool hintsMatch(picrossHint_t a, picrossHint_t b);
bool hintIsFilledIn(picrossHint_t* hint);
box_t boxFromCoord(int8_t x, int8_t y);
picrossHint_t newHintFromPuzzle(uint8_t index, bool isRow,
                                picrossSpaceType_t source[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE]);
void drawPicrossScene(void);
void drawPicrossHud(font_t* font);
void drawHint(font_t* font, picrossHint_t hint);
void drawPicrossInput(void);
void drawBackground(void);
void countInput(picrossDir_t input);
void drawNumberAtCoord(font_t* font, paletteColor_t color, uint8_t number, int8_t x, int8_t y, int16_t xOff,
                       int16_t yOff);
int8_t getHintShift(uint8_t hint);
void saveCompletedOnSelectedLevel(bool completed);
void enterSpace(uint8_t x, uint8_t y, picrossSpaceType_t newSpace);
bool toggleTentativeMark(uint8_t x, uint8_t y);
bool setTentativeMark(uint8_t x, uint8_t y, bool mark);
void picrossVictoryLEDs(uint32_t tElapsedUs, uint32_t arg, bool reset);
int8_t lerp(int8_t a, int8_t b, uint16_t amount);
//==============================================================================
// Variables
//==============================================================================

static picrossGame_t* p = NULL;

//==============================================================================
// Functions
//==============================================================================
/**
 * Initialize all data needed for the jumper game
 *
 * @param disp The display to draw to
 * @param mmFont The font used for teh HUD, already loaded
 *
 */
void picrossStartGame(font_t* mmFont, picrossLevelDef_t* selectedLevel, bool cont, menuMegaRenderer_t* renderer, wsg_t* bigBody)
{
    // calloc is 0'd and malloc leaves memory uninitialized. I dont know which to use so im not gonna touch it, and
    // doing things once on load can be slower.
    p                = heap_caps_calloc(1, sizeof(picrossGame_t), MALLOC_CAP_8BIT);
    p->selectedLevel = *selectedLevel;
    p->currentPhase  = PICROSS_SOLVING;
    p->exitThisFrame = false;

    // bg scrolling
    p->bgScrollTimer    = 0;
    p->bgScrollSpeed    = 50000;
    p->bgScrollXFrame   = 0;
    p->bgScrollYFrame   = 0;
    p->animtAccumulated = 0;
    p->ledAnimCount     = 0;

    // Puzzle
    p->puzzle = heap_caps_calloc(1, sizeof(picrossPuzzle_t), MALLOC_CAP_8BIT);
    // puzzle gets set in picrossSetupPuzzle.

    p->fadeHints = true; // When should this be turned on or off? Options menu is a lot.
    // Input Setup
    p->input                  = heap_caps_calloc(1, sizeof(picrossInput_t), MALLOC_CAP_8BIT);
    p->input->x               = 0;
    p->input->y               = 0;
    p->input->hoverBlockSizeX = 0;
    p->input->hoverBlockSizeY = 0;
    p->input->btnState        = 0;
    p->input->prevBtnState
        = 0x80 | 0x10 | 0x40
          | 0x20; // prevents us from instantly fillling in a square because A is held from selecting level.
    p->input->prevTouchState         = false;
    p->input->touchState             = false;
    p->countState                    = PICROSSDIR_IDLE;
    p->controlsEnabled               = true;
    p->input->DASActive              = false;
    p->input->blinkError             = false;
    p->input->blinkAnimTimer         = 0;
    p->input->startHeldType          = OUTOFBOUNDS;
    p->input->startTentativeMarkType = false;
    p->input->inputBoxColor = c005; // now greenish for more pop against marked. old burnt orange: c430. old green: c043
                                    // lets test this game against various forms of colorblindness. I'm concerned about
                                    // deuteranopia. Input square is my largest concern.
    p->input->inputBoxDefaultColor
        = p->input->inputBoxColor; // inputBoxColor gets changed during runtime, so we cache the desired.
    p->input->inputBoxErrorColor    = c500;
    p->input->markXColor            = c522;
    p->input->movedThisFrame        = false;
    p->input->changedLevelThisFrame = false;
    p->count                        = 1;
    p->input->holdingDir            = PICROSSDIR_IDLE;

    // input settings (adjust these)
    p->input->firstDASTime = 300000; //.5 seconds
    p->input->DASTime      = 100000; // 1,000,000 = 1 second.
    p->input->blinkTime    = 120000; // error blink. half a blink cycle (on)(off).
    p->input->blinkCount   = 6;      // twice blink count (3 blinks)

    // load options data
    p->input->showHints  = picrossGetSaveFlag(PO_SHOW_HINTS);
    p->input->showGuides = picrossGetLoadedSaveFlag(PO_SHOW_GUIDES);
    p->animateBG         = -1 + picrossGetLoadedSaveFlag(PO_BG_HEXAGONS) + (picrossGetLoadedSaveFlag(PO_BG_DOTS) * 2) + (picrossGetLoadedSaveFlag(PO_BG_NONE) * 3);
    p->markX             = picrossGetLoadedSaveFlag(PO_MARK_X);

    // cant tell if this is doing things the lazy way or not.
    for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        p->offLEDS[i].r = 0x00;
        p->offLEDS[i].g = 0x00;
        p->offLEDS[i].b = 0x00;

        if (i % 2 == 0)
        {
            p->errorALEDBlinkLEDS[i].r = 0xFF; // make less bright?
            p->errorALEDBlinkLEDS[i].g = 0x00;
            p->errorALEDBlinkLEDS[i].b = 0x00;

            p->errorBLEDBlinkLEDS[i].r = 0x00;
            p->errorBLEDBlinkLEDS[i].g = 0x00;
            p->errorBLEDBlinkLEDS[i].b = 0x00;
        }
        else
        {
            p->errorALEDBlinkLEDS[i].r = 0x00;
            p->errorALEDBlinkLEDS[i].g = 0x00;
            p->errorALEDBlinkLEDS[i].b = 0x00;

            p->errorBLEDBlinkLEDS[i].r = 0xFF;
            p->errorBLEDBlinkLEDS[i].g = 0x00;
            p->errorBLEDBlinkLEDS[i].b = 0x00;
        }
    }
    p->elapsedUs      = 0;
    p->loopingTimer   = 0;
    p->lerpAmount     = 0;
    p->marqueeScrollX = -10000000;

    // BG music
    // TODO MIDI music
    // buzzer_stop();
    // buzzer_play_bgm(&picross_music_bg);

    // Setup level
    picrossSetupPuzzle(cont);

    // set the winning positional offset.
    p->offsetX = (TFT_WIDTH / 2) - ((p->puzzle->width * p->drawScale) / 2) - p->drawScale - p->leftPad;
    p->offsetY = 38 - p->drawScale - p->topPad;

    //used for the mega man background effect
    setDrawBody(renderer, false);
    p->menu = initMenu("", NULL);
    p->renderer = renderer;
    p->bigBody = bigBody;
}

void picrossSetupPuzzle(bool cont)
{
    wsg_t* levelwsg = &p->selectedLevel.levelWSG;

    setCompleteLevelFromWSG(levelwsg);

    p->puzzle->width  = levelwsg->w;
    p->puzzle->height = levelwsg->h;

    // Calculate display information, now that we know the width/height of the puzzle.

    // rows
    for (int i = 0; i < p->puzzle->height; i++)
    {
        // todo: incompatible pointer type here?
        p->puzzle->rowHints[i] = newHintFromPuzzle(i, true, p->puzzle->completeLevel);
    }

    // cols
    for (int i = 0; i < p->puzzle->width; i++)
    {
        p->puzzle->colHints[i] = newHintFromPuzzle(i, false, p->puzzle->completeLevel);
    }

    // Set the current level appropriately.
    if (cont)
    {
        // Set the actual level from save data
        loadPicrossProgress();
        p->input->changedLevelThisFrame = true; // Force a check on load, so fil hints get greyed out.
    }
    else
    {
        // Set the actual level to be empty
        for (int i = 0; i < p->puzzle->width; i++)
        {
            for (int j = 0; j < p->puzzle->height; j++)
            {
                p->puzzle->level[i][j] = SPACE_EMPTY;
            }
        }
        savePicrossProgress();               // clear out your previous data.
        saveCompletedOnSelectedLevel(false); // clear out victory data: opening a level resets victory.
    }

    // Write the hint labels for each row and column

    // TFT_WIDTH = 240. max width is width+(ceil(width/2)) (15+8 = 23) = 10

    // max w/h. Non-square puzzles? I hope to write it so they will just... kind of work!

    uint16_t totalXCount        = p->puzzle->width + p->maxHintsX + 1;
    uint16_t totalYCount        = p->puzzle->height + p->maxHintsY + 1;
    uint16_t extraSizeForGuides = p->puzzle->height > 13 ? 3 : 2; // Makes enough room for the hints.
    uint16_t size               = (((totalXCount) > (totalYCount)) ? (totalXCount) : (totalYCount)); // math.max
    size                        = size
           + (p->input->showGuides ? extraSizeForGuides
                                   : 0); // add an extra space for the right-bottom side hover hints.

    p->clueGap            = 2; // this only being used for horizontal clues has added some complexity.
    uint16_t clueGapTotal = (p->maxHintsX - 1) * p->clueGap;

    // x/y rounds down, x(+y-1)/y rounds up. for positive numbers.
    p->drawScale
        = (TFT_WIDTH - (2 * p->maxHintsX) + (size - 1) - PICROSS_EXTRA_PADDING - clueGapTotal) / (size); // h drawscale
    uint16_t vdrawScale = (TFT_HEIGHT - (2 * p->maxHintsY) + (size - 1) - PICROSS_EXTRA_PADDING) / (size);
    // min between hDrawScale and vDrawScale
    p->drawScale = (((vdrawScale) < (p->drawScale)) ? (vdrawScale) : (p->drawScale));
    // paddingL = (TFTWidth - (totalCount*scale))/2;

    p->leftPad = (TFT_WIDTH - ((totalXCount * p->drawScale))) / 2 + p->maxHintsX * p->drawScale;
    p->topPad  = (TFT_HEIGHT - ((totalYCount * p->drawScale))) / 2 + p->maxHintsY * p->drawScale;

    // load the font
    // UIFont:
    loadFont(EARLY_GAMEBOY_FONT, &(p->UIFont), false);
    // Hint font:
    if (p->drawScale < 12)
    {
        // font
        loadFont(TOM_THUMB_FONT, &(p->hintFont), false);
    }
    else if (p->drawScale < 24)
    {
        loadFont(IBM_VGA_8_FONT, &(p->hintFont), false);
    }
    else
    {
        loadFont(EARLY_GAMEBOY_FONT, &(p->hintFont), false);
    }
    p->vFontPad = (p->drawScale - p->hintFont.height) / 2;
    // Calculate the shift to move the font square to the center of the level square.
    // fontShiftLeft = p->hont_font->w

    // Check the level immediately, in case there are any empty rows or columns whose hints we need to gray out at the
    // start
    picrossCheckLevel();
}

// Scans the levelWSG and creates the finished version of the puzzle in the proper data format (2D array of enum).
void setCompleteLevelFromWSG(wsg_t* puzz)
{
    // wsg_t puzz = puzzle;
    // go row by row
    paletteColor_t col = c555;
    // we can't ignore out of bounds data, because we want to be able to reset without reading garbage
    for (int r = 0; r < PICROSS_MAX_LEVELSIZE; r++) // todo: puzzles that dont have the right size?
    {
        if (r < puzz->h)
        {
            for (int c = 0; c < PICROSS_MAX_LEVELSIZE; c++)
            {
                if (c < puzz->w)
                {
                    col = puzz->px[(r * puzz->w) + c];
                    if (col == cTransparent || col == c555)
                    {
                        p->puzzle->completeLevel[c][r] = SPACE_EMPTY;
                        continue;
                    }
                    else
                    {
                        p->puzzle->completeLevel[c][r] = SPACE_FILLED;
                        continue;
                    }
                }
                else
                {
                    p->puzzle->completeLevel[c][r] = OUTOFBOUNDS;
                    continue;
                }
            }
        }
        else
        {
            for (int c = 0; c < PICROSS_MAX_LEVELSIZE; c++)
            {
                if (c < puzz->h)
                {
                    p->puzzle->completeLevel[c][r] = OUTOFBOUNDS;
                    continue;
                }
            }
        }
    }
}

// we use the same data type for rows and columns, hence the bool flag. index is the position in the row/col, and we
// pass in the level. I guess that could be a pointer? Somebody more experienced with c, code review this and optimize
// please.
picrossHint_t newHintFromPuzzle(uint8_t index, bool isRow,
                                picrossSpaceType_t source[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE])
{
    picrossHint_t t;
    t.complete = false;
    t.filledIn = false;
    t.correct  = false;
    t.isRow    = isRow;
    t.index    = index;

    // set all hints to 0.
    // for smaller levels than 15x15, we should set this dynamically to the current max hint size.
    // bit things will _work_ with larger hints, so its a minor refactor later.
    for (int i = 0; i < PICROSS_MAX_HINTCOUNT; i++)
    {
        t.hints[i] = 0;
    }

    // todo: non-square puzzles. get width/height passed in from newHint.
    picrossSpaceType_t prev = OUTOFBOUNDS; // starts empty because out of bounds is empty for how hints work.
    int8_t blockNumber      = 0;           //
    if (isRow)
    {
        for (int i = 0; i < p->puzzle->width; i++)
        {
            if (source[i][index] == OUTOFBOUNDS)
            {
                continue;
            }

            if (source[i][index] == SPACE_FILLED) // we have a clue.
            {
                // Counting up the hint size.
                if (prev == SPACE_FILLED)
                {
                    t.hints[blockNumber]++;
                }
                else if (prev == SPACE_EMPTY) // this is a new block, and not the first block.
                {
                    blockNumber++; // next block! (but we go left to right, which is the clue right to left)
                    t.hints[blockNumber]++;
                }
                else if (prev == OUTOFBOUNDS) // first block in sequence.
                {
                    // same as above but dont increase blocknumber.
                    t.hints[blockNumber]++;
                }
                prev = SPACE_FILLED;
            }
            else // if the space is empty
            {
                prev = SPACE_EMPTY;
                // i continues to increase.
            }
        }
        // Store the maximum number of actual visible hints, used for calculating display position.
        if (blockNumber > p->maxHintsX)
        {
            p->maxHintsX = blockNumber;
        }
    }
    else
    {
        // same logic but for columns
        for (int i = 0; i < p->puzzle->height; i++)
        {
            if (source[index][i] == OUTOFBOUNDS)
            {
                continue;
            }

            if (source[index][i] == SPACE_FILLED)
            {
                // Counting up the block size.
                if (prev == SPACE_FILLED)
                {
                    t.hints[blockNumber]++;
                }
                else if (prev == SPACE_EMPTY) // this is a new block, and not the first block.
                {
                    blockNumber++;
                    t.hints[blockNumber]++;
                }
                else if (prev == OUTOFBOUNDS)
                {
                    t.hints[blockNumber]++;
                }
                prev = SPACE_FILLED;
            }
            else // if empty
            {
                prev = SPACE_EMPTY;
                // i continues to increase.
            }
        }
        // Store the maximum number of actual visible hints, used for calculating display position.
        if (blockNumber > p->maxHintsY)
        {
            p->maxHintsY = blockNumber;
        }
    }
    return t;
}

void picrossGameLoop(int64_t elapsedUs)
{
    p->elapsedUs = elapsedUs;
    p->loopingTimer += elapsedUs >> 12;
    p->bgScrollTimer += elapsedUs;

    // We do this at the top of the loop for 2 reasons. THe first is so that changedLevelThisFrame doesn't get reset, so
    // when we set it from loading (true) or new (false), it updates. the second is so that when you hit the puzzle on a
    // victory, the check might be a little slower, and we want to draw the last filled in block and let that flash on
    // screen for a frame before jumping to the slv which is pretty minor but feels important. frankly, in a full game
    // we would't pop instantly but hide it all under a smooth celebration animation and sound before revealing the
    // puzzle. May explore that.

    // userInput sets this value when we change a block to or from filled.
    if (p->input->changedLevelThisFrame)
    {
        picrossCheckLevel();
    }

    picrossUserInput(elapsedUs);
    if (p->exitThisFrame)
    {
        setDrawBody(p->renderer, true); // make the menu renderer default again with the background body drawn.
        picrossExitGame();             // free variables
        returnToPicrossMenuFromGame(); // change to menu
        // dont do more processing, we have switched back to level select screen.
        return;
    }

    if (p->input->showGuides)
    {
        picrossCalculateHoverHint();
    }

    drawPicrossScene();

    // You won! Only called once, since you cant go from win->solving without resetting everything (ie: menu and back)
    if (p->previousPhase == PICROSS_SOLVING && p->currentPhase == PICROSS_YOUAREWIN)
    {
        // TODO MIDI music
        // buzzer_stop();
        // buzzer_play_bgm(&picross_music_win);

        // Unsave progress. Hides "current" in the main menu. we dont need to zero-out the actual data that will just
        // happen when we load a new level.
        writeNvs32(picrossCurrentPuzzleIndexKey, -1);

        // save the fact that we won
        saveCompletedOnSelectedLevel(true);
    }

    // Victory animation
    if (p->currentPhase == PICROSS_YOUAREWIN)
    {
        picrossVictoryLEDs(elapsedUs, 20000, false);
    }

    p->previousPhase = p->currentPhase;
}

void picrossCheckLevel()
{
    // Update if the puzzle is filled in, which we use to grey-out hints when drawing them.
    // if performance is bad, lets just cut this feature.
    for (int i = 0; i < p->puzzle->height; i++)
    {
        hintIsFilledIn(&p->puzzle->rowHints[i]);
    }
    for (int i = 0; i < p->puzzle->width; i++)
    {
        hintIsFilledIn(&p->puzzle->colHints[i]);
    }

    // we only need to do this if the puzzle is completely filled in, and we already checked that above.

    // check if the puzzle is correctly completed.
    for (int c = 0; c < p->puzzle->width; c++)
    {
        for (int r = 0; r < p->puzzle->height; r++) // flipped
        {
            if (p->puzzle->level[c][r] == SPACE_EMPTY || p->puzzle->level[c][r] == SPACE_MARKEMPTY)
            {
                if (p->puzzle->completeLevel[c][r] != SPACE_EMPTY)
                {
                    return;
                }
            }
            else // if space == SPACE_FILLED (we aren't using filled-hints so lets not bother specific if for now)
            {
                if (p->puzzle->completeLevel[c][r] != SPACE_FILLED)
                {
                    return;
                }
            }
        }
    }

    // Check level by clues.

    // We use this if we can't ensure there are unique solutions to the clues.
    // because if you got ANY solution, you should win.

    // but its solwer than just comparing numbers in the grid.
    //  picrossHint_t testingHint;
    //  for(int i = 0;i<p->puzzle->width;i++)
    //  {
    //      //rows
    //      testingHint = newHintFromPuzzle(i,true,p->puzzle->level);
    //      if(!hintsMatch(testingHint,p->puzzle->rowHints[i])){
    //          return;
    //      }
    //  }
    //  for(int i = 0;i<p->puzzle->height;i++)
    //  {
    //      //cols
    //      //rows
    //      testingHint = newHintFromPuzzle(i,false,p->puzzle->level);
    //      if(!hintsMatch(testingHint,p->puzzle->colHints[i])){
    //          return;
    //      }
    //  }

    p->currentPhase    = PICROSS_YOUAREWIN;
    p->controlsEnabled = false;
}

// bool hintsMatch(picrossHint_t a, picrossHint_t b)
// {
//     for(int i = 0;i<PICROSS_MAX_HINTCOUNT;i++)
//     {
//         if(a.hints[i] != b.hints[i])
//         {
//             return false;
//         }
//     }
//     return true;
// }
bool hintIsFilledIn(picrossHint_t* hint)
{
    uint8_t segmentIndex                          = 0;
    uint8_t segmentLengths[PICROSS_MAX_HINTCOUNT] = {0};
    picrossSpaceType_t lastSpace                  = OUTOFBOUNDS;

    uint8_t colInc = hint->isRow ? 1 : 0;
    uint8_t rowInc = hint->isRow ? 0 : 1;
    uint8_t row    = (hint->isRow) ? hint->index : 0;
    uint8_t col    = (hint->isRow) ? 0 : hint->index;

    bool isFilledIn = true;
    for (; row < p->puzzle->height && col < p->puzzle->width; row += rowInc, col += colInc)
    {
        switch (p->puzzle->level[col][row])
        {
            case SPACE_EMPTY:
                isFilledIn
                    = false; // we could just do this line and fall through the switch, but the compiler didn't like it.
                if (lastSpace == SPACE_FILLED)
                {
                    segmentIndex++;
                }
                lastSpace = SPACE_EMPTY;
                break;
            case SPACE_MARKEMPTY:
            case OUTOFBOUNDS:
            {
                if (lastSpace == SPACE_FILLED)
                {
                    segmentIndex++;
                }
                lastSpace = SPACE_EMPTY;
            }
            break;

            case SPACE_FILLED:
            {
                segmentLengths[segmentIndex]++;
                lastSpace = SPACE_FILLED;
            }
            break;
        }
    }
    hint->filledIn = isFilledIn;

    uint8_t skippedHints = 0;
    for (uint8_t hintIndex = 0; hintIndex < PICROSS_MAX_HINTCOUNT; hintIndex++)
    {
        // if the first hint is 0, we need to skip it so the segments match up
        // but only skip the first one, otherwise we would mark it as filled
        // even if there are extra segments after the ones that match the hints
        if (hint->hints[hintIndex] == 0 && skippedHints == 0)
        {
            skippedHints++;
            continue;
        }

        if (segmentLengths[hintIndex - skippedHints] != hint->hints[hintIndex])
        {
            hint->correct = false;
            return false;
        }
    }
    hint->correct = true;
    return true;
}

//==============================================
// CONTROLS & INPUT
//==============================================

void picrossCalculateHoverHint()
{
    // if ShowGuides is off, skip.

    // Determines the size of the blocks the player is hovering over.
    p->input->hoverBlockSizeX = 0;
    p->input->hoverBlockSizeY = 0;
    picrossSpaceType_t space;

    // get horizontal hit.

    // from current to the right
    for (int x = p->input->x; x < p->puzzle->width; x++)
    {
        space = p->puzzle->level[x][p->input->y];
        if (space == SPACE_FILLED)
        {
            p->input->hoverBlockSizeX++;
            continue;
        }
        else
        {
            break;
        }
    }
    // from zero to left if the block under us is filled (ie: hint is at least 1)
    if (p->input->hoverBlockSizeX >= 1)
    {
        for (int x = p->input->x - 1; x >= 0; x--)
        {
            space = p->puzzle->level[x][p->input->y];
            if (space == SPACE_FILLED)
            {
                p->input->hoverBlockSizeX++;
                continue;
            }
            else
            {
                break;
            }
        }
    }
    // from current to top
    for (int y = p->input->y; y < p->puzzle->height; y++)
    {
        space = p->puzzle->level[p->input->x][y];
        if (space == SPACE_FILLED)
        {
            p->input->hoverBlockSizeY++;
            continue;
        }
        else
        {
            break;
        }
    }
    // from current-1 to bottom, but only if there is a block under the input box.
    if (p->input->hoverBlockSizeY >= 1)
    {
        for (int y = p->input->y - 1; y >= 0; y--)
        {
            space = p->puzzle->level[p->input->x][y];
            if (space == SPACE_FILLED)
            {
                p->input->hoverBlockSizeY++;
                continue;
            }
            else
            {
                break;
            }
        }
    }
}

void picrossUserInput(int64_t elapsedUs)
{
    picrossInput_t* input      = p->input;
    uint8_t x                  = input->x;
    uint8_t y                  = input->y;
    picrossSpaceType_t current = p->puzzle->level[x][y];

    // reset
    input->movedThisFrame        = false;
    input->changedLevelThisFrame = false; // do we need to check for solution?

    // check for "exit" button before we escape when not in solving mode.
    // todo: how should exit work? A button when game is solved?
    if (input->btnState & PB_START && !(input->prevBtnState & PB_START)
        && !(input->btnState
             & PB_A)) // if we are holding a down when we leave, we instantly select a level on the select screen.
    {
        // dont bother saving if we are leaving victory screen.
        if (p->currentPhase == PICROSS_SOLVING)
        {
            savePicrossProgress();
        }
        // returnToLevelSelect();//im on the fence about how this should behave. to level or main.
        // returnToPicrossMenu();//now that it hovers over continue, i think main is better.
        p->exitThisFrame = true; // stops drawing to the screen, stops messing with variables, frees memory.
        return;
    }

    // victory screen input
    if (p->controlsEnabled == false || p->currentPhase == PICROSS_YOUAREWIN)
    {
        //&& !(p->input->prevBtnState & PB_B)
        if (p->input->btnState & PB_B && !(p->input->prevBtnState & PB_B))
        {
            // return to level select instead of main menu?
            p->exitThisFrame = true;
        }
        if (p->input->btnState & PB_LEFT)
        {
            p->marqueeScrollX -= elapsedUs * 8;
        }
        if (p->input->btnState & PB_RIGHT)
        {
            p->marqueeScrollX += elapsedUs * 3;
        }

        p->input->prevBtnState = p->input->btnState;
        return;
    }
    // Input checks

    // DAS
    // todo: we can cache the != checks, so we do 1 butmask instead of 3 (x4)

    if (input->btnState & PB_UP
        && !((input->btnState & PB_RIGHT) || (input->btnState & PB_LEFT) || (input->btnState & PB_DOWN)))
    {
        if (p->input->holdingDir == PICROSSDIR_UP)
        {
            p->input->timeHeldDirection = p->input->timeHeldDirection + elapsedUs;
        }
        else
        {
            p->input->timeHeldDirection = 0;
            p->input->holdingDir        = PICROSSDIR_UP;
            p->input->DASActive         = false;
        }
    }
    else if (input->btnState & PB_RIGHT
             && !((input->btnState & PB_DOWN) || (input->btnState & PB_UP) || (input->btnState & PB_LEFT)))
    {
        if (p->input->holdingDir == PICROSSDIR_RIGHT)
        {
            p->input->timeHeldDirection = p->input->timeHeldDirection + elapsedUs;
        }
        else
        {
            p->input->timeHeldDirection = 0;
            p->input->holdingDir        = PICROSSDIR_RIGHT;
            p->input->DASActive         = false;
        }
    }
    else if (input->btnState & PB_LEFT
             && !((input->btnState & PB_DOWN) || (input->btnState & PB_UP) || (input->btnState & PB_RIGHT)))
    {
        if (p->input->holdingDir == PICROSSDIR_LEFT)
        {
            p->input->timeHeldDirection = p->input->timeHeldDirection + elapsedUs;
        }
        else
        {
            p->input->timeHeldDirection = 0;
            p->input->holdingDir        = PICROSSDIR_LEFT;
            p->input->DASActive         = false; // active not true till second.
        }
    }
    else if (input->btnState & PB_DOWN
             && !((input->btnState & PB_LEFT) || (input->btnState & PB_RIGHT) || (input->btnState & PB_UP)))
    {
        if (p->input->holdingDir == PICROSSDIR_DOWN)
        {
            p->input->timeHeldDirection = p->input->timeHeldDirection + elapsedUs;
        }
        else
        {
            p->input->timeHeldDirection = 0;
            p->input->holdingDir        = PICROSSDIR_DOWN;
            p->input->DASActive         = false;
        }
    }
    else
    {
        // 0 or more than one button
        p->input->timeHeldDirection = 0;
        p->input->holdingDir        = PICROSSDIR_IDLE;
        p->input->DASActive         = false;
    } // now that checks have been done.

    // if we have been doing DAS and are faster than its the autospeed, OR (das is inactive and) we have held longer
    // than the longer time.
    if ((p->input->DASActive && p->input->timeHeldDirection > p->input->DASTime)
        || (!p->input->DASActive && p->input->timeHeldDirection > p->input->firstDASTime))
    {
        // all buts except l/r/u/d so we dont mess with start/select in the das
        p->input->prevBtnState      = p->input->prevBtnState & ~(PB_LEFT | PB_RIGHT | PB_UP | PB_DOWN);
        p->input->timeHeldDirection = 0;
        p->input->DASActive         = true;
    }

    if (input->btnState & PB_A && !(input->prevBtnState & PB_A))
    {
        // Button has no use here
        // toggle Selection

        // not doing clever cast to int and increment, because we have HINT in the enum.
        // not worrying about hints until... later.
        if (current != SPACE_FILLED)
        {
            current                      = SPACE_FILLED;
            input->startHeldType         = SPACE_FILLED;
            input->changedLevelThisFrame = true;
        }
        else
        {
            current                      = SPACE_EMPTY;
            input->startHeldType         = SPACE_EMPTY;
            input->changedLevelThisFrame = true;
        }
        // set the toggle.
        //  p->puzzle->level[x][y] = current;
        enterSpace(x, y, current);
    }

    if (input->btnState & PB_B && !(input->prevBtnState & PB_B))
    {
        // Button has no use here
        // toggle Hints
        // not doing clever cast to int and increment, because we have HINT in the enum.
        // not worrying about hints until... later.
        if (current != SPACE_MARKEMPTY)
        {
            current                      = SPACE_MARKEMPTY;
            input->startHeldType         = SPACE_MARKEMPTY;
            input->changedLevelThisFrame = true; // shouldnt be needed
        }
        else
        {
            current                      = SPACE_EMPTY;
            input->startHeldType         = SPACE_EMPTY;
            input->changedLevelThisFrame = true; // shouldnt be needed
        }
        // set the toggle.
        //  p->puzzle->level[x][y] = current;
        enterSpace(x, y, current);
    }

    if (input->btnState & PB_UP && !(input->prevBtnState & PB_UP))
    {
        if (input->y == 0)
        {
            if (!input->DASActive)
            {
                // wrap to opposite position
                input->y = p->puzzle->height - 1;
                p->count
                    = 1; // reset counter. Its not helpful to just keep increasing or decreasing, although it is funny.
                input->movedThisFrame = true;
                countInput(PICROSSDIR_UP);
            }
        }
        else
        {
            input->y              = y - 1;
            input->movedThisFrame = true;
            countInput(PICROSSDIR_UP);
        }
    }
    else if (input->btnState & PB_DOWN && !(input->prevBtnState & PB_DOWN))
    {
        if (input->y == p->puzzle->height - 1)
        {
            if (!input->DASActive)
            {
                // wrap to bottom position
                input->y              = 0;
                p->count              = 1;
                input->movedThisFrame = true;
                countInput(PICROSSDIR_DOWN);
            }
        }
        else
        {
            input->y              = input->y + 1;
            input->movedThisFrame = true;
            countInput(PICROSSDIR_DOWN);
        }
    }
    else if (input->btnState & PB_LEFT && !(input->prevBtnState & PB_LEFT))
    {
        if (input->x == 0)
        {
            if (!input->DASActive)
            {
                // wrap to right position
                input->x              = p->puzzle->width - 1;
                p->count              = 1;
                input->movedThisFrame = true;
                countInput(PICROSSDIR_LEFT);
            }
        }
        else
        {
            input->x              = input->x - 1;
            input->movedThisFrame = true;
            countInput(PICROSSDIR_LEFT);
        }
    }
    else if (input->btnState & PB_RIGHT && !(input->prevBtnState & PB_RIGHT))
    {
        if (input->x == p->puzzle->width - 1)
        {
            if (!input->DASActive)
            {
                // wrap to left position
                input->x              = 0;
                p->count              = 1;
                input->movedThisFrame = true;
                countInput(PICROSSDIR_RIGHT);
            }
        }
        else
        {
            input->x              = input->x + 1;
            input->movedThisFrame = true;
            countInput(PICROSSDIR_RIGHT);
        }
    }

    if (input->touchState && !(input->prevTouchState))
    {
        input->startTentativeMarkType = toggleTentativeMark(input->x, input->y);
    }

    // check for holding input while using d-pad
    if (input->movedThisFrame)
    {
        // todo: toggle.
        if (input->btnState & PB_A)
        {
            if (input->startHeldType == SPACE_FILLED)
            {
                enterSpace(input->x, input->y, SPACE_FILLED);
            }
            else if (input->startHeldType == SPACE_EMPTY)
            {
                enterSpace(input->x, input->y, SPACE_EMPTY);
            }
        }
        else if (input->btnState & PB_B)
        {
            if (input->startHeldType == SPACE_MARKEMPTY)
            {
                enterSpace(input->x, input->y, SPACE_MARKEMPTY);
            }
            else if (input->startHeldType == SPACE_EMPTY)
            {
                enterSpace(input->x, input->y, SPACE_EMPTY);
            }
        }
        else if (input->touchState)
        {
            setTentativeMark(input->x, input->y, input->startTentativeMarkType);
        }
    }

    // lastly, set prevBtnState to btnState
    input->prevBtnState   = input->btnState;
    input->prevTouchState = input->touchState;

    // doing prevBtn set below the blink code below breaks it?

    // blink timer.
    // if SHOW HINTS.
    if (input->blinkError)
    {
        uint64_t totalTime    = input->blinkTime * (input->blinkCount);
        input->blinkAnimTimer = input->blinkAnimTimer + elapsedUs;
        if (input->blinkAnimTimer > totalTime)
        {
            // finished!
            input->blinkError     = false; // will have to get set back to true elsewhere.
            input->blinkAnimTimer = 0;

            // reset default values
            input->inputBoxColor = input->inputBoxDefaultColor;
            setLeds(p->offLEDS, CONFIG_NUM_LEDS);
        }
        else
        {
            // check how many times blinkTime devides into remaining time.
            int times = (totalTime - input->blinkAnimTimer) / input->blinkTime;
            // remainder is still in microseconds
            // lets set it to something else.

            if (times % 2 == 0)
            {
                // turn off the LEDs
                input->inputBoxColor = input->inputBoxDefaultColor;
                setLeds(p->errorALEDBlinkLEDS, CONFIG_NUM_LEDS);
            }
            else
            {
                // turn on the LEDs
                setLeds(p->errorBLEDBlinkLEDS, CONFIG_NUM_LEDS);
                input->inputBoxColor = input->inputBoxErrorColor;
            }
        }
    }
}

// Toggles the tentative mark at the given slot
// Returns whether to add or remove marks if button is held + input moves
bool toggleTentativeMark(uint8_t x, uint8_t y)
{
    return setTentativeMark(x, y, !p->tentativeMarks[x][y]);
}

// Will set mark to provided value, unless the space is filled by something else
// Returns whether to add or remove marks for button held action
bool setTentativeMark(uint8_t x, uint8_t y, bool mark)
{
    if (p->puzzle->level[x][y] == SPACE_EMPTY)
    {
        p->tentativeMarks[x][y] = mark;
        return p->tentativeMarks[x][y];
    }

    // If marking a space that cannot be marked, I think that it makes the most sense to
    // return true, such that if you hold touch and move, empty spaces would then be marked
    return true;
}

void enterSpace(uint8_t x, uint8_t y, picrossSpaceType_t newSpace)
{
    // Clear tentative marks if newSpace is filled/marked empty
    if (newSpace != SPACE_EMPTY)
    {
        p->tentativeMarks[x][y] = false;
    }
    // this gets called frequently even if newSpace is current.
    if (p->puzzle->level[x][y] != newSpace)
    {
        p->puzzle->level[x][y] = newSpace;
        // if SHOW HINTS is active
        // showing hints for the "mark as empty" felt wrong. its a note, not a commitment, and you can use it to
        // remember locations too.
        if (newSpace == SPACE_FILLED && newSpace != p->puzzle->completeLevel[x][y])
        {
            if (p->input->showHints)
            {
                p->input->blinkError = true;
            }
        }
        // force validation check
        p->input->changedLevelThisFrame = true;
    }
}

void countInput(picrossDir_t input)
{
    // I need to work out how this should properly behave.
    // in my game, you can just hold shift to make a counter with a nice tooltip and hoverbox, and move it around. its
    // great. but we dont have enough inputs here. Maybe you have to activate the counter with start?
    if (input != p->countState)
    {
        if (p->count == 1)
        {
            p->count = 2;
        }
        else
        {
            p->count = 1;
        }
    }
    else
    {
        p->count++;
    }
    p->countState = input;
}

//==========================================================
/// DRAWING SCENE
//========================================================

/**
 * @brief Master draw function for picross gameplay. Draws the board and calls other picross draw functions.
 *
 */
void drawPicrossScene(void)
{
    uint8_t w = p->puzzle->width;
    uint8_t h = p->puzzle->height;

    clearPxTft();
    drawBackground();

    box_t box;
    box_t xBox;

    // todo: move color selection to constants somewhere
    paletteColor_t emptySpaceCol = c555;
    paletteColor_t hoverSpaceCol = emptySpaceCol; // this value is only different if guides are turned on.
    if (p->input->showGuides)
    {
        // I wish I could tint this less than I can. What if, instead, we tint the border-lines surrounding the spaces,
        // and not the spaces?
        hoverSpaceCol = c445;
    }

    if (p->currentPhase == PICROSS_SOLVING)
    {
        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                // shape of boxDraw
                box = boxFromCoord(i, j);

                switch (p->puzzle->level[i][j])
                {
                    case SPACE_EMPTY:
                    {
                        // slightly tint rows or columns
                        // todo: move this logic around to make code more efficient?
                        if (!(p->input->x == i)
                            != !(p->input->y == j)) // this is a strange boolean logic hack to do (x==i XOR y==j)
                        {
                            drawBox(box, hoverSpaceCol, true, 0);
                        }
                        else
                        {
                            drawBox(box, emptySpaceCol, true, 0);
                        }
                        break;
                    }
                    case SPACE_FILLED:
                    {
                        drawBox(box, c000, true, 0);
                        break;
                    }
                    case SPACE_MARKEMPTY:
                    {
                        // draw x again
                        //  plotCircle(d,)
                        if (!p->markX)
                        {
                            drawBox(box, c531, true, 0);
                        }
                        else
                        {
                            uint16_t xThick = p->drawScale / 5;
                            xThick          = xThick < 3 ? 3 : xThick; // min 2
                            uint16_t shrink = xThick < 3 ? 3 : xThick; // min 3
                            xBox            = box;
                            xBox.x0         = xBox.x0 + shrink;
                            xBox.x1         = xBox.x1 - shrink;
                            xBox.y0         = xBox.y0 + shrink;
                            xBox.y1         = xBox.y1 - shrink;

                            // c531
                            drawBox(box, c555, true, 0);

                            // corner to corner X
                            drawLine(xBox.x0, xBox.y1, xBox.x1, xBox.y0, p->input->markXColor, 0);
                            drawLine(xBox.x0, xBox.y0, xBox.x1, xBox.y1, p->input->markXColor, 0);

                            for (int t = 1; t < (xThick + 1) / 2; t++)
                            {
                                // bottom left to top right
                                drawLine(xBox.x0, xBox.y1 - t, xBox.x1 - t, xBox.y0, p->input->markXColor, 0);
                                drawLine(xBox.x0 + t, xBox.y1, xBox.x1, xBox.y0 + t, p->input->markXColor, 0);

                                // top left to bottom right
                                drawLine(xBox.x0 + t, xBox.y0, xBox.x1, xBox.y1 - t, p->input->markXColor, 0);
                                drawLine(xBox.x0, xBox.y0 + t, xBox.x1 - t, xBox.y1, p->input->markXColor, 0);
                            }
                        }
                        break;
                    }
                    case OUTOFBOUNDS:
                    {
                        // uh oh!
                        break;
                    }
                }

                if (p->tentativeMarks[i][j] == true)
                {
                    // Following convention of how X marks always use emptySpaceCol instead of hoverSpaceCol
                    int boxSize   = box.x1 - box.x0;
                    int lineWidth = (boxSize / 5);
                    if (boxSize < 12)
                    {
                        lineWidth++;
                    }
                    int outerPadding      = (boxSize / 4);
                    box_t innerBox        = {.x0 = box.x0 + outerPadding,
                                             .y0 = box.y0 + outerPadding,
                                             .x1 = box.x1 - outerPadding + 1,
                                             .y1 = box.y1 - outerPadding + 1};
                    box_t innerWhitespace = {.x0 = innerBox.x0 + lineWidth,
                                             .y0 = innerBox.y0 + lineWidth,
                                             .x1 = innerBox.x1 - lineWidth,
                                             .y1 = innerBox.y1 - lineWidth};

                    drawBox(box, emptySpaceCol, true, 0);
                    drawBox(innerBox, c314, true, 0);
                    drawBox(innerWhitespace, emptySpaceCol, true, 0);
                }
            }
        }
        drawPicrossHud(&p->hintFont);
        drawPicrossInput();
    } // end if phase is solving
    else if (p->currentPhase == PICROSS_YOUAREWIN)
    {
        p->lerpAmount += p->elapsedUs >> 9;
        if (p->lerpAmount > PICROSS_LERP_AMOUNT)
        {
            p->lerpAmount = PICROSS_LERP_AMOUNT; // cap lerp amount
        }
        int ox = lerp(0, p->offsetX, p->lerpAmount);
        int oy = lerp(0, p->offsetY, p->lerpAmount);

        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                if (p->lerpAmount >= PICROSS_LERP_AMOUNT)
                {
                    drawSinglePixelFromWSG(i, j, &p->selectedLevel.completedWSG, ox, oy);
                }
                else
                {
                    drawSinglePixelFromWSG(i, j, &p->selectedLevel.levelWSG, ox, oy);
                }
            }
        }

        // Draw the title of the puzzle, centered.
        if(p->animateBG == PICROSS_BG_HEXAGONS)
        {
            drawRectFilled(0, 7, TFT_WIDTH, 33, c001);
        }
        int16_t t = textWidth(&p->UIFont, p->selectedLevel.title);
        t         = ((TFT_WIDTH)-t) / 2; // from text width into padding.
        drawText(&p->UIFont, c555, p->selectedLevel.title, t, 14);

        // Draw the marquee fact.
        if (p->lerpAmount == PICROSS_LERP_AMOUNT)
        {
            if(p->animateBG == PICROSS_BG_HEXAGONS)
            {
                drawRectFilled(0, 190, TFT_WIDTH, 225, c001);
            }
            if (p->loopingTimer > 0)
            {
                drawText(&p->UIFont, c555, "<", 12, 200);
                drawText(&p->UIFont, c555, ">", 258, 200);
            }
            // Slow auto scroll.
            p->marqueeScrollX += p->elapsedUs * 3;
            drawTextMarquee(&p->UIFont, c555, p->selectedLevel.marqueeFact, 27, 200, TFT_WIDTH - 30,
                            &p->marqueeScrollX);
        }
    }
}

void drawSinglePixelFromWSG(int x, int y, wsg_t* image, int offsetX, int offsetY)
{
    box_t box = boxFromCoord(x, y);
    box.x0 += offsetX;
    box.y0 += offsetY;
    box.x1 += offsetX;
    box.y1 += offsetY;
    paletteColor_t v = image->px[(y * p->puzzle->width) + x];
    drawBox(box, v, true, 0);
}

//*
// * @brief Creates a box_t struct given the x/y coordinates. I haven't gotten to totally standardizing the coordinate
// system for picross yet, but if we are trying to adjust where on screen something goes, this function determines
// _most_ of it.
// *
// * @param x
// * @param y
// * @return box_t
// */
box_t boxFromCoord(int8_t x, int8_t y)
{
    uint16_t s = p->drawScale;
    box_t box  = {
         .x0 = (x * s) + s + p->leftPad,
         .y0 = (y * s) + s + p->topPad,
         .x1 = (x * s) + s + s + p->leftPad,
         .y1 = (y * s) + s + s + p->topPad,
    };
    return box;
}

void drawPicrossHud(font_t* font)
{
    uint8_t s = p->drawScale;
    // draw a vertical line every grid
    for (int i = 0; i <= p->puzzle->width; i++) // skip 0 and skip last. literally the fence post problem.
    {
        if (i == 0 || i == p->puzzle->width)
        {
            // draw border
            drawLine(((i * s) + s + p->leftPad), (s + p->topPad), ((i * s) + s + p->leftPad),
                     ((p->puzzle->height * s) + s + p->topPad), PICROSS_BORDER_COLOR, 0); // BORDER COLOR
            continue;
        }

        // this could be less confusing.
        drawLine(((i * s) + s + p->leftPad), (s + p->topPad), ((i * s) + s + p->leftPad),
                 ((p->puzzle->height * s) + s + p->topPad), c111, 0);
    }

    // draw a horizontal line every 5 units.
    // todo: also do the full grid.
    for (uint8_t i = 0; i <= p->puzzle->height; i++)
    {
        if (i == 0 || i == p->puzzle->height)
        {
            // draw border
            drawLine((s + p->leftPad) + 1, ((i * s) + s + p->topPad), ((p->puzzle->width * s) + s + p->leftPad) - 1,
                     ((i * s) + s + p->topPad), PICROSS_BORDER_COLOR, 0);
            continue;
        }

        // this should be less confusing.
        drawLine((s + p->leftPad) + 1, ((i * s) + s + p->topPad), ((p->puzzle->width * s) + s + p->leftPad) - 1,
                 ((i * s) + s + p->topPad), c111, 0);
    }

    // After drawing the other grid, draw the thicker yellow lines, so they overlap the others.
    for (int i = 5; i <= p->puzzle->height - 1; i += 5)
    {
        // Horizontal
        drawLine(s + p->leftPad + 1, (i * s) + s + p->topPad, (p->puzzle->width * s) + s + p->leftPad - 1,
                 (i * s) + s + p->topPad, PICROSS_MOD5_COLOR, 0);
        // drawLine(d,
        //     s + p->leftPad+1,
        //     (i * s) + s + p->topPad+1,
        //     (p->puzzle->width * s) + s + p->leftPad-1,
        //     (i * s) + s + p->topPad+1,
        //     PICROSS_MOD5_COLOR,0
        // );
        // drawLine(d,
        //     s + p->leftPad+1,
        //     (i * s) + s + p->topPad-1,
        //     (p->puzzle->width * s) + s + p->leftPad-1,
        //     (i * s) + s + p->topPad-1,
        //     PICROSS_MOD5_COLOR,0
        // );
    }
    for (int i = 5; i <= p->puzzle->width - 1; i += 5)
    {
        // Vertical
        drawLine((i * s) + s + p->leftPad, s + p->topPad + 1, (i * s) + s + p->leftPad,
                 (p->puzzle->height * s) + s + p->topPad - 1, PICROSS_MOD5_COLOR, 0);
        // drawLine(d,
        //     (i * s) + s + p->leftPad+1,
        //     s + p->topPad+1,
        //     (i * s) + s + p->leftPad+1,
        //     (p->puzzle->height * s) + s + p->topPad-1,
        //     PICROSS_MOD5_COLOR,0
        // );
        // drawLine(d,
        //     (i * s) + s + p->leftPad-1,
        //     s + p->topPad+1,
        //     (i * s) + s + p->leftPad-1,
        //     (p->puzzle->height * s) + s + p->topPad-1,
        //     PICROSS_MOD5_COLOR,0
        // );
    }

    // Draw the hints.
    // With square puzzles, we don't need two for loops. Perhaps this is... aspirational.
    for (uint8_t i = 0; i < p->puzzle->height; i++)
    {
        drawHint(font, p->puzzle->rowHints[i]);
    }
    for (uint8_t i = 0; i < p->puzzle->width; i++)
    {
        drawHint(font, p->puzzle->colHints[i]);
    }

    if (p->input->showGuides)
    {
        // Draw hover guide on right side.
        if (p->input->hoverBlockSizeX != 0)
        {
            drawNumberAtCoord(font, c445, p->input->hoverBlockSizeX, p->puzzle->width, p->input->y, 3, 0);
        }
        // draw hover guide on bottom
        if (p->input->hoverBlockSizeY != 0)
        {
            drawNumberAtCoord(font, c445, p->input->hoverBlockSizeY, p->input->x, p->puzzle->height, 0, 3);
        }
    }

    // Draw the UI last, so it goes above the guidelines (which are done in drawHint).
    // This means they COULD cover hints, since we don't really check for that procedurally. So keep an eye out when
    // making puzzles.

    // Draw current coordinates
    char textBuffer[9];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "%d,%d", p->input->x + 1, p->input->y + 1);
    drawText(&(p->UIFont), c555, textBuffer, 30, 25);

    // Draw counter
    snprintf(textBuffer, sizeof(textBuffer) - 1, "%d", p->count);
    drawText(&(p->UIFont), c334, textBuffer, 30, 25 + p->UIFont.height * 2);
}

void drawHint(font_t* font, picrossHint_t hint)
{
    uint8_t h;
    paletteColor_t hintShadeColor = c001; // todo: move to struct if we decide to keep this.
    paletteColor_t hintColor      = c555; // white/
    if (p->fadeHints)
    {
        if (hint.correct)
        {
            // fade, or fade more.
            hintColor = hint.filledIn ? c111 : c333;
        }
        else
        {
            // incorrect, or we are still working on it
            hintColor = hint.filledIn ? c533 : c555;
        }
    }

    if (hint.isRow)
    {
        uint8_t j = 0;
        // if current row, draw background squares.
        if (p->input->showGuides && hint.index == p->input->y)
        {
            box_t hintbox = boxFromCoord(-1, hint.index);
            hintbox.x0    = (p->animateBG == PICROSS_BG_HEXAGONS) * 26;
            drawBox(hintbox, hintShadeColor, true, 0);
        }

        // draw clues if they... are a thing.
        // todo: do the math on the max number of hints that _this_ level has, and just use that for all the math.
        for (uint8_t i = 0; i < PICROSS_MAX_HINTCOUNT; i++)
        {
            h = hint.hints[PICROSS_MAX_HINTCOUNT - 1 - i];
            if (h != 0)
            {
                // dont increase j.
                drawNumberAtCoord(font, hintColor, h, -j - 1, hint.index, 0, 0); // xOff: -(p->clueGap)*j
                j++;
            }
        }

        if (j == 0)
        {
            drawNumberAtCoord(font, hintColor, 0, -1, hint.index, 0, 0);
        }
    }
    else
    {
        uint8_t j = 0;
        // if current col, draw background square
        if (p->input->showGuides && hint.index == p->input->x)
        {
            box_t hintbox = boxFromCoord(hint.index, -1);
            hintbox.y0    = (p->animateBG == PICROSS_BG_HEXAGONS) * 21;
            drawBox(hintbox, hintShadeColor, true, 0);
        }

        // draw the line of hints
        for (uint8_t i = 0; i < PICROSS_MAX_HINTCOUNT; i++)
        {
            h = hint.hints[PICROSS_MAX_HINTCOUNT - 1 - i];
            if (h != 0)
            {
                drawNumberAtCoord(font, hintColor, h, hint.index, -j - 1, 0, 0);
                j++;
            }
        }

        if (j == 0)
        {
            drawNumberAtCoord(font, hintColor, 0, hint.index, -1, 0, 0);
        }
    }
}
/**
 * @brief Draws a number at a coordinate position. This will draw the number 0, although we never draw it in practice.
 * On principle it felt wrong to write a function "drawNumber" that can't write "0".
 *
 * @param d display
 * @param font font
 * @param color color
 * @param number The number to print to the screen (<= 2 digits)
 * @param x //x coordinate to draw at. top left of board is 0,0; hints are in a negative direction.
 * @param y //y cooridnate to draw at
 * @param xOff //additional x offset in pixels
 * @param yOff //additional y offset in pixels
 */
void drawNumberAtCoord(font_t* font, paletteColor_t color, uint8_t number, int8_t x, int8_t y, int16_t xOff,
                       int16_t yOff)
{
    box_t box = boxFromCoord(x, y);
    if (number < 10)
    {
        char letter[4];
        sprintf(letter, "%d", number);
        drawChar(color, font->height, &font->chars[(*letter) - ' '], getHintShift(number) + box.x0 + xOff,
                 ((int16_t)box.y0 + p->vFontPad) + yOff);
    }
    else
    { // double digit draws

        char letter[4];
        sprintf(letter, "%d", number);
        // as a "temporary" workaround, we will use x1 and y1 and subtract the drawscale.
        drawChar(color, font->height, &font->chars[(*letter) - ' '], getHintShift(number) + box.x0 + xOff,
                 box.y0 + p->vFontPad + yOff);
        sprintf(letter, "%d", number % 10);
        drawChar(color, font->height, &font->chars[(*letter) - ' '],
                 getHintShift(number) + box.x0 + p->drawScale / 2 + xOff, box.y0 + p->vFontPad + yOff);
    }
}

int8_t getHintShift(uint8_t hint)
{
    // todo: we have three fonts (drawScale 0-12, 12-24, 24+). Offsets need to be calibrated for each one.
    switch (hint)
    {
        case 0:
            return p->vFontPad + 2;
        case 1:
            return p->vFontPad + 2;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return p->vFontPad + 2;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return p->vFontPad - 2;
    }

    return 0;
}

void drawPicrossInput(void)
{
    // First, get the square that is the player input box
    box_t iBox = boxFromCoord(p->input->x, p->input->y);
    // pulled out of the box so it would be easier to type everything.
    uint32_t x0 = iBox.x0;
    uint32_t x1 = iBox.x1;
    uint32_t y0 = iBox.y0;
    uint32_t y1 = iBox.y1;

    // x0,y0 -a-- x1,y0
    //   |d         |b
    // x0,y1 -c-- x1,y1

    // line a
    drawLine(x0, y0, x1, y0, p->input->inputBoxColor, 0);
    // line b
    drawLine(x1, y0, x1, y1, p->input->inputBoxColor, 0);
    // line c
    drawLine(x0, y1, x1, y1, p->input->inputBoxColor, 0);
    // line d
    drawLine(x0, y0, x0, y1, p->input->inputBoxColor, 0);

    uint16_t thickness = p->drawScale / 5
                         - 1; // trial and error to find this 20% of square (ish). -1, we draw the above line "centered"
    thickness = thickness < 2 ? 2 : thickness; // min 2
    for (uint8_t j = 1; j < thickness; j++)
    {
        // Thickness is the number of pixels we want (+1, 0 is a single pixels square).
        // So as j increases, we add a line to the outside, then inside, then outside, then inside of the square, the %2
        // alternates (basically an even/odd check) the i increments half as quickly as j, so the lines aren't added
        // twice as far as they should be.
        uint8_t i = (j + 1) / 2;
        if (j % 2 != 0) // because i starts at 1, j starts odd, so this "prefers" shrinking, if theres an odd number for
                        // thicknes
        {
            // Shrink a
            drawLine(x0, y0 + i, x1, y0 + i, p->input->inputBoxColor, 0);
            // shrink b
            drawLine(x1 - i, y0, x1 - i, y1, p->input->inputBoxColor, 0);
            // shrink c
            drawLine(x0, y1 - i, x1, y1 - i, p->input->inputBoxColor, 0);
            // shrink d
            drawLine(x0 + i, y0, x0 + i, y1, p->input->inputBoxColor, 0);
        }
        else
        {
            // Grow a
            drawLine(x0 - i, y0 - i, x1 + i, y0 - i, p->input->inputBoxColor, 0);
            // Grow b
            drawLine(x1 + i, y0 - i, x1 + i, y1 + i, p->input->inputBoxColor, 0);
            // Grow c
            drawLine(x0 - i, y1 + i, x1 + i, y1 + i, p->input->inputBoxColor, 0);
            // Grow d
            drawLine(x0 - i, y0 - i, x0 - i, y1 + i, p->input->inputBoxColor, 0);
        }
    }
}

/**
 * @brief Draws the background.
 *
 * @param d display to draw to.
 */
void drawBackground(void)
{
    if(p->animateBG == PICROSS_BG_NONE)
    {
        return;
    }

    if (p->bgScrollTimer >= p->bgScrollSpeed)
    {
        p->bgScrollTimer = 0;

        p->bgScrollXFrame++;
        p->bgScrollYFrame += p->bgScrollXFrame % 2; // scroll y twice as slowly as x
        // loop frames
        p->bgScrollXFrame = p->bgScrollXFrame % 20;
        p->bgScrollYFrame = p->bgScrollYFrame % 20;
    }
    
    switch(p->animateBG)
    {
        case PICROSS_BG_HEXAGONS:
        {
            drawMenuMega(p->menu, p->renderer, p->elapsedUs);
            if(p->currentPhase != PICROSS_YOUAREWIN)
            {
                drawWsgSimple(p->bigBody, 0, 0);
            }
            break;
        }
        case PICROSS_BG_DOTS:
        {
            for (int16_t y = p->bgScrollYFrame; y < TFT_HEIGHT; y+=20)
            {
                for (int16_t x = 19 - p->bgScrollXFrame; x < TFT_WIDTH; x+=20)
                {
                    setPxTft(x, y, c111); // Grid
                }
            }
            break;
        }
        case PICROSS_BG_NONE:
        {
            break;
        }
    }
}

/**
 * @brief Called by the swadge system. We cache inputs and process them in the game loop.
 *
 * @param evt
 */
void picrossGameButtonCb(buttonEvt_t* evt)
{
    p->input->btnState = evt->state;
}

void picrossGameTouchCb(bool touched)
{
    p->input->touchState = touched;
}

/**
 * @brief Turn of LEDS, Free up memory, and exit.
 * I think this gets called when the swadge gets reset, so maybe I could put a savePicrossProgress() call here, but I
 * want to double-check how this function is actually being used.
 *
 */
void picrossExitGame(void)
{
    // TODO MIDI music
    // buzzer_stop();
    if (NULL != p)
    {
        // set LED's to off.
        setLeds(p->offLEDS, CONFIG_NUM_LEDS);

        // free the empty menu used for the megaman background effect.
        deinitMenu(p->menu);

        freeFont(&(p->hintFont));
        freeFont(&(p->UIFont));
        heap_caps_free(p->input);
        heap_caps_free(p->puzzle);
        heap_caps_free(p);
        p = NULL;
    }
}

///=====
// SAVING AND LOADING
//===========

/**
 * @brief Saves the current level to permastore.
 * We save the current level index elsewhere, and re-create the entire puzzle on loading, so we dont have to store any
 * clues or w/h info. Saving the index elsewhere also means we don't need to load the entire puzzle just to see if we
 * should have it say "Continue" in the menu Currently the puzzle only saves when we hit select to exit. Once I test
 * performance on device, I would like to add saving it when you also hit start, or as frequently as every time you make
 * a change.
 */
void savePicrossProgress()
{
    // save level progress
    size_t size                     = sizeof(picrossProgressData_t);
    picrossProgressData_t* progress = heap_caps_calloc(1, size, MALLOC_CAP_8BIT);

    // save tentative marks
    size_t markSize = sizeof(p->tentativeMarks);
    bool tentativeMarks[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];

    // I know im doing things a kind of brute-force way here, copying over every value 1 by 1. I should be, i dunno,
    // just swapping pointers around? and...freeing up the old pointer? also, we dont need to save OUTOFBOUNDS for
    // smaller levels, since it shouldnt get read. while debugging and poking around at what its saving, I prefer not
    // having garbage getting saved.

    for (int y = 0; y < PICROSS_MAX_LEVELSIZE; y++)
    {
        for (int x = 0; x < PICROSS_MAX_LEVELSIZE; x++)
        {
            if (x < p->puzzle->width && y < p->puzzle->height)
            {
                progress->level[x][y] = p->puzzle->level[x][y];
                tentativeMarks[x][y]  = p->tentativeMarks[x][y];
            }
            else
            {
                progress->level[x][y] = OUTOFBOUNDS;
            }
        }
    }
    writeNvsBlob(picrossProgressData, progress, size);
    writeNvsBlob(picrossMarksData, tentativeMarks, markSize);

    heap_caps_free(progress);
}
/**
 * @brief Loads the current picross level in from dataStore.
 * Current Level Index also needs to be set, but that already must have happened in order to know what level to set and
 * have the menus work correctly.
 *
 */
void loadPicrossProgress()
{
    picrossProgressData_t progress;
    size_t size = sizeof(progress); // why is size 0
    readNvsBlob(picrossProgressData, &progress, &size);

    bool tentativeMarks[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];
    size_t marksSize = sizeof(tentativeMarks);
    if (!readNvsBlob(picrossMarksData, &tentativeMarks, &marksSize))
    {
        // Backwards compatibility for 1.0 saves
        memset(tentativeMarks, 0, sizeof(tentativeMarks));
    }

    for (int y = 0; y < p->puzzle->height; y++)
    {
        for (int x = 0; x < p->puzzle->width; x++)
        {
            p->puzzle->level[x][y]  = progress.level[x][y];
            p->tentativeMarks[x][y] = tentativeMarks[x][y];
        }
    }
}

//**
// * @brief Sets the "we beat this level" victory status for the current level.
// *
// * @param completed. True to mark as victories (When we win), false to reset to incomplete (when we reselect the
// puzzle).
// */
void saveCompletedOnSelectedLevel(bool completed)
{
    size_t size                    = sizeof(picrossVictoryData_t);
    picrossVictoryData_t* victData = heap_caps_calloc(
        1, size, MALLOC_CAP_8BIT); // zero out. if data doesnt exist, then its been correctly initialized to all 0s.
    readNvsBlob(picrossCompletedLevelData, victData, &size);
    victData->victories[(int)*&p->selectedLevel.index] = completed;
    writeNvsBlob(picrossCompletedLevelData, victData, sizeof(picrossVictoryData_t));
    heap_caps_free(victData);
}

/// @brief Called when in victory state, animates RGBs. Copied from the dance code, needs cleaned up/minified; but it
/// works so it's fine.
void picrossVictoryLEDs(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    if (reset)
    {
        p->ledAnimCount
            = 0; // this doesn't behave like I anticipate it behaving? but im not gonna touch what isn't broken.
        p->animtAccumulated = arg;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    p->animtAccumulated += tElapsedUs;
    while (p->animtAccumulated >= arg)
    {
        p->animtAccumulated -= arg;
        ledsUpdated = true;

        p->ledAnimCount--;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = ((((i * 256) / CONFIG_NUM_LEDS)) + p->ledAnimCount) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[i].r = (color >> 0) & 0xFF;
            leds[i].g = (color >> 8) & 0xFF;
            leds[i].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

/**
 * @brief Draw a box
 *
 * @param disp The display to draw the box to
 * @param box The box to draw
 * @param color The color of the box to draw
 * @param isFilled true to draw a filled box, false to draw an outline
 * @param scalingFactor The scaling factor to apply before drawing the box
 */
void drawBox(box_t box, paletteColor_t color, bool isFilled, int32_t scalingFactor)
{
    if (color == cTransparent)
    {
        return;
    }
    if (isFilled)
    {
        fillDisplayArea(box.x0 >> scalingFactor, box.y0 >> scalingFactor, box.x1 >> scalingFactor,
                        box.y1 >> scalingFactor, color);
    }
    else
    {
        drawRect(box.x0 >> scalingFactor, box.y0 >> scalingFactor, box.x1 >> scalingFactor, box.y1 >> scalingFactor,
                 color);
    }
}

/**
 * @brief Linear interpolate between two integers a and b by amount
 *
 * @param a The starting integer
 * @param b The ending integer
 * @param amount Lerp amount from 0 to PICROSS_LERP_AMOUNT.
 *               0 returns a.
 *               PICROSS_LERP_AMOUNT returns b.
 */
int8_t lerp(int8_t a, int8_t b, uint16_t amount)
{
    return a + ((b - a) * amount) / PICROSS_LERP_AMOUNT;
}

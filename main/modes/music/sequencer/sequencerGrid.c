#include "sequencerGrid.h"

#define NUM_PIANO_KEYS 88
#define KEY_MARGIN     2
#define PX_PER_BEAT    16

static const char* keys[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static vec_t getCursorScreenPos(sequencerVars_t* sv);

/**
 * @brief TODO doc
 *
 * @param sv
 * @return vec_t
 */
static vec_t getCursorScreenPos(sequencerVars_t* sv)
{
    vec_t pos = {
        .x = sv->labelWidth + 1 + sv->cursorPos.x * (sv->cellWidth),
        .y = sv->cursorPos.y * sv->rowHeight,
    };
    return subVec2d(pos, sv->gridOffset);
}

/**
 * @brief TODO doc
 *
 * @param sv
 * @param evt
 */
void sequencerGridButton(sequencerVars_t* sv, buttonEvt_t* evt)
{
    if (evt->down)
    {
        int numCells = sv->timeSig * sv->numBars;

        switch (evt->button)
        {
            case PB_UP:
            {
                if (sv->cursorPos.y)
                {
                    sv->cursorPos.y--;

                    // Adjust grid offset to be on screen
                    while (getCursorScreenPos(sv).y < (sv->rowHeight * 2) && sv->gridOffset.y > 0)
                    {
                        // TODO smooth scrolling
                        sv->gridOffset.y -= sv->rowHeight;
                    }
                }
                break;
            }
            case PB_DOWN:
            {
                if (sv->cursorPos.y < (NUM_PIANO_KEYS - 1))
                {
                    sv->cursorPos.y++;

                    // Adjust grid offset to be on screen
                    while (getCursorScreenPos(sv).y >= (sv->rowHeight * 14)
                           && sv->gridOffset.y < (sv->rowHeight) * (NUM_PIANO_KEYS - 16))
                    {
                        // TODO smooth scrolling
                        sv->gridOffset.y += sv->rowHeight;
                    }
                }
                break;
            }
            case PB_LEFT:
            {
                if (sv->cursorPos.x)
                {
                    sv->cursorPos.x--;
                    // TODO Adjust grid offset to be on screen
                    // TODO smooth scrolling
                }
                break;
            }
            case PB_RIGHT:
            {
                if (sv->cursorPos.x < numCells - 1)
                {
                    sv->cursorPos.x++;
                    // TODO Adjust grid offset to be on screen
                    // TODO smooth scrolling
                }
                break;
            }
            case PB_A:
            {
                // place / delete

                // Make a new note
                sequencerNote_t* newNote = calloc(1, sizeof(sequencerNote_t));
                newNote->midiNum         = 108 - sv->cursorPos.y;
                // sv->gridSize is denominator, i.e. quarter note grid is 4, eighth note grid is 8, etc.
                newNote->sixteenthOn  = sv->cursorPos.x * 16 / sv->gridSize;
                newNote->sixteenthOff = (sv->cursorPos.x + 1) * 16 / sv->gridSize;

                // Check for overlaps
                node_t* addBeforeThis = NULL;
                node_t* noteNode      = sv->notes.first;
                while (noteNode)
                {
                    sequencerNote_t* setNote = noteNode->val;

                    if ((setNote->midiNum == newNote->midiNum) && (setNote->sixteenthOn) < (newNote->sixteenthOff)
                        && (setNote->sixteenthOff) > (newNote->sixteenthOn))
                    {
                        // Overlap, delete note
                        free(newNote);
                        free(setNote);
                        removeEntry(&sv->notes, noteNode);
                        return;
                    }
                    else
                    {
                        if ((NULL == addBeforeThis) && (newNote->sixteenthOn <= setNote->sixteenthOn))
                        {
                            addBeforeThis = noteNode;
                        }
                        // Iterate
                        noteNode = noteNode->next;
                    }
                }

                // Reached this far, insert the note, in order
                if (NULL == addBeforeThis)
                {
                    push(&sv->notes, newNote);
                }
                else
                {
                    addBefore(&sv->notes, newNote, addBeforeThis);
                }
                break;
            }
            case PB_B:
            {
                // Play / pause
                sv->isPlaying = !sv->isPlaying;
                if (sv->isPlaying)
                {
                    sv->gridOffset.x = 0;
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

/**
 * @brief TODO doc
 *
 * @param sv
 */
void measureSequencerGrid(sequencerVars_t* sv)
{
    sv->labelWidth = textWidth(&sv->ibm, "C#7") + (2 * KEY_MARGIN);
    sv->cellWidth  = 64 / sv->gridSize;
    sv->rowHeight  = sv->ibm.height + (2 * KEY_MARGIN) + 1;

    sv->usPerPx = sv->usPerBeat / PX_PER_BEAT;
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
void drawSequencerGrid(sequencerVars_t* sv, int32_t elapsedUs)
{
    // Scroll
    if (sv->isPlaying)
    {
        sv->scrollTimer += elapsedUs;
        while (sv->scrollTimer >= sv->usPerPx)
        {
            sv->scrollTimer -= sv->usPerPx;
            sv->gridOffset.x++;
        }

        // TODO check for note on/offs
    }

    // Draw horizontal grid lines
    int32_t yOff = sv->rowHeight - 1 - sv->gridOffset.y;
    while (yOff < 0)
    {
        yOff += sv->rowHeight;
    }
    while (yOff < TFT_HEIGHT)
    {
        // Draw the dividing line
        drawLineFast(0, yOff, TFT_WIDTH, yOff, c222);
        yOff += sv->rowHeight;
    }

    // Draw vertical grid lines
    int32_t xOff = sv->labelWidth - sv->gridOffset.x;
    int32_t lIdx = 0;

    // Increment until on screen
    while (xOff < sv->labelWidth)
    {
        xOff += sv->cellWidth;
        lIdx++;
    }

    // Only draw lines until the end of screen or song
    int numCells = sv->timeSig * sv->numBars;
    while (xOff < TFT_WIDTH && lIdx <= numCells)
    {
        // Lighten the line every bar
        paletteColor_t lineColor = c222;
        if (0 == lIdx % sv->timeSig)
        {
            lineColor = c444;
        }
        drawLineFast(xOff, 0, xOff, TFT_HEIGHT, lineColor);
        xOff += sv->cellWidth;
        lIdx++;
    }

    // Draw placed notes
    node_t* noteNode = sv->notes.first;
    while (noteNode)
    {
        sequencerNote_t* note = noteNode->val;

        vec_t topLeft = {
            .x = 1 + sv->labelWidth + (4 * note->sixteenthOn),
            .y = (108 - note->midiNum) * sv->rowHeight,
        };
        vec_t bottomRight = {
            .x = sv->labelWidth + (4 * note->sixteenthOff),
            .y = topLeft.y + sv->rowHeight - 1,
        };

        topLeft     = subVec2d(topLeft, sv->gridOffset);
        bottomRight = subVec2d(bottomRight, sv->gridOffset);

        fillDisplayArea(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, c005);

        noteNode = noteNode->next;
    }

    // Draw cursor
    vec_t rectPos = getCursorScreenPos(sv);
    drawRect(rectPos.x, rectPos.y, rectPos.x + sv->cellWidth - 1, rectPos.y + sv->rowHeight - 1, c550);

    // Keep track of key and index
    int32_t kIdx   = 0;
    int32_t octave = 8;

    // Start offset by the grid
    yOff = KEY_MARGIN - sv->gridOffset.y;

    // Draw key labels
    while (yOff < TFT_HEIGHT)
    {
        if (yOff + sv->rowHeight <= 0)
        {
            // Off-screen, just increment until we're on screen
            yOff += sv->ibm.height + (2 * KEY_MARGIN) + 1;
        }
        else
        {
            // On-screen, draw it

            // Print key to a string
            char tmp[8];
            snprintf(tmp, sizeof(tmp) - 1, "%s%" PRId32, keys[kIdx], octave);

            // Draw sharps with inverted colors
            paletteColor_t bgColor   = c555;
            paletteColor_t textColor = c000;
            if ('#' == tmp[1])
            {
                bgColor   = c000;
                textColor = c555;
            }

            // Draw the key label
            fillDisplayArea(0, yOff - KEY_MARGIN, sv->labelWidth, yOff + sv->ibm.height + KEY_MARGIN, bgColor);
            drawText(&sv->ibm, textColor, tmp, KEY_MARGIN, yOff);
            yOff += sv->rowHeight;
        }

        // Decrement the key index, wrapping around the octave
        if (0 == kIdx)
        {
            kIdx = ARRAY_SIZE(keys) - 1;
            octave--;
        }
        else
        {
            kIdx--;
        }
    }
}

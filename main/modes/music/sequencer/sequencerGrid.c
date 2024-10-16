#include "sequencerGrid.h"

#define KEY_MARGIN  2
#define PX_PER_BEAT 16

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
        .x = sv->labelWidth + 1 + sv->cursorPos.x * (PX_PER_BEAT / 4),
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
        switch (evt->button)
        {
            case PB_UP:
            {
                if (!sv->isPlaying && sv->cursorPos.y)
                {
                    // Move the cursor
                    sv->cursorPos.y--;

                    // Adjust the grid offset target to smoothly scroll
                    if (getCursorScreenPos(sv).y < sv->rowHeight)
                    {
                        sv->gridOffsetTarget.y -= sv->rowHeight;
                        if (sv->gridOffsetTarget.y < 0)
                        {
                            sv->gridOffsetTarget.y = 0;
                        }
                    }
                }
                break;
            }
            case PB_DOWN:
            {
                if (!sv->isPlaying && sv->cursorPos.y < (NUM_PIANO_KEYS - 1))
                {
                    // Move the cursor
                    sv->cursorPos.y++;

                    // Adjust the grid offset target to smoothly scroll
                    if (getCursorScreenPos(sv).y > TFT_HEIGHT - (2 * sv->rowHeight))
                    {
                        sv->gridOffsetTarget.y += sv->rowHeight;
                    }
                }
                break;
            }
            case PB_LEFT:
            {
                if (!sv->isPlaying && sv->cursorPos.x)
                {
                    // Move the cursor
                    sv->cursorPos.x -= (16 / sv->songParams.grid);

                    // Adjust the grid offset target to smoothly scroll
                    if (getCursorScreenPos(sv).x < sv->labelWidth + sv->cellWidth)
                    {
                        sv->gridOffsetTarget.x -= sv->cellWidth;
                        if (sv->gridOffsetTarget.x < 0)
                        {
                            sv->gridOffsetTarget.x = 0;
                        }
                    }
                }
                break;
            }
            case PB_RIGHT:
            {
                if (!sv->isPlaying && sv->cursorPos.x < (sv->songParams.songEnd - 1))
                {
                    // Move the cursor
                    sv->cursorPos.x += (16 / sv->songParams.grid);

                    // Adjust the grid offset target to smoothly scroll
                    if (getCursorScreenPos(sv).x > TFT_WIDTH - sv->cellWidth)
                    {
                        sv->gridOffsetTarget.x += sv->cellWidth;
                    }
                }
                break;
            }
            case PB_A:
            {
                // Don't modify notes while playing
                if (sv->isPlaying)
                {
                    return;
                }

                // Make a new note
                sequencerNote_t* newNote = calloc(1, sizeof(sequencerNote_t));
                newNote->midiNum         = 108 - sv->cursorPos.y;
                // sv->songParams.grid is denominator, i.e. quarter note grid is 4, eighth note grid is 8, etc.
                newNote->sixteenthOn  = sv->cursorPos.x;
                newNote->sixteenthOff = newNote->sixteenthOn + (16 / sv->noteParams.type);

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
                if (sv->isPlaying)
                {
                    // If it's playing, stop
                    sv->isPlaying = false;

                    // Stop MIDI
                    midiPlayerReset(globalMidiPlayerGet(MIDI_BGM));

                    // Stop here
                    sv->gridOffsetTarget.x = sv->gridOffset.x;

                    // Turn of all notes
                    node_t* noteNode = sv->notes.first;
                    while (noteNode)
                    {
                        sequencerNote_t* note = noteNode->val;
                        note->isOn            = false;
                        noteNode              = noteNode->next;
                    }
                }
                else if (sv->gridOffset.x)
                {
                    // If it's stopped, reset to beginning
                    sv->gridOffsetTarget.x = 0;
                    sv->cursorPos.x        = 0;
                }
                else
                {
                    // If it's at the beginning, play
                    sv->isPlaying = true;

                    sv->songTimer = 0;

                    midiPause(globalMidiPlayerGet(MIDI_BGM), false);
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
 * @brief TODO
 *
 * @param sv
 */
void sequencerGridTouch(sequencerVars_t* sv)
{
    int32_t phi = 0, r = 0, intensity = 0;
    bool touched = getTouchJoystick(&phi, &r, &intensity);

    bool isWheelMenuActive = wheelMenuActive(sv->noteMenu, sv->wheelRenderer);

    if (isWheelMenuActive || touched)
    {
        if (touched)
        {
            sv->noteMenu = wheelMenuTouch(sv->noteMenu, sv->wheelRenderer, phi, r);
        }
        else
        {
            sv->noteMenu = wheelMenuTouchRelease(sv->noteMenu, sv->wheelRenderer);
        }

        bool nowActive = wheelMenuActive(sv->noteMenu, sv->wheelRenderer);
        if (nowActive && !isWheelMenuActive)
        {
            // Menu just became active, reset it?
            // sd->updateMenu = true;
        }
        else if (isWheelMenuActive && !nowActive)
        {
            // sd->updateMenu     = true;
            // sd->forceResetMenu = true;
        }
        isWheelMenuActive = nowActive;
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
    sv->cellWidth  = (4 * PX_PER_BEAT) / sv->songParams.grid;
    sv->rowHeight  = sv->ibm.height + (2 * KEY_MARGIN) + 1;
    sv->numRows    = TFT_HEIGHT / sv->rowHeight;

    sv->usPerPx = sv->usPerBeat / PX_PER_BEAT;

    // Snap cursor to new grid
    int32_t interval = 16 / sv->songParams.grid;
    sv->cursorPos.x  = (sv->cursorPos.x / interval) * interval;
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
void drawSequencerGrid(sequencerVars_t* sv, int32_t elapsedUs)
{
    if (!sv->isPlaying)
    {
        RUN_TIMER_EVERY(sv->smoothScrollTimer, 16667, elapsedUs, {
            // Half the distance to X
            if (sv->gridOffsetTarget.x > sv->gridOffset.x)
            {
                sv->gridOffset.x += (sv->gridOffsetTarget.x - sv->gridOffset.x + 1) / 2;
            }
            else if (sv->gridOffsetTarget.x < sv->gridOffset.x)
            {
                sv->gridOffset.x -= (sv->gridOffset.x - sv->gridOffsetTarget.x + 1) / 2;
            }

            // Half the distance to Y
            if (sv->gridOffsetTarget.y > sv->gridOffset.y)
            {
                sv->gridOffset.y += (sv->gridOffsetTarget.y - sv->gridOffset.y + 1) / 2;
            }
            else if (sv->gridOffsetTarget.y < sv->gridOffset.y)
            {
                sv->gridOffset.y -= (sv->gridOffset.y - sv->gridOffsetTarget.y + 1) / 2;
            }
        });
    }

    // Scroll
    if (sv->isPlaying)
    {
        sv->scrollTimer += elapsedUs;
        while (sv->scrollTimer >= sv->usPerPx)
        {
            sv->scrollTimer -= sv->usPerPx;
            sv->gridOffset.x++;

            // Move cursor to be visible, even if it's not controllable
            if (getCursorScreenPos(sv).x < sv->labelWidth)
            {
                sv->cursorPos.x += (16 / sv->songParams.grid);
            }
        }

        // Check for note on/off
        sv->songTimer += elapsedUs;
        node_t* noteNode = sv->notes.first;
        while (noteNode)
        {
            sequencerNote_t* note = noteNode->val;
            int32_t usOn          = note->sixteenthOn * sv->usPerBeat / 4;
            int32_t usOff         = note->sixteenthOff * sv->usPerBeat / 4;

            if (usOn > sv->songTimer)
            {
                // Note start is past the song timer, stop looping
                break;
            }
            else if (usOn <= sv->songTimer && sv->songTimer <= usOff)
            {
                if (!note->isOn)
                {
                    note->isOn = true;

                    // Create MIDI on and push it into the queue to be picked up by the callback
                    midiEvent_t* evt  = calloc(1, sizeof(midiEvent_t));
                    evt->type         = MIDI_EVENT;
                    evt->midi.status  = 0x90;
                    evt->midi.data[0] = note->midiNum;
                    evt->midi.data[1] = 0x40;
#if !defined(__XTENSA__)
                    OGLockMutex(sv->midiQueueMutex);
#endif
                    push(&sv->midiQueue, evt);
#if !defined(__XTENSA__)
                    OGUnlockMutex(sv->midiQueueMutex);
#endif
                }
            }
            else if (note->isOn)
            {
                note->isOn = false;

                // Create MIDI off and push it into the queue to be picked up by the callback
                midiEvent_t* evt  = calloc(1, sizeof(midiEvent_t));
                evt->type         = MIDI_EVENT;
                evt->midi.status  = 0x80;
                evt->midi.data[0] = note->midiNum;
                evt->midi.data[1] = 0x40;

#if !defined(__XTENSA__)
                OGLockMutex(sv->midiQueueMutex);
#endif
                push(&sv->midiQueue, evt);
#if !defined(__XTENSA__)
                OGUnlockMutex(sv->midiQueueMutex);
#endif
            }
            noteNode = noteNode->next;
        }
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
    // TODO validate
    int numCells = sv->songParams.songEnd * sv->songParams.grid / 16;
    while (xOff < TFT_WIDTH && lIdx <= numCells)
    {
        // Lighten the line every bar
        paletteColor_t lineColor = c222;
        if (0 == lIdx % (sv->songParams.timeSig * sv->songParams.grid / 4))
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
    int32_t width = (4 * PX_PER_BEAT) / sv->noteParams.type;
    drawRect(rectPos.x, rectPos.y, rectPos.x + width - 1, rectPos.y + sv->rowHeight - 1, c550);

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

    if (wheelMenuActive(sv->noteMenu, sv->wheelRenderer))
    {
        // fillDisplayArea(sd->wheelTextArea.pos.x, sd->wheelTextArea.pos.y - 2,
        //                 sd->wheelTextArea.pos.x + sd->wheelTextArea.width,
        //                 sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2, c025);
        // drawTriangleOutlined(sd->wheelTextArea.pos.x, sd->wheelTextArea.pos.y - 2,
        //                      sd->wheelTextArea.pos.x - sd->betterFont.height / 2,
        //                      sd->wheelTextArea.pos.y + sd->wheelTextArea.height / 2, sd->wheelTextArea.pos.x,
        //                      sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2, c025, c025);
        // drawTriangleOutlined(sd->wheelTextArea.pos.x + sd->wheelTextArea.width, sd->wheelTextArea.pos.y - 2,
        //                      sd->wheelTextArea.pos.x + sd->wheelTextArea.width + sd->betterFont.height / 2,
        //                      sd->wheelTextArea.pos.y + sd->wheelTextArea.height / 2,
        //                      sd->wheelTextArea.pos.x + sd->wheelTextArea.width,
        //                      sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2, c025, c025);
        drawWheelMenu(sv->noteMenu, sv->wheelRenderer, elapsedUs);
    }
}

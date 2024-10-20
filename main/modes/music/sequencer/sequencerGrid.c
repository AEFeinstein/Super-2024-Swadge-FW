//==============================================================================
// Includes
//==============================================================================

#include "sequencerGrid.h"

//==============================================================================
// Defines
//==============================================================================

#define KEY_MARGIN  2
#define PX_PER_BEAT 16

#define MIDI_VELOCITY 0xFF

//==============================================================================
// Variables
//==============================================================================

static const char* keys[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

//==============================================================================
// Function declarations
//==============================================================================

static vec_t getCursorScreenPos(sequencerVars_t* sv);
static void stopSequencer(sequencerVars_t* sv);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Get the screen position of the cursor (top left)
 *
 * @param sv The entire sequencer state
 * @return The X,Y position of the cursor on the screen
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
 * @brief Handle a button input event which may move the cursor, edit notes, or play
 *
 * @param sv The entire sequencer state
 * @param evt The event that occurred
 */
void sequencerGridButton(sequencerVars_t* sv, buttonEvt_t* evt)
{
    if (evt->down)
    {
        // TODO hold directions to continuously scroll
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
                if (!sv->isPlaying)
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
                newNote->channel      = sv->noteParams.channel;

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

                        // Return, nothing to add
                        return;
                    }
                    else
                    {
                        // No overlap yet
                        if ((NULL == addBeforeThis) && (newNote->sixteenthOn <= setNote->sixteenthOn))
                        {
                            // Save where the note should be inserted in time order
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

                // Play the just-added note
                // Stop it first if it's currently exampling
                if (0 < sv->exampleMidiNoteTimer)
                {
                    midiNoteOff(globalMidiPlayerGet(MIDI_BGM), sv->exampleMidiChannel, sv->exampleMidiNote,
                                MIDI_VELOCITY);
                }

                // Set the example note and timer
                sv->exampleMidiNote      = newNote->midiNum;
                sv->exampleMidiChannel   = sv->noteParams.channel;
                sv->exampleMidiNoteTimer = (sv->usPerBeat * 4) / (sv->noteParams.type);

                // Play it
                midiNoteOn(globalMidiPlayerGet(MIDI_BGM), sv->exampleMidiChannel, sv->exampleMidiNote, MIDI_VELOCITY);
                break;
            }
            case PB_B:
            {
                if (sv->isPlaying)
                {
                    // If it's playing, stop
                    stopSequencer(sv);
                }
                else if (sv->gridOffset.x)
                {
                    // If it's stopped, reset to beginning
                    sv->gridOffsetTarget.x = 0;
                    sv->cursorPos.x        = 0;
                }
                else
                {
                    // If it's at the beginning, stop again to be safe, then play
                    stopSequencer(sv);
                    sv->isPlaying = true;
                    sv->songTimer = 0;
                }
                break;
            }
            // PB_START is handled in sequencerMainLoop()
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief Handle a touch event on the touchpad. This uses the wheel menu
 *
 * @param sv The entire sequencer state
 */
void sequencerGridTouch(sequencerVars_t* sv)
{
    // Poll the touchpad state
    int32_t phi = 0, r = 0, intensity = 0;
    bool touched = getTouchJoystick(&phi, &r, &intensity);

    if (wheelMenuActive(sv->noteMenu, sv->wheelRenderer) || touched)
    {
        if (touched)
        {
            sv->noteMenu = wheelMenuTouch(sv->noteMenu, sv->wheelRenderer, phi, r);
        }
        else
        {
            sv->noteMenu = wheelMenuTouchRelease(sv->noteMenu, sv->wheelRenderer);
        }
    }
}

/**
 * @brief Measure out how to draw the sequencer grid. This should be called after changing settings.
 *
 * @param sv The entire sequencer state
 */
void measureSequencerGrid(sequencerVars_t* sv)
{
    sv->labelWidth = textWidth(&sv->font_ibm, "C#7") + (2 * KEY_MARGIN);
    sv->cellWidth  = (4 * PX_PER_BEAT) / sv->songParams.grid;
    sv->rowHeight  = sv->font_ibm.height + (2 * KEY_MARGIN) + 1;
    sv->numRows    = TFT_HEIGHT / sv->rowHeight;

    sv->usPerPx = sv->usPerBeat / PX_PER_BEAT;

    // Snap cursor to new grid
    int32_t interval = 16 / sv->songParams.grid;
    sv->cursorPos.x  = (sv->cursorPos.x / interval) * interval;
}

/**
 * @brief Run the timers for sequencer grid, like scrolling the grid and playing the song
 *
 * @param sv The entire sequencer state
 * @param elapsedUs The time elapsed since the last function call
 */
void runSequencerTimers(sequencerVars_t* sv, int32_t elapsedUs)
{
    // Run timer for example note
    if (0 < sv->exampleMidiNoteTimer)
    {
        sv->exampleMidiNoteTimer -= elapsedUs;
        // If the timer expired
        if (0 >= sv->exampleMidiNoteTimer)
        {
            // Stop the example note
            midiNoteOff(globalMidiPlayerGet(MIDI_BGM), sv->exampleMidiChannel, sv->exampleMidiNote, MIDI_VELOCITY);
        }
    }

    // Run a timer to smoothly scroll the grid offset to match the target
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

    // If the song is playing, scroll everything along
    if (sv->isPlaying)
    {
        // Run scroll timer
        sv->scrollTimer += elapsedUs;
        while (sv->scrollTimer >= sv->usPerPx)
        {
            sv->scrollTimer -= sv->usPerPx;

            // Move the grid
            sv->gridOffset.x++;
            sv->gridOffsetTarget.x++;

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

            // Get the note on and off times
            int32_t usOn  = note->sixteenthOn * sv->usPerBeat / 4;
            int32_t usOff = note->sixteenthOff * sv->usPerBeat / 4;

            if (usOn > sv->songTimer)
            {
                // Note start is past the song timer, stop looping
                break;
            }
            // If the song time is between on and off
            else if (usOn <= sv->songTimer && sv->songTimer <= usOff)
            {
                // If the note isn't on yet
                if (!note->isOn)
                {
                    // Turn the note on
                    note->isOn = true;
                    midiNoteOn(globalMidiPlayerGet(MIDI_BGM), note->channel, note->midiNum, MIDI_VELOCITY);
                }
            }
            // If the note is on, and shouldn't be
            else if (note->isOn)
            {
                // Turn it off
                note->isOn = false;
                midiNoteOff(globalMidiPlayerGet(MIDI_BGM), note->channel, note->midiNum, MIDI_VELOCITY);
            }

            // Iterate
            noteNode = noteNode->next;
        }

        // Check for song end
        uint32_t songEndUs = (sv->songParams.songEnd * 60 * (int64_t)1000000) / (4 * sv->songParams.tempo);
        if (sv->songTimer >= songEndUs)
        {
            // Always stop
            stopSequencer(sv);

            // Loop if that's set
            if (sv->songParams.loop)
            {
                // reset to beginning
                sv->gridOffsetTarget.x = 0;
                sv->cursorPos.x        = 0;
                // Play
                sv->isPlaying = true;
                sv->songTimer = 0;
            }
        }
    }
}

/**
 * @brief Draw the sequencer grid
 *
 * @param sv The entire sequencer state
 * @param elapsedUs The time elapsed since the last function call
 */
void drawSequencerGrid(sequencerVars_t* sv, int32_t elapsedUs)
{
    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

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

    // Only draw lines until the end of screen
    while (xOff < TFT_WIDTH)
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

        fillDisplayArea(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, getChannelColor(note->channel));

        noteNode = noteNode->next;
    }

    // Draw cursor
    vec_t rectPos = getCursorScreenPos(sv);
    int32_t width = (4 * PX_PER_BEAT) / sv->noteParams.type;
    drawRect(rectPos.x, rectPos.y, rectPos.x + width - 1, rectPos.y + sv->rowHeight - 1, c550);

    // Draw song end line
    int32_t songEndX = sv->labelWidth + sv->songParams.songEnd * (PX_PER_BEAT / 4) - sv->gridOffset.x;
    fillDisplayArea(songEndX, 0, songEndX + 4, TFT_HEIGHT, c331);

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
            yOff += sv->font_ibm.height + (2 * KEY_MARGIN) + 1;
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
            fillDisplayArea(0, yOff - KEY_MARGIN, sv->labelWidth, yOff + sv->font_ibm.height + KEY_MARGIN, bgColor);
            drawText(&sv->font_ibm, textColor, tmp, KEY_MARGIN, yOff);
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
        drawWheelMenu(sv->noteMenu, sv->wheelRenderer, elapsedUs);
    }
}

/**
 * @brief Stop the sequencer from playing
 *
 * @param sv The entire sequencer state
 */
static void stopSequencer(sequencerVars_t* sv)
{
    sv->isPlaying = false;

    // Stop MIDI
    midiPlayerReset(globalMidiPlayerGet(MIDI_BGM));
    midiPause(globalMidiPlayerGet(MIDI_BGM), false);

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

    // Turn off example note
    sv->exampleMidiChannel   = 0;
    sv->exampleMidiNote      = 0;
    sv->exampleMidiNoteTimer = 0;
}

//==============================================================================
// Includes
//==============================================================================

#include "swadgeHero_game.h"

//==============================================================================
// Defines
//==============================================================================

#define HIT_BAR          16
#define GAME_NOTE_RADIUS 8

#define TRAVEL_US_PER_PX ((TRAVEL_TIME_US) / (TFT_HEIGHT - HIT_BAR + (2 * GAME_NOTE_RADIUS)))

//==============================================================================
// Const Variables
//==============================================================================

static const paletteColor_t colors_e[] = {c020, c004, c420, c222};
static const buttonBit_t noteToBtn_e[] = {PB_LEFT, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_e[]     = {-1, -1, 0, 1, 3, 2};

static const paletteColor_t colors_m[] = {c020, c400, c550, c004, c420, c222};
static const buttonBit_t noteToBtn_m[] = {PB_LEFT, PB_DOWN, PB_UP, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_m[]     = {2, 1, 0, 3, 5, 4};

static const paletteColor_t colors_h[] = {c020, c400, c550, c004, c420, c222};
static const buttonBit_t noteToBtn_h[] = {PB_LEFT, PB_DOWN, PB_UP, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_h[]     = {2, 1, 0, 3, 5, 4};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param sh
 * @param midi
 * @param chart
 */
void shLoadSong(shVars_t* sh, const char* midi, const char* chart)
{
    size_t sz    = 0;
    sh->numFrets = 1 + shLoadChartData(sh, cnfsGetFile(chart, &sz), sz);

    if (6 == sh->numFrets)
    {
        sh->btnToNote = btnToNote_h;
        sh->noteToBtn = noteToBtn_h;
        sh->colors    = colors_h;
    }
    else if (5 == sh->numFrets)
    {
        sh->btnToNote = btnToNote_m;
        sh->noteToBtn = noteToBtn_m;
        sh->colors    = colors_m;
    }
    else if (4 == sh->numFrets)
    {
        sh->btnToNote = btnToNote_e;
        sh->noteToBtn = noteToBtn_e;
        sh->colors    = colors_e;
    }

    // Load the MIDI file
    loadMidiFile(midi, &sh->midiSong, true);
    globalMidiPlayerPlaySong(&sh->midiSong, MIDI_BGM);
    globalMidiPlayerPauseAll();

    // Set the lead-in timer
    sh->leadInUs = TRAVEL_TIME_US;
}

/**
 * @brief TODO
 *
 * @param sh
 * @param data
 * @param size
 * @return uint32_t
 */
uint32_t shLoadChartData(shVars_t* sh, const uint8_t* data, size_t size)
{
    uint32_t maxFret  = 0;
    uint32_t dIdx     = 0;
    sh->numChartNotes = (data[dIdx++] << 8);
    sh->numChartNotes |= (data[dIdx++]);

    sh->chartNotes = heap_caps_calloc(sh->numChartNotes, sizeof(shChartNote_t), MALLOC_CAP_SPIRAM);

    for (int32_t nIdx = 0; nIdx < sh->numChartNotes; nIdx++)
    {
        sh->chartNotes[nIdx].tick = (data[dIdx + 0] << 24) | //
                                    (data[dIdx + 1] << 16) | //
                                    (data[dIdx + 2] << 8) |  //
                                    (data[dIdx + 3] << 0);
        dIdx += 4;

        sh->chartNotes[nIdx].note = data[dIdx++];

        if ((sh->chartNotes[nIdx].note & 0x7F) > maxFret)
        {
            maxFret = sh->chartNotes[nIdx].note & 0x7F;
        }

        if (0x80 & sh->chartNotes[nIdx].note)
        {
            sh->chartNotes[nIdx].note &= 0x7F;

            // Use the hold time to see when this note ends
            sh->chartNotes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                        (data[dIdx + 1] << 0);
            dIdx += 2;
        }
    }

    return maxFret;
}

/**
 * @brief TODO
 *
 * @param sh
 * @param elapsedUs
 */
void shRunTimers(shVars_t* sh, uint32_t elapsedUs)
{
    // Run a lead-in timer to allow notes to spawn before the song starts playing
    if (sh->leadInUs > 0)
    {
        sh->leadInUs -= elapsedUs;

        if (sh->leadInUs <= 0)
        {
            globalMidiPlayerResumeAll();
            sh->leadInUs = 0;
        }
    }

    // Get a reference to the player
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Get the position of the song and when the next event is, in ms
    int32_t songUs;
    if (sh->leadInUs > 0)
    {
        songUs = -sh->leadInUs;
    }
    else
    {
        songUs = SAMPLES_TO_US(player->sampleCount);
    }

    // Check events until one hasn't happened yet or the song ends
    while (sh->currentChartNote < sh->numChartNotes)
    {
        // When the next event occurs
        int32_t nextEventUs
            = MIDI_TICKS_TO_US(sh->chartNotes[sh->currentChartNote].tick, player->tempo, player->reader.division);

        // Check if the game note should be spawned now to reach the hit bar in time
        if (songUs + TRAVEL_TIME_US >= nextEventUs)
        {
            // Spawn an game note
            shGameNote_t* ni = heap_caps_calloc(1, sizeof(shGameNote_t), MALLOC_CAP_SPIRAM);
            ni->note         = sh->chartNotes[sh->currentChartNote].note;
            ni->headPosY     = TFT_HEIGHT + (GAME_NOTE_RADIUS * 2);

            // If this is a hold note
            if (sh->chartNotes[sh->currentChartNote].hold)
            {
                // Figure out at what microsecond the tail ends
                int32_t tailUs = MIDI_TICKS_TO_US(sh->chartNotes[sh->currentChartNote].hold, player->tempo,
                                                  player->reader.division);
                // Convert the time to a number of pixels
                int32_t tailPx = tailUs / TRAVEL_US_PER_PX;
                // Add the length pixels to the head to get the tail
                ni->tailPosY = ni->headPosY + tailPx;
            }
            else
            {
                // No tail
                ni->tailPosY = -1;
            }

            // Start the timer at zero
            ni->timer = 0;

            // Push into the list of game notes
            push(&sh->gameNotes, ni);

            // Increment the chart data
            sh->currentChartNote++;
        }
        else
        {
            // Nothing more to be spawned right now
            break;
        }
    }

    // Track if an game note was removed
    bool removed = false;

    // Run all the game note timers
    node_t* gameNoteNode = sh->gameNotes.first;
    while (gameNoteNode)
    {
        // Get a reference
        shGameNote_t* gameNote = gameNoteNode->val;

        // Run this game note's timer
        gameNote->timer += elapsedUs;
        while (gameNote->timer >= TRAVEL_US_PER_PX)
        {
            gameNote->timer -= TRAVEL_US_PER_PX;

            bool shouldRemove = false;

            // Move the whole game note up
            if (!gameNote->held)
            {
                gameNote->headPosY--;
                if (gameNote->tailPosY >= 0)
                {
                    gameNote->tailPosY--;
                }

                // If it's off screen
                if (gameNote->headPosY < -GAME_NOTE_RADIUS && (gameNote->tailPosY < 0))
                {
                    // Mark it for removal
                    shouldRemove = true;
                }
            }
            else // The game note is being held
            {
                // Only move the tail position
                if (gameNote->tailPosY >= HIT_BAR)
                {
                    gameNote->tailPosY--;
                }

                // If the tail finished
                if (gameNote->tailPosY < HIT_BAR)
                {
                    // Mark it for removal
                    shouldRemove = true;
                }
            }

            // If the game note should be removed
            if (shouldRemove)
            {
                // Remove this game note
                free(gameNoteNode->val);
                removeEntry(&sh->gameNotes, gameNoteNode);

                // Stop the while timer loop
                removed = true;
                break;
            }
        }

        // If an game note was removed
        if (removed)
        {
            // Stop iterating through notes
            break;
        }
        else
        {
            // Iterate to the next
            gameNoteNode = gameNoteNode->next;
        }
    }
}

/**
 * @brief TODO
 *
 * @param sh
 */
void shDrawGame(shVars_t* sh)
{
    // Clear the display
    clearPxTft();

    // Draw the target area
    drawLineFast(0, HIT_BAR, TFT_WIDTH - 1, HIT_BAR, c555);
    for (int32_t i = 0; i < sh->numFrets; i++)
    {
        int32_t xOffset = ((i * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
        drawCircle(xOffset, HIT_BAR, GAME_NOTE_RADIUS + 2, c555);
    }

    // Draw all the game notes
    node_t* gameNoteNode = sh->gameNotes.first;
    while (gameNoteNode)
    {
        // Draw the game note
        shGameNote_t* gameNote = gameNoteNode->val;
        int32_t xOffset        = ((gameNote->note * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
        drawCircleFilled(xOffset, gameNote->headPosY, GAME_NOTE_RADIUS, sh->colors[gameNote->note]);

        // If there is a tail
        if (gameNote->tailPosY >= 0)
        {
            // Draw the tail
            fillDisplayArea(xOffset - 2, gameNote->headPosY, xOffset + 3, gameNote->tailPosY,
                            sh->colors[gameNote->note]);
        }

        // Iterate
        gameNoteNode = gameNoteNode->next;
    }

    // Draw indicators that the button is pressed
    for (int32_t bIdx = 0; bIdx < sh->numFrets; bIdx++)
    {
        if (sh->btnState & sh->noteToBtn[bIdx])
        {
            int32_t xOffset = ((bIdx * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
            drawCircleOutline(xOffset, HIT_BAR, GAME_NOTE_RADIUS + 8, 4, sh->colors[bIdx]);
        }
    }

    // Set LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    // for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    // {
    //     leds[i].r = (255 * ((i + 0) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    //     leds[i].g = (255 * ((i + 3) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    //     leds[i].b = (255 * ((i + 6) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    // }
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief TODO
 *
 * @param sh
 * @param evt
 */
void shGameInput(shVars_t* sh, buttonEvt_t* evt)
{
    sh->btnState = evt->state;

    if (evt->down)
    {
        // Iterate through all currently shown game notes
        node_t* gameNoteNode = sh->gameNotes.first;
        while (gameNoteNode)
        {
            shGameNote_t* gameNote = gameNoteNode->val;

            // If the game note matches the button
            if (gameNote->note == sh->btnToNote[31 - __builtin_clz(evt->button)])
            {
                // Find how off the timing is
                int32_t pxOff = ABS(HIT_BAR - gameNote->headPosY);
                int32_t usOff = pxOff * TRAVEL_US_PER_PX;
                printf("%" PRId32 " us off\n", usOff);

                // Check if this button hit a note
                bool gameNoteHit = false;

                // Classify the time off
                if (usOff < 21500)
                {
                    printf("  Fantastic\n");
                    gameNoteHit = true;
                }
                else if (usOff < 43000)
                {
                    printf("  Marvelous\n");
                    gameNoteHit = true;
                }
                else if (usOff < 102000)
                {
                    printf("  Great\n");
                    gameNoteHit = true;
                }
                else if (usOff < 135000)
                {
                    printf("  Decent\n");
                    gameNoteHit = true;
                }
                else if (usOff < 180000)
                {
                    printf("  Way Off\n");
                    gameNoteHit = true;
                }
                else
                {
                    printf("  MISS\n");
                }

                // If it was close enough to hit
                if (gameNoteHit)
                {
                    if (gameNote->tailPosY >= 0)
                    {
                        // There is a tail, don't remove the note yet
                        gameNote->headPosY = HIT_BAR;
                        gameNote->held     = true;
                    }
                    else
                    {
                        // No tail, remove the game note
                        node_t* nextNode = gameNoteNode->next;
                        free(gameNoteNode->val);
                        removeEntry(&sh->gameNotes, gameNoteNode);
                        gameNoteNode = nextNode;
                    }
                }

                // the button was matched to an game note, break the loop
                break;
            }

            // Iterate to the next game note
            gameNoteNode = gameNoteNode->next;
        }
    }
    else
    {
        // TODO handle ups when holding tails
    }

    // Check for analog touch
    // int32_t centerVal, intensityVal;
    // if (getTouchCentroid(&centerVal, &intensityVal))
    // {
    //     printf("touch center: %" PRId32 ", intensity: %" PRId32 "\n", centerVal, intensityVal);
    // }
    // else
    // {
    //     printf("no touch\n");
    // }

    // // Get the acceleration
    // int16_t a_x, a_y, a_z;
    // accelGetAccelVec(&a_x, &a_y, &a_z);
}

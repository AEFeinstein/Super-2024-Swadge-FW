//==============================================================================
// Includes
//==============================================================================

#include <limits.h>
#include "swadgeHero_game.h"
#include "swadgeHero_menu.h"

//==============================================================================
// Defines
//==============================================================================

#define HIT_BAR 16

#define SH_TEXT_TIME 500000

//==============================================================================
// Enums
//==============================================================================

typedef struct
{
    int32_t val;
    const char* letter;
} shLetterGrade_t;

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t timer;
} drawStar_t;

//==============================================================================
// Const Variables
//==============================================================================

static const paletteColor_t colors_e[] = {c333, c512, c024, c400};
static const buttonBit_t noteToBtn_e[] = {PB_LEFT, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_e[]     = {-1, -1, 0, 1, 3, 2};
static const int32_t noteToIcon_e[]    = {0, 3, 4, 5};

static const paletteColor_t colors_m[] = {c333, c542, c512, c024, c400};
static const buttonBit_t noteToBtn_m[] = {PB_LEFT, PB_UP, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_m[]     = {1, -1, 0, 2, 4, 3};
static const int32_t noteToIcon_m[]    = {0, 2, 3, 4, 5};

static const paletteColor_t colors_h[] = {c333, c315, c542, c512, c024, c400};
static const buttonBit_t noteToBtn_h[] = {PB_LEFT, PB_DOWN, PB_UP, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_h[]     = {2, 1, 0, 3, 5, 4};
static const int32_t noteToIcon_h[]    = {0, 1, 2, 3, 4, 5};

static const char hit_early[] = "Early";
static const char hit_late[]  = "Late";

static const shLetterGrade_t grades[] = {
    {.val = 100, .letter = "S"},                                                          //
    {.val = 97, .letter = "A+"}, {.val = 93, .letter = "A"}, {.val = 90, .letter = "A-"}, //
    {.val = 87, .letter = "B+"}, {.val = 83, .letter = "B"}, {.val = 80, .letter = "B-"}, //
    {.val = 77, .letter = "C+"}, {.val = 73, .letter = "C"}, {.val = 70, .letter = "C-"}, //
    {.val = 67, .letter = "D+"}, {.val = 63, .letter = "D"}, {.val = 0, .letter = "F"},
};

const shTimingGrade_t timings[NUM_NOTE_TIMINGS] = {
    {.timing = 21500, .label = "Fantastic"}, //
    {.timing = 43000, .label = "Marvelous"}, //
    {.timing = 102000, .label = "Great"},    //
    {.timing = 135000, .label = "Decent"},   //
    {.timing = 180000, .label = "Way Off"},  //
    {.timing = INT32_MAX, .label = "Miss"},  //
};

//==============================================================================
// Function Declarations
//==============================================================================

static int32_t getMultiplier(shVars_t* sh);
static void shSongOver(void);
static void shMissNote(shVars_t* sh);
static void shHitNote(shVars_t* sh, int32_t baseScore);
static int32_t getXOffset(shVars_t* sh, int32_t note);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a song to play
 *
 * @param sh The Swadge Hero game state
 * @param song The name of the song to load
 * @param difficulty The song difficulty
 */
void shLoadSong(shVars_t* sh, const shSong_t* song, shDifficulty_t difficulty)
{
    // Don't immediately quit
    sh->gameEnd = false;

    // Load settings
    sh->failOn     = shGetSettingFail();
    sh->scrollTime = shGetSettingSpeed();

    // Save song name
    sh->songName = song->name;

    // Pick variables based on difficulty
    char suffix;
    switch (difficulty)
    {
        default:
        case SH_EASY:
        {
            suffix         = 'e';
            sh->btnToNote  = btnToNote_e;
            sh->noteToBtn  = noteToBtn_e;
            sh->colors     = colors_e;
            sh->noteToIcon = noteToIcon_e;
            break;
        }
        case SH_MEDIUM:
        {
            suffix         = 'm';
            sh->btnToNote  = btnToNote_m;
            sh->noteToBtn  = noteToBtn_m;
            sh->colors     = colors_m;
            sh->noteToIcon = noteToIcon_m;
            break;
        }
        case SH_HARD:
        {
            suffix         = 'h';
            sh->btnToNote  = btnToNote_h;
            sh->noteToBtn  = noteToBtn_h;
            sh->colors     = colors_h;
            sh->noteToIcon = noteToIcon_h;
            break;
        }
    }

    // Load chart data
    size_t sz          = 0;
    char chartFile[64] = {0};
    snprintf(chartFile, sizeof(chartFile) - 2, "%s_%c.cch", song->fName, suffix);
    sh->numFrets = 1 + shLoadChartData(sh, cnfsGetFile(chartFile, &sz), sz);

    // Save the key for the score
    shGetNvsKey(song->fName, difficulty, sh->hsKey);

    // Make sure MIDI player is initialized
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Disable channel 15
    player->channels[MIDI_CHANNEL_COUNT - 1].ignore = true;

    // Load the MIDI file
    char midiName[64] = {0};
    snprintf(midiName, sizeof(midiName) - 2, "%s.mid", song->fName);
    loadMidiFile(midiName, &sh->midiSong, true);

    // Set the file, but don't play yet
    midiPause(player, true);
    player->sampleCount = 0;
    midiSetFile(player, &sh->midiSong);

    // Seek to load the tempo and length, then reset
    midiSeek(player, -1);
    sh->tempo         = player->tempo;
    int32_t songLenUs = SAMPLES_TO_US(player->sampleCount);
    globalMidiPlayerStop(true);

    // Set the lead-in timer
    sh->leadInUs = sh->scrollTime;

    // Start with one fret line at t=0
    sh->lastFretLineUs     = 0;
    shFretLine_t* fretLine = heap_caps_calloc(1, sizeof(shFretLine_t), MALLOC_CAP_SPIRAM);
    fretLine->headPosY     = TFT_HEIGHT + 1;
    fretLine->headTimeUs   = sh->lastFretLineUs;
    push(&sh->fretLines, fretLine);

    // Reset scoring
    sh->score    = 0;
    sh->combo    = 0;
    sh->maxCombo = 0;
    sh->notesHit = 0;
    sh->grade    = grades[ARRAY_SIZE(grades) - 1].letter;

    // Figure out how often to sample the fail meter for the chart after the song
    sh->failSampleInterval = songLenUs / NUM_FAIL_METER_SAMPLES;
    clear(&sh->failSamples);
    sh->failMeter = 50;

    // Clear histogram
    memset(sh->noteHistogram, 0, sizeof(sh->noteHistogram));

    // Set up LED variables
    sh->usPerLedDecay = sh->tempo / (2 * 0xFF);
    sh->nextBlinkUs   = 0;
    sh->ledBaseVal    = 0;
    sh->ledDecayTimer = 0;
    memset(&sh->ledHitVal, 0, sizeof(sh->ledHitVal));

    // Set up
    sh->textTimerUs = 0;
    sh->hitText     = timings[ARRAY_SIZE(timings) - 1].label;
    sh->timingText  = hit_early;

    // Set up icon pulse variables
    sh->iconIdx     = 0;
    sh->iconTimerUs = 0;

    // Turn general MIDI on
    midiGmOn(player);
}

/**
 * @brief Load chart data about a song
 *
 * @param sh The Swadge Hero game state
 * @param data The data to parse
 * @param size The size of the data to load
 * @return The number of frets used by this chart
 */
uint32_t shLoadChartData(shVars_t* sh, const uint8_t* data, size_t size)
{
    // The number of frets in the track, counted while loading
    uint32_t maxFret = 0;

    // Data index
    uint32_t dIdx = 0;

    // The first 16 bits are number of notes
    sh->numChartNotes = (data[dIdx++] << 8);
    sh->numChartNotes |= (data[dIdx++]);

    // Start the chart at note 0
    sh->currentChartNote = 0;

    // Start a count of hit notes. This is incremented by hold notes
    sh->totalHitNotes = sh->numChartNotes;

    // Allocate memory for all the chart notes
    sh->chartNotes = heap_caps_calloc(sh->numChartNotes, sizeof(shChartNote_t), MALLOC_CAP_SPIRAM);

    // Read all the chart notes from the file
    for (int32_t nIdx = 0; nIdx < sh->numChartNotes; nIdx++)
    {
        // The tick this note should be hit at
        sh->chartNotes[nIdx].tick = (data[dIdx + 0] << 24) | //
                                    (data[dIdx + 1] << 16) | //
                                    (data[dIdx + 2] << 8) |  //
                                    (data[dIdx + 3] << 0);
        dIdx += 4;

        // Tye type of note
        sh->chartNotes[nIdx].note = data[dIdx++];

        // Keep track of the max fret. The top bit indicates a hold note
        if ((sh->chartNotes[nIdx].note & 0x7F) > maxFret)
        {
            maxFret = sh->chartNotes[nIdx].note & 0x7F;
        }

        // If this is a hold note
        if (0x80 & sh->chartNotes[nIdx].note)
        {
            // Clear the bit
            sh->chartNotes[nIdx].note &= 0x7F;

            // Read the next two bytes as the hold time
            sh->chartNotes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                        (data[dIdx + 1] << 0);
            dIdx += 2;

            // Increment this, because hold notes count as two for the letter ranking
            sh->totalHitNotes++;
        }
    }

    // Return the number of frets used
    return maxFret;
}

/**
 * @brief Tear down the Swadge Hero game and free memory
 *
 * @param sh The Swadge Hero game state
 */
void shTeardownGame(shVars_t* sh)
{
    // Free MIDI data
    globalMidiPlayerStop(true);
    unloadMidiFile(&sh->midiSong);

    // Free chart data
    free(sh->chartNotes);

    // Free game UI data
    void* val;
    while ((val = pop(&sh->gameNotes)))
    {
        free(val);
    }
    while ((val = pop(&sh->fretLines)))
    {
        free(val);
    }
    while ((val = pop(&sh->starList)))
    {
        free(val);
    }
}

/**
 * @brief Run timers for the Swadge Hero game
 *
 * @param sh The Swadge Hero game state
 * @param elapsedUs The time elapsed since the last time this function was called.
 * @return true if the game is still running, false if it is over
 */
bool shRunTimers(shVars_t* sh, uint32_t elapsedUs)
{
    if (sh->paused)
    {
        return true;
    }

    // If the game end flag is raised
    if (sh->gameEnd)
    {
        // Grade the performance
        int32_t notePct = (100 * sh->notesHit) / sh->totalHitNotes;
        // Find the letter grade based on the note percentage
        int32_t gradeIdx;
        for (gradeIdx = 0; gradeIdx < ARRAY_SIZE(grades); gradeIdx++)
        {
            if (notePct >= grades[gradeIdx].val)
            {
                sh->grade = grades[gradeIdx].letter;
                break;
            }
        }

        // Read old high score from NVS
        int32_t oldHs = 0;
        if (!readNvs32(sh->hsKey, &oldHs))
        {
            // No score saved yet, assume 0
            oldHs = 0;
        }

        // Write the new high score to NVS if it's larger
        if (sh->score > oldHs)
        {
            // four top bits are letter, bottom 28 bits are score
            int32_t nvsScore = ((gradeIdx & 0x0F) << 28) | (sh->score & 0x0FFFFFFF);
            writeNvs32(sh->hsKey, nvsScore);
        }

        // Switch to the game end screen
        shChangeScreen(sh, SH_GAME_END);

        // Return and indicate the game is over
        return false;
    }

    // Get a reference to the player
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Run a lead-in timer to allow notes to spawn before the song starts playing
    int32_t songUs;
    if (sh->leadInUs > 0)
    {
        // Decrement the lead in timer
        sh->leadInUs -= elapsedUs;

        // The song hasn't started yet, so the time is negative
        songUs = -sh->leadInUs;

        // Start the song
        if (sh->leadInUs <= 0)
        {
            // Lead in is over, start the song
            globalMidiPlayerPlaySongCb(&sh->midiSong, MIDI_BGM, shSongOver);
            sh->leadInUs = 0;
            songUs       = 0;
        }
    }
    else
    {
        // Song is playing, Get the position of the song and when the next event is, in microseconds
        songUs = SAMPLES_TO_US(player->sampleCount);
    }

    // Run a timer for pop-up text
    if (sh->textTimerUs > 0)
    {
        sh->textTimerUs -= elapsedUs;
    }

    // Run timers for stars. For each star
    node_t* starNode = sh->starList.first;
    while (starNode)
    {
        drawStar_t* ds = starNode->val;
        // Decrement the timer
        ds->timer -= elapsedUs;
        // If it expired
        if (0 >= ds->timer)
        {
            // Remove and free the star without disturbing the list
            node_t* toRemove = starNode;
            starNode         = starNode->next;
            free(toRemove->val);
            removeEntry(&sh->starList, toRemove);
        }
        else
        {
            // Just iterate
            starNode = starNode->next;
        }
    }

    // If failure is on and the song is playing
    if (sh->failOn && songUs >= 0)
    {
        // Run a timer to sample the fail meter for a chart
        while (sh->failSampleInterval * sh->failSamples.length <= songUs)
        {
            // Save the sample to display on the game over screen
            push(&sh->failSamples, (void*)((intptr_t)sh->failMeter));
        }
    }

    // Generate fret lines based on tempo
    int32_t nextFretLineUs = sh->lastFretLineUs + sh->tempo;
    if (songUs + sh->scrollTime >= nextFretLineUs)
    {
        // Increment time for the last fret line
        sh->lastFretLineUs += sh->tempo;

        // Allocate and push a new fret line
        shFretLine_t* fretLine = heap_caps_calloc(1, sizeof(shFretLine_t), MALLOC_CAP_SPIRAM);
        fretLine->headPosY     = TFT_HEIGHT + 1;
        fretLine->headTimeUs   = sh->lastFretLineUs;
        push(&sh->fretLines, fretLine);
    }

    // Iterate through fret lines
    node_t* fretLineNode = sh->fretLines.first;
    while (fretLineNode)
    {
        shFretLine_t* fretLine = fretLineNode->val;

        // Update positions
        fretLine->headPosY = (((TFT_HEIGHT - HIT_BAR) * (fretLine->headTimeUs - songUs)) / sh->scrollTime) + HIT_BAR;

        // Remove if off screen
        if (fretLine->headPosY < 0)
        {
            node_t* toRemove = fretLineNode;
            fretLineNode     = fretLineNode->next;
            free(toRemove->val);
            removeEntry(&sh->fretLines, toRemove);
        }
        else
        {
            // Iterate normally
            fretLineNode = fretLineNode->next;
        }
    }

    // Decay LEDs
    sh->ledDecayTimer += elapsedUs;
    while (sh->ledDecayTimer >= sh->usPerLedDecay)
    {
        sh->ledDecayTimer -= sh->usPerLedDecay;

        // Tempo LEDs
        if (sh->ledBaseVal)
        {
            sh->ledBaseVal--;
        }

        // LEDs from note hits
        if (sh->ledHitVal.r)
        {
            sh->ledHitVal.r--;
        }
        if (sh->ledHitVal.g)
        {
            sh->ledHitVal.g--;
        }
        if (sh->ledHitVal.b)
        {
            sh->ledHitVal.b--;
        }
    }

    // Blink LEDs based on tempo
    if (songUs >= sh->nextBlinkUs)
    {
        sh->nextBlinkUs += sh->tempo;
        sh->ledBaseVal = 0xFF;

        // Change icons on the beat & start a timer to revert
        sh->iconIdx     = 1;
        sh->iconTimerUs = sh->tempo / 4;
    }

    // Run a timer for icon pulsing
    if (sh->iconTimerUs > 0)
    {
        sh->iconTimerUs -= elapsedUs;
        if (sh->iconTimerUs <= 0)
        {
            // Return to the normal icon
            sh->iconIdx = 0;
        }
    }

    // Check events until one hasn't happened yet or the song ends
    while (sh->currentChartNote < sh->numChartNotes)
    {
        // When the next event occurs
        int32_t nextEventUs
            = MIDI_TICKS_TO_US(sh->chartNotes[sh->currentChartNote].tick, player->tempo, player->reader.division);

        // Check if the game note should be spawned now to reach the hit bar in time
        if (songUs + sh->scrollTime >= nextEventUs)
        {
            // Spawn a game note
            shGameNote_t* ni = heap_caps_calloc(1, sizeof(shGameNote_t), MALLOC_CAP_SPIRAM);
            ni->note         = sh->chartNotes[sh->currentChartNote].note;

            // Get an icon reference for spacing
            wsg_t* icon = &sh->icons[sh->noteToIcon[ni->note]][0];

            // Start the game note offscreen
            ni->headPosY = TFT_HEIGHT + (icon->h / 2);

            // Save when the note should be hit
            ni->headTimeUs = nextEventUs;

            // If this is a hold note
            if (sh->chartNotes[sh->currentChartNote].hold)
            {
                // Start the tail offscreen too
                ni->tailPosY = TFT_HEIGHT + (icon->h / 2);

                // Save when the tail ends
                int32_t tailTick = sh->chartNotes[sh->currentChartNote].tick + //
                                   sh->chartNotes[sh->currentChartNote].hold;
                ni->tailTimeUs = MIDI_TICKS_TO_US(tailTick, player->tempo, player->reader.division);
            }
            else
            {
                // No tail
                ni->tailPosY   = -1;
                ni->tailTimeUs = -1;
            }

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

    // Update note positions based on song position
    node_t* gameNoteNode = sh->gameNotes.first;
    while (gameNoteNode)
    {
        // Get a reference
        shGameNote_t* gameNote = gameNoteNode->val;
        wsg_t* icon            = &sh->icons[sh->noteToIcon[gameNote->note]][0];

        // Update note position
        if (gameNote->held)
        {
            // Held notes stick at the hit bar
            gameNote->headPosY = HIT_BAR;
        }
        else
        {
            // Moving notes follow this formula
            gameNote->headPosY
                = (((TFT_HEIGHT - HIT_BAR) * (gameNote->headTimeUs - songUs)) / sh->scrollTime) + HIT_BAR;
        }

        // Update tail position if there is a hold
        if (-1 != gameNote->tailPosY)
        {
            gameNote->tailPosY
                = (((TFT_HEIGHT - HIT_BAR) * (gameNote->tailTimeUs - songUs)) / sh->scrollTime) + HIT_BAR;
        }

        // Check if the note should be removed
        bool shouldRemove = false;
        if (-1 != gameNote->tailPosY)
        {
            // There is a tail
            if (gameNote->held)
            {
                if (gameNote->tailPosY < HIT_BAR)
                {
                    // Note is currently being held, tail reached the bar
                    shouldRemove = true;

                    // Note was held all the way, so score it.
                    shHitNote(sh, 5);
                }
            }
            else if (gameNote->tailPosY < 0)
            {
                // Note is not held, tail is offscreen
                shouldRemove = true;
            }
        }
        else if (gameNote->headPosY < -(icon->h / 2))
        {
            // There is no tail and the whole note is offscreen
            shouldRemove = true;

            // Break the combo
            shMissNote(sh);
        }

        // If the game note should be removed
        if (shouldRemove)
        {
            // Save the node to remove before iterating
            node_t* toRemove = gameNoteNode;

            // Iterate to the next
            gameNoteNode = gameNoteNode->next;

            // Remove the game note
            free(toRemove->val);
            removeEntry(&sh->gameNotes, toRemove);

            // Note that it was missed
            sh->hitText = timings[ARRAY_SIZE(timings) - 1].label;
            // Set a timer to not show the text forever
            sh->textTimerUs = SH_TEXT_TIME;
        }
        else
        {
            // Iterate to the next
            gameNoteNode = gameNoteNode->next;
        }
    }

    // Return that the game is still playing
    return true;
}

/**
 * @brief Draw the Swadge Hero game
 *
 * @param sh The Swadge Hero game state
 */
void shDrawGame(shVars_t* sh)
{
    // Display cleared in background callback

    // Draw fret lines first
    node_t* fretLineNode = sh->fretLines.first;
    while (fretLineNode)
    {
        shFretLine_t* fretLine = fretLineNode->val;
        drawLineFast(0, fretLine->headPosY, TFT_WIDTH, fretLine->headPosY, c111);
        fretLineNode = fretLineNode->next;
    }

    // Draw the target area
    for (int32_t i = 0; i < sh->numFrets; i++)
    {
        int32_t xOffset = ((i * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
        wsg_t* outline  = &sh->outlines[sh->noteToIcon[i]];
        drawWsgSimple(outline, xOffset - (outline->w / 2), HIT_BAR - (outline->h / 2));
    }

    // Draw indicators that the button is pressed
    for (int32_t bIdx = 0; bIdx < sh->numFrets; bIdx++)
    {
        if (sh->btnState & sh->noteToBtn[bIdx])
        {
            int32_t xOffset = ((bIdx * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
            wsg_t* pressed  = &sh->pressed[sh->noteToIcon[bIdx]];
            drawWsgSimple(pressed, xOffset - (pressed->w / 2), HIT_BAR - (pressed->h / 2));
        }
    }

    // Draw all the game notes
    node_t* gameNoteNode = sh->gameNotes.first;
    while (gameNoteNode)
    {
        shGameNote_t* gameNote = gameNoteNode->val;
        int32_t xOffset        = getXOffset(sh, gameNote->note);

        // If there is a tail
        if (gameNote->tailPosY >= 0)
        {
            // Draw the tail
            fillDisplayArea(xOffset - 2, gameNote->headPosY, xOffset + 3, gameNote->tailPosY,
                            sh->colors[gameNote->note]);
        }

        // Draw the game note
        wsg_t* icon = &sh->icons[sh->noteToIcon[gameNote->note]][sh->iconIdx];
        drawWsgSimple(icon, xOffset - (icon->w / 2), gameNote->headPosY - (icon->h / 2));

        // Iterate
        gameNoteNode = gameNoteNode->next;
    }

    // Draw stars
    node_t* starNode = sh->starList.first;
    while (starNode)
    {
        drawStar_t* ds = starNode->val;
        drawWsgSimple(&sh->star, ds->x, ds->y);
        starNode = starNode->next;
    }

    // Draw text
    if (sh->textTimerUs > 0)
    {
        int16_t tWidth = textWidth(&sh->ibm, sh->hitText);
        drawText(&sh->ibm, c555, sh->hitText, (TFT_WIDTH - tWidth) / 2, 100);

        if ((timings[0].label != sh->hitText) && (timings[ARRAY_SIZE(timings) - 1].label != sh->hitText))
        {
            tWidth = textWidth(&sh->ibm, sh->timingText);
            drawText(&sh->ibm, sh->timingText == hit_early ? c335 : c533, sh->timingText, (TFT_WIDTH - tWidth) / 2,
                     100 + sh->ibm.height + 16);
        }
    }

    if (sh->paused)
    {
        const char pStr[] = "Paused";
        int16_t tWidth    = textWidth(&sh->righteous, pStr);
        drawText(&sh->righteous, c555, pStr, (TFT_WIDTH - tWidth) / 2, 150);
    }

    int32_t textMargin    = 12;
    int32_t failBarHeight = 8;

    // Draw combo when 10+
    if (sh->combo >= 10)
    {
        char comboText[32] = {0};
        snprintf(comboText, sizeof(comboText) - 1, "Combo: %" PRId32, sh->combo);
        int16_t tWidth = textWidth(&sh->rodin, comboText);
        drawText(&sh->rodin, c555, comboText, (TFT_WIDTH - tWidth - textMargin),
                 TFT_HEIGHT - failBarHeight - sh->rodin.height);
    }

    // Always draw score
    char scoreText[32] = {0};
    snprintf(scoreText, sizeof(scoreText) - 1, "%" PRId32, sh->score);
    drawText(&sh->rodin, c555, scoreText, textMargin, TFT_HEIGHT - failBarHeight - sh->rodin.height);

    // If fail is on
    if (sh->failOn)
    {
        // Draw fail meter bar
        int32_t failMeterWidth = (sh->failMeter * TFT_WIDTH) / 100;
        fillDisplayArea(0, TFT_HEIGHT - failBarHeight, failMeterWidth, TFT_HEIGHT - 1, c440);
    }

    // Draw title and artist during lead in
    if (sh->leadInUs > 0)
    {
        int16_t tWidth = textWidth(&sh->rodin, sh->menuSong->name);
        drawText(&sh->rodin, c555, sh->menuSong->name, (TFT_WIDTH - tWidth) / 2,
                 (TFT_HEIGHT / 2) - sh->rodin.height - 2);
        tWidth = textWidth(&sh->rodin, sh->menuSong->artist);
        drawText(&sh->rodin, c555, sh->menuSong->artist, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2) + 2);
    }

    // Set LEDs
    led_t leds[CONFIG_NUM_LEDS];
    memset(leds, sh->ledBaseVal, sizeof(led_t) * 5);

    for (int32_t i = 5; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i] = sh->ledHitVal;
    }
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Helper draw function to get the X offset for a given note
 *
 * @param sh The Swadge Hero game state
 * @param note The note to get the X offset for
 * @return The X offset for the note
 */
static int32_t getXOffset(shVars_t* sh, int32_t note)
{
    return ((note * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
}

/**
 * @brief Handle button input during a Swadge Hero game
 *
 * @param sh The Swadge Hero game state
 * @param evt The button event
 */
void shGameInput(shVars_t* sh, buttonEvt_t* evt)
{
    // Save the button state
    sh->btnState = evt->state;

    // This pauses and unpauses
    if (PB_B < evt->button)
    {
        if (evt->down)
        {
            if (sh->paused)
            {
                // Resume
                sh->paused = false;
                globalMidiPlayerResumeAll();
            }
            else
            {
                // Pause
                sh->paused = true;
                globalMidiPlayerPauseAll();
            }
        }

        // No further processing
        return;
    }

    // Get the position of the song and when the next event is, in ms
    int32_t songUs;
    if (sh->leadInUs > 0)
    {
        songUs = -sh->leadInUs;
    }
    else
    {
        songUs = SAMPLES_TO_US(globalMidiPlayerGet(MIDI_BGM)->sampleCount);
    }

    // Find the note that corresponds to this button press
    int32_t notePressed = sh->btnToNote[31 - __builtin_clz(evt->button)];

    // If a note was pressed
    if (-1 != notePressed)
    {
        // See if the button press matches a note
        bool noteMatch = false;

        // Iterate through all currently shown game notes
        node_t* gameNoteNode = sh->gameNotes.first;
        while (gameNoteNode)
        {
            shGameNote_t* gameNote = gameNoteNode->val;

            // If the game note matches the button
            if (gameNote->note == notePressed)
            {
                // Button event matches a note on screen somewhere
                noteMatch = true;

                // Button was pressed
                if (evt->down)
                {
                    // Find how off the timing is
                    int32_t usOff = (songUs - gameNote->headTimeUs);

                    // Set either early or late
                    sh->timingText = (usOff > 0) ? hit_late : hit_early;

                    // Find the absolute difference
                    usOff = ABS(usOff);

                    // Check if this button hit a note
                    bool gameNoteHit  = false;
                    int32_t baseScore = 0;

                    // Classify the time off
                    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(timings); tIdx++)
                    {
                        if (usOff <= timings[tIdx].timing)
                        {
                            sh->hitText = timings[tIdx].label;
                            if (INT32_MAX != timings[tIdx].timing)
                            {
                                // Note hit
                                gameNoteHit = true;
                                baseScore   = ARRAY_SIZE(timings) - tIdx - 1;
                            }
                            else
                            {
                                // Break the combo
                                shMissNote(sh);
                            }
                            break;
                        }
                    }

                    // Set a timer to not show the text forever
                    sh->textTimerUs = SH_TEXT_TIME;

                    // If it was close enough to hit
                    if (gameNoteHit)
                    {
                        shHitNote(sh, baseScore);

                        // Draw a star for a moment
                        drawStar_t* ds = heap_caps_calloc(1, sizeof(drawStar_t), MALLOC_CAP_SPIRAM);
                        ds->x          = getXOffset(sh, gameNote->note) - sh->star.w / 2;
                        ds->y          = gameNote->headPosY - (sh->star.h / 2);
                        ds->timer      = 250000;
                        push(&sh->starList, ds);

                        // Light some LEDs
                        if (4 <= baseScore)
                        {
                            // Greenish
                            sh->ledHitVal.r = 0x00;
                            sh->ledHitVal.g = 0xFF;
                            sh->ledHitVal.b = 0x00;
                        }
                        else if (hit_late == sh->timingText)
                        {
                            // Reddish
                            sh->ledHitVal.r = 0xFF;
                            sh->ledHitVal.g = 0x00;
                            sh->ledHitVal.b = 0x00;
                        }
                        else
                        {
                            // Blueish
                            sh->ledHitVal.r = 0x00;
                            sh->ledHitVal.g = 0x00;
                            sh->ledHitVal.b = 0xFF;
                        }

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
                }
                else if (gameNote->held)
                {
                    // A held note was released. Remove it!
                    node_t* nextNode = gameNoteNode->next;
                    free(gameNoteNode->val);
                    removeEntry(&sh->gameNotes, gameNoteNode);
                    gameNoteNode = nextNode;
                }

                // the button was matched to a game note, break the loop
                break;
            }

            // Iterate to the next game note
            gameNoteNode = gameNoteNode->next;
        }
        // Done iterating through notes

        // Check if a button down didn't have a matching note on screen
        if (false == noteMatch && evt->down)
        {
            // Total miss
            sh->hitText = timings[ARRAY_SIZE(timings) - 1].label;
            shMissNote(sh);
        }
    }
}

/**
 * @brief Get the current Swadge Hero multiplier, based on combo
 *
 * Every 10 combo is another 1x multiplier
 *
 * @param sh The Swadge Hero game state
 * @return The current multiplier
 */
static int32_t getMultiplier(shVars_t* sh)
{
    return (sh->combo + 10) / 10;
}

/**
 * @brief Set a flag that the game is over and should be cleaned up next loop
 */
static void shSongOver(void)
{
    // Set a flag, end the game synchronously
    getShVars()->gameEnd = true;
}

/**
 * @brief Score a Swadge Hero note as hit
 *
 * @param sh The Swadge Hero game state
 * @param baseScore The score of this note, 1 to 5
 */
static void shHitNote(shVars_t* sh, int32_t baseScore)
{
    // Save this note in the histogram
    sh->noteHistogram[ARRAY_SIZE(timings) - 1 - baseScore]++;

    // Increment the score
    sh->score += getMultiplier(sh) * baseScore;

    // Increment the notes hit, for letter grading later
    sh->notesHit++;

    // Increment the combo & track the max combo
    sh->combo++;
    if (sh->combo > sh->maxCombo)
    {
        sh->maxCombo = sh->combo;
    }

    // Increment the fail meter
    if (sh->failOn)
    {
        sh->failMeter = MIN(100, sh->failMeter + 1);
    }
}

/**
 * @brief Score a Swadge Hero note as a miss
 *
 * @param sh The Swadge Hero game state
 */
static void shMissNote(shVars_t* sh)
{
    // Tally this as a miss in the histogram
    sh->noteHistogram[ARRAY_SIZE(timings) - 1]++;

    // Break the combo
    sh->combo = 0;

    // If fail is on
    if (sh->failOn)
    {
        // Decrement the fail meter
        sh->failMeter = MAX(0, sh->failMeter - 2);
        if (0 == sh->failMeter)
        {
            sh->gameEnd = true;
        }
    }
}

/**
 * @brief Get the letter grade for the grade index
 *
 * @param gradeIdx The grade index, 0 through 12
 * @return The letter grade for this index
 */
const char* getLetterGrade(int32_t gradeIdx)
{
    return grades[gradeIdx].letter;
}

//==============================================================================
// Includes
//==============================================================================

#include <limits.h>
#include "swadgeHero_game.h"
#include "swadgeHero_menu.h"

//==============================================================================
// Defines
//==============================================================================

#define HIT_BAR          16
#define GAME_NOTE_HEIGHT 16
#define GAME_NOTE_WIDTH  24

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

static const paletteColor_t colors_e[] = {c500, c330, c005, c303};
static const buttonBit_t noteToBtn_e[] = {PB_LEFT, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_e[]     = {-1, -1, 0, 1, 3, 2};
static const int32_t noteToIcon_e[]    = {0, 3, 4, 5};

static const paletteColor_t colors_m[] = {c500, c033, c330, c005, c303};
static const buttonBit_t noteToBtn_m[] = {PB_LEFT, PB_UP, PB_RIGHT, PB_B, PB_A};
static const int32_t btnToNote_m[]     = {1, -1, 0, 2, 4, 3};
static const int32_t noteToIcon_m[]    = {0, 2, 3, 4, 5};

static const paletteColor_t colors_h[] = {c500, c050, c033, c330, c005, c303};
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
static int32_t getXoffset(shVars_t* sh, int32_t note);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param sh
 * @param song
 * @param difficulty
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
    const char* chartFile;
    switch (difficulty)
    {
        default:
        case SH_EASY:
        {
            chartFile      = song->easy;
            sh->btnToNote  = btnToNote_e;
            sh->noteToBtn  = noteToBtn_e;
            sh->colors     = colors_e;
            sh->noteToIcon = noteToIcon_e;
            break;
        }
        case SH_MEDIUM:
        {
            chartFile      = song->med;
            sh->btnToNote  = btnToNote_m;
            sh->noteToBtn  = noteToBtn_m;
            sh->colors     = colors_m;
            sh->noteToIcon = noteToIcon_m;
            break;
        }
        case SH_HARD:
        {
            chartFile      = song->hard;
            sh->btnToNote  = btnToNote_h;
            sh->noteToBtn  = noteToBtn_h;
            sh->colors     = colors_h;
            sh->noteToIcon = noteToIcon_h;
            break;
        }
    }

    // Load chart data
    size_t sz    = 0;
    sh->numFrets = 1 + shLoadChartData(sh, cnfsGetFile(chartFile, &sz), sz);

    // Save the key for the score
    shGetNvsKey(song->midi, difficulty, sh->hsKey);

    // Make sure MIDI player is initialized
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Load the MIDI file
    loadMidiFile(song->midi, &sh->midiSong, true);

    // Set the file, but don't play yet
    midiPause(player, true);
    player->sampleCount = 0;
    midiSetFile(player, &sh->midiSong);

    // Seek to load the tempo and length, then reset
    midiSeek(player, -1);
    sh->tempo         = player->tempo;
    int32_t songLenUs = SAMPLES_TO_US(player->sampleCount);
    globalMidiPlayerStop(true);

    // Save the LED decay rate based on tempo
    sh->usPerLedDecay = sh->tempo / (2 * 0xFF);

    // Figure out how often to sample the fail meter for the chart after the song
    sh->failSampleInterval = songLenUs / NUM_FAIL_METER_SAMPLES;

    // Set the lead-in timer
    sh->leadInUs = sh->scrollTime;

    // Start with one fret line at t=0
    sh->lastFretLineUs = 0;

    shFretLine_t* fretLine = heap_caps_calloc(1, sizeof(shFretLine_t), MALLOC_CAP_SPIRAM);
    fretLine->headPosY     = TFT_HEIGHT + 1;
    fretLine->headTimeUs   = sh->lastFretLineUs;
    push(&sh->fretLines, fretLine);

    // Set score, combo, and fail
    sh->score     = 0;
    sh->combo     = 0;
    sh->maxCombo  = 0;
    sh->failMeter = 50;
}

/**
 * @brief TODO doc
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

    sh->totalNotes = sh->numChartNotes;

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

            // Use the hold time to see how long this note is held
            sh->chartNotes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                        (data[dIdx + 1] << 0);
            dIdx += 2;

            // Holds count as another note for letter ranking
            sh->totalNotes++;
        }
    }

    return maxFret;
}

/**
 * @brief TODO doc
 *
 * @param sh
 * @param elapsedUs
 * @return true
 * @return false
 */
bool shRunTimers(shVars_t* sh, uint32_t elapsedUs)
{
    if (sh->gameEnd)
    {
        // Grade the performance
        int32_t notePct = (100 * sh->notesHit) / sh->totalNotes;
        int gradeIdx;
        for (gradeIdx = 0; gradeIdx < ARRAY_SIZE(grades); gradeIdx++)
        {
            if (notePct >= grades[gradeIdx].val)
            {
                sh->grade = grades[gradeIdx].letter;
                break;
            }
        }

        // Save score to NVS
        int32_t oldHs = 0;
        if (!readNvs32(sh->hsKey, &oldHs))
        {
            // No score saved yet, assume 0
            oldHs = 0;
        }

        // Write the high score to NVS if it's larger
        if (sh->score > oldHs)
        {
            // four top bits are letter, bottom 28 bits are score
            int32_t nvsScore = ((gradeIdx & 0x0F) << 28) | (sh->score & 0x0FFFFFFF);
            writeNvs32(sh->hsKey, nvsScore);
        }

        // Switch to the game end screen
        shChangeScreen(sh, SH_GAME_END);
        return false;
    }

    // Run a lead-in timer to allow notes to spawn before the song starts playing
    if (sh->leadInUs > 0)
    {
        sh->leadInUs -= elapsedUs;

        if (sh->leadInUs <= 0)
        {
            globalMidiPlayerPlaySongCb(&sh->midiSong, MIDI_BGM, shSongOver);
            sh->leadInUs = 0;
        }
    }

    // Run a timer for pop-up text
    if (sh->textTimerUs > 0)
    {
        sh->textTimerUs -= elapsedUs;
    }

    // Run timers for stars
    node_t* starNode = sh->starList.first;
    while (starNode)
    {
        drawStar_t* ds = starNode->val;
        ds->timer -= elapsedUs;
        if (0 >= ds->timer)
        {
            node_t* toRemove = starNode;
            starNode         = starNode->next;
            heap_caps_free(toRemove->val);
            removeEntry(&sh->starList, toRemove);
        }
        else
        {
            starNode = starNode->next;
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

    if (sh->failOn && songUs >= 0)
    {
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
        sh->lastFretLineUs += sh->tempo;

        shFretLine_t* fretLine = heap_caps_calloc(1, sizeof(shFretLine_t), MALLOC_CAP_SPIRAM);
        fretLine->headPosY     = TFT_HEIGHT + 1;
        fretLine->headTimeUs   = sh->lastFretLineUs;
        push(&sh->fretLines, fretLine);
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

    // Blink based on tempo
    if (songUs >= sh->nextBlinkUs)
    {
        sh->nextBlinkUs += sh->tempo;
        sh->ledBaseVal = 0xFF;
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
            // Spawn an game note
            shGameNote_t* ni = heap_caps_calloc(1, sizeof(shGameNote_t), MALLOC_CAP_SPIRAM);
            ni->note         = sh->chartNotes[sh->currentChartNote].note;

            // Start the game note offscreen
            ni->headPosY = TFT_HEIGHT + (GAME_NOTE_HEIGHT / 2);

            // Save when the note should be hit
            ni->headTimeUs = nextEventUs;

            // If this is a hold note
            if (sh->chartNotes[sh->currentChartNote].hold)
            {
                // Start the tail offscreen too
                ni->tailPosY = TFT_HEIGHT + (GAME_NOTE_HEIGHT / 2);

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
        else if (gameNote->headPosY < -(GAME_NOTE_HEIGHT / 2))
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
            heap_caps_free(toRemove->val);
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
            heap_caps_free(toRemove->val);
            removeEntry(&sh->fretLines, toRemove);
        }
        else
        {
            // Iterate normally
            fretLineNode = fretLineNode->next;
        }
    }
    return true;
}

/**
 * @brief TODO doc
 *
 * @param sh
 */
void shDrawGame(shVars_t* sh)
{
    // Clear the display
    clearPxTft();

    // Draw fret lines first
    node_t* fretLineNode = sh->fretLines.first;
    while (fretLineNode)
    {
        shFretLine_t* fretLine = fretLineNode->val;
        drawLineFast(0, fretLine->headPosY, TFT_WIDTH, fretLine->headPosY, c111);
        fretLineNode = fretLineNode->next;
    }

    // Draw the target area
    drawLineFast(0, HIT_BAR, TFT_WIDTH - 1, HIT_BAR, c555);
    for (int32_t i = 0; i < sh->numFrets; i++)
    {
        int32_t margin  = 4;
        int32_t xOffset = ((i * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
        drawRect(xOffset - (GAME_NOTE_WIDTH / 2) - margin, HIT_BAR - (GAME_NOTE_HEIGHT / 2) - margin, //
                 xOffset + (GAME_NOTE_WIDTH / 2) + margin, HIT_BAR + (GAME_NOTE_HEIGHT / 2) + margin, c555);
    }

    // Draw all the game notes
    node_t* gameNoteNode = sh->gameNotes.first;
    while (gameNoteNode)
    {
        shGameNote_t* gameNote = gameNoteNode->val;
        int32_t xOffset        = getXoffset(sh, gameNote->note);

        // If there is a tail
        if (gameNote->tailPosY >= 0)
        {
            // Draw the tail
            fillDisplayArea(xOffset - 2, gameNote->headPosY, xOffset + 3, gameNote->tailPosY,
                            sh->colors[gameNote->note]);
        }

        // Draw the game note
        drawWsgTile(&sh->icons[sh->noteToIcon[gameNote->note]], xOffset - (GAME_NOTE_WIDTH / 2),
                    gameNote->headPosY - (GAME_NOTE_HEIGHT / 2));

        // Iterate
        gameNoteNode = gameNoteNode->next;
    }

    // Draw indicators that the button is pressed
    for (int32_t bIdx = 0; bIdx < sh->numFrets; bIdx++)
    {
        if (sh->btnState & sh->noteToBtn[bIdx])
        {
            int32_t margin  = 8;
            int32_t xOffset = ((bIdx * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
            drawRect(xOffset - (GAME_NOTE_WIDTH / 2) - margin, HIT_BAR - (GAME_NOTE_HEIGHT / 2) - margin, //
                     xOffset + (GAME_NOTE_WIDTH / 2) + margin, HIT_BAR + (GAME_NOTE_HEIGHT / 2) + margin,
                     sh->colors[bIdx]);
        }
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

    if (sh->failOn)
    {
        // Draw fail meter bar
        int32_t failMeterWidth = (sh->failMeter * TFT_WIDTH) / 100;
        fillDisplayArea(0, TFT_HEIGHT - failBarHeight, failMeterWidth, TFT_HEIGHT - 1, c440);
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
 * @brief TODO doc
 *
 * @param sh
 * @param evt
 */
void shGameInput(shVars_t* sh, buttonEvt_t* evt)
{
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

    sh->btnState = evt->state;

    if (PB_B < evt->button)
    {
        // TODO handle non-face buttons
        return;
    }

    int32_t notePressed = sh->btnToNote[31 - __builtin_clz(evt->button)];
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
                        ds->x          = getXoffset(sh, gameNote->note) - sh->star.w / 2;
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
                            heap_caps_free(gameNoteNode->val);
                            removeEntry(&sh->gameNotes, gameNoteNode);
                            gameNoteNode = nextNode;
                        }
                    }
                }
                else if (gameNote->held)
                {
                    // A held note was released. Remove it!
                    node_t* nextNode = gameNoteNode->next;
                    heap_caps_free(gameNoteNode->val);
                    removeEntry(&sh->gameNotes, gameNoteNode);
                    gameNoteNode = nextNode;
                }

                // the button was matched to an game note, break the loop
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

    // Check for analog touch
    // int32_t centerVal, intensityVal;
    // if (getTouchCentroid(&centerVal, &intensityVal))
    // {
    //     printf("touch center: %" PRId32 ", intensity: %" PRId32 "\n", centerVal, intensityVal);
    // }
    // else
    // {hold
    // }

    // // Get the acceleration
    // int16_t a_x, a_y, a_z;
    // accelGetAccelVec(&a_x, &a_y, &a_z);
}

/**
 * @brief TODO doc
 *
 * @param sh
 * @return int32_t
 */
static int32_t getMultiplier(shVars_t* sh)
{
    return (sh->combo + 10) / 10;
}

/**
 * @brief TODO doc
 *
 */
static void shSongOver(void)
{
    // Set a flag, end the game synchronously
    getShVars()->gameEnd = true;
}

/**
 * @brief TODO doc
 *
 * @param sh
 * @param baseScore
 */
static void shHitNote(shVars_t* sh, int32_t baseScore)
{
    sh->noteHistogram[ARRAY_SIZE(timings) - 1 - baseScore]++;

    // Increment the score
    sh->score += getMultiplier(sh) * baseScore;

    sh->notesHit++;

    // Increment the combo & fail meter
    sh->combo++;
    if (sh->combo > sh->maxCombo)
    {
        sh->maxCombo = sh->combo;
    }

    if (sh->failOn)
    {
        sh->failMeter = MIN(100, sh->failMeter + 1);
    }
}

/**
 * @brief TODO doc
 *
 * @param sh
 */
static void shMissNote(shVars_t* sh)
{
    sh->noteHistogram[ARRAY_SIZE(timings) - 1]++;

    sh->combo = 0;
    if (sh->failOn)
    {
        sh->failMeter = MAX(0, sh->failMeter - 2);
        if (0 == sh->failMeter)
        {
            sh->gameEnd = true;
        }
    }
}

/**
 * @brief TODO
 *
 * @param gradeIdx
 * @return const char*
 */
const char* getLetterGrade(int32_t gradeIdx)
{
    return grades[gradeIdx].letter;
}

/**
 * @brief TODO
 *
 * @param sh
 * @param note
 * @return int32_t
 */
static int32_t getXoffset(shVars_t* sh, int32_t note)
{
    return ((note * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));
}

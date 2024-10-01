//==============================================================================
// Includes
//==============================================================================

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

static const char hit_fantastic[] = "Fantastic";
static const char hit_marvelous[] = "Marvelous";
static const char hit_great[]     = "Great";
static const char hit_decent[]    = "Decent";
static const char hit_way_off[]   = "Way Off";
static const char hit_miss[]      = "Miss";

static const char hit_early[] = "Early";
static const char hit_late[]  = "Late";

//==============================================================================
// Function Declarations
//==============================================================================

static int32_t getMultiplier(shVars_t* sh);
static void shSongOver(void);
static void shMissNote(shVars_t* sh);

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
    // Don't immediately quit
    sh->gameEnd = false;

    // Load settings
    sh->failOn     = shGetSettingFail();
    sh->scrollTime = shGetSettingSpeed();

    // Load song
    size_t sz    = 0;
    sh->numFrets = 1 + shLoadChartData(sh, cnfsGetFile(chart, &sz), sz);

    if (6 == sh->numFrets)
    {
        sh->btnToNote  = btnToNote_h;
        sh->noteToBtn  = noteToBtn_h;
        sh->colors     = colors_h;
        sh->noteToIcon = noteToIcon_h;
    }
    else if (5 == sh->numFrets)
    {
        sh->btnToNote  = btnToNote_m;
        sh->noteToBtn  = noteToBtn_m;
        sh->colors     = colors_m;
        sh->noteToIcon = noteToIcon_m;
    }
    else if (4 == sh->numFrets)
    {
        sh->btnToNote  = btnToNote_e;
        sh->noteToBtn  = noteToBtn_e;
        sh->colors     = colors_e;
        sh->noteToIcon = noteToIcon_e;
    }

    // Make sure MIDI player is initialized
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Load the MIDI file
    loadMidiFile(midi, &sh->midiSong, true);

    // Set the file, but don't play yet
    midiPause(player, true);
    player->sampleCount = 0;
    midiSetFile(player, &sh->midiSong);

    // Seek to load the tempo and length, then reset
    midiSeek(player, -1);
    sh->tempo         = player->tempo;
    int32_t songLenUs = SAMPLES_TO_US(player->sampleCount); // TODO this is incorrect???
    globalMidiPlayerStop(true);

    // TODO not capturing NUM_FAIL_METER_SAMPLES points??
    sh->failSampleInterval = songLenUs / NUM_FAIL_METER_SAMPLES;

    printf("Song len us: %d, sample interval %d\n", songLenUs, sh->failSampleInterval);

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
    sh->failMeter = 50;
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

            // Use the hold time to see how long this note is held
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
 * @return true
 * @return false
 */
bool shRunTimers(shVars_t* sh, uint32_t elapsedUs)
{
    if (sh->gameEnd)
    {
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
            printf("sample at %d\n", songUs);
            // Save the sample to display on the game over screen
            push(&sh->failSamples, sh->failMeter);
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
            free(toRemove->val);
            removeEntry(&sh->gameNotes, toRemove);

            // Note that it was missed
            sh->hitText = hit_miss;
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
            free(toRemove->val);
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
 * @brief TODO
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
        int32_t xOffset        = ((gameNote->note * TFT_WIDTH) / sh->numFrets) + (TFT_WIDTH / (2 * sh->numFrets));

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

    // Draw text
    if (sh->textTimerUs > 0)
    {
        int16_t tWidth = textWidth(&sh->ibm, sh->hitText);
        drawText(&sh->ibm, c555, sh->hitText, (TFT_WIDTH - tWidth) / 2, 100);

        if ((hit_fantastic != sh->hitText) && (hit_miss != sh->hitText))
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

    if (evt->down)
    {
        // See if the button press matches a note
        bool noteMatch = false;

        // Iterate through all currently shown game notes
        node_t* gameNoteNode = sh->gameNotes.first;
        while (gameNoteNode)
        {
            shGameNote_t* gameNote = gameNoteNode->val;

            // If the game note matches the button
            if (gameNote->note == sh->btnToNote[31 - __builtin_clz(evt->button)])
            {
                noteMatch = true;

                // Find how off the timing is
                int32_t usOff = (songUs - gameNote->headTimeUs);

                // Set either early or late
                sh->timingText = (usOff > 0) ? hit_late : hit_early;

                // Find the absolute difference
                usOff = ABS(usOff);

                // Check if this button hit a note
                bool gameNoteHit = false;

                int32_t baseScore = 0;

                // Classify the time off
                if (usOff < 21500)
                {
                    sh->hitText = hit_fantastic;
                    gameNoteHit = true;
                    baseScore   = 5;
                }
                else if (usOff < 43000)
                {
                    sh->hitText = hit_marvelous;
                    gameNoteHit = true;
                    baseScore   = 4;
                }
                else if (usOff < 102000)
                {
                    sh->hitText = hit_great;
                    gameNoteHit = true;
                    baseScore   = 3;
                }
                else if (usOff < 135000)
                {
                    sh->hitText = hit_decent;
                    gameNoteHit = true;
                    baseScore   = 2;
                }
                else if (usOff < 180000)
                {
                    sh->hitText = hit_way_off;
                    gameNoteHit = true;
                    baseScore   = 1;
                }
                else
                {
                    sh->hitText = hit_miss;
                    // Break the combo
                    shMissNote(sh);
                }

                // Increment the score
                sh->score += getMultiplier(sh) * baseScore;

                // Set a timer to not show the text forever
                sh->textTimerUs = SH_TEXT_TIME;

                // If it was close enough to hit
                if (gameNoteHit)
                {
                    // Increment the combo
                    sh->combo++;
                    sh->failMeter = MIN(100, sh->failMeter + 1);

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

        if (false == noteMatch)
        {
            // Total miss
            sh->hitText = hit_miss;
            shMissNote(sh);

            // TODO ignore buttons not used for this difficulty
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
    // {hold
    // }

    // // Get the acceleration
    // int16_t a_x, a_y, a_z;
    // accelGetAccelVec(&a_x, &a_y, &a_z);
}

/**
 * @brief TODO
 *
 * @param sh
 * @return int32_t
 */
static int32_t getMultiplier(shVars_t* sh)
{
    return MIN(4, (sh->combo + 10) / 10);
}

/**
 * @brief TODO
 *
 */
static void shSongOver(void)
{
    // Set a flag, end the game synchronously
    getShVars()->gameEnd = true;
}

/**
 * @brief TODO
 *
 * @param sh
 */
static void shMissNote(shVars_t* sh)
{
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

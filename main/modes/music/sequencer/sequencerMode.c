//==============================================================================
// Includes
//==============================================================================

#include "menu.h"
#include "sequencerMode.h"
#include "sequencerGrid.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void sequencerEnterMode(void);
static void sequencerExitMode(void);
static void sequencerMainLoop(int64_t elapsedUs);
static void sequencerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void sequencerSongMenuCb(const char*, bool selected, uint32_t settingVal);
static void sequencerNoteMenuCb(const char*, bool selected, uint32_t settingVal);
static bool sequencerMidiCb(midiEvent_t* event);

//==============================================================================
// Const Variables
//==============================================================================

static const char sequencerName[] = "Sequencer";

static const char str_noteOptions[] = "Note Options";

static const char str_songOptions[] = "Song Options";

static const char str_songTempo[] = "Tempo: ";
static const char* tempoLabels[]
    = {"60",  "70",  "80",  "90",  "100", "110", "120", "130", "140", "150", "160", "170", "180",
       "190", "200", "210", "220", "230", "240", "250", "260", "270", "280", "290", "300"};
static const int32_t tempoVals[] = {60,  70,  80,  90,  100, 110, 120, 130, 140, 150, 160, 170, 180,
                                    190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300};

static const char str_songGrid[] = "Grid Lines: ";
static const char* gridLabels[]  = {"1/1", "1/2", "1/4", "1/8", "1/16"};
static const int32_t gridVals[]  = {1, 2, 4, 8, 16};

static const char str_songTimeSig[] = "Signature: ";
static const char* timeSigLabels[]  = {"2/4", "3/4", "4/4", "5/4"};
static const int32_t timeSigVals[]  = {2, 3, 4, 5};

static const char str_loop[]    = "Loop: ";
static const char* loopLabels[] = {"On", "Off"};
static const int32_t loopVals[] = {true, false};

static const char str_songEnd[] = "End Song Here";

static const char str_instrument[]    = "Instrument: ";
static const char* instrumentLabels[] = {"Piano", "Guitar", "Drums"};
static const int32_t instrumentVals[] = {1, 27, 99}; // TODO pick correct drum val
const char* const instrumentWsgs[]    = {"seq_piano.wsg", "seq_guitar.wsg", "seq_drums.wsg"};

static const char str_noteType[]    = "Note: ";
static const char* noteTypeLabels[] = {"Whole", "Half", "Quarter", "Eighth", "Sixteenth"};
static const int32_t noteTypeVals[] = {1, 2, 4, 8, 16};
const char* const noteWsgs[]
    = {"whole_note.wsg", "half_note.wsg", "quarter_note.wsg", "eighth_note.wsg", "sixteenth_note.wsg"};

swadgeMode_t sequencerMode = {
    .modeName                 = sequencerName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = sequencerEnterMode,
    .fnExitMode               = sequencerExitMode,
    .fnMainLoop               = sequencerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = sequencerBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

//==============================================================================
// Variables
//==============================================================================

static sequencerVars_t* sv;

//==============================================================================
// Functions
//==============================================================================

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void sequencerEnterMode(void)
{
    sv = calloc(1, sizeof(sequencerVars_t));

    sv->screen = SEQUENCER_SEQ;

    loadFont("ibm_vga8.font", &sv->ibm, false);

    sv->songMenu = initMenu(str_songOptions, sequencerSongMenuCb);

    settingParam_t sp_tempo = {
        .min = tempoVals[0],
        .max = tempoVals[ARRAY_SIZE(tempoVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->songMenu, str_songTempo, tempoLabels, tempoVals, ARRAY_SIZE(tempoVals), &sp_tempo,
                                 120);

    settingParam_t sp_grid = {
        .min = gridVals[0],
        .max = gridVals[ARRAY_SIZE(gridVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->songMenu, str_songGrid, gridLabels, gridVals, ARRAY_SIZE(gridVals), &sp_grid, 4);

    settingParam_t sp_timeSig = {
        .min = timeSigVals[0],
        .max = timeSigVals[ARRAY_SIZE(timeSigVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->songMenu, str_songTimeSig, timeSigLabels, timeSigVals, ARRAY_SIZE(timeSigVals),
                                 &sp_timeSig, 4);

    settingParam_t sp_loop = {
        .min = loopVals[0],
        .max = loopVals[ARRAY_SIZE(loopVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->songMenu, str_loop, loopLabels, loopVals, ARRAY_SIZE(loopVals), &sp_loop, 1);

    addSingleItemToMenu(sv->songMenu, str_songEnd);

    sv->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    //////////////////////////////////////////////////////////////////////////
    // Initialize wheel menu

    static const rectangle_t textRect = {
        .pos.x  = 15,
        .pos.y  = 0,
        .width  = TFT_WIDTH - 30,
        .height = 40,
    };
    sv->wheelRenderer = initWheelMenu(&sv->ibm, 90, &textRect);

    wheelMenuSetColor(sv->wheelRenderer, c555);

    sv->noteMenu = initMenu(str_noteOptions, sequencerNoteMenuCb);

    uint8_t noteMenuPos = 0;

    // Add instrument options
    for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
    {
        loadWsg(instrumentWsgs[i], &sv->instrumentWsgs[i], true);
    }
    settingParam_t sp_instrument = {
        .min = instrumentVals[0],
        .max = instrumentVals[ARRAY_SIZE(instrumentVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->noteMenu, str_instrument, instrumentLabels, instrumentVals,
                                 ARRAY_SIZE(instrumentVals), &sp_instrument, 1);
    wheelMenuSetItemInfo(sv->wheelRenderer, str_instrument, &sv->instrumentWsgs[0], noteMenuPos++, SCROLL_VERT);
    for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
    {
        wheelMenuSetItemInfo(sv->wheelRenderer, instrumentLabels[i], &sv->instrumentWsgs[i], i, NO_SCROLL);
    }

    // Add note type options
    for (uint32_t i = 0; i < ARRAY_SIZE(noteTypeVals); i++)
    {
        loadWsg(noteWsgs[i], &sv->noteWsgs[i], true);
    }
    settingParam_t sp_noteType = {
        .min = noteTypeVals[0],
        .max = noteTypeVals[ARRAY_SIZE(noteTypeVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->noteMenu, str_noteType, noteTypeLabels, noteTypeVals, ARRAY_SIZE(noteTypeVals),
                                 &sp_noteType, 4);
    wheelMenuSetItemInfo(sv->wheelRenderer, str_noteType, &sv->noteWsgs[2], noteMenuPos++, SCROLL_VERT);

    for (uint32_t i = 0; i < ARRAY_SIZE(noteTypeVals); i++)
    {
        wheelMenuSetItemInfo(sv->wheelRenderer, noteTypeLabels[i], &sv->noteWsgs[i], i, NO_SCROLL);
    }

    // Song defaults
    // TODO load from settings
    sv->songParams.grid    = 4;     // grid at quarter notes
    sv->songParams.loop    = false; // don't loop
    sv->songParams.songEnd = 480;   // one minute long
    sv->songParams.tempo   = 120;   // 120 bpm
    sv->songParams.timeSig = 4;     // 4/4 time

    sv->usPerBeat = (60 * 1000000) / sv->songParams.tempo;

    sv->noteParams.instrument = instrumentVals[0];
    sv->noteParams.type       = noteTypeVals[2];

    measureSequencerGrid(sv);

    // Init midi
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    // Configure MIDI for streaming
    player->mode              = MIDI_STREAMING;
    player->paused            = true;
    player->streamingCallback = sequencerMidiCb;

    // Hack to bring the cursor on screen
    sv->cursorPos.y = NUM_PIANO_KEYS / 2;
    buttonEvt_t evt = {
        .down   = true,
        .button = PB_UP,
        .state  = PB_UP,
    };
    sequencerGridButton(sv, &evt);
    evt.button = PB_DOWN;
    evt.state  = PB_DOWN;
    sequencerGridButton(sv, &evt);

    sv->gridOffset = sv->gridOffsetTarget;

#if !defined(__XTENSA__)
    sv->midiQueueMutex = OGCreateMutex();
#endif
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void sequencerExitMode(void)
{
    void* val;
    while ((val = pop(&sv->notes)))
    {
        free(val);
    }

#if !defined(__XTENSA__)
    OGLockMutex(sv->midiQueueMutex);
#endif
    while ((val = pop(&sv->midiQueue)))
    {
        free(val);
    }
#if !defined(__XTENSA__)
    OGUnlockMutex(sv->midiQueueMutex);
#endif

    freeFont(&sv->ibm);
    deinitMenuManiaRenderer(sv->menuRenderer);
    deinitMenu(sv->songMenu);

    deinitWheelMenu(sv->wheelRenderer);
    deinitMenu(sv->noteMenu);

    for (uint32_t i = 0; i < ARRAY_SIZE(noteTypeVals); i++)
    {
        freeWsg(&sv->noteWsgs[i]);
    }
    for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
    {
        freeWsg(&sv->instrumentWsgs[i]);
    }

#if !defined(__XTENSA__)
    OGDeleteMutex(sv->midiQueueMutex);
#endif

    free(sv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void sequencerMainLoop(int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && PB_START == evt.button)
        {
            if (SEQUENCER_MENU == sv->screen)
            {
                sv->screen = SEQUENCER_SEQ;
            }
            else
            {
                sv->screen = SEQUENCER_MENU;
            }
        }
        else
        {
            switch (sv->screen)
            {
                case SEQUENCER_MENU:
                {
                    sv->songMenu = menuButton(sv->songMenu, evt);
                    break;
                }
                case SEQUENCER_SEQ:
                {
                    sequencerGridButton(sv, &evt);
                    break;
                }
            }
        }
    }

    switch (sv->screen)
    {
        case SEQUENCER_MENU:
        {
            drawMenuMania(sv->songMenu, sv->menuRenderer, elapsedUs);
            break;
        }
        case SEQUENCER_SEQ:
        {
            // TODO
            sequencerGridTouch(sv);
            drawSequencerGrid(sv, elapsedUs);
            break;
        }
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void sequencerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void sequencerSongMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    printf("%s %s (%d)\n", selected ? "selected" : "scrolled to", label, settingVal);

    if (str_songTempo == label)
    {
        sv->songParams.tempo = settingVal;
        sv->usPerBeat        = (60 * 1000000) / sv->songParams.tempo;
    }
    else if (str_songGrid == label)
    {
        sv->songParams.grid = settingVal;
    }
    else if (str_songTimeSig == label)
    {
        sv->songParams.timeSig = settingVal;
    }
    else if (str_loop == label)
    {
        sv->songParams.loop = settingVal;
    }
    else if (str_songEnd == label)
    {
        sv->songParams.songEnd = settingVal;
    }

    measureSequencerGrid(sv);
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 * @param settingVal
 */
static void sequencerNoteMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (str_noteType == label)
        {
            wheelMenuSetItemIcon(sv->wheelRenderer, str_noteType, &sv->noteWsgs[__builtin_ctz(settingVal)]);
            sv->noteParams.type = settingVal;
        }
        else if (str_instrument == label)
        {
            sv->noteParams.instrument = settingVal;
            for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
            {
                if (instrumentVals[i] == settingVal)
                {
                    wheelMenuSetItemIcon(sv->wheelRenderer, str_instrument, &sv->instrumentWsgs[i]);
                    break;
                }
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param event
 * @return true
 * @return false
 */
static bool sequencerMidiCb(midiEvent_t* event)
{
    // If there is something in the midi queue, pop and return it
    bool retVal = false;
    midiEvent_t* qEvt;

#if !defined(__XTENSA__)
    OGLockMutex(sv->midiQueueMutex);
#endif
    if ((qEvt = pop(&sv->midiQueue)))
    {
        memcpy(event, qEvt, sizeof(midiEvent_t));
        free(qEvt);
        retVal = true;
    }
#if !defined(__XTENSA__)
    OGUnlockMutex(sv->midiQueueMutex);
#endif

    return retVal;
}

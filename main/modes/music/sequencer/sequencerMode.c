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

static void sequencerMenuCb(const char*, bool selected, uint32_t settingVal);
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

static const char str_songTimeSig[] = "Time Signature: ";
static const char* timeSigLabels[]  = {"2/4", "3/4", "4/4", "5/4"};
static const int32_t timeSigVals[]  = {2, 3, 4, 5};

static const char str_songLength[]    = "Length: ";
static const char* songLengthLabels[] = {"1 bar", "2 bars"};
static const int32_t songLengthVals[] = {1, 2};

static const char str_instrument[]    = "Instrument: ";
static const char* instrumentLabels[] = {"Piano", "Marimba", "Organ", "Guitar", "Bass", "Violin", "Trumpet", "Sax"};
static const int32_t instrumentVals[] = {1, 13, 20, 27, 39, 41, 57, 66};

static const char str_noteType[]    = "Type: ";
static const char* noteTypeLabels[] = {"Whole", "Half", "Quarter", "Eighth", "Sixteenth"};
static const int32_t noteTypeVals[] = {1, 2, 4, 8, 16};

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

    sv->songMenu = initMenu(str_songOptions, sequencerMenuCb);

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

    settingParam_t sp_songLength = {
        .min = songLengthVals[0],
        .max = songLengthVals[ARRAY_SIZE(songLengthVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->songMenu, str_songLength, songLengthLabels, songLengthVals, ARRAY_SIZE(songLengthVals),
                                 &sp_songLength, 1);


    sv->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    //////////////////////////////////////////////////////////////////////////
    // Attempt to initialize wheel menu

    static const rectangle_t textRect = {
        .pos.x = 15,
        .pos.y = 0,
        .width = TFT_WIDTH - 30,
        .height = 40,
    };
    sv->wheelRenderer = initWheelMenu(&sv->ibm, 0, &textRect);

    sv->noteMenu = initMenu(str_noteOptions, sequencerMenuCb);

    uint8_t noteMenuPos = 0;

    // Add instrument options
    settingParam_t sp_instrument = {
        .min = instrumentVals[0],
        .max = instrumentVals[ARRAY_SIZE(instrumentVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->noteMenu, str_instrument, instrumentLabels, instrumentVals, ARRAY_SIZE(instrumentVals),
                                 &sp_instrument, 1);
    wheelMenuSetItemInfo(sv->wheelRenderer, str_instrument, NULL, noteMenuPos++, SCROLL_VERT);
    wheelMenuSetItemTextIcon(sv->wheelRenderer, str_instrument, "I");
    for(uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
    {
        wheelMenuSetItemInfo(sv->wheelRenderer, instrumentLabels[i], NULL, i, NO_SCROLL);
        // wheelMenuSetItemTextIcon(sd->wheelMenu, itemInfo->label, itemInfo->shortLabel);
        // wheelMenuSetItemSize(sd->wheelMenu, itemInfo->label, -1, -1, WM_SHAPE_ROUNDED_RECT);
    }

    // Add note type options
    settingParam_t sp_noteType = {
        .min = noteTypeVals[0],
        .max = noteTypeVals[ARRAY_SIZE(noteTypeVals) - 1],
    };
    addSettingsOptionsItemToMenu(sv->noteMenu, str_noteType, noteTypeLabels, noteTypeVals, ARRAY_SIZE(noteTypeVals),
                                &sp_noteType, 4);
    wheelMenuSetItemInfo(sv->wheelRenderer, str_noteType, NULL, noteMenuPos++, SCROLL_VERT);
    wheelMenuSetItemTextIcon(sv->wheelRenderer, str_noteType, "NT");
    for(uint32_t i = 0; i < ARRAY_SIZE(noteTypeVals); i++)
    {
        wheelMenuSetItemInfo(sv->wheelRenderer, noteTypeLabels[i], NULL, i, NO_SCROLL);
        // wheelMenuSetItemTextIcon(sd->wheelMenu, itemInfo->label, itemInfo->shortLabel);
        // wheelMenuSetItemSize(sd->wheelMenu, itemInfo->label, -1, -1, WM_SHAPE_ROUNDED_RECT);
    }

    // Song defaults
    sv->usPerBeat = 500000; // 120bpm
    sv->numBars   = 8;      // 2 bar song
    sv->timeSig   = 4;      // 4/4 time
    sv->gridSize  = 4;      // Grid at quarter notes

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

    while ((val = pop(&sv->midiQueue)))
    {
        free(val);
    }
    freeFont(&sv->ibm);
    deinitMenuManiaRenderer(sv->menuRenderer);
    deinitMenu(sv->songMenu);

    deinitWheelMenu(sv->wheelRenderer);
    deinitMenu(sv->noteMenu);
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
static void sequencerMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    printf("%s %s\n", label, selected ? "selected" : "scrolled to");

    if (selected)
    {
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
    midiEvent_t* qEvt;
    if ((qEvt = pop(&sv->midiQueue)))
    {
        memcpy(event, qEvt, sizeof(midiEvent_t));
        free(qEvt);
        return true;
    }
    return false;
}

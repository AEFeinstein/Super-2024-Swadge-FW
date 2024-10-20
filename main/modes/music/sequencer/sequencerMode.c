//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "menu.h"
#include "sequencerMode.h"
#include "sequencerGrid.h"
#include "mainMenu.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void sequencerEnterMode(void);
static void sequencerExitMode(void);
static void sequencerMainLoop(int64_t elapsedUs);
static void sequencerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void sequencerSongMenuCb(const char*, bool selected, uint32_t settingVal);
static void sequencerNoteMenuCb(const char*, bool selected, uint32_t settingVal);

static void sequencerSaveSong(const char* fname);
static void sequencerLoadSong(const char* fname);
static bool sequencerIsSongSaved(const char* fname);

static void buildMainMenu(void);
static void setDefaultParameters(void);

//==============================================================================
// Const Variables
//==============================================================================

static const char sequencerName[] = "Sequencer";

static const char str_grid[] = "The Grid";
static const char str_help[] = "Help";
static const char str_exit[] = "Exit";

static const char str_file[]      = "File";
static const char str_load[]      = "Load";
static const char str_saveAs[]    = "Save As";
static const char str_overwrite[] = "Overwrite Save";
static const char str_cancel[]    = "Cancel";
static const char str_reset[]     = "Reset This Song";

static const char* const str_songNames[] = {
    "Song 1",
    "Song 2",
    "Song 3",
    "Song 4",
};

static const char str_noteOptions[] = "Note Options";

static const char str_songOptions[] = "Song Options";

static const char str_songTempo[] = "Tempo: ";
static const char* tempoLabels[]
    = {"60 bpm",  "70 bpm",  "80 bpm",  "90 bpm",  "100 bpm", "110 bpm", "120 bpm", "130 bpm", "140 bpm",
       "150 bpm", "160 bpm", "170 bpm", "180 bpm", "190 bpm", "200 bpm", "210 bpm", "220 bpm", "230 bpm",
       "240 bpm", "250 bpm", "260 bpm", "270 bpm", "280 bpm", "290 bpm", "300 bpm"};
static const int32_t tempoVals[] = {60,  70,  80,  90,  100, 110, 120, 130, 140, 150, 160, 170, 180,
                                    190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300};

static const char str_songGrid[] = "Grid: ";
static const char* gridLabels[]  = {"Whole", "Half", "Quarter", "Eighth", "Sixteenth"};
static const int32_t gridVals[]  = {1, 2, 4, 8, 16};

static const char str_songTimeSig[] = "Signature: ";
static const char* timeSigLabels[]  = {"2/4", "3/4", "4/4", "5/4", "6/4", "7/4"};
static const int32_t timeSigVals[]  = {2, 3, 4, 5, 6, 7};

static const char str_loop[]    = "Loop: ";
static const char* loopLabels[] = {"On", "Off"};
static const int32_t loopVals[] = {true, false};

static const char str_songEnd[] = "End Song Here";

static const char str_instrument[]             = "Instrument: ";
static const char* instrumentLabels[]          = {"Piano", "Guitar", "Drums"};
static const int32_t instrumentVals[]          = {0, 1, 2};
static const int32_t instrumentPrograms[]      = {0, 26, 13}; // TODO Pick better instruments
static const char* const instrumentWsgs[]      = {"seq_piano.wsg", "seq_guitar.wsg", "seq_drums.wsg"};
static const paletteColor_t instrumentColors[] = {c500, c050, c005};

static const char str_noteType[]    = "Note: ";
static const char* noteTypeLabels[] = {"Whole", "Half", "Quarter", "Eighth", "Sixteenth"};
static const int32_t noteTypeVals[] = {1, 2, 4, 8, 16};
static const char* const noteWsgs[]
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
    setFrameRateUs(16667);

    sv = calloc(1, sizeof(sequencerVars_t));

    sv->screen = SEQUENCER_MENU;

    loadFont("ibm_vga8.font", &sv->ibm, false);

    setDefaultParameters();

    // Build out the main menu
    buildMainMenu();

    sv->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    // Color the menu like Pixil
    led_t menuColor = {
        .r = 0x80,
        .g = 0x00,
        .b = 0x80,
    };
    static const paletteColor_t shadowColors[] = {c535, c524, c424, c314, c313, c302, c202, c203, c313, c413, c424};
    recolorMenuManiaRenderer(sv->menuRenderer, //
                             c202, c444, c000, // titleBgColor, titleTextColor, textOutlineColor
                             c333,             // bgColor
                             c521, c522,       // outerRingColor, innerRingColor
                             c000, c555,       // rowColor, rowTextColor
                             shadowColors, ARRAY_SIZE(shadowColors), menuColor);

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
    wheelMenuSetItemColor(sv->wheelRenderer, str_instrument, instrumentColors[0], instrumentColors[0]);

    for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
    {
        wheelMenuSetItemInfo(sv->wheelRenderer, instrumentLabels[i], &sv->instrumentWsgs[i], i, NO_SCROLL);
        wheelMenuSetItemColor(sv->wheelRenderer, instrumentLabels[i], instrumentColors[i], instrumentColors[i]);
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

    measureSequencerGrid(sv);

    // Init midi
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    // Configure MIDI for streaming
    player->mode              = MIDI_STREAMING;
    player->streamingCallback = NULL;

    midiGmOn(player);

    // Set each instrument
    for (int32_t ch = 0; ch < ARRAY_SIZE(instrumentVals); ch++)
    {
        midiSetProgram(player, ch, instrumentPrograms[ch]);
    }

    midiPause(player, false);

    // Start in the middle of the piano
    sv->cursorPos.y        = NUM_PIANO_KEYS / 2;
    sv->gridOffsetTarget.y = sv->rowHeight * (NUM_PIANO_KEYS - sv->numRows) / 2;
    sv->gridOffset         = sv->gridOffsetTarget;
}

/**
 * @brief Set the default song and note parameters
 */
static void setDefaultParameters(void)
{
    // Song defaults
    sv->songParams.grid    = gridVals[2];              // grid at quarter notes
    sv->songParams.loop    = loopVals[1];              // don't loop
    sv->songParams.tempo   = tempoVals[6];             // 120 bpm
    sv->songParams.songEnd = 4 * sv->songParams.tempo; // one minute long
    sv->songParams.timeSig = timeSigVals[2];           // 4/4 time
    sv->usPerBeat          = (60 * 1000000) / sv->songParams.tempo;

    // Note defaults
    sv->noteParams.channel = instrumentVals[0];
    sv->noteParams.type    = noteTypeVals[2];
}

/**
 * @brief Build out the menu, taking into account what is saved & loaded
 */
static void buildMainMenu(void)
{
    // Free the menu if it already exists
    if (sv->songMenu)
    {
        deinitMenu(sv->songMenu);
    }

    sv->songMenu = initMenu(sequencerName, sequencerSongMenuCb);

    // Start a submenu for file operations
    sv->songMenu = startSubMenu(sv->songMenu, str_file);
    {
        // Only add the "save" option if there's a loaded song already
        if (sv->loadedSong)
        {
            addSingleItemToMenu(sv->songMenu, sv->str_save);
        }

        // Start a submenu for save-as
        sv->songMenu = startSubMenu(sv->songMenu, str_saveAs);
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(str_songNames); sIdx++)
        {
            // If there's already a song in this slot
            if (sequencerIsSongSaved(str_songNames[sIdx]))
            {
                // Prompt the user to overwrite it
                sv->songMenu = startSubMenu(sv->songMenu, str_songNames[sIdx]);
                addSingleItemToMenu(sv->songMenu, str_cancel);
                addSingleItemToMenu(sv->songMenu, str_overwrite);
                sv->songMenu = endSubMenu(sv->songMenu);
            }
            else
            {
                addSingleItemToMenu(sv->songMenu, str_songNames[sIdx]);
            }
        }
        sv->songMenu = endSubMenu(sv->songMenu);

        // Load
        sv->songMenu = startSubMenu(sv->songMenu, str_load);
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(str_songNames); sIdx++)
        {
            // Check if song is saved first
            if (sequencerIsSongSaved(str_songNames[sIdx]))
            {
                addSingleItemToMenu(sv->songMenu, str_songNames[sIdx]);
            }
        }
        sv->songMenu = endSubMenu(sv->songMenu);

        // Add option to reset the current song
        addSingleItemToMenu(sv->songMenu, str_reset);
    }
    sv->songMenu = endSubMenu(sv->songMenu);

    sv->songMenu = startSubMenu(sv->songMenu, str_songOptions);
    {
        // Add option for tempo
        settingParam_t sp_tempo = {
            .min = tempoVals[0],
            .max = tempoVals[ARRAY_SIZE(tempoVals) - 1],
        };
        addSettingsOptionsItemToMenu(sv->songMenu, str_songTempo, tempoLabels, tempoVals, ARRAY_SIZE(tempoVals),
                                     &sp_tempo, sv->songParams.tempo);

        // Add option for grid marks
        settingParam_t sp_grid = {
            .min = gridVals[0],
            .max = gridVals[ARRAY_SIZE(gridVals) - 1],
        };
        addSettingsOptionsItemToMenu(sv->songMenu, str_songGrid, gridLabels, gridVals, ARRAY_SIZE(gridVals), &sp_grid,
                                     sv->songParams.grid);

        // Add option for time signature
        settingParam_t sp_timeSig = {
            .min = timeSigVals[0],
            .max = timeSigVals[ARRAY_SIZE(timeSigVals) - 1],
        };
        addSettingsOptionsItemToMenu(sv->songMenu, str_songTimeSig, timeSigLabels, timeSigVals, ARRAY_SIZE(timeSigVals),
                                     &sp_timeSig, sv->songParams.timeSig);

        // Add option for Looping
        settingParam_t sp_loop = {
            .min = loopVals[0],
            .max = loopVals[ARRAY_SIZE(loopVals) - 1],
        };
        addSettingsOptionsItemToMenu(sv->songMenu, str_loop, loopLabels, loopVals, ARRAY_SIZE(loopVals), &sp_loop,
                                     sv->songParams.loop);

        // Add option to mark the song end
        addSingleItemToMenu(sv->songMenu, str_songEnd);
    }
    sv->songMenu = endSubMenu(sv->songMenu);

    addSingleItemToMenu(sv->songMenu, str_grid);
    addSingleItemToMenu(sv->songMenu, str_help);
    addSingleItemToMenu(sv->songMenu, str_exit);
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void sequencerExitMode(void)
{
    // Autosave on exit, if a song is loaded
    if (sv->loadedSong)
    {
        sequencerSaveSong(sv->loadedSong);
    }
    else
    {
        // Try to find an unused slot and save in it
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(str_songNames); sIdx++)
        {
            if (!sequencerIsSongSaved(str_songNames[sIdx]))
            {
                sequencerSaveSong(str_songNames[sIdx]);
                break;
            }
        }
    }

    void* val;
    while ((val = pop(&sv->notes)))
    {
        free(val);
    }

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
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && PB_START == evt.button)
        {
            if (SEQUENCER_MENU == sv->screen)
            {
                globalMidiPlayerResumeAll();
                sv->screen = SEQUENCER_SEQ;
            }
            else if (SEQUENCER_SEQ == sv->screen)
            {
                globalMidiPlayerPauseAll();
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
                    if (sv->upOneMenuLevel)
                    {
                        sv->upOneMenuLevel = false;
                        sv->songMenu       = sv->songMenu->parentMenu;
                    }
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

    // If the menu is flagged to rebuild
    if (sv->rebuildMenu)
    {
        sv->rebuildMenu = false;
        // Rebuild it
        buildMainMenu();
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
            sequencerGridTouch(sv);
            runSequencerTimers(sv, elapsedUs);
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
    // Fill the background
    fillDisplayArea(x, y, x + w, y + h, c111);
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
    bool returnToGrid = false;

    // These menu options change when scrolling
    if (str_songTempo == label)
    {
        sv->songParams.tempo = settingVal;
        sv->usPerBeat        = (60 * 1000000) / sv->songParams.tempo;
        measureSequencerGrid(sv);
        returnToGrid = selected;
    }
    else if (str_songGrid == label)
    {
        sv->songParams.grid = settingVal;
        measureSequencerGrid(sv);
        returnToGrid = selected;
    }
    else if (str_songTimeSig == label)
    {
        sv->songParams.timeSig = settingVal;
        measureSequencerGrid(sv);
        returnToGrid = selected;
    }
    else if (str_loop == label)
    {
        sv->songParams.loop = settingVal;
        measureSequencerGrid(sv);
        returnToGrid = selected;
    }
    else if (selected)
    {
        // These menu options need to be selected, not just scrolled to
        if (str_songEnd == label)
        {
            sv->songParams.songEnd = sv->cursorPos.x;
            measureSequencerGrid(sv);
            returnToGrid = true;
        }
        else if (str_reset == label)
        {
            // reset track
            sv->loadedSong = NULL;
            void* val;
            while ((val = pop(&sv->notes)))
            {
                free(val);
            }
            returnToGrid = true;

            // Reset parameters
            setDefaultParameters();
            measureSequencerGrid(sv);
            sv->rebuildMenu = true;
        }
        else if (sv->str_save == label)
        {
            // Save with loaded label
            sequencerSaveSong(sv->loadedSong);
            sv->rebuildMenu = true;
            returnToGrid    = true;
        }
        else if (str_exit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (str_grid == label)
        {
            returnToGrid = true;
        }
        else if (str_help == label)
        {
            // TODO show help
        }
        else if (str_overwrite == label)
        {
            // Save the song
            sv->loadedSong = sv->songMenu->title;
            sprintf(sv->str_save, "Save %s", sv->loadedSong);
            sequencerSaveSong(sv->loadedSong);
            sv->rebuildMenu = true;
            returnToGrid    = true;
        }
        else if (str_cancel == label)
        {
            // Flag to go one level up
            sv->upOneMenuLevel = true;
        }
        else // This checks song name labels in a loop
        {
            // Check if a song for saving or loading was selected
            for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(str_songNames); sIdx++)
            {
                if (str_songNames[sIdx] == label)
                {
                    if (str_load == sv->songMenu->title)
                    {
                        sv->loadedSong = label;
                        sprintf(sv->str_save, "Save %s", sv->loadedSong);
                        sequencerLoadSong(sv->loadedSong);
                        sv->rebuildMenu = true;
                        returnToGrid    = true;
                    }
                    else if (str_saveAs == sv->songMenu->title)
                    {
                        // If this is the Save As menu and there's no overwrite warning
                        menuItem_t* cItem = sv->songMenu->currentItem->val;
                        if (NULL == cItem->subMenu)
                        {
                            // Save the song
                            sv->loadedSong = label;
                            sprintf(sv->str_save, "Save %s", sv->loadedSong);
                            sequencerSaveSong(sv->loadedSong);
                            sv->rebuildMenu = true;
                            returnToGrid    = true;
                        }
                    }
                    break;
                }
            }
        }
    }

    if (returnToGrid)
    {
        sv->screen = SEQUENCER_SEQ;
    }
}

/**
 * @brief Menu callback for the touch wheel note options menu
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
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
            sv->noteParams.channel = settingVal;
            for (uint32_t i = 0; i < ARRAY_SIZE(instrumentVals); i++)
            {
                if (instrumentVals[i] == settingVal)
                {
                    wheelMenuSetItemIcon(sv->wheelRenderer, str_instrument, &sv->instrumentWsgs[i]);
                    wheelMenuSetItemColor(sv->wheelRenderer, str_instrument, instrumentColors[i], instrumentColors[i]);
                    break;
                }
            }
        }
    }
}

/**
 * @brief Return the background color for a given channel (instrument)
 *
 * @param channel The channel to get a color for
 * @return The color of the channel
 */
paletteColor_t getChannelColor(int32_t channel)
{
    return instrumentColors[channel];
}

/**
 * @brief Save the current song to NVM
 *
 * @param fname The filename to save the song as
 */
static void sequencerSaveSong(const char* fname)
{
    // Figure out how big the blob will be
    size_t blobSize =              //
        sizeof(sv->songParams) +   //
        sizeof(sv->noteParams) +   //
        sizeof(sv->notes.length) + //
        (sizeof(sequencerNote_t) * sv->notes.length);

    // Allocate a blob
    uint8_t* blob    = heap_caps_calloc(1, blobSize, MALLOC_CAP_SPIRAM);
    uint32_t blobIdx = 0;

    // Copy song parameters into the blob
    memcpy(&blob[blobIdx], &sv->songParams, sizeof(sv->songParams));
    blobIdx += sizeof(sv->songParams);

    // Copy note parameters into the blob
    memcpy(&blob[blobIdx], &sv->noteParams, sizeof(sv->noteParams));
    blobIdx += sizeof(sv->noteParams);

    // Copy the number of notes into the blob
    memcpy(&blob[blobIdx], &sv->notes.length, sizeof(sv->notes.length));
    blobIdx += sizeof(sv->notes.length);

    // Copy the notes into the blob
    node_t* noteNode = sv->notes.first;
    while (noteNode)
    {
        memcpy(&blob[blobIdx], noteNode->val, sizeof(sequencerNote_t));
        blobIdx += sizeof(sequencerNote_t);
        noteNode = noteNode->next;
    }

    // Write the blob
    writeNvsBlob(fname, blob, blobSize);

    // Free the blob
    free(blob);
}

/**
 * @brief Check if a song is saved with the given name
 *
 * @param fname The filename to check
 * @return true if there's a saved song with this name, false if there isn't
 */
static bool sequencerIsSongSaved(const char* fname)
{
    size_t blobSize;
    return readNvsBlob(fname, NULL, &blobSize);
}

/**
 * @brief Load a song from NVM
 *
 * @param fname The name of the song to load
 */
static void sequencerLoadSong(const char* fname)
{
    // Get the blob size first and make sure the song exists
    size_t blobSize;
    if (readNvsBlob(fname, NULL, &blobSize))
    {
        // Read the blob from NVM
        uint8_t* blob = heap_caps_calloc(1, blobSize, MALLOC_CAP_SPIRAM);
        readNvsBlob(fname, blob, &blobSize);
        uint32_t blobIdx = 0;

        // Read song parameters from the blob
        memcpy(&sv->songParams, &blob[blobIdx], sizeof(sv->songParams));
        blobIdx += sizeof(sv->songParams);

        // Recalculate after load
        sv->usPerBeat = (60 * 1000000) / sv->songParams.tempo;
        measureSequencerGrid(sv);

        // Read note parameters from the blob
        memcpy(&sv->noteParams, &blob[blobIdx], sizeof(sv->noteParams));
        blobIdx += sizeof(sv->noteParams);

        // Read number of notes from the blob
        int numNotes = 0;
        memcpy(&numNotes, &blob[blobIdx], sizeof(sv->notes.length));
        blobIdx += sizeof(sv->notes.length);

        // Clear out current notes, just in case
        void* val;
        while ((val = pop(&sv->notes)))
        {
            free(val);
        }

        // Read notes from the blob
        for (int32_t nIdx = 0; nIdx < numNotes; nIdx++)
        {
            sequencerNote_t* newNote = heap_caps_calloc(1, sizeof(sequencerNote_t), MALLOC_CAP_SPIRAM);
            memcpy(newNote, &blob[blobIdx], sizeof(sequencerNote_t));
            blobIdx += sizeof(sequencerNote_t);
            push(&sv->notes, newNote);
        }

        // Free the blob
        free(blob);
    }
}

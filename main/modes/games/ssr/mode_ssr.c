#include "limits.h"
#include "menu.h"
#include "mode_ssr.h"

typedef struct
{
    int32_t tick;
    int32_t note;
    int32_t hold;
} ssrNote_t;

typedef struct
{
    font_t ibm;
    midiFile_t credits;
    ssrNote_t* notes;
    int32_t tDiv;
    int32_t tempo;
    int32_t cNote;
    int32_t tElapsedMs;
    int32_t draw;
} ssrVars_t;

ssrVars_t* ssr;

static void ssrEnterMode(void);
static void ssrExitMode(void);
static void ssrMainLoop(int64_t elapsedUs);
static void ssrBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void loadTrackData(ssrVars_t* ssr, const uint8_t* data, size_t size);

// static void ssrMenuCb(const char*, bool selected, uint32_t settingVal);

static const char ssrName[] = "Swadge Swadge Rebellion";

swadgeMode_t ssrMode = {
    .modeName                 = ssrName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ssrEnterMode,
    .fnExitMode               = ssrExitMode,
    .fnMainLoop               = ssrMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = ssrBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void ssrEnterMode(void)
{
    ssr = calloc(1, sizeof(ssrVars_t));
    loadFont("ibm_vga8.font", &ssr->ibm, false);

    size_t sz      = 0;
    const uint8_t* notes = cnfsGetFile("credits.cch", &sz);
    loadTrackData(ssr, notes, sz);

    loadMidiFile("credits.mid", &ssr->credits, false);

    globalMidiPlayerPlaySongCb(&ssr->credits, MIDI_BGM, NULL);

    ssr->tDiv  = ssr->credits.timeDivision;
    ssr->tempo = globalMidiPlayerGet(MIDI_BGM)->tempo;
    printf("%d @ %d\n", ssr->tDiv, ssr->tempo);
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void ssrExitMode(void)
{
    unloadMidiFile(&ssr->credits);
    free(ssr->notes);
    freeFont(&ssr->ibm);
    free(ssr);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void ssrMainLoop(int64_t elapsedUs)
{
    // What a hack!
    // ssr->tDiv  = ssr->credits.timeDivision;
    // ssr->tempo = globalMidiPlayerGet(MIDI_BGM). / 1000;


    ssr->tElapsedMs += (elapsedUs / 1000);

    // int nextMs = (ssr->notes[ssr->cNote].tick * ssr->tempo) / (ssr->tDiv);
    // if (ssr->tElapsedMs >= nextMs)
    // {
    //     ssr->draw = ssr->notes[ssr->cNote].note;
    //     printf("Note %d (%d, %d)\n", ssr->notes[ssr->cNote].note, nextMs, ssr->tElapsedMs);
    //     ssr->cNote++;
    // }

    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    int ticks = SAMPLES_TO_MIDI_TICKS(player->sampleCount, player->tempo, player->reader.division);
    int nextTick = ssr->notes[ssr->cNote].tick;
    if (ticks >= nextTick)
    {
        ssr->draw = ssr->notes[ssr->cNote].note;
        // printf("Note %d (%d, %d)\n", ssr->notes[ssr->cNote].note, nextMs, ssr->tElapsedMs);
        ssr->cNote++;
    }

    clearPxTft();

    paletteColor_t colors[] = {c020, c400, c550, c004, c420};
    // for(int i = 0; i < 5; i++)
    // {
    //     fillDisplayArea(i * (TFT_WIDTH / 5), 0, (i + 1) * (TFT_WIDTH / 5), TFT_HEIGHT, colors[i]);
    // }

    fillDisplayArea(ssr->draw * (TFT_WIDTH / 5), 0, (ssr->draw + 1) * (TFT_WIDTH / 5), TFT_HEIGHT, colors[ssr->draw]);

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        ; // DO SOMETHING
    }

    // ssr->credits.tracks

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
static void ssrBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
// static void ssrMenuCb(const char* label, bool selected, uint32_t settingVal)
// {
//     printf("%s %s\n", label, selected ? "selected" : "scrolled to");

//     if (selected)
//     {
//     }
// }

/**
 * @brief TODO
 *
 * @param data
 * @param size
 */
static void loadTrackData(ssrVars_t* ssrv, const uint8_t* data, size_t size)
{
    uint32_t dIdx     = 0;
    uint16_t numNotes = (data[dIdx++] << 8);
    numNotes |= (data[dIdx++]);

    ssrv->notes = calloc(numNotes, sizeof(ssrNote_t));

    for (int32_t nIdx = 0; nIdx < numNotes; nIdx++)
    {
        ssrv->notes[nIdx].tick = (data[dIdx + 0] << 24) | //
                                 (data[dIdx + 1] << 16) | //
                                 (data[dIdx + 2] << 8) |  //
                                 (data[dIdx + 3] << 0);
        dIdx += 4;

        ssrv->notes[nIdx].note = data[dIdx++];

        if (0x80 & ssrv->notes[nIdx].note)
        {
            printf("HOLD!\n");
            ssrv->notes[nIdx].note &= 0x7F;
            ssrv->notes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                     (data[dIdx + 1] << 0);
            dIdx += 2;
        }

        printf("%d %d %d\n", ssrv->notes[nIdx].tick, ssrv->notes[nIdx].note, ssrv->notes[nIdx].hold);
    }
}

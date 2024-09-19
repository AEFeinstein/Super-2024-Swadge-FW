//==============================================================================
// Defines
//==============================================================================

#include "mode_ssr.h"

//==============================================================================
// Defines
//==============================================================================

#define HIT_BAR     16
#define ICON_RADIUS 8

#define TRAVEL_TIME_US   2000000
#define TRAVEL_US_PER_PX ((TRAVEL_TIME_US) / (TFT_HEIGHT - HIT_BAR + (2 * ICON_RADIUS)))

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t tick;
    int32_t note;
    int32_t hold;
} ssrNote_t;

typedef struct
{
    int32_t note;
    int32_t timer;
    int32_t headPosY;
    int32_t tailPosY;
} ssrNoteIcon_t;

typedef struct
{
    // Font
    font_t ibm;

    // Song being played
    midiFile_t credits;
    int32_t leadInUs;

    // Track data
    int32_t numNotes;
    ssrNote_t* notes;
    int32_t cNote;

    // Drawing data
    list_t icons;
    buttonBit_t btnState;
} ssrVars_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void ssrEnterMode(void);
static void ssrExitMode(void);
static void ssrMainLoop(int64_t elapsedUs);
static void ssrBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ssrLoadTrackData(ssrVars_t* ssr, const uint8_t* data, size_t size);
// static void ssrMenuCb(const char*, bool selected, uint32_t settingVal);

//==============================================================================
// Variables
//==============================================================================

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

ssrVars_t* ssr;

static const paletteColor_t colors[] = {c020, c400, c550, c004, c420};

static const int32_t btnMap[][2] = {
    {
        PB_LEFT,
        0,
    },
    {
        PB_DOWN,
        1,
    },
    {
        PB_UP,
        2,
    },
    {
        PB_RIGHT,
        3,
    },
    {
        PB_B,
        4,
    },
    {
        PB_A,
        5,
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void ssrEnterMode(void)
{
    // 60FPS please
    setFrameRateUs(16667);

    // Allocate mode memory
    ssr = calloc(1, sizeof(ssrVars_t));

    // Load a font
    loadFont("ibm_vga8.font", &ssr->ibm, false);

    // Load the track data
    size_t sz = 0;
    ssrLoadTrackData(ssr, cnfsGetFile("credits.cch", &sz), sz);

    // Load the MIDI file
    loadMidiFile("credits.mid", &ssr->credits, false);
    globalMidiPlayerPlaySong(&ssr->credits, MIDI_BGM);
    globalMidiPlayerPauseAll();

    // Set the lead-in timer
    ssr->leadInUs = TRAVEL_TIME_US;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void ssrExitMode(void)
{
    // Free MIDI data
    unloadMidiFile(&ssr->credits);

    // Free track data
    free(ssr->notes);

    // Free UI data
    void* val;
    while ((val = pop(&ssr->icons)))
    {
        free(val);
    }

    // Free the font
    freeFont(&ssr->ibm);

    // Free mode memory
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
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        ssr->btnState = evt.state;
        for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(btnMap); bIdx++)
        {
            if (evt.button & btnMap[bIdx][0])
            {
                printf("%d %s\n", btnMap[bIdx][1], evt.down ? "Down" : "Up");
            }
        }

        // if (evt.down)
        // {
        //     switch (evt.button)
        //     {
        //         case PB_UP:
        //         case PB_DOWN:
        //         case PB_LEFT:
        //         case PB_RIGHT:
        //         case PB_A:
        //         case PB_B:
        //         case PB_START:
        //         case PB_SELECT:
        //         default:
        //         {
        //             break;
        //         }
        //     }
        // }
    }

    // Run a lead-in timer to allow notes to spawn before the song starts playing
    if (ssr->leadInUs > 0)
    {
        ssr->leadInUs -= elapsedUs;

        if (ssr->leadInUs <= 0)
        {
            globalMidiPlayerResumeAll();
            ssr->leadInUs = 0;
        }
    }

    // Get a reference to the player
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);

    // Get the position of the song and when the next event is, in ms
    int32_t songUs;
    if (ssr->leadInUs > 0)
    {
        songUs = -ssr->leadInUs;
    }
    else
    {
        songUs = SAMPLES_TO_US(player->sampleCount);
    }

    // Check events until one hasn't happened yet or the song ends
    while (ssr->cNote < ssr->numNotes)
    {
        // When the next event occurs
        int32_t nextEventUs = MIDI_TICKS_TO_US(ssr->notes[ssr->cNote].tick, player->tempo, player->reader.division);

        // Check if the icon should be spawned now to reach the hit bar in time
        if (songUs + TRAVEL_TIME_US >= nextEventUs)
        {
            // Spawn an icon
            ssrNoteIcon_t* ni = calloc(1, sizeof(ssrNoteIcon_t));
            ni->note          = ssr->notes[ssr->cNote].note;
            ni->headPosY      = TFT_HEIGHT + (ICON_RADIUS * 2);

            // If this is a hold note
            if (ssr->notes[ssr->cNote].hold)
            {
                // Figure out at what microsecond the tail ends
                int32_t tailUs = MIDI_TICKS_TO_US(ssr->notes[ssr->cNote].hold, player->tempo, player->reader.division);
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

            // Push into the list of icons
            push(&ssr->icons, ni);

            // Increment the track data
            ssr->cNote++;
        }
        else
        {
            // Nothing more to be spawned right now
            break;
        }
    }

    // Clear the display
    clearPxTft();

    // Draw the target area
    drawLineFast(0, HIT_BAR, TFT_WIDTH - 1, HIT_BAR, c555);
    for (int32_t i = 0; i < 5; i++)
    {
        int32_t xOffset = ((i * TFT_WIDTH) / 5) + (TFT_WIDTH / 10);
        drawCircle(xOffset, HIT_BAR, ICON_RADIUS + 2, c555);
    }

    // Draw all the icons
    node_t* iconNode = ssr->icons.first;
    while (iconNode)
    {
        // Draw the icon
        ssrNoteIcon_t* icon = iconNode->val;
        int32_t xOffset     = ((icon->note * TFT_WIDTH) / 5) + (TFT_WIDTH / 10);
        drawCircleFilled(xOffset, icon->headPosY, ICON_RADIUS, colors[icon->note]);
        // If there is a tail
        if (icon->tailPosY >= 0)
        {
            // Draw the tail
            fillDisplayArea(xOffset - 2, icon->headPosY, xOffset + 3, icon->tailPosY, colors[icon->note]);
        }

        // Highlight the target if the timing is good enough
        // if (HIT_BAR - 4 <= icon->posY && icon->posY <= HIT_BAR + 4)
        // {
        //     drawCircleOutline(xOffset, HIT_BAR, ICON_RADIUS + 8, 4, colors[icon->note]);
        // }

        // Track if this icon gets removed
        bool removed = false;

        // Run this icon's timer
        icon->timer += elapsedUs;
        while (icon->timer >= TRAVEL_US_PER_PX)
        {
            icon->timer -= TRAVEL_US_PER_PX;

            // Move the whole icon up
            icon->headPosY--;
            if (icon->tailPosY >= 0)
            {
                icon->tailPosY--;
            }

            // If it's off screen
            if (icon->headPosY < -ICON_RADIUS && (icon->tailPosY < 0))
            {
                // Remove this icon
                node_t* nodeToRemove = iconNode;
                iconNode             = iconNode->next;
                free(nodeToRemove->val);
                removeEntry(&ssr->icons, nodeToRemove);
                removed = true;
                break;
            }
        }

        // If this icon wasn't removed
        if (!removed)
        {
            // Iterate to the next
            iconNode = iconNode->next;
        }
    }

    // Draw indicators that the button is pressed
    for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(btnMap); bIdx++)
    {
        if (ssr->btnState & btnMap[bIdx][0])
        {
            int32_t xOffset = ((bIdx * TFT_WIDTH) / 5) + (TFT_WIDTH / 10);
            drawCircleOutline(xOffset, HIT_BAR, ICON_RADIUS + 8, 4, colors[bIdx]);
        }
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
static void ssrLoadTrackData(ssrVars_t* ssrv, const uint8_t* data, size_t size)
{
    uint32_t dIdx  = 0;
    ssrv->numNotes = (data[dIdx++] << 8);
    ssrv->numNotes |= (data[dIdx++]);

    ssrv->notes = calloc(ssrv->numNotes, sizeof(ssrNote_t));

    for (int32_t nIdx = 0; nIdx < ssrv->numNotes; nIdx++)
    {
        ssrv->notes[nIdx].tick = (data[dIdx + 0] << 24) | //
                                 (data[dIdx + 1] << 16) | //
                                 (data[dIdx + 2] << 8) |  //
                                 (data[dIdx + 3] << 0);
        dIdx += 4;

        ssrv->notes[nIdx].note = data[dIdx++];

        if (0x80 & ssrv->notes[nIdx].note)
        {
            ssrv->notes[nIdx].note &= 0x7F;

            // Use the hold time to see when this note ends
            ssrv->notes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                     (data[dIdx + 1] << 0);
            dIdx += 2;
        }
    }
}

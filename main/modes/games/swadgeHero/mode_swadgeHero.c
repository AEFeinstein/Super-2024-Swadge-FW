//==============================================================================
// Defines
//==============================================================================

#include "mode_swadgeHero.h"

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
} shNote_t;

typedef struct
{
    int32_t note;
    int32_t timer;
    int32_t headPosY;
    int32_t tailPosY;
    bool held;
} shNoteIcon_t;

typedef struct
{
    // Font
    font_t ibm;

    // Song being played
    midiFile_t credits;
    int32_t leadInUs;

    // Track data
    int32_t numNotes;
    shNote_t* notes;
    int32_t cNote;

    // Drawing data
    list_t icons;
    buttonBit_t btnState;
} shVars_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void shEnterMode(void);
static void shExitMode(void);
static void shMainLoop(int64_t elapsedUs);
static void shBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void shLoadTrackData(shVars_t* sh, const uint8_t* data, size_t size);
// static void shMenuCb(const char*, bool selected, uint32_t settingVal);
static uint32_t btnToNote(buttonBit_t btn);
static void shRunTimers(shVars_t* shv, uint32_t elapsedUs);
static void shDrawGame(shVars_t* shv);

//==============================================================================
// Variables
//==============================================================================

static const char shName[] = "Swadge Hero";

swadgeMode_t swadgeHeroMode = {
    .modeName                 = shName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = shEnterMode,
    .fnExitMode               = shExitMode,
    .fnMainLoop               = shMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = shBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

shVars_t* sh;

static const paletteColor_t colors[] = {c020, c400, c550, c004, c420, c222};
static const buttonBit_t noteToBtn[] = {PB_LEFT, PB_DOWN, PB_UP, PB_RIGHT, PB_B, PB_A};

//==============================================================================
// Functions
//==============================================================================

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void shEnterMode(void)
{
    // 60FPS please
    setFrameRateUs(16667);

    // Allocate mode memory
    sh = calloc(1, sizeof(shVars_t));

    // Load a font
    loadFont("ibm_vga8.font", &sh->ibm, false);

    // Load the track data
    size_t sz = 0;
    shLoadTrackData(sh, cnfsGetFile("credits.cch", &sz), sz);

    // Load the MIDI file
    loadMidiFile("credits.mid", &sh->credits, false);
    globalMidiPlayerPlaySong(&sh->credits, MIDI_BGM);
    globalMidiPlayerPauseAll();

    // Set the lead-in timer
    sh->leadInUs = TRAVEL_TIME_US;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void shExitMode(void)
{
    // Free MIDI data
    unloadMidiFile(&sh->credits);

    // Free track data
    free(sh->notes);

    // Free UI data
    void* val;
    while ((val = pop(&sh->icons)))
    {
        free(val);
    }

    // Free the font
    freeFont(&sh->ibm);

    // Free mode memory
    free(sh);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void shMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        sh->btnState = evt.state;

        if (evt.down)
        {
            // Iterate through all currently shown icons
            node_t* iconNode = sh->icons.first;
            while (iconNode)
            {
                shNoteIcon_t* icon = iconNode->val;

                // If the icon matches the button
                if (icon->note == btnToNote(evt.button))
                {
                    // Find how off the timing is
                    int32_t pxOff = ABS(HIT_BAR - icon->headPosY);
                    int32_t usOff = pxOff * TRAVEL_US_PER_PX;
                    printf("%" PRId32 " us off\n", usOff);

                    // Check if this button hit a note
                    bool iconHit = false;

                    // Classify the time off
                    if (usOff < 21500)
                    {
                        printf("  Fantastic\n");
                        iconHit = true;
                    }
                    else if (usOff < 43000)
                    {
                        printf("  Marvelous\n");
                        iconHit = true;
                    }
                    else if (usOff < 102000)
                    {
                        printf("  Great\n");
                        iconHit = true;
                    }
                    else if (usOff < 135000)
                    {
                        printf("  Decent\n");
                        iconHit = true;
                    }
                    else if (usOff < 180000)
                    {
                        printf("  Way Off\n");
                        iconHit = true;
                    }
                    else
                    {
                        printf("  MISS\n");
                    }

                    // If it was close enough to hit
                    if (iconHit)
                    {
                        if (icon->tailPosY >= 0)
                        {
                            // There is a tail, don't remove the note yet
                            icon->headPosY = HIT_BAR;
                            icon->held     = true;
                        }
                        else
                        {
                            // No tail, remove the icon
                            node_t* nextNode = iconNode->next;
                            removeEntry(&sh->icons, iconNode);
                            iconNode = nextNode;
                        }
                    }

                    // the button was matched to an icon, break the loop
                    break;
                }

                // Iterate to the next icon
                iconNode = iconNode->next;
            }
        }
        else
        {
            // TODO handle ups when holding tails
        }
    }

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
    while (sh->cNote < sh->numNotes)
    {
        // When the next event occurs
        int32_t nextEventUs = MIDI_TICKS_TO_US(sh->notes[sh->cNote].tick, player->tempo, player->reader.division);

        // Check if the icon should be spawned now to reach the hit bar in time
        if (songUs + TRAVEL_TIME_US >= nextEventUs)
        {
            // Spawn an icon
            shNoteIcon_t* ni = calloc(1, sizeof(shNoteIcon_t));
            ni->note          = sh->notes[sh->cNote].note;
            ni->headPosY      = TFT_HEIGHT + (ICON_RADIUS * 2);

            // If this is a hold note
            if (sh->notes[sh->cNote].hold)
            {
                // Figure out at what microsecond the tail ends
                int32_t tailUs = MIDI_TICKS_TO_US(sh->notes[sh->cNote].hold, player->tempo, player->reader.division);
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
            push(&sh->icons, ni);

            // Increment the track data
            sh->cNote++;
        }
        else
        {
            // Nothing more to be spawned right now
            break;
        }
    }

    shRunTimers(sh, elapsedUs);
    shDrawGame(sh);

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
 * @brief TODO
 *
 * @param shv
 * @param elapsedUs
 */
static void shRunTimers(shVars_t* shv, uint32_t elapsedUs)
{
    // Track if an icon was removed
    bool removed = false;

    // Run all the icon timers
    node_t* iconNode = shv->icons.first;
    while (iconNode)
    {
        // Get a reference
        shNoteIcon_t* icon = iconNode->val;

        // Run this icon's timer
        icon->timer += elapsedUs;
        while (icon->timer >= TRAVEL_US_PER_PX)
        {
            icon->timer -= TRAVEL_US_PER_PX;

            bool shouldRemove = false;

            // Move the whole icon up
            if (!icon->held)
            {
                icon->headPosY--;
                if (icon->tailPosY >= 0)
                {
                    icon->tailPosY--;
                }

                // If it's off screen
                if (icon->headPosY < -ICON_RADIUS && (icon->tailPosY < 0))
                {
                    // Mark it for removal
                    shouldRemove = true;
                }
            }
            else // The icon is being held
            {
                // Only move the tail position
                if (icon->tailPosY >= HIT_BAR)
                {
                    icon->tailPosY--;
                }

                // If the tail finished
                if (icon->tailPosY < HIT_BAR)
                {
                    // Mark it for removal
                    shouldRemove = true;
                }
            }

            // If the icon should be removed
            if (shouldRemove)
            {
                // Remove this icon
                free(iconNode->val);
                removeEntry(&shv->icons, iconNode);

                // Stop the while timer loop
                removed = true;
                break;
            }
        }

        // If an icon was removed
        if (removed)
        {
            // Stop iterating through notes
            break;
        }
        else
        {
            // Iterate to the next
            iconNode = iconNode->next;
        }
    }
}

/**
 * @brief TODO
 *
 * @param shv
 */
static void shDrawGame(shVars_t* shv)
{
    // Clear the display
    clearPxTft();

    // Draw the target area
    drawLineFast(0, HIT_BAR, TFT_WIDTH - 1, HIT_BAR, c555);
    for (int32_t i = 0; i < ARRAY_SIZE(noteToBtn); i++)
    {
        int32_t xOffset = ((i * TFT_WIDTH) / ARRAY_SIZE(noteToBtn)) + (TFT_WIDTH / 10);
        drawCircle(xOffset, HIT_BAR, ICON_RADIUS + 2, c555);
    }

    // Draw all the icons
    node_t* iconNode = shv->icons.first;
    while (iconNode)
    {
        // Draw the icon
        shNoteIcon_t* icon = iconNode->val;
        int32_t xOffset     = ((icon->note * TFT_WIDTH) / ARRAY_SIZE(noteToBtn)) + (TFT_WIDTH / 10);
        drawCircleFilled(xOffset, icon->headPosY, ICON_RADIUS, colors[icon->note]);

        // If there is a tail
        if (icon->tailPosY >= 0)
        {
            // Draw the tail
            fillDisplayArea(xOffset - 2, icon->headPosY, xOffset + 3, icon->tailPosY, colors[icon->note]);
        }

        // Iterate
        iconNode = iconNode->next;
    }

    // Draw indicators that the button is pressed
    for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(noteToBtn); bIdx++)
    {
        if (shv->btnState & noteToBtn[bIdx])
        {
            int32_t xOffset = ((bIdx * TFT_WIDTH) / ARRAY_SIZE(noteToBtn)) + (TFT_WIDTH / 10);
            drawCircleOutline(xOffset, HIT_BAR, ICON_RADIUS + 8, 4, colors[bIdx]);
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
static void shBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
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
// static void shMenuCb(const char* label, bool selected, uint32_t settingVal)
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
static void shLoadTrackData(shVars_t* shv, const uint8_t* data, size_t size)
{
    uint32_t dIdx  = 0;
    shv->numNotes = (data[dIdx++] << 8);
    shv->numNotes |= (data[dIdx++]);

    shv->notes = calloc(shv->numNotes, sizeof(shNote_t));

    for (int32_t nIdx = 0; nIdx < shv->numNotes; nIdx++)
    {
        shv->notes[nIdx].tick = (data[dIdx + 0] << 24) | //
                                 (data[dIdx + 1] << 16) | //
                                 (data[dIdx + 2] << 8) |  //
                                 (data[dIdx + 3] << 0);
        dIdx += 4;

        shv->notes[nIdx].note = data[dIdx++];

        if (0x80 & shv->notes[nIdx].note)
        {
            shv->notes[nIdx].note &= 0x7F;

            // Use the hold time to see when this note ends
            shv->notes[nIdx].hold = (data[dIdx + 0] << 8) | //
                                     (data[dIdx + 1] << 0);
            dIdx += 2;
        }
    }
}

/**
 * @brief TODO
 *
 * @param btn
 * @return uint32_t
 */
static uint32_t btnToNote(buttonBit_t btn)
{
    switch (btn)
    {
        case PB_LEFT:
        {
            return 0;
        }
        case PB_DOWN:
        {
            return 1;
        }
        case PB_UP:
        {
            return 2;
        }
        case PB_RIGHT:
        {
            return 3;
        }
        case PB_B:
        {
            return 4;
        }
        case PB_A:
        {
            return 5;
        }
        default:
        {
            return -1;
        }
    }
}

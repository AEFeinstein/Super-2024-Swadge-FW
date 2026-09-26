#include "heyListen.h"
#include "embeddedOut.h"
#include "heatshrink_helper.h"
#include "mainMenu.h"
#include "esp_random.h"

// Limits for detecting yells
#define MIC_ENERGY_THRESHOLD   100000
#define MIC_ENERGY_HYSTERESIS  20
#define WAIT_EVENT_US          200
#define RANDOM_EVENT_THRESHOLD 10500
//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    HL_INTRO,
    HL_MENU,
    HL_ECHO,
    HL_RANDOM,
    HL_TRIGGER,
    HL_SHAKE,
    HL_SETTINGS,
} heyListenScreen_t;

typedef enum
{
    EVT_HEY,
    EVT_HEYLISTEN,
    EVT_WHATSUP,
    EVT_PHRASE,
    MAX_NUM_EVTS,
} heyListenEvt_t;

//==============================================================================
// Function Declarations
//==============================================================================
static void heyListenEnterMode(void);
static void heyListenExitMode(void);
static void heyListenMainLoop(int64_t elapsedUs);
static void heyListenDacCallback(uint8_t* samples, int16_t len);
static void heyListenAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void heyListenCheckSpeech(int64_t elapsedUs);
static bool heyListenMenuCb(const char* label, bool selected, uint32_t value);
static void heyListenSwitchToScreen(heyListenScreen_t newScreen);
bool heyListenCheckForShake(void);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t* samples;
    uint32_t len;
} rawSample_t;

typedef struct
{
    cnfsFileIdx_t sfx_fidx;
    paletteColor_t bgColor;
    led_t ledColor;
} heyListenEvtData_t;

typedef struct
{
    // Screen
    heyListenScreen_t screen;

    // Art
    wsg_t* heyListenImgs;
    cnfsFileIdx_t heyListenImages;
    const paletteColor_t eyeColor;
    const paletteColor_t bgColor;
    const led_t ledColor;
    font_t font;

    // Menu
    menu_t* hlmenu;
    menuZorldoRenderer_t* menuRenderer;

    // Audio
    rawSample_t sfx[MAX_NUM_EVTS];
    int32_t sampleIdx;
    const heyListenEvtData_t* heyListenEvt;
    int8_t currentEvt; // tracking current event

    // Flag to switch from speaker to mic mode
    bool pendingSwitchToMic;
    bool isListening;

    // Microphone variables
    int32_t micSamplesProcessed;
    list_t micFrameEnergyHistory;
    bool isYelling;
    bool yellInput;
    dft32_data dd;       // Colorchord is used for spectral analysis
    embeddedNf_data end; // Colorchord is used for spectral analysis
    int32_t timeToNextEvent;
    bool initialWait;
    int32_t nextEvtTimer;
    int32_t randomizer;
    int32_t speechDelayUs; // Timer to pause between verbal commands
    list_t speechQueue;    // A queue of verbal commands

    char timerStr[16];

    // IMU Variables
    vec3d_t lastOrientation;
    list_t shakeHistory;
    bool isShook;

    // nvs
    // NONE FOR NOW

} heyListenData_t;

//==============================================================================
// Const data
//==============================================================================
static const char heyListenStrName[]     = "Hey, Listen!";
static const char warningStrName[]       = "This mode is annoying!";
static const char beniceStrName[]        = "Don't play this in quiet spaces!";
static const char heyListenStrMenu[]     = "Menu";
static const char heyListenStrEcho[]     = "Echo Mode";
static const char heyListenStrRandom[]   = "Random Mode";
static const char heyListenStrTrigger[]  = "Trigger Mode";
static const char heyListenStrShake[]    = "Shake Mode";
static const char heyListenStrSettings[] = "Settings";
static const char heyListenStrExit[]     = "Exit";

/** Must match order of heyListenEvt_t */
const heyListenEvtData_t hlEvtData[] = {
    {
        .sfx_fidx = BLOW_IT_RAW,
        .bgColor  = c132,
        .ledColor = {.r = 0x00, .g = 0xCC, .b = 0x99},
    },
    {
        .sfx_fidx = SHAKE_IT_RAW,
        .bgColor  = c030,
        .ledColor = {.r = 0x00, .g = 0xCC, .b = 0x99},
    },
    {
        .sfx_fidx = PRESS_IT_RAW,
        .bgColor  = c002,
        .ledColor = {.r = 0x00, .g = 0xCC, .b = 0x99},
    },
    {
        .sfx_fidx = DAC_SCREAM_RAW,
        .bgColor  = c200,
        .ledColor = {.r = 0x00, .g = 0xCC, .b = 0x99},
    },
};

// Trophy Data
const trophyData_t heyListenTrophies[] = {
    {
        .title       = "Hey, Listen!",
        .description = "Hey!",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
};

//==============================================================================
// Individual Mode Settings
//==============================================================================

const trophySettings_t heyListenTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 4,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = heyListenStrName,
};

const trophyDataList_t heyListenTrophyData = {
    .settings = &heyListenTrophySettings,
    .list     = heyListenTrophies,
    .length   = ARRAY_SIZE(heyListenTrophies),
};
swadgeMode_t heyListenMode = {
    .modeName = heyListenStrName,
    .wifiMode = NO_WIFI,        // If we want WiFi. WiFi is expensive computationally/battery-wise, so disable
                                // it if you're not going to use it.
    .overrideUsb = false,       // Overrides the default USB behavior. This is helpful for the game controller
                                // mode but unlikely to be useful for your game.
    .usesAccelerometer = true,  // If we're using motion controls
    .usesThermometer   = false, // If we're using the internal thermometer
    .overrideSelectBtn = false, // The select/Menu button has a default behavior. If you want to override it,
                                // you can set this to true but you'll need to re-implement the
                                // 'return to main menu' behavior.
    .fnEnterMode              = heyListenEnterMode,     // The enter mode function
    .fnExitMode               = heyListenExitMode,      // The exit mode function
    .fnMainLoop               = heyListenMainLoop,      // The loop function
    .fnAudioCallback          = heyListenAudioCallback, // If the mode uses the microphone
    .fnDacCb                  = heyListenDacCallback,   // If the mode fills its own DAC samples
    .fnBackgroundDrawCallback = NULL,                   // Draws a section of the display
    .fnEspNowRecvCb           = NULL,                   // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,                   // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,                   // If using advanced USB things.
    .trophyData               = &heyListenTrophyData,
};

heyListenData_t* hld;

static void heyListenEnterMode()
{
    hld         = (heyListenData_t*)heap_caps_calloc(1, sizeof(heyListenData_t), MALLOC_CAP_8BIT);
    hld->screen = HL_INTRO;

    // Switching to speaker disables the microphone
    switchToSpeaker();
    // This disables speaker too
    dacStop();
    // TODO swadgepass processing here, if anything
    //  This re-enables the speakers
    dacStart();

    // Allocate menu
    hld->hlmenu = initMenu(heyListenStrMenu, heyListenMenuCb);
    addSingleItemToMenu(hld->hlmenu, heyListenStrEcho);
    addSingleItemToMenu(hld->hlmenu, heyListenStrRandom);
    addSingleItemToMenu(hld->hlmenu, heyListenStrTrigger);
    addSingleItemToMenu(hld->hlmenu, heyListenStrShake);
    addSingleItemToMenu(hld->hlmenu, heyListenStrSettings);
    addSingleItemToMenu(hld->hlmenu, heyListenStrExit);
    hld->menuRenderer = initMenuZorldoRenderer(NULL, NULL);

    // Load fonts
    loadFont(OXANIUM_13MED_FONT, &hld->font, true);

    // Load all SFX samples
    // Note to self for later: how can I allocate different voice recordings here? maybe make an array of all the files
    // and then incrementing at i+j, where j is the voice selection? idk
    for (int8_t i = 0; i < MAX_NUM_EVTS; i++)
    {
        hld->sfx[i].samples = readHeatshrinkFile(hlEvtData[i].sfx_fidx, &hld->sfx[i].len, true);
    }

    // For yell detection
    InitColorChord(&hld->end, &hld->dd);
    hld->nextEvtTimer = 0;

    // for random mode
    hld->randomizer = 0;

    // TODO high scores, NVS
}

static void heyListenExitMode()
{
    deinitMenuZorldoRenderer(hld->menuRenderer);
    deinitMenu(hld->hlmenu);
    // Free SFX
    for (int8_t i = 0; i < ARRAY_SIZE(hld->sfx); i++)
    {
        heap_caps_free(hld->sfx[i].samples);
    }
    // TODO free imgs; no WSGs loaded for now, doing nothing

    freeFont(&hld->font);
    clear(&hld->shakeHistory);
    clear(&hld->speechQueue);
    clear(&hld->micFrameEnergyHistory);
    heap_caps_free(hld);
}

static void heyListenMainLoop(int64_t elapsedUs)
{
    // Count down the pause between queued verbal commands
    heyListenCheckSpeech(elapsedUs);

    // Check button input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        switch (hld->screen)
        {
            case HL_MENU:
            {
                hld->hlmenu = menuButton(hld->hlmenu, evt);
                break;
            }
            case HL_TRIGGER:
            {
                bool evtTriggered = true;
                if (evt.button == PB_UP)
                {
                    hld->currentEvt = EVT_HEY;
                }
                else if (evt.button == PB_DOWN)
                {
                    hld->currentEvt = EVT_HEYLISTEN;
                }
                else if (evt.button == PB_LEFT)
                {
                    hld->currentEvt = EVT_WHATSUP;
                }
                else if (evt.button == PB_RIGHT)
                {
                    hld->currentEvt = EVT_PHRASE;
                }
                else if (evt.button == PB_A)
                {
                    hld->currentEvt = 0;
                }
                else
                {
                    evtTriggered = false;
                }

                // Interrupt whatever is playing and queue up the newly triggered event
                if (evt.down && evtTriggered)
                {
                    clear(&hld->speechQueue);
                    hld->sampleIdx          = 0;
                    hld->pendingSwitchToMic = false;
                    push(&hld->speechQueue, (void*)(intptr_t)hld->currentEvt);
                }
            }
            break;
            // no buttons for these modes:
            case HL_ECHO:
            case HL_RANDOM:
            case HL_SHAKE:
            case HL_SETTINGS:
            default:
                break;
        }

        // all modes are exited by B
        if (evt.button == PB_B)
        {
            hld->nextEvtTimer = 0;
            hld->screen       = HL_MENU;
            heyListenSwitchToScreen(hld->screen);
        }
    }

    // actual gameplay:
    switch (hld->screen)
    {
        case HL_MENU:
        {
            drawMenuZorldo(hld->hlmenu, hld->menuRenderer, elapsedUs);
            break;
        }
        case HL_ECHO:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            if (hld->pendingSwitchToMic)
            {
                switchToMicrophone();
                hld->isListening        = true;
                hld->pendingSwitchToMic = false;
            }
            // ready for a new yell
                if (hld->nextEvtTimer >= 400)
                {
                    hld->currentEvt = EVT_PHRASE; // trigger the "PHRASE" event
                }
                else if (hld->nextEvtTimer >= 200)
                {
                    hld->currentEvt = EVT_HEYLISTEN; // trigger the "HEYLISTEN" event
                }
                else if (hld->nextEvtTimer >= 100)
                {
                    hld->currentEvt = EVT_HEY; // trigger the "HEY" event
                }
                else
                {
                    hld->currentEvt = MAX_NUM_EVTS; // no event triggered yet
                }
            }
            else if (hld->nextEvtTimer > 0)
            {
                if (hld->currentEvt < MAX_NUM_EVTS)
                {
                    // done listening, queue up the detected event and echo it back
                    clear(&hld->speechQueue);
                    push(&hld->speechQueue, (void*)(intptr_t)hld->currentEvt);
                    switchToSpeaker();
                    hld->sampleIdx          = 0;
                    hld->isListening        = false;
                    hld->pendingSwitchToMic = false;
                }
                else
                {
                    // yell was too short to identify, keep listening and restart the timer
                    hld->nextEvtTimer = 0;
                }
            }

            sprintf(hld->timerStr, "%d", (int)hld->nextEvtTimer);
            drawText(&hld->font, c555, "Timer:", 20, 60);
            drawText(&hld->font, c555, hld->timerStr, 20, 80);
            drawText(&hld->font, c555, "Current Event:", 20, 100);
            drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 120);
            drawText(&hld->font, c555, "Is yelling?:", 20, 140);
            drawText(&hld->font, c555, (char[]){hld->yellInput + '0', '\0'}, 20, 160);
            drawText(&hld->font, c555, "Is listening?:", 20, 180);
            drawText(&hld->font, c555, (char[]){hld->isListening + '0', '\0'}, 20, 200);

            if (!hld->isListening)
            {
                hld->nextEvtTimer = 0;
                // allow for another yell
            }
            break;
        }

        case HL_TRIGGER:
        {
            // for testing, just draw the screen and print the event:
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            drawText(&hld->font, c555, "Current Event:", 20, 20);
            drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 40);
            // end of testing stuff

            // Playback is queued from the button handler above and filled by heyListenDacCallback()
            break;
        }
        case HL_RANDOM:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            if (hld->randomizer > RANDOM_EVENT_THRESHOLD) // if the random number is greater than
                                                          // RANDOM_EVENT_THRESHOLD, trigger an event
            {
                hld->currentEvt   = esp_random() % MAX_NUM_EVTS;
                hld->nextEvtTimer = 0; // reset the event timer when a new event is triggered

                // queue up the randomly chosen event for playback
                clear(&hld->speechQueue);
                hld->sampleIdx          = 0;
                hld->pendingSwitchToMic = false;
                push(&hld->speechQueue, (void*)(intptr_t)hld->currentEvt);
            }

            hld->randomizer = esp_random() % 10000; // generate a new random number for the next check
            hld->nextEvtTimer++;
            // TODO evaluate better timing by uS and consider setting a framerate like Swadge-It did, You could even use
            // the RUN_TIMER_EVERY() function macro to ramp up randomizer at a nice interval (like whatever the
            // framerate is).
            hld->randomizer += hld->nextEvtTimer; // accelerate chaos the longer we've been waiting

            // for testing
            drawText(&hld->font, c555, "Random Event:", 20, 20);
            drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 40);
            drawText(&hld->font, c555, "Next Event Timer:", 20, 60);
            sprintf(hld->timerStr, "%d", (int)hld->nextEvtTimer);
            drawText(&hld->font, c555, hld->timerStr, 20, 80);
            drawText(&hld->font, c555, "Randomizer:", 20, 100);
            sprintf(hld->timerStr, "%d", (int)hld->randomizer);
            drawText(&hld->font, c555, hld->timerStr, 20, 120);
            sprintf(hld->timerStr, "%d", (int)hld->randomizer);
            drawText(&hld->font, c555, hld->timerStr, 20, 120);
            // end testing stuff

            break;
        }
        case HL_SHAKE:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            bool shook = heyListenCheckForShake();
            hld->nextEvtTimer++;

            // for testing, just draw the screen and print the event:
            drawText(&hld->font, c555, "Shake Detected:", 20, 20);
            drawText(&hld->font, c555, (char[]){shook + '0', '\0'}, 20, 40);
            drawText(&hld->font, c555, "Next Event Timer:", 20, 60);
            sprintf(hld->timerStr, "%d", (int)hld->nextEvtTimer);
            drawText(&hld->font, c555, hld->timerStr, 20, 80);
            drawText(&hld->font, c555, "Current Event:", 20, 100);
            drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 120);
            // end of testing stuff

            if (shook && hld->nextEvtTimer >= WAIT_EVENT_US)
            {
                hld->isShook      = false;
                hld->nextEvtTimer = 0;
                clear(&hld->shakeHistory);
                hld->currentEvt = esp_random() % MAX_NUM_EVTS;

                // queue up the yell
                clear(&hld->speechQueue);
                hld->sampleIdx          = 0;
                hld->pendingSwitchToMic = false;
                push(&hld->speechQueue, (void*)(intptr_t)hld->currentEvt);
            }
            break;
        }
        case HL_SETTINGS:
        {
            break;
        }
        case HL_INTRO:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            drawText(&hld->font, c555, heyListenStrName, 20, 40);
            drawText(&hld->font, c555, warningStrName, 20, 60);
            drawText(&hld->font, c555, beniceStrName, 20, 80);
            drawText(&hld->font, c555, "Press B to Continue", 20, 100);
            // TODO art instead of this
            break;
        }
        default:
        {
            break;
        }
    }
}
/**
 * @brief This function is called to fill sample buffers for the DAC. If this is NULL, then
 * globalMidiPlayerFillBuffer() will be used instead to fill sample buffers
 *
 * @param samples The sample buffer to fill
 * @param len The number of samples to fill
 */
static void heyListenDacCallback(uint8_t* samples, int16_t len)
{
    if (hld->speechDelayUs <= 0 &&  // if the delay between verbal commands isn't running and
        !hld->pendingSwitchToMic && // we aren't about to switch to the microphone
        hld->speechQueue.length)    // there is something to say
    {
        // get raw samples
        heyListenEvt_t evt    = (heyListenEvt_t)hld->speechQueue.first->val;
        const rawSample_t* rs = NULL;
        if (evt < MAX_NUM_EVTS)
        {
            rs = &hld->sfx[evt];
        }
        else
        {
            // invalid event queued, drop it so we don't get stuck
            shift(&hld->speechQueue);
        }

        if (rs && rs->samples)
        {
            // Make sure we don't read out of bounds
            int16_t cpLen = len;
            if (hld->sampleIdx + len > rs->len)
            {
                cpLen = rs->len - hld->sampleIdx;
            }

            // copy samples out to dac
            memcpy(samples, &rs->samples[hld->sampleIdx], cpLen);
            hld->sampleIdx += cpLen;

            // advance past the copied audio so it isn't overwritten by the blank-fill below
            samples += cpLen;
            len -= cpLen;

            // if copied and now its over
            if (cpLen && hld->sampleIdx >= rs->len)
            {
                // done with this sample, dequeue it and reset for the next one
                shift(&hld->speechQueue);
                hld->sampleIdx = 0;

                // if this is the last one
                if (0 == hld->speechQueue.length)
                {
                    hld->pendingSwitchToMic = true;
                }
                else
                {
                    // set timer to pause between commands
                    hld->speechDelayUs = 500;
                }
            }
        }
    }

    // anything else to write:
    if (len)
    {
        // write blanks
        memset(samples, 127, len);
    }
}

/**
 * @brief This function is called whenever audio samples are read from the microphone (ADC) and are ready for
 * processing. Samples are read at 8KHz.
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
static void heyListenAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    while (sampleCnt--)
    {
        // Get and process the sample
        int16_t samp = *(samples++);

        // Push to colorchord
        PushSample32(&hld->dd, samp);

        // If enough samples have been processed
        hld->micSamplesProcessed++;
        if (128 == hld->micSamplesProcessed)
        {
            // Handle the frame
            hld->micSamplesProcessed = 0;
            HandleFrameInfo(&hld->end, &hld->dd);

            // Sum total energy
            int32_t totalEnergy = 0;
            for (uint16_t i = 0; i < FIX_BINS; i++)
            {
                totalEnergy += hld->end.fuzzed_bins[i];
            }

            // Add total energy to queue
            push(&hld->micFrameEnergyHistory, (void*)((intptr_t)totalEnergy));
            if (hld->micFrameEnergyHistory.length > MIC_ENERGY_HYSTERESIS)
            {
                shift(&hld->micFrameEnergyHistory);
            }

            // Check for yelling and not yelling
            if (!hld->isYelling)
            {
                // One sample is enough to yell
                if (totalEnergy > MIC_ENERGY_THRESHOLD)
                {
                    hld->isYelling = true;
                    // Process the input on the main loop
                    hld->yellInput = true;
                }
            }
            else // Is yelling, check for return to quiet
            {
                // Returning to quiet takes a few samples
                node_t* energyNode = hld->micFrameEnergyHistory.first;
                bool stillYelling  = false;
                while (energyNode)
                {
                    if ((intptr_t)energyNode->val > MIC_ENERGY_THRESHOLD)
                    {
                        // Still yelling
                        stillYelling = true;
                        break;
                    }
                    energyNode = energyNode->next;
                }

                // Looped without finding a loud frame, must not be yelling
                if (!stillYelling)
                {
                    hld->isYelling = false;
                }
            }
        }
    }
}
static void heyListenCheckSpeech(int64_t elapsedUs)
{
    if (hld->speechDelayUs > 0)
    {
        hld->speechDelayUs -= elapsedUs;
        if (hld->speechDelayUs < 0)
        {
            hld->speechDelayUs = 0;
        }
    }
}

static void heyListenSwitchToScreen(heyListenScreen_t newScreen)
{
    // Clear SFX & SPK variables
    hld->sampleIdx          = 0;
    hld->pendingSwitchToMic = true;

    // Clear gameplay variables
    hld->timeToNextEvent = WAIT_EVENT_US;
    hld->initialWait     = true;
    hld->nextEvtTimer    = 0;
    hld->speechDelayUs   = 0;

    // Clear IMU variables
    memset(&hld->lastOrientation, 0, sizeof(vec3d_t));
    clear(&hld->shakeHistory);
    hld->isShook = false;

    // Set the new screen
    hld->screen = newScreen;

    // Screen-specific setup
    switch (newScreen)
    {
        case HL_ECHO:
        {
            // Enable speaker for a new verbal command and reset sample count
            switchToSpeaker();
            hld->sampleIdx          = 0;
            hld->isListening        = false;
            hld->pendingSwitchToMic = false;

            // Enqueue special event to yell
            clear(&hld->speechQueue);
            if (hld->currentEvt < MAX_NUM_EVTS)
            {
                heyListenEvt_t newEvt = hld->currentEvt;
                push(&hld->speechQueue, (void*)newEvt);
            }
            else
            {
                heyListenEvt_t newEvt = EVT_HEY; // adding this to avoid a crash if the player backs out of the mode
                                                 // before finishing the yell
                push(&hld->speechQueue, (void*)newEvt);
            }

            break;
        }
        case HL_MENU:
        {
            break;
        }
        case HL_TRIGGER:
        {
            // Enable the speaker so triggered SFX can be played back
            switchToSpeaker();
            hld->sampleIdx          = 0;
            hld->isListening        = false;
            hld->pendingSwitchToMic = false;
            clear(&hld->speechQueue);
            break;
        }
        case HL_RANDOM:
        {
            // Enable the speaker for random events
            switchToSpeaker();
            hld->sampleIdx          = 0;
            hld->isListening        = false;
            hld->pendingSwitchToMic = false;
            clear(&hld->speechQueue);
            break;
        }
        case HL_SHAKE:
        {
            // Enable the speaker for shake events
            switchToSpeaker();
            hld->sampleIdx          = 0;
            hld->isListening        = false;
            hld->pendingSwitchToMic = false;
            clear(&hld->speechQueue);
            break;
        }
        case HL_SETTINGS:
        case HL_INTRO:
        default:
        {
            break;
        }
    }
}
/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 * @return true to go up a menu level, false to remain here
 */
static bool heyListenMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (heyListenStrEcho == label)
        {
            heyListenSwitchToScreen(HL_ECHO);
        }
        else if (heyListenStrRandom == label)
        {
            heyListenSwitchToScreen(HL_RANDOM);
        }
        else if (heyListenStrTrigger == label)
        {
            heyListenSwitchToScreen(HL_TRIGGER);
        }
        else if (heyListenStrShake == label)
        {
            heyListenSwitchToScreen(HL_SHAKE);
        }
        else if (heyListenStrSettings == label)
        {
            heyListenSwitchToScreen(HL_SETTINGS);
        }
        else if (heyListenStrExit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}
bool heyListenCheckForShake(void)
{
    // Check if there is a shake state change
    if (checkForShake(&hld->lastOrientation, &hld->shakeHistory, &hld->isShook))
    {
        // There was a change, check if it's shaking
        if (hld->isShook)
        {
            // It is shaking, check if inputs are accepted
            if (!(hld->screen == HL_SHAKE))
            {
                // Input not accepted, mark as not shaking
                hld->isShook = false;
            }
        }
    }
    return hld->isShook;
}
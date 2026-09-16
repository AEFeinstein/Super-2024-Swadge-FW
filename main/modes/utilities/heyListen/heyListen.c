#include "heyListen.h"
#include "embeddedOut.h"
#include "heatshrink_helper.h"
#include "mainMenu.h"
#include "esp_random.h"

const char heyListenModeName[] = "Hey, Listen!";

// Limits for detecting yells
#define MIC_ENERGY_THRESHOLD  100000
#define MIC_ENERGY_HYSTERESIS 20
#define WAIT_EVENT_US         5000
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
    EVT_NULL,
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

//Screen
heyListenScreen_t screen;

// Art
wsg_t* heyListenImgs;
cnfsFileIdx_t heyListenImages;
const paletteColor_t eyeColor;
const paletteColor_t bgColor;
const led_t ledColor;
font_t font;

//Menu
menu_t* hlmenu;
menuZorldoRenderer_t* menuRenderer;

//Audio
rawSample_t sfx[MAX_NUM_EVTS];
int32_t sampleIdx;
heyListenEvt_t currentEvt;

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
int32_t speechDelayUs; // Timer to pause between verbal commands
list_t speechQueue;    // A queue of verbal commands

char timerStr[16];

// IMU Variables
vec3d_t lastOrientation;
list_t shakeHistory;
bool isShook;

//nvs
//NONE FOR NOW

} heyListenData_t;

//==============================================================================
// Const data
//==============================================================================
static const char heyListenStrName[]        = "Hey, Listen!";
static const char warningStrName[]          = "This mode is annoying!";
static const char beniceStrName[]           = "Don't play this in quiet spaces!";
static const char heyListenStrMenu[]        = "Menu";
static const char heyListenStrEcho[]        = "Echo Mode";
static const char heyListenStrRandom[]      = "Random Mode";
static const char heyListenStrTrigger[]     = "Trigger Mode";
static const char heyListenStrShake[]       = "Shake Mode";
static const char heyListenStrSettings[]    = "Settings";
static const char heyListenStrExit[]          = "Exit";


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

// Individual mode settings

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
    .modeName                 = heyListenModeName,  // Assign the name we created here
    .wifiMode                 = NO_WIFI,         // If we want WiFi. WiFi is expensive computationally/battery-wise, so disable 
                                                 // it if you're not going to use it.
    .overrideUsb              = false,           // Overrides the default USB behavior. This is helpful for the game controller 
                                                 // mode but unlikely to be useful for your game.
    .usesAccelerometer        = true,           // If we're using motion controls
    .usesThermometer          = false,           // If we're using the internal thermometer
    .overrideSelectBtn        = false,           // The select/Menu button has a default behavior. If you want to override it, 
                                                 // you can set this to true but you'll need to re-implement the 
                                                 // 'return to main menu' behavior.
    .fnEnterMode              = heyListenEnterMode, // The enter mode function
    .fnExitMode               = heyListenExitMode,  // The exit mode function
    .fnMainLoop               = heyListenMainLoop,  // The loop function
    .fnAudioCallback          = heyListenAudioCallback,            // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,            // Draws a section of the display
    .fnEspNowRecvCb           = NULL,            // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,            // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,            // If using advanced USB things.
    .trophyData               = &heyListenTrophyData,
};

heyListenData_t* hld;

static void heyListenEnterMode()
{
    hld = (heyListenData_t*)heap_caps_calloc(1, sizeof(heyListenData_t), MALLOC_CAP_8BIT);
    hld->screen = HL_INTRO;

// Switching to speaker disables the microphone
    switchToSpeaker();
// This disables speaker too
    dacStop();
//TODO swadgepass processing here, if anything
// This re-enables the speakers
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




// For yell detection
InitColorChord(&hld->end, &hld->dd);
hld->nextEvtTimer = 0;

}

static void heyListenExitMode()
{
    deinitMenuZorldoRenderer(hld->menuRenderer);
    deinitMenu(hld->hlmenu);
    //TODO free sfx
    //TODO free imgs
    
    freeFont(&hld->font);
    clear(&hld->shakeHistory);
    clear(&hld->speechQueue);
    clear(&hld->micFrameEnergyHistory);
    heap_caps_free(hld);
}

static void heyListenMainLoop(int64_t elapsedUs)
{
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
                    if(evt.button == PB_UP)
                        {
                        hld->currentEvt = EVT_HEY;
                        }
                    else if(evt.button == PB_DOWN)
                        {
                        hld->currentEvt = EVT_HEYLISTEN;
                        }
                    else if(evt.button == PB_LEFT)
                        {
                        hld->currentEvt = EVT_WHATSUP;
                        }
                    else if(evt.button == PB_RIGHT)
                        {
                        hld->currentEvt = EVT_PHRASE;
                        }
                    else if(evt.button == PB_A)
                    {
                        hld->currentEvt = EVT_NULL;
                    }

                }
                break;
            //no buttons for these modes:
            case HL_ECHO:
            case HL_RANDOM:
            case HL_SHAKE:
            case HL_SETTINGS:
            default:
                break;
        }
    
    //all modes are exited by B
    if(evt.button == PB_B)
        {
        hld->nextEvtTimer = 0;
        hld->screen = HL_MENU;  
        heyListenSwitchToScreen(hld->screen);             
        }
    }

    //actual gameplay:
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
            if(hld->nextEvtTimer == 0){
                switchToMicrophone();
                hld->isListening = true;
                //ready for a new yell
            if(hld->yellInput) // if the player is currently yelling, kick off the timer
            {
                hld->nextEvtTimer++;
                if(hld->nextEvtTimer >= 100)
                {
                    hld->currentEvt = EVT_HEY; // trigger the "HEY" event
                }
                if(hld->nextEvtTimer >= 200)
                {
                    hld->currentEvt = EVT_HEYLISTEN; // trigger the "HEYLISTEN" event
                }
                if(hld->nextEvtTimer >= 400)
                {
                    hld->currentEvt = EVT_PHRASE; // trigger the "NULL" event
                }
                else
                {
                    hld->currentEvt = EVT_NULL; // trigger the "NULL" event
                }
            }
            else
            {
                //done listening, time to echo
                switchToSpeaker();
                hld->sampleIdx          = 0;
                hld->isListening        = false;
                hld->pendingSwitchToMic = false;
                //TODO: yell
            }

                sprintf(hld->timerStr, "%ld", hld->nextEvtTimer);
                drawText(&hld->font, c555, "Timer:", 20, 60);
                drawText(&hld->font, c555, hld->timerStr, 20, 80);
                drawText(&hld->font, c555, "Current Event:", 20, 100);
                drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 120);
                drawText(&hld->font, c555, "Is yelling?:", 20, 140);
                drawText(&hld->font, c555, (char[]){hld->yellInput + '0', '\0'}, 20, 160);
                drawText(&hld->font, c555, "Is listening?:", 20, 180);
                drawText(&hld->font, c555, (char[]){hld->isListening + '0', '\0'}, 20, 200);
            
            }
            
            if(!hld->isListening)
                {
                    hld->nextEvtTimer = 0;
                    //allow for another yell
                }
            
        }
            break;

        case HL_TRIGGER:
        {
            //for testing, just draw the screen and print the event:
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            drawText(&hld->font, c555, "Current Event:", 20, 20);
            drawText(&hld->font, c555, (char[]){hld->currentEvt + '0', '\0'}, 20, 40);
            //end of testing stuff

            //TODO yell
            break;
        }
        case HL_RANDOM:
        {
            break;
        }
        case HL_SHAKE:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            bool shook = heyListenCheckForShake();
            hld->nextEvtTimer++;

            //for testing, just draw the screen and print the event:
            drawText(&hld->font, c555, "Shake Detected:", 20, 20);
            drawText(&hld->font, c555, (char[]){shook + '0', '\0'}, 20, 40);
            drawText(&hld->font, c555, "Next Event Timer:", 20, 60);
            
            sprintf(hld->timerStr, "%ld", hld->nextEvtTimer);
            drawText(&hld->font, c555, hld->timerStr, 20, 80);
            //end of testing stuff

            if(shook && hld->nextEvtTimer >= WAIT_EVENT_US)
            {   
                hld->isShook = false;
                hld->nextEvtTimer = 0;
                clear(&hld->shakeHistory);
                //TODO: yell
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
            break;
        }
        default:
        {
            break;
        }
    }
}

static void heyListenDacCallback(uint8_t* samples, int16_t len)
{

}
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
                while (energyNode)
                {
                    if ((intptr_t)energyNode->val > MIC_ENERGY_THRESHOLD)
                    {
                        // Still yelling
                        return;
                    }
                    energyNode = energyNode->next;
                }

                // Looped without returning, must not be yelling
                hld->isYelling = false;
            }
        }
    }
}
static void heyListenCheckSpeech(int64_t elapsedUs)
{

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
    hld->speechDelayUs = 0;

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

            // Enqueue special event to scream
            clear(&hld->speechQueue);
            heyListenEvt_t newEvt = MAX_NUM_EVTS;
            push(&hld->speechQueue, (void*)newEvt);

            break;
        }
        case HL_MENU:
        {
           
            break;
        }
        case HL_TRIGGER:
        case HL_RANDOM:
        case HL_SHAKE:
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
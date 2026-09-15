#include "heyListen.h"
#include "embeddedOut.h"
#include "heatshrink_helper.h"
const char heyListenModeName[] = "Hey, Listen!";

// Limits for detecting yells
#define MIC_ENERGY_THRESHOLD  100000
#define MIC_ENERGY_HYSTERESIS 20

//==============================================================================
// Function Declarations
//==============================================================================
static void heyListenEnterMode(void);
static void heyListenExitMode(void);
static void heyListenMainLoop(int64_t elapsedUs);
static void heyListenCheckForYell(int64_t elapsedUs);
static void heyListenDacCallback(uint8_t* samples, int16_t len);
static void heyListenAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void heyListenCheckSpeech(int64_t elapsedUs);

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
} heyListenScreen_t;

typedef enum
{
    EVT_NULL,
    EVT_HEY,
    EVT_HEYLISTEN,
    EVT_WHATSUP,
    MAX_NUM_EVTS,
} heyListenEvt_t;

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
const led_t ledColor;
font_t font;

//Audio
    rawSample_t sfx[MAX_NUM_EVTS];
    int32_t sampleIdx;

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


//nvs
//NONE FOR NOW

} heyListenData_t;

//==============================================================================
// Const data
//==============================================================================
static const char heyListenStrName[]       = "Hey, Listen!";
static const char warningStrName[]         = "This mode is annoying!";
static const char beniceStrName[]          = "Don't play this in quiet spaces!";
static const char heyListenStrMenu[]        = "Menu/Settings";
static const char heyListenstrEcho[]        = "Echo";
static const char heyListenStrRandom[]      = "Random";
static const char heyListenStrTrigger[]     = "Trigger";
static const char heyListenStrShake[]       = "Shake";


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

    // Load fonts
    loadFont(OXANIUM_13MED_FONT, &hld->font, true);

    drawText(&hld->font, c555, heyListenStrName, 20, TFT_HEIGHT/2 - 20);
    drawText(&hld->font, c555, warningStrName, 20, TFT_HEIGHT/2 + 10);
    drawText(&hld->font, c555, beniceStrName, 20, TFT_HEIGHT/2 + 30);
    drawText(&hld->font, c555, "Press A to Continue", 20, TFT_HEIGHT/2 + 50);
    //TODO: art instead of this
}

static void heyListenExitMode()
{
    heap_caps_free(hld);
    freeFont(&hld->font);
}

static void heyListenMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if(evt.button == PB_A)
        {
         hld->screen = HL_MENU;
        }
    }
}

static void heyListenCheckForYell(int64_t elapsedUs)
{

}

static void heyListenDacCallback(uint8_t* samples, int16_t len)
{

}
static void heyListenAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{

}

static void heyListenCheckSpeech(int64_t elapsedUs)
{

}
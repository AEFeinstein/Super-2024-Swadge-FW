#include "cipher.h"
#include "math.h"

static const char DR_NAMESTRING[] = "Caesar Cipher";
//static const char ALPHABET[] = "ABCDEFG";

typedef struct 
{
    list_t* elements;
    bool rotating;
    bool direction;
    int32_t raceDiam;
} cipherRace_t;


static void cipherEnterMode(void);
static void cipherExitMode(void);
static void cipherMainLoop(int64_t elapsedUs);
static vec_t RThetaToXY(vec_t);


swadgeMode_t cipherMode = {
    .modeName                 = DR_NAMESTRING,  // Assign the name we created here
    .wifiMode                 = NO_WIFI,         // If we want WiFi. WiFi is expensive computationally/battery-wise, so disable 
                                                 // it if you're not going to use it.
    .overrideUsb              = false,           // Overrides the default USB behavior. This is helpful for the game controller 
                                                 // mode but unlikely to be useful for your game.
    .usesAccelerometer        = false,           // If we're using motion controls
    .usesThermometer          = false,           // If we're using the internal thermometer
    .overrideSelectBtn        = false,           // The select/Menu button has a default behavior. If you want to override it, 
                                                 // you can set this to true but you'll need to re-implement the 
                                                 // 'return to main menu' behavior.
    .fnEnterMode              = cipherEnterMode, // The enter mode function
    .fnExitMode               = cipherExitMode,  // The exit mode function
    .fnMainLoop               = cipherMainLoop,  // The loop function
    .fnAudioCallback          = NULL,            // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,            // Draws a section of the display
    .fnEspNowRecvCb           = NULL,            // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,            // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,            // If using advanced USB things.
};
cipherRace_t* innerRace;
cipherRace_t* outerRace;


static void cipherEnterMode()
{
    innerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    outerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);

    ESP_LOGI("info","This is some text  %f",sin(3.14f));
}
 
static void cipherExitMode()
{
    heap_caps_free(innerRace);
    heap_caps_free(outerRace);
}
 
static void cipherMainLoop(int64_t elapsedUs)
{
    
}
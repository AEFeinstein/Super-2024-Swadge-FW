#include "cipher.h"
#include "math.h"

static const char DR_NAMESTRING[] = "Caesar Cipher";
static const int rotateRPM = 1;
static const int64_t UsPerDeg = (60000000 / (rotateRPM*360));
static const vec_t ScreenCenter = {
    .x = 140,
    .y = 120,
};
static const char lettersRace[] = {'M','4','R','H','N','Y','8','A','C','T','3','Z','K','W','F','1','X','G','Q','E','0','J','L','D','9','B','V','S','5','7','I','P','2','0','6','U','\0'};
//static const int numbersRace[] = {8,35,6,23,15,9,33,3,20,11,36,27,14,25,19,32,2,28,21,34,7,16,30,5,1,22,13,10,18,26,31,24,17,4,29,12};

//Declare struct for spinning races
typedef struct 
{
    bool rotating;
    bool direction;
    int32_t raceDiam;
    int64_t timeSpinning;
} cipherRace_t;

//Function Definitions
static void cipherEnterMode(void);
static void cipherExitMode(void);
static void cipherMainLoop(int64_t elapsedUs);
static vec_t RThetaToXY(vec_t);
static void DrawDividerLine(int,int,int);

//Boilerplate
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

//Declare Variables
static font_t ibm;
cipherRace_t* innerRace;
cipherRace_t* outerRace;

//Define functions
static vec_t RThetaToXY(vec_t RTheta){
    //ESP_LOGI("RTheta","x: %d  | y: %d",(int)RTheta.x, (int)RTheta.y);
    return (vec_t){
        .x = ScreenCenter.x + RTheta.x * sin(RTheta.y * M_PI / 180),
        .y = ScreenCenter.y + RTheta.x * cos(RTheta.y * M_PI / 180),
    };
}

static void DrawDividerLine(int RStart, int REnd, int ThetaDeg){
    vec_t start = RThetaToXY( (vec_t){
        .x=RStart,
        .y=ThetaDeg,
    });

    vec_t end = RThetaToXY( (vec_t){
        .x=REnd,
        .y=ThetaDeg,
    });

    drawLine (start.x, start.y, end.x, end.y, c555, 0);
}

static void cipherEnterMode(){
    innerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    outerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    loadFont(IBM_VGA_8_FONT, &ibm, false);
}
 
static void cipherExitMode(){
    heap_caps_free(innerRace);
    heap_caps_free(outerRace);
    freeFont(&ibm);
}
 
static void cipherMainLoop(int64_t elapsedUs){
    clearPxTft();

    innerRace->timeSpinning = (innerRace->timeSpinning + elapsedUs) % (UsPerDeg*360);
    
    for(int i=30;i<36;i++){
        vec_t hold = {
            .x = 110,
            .y = (innerRace->timeSpinning / UsPerDeg) + 10*i, //Hard-coding a 36-part race => 360/36=10 degrees of difference to each text
        };
        hold = RThetaToXY(hold);
        drawText(&ibm, c252, &lettersRace[i]+'\0', hold.x-7, hold.y-7);

        DrawDividerLine(100,120,(innerRace->timeSpinning / UsPerDeg) + 10*i+5);
    }
}
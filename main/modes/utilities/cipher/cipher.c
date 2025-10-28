#include "cipher.h"
#include "math.h"

static const char DR_NAMESTRING[] = "Caesar Cipher";
static const int rotateRPM = 5;
static const int64_t UsPerDeg = (60000000 / (rotateRPM*360));
static const vec_t ScreenCenter = {
    .x = 140,
    .y = 135, //120 is the default center of the screen
};
static const char* lettersRace[] = {"M","4","R","H","N","Y","8","A","C","T","3","Z","K","W","F","1","X","G","Q","E","O","J","L","D","9","B","V","S","5","7","I","P","2","0","6","U"};;
static const char* numbersRace[] = {"8","35","6","23","15","9","33","3","20","11","36","27","14","25","19","32","2","28","21","34","7","16","30","5","1","22","13","10","18","26","31","24","17","4","29","12"};

//Declare struct for spinning races
typedef struct 
{
    bool rotating;
    bool direction;
    int32_t raceRad;
    int64_t timeSpinning;
} cipherRace_t;

//Function Definitions
static void cipherEnterMode(void);
static void cipherExitMode(void);
static void cipherMainLoop(int64_t elapsedUs);
static vec_t RThetaToXY(vec_t, bool);
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
int64_t RaceOffset = 0;
bool OffsettingLeft = false;
bool OffsettingRight = false;

//Define functions
static vec_t RThetaToXY(vec_t RTheta, bool TextBuffer){
    //This function uses the global ScreenCenter vec_t object to allow text to be pushed to the corners of the screen without overrunning it
    vec_t mathPos = {
        .x = ScreenCenter.x + RTheta.x * sin(RTheta.y * M_PI / 180),
        .y = ScreenCenter.y + RTheta.x * cos(RTheta.y * M_PI / 180),
    };
    //ESP_LOGI("func","R: %d, Theta: %d, Xi: %d, Yi: %d", RTheta.x, RTheta.y, mathPos.x, mathPos.y);

    if(TextBuffer){
        //Keep x above 4 and below 280-18=>262
        //Keep y above 4 and below 240-14=>216
        int hypX = fabs(fmax( fmax(mathPos.x - (ScreenCenter.x * 2 - 18), 0) , fmax(4 - mathPos.x, 0) ) / sin(RTheta.y * M_PI / 180));
        int hypY = fabs(fmax( fmax(mathPos.y - (ScreenCenter.y * 2 - 14), 0) , fmax(4 - mathPos.y, 0) ) / cos(RTheta.y * M_PI / 180));
        //ESP_LOGI("trim", "hypX: %d, hypY: %d", hypX, hypY);
        mathPos = (vec_t){
            .x = ScreenCenter.x + (RTheta.x - fmax(hypX, hypY)) * sin(RTheta.y * M_PI / 180),
            .y = ScreenCenter.y + (RTheta.x - fmax(hypX, hypY)) * cos(RTheta.y * M_PI / 180),
        };
        //ESP_LOGI("final", "Xf: %d, Yf: %d", mathPos.x, mathPos.y);
    }

    return mathPos; 
}

static void DrawDividerLine(int RStart, int REnd, int ThetaDeg){
    vec_t start = RThetaToXY( (vec_t){
        .x=RStart,
        .y=ThetaDeg,
    }, true);

    vec_t end = RThetaToXY( (vec_t){
        .x=REnd,
        .y=ThetaDeg,
    }, false);

    drawLine (start.x, start.y, end.x, end.y, c555, 0);
}

static void cipherEnterMode(){
    innerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    innerRace->rotating = true;
    innerRace->direction = true;
    innerRace->raceRad = 100;
    outerRace = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    outerRace->rotating = true;
    outerRace->direction=false;
    outerRace->raceRad = 125;

    loadFont(IBM_VGA_8_FONT, &ibm, false);
}
 
static void cipherExitMode(){
    heap_caps_free(innerRace);
    heap_caps_free(outerRace);
    freeFont(&ibm);
}
 
static void cipherMainLoop(int64_t elapsedUs){
    clearPxTft();

    if(innerRace->rotating || innerRace->timeSpinning % (UsPerDeg*10) > 2){
        innerRace->timeSpinning = (innerRace->timeSpinning + (int)fmin(elapsedUs,(UsPerDeg*10) - (innerRace->timeSpinning % (UsPerDeg*10)) + 1)) % (UsPerDeg*360);
    }
    if(outerRace->rotating || outerRace->timeSpinning % (UsPerDeg*10) > 2){
        outerRace->timeSpinning = (outerRace->timeSpinning + (int)fmin(elapsedUs,(UsPerDeg*10) - (outerRace->timeSpinning % (UsPerDeg*10)) + 1)) % (UsPerDeg*360);
    }
    if(OffsettingLeft){
        RaceOffset = (RaceOffset + elapsedUs) % (UsPerDeg*360);
    }
    if(OffsettingRight){
        RaceOffset = ((RaceOffset - elapsedUs) + (UsPerDeg*360)) % (UsPerDeg*360);
    }


    for(int i=0;i<36;i++){ //i<36

        //Draw inner race
        vec_t inPos = {
            .x = innerRace->raceRad,
            .y = ((innerRace->timeSpinning + RaceOffset) / UsPerDeg) + 10*i, //Hard-coding a 36-part race => 360/36=10 degrees of difference to each text
        };
        inPos = RThetaToXY(inPos, false);
        drawText(&ibm, c252, lettersRace[i], inPos.x-4, inPos.y-5);

        //draw outer race
        vec_t outPos = {
            .x = outerRace->raceRad,
            .y = (( (UsPerDeg*360) - outerRace->timeSpinning + RaceOffset) / UsPerDeg) + 10*i , //Hard-coding a 36-part race => 360/36=10 degrees of difference to each text
        };
        outPos = RThetaToXY(outPos, false);
        drawText(&ibm, c252, numbersRace[i], outPos.x-7, outPos.y-5);

        DrawDividerLine(innerRace->raceRad-15, innerRace->raceRad+40,(RaceOffset / UsPerDeg) + 10*i+5);
    }

    //handle input
    buttonEvt_t evt;
    while(checkButtonQueueWrapper(&evt))
    {
        if(evt.down){
            switch (evt.button){
                case PB_B:
                    innerRace->rotating = !innerRace->rotating;
                    break;
                case PB_A:
                    outerRace->rotating = !outerRace->rotating;
                    break;
                case PB_LEFT:
                    OffsettingLeft = true;
                    break;
                case PB_RIGHT:
                    OffsettingRight = true;
                    break;
                
                default:
                    break;
            }
        }else{ //The button is released
            switch (evt.button){
                case PB_LEFT:
                    OffsettingLeft = false;
                    break;
                case PB_RIGHT:
                    OffsettingRight = false;
                    break;
                default:break;
            }
        }
        
        
    }
}
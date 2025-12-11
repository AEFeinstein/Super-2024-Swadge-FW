//==============================================================================
// Includes
//==============================================================================

#include "cipher.h"
#include "math.h"

//==============================================================================
// Structs
//==============================================================================

// Declare struct for spinning races
typedef struct
{
    bool rotating;
    int32_t lastSpin;
    int32_t raceRad;
    int64_t timeSpinning;
} cipherRace_t;

typedef struct
{
    font_t* ibm;
    cipherRace_t* innerRace;
    cipherRace_t* outerRace;
    int64_t RaceOffset;
    bool OffsettingLeft;
    bool OffsettingRight;
    wsg_t logo;

    paletteColor_t bg;
    paletteColor_t shade;
    paletteColor_t text;
    paletteColor_t spinning;
    paletteColor_t notSpinning;

    bool splash;
} cipher_t;

//==============================================================================
// Function Definitions
//==============================================================================

static void cipherEnterMode(void);
static void cipherExitMode(void);
void cipherBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cipherMainLoop(int64_t elapsedUs);
static vec_t RThetaToXY(vec_t, bool);
static void DrawDividerLine(int, int, int);

//==============================================================================
// Const variables
//==============================================================================

static const char DR_NAMESTRING[] = "Secret Decoder Ring";
static const int rotateRPM        = 5;
static const int64_t UsPerDeg     = (60000000 / (rotateRPM * 360));

static const vec_t ScreenCenter = {
    .x = 140,
    .y = 140, // 120 is the default center of the screen
};

static const char* lettersRace[]
    = {"M", "4", "R", "H", "N", "Y", "8", "A", "C", "T", "3", "Z", "K", "W", "F", "1", "X", "G",
       "Q", "E", "O", "J", "L", "D", "9", "B", "V", "S", "5", "7", "I", "P", "2", "0", "6", "U"};

static const char* numbersRace[]
    = {"8",  "35", "6", "23", "15", "9", "33", "3",  "20", "11", "36", "27", "14", "25", "19", "32", "2",  "28",
       "21", "34", "7", "16", "30", "5", "1",  "22", "13", "10", "18", "26", "31", "24", "17", "4",  "29", "12"};

static const char* splashInstructions[] = {
    "Welcome to the Decoder Ring!",    //
    "Left/Right spin unlocked rings.", //
    "A/B lock or unlock a ring.",      //
    "Start shows you this again.",     //
    "Enjoy your secrets!",
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t cipherMode = {
    .modeName = DR_NAMESTRING,  // Assign the name we created here
    .wifiMode = NO_WIFI,        // If we want WiFi. WiFi is expensive computationally/battery-wise, so disable
                                // it if you're not going to use it.
    .overrideUsb = false,       // Overrides the default USB behavior. This is helpful for the game controller
                                // mode but unlikely to be useful for your game.
    .usesAccelerometer = false, // If we're using motion controls
    .usesThermometer   = false, // If we're using the internal thermometer
    .overrideSelectBtn = false, // The select/Menu button has a default behavior. If you want to override it,
                                // you can set this to true but you'll need to re-implement the
                                // 'return to main menu' behavior.
    .fnEnterMode              = cipherEnterMode,              // The enter mode function
    .fnExitMode               = cipherExitMode,               // The exit mode function
    .fnMainLoop               = cipherMainLoop,               // The loop function
    .fnAudioCallback          = NULL,                         // If the mode uses the microphone
    .fnBackgroundDrawCallback = cipherBackgroundDrawCallback, // Draws a section of the display
    .fnEspNowRecvCb           = NULL,                         // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,                         // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,                         // If using advanced USB things.
};

cipher_t* cipher;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Using global vec_t ScreenCenter, convert a vec_t RTheta from <radius in pixels, rotation in degrees> to <x
 * coordinate, y coordinate>, optionally ensuring the coordinates are buffered from the edge of the screen
 *
 * @param RTheta A vec_t in the form <radius in pixels, rotation in degrees>
 * @param TextBuffer A boolean to ensure the coordinates are inside the visible area of the screen
 * @return The X,Y coordinate to draw the text at for the desired angle from the screen center
 */
static vec_t RThetaToXY(vec_t RTheta, bool TextBuffer)
{
    // This function uses the global ScreenCenter vec_t object to allow text to be pushed to the corners of the screen
    // without overrunning it
    vec_t mathPos = {
        .x = ScreenCenter.x + RTheta.x * sin(RTheta.y * M_PI / 180),
        .y = ScreenCenter.y + RTheta.x * cos(RTheta.y * M_PI / 180),
    };
    // ESP_LOGI("func","R: %d, Theta: %d, Xi: %d, Yi: %d", RTheta.x, RTheta.y, mathPos.x, mathPos.y);

    if (TextBuffer)
    {
        // Keep x above 4 and below 280-18=>262
        // Keep y above 4 and below 240-14=>216
        int hypX = fabs(fmax(fmax(mathPos.x - (ScreenCenter.x * 2 - 18), 0), fmax(4 - mathPos.x, 0))
                        / sin(RTheta.y * M_PI / 180));
        int hypY = fabs(fmax(fmax(mathPos.y - (ScreenCenter.y * 2 - 14), 0), fmax(4 - mathPos.y, 0))
                        / cos(RTheta.y * M_PI / 180));
        // ESP_LOGI("trim", "hypX: %d, hypY: %d", hypX, hypY);
        mathPos = (vec_t){
            .x = ScreenCenter.x + (RTheta.x - fmax(hypX, hypY)) * sin(RTheta.y * M_PI / 180),
            .y = ScreenCenter.y + (RTheta.x - fmax(hypX, hypY)) * cos(RTheta.y * M_PI / 180),
        };
        // ESP_LOGI("final", "Xf: %d, Yf: %d", mathPos.x, mathPos.y);
    }

    return mathPos;
}

/**
 * @brief Given an angle in degrees around ScreenCenter, and radius start and end, draw a divider line
 *
 * @param RStart The radius to start the line at
 * @param REnd The radius to end the line at
 * @param ThetaDeg The angle to draw the line at
 */
static void DrawDividerLine(int RStart, int REnd, int ThetaDeg)
{
    vec_t start = RThetaToXY(
        (vec_t){
            .x = RStart,
            .y = ThetaDeg,
        },
        true);

    vec_t end = RThetaToXY(
        (vec_t){
            .x = REnd,
            .y = ThetaDeg,
        },
        false);

    drawLine(start.x, start.y, end.x, end.y, cipher->bg, 0);
}

/**
 * @brief Enter cipher mode, allocate necessary memory
 */
static void cipherEnterMode()
{
    cipher = heap_caps_calloc(1, sizeof(cipher_t), MALLOC_CAP_8BIT);

    cipher->bg          = c001;
    cipher->shade       = c112;
    cipher->text        = c434;
    cipher->spinning    = c555;
    cipher->notSpinning = c222;

    cipher->innerRace           = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    cipher->innerRace->rotating = true;
    cipher->innerRace->lastSpin = 0;
    cipher->innerRace->raceRad  = 100;

    cipher->outerRace           = (cipherRace_t*)heap_caps_calloc(1, sizeof(cipherRace_t), MALLOC_CAP_8BIT);
    cipher->outerRace->rotating = true;
    cipher->outerRace->lastSpin = 0;
    cipher->outerRace->raceRad  = 125;

    cipher->splash = true;

    cipher->ibm = getSysFont();
    initShapes();

    loadWsg(PROTOMEN_SMALL_WHITE_WSG, &cipher->logo, true);
}

/**
 * @brief Exit cipher mode and free memory
 */
static void cipherExitMode()
{
    heap_caps_free(cipher->innerRace);
    heap_caps_free(cipher->outerRace);
    heap_caps_free(cipher);
}

/**
 * @brief Background callback which fills a single color
 *
 * @param x The X coordinate to start drawing at
 * @param y The Y coordinate to start drawing at
 * @param w The width of the section to draw
 * @param h The height of the section to draw
 * @param up unused
 * @param upNum unused
 */
void cipherBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Set background as quickly as possible
    memset(&getPxTftFramebuffer()[(y * w) + x], cipher->bg, w * h);
}

/**
 * @brief The main cipher loop which draws the screen and handles button inputs
 *
 * @param elapsedUs The time since this function was last called
 */
static void cipherMainLoop(int64_t elapsedUs)
{
    if (cipher->splash)
    {
        // Draw the splash screen
        for (int i = 0; i < ARRAY_SIZE(splashInstructions); i++)
        {
            drawText(cipher->ibm, cipher->text, splashInstructions[i], 10 + 15 * i, 75 + 20 * i);
        }
    }
    else
    {
        if (cipher->OffsettingLeft)
        {
            if (cipher->innerRace->rotating)
            {
                cipher->innerRace->lastSpin     = 1;
                cipher->innerRace->timeSpinning = (cipher->innerRace->timeSpinning + elapsedUs * 2) % (UsPerDeg * 360);
            }
            if (cipher->outerRace->rotating)
            {
                cipher->outerRace->lastSpin     = 1;
                cipher->outerRace->timeSpinning = (cipher->outerRace->timeSpinning + elapsedUs * 2) % (UsPerDeg * 360);
            }
        }
        else if (cipher->OffsettingRight)
        {
            if (cipher->innerRace->rotating)
            {
                cipher->innerRace->lastSpin = -1;
                cipher->innerRace->timeSpinning
                    = ((cipher->innerRace->timeSpinning - elapsedUs * 2) + (UsPerDeg * 360)) % (UsPerDeg * 360);
            }
            if (cipher->outerRace->rotating)
            {
                cipher->outerRace->lastSpin = -1;
                cipher->outerRace->timeSpinning
                    = ((cipher->outerRace->timeSpinning - elapsedUs * 2) + (UsPerDeg * 360)) % (UsPerDeg * 360);
            }
        }
        else
        {
            if ((cipher->innerRace->timeSpinning) % (UsPerDeg * 10) > 2)
            {
                cipher->innerRace->timeSpinning
                    = cipher->innerRace->timeSpinning
                      + cipher->innerRace->lastSpin
                            * fmin(elapsedUs,
                                   fmin(cipher->innerRace->timeSpinning % (UsPerDeg * 10),
                                        (UsPerDeg * 10 - (cipher->innerRace->timeSpinning % (UsPerDeg * 10)))));
            }
            if ((cipher->outerRace->timeSpinning) % (UsPerDeg * 10) > 2)
            {
                cipher->outerRace->timeSpinning
                    = cipher->outerRace->timeSpinning
                      + cipher->outerRace->lastSpin
                            * fmin(elapsedUs,
                                   fmin(cipher->outerRace->timeSpinning % (UsPerDeg * 10),
                                        (UsPerDeg * 10 - (cipher->outerRace->timeSpinning % (UsPerDeg * 10)))));
            }
        }
        // Draw the circle that denotes whether the outer race is spinning or not
        if (cipher->outerRace->rotating)
        {
            drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->outerRace->raceRad + 12, cipher->spinning);
        }
        else
        {
            drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->outerRace->raceRad + 12, cipher->notSpinning);
        }

        // Draw filled circle for text to live on
        drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->outerRace->raceRad + 10, cipher->shade);

        // Draw the circle that denotes whether the inner race is spinning or not
        if (cipher->innerRace->rotating)
        {
            drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->innerRace->raceRad - 8, cipher->spinning);
        }
        else
        {
            drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->innerRace->raceRad - 8, cipher->notSpinning);
        }

        // Draw inner circle "cutout" for logo to sit on
        drawCircleFilled(ScreenCenter.x, ScreenCenter.y, cipher->innerRace->raceRad - 10, cipher->bg);

        // Draw Logo
        drawWsg(&cipher->logo, 65, 80, false, false, 0);

        int32_t degPerChar = 360 / ARRAY_SIZE(numbersRace);

        // Draw text dividers that do not move when either race is spinning
        for (int i = 0; i < ARRAY_SIZE(numbersRace); i++)
        {
            // Draw divider lines
            DrawDividerLine(cipher->innerRace->raceRad - 10, cipher->outerRace->raceRad + 10,
                            ((cipher->RaceOffset) / UsPerDeg) + degPerChar * i + (degPerChar / 2));
        }

        // Draw Text on top
        for (int i = 0; i < ARRAY_SIZE(numbersRace); i++)
        {
            // Draw inner race
            vec_t inPos = {
                .x = cipher->innerRace->raceRad,
                .y = ((cipher->innerRace->timeSpinning + cipher->RaceOffset) / UsPerDeg) + degPerChar * i,
            };
            inPos = RThetaToXY(inPos, false);
            drawText(cipher->ibm, cipher->text, lettersRace[i], inPos.x - (textWidth(cipher->ibm, lettersRace[i]) / 2),
                     inPos.y - 5);

            // draw outer race
            vec_t outPos = {
                .x = cipher->outerRace->raceRad,
                .y = ((cipher->outerRace->timeSpinning + cipher->RaceOffset) / UsPerDeg) + degPerChar * i,
            };
            outPos = RThetaToXY(outPos, false);
            drawText(cipher->ibm, cipher->text, numbersRace[i], outPos.x - (textWidth(cipher->ibm, numbersRace[i]) / 2),
                     outPos.y - 5);
        }
    }

    // handle input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (cipher->splash)
        {
            if (evt.down)
            {
                switch (evt.button)
                {
                    default:
                        cipher->splash = false;
                        break;
                }
            }
        }
        else
        {
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_B:
                    {
                        cipher->innerRace->rotating = !cipher->innerRace->rotating;
                        break;
                    }
                    case PB_A:
                    {
                        cipher->outerRace->rotating = !cipher->outerRace->rotating;
                        break;
                    }
                    case PB_LEFT:
                    {
                        cipher->OffsettingLeft = true;
                        break;
                    }
                    case PB_RIGHT:
                    {
                        cipher->OffsettingRight = true;
                        break;
                    }
                    case PB_START:
                        cipher->splash = true;
                        break;
                    default:
                    {
                        break;
                    }
                }
            }
            else
            {
                // The button is released
                switch (evt.button)
                {
                    case PB_LEFT:
                    {
                        cipher->OffsettingLeft = false;
                        break;
                    }
                    case PB_RIGHT:
                    {
                        cipher->OffsettingRight = false;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }
    }
}

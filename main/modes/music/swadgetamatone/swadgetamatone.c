#include "swadgetamatone.h"

#include "mainMenu.h"
#include "swadge2024.h"
#include "waveTables.h"

static const char swadgetamatoneName[] = "Swadgetamatone";
static const char sttSingLbl[]         = "Sing";
static const char sttTutorialLbl[]     = "Tutorial";
static const char sttExitLbl[]         = "Exit";

typedef enum
{
    STT_MENU,
    STT_TUTORIAL,
    STT_SINGING,
} sttState_t;

typedef enum
{
    NONE,
    HIGH,
    MID,
    LOW,
} sttOctave_t;

typedef struct
{
    sttState_t state;

    menu_t* menu;
    menuMegaRenderer_t* menuRenderer;

    vec_t touchpad;
    sttOctave_t octavePressed;

    int64_t noteStateElapsedUs;
    int32_t freq;

    synthOscillator_t* oscillators[1];
    synthOscillator_t sttOsc;

    wsg_t background;
} swadgetamatone_t;
swadgetamatone_t* stt = NULL;

static void sttEnterMode(void);
static void sttExitMode(void);
static void sttMenu(const char* label, bool selected, uint32_t value);
static void sttMainLoop(int64_t elapsedUs);
static void sttBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void sttDrawMouthForegound(int16_t xRadius, int16_t yRadius, paletteColor_t color);
static void sttDacCallback(uint8_t* samples, int16_t len);
static int8_t sttGenerateWaveform(uint16_t idx, void* data);

swadgeMode_t swadgetamatoneMode = {
    .modeName                 = swadgetamatoneName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = sttEnterMode,
    .fnExitMode               = sttExitMode,
    .fnMainLoop               = sttMainLoop,
    .fnBackgroundDrawCallback = sttBackgroundDrawCallback,
    .fnDacCb                  = sttDacCallback,
};

#define LOW_MIN_HZ  110
#define LOW_MAX_HZ  208
#define MID_MIN_HZ  220
#define MID_MAX_HZ  415
#define HIGH_MIN_HZ 440
#define HIGH_MAX_HZ 831

/// Time to lerp pitch from (100 - PITCH_ATTACK_BEND_PCT)% to target pitch at button press
#define PITCH_LERP_US 200000
/// Percentage of the pitch to lerp linearly over PITCH_LERP_US at button press
#define PITCH_ATTACK_BEND_PCT 15
/// Time to lerp volume from 0 to target volume at button press
#define VOLUME_LERP_US 100000

static void sttEnterMode(void)
{
    setFrameRateUs(1000000 / 60);

    stt        = heap_caps_calloc(1, sizeof(swadgetamatone_t), MALLOC_CAP_8BIT);
    stt->state = STT_SINGING; // TODO: Go to tutorial on first run

    stt->touchpad.x = 512;
    stt->touchpad.y = 0;

    swSynthInitOscillatorWave(&stt->sttOsc, sttGenerateWaveform, 0, 0, 0);
    stt->oscillators[0] = &stt->sttOsc;

    stt->menu = initMenu(swadgetamatoneName, sttMenu);
    addSingleItemToMenu(stt->menu, sttSingLbl);
    addSingleItemToMenu(stt->menu, sttTutorialLbl);
    addSingleItemToMenu(stt->menu, sttExitLbl);
    stt->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);

    loadWsg(STT_BACKGROUND_WSG, &stt->background, false);
}

static void sttExitMode(void)
{
    deinitMenuMegaRenderer(stt->menuRenderer);
    deinitMenu(stt->menu);
    freeWsg(&stt->background);
    heap_caps_free(stt);
}

static void sttMenu(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == sttTutorialLbl)
        {
            // TODO
        }
        else if (label == sttSingLbl)
        {
            stt->state = STT_SINGING;
        }
        else if (label == sttExitLbl)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}

static void sttMainLoop(int64_t elapsedUs)
{
    stt->noteStateElapsedUs += elapsedUs;

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (stt->state == STT_MENU)
        {
            stt->menu = menuButton(stt->menu, evt);
        }
        else
        {
            if (evt.button == PB_START && evt.down)
            {
                stt->state = STT_MENU;
            }

            if (evt.button == PB_LEFT)
            {
                if (evt.down)
                {
                    stt->octavePressed      = LOW;
                    stt->noteStateElapsedUs = 0;
                }
                else if (!evt.down && stt->octavePressed == LOW)
                {
                    stt->octavePressed      = NONE;
                    stt->noteStateElapsedUs = 0;
                }
            }
            else if (evt.button == PB_UP)
            {
                if (evt.down)
                {
                    stt->octavePressed      = MID;
                    stt->noteStateElapsedUs = 0;
                }
                else if (!evt.down && stt->octavePressed == MID)
                {
                    stt->octavePressed      = NONE;
                    stt->noteStateElapsedUs = 0;
                }
            }
            else if (evt.button == PB_RIGHT)
            {
                if (evt.down)
                {
                    stt->octavePressed      = HIGH;
                    stt->noteStateElapsedUs = 0;
                }
                else if (!evt.down && stt->octavePressed == HIGH)
                {
                    stt->octavePressed      = NONE;
                    stt->noteStateElapsedUs = 0;
                }
            }
        }
    }

    switch (stt->state)
    {
        case STT_MENU:
        {
            drawMenuMega(stt->menu, stt->menuRenderer, elapsedUs);
            break;
        }

        case STT_TUTORIAL:
        {
            // TODO
            break;
        }

        case STT_SINGING:
        {
            int32_t angle, radius, intensity;
            if (getTouchJoystick(&angle, &radius, &intensity))
            {
                getTouchCartesianSquircle(angle, radius, &stt->touchpad.x, &stt->touchpad.y);
            }

            if (stt->octavePressed != NONE)
            {
                uint16_t minHz, maxHz;
                switch (stt->octavePressed)
                {
                    case LOW:
                        minHz = LOW_MIN_HZ;
                        maxHz = LOW_MAX_HZ;
                        break;
                    case MID:
                        minHz = MID_MIN_HZ;
                        maxHz = MID_MAX_HZ;
                        break;
                    case HIGH:
                        minHz = HIGH_MIN_HZ;
                        maxHz = HIGH_MAX_HZ;
                        break;
                    default:
                        minHz = 0;
                        maxHz = 0;
                        break;
                }

                uint32_t noteHz = stt->touchpad.x * (maxHz - minHz) / 1023 + minHz;
                uint8_t volume  = stt->touchpad.y * 255 / 1023;

                if (stt->noteStateElapsedUs < PITCH_LERP_US)
                {
                    noteHz -= noteHz / 100 * PITCH_ATTACK_BEND_PCT * (PITCH_LERP_US - stt->noteStateElapsedUs)
                              / PITCH_LERP_US;
                }
                if (stt->noteStateElapsedUs < VOLUME_LERP_US)
                {
                    volume = volume * ((float)stt->noteStateElapsedUs / VOLUME_LERP_US);
                }

                swSynthSetFreq(&stt->sttOsc, noteHz);
                swSynthSetVolume(&stt->sttOsc, volume);
            }
            else
            {
                swSynthSetVolume(&stt->sttOsc, 0);
            }

            int16_t xRadius = CLAMP(stt->touchpad.x * TFT_WIDTH / 1023, 0, TFT_WIDTH - 6) / 2;
            int16_t yRadius = CLAMP(stt->touchpad.y * TFT_HEIGHT / 1023, 0, TFT_HEIGHT - 6) / 2;
            sttDrawMouthForegound(xRadius, yRadius, c444);
            drawEllipse(TFT_WIDTH / 2, TFT_HEIGHT / 2, xRadius, yRadius, c000);

            break;
        }
    }
}

static void sttBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    paletteColor_t* tftFb = getPxTftFramebuffer();
    memcpy(&tftFb[y * TFT_WIDTH + x], &stt->background.px[y * TFT_WIDTH + x], w * h * sizeof(paletteColor_t));
}

/// Draws an inverted filled ellipse centered on the display.
/// Adapted from the fast filled ellipse algorithm found here: https://stackoverflow.com/a/10322607
static void sttDrawMouthForegound(int16_t xRadius, int16_t yRadius, paletteColor_t color)
{
    int32_t ww   = xRadius * xRadius;
    int32_t hh   = yRadius * yRadius;
    int64_t hhww = hh * ww;
    int16_t x0   = xRadius;
    int16_t dx   = 0;

    // Draw lines at origin
    drawLineFast(0, TFT_HEIGHT / 2, TFT_WIDTH / 2 - xRadius, TFT_HEIGHT / 2, color);
    drawLineFast(TFT_WIDTH / 2 + xRadius, TFT_HEIGHT / 2, TFT_WIDTH - 1, TFT_HEIGHT / 2, color);

    // Draw all 4 quadrants at the same time, away from the origin
    for (int16_t y = 1; y <= yRadius; y++)
    {
        int16_t x1 = x0 - (dx - 1); // try slopes of dx - 1 or more
        for (; x1 > 0; x1--)
        {
            if (x1 * x1 * hh + y * y * ww <= hhww)
            {
                break;
            }
        }
        dx = x0 - x1; // current approximation of the slope
        x0 = x1;

        drawLineFast(0, TFT_HEIGHT / 2 + y, TFT_WIDTH / 2 - x0 - 1, TFT_HEIGHT / 2 + y, color);
        drawLineFast(TFT_WIDTH / 2 + x0 + 1, TFT_HEIGHT / 2 + y, TFT_WIDTH, TFT_HEIGHT / 2 + y, color);
        drawLineFast(0, TFT_HEIGHT / 2 - y, TFT_WIDTH / 2 - x0 - 1, TFT_HEIGHT / 2 - y, color);
        drawLineFast(TFT_WIDTH / 2 + x0 + 1, TFT_HEIGHT / 2 - y, TFT_WIDTH, TFT_HEIGHT / 2 - y, color);
    }

    // Fill top and bottom outside of ellipse
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT / 2 - yRadius, color);
    fillDisplayArea(0, TFT_HEIGHT / 2 + yRadius, TFT_WIDTH, TFT_HEIGHT, color);
}

static void sttDacCallback(uint8_t* samples, int16_t len)
{
    for (int16_t i = 0; i < len; i++)
    {
        samples[i] = swSynthMixOscillators(stt->oscillators, ARRAY_SIZE(stt->oscillators));
    }
}

/// Precomputed table of a fourier series with terms:
/// a: [0, 0, -0.37, -0.75, -1, -1, -1, -1]
/// b: [0, 1, 0.58, -0.1, -0.7, -1, -1, -1]
static const int8_t sttWaveTable[]
    = {108, 115, 121, 125, 127, 127, 126, 122, 117, 111, 103, 93,  83,  71,  59,  46,  34,  21,  8,   -4,  -16, -27,
       -37, -45, -52, -58, -62, -65, -67, -67, -66, -63, -60, -56, -50, -45, -38, -32, -25, -19, -13, -7,  -2,  3,
       7,   10,  13,  15,  16,  16,  16,  15,  14,  13,  11,  9,   7,   5,   3,   2,   1,   0,   0,   0,   0,   2,
       3,   5,   7,   10,  13,  16,  19,  22,  24,  27,  29,  31,  32,  33,  34,  34,  34,  33,  32,  30,  28,  26,
       24,  22,  19,  17,  15,  13,  11,  10,  9,   9,   9,   9,   10,  11,  13,  15,  17,  19,  21,  24,  26,  29,
       31,  33,  34,  35,  36,  37,  37,  37,  36,  35,  33,  32,  30,  28,  26,  24,  22,  20,  18,  17,  16,  15,
       15,  15,  16,  16,  18,  19,  21,  23,  25,  27,  30,  32,  34,  35,  37,  38,  39,  40,  40,  39,  39,  38,
       36,  35,  33,  31,  29,  27,  25,  23,  21,  20,  18,  18,  17,  18,  18,  19,  21,  22,  24,  27,  29,  32,
       35,  37,  40,  42,  44,  46,  47,  48,  48,  48,  48,  47,  45,  43,  41,  39,  36,  34,  31,  29,  27,  25,
       24,  23,  23,  24,  25,  26,  29,  31,  35,  39,  43,  47,  52,  56,  61,  65,  68,  72,  74,  75,  76,  76,
       75,  72,  69,  64,  59,  53,  46,  38,  30,  22,  14,  5,   -3,  -10, -17, -22, -27, -30, -33, -33, -32, -30,
       -26, -21, -14, -6,  3,   13,  24,  35,  46,  58,  69,  80,  91,  100};

static int8_t sttGenerateWaveform(uint16_t idx, void* data)
{
    return sttWaveTable[idx];
}

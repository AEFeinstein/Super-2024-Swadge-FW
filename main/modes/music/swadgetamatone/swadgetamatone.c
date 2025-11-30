#include "swadgetamatone.h"

#include "mainMenu.h"
#include "swadge2024.h"
#include "waveTables.h"

static const char swadgetamatoneName[] = "Swadgetamatone";
static const char sttShowNoteOn[]      = "Note Display: On";
static const char sttShowNoteOff[]     = "Note Display: Off";

static const char sttNvsNamespace[]          = "stt";
static const char sttNvsKeyTotalTimePlayed[] = "total_time_played";
static const char sttNvsKeyShowNote[]        = "show_note";

const trophyData_t sttTrophies[] = {
    {
        .title       = "Say \"AHHHHH\"",
        .description = "Play your first note",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Beginning Of A Career",
        .description = "Play for 1 minute total",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Bad Roommate",
        .description = "Play for 10 minutes total",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 10,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Swadgetamatone Master",
        .description = "Play for 60 minutes total",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 60,
        .hidden      = false,
        .identifier  = NULL,
    },
};
const trophySettings_t sttTrophySettings = {
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = swadgetamatoneName,
};
const trophyDataList_t sttTrophyData = {
    .settings = &sttTrophySettings,
    .list     = sttTrophies,
    .length   = ARRAY_SIZE(sttTrophies),
};

typedef struct
{
    vec_t touchpad;
    uint16_t buttonState;
    /// Number from 1-4 for low-high. 0 means none pressed.
    uint8_t octavePressed;

    int32_t showNote;
    int64_t noteMessageTimeUs;

    /// Time since a button to play a note was pressed or released
    int64_t noteStateElapsedUs;
    int32_t freq;

    int32_t totalTimePlayedMs;

    bool blinkInProgress;
    int64_t blinkTimerUs;

    float cheekFlushRate;

    synthOscillator_t* oscillators[1];
    synthOscillator_t sttOsc;

    wsg_t background;
    font_t font;
    font_t fontOutline;

    led_t leds[CONFIG_NUM_LEDS];
} swadgetamatone_t;
swadgetamatone_t* stt = NULL;

static void sttEnterMode(void);
static void sttExitMode(void);
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
    .trophyData               = &sttTrophyData,
};

static const char* noteNames[]   = {"F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#", "E", "F"};
static const int32_t noteFreqs[] = {87, 93, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175};
#define STT_MIN_HZ noteFreqs[0]
#define STT_MAX_HZ noteFreqs[ARRAY_SIZE(noteFreqs) - 1]

#define STT_MIN_VOLUME 10
#define STT_MAX_VOLUME 255

/// Time to lerp pitch from (100 - PITCH_ATTACK_BEND_PCT)% to target pitch at button press
#define PITCH_LERP_US 200000
/// Percentage of the pitch to lerp linearly over PITCH_LERP_US at button press
#define PITCH_ATTACK_BEND_PCT 15
/// Time to lerp volume from 0 to target volume at button press
#define VOLUME_LERP_US 100000

#define EYES_SLOT_BLINK   3
#define EYES_SLOT_DEFAULT 4
/// Eyes for each octave are this + the octave number (1-4). +0 is the default eyes.
#define EYES_SLOT_OCTAVES 4

#define BLINK_DELAY_MIN_US  2000000
#define BLINK_DELAY_MAX_US  6000000
#define BLINK_LENGTH_MIN_US 100000
#define BLINK_LENGTH_MAX_US 200000

#define CHEEK_FLUSH_DELAY_US 1000000
#define CHEEK_FLUSH_TIME_US  6000000

static void sttEnterMode(void)
{
    setFrameRateUs(1000000 / 60);

    stt = heap_caps_calloc(1, sizeof(swadgetamatone_t), MALLOC_CAP_8BIT);

    stt->touchpad.x = 512;
    stt->touchpad.y = 512;
    // Prevent mouth from animating closed at mode launch
    stt->noteStateElapsedUs = VOLUME_LERP_US;
    // Prevent immediate blink at mode launch
    stt->blinkInProgress = true;

    readNamespaceNvs32(sttNvsNamespace, sttNvsKeyTotalTimePlayed, &stt->totalTimePlayedMs);
    readNamespaceNvs32(sttNvsNamespace, sttNvsKeyShowNote, &stt->showNote);

    // Initial freq/volume values don't matter since they'll get overwritten by touchpad data
    swSynthInitOscillatorWave(&stt->sttOsc, sttGenerateWaveform, 0, 0, 0);
    stt->oscillators[0] = &stt->sttOsc;

    loadWsg(STT_BACKGROUND_WSG, &stt->background, false);

    loadFont(OXANIUM_FONT, &stt->font, false);
    makeOutlineFont(&stt->font, &stt->fontOutline, false);

    ch32v003WriteBitmapAsset(EYES_SLOT_BLINK, STT_EYES_BLINK_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_DEFAULT, STT_EYES_DEFAULT_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_OCTAVES + 1, STT_EYES_PLAYING_1_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_OCTAVES + 2, STT_EYES_PLAYING_2_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_OCTAVES + 3, STT_EYES_PLAYING_3_GS);
    ch32v003WriteBitmapAsset(EYES_SLOT_OCTAVES + 4, STT_EYES_PLAYING_4_GS);

    ch32v003SelectBitmap(EYES_SLOT_DEFAULT);
}

static void sttExitMode(void)
{
    writeNamespaceNvs32(sttNvsNamespace, sttNvsKeyTotalTimePlayed, stt->totalTimePlayedMs);

    freeWsg(&stt->background);
    freeFont(&stt->font);
    freeFont(&stt->fontOutline);
    heap_caps_free(stt);
}

static void sttMainLoop(int64_t elapsedUs)
{
    stt->noteStateElapsedUs += elapsedUs;

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        stt->buttonState = evt.state;

        if (evt.button == PB_A && evt.down)
        {
            stt->showNote          = !stt->showNote;
            stt->noteMessageTimeUs = 1000000;
            writeNamespaceNvs32(sttNvsNamespace, sttNvsKeyShowNote, stt->showNote);
        }

        if (evt.button == PB_START && evt.down)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }

    uint8_t octavePressed = 0;
    if (stt->buttonState & PB_DOWN)
    {
        octavePressed = 1;
    }
    else if (stt->buttonState & PB_LEFT)
    {
        octavePressed = 2;
    }
    else if (stt->buttonState & PB_RIGHT)
    {
        octavePressed = 3;
    }
    else if (stt->buttonState & PB_UP)
    {
        octavePressed = 4;
    }

    if (octavePressed != stt->octavePressed)
    {
        if (octavePressed != 0)
        {
            trophyUpdate(&sttTrophies[0], 1, true);
        }
        else
        {
            int32_t previousTotalTimePlayedMs = stt->totalTimePlayedMs;
            stt->totalTimePlayedMs += stt->noteStateElapsedUs / 1000;

            int32_t minutesPlayed = stt->totalTimePlayedMs / (60 * 1000);
            // Only update trophy progress once per minute of play time, since the milestone calls write to NVS
            if (previousTotalTimePlayedMs / (60 * 1000) < minutesPlayed)
            {
                writeNamespaceNvs32(sttNvsNamespace, sttNvsKeyTotalTimePlayed, stt->totalTimePlayedMs);
                trophyUpdateMilestone(&sttTrophies[1], minutesPlayed, 100);
                trophyUpdateMilestone(&sttTrophies[2], minutesPlayed, 20);
                trophyUpdateMilestone(&sttTrophies[3], minutesPlayed, 25);
            }
        }

        stt->noteStateElapsedUs = 0;
        stt->octavePressed      = octavePressed;
        ch32v003SelectBitmap(EYES_SLOT_OCTAVES + octavePressed);
    }

    if (stt->octavePressed == 0)
    {
        stt->blinkTimerUs -= elapsedUs;
        if (stt->blinkTimerUs <= 0)
        {
            if (stt->blinkInProgress)
            {
                ch32v003SelectBitmap(EYES_SLOT_DEFAULT);
                stt->blinkTimerUs = esp_random() % (BLINK_DELAY_MAX_US - BLINK_DELAY_MIN_US) + BLINK_DELAY_MIN_US;
            }
            else
            {
                ch32v003SelectBitmap(EYES_SLOT_BLINK);
                stt->blinkTimerUs = esp_random() % (BLINK_LENGTH_MAX_US - BLINK_LENGTH_MIN_US) + BLINK_LENGTH_MIN_US;
            }
            stt->blinkInProgress = !stt->blinkInProgress;
        }
    }

    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        getTouchCartesianSquircle(angle, radius, &stt->touchpad.x, &stt->touchpad.y);
    }

    const char* note = NULL;
    uint8_t volume;
    if (stt->octavePressed != 0)
    {
        int32_t noteHz = stt->touchpad.x * (STT_MAX_HZ - STT_MIN_HZ) / 1023 + STT_MIN_HZ;

        int32_t closestFreq = 0;
        for (int i = 0; i < ARRAY_SIZE(noteFreqs); i++)
        {
            if (ABS(noteHz - noteFreqs[i]) < ABS(noteHz - closestFreq))
            {
                closestFreq = noteFreqs[i];
                note        = noteNames[i];
            }
        }

        noteHz = noteHz << (stt->octavePressed - 1);
        if (stt->noteStateElapsedUs < PITCH_LERP_US)
        {
            noteHz -= noteHz / 100 * PITCH_ATTACK_BEND_PCT * (PITCH_LERP_US - stt->noteStateElapsedUs) / PITCH_LERP_US;
        }
        swSynthSetFreq(&stt->sttOsc, noteHz);

        volume = MAX(stt->touchpad.y * STT_MAX_VOLUME / 1023, STT_MIN_VOLUME);
        if (stt->noteStateElapsedUs < VOLUME_LERP_US)
        {
            volume = volume * ((float)stt->noteStateElapsedUs / VOLUME_LERP_US);
        }

        stt->cheekFlushRate
            = CLAMP(1.0f - ((float)stt->noteStateElapsedUs - CHEEK_FLUSH_DELAY_US) / CHEEK_FLUSH_TIME_US, 0, 1.0f);
    }
    else
    {
        if (stt->noteStateElapsedUs < VOLUME_LERP_US)
        {
            volume = stt->touchpad.y * STT_MAX_VOLUME / 1023;
            volume -= volume * ((float)stt->noteStateElapsedUs / VOLUME_LERP_US);
        }
        else
        {
            volume = 0;
        }
    }

    swSynthSetVolume(&stt->sttOsc, volume);

    for (int i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        stt->leds[i].r = volume;
        if (i == 1 || i == 4) // Bottom 2 LED indices
        {
            stt->leds[i].g = volume * stt->cheekFlushRate;
            stt->leds[i].b = volume * stt->cheekFlushRate;
        }
        else
        {
            stt->leds[i].g = volume;
            stt->leds[i].b = volume;
        }
    }
    setLeds(stt->leds, CONFIG_NUM_LEDS);

    int16_t xRadius = CLAMP(stt->touchpad.x * TFT_WIDTH / 1023, 8, TFT_WIDTH - 6) / 2;
    // Use the calculated volume for height so it animates open/closed with the volume fade in/out
    int16_t yRadius = MIN(volume * TFT_HEIGHT / STT_MAX_VOLUME, TFT_HEIGHT - 6) / 2;
    sttDrawMouthForegound(xRadius, yRadius, c444);
    drawEllipse(TFT_WIDTH / 2, TFT_HEIGHT / 2, xRadius, yRadius, c000);

    const char* msg = NULL;
    if (stt->noteMessageTimeUs > 0)
    {
        if (stt->showNote)
        {
            msg = sttShowNoteOn;
        }
        else
        {
            msg = sttShowNoteOff;
        }

        stt->noteMessageTimeUs -= elapsedUs;
    }
    else if (stt->showNote)
    {
        msg = note;
    }
    if (msg != NULL)
    {
        drawText(&stt->font, c444, msg, 20, TFT_HEIGHT - stt->font.height);
        drawText(&stt->fontOutline, c222, msg, 20, TFT_HEIGHT - stt->fontOutline.height);
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

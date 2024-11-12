#include "introMode.h"

#include <esp_log.h>
#include <inttypes.h>
#include <stdint.h>

#include "mainMenu.h"
#include "fs_font.h"
#include "fs_wsg.h"
#include "fs_json.h"
#include "swadge2024.h"
#include "macros.h"
#include "menu.h"
#include "font.h"
#include "shapes.h"
#include "wsg.h"

#include "embeddedOut.h"

static void introEnterMode(void);
static void introExitMode(void);
static void introMainLoop(int64_t elapsedUs);
static void introBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void introAudioCallback(uint16_t* samples, uint32_t sampleCnt);

// static void introMenuCb(const char*, bool selected, uint32_t settingVal);
static void introTutorialCb(const tutorialState_t* state, const tutorialStep_t* prev, const tutorialStep_t* next,
                            bool backtrack);
static bool introCheckQuickSettingsTrigger(const tutorialState_t* state, const tutorialTrigger_t* trigger);

static void introDrawSwadge(int64_t elapsedUs, int16_t x, int16_t y, buttonBit_t buttons, touchJoystick_t joysticks);

#define ALL_BUTTONS  (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT)
#define DPAD_BUTTONS (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT)

// #define ALL_TOUCH (TB_CENTER | TB_RIGHT | TB_UP | TB_LEFT | TB_DOWN)

static const char startTitle[]        = "Welcome!";
static const char holdLongerMessage[] = "Almost! Keep holding SELECT for one second to exit.";
static const char endTitle[]          = "Exiting Modes";
static const char endDetail[]         = "You are now Swadge Certified! Remember, with great power comes great "
                                        "responsibility. Hold SELECT to exit the tutorial and get started!";

static const tutorialStep_t buttonsSteps[] = {
    {
        .trigger = {
            .type = BUTTON_PRESS_ANY,
            .buttons = ALL_BUTTONS,
        },
        .title = startTitle,
        .detail = "Press any button to continue",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS_ALL,
            .buttons = DPAD_BUTTONS,
        },
        .title = "The D-Pad",
        .detail = "These four buttons on the left side of the Swadge are the D-Pad. Use them to navigate. Try them all out!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_A,
        },
        .title = "A Button",
        .detail = "The A Button is used to select, or trigger a primary action. Give it a try!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_B,
        },
        .title = "B Button",
        .detail = "The B Button is used to go back, or trigger a secondary action. Back it up!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_START,
        },
        .title = "Pause Button",
        .detail = "The Pause button is mode-specific, but usually pauses a game or performs a special function. Give it a try!",
    },
    // TODO draw touchpad
    // TODO write actual text
    {
        .trigger = {
            .type = TOUCH_SPIN,
            .intData = -2,
        },
        .title = "Touchpad",
        .detail = "Spin clockwise twice!"
    },
    // TODO Draw IMU
    // TODO write actual text
    {
        .trigger = {
            .type = IMU_SHAKE,
        },
        .title = "Tilt Controls",
        .detail = "Wiggle around!"
    },
    // TODO microphone
    {
        .trigger = {
            .type = MIC_LOUD,
        },
        .title = "Microphone",
        .detail = "Objetion!",
    },
    // TODO speaker
    // TODO flip to SPK mode
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_A,
        },
        .title = "Speaker",
        .detail = "Try the volume dial, press A when done",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_SELECT,
        },
        .title = "Menu Button",
        .detail = "The Menu button is special! A short press opens up the Quick Settings menu.",
    },
    {
        .trigger = {
            .type = BUTTON_RELEASE,
            .buttons = PB_SELECT,
        },
        .backtrack = {
            .type = TIME_PASSED,
            .intData = EXIT_TIME_US,
        },
        .backtrackSteps = 1,
        .backtrackMessage = "That's too long! Try again!",
        .title = "Pause Button",
        .detail = "Now let go!",
    },
    {
        .trigger = {
            .type = CUSTOM_TRIGGER,
            .custom.checkFn = introCheckQuickSettingsTrigger,
        },
        .title = "",
        .detail = "This is the Quick Settings Menu! It's available in all modes, except for the Main Menu and USB Gamepad. Press the Menu button again to close it.",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_SELECT,
        },
        .title = endTitle,
        .detail = endDetail,
    },
    {
        .trigger = {
            .type = TIME_PASSED,
            .intData = EXIT_TIME_US / 3,
        },
        .backtrack = {
            .type = BUTTON_RELEASE,
            .buttons = PB_SELECT,
        },
        .backtrackSteps = 1,
        .backtrackMessage = holdLongerMessage,
        .backtrackMessageTime = 3000000,
        .title = "Exiting in 3...",
        .detail = "Keep holding!",
    },
    {
        .trigger = {
            .type = TIME_PASSED,
            .intData = EXIT_TIME_US / 3,
        },
        .backtrack = {
            .type = BUTTON_RELEASE,
            .buttons = PB_SELECT,
        },
        .backtrackSteps = 2,
        .backtrackMessage = holdLongerMessage,
        .backtrackMessageTime = 3000000,
        .title = "Exiting in 2...",
        .detail = "Keep holding!",
    },
    {
        .trigger = {
            .type = TIME_PASSED,
            .intData = EXIT_TIME_US / 3,
        },
        .backtrack = {
            .type = BUTTON_RELEASE,
            .buttons = PB_SELECT,
        },
        .backtrackSteps = 3,
        .backtrackMessage = holdLongerMessage,
        .backtrackMessageTime = 3000000,
        .title = "Exiting in 1...",
        .detail = "Keep holding!",
    },
    {
        .trigger = {
            .type = NO_TRIGGER
        },
        .title = "Exiting in 0...",
        .detail = "Goodbye!",
    },
};

static const char introName[] = "Tutorial";

swadgeMode_t introMode = {
    .modeName                 = introName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = true,
    .fnEnterMode              = introEnterMode,
    .fnExitMode               = introExitMode,
    .fnMainLoop               = introMainLoop,
    .fnAudioCallback          = introAudioCallback,
    .fnBackgroundDrawCallback = introBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t rot;

    bool flipLR;
    bool flipUD;

    wsg_t* icon;
    bool visible;
} iconPos_t;

typedef struct
{
    font_t smallFont;
    font_t bigFont;
    font_t logoFont;
    font_t logoFontOutline;

    // Holds all the internal tutorial state
    tutorialState_t tut;

    struct
    {
        struct
        {
            wsg_t a;
            wsg_t b;
            wsg_t menu;
            wsg_t pause;
            wsg_t up;
        } button;

        struct
        {
            wsg_t base;
            wsg_t joystick;
            wsg_t spin;
        } touch;
    } icon;

    buttonBit_t buttons;
    touchJoystick_t joysticks;

    iconPos_t buttonIcons[8];
    int16_t swadgeViewWidth;
    int16_t swadgeViewHeight;

    bool quickSettingsOpened;

#ifdef CUSTOM_INTRO_SOUND
    bool playingSound;
    uint8_t* sound;
    size_t soundSize;
    size_t soundProgress;
    bool introComplete;
#endif

    menu_t* bgMenu;
    menuManiaRenderer_t* renderer;

    // Microphone test
    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    uint8_t samplesProcessed;
    uint16_t maxValue;

} introVars_t;

static introVars_t* iv;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void introEnterMode(void)
{
    iv = calloc(1, sizeof(introVars_t));

    loadFont("ibm_vga8.font", &iv->smallFont, false);
    loadFont("righteous_150.font", &iv->bigFont, false);
    loadFont("retro_logo.font", &iv->logoFont, false);
    makeOutlineFont(&iv->logoFont, &iv->logoFontOutline, false);

    loadWsg("button_a.wsg", &iv->icon.button.a, false);
    loadWsg("button_b.wsg", &iv->icon.button.b, false);
    loadWsg("button_menu.wsg", &iv->icon.button.menu, false);
    loadWsg("button_pause.wsg", &iv->icon.button.pause, false);
    loadWsg("button_up.wsg", &iv->icon.button.up, false);

    iv->bgMenu   = initMenu(startTitle, NULL);
    iv->renderer = initMenuManiaRenderer(&iv->bigFont, NULL, &iv->smallFont);

    // up
    iv->buttonIcons[0].icon    = &iv->icon.button.up;
    iv->buttonIcons[0].visible = true;
    iv->buttonIcons[0].x       = 15;
    iv->buttonIcons[0].y       = 5;

    // down
    iv->buttonIcons[1].icon    = &iv->icon.button.up;
    iv->buttonIcons[1].flipUD  = true;
    iv->buttonIcons[1].visible = true;
    iv->buttonIcons[1].x       = 15;
    iv->buttonIcons[1].y       = 25;

    // left
    iv->buttonIcons[2].icon    = &iv->icon.button.up;
    iv->buttonIcons[2].rot     = 270;
    iv->buttonIcons[2].visible = true;
    iv->buttonIcons[2].x       = 5;
    iv->buttonIcons[2].y       = 15;

    // right
    iv->buttonIcons[3].icon    = &iv->icon.button.up;
    iv->buttonIcons[3].rot     = 90;
    iv->buttonIcons[3].visible = true;
    iv->buttonIcons[3].x       = 25;
    iv->buttonIcons[3].y       = 15;

    // a
    iv->buttonIcons[4].icon    = &iv->icon.button.a;
    iv->buttonIcons[4].visible = true;
    iv->buttonIcons[4].x       = 105;
    iv->buttonIcons[4].y       = 10;

    // b
    iv->buttonIcons[5].icon    = &iv->icon.button.b;
    iv->buttonIcons[5].visible = true;
    iv->buttonIcons[5].x       = 95;
    iv->buttonIcons[5].y       = 20;

    // start
    iv->buttonIcons[6].icon    = &iv->icon.button.pause;
    iv->buttonIcons[6].visible = true;
    iv->buttonIcons[6].x       = 70;
    iv->buttonIcons[6].y       = 15;

    // select
    iv->buttonIcons[7].icon    = &iv->icon.button.menu;
    iv->buttonIcons[7].visible = true;
    iv->buttonIcons[7].x       = 50;
    iv->buttonIcons[7].y       = 15;

    iv->swadgeViewWidth  = iv->buttonIcons[5].x + iv->buttonIcons[5].icon->w + 5;
    iv->swadgeViewHeight = iv->buttonIcons[1].y + iv->buttonIcons[1].icon->h + 5;

    tutorialSetup(&iv->tut, introTutorialCb, buttonsSteps, ARRAY_SIZE(buttonsSteps), iv);

#ifdef CUSTOM_INTRO_SOUND
    iv->sound         = NULL;
    iv->playingSound  = false;
    iv->introComplete = false;
#endif

    // Init CC
    InitColorChord(&iv->end, &iv->dd);
    iv->maxValue = 1;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void introExitMode(void)
{
    deinitMenuManiaRenderer(iv->renderer);
    deinitMenu(iv->bgMenu);

    freeFont(&iv->smallFont);
    freeFont(&iv->bigFont);
    freeFont(&iv->logoFont);
    freeFont(&iv->logoFontOutline);

    freeWsg(&iv->icon.button.a);
    freeWsg(&iv->icon.button.b);
    freeWsg(&iv->icon.button.menu);
    freeWsg(&iv->icon.button.pause);
    freeWsg(&iv->icon.button.up);

#ifdef CUSTOM_INTRO_SOUND
    if (iv->sound != NULL)
    {
        free(iv->sound);
    }
#endif

    free(iv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void introMainLoop(int64_t elapsedUs)
{
    // clearPxTft();

#ifdef CUSTOM_INTRO_SOUND
    if (iv->playingSound || !iv->introComplete)
    {
        int64_t songTime     = (iv->soundSize * 1000000 + DAC_SAMPLE_RATE_HZ - 1) / DAC_SAMPLE_RATE_HZ;
        int64_t animTime     = (songTime * 2 / 3);
        static int64_t timer = 0;

        shadeDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, 3, c234);

        char title[64] = {0};
        char sub[64]   = {0};

        strcat(title, "MAGFest");
        strcat(sub, "Swadge 2025");
        int64_t titleTicksPerChar = ((animTime) / strlen(title));
        int64_t subTicksPerChar   = ((animTime) / strlen(sub));

        int16_t titleWidth = textWidth(&iv->logoFont, title);
        int16_t titleX     = (TFT_WIDTH - titleWidth) / 2;
        int16_t titleY     = (TFT_HEIGHT - iv->bigFont.height - iv->smallFont.height - 6) / 2;

        int16_t subWidth = textWidth(&iv->logoFont, sub);
        int16_t subX     = (TFT_WIDTH - subWidth) / 2;
        int16_t subY     = titleY + iv->bigFont.height + 5;

        int titleLen = MIN(strlen(title), timer / titleTicksPerChar);
        int subLen   = MIN(strlen(sub), timer / subTicksPerChar);
        // Trim the string
        title[titleLen] = '\0';
        sub[subLen]     = '\0';

        drawText(&iv->logoFont, c555, title, titleX, titleY);
        drawText(&iv->logoFont, c000, title, titleX - 1, titleY + 1);

        drawText(&iv->logoFont, c555, sub, subX, subY);
        drawText(&iv->logoFont, c000, sub, subX - 1, subY + 1);

        timer += elapsedUs;

        // Return early -- Don't do the rest of the stuff yet
        if (iv->playingSound)
        {
            return;
        }
    }
#endif

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
#ifdef CUSTOM_INTRO_SOUND
        if (!iv->introComplete)
        {
            iv->introComplete = true;
            break;
        }
#endif
        iv->buttons = evt.state;

        tutorialOnButton(&iv->tut, &evt);
    }

#ifdef CUSTOM_INTRO_SOUND
    if (!iv->introComplete)
    {
        return;
    }
#endif

    int32_t phi, r, intensity;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        iv->joysticks = getTouchJoystickZones(phi, r, true, true);

        tutorialOnTouch(&iv->tut, phi, r, intensity);
    }
    else
    {
        iv->joysticks = 0;

        tutorialOnTouch(&iv->tut, 0, 0, 0);
    }

    // Declare variables to receive acceleration
    int16_t a_x, a_y, a_z;
    // Get the current acceleration
    if (ESP_OK == accelGetOrientVec(&a_x, &a_y, &a_z))
    {
        // Values are roughly -256 to 256
        tutorialOnMotion(&iv->tut, a_x, a_y, a_z);
    }
    else
    {
        tutorialOnMotion(&iv->tut, 0, 0, 0);
    }

    tutorialCheckTriggers(&iv->tut);

    // Fill the display area with a dark cyan

    const char* title  = iv->tut.curStep->title;
    const char* detail = iv->tut.curStep->detail;

    if (iv->tut.tempMessage && (!iv->tut.tempMessageExpiry || iv->tut.tempMessageExpiry > esp_timer_get_time()))
    {
        detail = iv->tut.tempMessage;
    }

    int16_t titleY    = 20;
    iv->bgMenu->title = title;
    drawMenuMania(iv->bgMenu, iv->renderer, elapsedUs);

    int16_t detailYmin = titleY + iv->bigFont.height + 1 + 10;
    int16_t detailYmax = TFT_HEIGHT - 20;
    int16_t detailX    = 10;
    uint16_t detailH   = textWordWrapHeight(&iv->smallFont, detail, TFT_WIDTH - 20, detailYmax - detailYmin);
    int16_t detailY    = detailYmax - detailH;

    int16_t viewX = (TFT_WIDTH - iv->swadgeViewWidth) / 2;
    introDrawSwadge(elapsedUs, viewX, titleY + iv->bigFont.height + 1 + 25, iv->buttons, iv->joysticks);

    const char* remaining
        = drawTextWordWrap(&iv->smallFont, c000, detail, &detailX, &detailY, TFT_WIDTH - 5, detailYmax);
    if (NULL != remaining)
    {
        ESP_LOGI("Intro", "Remaining text: ...%s", remaining);
        // TODO handle remaining text sensibly
    }

    // Draw the spectrum as a bar graph. Figure out bar and margin size
    int16_t binWidth  = (TFT_WIDTH / FIX_BINS);
    int16_t binMargin = (TFT_WIDTH - (binWidth * FIX_BINS)) / 2;

    // Find the max value
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        if (iv->end.fuzzed_bins[i] > iv->maxValue)
        {
            iv->maxValue = iv->end.fuzzed_bins[i];
        }
    }

    // Plot the bars
    int32_t energy = 0;
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        energy += iv->end.fuzzed_bins[i];
        uint8_t height       = ((TFT_HEIGHT / 2) * iv->end.fuzzed_bins[i]) / iv->maxValue;
        paletteColor_t color = c000;
        int16_t x0           = binMargin + (i * binWidth);
        int16_t x1           = binMargin + ((i + 1) * binWidth);
        // Big enough, fill an area
        fillDisplayArea(x0, TFT_HEIGHT - height, x1, TFT_HEIGHT, color);
    }

    tutorialOnSound(&iv->tut, energy);
}

/**
 * @brief TODO
 *
 * @param samples
 * @param sampleCnt
 */
void introAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    // For each sample
    for (uint32_t idx = 0; idx < sampleCnt; idx++)
    {
        // Push to test
        PushSample32(&iv->dd, samples[idx]);

        // If 128 samples have been pushed
        iv->samplesProcessed++;
        if (iv->samplesProcessed >= 128)
        {
            // Update LEDs
            iv->samplesProcessed = 0;
            HandleFrameInfo(&iv->end, &iv->dd);
        }
    }
}

/**
 * @brief TODO
 *
 * @param x
 * @param y
 * @param w
 * @param h
 * @param up
 * @param upNum
 */
static void introBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    accelIntegrate();
}

#ifdef CUSTOM_INTRO_SOUND
static void introDacCallback(uint8_t* samples, int16_t len)
{
    #define SPK_SILENCE (INT8_MIN)
    if (iv->playingSound)
    {
        if (iv->soundProgress + len >= iv->soundSize)
        {
            size_t send = (iv->soundSize - iv->soundProgress);
            memcpy(samples, iv->sound + iv->soundProgress, send);
            memset(samples + send, SPK_SILENCE, len - send);
            iv->playingSound = false;
        }
        else
        {
            memcpy(samples, iv->sound + iv->soundProgress, len);
            iv->soundProgress += len;
        }
    }
    else
    {
        memset(samples, SPK_SILENCE, len);
    }
}
#endif

static void introTutorialCb(const tutorialState_t* state, const tutorialStep_t* prev, const tutorialStep_t* next,
                            bool backtrack)
{
    ESP_LOGI("Intro", "'%s' Triggered!", prev->title);

    // TODO fix this
    if (next == (buttonsSteps + 7))
    {
        ESP_LOGI("Intro", "Oh it's the one we want: %s", next->title);
        iv->quickSettingsOpened = true;
        openQuickSettings();
    }
    else if (next == (buttonsSteps + 8))
    {
        iv->quickSettingsOpened = false;
    }
    else if (next != NULL && next->trigger.type == NO_TRIGGER)
    {
        setTutorialCompletedSetting(true);
        switchToSwadgeMode(&mainMenuMode);
        ESP_LOGI("Intro", "Last trigger entered!");
    }
}

static bool introCheckQuickSettingsTrigger(const tutorialState_t* state, const tutorialTrigger_t* trigger)
{
    return iv->quickSettingsOpened;
}

static void introDrawSwadge(int64_t elapsedUs, int16_t x, int16_t y, buttonBit_t buttons, touchJoystick_t joysticks)
{
#define BLINK_ON   550000
#define BLINK_OFF  200000
#define BLINK_TIME (BLINK_ON + BLINK_OFF)

    static const int buttonBlinkOrder[] = {
        1, // UP
        2, // DOWN
        0, // LEFT
        3, // RIGHT
        7, // A
        6, // B
        5, // START
        4, // SELECT
    };
    static int64_t time = 0;
    time += elapsedUs;
    bool blink = true;

    // Draw the background of the swadge
    int16_t leftCircleX  = x + iv->buttonIcons[2].x + iv->buttonIcons[2].icon->w / 2;
    int16_t rightCircleX = x + iv->buttonIcons[5].x + iv->buttonIcons[5].icon->w / 2;
    int16_t bgCircleY    = y + iv->buttonIcons[2].y + iv->buttonIcons[2].icon->h / 2;
    int16_t bgCircleR    = 30;

    paletteColor_t bgColor     = c234;
    paletteColor_t borderColor = c000;

    drawCircleFilled(leftCircleX, bgCircleY, bgCircleR, bgColor);
    drawCircleFilled(rightCircleX, bgCircleY, bgCircleR, bgColor);
    fillDisplayArea(leftCircleX, bgCircleY - bgCircleR, rightCircleX, bgCircleY + bgCircleR, bgColor);

    drawLineFast(leftCircleX, bgCircleY - bgCircleR, rightCircleX, bgCircleY - bgCircleR, borderColor);
    drawLineFast(leftCircleX, bgCircleY + bgCircleR, rightCircleX, bgCircleY + bgCircleR, borderColor);

    drawCircleQuadrants(leftCircleX, bgCircleY, bgCircleR, false, true, true, false, borderColor);
    drawCircleQuadrants(rightCircleX, bgCircleY, bgCircleR, true, false, false, true, borderColor);

    paletteColor_t notPressedColor = c531;
    paletteColor_t pressedColor    = c243;
    paletteColor_t notNeededColor  = c111;

    for (int stage = 0; stage < 2; stage++)
    {
        for (int i = 0; i < 8; i++)
        {
            buttonBit_t button = (1 << i);
            iconPos_t* icon    = &iv->buttonIcons[i];

            if (icon->visible)
            {
                if (stage == 0)
                {
                    int64_t timeOffset = ((7 - buttonBlinkOrder[i]) * (BLINK_TIME / 6)) / 8;
                    bool showBlink     = ((time + timeOffset) % BLINK_TIME) <= BLINK_ON;
                    int16_t circleX    = x + icon->x + icon->icon->w / 2;
                    int16_t circleY    = y + icon->y + icon->icon->h / 2;
                    int16_t strokeW    = 3;
                    int16_t circleR    = icon->icon->w * 3 / 5 + strokeW;
                    if (iv->tut.curStep && iv->tut.curStep->trigger.type == BUTTON_PRESS_ALL
                        && (iv->tut.curStep->trigger.buttons & button) == button)
                    {
                        if ((iv->tut.allButtons & button) == button)
                        {
                            // Solid Green around already-satisfied button
                            drawCircleFilled(circleX, circleY, circleR, pressedColor);
                        }
                        else if (!blink || showBlink)
                        {
                            // Yellow around not-yet-satisfied button that must be pressed
                            drawCircleOutline(circleX, circleY, circleR, strokeW, notPressedColor);
                        }
                    }
                    else if (iv->tut.curStep && iv->tut.curStep->trigger.type == BUTTON_PRESS_ANY
                             && (iv->tut.curStep->trigger.buttons & button) == button)
                    {
                        // Yellow around not-yet-satisfied 'ANY' button
                        if (!blink || showBlink)
                        {
                            drawCircleOutline(circleX, circleY, circleR, strokeW, notPressedColor);
                        }
                    }
                    else if (iv->tut.curStep && iv->tut.curStep->trigger.type == BUTTON_PRESS
                             && iv->tut.curStep->trigger.buttons == button)
                    {
                        if (!blink || showBlink)
                        {
                            // Yellow around not-yet-satisfied 'PRESS' button
                            drawCircleOutline(circleX, circleY, circleR, strokeW, notPressedColor);
                        }
                    }
                    else if ((buttons & button) == button)
                    {
                        // Gray around currently-pressed button that's not otherwise needed
                        drawCircleFilled(circleX, circleY, circleR, notNeededColor);
                    }
                    else if (iv->tut.curStep == (buttonsSteps + 7) && button == PB_SELECT)
                    {
                        // Special case - draw a circle around the Pause button for exiting quick settings
                        drawCircleOutline(circleX, circleY, circleR, strokeW, notPressedColor);
                    }
                }
                else if (stage == 1)
                {
                    drawWsg(icon->icon, x + icon->x, y + icon->y, icon->flipLR, icon->flipUD, icon->rot);
                }
            }
        }
    }
}

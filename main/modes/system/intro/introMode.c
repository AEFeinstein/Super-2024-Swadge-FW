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
#include "bunny.h"

#define CUSTOM_INTRO_SOUND

static void introEnterMode(void);
static void introExitMode(void);
static void introMainLoop(int64_t elapsedUs);
static void introBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void introAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void introDacCallback(uint8_t* samples, int16_t len);

// static void introMenuCb(const char*, bool selected, uint32_t settingVal);
static void introTutorialCb(tutorialState_t* state, const tutorialStep_t* prev, const tutorialStep_t* next,
                            bool backtrack);
static bool introCheckQuickSettingsTrigger(const tutorialState_t* state, const tutorialTrigger_t* trigger);

static vec_t getTouchScreenCoord(vec_t touchPoint);

static void introDrawSwadgeButtons(int64_t elapsedUs, int16_t x, int16_t y, buttonBit_t buttons);
static void introDrawSwadgeTouchpad(int64_t elapsedUs, vec_t touchPoint, list_t* touchHist);
static void introDrawSwadgeImu(int64_t elapsedUs);
static void introDrawSwadgeSpeaker(int64_t elapsedUs);
static void introDrawSwadgeMicrophone(int64_t elapsedUs, uint16_t* fuzzed_bins, uint16_t maxValue);

#define ALL_BUTTONS  (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT)
#define DPAD_BUTTONS (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT)

// #define ALL_TOUCH (TB_CENTER | TB_RIGHT | TB_UP | TB_LEFT | TB_DOWN)

static const char startTitle[]        = "Welcome!";
static const char holdLongerMessage[] = "Almost! Keep holding MENU for one second to exit.";
static const char endTitle[]          = "Exiting Modes";
static const char endDetail[]         = "You are now Swadge Certified! Remember, with great power comes great "
                                        "responsibility. Hold MENU to exit the tutorial and get started!";

static const char dpadTitle[]     = "The D-Pad";
static const char aBtnTitle[]     = "A Button";
static const char bBtnTitle[]     = "B Button";
static const char mnuBtnTitle[]   = "Menu Button";
static const char pauseBtnTitle[] = "Pause Button";
static const char spkTitle[]      = "Speaker";
static const char micTitle[]      = "Microphone";
static const char touchpadTitle[] = "Touchpad";
static const char imuTitle[]      = "Tilt Controls";

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
        .title = dpadTitle,
        .detail = "These four buttons on the left side of the Swadge are the D-Pad. Use them to navigate. Try them all out!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_A,
        },
        .title = aBtnTitle,
        .detail = "The A Button is used to select, or trigger a primary action. Give it a try!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_B,
        },
        .title = bBtnTitle,
        .detail = "The B Button is used to go back, or trigger a secondary action. Back it up!",
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_START,
        },
        .title = pauseBtnTitle,
        .detail = "The Pause button is mode-specific, but usually pauses a game or performs a special function. Give it a try!",
    },
    {
        .trigger = {
            .type = TOUCH_SPIN,
            .intData = -1,
        },
        .title = touchpadTitle,
        .detail = "The C Gem is a touchpad! Give it a try by spinning your finger around it clockwise."
    },
    {
        .trigger = {
            .type = TOUCH_SPIN,
            .intData = 1,
        },
        .title = touchpadTitle,
        .detail = "OK, now your finger spin around it counter-clockwise. Always remember to unwind the touchpad after use!"
    },
    {
        .trigger = {
            .type = IMU_ORIENT,
            .orientation = {0, 0, 245},
        },
        .title = imuTitle,
        .detail = "The Swadge is tilt-sensitive. Here's a wireframe of a cute bunny! Try looking at it top-down by putting the Swadge face up on a flat surface."
    },
    {
        .trigger = {
            .type = IMU_ORIENT,
            .orientation = {0, 0, -245},
        },
        .title = imuTitle,
        .detail = "Now try looking at the bunny from the bottom. You may need to hold the Swadge over your head."
    },
    {
        .trigger = {
            .type = MIC_LOUD,
        },
        .title = micTitle,
        .detail = "The metal dot to the right of the screen is a microphone. Give a roar (or blow on it) to continue."
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_A,
        },
        .title = spkTitle,
        .detail = "The Swadge has a speaker, so enjoy this tune! The volume dial is on the top left next to the headphone jack. Adjust to your liking. Press A to continue."
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_A,
        },
        .title = spkTitle,
        .detail = "If the speaker sounds a little fuzzy at high volumes, that's OK. It just means it's happy! Press A to continue."
    },
    {
        .trigger = {
            .type = BUTTON_PRESS,
            .buttons = PB_SELECT,
        },
        .title = mnuBtnTitle,
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
        .title = mnuBtnTitle,
        .detail = "Now let go!",
    },
    {
        .trigger = {
            .type = CUSTOM_TRIGGER,
            .custom.checkFn = introCheckQuickSettingsTrigger,
        },
        .title = mnuBtnTitle,
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
#ifdef CUSTOM_INTRO_SOUND
    .fnDacCb                  = introDacCallback,
#else
    .fnDacCb                  = NULL,
#endif
};

#ifdef CUSTOM_INTRO_SOUND
typedef struct
{
    const uint8_t* sample;
    size_t sampleCount;
    
    // Number of times each sample in the file needs to be played to match the audio sample rate
    uint8_t factor;

    // Number of audio samples played
    size_t progress;
} audioSamplePlayer_t;

bool playSample(audioSamplePlayer_t* player, uint8_t* samples, int16_t len);
#endif

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

typedef enum
{
    DRAW_BUTTONS,
    DRAW_TOUCHPAD,
    DRAW_IMU,
    DRAW_SPK,
    DRAW_MIC,
} introDrawMode_t;

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

        // struct
        // {
        //     wsg_t base;
        //     wsg_t joystick;
        //     wsg_t spin;
        // } touch;

        wsg_t speaker;
        wsg_t touchGem;
        wsg_t swadge;
    } icon;

    buttonBit_t buttons;
    vec_t touch;
    list_t touchHist;
    int32_t angle;
    int32_t angleTimer;

    iconPos_t buttonIcons[8];
    int16_t swadgeViewWidth;
    int16_t swadgeViewHeight;

    bool quickSettingsOpened;

#ifdef CUSTOM_INTRO_SOUND
    bool playingSound;
    bool introComplete;
    audioSamplePlayer_t samplePlayer;
#endif

    menu_t* bgMenu;
    menuManiaRenderer_t* renderer;
    introDrawMode_t drawMode;

    // Microphone test
    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    uint8_t samplesProcessed;
    uint16_t maxValue;

    // Speaker test
    midiFile_t song;
} introVars_t;

static introVars_t* iv;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void introEnterMode(void)
{
    iv = heap_caps_calloc(1, sizeof(introVars_t), MALLOC_CAP_8BIT);

    loadFont("ibm_vga8.font", &iv->smallFont, true);
    loadFont("righteous_150.font", &iv->bigFont, true);
    loadFont("retro_logo.font", &iv->logoFont, true);
    makeOutlineFont(&iv->logoFont, &iv->logoFontOutline, true);

#ifdef CUSTOM_INTRO_SOUND
    iv->samplePlayer.sample = cnfsGetFile("magfest.bin", &iv->samplePlayer.sampleCount);
    iv->samplePlayer.factor = 1;
#endif

    loadWsg("button_a.wsg", &iv->icon.button.a, true);
    loadWsg("button_b.wsg", &iv->icon.button.b, true);
    loadWsg("button_menu.wsg", &iv->icon.button.menu, true);
    loadWsg("button_pause.wsg", &iv->icon.button.pause, true);
    loadWsg("button_up.wsg", &iv->icon.button.up, true);

    loadWsg("spk.wsg", &iv->icon.speaker, true);
    loadWsg("touch-gem.wsg", &iv->icon.touchGem, true);
    loadWsg("intro_swadge.wsg", &iv->icon.swadge, true);

    iv->bgMenu   = initMenu(startTitle, NULL);
    iv->renderer = initMenuManiaRenderer(&iv->bigFont, NULL, &iv->smallFont);

    // up
    iv->buttonIcons[0].icon    = &iv->icon.button.up;
    iv->buttonIcons[0].visible = true;
    iv->buttonIcons[0].x       = 31;
    iv->buttonIcons[0].y       = 41;

    // down
    iv->buttonIcons[1].icon    = &iv->icon.button.up;
    iv->buttonIcons[1].flipUD  = true;
    iv->buttonIcons[1].visible = true;
    iv->buttonIcons[1].x       = 31;
    iv->buttonIcons[1].y       = 61;

    // left
    iv->buttonIcons[2].icon    = &iv->icon.button.up;
    iv->buttonIcons[2].rot     = 270;
    iv->buttonIcons[2].visible = true;
    iv->buttonIcons[2].x       = 21;
    iv->buttonIcons[2].y       = 51;

    // right
    iv->buttonIcons[3].icon    = &iv->icon.button.up;
    iv->buttonIcons[3].rot     = 90;
    iv->buttonIcons[3].visible = true;
    iv->buttonIcons[3].x       = 41;
    iv->buttonIcons[3].y       = 51;

    // a
    iv->buttonIcons[4].icon    = &iv->icon.button.a;
    iv->buttonIcons[4].visible = true;
    iv->buttonIcons[4].x       = 174;
    iv->buttonIcons[4].y       = 34;

    // b
    iv->buttonIcons[5].icon    = &iv->icon.button.b;
    iv->buttonIcons[5].visible = true;
    iv->buttonIcons[5].x       = 156;
    iv->buttonIcons[5].y       = 42;

    // start (pause)
    iv->buttonIcons[6].icon    = &iv->icon.button.pause;
    iv->buttonIcons[6].visible = true;
    iv->buttonIcons[6].x       = 60;
    iv->buttonIcons[6].y       = 56;

    // select (menu)
    iv->buttonIcons[7].icon    = &iv->icon.button.menu;
    iv->buttonIcons[7].visible = true;
    iv->buttonIcons[7].x       = 62;
    iv->buttonIcons[7].y       = 69;

    iv->swadgeViewWidth  = iv->icon.swadge.w * 2;
    iv->swadgeViewHeight = iv->icon.swadge.w * 2;

    tutorialSetup(&iv->tut, introTutorialCb, buttonsSteps, ARRAY_SIZE(buttonsSteps), iv);

#ifdef CUSTOM_INTRO_SOUND
    iv->playingSound  = true;
    iv->introComplete = false;
    switchToSpeaker();
#endif

    // Load the MIDI file
    loadMidiFile("hd_credits.mid", &iv->song, true);

    // Init CC
    InitColorChord(&iv->end, &iv->dd);
    iv->maxValue = 1;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void introExitMode(void)
{
    clear(&iv->touchHist);

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
    freeWsg(&iv->icon.speaker);
    freeWsg(&iv->icon.touchGem);
    freeWsg(&iv->icon.swadge);

    unloadMidiFile(&iv->song);

    heap_caps_free(iv);
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
        //drawMenuMania(iv->bgMenu, iv->renderer, elapsedUs);
        for (int i = 0; i < TFT_HEIGHT; i++)
        {
            drawLineFast(0, i, TFT_WIDTH, i, (i % 2) ? c555 : c444);
        }

        static int32_t timer = 0;

        const char mag[] = "MA";
        const char fest[] = "GFest";

        const char sub[]   = "Sw";
        const char sub2[]  = "adge";

        int16_t magW = textWidth(&iv->logoFont, mag);
        int16_t festW = textWidth(&iv->logoFont, fest);
        int16_t kernAG = -3;

        int16_t titleWidth = magW + 1 + festW + kernAG;
        int16_t titleX     = (TFT_WIDTH - titleWidth) / 2;
        int16_t titleY     = (TFT_HEIGHT - iv->bigFont.height - iv->smallFont.height - 6) / 2;

        int16_t magX = titleX;
        int16_t festX = magX + magW + 1 + kernAG;

        int16_t subOneWidth = textWidth(&iv->logoFont, sub);
        int16_t subTwoWidth = textWidth(&iv->logoFont, sub2);
        int16_t kernWA = -8;
        int16_t subWidth = textWidth(&iv->logoFont, sub) + textWidth(&iv->logoFont, sub2) + kernWA + 1;
        int16_t subX     = (TFT_WIDTH - subWidth) / 2;
        int16_t subY     = titleY + iv->bigFont.height + 5;

        paletteColor_t outlineCol = c005;

        paletteColor_t colors[] = {c003, c035, c555, c001, c003, c035, c555, c001};
        int magColOffset = ((timer % 400000) / 100000);
        int festColOffset = (magColOffset + (3 - magW % 4)) % 4;
        drawTextMulticolored(&iv->logoFont, mag, magX, titleY, colors + magColOffset, 4, magW);
        drawTextMulticolored(&iv->logoFont, fest, festX, titleY, colors + festColOffset, 4, festW);
        drawText(&iv->logoFontOutline, outlineCol, mag, magX, titleY);
        drawText(&iv->logoFontOutline, outlineCol, fest, festX, titleY);

        int swColOffset = 3 - ((timer % 600000) / 150000);
        int adgeColOffset = (swColOffset + (3 - (subOneWidth + kernWA) % 4)) % 4;
        drawTextMulticolored(&iv->logoFont, sub, subX, subY, colors + swColOffset, 4, subOneWidth);
        drawTextMulticolored(&iv->logoFont, sub2, subX + subOneWidth + kernWA, subY, colors + adgeColOffset, 4, subTwoWidth);
        drawText(&iv->logoFontOutline, outlineCol, sub, subX, subY);
        drawText(&iv->logoFontOutline, outlineCol, sub2, subX + subOneWidth + kernWA, subY);

        timer += elapsedUs;

        // Return early -- Don't do the rest of the stuff yet
        if (iv->playingSound)
        {
            return;
        }
        else
        {
            iv->introComplete = true;
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
        getTouchCartesian(phi, r, &iv->touch.x, &iv->touch.y);
        tutorialOnTouch(&iv->tut, phi, r, intensity);

        // Save this point to draw a trail
        vec_t screenTouch = getTouchScreenCoord(iv->touch);
        intptr_t tPoint   = ((screenTouch.x & 0xFFFF) << 16) | (screenTouch.y & 0xFFFF);
        push(&iv->touchHist, (void*)tPoint);
    }
    else
    {
        iv->touch.x = 0;
        iv->touch.y = 0;
        tutorialOnTouch(&iv->tut, 0, 0, 0);
        clear(&iv->touchHist);
    }

    // Get the current acceleration
    int16_t a_x;
    int16_t a_y;
    int16_t a_z;
    if (ESP_OK != accelGetOrientVec(&a_x, &a_y, &a_z))
    {
        a_x = 0;
        a_y = 0;
        a_z = 0;
    }
    // Values are roughly -256 to 256
    tutorialOnMotion(&iv->tut, a_x, a_y, a_z);

    // Find the overall sound energy
    int32_t energy = 0;
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        // Find the max value
        if (iv->end.fuzzed_bins[i] > iv->maxValue)
        {
            iv->maxValue = iv->end.fuzzed_bins[i];
        }
        energy += iv->end.fuzzed_bins[i];
    }
    tutorialOnSound(&iv->tut, energy);

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

    switch (iv->drawMode)
    {
        default:
        case DRAW_BUTTONS:
        {
            int16_t viewX = (TFT_WIDTH - iv->swadgeViewWidth) / 2;
            int16_t viewY = titleY + iv->logoFont.height + 10;
            introDrawSwadgeButtons(elapsedUs, viewX, viewY, iv->buttons);
            break;
        }
        case DRAW_TOUCHPAD:
        {
            introDrawSwadgeTouchpad(elapsedUs, iv->touch, &iv->touchHist);
            break;
        }
        case DRAW_IMU:
        {
            introDrawSwadgeImu(elapsedUs);
            break;
        }
        case DRAW_SPK:
        {
            introDrawSwadgeSpeaker(elapsedUs);
            break;
        }
        case DRAW_MIC:
        {
            introDrawSwadgeMicrophone(elapsedUs, iv->end.fuzzed_bins, iv->maxValue);
            break;
        }
    }

    const char* remaining
        = drawTextWordWrap(&iv->smallFont, c000, detail, &detailX, &detailY, TFT_WIDTH - 5, detailYmax);
    if (NULL != remaining)
    {
        ESP_LOGI("Intro", "Remaining text: ...%s", remaining);
        // TODO handle remaining text sensibly
    }
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
bool playSample(audioSamplePlayer_t* player, uint8_t* samples, int16_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (player->progress >= player->sampleCount * player->factor)
        {
            // Set all to silence
            memset(&samples[i], INT8_MIN, len - i);
            return false;
        }
        else
        {
            samples[i] = player->sample[player->progress / player->factor];
            player->progress++;
        }
    }
    return player->progress < player->sampleCount * player->factor;
}

static void introDacCallback(uint8_t* samples, int16_t len)
{
    if (iv->playingSound && iv->samplePlayer.sample)
    {
        iv->playingSound = playSample(&iv->samplePlayer, samples, len);
        if (!iv->playingSound)
        {
            ESP_LOGI("Intro", "Finished playing sample");
        }
    }
    else
    {
        globalMidiPlayerFillBuffer(samples, len);
    }
}
#endif

static void introTutorialCb(tutorialState_t* state, const tutorialStep_t* prev, const tutorialStep_t* next,
                            bool backtrack)
{
    ESP_LOGI("Intro", "'%s' Triggered!", prev->title);
    ESP_LOGI("Intro", "Onto '%s'", next->title);

    // Switch peripherals
    if (spkTitle == next->title)
    {
        if (DRAW_SPK != iv->drawMode)
        {
            switchToSpeaker();

            // Set and play the song
            midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
            midiGmOn(player);
            midiSetFile(player, &iv->song);
            player->loop = true;
            midiPause(player, false);

            iv->drawMode = DRAW_SPK;
        }
        else
        {
            state->lastButton     = 0;
            state->lastButtonDown = 0;
            state->curButtons     = 0;
            state->allButtons     = 0;
        }
    }
    else if (micTitle == next->title)
    {
        switchToMicrophone();
        iv->drawMode = DRAW_MIC;
    }
    else if (touchpadTitle == next->title)
    {
        globalMidiPlayerPauseAll();
        iv->drawMode = DRAW_TOUCHPAD;
        // Reset touch history and spin state
        clear(&iv->touchHist);
        memset(&iv->tut.spinState, 0, sizeof(touchSpinState_t));
    }
    else if (imuTitle == next->title)
    {
        globalMidiPlayerPauseAll();
        iv->drawMode = DRAW_IMU;
    }
    else
    {
        globalMidiPlayerPauseAll();
        iv->drawMode = DRAW_BUTTONS;
    }

    // TODO maybe don't hardcode this
    if (next == (buttonsSteps + 14))
    {
        ESP_LOGI("Intro", "Oh it's the one we want: %s", next->title);
        iv->quickSettingsOpened = true;
        openQuickSettings();
    }
    else if (next == (buttonsSteps + 15))
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

static void introDrawSwadgeButtons(int64_t elapsedUs, int16_t x, int16_t y, buttonBit_t buttons)
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

    paletteColor_t notPressedColor = c531;
    paletteColor_t pressedColor    = c243;
    paletteColor_t notNeededColor  = c111;

    // Draw the background of the swadge
    drawWsgSimpleScaled(&iv->icon.swadge, x, y, 2, 2);

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

#define TOUCHPAD_RADIUS 50
#define TOUCHPAD_X      (TFT_WIDTH / 2)
#define TOUCHPAD_Y      (TFT_HEIGHT / 2)

/**
 * @brief TODO
 *
 * @param touchPoint
 * @return vec_t
 */
static vec_t getTouchScreenCoord(vec_t touchPoint)
{
    // Draw the dot (0 to 1024)
    vec_t cartesianStart = {
        .x = TOUCHPAD_X - TOUCHPAD_RADIUS,
        .y = TOUCHPAD_Y - TOUCHPAD_RADIUS,
    };

    vec_t drawPoint = addVec2d(cartesianStart, divVec2d(mulVec2d(touchPoint, TOUCHPAD_RADIUS * 2), 1024));
    drawPoint.y     = TOUCHPAD_Y + (TOUCHPAD_Y - drawPoint.y);
    return drawPoint;
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 * @param touchPoint
 * @param touchHist
 */
static void introDrawSwadgeTouchpad(int64_t elapsedUs, vec_t touchPoint, list_t* touchHist)
{
    // Draw the pad
    drawWsgSimple(&iv->icon.touchGem, (TFT_WIDTH - iv->icon.touchGem.w) / 2, (TFT_HEIGHT - iv->icon.touchGem.h) / 2);

    // Animate how the user should spin
    iv->angleTimer += elapsedUs;
    while (iv->angleTimer > (1000000 / 180))
    {
        iv->angleTimer -= (1000000 / 180);
        iv->angle++;
        if (360 == iv->angle)
        {
            iv->angle = 0;
        }
    }

    // Draw a guide
    for (int32_t a = 0; a < iv->angle; a++)
    {
        int32_t x = TOUCHPAD_X;
        if (iv->tut.curStep->trigger.intData < 0)
        {
            x += (TOUCHPAD_RADIUS * 3 * getSin1024(a)) / (1024 * 4);
        }
        else
        {
            x -= (TOUCHPAD_RADIUS * 3 * getSin1024(a)) / (1024 * 4);
        }
        int32_t y = TOUCHPAD_Y - (TOUCHPAD_RADIUS * 3 * getCos1024(a)) / (1024 * 4);
        drawCircleFilled(x, y, 4, c115);
    }

    // Draw the touch point
    if (0 != touchPoint.x || 0 != touchPoint.y)
    {
        vec_t drawPoint = getTouchScreenCoord(touchPoint);
        drawCircleFilled(drawPoint.x, drawPoint.y, 8, c511);
    }

    // Draw the touch tail
    node_t* touchNode = touchHist->first;
    while (touchNode && touchNode->next)
    {
        vec_t startPoint = {
            .x = ((intptr_t)touchNode->val >> 16) & 0xFFFF,
            .y = ((intptr_t)touchNode->val) & 0xFFFF,
        };

        vec_t endPoint = {
            .x = ((intptr_t)touchNode->next->val >> 16) & 0xFFFF,
            .y = ((intptr_t)touchNode->next->val) & 0xFFFF,
        };

        drawLineFast(startPoint.x, startPoint.y, endPoint.x, endPoint.y, c511);

        touchNode = touchNode->next;
    }
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
static void introDrawSwadgeImu(int64_t elapsedUs)
{
    // static void accelDrawBunny(void)
    // Produce a model matrix from a quaternion.
    float plusx_out[3] = {1, 0, 0};
    float plusy_out[3] = {0, 1, 0};
    float plusz_out[3] = {0, 0, 1};

    mathRotateVectorByQuaternion(plusy_out, LSM6DSL.fqQuat, plusy_out);
    mathRotateVectorByQuaternion(plusx_out, LSM6DSL.fqQuat, plusx_out);
    mathRotateVectorByQuaternion(plusz_out, LSM6DSL.fqQuat, plusz_out);

    int16_t bunny_verts_out[numBunnyVerts() / 3 * 3];
    memset(bunny_verts_out, 0, sizeof(bunny_verts_out));
    int i, vertices = 0;
    for (i = 0; i < numBunnyVerts(); i += 3)
    {
        // Performingthe transform this way is about 700us.
        float bx                          = bunny_verts[i + 2];
        float by                          = bunny_verts[i + 1];
        float bz                          = -bunny_verts[i + 0];
        float bunnyvert[3]                = {bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2],
                                             bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2],
                                             bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2]};
        bunny_verts_out[vertices * 3 + 0] = bunnyvert[0] / 250 + 280 / 2;
        // Convert from right-handed to left-handed coordinate frame.
        bunny_verts_out[vertices * 3 + 1] = -bunnyvert[1] / 250 + 240 / 2;
        bunny_verts_out[vertices * 3 + 2] = bunnyvert[2];
        vertices++;
    }

    int lines = 0;
    for (i = 0; i < numBunnyLines(); i += 2)
    {
        int v1 = bunny_lines[i] * 3;
        int v2 = bunny_lines[i + 1] * 3;
        drawLineFast(bunny_verts_out[v1], bunny_verts_out[v1 + 1], bunny_verts_out[v2], bunny_verts_out[v2 + 1], c511);
        lines++;
    }
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
static void introDrawSwadgeSpeaker(int64_t elapsedUs)
{
    // Draw speaker icon
    drawWsgSimple(&iv->icon.speaker, (TFT_WIDTH - iv->icon.speaker.w) / 2, MANIA_TITLE_HEIGHT + 8);
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 * @param fuzzed_bins
 * @param maxValue
 */
static void introDrawSwadgeMicrophone(int64_t elapsedUs, uint16_t* fuzzed_bins, uint16_t maxValue)
{
    // Draw the spectrum as a bar graph. Figure out bar and margin size
    int16_t binWidth  = (TFT_WIDTH / FIX_BINS);
    int16_t binMargin = (TFT_WIDTH - (binWidth * FIX_BINS)) / 2;

    int16_t barsTop    = MANIA_TITLE_HEIGHT;
    int16_t barsBottom = 176;
    int16_t barsCenter = (barsTop + barsBottom) / 2;
    int16_t barsHeight = barsBottom - barsTop;

    // Plot the bars
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        uint8_t height       = (barsHeight * fuzzed_bins[i]) / (2 * maxValue);
        height               = MAX(height, 1);
        paletteColor_t color = c511;
        int16_t x0           = binMargin + (i * binWidth);
        int16_t x1           = binMargin + ((i + 1) * binWidth);
        // Big enough, fill an area
        fillDisplayArea(x0, barsCenter - height, x1, barsCenter + height, color);
    }
}

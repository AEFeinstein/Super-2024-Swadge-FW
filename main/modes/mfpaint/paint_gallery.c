#include "paint_gallery.h"

#include "settingsManager.h"

#include "mode_paint.h"
#include "paint_common.h"
#include "paint_browser.h"
#include "paint_nvs.h"
#include "paint_util.h"
#include "touchUtils.h"

#include <string.h>

static const char transitionTime[]       = "Slideshow: %g sec";
static const char transitionOff[]        = "Slideshow: Off";
static const char transitionTimeNvsKey[] = "paint_gal_time";
static const char viewStyleNvsKey[]      = "paint_gal_view";
static const char galleryNvsSlotKey[]    = "paint_gal_slot";
// static const char danceIndexKey[] = "paint_dance_idx";

// Possible transition times, in ms
static const uint16_t transitionTimeMap[] = {
    0, // off
    500, 1000, 2000, 5000, 10000, 15000, 30000, 45000, 60000,
};

// Denotes 5s
#define DEFAULT_TRANSITION_INDEX 4

#define US_PER_SEC 1000000
#define US_PER_MS  1000

// 3s for info text to stay up
#define GALLERY_INFO_TIME     3000000
#define GALLERY_INFO_Y_MARGIN 18
#define GALLERY_INFO_X_MARGIN 24
#define GALLERY_ARROW_MARGIN  6

paintGallery_t* paintGallery;

static int16_t arrowCharToRot(char dir);
static wsg_t* iconCharToWsg(char icon);
static void paintGalleryBrowserCb(const char* nvsKey, imageBrowserAction_t action);

void paintGallerySetup(bool screensaver)
{
    paintGallery                  = calloc(sizeof(paintGallery_t), 1);
    paintGallery->galleryTime     = 0;
    paintGallery->screensaverMode = screensaver;
    // Show the UI at the start if we're a screensaver
    paintGallery->showUi = !screensaver;
    paintGallery->infoTimeRemaining = paintGallery->showUi ? GALLERY_INFO_TIME * 2 : 0;
    paintGallery->infoView = GALLERY_INFO_CONTROLS;
    loadFont("ibm_vga8.font", &paintGallery->infoFont, false);
    loadWsg("button_up.wsg", &paintGallery->arrow, false);
    loadWsg("button_a.wsg", &paintGallery->aWsg, false);
    loadWsg("button_b.wsg", &paintGallery->bWsg, false);
    loadWsg("button_pause.wsg", &paintGallery->pauseWsg, false);
    loadWsg("touchpad_spin.wsg", &paintGallery->spinWsg, false);

    if (!readNvs32(transitionTimeNvsKey, &paintGallery->gallerySpeedIndex))
    {
        // Default of 5s if not yet set
        paintGallery->gallerySpeedIndex = DEFAULT_TRANSITION_INDEX;
    }

    if (!readNvs32(viewStyleNvsKey, &paintGallery->browser.viewStyle))
    {
        paintGallery->browser.viewStyle = BROWSER_GALLERY;
    }

    if (!readNvsBlob(galleryNvsSlotKey, paintGallery->gallerySlotKey, NULL))
    {
        // If this returns false, the slot is already zero'd so don't worry about it
        paintGetLastSlot(paintGallery->gallerySlotKey);
    }

    if (paintGallery->gallerySpeedIndex < 0
        || paintGallery->gallerySpeedIndex >= sizeof(transitionTimeMap) / sizeof(*transitionTimeMap))
    {
        paintGallery->gallerySpeedIndex = DEFAULT_TRANSITION_INDEX;
    }

    paintGallery->gallerySpeed = US_PER_MS * transitionTimeMap[paintGallery->gallerySpeedIndex];

    paintGallery->browser.callback = paintGalleryBrowserCb;
    paintGallery->browser.wraparound = true;
    paintGallery->browser.cols = 4;
    setupImageBrowser(&paintGallery->browser, &paintGallery->infoFont, PAINT_NS_DATA, NULL, BROWSER_OPEN,
                      BROWSER_DELETE);

    /* TODO: Add this back when LED dances are solved
    paintGallery->portableDances = initPortableDance(danceIndexKey);
    portableDanceDisableDance(paintGallery->portableDances, "Flashlight");

    if (!(paintGallery->index & PAINT_ENABLE_LEDS))
    {
        portableDanceSetByName(paintGallery->portableDances, "None");
    }
    */

    // clear LEDs, which might still be set by menu
    led_t leds[CONFIG_NUM_LEDS];
    memset(leds, 0, sizeof(led_t) * CONFIG_NUM_LEDS);
    setLeds(leds, CONFIG_NUM_LEDS);
}

void paintGalleryCleanup(void)
{
    freeFont(&paintGallery->infoFont);
    freeWsg(&paintGallery->arrow);
    freeWsg(&paintGallery->aWsg);
    freeWsg(&paintGallery->bWsg);
    freeWsg(&paintGallery->pauseWsg);
    freeWsg(&paintGallery->spinWsg);
    // freePortableDance(paintGallery->portableDances);
    resetImageBrowser(&paintGallery->browser);
    free(paintGallery);
}

void paintGalleryMainLoop(int64_t elapsedUs)
{
    paintGalleryModePollTouch();
    // portableDanceMainLoop(paintGallery->portableDances, elapsedUs);

    if (paintGallery->infoTimeRemaining > 0)
    {
        paintGallery->infoTimeRemaining -= elapsedUs;

        if (paintGallery->infoTimeRemaining <= 0)
        {
            paintGallery->infoTimeRemaining = 0;
            paintGallery->showUi            = false;
        }
    }
    else
    {
        if (paintGallery->gallerySpeed != 0 && paintGallery->galleryTime >= paintGallery->gallerySpeed)
        {
            buttonEvt_t fakeEvent = {
                .button = PB_RIGHT, .down = true, .state = PB_RIGHT
            };
            imageBrowserButton(&paintGallery->browser, &fakeEvent);
            paintGallery->galleryTime %= paintGallery->gallerySpeed;

            // reset info time if we're going to transition and clear the screen
            //paintGallery->infoTimeRemaining = 0;
        }

        paintGallery->galleryTime += elapsedUs;
        paintGallery->showUi = false;
        paintGallery->infoTimeRemaining = 0;
        paintGallery->infoView = 0;
    }

    drawImageBrowser(&paintGallery->browser);

    if (paintGallery->showUi)
    {
        paintGalleryDrawUi();
    }
}

static int16_t arrowCharToRot(char dir)
{
    switch (dir)
    {
        case 'L':
        case 'l':
            return 270;

        case 'R':
        case 'r':
            return 90;

        case 'D':
        case 'd':
            return 180;

        case 'U':
        case 'u':
        default:
            return 0;
    }
}

static wsg_t* iconCharToWsg(char icon)
{
    switch (icon)
    {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
        case 'U':
        case 'u':
        case 'D':
        case 'd':
            return &paintGallery->arrow;

        case 'P':
        case 'p':
            return &paintGallery->pauseWsg;

        case 'T':
        case 't':
            return &paintGallery->spinWsg;

        case 'A':
        case 'a':
            return &paintGallery->aWsg;

        case 'B':
        case 'b':
            return &paintGallery->bWsg;

        default:
        return NULL;
    }
}

void paintGalleryDrawUi(void)
{
    char text[32];
    int row = 0;
    bool center = false;

    if ((paintGallery->infoView & GALLERY_INFO_CONTROLS) == GALLERY_INFO_CONTROLS)
    {
        paintGalleryAddInfoText("Show Controls", row++, center, 'A', 0);
    }
    if (paintGallery->infoView & GALLERY_INFO_EXIT)
    {
        paintGalleryAddInfoText("Exit", row++, center, 'B', 0);
    }
    if (paintGallery->infoView & GALLERY_INFO_VIEW)
    {
        paintGalleryAddInfoText("Fullscreen", row++, center, 'P', 0);
    }
    if (paintGallery->infoView & GALLERY_INFO_NEXT)
    {
        paintGalleryAddInfoText("Navigate", row++, center, 'L', 'R');
    }
    if (paintGallery->infoView & GALLERY_INFO_SPEED)
    {
        // Draw speed 2 rows from the bottom
        if (paintGallery->gallerySpeed == 0)
        {
            paintGalleryAddInfoText(transitionOff, row++, center, 'U', 0);
        }
        else
        {
            snprintf(text, sizeof(text), transitionTime, (1.0 * paintGallery->gallerySpeed / US_PER_SEC));
            paintGalleryAddInfoText(
                text, row++, center,
                (paintGallery->gallerySpeedIndex + 1 < sizeof(transitionTimeMap) / sizeof(*transitionTimeMap)) ? 'U' : 0,
                'D');
        }
    }

    if (paintGallery->infoView & GALLERY_INFO_BRIGHTNESS)
    {
        snprintf(text, sizeof(text), "LED Brightness: %d", getLedBrightnessSetting());
        paintGalleryAddInfoText(text, row++, center, 'T', 0);
    }

    if (paintGallery->infoView & GALLERY_INFO_DANCE)
    {
        //snprintf(text, sizeof(text), "LEDs: %s", portableDanceGetName(paintGallery->portableDances));
        //paintGalleryAddInfoText(text, row++, center, 'L', 'R');
    }
}

void paintGalleryAddInfoText(const char* text, int8_t row, bool center, char leftIcon, char rightIcon)
{
    wsg_t* lWsg = iconCharToWsg(leftIcon);
    wsg_t* rWsg = iconCharToWsg(rightIcon);
    uint16_t padding = 5;
    uint16_t width   = textWidth(&paintGallery->infoFont, text) + (lWsg ? padding + lWsg->w : 0) + (rWsg ? padding + rWsg->w : 0);
    int16_t yOffset;
    int16_t xOffset = center ? ((TFT_WIDTH - width) / 2) : GALLERY_INFO_X_MARGIN;// - (lWsg ? padding + lWsg->w : 0);

    if (row < 0)
    {
        yOffset = TFT_WIDTH + ((row) * (paintGallery->infoFont.height + padding * 2)) - GALLERY_INFO_Y_MARGIN;
    }
    else
    {
        yOffset = GALLERY_INFO_Y_MARGIN + row * (paintGallery->infoFont.height + padding * 2);
    }

    fillDisplayArea(0, yOffset, TFT_WIDTH, yOffset + padding * 2 + paintGallery->infoFont.height, c555);

    if (leftIcon != 0 && lWsg != NULL)
    {
        // assumes arrows are always square, flip between W and H if that's ever the case
        drawWsg(lWsg, xOffset,
                yOffset + padding + (paintGallery->infoFont.height - lWsg->h) / 2, false, false,
                arrowCharToRot(leftIcon));
    }

    if (rightIcon != 0 && rWsg != NULL)
    {
        // assumes arrows are always square, flip between W and H if that's ever the case
        drawWsg(rWsg, xOffset + (lWsg ? padding + lWsg->w : 0),
                yOffset + padding + (paintGallery->infoFont.height - rWsg->h) / 2, false, false,
                arrowCharToRot(rightIcon));
    }

    drawText(&paintGallery->infoFont, c000, text, xOffset + (lWsg ? padding + lWsg->w : 0) + (rWsg ? padding + rWsg->w : 0), yOffset + padding);
}

void paintGalleryDecreaseSpeed(void)
{
    if (paintGallery->gallerySpeedIndex + 1 < sizeof(transitionTimeMap) / sizeof(*transitionTimeMap))
    {
        paintGallery->gallerySpeedIndex++;
        paintGallery->gallerySpeed = US_PER_MS * transitionTimeMap[paintGallery->gallerySpeedIndex];
        writeNvs32(transitionTimeNvsKey, paintGallery->gallerySpeedIndex);

        paintGallery->galleryTime = 0;
    }
}

void paintGalleryIncreaseSpeed(void)
{
    if (paintGallery->gallerySpeedIndex > 0)
    {
        paintGallery->gallerySpeedIndex--;
        paintGallery->gallerySpeed = US_PER_MS * transitionTimeMap[paintGallery->gallerySpeedIndex];
        writeNvs32(transitionTimeNvsKey, paintGallery->gallerySpeedIndex);

        paintGallery->galleryTime = 0;
    }
}

void paintGalleryModeButtonCb(buttonEvt_t* evt)
{
    if (evt->down)
    {
        if (paintGallery->screensaverMode)
        {
            // Return to main menu immediately
            paintReturnToMainMenu();
            return;
        }

        switch (evt->button)
        {
            case PB_UP:
            {
                paintGallery->infoView |= GALLERY_INFO_SPEED;
                paintGalleryDecreaseSpeed();
                break;
            }

            case PB_DOWN:
            {
                paintGallery->infoView |= GALLERY_INFO_SPEED;
                paintGalleryIncreaseSpeed();
                break;
            }

            case PB_LEFT:
            case PB_RIGHT:
            {
                imageBrowserButton(&paintGallery->browser, evt);
                // Return so the normal info view logic doesn't run
                return;
            }

            case PB_SELECT:
            case PB_B:
            {
                // Exit
                paintReturnToMainMenu();
                return;
            }

            case PB_A:
            {
                // start the timer to clear the screen
                if (paintGallery->infoView && paintGallery->showUi)
                {
                    paintGallery->showUi = false;
                }
                else
                {
                    paintGallery->infoTimeRemaining = GALLERY_INFO_TIME * 2;
                    paintGallery->infoView = GALLERY_INFO_CONTROLS;
                    paintGallery->showUi = true;
                }
                // Return so the normal info view logic doesn't run
                return;
            }

            case PB_START:
            {
                paintGallery->infoView |= GALLERY_INFO_VIEW;
                // Increase size
                if (paintGallery->browser.viewStyle == BROWSER_FULLSCREEN)
                {
                    paintGallery->browser.viewStyle = BROWSER_GALLERY;
                }
                else
                {
                    paintGallery->browser.viewStyle = BROWSER_FULLSCREEN;
                }
                paintGallery->galleryScale++;
                break;
            }
        }

        paintGallery->infoTimeRemaining = GALLERY_INFO_TIME;
        paintGallery->showUi = true;
    }
}

void paintGalleryModePollTouch(void)
{
    int32_t centroid, intensity;

    int32_t phi, r;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        if (!paintGallery->spinState.startSet)
        {
            paintGallery->startBrightness = getLedBrightnessSetting();
        }

        getTouchSpins(&paintGallery->spinState, phi, r);

        int32_t diff
            = CLAMP((((paintGallery->spinState.spins * 360 + paintGallery->spinState.remainder) * (7)) / -360),
                    -7, 7);

        setLedBrightnessSetting(CLAMP(paintGallery->startBrightness + diff, 0, 7));

        paintGallery->infoTimeRemaining = GALLERY_INFO_TIME;
        paintGallery->showUi = true;
        paintGallery->infoView |= GALLERY_INFO_BRIGHTNESS;
    }
    else
    {
        paintGallery->spinState.startSet = false;
    }
}

static void paintGalleryBrowserCb(const char* nvsKey, imageBrowserAction_t action)
{
    PAINT_LOGI("Key %s!", nvsKey);
}
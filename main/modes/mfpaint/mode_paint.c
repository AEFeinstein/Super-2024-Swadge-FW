#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "swadge2024.h"
#include "hdw-btn.h"
#include "menu.h"
#include "menuLogbookRenderer.h"
#include "mainMenu.h"
#include "swadge2024.h"
#include "settingsManager.h"
#include "shapes.h"
#include "math.h"

#include "settingsManager.h"

#include "mode_paint.h"
#include "paint_common.h"
#include "paint_util.h"
#include "paint_draw.h"
#include "paint_gallery.h"
#include "paint_share.h"
#include "paint_nvs.h"

const char paintTitle[]      = "MFPaint";
const char menuOptDraw[]     = "Draw";
const char menuOptHelp[]     = "Tutorial";
const char menuOptGallery[]  = "Gallery";
const char menuOptNetwork[]  = "Sharing";
const char menuOptShare[]    = "Share";
const char menuOptReceive[]  = "Receive";
const char menuOptSettings[] = "Settings";

const char menuOptLeds[]  = "LEDs";
const char menuOptBlink[] = "Blink Picks";

const char menuOptLedsOn[]       = "LEDs: On";
const char menuOptLedsOff[]      = "LEDs: Off";
const char menuOptBlinkOn[]      = "Blink Picks: On";
const char menuOptBlinkOff[]     = "Blink Picks: Off";
const char menuOptEraseData[]    = "Erase: All";
char menuOptEraseSlot[]          = "Erase: Slot 1";
const char menuOptCancelErase[]  = "Confirm: No!";
const char menuOptConfirmErase[] = "Confirm: Yes";

const char menuOptExit[] = "Exit";
const char menuOptBack[] = "Back";

// Mode struct function declarations
void paintEnterMode(void);
void paintExitMode(void);
void paintMainLoop(int64_t elapsedUs);
void paintButtonCb(buttonEvt_t* evt);
void paintEventCb(const swadgeEvt_t* evt);

swadgeMode_t modePaint = {
    .modeName                 = paintTitle,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = paintEnterMode,
    .fnExitMode               = paintExitMode,
    .fnMainLoop               = paintMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

// Menu callback declaration
void paintMenuCb(const char* label, bool selected, uint32_t value);

// Util function declarations

void paintDeleteAllData(void);
void paintMenuInitialize(void);
void paintMenuPrevEraseOption(void);
void paintMenuNextEraseOption(void);
void paintSetupMainMenu(void);

// Mode struct function implemetations

void paintEnterMode(void)
{
    PAINT_LOGI("Allocating %" PRIu32 " bytes for paintMenu...", (uint32_t)sizeof(paintMainMenu_t));
    paintMenu = calloc(1, sizeof(paintMainMenu_t));

    loadFont("logbook.font", &(paintMenu->menuFont), false);

    paintMenu->menu         = initMenu(paintTitle, paintMenuCb);
    paintMenu->menuRenderer = initMenuLogbookRenderer(&paintMenu->menuFont);

    paintMenuInitialize();
}

void paintExitMode(void)
{
    PAINT_LOGD("Exiting");

    // Cleanup any sub-modes based on paintMenu->screen
    paintReturnToMainMenu();

    deinitMenu(paintMenu->menu);
    deinitMenuLogbookRenderer(paintMenu->menuRenderer);
    freeFont(&(paintMenu->menuFont));

    free(paintMenu);
    paintMenu = NULL;
}

void paintMainLoop(int64_t elapsedUs)
{
    // Handle all input frst regardless of screen
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        paintButtonCb(&evt);
    }

    switch (paintMenu->screen)
    {
        case PAINT_MENU:
        {
            if (paintMenu->enableScreensaver && getScreensaverTimeSetting() != 0)
            {
                paintMenu->idleTimer += elapsedUs;
            }

            if (getScreensaverTimeSetting() != 0 && paintMenu->idleTimer >= (getScreensaverTimeSetting() * 1000000))
            {
                PAINT_LOGI("Selected Gallery");
                paintGallerySetup(true);
                paintGallery->returnScreen = paintMenu->screen;
                paintMenu->screen          = PAINT_GALLERY;
            }
            else
            {
                drawMenuLogbook(paintMenu->menu, paintMenu->menuRenderer, elapsedUs);
            }
            break;
        }

        case PAINT_DRAW:
        case PAINT_HELP:
        {
            paintDrawScreenMainLoop(elapsedUs);
            break;
        }

        case PAINT_SHARE:
            // Implemented in a different mode
            break;

        case PAINT_RECEIVE:
            // Implemented in a different mode
            break;

        case PAINT_GALLERY:
        {
            paintGalleryMainLoop(elapsedUs);
            break;
        }
    }
}

void paintButtonCb(buttonEvt_t* evt)
{
    paintMenu->idleTimer = 0;

    switch (paintMenu->screen)
    {
        case PAINT_MENU:
        {
            paintMenu->menu = menuButton(paintMenu->menu, *evt);
            break;
        }

        case PAINT_DRAW:
        case PAINT_HELP:
        {
            paintDrawScreenButtonCb(evt);
            break;
        }

        case PAINT_GALLERY:
        {
            paintGalleryModeButtonCb(evt);
            break;
        }

        case PAINT_SHARE:
        case PAINT_RECEIVE:
            // Handled in a different mode
            break;
    }
}

void paintEventCb(const swadgeEvt_t* evt)
{
    switch (paintMenu->screen)
    {
        case PAINT_DRAW:
        case PAINT_HELP:
        {
            switch (evt->type)
            {
                case QUICK_SETTINGS_SHOW:
                {
                    if (paintState->buttonMode == BTN_MODE_DRAW)
                    {
                        paintSaveCanvas(&paintState->canvas);
                    }
                    break;
                }

                case QUICK_SETTINGS_HIDE:
                {
                    if (paintState->buttonMode == BTN_MODE_DRAW)
                    {
                        paintRestoreCanvas(&paintState->canvas);
                    }
                    break;
                }
            }
            break;
        }

        case PAINT_GALLERY:
        {
            break;
        }

        case PAINT_MENU:
        case PAINT_SHARE:
        case PAINT_RECEIVE:
            break;
    }
}

// Util function implementations

void paintMenuInitialize(void)
{
    paintMenu->idleTimer = 0;
    paintMenu->screen    = PAINT_MENU;

    paintSetupMainMenu();
}

void paintSetupMainMenu(void)
{
    int32_t index;
    paintLoadIndex(&index);

    addSingleItemToMenu(paintMenu->menu, menuOptDraw);

    if (paintGetAnySlotInUse(index))
    {
        // Only add "gallery" if there's something to view
        paintMenu->enableScreensaver = true;
        addSingleItemToMenu(paintMenu->menu, menuOptGallery);
    }
    else
    {
        paintMenu->enableScreensaver = false;
    }

    paintMenu->menu = startSubMenu(paintMenu->menu, menuOptNetwork);
    if (paintGetAnySlotInUse(index))
    {
        addSingleItemToMenu(paintMenu->menu, menuOptShare);
    }
    addSingleItemToMenu(paintMenu->menu, menuOptReceive);
    paintMenu->menu = endSubMenu(paintMenu->menu);

    addSingleItemToMenu(paintMenu->menu, menuOptHelp);

    paintMenu->menu = startSubMenu(paintMenu->menu, menuOptSettings);
    addSettingsItemToMenu(paintMenu->menu, menuOptLeds, paintGetEnableLedsBounds(), paintGetEnableLeds());
    addSettingsItemToMenu(paintMenu->menu, menuOptBlink, paintGetEnableBlinkBounds(), paintGetEnableBlink());

    paintMenu->menu = endSubMenu(paintMenu->menu);

    addSingleItemToMenu(paintMenu->menu, menuOptExit);
}

void paintMenuPrevEraseOption(void)
{
    int32_t index;
    paintLoadIndex(&index);

    if (paintGetAnySlotInUse(index))
    {
        PAINT_LOGD("A slot is in use");
        uint8_t prevSlot = paintGetPrevSlotInUse(index, paintMenu->eraseSlot);

        PAINT_LOGD("Current eraseSlot: %d, prev: %d", paintMenu->eraseSlot, prevSlot);
        if (paintMenu->eraseSlot != PAINT_SAVE_SLOTS && prevSlot >= paintMenu->eraseSlot)
        {
            PAINT_LOGD("Wrapped, moving to All");
            // we wrapped around
            paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
        }
        else
        {
            paintMenu->eraseSlot = prevSlot;
        }
    }
    else
    {
        PAINT_LOGD("No in-use slots, moving to All");
        paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
    }
}

void paintMenuNextEraseOption(void)
{
    int32_t index;
    paintLoadIndex(&index);

    if (paintGetAnySlotInUse(index))
    {
        PAINT_LOGD("A slot is in use");
        uint8_t nextSlot = paintGetNextSlotInUse(
            index, (paintMenu->eraseSlot == PAINT_SAVE_SLOTS) ? PAINT_SAVE_SLOTS - 1 : paintMenu->eraseSlot);
        PAINT_LOGD("Current eraseSlot: %d, next: %d", paintMenu->eraseSlot, nextSlot);
        if (paintMenu->eraseSlot != PAINT_SAVE_SLOTS && nextSlot <= paintMenu->eraseSlot)
        {
            PAINT_LOGD("Wrapped, moving to All");
            // we wrapped around
            paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
        }
        else
        {
            paintMenu->eraseSlot = nextSlot;
        }
    }
    else
    {
        PAINT_LOGD("No in-use slots, moving to All");
        paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
    }
}

void paintMenuCb(const char* opt, bool selected, uint32_t value)
{
    if (selected)
    {
        if (opt == menuOptDraw)
        {
            PAINT_LOGI("Selected Draw");
            paintMenu->screen = PAINT_DRAW;
            paintDrawScreenSetup();
        }
        else if (opt == menuOptGallery)
        {
            PAINT_LOGI("Selected Gallery");
            paintMenu->screen = PAINT_GALLERY;
            paintGallerySetup(false);
        }
        else if (opt == menuOptHelp)
        {
            PAINT_LOGE("Selected Help");
            paintMenu->screen = PAINT_HELP;
            paintTutorialSetup();
            paintDrawScreenSetup();
        }
        else if (opt == menuOptShare)
        {
            PAINT_LOGI("Selected Share");
            paintMenu->screen = PAINT_SHARE;
            switchToSwadgeMode(&modePaintShare);
        }
        else if (opt == menuOptReceive)
        {
            PAINT_LOGI("Selected Receive");
            paintMenu->screen = PAINT_RECEIVE;
            switchToSwadgeMode(&modePaintReceive);
        }
        else if (opt == menuOptExit)
        {
            PAINT_LOGI("Selected Exit");
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        if (opt == menuOptLeds)
        {
            paintSetEnableLeds(value);
        }
        else if (opt == menuOptBlink)
        {
            paintSetEnableBlink(value);
        }
    }
}

void paintReturnToMainMenu(void)
{
    switch (paintMenu->screen)
    {
        case PAINT_MENU:
        case PAINT_SHARE:
        case PAINT_RECEIVE:
            break;

        case PAINT_DRAW:
            paintDrawScreenCleanup();
            break;

        case PAINT_GALLERY:
            if (paintGallery->screensaverMode)
            {
                paintMenu->screen = paintGallery->returnScreen;
                paintGalleryCleanup();
                if (paintMenu->screen == PAINT_MENU)
                {
                    paintSetupMainMenu();
                }
                return;
            }
            else
            {
                paintGalleryCleanup();
            }
            break;

        case PAINT_HELP:
            paintTutorialCleanup();
            paintDrawScreenCleanup();
            break;
    }

    paintMenu->screen    = PAINT_MENU;
    paintMenu->idleTimer = 0;
    paintSetupMainMenu();
}

void paintDeleteAllData(void)
{
    int32_t index;
    paintLoadIndex(&index);

    for (uint8_t i = 0; i < PAINT_SAVE_SLOTS; i++)
    {
        paintDeleteSlot(&index, i);
    }

    paintDeleteIndex();
}

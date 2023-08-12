#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "swadge2024.h"
#include "hdw-btn.h"
#include "menu.h"
#include "menuLogbookRenderer.h"
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


const char paintTitle[] = "MFPaint";
const char menuOptDraw[] = "Draw";
const char menuOptHelp[] = "Tutorial";
const char menuOptGallery[] = "Gallery";
const char menuOptNetwork[] = "Sharing";
const char menuOptShare[] = "Share";
const char menuOptReceive[] = "Receive";
const char menuOptSettings[] = "Settings";

const char menuOptLedsOn[] = "LEDs: On";
const char menuOptLedsOff[] = "LEDs: Off";
const char menuOptBlinkOn[] = "Blink Picks: On";
const char menuOptBlinkOff[] = "Blink Picks: Off";
const char menuOptEraseData[] = "Erase: All";
char menuOptEraseSlot[] = "Erase: Slot 1";
const char menuOptCancelErase[] = "Confirm: No!";
const char menuOptConfirmErase[] = "Confirm: Yes";

const char menuOptExit[] = "Exit";
const char menuOptBack[] = "Back";

// Mode struct function declarations
void paintEnterMode(void);
void paintExitMode(void);
void paintMainLoop(int64_t elapsedUs);
void paintButtonCb(buttonEvt_t* evt);
void paintTouchCb(buttonEvt_t* evt);
void paintMainMenuCb(const char* opt);
void paintNetworkMenuCb(const char* opt);
void paintSettingsMenuCb(const char* opt);


swadgeMode_t modePaint =
{
    .modeName = paintTitle,
    .wifiMode = ESP_NOW,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .fnEnterMode = paintEnterMode,
    .fnExitMode = paintExitMode,
    .fnMainLoop = paintMainLoop,
    .fnAudioCallback = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
    .fnAdvancedUSB = NULL,
};

// Util function declarations

void paintDeleteAllData(void);
void paintMenuInitialize(void);
void paintMenuPrevEraseOption(void);
void paintMenuNextEraseOption(void);
void paintSetupMainMenu(bool reset);
void paintSetupNetworkMenu(bool reset);
void paintSetupSettingsMenu(bool reset);

// Mode struct function implemetations

void paintEnterMode(void)
{
    PAINT_LOGI("Allocating %zu bytes for paintMenu...", sizeof(paintMenu_t));
    paintMenu = calloc(1, sizeof(paintMenu_t));

    loadFont("mm.font", &(paintMenu->menuFont), false);

    paintMenu->menu = initMeleeMenu(paintTitle, &(paintMenu->menuFont), paintMainMenuCb);

    paintMenuInitialize();
}

void paintExitMode(void)
{
    PAINT_LOGD("Exiting");

    // Cleanup any sub-modes based on paintMenu->screen
    paintReturnToMainMenu();

    deinitMeleeMenu(paintMenu->menu);
    freeFont(&(paintMenu->menuFont));

    free(paintMenu);
    paintMenu = NULL;
}

void paintMainLoop(int64_t elapsedUs)
{
    switch (paintMenu->screen)
    {
    case PAINT_MENU:
    case PAINT_NETWORK_MENU:
    case PAINT_SETTINGS_MENU:
    {
        if (paintMenu->enableScreensaver && getScreensaverTime() != 0)
        {
            paintMenu->idleTimer += elapsedUs;
        }

        if (getScreensaverTime() != 0 && paintMenu->idleTimer >= (getScreensaverTime() * 1000000))
        {
            PAINT_LOGI("Selected Gallery");
            paintGallerySetup(true);
            paintGallery->returnScreen = paintMenu->screen;
            paintMenu->screen = PAINT_GALLERY;
        }
        else
        {
            drawMeleeMenu(paintMenu->menu);
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
            if (evt->down)
            {
                meleeMenuButton(paintMenu->menu, evt->button);
            }
            break;
        }

        case PAINT_NETWORK_MENU:
        {
            if (evt->down)
            {
                if (evt->button == PB_B)
                {
                    paintMenu->screen = PAINT_MENU;
                    paintSetupMainMenu(false);
                }
                else
                {
                    meleeMenuButton(paintMenu->menu, evt->button);
                }
            }
            break;
        }

        case PAINT_SETTINGS_MENU:
        {
            if (evt->down)
            {
                const char* selectedOption = paintMenu->menu->rows[paintMenu->menu->selectedRow];
                if (evt->button == PB_LEFT || evt->button == PB_RIGHT)
                {
                    if (menuOptBlinkOn == selectedOption || menuOptBlinkOff == selectedOption ||
                        menuOptLedsOn == selectedOption || menuOptLedsOff == selectedOption)
                    {
                        // left, right, a, same thing
                        paintSettingsMenuCb(selectedOption);
                    }
                    else if (menuOptCancelErase == selectedOption)
                    {
                        paintMenu->eraseDataConfirm = true;
                        paintSetupSettingsMenu(false);
                    }
                    else if (menuOptConfirmErase == selectedOption)
                    {
                        paintMenu->eraseDataConfirm = false;
                        paintSetupSettingsMenu(false);
                    }
                    else if (menuOptEraseSlot == selectedOption || menuOptEraseData == selectedOption)
                    {
                        paintMenu->settingsMenuSelection = paintMenu->menu->selectedRow;
                        if (evt->button == PB_LEFT)
                        {
                            paintMenuPrevEraseOption();
                            paintSetupSettingsMenu(false);
                        }
                        else if (evt->button == PB_RIGHT)
                        {
                            paintMenuNextEraseOption();
                            paintSetupSettingsMenu(false);
                        }
                    }
                }
                else if (evt->button == PB_B)
                {
                    if (paintMenu->eraseDataSelected)
                    {
                        paintMenu->eraseDataSelected = false;
                        paintSetupSettingsMenu(false);
                    }
                    else
                    {
                        paintMenu->screen = PAINT_MENU;
                        paintSetupMainMenu(false);
                    }
                }
                else
                {
                    meleeMenuButton(paintMenu->menu, evt->button);
                    selectedOption = paintMenu->menu->rows[paintMenu->menu->selectedRow];
                }

                if (paintMenu->eraseDataSelected && menuOptCancelErase != selectedOption &&
                    menuOptConfirmErase != selectedOption)
                {
                    // If the confirm-erase option is not selected, reset eraseDataConfirm and redraw the menu
                    paintMenu->settingsMenuSelection = paintMenu->menu->selectedRow;
                    paintMenu->eraseDataSelected = false;
                    paintMenu->eraseDataConfirm = false;
                    paintSetupSettingsMenu(false);
                }
            }
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

void paintTouchCb(buttonEvt_t* evt)
{
    if (paintMenu->screen == PAINT_DRAW || paintMenu->screen == PAINT_HELP)
    {
        paintDrawScreenTouchCb(evt);
    }
    else if (paintMenu->screen == PAINT_GALLERY)
    {
        // TODO: Make sure the gallery does its touch events
        // I deleted the callback because
    }
}

// Util function implementations

void paintMenuInitialize(void)
{
    paintMenu->menuSelection = 0;
    paintMenu->networkMenuSelection = 0;
    paintMenu->settingsMenuSelection = 0;
    paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
    paintMenu->eraseDataSelected = false;
    paintMenu->eraseDataConfirm = false;
    paintMenu->idleTimer = 0;

    paintMenu->screen = PAINT_MENU;

    paintSetupMainMenu(true);
}

void paintSetupMainMenu(bool reset)
{
    int32_t index;
    paintLoadIndex(&index);

    resetMeleeMenu(paintMenu->menu, paintTitle, paintMainMenuCb);
    addRowToMeleeMenu(paintMenu->menu, menuOptDraw);

    if (paintGetAnySlotInUse(index))
    {
        // Only add "gallery" if there's something to view
        paintMenu->enableScreensaver = true;
        addRowToMeleeMenu(paintMenu->menu, menuOptGallery);
    }
    else
    {
        paintMenu->enableScreensaver = false;
    }

    addRowToMeleeMenu(paintMenu->menu, menuOptNetwork);
    addRowToMeleeMenu(paintMenu->menu, menuOptHelp);
    addRowToMeleeMenu(paintMenu->menu, menuOptSettings);
    addRowToMeleeMenu(paintMenu->menu, menuOptExit);

    if (reset)
    {
        paintMenu->menuSelection = 0;
    }

    paintMenu->menu->selectedRow = paintMenu->menuSelection;
}

void paintSetupNetworkMenu(bool reset)
{
    int32_t index;
    paintLoadIndex(&index);

    resetMeleeMenu(paintMenu->menu, menuOptNetwork, paintNetworkMenuCb);

    if (paintGetAnySlotInUse(index))
    {
        // Only add "share" if there's something to share
        addRowToMeleeMenu(paintMenu->menu, menuOptShare);
    }

    addRowToMeleeMenu(paintMenu->menu, menuOptReceive);
    addRowToMeleeMenu(paintMenu->menu, menuOptBack);

    if (reset)
    {
        paintMenu->networkMenuSelection = 0;
    }

    paintMenu->menu->selectedRow = paintMenu->networkMenuSelection;
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
    paintSetupSettingsMenu(false);
}

void paintMenuNextEraseOption(void)
{
    int32_t index;
    paintLoadIndex(&index);

    if (paintGetAnySlotInUse(index))
    {
        PAINT_LOGD("A slot is in use");
        uint8_t nextSlot = paintGetNextSlotInUse(index, (paintMenu->eraseSlot == PAINT_SAVE_SLOTS) ? PAINT_SAVE_SLOTS - 1 : paintMenu->eraseSlot);
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

void paintSetupSettingsMenu(bool reset)
{
    int32_t index;

    resetMeleeMenu(paintMenu->menu, menuOptSettings, paintSettingsMenuCb);

    if (reset)
    {
        paintMenu->settingsMenuSelection = 0;
        paintMenu->eraseDataSelected = false;
        paintMenu->eraseDataConfirm = false;
        paintMenu->eraseSlot = PAINT_SAVE_SLOTS;
    }

    paintLoadIndex(&index);
    if (index & PAINT_ENABLE_LEDS)
    {
        addRowToMeleeMenu(paintMenu->menu, menuOptLedsOn);
    }
    else
    {
        addRowToMeleeMenu(paintMenu->menu, menuOptLedsOff);
    }

    if (index & PAINT_ENABLE_BLINK)
    {
        addRowToMeleeMenu(paintMenu->menu, menuOptBlinkOn);
    }
    else
    {
        addRowToMeleeMenu(paintMenu->menu, menuOptBlinkOff);
    }

    if (paintMenu->eraseDataSelected)
    {
        if (paintMenu->eraseDataConfirm)
        {
            addRowToMeleeMenu(paintMenu->menu, menuOptConfirmErase);
        }
        else
        {
            addRowToMeleeMenu(paintMenu->menu, menuOptCancelErase);
        }
    }
    else
    {
        if (paintMenu->eraseSlot == PAINT_SAVE_SLOTS)
        {
            addRowToMeleeMenu(paintMenu->menu, menuOptEraseData);
        }
        else
        {
            snprintf(menuOptEraseSlot, sizeof(menuOptEraseSlot), "Erase: Slot %d", paintMenu->eraseSlot % PAINT_SAVE_SLOTS + 1);
            addRowToMeleeMenu(paintMenu->menu, menuOptEraseSlot);
        }
    }
    addRowToMeleeMenu(paintMenu->menu, menuOptBack);

    paintMenu->menu->selectedRow = paintMenu->settingsMenuSelection;
}

void paintMainMenuCb(const char* opt)
{
    paintMenu->menuSelection = paintMenu->menu->selectedRow;
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
    else if (opt == menuOptNetwork)
    {
        PAINT_LOGI("Selected Network");
        paintSetupNetworkMenu(true);
        paintMenu->screen = PAINT_NETWORK_MENU;
    }
    else if (opt == menuOptHelp)
    {
        PAINT_LOGE("Selected Help");
        paintMenu->screen = PAINT_HELP;
        paintTutorialSetup();
        paintDrawScreenSetup();
    }
    else if (opt == menuOptSettings)
    {
        paintSetupSettingsMenu(true);
        paintMenu->screen = PAINT_SETTINGS_MENU;
    }
    else if (opt == menuOptExit)
    {
        PAINT_LOGI("Selected Exit");
        switchToSwadgeMode(&modeMainMenu);
    }
}

void paintNetworkMenuCb(const char* opt)
{
    paintMenu->networkMenuSelection = paintMenu->menu->selectedRow;
    if (opt == menuOptShare)
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
    else if (opt == menuOptBack)
    {
        PAINT_LOGI("Selected Back");
        paintSetupMainMenu(false);
        paintMenu->screen = PAINT_MENU;
    }
}

void paintSettingsMenuCb(const char* opt)
{
    paintMenu->settingsMenuSelection = paintMenu->menu->selectedRow;

    int32_t index;
    if (opt == menuOptLedsOff)
    {
        // Enable the LEDs
        paintLoadIndex(&index);
        index |= PAINT_ENABLE_LEDS;
        paintSaveIndex(index);
    }
    else if (opt == menuOptLedsOn)
    {
        paintLoadIndex(&index);
        index &= ~PAINT_ENABLE_LEDS;
        paintSaveIndex(index);
    }
    else if (opt == menuOptBlinkOff)
    {
        paintLoadIndex(&index);
        index |= PAINT_ENABLE_BLINK;
        paintSaveIndex(index);
    }
    else if (opt == menuOptBlinkOn)
    {
        paintLoadIndex(&index);
        index &= ~PAINT_ENABLE_BLINK;
        paintSaveIndex(index);
    }
    else if (opt == menuOptEraseData)
    {
        paintMenu->eraseDataSelected = true;
    }
    else if (opt == menuOptEraseSlot)
    {
        paintMenu->eraseDataSelected = true;
        paintMenu->eraseDataConfirm = false;
    }
    else if (opt == menuOptConfirmErase)
    {
        if (paintMenu->eraseSlot == PAINT_SAVE_SLOTS)
        {
            paintDeleteAllData();
        }
        else
        {
            paintLoadIndex(&index);
            paintDeleteSlot(&index, paintMenu->eraseSlot);
            paintMenuNextEraseOption();
        }
        paintMenu->enableScreensaver = paintMenu->enableScreensaver && paintGetAnySlotInUse(index);
        paintMenu->eraseDataConfirm = false;
        paintMenu->eraseDataSelected = false;
    }
    else if (opt == menuOptCancelErase)
    {
        paintMenu->eraseDataSelected = false;
        paintMenu->eraseDataConfirm = false;
    }
    else if (opt == menuOptBack)
    {
        PAINT_LOGI("Selected Back");
        paintSetupMainMenu(false);
        paintMenu->screen = PAINT_MENU;
        return;
    }

    paintSetupSettingsMenu(false);
}

void paintReturnToMainMenu(void)
{
    switch(paintMenu->screen)
    {
        case PAINT_MENU:
        case PAINT_NETWORK_MENU:
        case PAINT_SETTINGS_MENU:
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
                    paintSetupMainMenu(false);
                }
                else if (paintMenu->screen == PAINT_NETWORK_MENU)
                {
                    paintSetupNetworkMenu(false);
                }
                else if (paintMenu->screen == PAINT_SETTINGS_MENU)
                {
                    paintSetupSettingsMenu(false);
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

    paintMenu->screen = PAINT_MENU;
    paintMenu->idleTimer = 0;
    paintSetupMainMenu(false);
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

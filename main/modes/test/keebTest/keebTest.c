/**
 * @file keebTest.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode designed to test keyboard variations in rapid succession
 * @version 1.0
 * @date 2024-07-06
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "keebTest.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void keebEnterMode(void);
static void keebExitMode(void);
static void keebMainLoop(int64_t elapsedUs);
static void kbMenuCb(const char* label, bool selected, uint32_t settingVal);
static void setWarning(int64_t duration, char* text);
static void drawWarning(void);

//==============================================================================
// Menu Strings
//==============================================================================

// TODO:
// Add menu to configure text entry
// Add a weird pattern to demonstrate transparancy
// Add a character count for measurement purposes
// Option to keep string instead of overwriting it

/*   Menu
 * - Start typing
 * - Change font
 * - Change BG
 *   - Transparent
 *   - WSG
 *   - Color (provide options)
 * - Change Text color (provide options)
 * - Change Emphasis color (provide options)
 * - Change shadow box color and enabled (provide options for color)
 * - Set Enter visual
 * - Set Caps visual
 * - Set multiline or not
 * - Show character count
 * - Should string be reset each run?
 * - Reset to defaults
 */

const char keebTestName[] = "Keyboard Test"; // Doubles as mode name

static const char teMenuStart[]        = "Start Typing!";
static const char teMenuFont[]         = "Font: ";
static const char teMenuBG[]           = "Background";
static const char teMenuTransparent[]  = "Transparent";
static const char teMenuSprite[]       = "Sprite";
static const char teMenuSolid[]        = "Solid color";
static const char teMenuSolidActive[]  = "Set Active";
static const char teMenuShadow[]       = "Shadow Boxes";
static const char teMenuShadowActive[] = "Active";
static const char teMenuEnter[]        = "Enter Style";
static const char teMenuCaps[]         = "Caps Style";
static const char teMenuMulti[]        = "Multiline";
static const char teMenuCount[]        = "Char Count";
static const char teMenuReset[]        = "Reset String";
static const char teMenuResetHard[]    = "Reset TextEntry";

static const char colorSettingLabel1[] = "Color: ";    // Solid background color
static const char colorSettingLabel2[] = "Text: ";     // Text
static const char colorSettingLabel3[] = "Emphasis: "; // Emphasis
static const char colorSettingLabel4[] = "Color: ";    // Shadowboxes

static const int32_t colorSettingsValues[]
    = {c000, c500, c050, c005, c550, c505, c055, c555, c111, c222, c444, c433, c303};

static const char* const colorSettingsOptions[] = {
    "Black", "Red",       "Green",       "Blue",       "Yellow", "Magenta", "Cyan",
    "White", "Dark gray", "Medium Gray", "Light Gray", "Pink",   "Purple",
};

static const int32_t fontSettingsValues[] = {0, 1, 2, 3};

static const char* const fontSettingsOptions[] = {
    "vga_ibm8",
    "radiostars",
    "rodin",
    "rightous",
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t keebTestMode = {
    .modeName                 = keebTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = keebEnterMode,
    .fnExitMode               = keebExitMode,
    .fnMainLoop               = keebMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

keebTest_t* kbTest;

//==============================================================================
// Function definitions
//==============================================================================

static void keebEnterMode(void)
{
    // Initialize
    kbTest = (keebTest_t*)calloc(1, sizeof(keebTest_t));

    // Get resources
    loadWsg("kid0.wsg", &kbTest->bg, false);
    loadFont("ibm_vga8.font", &kbTest->fnt[0], false);
    loadFont("radiostars.font", &kbTest->fnt[1], false);
    loadFont("rodin_eb.font", &kbTest->fnt[2], false);
    loadFont("righteous_150.font", &kbTest->fnt[3], false);

    // Init Menu
    kbTest->menu = initMenu(keebTestName, kbMenuCb);
    addSingleItemToMenu(kbTest->menu, teMenuStart);
    addSettingsOptionsItemToMenu(kbTest->menu, teMenuFont, fontSettingsOptions, fontSettingsValues,
                                 ARRAY_SIZE(fontSettingsValues), getScreensaverTimeSettingBounds(), 0);
    kbTest->menu = startSubMenu(kbTest->menu, teMenuBG);
    addSingleItemToMenu(kbTest->menu, teMenuTransparent);
    kbTest->menu = startSubMenu(kbTest->menu, teMenuSolid);
    addSingleItemToMenu(kbTest->menu, teMenuSolidActive);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel1, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), 0);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSingleItemToMenu(kbTest->menu, teMenuSprite);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel2, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), 7);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel3, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), 7);
    kbTest->menu = startSubMenu(kbTest->menu, teMenuShadow);
    addSingleItemToMenu(kbTest->menu, teMenuShadowActive);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel4, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), 8);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSingleItemToMenu(kbTest->menu, teMenuEnter);
    addSingleItemToMenu(kbTest->menu, teMenuCaps);
    addSingleItemToMenu(kbTest->menu, teMenuMulti);
    addSingleItemToMenu(kbTest->menu, teMenuCount);
    addSingleItemToMenu(kbTest->menu, teMenuReset);
    addSingleItemToMenu(kbTest->menu, teMenuResetHard);

    // Init renderer
    kbTest->renderer = initMenuManiaRenderer(&kbTest->fnt[3], &kbTest->fnt[2]);

    // Set MENU as the starting state
    kbTest->currState = MENU;
    textEntryInit(&kbTest->fnt[0], MAX_TEXT_LEN, kbTest->typedText);
}

static void keebExitMode(void)
{
    // Deinit menu
    deinitMenu(kbTest->menu);

    // Deinit renderer
    deinitMenuManiaRenderer(kbTest->renderer);

    // Deinit fonts
    freeFont(&kbTest->fnt[0]);
    freeFont(&kbTest->fnt[1]);
    freeFont(&kbTest->fnt[2]);
    freeFont(&kbTest->fnt[3]);

    // Deinit wsgs
    freeWsg(&kbTest->bg);

    // Free mode
    free(kbTest);
}

static void keebMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    switch (kbTest->currState)
    {
        case MENU:
            while (checkButtonQueueWrapper(&evt))
            {
                kbTest->menu = menuButton(kbTest->menu, evt);
            }
            drawMenuMania(kbTest->menu, kbTest->renderer, elapsedUs);
            break;
        case TYPING:
            bool done = false;
            while (checkButtonQueueWrapper(&evt))
            {
                done = !textEntryInput(evt.down, evt.button);
            }
            if (done)
            {
                kbTest->currState = DISPLAYING;
            }
            textEntryDraw(elapsedUs);
            break;
        case DISPLAYING:
            // TODO: Text display screen
            break;
        case WARNING:
            kbTest->warningTimer -= elapsedUs;
            if (0 > kbTest->warningTimer)
            {
                kbTest->currState = MENU;
            }
            else
            {
                drawWarning();
            }
            break;
        default:
            break;
    }
}

static void kbMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == teMenuStart)
        {
            // initalize all stuff
            textEntrySetFont(&kbTest->fnt[0]);
            switch (kbTest->bckgrnd)
            {
                case COLOR_BG:
                    textEntrySetBGColor(kbTest->bgColor);
                    break;
                case WSG_BG:
                    textEntrySetBgWsg(&kbTest->bg);
                    break;
                case CLEAR_BG:
                    textEntrySetBGTransparent();
                    break;
            }
            textEntrySetTextColor(kbTest->textColor, false);
            textEntrySetEmphasisColor(kbTest->empColor);
            textEntrySetShadowboxColor(kbTest->shadow, kbTest->shadowColor);
            textEntrySetNewEnterStyle(kbTest->enter);
            textEntrySetNewCapsStyle(kbTest->caps);
            textEntrySetMultiline(kbTest->multi);
            // Switch state
            kbTest->currState = TYPING;
        }
        else if (label == teMenuTransparent)
        {
            kbTest->bckgrnd = CLEAR_BG;
            setWarning(1000, "Set BG to transparent");
        }
        else if (label == teMenuSprite)
        {
            kbTest->bckgrnd = WSG_BG;
            setWarning(1000, "Set BG to sample WSG");
        }
        else if (label == teMenuSolidActive)
        {
            kbTest->bckgrnd = COLOR_BG;
            setWarning(1000, "Set Solid BG active");
        }
        else if (label == teMenuShadowActive)
        {
            kbTest->shadow = !kbTest->shadow;
            if (kbTest->shadow)
            {
                setWarning(1000, "Shadwonboxes enabled");
            }
            else
            {
                setWarning(1000, "Shadwonboxes disabled");
            }
        }
        else if (label == teMenuEnter)
        {
            kbTest->enter = !kbTest->enter;
            if (kbTest->enter)
            {
                setWarning(1000, "Using Return symbol");
            }
            else
            {
                setWarning(1000, "Using \"ok\" for enter");
            }
        }
        else if (label == teMenuCaps)
        {
            kbTest->caps = !kbTest->caps;
            if (kbTest->caps)
            {
                setWarning(1000, "Using new caps symbol");
            }
            else
            {
                setWarning(1000, "Using old caps symbol");
            }
        }
        else if (label == teMenuMulti)
        {
            kbTest->multi = !kbTest->multi;
            if (kbTest->multi)
            {
                setWarning(1000, "Using Multi-line text entry");
            }
            else
            {
                setWarning(1000, "Using single line text entry");
            }
        }
        else if (label == teMenuCount)
        {
            kbTest->count = !kbTest->count;
            if (kbTest->count)
            {
                setWarning(1000, "Character count enabled");
            }
            else
            {
                setWarning(1000, "Character count disabled");
            }
        }
        else if (label == teMenuReset)
        {
            kbTest->reset = !kbTest->reset;
            if (kbTest->reset)
            {
                setWarning(1000, "Reset str each cycle");
            }
            else
            {
                setWarning(1000, "str persists");
            }
        }
        else if (label == teMenuResetHard)
        {
            textEntryInit(&kbTest->fnt[0], MAX_TEXT_LEN, kbTest->typedText);
            setWarning(1000, "Text Entry fully reset!");
        }
    }
    if (label == teMenuFont)
    {
        kbTest->selFnt = &kbTest->fnt[settingVal];
    }
    if (label == colorSettingLabel1)
    {
        kbTest->bgColor = settingVal;
    }
    if (label == colorSettingLabel2)
    {
        kbTest->textColor = settingVal;
    }
    if (label == colorSettingLabel3)
    {
        kbTest->empColor = settingVal;
    }
    if (label == colorSettingLabel4)
    {
        kbTest->shadowColor = settingVal;
    }
}

static void setWarning(int64_t duration, char* text)
{
    strcpy(kbTest->warningText, text);
    kbTest->warningTimer = duration * 1000;
    kbTest->currState    = WARNING;
}

static void drawWarning()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    uint8_t width = textWidth(&kbTest->fnt[0], kbTest->warningText);
    drawText(&kbTest->fnt[0], c555, kbTest->warningText, (TFT_WIDTH - width) / 2, TFT_HEIGHT / 2);
}
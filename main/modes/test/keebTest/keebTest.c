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

/**
 * @brief Entrance into the mode
 *
 */
static void keebEnterMode(void);

/**
 * @brief Mode cleanup
 *
 */
static void keebExitMode(void);

/**
 * @brief Main Loop
 *
 * @param elapsedUs Amount of time since last called
 */
static void keebMainLoop(int64_t elapsedUs);

/**
 * @brief Callback function for the menu. When input is put into a menu object, this is called.
 *
 * @param label      The string key
 * @param selected   If the A button was pressed
 * @param settingVal If scrolled to, indicates the value
 */
static void kbMenuCb(const char* label, bool selected, uint32_t settingVal);

/**
 * @brief Draws the result of the text stream
 *
 */
static void drawResult(void);

/**
 * @brief Draws the number of characters in the string
 *
 */
static void drawCount(void);

/**
 * @brief Draws random lines. Used to prove the background is transparent
 *
 */
static void drawIncidentalBG(void);

/**
 * @brief Resets the settings for the text entry
 *
 */
static void reset(void);

/**
 * @brief Set the Warning object
 *
 * @param duration Milliseconds to keep the warning up
 * @param text     A string to display
 */
static void setWarning(int64_t duration, char* text);

/**
 * @brief Draws the warning text box
 *
 */
static void drawWarning(void);

//==============================================================================
// Menu Strings
//==============================================================================

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
    loadWsg("exampleBG.wsg", &kbTest->bg, false);
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
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), c000);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSingleItemToMenu(kbTest->menu, teMenuSprite);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel2, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), c555);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel3, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), c555);
    kbTest->menu = startSubMenu(kbTest->menu, teMenuShadow);
    addSingleItemToMenu(kbTest->menu, teMenuShadowActive);
    addSettingsOptionsItemToMenu(kbTest->menu, colorSettingLabel4, colorSettingsOptions, colorSettingsValues,
                                 ARRAY_SIZE(colorSettingsValues), getScreensaverTimeSettingBounds(), c111);
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

    reset();
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
            drawIncidentalBG();
            while (checkButtonQueueWrapper(&evt))
            {
                if (!textEntryInput(evt.down, evt.button))
                {
                    kbTest->currState = DISPLAYING;
                }
            }
            textEntryDraw(elapsedUs);
            if (kbTest->count)
            {
                drawCount();
            }
            break;
        case DISPLAYING:
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    kbTest->currState = MENU;
                    textEntrySoftReset();
                }
            }
            drawResult();
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
            textEntrySetFont(&kbTest->fnt[kbTest->fontSel]);
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
            if (kbTest->reset)
            {
                strcpy(kbTest->typedText, "");
            }
            // Switch state
            kbTest->currState = TYPING;
        }
        else if (label == teMenuTransparent)
        {
            kbTest->bckgrnd = CLEAR_BG;
            setWarning(500, "Set BG to transparent");
        }
        else if (label == teMenuSprite)
        {
            kbTest->bckgrnd = WSG_BG;
            setWarning(500, "Set BG to sample WSG");
        }
        else if (label == teMenuSolidActive)
        {
            kbTest->bckgrnd = COLOR_BG;
            setWarning(500, "Set Solid BG active");
        }
        else if (label == teMenuShadowActive)
        {
            kbTest->shadow = !kbTest->shadow;
            if (kbTest->shadow)
            {
                setWarning(500, "Shadwonboxes enabled");
            }
            else
            {
                setWarning(500, "Shadwonboxes disabled");
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
                setWarning(500, "Using \"ok\" for enter");
            }
        }
        else if (label == teMenuCaps)
        {
            kbTest->caps = !kbTest->caps;
            if (kbTest->caps)
            {
                setWarning(500, "Using new caps symbol");
            }
            else
            {
                setWarning(500, "Using old caps symbol");
            }
        }
        else if (label == teMenuMulti)
        {
            kbTest->multi = !kbTest->multi;
            if (kbTest->multi)
            {
                setWarning(500, "Using Multi-line text entry");
            }
            else
            {
                setWarning(500, "Using single line text entry");
            }
        }
        else if (label == teMenuCount)
        {
            kbTest->count = !kbTest->count;
            if (kbTest->count)
            {
                setWarning(500, "Character count enabled");
            }
            else
            {
                setWarning(500, "Character count disabled");
            }
        }
        else if (label == teMenuReset)
        {
            kbTest->reset = !kbTest->reset;
            if (kbTest->reset)
            {
                setWarning(500, "Reset str each cycle");
            }
            else
            {
                setWarning(500, "str persists");
            }
        }
        else if (label == teMenuResetHard)
        {
            reset();
            setWarning(500, "Text Entry fully reset!");
        }
    }
    if (label == teMenuFont)
    {
        kbTest->fontSel = settingVal;
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

static void reset()
{
    kbTest->fontSel     = 0;
    kbTest->bckgrnd     = COLOR_BG;
    kbTest->bgColor     = c000;
    kbTest->textColor   = c555;
    kbTest->empColor    = c555;
    kbTest->shadow      = true;
    kbTest->shadowColor = c111;
    kbTest->enter       = false;
    kbTest->caps        = false;
    kbTest->multi       = false;
    strcpy(kbTest->typedText, "");
    textEntryInit(&kbTest->fnt[0], MAX_TEXT_LEN, kbTest->typedText);
}

static void drawIncidentalBG()
{
    // Draw random line
    int16_t a     = esp_random() % 280;
    int16_t b     = esp_random() % 240;
    int16_t c     = esp_random() % 280;
    int16_t d     = esp_random() % 240;
    uint8_t color = esp_random() % 216;
    drawLineFast(a, b, c, d, color);
}

static void drawResult()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    int16_t x1 = 20;
    int16_t y1 = 20;
    int16_t x2 = 260;
    int16_t y2 = 220;
    drawTextWordWrap(&kbTest->fnt[0], c555, kbTest->typedText, &x1, &y1, x2, y2);
}

static void drawCount()
{
    fillDisplayArea(20, 0, TFT_WIDTH, 13, c000);
    static char buff[32];
    snprintf(buff, 31, "Count %" PRIi16, (int16_t)strlen(kbTest->typedText));
    drawText(&kbTest->fnt[0], c555, buff, 20, 0);
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
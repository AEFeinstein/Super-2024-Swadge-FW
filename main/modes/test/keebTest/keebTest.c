/**
 * @file keebTest.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode designed to test keyboard variations in rapid succession
 * @version 1.1
 * @date 2024-07-06
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "keebTest.h"
#include "menu.h"
#include "textEntry.h"
#include "esp_random.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_TEXT_LEN 64
#define PADDING      20

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MENU,
    TYPING,
    DISPLAYING,
} State_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Assets
    font_t fnt[5];
    char text[MAX_TEXT_LEN];

    // Menu
    State_t state;
    menu_t* menu;
    menuMegaRenderer_t* renderer;

    // Settings
    bool drawLines;
} keebTest_t;

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
 * @return true to go up a menu level, false to remain here
 */
static bool kbMenuCb(const char* label, bool selected, uint32_t settingVal);

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

//==============================================================================
// Menu Strings
//==============================================================================

const char keebTestName[] = "Keyboard Test"; // Doubles as mode name

static const char teMenuStart[]  = "Start Typing!";
static const char teFontSelect[] = "Choose font: ";
static const char teMenuReset[]  = "Reset TextEntry";

static const char textSamplePrompt[] = "Type some text!";

static const int32_t fontSettingsValues[] = {0, 1, 2, 3, 4};

static const char* const fontSettingsOptions[] = {"vga_ibm8", "radiostars", "rodin", "righteous", "retro_logo"};

//==============================================================================
// Variables
//==============================================================================

static const textEntrySettings_t teSettings = {
    .textPrompt      = textSamplePrompt,
    .maxLen          = MAX_TEXT_LEN,
    .startKMod       = TE_PROPER_NOUN,
    .useMultiLine    = true,
    .useNewCapsStyle = true,
    .useOKEnterStyle = true,
    .blink           = true,
    .textColor       = c454,
    .emphasisColor   = c511,
    .bgColor         = c000,
    .shadowboxColor  = c222,
};

swadgeMode_t keebTestMode = {
    .modeName          = keebTestName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = keebEnterMode,
    .fnExitMode        = keebExitMode,
    .fnMainLoop        = keebMainLoop,
};

keebTest_t* kbTest;

//==============================================================================
// Function definitions
//==============================================================================

static void keebEnterMode(void)
{
    // Initialize
    kbTest = (keebTest_t*)heap_caps_calloc(1, sizeof(keebTest_t), MALLOC_CAP_8BIT);

    // Load fonts
    kbTest->fnt[0] = *getSysFont();
    loadFont(RADIOSTARS_FONT, &kbTest->fnt[1], false);
    loadFont(RODIN_EB_FONT, &kbTest->fnt[2], false);
    loadFont(RIGHTEOUS_150_FONT, &kbTest->fnt[3], false);
    loadFont(RETRO_LOGO_FONT, &kbTest->fnt[4], false);

    // Init Menu
    kbTest->menu = initMenu(keebTestName, kbMenuCb);
    addSingleItemToMenu(kbTest->menu, teMenuStart);
    settingParam_t fontOpt = {
        .def = fontSettingsValues[0],
        .key = NULL,
        .min = fontSettingsValues[0],
        .max = fontSettingsValues[ARRAY_SIZE(fontSettingsValues) - 1],
    };
    addSettingsOptionsItemToMenu(kbTest->menu, teFontSelect, fontSettingsOptions, fontSettingsValues,
                                 ARRAY_SIZE(fontSettingsValues), &fontOpt, fontSettingsValues[0]);
    kbTest->menu = endSubMenu(kbTest->menu);
    addSingleItemToMenu(kbTest->menu, teMenuReset);

    // Init renderer
    kbTest->renderer = initMenuMegaRenderer(NULL, NULL, NULL);

    // Init
    kbTest->state = MENU;
    textEntryInit(&teSettings, kbTest->text, &kbTest->fnt[0]);
}

static void keebExitMode(void)
{
    // Text entry
    textEntryDeinit();

    // Deinit menu
    deinitMenu(kbTest->menu);

    // Deinit renderer
    deinitMenuMegaRenderer(kbTest->renderer);

    // Deinit fonts
    freeFont(&kbTest->fnt[1]);
    freeFont(&kbTest->fnt[2]);
    freeFont(&kbTest->fnt[3]);
    freeFont(&kbTest->fnt[4]);

    // Free mode
    heap_caps_free(kbTest);
}

static void keebMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    switch (kbTest->state)
    {
        case MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                kbTest->menu = menuButton(kbTest->menu, evt);
            }
            drawMenuMega(kbTest->menu, kbTest->renderer, elapsedUs);
            break;
        }
        case TYPING:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (textEntryInput(evt))
                {
                    kbTest->state     = DISPLAYING;
                    kbTest->drawLines = false;
                }
            }
            if (kbTest->drawLines)
            {
                drawIncidentalBG();
            }

            textEntryDraw(elapsedUs);
            drawCount();
            break;
        }
        case DISPLAYING:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    kbTest->state = MENU;
                    textEntrySoftReset();
                }
            }
            drawResult();
            break;
        }

        default:
        {
            break;
        }
    }
}

static bool kbMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == teMenuStart)
        {
            kbTest->drawLines = true;
            kbTest->state     = TYPING;
        }
        else if (label == teMenuReset)
        {
            textEntrySoftReset();
        }
    }
    return false;
}

static void drawIncidentalBG()
{
    // Draw random line
    int16_t a     = esp_random() % TFT_WIDTH;
    int16_t b     = esp_random() % TFT_HEIGHT;
    int16_t c     = esp_random() % TFT_WIDTH;
    int16_t d     = esp_random() % TFT_HEIGHT;
    uint8_t color = esp_random() % cTransparent;
    drawLineFast(a, b, c, d, color);
}

static void drawResult()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    int16_t x1 = PADDING;
    int16_t y1 = PADDING;
    int16_t x2 = TFT_WIDTH - PADDING;
    int16_t y2 = TFT_HEIGHT - PADDING;
    drawTextWordWrap(&kbTest->fnt[0], c555, kbTest->text, &x1, &y1, x2, y2);
}

static void drawCount()
{
    fillDisplayArea(180, 0, TFT_WIDTH, 13, c000);
    static char buff[32];
    snprintf(buff, 31, "Count %" PRIi16, (int16_t)strlen(kbTest->text));
    drawText(&kbTest->fnt[0], c555, buff, 180, 1);
}
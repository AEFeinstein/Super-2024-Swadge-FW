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
#include "textEntry.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void keebEnterMode(void);
static void keebExitMode(void);
static void keebMainLoop(int64_t elapsedUs);
static void menuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Menu Strings
//==============================================================================
#define MAX_TEXT_LEN 128

typedef struct
{
    wsg_t bg;
    font_t fnt;
    bool displayText;
    char typedText[MAX_TEXT_LEN];
} keebTest_t;

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
static const char teMenuFont[]         = "Edit Font";
static const char teMenuBG[]           = "Background";
static const char teMenuTransparent[]  = "Transparent";
static const char teMenuSprite[]       = "Sprite";
static const char teMenuSolid[]        = "Solid color";
static const char teMenuText[]         = "Text color";
static const char teMenuEmphasis[]     = "Emphasis color";
static const char teMenuShadow[]       = "Shadow Boxes";
static const char teMenuShadowActive[] = "Active";
static const char teMenuColor[]        = "Color";
static const char teMenuEnter[]        = "Enter Style";
static const char teMenuCaps[]         = "Caps Style";
static const char teMenuMulti[]        = "Multiline";
static const char teMenuCount[]        = "Char Count";
static const char teMenuReset[]        = "Reset str";
static const char teMenuResetHard[]    = "Reset TE";

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
    loadFont("ibm_vga8.font", &kbTest->fnt1, false);
    loadFont("radiostars.font", &kbTest->fnt2, false);
    loadFont("rodin_eb.font", &kbTest->fnt3, false);
    loadFont("righteous_150.font", &kbTest->fnt4, false);

    // Init Menu
    kbtest->menu = initMenu(keebTestName, menuCb);
    addSingleItemToMenu(kbtest->menu, teMenuStart);
    addSingleItemToMenu(kbtest->menu, teMenuFont);
    kbtest->menu = startSubMenu(kbtest->menu, teMenuBG);
    addSingleItemToMenu(kbtest->menu, teMenuTransparent);
    addSingleItemToMenu(kbtest->menu, teMenuSprite);
    addSingleItemToMenu(kbtest->menu, teMenuSolid);
    kbtest->menu = endSubMenu(kbtest->menu);
    addSingleItemToMenu(kbtest->menu, teMenuText);
    addSingleItemToMenu(kbtest->menu, teMenuEmphasis);
    kbtest->menu = startSubMenu(kbtest->menu, teMenuShadow);
    addSingleItemToMenu(kbtest->menu, teMenuShadowActive);
    addSingleItemToMenu(kbtest->menu, teMenuColor);
    kbtest->menu = endSubMenu(kbtest->menu);
    addSingleItemToMenu(kbtest->menu, teMenuEnter);
    addSingleItemToMenu(kbtest->menu, teMenuCaps);
    addSingleItemToMenu(kbtest->menu, teMenuMulti);
    addSingleItemToMenu(kbtest->menu, teMenuCount);
    addSingleItemToMenu(kbtest->menu, teMenuReset);
    addSingleItemToMenu(kbtest->menu, teMenuResetHard);
 
    // Init renderer
    menuManiaRenderer_t* renderer = initMenuManiaRenderer(NULL, NULL, NULL);

    // Set MENU as the starting state
    kbTest->displayText = true;
}

static void keebExitMode(void)
{
    freeFont(&kbTest->fnt);
    freeWsg(&kbTest->bg);
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
                menu = menuButton(menu, evt);
            }
            // drawMenuMania();
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
    }
}

static void menuCb(const char* label, bool selected, uint32_t settingVal)
{
}
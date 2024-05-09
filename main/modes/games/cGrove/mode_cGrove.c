/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small Chao garden inspiried program
 * @version 0.1
 * @date 2024-05-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// Includes
//==============================================================================
#include "mode_cGrove.h"

// Variables
//==============================================================================
 swadgeMode_t cGroveMode = {
    .modeName                 = cGroveTitle,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = cGroveEnterMode,
    .fnExitMode               = cGroveExitMode,
    .fnMainLoop               = cGroveMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = cGroveBackgroundDrawCallback,
    .fnEspNowRecvCb           = cGroveEspNowRecvCb,
    .fnEspNowSendCb           = cGroveEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

cGrove_t* grove = NULL;

// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    // Mode memory allocation
    grove = calloc(1, sizeof(cGrove_t));

    // Load a font
    loadFont("logbook.font", &grove->menuFont, false);

    // Menu initialization
    cGroveInitMenu();

    // Initialize system
    grove->currState = MENU;

    // TEST CODE
    static char test[] = "test ";
    static char nameBuffer[USERNAME_CHARS];
    strcpy(grove->player.username, test);
    grove->player.pronouns = HE_HIM;
    grove->player.mood = HAPPY;
    for (int i = 0; i < MAX_PREV_GUESTS; i++){
        snprintf(nameBuffer, sizeof(nameBuffer)-1, "Test_%" PRIu32, i);
        strcpy(grove->guests[i].username, nameBuffer);
        grove->guests[i].pronouns = HE_HIM;
        grove->guests[i].mood = HAPPY;
    }
}

static void cGroveExitMode(void)
{
    deinitMenu(grove->cGroveMenu);
    deinitMenuLogbookRenderer(grove->rndr);
    freeFont(&grove->menuFont);
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{
    buttonEvt_t evt = {0};
    switch (grove->currState){
        case MENU:
            while(checkButtonQueueWrapper(&evt))
            {
                grove->cGroveMenu = menuButton(grove->cGroveMenu, evt);
            }
            drawMenuLogbook(grove->cGroveMenu, grove->rndr, 0);
            break;
        case PROFILE:
            cGroveProfileMain(grove);
            break;
        case SUBPROFILE:
            cGroveShowSubProfile(grove);
            break;
        default:
            // TODO: Default
    }
}

static void cGroveMenuCB(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == cGroveStrPlay) {
            // TODO: Load Grove
        } else if (label == cGroveCompStrFight) {
            // TODO: Load Fight
        } else if (label == cGroveCompStrRace) {
            // TODO: Load Race
        } else if (label == cGroveCompStrPerf) {
            // TODO: Load Performance
        } else if (label == cGroveShopStrGroc) {
            // TODO: Load Store with grocery
        } else if (label == cGroveShopStrToy) {
            // TODO: Load Store with toys
        } else if (label == cGroveShopStrBook) {
            // TODO: Load Store with books
        } else if (label == cGroveShopStrSchool) {
            // TODO: Load School
        } else if (label == cGroveOLStrViewProf) {
            grove->customMenuSelect = 0;
            grove->currState = PROFILE;
        } else if (label == cGroveUser) {
            // TODO: Load Text entry
        } else if (label == cGrovePronounHe) {
            grove->player.pronouns = HE_HIM;
        } else if (label == cGrovePronounShe) {
            grove->player.pronouns = SHE_HER;
        } else if (label == cGrovePronounThey) {
            grove->player.pronouns = THEY_THEM;
        } else if (label == cGrovePronounOther) {
            grove->player.pronouns = OTHER;
        } else if (label == cGroveOLStrEnable) {
            cGroveToggleOnlineFeatures(grove);
        }
    }
}

static void cGroveInitMenu()
{
    grove->cGroveMenu = initMenu(cGroveTitle, cGroveMenuCB);
    addSingleItemToMenu(grove->cGroveMenu, cGroveStrPlay);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrCompete);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrFight);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrRace);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrPerf);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrShops);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrGroc);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrToy);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrBook);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrSchool);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrOnline);
    addSingleItemToMenu(grove->cGroveMenu, cGroveOLStrViewProf);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveOLStrEditProf);
    addSingleItemToMenu(grove->cGroveMenu, cGroveUser);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGrovePronoun);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounHe);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounShe);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounThey);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounOther);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveMood);
    // TODO: Add mood selections
    //addSingleItemToMenu(grove->cGroveMenu, );
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    addSingleItemToMenu(grove->cGroveMenu, cGroveOLStrEnable);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->rndr = initMenuLogbookRenderer(&grove->menuFont);
}

// Graphics Functions
//==============================================================================
static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    
}
//==============================================================================
// Includes
//==============================================================================

#include "ggDraw.h"

// Data
#include "ggWSGs.h"

// Functions
#include "ggLevel.h"
#include "ggScoring.h"

//==============================================================================
// Defines
//==============================================================================

// All
#define TITLE_TEXT_Y 42
#define BUFFER_X     12
#define FLOOR_HEIGHT 40

// Warning
#define CONTROL_LOCKOUT (GG_SECOND)
#define TEXT_START      64

// Splash
#define SPLASH_TEXT_BUFFER 2
#define SPLASH_TEXT_X      44
#define SPLASH_TEXT_Y      44
#define SPLASH_TEXT_Y_OFF  210
#define SPLASH_US          (GG_SECOND / 2)

// Menu
#define OPTION_SPACING 24
#define OPTION_BUFFER  80
#define OPTION_X       32
#define OPTION_GAP     36

// Rules
#define RULES_X_BORDER 32
#define RULES_TITLE_Y  18
#define RULES_DESC_Y   8

// Level
// Ready
#define READY_Y_OFFSET   100
#define NUMBERS_Y_OFFSET 160
#define NUMBERS_SPACING  (TFT_WIDTH / 4)
// Pause
#define PAUSE_INSTRUCTIONS_Y 64
// UI
#define UI_STALL_X       12
#define UI_STALL_Y       44
#define UI_SPACING       4
#define UI_X_DIM         12
#define UI_BAR_PADDING_X 12
#define UI_BAR_PADDING_Y 28
#define UI_BAR_HEIGHT    10
#define UI_TIMER_X       64
#define UI_TIMER_Y       5
#define UI_ARROW_Y       60
// End screen
#define SCORE_X          32
#define SCORE_Y          80
#define END_INSTR_OFFSET 160

// Urinals
#define URINAL_SPACING      41
#define URINAL_7_SPACING    37
#define URINAL_ROW_WIDTH    259
#define URINAL_HEIGHT       132
#define SMALL_URINAL_OFFSET 15
// Dividers
#define DIVIDER_X_OFFSET   32
#define DIVIDER_TOP_OFFSET -24
#define DIVIDER_BOT_OFFSET 12
// NPCs
#define NPC_X_OFFSET       3
#define NPC_Y_OFFSET       (URINAL_HEIGHT - 100)
#define NPC_LEGS_OFFSET    8
#define NPC_PANTS_X_OFFSET 6
#define NPC_PANTS_X_LARGE  -3
#define NPC_PANTS_Y_HIKE   -12

//==============================================================================
// Const
//==============================================================================

static const char* titleText = "Press 'A' to start!";

static const char* const warningText[] = {
    "ATTENTION",
    "This game contains 'suggestive themes' and may not be suitable for all audiences. Press A to continue and "
    "anything other button to back out.",
    "Suggestive themes is a stupid way to put 'this game shows butts.' Saturday morning cartoons shows butts. "
    "Everyone has one, this isn't going to cause someone to have some sort of awakening or turn your children into "
    "perverts, it's just some very low pixel count butts. Geting you underwear in a twist over this says more about "
    "you that you would probably like.",
};

static const char* const levelText[] = {
    "Ready?",
    "3",
    "2",
    "1",
    "Paused",
    "Press A to continue, B to quit",
    "Good choice!",
    "You used a stall.",
    "Press any button to advance to next round",
    "You peed yourself",
    "You used a broken toilet",
    "All the stalls were filled",
    "Press any button to go back to the menu",
};

static const char* const menuText[] = {
    "Play!", "Rules", "High Scores", "Options", "Activate Helper Mode: ", "Touch entry: ", "Quit", "Back", "On", "Off",
};

static const paletteColor_t skinColors[] = {
    c555, c333, c444, c023, c402, c233, c343,
};
static const paletteColor_t shirtColors[] = {
    c500, c050, c005, c440, c204, c404, c044,
};
static const paletteColor_t shirtAccentColors[] = {
    c400, c040, c004, c330, c103, c303, c033,
};
static const paletteColor_t pantsColors[] = {
    c024, c330, c222, c224, c503, c240, c031,
};
static const paletteColor_t pantsAccentColors[] = {
    c012, c220, c111, c113, c402, c130, c020,
};
static const paletteColor_t shoeColors[] = {
    c210,
    c000,
    c111,
    c300,
};

//==============================================================================
// Function Declarations
//==============================================================================

// Common

/**
 * @brief Draws the background
 *
 * @param ggd Game data
 */
static void drawBackground(ggData_t* ggd);

/**
 * @brief Draws all of urinals
 *
 * @param ggd Game data
 */
static void drawUrinalArray(ggData_t* ggd);

/**
 * @brief Get the star position fo the Urinal row
 *
 * @param ggd Game data
 * @return int Start pixel for the urinals based on spacing
 */
static int getUrinalRowStart(ggData_t* ggd);

/**
 * @brief Draws the urinal
 *
 * @param ggd Game data
 * @param u urinal to draw
 * @param x X position
 * @param y Y position
 */
static void drawUrinal(ggData_t* ggd, ggUrinal_t* u, int x, int y);

/**
 * @brief Draws an NPC
 *
 * @param ggd Game data
 * @param u associated urinal
 * @param x X position
 * @param y Y position
 */
static void drawNPC(ggData_t* ggd, ggUrinal_t* u, int x, int y);

/**
 * @brief Draws the title
 *
 * @param ggd Game data
 */
static void drawTitleText(ggData_t* ggd);

/**
 * @brief Draws the current scores
 *
 * @param ggd Game data
 * @param x X position
 * @param y Y Position
 */
static void drawScores(ggData_t* ggd, int x, int y);

/**
 * @brief Draws the UI layer
 *
 * @param ggd Game data
 * @param solution If the solution should be drawn
 */
static void drawUI(ggData_t* ggd, bool solution);

/**
 * @brief Draws the solution overlay
 *
 * @param ggd Game data
 */
static void drawSolution(ggData_t* ggd);

//==============================================================================
// Functions
//==============================================================================

// Initialization

void ggInitWarning(ggData_t* ggd)
{
    if (ggd->toggle)
    {
        ggd->timeLimit = 0;
    }
    else
    {
        ggd->timeLimit = CONTROL_LOCKOUT;
    }
}

void ggInitSplash(ggData_t* ggd)
{
    // Splash screen
    ggClearUrinals(ggd);
    ggd->timer                     = 0;
    ggd->numActive                 = 6;
    ggd->urinals[0].small          = 1;
    ggd->urinals[1].npc.active     = 1;
    ggd->urinals[1].npc.skinColor  = 4;
    ggd->urinals[1].npc.shirtColor = 2;
    ggd->urinals[1].npc.randOffset = 4;
    ggd->urinals[1].npc.pantsColor = 2;
    ggd->urinals[1].npc.randOffset = -3;
    ggd->urinals[2].divider        = GG_DIV_BOTTOM;
    ggd->urinals[4].puddle         = GG_PEE_MED;
    ggd->urinals[4].outOfOrder     = 1;
    ggd->urinals[5].npc.active     = 1;
    ggd->urinals[5].npc.skinColor  = 6;
    ggd->urinals[5].npc.shirtColor = 5;
    ggd->urinals[5].npc.pantsColor = 1;
    ggd->urinals[5].npc.pants      = GG_SHORTS;
}

// Single screens

void ggDrawWarning(ggData_t* ggd, int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    const char* displayText = warningText[GG_WARNING_MESSAGE];
    // Timer
    ggd->timer += elapsedUs;
    if (ggd->timer >= LONG_WAIT)
    {
        displayText = warningText[GG_WARNING_MANI];
    }
    else if (ggd->timer >= ggd->timeLimit)
    {
        ggd->toggle = true;
    }
    // Draw
    drawText(&ggd->titleFont, c500, warningText[GG_WARNING_TEXT],
             (TFT_WIDTH - textWidth(&ggd->titleFont, warningText[GG_WARNING_TEXT])) / 2, TITLE_TEXT_Y);

    int16_t xOff = BUFFER_X;
    int16_t yOff = TEXT_START;
    drawTextWordWrap(&ggd->descFont, c555, displayText, &xOff, &yOff, TFT_WIDTH - BUFFER_X, TFT_HEIGHT);
}

void ggDrawSplash(ggData_t* ggd, int64_t elapsedUs)
{
    drawBackground(ggd);
    drawUrinalArray(ggd);
    drawTitleText(ggd);
    ggd->timer += elapsedUs;
    if (ggd->timer >= SPLASH_US)
    {
        ggd->toggle = !ggd->toggle;
        ggd->timer  = 0;
    }
    if (!ggd->toggle)
    {
        int x = (TFT_WIDTH - textWidth(&ggd->titleFont, titleText)) / 2;
        drawText(&ggd->titleFont, c555, titleText, x, SPLASH_TEXT_Y_OFF);
        drawText(&ggd->titleFontOutline, c000, titleText, x, SPLASH_TEXT_Y_OFF);
    }
}

void ggDrawReady(ggData_t* ggd, int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ggd->titleFont, c555, levelText[GG_TEXT_READY],
             (TFT_WIDTH - textWidth(&ggd->titleFont, levelText[GG_TEXT_READY])) / 2, READY_Y_OFFSET);
    drawText(&ggd->descFont, c555, levelText[GG_TEXT_3],
             NUMBERS_SPACING - textWidth(&ggd->titleFont, levelText[GG_TEXT_3]) / 2, NUMBERS_Y_OFFSET);
    if (ggd->timer >= GG_SECOND)
    {
        drawText(&ggd->descFont, c555, levelText[GG_TEXT_2],
                 NUMBERS_SPACING * 2 - textWidth(&ggd->titleFont, levelText[GG_TEXT_3]) / 2, NUMBERS_Y_OFFSET);
    }
    if (ggd->timer >= GG_SECOND * 2)
    {
        drawText(&ggd->descFont, c555, levelText[GG_TEXT_1],
                 NUMBERS_SPACING * 3 - textWidth(&ggd->titleFont, levelText[GG_TEXT_3]) / 2, NUMBERS_Y_OFFSET);
    }
}

void ggDrawPause(ggData_t* ggd)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ggd->titleFont, c555, levelText[GG_TEXT_PAUSED],
             (TFT_WIDTH - textWidth(&ggd->titleFont, levelText[GG_TEXT_PAUSED])) / 2, TITLE_TEXT_Y);
    int16_t xOff = BUFFER_X;
    int16_t yOff = PAUSE_INSTRUCTIONS_Y;
    drawTextWordWrap(&ggd->descFont, c555, levelText[GG_TEXT_INSTR_PAUSE], &xOff, &yOff, TFT_WIDTH - BUFFER_X,
                     TFT_HEIGHT);
}

void ggDrawResult(ggData_t* ggd)
{
    drawBackground(ggd);
    // Pick Title text
    ggLevelText_t title = GG_TEXT_BAD_2;
    if (!ggd->hasLost)
    {
        if (ggd->stallUsed)
        {
            title = GG_TEXT_GOOD_2;
        }
        else
        {
            title = GG_TEXT_GOOD_1;
        }
    }
    else if (ggd->timeRemaining <= 0)
    {
        title = GG_TEXT_BAD_1;
    }
    else if (ggd->stallUsed && ggd->stallUses <= 0)
    {
        title = GG_TEXT_BAD_3;
    }
    drawText(&ggd->titleFont, c555, levelText[title], (TFT_WIDTH - textWidth(&ggd->titleFont, levelText[title])) / 2,
             TITLE_TEXT_Y);
    drawText(&ggd->titleFont, c000, levelText[title], (TFT_WIDTH - textWidth(&ggd->titleFont, levelText[title])) / 2,
             TITLE_TEXT_Y);
    // Draw score
    drawScores(ggd, SCORE_X, SCORE_Y);
    // Draw instructions
    int16_t xOff = BUFFER_X + 5;
    int16_t yOff = END_INSTR_OFFSET;
    drawTextWordWrap(&ggd->descFont, c000, levelText[((ggd->hasLost) ? GG_TEXT_INSTR_BAD : GG_TEXT_INSTR_GOOD)], &xOff,
                     &yOff, TFT_WIDTH - BUFFER_X, TFT_HEIGHT);
}

// Levels
void ggDrawLevel(ggData_t* ggd, bool solution)
{
    drawBackground(ggd);
    drawUrinalArray(ggd);
    drawUI(ggd, solution);
}

// Menus
void ggDrawMenu(ggData_t* ggd)
{
    // Draw background
    drawBackground(ggd);
    // Draw Title
    drawTitleText(ggd);
    // Draw options
    for (int idx = 0; idx < GG_TEXT_MENU_COUNT; idx++)
    {
        drawText(&ggd->titleFont, c555, menuText[idx], OPTION_X, OPTION_BUFFER + idx * OPTION_SPACING);
        drawText(&ggd->titleFontOutline, c000, menuText[idx], OPTION_X, OPTION_BUFFER + idx * OPTION_SPACING);
    }
    // Draw quit
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_QUIT], OPTION_X,
             OPTION_BUFFER + GG_TEXT_MENU_COUNT * OPTION_SPACING);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_QUIT], OPTION_X,
             OPTION_BUFFER + GG_TEXT_MENU_COUNT * OPTION_SPACING);
    // Draw selection
    int yOff = OPTION_BUFFER + (ggd->selection) * OPTION_SPACING - 6;
    int xOff;
    if (ggd->selection == GG_TEXT_MENU_COUNT)
    {
        xOff = OPTION_X + textWidth(&ggd->titleFont, menuText[GG_TEXT_QUIT]) + 10;
    }
    else
    {
        xOff = OPTION_X + textWidth(&ggd->titleFont, menuText[ggd->selection]) + 10;
    }
    drawWsgSimple(&ggd->uiImages[1], xOff, yOff);
}

void ggDrawRules(ggData_t* ggd)
{
    // Draw background
    drawBackground(ggd);
    // Draw information
    int16_t xOff = 32;
    int16_t yOff = 32;
    drawText(&ggd->titleFont, c555, rulesText[ggd->selection * 2], RULES_X_BORDER, RULES_TITLE_Y);
    drawText(&ggd->titleFontOutline, c000, rulesText[ggd->selection * 2], RULES_X_BORDER, RULES_TITLE_Y);
    yOff += RULES_DESC_Y;
    drawTextWordWrap(&ggd->descFont, c111, rulesText[ggd->selection * 2 + 1], &xOff, &yOff, TFT_WIDTH - RULES_X_BORDER,
                     TFT_HEIGHT);
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Page %d/%d", ggd->selection + 1, (int)ARRAY_SIZE(rulesText) / 2);
    drawText(&ggd->descFont, c000, buffer, RULES_X_BORDER, TFT_HEIGHT - 32);
}

void ggDrawHighScore(ggData_t* ggd)
{
    // Draw background
    drawBackground(ggd);
    // TItle
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_MENU_HS], RULES_X_BORDER, RULES_TITLE_Y);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_MENU_HS], RULES_X_BORDER, RULES_TITLE_Y);
    // TODO: Revise for Swadgepass
    // Draw high Scores based on selection page
    /* char buffer[36];
    int32_t outVal;
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest number of games won: %" PRId32, outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 60);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[2], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest adjusted score: %" PRId32, outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 80);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[3], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest total score: %" PRId32, outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 100);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[4], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest average: %" PRId32, outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 120); */
}

void ggDrawOptions(ggData_t* ggd)
{
    // Draw background
    drawBackground(ggd);
    // Title
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_MENU_OPTIONS], OPTION_X / 2, 10);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_MENU_OPTIONS], OPTION_X / 2, 10);
    // Options
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_OPTIONS_HELPER], OPTION_X / 4, 10 + OPTION_GAP);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_OPTIONS_HELPER], OPTION_X / 4, 10 + OPTION_GAP);
    if (ggd->helper)
    {
        drawText(&ggd->titleFont, c000, menuText[GG_TEXT_OFF], OPTION_X, 10 + 2 * OPTION_GAP);
        drawText(&ggd->titleFont, c555, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 2 * OPTION_GAP);
        drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 2 * OPTION_GAP);
    }
    else
    {
        drawText(&ggd->titleFont, c555, menuText[GG_TEXT_OFF], OPTION_X, 10 + 2 * OPTION_GAP);
        drawText(&ggd->titleFont, c000, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 2 * OPTION_GAP);
        drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_OFF], OPTION_X, 10 + 2 * OPTION_GAP);
    }
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_OPTIONS_TOUCH], OPTION_X / 4, 10 + 3 * OPTION_GAP);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_OPTIONS_TOUCH], OPTION_X / 4, 10 + 3 * OPTION_GAP);
    if (ggd->touch)
    {
        drawText(&ggd->titleFont, c000, menuText[GG_TEXT_OFF], OPTION_X, 10 + 4 * OPTION_GAP);
        drawText(&ggd->titleFont, c555, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 4 * OPTION_GAP);
        drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 4 * OPTION_GAP);
    }
    else
    {
        drawText(&ggd->titleFont, c555, menuText[GG_TEXT_OFF], OPTION_X, 10 + 4 * OPTION_GAP);
        drawText(&ggd->titleFont, c000, menuText[GG_TEXT_ON],
                 TFT_WIDTH - OPTION_X - textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]), 10 + 4 * OPTION_GAP);
        drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_OFF], OPTION_X, 10 + 4 * OPTION_GAP);
    }
    drawText(&ggd->titleFont, c555, menuText[GG_TEXT_BACK], OPTION_X / 2, TFT_HEIGHT - 40);
    drawText(&ggd->titleFontOutline, c000, menuText[GG_TEXT_BACK], OPTION_X / 2, TFT_HEIGHT - 40);

    // Draw selection
    switch (ggd->selection)
    {
        case 0:
        {
            if (ggd->helper)
            {
                drawLineFast(TFT_WIDTH - OPTION_X, 10 + 2 * OPTION_GAP + 20,
                             TFT_WIDTH - (textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]) + OPTION_X),
                             10 + 2 * OPTION_GAP + 20, c500);
            }
            else
            {
                drawLineFast(OPTION_X, 10 + 2 * OPTION_GAP + 20,
                             OPTION_X + textWidth(&ggd->titleFont, menuText[GG_TEXT_OFF]), 10 + 2 * OPTION_GAP + 20,
                             c500);
            }
            break;
        }
        case 1:
        {
            if (ggd->touch)
            {
                drawLineFast(TFT_WIDTH - OPTION_X, 10 + 4 * OPTION_GAP + 20,
                             TFT_WIDTH - (textWidth(&ggd->titleFont, menuText[GG_TEXT_ON]) + OPTION_X),
                             10 + 4 * OPTION_GAP + 20, c500);
            }
            else
            {
                drawLineFast(OPTION_X, 10 + 4 * OPTION_GAP + 20,
                             OPTION_X + textWidth(&ggd->titleFont, menuText[GG_TEXT_OFF]), 10 + 4 * OPTION_GAP + 20,
                             c500);
            }
            break;
        }
        default:
        {
            drawLineFast(OPTION_X / 2, TFT_HEIGHT - 20,
                         OPTION_X / 2 + textWidth(&ggd->titleFont, menuText[GG_TEXT_BACK]), TFT_HEIGHT - 20, c500);
            break;
        }
    }

    // Red line under option
    // - sel = 3 pos
    // - either true ot false
}

//==============================================================================
// Static Functions
//==============================================================================

// Common
static void drawBackground(ggData_t* ggd)
{
    fillDisplayArea(0, 0, TFT_WIDTH, ggd->backgroundImages[1].h, c222);
    drawLineFast(0, ggd->backgroundImages[1].h, TFT_WIDTH, ggd->backgroundImages[1].h, c000);
    for (int x = 0; x < TFT_WIDTH; x++)
    {
        for (int y = 1; y < TFT_HEIGHT; y++)
        {
            drawWsgSimple(&ggd->backgroundImages[1], x * ggd->backgroundImages[1].w, y * ggd->backgroundImages[1].h);
        }
    }
    fillDisplayArea(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT, c445);
    drawLineFast(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT - FLOOR_HEIGHT, c000);
    drawWsgSimple(&ggd->backgroundImages[0], TFT_WIDTH - ggd->backgroundImages[0].w,
                  TFT_HEIGHT - (ggd->backgroundImages[0].h + FLOOR_HEIGHT));
}

static void drawUrinalArray(ggData_t* ggd)
{
    int xStart = getUrinalRowStart(ggd);
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        int spacing = ((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING);
        drawUrinal(ggd, &ggd->urinals[idx], idx * spacing + xStart, URINAL_HEIGHT);
        drawNPC(ggd, &ggd->urinals[idx], idx * spacing + xStart, URINAL_HEIGHT);
    }
}

static int getUrinalRowStart(ggData_t* ggd)
{
    return (URINAL_ROW_WIDTH - (((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING) * ggd->numActive))
           / 2;
}

static void drawUrinal(ggData_t* ggd, ggUrinal_t* u, int x, int y)
{
    // Divider - Always at same height
    switch (u->divider)
    {
        case GG_DIV_FULL:
        default:
        {
            drawWsgSimple(&ggd->urinalImages[18],
                          x + ((ggd->numActive == MAX_URINALS) ? DIVIDER_X_OFFSET : DIVIDER_X_OFFSET + 2),
                          y + DIVIDER_TOP_OFFSET);
            break;
        }
        case GG_DIV_TOP:
        {
            drawWsgSimple(&ggd->urinalImages[19],
                          x + ((ggd->numActive == MAX_URINALS) ? DIVIDER_X_OFFSET : DIVIDER_X_OFFSET + 2),
                          y + DIVIDER_TOP_OFFSET);
            break;
        }
        case GG_DIV_BOTTOM:
        {
            drawWsgSimple(&ggd->urinalImages[20],
                          x + ((ggd->numActive == MAX_URINALS) ? DIVIDER_X_OFFSET : DIVIDER_X_OFFSET + 2),
                          y + DIVIDER_BOT_OFFSET);
            break;
        }
        case GG_DIV_NONE:
        {
            break;
        }
    }
    // Set height offset
    y += (u->small) ? SMALL_URINAL_OFFSET : 0;
    // Puddle - Needs to happen before toilet is generated since debris can be on top
    switch (u->puddle)
    {
        case GG_PEE_NONE:
        default:
        {
            break;
        }
        case GG_PEE_SMALL:
        {
            drawWsgSimple(&ggd->urinalImages[21], x + 6, TFT_HEIGHT - FLOOR_HEIGHT + 4);
            break;
        }
        case GG_PEE_MED:
        {
            drawWsgSimple(&ggd->urinalImages[22], x + 3, TFT_HEIGHT - FLOOR_HEIGHT + 1);
            break;
        }
        case GG_PEE_LARGE:
        {
            drawWsgSimple(&ggd->urinalImages[23], x - 2, TFT_HEIGHT - FLOOR_HEIGHT + 1);
            break;
        }
    }
    // Flush
    drawWsgSimple(&ggd->urinalImages[u->autoFlush],
                  x + (ggd->urinalImages[2].w - ggd->urinalImages[u->autoFlush].w) / 2,
                  y - ggd->urinalImages[u->autoFlush].h);
    // Top
    drawWsgSimple(&ggd->urinalImages[2], x, y);
    // Middle
    int currHeight;
    for (currHeight = ggd->urinalImages[2].h; currHeight < u->height; currHeight++)
    {
        drawWsgSimple(&ggd->urinalImages[3], x, y + currHeight);
    }
    // Bottom
    if (u->brokenBowl)
    {
        drawWsgSimple(&ggd->urinalImages[5], x, y + currHeight);
        drawWsgSimple(&ggd->urinalImages[6], x + 16, TFT_HEIGHT - FLOOR_HEIGHT + 4);
    }
    else
    {
        drawWsgSimple(&ggd->urinalImages[4], x, y + currHeight);
    }
    // Drain
    if (u->brokenDrain)
    {
        drawWsgSimple(&ggd->urinalImages[8], x + (ggd->urinalImages[2].w - ggd->urinalImages[8].w) / 2,
                      y + currHeight + ggd->urinalImages[4].h);
        drawWsgSimple(&ggd->urinalImages[9], x + 5, TFT_HEIGHT - FLOOR_HEIGHT + 4);
    }
    else
    {
        drawWsgSimple(&ggd->urinalImages[7], x + (ggd->urinalImages[2].w - ggd->urinalImages[7].w) / 2,
                      y + currHeight + ggd->urinalImages[4].h);
    }
    // Graffiti
    if ((u->graffiti & 0x1) == 0x1)
    {
        drawWsgSimple(&ggd->urinalImages[10], x + 16, y + 25);
    }
    if ((u->graffiti & 0x2) == 0x2)
    {
        drawWsgSimple(&ggd->urinalImages[11], x + 5, y + 15);
    }
    if ((u->graffiti & 0x4) == 0x4)
    {
        drawWsgSimple(&ggd->urinalImages[12], x + 7, y + 5);
    }
    // Cracks
    if ((u->cracks & 0x1) == 0x1)
    {
        drawWsgSimple(&ggd->urinalImages[13], x + 3, y + 25);
    }
    if ((u->cracks & 0x2) == 0x2)
    {
        drawWsgSimple(&ggd->urinalImages[14], x + ggd->urinalImages[2].w - (ggd->urinalImages[14].w + 4), y + 8);
    }
    // Water Leak
    if (u->waterLeak)
    {
        drawWsgSimple(&ggd->urinalImages[15], x + 12, y + 4);
    }
    // Plugged Drain
    if (u->pluggedDrain)
    {
        drawWsgSimple(&ggd->urinalImages[16], x + 5, y + currHeight - 1);
    }
    // OOO
    if (u->outOfOrder)
    {
        drawWsgSimple(&ggd->urinalImages[17], x + 5, y + 9);
    }
}

static void drawNPC(ggData_t* ggd, ggUrinal_t* u, int x, int y)
{
    ggNPC_t* n = &u->npc;
    // Bail if not active
    if (!n->active)
    {
        return;
    }
    // Set height offset
    y += (u->small) ? SMALL_URINAL_OFFSET : 0;
    // Set palette
    wsgPaletteSet(&ggd->npcPalette, c555, skinColors[n->skinColor]);
    wsgPaletteSet(&ggd->npcPalette, c500, shirtColors[n->shirtColor]);
    wsgPaletteSet(&ggd->npcPalette, c300, shirtAccentColors[n->shirtColor]);
    wsgPaletteSet(&ggd->npcPalette, c024, pantsColors[n->pantsColor]);
    wsgPaletteSet(&ggd->npcPalette, c013, pantsAccentColors[n->pantsColor]);
    wsgPaletteSet(&ggd->npcPalette, c005, c555);
    if (n->shoes)
    {
        wsgPaletteSet(&ggd->npcPalette, c210, shoeColors[n->shoeColor]);
    }
    else
    {
        wsgPaletteSet(&ggd->npcPalette, c210, skinColors[n->skinColor]);
    }
    // Vars
    int xOff = x - NPC_X_OFFSET;
    int yOff = y - NPC_Y_OFFSET - ((u->small) ? SMALL_URINAL_OFFSET : 0) + ((n->small) ? ggd->npcImages[1].h : 0)
               + n->randOffset;
    // Body
    drawWsgPaletteSimple(&ggd->npcImages[0], xOff, yOff, &ggd->npcPalette);
    drawWsgPaletteSimple(&ggd->npcImages[1], xOff + NPC_LEGS_OFFSET, yOff + ggd->npcImages[0].h, &ggd->npcPalette);
    if (!n->small)
    {
        drawWsgPaletteSimple(&ggd->npcImages[1], xOff + NPC_LEGS_OFFSET,
                             yOff + ggd->npcImages[0].h + ggd->npcImages[1].h, &ggd->npcPalette);
        drawWsgPaletteSimple(&ggd->npcImages[2], xOff + NPC_LEGS_OFFSET,
                             yOff + ggd->npcImages[0].h + 2 * ggd->npcImages[1].h, &ggd->npcPalette);
    }
    else
    {
        drawWsgPaletteSimple(&ggd->npcImages[2], xOff + NPC_LEGS_OFFSET,
                             yOff + ggd->npcImages[0].h + ggd->npcImages[1].h, &ggd->npcPalette);
    }
    // Shirts
    if (n->shirt)
    {
        drawWsgPaletteSimple(&ggd->npcImages[4], xOff + 1, yOff + 24, &ggd->npcPalette);
    }
    // Pants
    switch (n->pants)
    {
        case GG_PANTS:
        default:
        {
            if (n->small)
            {
                drawWsgPaletteSimple(&ggd->npcImages[6], x + NPC_PANTS_X_OFFSET,
                                     yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            }
            else
            {
                drawWsgPaletteSimple(&ggd->npcImages[5], x + NPC_PANTS_X_OFFSET,
                                     yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            }
            break;
        }
        case GG_SHORTS:
        {
            drawWsgPaletteSimple(&ggd->npcImages[7], x + NPC_PANTS_X_OFFSET,
                                 yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            break;
        }
        case GG_SKIRT:
        {
            drawWsgPaletteSimple(&ggd->npcImages[8], x + NPC_PANTS_X_LARGE, yOff + ggd->npcImages[0].h - 13,
                                 &ggd->npcPalette);
            break;
        }
        case GG_DOWN:
        {
            drawWsgPaletteSimple(&ggd->npcImages[9], x + NPC_PANTS_X_LARGE, yOff + ggd->npcImages[0].h + 17,
                                 &ggd->npcPalette);
            break;
        }
        case GG_UNDERWEAR:
        {
            drawWsgPaletteSimple(&ggd->npcImages[10], x + NPC_PANTS_X_OFFSET, yOff + ggd->npcImages[0].h - 10,
                                 &ggd->npcPalette);
            break;
        }
        case GG_NO_PANTS:
        {
            break;
        }
    }
    // Stink
    if (n->stink)
    {
        drawWsgSimple(&ggd->npcImages[3], x + 8, yOff - 26);
    }
}

static void drawTitleText(ggData_t* ggd)
{
    int stdSpacing = SPLASH_TEXT_BUFFER + ggd->titleImages[0].w;
    drawWsgSimple(&ggd->titleImages[0], SPLASH_TEXT_X + stdSpacing * 0, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[5], SPLASH_TEXT_X + stdSpacing * 1, SPLASH_TEXT_Y - 1);
    drawWsgSimple(&ggd->titleImages[2], SPLASH_TEXT_X + stdSpacing * 2, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[2], SPLASH_TEXT_X + stdSpacing * 3, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[3], SPLASH_TEXT_X + stdSpacing * 4 - 4, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[0], SPLASH_TEXT_X + stdSpacing * 6 - 20, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[1], SPLASH_TEXT_X + stdSpacing * 7 - 20, SPLASH_TEXT_Y);
    drawWsgSimple(&ggd->titleImages[4], SPLASH_TEXT_X + stdSpacing * 8 - 20, SPLASH_TEXT_Y);
}

static void drawScores(ggData_t* ggd, int x, int y)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Number of Levels completed: %d", ggd->numLevels);
    drawText(&ggd->descFont, c000, buffer, x, y + 0 * (ggd->descFont.height + 4));
    snprintf(buffer, sizeof(buffer) - 1, "Total score: %d", ggd->totalScore);
    drawText(&ggd->descFont, c000, buffer, x, y + 1 * (ggd->descFont.height + 4));
    snprintf(buffer, sizeof(buffer) - 1, "Accuracy score: %d.%d", ggd->accScore / 10, ggd->accScore % 10);
    drawText(&ggd->descFont, c000, buffer, x, y + 2 * (ggd->descFont.height + 4));
    snprintf(buffer, sizeof(buffer) - 1, "Adjusted score: %d", ggd->adjScore);
    drawText(&ggd->descFont, c000, buffer, x, y + 3 * (ggd->descFont.height + 4));
}

static void drawUI(ggData_t* ggd, bool solution)
{
    // Stalls icon
    drawWsgSimple(&ggd->uiImages[0], UI_STALL_X, UI_STALL_Y);
    // "X"
    int xStart = UI_STALL_X + ggd->uiImages[0].w + UI_SPACING;
    int yStart = UI_STALL_Y + (ggd->uiImages[0].h - UI_X_DIM) / 2 - 4;
    drawLineFast(xStart, yStart, xStart + UI_X_DIM, yStart + UI_X_DIM, c000);
    drawLineFast(xStart + UI_X_DIM, yStart, xStart, yStart + UI_X_DIM, c000);
    // Number of stall uses remaining
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "%i", ggd->stallUses);
    drawText(&ggd->descFont, c000, buffer, xStart + UI_X_DIM + UI_SPACING, yStart);
    // Draw selection
    xStart = getUrinalRowStart(ggd);
    drawWsg(&ggd->uiImages[1],
            xStart + ggd->selection * ((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING), UI_ARROW_Y,
            false, false, 270);
    // Timer/Solution bits
    if (solution && ggd->helper)
    {
        drawSolution(ggd);
    }
    else if (!solution)
    {
        long timeLeft = ggd->timeLimit - ggd->timer;
        snprintf(buffer, sizeof(buffer) - 1, "Time left: %ld.%03ld", timeLeft / GG_SECOND,
                 (timeLeft % GG_SECOND) / 1000);
        drawText(&ggd->descFont, c550, buffer, UI_TIMER_X, UI_TIMER_Y);
        // Prog bar
        drawRectFilled(UI_BAR_PADDING_X, UI_BAR_PADDING_Y, TFT_WIDTH - UI_BAR_PADDING_X,
                       UI_BAR_PADDING_Y + UI_BAR_HEIGHT, c111);
        drawRectFilled(UI_BAR_PADDING_X + 1, UI_BAR_PADDING_Y + 1,
                       UI_BAR_PADDING_X + 1
                           + ((ggd->timer * (TFT_WIDTH - (2 * (UI_BAR_PADDING_X + 1) + 1))) / ggd->timeLimit),
                       UI_BAR_PADDING_Y + UI_BAR_HEIGHT - 1, c440);
    }
    else if (ggd->stallUsed)
    {
        drawWsgSimpleHalf(&ggd->uiImages[(ggd->hasLost) ? 2 : 3], 35, 45);
    }
    else
    {
        ggUrinal_t* u = &ggd->urinals[ggd->selection];
        bool bad      = u->brokenBowl || u->brokenDrain || u->outOfOrder || u->pluggedDrain;
        drawWsgSimpleHalf(
            &ggd->uiImages[(bad) ? 2 : 3],
            ggd->selection * ((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart - 1, 140);
    }
}

static void drawSolution(ggData_t* ggd)
{
    int values[7] = {0};
    int best, worst;
    ggCalcUrinalScores(ggd->urinals, ggd->numActive, values, &best, &worst);

    int bestUIPos[7]  = {0};
    int worstUIPos[7] = {0};
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        ggUrinal_t* u = &ggd->urinals[idx];
        if (u->npc.active)
        {
            continue; // Not an option, and usually have a lower score
        }
        else
        {
            if (best == values[idx] && !(u->brokenBowl || u->brokenDrain || u->outOfOrder || u->pluggedDrain))
            {
                bestUIPos[idx] = 1;
            }
            if (worst == values[idx])
            {
                worstUIPos[idx] = 1;
            }
        }
    }
    for (int idx = 0; idx < ggd->numActive; idx++)
    { // Second pass to ensure all the "Can't pick" options are marked off
        ggUrinal_t* u = &ggd->urinals[idx];
        if (u->npc.active)
        {
            continue; // Not an option, and usually have a lower score
        }
        if (u->brokenBowl || u->brokenDrain || u->outOfOrder || u->pluggedDrain)
        {
            worstUIPos[idx] = 1;
        }
    }
    int xStart = getUrinalRowStart(ggd);
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        if (bestUIPos[idx] == 1)
        {
            drawWsgSimpleHalf(&ggd->uiImages[3],
                              idx * ((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart - 1,
                              140);
        }
        if (worstUIPos[idx] == 1 && worstUIPos[idx] != bestUIPos[idx])
        {
            drawWsgSimpleHalf(&ggd->uiImages[2],
                              idx * ((ggd->numActive == MAX_URINALS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart - 1,
                              140);
        }
    }
}
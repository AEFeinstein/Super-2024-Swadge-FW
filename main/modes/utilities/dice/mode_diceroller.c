// TODO: Add LED interaction.
// TODO: Add outer geometry of dice to make them more recognizable

//==============================================================================
// Includes
//==============================================================================

#include "mode_diceroller.h"

//==============================================================================
// Defines
//==============================================================================

#define DR_MAX_HIST 6
#define MAX_DICE    6
#define COUNT_COUNT 8

//==============================================================================
// Enums
//==============================================================================

enum dr_stateVals
{
    DR_STARTUP   = 0,
    DR_SHOW_ROLL = 1,
    DR_ROLLING   = 2
};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t x;
    int32_t y;
} vector_t;

typedef struct
{
    int total;
    int side;
    int count;
} rollHistoryEntry_t;

typedef struct
{
    int numFaces;
    int polyEdges;
} die_t;

typedef struct
{
    font_t ibm_vga8;
    wsg_t woodTexture;
    wsg_t cursor;
    wsg_t corner;

    int stateAdvanceFlag;
    int state;

    int requestCount;
    int requestDieIdx;
    bool activeSelection;

    int32_t rollRotAnimTimerUs;
    int32_t rollNumAnimTimerUs;

    const die_t* rollDie;
    int rollSize;
    int rollTotal;
    int* rolls;
    int* fakeVals;

    list_t history;

    vec3d_t lastOrientation;
    list_t shakeHistory;
    bool isShook;
} diceRoller_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void diceEnterMode(void);
void diceExitMode(void);
void diceMainLoop(int64_t elapsedUs);
void diceButtonCb(buttonEvt_t* evt);

void getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius, vector_t* vertices);
void drawRegularPolygon(int xCenter, int yCenter, int8_t sides, float rotDeg, int16_t radius, paletteColor_t col,
                        int dashWidth);
void changeActiveSelection(void);
void changeDiceCountRequest(int change);
void changeDiceSidesRequest(int change);
void doRoll(int count, const die_t* die);
void doStateMachine(int64_t elapsedUs);

void drawSelectionText(void);
void drawSelectionPointerSprite(void);
void drawDiceText(int* diceVals);
void drawDiceBackground(int rotationOffsetDeg);
void genFakeVals(void);

void drawCurrentTotal(void);

void printHistory(void);
void addTotalToHistory(void);

void drawPanel(int x0, int y0, int x1, int y1);
void drawHistoryPanel(void);

void drawBackgroundTable(void);

float cosDeg(float degrees);
float sinDeg(float degrees);

//==============================================================================
// Const Variables
//==============================================================================

static const die_t dice[] = {
    {.numFaces = 2, .polyEdges = 10}, {.numFaces = 4, .polyEdges = 3},   {.numFaces = 6, .polyEdges = 4},
    {.numFaces = 8, .polyEdges = 6},  {.numFaces = 10, .polyEdges = 4},  {.numFaces = 12, .polyEdges = 10},
    {.numFaces = 20, .polyEdges = 6}, {.numFaces = 100, .polyEdges = 6},
};

static const int32_t rollAnimationPeriod = 1000000; // 1 Second Spin
static const int32_t fakeValRerollPeriod = 100000;  // Change numbers every 0.1s

static const char DR_NAMESTRING[]        = "Dice Roller";
static const char str_next_roll_format[] = "Next roll is %dd%d";

static const paletteColor_t diceBackgroundColor = c112;
static const paletteColor_t diceTextColor       = c550;
static const paletteColor_t selectionTextColor  = c555;
static const paletteColor_t diceOutlineColor    = c223;
static const paletteColor_t totalTextColor      = c555;
static const paletteColor_t histTextColor       = c444;
// Panel colors
static const paletteColor_t outerGold  = c550;
static const paletteColor_t innerGold  = c540;
static const paletteColor_t panelColor = c400;
static const paletteColor_t textColor  = c555;

static const int xGridOffset    = TFT_WIDTH / 8;
static const int xGridMargin    = TFT_WIDTH / 5;
static const int yGridMargin    = TFT_HEIGHT / 7;
static const int xGridOffsets[] = {
    TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 + xGridOffset,               //
    TFT_WIDTH / 2 + xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
    TFT_WIDTH / 2 + xGridOffset,               //
    TFT_WIDTH / 2 + xGridMargin + xGridOffset,
};
static const int yGridOffsets[] = {
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 - yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin, //
    TFT_HEIGHT / 2 + yGridMargin,
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t modeDiceRoller = {
    .modeName                 = DR_NAMESTRING,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = diceEnterMode,
    .fnExitMode               = diceExitMode,
    .fnMainLoop               = diceMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

diceRoller_t* diceRoller;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter and set up the dice roller
 */
void diceEnterMode(void)
{
    diceRoller = heap_caps_calloc(1, sizeof(diceRoller_t), MALLOC_CAP_8BIT);

    loadFont("ibm_vga8.font", &diceRoller->ibm_vga8, false);
    loadWsg("woodTexture64.wsg", &diceRoller->woodTexture, false);
    loadWsg("upCursor8.wsg", &diceRoller->cursor, false);
    loadWsg("goldCornerTR.wsg", &diceRoller->corner, false);

    diceRoller->rolls = NULL;

    diceRoller->rollSize  = 0;
    diceRoller->rollDie   = NULL;
    diceRoller->rollTotal = 0;

    diceRoller->requestCount  = 1;
    diceRoller->requestDieIdx = 6;

    diceRoller->activeSelection = false;

    diceRoller->state            = DR_STARTUP;
    diceRoller->stateAdvanceFlag = 0;

    clear(&diceRoller->history);

    // Turn LEDs off
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Free dice roller memory and exit
 */
void diceExitMode(void)
{
    freeFont(&diceRoller->ibm_vga8);
    freeWsg(&diceRoller->woodTexture);
    freeWsg(&diceRoller->cursor);
    freeWsg(&diceRoller->corner);

    if (diceRoller->rolls != NULL)
    {
        heap_caps_free(diceRoller->rolls);
    }
    if (diceRoller->fakeVals != NULL)
    {
        heap_caps_free(diceRoller->fakeVals);
    }

    // Clear history
    while (diceRoller->history.length)
    {
        heap_caps_free(pop(&diceRoller->history));
    }

    heap_caps_free(diceRoller);
}

/**
 * @brief Dice roller main loop, handle buttons, state machine, and animation
 *
 * @param elapsedUs The time since this function was last called
 */
void diceMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        diceButtonCb(&evt);
    }

    if (checkForShake(&diceRoller->lastOrientation, &diceRoller->shakeHistory, &diceRoller->isShook))
    {
        if (diceRoller->isShook)
        {
            doRoll(diceRoller->requestCount, &dice[diceRoller->requestDieIdx]);
        }
    }
    doStateMachine(elapsedUs);
}

/**
 * @brief Handle a dice roller button event. Moves cursor and rolls dice
 *
 * @param evt The button event
 */
void diceButtonCb(buttonEvt_t* evt)
{
    switch (evt->button)
    {
        case PB_A:
        case PB_B:
        default:
        {
            if (evt->down)
            {
                doRoll(diceRoller->requestCount, &dice[diceRoller->requestDieIdx]);
            }
            break;
        }
        case PB_UP:
        case PB_DOWN:
        {
            int dir = (evt->button == PB_UP) ? 1 : -1;
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                if (!(diceRoller->activeSelection))
                {
                    changeDiceCountRequest(dir);
                }
                else
                {
                    changeDiceSidesRequest(dir);
                }
            }
            break;
        }
        case PB_LEFT:
        case PB_RIGHT:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                changeActiveSelection();
            }
            break;
        }
    }
}

/**
 * @brief Draws a tiled wood texture on the entire background
 *
 * This could be a background callback, but it works fine as-is and this mode doesn't need to be super performant
 */
void drawBackgroundTable(void)
{
    for (int x = 0; x < TFT_WIDTH; x += diceRoller->woodTexture.w)
    {
        for (int y = 0; y < TFT_HEIGHT; y += diceRoller->woodTexture.h)
        {
            drawWsgTile(&diceRoller->woodTexture, x, y);
        }
    }
}

/**
 * @brief Main state machine for the dice roller. Handles rolling, drawing, and animation
 *
 * @param elapsedUs The time since this function was last called
 */
void doStateMachine(int64_t elapsedUs)
{
    // Always draw the background table first
    drawBackgroundTable();

    // Always draw the selection text
    drawSelectionText();

    switch (diceRoller->state)
    {
        case DR_STARTUP:
        default:
        {
            // Draw the mode name
            drawText(&diceRoller->ibm_vga8, textColor, DR_NAMESTRING,
                     TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, DR_NAMESTRING) / 2, TFT_HEIGHT / 2);

            // Draw selection pointer
            drawSelectionPointerSprite();

            // If a roll should begin
            if (diceRoller->stateAdvanceFlag)
            {
                // Set the flags
                diceRoller->state            = DR_ROLLING;
                diceRoller->stateAdvanceFlag = 0;
            }
            break;
        }
        case DR_SHOW_ROLL:
        {
            // Draw selection pointer
            drawSelectionPointerSprite();

            // Draw everything
            drawDiceBackground(0);
            drawDiceText(diceRoller->rolls);
            drawCurrentTotal();
            drawHistoryPanel();
            printHistory();

            // If a roll should begin
            if (diceRoller->stateAdvanceFlag)
            {
                // Set the flags
                diceRoller->state            = DR_ROLLING;
                diceRoller->stateAdvanceFlag = 0;
            }
            break;
        }
        case DR_ROLLING:
        {
            // Run animation timers
            diceRoller->rollRotAnimTimerUs += elapsedUs;

            // Run a timer to generate 'fake' numbers
            diceRoller->rollNumAnimTimerUs += elapsedUs;
            while (diceRoller->rollNumAnimTimerUs >= fakeValRerollPeriod)
            {
                diceRoller->rollNumAnimTimerUs -= fakeValRerollPeriod;
                genFakeVals();
            }

            // Draw everything
            drawDiceBackground((diceRoller->rollRotAnimTimerUs * 360) / rollAnimationPeriod);
            drawDiceText(diceRoller->fakeVals);
            drawHistoryPanel();
            printHistory();

            // If the roll elapsed
            if (!diceRoller->isShook && diceRoller->rollRotAnimTimerUs > rollAnimationPeriod)
            {
                // Add to history and change the state
                addTotalToHistory();
                diceRoller->state = DR_SHOW_ROLL;
            }
            break;
        }
    }
}

/**
 * @brief Draw a panel with a filled background and border
 *
 * @param x0 The X offset to start the panel at
 * @param y0 The Y offset to start the panel at
 * @param x1 The X offset to end the panel at
 * @param y1 The Y offset to end the panel at
 */
void drawPanel(int x0, int y0, int x1, int y1)
{
    // Draw the panel
    drawRect(x0, y0, x1, y1, outerGold);
    drawRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, innerGold);
    oddEvenFill(x0, y0, x1, y1, innerGold, panelColor);

    // Draw corners around the panel
    int cornerEdge = 8;
    drawWsg(&diceRoller->corner, x0, y0, true, false, 0);                           // Draw TopLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y0, false, false, 0);             // Draw TopRight
    drawWsg(&diceRoller->corner, x0, y1 - cornerEdge, true, true, 0);               // Draw BotLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y1 - cornerEdge, false, true, 0); // Draw BotRight
}

/**
 * @brief Draw the panel for dice history
 */
void drawHistoryPanel(void)
{
    // Calculate margins
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;
    int xMargin          = 45;
    int yMargin          = 10;

    // Draw the panel
    drawPanel(histX - xMargin, //
              histY - yMargin, //
              histX + xMargin, //
              histY + (DR_MAX_HIST + 1) * histYEntryOffset + yMargin);
}

/**
 * @brief Draw dice history to the TFT
 */
void printHistory(void)
{
    // Calculate offsets
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;

    // Draw the header
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "History");
    drawText(&diceRoller->ibm_vga8, totalTextColor, totalStr,        //
             histX - textWidth(&diceRoller->ibm_vga8, totalStr) / 2, //
             histY);

    // For all the history
    node_t* histNode = diceRoller->history.first;
    int i            = 0;
    while (histNode)
    {
        rollHistoryEntry_t* entry = histNode->val;

        // Draw this history entry
        snprintf(totalStr, sizeof(totalStr), "%dd%d: %d", entry->count, entry->side, entry->total);
        drawText(&diceRoller->ibm_vga8, histTextColor, totalStr,         //
                 histX - textWidth(&diceRoller->ibm_vga8, totalStr) / 2, //
                 histY + (i + 1) * histYEntryOffset);

        // Iterate
        histNode = histNode->next;
        i++;
    }
}

/**
 * @brief Add the last dice roll to the history
 */
void addTotalToHistory(void)
{
    rollHistoryEntry_t* entry = heap_caps_calloc(1, sizeof(rollHistoryEntry_t), MALLOC_CAP_8BIT);
    entry->count              = diceRoller->rollSize;
    entry->side               = diceRoller->rollDie->numFaces;
    entry->total              = diceRoller->rollTotal;
    unshift(&diceRoller->history, entry);

    while (diceRoller->history.length > DR_MAX_HIST)
    {
        heap_caps_free(pop(&diceRoller->history));
    }
}

/**
 * @brief Draw the current dice roll total
 */
void drawCurrentTotal(void)
{
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "Total: %d", diceRoller->rollTotal);
    drawText(&diceRoller->ibm_vga8, totalTextColor, totalStr,
             TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, totalStr) / 2, TFT_HEIGHT * 7 / 8);
}

/**
 * @brief Change the cursor selection between number of dice and number of faces
 */
void changeActiveSelection(void)
{
    diceRoller->activeSelection = !(diceRoller->activeSelection);
}

/**
 * @brief Change the number of dice to roll
 *
 * Never less than 1 except at mode start, never greater than MAX_DICE
 *
 * @param change
 */
void changeDiceCountRequest(int change)
{
    diceRoller->requestCount = (diceRoller->requestCount - 1 + change + MAX_DICE) % MAX_DICE + 1;
}

/**
 * @brief Change the number of faces of the dice to roll
 *
 * @param change
 */
void changeDiceSidesRequest(int change)
{
    diceRoller->requestDieIdx = (diceRoller->requestDieIdx + change + COUNT_COUNT) % COUNT_COUNT;
}

/**
 * @brief Roll the dice
 *
 * @param count The number of dice to roll
 * @param die The type of die to roll
 */
void doRoll(int count, const die_t* die)
{
    // Check if it's already rolling
    if (diceRoller->state == DR_ROLLING)
    {
        return;
    }

    // Validate input
    if (0 >= count || NULL == die)
    {
        return;
    }

    // Reallocate rolls because the number of rolls may have changed
    if (diceRoller->rolls)
    {
        heap_caps_free(diceRoller->rolls);
    }
    diceRoller->rolls = (int*)heap_caps_calloc(count, sizeof(int), MALLOC_CAP_8BIT);

    // Make space for fake rolls too
    if (diceRoller->fakeVals)
    {
        heap_caps_free(diceRoller->fakeVals);
    }
    diceRoller->fakeVals = (int*)heap_caps_calloc(count, sizeof(int), MALLOC_CAP_8BIT);

    // Start timers fresh
    diceRoller->rollRotAnimTimerUs = 0;
    diceRoller->rollNumAnimTimerUs = 0;

    // Roll the dice!
    int total = 0;
    for (int m = 0; m < count; m++)
    {
        int curVal           = (esp_random() % die->numFaces) + 1;
        diceRoller->rolls[m] = curVal;
        total += curVal;
    }

    // Save the roll
    diceRoller->rollSize  = count;
    diceRoller->rollDie   = die;
    diceRoller->rollTotal = total;

    // Generate fake values to start
    genFakeVals();

    // Set the state to rolling
    diceRoller->state = DR_ROLLING;
}

/**
 * @brief Helper function to get cosine in degrees
 *
 * @param degrees The number, in degrees, to get the cosine of
 * @return The cosine of the input
 */
float cosDeg(float degrees)
{
    return cosf(degrees / 360.0f * 2 * M_PI);
}

/**
 * @brief Helper function to get sine in degrees
 *
 * @param degrees The number, in degrees, to get the sine of
 * @return The sine of the input
 */
float sinDeg(float degrees)
{
    return sinf(degrees / 360.0f * 2 * M_PI);
}

/**
 * @brief Get the Regular Polygon Vertices object. Used in drawRegularPolygon(). The vector array returned
 * by this function must be freed to prevent memory leaks.
 *
 * @param sides Number of sides of regular polygon
 * @param rotDeg rotation of regular polygon clockwise in degrees (first point is draw pointing to the right)
 * @param radius Radius in pixels on which vertices will be placed.
 * @param vertices [OUT] where the vertices are written, an array of (x,y) coordinates in pixels centered at (0,0) of
 * length sides.
 */
void getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius, vector_t* vertices)
{
    float increment = 360.0f / sides;
    for (int k = 0; k < sides; k++)
    {
        vertices[k].x = round(radius * cosDeg(increment * k + rotDeg));
        vertices[k].y = round(radius * sinDeg(increment * k + rotDeg));
    }
}

/**
 * @brief Draw a regular polygon given a center point, number of sides, rotation, radius, outline color, and dash width.
 * WARNING: Lines are not guaranteed to be exactly one pixel thick due to behavior of the plotLine function.
 *
 * @param xCenter x axis center of polygon
 * @param yCenter y axis center of polygon
 * @param sides number of sides of polygon
 * @param rotDeg clockwise degrees of rotation
 * @param radius radius on which vertices will be placed from center
 * @param col color of outline
 * @param dashWidth dotted line behavior. 0 for solid line.
 */
void drawRegularPolygon(int xCenter, int yCenter, int8_t sides, float rotDeg, int16_t radius, paletteColor_t col,
                        int dashWidth)
{
    // For each vertex in the polygon
    vector_t vertices[sides];
    getRegularPolygonVertices(sides, rotDeg, radius, vertices);
    for (int vertInd = 0; vertInd < sides; vertInd++)
    {
        // Find the next vertex, may wrap around
        int8_t endInd = (vertInd + 1) % sides;

        // Draw a line
        drawLine(xCenter + vertices[vertInd].x, yCenter + vertices[vertInd].y, xCenter + vertices[endInd].x,
                 yCenter + vertices[endInd].y, col, dashWidth);
    }
}

/**
 * @brief Draw text for what the current selection is
 */
void drawSelectionText(void)
{
    // Create the string
    char rollStr[32];
    snprintf(rollStr, sizeof(rollStr) - 1, str_next_roll_format, diceRoller->requestCount,
             dice[diceRoller->requestDieIdx].numFaces);

    // Draw it to the screen
    drawText(&diceRoller->ibm_vga8, selectionTextColor, rollStr,            //
             TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, rollStr) / 2, //
             TFT_HEIGHT / 8);
}

/**
 * @brief Draw the cursor for the number of dice or faces
 */
void drawSelectionPointerSprite(void)
{
    char rollStr[32];

    // Measure exactly where to draw the cursor based on the string
    snprintf(rollStr, sizeof(rollStr) - 1, str_next_roll_format, diceRoller->requestCount,
             dice[diceRoller->requestDieIdx].numFaces);
    int centerToEndPix = textWidth(&diceRoller->ibm_vga8, rollStr) / 2;
    snprintf(rollStr, sizeof(rollStr) - 1, "%dd%d", diceRoller->requestCount, dice[diceRoller->requestDieIdx].numFaces);
    int endToNumStartPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    snprintf(rollStr, sizeof(rollStr) - 1, "%d", diceRoller->requestCount);
    int firstNumPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    snprintf(rollStr, sizeof(rollStr) - 1, "%d", dice[diceRoller->requestDieIdx].numFaces);
    int lastNumPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    int countSelX  = TFT_WIDTH / 2 + centerToEndPix - endToNumStartPix + firstNumPix / 2 - 3;
    int sideSelX   = TFT_WIDTH / 2 + centerToEndPix - lastNumPix / 2 - 3;

    // Draw the cursor
    int yPointerOffset = 17;
    drawWsgSimple(&diceRoller->cursor,                                //
                  diceRoller->activeSelection ? sideSelX : countSelX, //
                  TFT_HEIGHT / 8 + yPointerOffset - 4);
}

/**
 * @brief Draw dice backgrounds, which are rotated, outlined, filled polygons
 *
 * @param rotationOffsetDeg The rotation to apply, in degrees
 */
void drawDiceBackground(int rotationOffsetDeg)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        // Draw the polygon outline
        drawRegularPolygon(xGridOffsets[m], yGridOffsets[m] + 5, diceRoller->rollDie->polyEdges,
                           -90 + rotationOffsetDeg, 20, diceOutlineColor, 0);

        // Fill the polygon
        int oERadius = 23;
        oddEvenFill(xGridOffsets[m] - oERadius, yGridOffsets[m] - oERadius + 5, xGridOffsets[m] + oERadius,
                    yGridOffsets[m] + oERadius + 5, diceOutlineColor, diceBackgroundColor);
    }
}

/**
 * @brief Draw the text for the dice. This should be done after drawDiceBackground()
 */
void drawDiceText(int* diceVals)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        // Convert integer to string
        char rollOutcome[32];
        snprintf(rollOutcome, sizeof(rollOutcome), "%d", diceVals[m]);

        // Draw the text
        drawText(&diceRoller->ibm_vga8, diceTextColor, rollOutcome,
                 xGridOffsets[m] - textWidth(&diceRoller->ibm_vga8, rollOutcome) / 2, yGridOffsets[m]);
    }
}

/**
 * @brief Generate 'fake' values to display on die while rollingZ
 */
void genFakeVals(void)
{
    // For each rolled die
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        // Pick a random number
        diceRoller->fakeVals[m] = esp_random() % diceRoller->rollDie->numFaces + 1;
    }
}

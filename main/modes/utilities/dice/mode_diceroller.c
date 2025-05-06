// TODO: Add Cosmetic Features of Colored background/foreground objects. Add Smoother animations. Add LED interaction.
// TODO: Consider adding outer geometry of dice to make them more recognizable

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
    font_t ibm_vga8;
    wsg_t woodTexture;
    wsg_t cursor;
    wsg_t corner;

    float timeAngle;

    int stateAdvanceFlag;
    int state;

    int requestCount;
    int requestSides;
    int sideIndex;

    // esp_timer_handle_t rollTimer;
    int64_t rollStartTimeUs;
    int fakeVal;
    int fakeValIndex;

    bool activeSelection;

    int rollIndex;
    int rollSides;
    int rollSize;
    int* rolls;
    int rollTotal;

    uint32_t timeUs;
    uint32_t lastCallTimeUs;
    uint32_t rollerNum;

    int histTotals[DR_MAX_HIST];
    int histSides[DR_MAX_HIST];
    int histCounts[DR_MAX_HIST];
    int histSize;
} diceRoller_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void diceEnterMode(void);
void diceExitMode(void);
void diceMainLoop(int64_t elapsedUs);
void diceButtonCb(buttonEvt_t* evt);

vector_t* getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius);
void drawRegularPolygon(int xCenter, int yCenter, int8_t sides, float rotDeg, int16_t radius, paletteColor_t col,
                        int dashWidth);
void changeActiveSelection(void);
void changeDiceCountRequest(int change);
void changeDiceSidesRequest(int change);
void doRoll(int count, int sides, int index);
void doStateMachine(int64_t elapsedUs);

void drawSelectionText(void);
void drawSelectionPointerSprite(void);
void drawDiceBackground(int* xGridOffsets, int* yGridOffsets);
void drawDiceText(int* xGridOffsets, int* yGridOffsets);
void drawDiceBackgroundAnimation(int* xGridOffsets, int* yGridOffsets, int32_t rollAnimationTimUs,
                                 float rotationOffsetDeg);
void drawFakeDiceText(int* xGridOffsets, int* yGridOffsets);
void genFakeVal(int32_t rollAnimationTimeUs, float rotationOffsetDeg);

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

static const int8_t validSides[]   = {2, 4, 6, 8, 10, 12, 20, 100};
static const int8_t polygonSides[] = {10, 3, 4, 6, 4, 10, 6, 6};

static const int32_t rollAnimationPeriod = 1000000; // 1 Second Spin
static const int32_t fakeValRerollPeriod = 90919;   //(rollAnimationPeriod / (ticksPerRollAnimation + 1)) + 10;

static const char DR_NAMESTRING[]        = "Dice Roller";
static const char str_next_roll_format[] = "Next roll is %dd%d";

static const paletteColor_t diceBackgroundColor = c112;
static const paletteColor_t diceTextColor       = c550;
static const paletteColor_t selectionTextColor  = c555;
static const paletteColor_t diceOutlineColor    = c223;
static const paletteColor_t totalTextColor      = c555;
static const paletteColor_t histTextColor       = c444;

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

    diceRoller->timeAngle = 0;
    diceRoller->rollSize  = 0;
    diceRoller->rollSides = 0;
    diceRoller->rollTotal = 0;

    diceRoller->rollerNum = 0;

    diceRoller->requestCount = 1;
    diceRoller->sideIndex    = 6;
    diceRoller->requestSides = validSides[diceRoller->sideIndex];

    diceRoller->activeSelection = false;

    diceRoller->state            = DR_STARTUP;
    diceRoller->stateAdvanceFlag = 0;

    diceRoller->histSize = 0;
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
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                if (diceRoller->requestCount > 0 && diceRoller->requestSides > 0)
                {
                    doRoll(diceRoller->requestCount, diceRoller->requestSides, diceRoller->sideIndex);
                    diceRoller->rollStartTimeUs = esp_timer_get_time();
                    diceRoller->fakeValIndex    = -1;
                    diceRoller->state           = DR_ROLLING;
                }
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
 * TODO move this function to background callback?
 */
void drawBackgroundTable(void)
{
    int edgeSize = diceRoller->woodTexture.w;
    int x_seg    = TFT_WIDTH / edgeSize + 1;
    int y_seg    = TFT_HEIGHT / edgeSize + 1;
    for (int j = 0; j < y_seg; j++)
    {
        for (int k = 0; k < x_seg; k++)
        {
            drawWsgTile(&diceRoller->woodTexture, edgeSize * k, edgeSize * j);
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
            drawText(&diceRoller->ibm_vga8, c555, DR_NAMESTRING,
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

            // Calculate grid offsets for drawing dice
            int xGridOffset    = TFT_WIDTH / 8;
            int xGridMargin    = TFT_WIDTH / 5;
            int yGridMargin    = TFT_HEIGHT / 7;
            int xGridOffsets[] = {
                TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 + xGridOffset,               //
                TFT_WIDTH / 2 + xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 + xGridOffset,               //
                TFT_WIDTH / 2 + xGridMargin + xGridOffset,
            };
            int yGridOffsets[] = {
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin,
            };

            // Draw everything
            drawDiceBackground(xGridOffsets, yGridOffsets);
            drawDiceText(xGridOffsets, yGridOffsets);
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
            // Calculate grid offsets for drawing dice
            int xGridOffset    = TFT_WIDTH / 8;
            int xGridMargin    = TFT_WIDTH / 5;
            int yGridMargin    = TFT_HEIGHT / 7;
            int xGridOffsets[] = {
                TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 + xGridOffset,               //
                TFT_WIDTH / 2 + xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 - xGridMargin + xGridOffset, //
                TFT_WIDTH / 2 + xGridOffset,               //
                TFT_WIDTH / 2 + xGridMargin + xGridOffset,
            };
            int yGridOffsets[] = {
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 - yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin, //
                TFT_HEIGHT / 2 + yGridMargin,
            };

            // Run animation timers
            // TODO use elapsedUs rather than esp_timer_get_time()?
            int32_t rollAnimationTimeUs = esp_timer_get_time() - diceRoller->rollStartTimeUs;
            float rotationOffsetDeg     = rollAnimationTimeUs / (float)rollAnimationPeriod * 360.0f;
            genFakeVal(rollAnimationTimeUs, rotationOffsetDeg);

            // Draw everything
            drawDiceBackgroundAnimation(xGridOffsets, yGridOffsets, rollAnimationTimeUs, rotationOffsetDeg);
            drawFakeDiceText(xGridOffsets, yGridOffsets);
            drawHistoryPanel();
            printHistory();

            // If the roll elapsed
            if (rollAnimationTimeUs > rollAnimationPeriod)
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
    // Panel colors
    paletteColor_t outerGold  = c550;
    paletteColor_t innerGold  = c540;
    paletteColor_t panelColor = c400;

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
    for (int i = 0; i < diceRoller->histSize; i++)
    {
        // Draw this history entry
        snprintf(totalStr, sizeof(totalStr), "%dd%d: %d", diceRoller->histCounts[i], diceRoller->histSides[i],
                 diceRoller->histTotals[i]);
        drawText(&diceRoller->ibm_vga8, histTextColor, totalStr,         //
                 histX - textWidth(&diceRoller->ibm_vga8, totalStr) / 2, //
                 histY + (i + 1) * histYEntryOffset);
    }
}

/**
 * @brief Add the last dice roll to the history
 *
 * TODO convert history to a list_t?
 */
void addTotalToHistory(void)
{
    if (diceRoller->histSize < DR_MAX_HIST)
    {
        int size = diceRoller->histSize;
        for (int i = 0; i < size; i++)
        {
            diceRoller->histTotals[size - i] = diceRoller->histTotals[size - i - 1]; // Shift vals to right
            diceRoller->histCounts[size - i] = diceRoller->histCounts[size - i - 1];
            diceRoller->histSides[size - i]  = diceRoller->histSides[size - i - 1];
        }
        diceRoller->histTotals[0] = diceRoller->rollTotal;
        diceRoller->histCounts[0] = diceRoller->rollSize;
        diceRoller->histSides[0]  = diceRoller->rollSides;
        diceRoller->histSize += 1;
    }
    else // shift out last value;
    {
        int size = diceRoller->histSize;
        for (int i = 0; i < size; i++)
        {
            if (i < size - 1)
            {
                diceRoller->histTotals[size - 1 - i] = diceRoller->histTotals[size - 2 - i]; // Shift vals to right
                diceRoller->histCounts[size - 1 - i] = diceRoller->histCounts[size - 2 - i];
                diceRoller->histSides[size - 1 - i]  = diceRoller->histSides[size - 2 - i];
            }
            else
            {
                diceRoller->histTotals[0] = diceRoller->rollTotal;
                diceRoller->histCounts[0] = diceRoller->rollSize;
                diceRoller->histSides[0]  = diceRoller->rollSides;
            }
        }
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
    diceRoller->sideIndex    = (diceRoller->sideIndex + change + COUNT_COUNT) % COUNT_COUNT;
    diceRoller->requestSides = validSides[diceRoller->sideIndex];
}

/**
 * @brief Roll the dice
 *
 * @param count The number of dice to roll
 * @param sides The number of faces per die
 * @param ind The index for the number of faces, see validSides[]
 */
void doRoll(int count, int sides, int ind)
{
    // Reallocate rolls because the number of rolls may have changed
    heap_caps_free(diceRoller->rolls);
    diceRoller->rolls = (int*)heap_caps_calloc(count, sizeof(int), MALLOC_CAP_8BIT);

    // Roll the dice!
    int total = 0;
    for (int m = 0; m < count; m++)
    {
        int curVal           = (esp_random() % sides) + 1;
        diceRoller->rolls[m] = curVal;
        total += curVal;
    }

    // Save the roll
    diceRoller->rollSize  = count;
    diceRoller->rollSides = sides;
    diceRoller->rollIndex = ind;
    diceRoller->rollTotal = total;
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
 * @return The csine of the input
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
 * @return vector_t* Returns vertices in an array of (x,y) coordinates in pixels centered at (0,0) of length sides.
 */
vector_t* getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius)
{
    vector_t* vertices = (vector_t*)heap_caps_calloc(sides, sizeof(vector_t), MALLOC_CAP_8BIT);
    float increment    = 360.0f / sides;
    for (int k = 0; k < sides; k++)
    {
        vertices[k].x = round(radius * cosDeg(increment * k + rotDeg));
        vertices[k].y = round(radius * sinDeg(increment * k + rotDeg));
    }
    return vertices;
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
    vector_t* vertices = getRegularPolygonVertices(sides, rotDeg, radius);
    for (int vertInd = 0; vertInd < sides; vertInd++)
    {
        // Find the next vertex, may wrap around
        int8_t endInd = (vertInd + 1) % sides;

        // Draw a line
        drawLine(xCenter + vertices[vertInd].x, yCenter + vertices[vertInd].y, xCenter + vertices[endInd].x,
                 yCenter + vertices[endInd].y, col, dashWidth);
    }

    // Free the vertices
    heap_caps_free(vertices);
}

/**
 * @brief Draw text for what the current selection is
 */
void drawSelectionText(void)
{
    // Create the string
    char rollStr[32];
    snprintf(rollStr, sizeof(rollStr) - 1, str_next_roll_format, diceRoller->requestCount, diceRoller->requestSides);

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
    snprintf(rollStr, sizeof(rollStr) - 1, str_next_roll_format, diceRoller->requestCount, diceRoller->requestSides);
    int centerToEndPix = textWidth(&diceRoller->ibm_vga8, rollStr) / 2;
    snprintf(rollStr, sizeof(rollStr) - 1, "%dd%d", diceRoller->requestCount, diceRoller->requestSides);
    int endToNumStartPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    snprintf(rollStr, sizeof(rollStr) - 1, "%d", diceRoller->requestCount);
    int firstNumPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    snprintf(rollStr, sizeof(rollStr) - 1, "%d", diceRoller->requestSides);
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
 * @brief TODO doc
 *
 * @param xGridOffsets
 * @param yGridOffsets
 */
void drawDiceBackground(int* xGridOffsets, int* yGridOffsets)
{
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        drawRegularPolygon(xGridOffsets[m], yGridOffsets[m] + 5, polygonSides[diceRoller->rollIndex], -90, 20,
                           diceOutlineColor, 0);
        int oERadius = 23;

        oddEvenFill(xGridOffsets[m] - oERadius, yGridOffsets[m] - oERadius + 5, xGridOffsets[m] + oERadius,
                    yGridOffsets[m] + oERadius + 5, diceOutlineColor, diceBackgroundColor);
    }
}

/**
 * @brief TODO doc
 *
 * @param xGridOffsets
 * @param yGridOffsets
 */
void drawDiceText(int* xGridOffsets, int* yGridOffsets)
{
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        char rollOutcome[32];
        snprintf(rollOutcome, sizeof(rollOutcome), "%d", diceRoller->rolls[m]);

        drawText(

            &diceRoller->ibm_vga8, diceTextColor, rollOutcome,
            xGridOffsets[m] - textWidth(&diceRoller->ibm_vga8, rollOutcome) / 2, yGridOffsets[m]);
    }
}

/**
 * @brief TODO doc
 *
 * @param xGridOffsets
 * @param yGridOffsets
 * @param rollAnimationTimUs
 * @param rotationOffsetDeg
 */
void drawDiceBackgroundAnimation(int* xGridOffsets, int* yGridOffsets, int32_t rollAnimationTimUs,
                                 float rotationOffsetDeg)
{
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        drawRegularPolygon(xGridOffsets[m], yGridOffsets[m] + 5, polygonSides[diceRoller->rollIndex],
                           -90 + rotationOffsetDeg, 20, diceOutlineColor, 0);

        int oERadius = 23;

        oddEvenFill(xGridOffsets[m] - oERadius, yGridOffsets[m] - oERadius + 5, xGridOffsets[m] + oERadius,
                    yGridOffsets[m] + oERadius + 5, diceOutlineColor, diceBackgroundColor);
    }
}

/**
 * @brief TODO doc
 *
 * @param xGridOffsets
 * @param yGridOffsets
 */
void drawFakeDiceText(int* xGridOffsets, int* yGridOffsets)
{
    for (int m = 0; m < diceRoller->rollSize; m++)
    {
        // total += diceRoller->rolls[m];
        char rollOutcome[32];
        snprintf(rollOutcome, sizeof(rollOutcome), "%d", diceRoller->fakeVal);

        drawText(

            &diceRoller->ibm_vga8, diceTextColor, rollOutcome,
            xGridOffsets[m] - textWidth(&diceRoller->ibm_vga8, rollOutcome) / 2, yGridOffsets[m]);
    }
}

/**
 * @brief TODO doc
 *
 * @param rollAnimationTimeUs
 * @param rotationOffsetDeg
 */
void genFakeVal(int32_t rollAnimationTimeUs, float rotationOffsetDeg)
{
    if (floor(rollAnimationTimeUs / fakeValRerollPeriod) > diceRoller->fakeValIndex)
    {
        diceRoller->fakeValIndex = floor(rollAnimationTimeUs / fakeValRerollPeriod);
        diceRoller->fakeVal      = esp_random() % diceRoller->rollSides + 1;
    }
}

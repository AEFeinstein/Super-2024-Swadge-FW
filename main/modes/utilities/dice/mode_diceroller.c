#include "mode_diceroller.h"

// TODO: Add Doxygen Blocks
// TODO: Add Cosmetic Features of Colored background/foreground objects. Add Smoother animations. Add LED interaction.

typedef struct
{
    int32_t x;
    int32_t y;
} vector_t;

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

void drawSelectionText(int w, int h, char* rollStr, int bfrSize);
// void drawSelectionPointer(int w,int h,char* rollStr,int bfrSize);
void drawSelectionPointerSprite(int w, int h, char* rollStr, int bfrSize);
void drawDiceBackground(int* xGridOffsets, int* yGridOffsets);
void drawDiceText(int* xGridOffsets, int* yGridOffsets);
void drawDiceBackgroundAnimation(int* xGridOffsets, int* yGridOffsets, int32_t rollAnimationTimUs,
                                 double rotationOffsetDeg);
void drawFakeDiceText(int* xGridOffsets, int* yGridOffsets);
void genFakeVal(int32_t rollAnimationTimeUs, double rotationOffsetDeg);

void drawCurrentTotal(int w, int h);

void printHistory(void);
void addTotalToHistory(void);
// void dbgPrintHist(void);

void drawPanel(int x0, int y0, int x1, int y1);
void drawHistoryPanel(void);

void drawBackgroundTable(void);

double cosDeg(double degrees);
double sinDeg(double degrees);

#define DR_MAXHIST 6

const int MAXDICE         = 6;
const int COUNTCOUNT      = 8;
const int8_t validSides[] = {2, 4, 6, 8, 10, 12, 20, 100};
// const int8_t polygonSides[] = {3, 4, 3, 4, 5, 3, 6};
const int8_t polygonSides[] = {10, 3, 4, 6, 4, 10, 6, 6};

const int32_t rollAnimationPeriod = 1000000; // 1 Second Spin
// const int32_t fakeValRerollPeriod = 200000; //200 ms
// const uint8_t ticksPerRollAnimation = 10;
const int32_t fakeValRerollPeriod = 90919; //(rollAnimationPeriod / (ticksPerRollAnimation + 1)) + 10;
const float spinScaler            = 1;

const char DR_NAMESTRING[]               = "Dice Roller";
static const char str_next_roll_format[] = "Next roll is %dd%d";

const paletteColor_t diceBackgroundColor = c112;
const paletteColor_t diceTextColor       = c550;
const paletteColor_t selectionArrowColor = c555;
const paletteColor_t selectionTextColor  = c555;
const paletteColor_t diceOutlineColor    = c223;
// const paletteColor_t diceSecondaryOutlineColor = c224;
const paletteColor_t rollTextColor  = c555;
const paletteColor_t totalTextColor = c555;
const paletteColor_t histTextColor  = c444;

enum dr_stateVals
{
    DR_STARTUP  = 0,
    DR_SHOWROLL = 1,
    DR_ROLLING  = 2
};

// int polygonOuterSides[] = []
// Consider adding outer geometry of dice to make them more recognizable

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

typedef struct
{
    font_t ibm_vga8;
    wsg_t woodTexture;
    wsg_t cursor;
    wsg_t corner;

    double timeAngle;

    int stateAdvanceFlag;
    int state;

    int requestCount;
    int requestSides;
    int sideIndex;

    // esp_timer_handle_t rollTimer;
    int64_t rollStartTimeUs;
    int fakeVal;
    int fakeValIndex;

    int activeSelection;

    int rollIndex;
    int rollSides;
    int rollSize;
    int* rolls;
    int rollTotal;

    uint32_t timeUs;
    uint32_t lastCallTimeUs;
    uint32_t rollerNum;

    int histTotals[DR_MAXHIST];
    int histSides[DR_MAXHIST];
    int histCounts[DR_MAXHIST];
    int histSize;
} diceRoller_t;

diceRoller_t* diceRoller;

void diceEnterMode(void)
{
    diceRoller = calloc(1, sizeof(diceRoller_t));

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

    diceRoller->activeSelection = 0;

    diceRoller->state            = DR_STARTUP;
    diceRoller->stateAdvanceFlag = 0;

    diceRoller->histSize = 0;
}

void diceExitMode(void)
{
    freeFont(&diceRoller->ibm_vga8);
    freeWsg(&diceRoller->woodTexture);
    freeWsg(&diceRoller->cursor);
    freeWsg(&diceRoller->corner);

    if (diceRoller->rolls != NULL)
    {
        free(diceRoller->rolls);
    }
    free(diceRoller);
    // bootloader_random_disable();
}

void diceMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        diceButtonCb(&evt);
    }
    doStateMachine(elapsedUs);
}

void diceButtonCb(buttonEvt_t* evt)
{
    switch (evt->button)
    {
        case PB_A:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                // diceRoller->rollerNum = (esp_random() % 20) + 1;
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
        case PB_B:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                // diceRoller->rollerNum = (esp_random() % 20) + 1;
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
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                if (!(diceRoller->activeSelection))
                {
                    changeDiceCountRequest(1);
                }
                else
                {
                    changeDiceSidesRequest(1);
                }
            }
            break;
        }
        case PB_DOWN:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                if (!(diceRoller->activeSelection))
                {
                    changeDiceCountRequest(-1);
                }
                else
                {
                    changeDiceSidesRequest(-1);
                }
            }
            break;
        }
        case PB_LEFT:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                changeActiveSelection();
            }
            break;
        }
        case PB_RIGHT:
        {
            if (evt->down && (diceRoller->state != DR_ROLLING))
            {
                changeActiveSelection();
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
//{

//}

void drawBackgroundTable(void)
{
    int edgeSize = 64;
    int x_seg    = TFT_WIDTH / edgeSize + 1;
    int y_seg    = TFT_HEIGHT / edgeSize + 1;
    for (int j = 0; j < y_seg; j++)
    {
        for (int k = 0; k < x_seg; k++)
        {
            drawWsg(&diceRoller->woodTexture, edgeSize * k, edgeSize * j, false, false, 0);
        }
    }
    // drawWsg(&diceRoller->woodTexture,TFT_WIDTH/2,TFT_HEIGHT/2,false,false,0);
}

void doStateMachine(int64_t elapsedUs)
{
    switch (diceRoller->state)
    {
        case DR_STARTUP:
        {
            clearPxTft();
            drawBackgroundTable();
            drawText(

                &diceRoller->ibm_vga8, c555, DR_NAMESTRING,
                TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, DR_NAMESTRING) / 2, TFT_HEIGHT / 2);

            int w = TFT_WIDTH;
            int h = TFT_HEIGHT;
            char rollStr[32];
            drawSelectionText(w, h, rollStr, 32);
            drawSelectionPointerSprite(w, h, rollStr, 32);

            if (diceRoller->stateAdvanceFlag)
            {
                diceRoller->state            = DR_ROLLING;
                diceRoller->stateAdvanceFlag = 0;
            }
            break;
        }
        case DR_SHOWROLL:
        {
            clearPxTft();
            drawBackgroundTable();
            int w = TFT_WIDTH;
            int h = TFT_HEIGHT;
            char rollStr[32];
            drawSelectionText(w, h, rollStr, 32);
            drawSelectionPointerSprite(w, h, rollStr, 32);

            int xGridOffset = w / 8;
            // int xGridMargin = w/4;
            int xGridMargin = w / 5;
            // int yGridMargin = h/7;
            int yGridMargin = h / 7;
            int xGridOffsets[]
                = {w / 2 - xGridMargin + xGridOffset, w / 2 + xGridOffset, w / 2 + xGridMargin + xGridOffset,
                   w / 2 - xGridMargin + xGridOffset, w / 2 + xGridOffset, w / 2 + xGridMargin + xGridOffset};
            int yGridOffsets[] = {h / 2 - yGridMargin, h / 2 - yGridMargin, h / 2 - yGridMargin,
                                  h / 2 + yGridMargin, h / 2 + yGridMargin, h / 2 + yGridMargin};
            drawDiceBackground(xGridOffsets, yGridOffsets);
            drawDiceText(xGridOffsets, yGridOffsets);
            drawCurrentTotal(w, h);
            // dbgPrintHist();
            drawHistoryPanel();
            printHistory();
            if (diceRoller->stateAdvanceFlag)
            {
                diceRoller->state            = DR_ROLLING;
                diceRoller->stateAdvanceFlag = 0;
            }
            break;
        }
        case DR_ROLLING:
        {
            clearPxTft();
            drawBackgroundTable();

            int w = TFT_WIDTH;
            int h = TFT_HEIGHT;
            char rollStr[32];
            drawSelectionText(w, h, rollStr, 32);

            int xGridOffset = w / 8;
            // int xGridMargin = w/4;
            int xGridMargin = w / 5;
            // int yGridMargin = h/7;
            int yGridMargin = h / 7;
            int xGridOffsets[]
                = {w / 2 - xGridMargin + xGridOffset, w / 2 + xGridOffset, w / 2 + xGridMargin + xGridOffset,
                   w / 2 - xGridMargin + xGridOffset, w / 2 + xGridOffset, w / 2 + xGridMargin + xGridOffset};
            int yGridOffsets[] = {h / 2 - yGridMargin, h / 2 - yGridMargin, h / 2 - yGridMargin,
                                  h / 2 + yGridMargin, h / 2 + yGridMargin, h / 2 + yGridMargin};

            // int total = 0;
            int32_t rollAnimationTimeUs = esp_timer_get_time() - diceRoller->rollStartTimeUs;
            double rotationOffsetDeg    = rollAnimationTimeUs / (double)rollAnimationPeriod * 360.0 * spinScaler;
            genFakeVal(rollAnimationTimeUs, rotationOffsetDeg);
            drawDiceBackgroundAnimation(xGridOffsets, yGridOffsets, rollAnimationTimeUs, rotationOffsetDeg);
            drawFakeDiceText(xGridOffsets, yGridOffsets);
            drawHistoryPanel();
            printHistory();
            if (rollAnimationTimeUs > rollAnimationPeriod)
            {
                diceRoller->state = DR_SHOWROLL;
                addTotalToHistory();
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void drawPanel(int x0, int y0, int x1, int y1)
{
    paletteColor_t outerGold  = c550;
    paletteColor_t innerGold  = c540;
    paletteColor_t panelColor = c400;
    drawRect(x0, y0, x1, y1, outerGold);
    drawRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, innerGold);
    oddEvenFill(x0, y0, x1, y1, innerGold, panelColor);

    int cornerEdge = 8;
    drawWsg(&diceRoller->corner, x0, y0, true, false, 0);                           // Draw TopLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y0, false, false, 0);             // Draw TopRight
    drawWsg(&diceRoller->corner, x0, y1 - cornerEdge, true, true, 0);               // Draw BotLeft
    drawWsg(&diceRoller->corner, x1 - cornerEdge, y1 - cornerEdge, false, true, 0); // Draw BotRight
}

void drawHistoryPanel(void)
{
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;
    int xMargin          = 45;
    int yMargin          = 10;

    drawPanel(histX - xMargin, histY - yMargin, histX + xMargin, histY + (DR_MAXHIST + 1) * histYEntryOffset + yMargin);
}

void printHistory(void)
{
    int histX            = TFT_WIDTH / 14 + 30;
    int histY            = TFT_HEIGHT / 8 + 40;
    int histYEntryOffset = 15;

    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "History");
    drawText(

        &diceRoller->ibm_vga8, totalTextColor, totalStr, histX - textWidth(&diceRoller->ibm_vga8, totalStr) / 2, histY);

    for (int i = 0; i < diceRoller->histSize; i++)
    {
        snprintf(totalStr, sizeof(totalStr), "%dd%d: %d", diceRoller->histCounts[i], diceRoller->histSides[i],
                 diceRoller->histTotals[i]);
        drawText(

            &diceRoller->ibm_vga8, histTextColor, totalStr, histX - textWidth(&diceRoller->ibm_vga8, totalStr) / 2,
            histY + (i + 1) * histYEntryOffset);
    }
}

void addTotalToHistory(void)
{
    if (diceRoller->histSize < DR_MAXHIST) // S
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

// void dbgPrintHist(void)
// {
//     ESP_LOGD("DICEROLLER","History:");
//     for(int i = 0; i < diceRoller->histSize; i++)
//     {
//         ESP_LOGD("DICEROLLER","%dd%d:%d",diceRoller->histCounts[i],diceRoller->histSides[i],diceRoller->histTotals[i]);
//         if(i < diceRoller->histSize-1)
//         {
//             ESP_LOGD("DICEROLLER",", ");
//         }
//     }
//     ESP_LOGD("DICEROLLER","\n");
// }

void drawCurrentTotal(int w, int h)
{
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "Total: %d", diceRoller->rollTotal);
    drawText(

        &diceRoller->ibm_vga8, totalTextColor, totalStr, TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, totalStr) / 2,
        TFT_HEIGHT * 7 / 8);
}

void changeActiveSelection(void)
{
    diceRoller->activeSelection = !(diceRoller->activeSelection);
}

// Never less than 1 except at mode start, never greater than MAXDICE
void changeDiceCountRequest(int change)
{
    diceRoller->requestCount = (diceRoller->requestCount - 1 + change + MAXDICE) % MAXDICE + 1;
}

void changeDiceSidesRequest(int change)
{
    diceRoller->sideIndex    = (diceRoller->sideIndex + COUNTCOUNT + change) % COUNTCOUNT;
    diceRoller->requestSides = validSides[diceRoller->sideIndex];
}

void doRoll(int count, int sides, int ind)
{
    free(diceRoller->rolls);
    diceRoller->rolls = (int*)malloc(sizeof(int) * count);
    int total         = 0;
    for (int m = 0; m < count; m++)
    {
        int curVal           = (esp_random() % sides) + 1;
        diceRoller->rolls[m] = curVal;
        total += curVal;
    }
    diceRoller->rollSize  = count;
    diceRoller->rollSides = sides;
    diceRoller->rollIndex = ind;
    diceRoller->rollTotal = total;
}

double cosDeg(double degrees)
{
    return cos(degrees / 360.0 * 2 * M_PI);
}

double sinDeg(double degrees)
{
    return sin(degrees / 360.0 * 2 * M_PI);
}

/**
 * @brief Get the Regular Polygon Vertices object. Used in drawRegularPolygon. The vector array returned
 * by this function must be freed to prevent memory leaks.
 *
 * @param sides Number of sides of regular polygon
 * @param rotDeg rotation of regular polygon clockwise in degrees (first point is draw pointing to the right)
 * @param radius Radius in pixels on which vertices will be placed.
 * @return vector_t* Returns vertices in an array of (x,y) coordinates in pixels centered at (0,0) of length sides.
 */
vector_t* getRegularPolygonVertices(int8_t sides, float rotDeg, int16_t radius)
{
    vector_t* vertices = (vector_t*)malloc(sizeof(vector_t) * sides);
    double increment   = 360.0 / sides;
    for (int k = 0; k < sides; k++)
    {
        // use display.c math functions
        vertices[k].x = round(radius * cosDeg(increment * k + rotDeg));
        vertices[k].y = round(radius * sinDeg(increment * k + rotDeg));
    }
    return vertices;
}

/**
 * @brief Draw a regular polygon given a center point, number of sides, rotation, radius, outline color, and dash width.
 * WARNING: Lines are not guaranteed to be exactly one pixel thick due to behavior of the plotLine function.
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
    vector_t* vertices = getRegularPolygonVertices(sides, rotDeg, radius);

    for (int vertInd = 0; vertInd < sides; vertInd++)
    {
        int8_t endInd = (vertInd + 1) % sides;

        drawLine(

            xCenter + vertices[vertInd].x, yCenter + vertices[vertInd].y, xCenter + vertices[endInd].x,
            yCenter + vertices[endInd].y, col, dashWidth);
    }

    free(vertices);
}

void drawSelectionText(int w, int h, char* rollStr, int bfrSize)
{
    snprintf(rollStr, bfrSize, str_next_roll_format, diceRoller->requestCount, diceRoller->requestSides);

    drawText(

        &diceRoller->ibm_vga8, selectionTextColor, rollStr,
        TFT_WIDTH / 2 - textWidth(&diceRoller->ibm_vga8, rollStr) / 2, TFT_HEIGHT / 8);
}

// void drawSelectionPointer(int w,int h,char* rollStr,int bfrSize)
// {

//     // int xPointerOffset = 40;
//     // int xPointerSelectionOffset = 16;
//     int yPointerOffset = 17;
//     //printf("rollStrSize: %d\n",(int)sizeof(rollStr));
//     snprintf(rollStr,bfrSize,"Next roll is %dd%d",diceRoller->requestCount,diceRoller->requestSides);
//     int centerToEndPix = textWidth(&diceRoller->ibm_vga8,rollStr)/2;
//     snprintf(rollStr,bfrSize,"%dd%d",diceRoller->requestCount,diceRoller->requestSides);
//     int endToNumStartPix = textWidth(&diceRoller->ibm_vga8,rollStr);
//     snprintf(rollStr,bfrSize,"%d",diceRoller->requestCount);
//     int firstNumPix = textWidth(&diceRoller->ibm_vga8,rollStr);
//     // int dWidth = textWidth(&diceRoller->ibm_vga8,"d");
//     snprintf(rollStr,bfrSize,"%d",diceRoller->requestSides);
//     int lastNumPix = textWidth(&diceRoller->ibm_vga8,rollStr);

//     //printf("%s\n", rollStr);

//     int countSelX = w/2 + centerToEndPix - endToNumStartPix + firstNumPix/2;
//     int sideSelX = w/2 + centerToEndPix - lastNumPix/2;
//     drawRegularPolygon(diceRoller->activeSelection ? sideSelX : countSelX,
//         h/8+yPointerOffset, 3, -90, 5, selectionArrowColor, 0
//     );
// }

void drawSelectionPointerSprite(int w, int h, char* rollStr, int bfrSize)
{
    // int xPointerOffset = 40;
    // int xPointerSelectionOffset = 16;
    int yPointerOffset = 17;
    // printf("rollStrSize: %d\n",(int)sizeof(rollStr));
    snprintf(rollStr, bfrSize, str_next_roll_format, diceRoller->requestCount, diceRoller->requestSides);
    int centerToEndPix = textWidth(&diceRoller->ibm_vga8, rollStr) / 2;
    snprintf(rollStr, bfrSize, "%dd%d", diceRoller->requestCount, diceRoller->requestSides);
    int endToNumStartPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    snprintf(rollStr, bfrSize, "%d", diceRoller->requestCount);
    int firstNumPix = textWidth(&diceRoller->ibm_vga8, rollStr);
    // int dWidth = textWidth(&diceRoller->ibm_vga8,"d");
    snprintf(rollStr, bfrSize, "%d", diceRoller->requestSides);
    int lastNumPix = textWidth(&diceRoller->ibm_vga8, rollStr);

    // printf("%s\n", rollStr);

    int countSelX = w / 2 + centerToEndPix - endToNumStartPix + firstNumPix / 2 - 3;
    int sideSelX  = w / 2 + centerToEndPix - lastNumPix / 2 - 3;
    drawWsg(&diceRoller->cursor, diceRoller->activeSelection ? sideSelX : countSelX, h / 8 + yPointerOffset - 4, false,
            false, 0);
    // drawRegularPolygon(diceRoller->activeSelection ? sideSelX : countSelX,
    //     h/8+yPointerOffset, 3, -90, 5, selectionArrowColor, 0
    //);
}

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

void drawDiceBackgroundAnimation(int* xGridOffsets, int* yGridOffsets, int32_t rollAnimationTimUs,
                                 double rotationOffsetDeg)
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

void genFakeVal(int32_t rollAnimationTimeUs, double rotationOffsetDeg)
{
    if (floor(rollAnimationTimeUs / fakeValRerollPeriod) > diceRoller->fakeValIndex)
    {
        diceRoller->fakeValIndex = floor(rollAnimationTimeUs / fakeValRerollPeriod);
        diceRoller->fakeVal      = esp_random() % diceRoller->rollSides + 1;
    }
}

void maskDiceTexture();
void drawDie(int sides, int value);
void drawDice(int sides, int* values);

// void diceState(int input)
//{

//}
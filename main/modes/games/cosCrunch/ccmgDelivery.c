#include "ccmgDelivery.h"

static void ccmgDeliveryInitMicrogame(void);
static void ccmgDeliveryDestroyMicrogame(void);
static void ccmgDeliveryMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                                 cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static void ccmgDeliveryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ccmgDeliveryDrawStar(int16_t x, int16_t y);

#define MINIMUM_WAIT_US        500000
#define MAXIMUM_WAIT_US        3000000
#define BUTTON_PRESS_WINDOW_US 700000
#define KNOCK_DURATION_US      400000
#define KNOCK_DELAY_US         200000

#define DAYTIME_BG_COLOR   c355
#define NIGHTTIME_BG_COLOR c013
#define STAR_COLOR         c554
#define OUTLINE_COLOR      c222
#define FLOOR_COLOR        c431
#define DOOR_COLOR         c432
#define DOORKNOB_COLOR     c444

#define FLOOR_TOP_Y       157
#define FLOOR_BOTTOM_Y    CC_DRAWABLE_HEIGHT
#define FLOOR_PLANK_COUNT 18
#define FLOOR_PLANK_WIDTH (TFT_WIDTH / FLOOR_PLANK_COUNT)

#define DOOR_WIDTH       65
#define DOOR_X           ((TFT_WIDTH - DOOR_WIDTH) / 2)
#define DOOR_TOP_Y       60
#define DOORKNOB_RADIUS  4
#define DOORKNOB_HEIGHT  45
#define DOORKNOB_X_INSET 10

#define WINDOW_TOP_Y    50
#define WINDOW_OFFSET_X 40
#define WINDOW_WIDTH    29

#define STARBURST_ROT_US_PER_DEG  8000
#define TUMBLEWEED_X_OFFSET       ((TFT_WIDTH - DOOR_WIDTH) / 2 - 20)
#define TUMBLEWEED_X_US_PER_PX    20000
#define TUMBLEWEED_Y_OFFSET       (FLOOR_TOP_Y - 35)
#define TUMBLEWEED_Y_RANGE        15
#define TUMBLEWEED_ROT_US_PER_DEG 12000

static const char ccmgDeliveryVerb[]       = "Delivery";
static const char ccmgDeliverySuccessMsg[] = "Package Get!";
static const char ccmgDeliveryKnock[]      = "KNOCK";
const cosCrunchMicrogame_t ccmgDelivery    = {
       .verb                     = ccmgDeliveryVerb,
       .successMsg               = ccmgDeliverySuccessMsg,
       .failureMsg               = NULL,
       .timeoutUs                = MAXIMUM_WAIT_US + BUTTON_PRESS_WINDOW_US + 500000,
       .fnInitMicrogame          = ccmgDeliveryInitMicrogame,
       .fnDestroyMicrogame       = ccmgDeliveryDestroyMicrogame,
       .fnMainLoop               = ccmgDeliveryMainLoop,
       .fnBackgroundDrawCallback = ccmgDeliveryBackgroundDrawCallback,
       .fnMicrogameTimeout       = NULL,
};

paletteColor_t const timeOfDayColors[] = {
    DAYTIME_BG_COLOR,
    NIGHTTIME_BG_COLOR,
};

paletteColor_t const wallColors[] = {
    // Pea soup yellow
    c442,
    // Mint green
    c232,
    // Simpsons' living room pink
    c434,
};

typedef struct
{
    uint64_t waitTimeUs;
    uint64_t doorOpenElapsedTimeUs;
    uint16_t tumbleweedRotationOffset;
    int32_t tumbleweedMinY;
    int32_t tumbleweedLastY;
    uint8_t knockCount;

    paletteColor_t timeOfDayColor;
    paletteColor_t wallColor;

    struct
    {
        wsg_t grass;
        wsg_t package;
        wsg_t starburst;
        wsg_t tumbleweed;
    } wsg;

    font_t knockFont;

    midiFile_t packageGetSfx;
} ccmgDelivery_t;
ccmgDelivery_t* ccmgd = NULL;

static void ccmgDeliveryInitMicrogame()
{
    ccmgd = heap_caps_calloc(1, sizeof(ccmgDelivery_t), MALLOC_CAP_8BIT);

    ccmgd->waitTimeUs               = esp_random() % (MAXIMUM_WAIT_US - MINIMUM_WAIT_US) + MINIMUM_WAIT_US;
    ccmgd->tumbleweedRotationOffset = esp_random() % 360;
    ccmgd->tumbleweedMinY           = INT32_MAX;

    ccmgd->timeOfDayColor = timeOfDayColors[esp_random() % ARRAY_SIZE(timeOfDayColors)];
    ccmgd->wallColor      = wallColors[esp_random() % ARRAY_SIZE(wallColors)];

    loadWsg(CC_GRASS_WSG, &ccmgd->wsg.grass, false);
    loadWsg(CC_PACKAGE_WSG, &ccmgd->wsg.package, false);
    loadWsg(CC_PACKAGE_STARBURST_WSG, &ccmgd->wsg.starburst, false);
    loadWsg(CC_TUMBLEWEED_WSG, &ccmgd->wsg.tumbleweed, false);

    loadFont(RODIN_EB_FONT, &ccmgd->knockFont, false);

    loadMidiFile(CC_PACKAGE_GET_MID, &ccmgd->packageGetSfx, false);
}

static void ccmgDeliveryDestroyMicrogame()
{
    freeWsg(&ccmgd->wsg.grass);
    freeWsg(&ccmgd->wsg.package);
    freeWsg(&ccmgd->wsg.starburst);
    freeWsg(&ccmgd->wsg.tumbleweed);

    freeFont(&ccmgd->knockFont);

    unloadMidiFile(&ccmgd->packageGetSfx);

    heap_caps_free(ccmgd);
}

static void ccmgDeliveryMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                                 cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    // The tumbleweed needs to draw over this but behind the walls, so draw it first
    ccmgDeliveryDrawStar(122, 76);
    ccmgDeliveryDrawStar(161, 90);
    ccmgDeliveryDrawStar(139, 111);
    drawWsgTile(&ccmgd->wsg.grass, DOOR_X, FLOOR_TOP_Y - ccmgd->wsg.grass.h);

    switch (state)
    {
        case CC_MG_GET_READY:
            // Do nothing
            break;

        case CC_MG_PLAYING:
        {
            uint64_t totalElapsedTime = ccmgDelivery.timeoutUs - timeRemainingUs;
            for (uint8_t i = 0; i < buttonEvtCount; i++)
            {
                if (buttonEvts[i].button == PB_A && buttonEvts[i].down)
                {
                    bool successful = totalElapsedTime >= ccmgd->waitTimeUs
                                      && totalElapsedTime < ccmgd->waitTimeUs + BUTTON_PRESS_WINDOW_US;
                    cosCrunchMicrogameResult(successful);

                    if (successful)
                    {
                        globalMidiPlayerPlaySong(&ccmgd->packageGetSfx, MIDI_SFX);
                    }
                    break;
                }
            }
            break;
        }

        case CC_MG_CELEBRATING:
        {
            ccmgd->doorOpenElapsedTimeUs += elapsedUs;
            int32_t rotationDeg = (ccmgd->doorOpenElapsedTimeUs / STARBURST_ROT_US_PER_DEG) % 360;
            drawWsg(&ccmgd->wsg.starburst, (TFT_WIDTH - ccmgd->wsg.starburst.w) / 2,
                    (TFT_HEIGHT - ccmgd->wsg.starburst.h) / 2, false, false, rotationDeg);
            drawWsgSimple(&ccmgd->wsg.package, (TFT_WIDTH - ccmgd->wsg.package.w) / 2,
                          (TFT_HEIGHT - ccmgd->wsg.package.h) / 2);

            // It's not great to hard code the tempo, but we can't retrieve it until the MIDI file has played for
            // a few frames so eh
            globalMidiPlayerGet(MIDI_SFX)->tempo = 400000 / timeScale;
            break;
        }

        case CC_MG_DESPAIRING:
        {
            ccmgd->doorOpenElapsedTimeUs += elapsedUs;
            int32_t rotationDeg
                = (ccmgd->doorOpenElapsedTimeUs / TUMBLEWEED_ROT_US_PER_DEG + ccmgd->tumbleweedRotationOffset) % 360;
            int32_t x = ccmgd->doorOpenElapsedTimeUs / TUMBLEWEED_X_US_PER_PX;
            int32_t y = -ABS(getSin1024((rotationDeg * 2 + 90) % 360)) / (1024 / TUMBLEWEED_Y_RANGE);
            drawWsg(&ccmgd->wsg.tumbleweed, x + TUMBLEWEED_X_OFFSET, y + TUMBLEWEED_Y_OFFSET, false, false,
                    rotationDeg);

            if (ccmgd->tumbleweedLastY > y && y > ccmgd->tumbleweedMinY)
            {
                midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, SPLASH_CYMBAL, 0x7f);
                ccmgd->tumbleweedMinY = INT32_MAX;
            }
            else
            {
                ccmgd->tumbleweedMinY = MIN(ccmgd->tumbleweedMinY, y);
            }
            ccmgd->tumbleweedLastY = y;
            break;
        }
    }

    // Floor
    drawRectFilled(0, FLOOR_TOP_Y, TFT_WIDTH, FLOOR_BOTTOM_Y, FLOOR_COLOR);
    drawLineFast(0, FLOOR_TOP_Y, TFT_WIDTH, FLOOR_TOP_Y, OUTLINE_COLOR);
    for (int8_t i = -(FLOOR_PLANK_COUNT / 2); i < FLOOR_PLANK_COUNT / 2 + 1; i++)
    {
        drawLineFast(TFT_WIDTH / 2 + i * FLOOR_PLANK_WIDTH - 1, FLOOR_TOP_Y,
                     TFT_WIDTH / 2 + (i * 1.8) * FLOOR_PLANK_WIDTH - 1, FLOOR_BOTTOM_Y, OUTLINE_COLOR);
    }

    // Walls, drawn in 3 rects to leave the doorway open, plus doorway outline
    drawRectFilled(0, 0, TFT_WIDTH, DOOR_TOP_Y, ccmgd->wallColor);
    drawRectFilled(0, DOOR_TOP_Y, DOOR_X, FLOOR_TOP_Y, ccmgd->wallColor);
    drawRectFilled(DOOR_X + DOOR_WIDTH, DOOR_TOP_Y, TFT_WIDTH, FLOOR_TOP_Y, ccmgd->wallColor);
    drawRect(DOOR_X - 1, DOOR_TOP_Y - 1, DOOR_X + DOOR_WIDTH + 1, FLOOR_TOP_Y + 1, OUTLINE_COLOR);

    // Left window
    drawRectFilled(WINDOW_OFFSET_X, WINDOW_TOP_Y, WINDOW_OFFSET_X + WINDOW_WIDTH, WINDOW_TOP_Y + WINDOW_WIDTH,
                   ccmgd->timeOfDayColor);
    drawRect(WINDOW_OFFSET_X, WINDOW_TOP_Y, WINDOW_OFFSET_X + WINDOW_WIDTH, WINDOW_TOP_Y + WINDOW_WIDTH, OUTLINE_COLOR);
    drawLineFast(WINDOW_OFFSET_X + WINDOW_WIDTH / 2, WINDOW_TOP_Y, WINDOW_OFFSET_X + WINDOW_WIDTH / 2,
                 WINDOW_TOP_Y + WINDOW_WIDTH - 1, OUTLINE_COLOR);
    drawLineFast(WINDOW_OFFSET_X, WINDOW_TOP_Y + WINDOW_WIDTH / 2, WINDOW_OFFSET_X + WINDOW_WIDTH - 1,
                 WINDOW_TOP_Y + WINDOW_WIDTH / 2, OUTLINE_COLOR);
    ccmgDeliveryDrawStar(47, 59);

    // Right window
    drawRectFilled(TFT_WIDTH - (WINDOW_WIDTH + WINDOW_OFFSET_X), WINDOW_TOP_Y, TFT_WIDTH - WINDOW_OFFSET_X,
                   WINDOW_TOP_Y + WINDOW_WIDTH, ccmgd->timeOfDayColor);
    drawRect(TFT_WIDTH - WINDOW_WIDTH - WINDOW_OFFSET_X, WINDOW_TOP_Y, TFT_WIDTH - WINDOW_OFFSET_X,
             WINDOW_TOP_Y + WINDOW_WIDTH, OUTLINE_COLOR);
    drawLineFast(TFT_WIDTH - (WINDOW_OFFSET_X + WINDOW_WIDTH / 2) - 1, WINDOW_TOP_Y,
                 TFT_WIDTH - (WINDOW_OFFSET_X + WINDOW_WIDTH / 2) - 1, WINDOW_TOP_Y + WINDOW_WIDTH - 1, OUTLINE_COLOR);
    drawLineFast(TFT_WIDTH - (WINDOW_OFFSET_X + WINDOW_WIDTH), WINDOW_TOP_Y + WINDOW_WIDTH / 2,
                 TFT_WIDTH - WINDOW_OFFSET_X - 1, WINDOW_TOP_Y + WINDOW_WIDTH / 2, OUTLINE_COLOR);
    ccmgDeliveryDrawStar(234, 56);
    ccmgDeliveryDrawStar(218, 73);

    switch (state)
    {
        case CC_MG_GET_READY:
        case CC_MG_PLAYING:
        {
            // Door
            drawRectFilled(DOOR_X, DOOR_TOP_Y, DOOR_X + DOOR_WIDTH, FLOOR_TOP_Y, DOOR_COLOR);

            // Doorknob
            drawCircleFilled(DOOR_X + DOOR_WIDTH - DOORKNOB_X_INSET, FLOOR_TOP_Y - DOORKNOB_HEIGHT, DOORKNOB_RADIUS,
                             DOORKNOB_COLOR);
            drawCircle(DOOR_X + DOOR_WIDTH - DOORKNOB_X_INSET, FLOOR_TOP_Y - DOORKNOB_HEIGHT, DOORKNOB_RADIUS,
                       OUTLINE_COLOR);

            uint64_t totalElapsedTime = ccmgDelivery.timeoutUs - timeRemainingUs;
            if (totalElapsedTime > ccmgd->waitTimeUs && totalElapsedTime < ccmgd->waitTimeUs + KNOCK_DURATION_US)
            {
                drawText(&ccmgd->knockFont, c000, ccmgDeliveryKnock, 100, 80);
                if (ccmgd->knockCount == 0)
                {
                    midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, ELECTRIC_SNARE_OR_RIMSHOT, 0x7f);
                    ccmgd->knockCount++;
                }
            }
            if (totalElapsedTime > ccmgd->waitTimeUs + KNOCK_DELAY_US
                && totalElapsedTime < ccmgd->waitTimeUs + KNOCK_DURATION_US)
            {
                drawText(&ccmgd->knockFont, c000, ccmgDeliveryKnock, 120, 100);
                if (ccmgd->knockCount == 1)
                {
                    midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, ELECTRIC_SNARE_OR_RIMSHOT, 0x7f);
                    ccmgd->knockCount++;
                }
            }
            break;
        }

        case CC_MG_CELEBRATING:
        case CC_MG_DESPAIRING:
            // Door
            drawRectFilled(DOOR_X - DOOR_WIDTH - 1, DOOR_TOP_Y, DOOR_X - 1, FLOOR_TOP_Y, DOOR_COLOR);
            drawRect(DOOR_X - DOOR_WIDTH - 2, DOOR_TOP_Y - 1, DOOR_X, FLOOR_TOP_Y + 1, OUTLINE_COLOR);

            // Doorknob
            drawCircleFilled(DOOR_X - DOOR_WIDTH + DOORKNOB_X_INSET - 1, FLOOR_TOP_Y - DOORKNOB_HEIGHT, DOORKNOB_RADIUS,
                             DOORKNOB_COLOR);
            drawCircle(DOOR_X - DOOR_WIDTH + DOORKNOB_X_INSET - 1, FLOOR_TOP_Y - DOORKNOB_HEIGHT, DOORKNOB_RADIUS,
                       OUTLINE_COLOR);
            break;
    }
}

static void ccmgDeliveryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, ccmgd->timeOfDayColor);
}

static void ccmgDeliveryDrawStar(int16_t x, int16_t y)
{
    if (ccmgd->timeOfDayColor == NIGHTTIME_BG_COLOR)
    {
        drawLineFast(x - 1, y, x + 1, y, STAR_COLOR);
        drawLineFast(x, y - 2, x, y + 2, STAR_COLOR);
    }
}

#include "sonaTest.h"
#include "swadgesona.h"

#define OPTION_COUNT 14

const char sonaTestName[]          = "Swadgesona test";
static const char* const strings[] = {
    "Press A to save current swadgesona",
    "Press B to load swadgesona",
    "Press Up or down to change Swadgesona slot",
    "Press start to go back",
};
const int arrayLims[OPTION_COUNT] = {
    SKIN_COLOR_COUNT, HAIR_COLOR_COUNT, EYE_COLOR_COUNT, C_COUNT,  HA_COLOR_COUNT, GC_COUNT, BME_COUNT,
    EAE_COUNT,        EBE_COUNT,        EE_COUNT,        HE_COUNT, HAE_COUNT,      ME_COUNT, G_COUNT,
};

static void stEnterMode(void);
static void stExitMode(void);
static void stMainLoop(int64_t elapsedUs);
static void stCopyListToSona(swadgesona_t* swsn, int* list);

typedef struct
{
    int selection;
    bool update;
    swadgesona_t swsn;
    int list[OPTION_COUNT];
    bool menu;
    int16_t menuSlot;
} stData_t;

swadgeMode_t sonaTestMode = {
    .modeName    = sonaTestName,
    .fnEnterMode = stEnterMode,
    .fnExitMode  = stExitMode,
    .fnMainLoop  = stMainLoop,
};

stData_t* st;

static void stEnterMode(void)
{
    st = heap_caps_calloc(sizeof(stData_t), 1, MALLOC_CAP_8BIT);
    stCopyListToSona(&st->swsn, st->list);
}

static void stExitMode(void)
{
    heap_caps_free(st);
}

static void stMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    if (st->menu)
    {
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down)
            {
                if (evt.button & PB_A)
                {
                    saveSwadgesona(&st->swsn, st->menuSlot);
                }
                else if (evt.button & PB_B)
                {
                    loadSwadgesona(&st->swsn, st->menuSlot);
                }
                else if (evt.button & PB_UP)
                {
                    st->menuSlot++;
                    if (st->menuSlot > 9)
                    {
                        st->menuSlot = 0;
                    }
                }
                else if (evt.button & PB_DOWN)
                {
                    st->menuSlot--;
                    if (st->menuSlot < 0)
                    {
                        st->menuSlot = 9;
                    }
                }
                else if (evt.button & PB_START)
                {
                    st->menu = false;
                }
            }
        }
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
        int16_t x = 12;
        int16_t y = 24;
        drawTextWordWrap(getSysFont(), c550, strings[0], &x, &y, TFT_WIDTH - 12, TFT_HEIGHT);
        drawText(getSysFont(), c505, strings[1], 12, 48);
        x = 12;
        y = 64;
        drawTextWordWrap(getSysFont(), c055, strings[2], &x, &y, TFT_WIDTH - 12, TFT_HEIGHT);
        drawText(getSysFont(), c555, strings[3], 12, 88);
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "Current slot: %" PRId16, st->menuSlot);
        drawText(getSysFont(), c500, buffer, 12, 120);
        return;
    }
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            // Up/Down changes slot
            if (evt.button & PB_UP)
            {
                st->selection--;
                if (st->selection < 0)
                {
                    st->selection = OPTION_COUNT - 1;
                }
            }
            else if (evt.button & PB_DOWN)
            {
                st->selection++;
                if (st->selection >= OPTION_COUNT)
                {
                    st->selection = 0;
                }
            }
            // Left/Right changes within slot
            else if (evt.button & PB_LEFT)
            {
                st->list[st->selection]--;
                if (st->list[st->selection] < 0)
                {
                    st->list[st->selection] = arrayLims[st->selection] - 1;
                }
            }
            else if (evt.button & PB_RIGHT)
            {
                st->list[st->selection]++;
                if (st->list[st->selection] > arrayLims[st->selection] - 1)
                {
                    st->list[st->selection] = 0;
                }
            }
            // B randomizes
            else if (evt.button & PB_B)
            {
                generateRandomSwadgesona(&st->swsn);
            }
            // A sets update
            else if (evt.button & PB_A)
            {
                st->update = true;
            }
            // Start accesses save options
            else if (evt.button & PB_START)
            {
                st->menu = true;
            }
        }
    }
    if (st->update)
    {
        // Don't spam
        st->update = false;
        // Reload wsg
        stCopyListToSona(&st->swsn, st->list);
    }
    // Draw debug text
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawWsgSimpleScaled(&st->swsn.image, 32, 10, 3, 3);
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Selection: %" PRId16, st->selection);
    drawText(getSysFont(), c555, buffer, 32, TFT_HEIGHT - 32);
    snprintf(buffer, sizeof(buffer) - 1, "Index: %" PRId16, st->list[st->selection]);
    drawText(getSysFont(), c555, buffer, 32, TFT_HEIGHT - 16);
    drawText(getSysFont(), c550, st->swsn.name.nameBuffer, 16, 16);
}

static void stCopyListToSona(swadgesona_t* swsn, int* list)
{
    st->swsn.core.skin         = st->list[0];
    st->swsn.core.hairColor    = st->list[1];
    st->swsn.core.eyeColor     = st->list[2];
    st->swsn.core.clothes      = st->list[3];
    st->swsn.core.hatColor     = st->list[4];
    st->swsn.core.glassesColor = st->list[5];
    st->swsn.core.bodyMarks    = st->list[6];
    st->swsn.core.earShape     = st->list[7];
    st->swsn.core.eyebrows     = st->list[8];
    st->swsn.core.eyeShape     = st->list[9];
    st->swsn.core.hairStyle    = st->list[10];
    st->swsn.core.hat          = st->list[11];
    st->swsn.core.mouthShape   = st->list[12];
    st->swsn.core.glasses      = st->list[13];
    generateSwadgesonaImage(&st->swsn);
}
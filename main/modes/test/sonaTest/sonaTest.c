#include "sonaTest.h"
#include "swadgesona.h"

#define OPTION_COUNT 12

const char sonaTestName[]         = "Swadgesona test";
const int arrayLims[OPTION_COUNT] = {
    SKIN_COLOR_COUNT, HAIR_COLOR_COUNT, EYE_COLOR_COUNT, CLOTHES_COLOR_COUNT, HA_COLOR_COUNT, BME_COUNT, EAE_COUNT,
    EBE_COUNT,        EE_COUNT,         HE_COUNT,        HAE_COUNT,           ME_COUNT,
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
    st           = heap_caps_calloc(sizeof(stData_t), 1, MALLOC_CAP_8BIT);
    stCopyListToSona(&st->swsn, st->list);
}

static void stExitMode(void)
{
    free(st);
}

static void stMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
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
}

static void stCopyListToSona(swadgesona_t* swsn, int* list)
{
    st->swsn.core.skin       = st->list[0]; // Working
    st->swsn.core.hairColor  = st->list[1]; // Working
    st->swsn.core.eyeColor   = st->list[2]; // Working
    st->swsn.core.clothes    = st->list[3]; // Unimplemented
    st->swsn.core.hatColor   = st->list[4];
    st->swsn.core.bodyMarks  = st->list[5]; // Working
    st->swsn.core.earShape   = st->list[6]; // Failing
    st->swsn.core.eyebrows   = st->list[7];
    st->swsn.core.eyeShape   = st->list[8]; // Working
    st->swsn.core.hairStyle  = st->list[9];
    st->swsn.core.hat        = st->list[10];
    st->swsn.core.mouthShape = st->list[11];
    generateSwadgesonaImage(&st->swsn);
}
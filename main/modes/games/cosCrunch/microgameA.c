#include "microgameA.h"
#include "cosCrunch.h"

static void microgameAInitMicrogame(void);
static void microgameADestroyMicrogame(void);
static void microgameAMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);

static const char microgameAVerb[] = "Just Press \"A\"!";
cosCrunchMicrogame_t ccMicrogameA  = {
     .verb               = microgameAVerb,
     .timeoutSeconds     = 3,
     .fnInitMicrogame    = microgameAInitMicrogame,
     .fnDestroyMicrogame = microgameADestroyMicrogame,
     .fnMainLoop         = microgameAMainLoop,
};

typedef struct
{
    font_t font;
    cosCrunchMicrogameState previousState;
    uint64_t timeSinceTextMovedUs;
    uint16_t textX;
    uint16_t textY;
} microgameA_t;
microgameA_t* microgameA = NULL;

#define TEXT_MOVE_DELAY_US 600000

static void microgameAInitMicrogame(void)
{
    microgameA = heap_caps_calloc(1, sizeof(microgameA_t), MALLOC_CAP_8BIT);
    loadFont(IBM_VGA_8_FONT, &microgameA->font, false);
}

static void microgameADestroyMicrogame(void)
{
    freeFont(&microgameA->font);
    heap_caps_free(microgameA);
}

static void microgameAMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    if (state == CC_MG_PLAYING)
    {
        for (uint8_t i = 0; i < buttonEvtCount; i++)
        {
            if (buttonEvts[i].down)
            {
                if (buttonEvts[i].button == PB_A)
                {
                    cosCrunchMicrogameResult(true);
                }
                else
                {
                    cosCrunchMicrogameResult(false);
                }
            }
        }
    }

    char* msg = NULL;
    switch (state)
    {
        case CC_MG_GET_READY:
            msg = "Get ready...";
            break;

        case CC_MG_PLAYING:
            msg = "Press \"A\" now!";
            break;

        case CC_MG_CELEBRATING:
            msg = "Good job!";
            break;

        case CC_MG_DESPAIRING:
            msg = "lol @ u";
            break;
    }
    if (msg != NULL)
    {
        microgameA->timeSinceTextMovedUs += elapsedUs;
        if (microgameA->timeSinceTextMovedUs >= TEXT_MOVE_DELAY_US || microgameA->previousState != state)
        {
            microgameA->timeSinceTextMovedUs = 0;
            uint16_t tw                      = textWidth(&microgameA->font, msg);
            microgameA->textX                = esp_random() % (TFT_WIDTH - tw);
            microgameA->textY                = esp_random() % (TFT_HEIGHT - microgameA->font.height);
        }
        drawText(&microgameA->font, c555, msg, microgameA->textX, microgameA->textY);
    }

    microgameA->previousState = state;
}

#include "microgameB.h"
#include "cosCrunch.h"

static void microgameBInitMicrogame(void);
static void microgameBDestroyMicrogame(void);
static void microgameBMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);

static const char microgameBVerb[] = "It's \"B\" This Time";
cosCrunchMicrogame_t ccMicrogameB  = {
     .verb               = microgameBVerb,
     .timeoutSeconds     = 3,
     .fnInitMicrogame    = microgameBInitMicrogame,
     .fnDestroyMicrogame = microgameBDestroyMicrogame,
     .fnMainLoop         = microgameBMainLoop,
};

typedef struct
{
    font_t font;
    cosCrunchMicrogameState previousState;
    uint64_t timeSinceTextMovedUs;
    uint16_t textX;
    uint16_t textY;
} microgameB_t;
microgameB_t* microgameB = NULL;

#define TEXT_MOVE_DELAY_US 600000

static void microgameBInitMicrogame(void)
{
    microgameB = heap_caps_calloc(1, sizeof(microgameB_t), MALLOC_CAP_8BIT);
    loadFont(IBM_VGA_8_FONT, &microgameB->font, false);
}

static void microgameBDestroyMicrogame(void)
{
    freeFont(&microgameB->font);
    heap_caps_free(microgameB);
}

static void microgameBMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    if (state == CC_MG_PLAYING)
    {
        for (uint8_t i = 0; i < buttonEvtCount; i++)
        {
            if (buttonEvts[i].down)
            {
                if (buttonEvts[i].button == PB_B)
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
            msg = "It's \"B\" time";
            break;

        case CC_MG_CELEBRATING:
            msg = "BEEEEEEEES!";
            break;

        case CC_MG_DESPAIRING:
            msg = "Really, dude?";
            break;
    }
    if (msg != NULL)
    {
        microgameB->timeSinceTextMovedUs += elapsedUs;
        if (microgameB->timeSinceTextMovedUs >= TEXT_MOVE_DELAY_US || microgameB->previousState != state)
        {
            microgameB->timeSinceTextMovedUs = 0;
            uint16_t tw                      = textWidth(&microgameB->font, msg);
            microgameB->textX                = esp_random() % (TFT_WIDTH - tw);
            microgameB->textY                = esp_random() % (TFT_HEIGHT - microgameB->font.height);
        }
        drawText(&microgameB->font, c555, msg, microgameB->textX, microgameB->textY);
    }

    microgameB->previousState = state;
}

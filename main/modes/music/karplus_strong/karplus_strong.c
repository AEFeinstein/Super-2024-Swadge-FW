//==============================================================================
// Includes
//==============================================================================

#include "karplus_strong.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
} karplusStrong_t;

//==============================================================================
// Const data
//==============================================================================

static const char karplusStrongName[] = "Karplus Strong";

const trophyData_t ksTrophies[] = {

};

const trophySettings_t ksTrophySettings = {
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = karplusStrongName,
};

const trophyDataList_t ksTrophyData = {
    .settings = &ksTrophySettings,
    .list     = ksTrophies,
    .length   = ARRAY_SIZE(ksTrophies),
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void ksEnterMode(void);
static void ksExitMode(void);
static void ksMainLoop(int64_t elapsedUs);
static void ksBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ksDacCallback(uint8_t* samples, int16_t len);

//==============================================================================
// Variables
//==============================================================================

karplusStrong_t* ks = NULL;

swadgeMode_t karplusStrongMode = {
    .modeName                 = karplusStrongName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ksEnterMode,
    .fnExitMode               = ksExitMode,
    .fnMainLoop               = ksMainLoop,
    .fnBackgroundDrawCallback = ksBackgroundDrawCallback,
    .fnDacCb                  = ksDacCallback,
    .trophyData               = &ksTrophyData,
};

//==============================================================================
// Functions
//==============================================================================

static void ksEnterMode(void)
{
    ks = heap_caps_calloc(1, sizeof(karplusStrong_t), MALLOC_CAP_8BIT);
}

static void ksExitMode(void)
{
    heap_caps_free(ks);
}

static void ksMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
    }

    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        vec_t touchpad;
        getTouchCartesianSquircle(angle, radius, &touchpad.x, &wtouchpad.y);
    }
}

static void ksBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    paletteColor_t* tftFb = getPxTftFramebuffer();
    memset(&tftFb[y * TFT_WIDTH + x], c111, w * h);
}

static void ksDacCallback(uint8_t* samples, int16_t len)
{
    for (int16_t i = 0; i < len; i++)
    {
        // TODO KS synthesis
        samples[i] = 0;
    }
}

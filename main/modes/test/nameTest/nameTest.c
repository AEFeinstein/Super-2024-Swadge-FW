#include "nameTest.h"
#include "nameList.h"

static const char nameMode[] = "NameTest";

static void ntEnterMode(void);
static void ntExitMode(void);
static void ntMainLoop(int64_t elapsedUs);

typedef struct
{
    nameData_t nd;
} ntData_t;

swadgeMode_t nameTestMode = {
    .modeName          = nameMode,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = ntEnterMode,
    .fnExitMode        = ntExitMode,
    .fnMainLoop        = ntMainLoop,
};

static ntData_t* nt;

static void ntEnterMode(void)
{
    nt = heap_caps_calloc(sizeof(ntData_t), 1, MALLOC_CAP_8BIT);

    // Run all things for testing purposes
    generateMACUsername(&nt->nd);
    ESP_LOGI("NAME", "MAC-Based: %s", nt->nd.nameBuffer);
    // FIXME: 20 is hardcoded half of lists, all three lists are 40
    for (int idx = 0; idx < 20; idx++)
    {
        setUsernameFromIdxs(&nt->nd, idx, idx, idx, idx, true);
        ESP_LOGI("NAME", "User: %s", nt->nd.nameBuffer);
    }
    // FIXME: 40 is len of lists, all three lists are equal. Will break if changed
    for (int idx = 0; idx < 40; idx++)
    {
        setUsernameFromIdxs(&nt->nd, idx, idx, idx, idx, false);
        ESP_LOGI("NAME", "Sona: %s", nt->nd.nameBuffer);
    }
    generateRandUsername(&nt->nd, false);
    ESP_LOGI("NAME", "Rand1: %s", nt->nd.nameBuffer);
    generateRandUsername(&nt->nd, true);
    ESP_LOGI("NAME", "Rand2: %s", nt->nd.nameBuffer);
}

static void ntExitMode(void)
{
    free(nt);
}

static void ntMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        handleUsernamePickerInput(&evt, &nt->nd, false);
    }

    // Draw nt->string
    drawUsernamePicker(false);
}

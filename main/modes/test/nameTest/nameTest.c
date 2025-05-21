#include "nameTest.h"
#include "nameList.h"

static const char nameMode[] = "NameTest";

static void ntEnterMode(void);
static void ntExitMode(void);
static void ntMainLoop(int64_t elapsedUs);
static void ntTestWPrint(void);

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
    nt          = heap_caps_calloc(sizeof(ntData_t), 1, MALLOC_CAP_8BIT);
    nt->nd.user = false;

    // Run all things for testing purposes
    ntTestWPrint();
}

static void ntExitMode(void)
{
    heap_caps_free(nt);
}

static void ntMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    bool finished = false;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && evt.button & PB_B)
        {
            nt->nd.user = !nt->nd.user;
        }
        finished = handleUsernamePickerInput(&evt, &nt->nd);
    }

    if (finished)
    {
        // do more code
    }

    // Draw nt->string
    drawUsernamePicker(&nt->nd);
}

static void ntTestWPrint()
{
    nt->nd.user = true;
    generateMACUsername(&nt->nd);
    ESP_LOGI("NAME", "MAC-Based: %s", nt->nd.nameBuffer);

    // FIXME: 20 is hardcoded half of lists, all three lists are 40
    for (int idx = 0; idx < 20; idx++)
    {
        setUsernameFromIdxs(&nt->nd, idx, idx, idx, idx);
        ESP_LOGI("NAME", "User: %s", nt->nd.nameBuffer);
    }

    // FIXME: 40 is len of lists, all three lists are equal. Will break if changed
    nt->nd.user = false;
    for (int idx = 0; idx < 40; idx++)
    {
        setUsernameFromIdxs(&nt->nd, idx, idx, idx, idx);
        ESP_LOGI("NAME", "Sona: %s", nt->nd.nameBuffer);
    }

    generateRandUsername(&nt->nd);
    ESP_LOGI("NAME", "Rand1: %s", nt->nd.nameBuffer);
    nt->nd.user = true;
    generateRandUsername(&nt->nd);
    ESP_LOGI("NAME", "Rand2: %s", nt->nd.nameBuffer);

    // Set the longest name for testing
    nt->nd.user = false;
    setUsernameFromIdxs(&nt->nd, 26, 13, 24, 255);
}

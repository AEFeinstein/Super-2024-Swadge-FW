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
    nameData_t sys;
    bool user;
} ntData_t;

swadgeMode_t nameTestMode = {
    .modeName          = nameMode,
    .wifiMode          = ESP_NOW,
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
    
    nameData_t* temp = getSystemUsername();
    nt->sys          = *temp;

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
            nt->user = !nt->user;
        }
        if (nt->user)
        {
            finished = handleUsernamePickerInput(&evt, &nt->sys);
        }
        else
        {
            finished = handleUsernamePickerInput(&evt, &nt->nd);
        }
    }

    drawUsernamePicker((nt->user) ? &nt->sys : &nt->nd);

    if (finished)
    {
        if (nt->user)
        {
            // Save
            setSystemUsername(&nt->sys);
        }
    }

    // Draw current username
    drawText(getSysFont(), c555, getSystemUsername()->nameBuffer, 32, 8);
}

static void ntTestWPrint()
{
    nt->nd.user = true;
    generateMACUsername(&nt->nd);
    ESP_LOGI("NAME", "MAC-Based: %s", nt->nd.nameBuffer);

    for (int idx = 0; idx < 12; idx++)
    {
        setUsernameFromIdxs(&nt->nd, idx, idx, idx, idx);
        ESP_LOGI("NAME", "User: %s", nt->nd.nameBuffer);
    }

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
    setUsernameFromIdxs(&nt->nd, 0, 0, 0, 0);
}

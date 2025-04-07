#include "roboRunner.h"

#define JUMP_HEIGHT          -12
#define Y_ACCEL              1
#define GROUND_HEIGHT        184
#define PLAYER_GROUND_OFFSET (GROUND_HEIGHT - 56)
#define PLAYER_X             32

const char runnerModeName[] = "Robo Runner";

typedef struct
{
    // Assets
    wsg_t character;

    // Robot
    bool onGround;
    int ySpeed;
    int yPos;
} runnerData_t;

static void runnerEnterMode(void);
static void runnerExitMode(void);
static void runnerMainLoop(int64_t elapsedUs);

swadgeMode_t roboRunnerMode = {
    .modeName                 = runnerModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = runnerEnterMode,
    .fnExitMode               = runnerExitMode,
    .fnMainLoop               = runnerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

runnerData_t* rd;

static void runnerEnterMode()
{
    rd = (runnerData_t*)heap_caps_calloc(1, sizeof(runnerData_t), MALLOC_CAP_8BIT);
    loadWsg("RoboStanding.wsg", &rd->character, true);
}

static void runnerExitMode()
{
    freeWsg(&rd->character);
    free(rd);
}

static void runnerMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.button & PB_A || evt.button & PB_UP) && rd->onGround)
            {
                rd->ySpeed   = JUMP_HEIGHT;
                rd->onGround = false;
            }
        }
    }
    rd->yPos += rd->ySpeed;
    rd->ySpeed += Y_ACCEL;
    if (rd->yPos > PLAYER_GROUND_OFFSET)
    {
        rd->onGround = true;
        rd->yPos     = PLAYER_GROUND_OFFSET;
        rd->ySpeed   = 0;
    }
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    drawLine(0, GROUND_HEIGHT, TFT_WIDTH, GROUND_HEIGHT, c555, 0);
    drawWsgSimple(&rd->character, PLAYER_X, rd->yPos);
}
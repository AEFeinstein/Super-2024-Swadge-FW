//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <esp_timer.h>
#include <esp_log.h>

#include "modeTimer.h"
#include "swadge2024.h"
#include "tinyphysicsengine.h"
#include "quaternions.h"
#include "matrixMath.h"
#include "spiffs_model.h"
#include "model.h"
#include "macros.h"
#include "hdw-imu.h"
#include "hdw-btn.h"
#include "font.h"
#include "wsg.h"

//==============================================================================
// Defines
//==============================================================================

#define HOURGLASS_FRAMES 12
#define PAUSE_FLASH_SPEED 500000
#define EXPIRE_FLASH_SPEED 250000

#define MAX_DICE 12

#define CUBE_SCALE .0625

// The scale of the physics world as compared to the screen
#define WORLD_SCALE 4

#define DIE_SIZE (16 * WORLD_SCALE * TPE_F)
#define DIE_RADIUS (TPE_F / 4)
#define DIE_MASS (50 * TPE_F)

// Dimensions of the 'room' the dice are in
#define ROOM_W (280 * WORLD_SCALE * TPE_F)
#define ROOM_H (240 * WORLD_SCALE * TPE_F)
#define ROOM_D (100 * WORLD_SCALE * TPE_F)

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    STOPPED = 0,
    PAUSED,
    RUNNING,
    EXPIRED,
} timerState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief Current timer state
    timerState_t timerState;

    /// @brief If true, counts up (stopwatch), otherwise counts down (timer)
    bool stopwatch;

    /// @brief The amount of time being counted down from
    int64_t countdownTime;

    /// @brief Any time that was accumulated before the timer was resumed
    int64_t accumulatedDuration;

    /// @brief The actual time the timer was started
    int64_t startTime;
} boardTimer_t;

typedef struct
{
    const model_t* model;
    uint8_t sides;

} dieDef_t;

typedef struct
{
    font_t textFont;
    font_t numberFont;
    wsg_t hourglassFrames[HOURGLASS_FRAMES];

    /////////////
    // FPS Timer
    /////////////

    int64_t avgFrameTime;

    ///////////////////
    // Physics Objects
    ///////////////////

    TPE_Body bodies[MAX_DICE];
    // TODO this is only enough for 6-sided dice
    TPE_Joint joints[MAX_DICE * 8];
    TPE_Connection conns[MAX_DICE * 16];
    TPE_World world;

    uint8_t dieCount;

    ///////////////
    // Timer Stuff
    ///////////////

    // Holds info for actual timer state
    boardTimer_t timer;

    /////////////////////
    // Rendering Objects
    /////////////////////
    scene_t scene;

    // wireframe model of the box the dice are contained in
    model_t envModel;

    // model of a coin
    model_t coinModel;

    // model of a 4-sided die
    model_t d4model;

    // model of 6-sided die
    model_t d6model;

    // model of a 20-sided die
    model_t d20model;

    // model of a 100-sided die

    model_t d100model;
} timerMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void timerEnterMode(void);
static void timerExitMode(void);
static void timerMainLoop(int64_t elapsedUs);
static void timerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

// Physics engine callbacks
TPE_Vec3 environmentDistance(TPE_Vec3 point, TPE_Unit maxdistance);

// Physics <-> rendering coordinate conversion functions
TPE_Vec3 worldToTpeCoord(const float* vf);
void tpeToWorldCoord(float* vf, TPE_Vec3 vec);

static uint8_t diceTpeCollisionCallback(uint16_t body1, uint16_t joint1, uint16_t body2, uint16_t joint2, TPE_Vec3 vec);

static void runTimer(int64_t time, boardTimer_t* timer);
static void drawTimer(int64_t time, const boardTimer_t* timer);

static void setupScene(void);
static void stepPhysics(void);
static void drawDice(void);

//==============================================================================
// Strings
//==============================================================================

static const char timerName[] = "Timer";
static const char countdownStr[] = "Timer";
static const char stopwatchStr[] = "Stopwatch";
static const char hourglassFrameFmt[] = "hourglass_%02" PRIu8 ".wsg";
static const char minutesSecondsFmt[] = "%" PRIu8 ":%02" PRIu8 ".%03" PRIu16;
static const char hoursMinutesSecondsFmt[] = "%" PRIu64 ":%02" PRIu8 ":%02" PRIu8 ".%03" PRIu16;

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t timerMode = {
    .modeName                 = timerName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = timerEnterMode,
    .fnExitMode               = timerExitMode,
    .fnMainLoop               = timerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = timerBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static timerMode_t* timerData = NULL;

//==============================================================================
// Functions
//==============================================================================

static void timerEnterMode(void)
{
    timerData = calloc(1, sizeof(timerMode_t));

    for (uint8_t i = 0; i < HOURGLASS_FRAMES; i++)
    {
        char wsgName[18];
        snprintf(wsgName, sizeof(wsgName), hourglassFrameFmt, i);
        loadWsg(wsgName, &timerData->hourglassFrames[i], false);
    }

    loadFont("ibm_vga8.font", &timerData->textFont, false);
    loadFont("seven_segment.font", &timerData->numberFont, false);

    // Load models
    loadModel("wirebox.mdl", &timerData->envModel, false);
    loadModel("cube.mdl", &timerData->d6model, true);
    //loadModel("donut.mdl", &timerData->d6model, true);


    // 30FPS as recommended by physics
    setFrameRateUs(1000000 / 30);

    setupScene();

    // Default to 30s
    timerData->timer.countdownTime = 30 * 1000000;
    timerData->timer.timerState = STOPPED;
}

static void timerExitMode(void)
{
    for (uint8_t i = 0; i < HOURGLASS_FRAMES; i++)
    {
        freeWsg(&timerData->hourglassFrames[i]);
    }

    freeFont(&timerData->textFont);
    freeFont(&timerData->numberFont);

    deinitRenderer();

    freeModel(&timerData->d6model);
    freeModel(&timerData->envModel);

    free(timerData);
    timerData = NULL;
}

static void timerMainLoop(int64_t elapsedUs)
{
    // Check for countdown timer expiration
    int64_t now = esp_timer_get_time();
    runTimer(now, &timerData->timer);

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && evt.button == PB_A)
        {
            // A pressed, pause/unpause/start
            switch (timerData->timer.timerState)
            {
                case STOPPED:
                    timerData->timer.accumulatedDuration = 0;
                    // Intentional fall-through
                case PAUSED:
                {
                    timerData->timer.startTime = now;
                    timerData->timer.timerState = RUNNING;
                    break;
                }

                case RUNNING:
                {
                    timerData->timer.accumulatedDuration += (now - timerData->timer.startTime);
                    timerData->timer.timerState = PAUSED;
                    break;
                }

                case EXPIRED:
                {
                    timerData->timer.timerState = STOPPED;
                    break;
                }
            }
        }
        else if (evt.down && evt.button == PB_B)
        {
            // B pressed, pause/stop/reset
            // TODO
            switch (timerData->timer.timerState)
            {
                case PAUSED:
                {
                    timerData->timer.timerState = STOPPED;
                    timerData->timer.accumulatedDuration = 0;
                    break;
                }

                case RUNNING:
                {
                    timerData->timer.accumulatedDuration += (now - timerData->timer.startTime);
                    timerData->timer.timerState = PAUSED;
                    break;
                }

                case STOPPED:
                case EXPIRED:
                break;
            }
        }
        else if (evt.down && (evt.button == PB_LEFT || evt.button == PB_RIGHT))
        {
            if (timerData->timer.timerState == STOPPED)
            {
                timerData->timer.stopwatch = !timerData->timer.stopwatch;
            }
        }
        else if (evt.down && (evt.button == PB_UP || evt.button == PB_DOWN))
        {
            if (timerData->timer.timerState == STOPPED)
            {
                if (!timerData->timer.stopwatch)
                {
                    timerData->timer.countdownTime += (evt.button == PB_UP) ? 30000000 : -30000000;
                }
            }
        }
    }

    if (timerData->timer.stopwatch)
    {
        drawText(&timerData->textFont, c050, stopwatchStr, 40, 5);
    }
    else
    {
        drawText(&timerData->textFont, c050, countdownStr, 40, 5);
    }

    drawTimer(now, &timerData->timer);

    // Update physics
    stepPhysics();

    // And render the dice
    drawDice();

    // Render FPS Counter
    timerData->avgFrameTime = (timerData->avgFrameTime * 9 / 10) + (elapsedUs * 1 / 10);

    int32_t framesPerSecond = (1000000 / (timerData->avgFrameTime + 1));
    int32_t tenthFramesPerSecond = (10000000 / (timerData->avgFrameTime + 1)) % 10;

    char fpsBuffer[32];
    sprintf(fpsBuffer, "%" PRId32 ".%" PRId32, framesPerSecond, tenthFramesPerSecond);

    drawText(&timerData->textFont, c550, fpsBuffer, TFT_WIDTH - 30 - textWidth(&timerData->textFont, fpsBuffer), 5);
}

static void timerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    //stepPhysics();
    accelIntegrate();
    fillDisplayArea(x, y, x + w, y + h, c000);
}

TPE_Vec3 environmentDistance(TPE_Vec3 point, TPE_Unit maxDistance)
{
  // our environemnt: just a simple room
  return TPE_envAABoxInside(point, TPE_vec3(0, 0, 0),
                            TPE_vec3(ROOM_W ,ROOM_H, ROOM_D));
}

TPE_Unit worldToTpeQuantity(float value)
{
    // TODO conversion factors???
    return (TPE_Unit)(value / TPE_F);
}

/**
 * @brief Convert world-space coordinates to TPE coordinates
 *
 * @param x
 * @param y
 * @param z
 * @return TPE_Vec3
 */
TPE_Vec3 worldToTpeCoord(const float* vf)
{
    return TPE_vec3((TPE_Unit)(vf[0] * TPE_F * WORLD_SCALE), (TPE_Unit)(vf[1] * TPE_F * WORLD_SCALE), (TPE_Unit)(vf[2] * TPE_F * WORLD_SCALE));
}

/**
 * @brief Convert TPE coordinates to world-space coordinates
 *
 * @param x
 * @param y
 * @param z
 * @param vec
 */
void tpeToWorldCoord(float* vf, TPE_Vec3 vec)
{
    //#define ROOM_W (240 * 10 * TPE_F)
    vf[0] = 1.0 * vec.x / WORLD_SCALE / TPE_F;
    vf[1] = 1.0 * vec.y / WORLD_SCALE / TPE_F;
    vf[2] = 1.0 * vec.z / WORLD_SCALE / TPE_F;
}

static uint8_t diceTpeCollisionCallback(uint16_t body1, uint16_t joint1, uint16_t body2, uint16_t joint2, TPE_Vec3 vec)
{
    //ESP_LOGI("Timer", "Collision B[%" PRIu16 "][%" PRIu16 "] <-> B[%" PRIu16 "][%" PRIu16 "] @ (%" PRId16 ", %" PRId16 ", %" PRId16 ")", body1, joint1, body2, joint2, (int16_t)vec.x / TPE_F, (int16_t)vec.y / TPE_F, (int16_t)vec.z / TPE_F);
    int minV = 0;
    int maxV = 0;
    char min = '?';
    char max = '?';

    if (vec.x < minV)
    {
        minV = vec.x;
        min = 'x';
    }
    else if (vec.x > maxV)
    {
        maxV = vec.x;
        max = 'x';
    }
    if (vec.y < minV)
    {
        minV = vec.y;
        min = 'y';
    }
    else if (vec.y > maxV)
    {
        maxV = vec.y;
        max = 'y';
    }
    if (vec.z < minV)
    {
        minV = vec.z;
        min = 'z';
    }
    else if (vec.z > maxV)
    {
        maxV = vec.z;
        max = 'z';
    }

    char dir = '+';
    if (ABS(min) > ABS(max))
    {
        // Negative axis was touched
        dir = '-';
    }

    ESP_LOGI("Timer", "Collision at %c%c", dir, (dir == '+' ? max : min));

    return 1;
}

static void runTimer(int64_t time, boardTimer_t* timer)
{
    if (timer->timerState == RUNNING
        && (!timer->stopwatch && timer->accumulatedDuration + (time - timer->startTime) >= timer->countdownTime))
    {
        timer->accumulatedDuration = timer->countdownTime;
        timer->timerState = EXPIRED;
    }
}

static void drawTimer(int64_t time, const boardTimer_t* timer)
{
    int64_t remaining = timer->stopwatch
                        ? timer->accumulatedDuration
                        : timer->countdownTime - timer->accumulatedDuration;

    if (timer->timerState == RUNNING)
    {
        remaining += (timer->stopwatch ? 1 : -1) * (time - timer->startTime);
    }

    uint16_t remainingMillis = (remaining / 1000) % 1000;
    uint8_t remainingSecs = (remaining / 1000000) % 60;
    uint8_t remainingMins = (remaining / (60 * 1000000)) % 60;
    // Might as well...
    uint64_t remainingHrs = remaining / 3600000000;

    // Draw / blink the timer time
    bool blink = (timer->timerState == PAUSED && 0 == (time / PAUSE_FLASH_SPEED) % 2)
                 || (timer->timerState == EXPIRED && 0 == (time / EXPIRE_FLASH_SPEED) % 2);

    if (!blink)
    {
        // Write the timer text
        char buffer[64];
        if (remainingHrs > 0)
        {
            snprintf(buffer, sizeof(buffer), hoursMinutesSecondsFmt,
                    remainingHrs, remainingMins, remainingSecs, remainingMillis);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), minutesSecondsFmt,
                    remainingMins, remainingSecs, remainingMillis);
        }
        uint16_t textX = TFT_WIDTH - 20 - textWidth(&timerData->numberFont, buffer);
        uint16_t textY = (TFT_HEIGHT - timerData->numberFont.height) / 4;
        drawText(&timerData->numberFont, c050, buffer, textX, textY);
    }
}

static void setupScene(void)
{
    timerData->dieCount = 1;

    float translate[3] = {0};
    float rotate[4] = {0};
    rotate[0] = 1.0;
    float scale[3] = {CUBE_SCALE, CUBE_SCALE, CUBE_SCALE};

    // TODO: Different dice shapes, obviously
    for (int i = 0; i < timerData->dieCount; i++)
    {
        // Setup physics
        TPE_makeBox(timerData->joints + 8 * i, timerData->conns + 16 * i, DIE_SIZE, DIE_SIZE, DIE_SIZE, DIE_RADIUS);
        TPE_bodyInit(&timerData->bodies[i], timerData->joints + i * 8, 8, timerData->conns, 16, DIE_MASS);
        TPE_bodyMoveTo(&timerData->bodies[i], TPE_vec3(0, i * (DIE_SIZE + TPE_F), 0));

        // Setup renderer model info
        timerData->scene.objects[i].model = &timerData->d6model;
        createTransformMatrix(timerData->scene.objects[i].transform, translate, rotate, scale);
    }

    // We'll have one more model, for the box the dice are in
    timerData->scene.objectCount = timerData->dieCount + 1;
    timerData->scene.objects[timerData->dieCount].model = &timerData->envModel;
    scale[0] = (280.0 / 255.0);
    scale[1] = (240.0 / 255.0);
    scale[2] = 1.0;
    createTransformMatrix(timerData->scene.objects[timerData->dieCount].transform, translate, rotate, scale);
    identityMatrix(timerData->scene.transform);


    // Init physics engine world
    TPE_worldInit(&timerData->world, timerData->bodies, timerData->dieCount, environmentDistance);
    timerData->world.collisionCallback = diceTpeCollisionCallback;

    //TPE_bodyAccelerate(&timerData->world.bodies[0], TPE_vec3(-1 * TPE_F / 8, TPE_F / 3, 0));
    //TPE_bodySpin(&timerData->world.bodies[0], TPE_vec3(0, 1 * TPE_F / 10, -1 * TPE_F / 10));
    //TPE_bodySpin(&timerData->world.bodies[1], TPE_vec3(1 * TPE_F / 10, 0, 1 * TPE_F / 10));

    // Init renderer
    initRendererScene(&timerData->scene);
}

static void stepPhysics(void)
{
    // Get accel vectors
    int16_t orientX, orientY, orientZ;
    int16_t accelX, accelY, accelZ;
    accelGetOrientVec(&orientX, &orientY, &orientZ);
    accelGetAccelVecRaw(&accelX, &accelY, &accelZ);

    // simulate next tick
    TPE_worldStep(&timerData->world);

    // Apply them to all bodies
    for (int j = 0; j < timerData->dieCount; j++)
    {
        #define ACCEL_MULT (TPE_F / 512)
        TPE_Vec3 accelForce = TPE_vec3(-accelX * ACCEL_MULT, -accelY * ACCEL_MULT, accelZ * ACCEL_MULT);
        ESP_LOGI("Timer", "Applying accel force (%" PRId16 ", %" PRId16 ", %" PRId16, (int16_t)accelForce.x / TPE_F, (int16_t)accelForce.y / TPE_F, (int16_t)accelForce.z / TPE_F);
        TPE_bodyAccelerate(&timerData->world.bodies[j], accelForce);
        //TPE_bodyApplyGravity(&timerData->world.bodies[j], TPE_F / 100);
    }
}

static void drawDice(void)
{
    for (uint32_t i = 0; i < timerData->dieCount; i++)
    {
        TPE_Body* body = &timerData->world.bodies[i];
        TPE_Vec3 pos = TPE_bodyGetCenterOfMass(body);
        TPE_Vec3 vel = TPE_bodyGetLinearVelocity(body);
        // joints in bodyGetRotation are:
        // - Forward: (1 -> 2)
        // - Right: (1 -> 3)
        // so this is fine, assuming a cube
        TPE_Vec3 rot = TPE_bodyGetRotation(body, 0, 1, 2);

        float eulerRot[3] = {
            rot.x * 2 * 3.1415926535 / TPE_F,
            rot.y * 2 * 3.1415926535 / TPE_F,
            rot.z * 2 * 3.1415926535 / TPE_F,
        };

        // Apply the physics rotation to the 3D model
        //mathEulerToQuat(timerData->scene.objects[i].orient, eulerRot);
        float translate[3];
        float rotate[4];
        float scale[3] = {CUBE_SCALE, CUBE_SCALE, CUBE_SCALE};
        tpeToWorldCoord(translate, pos);
        mathEulerToQuat(rotate, eulerRot);
        createTransformMatrix(timerData->scene.objects[i].transform, translate, rotate, scale);

        ESP_LOGI("Timer", "Object [%" PRIu32 "] Position is %" PRId16 ", %" PRId16 ", %" PRId16, i,
                 (int16_t)pos.x / TPE_F, (int16_t)pos.y / TPE_F, (int16_t)pos.z / TPE_F);
        ESP_LOGI("Timer", "Object [%" PRIu32 "] Velocity is %" PRId16 ", %" PRId16 ", %" PRId16, i,
                 (int16_t)vel.x / TPE_F, (int16_t)vel.y / TPE_F, (int16_t)vel.z / TPE_F);

        drawScene(&timerData->scene, 0, 0, TFT_WIDTH, TFT_HEIGHT);
    }
}
//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <esp_timer.h>
#include <esp_log.h>

#include "modeDice.h"
#include "swadge2024.h"
#include "tinyphysicsengine.h"
#include "small3dlib.h"
#include "quaternions.h"
#include "matrixMath.h"
#include "spiffs_model.h"
#include "model.h"
#include "macros.h"
#include "hdw-imu.h"
#include "hdw-btn.h"
#include "hdw-tft.h"
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
// Structs
//==============================================================================

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

    /////////////////////
    // Rendering Objects
    /////////////////////
    // raw data for the D6 mode
    object3dInfo_t d6objInfo;

    S3L_Scene scene;

    // wireframe model of the box the dice are contained in

    S3L_Model3D envModel;

    // model of a coin
    S3L_Model3D coinModel;

    // model of a 4-sided die
    S3L_Model3D d4model;

    // model of 6-sided die
    S3L_Model3D d6model;

    // model of a 20-sided die
    S3L_Model3D d20model;

    // model of a 100-sided die
    S3L_Model3D d100model;
} diceMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void diceEnterMode(void);
static void diceExitMode(void);
static void diceMainLoop(int64_t elapsedUs);
static void diceBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

// Physics engine callbacks
TPE_Vec3 environmentDistance(TPE_Vec3 point, TPE_Unit maxdistance);

// Physics <-> rendering coordinate conversion functions
TPE_Vec3 worldToTpeCoord(const float* vf);
void tpeToWorldCoord(float* vf, TPE_Vec3 vec);

static uint8_t diceTpeCollisionCallback(uint16_t body1, uint16_t joint1, uint16_t body2, uint16_t joint2, TPE_Vec3 vec);
static void dicePixelCallback(S3L_PixelInfo* pixelInfo);

static void setupScene(void);
static void stepPhysics(void);
static void drawDice(void);

//==============================================================================
// Strings
//==============================================================================

static const char diceName[] = "Dice Roller";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t diceMode = {
    .modeName                 = diceName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = diceEnterMode,
    .fnExitMode               = diceExitMode,
    .fnMainLoop               = diceMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = diceBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static diceMode_t* diceData = NULL;

//==============================================================================
// Functions
//==============================================================================

static void diceEnterMode(void)
{
    diceData = calloc(1, sizeof(diceMode_t));

    loadFont("ibm_vga8.font", &diceData->textFont, false);
    loadFont("seven_segment.font", &diceData->numberFont, false);

    // Load models
    //loadModelS3d("wirebox.mdl", &diceData->envModel, false);
    //loadModelS3d("cube.mdl", &diceData->d6model, true);
    //loadModel("donut.mdl", &diceData->d6model, true);

    // Load the raw object info
    loadObjInfo("donut.mdl", &diceData->d6objInfo, true);

    // 30FPS as recommended by physics
    setFrameRateUs(1000000 / 30);

    setupScene();
}

static void diceExitMode(void)
{
    for (uint8_t i = 0; i < HOURGLASS_FRAMES; i++)
    {
        freeWsg(&diceData->hourglassFrames[i]);
    }

    freeFont(&diceData->textFont);
    freeFont(&diceData->numberFont);

    //deinitRenderer();

    //freeModel(&diceData->d6model);
    //freeModel(&diceData->envModel);

    freeObjInfo(&diceData->d6objInfo);

    free(diceData);
    diceData = NULL;
}

static void diceMainLoop(int64_t elapsedUs)
{
    // Check for countdown dice expiration
    int64_t now = esp_timer_get_time();

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Do we have any buttons?
    }

    // Update physics
    stepPhysics();

    // And render the dice
    drawDice();

    // Render FPS Counter
    diceData->avgFrameTime = (diceData->avgFrameTime * 9 / 10) + (elapsedUs * 1 / 10);

    int32_t framesPerSecond = (1000000 / (diceData->avgFrameTime + 1));
    int32_t tenthFramesPerSecond = (10000000 / (diceData->avgFrameTime + 1)) % 10;

    char fpsBuffer[32];
    sprintf(fpsBuffer, "%" PRId32 ".%" PRId32, framesPerSecond, tenthFramesPerSecond);

    drawText(&diceData->textFont, c550, fpsBuffer, TFT_WIDTH - 30 - textWidth(&diceData->textFont, fpsBuffer), 5);
}

static void diceBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
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
    //ESP_LOGI("Dice", "Collision B[%" PRIu16 "][%" PRIu16 "] <-> B[%" PRIu16 "][%" PRIu16 "] @ (%" PRId16 ", %" PRId16 ", %" PRId16 ")", body1, joint1, body2, joint2, (int16_t)vec.x / TPE_F, (int16_t)vec.y / TPE_F, (int16_t)vec.z / TPE_F);
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

    ESP_LOGI("Dice", "Collision at %c%c", dir, (dir == '+' ? max : min));

    return 1;
}

static void dicePixelCallback(S3L_PixelInfo* pixelInfo)
{
    paletteColor_t col = c555;
    setPxTft(pixelInfo->x, pixelInfo->y, col);
}

static void setupScene(void)
{
    diceData->dieCount = 1;

    //float translate[3] = {0};
    //float rotate[4] = {0};
    //rotate[0] = 1.0;
    //float scale[3] = {CUBE_SCALE, CUBE_SCALE, CUBE_SCALE};

    configureS3dCallback(TFT_WIDTH, TFT_HEIGHT, dicePixelCallback);

    S3L_model3DInit(
        diceData->d6objInfo.verts,
        diceData->d6objInfo.vertCount,
        diceData->d6objInfo.tris,
        diceData->d6objInfo.triCount,
        &diceData->d6model
    );

    S3L_sceneInit(&diceData->d6model, 1, &diceData->scene);

    // TODO: Different dice shapes, obviously
    for (int i = 0; i < diceData->dieCount; i++)
    {
        // Setup physics
        TPE_makeBox(diceData->joints + 8 * i, diceData->conns + 16 * i, DIE_SIZE, DIE_SIZE, DIE_SIZE, DIE_RADIUS);
        TPE_bodyInit(&diceData->bodies[i], diceData->joints + i * 8, 8, diceData->conns, 16, DIE_MASS);
        TPE_bodyMoveTo(&diceData->bodies[i], TPE_vec3(0, i * (DIE_SIZE + TPE_F), 0));

        // Setup renderer model info
        //diceData->scene.objects[i].model = &diceData->d6model;
        //createTransformMatrix(diceData->scene.objects[i].transform, translate, rotate, scale);
    }

    // We'll have one more model, for the box the dice are in
    //diceData->scene.objectCount = diceData->dieCount + 1;
    //diceData->scene.objects[diceData->dieCount].model = &diceData->envModel;
    //scale[0] = (280.0 / 255.0);
    //scale[1] = (240.0 / 255.0);
    //scale[2] = 1.0;
    //createTransformMatrix(diceData->scene.objects[diceData->dieCount].transform, translate, rotate, scale);
    //identityMatrix(diceData->scene.transform);


    // Init physics engine world
    TPE_worldInit(&diceData->world, diceData->bodies, diceData->dieCount, environmentDistance);
    diceData->world.collisionCallback = diceTpeCollisionCallback;

    //TPE_bodyAccelerate(&diceData->world.bodies[0], TPE_vec3(-1 * TPE_F / 8, TPE_F / 3, 0));
    //TPE_bodySpin(&diceData->world.bodies[0], TPE_vec3(0, 1 * TPE_F / 10, -1 * TPE_F / 10));
    //TPE_bodySpin(&diceData->world.bodies[1], TPE_vec3(1 * TPE_F / 10, 0, 1 * TPE_F / 10));

    // Init renderer
    //initRendererScene(&diceData->scene);
}

static void stepPhysics(void)
{
    // Get accel vectors
    int16_t orientX, orientY, orientZ;
    int16_t accelX, accelY, accelZ;
    accelGetOrientVec(&orientX, &orientY, &orientZ);
    accelGetAccelVecRaw(&accelX, &accelY, &accelZ);

    // simulate next tick
    TPE_worldStep(&diceData->world);

    // Apply them to all bodies
    for (int j = 0; j < diceData->dieCount; j++)
    {
        #define ACCEL_MULT (TPE_F / 512)
        TPE_Vec3 accelForce = TPE_vec3(-accelX * ACCEL_MULT, -accelY * ACCEL_MULT, accelZ * ACCEL_MULT);
        ESP_LOGI("Dice", "Applying accel force (%" PRId16 ", %" PRId16 ", %" PRId16, (int16_t)accelForce.x / TPE_F, (int16_t)accelForce.y / TPE_F, (int16_t)accelForce.z / TPE_F);
        TPE_bodyAccelerate(&diceData->world.bodies[j], accelForce);
        //TPE_bodyApplyGravity(&diceData->world.bodies[j], TPE_F / 100);
    }
}

static void drawDice(void)
{
    float orientQuat[4];
    accelGetQuaternion(orientQuat);

    diceData->scene.camera.transform.rotation.x = (int)(orientQuat[0] * (S3L_F / 2) / 3.1415626535);
    diceData->scene.camera.transform.rotation.y = (int)(orientQuat[1] * (S3L_F / 2) / 3.1415626535);
    diceData->scene.camera.transform.rotation.z = (int)(orientQuat[2] * (S3L_F / 2) / 3.1415626535);
    diceData->scene.camera.transform.rotation.w = (int)(orientQuat[3] * (S3L_F / 2) / 3.1415626535);

    diceData->scene.camera.transform.translation.z = -2 * S3L_F;

    for (uint32_t i = 0; i < diceData->dieCount; i++)
    {
        TPE_Body* body = &diceData->world.bodies[i];
        TPE_Vec3 pos = TPE_bodyGetCenterOfMass(body);
        TPE_Vec3 vel = TPE_bodyGetLinearVelocity(body);
        // joints in bodyGetRotation are:
        // - Forward: (1 -> 2)
        // - Right: (1 -> 3)
        // so this is fine, assuming a cube
        TPE_Vec3 rot = TPE_bodyGetRotation(body, 0, 1, 2);

        S3L_Model3D* model = &diceData->d6model;
        model->transform.translation.x = pos.x;
        model->transform.translation.y = pos.y;
        model->transform.translation.z = pos.z;

        // this is definitely not right
        //model->transform.rotation.x = rot.x;
        //model->transform.rotation.y = rot.y;
        //model->transform.rotation.z = rot.z;

        /*float eulerRot[3] = {
            rot.x * 2 * 3.1415926535 / TPE_F,
            rot.y * 2 * 3.1415926535 / TPE_F,
            rot.z * 2 * 3.1415926535 / TPE_F,
        };

        // Apply the physics rotation to the 3D model
        //mathEulerToQuat(diceData->scene.objects[i].orient, eulerRot);
        float translate[3];
        float rotate[4];
        float scale[3] = {CUBE_SCALE, CUBE_SCALE, CUBE_SCALE};
        tpeToWorldCoord(translate, pos);
        mathEulerToQuat(rotate, eulerRot);
        createTransformMatrix(diceData->scene.objects[i].transform, translate, rotate, scale);

        ESP_LOGI("Dice", "Object [%" PRIu32 "] Position is %" PRId16 ", %" PRId16 ", %" PRId16, i,
                 (int16_t)pos.x / TPE_F, (int16_t)pos.y / TPE_F, (int16_t)pos.z / TPE_F);
        ESP_LOGI("Dice", "Object [%" PRIu32 "] Velocity is %" PRId16 ", %" PRId16 ", %" PRId16, i,
                 (int16_t)vel.x / TPE_F, (int16_t)vel.y / TPE_F, (int16_t)vel.z / TPE_F);

        drawScene(&diceData->scene, 0, 0, TFT_WIDTH, TFT_HEIGHT);*/
    }

    S3L_newFrame();
    S3L_drawScene(diceData->scene);
}

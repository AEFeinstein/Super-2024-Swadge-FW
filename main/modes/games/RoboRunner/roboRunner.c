#include "roboRunner.h"
#include "geometry.h"

#define JUMP_HEIGHT          -12
#define Y_ACCEL              1
#define GROUND_HEIGHT        184
#define PLAYER_GROUND_OFFSET (GROUND_HEIGHT - 56)
#define BARREL_GROUND_OFFSET (GROUND_HEIGHT - 18)
#define PLAYER_X             32
#define MAX_OBSTACLES        2

const char runnerModeName[] = "Robo Runner";

static const char* const obstacleImageNames[] = {
    "Barrel-1.wsg",
    "Lamp.wsg",
};

typedef enum
{
    BARREL,
    LAMP,
} ObstacleType_t;

typedef struct
{
    rectangle_t rect; // Contains the x, y, width and height
    wsg_t img;        // The image
    bool onGround;    // If the player is touching the ground
    int ySpeed;       // The vertical speed. negative numbers are up.
} player_t;

typedef struct
{
    rectangle_t rect; // Contains the x, y, width and height
    int img;          // Image to display
    int speed;        // Speed of the obstacle
    bool active;      // If the obstacle is in play
} obstacle_t;

typedef struct
{
    // Robot
    player_t robot; // Player object

    // Obstacles
    wsg_t* obstacleImgs;                 // Array of obstacle images
    obstacle_t obstacles[MAX_OBSTACLES]; // Object data
    int obstacleIdx;                     // Index of the next obstacle
} runnerData_t;

static void runnerEnterMode(void);
static void runnerExitMode(void);
static void runnerMainLoop(int64_t elapsedUs);
static void runnerLogic(int64_t elapsedUS);
static void spawnObstacle(ObstacleType_t type, int idx);
static void draw(void);

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
    loadWsg("RoboStanding.wsg", &rd->robot.img, true);

    // Useful for loading a lot of sprites into one place.
    rd->obstacleImgs = heap_caps_calloc(ARRAY_SIZE(obstacleImageNames), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int32_t idx = 0; idx < ARRAY_SIZE(obstacleImageNames); idx++)
    {
        loadWsg(obstacleImageNames[idx], &rd->obstacleImgs[idx], true);
    }

    // Initialize the obstacles so we don't accidentally
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        rd->obstacles[idx].active = false;
    }

    // Temp
    spawnObstacle(BARREL, 0);
    spawnObstacle(LAMP, 1);
}

static void runnerExitMode()
{
    // Remember to de-allocate whatever you use!
    for (uint8_t idx = 0; idx < ARRAY_SIZE(obstacleImageNames); idx++)
    {
        freeWsg(&rd->obstacleImgs[idx]);
    }
    heap_caps_free(rd->obstacleImgs);

    freeWsg(&rd->robot.img);
    free(rd);
}

static void runnerMainLoop(int64_t elapsedUs)
{
    // Check input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.button & PB_A || evt.button & PB_UP) && rd->robot.onGround)
            {
                rd->robot.ySpeed   = JUMP_HEIGHT;
                rd->robot.onGround = false;
            }
        }
    }

    // Update player position
    runnerLogic(elapsedUs);

    // Draw screen
    draw();
}

static void runnerLogic(int64_t elapsedUS)
{
    rd->robot.rect.pos.y += rd->robot.ySpeed;
    rd->robot.ySpeed += Y_ACCEL;
    if (rd->robot.rect.pos.y > PLAYER_GROUND_OFFSET)
    {
        rd->robot.onGround   = true;
        rd->robot.rect.pos.y = PLAYER_GROUND_OFFSET;
        rd->robot.ySpeed     = 0;
    }
}

static void spawnObstacle(ObstacleType_t type, int idx)
{
    // Change this obstacle to active only if not already active, and abort if not.
    if (rd->obstacles[idx].active)
    {
        return;
    }
    // Might want to be more explicit
    rd->obstacles[idx].active = true;

    // Set data that's not going to change
    rd->obstacles[idx].rect.pos.x = TFT_WIDTH - 64;

    // Set box size
    // Note that both my obstacles are the same size, so changing it at all is redundant
    // Since they only need to be set once, we can even move them to the initialization step
    // If they were different, we could put them below.
    rd->obstacles[idx].rect.height = 24;
    rd->obstacles[idx].rect.width  = 12;

    // Switch based on type
    switch (type)
    {
        case BARREL:
        default:
        {
            // Set Y position
            rd->obstacles[idx].rect.pos.y = BARREL_GROUND_OFFSET;

            // Set sprite
            rd->obstacles[idx].img
                = BARREL; // Only works because the order we loaded the sprites into the initializer list.
            break;
        }
        case LAMP:
        {
            // Set y position
            rd->obstacles[idx].rect.pos.y = 0; // 0 is the top of the screen

            // Set sprite
            rd->obstacles[idx].img = LAMP;
            break;
        }
    }
}

static void draw()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    drawLine(0, GROUND_HEIGHT, TFT_WIDTH, GROUND_HEIGHT, c555, 0);
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        if (rd->obstacles[idx].active)
        {
            drawWsgSimple(&rd->obstacleImgs[rd->obstacles[idx].img], rd->obstacles[idx].rect.pos.x,
                          rd->obstacles[idx].rect.pos.y);
        }
    }
    drawWsgSimple(&rd->robot.img, PLAYER_X, rd->robot.rect.pos.y);
}

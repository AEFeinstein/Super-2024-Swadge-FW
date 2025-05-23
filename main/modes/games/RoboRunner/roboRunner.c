#include "roboRunner.h"
#include "geometry.h"

// Level
#define GROUND_HEIGHT  184
#define CEILING_HEIGHT 48

// Robot
#define JUMP_HEIGHT -12
#define Y_ACCEL     1
#define HBOX_WIDTH  30
#define HBOX_HEIGHT 24

// Positioning
#define PLAYER_X             48
#define PLAYER_X_IMG_OFFSET  12
#define PLAYER_Y_IMG_OFFSET  16
#define PLAYER_GROUND_OFFSET (GROUND_HEIGHT - 40)
#define BARREL_GROUND_OFFSET (GROUND_HEIGHT - 18)

// Obstacles
#define MAX_OBSTACLES    5
#define START_OBSTACLES  2
#define SPAWN_RATE_BASE  80
#define SPAWN_RATE_TIMER 15000000 // Fifteen seconds
#define SPEED_BASE       15
#define SPEED_TIMER      3000000 // Three seconds
#define SPEED_NUMERATOR  100000

// Score
#define SCORE_MOD 10000

const char runnerModeName[]   = "Robo Runner";
const char roboRunnerNVSKey[] = "roboRunner";

static const cnfsFileIdx_t obstacleImages[] = {
    BARREL_1_WSG,
    LAMP_WSG,
};

typedef enum
{
    BARREL,
    LAMP,
    NUM_OBSTACLE_TYPES
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

    // Score
    int32_t score;         // Current score
    int32_t prevScore;     // Previous high score
    int64_t remainingTime; // Time left over after we've added points

    // Difficulty
    int spawnRate;             // 1 / spawnRate per frame
    int64_t spawnRateTimer;    // Timer until we increase rate
    int speedDivisor;          // 10,000 / speedDivisor
    int64_t speedDivisorTimer; // Timer until we increase speed
    int currentMaxObstacles;   // 2-5
    int64_t maxObstacleTimer;  // Timer until we increase max obstacles
} runnerData_t;

static void runnerEnterMode(void);
static void runnerExitMode(void);
static void runnerMainLoop(int64_t elapsedUs);
static void resetGame(void);
static void runnerLogic(int64_t elapsedUS);
static void spawnObstacle(ObstacleType_t type, int idx);
static void updateObstacle(obstacle_t* obs, int64_t elapsedUs);
static void trySpawnObstacle(void);
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
    loadWsg(ROBO_STANDING_WSG, &rd->robot.img, true);

    // Useful for loading a lot of sprites into one place.
    rd->obstacleImgs = heap_caps_calloc(ARRAY_SIZE(obstacleImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(obstacleImages); idx++)
    {
        loadWsg(obstacleImages[idx], &rd->obstacleImgs[idx], true);
    }

    // Initialize the obstacles so we don't accidentally call unloaded data.
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        rd->obstacles[idx].active = false;
    }

    // Initialize the high score
    if (!readNvs32(roboRunnerNVSKey, &rd->prevScore))
    {
        rd->prevScore = 0;
    }

    resetGame();

    // Set Robot's rect
    rd->robot.rect.height = rd->robot.img.h - HBOX_HEIGHT;
    rd->robot.rect.width  = rd->robot.img.w - HBOX_WIDTH;
    rd->robot.rect.pos.x  = PLAYER_X;
}

static void runnerExitMode()
{
    // Remember to de-allocate whatever you use!
    for (int idx = 0; idx < ARRAY_SIZE(obstacleImages); idx++)
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

    // Update obstacles
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        // Update the obstacle
        updateObstacle(&rd->obstacles[idx], elapsedUs);

        // Check for a collision
        vec_t colVec;
        if (rd->obstacles[idx].active && rectRectIntersection(rd->robot.rect, rd->obstacles[idx].rect, &colVec))
        {
            resetGame();
        }
    }
    trySpawnObstacle();

    // Update score
    rd->remainingTime += elapsedUs;
    while (rd->remainingTime > SCORE_MOD)
    {
        rd->remainingTime -= SCORE_MOD;
        rd->score++;
    }

    // Adjust spawn rate
    rd->spawnRateTimer += elapsedUs;
    if (rd->spawnRateTimer > SPAWN_RATE_TIMER && rd->spawnRate > 1)
    {
        rd->spawnRate--;
        rd->spawnRateTimer = 0;
    }

    // Adjust speed
    rd->speedDivisorTimer += elapsedUs;
    if (rd->speedDivisorTimer > SPEED_TIMER && rd->speedDivisor < SPEED_NUMERATOR)
    {
        rd->speedDivisor++;
        rd->speedDivisorTimer = 0;
    }

    // Increase obstacles
    rd->maxObstacleTimer += elapsedUs;
    if (rd->speedDivisorTimer > SPEED_TIMER && rd->currentMaxObstacles < MAX_OBSTACLES)
    {
        rd->currentMaxObstacles++;
        rd->maxObstacleTimer = 0;
    }

    // Draw screen
    draw();
}

static void resetGame()
{
    // Initialize the obstacles
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        rd->obstacles[idx].active     = false;
        rd->obstacles[idx].rect.pos.x = -40;
    }
    if (rd->prevScore < rd->score)
    {
        rd->prevScore = rd->score;
        writeNvs32(roboRunnerNVSKey, rd->score);
    }
    rd->remainingTime       = 0;
    rd->score               = 0;
    rd->spawnRate           = SPAWN_RATE_BASE;
    rd->spawnRateTimer      = 0;
    rd->speedDivisor        = SPEED_BASE;
    rd->speedDivisorTimer   = 0;
    rd->currentMaxObstacles = START_OBSTACLES;
    rd->maxObstacleTimer    = 0;
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
    rd->obstacles[idx].rect.pos.x = TFT_WIDTH;

    // Set box size
    // Note that both my obstacles are the same size, so changing it at all is redundant
    // Since they only need to be set once, we can even move them to the initialization step
    // If they were different, we could put them below and adjust based on the specific requirements.
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
            rd->obstacles[idx].rect.pos.y = CEILING_HEIGHT; // 0 is the top of the screen

            // Set sprite
            rd->obstacles[idx].img = LAMP;
            break;
        }
    }
}

static void updateObstacle(obstacle_t* obs, int64_t elapsedUs)
{
    // Ditch if not active
    if (!obs->active)
    {
        return;
    }

    // Set value to subtract
    int moveSpeed = SPEED_NUMERATOR / rd->speedDivisor;
    while (elapsedUs > moveSpeed)
    {
        elapsedUs -= moveSpeed; // This ensures we don't get locked into a loop
        obs->rect.pos.x -= 1;   // Each time the loop executes, subtract 1
    }

    // If the obstacle is off-screen, disable it
    if (obs->rect.pos.x < -40)
    {
        obs->active = false;
        rd->score += 50;
    }
}

static void trySpawnObstacle()
{
    // Get a random number to try
    bool spawn = (esp_random() % rd->spawnRate) == 0;
    if (spawn)
    {
        for (int idx = 0; idx < rd->currentMaxObstacles; idx++)
        {
            if (!rd->obstacles[idx].active) // If the obstacle is already being used, we don't want set it!
            {
                spawnObstacle(esp_random() % NUM_OBSTACLE_TYPES, idx);
                return; // Exits function to stop it filling every slot
            }
        }
    }
}

static void draw()
{
    // Draw the level
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c112);
    drawLine(0, GROUND_HEIGHT, TFT_WIDTH, GROUND_HEIGHT, c555, 0);
    drawLine(0, CEILING_HEIGHT, TFT_WIDTH, CEILING_HEIGHT, c555, 0);

    // Draw the obstacles
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        if (rd->obstacles[idx].active)
        {
            drawWsgSimple(&rd->obstacleImgs[rd->obstacles[idx].img], rd->obstacles[idx].rect.pos.x,
                          rd->obstacles[idx].rect.pos.y);
        }
    }

    // Draw the player
    drawWsgSimple(&rd->robot.img, PLAYER_X - PLAYER_X_IMG_OFFSET, rd->robot.rect.pos.y - PLAYER_Y_IMG_OFFSET);

    // Draw the score
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Score: %" PRIu32, rd->score);
    drawText(getSysFont(), c555, buffer, 32, 4);
    snprintf(buffer, sizeof(buffer) - 1, "High score: %" PRIu32, rd->prevScore);
    drawText(getSysFont(), c555, buffer, 32, 20);

    // Uncomment these to draw the hitboxes
    /* drawRect(rd->robot.rect.pos.x, rd->robot.rect.pos.y, rd->robot.rect.pos.x + rd->robot.rect.width,
             rd->robot.rect.pos.y + rd->robot.rect.height, c500);
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        drawRect(rd->obstacles[idx].rect.pos.x, rd->obstacles[idx].rect.pos.y,
                 rd->obstacles[idx].rect.pos.x + rd->obstacles[idx].rect.width,
                 rd->obstacles[idx].rect.pos.y + rd->obstacles[idx].rect.height, c500);
    } */
}

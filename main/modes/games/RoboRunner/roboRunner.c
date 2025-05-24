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

// Drawing
#define WINDOW_PANE   25
#define WINDOW_BORDER 5
#define WINDOW_HEIGHT 120
#define WINDOW_COUNT  4

const char runnerModeName[]   = "Robo Runner";
const char roboRunnerNVSKey[] = "roboRunner";

static const cnfsFileIdx_t obstacleImages[] = {
    BARREL_1_WSG,
    BARREL_2_WSG,
    BARREL_3_WSG,
    LAMP_WSG,
};

static const cnfsFileIdx_t robotImages[] = {
    ROBO_STANDING_WSG,
    ROBO_RIGHT_WSG,
    ROBO_LEFT_WSG,
    ROBO_DEAD_WSG,
};

typedef enum
{
    BARREL,
    LAMP,
    NUM_OBSTACLE_TYPES
} ObstacleType_t;

typedef struct
{
    rectangle_t rect;  // Contains the x, y, width and height
    wsg_t* imgs;       // The images
    int animIdx;       // Animation index
    int64_t walkTimer; // time until we change animations
    bool onGround;     // If the player is touching the ground
    int ySpeed;        // The vertical speed. negative numbers are up.
    bool dead;         // If the player is dead
} player_t;

typedef struct
{
    rectangle_t rect; // Contains the x, y, width and height
    bool active;      // If the obstacle is in play
    ObstacleType_t t; // Type of obstacle
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

    // Audio
    midiFile_t bgm;          // BGM
    midiPlayer_t* sfxPlayer; // Player for SFX

    // Animation
    int64_t barrelAnimTimer;         // Time until the animation move to the next flame
    int barrelAnimIdx;               // Which index the barrel is using.
    int windowXCoords[WINDOW_COUNT]; // The window x coordinates
} runnerData_t;

static void runnerEnterMode(void);
static void runnerExitMode(void);
static void runnerMainLoop(int64_t elapsedUs);
static void resetGame(void);
static void runnerLogic(int64_t elapsedUS);
static void spawnObstacle(ObstacleType_t type, int idx);
static void updateObstacle(obstacle_t* obs, int64_t elapsedUs);
static void trySpawnObstacle(void);

// Drawing functions
static void drawWindow(int xCoord);
static void drawObstacles(int64_t elapsedUs);
static void drawPlayer(int64_t elapsedUs);
static void draw(int64_t elapsedUs);

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

    // Load all robot WSGs
    rd->robot.imgs = heap_caps_calloc(ARRAY_SIZE(robotImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(robotImages); idx++)
    {
        loadWsg(robotImages[idx], &rd->robot.imgs[idx], true);
    }

    // Useful for loading a lot of sprites into one place.
    rd->obstacleImgs = heap_caps_calloc(ARRAY_SIZE(obstacleImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(obstacleImages); idx++)
    {
        loadWsg(obstacleImages[idx], &rd->obstacleImgs[idx], true);
    }

    // Load BGM
    loadMidiFile(CHOWA_RACE_MID, &rd->bgm, true);
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    player->loop         = true;
    midiGmOn(player);
    globalMidiPlayerSetVolume(MIDI_BGM, 12);
    globalMidiPlayerPlaySong(&rd->bgm, MIDI_BGM);

    // Init SFX
    rd->sfxPlayer = globalMidiPlayerGet(MIDI_SFX);
    midiGmOn(rd->sfxPlayer);
    midiPause(rd->sfxPlayer, false);

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

    // Initializer windows
    for (int idx = 0; idx < WINDOW_COUNT; idx++)
    {
        rd->windowXCoords[idx] = idx * (TFT_WIDTH + 4 * WINDOW_BORDER + WINDOW_BORDER) / WINDOW_COUNT;
    }

    // Set Robot's rect
    rd->robot.rect.height = rd->robot.imgs[0].h - HBOX_HEIGHT;
    rd->robot.rect.width  = rd->robot.imgs[0].w - HBOX_WIDTH;
    rd->robot.rect.pos.x  = PLAYER_X;
}

static void runnerExitMode()
{
    globalMidiPlayerStop(MIDI_BGM);
    unloadMidiFile(&rd->bgm);

    // Remember to de-allocate whatever you use!
    for (int idx = 0; idx < ARRAY_SIZE(obstacleImages); idx++)
    {
        freeWsg(&rd->obstacleImgs[idx]);
    }
    heap_caps_free(rd->obstacleImgs);
    for (int idx = 0; idx < ARRAY_SIZE(robotImages); idx++)
    {
        freeWsg(&rd->robot.imgs[idx]);
    }
    heap_caps_free(&rd->robot.imgs);
    heap_caps_free(rd);
}

static void runnerMainLoop(int64_t elapsedUs)
{
    // Check input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (rd->robot.dead)
            {
                resetGame();
            }
            else if ((evt.button & PB_A || evt.button & PB_UP) && rd->robot.onGround)
            {
                rd->robot.ySpeed   = JUMP_HEIGHT;
                rd->robot.onGround = false;
            }
        }
    }

    // Update player position
    runnerLogic(elapsedUs);

    // Update obstacles
    if (!rd->robot.dead)
    {
        for (int idx = 0; idx < MAX_OBSTACLES; idx++)
        {
            // Update the obstacle
            updateObstacle(&rd->obstacles[idx], elapsedUs);

            // Check for a collision
            vec_t colVec;
            if (rd->obstacles[idx].active && rectRectIntersection(rd->robot.rect, rd->obstacles[idx].rect, &colVec))
            {
                midiNoteOn(rd->sfxPlayer, 9, HIGH_TOM, 0x7F);
                rd->robot.dead    = true;
                rd->robot.animIdx = 0;
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
    draw(elapsedUs);
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
    rd->robot.animIdx       = 0;
    rd->robot.dead          = false;
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
    rd->obstacles[idx].t           = type;

    // Switch based on type
    switch (type)
    {
        case BARREL:
        default:
        {
            // Set Y position
            rd->obstacles[idx].rect.pos.y = BARREL_GROUND_OFFSET;
            break;
        }
        case LAMP:
        {
            // Set y position
            rd->obstacles[idx].rect.pos.y = CEILING_HEIGHT; // 0 is the top of the screen
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

// Draw functions
static void drawWindow(int xCoord)
{
    fillDisplayArea(xCoord, WINDOW_HEIGHT, xCoord + WINDOW_PANE, WINDOW_HEIGHT + WINDOW_PANE, c035);
    fillDisplayArea(xCoord + WINDOW_BORDER + WINDOW_PANE, WINDOW_HEIGHT, xCoord + 2 * WINDOW_PANE + WINDOW_BORDER,
                    WINDOW_HEIGHT + WINDOW_PANE, c035);
    fillDisplayArea(xCoord, WINDOW_HEIGHT + WINDOW_BORDER + WINDOW_PANE, xCoord + WINDOW_PANE,
                    WINDOW_HEIGHT + 2 * WINDOW_PANE + WINDOW_BORDER, c035);
    fillDisplayArea(xCoord + WINDOW_BORDER + WINDOW_PANE, WINDOW_HEIGHT + WINDOW_BORDER + WINDOW_PANE,
                    xCoord + 2 * WINDOW_PANE + WINDOW_BORDER, WINDOW_HEIGHT + 2 * WINDOW_PANE + WINDOW_BORDER, c035);
}

static void drawObstacles(int64_t elapsedUs)
{
    // Draw the obstacles
    rd->barrelAnimTimer += elapsedUs;
    if (rd->barrelAnimTimer > 300000)
    {
        rd->barrelAnimIdx++;
        if (rd->barrelAnimIdx > 2)
        {
            rd->barrelAnimIdx = 0;
        }
    }
    for (int idx = 0; idx < MAX_OBSTACLES; idx++)
    {
        if (rd->obstacles[idx].active)
        {
            switch (rd->obstacles[idx].t)
            {
                case BARREL:
                {
                    drawWsgSimple(&rd->obstacleImgs[rd->barrelAnimIdx], rd->obstacles[idx].rect.pos.x,
                                  rd->obstacles[idx].rect.pos.y);
                    break;
                }
                case LAMP:
                {
                    drawWsgSimple(&rd->obstacleImgs[3], rd->obstacles[idx].rect.pos.x, rd->obstacles[idx].rect.pos.y);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
}

static void drawPlayer(int64_t elapsedUs)
{
    // Player animation frames
    rd->robot.walkTimer += elapsedUs;
    if (!rd->robot.dead && rd->robot.walkTimer > 50 * SPEED_NUMERATOR / rd->speedDivisor)
    {
        rd->robot.walkTimer = 0;
        rd->robot.animIdx++;
        if (rd->robot.animIdx > 1)
        {
            rd->robot.animIdx = 0;
        }
    }
    else if (rd->robot.dead && rd->robot.walkTimer > 50000 && rd->robot.animIdx < 10)
    {
        rd->robot.animIdx++;
        rd->robot.walkTimer = 0;
    }
    // Draw the player
    if (rd->robot.dead)
    {
        drawWsg(&rd->robot.imgs[3], PLAYER_X - PLAYER_X_IMG_OFFSET,
                rd->robot.rect.pos.y - PLAYER_Y_IMG_OFFSET + 5 * rd->robot.animIdx, false, false,
                360 - rd->robot.animIdx * 9);
    }
    else if (!rd->robot.onGround)
    {
        drawWsgSimple(&rd->robot.imgs[0], PLAYER_X - PLAYER_X_IMG_OFFSET, rd->robot.rect.pos.y - PLAYER_Y_IMG_OFFSET);
    }
    else
    {
        drawWsgSimple(&rd->robot.imgs[rd->robot.animIdx + 1], PLAYER_X - PLAYER_X_IMG_OFFSET,
                      rd->robot.rect.pos.y - PLAYER_Y_IMG_OFFSET);
    }
}

static void draw(int64_t elapsedUs)
{
    // Draw the level
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c112);
    drawLine(0, GROUND_HEIGHT, TFT_WIDTH, GROUND_HEIGHT, c555, 0);
    drawLine(0, CEILING_HEIGHT, TFT_WIDTH, CEILING_HEIGHT, c555, 0);
    // Windows
    if (!rd->robot.dead)
    {
        int64_t windowTimer = elapsedUs;
        while (windowTimer > SPEED_NUMERATOR / rd->speedDivisor)
        {
            windowTimer -= SPEED_NUMERATOR / rd->speedDivisor;
            for (int idx = 0; idx < WINDOW_COUNT; idx++)
            {
                rd->windowXCoords[idx] -= 1;
                if (rd->windowXCoords[idx] < -(4 * WINDOW_PANE + WINDOW_BORDER))
                {
                    rd->windowXCoords[idx] = TFT_WIDTH;
                }
            }
        }
    }
    for (int idx = 0; idx < WINDOW_COUNT; idx++)
    {
        drawWindow(rd->windowXCoords[idx]);
    }

    drawObstacles(elapsedUs);

    drawPlayer(elapsedUs);

    // Draw the score
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Score: %" PRIu32, rd->score);
    drawText(getSysFont(), c555, buffer, 32, 4);
    snprintf(buffer, sizeof(buffer) - 1, "High score: %" PRIu32, rd->prevScore);
    drawText(getSysFont(), c555, buffer, 32, 20);
}

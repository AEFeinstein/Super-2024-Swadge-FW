/*==============================================================================
 * Includes
 *============================================================================*/

#include <math.h>
#include <esp_random.h>
#include <esp_log.h>
#include "mode_shooter.h"
#include "mainMenu.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define WHITE c555
#define BLACK c000

// Math things
#define PI_DIV_4      0.70710678f
#define USEC_IN_SEC   1000000.0f
#define USEC_IN_D_SEC 100000

// Map macros
#define MAP_TILE(x, y) rc->map[(y) + ((x) * (rc->mapH))]

// Maximum number of sprites
#define NUM_SPRITES 60 ///< maximum number of sprites

// For the player
#define PLAYER_SHOT_COOLDOWN 300000 ///< Time between the player can shoot
#define LED_ON_TIME          500000 ///< Time the LEDs flash after shooting something or getting shot

#define PLAYER_MOVE_SPEED_E 5.0 ///< map tiles per second
#define PLAYER_MOVE_SPEED_M 5.0 ///< map tiles per second
#define PLAYER_MOVE_SPEED_H 6.0 ///< map tiles per second
#define PLAYER_ROT_SPEED_E  3.0 ///< radians per second
#define PLAYER_ROT_SPEED_M  3.0 ///< radians per second
#define PLAYER_ROT_SPEED_H  3.6 ///< radians per second

#define GUITAR_SHOT_RANGE 36.0f ///< range of the guitar's shots, in cells squared (i.e. range 6 -> 36)
/**
 * Damage is FLOOR(1 + ((GUITAR_SHOT_RANGE - distSqr) / DAMAGE_DIVISOR))
 * 11.7 yields 4dmg when less than 1 tile away
 *             3dmg between 1    and 3.5  tiles
 *             2dmg between 3.5  and 4.75 tiles
 *             1dmg between 4.75 and 6    tiles
 */
#define DAMAGE_DIVISOR      11.7f
#define CRITICAL_DISTANCE   1 ///< If something is shot less than this sqr distance, its a crit
#define HEALTH_PER_CRITICAL 2 ///< This much health is gained per critical hit

// For enemy textures, all times in microseconds
#define ENEMY_SHOT_COOLDOWN 3000000 ///< Minimum time between a sprite taking a shot
#define SHOOTING_ANIM_TIME \
    500000 ///< Time for each frame to be displayed when shooting. Overall shot time is SHOOTING_ANIM_TIME *
           ///< NUM_SHOT_FRAMES
#define GOT_SHOT_ANIM_TIME \
    500000 ///< Time for each frame to be displayed getting shot. Overall shot time is GOT_SHOT_ANIM_TIME *
           ///< NUM_HURT_FRAMES
#define LONG_WALK_ANIM_TIME 3000000 ///< Time a sprite will walk in a random direction for after being blocked
#define WALK_ANIM_TIME      1000000 ///< Time a sprite will walk towards the player
#define STEP_ANIM_TIME      250000  ///< Time each step frame will be displayed when walking
#define INVINCIBILITY_TIME \
    ((GOT_SHOT_ANIM_TIME * NUM_HURT_FRAMES) + 1000000) ///< Time a sprite is invincible after being shot
#define INVINCIBILITY_HZ 400000                        ///< Period for the invincibility flicker

#define SPRITE_MOVE_SPEED_E 1.4f ///< map tiles per second
#define SPRITE_MOVE_SPEED_M 2.0f ///< map tiles per second
#define SPRITE_MOVE_SPEED_H 3.0f ///< map tiles per second

// Animation frame counts, don't change these unless also changing textures
#define NUM_WALK_FRAMES 2
#define NUM_SHOT_FRAMES 2
#define NUM_HURT_FRAMES 2

#define NUM_GTR_FRAMES  5
#define NUM_BOOT_FRAMES 3

// Starting enemy health, per difficulty
#define ENEMY_HEALTH_E 1
#define ENEMY_HEALTH_M 2
#define ENEMY_HEALTH_H 4

// Effective radar distance
#define RADAR_RANGE 81 ///< Radar distance, in cells squared (i.e. range 9 -> 81)

// Input timer
#define STRAFE_BUTTON_TIMEOUT 500000 ///< Time allowed between strafe double-taps
#define STRAFE_TIME           250000 ///< Time spent strafing per double-tap

// Game over defines
#define GAME_OVER_BUTTON_LOCK_US 2000000

/*==============================================================================
 * Enums
 *============================================================================*/

typedef enum __attribute__((__packed__))
{
    E_IDLE,
    E_PICK_DIR_PLAYER,
    E_PICK_DIR_RAND,
    E_WALKING,
    E_SHOOTING,
    E_GOT_SHOT,
    E_DEAD
} enemyState_t;

typedef enum
{
    RC_MENU,
    RC_MAP_SELECT,
    RC_GAME,
    RC_GAME_OVER,
    RC_SCORES
} shooterMode_t;

// World map tiles. Make sure this is packed
typedef enum
{
    WMT_W1 = 0, ///< Wall 1
    WMT_W2 = 1, ///< Wall 2
    WMT_W3 = 2, ///< Wall 2
    WMT_C  = 3, ///< Column
    WMT_E  = 4, ///< Empty
    WMT_S  = 5, ///< Spawn point
} WorldMapTile_t;

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    uint8_t mapX;
    uint8_t mapY;
    uint8_t side;
    int32_t drawStart;
    int32_t drawEnd;
    float perpWallDist;
    float rayDirX;
    float rayDirY;
} rayResult_t;

typedef struct
{
    // Sprite location and direction
    float posX;
    float posY;
    float dirX;
    float dirY;

    // Sprite texture
    wsg_t* texture;
    int32_t texTimer;
    int8_t texFrame;
    bool mirror;

    // Sprite logic
    enemyState_t state;
    int32_t stateTimer;
    int32_t shotCooldown;
    bool isBackwards;
    int32_t health;
    int32_t invincibilityTimer;
    bool shotWillMiss;
} raySprite_t;

typedef struct
{
    shooterMode_t mode;
    menu_t* menu;

    uint8_t rButtonState;
    int32_t strafeLeftTmr;
    int32_t strafeRightTmr;
    float posX;
    float posY;
    float dirX;
    float dirY;
    float planeX;
    float planeY;
    int32_t shotCooldown;
    bool checkShot;
    int32_t initialHealth;
    int32_t health;
    uint32_t tRoundStartedUs;
    uint32_t tRoundElapsed;
    shooterDifficulty_t difficulty;

    // The enemies
    raySprite_t sprites[NUM_SPRITES];
    uint8_t liveSprites;
    uint8_t kills;

    // Storage for textures
    wsg_t stoneTex;
    wsg_t stripeTex;
    wsg_t brickTex;
    wsg_t sinTex;

    wsg_t walk[NUM_WALK_FRAMES];
    wsg_t shooting[NUM_SHOT_FRAMES];
    wsg_t hurt[NUM_HURT_FRAMES];
    wsg_t dead;

    // Storage for HUD images
    wsg_t heart;
    wsg_t mNote;
    wsg_t gtr[NUM_GTR_FRAMES];
    wsg_t boot[NUM_BOOT_FRAMES];

    // For the map
    uint16_t mapW;
    uint16_t mapH;
    const WorldMapTile_t* map;
    shooterMap_t mapIdx;

    rayResult_t rayResult[TFT_WIDTH];

    // For LEDs
    uint32_t ledTimerAccum;
    uint32_t closestDist;
    float closestAngle;
    bool radarObstructed;
    int32_t warningShotTimer;
    float warningShotAngle;
    int32_t gotShotTimer;
    float shotFromAngle;
    int32_t shotSomethingTimer;
    bool isCriticalHit;
    int32_t killedSpriteTimer;
    int32_t healthWarningTimer;
    bool healthWarningInc;

    // For the game over screen
    int32_t gameOverButtonLockUs;

    // Fonts
    font_t ibm;
    font_t radioStars;
    font_t tomThumb;
} shooter_t;

/*==============================================================================
 * Prototypes
 *============================================================================*/

void shooterEnterMode(void);
void shooterExitMode(void);
void shooterButtonCallback(buttonEvt_t evt);
void shooterMenuButtonCallback(const char* label, bool selected, uint32_t value);
void shooterRenderTask(int64_t tElapsedUs);
void shooterGameRenderer(int64_t tElapsedUs);
void shooterLedTimer(void);
void shooterDrawScores(void);
void shooterScoreDisplayButton(uint32_t);
void shooterEndRound(void);
void shooterDrawRoundOver(uint32_t tElapsedUs);

void moveEnemies(uint32_t tElapsedUs);
void handleRayInput(uint32_t tElapsedUs);

void castRays(rayResult_t* rayResult);
void drawTextures(rayResult_t* rayResult);
void drawOutlines(rayResult_t* rayResult);
void drawSprites(rayResult_t* rayResult);
void drawHUD(void);

void shooterInitGame(shooterDifficulty_t difficulty);
void sortSprites(int32_t* order, float* dist, int32_t amount);
float Q_rsqrt(float number);
bool checkWallsBetweenPoints(float sX, float sY, float pX, float pY);
void setSpriteState(raySprite_t* sprite, enemyState_t state);
float angleBetween(float pPosX, float pPosY, float pDirX, float pDirY, float sPosX, float sPosY);
void lightLedsFromAngle(led_t* leds, float angle, int hue, int valNumerator, int valDenominator);

void shooterMapSelectRenderer(uint32_t tElapsedUs);
void shooterMapSelectButton(int32_t button);
void shooterSetMap(void);

void addShooterScore(shooterDifficulty_t difficulty, shooterMap_t mapIdx, uint16_t kills, uint32_t tElapsed);
shooterScores_t* getShooterScores(void);

/*==============================================================================
 * Variables
 *============================================================================*/

const char TAG[] = "SH";

swadgeMode_t shooterMode = {
    .modeName        = "shooter",
    .fnEnterMode     = shooterEnterMode,
    .fnExitMode      = shooterExitMode,
    .fnMainLoop      = shooterRenderTask,
    .wifiMode        = NO_WIFI,
    .fnEspNowRecvCb  = NULL,
    .fnEspNowSendCb  = NULL,
    .fnAudioCallback = NULL,
    // TODO missing stuff
};

shooter_t* rc;

#define MAP_L_W 48
#define MAP_L_H 48
static const WorldMapTile_t worldMap_l[MAP_L_W][MAP_L_H] = {
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 2,
        4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 5, 4, 5, 4, 4, 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 5, 4, 1, 2,
        4, 5, 4, 4, 4, 4, 2, 4, 4, 4, 4, 5, 4, 2, 2, 2, 1, 4, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 1, 2,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 3, 3, 4, 4, 4, 0, 4, 4, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 1, 2,
        4, 4, 4, 3, 4, 4, 5, 4, 4, 3, 4, 4, 4, 2, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 3, 3, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 1, 2,
        4, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 4, 2, 1, 4, 3, 4, 3, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 2,
        4, 4, 4, 4, 4, 3, 4, 3, 4, 4, 4, 4, 4, 2, 1, 4, 5, 5, 5, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 1, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 1, 2,
        2, 2, 4, 5, 4, 4, 3, 4, 4, 5, 4, 2, 2, 2, 1, 4, 3, 4, 3, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 1, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 1, 2,
        4, 4, 4, 4, 4, 3, 4, 3, 4, 4, 4, 4, 4, 2, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 5, 4, 1, 2,
        4, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 4, 2, 1, 4, 3, 4, 3, 4, 1, 1, 1, 1,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 2,
        4, 4, 4, 3, 4, 4, 5, 4, 4, 3, 4, 4, 4, 2, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 2,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 4, 3, 4, 3, 4, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 4, 4, 4, 4, 1, 2,
        4, 5, 4, 4, 4, 4, 2, 4, 4, 4, 4, 5, 4, 2, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2,
        4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2,
        4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 0,
        4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 4, 3, 4, 4, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 2, 0,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 0, 0, 0, 0, 0, 4, 4, 0,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 3, 3, 3, 4, 5, 4, 3, 3, 3, 4, 4, 4, 4, 4, 2, 0,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 4, 0, 4, 4, 4, 0, 0, 0, 4, 4, 4, 0,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 4, 3, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 5, 4, 2, 0,
        4, 4, 3, 3, 3, 4, 4, 3, 3, 4, 5, 5, 4, 0, 4, 5, 4, 4, 0, 4, 4, 5, 4, 0,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 5, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 3, 4, 4, 4, 2, 0,
        4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
    },
    {
        0, 0, 0, 0, 0, 2, 4, 4, 4, 4, 4, 3, 3, 3, 4, 4, 4, 3, 3, 3, 4, 4, 2, 0,
        4, 4, 3, 4, 5, 5, 4, 4, 4, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0, 0,
    },
    {
        0, 0, 2, 2, 2, 2, 4, 4, 3, 4, 4, 4, 3, 4, 4, 5, 4, 4, 3, 4, 4, 4, 2, 0,
        4, 4, 4, 4, 5, 5, 4, 4, 3, 3, 3, 4, 4, 0, 0, 0, 4, 4, 5, 4, 4, 0, 0, 0,
    },
    {
        0, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 0,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 0, 0, 0, 0, 4, 5, 4, 0, 0, 0, 0,
    },
    {
        0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 0,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 0, 0, 0, 4, 4, 5, 4, 4, 0, 0, 0,
    },
    {
        0, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0, 0,
    },
    {
        0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 0, 4, 5, 4, 5, 4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 1, 0, 4, 4, 3, 4, 4, 0, 4, 5, 4, 4, 0, 4, 4, 5, 4, 0,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 1, 0, 4, 4, 3, 4, 4, 0, 4, 4, 4, 0, 0, 0, 4, 4, 4, 0,
    },
    {
        0, 4, 4, 3, 4, 4, 3, 3, 3, 3, 4, 4, 0, 1, 4, 4, 3, 3, 3, 3, 3, 4, 4, 3,
        3, 3, 3, 3, 4, 4, 1, 0, 4, 5, 4, 5, 4, 0, 4, 4, 0, 0, 0, 0, 0, 4, 4, 0,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 3, 4, 4, 1, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0,
    },
    {
        0, 4, 4, 4, 4, 4, 5, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 5, 4, 4, 4, 4, 4,
        4, 5, 4, 3, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 4, 4, 0,
    },
    {
        0, 4, 4, 3, 3, 3, 3, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 3, 4, 4, 3,
        3, 4, 4, 3, 4, 4, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2,
    },
    {
        0, 4, 4, 3, 4, 4, 4, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4,
        3, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        0, 4, 4, 3, 4, 4, 5, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 4, 5, 4,
        3, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 3, 3, 3, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        0, 4, 4, 3, 4, 4, 3, 3, 3, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4,
        3, 4, 4, 3, 4, 4, 1, 2, 4, 4, 4, 2, 4, 5, 4, 4, 4, 4, 4, 2, 4, 5, 4, 2,
    },
    {
        0, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4,
        4, 4, 4, 4, 4, 4, 1, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        0, 4, 4, 3, 4, 4, 5, 4, 4, 4, 5, 4, 0, 1, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4,
        4, 4, 4, 4, 4, 4, 1, 2, 4, 4, 4, 2, 4, 5, 4, 2, 4, 4, 4, 2, 4, 5, 4, 2,
    },
    {
        0, 4, 4, 3, 3, 3, 3, 4, 4, 3, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4,
        3, 4, 4, 3, 4, 4, 1, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 5, 4, 4,
        3, 4, 4, 3, 4, 4, 1, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2,
    },
    {
        0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4,
        3, 4, 4, 3, 4, 4, 1, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 1, 4, 4, 3, 4, 4, 3, 3, 4, 4, 3,
        3, 4, 4, 3, 4, 4, 1, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2, 4, 3, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0, 4, 4, 0, 1, 1, 4, 4, 3, 4, 5, 4, 4, 4, 4, 4,
        4, 5, 4, 3, 4, 4, 1, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0, 4, 4, 0, 1, 1, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 3, 4, 4, 1, 2, 4, 5, 4, 2, 4, 4, 4, 2, 4, 5, 4, 2, 4, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0, 4, 4, 4, 1, 1, 4, 4, 3, 3, 3, 3, 3, 4, 4, 3,
        3, 3, 3, 3, 4, 4, 1, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 1, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 5, 4, 4, 4, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 1, 2, 4, 4, 3, 3, 3, 4, 4, 2, 4, 4, 3, 3, 3, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    },
};

#define MAP_M_W 30
#define MAP_M_H 30
static const WorldMapTile_t worldMap_m[MAP_M_W][MAP_M_H] = {
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 5, 4, 4, 4, 5, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
    },
    {
        1, 4, 4, 3, 3, 3, 4, 3, 3, 3, 4, 4, 1, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 4, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 4, 4, 3, 3, 4, 4, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 4, 4, 3, 3, 4, 4, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 4, 4, 3, 3, 4, 4, 0,
    },
    {
        1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 4, 5, 4, 4, 5, 4, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 4, 4, 4, 4, 4, 4, 4, 0, 5, 4, 0, 0, 0, 4, 4, 4, 4, 4, 4, 0,
    },
    {
        1, 5, 4, 4, 4, 4, 4, 4, 4, 1, 4, 4, 5, 4, 4, 4, 4, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 0, 2, 2, 2, 2, 2, 2, 4, 4, 2,
    },
    {
        1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 0, 0, 4, 4, 0, 4, 4, 0, 2, 4, 3, 4, 4, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 4, 4, 0, 4, 4, 0, 2, 4, 5, 3, 4, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 4, 4, 0, 4, 5, 0, 2, 4, 4, 4, 3, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 4, 5, 4, 4, 4, 0, 2, 4, 4, 4, 4, 3, 4, 4, 2,
    },
    {
        1, 4, 4, 3, 3, 3, 4, 3, 3, 3, 4, 4, 1, 0, 0, 4, 4, 4, 4, 4, 0, 2, 4, 4, 4, 4, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 4, 3, 4, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 5, 4, 4, 4, 5, 4, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 3, 4, 4, 4, 2,
    },
    {
        1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 3, 4, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 5, 5, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 5, 5, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 3, 4, 4, 4, 4, 3, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    },
};

#define MAP_S_W 12
#define MAP_S_H 12
static const WorldMapTile_t worldMap_s[MAP_S_W][MAP_S_H] = {
    {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
    },
    {
        1,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        1,
    },
    {
        1,
        4,
        5,
        4,
        4,
        5,
        3,
        3,
        4,
        5,
        4,
        1,
    },
    {
        1,
        4,
        3,
        3,
        4,
        4,
        4,
        4,
        4,
        3,
        4,
        1,
    },
    {
        1,
        4,
        4,
        5,
        4,
        3,
        4,
        5,
        4,
        3,
        5,
        1,
    },
    {
        1,
        4,
        3,
        4,
        4,
        3,
        4,
        4,
        4,
        4,
        4,
        1,
    },
    {
        1,
        4,
        3,
        5,
        4,
        5,
        4,
        3,
        3,
        5,
        4,
        1,
    },
    {
        1,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        1,
    },
    {
        1,
        4,
        5,
        4,
        3,
        3,
        4,
        5,
        4,
        4,
        5,
        1,
    },
    {
        1,
        5,
        3,
        4,
        5,
        4,
        4,
        1,
        1,
        1,
        4,
        1,
    },
    {
        1,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        1,
        5,
        1,
    },
    {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
    },
};

static const char rc_title[] = "SHREDDER";

static const char rc_small[]  = "SM";
static const char rc_medium[] = "MD";
static const char rc_large[]  = "LG";

const char str_easy[]   = "GENTLE";
const char str_medium[] = "HURT ME";
const char str_hard[]   = "VIOLENT";
const char str_v_hard[] = "NIGHTMARE";

static const char* const difficultyLabels[] = {str_easy, str_medium, str_hard, str_v_hard};

const char str_high_scores[] = "HIGH SCORES";
const char str_quit[]        = "QUIT";

/*==============================================================================
 * Functions
 *============================================================================*/

/**
 * Initialize the shooter mode and allocate all assets
 */
void shooterEnterMode(void)
{
    // Allocate and zero out everything
    rc = calloc(1, sizeof(shooter_t));
    memset(rc, 0, sizeof(shooter_t));

    // Initialize and start at the menu
    rc->mode = RC_MENU;
    rc->menu = initMenu(rc_title, shooterMenuButtonCallback);
    addMultiItemToMenu(rc->menu, difficultyLabels, ARRAY_SIZE(difficultyLabels), 0);
    addSingleItemToMenu(rc->menu, str_high_scores);
    addSingleItemToMenu(rc->menu, str_quit);

    // Load the enemy texture to RAM
    loadWsg("h8_wlk1.wsg", &rc->walk[0], true);
    loadWsg("h8_wlk2.wsg", &rc->walk[1], true);
    loadWsg("h8_atk1.wsg", &rc->shooting[0], true);
    loadWsg("h8_atk2.wsg", &rc->shooting[1], true);
    loadWsg("h8_hrt1.wsg", &rc->hurt[0], true);
    loadWsg("h8_hrt2.wsg", &rc->hurt[1], true);
    loadWsg("h8_ded.wsg", &rc->dead, true);

    // Load the wall textures to RAM
    loadWsg("txStone.wsg", &rc->stoneTex, true);
    loadWsg("txStripe.wsg", &rc->stripeTex, true);
    loadWsg("txBrick.wsg", &rc->brickTex, true);
    loadWsg("txSinW.wsg", &rc->sinTex, true);

    // Load the HUD assets
    loadWsg("heart.wsg", &(rc->heart), true);
    loadWsg("mNote.wsg", &(rc->mNote), true);

    loadWsg("gtr1.wsg", &rc->gtr[0], true);
    loadWsg("gtr2.wsg", &rc->gtr[1], true);
    loadWsg("gtr3.wsg", &rc->gtr[2], true);
    loadWsg("gtr4.wsg", &rc->gtr[3], true);
    loadWsg("gtr5.wsg", &rc->gtr[4], true);

    loadWsg("boot1.wsg", &rc->boot[0], true);
    loadWsg("boot2.wsg", &rc->boot[1], true);
    loadWsg("boot3.wsg", &rc->boot[2], true);

    // Load fonts (TODO, actual fonts)
    loadFont("ibm_vga8.font", &rc->tomThumb, true);
    loadFont("ibm_vga8.font", &rc->ibm, true);
    loadFont("ibm_vga8.font", &rc->radioStars, true);

    // Set up the LED timer
    rc->closestDist     = 0xFFFFFFFF;
    rc->closestAngle    = 0;
    rc->radarObstructed = false;

    // Set map
    shooterSetMap();

    // buttonEvt_t evtDown = {.button = PB_A, .down = true, .state = PB_A};
    // buttonEvt_t evtUp   = {.button = PB_A, .down = false, .state = 0};
    // shooterButtonCallback(evtDown);
    // shooterButtonCallback(evtUp);
    // shooterButtonCallback(evtDown);
    // shooterButtonCallback(evtUp);
}

/**
 * Free all resources allocated in shooterEnterMode
 */
void shooterExitMode(void)
{
    // Free menu
    deinitMenu(rc->menu);

    // Free assets
    freeWsg(&rc->walk[0]);
    freeWsg(&rc->walk[1]);
    freeWsg(&rc->shooting[0]);
    freeWsg(&rc->shooting[1]);
    freeWsg(&rc->hurt[0]);
    freeWsg(&rc->hurt[1]);
    freeWsg(&rc->dead);

    freeWsg(&rc->stoneTex);
    freeWsg(&rc->stripeTex);
    freeWsg(&rc->brickTex);
    freeWsg(&rc->sinTex);

    freeWsg(&(rc->heart));
    freeWsg(&(rc->mNote));

    freeWsg(&rc->gtr[0]);
    freeWsg(&rc->gtr[1]);
    freeWsg(&rc->gtr[2]);
    freeWsg(&rc->gtr[3]);
    freeWsg(&rc->gtr[4]);

    freeWsg(&rc->boot[0]);
    freeWsg(&rc->boot[1]);
    freeWsg(&rc->boot[2]);

    freeFont(&rc->tomThumb);
    freeFont(&rc->ibm);
    freeFont(&rc->radioStars);

    // Free everything else
    free(rc);
    rc = NULL;
}

/**
 * Initialize the game state and start it
 *
 * @param difficulty This round's difficulty
 */
void shooterInitGame(shooterDifficulty_t difficulty)
{
    // Set the mode and difficulty
    rc->mode       = RC_GAME;
    rc->difficulty = difficulty;

    // Clear the button state
    rc->rButtonState   = 0;
    rc->strafeLeftTmr  = 0;
    rc->strafeRightTmr = 0;

    // x and y start position
    rc->posY = 8;
    rc->posX = 10.5;
    // initial direction vector
    rc->dirX = 0;
    rc->dirY = -1;
    // the 2d shooter version of camera plane
    rc->planeX = -0.66;
    rc->planeY = 0;

    // Make sure all sprites are initially out of bounds
    for (uint8_t i = 0; i < NUM_SPRITES; i++)
    {
        rc->sprites[i].posX = -1;
        rc->sprites[i].posY = -1;
    }

    // Set the number of enemies based on the difficulty
    uint16_t diffMod = 0;
    if (RC_EASY == difficulty)
    {
        diffMod = 2;
    }
    else if (RC_MED == difficulty)
    {
        diffMod = 3;
    }
    else if (RC_HARD == difficulty)
    {
        diffMod = 1;
    }

    // Start spawning sprites
    rc->liveSprites   = 0;
    rc->kills         = 0;
    uint16_t spawnIdx = 0;
#ifndef TEST_GAME_OVER
    // Look over the whole map
    for (uint8_t x = 0; x < rc->mapW; x++)
    {
        for (uint8_t y = 0; y < rc->mapH; y++)
        {
            // If this is a spawn point
            if (MAP_TILE(x, y) == WMT_S)
            {
                // And a sprite should be spawned
                if (rc->liveSprites < NUM_SPRITES && (((spawnIdx % diffMod) > 0) || (RC_HARD == difficulty)))
                {
                    // Spawn it here
                    rc->sprites[rc->liveSprites].posX               = x;
                    rc->sprites[rc->liveSprites].posY               = y;
                    rc->sprites[rc->liveSprites].dirX               = 0;
                    rc->sprites[rc->liveSprites].dirX               = 0;
                    rc->sprites[rc->liveSprites].shotCooldown       = 0;
                    rc->sprites[rc->liveSprites].shotWillMiss       = 0;
                    rc->sprites[rc->liveSprites].isBackwards        = false;
                    rc->sprites[rc->liveSprites].invincibilityTimer = 0;
                    switch (rc->difficulty)
                    {
                        default:
                        case RC_NUM_DIFFICULTIES:
                        case RC_EASY:
                        {
                            rc->sprites[rc->liveSprites].health = ENEMY_HEALTH_E;
                            break;
                        }
                        case RC_MED:
                        {
                            rc->sprites[rc->liveSprites].health = ENEMY_HEALTH_M;
                            break;
                        }
                        case RC_HARD:
                        {
                            rc->sprites[rc->liveSprites].health = ENEMY_HEALTH_H;
                            break;
                        }
                    }
                    setSpriteState(&(rc->sprites[rc->liveSprites]), E_IDLE);
                    rc->liveSprites++;
                }
                spawnIdx++;
            }
        }
    }
#else
    // For testing, just spawn one sprite
    rc->sprites[rc->liveSprites].posX               = rc->posX;
    rc->sprites[rc->liveSprites].posY               = rc->posY;
    rc->sprites[rc->liveSprites].dirX               = 0;
    rc->sprites[rc->liveSprites].dirX               = 0;
    rc->sprites[rc->liveSprites].shotCooldown       = 0;
    rc->sprites[rc->liveSprites].shotWillMiss       = 0;
    rc->sprites[rc->liveSprites].isBackwards        = false;
    rc->sprites[rc->liveSprites].invincibilityTimer = 0;
    rc->sprites[rc->liveSprites].health             = ENEMY_HEALTH_E;
    setSpriteState(&(rc->sprites[rc->liveSprites]), E_IDLE);
    rc->liveSprites++;
#endif

    // Set health based on the number of enemies and difficulty
    if (RC_EASY == difficulty)
    {
        rc->initialHealth = rc->liveSprites * 2;
    }
    else if (RC_MED == difficulty)
    {
        rc->initialHealth = (rc->liveSprites * 3) / 2;
    }
    else if (RC_HARD == difficulty)
    {
        rc->initialHealth = rc->liveSprites;
    }
    rc->health = rc->initialHealth;

    // Clear player shot variables
    rc->shotCooldown       = 0;
    rc->checkShot          = false;
    rc->warningShotTimer   = 0;
    rc->warningShotAngle   = 0;
    rc->gotShotTimer       = 0;
    rc->shotFromAngle      = 0;
    rc->shotSomethingTimer = 0;
    rc->isCriticalHit      = false;

    rc->killedSpriteTimer  = 0;
    rc->healthWarningTimer = 0;
    rc->healthWarningInc   = true;

    // Reset the closest distance to not shine LEDs
    rc->closestDist     = 0xFFFFFFFF;
    rc->closestAngle    = 0;
    rc->radarObstructed = false;

    // Start the round clock
    rc->tRoundElapsed   = 0;
    rc->tRoundStartedUs = esp_timer_get_time();
}

/**
 * The button callback. Handle the buttons differently per-mode
 *
 * @param state  A bitmask with all the current button states
 * @param button The button that caused this interrupt
 * @param down   true if the button was pushed, false if it was released
 */
void shooterButtonCallback(buttonEvt_t evt)
{
    switch (rc->mode)
    {
        default:
        case RC_MENU:
        {
            if (evt.down)
            {
                // Just pass the button along to the menu button handler
                rc->menu = menuButton(rc->menu, evt);
            }
            break;
        }
        case RC_MAP_SELECT:
        {
            if (evt.down)
            {
                shooterMapSelectButton(evt.button);
            }
            break;
        }
        case RC_GAME:
        {
            // Save the button state for the game to process later
            rc->rButtonState = evt.state;
            break;
        }
        case RC_GAME_OVER:
        {
            // If any button is pressed on game over, just return to the menu
            if (rc->gameOverButtonLockUs == 0 && evt.down)
            {
                rc->mode = RC_MENU;
            }
            break;
        }
        case RC_SCORES:
        {
            // If the button is pressed on the score screen
            if (evt.down)
            {
                shooterScoreDisplayButton(evt.button);
            }
            break;
        }
    }
}

/**
 * @brief Button callback from the top level menu when an item is selected
 *
 * @param label
 * @param selected
 * @param value
 */
void shooterMenuButtonCallback(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        // Do a thing based on the menu option selected
        if (str_quit == label)
        {
            // Quit to the Swadge menu
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (str_easy == label)
        {
            rc->difficulty = RC_EASY;
        }
        else if (str_medium == label)
        {
            rc->difficulty = RC_MED;
        }
        else if (str_v_hard == label)
        {
            rc->difficulty = RC_HARD;
        }
        else if (str_high_scores == label)
        {
            // Show some scores
            rc->mode       = RC_SCORES;
            rc->difficulty = RC_EASY;
        }

        // Then set mode to map select if it was a game start
        if ((str_easy == label) || (str_medium == label) || (str_v_hard == label))
        {
            rc->mode   = RC_MAP_SELECT;
            rc->mapIdx = RC_MAP_S;
            shooterSetMap();
        }
    }
}

/**
 * This is called whenever there is a screen to render
 * Draw the menu, game, game over, or high scores
 *
 * @param tElapsedUs The time since this was last called
 */
void shooterRenderTask(int64_t tElapsedUs)
{
    rc->ledTimerAccum += tElapsedUs;
    while (rc->ledTimerAccum >= 10000)
    {
        rc->ledTimerAccum -= 10000;
        shooterLedTimer();
    }

    // Check all queued button events
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        shooterButtonCallback(evt);
    }

    // Draw something based on the mode
    switch (rc->mode)
    {
        default:
        case RC_MENU:
        {
            drawMenu(rc->menu, &rc->ibm);
            break;
        }
        case RC_MAP_SELECT:
        {
            shooterMapSelectRenderer(tElapsedUs);
            break;
        }
        case RC_GAME:
        {
            shooterGameRenderer(tElapsedUs);
            break;
        }
        case RC_GAME_OVER:
        {
            shooterDrawRoundOver(tElapsedUs);
            break;
        }
        case RC_SCORES:
        {
            shooterDrawScores();
            break;
        }
    }
}

/**
 * This function renders the scene and handles input. It is called whenever
 * the system is ready to render a new frame
 *
 * @param tElapsedUs The time since the last call
 */
void shooterGameRenderer(int64_t tElapsedUs)
{
    // First handle button input
    handleRayInput(tElapsedUs);
    // Then move enemies around
    moveEnemies(tElapsedUs);
    // Tick down the timer to flash LEDs when shot or shooting
    if (rc->warningShotTimer > 0)
    {
        rc->warningShotTimer -= tElapsedUs;
    }
    if (rc->gotShotTimer > 0)
    {
        rc->gotShotTimer -= tElapsedUs;
    }
    if (rc->shotSomethingTimer > 0)
    {
        rc->shotSomethingTimer -= tElapsedUs;
    }
    if (rc->killedSpriteTimer > 0)
    {
        rc->killedSpriteTimer -= tElapsedUs;
    }

    // Cast all the rays for the scene and save the result
    castRays(rc->rayResult);
    // Clear the display, then draw all the layers
    clearPxTft();
    drawTextures(rc->rayResult);
    drawOutlines(rc->rayResult);
    drawSprites(rc->rayResult);
    drawHUD();
}

/**
 * Cast all the rays into the scene, iterating across the X axis, and save the
 * results in the rayResult argument
 *
 * @param rayResult A pointer to an array of rayResult_t where this scene's
 *                  information is stored
 */
void castRays(rayResult_t* rayResult)
{
    for (int32_t x = 0; x < TFT_WIDTH; x++)
    {
        // calculate ray position and direction
        // x-coordinate in camera space
        float cameraX = 2 * x / (float)TFT_WIDTH - 1;
        float rayDirX = rc->dirX + rc->planeX * cameraX;
        float rayDirY = rc->dirY + rc->planeY * cameraX;

        // which box of the map we're in
        int32_t mapX = (int32_t)(rc->posX);
        int32_t mapY = (int32_t)(rc->posY);

        // length of ray from current position to next x or y-side
        float sideDistX;
        float sideDistY;

        // length of ray from one x or y-side to next x or y-side
        float deltaDistX = (1 / rayDirX);
        if (deltaDistX < 0)
        {
            deltaDistX = -deltaDistX;
        }

        float deltaDistY = (1 / rayDirY);
        if (deltaDistY < 0)
        {
            deltaDistY = -deltaDistY;
        }

        // what direction to step in x or y-direction (either +1 or -1)
        int32_t stepX;
        int32_t stepY;

        int32_t hit  = 0; // was there a wall hit?
        int32_t side = 0; // was a NS or a EW wall hit?
        // calculate step and initial sideDist
        if (rayDirX < 0)
        {
            stepX     = -1;
            sideDistX = (rc->posX - mapX) * deltaDistX;
        }
        else
        {
            stepX     = 1;
            sideDistX = (mapX + 1.0 - rc->posX) * deltaDistX;
        }

        if (rayDirY < 0)
        {
            stepY     = -1;
            sideDistY = (rc->posY - mapY) * deltaDistY;
        }
        else
        {
            stepY     = 1;
            sideDistY = (mapY + 1.0 - rc->posY) * deltaDistY;
        }

        // perform DDA
        while (hit == 0)
        {
            // jump to next map square, OR in x-direction, OR in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            // Check if ray has hit a wall
            if (MAP_TILE(mapX, mapY) <= WMT_C)
            {
                hit = 1;
            }
        }

        // Calculate distance projected on camera direction
        // (Euclidean distance will give fisheye effect!)
        float perpWallDist;
        if (side == 0)
        {
            perpWallDist = (mapX - rc->posX + (1 - stepX) / 2) / rayDirX;
        }
        else
        {
            perpWallDist = (mapY - rc->posY + (1 - stepY) / 2) / rayDirY;
        }

        // Calculate height of line to draw on screen
        int32_t lineHeight;
        if (0 != perpWallDist)
        {
            lineHeight = (int32_t)(TFT_HEIGHT / perpWallDist);
        }
        else
        {
            lineHeight = 0;
        }

        // calculate lowest and highest pixel to fill in current stripe
        int32_t drawStart = -lineHeight / 2 + TFT_HEIGHT / 2;
        int32_t drawEnd   = lineHeight / 2 + TFT_HEIGHT / 2;

        // Because of how drawStart and drawEnd are calculated, lineHeight needs to be recomputed
        lineHeight = drawEnd - drawStart;

        // Save a bunch of data to render the scene later
        rayResult[x].mapX         = mapX;
        rayResult[x].mapY         = mapY;
        rayResult[x].side         = side;
        rayResult[x].drawEnd      = drawEnd;
        rayResult[x].drawStart    = drawStart;
        rayResult[x].perpWallDist = perpWallDist;
        rayResult[x].rayDirX      = rayDirX;
        rayResult[x].rayDirY      = rayDirY;
    }
}

/**
 * With the data in rayResult, render all the wall textures to the scene
 *
 * @param rayResult The information for all the rays cast
 */
void drawTextures(rayResult_t* rayResult)
{
    SETUP_FOR_TURBO();
    for (int32_t x = 0; x < TFT_WIDTH; x++)
    {
        // For convenience
        uint8_t mapX      = rayResult[x].mapX;
        uint8_t mapY      = rayResult[x].mapY;
        uint8_t side      = rayResult[x].side;
        int16_t drawStart = rayResult[x].drawStart;
        int16_t drawEnd   = rayResult[x].drawEnd;

        // Only draw textures for walls and columns, not empty space or spawn points
        if (MAP_TILE(mapX, mapY) <= WMT_C)
        {
            // Make sure not to waste any draws out-of-bounds
            if (drawStart < 0)
            {
                drawStart = 0;
            }
            else if (drawStart > TFT_HEIGHT)
            {
                drawStart = TFT_HEIGHT;
            }

            if (drawEnd < 0)
            {
                drawEnd = 0;
            }
            else if (drawEnd > TFT_HEIGHT)
            {
                drawEnd = TFT_HEIGHT;
            }

            // Pick a texture
            wsg_t* wallTex = NULL;
            switch (MAP_TILE(mapX, mapY))
            {
                case WMT_W1:
                {
                    wallTex = &rc->sinTex;
                    break;
                }
                case WMT_W2:
                {
                    wallTex = &rc->brickTex;
                    break;
                }
                case WMT_W3:
                {
                    wallTex = &rc->stripeTex;
                    break;
                }
                case WMT_C:
                {
                    wallTex = &rc->stoneTex;
                    break;
                }
                default:
                case WMT_E:
                case WMT_S:
                {
                    // Empty spots don't have textures
                    break;
                }
            }

            // If there's no texture to draw, continue looping
            if (NULL == wallTex)
            {
                continue;
            }

            // calculate value of wallX, where exactly the wall was hit
            float wallX;
            if (side == 0)
            {
                wallX = rc->posY + rayResult[x].perpWallDist * rayResult[x].rayDirY;
            }
            else
            {
                wallX = rc->posX + rayResult[x].perpWallDist * rayResult[x].rayDirX;
            }
            wallX -= (int32_t)(wallX);

            // X coordinate on the texture. Make sure it's in bounds
            int32_t texX = (int32_t)(wallX * wallTex->w);
            if (texX >= wallTex->w)
            {
                texX = wallTex->w - 1;
            }

            // Draw this texture's vertical stripe
            // Calculate how much to increase the texture coordinate per screen pixel
            int32_t lineHeight = rayResult[x].drawEnd - rayResult[x].drawStart;
            float step         = wallTex->h / (float)lineHeight;
            // Starting texture coordinate
            float texPos = (drawStart - TFT_HEIGHT / 2 + lineHeight / 2) * step;
            for (int32_t y = drawStart; y < drawEnd; y++)
            {
                // Y coordinate on the texture. Round it, make sure it's in bounds
                int32_t texY = (int32_t)(texPos);
                if (texY >= wallTex->h)
                {
                    texY = wallTex->h - 1;
                }

                // Increment the texture position by the step size
                texPos += step;

                // Draw the pixel specified by the texture
                TURBO_SET_PIXEL(x, y, wallTex->px[(texY * wallTex->w) + texX]);
            }
        }
    }
}

/**
 * Draw the outlines of all the walls and corners based on the cast ray info
 *
 * @param rayResult The information for all the rays cast
 */
void drawOutlines(rayResult_t* rayResult)
{
    // Set starting points
    int16_t tLineStartX = 0;
    int16_t tLineStartY = rayResult[0].drawStart;
    int16_t bLineStartX = 0;
    int16_t bLineStartY = rayResult[0].drawEnd;

    // Iterate over all but the first and last strips of pixels
    for (int32_t x = 1; x < TFT_WIDTH - 1; x++)
    {
        // See if this strip is part of the previous or next strip's wall
        bool sameWallAsPrev
            = (rayResult[x].side == rayResult[x - 1].side)
              && (ABS(rayResult[x].mapX - rayResult[x - 1].mapX) + ABS(rayResult[x].mapY - rayResult[x - 1].mapY) < 2);
        bool sameWallAsNext
            = (rayResult[x].side == rayResult[x + 1].side)
              && (ABS(rayResult[x].mapX - rayResult[x + 1].mapX) + ABS(rayResult[x].mapY - rayResult[x + 1].mapY) < 2);

        // If this is a boundary
        if (!(sameWallAsNext && sameWallAsPrev))
        {
            // Always draw a vertical stripe
            drawLineFast(x, rayResult[x].drawStart, x, rayResult[x].drawEnd, WHITE);

            // If we didn't draw a vertical stripe in the previous x index
            // Or if we are drawing in the first two pixels
            if (tLineStartX != (x - 1) || (0 == tLineStartX))
            {
                // Connect the top and bottom wall lines too
                drawLineFast(tLineStartX, tLineStartY, x, rayResult[x].drawStart, WHITE);
                drawLineFast(bLineStartX, bLineStartY, x, rayResult[x].drawEnd, WHITE);
            }

            // Save points for the next lines
            tLineStartX = x;
            tLineStartY = rayResult[x].drawStart;
            bLineStartX = x;
            bLineStartY = rayResult[x].drawEnd;
        }
    }

    // Draw the final top and bottom wall lines
    drawLineFast(tLineStartX, tLineStartY, TFT_WIDTH - 1, rayResult[TFT_WIDTH - 1].drawStart, WHITE);
    drawLineFast(bLineStartX, bLineStartY, TFT_WIDTH - 1, rayResult[TFT_WIDTH - 1].drawEnd, WHITE);
}

/**
 * Draw all the sprites
 *
 * @param rayResult The information for all the rays cast
 */
void drawSprites(rayResult_t* rayResult)
{
    SETUP_FOR_TURBO();

    // Local memory for figuring out the draw order
    int32_t spriteOrder[NUM_SPRITES];
    float spriteDistance[NUM_SPRITES];

    // Track if any sprite was shot
    int16_t spriteIdxShot = -1;

    // sort sprites from far to close
    for (uint32_t i = 0; i < NUM_SPRITES; i++)
    {
        spriteOrder[i] = i;
        // sqrt not taken, unneeded
        spriteDistance[i] = ((rc->posX - rc->sprites[i].posX) * (rc->posX - rc->sprites[i].posX)
                             + (rc->posY - rc->sprites[i].posY) * (rc->posY - rc->sprites[i].posY));
    }
    sortSprites(spriteOrder, spriteDistance, NUM_SPRITES);

    // after sorting the sprites, do the projection and draw them
    for (uint32_t i = 0; i < NUM_SPRITES; i++)
    {
        // Skip over the sprite if posX is negative
        if (rc->sprites[spriteOrder[i]].posX < 0)
        {
            continue;
        }
        // translate sprite position to relative to camera
        float spriteX = rc->sprites[spriteOrder[i]].posX - rc->posX;
        float spriteY = rc->sprites[spriteOrder[i]].posY - rc->posY;

        // transform sprite with the inverse camera matrix
        // [ planeX dirX ] -1                                  [ dirY     -dirX ]
        // [             ]    =  1/(planeX*dirY-dirX*planeY) * [                ]
        // [ planeY dirY ]                                     [ -planeY planeX ]

        // required for correct matrix multiplication
        float invDet = 1.0 / (rc->planeX * rc->dirY - rc->dirX * rc->planeY);

        float transformX = invDet * (rc->dirY * spriteX - rc->dirX * spriteY);
        // this is actually the depth inside the screen, that what Z is in 3D
        float transformY = invDet * (-rc->planeY * spriteX + rc->planeX * spriteY);

        // If this is negative, the texture isn't going to be drawn, so just stop here
        if (transformY < 0)
        {
            continue;
        }

        int32_t spriteScreenX = (int32_t)((TFT_WIDTH / 2) * (1 + transformX / transformY));

        // calculate height of the sprite on screen
        // using 'transformY' instead of the real distance prevents fisheye
        int32_t spriteHeight = (int32_t)(TFT_HEIGHT / (transformY));
        if (spriteHeight < 0)
        {
            spriteHeight = -spriteHeight;
        }

        // calculate lowest and highest pixel to fill in current stripe
        int32_t drawStartY = -spriteHeight / 2 + TFT_HEIGHT / 2;
        if (drawStartY < 0)
        {
            drawStartY = 0;
        }

        int32_t drawEndY = spriteHeight / 2 + TFT_HEIGHT / 2;
        if (drawEndY > TFT_HEIGHT)
        {
            drawEndY = TFT_HEIGHT;
        }

        // calculate width of the sprite
        int32_t spriteWidth = ((int32_t)(TFT_HEIGHT / (transformY)));
        if (spriteWidth < 0)
        {
            spriteWidth = -spriteWidth;
        }
        int32_t drawStartX = -spriteWidth / 2 + spriteScreenX;
        if (drawStartX < 0)
        {
            drawStartX = 0;
        }
        int32_t drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawEndX > TFT_WIDTH)
        {
            drawEndX = TFT_WIDTH;
        }

        // Get a reference to the texture
        wsg_t* spriteTex = rc->sprites[spriteOrder[i]].texture;

        // loop through every vertical stripe of the sprite on screen
        for (int32_t stripe = drawStartX; stripe < drawEndX; stripe++)
        {
            int32_t texX
                = (int32_t)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * spriteTex->w / spriteWidth) / 256;
            // the conditions in the if are:
            // 1) it's in front of camera plane so you don't see things behind you
            // 2) it's on the screen (left)
            // 3) it's on the screen (right)
            // 4) ZBuffer, with perpendicular distance
            if (transformY < rayResult[stripe].perpWallDist)
            {
                // for every pixel of the current stripe
                for (int32_t y = drawStartY; y < drawEndY; y++)
                {
                    // 256 and 128 factors to avoid floats
                    int32_t d    = (y)*256 - TFT_HEIGHT * 128 + spriteHeight * 128;
                    int32_t texY = ((d * spriteTex->h) / spriteHeight) / 256;
                    // get current color from the texture
                    uint16_t texIdx = 0;

                    // If the sprite is mirrored, get the mirrored idx
                    if (rc->sprites[spriteOrder[i]].mirror)
                    {
                        texIdx = (texY * spriteTex->w) + (spriteTex->w - texX - 1);
                    }
                    else
                    {
                        texIdx = (texY * spriteTex->w) + texX;
                    }

                    // draw the pixel for the texture, maybe inverted
                    paletteColor_t pxColor = spriteTex->px[texIdx];
                    if (rc->sprites[spriteOrder[i]].invincibilityTimer > 0
                        && (rc->sprites[spriteOrder[i]].invincibilityTimer % INVINCIBILITY_HZ > (INVINCIBILITY_HZ / 2)))
                    {
                        switch ((int)pxColor)
                        {
                            case BLACK:
                            {
                                TURBO_SET_PIXEL(stripe, y, WHITE);
                                break;
                            }
                            case WHITE:
                            {
                                TURBO_SET_PIXEL(stripe, y, BLACK);
                                break;
                            }
                            default:
                            {
                                // These don't draw
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (cTransparent != pxColor)
                        {
                            TURBO_SET_PIXEL(stripe, y, pxColor);
                        }
                    }

                    // If we should check a shot, and a sprite is centered
                    if (true == rc->checkShot && (stripe == 63 || stripe == 64)
                        && rc->sprites[spriteOrder[i]].health > 0)
                    {
                        // Mark that sprite as shot
                        spriteIdxShot = spriteOrder[i];
                    }
                }
            }
        }
    }

    // If you shot something
    if (spriteIdxShot >= 0)
    {
        // And it's fewer than six units away
        float distSqr = ((rc->sprites[spriteIdxShot].posX - rc->posX) * (rc->sprites[spriteIdxShot].posX - rc->posX))
                        + ((rc->sprites[spriteIdxShot].posY - rc->posY) * (rc->sprites[spriteIdxShot].posY - rc->posY));
        if (distSqr < GUITAR_SHOT_RANGE)
        {
            // And it's not already getting shot or dead or invincible
            if (rc->sprites[spriteIdxShot].state != E_GOT_SHOT && rc->sprites[spriteIdxShot].state != E_DEAD
                && rc->sprites[spriteIdxShot].invincibilityTimer <= 0)
            {
                // Calculate damage based on distance
                uint8_t damage = 1 + ((GUITAR_SHOT_RANGE - distSqr) / DAMAGE_DIVISOR);
                if (damage > rc->sprites[spriteIdxShot].health)
                {
                    damage = rc->sprites[spriteIdxShot].health;
                }
                // decrement the health
                rc->sprites[spriteIdxShot].health -= damage;
                // Animate getting shot
                setSpriteState(&(rc->sprites[spriteIdxShot]), E_GOT_SHOT);
                // Flash LEDs that we shot something
                rc->shotSomethingTimer = LED_ON_TIME;

                // If this is a really close shot
                if (distSqr <= CRITICAL_DISTANCE)
                {
                    // Count it as a critical hit
                    rc->isCriticalHit = true;
                    // Which gains some health
                    rc->health += HEALTH_PER_CRITICAL;
                }
                else
                {
                    // Otherwise it's normal
                    rc->isCriticalHit = false;
                }
            }
        }
    }

    // Shot was checked, so clear the boolean
    rc->checkShot = false;
}

/**
 * Bubble sort algorithm to which sorts both order and dist by the values in dist
 *
 * @param order  Sprite indices to be sorted by dist
 * @param dist   The distances from the camera to the sprites
 * @param amount The number of values to sort
 */
void sortSprites(int32_t* order, float* dist, int32_t amount)
{
    // Iterate over everything
    for (int32_t i = 0; i < amount - 1; i++)
    {
        // Last i elements are already in place
        for (int32_t j = 0; j < amount - i - 1; j++)
        {
            // If the smaller distance is first
            if (dist[j] < dist[j + 1])
            {
                // Swap the dist[] elements
                float tmp   = dist[j];
                dist[j]     = dist[j + 1];
                dist[j + 1] = tmp;

                // And also swap the order[] elements
                int32_t tmp2 = order[j];
                order[j]     = order[j + 1];
                order[j + 1] = tmp2;
            }
        }
    }
}

/**
 * Update the camera position based on the current button state
 * Also handle firing the gun
 *
 * @param tElapsedUs The time elapsed since the last call
 */
void handleRayInput(uint32_t tElapsedUs)
{
    // Double tap means strafe
    static uint8_t buttonStateLast     = 0;
    static int32_t strafeTmr[2]        = {0};
    static uint8_t strafeBtn[2]        = {0};
    static const uint8_t strafeMasks[] = {PB_LEFT, PB_RIGHT};

    // Run double tap detection for left and right buttons
    for (int i = 0; i < 2; i++)
    {
        // Run down strafe timer
        if (strafeTmr[i] > 0)
        {
            strafeTmr[i] -= tElapsedUs;
        }
        else
        {
            // Clear all taps
            strafeBtn[i] = 0;
        }

        // Check for presses and releases
        if (!(buttonStateLast & strafeMasks[i]) && (rc->rButtonState & strafeMasks[i]))
        {
            // Button press
            if (0 >= strafeTmr[i])
            {
                // First tap
                strafeTmr[i] = STRAFE_BUTTON_TIMEOUT;
                strafeBtn[i] = 0;
            }
        }
        else if ((buttonStateLast & strafeMasks[i]) && !(rc->rButtonState & strafeMasks[i]))
        {
            // Button release
            if (strafeTmr[i] > 0)
            {
                // Button released while timer is active, count as a tap
                strafeBtn[i]++;
                // If there are two taps
                if (2 == strafeBtn[i])
                {
                    // Strafe!
                    strafeTmr[i] = 0;
                    strafeBtn[i] = 0;

                    if (PB_LEFT == strafeMasks[i])
                    {
                        rc->strafeLeftTmr = STRAFE_TIME;
                    }
                    else
                    {
                        rc->strafeRightTmr = STRAFE_TIME;
                    }

                    // Also mark any shots as dodged
                    for (uint8_t spr = 0; spr < NUM_SPRITES; spr++)
                    {
                        // Skip over sprites with negative position, these weren't spawned
                        if ((rc->sprites[spr].posX >= 0) && (E_SHOOTING == rc->sprites[spr].state))
                        {
                            rc->sprites[spr].shotWillMiss = true;
                        }
                    }
                }
            }
        }
    }

    // Save this button state to detect taps later
    buttonStateLast = rc->rButtonState;

    float frameTime = (tElapsedUs) / USEC_IN_SEC;

    // speed modifiers
    float moveSpeed;
    float rotSpeed;

    switch (rc->difficulty)
    {
        default:
        case RC_NUM_DIFFICULTIES:
        case RC_EASY:
        {
            moveSpeed = frameTime * PLAYER_MOVE_SPEED_E;
            rotSpeed  = frameTime * PLAYER_ROT_SPEED_E;
            break;
        }
        case RC_MED:
        {
            moveSpeed = frameTime * PLAYER_MOVE_SPEED_M;
            rotSpeed  = frameTime * PLAYER_ROT_SPEED_M;
            break;
        }
        case RC_HARD:
        {
            moveSpeed = frameTime * PLAYER_MOVE_SPEED_H;
            rotSpeed  = frameTime * PLAYER_ROT_SPEED_H;
            break;
        }
    }

    // move forward if no wall in front of you
    if (rc->rButtonState & PB_UP)
    {
        if (MAP_TILE((int32_t)(rc->posX + rc->dirX * moveSpeed), (int32_t)(rc->posY)) > WMT_C)
        {
            rc->posX += rc->dirX * moveSpeed;
        }
        if (MAP_TILE((int32_t)(rc->posX), (int32_t)(rc->posY + rc->dirY * moveSpeed)) > WMT_C)
        {
            rc->posY += rc->dirY * moveSpeed;
        }
    }

    // move backwards if no wall behind you
    if (rc->rButtonState & PB_DOWN)
    {
        if (MAP_TILE((int32_t)(rc->posX - rc->dirX * moveSpeed), (int32_t)(rc->posY)) > WMT_C)
        {
            rc->posX -= rc->dirX * moveSpeed;
        }
        if (MAP_TILE((int32_t)(rc->posX), (int32_t)(rc->posY - rc->dirY * moveSpeed)) > WMT_C)
        {
            rc->posY -= rc->dirY * moveSpeed;
        }
    }

    // Strafe left
    if (rc->strafeLeftTmr > 0)
    {
        if (MAP_TILE((int32_t)(rc->posX - rc->dirY * moveSpeed), (int32_t)(rc->posY)) > WMT_C)
        {
            rc->posX -= rc->dirY * moveSpeed;
        }
        if (MAP_TILE((int32_t)(rc->posX), (int32_t)(rc->posY + rc->dirX * moveSpeed)) > WMT_C)
        {
            rc->posY += rc->dirX * moveSpeed;
        }

        rc->strafeLeftTmr -= tElapsedUs;
    }

    // Strafe right
    if (rc->strafeRightTmr > 0)
    {
        if (MAP_TILE((int32_t)(rc->posX + rc->dirY * moveSpeed), (int32_t)(rc->posY)) > WMT_C)
        {
            rc->posX += rc->dirY * moveSpeed;
        }
        if (MAP_TILE((int32_t)(rc->posX), (int32_t)(rc->posY - rc->dirX * moveSpeed)) > WMT_C)
        {
            rc->posY -= rc->dirX * moveSpeed;
        }

        rc->strafeRightTmr -= tElapsedUs;
    }

    // rotate to the right
    if (rc->rButtonState & PB_RIGHT)
    {
        // both camera direction and camera plane must be rotated
        float oldDirX   = rc->dirX;
        rc->dirX        = rc->dirX * cos(-rotSpeed) - rc->dirY * sin(-rotSpeed);
        rc->dirY        = oldDirX * sin(-rotSpeed) + rc->dirY * cos(-rotSpeed);
        float oldPlaneX = rc->planeX;
        rc->planeX      = rc->planeX * cos(-rotSpeed) - rc->planeY * sin(-rotSpeed);
        rc->planeY      = oldPlaneX * sin(-rotSpeed) + rc->planeY * cos(-rotSpeed);
    }

    // rotate to the left
    if (rc->rButtonState & PB_LEFT)
    {
        // both camera direction and camera plane must be rotated
        float oldDirX   = rc->dirX;
        rc->dirX        = rc->dirX * cos(rotSpeed) - rc->dirY * sin(rotSpeed);
        rc->dirY        = oldDirX * sin(rotSpeed) + rc->dirY * cos(rotSpeed);
        float oldPlaneX = rc->planeX;
        rc->planeX      = rc->planeX * cos(rotSpeed) - rc->planeY * sin(rotSpeed);
        rc->planeY      = oldPlaneX * sin(rotSpeed) + rc->planeY * cos(rotSpeed);
    }

    // Decrement the shot cooldown
    if (rc->shotCooldown > 0)
    {
        rc->shotCooldown -= tElapsedUs;
    }
    else
    {
        rc->shotCooldown = 0;
    }

    // Static bool keeps track of trigger state to require button releases to shoot
    static bool triggerPulled = false;

    // If the trigger hasn't been pulled, and the button was pressed, and cooldown isn't active
    if (!triggerPulled && (rc->rButtonState & PB_A) && 0 == rc->shotCooldown)
    {
        // Set the cooldown, tell drawSprites() to check the shot, and pull the trigger
        rc->shotCooldown = PLAYER_SHOT_COOLDOWN;
        rc->checkShot    = true;
        // Assume it's not a critical hit (misses aren't critical!)
        rc->isCriticalHit = false;
        triggerPulled     = true;
    }
    else if (triggerPulled && (rc->rButtonState & PB_A) == 0)
    {
        // If the button was released, release the trigger
        triggerPulled = false;
    }
}

/**
 * Fast inverse square root. Works like magic. Ignore the warning
 *
 * See: https://en.wikipedia.org/wiki/Fast_inverse_square_root
 *
 * @param number The number to find 1/sqrt(x) for
 * @return 1/sqrt(x), or close enough
 */
float Q_rsqrt(float number)
{
    const float x2         = number * 0.5F;
    const float threeHalfs = 1.5F;

    union
    {
        float f;
        unsigned long i;
    } conv = {.f = number};
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= (threeHalfs - (x2 * conv.f * conv.f));
    return conv.f;
}

/**
 * Move all sprites around and govern enemy behavior
 *
 * @param tElapsed The time elapsed since this was last called
 */
void moveEnemies(uint32_t tElapsedUs)
{
    float frameTimeSec = (tElapsedUs) / USEC_IN_SEC;

    // Keep track of the closest live sprite
    rc->closestDist     = 0xFFFFFFFF;
    rc->closestAngle    = 0;
    rc->radarObstructed = false;

    // Figure out the movement speed for this frame
    float moveSpeed;
    switch (rc->difficulty)
    {
        default:
        case RC_NUM_DIFFICULTIES:
        case RC_EASY:
        {
            moveSpeed = frameTimeSec * SPRITE_MOVE_SPEED_E;
            break;
        }
        case RC_MED:
        {
            moveSpeed = frameTimeSec * SPRITE_MOVE_SPEED_M;
            break;
        }
        case RC_HARD:
        {
            moveSpeed = frameTimeSec * SPRITE_MOVE_SPEED_H;
            break;
        }
    }

    // For each sprite
    int16_t closestIdx = -1;
    for (uint8_t i = 0; i < NUM_SPRITES; i++)
    {
        // Skip over sprites with negative position, these weren't spawned
        if (rc->sprites[i].posX < 0)
        {
            continue;
        }

        // Always run down the sprite's shot cooldown, regardless of state
        if (rc->sprites[i].shotCooldown > 0)
        {
            rc->sprites[i].shotCooldown -= tElapsedUs;
        }

        // Always run down the sprite's invincibility timer, regardless of state
        if (rc->sprites[i].invincibilityTimer > 0)
        {
            rc->sprites[i].invincibilityTimer -= tElapsedUs;
        }

        // Find the distance between this sprite and the player, generally useful
        float toPlayerX = rc->posX - rc->sprites[i].posX;
        float toPlayerY = rc->posY - rc->sprites[i].posY;
        float magSqr    = (toPlayerX * toPlayerX) + (toPlayerY * toPlayerY);

        // Keep track of the closest live sprite
        if (rc->sprites[i].health > 0 && (uint32_t)magSqr < rc->closestDist)
        {
            rc->closestDist = (uint32_t)magSqr;
            closestIdx      = i;
        }

        switch (rc->sprites[i].state)
        {
            default:
            case E_IDLE:
            {
                // If the sprite iss less than 8 units away (avoid the sqrt!)
                if (magSqr < 64)
                {
                    // Wake it up and have it hunt the player
                    setSpriteState(&(rc->sprites[i]), E_PICK_DIR_PLAYER);
                }
                break;
            }
            case E_PICK_DIR_RAND:
            {
                // Walking randomly is good if the sprite can't make a valid move
                // Get a random unit vector in the first quadrant
                rc->sprites[i].dirX = (esp_random() % 90) / 90.0f;
                rc->sprites[i].dirY = 1 / Q_rsqrt(1 - (rc->sprites[i].dirX * rc->sprites[i].dirX));

                // Randomize the unit vector quadrant
                switch (esp_random() % 4)
                {
                    default:
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        rc->sprites[i].dirX = -rc->sprites[i].dirX;
                        break;
                    }
                    case 2:
                    {
                        rc->sprites[i].dirY = -rc->sprites[i].dirY;
                        break;
                    }
                    case 3:
                    {
                        rc->sprites[i].dirX = -rc->sprites[i].dirX;
                        rc->sprites[i].dirY = -rc->sprites[i].dirY;
                        break;
                    }
                }

                // Have the sprite move forward
                rc->sprites[i].isBackwards = false;

                // And let the sprite walk for a bit
                setSpriteState(&(rc->sprites[i]), E_WALKING);
                // Walk for a little extra in the random direction
                rc->sprites[i].stateTimer = LONG_WALK_ANIM_TIME;
                break;
            }
            case E_PICK_DIR_PLAYER:
            {
                // Either pick a direction to walk towards the player, or take a shot

                // Randomly take a shot
                if (rc->sprites[i].shotCooldown <= 0 &&       // If the sprite is not in cooldown
                    (uint32_t)magSqr < 64 &&                  // If the player is no more than 8 units away
                    ((esp_random() % 64) > (uint32_t)magSqr)) // And distance-weighted RNG says so
                {
                    // Take the shot!
                    setSpriteState(&(rc->sprites[i]), E_SHOOTING);
                }
                else // Pick a direction to walk in
                {
                    // Normalize the vector
                    float invSqr = Q_rsqrt(magSqr);
                    toPlayerX *= invSqr;
                    toPlayerY *= invSqr;

                    // Rotate the vector, maybe
                    switch (esp_random() % 3)
                    {
                        default:
                        case 0:
                        {
                            // Straight ahead!
                            break;
                        }
                        case 1:
                        {
                            // Rotate 45 degrees
                            toPlayerX = toPlayerX * PI_DIV_4 - toPlayerY * PI_DIV_4;
                            toPlayerY = toPlayerY * PI_DIV_4 + toPlayerX * PI_DIV_4;
                            break;
                        }
                        case 2:
                        {
                            // Rotate -45 degrees
                            toPlayerX = toPlayerX * PI_DIV_4 + toPlayerY * PI_DIV_4;
                            toPlayerY = toPlayerY * PI_DIV_4 - toPlayerX * PI_DIV_4;
                            break;
                        }
                    }

                    // Set the sprite's direction
                    rc->sprites[i].dirX        = toPlayerX;
                    rc->sprites[i].dirY        = toPlayerY;
                    rc->sprites[i].isBackwards = false;

                    // And let the sprite walk for a bit
                    setSpriteState(&(rc->sprites[i]), E_WALKING);
                }
                break;
            }
            case E_WALKING:
            {
                // This is just when the sprite is walking in a straight line

                // See if we're done walking
                rc->sprites[i].stateTimer -= tElapsedUs;
                if (rc->sprites[i].stateTimer <= 0)
                {
                    // When done walking, pick a new direction
                    setSpriteState(&(rc->sprites[i]), E_PICK_DIR_PLAYER);
                    break;
                }

                // Mirror sprites for a walking animation
                rc->sprites[i].texTimer -= tElapsedUs;
                while (rc->sprites[i].texTimer < 0)
                {
                    // Animate a 'step'
                    rc->sprites[i].texTimer += STEP_ANIM_TIME;
                    // Pick the next texture
                    rc->sprites[i].texFrame = (rc->sprites[i].texFrame + 1) % NUM_WALK_FRAMES;
                    rc->sprites[i].texture  = &rc->walk[rc->sprites[i].texFrame];
                    // Mirror the texture if we're back to 0
                    if (0 == rc->sprites[i].texFrame)
                    {
                        rc->sprites[i].mirror = !rc->sprites[i].mirror;
                    }
                }

                // Find the new position
                // If the sprite is really close and not walking backwards
                if (magSqr < 0.5f && false == rc->sprites[i].isBackwards)
                {
                    // walk backwards instead
                    rc->sprites[i].dirX        = -rc->sprites[i].dirX;
                    rc->sprites[i].dirY        = -rc->sprites[i].dirY;
                    rc->sprites[i].isBackwards = true;
                }
                // If the sprite is far enough away and walking backwards
                else if (magSqr > 1.5f && true == rc->sprites[i].isBackwards)
                {
                    // Walk forwards again
                    rc->sprites[i].dirX        = -rc->sprites[i].dirX;
                    rc->sprites[i].dirY        = -rc->sprites[i].dirY;
                    rc->sprites[i].isBackwards = false;
                }

                // Find the new position
                float newPosX = rc->sprites[i].posX + (rc->sprites[i].dirX * moveSpeed);
                float newPosY = rc->sprites[i].posY + (rc->sprites[i].dirY * moveSpeed);

                // Integer-ify it
                int newPosXi = newPosX;
                int newPosYi = newPosY;

                // Make sure the new position is in bounds, assume invalid
                bool moveIsValid = false;
                if ((0 <= newPosXi && newPosXi < rc->mapW) && (0 <= newPosYi && newPosYi < rc->mapH) &&
                    // And that it's not occupied by a wall or column
                    (MAP_TILE(newPosXi, newPosYi) > WMT_C))
                {
                    // In bounds, so it's valid so far
                    moveIsValid = true;

                    // Make sure the new square is unoccupied
                    for (uint8_t oth = 0; oth < NUM_SPRITES; oth++)
                    {
                        // If some other sprite is in the new square
                        if ((oth != i) && (int)rc->sprites[oth].posX == newPosXi
                            && (int)rc->sprites[oth].posY == newPosYi)
                        {
                            // Occupied, so this move is invalid
                            moveIsValid = false;
                            break;
                        }
                    }
                }

                // If the move is valid, move there
                if (moveIsValid)
                {
                    rc->sprites[i].posX = newPosX;
                    rc->sprites[i].posY = newPosY;
                }
                else
                {
                    // Else pick a new direction, totally random
                    setSpriteState(&(rc->sprites[i]), E_PICK_DIR_RAND);
                }
                break;
            }
            case E_SHOOTING:
            {
                // Animate a shot
                rc->sprites[i].texTimer -= tElapsedUs;
                while (rc->sprites[i].texTimer < 0)
                {
                    // Hard mode is twice as fast
                    if (RC_HARD == rc->difficulty)
                    {
                        rc->sprites[i].texTimer += (SHOOTING_ANIM_TIME / 2);
                    }
                    else
                    {
                        rc->sprites[i].texTimer += SHOOTING_ANIM_TIME;
                    }

                    // Pick the next texture
                    rc->sprites[i].texFrame = (rc->sprites[i].texFrame + 1) % NUM_SHOT_FRAMES;
                    rc->sprites[i].texture  = &rc->shooting[rc->sprites[i].texFrame];

                    // Reset if we're back to zero
                    if (0 == rc->sprites[i].texFrame)
                    {
                        // Actually take the shot
                        rc->sprites[i].shotCooldown = ENEMY_SHOT_COOLDOWN;
                        // Half cooldown for hard mode
                        if (RC_HARD == rc->difficulty)
                        {
                            rc->sprites[i].shotCooldown /= 2;
                        }

                        // Check if the player hasn't strafed, and if the sprite can still see the player
                        if (false == rc->sprites[i].shotWillMiss
                            && checkWallsBetweenPoints(rc->sprites[i].posX, rc->sprites[i].posY, rc->posX, rc->posY))
                        {
                            // If it can, the player got shot
                            uint8_t damage = 1 + ((GUITAR_SHOT_RANGE - magSqr) / DAMAGE_DIVISOR);
                            if (damage > rc->health)
                            {
                                damage = rc->health;
                            }
                            rc->health -= damage;
                            rc->gotShotTimer  = LED_ON_TIME;
                            rc->shotFromAngle = angleBetween(rc->posX, rc->posY, rc->dirX, rc->dirY,
                                                             rc->sprites[i].posX, rc->sprites[i].posY);

                            // If the player is out of health
                            if (rc->health == 0)
                            {
                                // End the round
                                shooterEndRound();
                            }
                        }

                        // After the shot, go to E_PICK_DIR_PLAYER
                        setSpriteState(&(rc->sprites[i]), E_PICK_DIR_PLAYER);
                    }
                }
                break;
            }
            case E_GOT_SHOT:
            {
                // Animate getting shot
                rc->sprites[i].texTimer -= tElapsedUs;
                while (rc->sprites[i].texTimer < 0)
                {
                    // Hard mode is twice as fast
                    if (RC_HARD == rc->difficulty)
                    {
                        rc->sprites[i].texTimer += (GOT_SHOT_ANIM_TIME / 2);
                    }
                    else
                    {
                        rc->sprites[i].texTimer += GOT_SHOT_ANIM_TIME;
                    }

                    // Pick the next texture
                    rc->sprites[i].texFrame = (rc->sprites[i].texFrame + 1) % NUM_HURT_FRAMES;
                    rc->sprites[i].texture  = &rc->hurt[rc->sprites[i].texFrame];

                    // Reset if we're back to zero
                    if (0 == rc->sprites[i].texFrame)
                    {
                        // If the enemy has any health
                        if (rc->sprites[i].health > 0)
                        {
                            // Go back to E_PICK_DIR_PLAYER
                            setSpriteState(&(rc->sprites[i]), E_PICK_DIR_PLAYER);
                        }
                        else
                        {
                            // If there is no health, go to E_DEAD
                            setSpriteState(&(rc->sprites[i]), E_DEAD);
                        }
                    }
                }
                break;
            }
            case E_DEAD:
            {
                // Do nothing, ya dead
                break;
            }
        }
    }

    // If there is a nearby sprite
    if (closestIdx >= 0)
    {
        rc->closestAngle = angleBetween(rc->posX, rc->posY, rc->dirX, rc->dirY, rc->sprites[closestIdx].posX,
                                        rc->sprites[closestIdx].posY);
        rc->radarObstructed
            = checkWallsBetweenPoints(rc->posX, rc->posY, rc->sprites[closestIdx].posX, rc->sprites[closestIdx].posY);
    }
}

/**
 * Find the angle between a vector (position, direction) and a point
 * This is used to run the radar
 *
 * @param pPosX The X position of the vector (player)
 * @param pPosY The Y position of the vector
 * @param pDirX The X direction of the vector (must be a unit vector (mag==1))
 * @param pDirY The Y direction of the vector
 * @param sPosX The X position of the target point (sprite)
 * @param sPosY The Y position of the target point
 * @return float
 */
float angleBetween(float pPosX, float pPosY, float pDirX, float pDirY, float sPosX, float sPosY)
{
    // Find the angle between the 'straight ahead' vector and the nearest sprite
    // Use pDirX, pDirY as vector straight ahead
    // This is the vector to the closest sprite
    float xClosest = (sPosX - pPosX);
    float yClosest = (sPosY - pPosY);

    // Find the angle between the two vectors
    float closestAngle = acosf(((pDirX * xClosest) + (pDirY * yClosest)) *              // Dot Product
                               Q_rsqrt((xClosest * xClosest) + (yClosest * yClosest))); // (1 / distance to sprite)
    // The magnitude of the 'straight ahead' vector is always 1, so it's left out of the denominator

    // Check if the sprite is to the left or right of us
    if (((pDirX) * (yClosest) - (pDirY) * (xClosest)) > 0)
    {
        // If it's to the left, rotate the angle to the pi->2*pi range
        closestAngle = (2 * M_PI) - closestAngle;
    }
    return closestAngle;
}

/**
 * Use DDA to draw a line between the player and a sprite, and check if there
 * are any walls between the two. This line drawing algorithm is the same one
 * used to cast rays from the player to walls
 *
 * @param sprite The sprite to draw a line from
 * @param pX     The player's X position
 * @param pY     The player's Y position
 * @return true  if there is a clear line between the player and sprite,
 *         false if there is an obstruction
 */
bool checkWallsBetweenPoints(float sX, float sY, float pX, float pY)
{
    // If the sprite and the player are in the same cell
    if (((int32_t)sX == (int32_t)pX) && ((int32_t)sY == (int32_t)pY))
    {
        // We can definitely draw a line between the two
        return true;
    }

    // calculate ray position and direction
    // x-coordinate in camera space
    float rayDirX = sX - pX;
    float rayDirY = sY - pY;

    // which box of the map we're in
    int32_t mapX = (int32_t)(pX);
    int32_t mapY = (int32_t)(pY);

    // length of ray from current position to next x or y-side
    float sideDistX;
    float sideDistY;

    // length of ray from one x or y-side to next x or y-side
    float deltaDistX = (1 / rayDirX);
    if (deltaDistX < 0)
    {
        deltaDistX = -deltaDistX;
    }

    float deltaDistY = (1 / rayDirY);
    if (deltaDistY < 0)
    {
        deltaDistY = -deltaDistY;
    }

    // what direction to step in x or y-direction (either +1 or -1)
    int32_t stepX;
    int32_t stepY;

    // calculate step and initial sideDist
    if (rayDirX < 0)
    {
        stepX     = -1;
        sideDistX = (pX - mapX) * deltaDistX;
    }
    else
    {
        stepX     = 1;
        sideDistX = (mapX + 1.0 - pX) * deltaDistX;
    }

    if (rayDirY < 0)
    {
        stepY     = -1;
        sideDistY = (pY - mapY) * deltaDistY;
    }
    else
    {
        stepY     = 1;
        sideDistY = (mapY + 1.0 - pY) * deltaDistY;
    }

    // perform DDA until a wall is hit or the ray reaches the sprite
    while (true)
    {
        // jump to next map square, OR in x-direction, OR in y-direction
        if (sideDistX < sideDistY)
        {
            sideDistX += deltaDistX;
            mapX += stepX;
        }
        else
        {
            sideDistY += deltaDistY;
            mapY += stepY;
        }

        // Check if ray has hit a wall
        if (MAP_TILE(mapX, mapY) <= WMT_C)
        {
            // There is a wall between the player and the sprite
            return false;
        }
        else if (mapX == (int32_t)sX && mapY == (int32_t)sY)
        {
            // Ray reaches from the player to the sprite unobstructed
            return true;
        }
    }
    // Shouldn't reach here
    return false;
}

/**
 * Set the sprite state and associated timers and textures
 *
 * @param sprite The sprite to set state for
 * @param state  The state to set
 */
void setSpriteState(raySprite_t* sprite, enemyState_t state)
{
    // See if the sprite had been in a walking state
    // When transitioning between 'walking' states, don't reset texture indices
    bool wasWalking;
    switch (sprite->state)
    {
        default:
        case E_IDLE:
        case E_PICK_DIR_PLAYER:
        case E_PICK_DIR_RAND:
        case E_WALKING:
        {
            wasWalking = true;
            break;
        }
        case E_SHOOTING:
        case E_GOT_SHOT:
        case E_DEAD:
        {
            wasWalking = false;
        }
    }

    // Set the state, reset the texture frame
    sprite->state    = state;
    sprite->texFrame = 0;

    // Set timers and textures
    switch (state)
    {
        default:
        case E_IDLE:
        case E_PICK_DIR_PLAYER:
        case E_PICK_DIR_RAND:
        {
            // These state are transient, so the timer is 0
            sprite->stateTimer = 0;
            // If the sprite wasn't walking before, or isn't initialized
            if (!wasWalking || NULL == sprite->texture)
            {
                // Set up the walking texture
                sprite->texture  = &rc->walk[0];
                sprite->mirror   = false;
                sprite->texTimer = 0;
            }
            break;
        }
        case E_WALKING:
        {
            // Walk for WALK_ANIM_TIME
            sprite->stateTimer = WALK_ANIM_TIME;
            // If the sprite wasn't walking before, or isn't initialized
            if (!wasWalking || NULL == sprite->texture)
            {
                // Set up the walking texture
                sprite->texture  = &rc->walk[0];
                sprite->mirror   = false;
                sprite->texTimer = STEP_ANIM_TIME;
            }
            break;
        }
        case E_SHOOTING:
        {
            // Set up the shooting texture
            sprite->stateTimer = SHOOTING_ANIM_TIME;
            // Hard mode is twice as fast
            if (RC_HARD == rc->difficulty)
            {
                sprite->stateTimer /= 2;
            }
            sprite->texture  = &rc->shooting[0];
            sprite->texTimer = sprite->stateTimer;

            // Shot won't miss unless the player strafes
            sprite->shotWillMiss = false;

            // Show a warning that a shot is coming from this angle
            rc->warningShotTimer = LED_ON_TIME;
            rc->warningShotAngle = angleBetween(rc->posX, rc->posY, rc->dirX, rc->dirY, sprite->posX, sprite->posY);
            break;
        }
        case E_GOT_SHOT:
        {
            // Set up the got shot texture
            sprite->stateTimer = GOT_SHOT_ANIM_TIME;
            // Hard mode is twice as fast
            if (RC_HARD == rc->difficulty)
            {
                sprite->stateTimer /= 2;
            }
            sprite->texture  = &rc->hurt[0];
            sprite->texTimer = sprite->stateTimer;
            // Set some invincibility frames
            sprite->invincibilityTimer = INVINCIBILITY_TIME;
            break;
        }
        case E_DEAD:
        {
            // If the sprite is dead, decrement live sprite and increment kills
            rc->liveSprites--;
            rc->killedSpriteTimer = LED_ON_TIME;
            rc->kills++;
            // If there are no sprites left
            if (0 == rc->liveSprites)
            {
                // End the round
                shooterEndRound();
            }

            // ALso set up the corpse texture
            sprite->stateTimer         = 0;
            sprite->texture            = &rc->dead;
            sprite->texTimer           = 0;
            sprite->invincibilityTimer = 0;
        }
    }
}

/**
 * Draw the HUD, including the weapon and health
 */
void drawHUD(void)
{
    // Figure out width for note display
    char notes[8] = {0};
    snprintf(notes, sizeof(notes) - 1, "%d", rc->liveSprites);
    int16_t noteWidth = textWidth(&rc->ibm, notes);

    // Clear area behind note display
    fillDisplayArea(0, TFT_HEIGHT - rc->mNote.h, noteWidth + rc->mNote.w + 1, TFT_HEIGHT, BLACK);

    // Draw note display
    if (rc->killedSpriteTimer <= 0)
    {
        // Blink the note when an enemy is defeated
        drawWsgSimple(&rc->mNote, 0, TFT_HEIGHT - rc->mNote.h);
    }
    else
    {
        // Invert this texture
        // TODO Draw something different
        drawWsgSimple(&rc->mNote, 0, TFT_HEIGHT - rc->mNote.h);
    }
    drawText(&rc->ibm, WHITE, notes, rc->mNote.w + 2, TFT_HEIGHT - rc->ibm.height - 1);

    // Figure out width for health display
    char health[8] = {0};
    snprintf(health, sizeof(health) - 1, "%" PRId32, rc->health);
    int16_t healthWidth = textWidth(&rc->ibm, health);
    int16_t healthDrawX = TFT_WIDTH - rc->heart.w - 1 - healthWidth;

    // Clear area behind health display
    fillDisplayArea(healthDrawX - 1, TFT_HEIGHT - rc->heart.h, TFT_WIDTH, TFT_HEIGHT, BLACK);

    // Draw health display
    if (rc->gotShotTimer <= 0)
    {
        // Blink the heart when getting shot, same as the red LED
        drawWsgSimple(&(rc->heart), TFT_WIDTH - rc->heart.w, TFT_HEIGHT - rc->heart.h);
    }
    else
    {
        // Invert this texture
        // TODO draw something different
        drawWsgSimple(&(rc->heart), TFT_WIDTH - rc->heart.w, TFT_HEIGHT - rc->heart.h);
    }

    drawText(&rc->ibm, WHITE, health, healthDrawX, TFT_HEIGHT - rc->ibm.height - 1);

    // Draw Guitar
    if (0 == rc->shotCooldown)
    {
        // This is just the 0th frame, not shooting
        drawWsgSimple(&rc->gtr[0], (TFT_WIDTH - rc->gtr[0].w) / 2, (TFT_HEIGHT - rc->gtr[0].h));
    }
    else
    {
        // If it's a critical hit, draw DAS BOOT, otherwise draw the guitar
        if (rc->isCriticalHit)
        {
            // Get the frame index from the shotCooldown count
            int8_t idx = (NUM_BOOT_FRAMES * (PLAYER_SHOT_COOLDOWN - rc->shotCooldown)) / PLAYER_SHOT_COOLDOWN;
            // Make sure this is in bounds
            if (idx >= NUM_BOOT_FRAMES)
            {
                idx = NUM_BOOT_FRAMES - 1;
            }
            // Draw the boot frame
            drawWsgSimple(&rc->boot[idx], (TFT_WIDTH - rc->boot[idx].w) / 2, (TFT_HEIGHT - rc->boot[idx].h));
        }
        else
        {
            // Get the frame index from the shotCooldown count
            int8_t idx = (NUM_GTR_FRAMES * (PLAYER_SHOT_COOLDOWN - rc->shotCooldown)) / PLAYER_SHOT_COOLDOWN;
            // Make sure this is in bounds
            if (idx >= NUM_GTR_FRAMES)
            {
                idx = NUM_GTR_FRAMES - 1;
            }
            // Draw the guitar frame
            drawWsgSimple(&rc->gtr[idx], (TFT_WIDTH - rc->gtr[idx].w) / 2, (TFT_HEIGHT - rc->gtr[idx].h));
        }
    }

    // Only draw clock while playing the game
    if (RC_GAME == rc->mode)
    {
        // Plot the elapsed time
        uint32_t tElapsed = esp_timer_get_time() - rc->tRoundStartedUs;
        uint32_t dSec     = (tElapsed / USEC_IN_D_SEC) % 10;
        uint32_t sec      = (tElapsed / (int)USEC_IN_SEC) % 60;
        uint32_t min      = (tElapsed / (USEC_IN_SEC * 60));
        char timeStr[64]  = {0};
        snprintf(timeStr, sizeof(timeStr) - 1, "%02" PRIu32 ":%02" PRIu32 ".%" PRIu32 "", min, sec, dSec);
        int16_t timeWidth = textWidth(&rc->tomThumb, timeStr);
        fillDisplayArea(TFT_WIDTH - timeWidth, 0, TFT_WIDTH, rc->tomThumb.height, BLACK);
        drawText(&rc->tomThumb, WHITE, timeStr, TFT_WIDTH - timeWidth + 1, 0);
    }
}

/**
 * Called when the round ends
 */
void shooterEndRound(void)
{
    // See how long this round took
    rc->tRoundElapsed = esp_timer_get_time() - rc->tRoundStartedUs;

    // Save the score
    addShooterScore(rc->difficulty, rc->mapIdx, rc->kills, rc->tRoundElapsed);

    // Show game over screen, disable radar
    rc->mode                 = RC_GAME_OVER;
    rc->gameOverButtonLockUs = GAME_OVER_BUTTON_LOCK_US;
    rc->closestDist          = 0xFFFFFFFF;
    rc->closestAngle         = 0;
    rc->radarObstructed      = false;
    rc->killedSpriteTimer    = 0;
    rc->warningShotTimer     = 0;
    rc->warningShotAngle     = 0;
    rc->gotShotTimer         = 0;
    rc->shotFromAngle        = 0;
}

/**
 * Display the time elapsed and number of kills
 *
 * @param tElapsedUs Time elapsed since the last call
 */
void shooterDrawRoundOver(uint32_t tElapsedUs)
{
    // Break down the elapsed time in microseconds to human readable numbers
    uint32_t dSec = (rc->tRoundElapsed / USEC_IN_D_SEC) % 10;
    uint32_t sec  = (rc->tRoundElapsed / (int)USEC_IN_SEC) % 60;
    uint32_t min  = (rc->tRoundElapsed / (USEC_IN_SEC * 60));

    // Create the time and kills string
    char timeStr[64] = {0};
    snprintf(timeStr, sizeof(timeStr), "Time: %02" PRIu32 ":%02" PRIu32 ".%" PRIu32 "", min, sec, dSec);
    char killStr[64] = {0};
    snprintf(killStr, sizeof(killStr), "Kills: %d", rc->kills);

    clearPxTft();

    // Plot a title depending on if all enemies were killed or not
    uint16_t width;
    if (rc->liveSprites > 0)
    {
        width = textWidth(&rc->radioStars, "YOU LOSE");
        drawText(&rc->radioStars, WHITE, "YOU LOSE", (TFT_WIDTH - width) / 2, 0);
    }
    else
    {
        width = textWidth(&rc->radioStars, "YOU WIN!");
        drawText(&rc->radioStars, WHITE, "YOU WIN!", (TFT_WIDTH - width) / 2, 0);
    }

    // Plot the round stats
    width = textWidth(&rc->ibm, killStr);
    drawText(&rc->ibm, WHITE, killStr, (TFT_WIDTH - width) / 2, (rc->radioStars.height + 5));
    width = textWidth(&rc->ibm, timeStr);
    drawText(&rc->ibm, WHITE, timeStr, (TFT_WIDTH - width) / 2, (rc->radioStars.height + 5) + (rc->ibm.height + 3));

    // Only draw the guitar on this screen, never the boot
    rc->isCriticalHit = false;

    // Draw the HUD, with animation if you win!
    if (rc->liveSprites > 0)
    {
        rc->shotCooldown = 0;
    }
    else
    {
        if (0 == rc->shotCooldown)
        {
            rc->shotCooldown = PLAYER_SHOT_COOLDOWN;
        }
        else if (rc->shotCooldown > 0)
        {
            rc->shotCooldown -= tElapsedUs;
        }
        else
        {
            rc->shotCooldown = 0;
        }
    }

    rc->gameOverButtonLockUs -= tElapsedUs;
    if (rc->gameOverButtonLockUs < 0)
    {
        rc->gameOverButtonLockUs = 0;
    }
    drawHUD();
}

/**
 * Illuminate LEDs based on a given angle
 *
 * @param leds The LEDs to light, arranged in a six-LED 'circle'
 * @param angle The angle at which to light the LEDs
 * @param hue The hue to light the LEDs with
 * @param valNumerator The numerator to multiply the value by (brightness)
 * @param valDenominator The denominator to multiply the value by (brightness)
 */
void lightLedsFromAngle(led_t* leds, float angle, int hue, int valNumerator, int valDenominator)
{
    // Associate each LED with a radian angle around the circle
    const float ledAngles[CONFIG_NUM_LEDS] = {
        3.665191f, 4.712389f, 5.759587f, 0.523599f, 1.570796f, 2.617994f,
    };

    // For each LED
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        // Find the distance between this LED's angle and the angle to the nearest enemy
        float dist = ABS(angle - ledAngles[i]);
        if (dist > M_PI)
        {
            dist = (2 * M_PI) - dist;
        }

        // If the distance is small enough
        if (dist < 1)
        {
            // Light this LED proportional to the player distance to the sprite
            // and the LED distance to the angle
            leds[i] = LedEHSVtoHEXhelper(hue, 0xFF, (255 * valNumerator * (1 - dist)) / valDenominator, true);
        }
    }
}

/**
 * @brief Drive LEDs based on game state
 * Called every 10ms
 */
void shooterLedTimer(void)
{
    led_t leds[CONFIG_NUM_LEDS] = {{0}};

    // If we were shot, flash red at the angle from the shot
    if (rc->gotShotTimer > 0)
    {
        // 0 hue is red
        lightLedsFromAngle(leds, rc->shotFromAngle, 0, rc->gotShotTimer, LED_ON_TIME);
    }
    else if (rc->warningShotTimer > 0)
    {
        // If we're about to be shot, flash yellow
        lightLedsFromAngle(leds, rc->warningShotAngle, 43, 1, 1);
    }
    // Otherwise if we shot something, flash green
    else if (rc->shotSomethingTimer > 0)
    {
        // Display different LEDs for criticality
        if (rc->isCriticalHit)
        {
            // Rainbow for a critical hit
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                leds[i] = LedEHSVtoHEXhelper(
                    /* Hue */ (i * 256) / CONFIG_NUM_LEDS,
                    /* Sat */ 0xFF,
                    /* Val */ (rc->shotSomethingTimer * 0xFF) / LED_ON_TIME, true);
            }
        }
        else
        {
            // Normal green for a normal shot
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                leds[i].g = (rc->shotSomethingTimer * 0x40) / LED_ON_TIME;
            }
        }
    }
    // Otherwise use the LEDs like a radar
    else if (rc->closestDist < RADAR_RANGE) // This is the squared dist, so check against the radius
    {
        // 0 is red, 85 is green, 171 is blue
        if (rc->radarObstructed)
        {
            lightLedsFromAngle(leds, rc->closestAngle, 140, (RADAR_RANGE - rc->closestDist), RADAR_RANGE);
        }
        else
        {
            lightLedsFromAngle(leds, rc->closestAngle, 200, (RADAR_RANGE - rc->closestDist), RADAR_RANGE);
        }
    }

    // If we're at a quarter health or less, and not displaying 'got shot'
    if (rc->mode == RC_GAME && rc->health <= rc->initialHealth / 4 && rc->gotShotTimer <= 0)
    {
        // Increment or decrement the red warning LED
        if (true == rc->healthWarningInc)
        {
            rc->healthWarningTimer++;
            if (rc->healthWarningTimer == 0x80)
            {
                rc->healthWarningInc = false;
            }
        }
        else
        {
            rc->healthWarningTimer--;
            if (rc->healthWarningTimer == 0)
            {
                rc->healthWarningInc = true;
            }
        }

        // Pulse all LEDs red as a warning, reduce G and B
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if ((rc->healthWarningTimer / 4) > leds[i].r)
            {
                leds[i].r = (rc->healthWarningTimer / 8);
            }
            leds[i].g /= 4;
            leds[i].b /= 4;
        }
    }

    // Push out the LEDs
    setLeds(leds, sizeof(leds));
}

/**
 * Display the high scores
 */
void shooterDrawScores(void)
{
    clearPxTft();

    const char* difficulty;
    const char* map;

    // Pick difficulty
    switch (rc->difficulty)
    {
        default:
        case RC_NUM_DIFFICULTIES:
        case RC_EASY:
        {
            difficulty = str_easy;
            break;
        }
        case RC_MED:
        {
            difficulty = str_medium;
            break;
        }
        case RC_HARD:
        {
            difficulty = str_v_hard;
            break;
        }
    }

    // Pick map size
    switch (rc->mapIdx)
    {
        default:
        case RC_NUM_MAPS:
        case RC_MAP_S:
        {
            map = rc_small;
            break;
        }
        case RC_MAP_M:
        {
            map = rc_medium;
            break;
        }
        case RC_MAP_L:
        {
            map = rc_large;
            break;
        }
    }

    // Plot the title
    char title[64] = {0};
    snprintf(title, sizeof(title) - 1, "%s (%s)", difficulty, map);
    uint16_t width = textWidth(&rc->ibm, title);
    drawText(&rc->ibm, WHITE, title, (TFT_WIDTH - width) / 2, 0);

    // Display scores
    shooterScores_t* s = getShooterScores();
    for (uint8_t i = 0; i < RC_NUM_SCORES; i++)
    {
        shooterScore_t* score = &(s->scores[rc->mapIdx][rc->difficulty][i]);
        // If this is a valid score
        if (score->kills > 0 || score->tElapsedUs > 0)
        {
            // Make the time human readable
            uint32_t dSec = (score->tElapsedUs / USEC_IN_D_SEC) % 10;
            uint32_t sec  = (score->tElapsedUs / (int)USEC_IN_SEC) % 60;
            uint32_t min  = (score->tElapsedUs / (USEC_IN_SEC * 60));

            // Print the string to an array
            char scoreStr[64] = {0};
            snprintf(scoreStr, sizeof(scoreStr), "%2" PRIu16 " in %02" PRIu32 ":%02" PRIu32 ".%" PRIu32 "",
                     score->kills, min, sec, dSec);

            // Plot the string
            drawText(&rc->ibm, WHITE, scoreStr, 0, rc->radioStars.height + 3 + (i * (rc->ibm.height + 3)));
        }
    }
}

/**
 * @brief Handle button input when displaying scores
 *
 * @param button The button that was pressed down
 */
void shooterScoreDisplayButton(uint32_t button)
{
    switch (button)
    {
        case PB_RIGHT:
        {
            // Cycle difficulty
            rc->difficulty = (rc->difficulty + 1) % RC_NUM_DIFFICULTIES;
            break;
        }
        case PB_LEFT:
        {
            // Cycle difficulty
            if (0 == rc->difficulty)
            {
                rc->difficulty = RC_NUM_DIFFICULTIES - 1;
            }
            else
            {
                rc->difficulty--;
            }
            break;
        }
        case PB_UP:
        {
            // Cycle maps
            if (0 == rc->mapIdx)
            {
                rc->mapIdx = RC_NUM_MAPS - 1;
            }
            else
            {
                rc->mapIdx--;
            }
            break;
        }
        case PB_DOWN:
        {
            // Cycle maps
            rc->mapIdx = (rc->mapIdx + 1) % RC_NUM_MAPS;
            break;
        }
        case PB_A:
        {
            // Action, return to the menu
            rc->mode = RC_MENU;
            break;
        }
        default:
        {
            // Do nothing
            break;
        }
    }
}

/**
 * Draw the map options to the screen
 *
 * @param tElapsedUs The time elapsed since the last call to this function
 */
void shooterMapSelectRenderer(uint32_t tElapsedUs __attribute__((unused)))
{
    clearPxTft();
    int16_t xOffset = (TFT_WIDTH - rc->mapW) / 2;
    int16_t yOffset = rc->ibm.height + (TFT_HEIGHT - rc->mapH - rc->ibm.height) / 2;

    int16_t textOffset = (TFT_WIDTH - textWidth(&rc->ibm, "MAP SELECT")) / 2;
    drawText(&rc->ibm, WHITE, "MAP SELECT", textOffset, 0);
    for (int16_t y = 0; y < rc->mapH; y++)
    {
        for (int16_t x = 0; x < rc->mapW; x++)
        {
            switch (MAP_TILE(y, x))
            {
                case WMT_W1:
                case WMT_W2:
                case WMT_W3:
                case WMT_C:
                {
                    setPxTft(x + xOffset, y + yOffset, WHITE);
                    break;
                }
                default:
                case WMT_E:
                case WMT_S:
                {
                    setPxTft(x + xOffset, y + yOffset, BLACK);
                    break;
                }
            }
        }
    }

    // Draw arrows to scroll the maps
    drawText(&rc->tomThumb, WHITE, "<", 0, TFT_HEIGHT - rc->tomThumb.height);
    drawText(&rc->tomThumb, WHITE, ">", TFT_WIDTH - 3, TFT_HEIGHT - rc->tomThumb.height);
}

/**
 * Handle button input when selecting the map
 *
 * @param button The button that was pressed down
 */
void shooterMapSelectButton(int32_t button)
{
    switch (button)
    {
        case PB_UP:
        case PB_DOWN:
        default:
        {
            // Do nothing
            break;
        }
        case PB_LEFT:
        {
            // Scroll maps
            if (rc->mapIdx == 0)
            {
                rc->mapIdx = RC_NUM_MAPS - 1;
            }
            else
            {
                rc->mapIdx--;
            }
            shooterSetMap();
            break;
        }
        case PB_RIGHT:
        {
            // Scroll maps
            rc->mapIdx = (rc->mapIdx + 1) % RC_NUM_MAPS;
            shooterSetMap();
            break;
        }
        case PB_A:
        {
            shooterSetMap();
            shooterInitGame(rc->difficulty);
            break;
        }
    }
}

/**
 * Set the current map pointer and sizes from the current index
 */
void shooterSetMap(void)
{
    switch (rc->mapIdx)
    {
        default:
        case RC_NUM_MAPS:
        case RC_MAP_S:
        {
            // Set map vars
            rc->mapW = MAP_S_W;
            rc->mapH = MAP_S_H;
            rc->map  = &(worldMap_s[0][0]);
            break;
        }
        case RC_MAP_M:
        {
            // Set map vars
            rc->mapW = MAP_M_W;
            rc->mapH = MAP_M_H;
            rc->map  = &(worldMap_m[0][0]);
            break;
        }
        case RC_MAP_L:
        {
            // Set map vars
            rc->mapW = MAP_L_W;
            rc->mapH = MAP_L_H;
            rc->map  = &(worldMap_l[0][0]);
            break;
        }
    }
}

//==============================================================================
// TODO struct this
//==============================================================================

shooterScores_t shooterScores;
bool scoresRead = false;

shooterScores_t* getShooterScores(void)
{
    if (!scoresRead)
    {
        size_t scoreSize = sizeof(shooterScore_t);
        readNvsBlob("shooter_score", &shooterScores, &scoreSize);
        scoresRead = true;
    }
    return &(shooterScores);
}

void addShooterScore(shooterDifficulty_t difficulty, shooterMap_t mapIdx, uint16_t kills, uint32_t tElapsed)
{
    // Make sure they're loaded first
    getShooterScores();

    // Look through the table for a spot to insert
    for (uint8_t i = 0; i < RC_NUM_SCORES; i++)
    {
        // More kills, insert before this index
        if ((shooterScores.scores[mapIdx][difficulty][i].kills < kills) ||
            // Same kills but faster, insert before this index
            (shooterScores.scores[mapIdx][difficulty][i].kills == kills
             && shooterScores.scores[mapIdx][difficulty][i].tElapsedUs > tElapsed)
            ||
            // Or slot is empty, insert before this index
            (shooterScores.scores[mapIdx][difficulty][i].kills == 0
             && shooterScores.scores[mapIdx][difficulty][i].tElapsedUs == 0))
        {
            // Make room for this element
            memmove(&shooterScores.scores[mapIdx][difficulty][i + 1], &shooterScores.scores[mapIdx][difficulty][i],
                    sizeof(shooterScore_t) * (RC_NUM_SCORES - 1 - i));

            // Write the new data
            shooterScores.scores[mapIdx][difficulty][i].kills      = kills;
            shooterScores.scores[mapIdx][difficulty][i].tElapsedUs = tElapsed;

            // Save the settings
            writeNvsBlob("shooter_score", &shooterScores, sizeof(shooterScore_t));

            return;
        }
    }
}
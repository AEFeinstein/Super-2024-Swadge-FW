//==============================================================================
// Includes
//==============================================================================
#include "bombadeetle.h"
#include "cnfs_image.h"
#include "fs_font.h"
#include "fs_wsg.h"
#include "hdw-btn.h"
#include "palette.h"
#include "shapes.h"
#include "wsg.h"
#include <ctype.h>
#include <stdbool.h>

//==============================================================================
// Defines
//==============================================================================
#define TAG                         "BOMBADEETLE"
#define OFFSETMAP_X                 44
#define OFFSETMAP_Y                 10
#define OFFSETSELECTBUTTON_X        42
#define OFFSETSELECTBUTTON_Y        72
#define ANIMATIONSPEED              125
#define CURSORSPEED                 125
#define GAME_MOVE_TIME              1
#define SHLOOG_MOVE_TIME            40
#define BOMBADEETLE_MOVE_TIME       25
#define MOVETIME                    75
#define TILESIZE                    16

#define DEFAULT_MOVE_AMOUNT         4

#define GRIDHEIGHT                  9
#define GRIDWIDTH                   12

#define SHLOOG_HEIGHT_OFFSET        -8

#define WALL_W                      1
#define WALL_S                      2
#define WALL_E                      4
#define WALL_N                      8
#define TELEPORT                    16
#define GOAL                        32
#define HOLE                        64


#define DIRECTION_NONE                    0
#define DIRECTION_W                       1
#define DIRECTION_S                       2
#define DIRECTION_E                       4
#define DIRECTION_N                       8

#define BG_LEVELNAME_X              44
#define BG_LEVELNAME_Y              166

#define BOMBADEETLE_FRAMECOUNT       4
#define BOMBADEETLE_COUNT            100
#define SHLOOG_MAX_COUNT             16

#define COLLISION_RANGE              12

#define COLOR_BOMBLUE                c225

#define TIMING_SUCCESS               500




const char bombadeetleModeName[] = "Bombadeetle";

typedef enum
{
    MENU,
    STATE_STAGESELECT,
    STATE_PLACING,
    STATE_RUNNING,
    STATE_COLLISION,
    STATE_WIN,

} bombState_t;

static const cnfsFileIdx_t bombadeetleTiles[] ={
    BOMB_TILE_001_WSG, BOMB_TILE_002_WSG
};

static const cnfsFileIdx_t bombadeetleArrows[] ={
    BOMB_ARROW_LEFT_WSG, BOMB_ARROW_DOWN_WSG, BOMB_ARROW_RIGHT_WSG, BOMB_ARROW_UP_WSG,
};

static const cnfsFileIdx_t bombadeetleCursor[] ={
    BOMB_SBOX_001_WSG, BOMB_SBOX_002_WSG, BOMB_SBOX_003_WSG,
};
static const cnfsFileIdx_t bombadeetleActiveCursor[] ={
    BOMB_ABOX_001_WSG, BOMB_ABOX_002_WSG, BOMB_ABOX_003_WSG,
};

static const cnfsFileIdx_t bombadeetleWall[] = {
    BOMB_WALL_V_WSG, BOMB_WALL_H_WSG
};

static const cnfsFileIdx_t bombadeetleBombadeetleSprite[] = {
    BOMB_BEETLETES_101_WSG, BOMB_BEETLETES_102_WSG, BOMB_BEETLETES_103_WSG, BOMB_BEETLETES_201_WSG, BOMB_BEETLETES_202_WSG, BOMB_BEETLETES_203_WSG, BOMB_BEETLETES_301_WSG, BOMB_BEETLETES_302_WSG, BOMB_BEETLETES_303_WSG,
};


static const cnfsFileIdx_t bombadeetleShloogSprite[] = {
    BOMB_SHLOOG_101_WSG, BOMB_SHLOOG_102_WSG, BOMB_SHLOOG_103_WSG, BOMB_SHLOOG_201_WSG, BOMB_SHLOOG_202_WSG, BOMB_SHLOOG_203_WSG, BOMB_SHLOOG_301_WSG, BOMB_SHLOOG_302_WSG, BOMB_SHLOOG_303_WSG,
};

static const cnfsFileIdx_t bombadeetleGoal[] = {
    BOMB_GOAL_001_WSG, BOMB_GOAL_002_WSG, BOMB_GOAL_003_WSG, BOMB_GOAL_004_WSG, BOMB_GOAL_005_WSG, BOMB_GOAL_006_WSG,
};

static const cnfsFileIdx_t bombadeetleLevels[] = {
    BOMB_ONE_BIN, BOMB_HELLO_BIN, BOMB_NOHOLES_BIN, BOMB_MAG_1_BIN, BOMB_CREPUSCULAR_BIN,BOMB_MOWWOW_BIN , BOMB_JERO_BIN,
};

static const cnfsFileIdx_t bombadeetleTeleporter[] = {
     BOMB_TELEPORT_1_WSG, BOMB_TELEPORT_2_WSG, BOMB_TELEPORT_3_WSG, BOMB_TELEPORT_4_WSG, BOMB_TELEPORT_5_WSG, BOMB_TELEPORT_6_WSG,
};

static const int bombadeetleCursorFrames[] ={
    0, 1, 2, 1,
};
static const int bombadeetleBeetleFrames[] ={
    0, 1, 2, 1,
};

static const int bombadeetleGoalAnimation[] ={
    0, 1, 2, 3, 4, 5, 4, 3,
};


static void bombadeetleEnterMode(void);
static void bombadeetleExitMode(void);
static void bombadeetleMainLoop(int64_t elapsedUs);
static void bombadeetleLoadMap();
static void bombadeetleImportMap(int64_t index);
static bool bombadeetleMove(int tile, int direction);
static int bombadeetleRight(int direction);
static int bombadeetleLeft(int direction);
static int bombadeetleTurnAround(int direction);
static void bombadeetleCheckShloogs(bool update);
static void bombadeetleCheckBombadeetles(bool update);
static void bombadeetleDrawBackground();
static void bombadeetleDrawGame();
static void bombadeetleDrawSelect();
static void bombadeetleGameLoop(int64_t elapsedUs);
static void bombadeetleStageSelectLoop(int64_t elapsedUs);





swadgeMode_t bombadeetleMode = {
    .modeName                   = bombadeetleModeName,
    .wifiMode                   = NO_WIFI,

    .overrideUsb                = false,
    .usesAccelerometer          = false,
    .usesThermometer            = false,
    .overrideSelectBtn          = false,
    .fnEnterMode                = bombadeetleEnterMode,
    .fnExitMode                 = bombadeetleExitMode,
    .fnMainLoop                 = bombadeetleMainLoop,
    .fnAudioCallback            = NULL,
    .fnBackgroundDrawCallback   = NULL,
    .fnEspNowRecvCb             = NULL,
    .fnEspNowSendCb             = NULL,
    .fnAdvancedUSB              = NULL,
};

typedef struct
{
    wsg_t* tiles;
    
} bombadeetleBackground_t;

typedef struct{
    char name[17];
    int8_t map[GRIDWIDTH * GRIDHEIGHT];
    int8_t bombadeetleSpawn[GRIDWIDTH * GRIDHEIGHT];
    int8_t shloogSpawn[GRIDWIDTH * GRIDHEIGHT];
    int8_t left;
    int8_t up;
    int8_t down;
    int8_t right;

} bombadeetleCurrentLevel_t;

typedef struct
{
    int8_t direction; 
    int8_t tileX;
    int8_t tileY;
    int8_t frame;

    int16_t locX;
    int16_t locY;

    bool goal;
    bool doomed;
    
} bombadeetleEntity_t;

typedef struct
{
    bombState_t state;

    bombadeetleCurrentLevel_t mapFile;

    wsg_t* arrows;
    wsg_t* cursor;
    wsg_t* acursor;
    wsg_t* walls;
    wsg_t* bombadeetleSprites;
    wsg_t* shloogSprites;
    wsg_t* teleporterSprites;
    wsg_t* goal;
    wsg_t backgroundTile;
    wsg_t success;
    wsg_t stageSelect;
    wsg_t stageSelectButton;
    wsg_t stageActiveSelectButton;
    wsg_t levelNameBackground;
    wsg_t tools;
    wsg_t collisionSprite;
    wsg_t holeSprite;

    bool building;
    bool goalAnimating;
    
    bombadeetleBackground_t background;
    int8_t cursorFrame;
    int16_t cursorTime;
    int8_t cursorX;
    int8_t cursorY;

    int8_t goalFrame;
    int16_t goalTime;
    int8_t goalCount;

    int16_t animationTime;

    int8_t teleporterFrame;

    int8_t levelIndex;
    int8_t levelMax;

    int8_t backgroundOffset;

    int16_t successTime;

    int16_t collisionX;
    int16_t collisionY;

    int8_t stageSelectIndex;
    int8_t stageSelectPageIndex;
    
    int8_t bombadeetleMoveAmount;
    int8_t shloogMoveAmount;
    font_t mainFont;

    int16_t cursorMoveTime;
    int16_t gameMoveTime;
    int8_t gameMoveTick;
    int8_t gameSpeed;

    int8_t grid[GRIDHEIGHT * GRIDWIDTH]; // I don't like this.
    int8_t map[GRIDHEIGHT * GRIDWIDTH];

    bombadeetleEntity_t* bombadeetles;
    bombadeetleEntity_t* shloogs;
    
    bool paused;

} bombadeetleData_t;


bombadeetleData_t* bombadeetle;


static void bombadeetleEnterMode()
{
    bombadeetle = (bombadeetleData_t*)heap_caps_calloc(1,sizeof(bombadeetleData_t), MALLOC_CAP_8BIT);

    bombadeetle->cursorTime = 0;
    bombadeetle->cursorX = 0;
    bombadeetle->cursorY = 0;
    bombadeetle->building = false;
    bombadeetle->goalCount = 0;
    bombadeetle->successTime = 0;
    bombadeetle->collisionX = -1;
    bombadeetle->collisionY = -1;
    bombadeetle->teleporterFrame = 0;
    bombadeetle->backgroundOffset = 0;
    bombadeetle->stageSelectIndex = 0;
    
    
    bombadeetle->gameSpeed = DEFAULT_MOVE_AMOUNT;
    bombadeetle->cursorMoveTime = 0;

    bombadeetle->state = STATE_STAGESELECT;

    bombadeetle->bombadeetles = (bombadeetleEntity_t*)heap_caps_calloc(BOMBADEETLE_COUNT, sizeof(bombadeetleEntity_t), MALLOC_CAP_8BIT);
    bombadeetle->shloogs = (bombadeetleEntity_t*)heap_caps_calloc(SHLOOG_MAX_COUNT, sizeof(bombadeetleEntity_t), MALLOC_CAP_8BIT);
    
    //Load from disk to see what the current level max is
    bombadeetle->levelMax = 3;
    bombadeetle->levelIndex = 3;

    loadWsg(BOMB_SUCCESS_WSG, &bombadeetle->success, true);
    loadWsg(BOMB_STAGE_SELECT_BACKGROUND_WSG, &bombadeetle->stageSelect, true);
    loadWsg(BOMB_STAGE_SELECT_INDEX_BUTTON_WSG, &bombadeetle->stageSelectButton, true);
    loadWsg(BOMB_STAGE_SELECT_INDEX_ACTIVE_BUTTON_WSG, &bombadeetle->stageActiveSelectButton, true);


    loadWsg(BOMB_LEVEL_NAME_WSG, &bombadeetle->levelNameBackground, true);
    loadWsg(BOMB_TOOLS_WSG, &bombadeetle->tools, true);
    loadWsg(BOMB_COLLISION_WSG, &bombadeetle->collisionSprite,true);
    loadWsg(BOMB_HOLE_WSG, &bombadeetle->holeSprite, true);
    loadWsg(BOMB_BACKGROUND_WSG, &bombadeetle->backgroundTile, true);
    bombadeetle->background.tiles = heap_caps_calloc(ARRAY_SIZE(bombadeetleTiles), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleTiles); idx++)
    {
        loadWsg(bombadeetleTiles[idx], &bombadeetle->background.tiles[idx], true);
    }
    
    bombadeetle->arrows = heap_caps_calloc(ARRAY_SIZE(bombadeetleArrows), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleArrows); idx++)
    {
        loadWsg(bombadeetleArrows[idx], &bombadeetle->arrows[idx], true);
    }

    bombadeetle->teleporterSprites = heap_caps_calloc(ARRAY_SIZE(bombadeetleTeleporter), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleTeleporter); idx++)
    {
        loadWsg(bombadeetleTeleporter[idx], &bombadeetle->teleporterSprites[idx], true);
    }

    bombadeetle->cursor = heap_caps_calloc(ARRAY_SIZE(bombadeetleCursor), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleCursor); idx++)
    {
        loadWsg(bombadeetleCursor[idx], &bombadeetle->cursor[idx], true);
    }
    
    bombadeetle->acursor = heap_caps_calloc(ARRAY_SIZE(bombadeetleActiveCursor), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleActiveCursor); idx++)
    {
        loadWsg(bombadeetleActiveCursor[idx], &bombadeetle->acursor[idx], true);
    }

    bombadeetle->walls = heap_caps_calloc(ARRAY_SIZE(bombadeetleWall), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleWall); idx++)
    {
        loadWsg(bombadeetleWall[idx], &bombadeetle->walls[idx], true);
    }

    bombadeetle->bombadeetleSprites = heap_caps_calloc(ARRAY_SIZE(bombadeetleBombadeetleSprite), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleBombadeetleSprite); idx++)
    {
        loadWsg(bombadeetleBombadeetleSprite[idx], &bombadeetle->bombadeetleSprites[idx], true);
    }
    
    bombadeetle->shloogSprites = heap_caps_calloc(ARRAY_SIZE(bombadeetleShloogSprite), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleShloogSprite); idx++)
    {
        loadWsg(bombadeetleShloogSprite[idx], &bombadeetle->shloogSprites[idx], true);
    }

    bombadeetle->goal = heap_caps_calloc(ARRAY_SIZE(bombadeetleGoal), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleGoal); idx++)
    {
        loadWsg(bombadeetleGoal[idx], &bombadeetle->goal[idx], true);
    }
    loadFont(BOMBBADEETLE_FONT, &bombadeetle->mainFont, true);

    
    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        bombadeetle->grid[idx] = 0;
        bombadeetle->map[idx] = 0; //Clear it before loading level. 

        if (idx % GRIDWIDTH == 0)
        {
            bombadeetle->map[idx] |= WALL_W;
        }

        if (idx % GRIDWIDTH == GRIDWIDTH - 1)
        {
            bombadeetle->map[idx] |= WALL_E;
        }

        if (idx < GRIDWIDTH)
        {
            bombadeetle->map[idx] |= WALL_N;
        }

        if (idx / GRIDWIDTH == GRIDHEIGHT - 1)
        {
            bombadeetle->map[idx] |= WALL_S;
        }
    }
}


static void bombadeetleCheckBombadeetles(bool update)
{
    int8_t checkTile;

    for(int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
    {
        if (bombadeetle->bombadeetles[idx].direction == DIRECTION_NONE) continue;
        if (bombadeetle->bombadeetles[idx].direction == GOAL) continue;

        if (update)
        {
            
            //Update direction
            switch (bombadeetle->bombadeetles[idx].direction)
            {
                case DIRECTION_E:
                    bombadeetle->bombadeetles[idx].tileX++;
                    break;
                    case DIRECTION_N:
                    bombadeetle->bombadeetles[idx].tileY--;
                    break;
                case DIRECTION_S:
                    bombadeetle->bombadeetles[idx].tileY++;
                    break;
                case DIRECTION_W:
                    bombadeetle->bombadeetles[idx].tileX--;
                    break;
                default:
                    continue;
            }
        }        

        switch (bombadeetle->bombadeetles[idx].direction)
        {

            case DIRECTION_E:
                bombadeetle->bombadeetles[idx].locX = (bombadeetle->bombadeetles[idx].tileX * TILESIZE) + bombadeetle->bombadeetleMoveAmount;
                bombadeetle->bombadeetles[idx].locY = (bombadeetle->bombadeetles[idx].tileY * TILESIZE);
            break;
            case DIRECTION_N:
                bombadeetle->bombadeetles[idx].locX = (bombadeetle->bombadeetles[idx].tileX * TILESIZE) ;
                bombadeetle->bombadeetles[idx].locY = (bombadeetle->bombadeetles[idx].tileY * TILESIZE) - bombadeetle->bombadeetleMoveAmount;
                break;
            case DIRECTION_S:                
                bombadeetle->bombadeetles[idx].locX = (bombadeetle->bombadeetles[idx].tileX * TILESIZE) ;
                bombadeetle->bombadeetles[idx].locY = (bombadeetle->bombadeetles[idx].tileY * TILESIZE) + bombadeetle->bombadeetleMoveAmount;
                break;
            case DIRECTION_W:
                bombadeetle->bombadeetles[idx].locX = (bombadeetle->bombadeetles[idx].tileX * TILESIZE) - bombadeetle->bombadeetleMoveAmount;
                bombadeetle->bombadeetles[idx].locY = (bombadeetle->bombadeetles[idx].tileY * TILESIZE);
                break;
            default:
            break;
        }
        
        checkTile = (bombadeetle->bombadeetles[idx].tileX) + (bombadeetle->bombadeetles[idx].tileY * GRIDWIDTH);
        //Check if on arrow tile
        if (bombadeetle->grid[checkTile] != DIRECTION_NONE)
        {
            bombadeetle->bombadeetles[idx].direction = bombadeetle->grid[checkTile];
        }

        
        //Check if on hole tile
        if (bombadeetle->map[checkTile] & HOLE)
        {
            ESP_LOGI(TAG, "OMG! HOLE!");
             bombadeetle->state = STATE_COLLISION;

            bombadeetle->collisionX = bombadeetle->bombadeetles[idx].locX;
            bombadeetle->collisionY = bombadeetle->bombadeetles[idx].locY;
            bombadeetle->bombadeetles[idx].direction = DIRECTION_NONE;
            
        }

        
        //Check if on teleport tile
        if (bombadeetle->map[checkTile] & TELEPORT && update)
        {
            ESP_LOGI(TAG, "OMG! TELEPORT!");
            for (int ndx = 1; ndx < 108; ndx++)
            {
                if (bombadeetle->map[(checkTile + ndx) % 108] & TELEPORT)
                {
                    bombadeetle->bombadeetles[idx].tileX = ((checkTile + ndx) % 108) % GRIDWIDTH;
                    bombadeetle->bombadeetles[idx].tileY = ((checkTile + ndx) % 108) / GRIDWIDTH;
                    ESP_LOGI(TAG, "TELEPORT FOUND! %d %d", checkTile, (checkTile + ndx)%108);

                    break;
                }
            }            
        }
        
        //Check direction
        int8_t tile;
        tile = bombadeetle->map[(bombadeetle->bombadeetles[idx].tileX) + (bombadeetle->bombadeetles[idx].tileY * GRIDWIDTH)];
        if (tile & GOAL)
        {
            bombadeetle->bombadeetles[idx].direction = GOAL;
            bombadeetle->bombadeetles[idx].goal = true;
            bombadeetle->goalAnimating = true;
            bombadeetle->goalFrame = 0;
            bombadeetle->goalTime = 0;

            ESP_LOGI(TAG, "GOAL!");
        }


        if (bombadeetleMove(tile, bombadeetle->bombadeetles[idx].direction))
        {

        }
        else if (bombadeetleMove(tile, bombadeetleRight(bombadeetle->bombadeetles[idx].direction)))
        {
            bombadeetle->bombadeetles[idx].direction = bombadeetleRight(bombadeetle->bombadeetles[idx].direction);
        }
        else if (bombadeetleMove(tile, bombadeetleLeft(bombadeetle->bombadeetles[idx].direction)))
        {
            bombadeetle->bombadeetles[idx].direction = bombadeetleLeft(bombadeetle->bombadeetles[idx].direction);                    
        }
        else
        {
            bombadeetle->bombadeetles[idx].direction = bombadeetleTurnAround(bombadeetle->bombadeetles[idx].direction);
        }


        
    }

}

static void bombadeetleCheckShloogs(bool update)
{

    
    for(int idx = 0; idx < SHLOOG_MAX_COUNT; idx++)
    {
        if (bombadeetle->shloogs[idx].direction == DIRECTION_NONE) continue;
        if (bombadeetle->shloogs[idx].direction == GOAL) continue;

        if (update)
        {
            
            //Update direction
            switch (bombadeetle->shloogs[idx].direction)
            {
                case DIRECTION_E:
                    bombadeetle->shloogs[idx].tileX++;
                    break;
                    case DIRECTION_N:
                    bombadeetle->shloogs[idx].tileY--;
                    break;
                case DIRECTION_S:
                    bombadeetle->shloogs[idx].tileY++;
                    break;
                case DIRECTION_W:
                    bombadeetle->shloogs[idx].tileX--;
                    break;
                default:
                    continue;
            }
        }

        
        switch (bombadeetle->shloogs[idx].direction)
        {

            case DIRECTION_E:
                bombadeetle->shloogs[idx].locX = (bombadeetle->shloogs[idx].tileX * TILESIZE) + bombadeetle->shloogMoveAmount;
                bombadeetle->shloogs[idx].locY = (bombadeetle->shloogs[idx].tileY * TILESIZE) + SHLOOG_HEIGHT_OFFSET;
            break;
            case DIRECTION_N:
                bombadeetle->shloogs[idx].locX = (bombadeetle->shloogs[idx].tileX * TILESIZE) ;
                bombadeetle->shloogs[idx].locY = (bombadeetle->shloogs[idx].tileY * TILESIZE) - bombadeetle->shloogMoveAmount + SHLOOG_HEIGHT_OFFSET;
                break;
            case DIRECTION_S:                
                bombadeetle->shloogs[idx].locX = (bombadeetle->shloogs[idx].tileX * TILESIZE) ;
                bombadeetle->shloogs[idx].locY = (bombadeetle->shloogs[idx].tileY * TILESIZE) + bombadeetle->shloogMoveAmount + SHLOOG_HEIGHT_OFFSET;
                break;
            case DIRECTION_W:
                bombadeetle->shloogs[idx].locX = (bombadeetle->shloogs[idx].tileX * TILESIZE) - bombadeetle->shloogMoveAmount;
                bombadeetle->shloogs[idx].locY = (bombadeetle->shloogs[idx].tileY * TILESIZE) + SHLOOG_HEIGHT_OFFSET;
                break;
            default:
            break;
        }

        //Check if on arrow tile
        if (bombadeetle->grid[(bombadeetle->shloogs[idx].tileX) + (bombadeetle->shloogs[idx].tileY * GRIDWIDTH)] != DIRECTION_NONE)
        {
            bombadeetle->shloogs[idx].direction = bombadeetle->grid[(bombadeetle->shloogs[idx].tileX) + (bombadeetle->shloogs[idx].tileY * GRIDWIDTH)];
        }
        
        //Check direction
        int8_t tile;
        tile = bombadeetle->map[(bombadeetle->shloogs[idx].tileX) + (bombadeetle->shloogs[idx].tileY * GRIDWIDTH)];
        if (tile & GOAL)
        {
            bombadeetle->shloogs[idx].direction = GOAL;
            bombadeetle->shloogs[idx].goal = true;

            ESP_LOGI(TAG, "GOAL!");
        }


        if (bombadeetleMove(tile, bombadeetle->shloogs[idx].direction))
        {

        }
        else if (bombadeetleMove(tile, bombadeetleRight(bombadeetle->shloogs[idx].direction)))
        {
            bombadeetle->shloogs[idx].direction = bombadeetleRight(bombadeetle->shloogs[idx].direction);
        }
        else if (bombadeetleMove(tile, bombadeetleLeft(bombadeetle->shloogs[idx].direction)))
        {
            ESP_LOGI(TAG, "Turn left");

            bombadeetle->shloogs[idx].direction = bombadeetleLeft(bombadeetle->shloogs[idx].direction);                    
        }
        else
        {
            ESP_LOGI(TAG, "Turn around");

            bombadeetle->shloogs[idx].direction = bombadeetleTurnAround(bombadeetle->shloogs[idx].direction);
        }

        for (int ndx = 0; ndx < BOMBADEETLE_COUNT; ndx++)
        {
            if (bombadeetle->bombadeetles[ndx].goal || bombadeetle->bombadeetles[ndx].direction == DIRECTION_NONE) continue;

            if (bombadeetle->bombadeetles[ndx].tileX == bombadeetle->shloogs[idx].tileX && ABS(bombadeetle->bombadeetles[ndx].locY - (bombadeetle->shloogs[idx].locY - SHLOOG_HEIGHT_OFFSET)) < COLLISION_RANGE)
            {
                bombadeetle->state = STATE_COLLISION;
                            
                bombadeetle->collisionX = (bombadeetle->bombadeetles[ndx].locX + bombadeetle->shloogs[idx].locX - 16)/2 ;
                bombadeetle->collisionY = (bombadeetle->bombadeetles[ndx].locY + bombadeetle->shloogs[idx].locY - SHLOOG_HEIGHT_OFFSET - 8)/2;
            }

            
            if (bombadeetle->bombadeetles[ndx].tileY == bombadeetle->shloogs[idx].tileY && ABS(bombadeetle->bombadeetles[ndx].locX - bombadeetle->shloogs[idx].locX) < COLLISION_RANGE)
            {
                ESP_LOGI(TAG, "COLLISION!");
                bombadeetle->state = STATE_COLLISION;

                bombadeetle->collisionX = (bombadeetle->bombadeetles[ndx].locX + bombadeetle->shloogs[idx].locX - 16)/2 ;
                bombadeetle->collisionY = (bombadeetle->bombadeetles[ndx].locY + bombadeetle->shloogs[idx].locY - SHLOOG_HEIGHT_OFFSET - 8)/2;
            
            }
        }

    }
}



static void bombadeetleImportMap(int64_t index)
{
    //Clear previous map?

    for(int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
    {
        bombadeetle->bombadeetles[idx].direction = 0;
        bombadeetle->bombadeetles[idx].goal = false;
        bombadeetle->bombadeetles[idx].doomed = false;
        bombadeetle->bombadeetles[idx].frame = 0;
        bombadeetle->bombadeetles[idx].tileX = -1;
        bombadeetle->bombadeetles[idx].tileY = -1;
    }
    
    for(int idx = 0; idx < SHLOOG_MAX_COUNT; idx++)
    {
        bombadeetle->shloogs[idx].direction = 0;
        bombadeetle->shloogs[idx].goal = false;
        bombadeetle->shloogs[idx].doomed = false;
        bombadeetle->shloogs[idx].frame = 0;
        bombadeetle->shloogs[idx].tileX = -1;
        bombadeetle->shloogs[idx].tileY = -1;
    }

    //Bring in new map
    size_t levelSize = 0;
    int offset = 0;
    uint8_t *levelFile = cnfsGetFile(bombadeetleLevels[index], &levelSize);
    bombadeetle->goalCount = 0;

    for (int idx = 0; idx < 16; idx ++)
    {
        bombadeetle->mapFile.name[idx] = toupper(levelFile[idx]);
    }


    
    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        bombadeetle->grid[idx] = 0;
    }
    
    offset = 16;
    for (int idx = offset; idx < offset + 108; idx++)
    {
        bombadeetle->mapFile.map[idx - offset] = levelFile[idx];
        bombadeetle->map[idx - offset] = levelFile[idx];

        if ((idx - offset) % GRIDWIDTH == 0)
        {
            bombadeetle->map[(idx - offset)] |= WALL_W;
        }

        if ((idx - offset) % GRIDWIDTH == GRIDWIDTH - 1)
        {
            bombadeetle->map[(idx - offset)] |= WALL_E;
        }

        if ((idx - offset) < GRIDWIDTH)
        {
            bombadeetle->map[(idx - offset)] |= WALL_N;
        }

        if ((idx - offset) / GRIDWIDTH == GRIDHEIGHT - 1)
        {
            bombadeetle->map[(idx - offset)] |= WALL_S;
        }
    }

    offset += 108;    
    for (int idx = offset; idx < offset + 108; idx++)
    {
        bombadeetle->mapFile.bombadeetleSpawn[idx - offset] = levelFile[idx];
    }

    offset += 108;
    for (int idx = offset; idx < offset + 108; idx++)
    {
       bombadeetle->mapFile.shloogSpawn[idx - offset] = levelFile[idx];
    }

    offset += 108;
    
    bombadeetle->mapFile.left = levelFile[offset+ 0];
    bombadeetle->mapFile.up = levelFile[offset+ 1];
    bombadeetle->mapFile.down = levelFile[offset+ 2];
    bombadeetle->mapFile.right = levelFile[offset+ 3];
    //ESP_LOGI(TAG, "%d %d %d %d", levelFile[offset +0], levelFile[offset +1], levelFile[offset +2], levelFile[offset +3] );
}

static void bombadeetleLoadMap()
{
    int bombadeetleIndex = 0;
    int shloogIndex = 0;
    int tileIndex = 0;
    
    bombadeetle->state = STATE_PLACING;    
    bombadeetle->gameMoveTime = 0;
    bombadeetle->bombadeetleMoveAmount = 0;
    bombadeetle->goalCount = 0;
    bombadeetle->goalAnimating = false;
    bombadeetle->gameSpeed = DEFAULT_MOVE_AMOUNT;

    bombadeetle->collisionX = -1;
    bombadeetle->collisionY = -1;

    for (int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
    {
        bombadeetle->bombadeetles[idx].direction = DIRECTION_NONE;
    }

    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        tileIndex = bombadeetle->mapFile.bombadeetleSpawn[idx];
        if (tileIndex & DIRECTION_N || tileIndex & DIRECTION_E || tileIndex & DIRECTION_S || tileIndex & DIRECTION_W)
        {
            if (bombadeetleIndex >= BOMBADEETLE_COUNT) continue;
            

            bombadeetle->goalCount++;
            bombadeetle->bombadeetles[bombadeetleIndex].direction = tileIndex;
            bombadeetle->bombadeetles[bombadeetleIndex].tileX = idx % GRIDWIDTH;
            bombadeetle->bombadeetles[bombadeetleIndex].tileY = idx / GRIDWIDTH;
            bombadeetle->bombadeetles[bombadeetleIndex].locX = bombadeetle->bombadeetles[bombadeetleIndex].tileX * TILESIZE;
            bombadeetle->bombadeetles[bombadeetleIndex].locY = (bombadeetle->bombadeetles[bombadeetleIndex].tileY * TILESIZE);
            bombadeetleIndex++;          

        }
        

        tileIndex = bombadeetle->mapFile.shloogSpawn[idx];
        if (tileIndex & DIRECTION_N || tileIndex & DIRECTION_E || tileIndex & DIRECTION_S || tileIndex & DIRECTION_W)
        {
            if (shloogIndex >= SHLOOG_MAX_COUNT) continue;
           

            bombadeetle->shloogs[shloogIndex].direction = tileIndex;
            bombadeetle->shloogs[shloogIndex].tileX = idx % GRIDWIDTH;
            bombadeetle->shloogs[shloogIndex].tileY = idx / GRIDWIDTH;
            bombadeetle->shloogs[shloogIndex].locX = bombadeetle->shloogs[shloogIndex].tileX * TILESIZE;
            bombadeetle->shloogs[shloogIndex].locY = (bombadeetle->shloogs[shloogIndex].tileY * TILESIZE) + SHLOOG_HEIGHT_OFFSET;
            
            shloogIndex++;

        }
    }
}

static bool bombadeetleMove(int tile, int direction)
{
    return (tile & direction) == 0;
}

static int bombadeetleRight(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_N;
        case DIRECTION_N: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_W;
    }

    return direction;
}

static int bombadeetleLeft(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_N;
        case DIRECTION_N: return DIRECTION_W;
    }

    return direction;
}

static int bombadeetleTurnAround(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_W;
        case DIRECTION_N: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_N;
    }

    return direction;
}

static void bombadeetleMainLoop(int64_t elapsedUs)
{
    switch (bombadeetle->state)
    {
        case STATE_STAGESELECT:
            bombadeetleStageSelectLoop(elapsedUs);
            break;
        default:
            bombadeetleGameLoop(elapsedUs);
            break;
    }

}

static void bombadeetleStageSelectLoop(int64_t elapsedUs)
{
    
    buttonEvt_t evt;
     int16_t tick = elapsedUs / 1000;
    
     //animationTime

    
    bombadeetle->animationTime += tick;
    if (bombadeetle->animationTime > ANIMATIONSPEED)
    {
        bombadeetle->animationTime -= ANIMATIONSPEED;

        bombadeetle->backgroundOffset++;
        bombadeetle->backgroundOffset %=16;        
    }
    
    while(checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_DOWN)
            {
                
                bombadeetle->stageSelectIndex += 5;
                if (bombadeetle->stageSelectIndex >= 20)
                {
                    bombadeetle->stageSelectIndex %= 20;
                }
            }

            if (evt.button & PB_UP)
            {
               
                bombadeetle->stageSelectIndex -= 5;

                if (bombadeetle->stageSelectIndex < 0)
                {
                    bombadeetle->stageSelectIndex += 20;
                }
            
            }

            if (evt.button & PB_LEFT)
            {
                if (bombadeetle->stageSelectIndex % 5 != 0)
                {
                    bombadeetle->stageSelectIndex--;
                }
                else
                {
                    bombadeetle->stageSelectIndex += 4;
                }

            }

            if (evt.button & PB_RIGHT)
            {
                if (bombadeetle->stageSelectIndex % 5 != 4)
                {
                    bombadeetle->stageSelectIndex++;
                }
                else
                {
                    bombadeetle->stageSelectIndex -= 4;
                }
            }

            if (evt.button & PB_A)
            {
                bombadeetle->levelIndex = bombadeetle->stageSelectIndex + (bombadeetle->stageSelectPageIndex * 20);
                bombadeetle->state = STATE_PLACING;
                bombadeetleImportMap(bombadeetle->levelIndex);
                bombadeetleLoadMap();

            }


            if (bombadeetle->stageSelectIndex + (bombadeetle->stageSelectPageIndex * 20) >= bombadeetle->levelMax)
            {
                ESP_LOGI(TAG, "FIXED %d %d", bombadeetle->stageSelectIndex , (bombadeetle->levelMax - 1) % 20);
                bombadeetle->stageSelectIndex = (bombadeetle->levelMax) % 20;

                if (bombadeetle->stageSelectIndex < 0) bombadeetle->stageSelectIndex = 0;

            }

            if (bombadeetle->stageSelectIndex + (bombadeetle->stageSelectPageIndex * 20) >= ARRAY_SIZE(bombadeetleLevels))
            {
                bombadeetle->stageSelectIndex = (ARRAY_SIZE(bombadeetleLevels)-1) % 20;
            }

        }
    }
    

    bombadeetleDrawSelect();
}

static void bombadeetleGameLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    int16_t tick = elapsedUs / 1000;
    int8_t gridIndex = bombadeetle->cursorX + (bombadeetle->cursorY * GRIDWIDTH);
    bool bombadeetleUpdate = false;
    bool shloogUpdate = false;
    int8_t winCount = 0;

    if (bombadeetle->paused) return;

    bombadeetle->animationTime += tick;
    if (bombadeetle->animationTime > ANIMATIONSPEED)
    {
        bombadeetle->animationTime -= ANIMATIONSPEED;

        bombadeetle->backgroundOffset++;
        bombadeetle->backgroundOffset %=16;
        
        bombadeetle->teleporterFrame++;
        bombadeetle->teleporterFrame %= ARRAY_SIZE(bombadeetleTeleporter);
        
        for(int i = 0; i < BOMBADEETLE_COUNT; i++)
        {
            if (bombadeetle->bombadeetles[i].direction != 0)
            {
                bombadeetle->bombadeetles[i].frame++;
                bombadeetle->bombadeetles[i].frame %= BOMBADEETLE_FRAMECOUNT;
            }
        }
        
        for(int i = 0; i < SHLOOG_MAX_COUNT; i++)
        {
            if (bombadeetle->shloogs[i].direction != 0)
            {
                bombadeetle->shloogs[i].frame++;
                bombadeetle->shloogs[i].frame %= BOMBADEETLE_FRAMECOUNT;
            }
        }
    }
    
    bombadeetle->cursorTime += tick;
    if (bombadeetle->cursorTime > CURSORSPEED)
    {

        bombadeetle->cursorTime -= CURSORSPEED;
        bombadeetle->cursorFrame ++;
        bombadeetle->cursorFrame %=4;
    }

    if (bombadeetle->cursorMoveTime > 0)
    {
        bombadeetle->cursorMoveTime -= tick;
    }
    

    //Controls
    switch (bombadeetle->state)
    {
        case STATE_WIN:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_A)
                    {
                        if (bombadeetle->successTime < TIMING_SUCCESS)
                        {
                            bombadeetle->successTime = TIMING_SUCCESS;
                        }
                        else
                        {

                            //Next map
                            
                            if (bombadeetle->levelIndex >= ARRAY_SIZE(bombadeetleLevels))
                            {
                                bombadeetle->levelIndex = ARRAY_SIZE(bombadeetleLevels ) - 1;
                            }
                            
                            
                            bombadeetleImportMap(bombadeetle->levelIndex);
                            bombadeetleLoadMap();
                        }
                    }

                    if (evt.button & PB_B)
                    {
                        //Reset map
                        bombadeetle->state = STATE_STAGESELECT;
                    }
                }
            }
        case STATE_RUNNING:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_B)
                    {
                        //Reset map
                        bombadeetleLoadMap();
                    }

                    if (evt.button & PB_SELECT)
                    {
                        bombadeetle->paused = !bombadeetle->paused;

                    }
                }
            }

            linearTouch_t touches[2] = {0};
            getTouchLinear(touches, ARRAY_SIZE(touches));
            for (uint8_t tIdx = 0; tIdx < ARRAY_SIZE(touches); tIdx++)
            {
                if (touches[tIdx].touched && tIdx == 0)
                {

                    int speed = touches[tIdx].position / 125;
                    if (speed % 2 == 1) speed--;

                    if (speed < 2) speed = 2;
                    if (speed > 8) speed = 8;
                    ESP_LOGI(TAG, "Game speed %d", speed);
                    bombadeetle->gameSpeed = speed;

                }
            }
        case STATE_PLACING:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {

                    if (bombadeetle->building)
                    {
                        if (evt.button & PB_DOWN && bombadeetle->mapFile.down)
                        {
                            bombadeetle->mapFile.down--;
                            bombadeetle->grid[gridIndex] = DIRECTION_S;   
                            
                            bombadeetle->building = false;                    
                        }
                        else if (evt.button & PB_UP && bombadeetle->mapFile.up)
                        {
                            bombadeetle->mapFile.up--;
                            bombadeetle->grid[gridIndex] = DIRECTION_N;
                            
                            bombadeetle->building = false;
                        }
                        else if (evt.button & PB_LEFT && bombadeetle->mapFile.left)
                        {
                            bombadeetle->grid[gridIndex] = DIRECTION_W;
                            bombadeetle->mapFile.left--;
                            
                            bombadeetle->building = false;
                        }
                        else if (evt.button & PB_RIGHT && bombadeetle->mapFile.right)
                        {
                            bombadeetle->grid[gridIndex] = DIRECTION_E;
                            bombadeetle->mapFile.right--;
                            
                            bombadeetle->building = false;
                        }
                    }
                    else {
                        
                        if (evt.button & PB_DOWN)
                        {
                            bombadeetle->cursorY++;                        
                            bombadeetle->cursorMoveTime = MOVETIME;       
                        }
                        else if (evt.button & PB_UP)
                        {
                            bombadeetle->cursorY--;
                            bombadeetle->cursorMoveTime = MOVETIME;
                        }
                        else if (evt.button & PB_LEFT)
                        {
                            bombadeetle->cursorX--;
                        }
                        else if (evt.button & PB_RIGHT)
                        {
                            bombadeetle->cursorX++;
                        }
                    }

                    if (evt.button & PB_B && !bombadeetle->building)
                    {                        
                        bombadeetle->state = STATE_RUNNING;
                        bombadeetleCheckShloogs(false);
                        bombadeetleCheckBombadeetles(false);

                        
                    }

                }
                
                if (evt.button & PB_A)
                {
                    if (evt.down)
                    {
                        switch (bombadeetle->grid[gridIndex])
                        {
                            case DIRECTION_N:
                                bombadeetle->mapFile.up++;
                                bombadeetle->building = false;                               
                                bombadeetle->grid[gridIndex] = 0;   
                                break;
                            case DIRECTION_S:
                                bombadeetle->mapFile.down++;
                                bombadeetle->building = false;                               
                                bombadeetle->grid[gridIndex] = 0;   
                                break;
                                
                            case DIRECTION_E:
                                bombadeetle->mapFile.right++;
                                bombadeetle->building = false;                               
                                bombadeetle->grid[gridIndex] = 0;   
                                break;
                                
                            case DIRECTION_W:
                                bombadeetle->mapFile.left++;
                                bombadeetle->building = false;                                
                                bombadeetle->grid[gridIndex] = 0;   
                                break;
                            default:
                                bombadeetle->building = true;
                                break;

                        }
                        
                    }
                    else
                    {
                        bombadeetle->building = false;
                    }
                }
            }
            break;
        case STATE_COLLISION:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_B)
                    {
                        //Reset map
                        bombadeetleLoadMap();
                    }
                }
            }
            break;
        default:
            break;
    }
    
    //Level updating
    if (bombadeetle->state == STATE_RUNNING)
    {
        bombadeetle->gameMoveTime += tick;

        while (bombadeetle->gameMoveTime >= GAME_MOVE_TIME)
        {
            bombadeetle->gameMoveTime -= GAME_MOVE_TIME;
            bombadeetle->gameMoveTick++;
            bombadeetle->gameMoveTick %= 200;
            shloogUpdate = false;
            bombadeetleUpdate = false;

            if (bombadeetle->gameMoveTime % BOMBADEETLE_MOVE_TIME == 0)
            {
                bombadeetle->bombadeetleMoveAmount += bombadeetle->gameSpeed;
                if (bombadeetle->bombadeetleMoveAmount >= TILESIZE)
                {
                    bombadeetle->bombadeetleMoveAmount = 0;
                    bombadeetleUpdate = true;
                }
            }

            if (bombadeetle->gameMoveTime % SHLOOG_MOVE_TIME == 0)
            {
                bombadeetle->shloogMoveAmount += bombadeetle->gameSpeed;
                            
                if (bombadeetle->shloogMoveAmount >= TILESIZE)
                {
                    bombadeetle->shloogMoveAmount = 0;
                    shloogUpdate = true;
                }
            }            

            bombadeetleCheckBombadeetles(bombadeetleUpdate);
            bombadeetleCheckShloogs(shloogUpdate);

        }        

        if (bombadeetle->goalAnimating)
        {
            bombadeetle->goalTime += tick;
            if (bombadeetle->goalTime >= 75)
            {
                bombadeetle->goalTime -= 75;
                bombadeetle->goalFrame++;
                
                if (bombadeetle->goalFrame >= ARRAY_SIZE(bombadeetleGoalAnimation))
                {
                    bombadeetle->goalFrame = 0;
                    bombadeetle->goalAnimating = false;
                }
            }
        }

        
        for (int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
        {
            if (bombadeetle->bombadeetles[idx].goal)
            {
                winCount++;
            }
        }

        if (winCount >= bombadeetle->goalCount)
        {
            ESP_LOGI(TAG, "WIN!");
            bombadeetle->state = STATE_WIN;
            
            bombadeetle->levelIndex++;
            bombadeetle->successTime = 0;
            
            if (bombadeetle->levelIndex > bombadeetle->levelMax)
            {
                bombadeetle->levelMax = bombadeetle->levelIndex;
                //Save max level
            }

        }

    }

    if (bombadeetle->state == STATE_WIN)
    {
        bombadeetle->successTime += tick;
        if (bombadeetle->successTime > TIMING_SUCCESS)
        {
            bombadeetle->successTime = TIMING_SUCCESS;
        }
    }

    if (bombadeetle->cursorY >= GRIDHEIGHT)
    {
        bombadeetle->cursorY = GRIDHEIGHT - 1;
    }
    if (bombadeetle->cursorY < 0)
    {
        bombadeetle->cursorY = 0;
    }
    if (bombadeetle->cursorX < 0)
    {
        bombadeetle->cursorX = 0;
    }
    if (bombadeetle->cursorX >= GRIDWIDTH)
    {
        bombadeetle->cursorX = GRIDWIDTH - 1;
    }
    
    bombadeetleDrawGame();
}

static void bombadeetleDrawBackground()
{
    //Drawing has to be the last thing in the stack
    for(int x = -1; x < 19; x++)
    {
        for (int y = -1; y < 17; y++)
        {
            drawWsgSimple(&bombadeetle->backgroundTile , (x * 16)+ bombadeetle->backgroundOffset, (y * 16) + bombadeetle->backgroundOffset);
        }

    }

}

static void bombadeetleDrawSelect()
{
    
    bombadeetleDrawBackground();

    
    drawWsgSimple(&bombadeetle->stageSelect, 15, 14);
    
    char buffer[32];
    int displayIndex = 0;
    int maxLevels = ARRAY_SIZE(bombadeetleLevels);
    
    for (int8_t idx = 0; idx < 20; idx++)
    {
        //stageActiveSelectButton
        displayIndex = (idx + (bombadeetle->stageSelectPageIndex * 20)) + 1;

        if (displayIndex > maxLevels) continue;
        if (displayIndex - 1 > bombadeetle->levelMax) 
        {
            break;
        }

        if (idx == bombadeetle->stageSelectIndex)
        {
            drawWsgSimple(&bombadeetle->stageActiveSelectButton, OFFSETSELECTBUTTON_X + ((idx % 5) * 42), OFFSETSELECTBUTTON_Y + ((idx / 5) * 32));
        }
        else
        {
            drawWsgSimple(&bombadeetle->stageSelectButton, OFFSETSELECTBUTTON_X + ((idx % 5) * 42), OFFSETSELECTBUTTON_Y + ((idx / 5) * 32));
        }
        
        if (displayIndex < 10)
        {
            sprintf(buffer,  "0%d" , displayIndex);
        }
        else
        {
            sprintf(buffer,  "%d" , displayIndex);
        }

        drawText(&bombadeetle->mainFont, c555, buffer, OFFSETSELECTBUTTON_X + ((idx % 5) * 42) + 10, OFFSETSELECTBUTTON_Y + ((idx / 5) * 32) + 5);
    }
}

static void bombadeetleDrawGame()
{

    int tileIndex = 0;
    bombadeetleDrawBackground();

    //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c112);

    for (int y = 0; y < GRIDHEIGHT; y++)
    {
        for (int x = 0; x < GRIDWIDTH; x++)
        {      
            tileIndex = (y * GRIDWIDTH)+ x;
            drawWsgSimple(&bombadeetle->background.tiles[(x+y)% 2], OFFSETMAP_X + (x * 16), OFFSETMAP_Y + (y * 16));
            if (bombadeetle->grid[tileIndex] != 0)
            {
                
                drawWsgSimple(&bombadeetle->arrows[__builtin_ctz(bombadeetle->grid[tileIndex])],  OFFSETMAP_X + (x * 16 + 1), OFFSETMAP_Y + (y * 16) + 1);
            }
        }
    }

    
    for (int y = 0; y < GRIDHEIGHT; y++)
    {
        for (int x = 0; x < GRIDWIDTH; x++)
        {
            tileIndex = (y * GRIDWIDTH)+ x;
            
            if (bombadeetle->map[tileIndex] & GOAL)
            {
                drawWsgSimple(&bombadeetle->goal[bombadeetleGoalAnimation[bombadeetle->goalFrame]],OFFSETMAP_X + (x * TILESIZE), OFFSETMAP_Y + (y * TILESIZE));
            }

            if (bombadeetle->map[tileIndex] & HOLE)
            {
                drawWsgSimple(&bombadeetle->holeSprite, OFFSETMAP_X + (x * TILESIZE), OFFSETMAP_Y + (y * TILESIZE));
            }

            
            if (bombadeetle->map[tileIndex] & TELEPORT)
            {
                drawWsgSimple(&bombadeetle->teleporterSprites[bombadeetle->teleporterFrame], OFFSETMAP_X + (x * TILESIZE), OFFSETMAP_Y + (y * TILESIZE));
            }

            if (y == 0)
            {
                drawWsgSimple(&bombadeetle->walls[1], OFFSETMAP_X + (x * TILESIZE) - 2,  OFFSETMAP_Y + (y * TILESIZE) - 2);
            }

            if (x == 0)
            {
                drawWsgSimple(&bombadeetle->walls[0], OFFSETMAP_X + ((x) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE) - 2);
                
            }
            
            if (bombadeetle->map[tileIndex] & WALL_E)
            {
                drawWsgSimple(&bombadeetle->walls[0], OFFSETMAP_X + ((x+1) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE) - 2);
                //drawRectFilled(OFFSETMAP_X + ((x+1) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE), OFFSETMAP_X + ((x+1) * TILESIZE) + 2, OFFSETMAP_Y + ((y +1) * TILESIZE) + 2, c505);
            }
            
            if (bombadeetle->map[tileIndex] & WALL_S)
            {
                drawWsgSimple(&bombadeetle->walls[1], OFFSETMAP_X + (x * TILESIZE) -2,  OFFSETMAP_Y + ((y+1) * TILESIZE) - 2);
                
                // drawRectFilled(OFFSETMAP_X + (x * TILESIZE) , OFFSETMAP_Y + ((y+1) * TILESIZE) - 2, OFFSETMAP_X + ((x+1) * TILESIZE), OFFSETMAP_Y + ((y+1) * TILESIZE) + 2, c505);
            }
            
        }
    }
    //
    
    for( int idx =0; idx < BOMBADEETLE_COUNT; idx++)
    {
        switch (bombadeetle->bombadeetles[idx].direction)
        {
            case DIRECTION_E:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame]],OFFSETMAP_X + bombadeetle->bombadeetles[idx].locX, OFFSETMAP_Y + bombadeetle->bombadeetles[idx].locY);
            //drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame]],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE) + bombadeetle->bombadeetleMoveAmount, OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE));
            break;
            case DIRECTION_N:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame] + 3],OFFSETMAP_X + bombadeetle->bombadeetles[idx].locX, OFFSETMAP_Y + bombadeetle->bombadeetles[idx].locY);
            break;
            case DIRECTION_S:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame] + 6],OFFSETMAP_X + bombadeetle->bombadeetles[idx].locX, OFFSETMAP_Y + bombadeetle->bombadeetles[idx].locY);
            break;
            
            case DIRECTION_W:
            drawWsg(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame]],OFFSETMAP_X + bombadeetle->bombadeetles[idx].locX, OFFSETMAP_Y + bombadeetle->bombadeetles[idx].locY, true, false, 0);
            break;
            
            case GOAL:
            //drawRectFilled(OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE), OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE) + 16, OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE) + 16, c505);
            break;
            
            default:
            break;
        }
    }

    for( int idx =0; idx < SHLOOG_MAX_COUNT; idx++)
    {
        switch (bombadeetle->shloogs[idx].direction)
        {
            case DIRECTION_E:
            drawWsgSimple(&bombadeetle->shloogSprites[bombadeetleBeetleFrames[bombadeetle->shloogs[idx].frame]],OFFSETMAP_X + bombadeetle->shloogs[idx].locX, OFFSETMAP_Y + bombadeetle->shloogs[idx].locY );
            break;
            case DIRECTION_N:
            drawWsgSimple(&bombadeetle->shloogSprites[bombadeetleBeetleFrames[bombadeetle->shloogs[idx].frame] + 3],OFFSETMAP_X + bombadeetle->shloogs[idx].locX, OFFSETMAP_Y + bombadeetle->shloogs[idx].locY );
            break;
            case DIRECTION_S:
            drawWsgSimple(&bombadeetle->shloogSprites[bombadeetleBeetleFrames[bombadeetle->shloogs[idx].frame] + 6],OFFSETMAP_X + bombadeetle->shloogs[idx].locX, OFFSETMAP_Y + bombadeetle->shloogs[idx].locY );
            break;
            
            case DIRECTION_W:
            drawWsg(&bombadeetle->shloogSprites[bombadeetleBeetleFrames[bombadeetle->shloogs[idx].frame]],OFFSETMAP_X + bombadeetle->shloogs[idx].locX, OFFSETMAP_Y + bombadeetle->shloogs[idx].locY , true, false, 0);
            break;            
            default:
            break;
        }
    }
    
    drawWsgSimple(&bombadeetle->tools, 2, 31);

    char buffer[32];
    if (bombadeetle->mapFile.right > 0)
    {
        sprintf(buffer,  "%d" , bombadeetle->mapFile.right );
        drawText(&bombadeetle->mainFont, COLOR_BOMBLUE, buffer, 16,70);
    }
    
    if (bombadeetle->mapFile.left > 0)
    {
        sprintf(buffer,  "%d" , bombadeetle->mapFile.left );
        drawText(&bombadeetle->mainFont, COLOR_BOMBLUE, buffer, 16,105);
    }
    
    if (bombadeetle->mapFile.down > 0)
    {
        sprintf(buffer,  "%d" , bombadeetle->mapFile.down );
        drawText(&bombadeetle->mainFont, COLOR_BOMBLUE, buffer, 16,140);
    }
    
    if (bombadeetle->mapFile.up > 0)
    {
        sprintf(buffer,  "%d" , bombadeetle->mapFile.up );
        drawText(&bombadeetle->mainFont, COLOR_BOMBLUE, buffer, 16,175);
    }

    drawWsgSimple(&bombadeetle->levelNameBackground, BG_LEVELNAME_X, BG_LEVELNAME_Y);
    drawText(&bombadeetle->mainFont, c225, bombadeetle->mapFile.name, BG_LEVELNAME_X + 10, BG_LEVELNAME_Y + 8);
    
    
    if (bombadeetle->state == STATE_PLACING) 
    {
        if (bombadeetle->building)
        {
            drawWsgSimple(&bombadeetle->acursor[bombadeetleCursorFrames[bombadeetle->cursorFrame]], OFFSETMAP_X + (bombadeetle->cursorX * TILESIZE) - 4, OFFSETMAP_Y + (bombadeetle->cursorY * TILESIZE) - 4);
        }
        else
        {
            drawWsgSimple(&bombadeetle->cursor[bombadeetleCursorFrames[bombadeetle->cursorFrame]], OFFSETMAP_X + (bombadeetle->cursorX * TILESIZE) - 2, OFFSETMAP_Y + (bombadeetle->cursorY * TILESIZE) - 2);
        }
    }

    if (bombadeetle->state == STATE_COLLISION)
    {
        drawWsgSimple(&bombadeetle->collisionSprite, OFFSETMAP_X + bombadeetle->collisionX,OFFSETMAP_Y +  bombadeetle->collisionY);
    }
    
    if (bombadeetle->state == STATE_WIN) 
    {
        float successTime = 1 - ((float)bombadeetle->successTime/TIMING_SUCCESS);
        drawWsgSimple(&bombadeetle->success, 33,61 + (200 * successTime));
    }
    
}

static void bombadeetleExitMode()
{
    freeFont(&bombadeetle->mainFont);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleShloogSprite); idx++)
    {
        freeWsg( &bombadeetle->shloogSprites[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleGoal); idx++)
    {
        freeWsg(&bombadeetle->goal[idx]);
    }
       
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleBombadeetleSprite); idx++)
    {
        freeWsg( &bombadeetle->bombadeetleSprites[idx]);
    }
    
     for (int idx = 0; idx < ARRAY_SIZE(bombadeetleWall); idx++)
    {
        freeWsg( &bombadeetle->walls[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleActiveCursor); idx++)
    {
        freeWsg(&bombadeetle->acursor[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleCursor); idx++)
    {
        freeWsg( &bombadeetle->cursor[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleTeleporter); idx++)
    {
        freeWsg( &bombadeetle->teleporterSprites[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleArrows); idx++)
    {
        freeWsg( &bombadeetle->arrows[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleTiles); idx++)
    {
        freeWsg(&bombadeetle->background.tiles[idx]);
    }
    
    freeWsg(bombadeetle->shloogSprites);
    freeWsg(bombadeetle->goal);
    freeWsg(bombadeetle->bombadeetleSprites);
    freeWsg(bombadeetle->walls);
    freeWsg(bombadeetle->acursor);
    freeWsg(bombadeetle->cursor);
    freeWsg(bombadeetle->teleporterSprites);
    freeWsg(bombadeetle->arrows);
    freeWsg(bombadeetle->background.tiles);
    
    freeWsg(&bombadeetle->backgroundTile);
    freeWsg( &bombadeetle->success);
    freeWsg( &bombadeetle->levelNameBackground);
    freeWsg( &bombadeetle->tools);
    freeWsg( &bombadeetle->collisionSprite);
    freeWsg( &bombadeetle->holeSprite);

    heap_caps_free(bombadeetle->bombadeetles);
    heap_caps_free(bombadeetle->shloogs);

    heap_caps_free(bombadeetle);
   
    
}
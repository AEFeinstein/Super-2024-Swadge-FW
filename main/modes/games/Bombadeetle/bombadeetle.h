#pragma once
#include "settingsManager.h"
#include "swadge.h"

#define GRIDHEIGHT 9
#define GRIDWIDTH  12

typedef enum
{
    STATE_SWADGEMENU,
    STATE_MENU,
    STATE_STAGESELECT,
    STATE_INSTRUCTIONS,
    STATE_PLACING,
    STATE_RUNNING,
    STATE_COLLISION,
    STATE_WIN,

} bombState_t;

typedef struct
{
    wsg_t* tiles;

} bombadeetleBackground_t;

typedef struct
{
    char name[17];
    int8_t map[GRIDWIDTH * GRIDHEIGHT];
    int8_t bombadeetleSpawn[GRIDWIDTH * GRIDHEIGHT];
    int8_t shloogSpawn[GRIDWIDTH * GRIDHEIGHT];
    int8_t left;
    int8_t up;
    int8_t down;
    int8_t right;
    uint8_t nameWidthOffset;

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
    bombState_t lastState;

    bombadeetleCurrentLevel_t mapFile;

    wsg_t* arrows;
    wsg_t* cursor;
    wsg_t* acursor;
    wsg_t* walls;
    wsg_t* bombadeetleSprites;
    wsg_t* shloogSprites;
    wsg_t* teleporterSprites;
    wsg_t* instructionsPages;
    wsg_t* goal;
    wsg_t backgroundTile;
    wsg_t success;
    wsg_t unsuccessful;
    wsg_t genericBackground;
    wsg_t mainTitle;
    wsg_t mainSelect;
    wsg_t mainOptions;
    wsg_t stageSelect;
    wsg_t stageSelectButton;
    wsg_t stageActiveSelectButton;
    wsg_t pausedBackground;
    wsg_t levelNameBackground;
    wsg_t tools;
    wsg_t collisionSprite;
    wsg_t holeSprite;
    wsg_t noMoreIcon;

    bool building;
    bool goalAnimating;
    bool losePrompt;

    // QoL
    bool backgroundMoving;
    int wallType;
    int tileSet;

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
    int8_t backgroundSpeed;

    int8_t holdingDir;
    int32_t timeHeldDireciton;

    int16_t successTime;

    int16_t collisionX;
    int16_t collisionY;

    int8_t mainSelectIndex;

    int8_t stageSelectIndex;
    int8_t stageSelectPageIndex;

    int8_t instructionsPage;

    int8_t bombadeetleMoveAmount;
    int8_t shloogMoveAmount;
    font_t mainFont;
    font_t menuFont;

    int16_t menuTimer;
    int32_t dontHaveArrowTimer[4];

    int16_t cursorMoveTime;
    int16_t gameMoveTime;
    int16_t gameMoveTick;
    int8_t gameSpeed;

    int8_t grid[GRIDHEIGHT * GRIDWIDTH]; // I don't like this.
    int8_t map[GRIDHEIGHT * GRIDWIDTH];

    bombadeetleEntity_t* bombadeetles;
    bombadeetleEntity_t* shloogs;

    menu_t* menu;
    menuZorldoRenderer_t* renderer;
    
    midiFile_t music[6];

    bool paused;
    bool gameStarted;

} bombadeetleData_t;




extern swadgeMode_t bombadeetleMode;
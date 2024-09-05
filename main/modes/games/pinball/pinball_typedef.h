#pragma once

#include <stdint.h>
#include <float.h>
#include <stdbool.h>
#include <stddef.h>

#include "vector2d.h"
#include "vectorFl2d.h"
#include "macros.h"
#include "linked_list.h"

#define PIN_INVALID_ID 0xFFFF

typedef enum
{
    JS_WALL,
    JS_SLINGSHOT,
    JS_DROP_TARGET,
    JS_STANDUP_TARGET,
    JS_SPINNER,
    JS_SCOOP,
} jsLineType_t;

typedef enum
{
    JS_BUMPER,
    JS_ROLLOVER
} jsCircleType_t;

typedef enum
{
    JS_BALL_SPAWN,
    JS_ITEM_SPAWN,
} jsPointType_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    jsPointType_t type;
    vecFl_t pos;
} jsPoint_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    jsLineType_t type;
    vecFl_t p1;
    vecFl_t p2;
    float pushVel;
    bool isSolid;
    bool isUp;
    int32_t litTimer;
    int32_t resetTimer;
} jsLine_t;

typedef struct
{
    // fixed
    float radius;
    vecFl_t pos;
    float length;
    float restAngle;
    float maxRotation;
    float sign;
    float angularVelocity;
    // changing
    float rotation;
    float currentAngularVelocity;
    bool buttonHeld;
    bool facingRight;
} jsFlipper_t;

// TODO merge these
typedef struct
{
    float radius;
    float mass;
    float restitution;
    vecFl_t pos;
    vecFl_t vel;
    int32_t scoopTimer;
} jsBall_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    float radius;
    vecFl_t pos;
    jsCircleType_t type;
    float pushVel;
    int32_t litTimer;
} jsCircle_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    vecFl_t p1;
    vecFl_t p2;
    vecFl_t p3;
    bool isBlinking;
    bool isOn;
    int32_t blinkTimer;
} jsTriangle_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    vecFl_t pos;
    float width;
    float height;
    bool buttonHeld;
    float impulse;
} jsLauncher_t;

#define MAX_NUM_LINES     1024
#define MAX_NUM_BALLS     128
#define MAX_NUM_CIRCLES   128
#define MAX_NUM_FLIPPERS  128
#define MAX_NUM_LAUNCHERS 128
#define MAX_NUM_TRIANGLES 128
#define MAX_NUM_POINTS    128

typedef struct
{
    vecFl_t gravity;
    int32_t score;
    bool paused;
    int32_t numGroups;
    list_t* groups;
    jsLine_t lines[MAX_NUM_LINES];
    int32_t numLines;
    jsBall_t balls[MAX_NUM_BALLS];
    int32_t numBalls;
    jsCircle_t circles[MAX_NUM_CIRCLES];
    int32_t numCircles;
    jsFlipper_t flippers[MAX_NUM_FLIPPERS];
    int32_t numFlippers;
    jsLauncher_t launchers[MAX_NUM_LAUNCHERS];
    int32_t numLaunchers;
    jsTriangle_t triangles[MAX_NUM_TRIANGLES];
    int32_t numTriangles;
    jsPoint_t points[MAX_NUM_POINTS];
    int32_t numPoints;
    vec_t cameraOffset;
    vecFl_t tableDim;
    bool launchTubeClosed;
    uint16_t touchedLoopId;
    uint16_t loopHistory[3];
} jsScene_t;
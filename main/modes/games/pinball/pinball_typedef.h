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
    JS_BALL_LOST,
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

typedef struct
{
    vecFl_t gravity;
    int32_t score;
    bool paused;
    int32_t numGroups;
    list_t* groups;
    jsLine_t* lines;
    int32_t numLines;
    list_t balls;
    jsCircle_t* circles;
    int32_t numCircles;
    jsFlipper_t* flippers;
    int32_t numFlippers;
    jsLauncher_t* launchers;
    int32_t numLaunchers;
    jsTriangle_t* triangles;
    int32_t numTriangles;
    jsPoint_t* points;
    int32_t numPoints;
    vec_t cameraOffset;
    vecFl_t tableDim;
    bool launchTubeClosed;
    uint16_t touchedLoopId;
    uint16_t loopHistory[3];
    int32_t saveTimer;
    int32_t scoopCount;
    int32_t ballCount;
} jsScene_t;
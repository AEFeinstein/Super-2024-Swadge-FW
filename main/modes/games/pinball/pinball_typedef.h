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
    PB_WALL,
    PB_SLINGSHOT,
    PB_DROP_TARGET,
    PB_STANDUP_TARGET,
    PB_SPINNER,
    PB_SCOOP,
    PB_BALL_LOST,
    PB_LAUNCH_DOOR,
} pbLineType_t;

typedef enum
{
    PB_BUMPER,
    PB_ROLLOVER
} pbCircleType_t;

typedef enum
{
    PB_BALL_SPAWN,
    PB_ITEM_SPAWN,
} pbPointType_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    pbPointType_t type;
    vecFl_t pos;
} pbPoint_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    pbLineType_t type;
    vecFl_t p1;
    vecFl_t p2;
    float pushVel;
    bool isUp;
    int32_t litTimer;
    int32_t resetTimer;
} pbLine_t;

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
} pbFlipper_t;

// TODO merge these
typedef struct
{
    float radius;
    float mass;
    float restitution;
    vecFl_t pos;
    vecFl_t vel;
    int32_t scoopTimer;
} pbBall_t;

typedef struct
{
    uint16_t id;
    uint8_t groupId;
    list_t* group;
    float radius;
    vecFl_t pos;
    pbCircleType_t type;
    float pushVel;
    int32_t litTimer;
} pbCircle_t;

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
} pbTriangle_t;

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
} pbLauncher_t;

typedef struct
{
    vecFl_t gravity;
    int32_t score;
    bool paused;
    int32_t numGroups;
    list_t* groups;
    pbLine_t* lines;
    int32_t numLines;
    list_t balls;
    pbCircle_t* circles;
    int32_t numCircles;
    pbFlipper_t* flippers;
    int32_t numFlippers;
    pbLauncher_t* launchers;
    int32_t numLaunchers;
    pbTriangle_t* triangles;
    int32_t numTriangles;
    pbPoint_t* points;
    int32_t numPoints;
    vec_t cameraOffset;
    vecFl_t tableDim;
    bool launchTubeClosed;
    uint16_t touchedLoopId;
    uint16_t loopHistory[3];
    int32_t saveTimer;
    int32_t scoopCount;
    int32_t ballCount;
} pbScene_t;
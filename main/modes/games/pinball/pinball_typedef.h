#pragma once

#include <stdint.h>
#include <float.h>
#include <stdbool.h>
#include <stddef.h>

#include "vector2d.h"
#include "vectorFl2d.h"
#include "macros.h"
#include "linked_list.h"

typedef enum
{
    JS_WALL,
    JS_SLINGSHOT,
    JS_DROP_TARGET,
    JS_STANDUP_TARGET,
    JS_SPINNER,
} jsLineType_t;

typedef struct
{
    int32_t id;
    list_t* group;
    jsLineType_t type;
    vecFl_t p1;
    vecFl_t p2;
    float pushVel;
    bool isSolid;
    bool isUp;
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
} jsBall_t;

typedef struct
{
    int32_t id;
    list_t* group;
    float radius;
    vecFl_t pos;
    float pushVel;
} jsObstacle_t;

typedef struct
{
    int32_t id;
    list_t* group;
    vecFl_t pos;
    float width;
    float height;
    bool buttonHeld;
    float impulse;
} jsLauncher_t;

#define MAX_NUM_LINES     128
#define MAX_NUM_BALLS     128
#define MAX_NUM_OBSTACLES 128
#define MAX_NUM_FLIPPERS  128
#define MAX_NUM_LAUNCHERS 128

typedef struct
{
    vecFl_t gravity;
    float dt;
    int32_t score;
    bool paused;
    int32_t numGroups;
    list_t* groups;
    jsLine_t lines[MAX_NUM_LINES];
    int32_t numLines;
    jsBall_t balls[MAX_NUM_BALLS];
    int32_t numBalls;
    jsObstacle_t obstacles[MAX_NUM_OBSTACLES];
    int32_t numObstacles;
    jsFlipper_t flippers[MAX_NUM_FLIPPERS];
    int32_t numFlippers;
    jsLauncher_t launchers[MAX_NUM_LAUNCHERS];
    int32_t numLaunchers;
    vec_t cameraOffset;
    vecFl_t tableDim;
} jsScene_t;
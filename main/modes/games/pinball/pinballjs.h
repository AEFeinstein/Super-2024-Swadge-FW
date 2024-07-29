#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vectorFl2d.h>
#include "hdw-btn.h"

#define MAX_NUM_WALLS     128
#define MAX_NUM_BALLS     128
#define MAX_NUM_OBSTACLES 128
#define MAX_NUM_FLIPPERS  128
#define MAX_NUM_LAUNCHERS 128

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
    float radius;
    vecFl_t pos;
    float pushVel;
} jsObstacle_t;

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
} jsFlipper_t;

typedef struct
{
    vecFl_t p1;
    vecFl_t p2;
} jsLine_t;

typedef struct
{
    vecFl_t pos;
    float width;
    float height;
    bool buttonHeld;
    float impulse;
} jsLauncher_t;

typedef struct
{
    vecFl_t gravity;
    float dt;
    int32_t score;
    bool paused;
    jsLine_t walls[MAX_NUM_WALLS];
    int32_t numWalls;
    jsBall_t balls[MAX_NUM_BALLS];
    int32_t numBalls;
    jsObstacle_t obstacles[MAX_NUM_OBSTACLES];
    int32_t numObstacles;
    jsFlipper_t flippers[MAX_NUM_FLIPPERS];
    int32_t numFlippers;
    jsLauncher_t launchers[MAX_NUM_LAUNCHERS];
    int32_t numLaunchers;
} jsScene_t;

void jsSceneInit(jsScene_t* scene);
void jsSimulate(jsScene_t* scene);
void jsSceneDraw(jsScene_t* scene);
void jsButtonPressed(jsScene_t* scene, buttonEvt_t* event);

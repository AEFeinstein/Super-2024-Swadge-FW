#include "FoxsGame.h"
#include <math.h>

static const char foxName[] = "Fox";
static void foxMainLoop(int64_t elapsedUs);
static void foxEnterMode();
static void foxExitMode();

int fps = 1000000 / 30;
uint16_t btnState;
bool runner = true;

const int x1 = 100; 
const int z1 = 100;
const int x2 = 150;
const int z2 = 150;

typedef struct{
    int pos[2];
    int lastPos[2];
    int speed[2];
    int state;
    int falling;
    int facing;
} P1_t;

typedef struct{
    int pos[2];
    int lastPos[2];
    int speed[2];
    int state;
    int falling;
    int facing;
} Dummy_t;

swadgeMode_t foxMode = {
    .modeName = foxName,
    .wifiMode = ESP_NOW,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .overrideSelectBtn = false,
    .fnMainLoop = foxMainLoop,
    .fnEnterMode = foxEnterMode,
    .fnExitMode = foxExitMode
};

P1_t* P1 = NULL;
Dummy_t* Dummy = NULL;
static void foxEnterMode()
{
    P1 = calloc(1, sizeof(P1_t));
    Dummy = calloc(1, sizeof(Dummy_t));

    P1->pos[0] = 110;
    P1->pos[1] = 150;
    P1->facing = 1;
}

rectangle_t me = {
    .pos.x  = x1,
    .pos.y  = z1,
    .width  = 10,
    .height = 10,
};

rectangle_t other = {
    .pos.x  = x2,
    .pos.y  = z2,
    .width  = 10,
    .height = 10,
};


rectangle_t l5 = {
    .pos.x  = x1 + 15,
    .pos.y  = z1 + 5,
    .width  = 10,
    .height = 5,
};

rectangle_t h5 = {
    .pos.x  = x1 + 15,
    .pos.y  = z1 + 5,
    .width  = 20,
    .height = 5,
};

static void foxExitMode()
{
    free(P1);
}

void BGFill(){
    fillDisplayArea(0, 0, 280, 240, c000);
}

static void debug(){
    BGFill();

    drawRect(P1->pos[0], P1->pos[1], 10, 10, c555);
    if(P1->state >= 1){
        drawRect((P1->pos[0] + 8) - P1->facing, P1->pos[1] + 5, 10, 5, c555);
    }
}

/*
static void collisionCheck(){
}
*/

static void Player1() {
    P1->speed[0] = 0;
    P1->state = 0;

    P1->lastPos[0] = P1->pos[0];
    P1->lastPos[1] = P1->pos[1];

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnState = evt.state;
        printf("state: %04X, button: %d, down: %s\n",
               evt.state, evt.button, evt.down ? "down" : "up");
    }

    if (btnState & PB_A) {
        P1->state = 1;
    }
    if (btnState & PB_LEFT) {
        P1->speed[0] = -2;
        P1->facing = 16;
    }
    if (btnState & PB_RIGHT) {
        P1->speed[0] = 2;
        P1->facing = 0;
    }
    if (btnState & PB_UP && P1->falling < 2) {
        P1->pos[1] -= 1;
        P1->speed[1] -= 5;
    }

    if ( P1->pos[1] < 150 ) {
        P1->speed[1] += 1;
        P1->falling += 1;
    }
    if( P1->pos[1] >= 150 ) {
        P1->speed[1] = 0;
        P1->pos[1] = 150;
        P1->falling = 0;
    }

    P1->pos[0] += P1->speed[0];
    P1->pos[1] += P1->speed[1];

    // collisionCheck(); //
}

static void foxMainLoop(int64_t elapsedUs)
{
    setFrameRateUs(fps);
    
    Player1();
    debug();
}
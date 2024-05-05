#include "FoxsGame.h"
#include <math.h>

static const char foxName[] = "Fox";
static void foxMainLoop(int64_t elapsedUs);
static void foxEnterMode();
static void foxExitMode();

int fps = 1000000 / 30;
uint16_t btnState;


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

int hitBoxes[2][4][4] = {
    {
        {0, 0, 10, 0},
        {10, 0, 10, 10},
        {10, 10, 0, 10},
        {0, 10, 0, 0}
    },
    {
        {0, 150, 280, 150},
        {280, 150, 280, 240},
        {280, 240, 0, 240},
        {0, 240, 0, 150}
    }
};

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

static void foxExitMode()
{
    free(P1);
}

void BGFill(){
    fillDisplayArea(0, 0, 280, 240, c000);
}

static void debug(){
    BGFill();

    for(int z = 0; z < 2; z++){
        if( z == 0 ){
            drawLineFast(hitBoxes[z][0][0] + P1->pos[0], hitBoxes[z][0][1] + P1->pos[1], hitBoxes[z][0][2] + P1->pos[0], hitBoxes[z][0][3] + P1->pos[1], c555);
            drawLineFast(hitBoxes[z][1][0] + P1->pos[0], hitBoxes[z][1][1] + P1->pos[1], hitBoxes[z][1][2] + P1->pos[0], hitBoxes[z][1][3] + P1->pos[1], c555);
            drawLineFast(hitBoxes[z][2][0] + P1->pos[0], hitBoxes[z][2][1] + P1->pos[1], hitBoxes[z][2][2] + P1->pos[0], hitBoxes[z][2][3] + P1->pos[1], c555);
            drawLineFast(hitBoxes[z][3][0] + P1->pos[0], hitBoxes[z][3][1] + P1->pos[1], hitBoxes[z][3][2] + P1->pos[0], hitBoxes[z][3][3] + P1->pos[1], c555);
        }
    }

    if(P1->state >= 1){
        drawLineFast(hitBoxes[0][0][0] + P1->pos[0], hitBoxes[0][0][1] + P1->pos[1], hitBoxes[0][0][2] + P1->pos[0], hitBoxes[0][0][3] + P1->pos[1], c555);
        drawLineFast(hitBoxes[0][1][0] + P1->pos[0], hitBoxes[0][1][1] + P1->pos[1], hitBoxes[0][1][2] + P1->pos[0], hitBoxes[0][1][3] + P1->pos[1], c555);
        drawLineFast(hitBoxes[0][2][0] + P1->pos[0], hitBoxes[0][2][1] + P1->pos[1], hitBoxes[0][2][2] + P1->pos[0], hitBoxes[0][2][3] + P1->pos[1], c555);
        drawLineFast(hitBoxes[0][3][0] + P1->pos[0], hitBoxes[0][3][1] + P1->pos[1], hitBoxes[0][3][2] + P1->pos[0], hitBoxes[0][3][3] + P1->pos[1], c555);
    }
}

static void Player1() {
    P1->speed[0] = 0;
    P1->state = 0;

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
}

static void foxMainLoop(int64_t elapsedUs)
{
    setFrameRateUs(fps);
    
    Player1();
    debug();
}
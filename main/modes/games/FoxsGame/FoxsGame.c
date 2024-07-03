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
    int states[2];
    int thisState;
    int lastState;
    int stateTime;
    int floorCol;
    int facing;
} P1_t;

typedef struct{
    int pos[2];
    int lastPos[2];
    int speed[2];
    int states[2];
    int thisState;
    int lastState;
    int stateTime;
    int floorCol;
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

static void foxExitMode()
{
    free(P1);
}

void BGFill(){
    fillDisplayArea(0, 0, 280, 240, c000);
}

int hitBoxes[2][4][2] = {
    {
        {0, 0},
        {20, 0},
        {20, 30},
        {0, 30}
    },
    {
        {0, 160},
        {279, 160},
        {279, 239},
        {0, 239}
    }
}; int h = 30;

static void debug(){
    BGFill();

    for(int z = 0; z < 4; z++){
        if( z <= 2 ){
            drawLineFast(hitBoxes[0][z][0] + P1->pos[0], hitBoxes[0][z][1] + P1->pos[1], hitBoxes[0][z + 1][0] + P1->pos[0], hitBoxes[0][z + 1][1] + P1->pos[1], c555);
            drawLineFast(hitBoxes[1][z][0], hitBoxes[1][z][1], hitBoxes[1][z + 1][0], hitBoxes[1][z + 1][1], c555);
        } else if ( z == 3 ){
            drawLineFast(hitBoxes[0][z][0] + P1->pos[0], hitBoxes[0][z][1] + P1->pos[1], hitBoxes[0][0][0] + P1->pos[0], hitBoxes[0][1][1] + P1->pos[1], c555);
            drawLineFast(hitBoxes[1][z][0], hitBoxes[1][z][1], hitBoxes[1][0][0], hitBoxes[1][0][1], c555);
        }
    }
}

static void stateMachine(){
    P1->lastState = P1->thisState;

    /* 
        States:
        0: Idle
        1: WalkForward
        2: WalkBack
        3: Crouch
        4: Jump
        5: AirFall
        6: AirDash ( J.A + J.B )
        7: G.A5
        8: G.A2
        9: G.A3 / A4
        10: G.B5
        11: G.B2
        12: G.B3 / G.B4
        13: J.A5
        14: J.A2
        15: J.A3 / J.A4
        16: J.B5
        17: J.B2
        18: J.B3 / J.B4
        19: Hurt-Ground
        20: Hurt-Air
        21: KnockDown
        22: Bounce
    */

    if(btnState & PB_A && P1->states[0] == 0  && P1->states[0] == 0){
        P1->thisState = 7;
    }
    if(btnState & PB_A && P1->states[0] == 0  && P1->states[0] == 2){
        P1->thisState = 8;
    }
    if(btnState & PB_A && P1->states[0] == 1  && P1->states[0] == 2 && P1->facing == 0){
        P1->thisState = 9;
    }
    if(btnState & PB_A && P1->states[0] == 2  && P1->states[0] == 2 && P1->facing == 1){
        P1->thisState = 9;
    }

    if(btnState & PB_B && P1->states[0] == 0  && P1->states[0] == 0){
        P1->thisState = 10;
    }
    if(btnState & PB_B && P1->states[0] == 0  && P1->states[0] == 2){
        P1->thisState = 11;
    }
    if(btnState & PB_B && P1->states[0] == 1  && P1->states[0] == 2 && P1->facing == 0){
        P1->thisState = 12;
    }
    if(btnState & PB_B && P1->states[0] == 2  && P1->states[0] == 2 && P1->facing == 1){
        P1->thisState = 12;
    }

}

static void btnStateMaker(){
    P1->states[0] = 0;
    P1->states[1] = 0;

    if(btnState & PB_UP){
        P1->states[0] = 1;
    }
    if(btnState & PB_DOWN){
        P1->states[0] = 2;
    }
    if(btnState & PB_LEFT){
        P1->states[1] = 2;
    }
    if(btnState & PB_RIGHT){
        P1->states[1] = 1;
    }
}

static void runStates(){
    if(P1->lastState != P1->thisState){
        P1->stateTime = 0;
    }


}

static void Player1() {
    P1->speed[0] = 0;
    hitBoxes[0][0][1] = 0;
    hitBoxes[0][1][1] = 0;

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnState = evt.state;
        printf("state: %04X, button: %d, down: %s\n",
               evt.state, evt.button, evt.down ? "down" : "up");
    }

    btnStateMaker();
    stateMachine();

    if(P1->thisState <= 6){
        if (P1->states[0] == 2 && P1->floorCol == 0) {
            hitBoxes[0][0][1] = 15;
            hitBoxes[0][1][1] = 15;
        }
        if (P1->states[1] == 2) {
            P1->speed[0] = -2;
            P1->facing = 1;
        }
        if (P1->states[1] == 1) {
            P1->speed[0] = 2;
            P1->facing = 0;
        }
        if (P1->states[0] == 1 && P1->floorCol == 0) {
            P1->pos[1] -= 1;
            if(btnState & PB_DOWN){
                P1->speed[1] -= 15;
            } else {
                P1->speed[1] -= 10;
            }
            P1->floorCol = 1;
        }
    }

    runStates();

    P1->pos[0] += P1->speed[0];
    P1->pos[1] += P1->speed[1];

    if ( P1->pos[1] < (160 - h) ) {
        P1->speed[1] += 1;
        P1->floorCol = 1;
    }
    if( P1->pos[1] >= (160 - h) ) {
        P1->speed[1] = 0;
        P1->pos[1] = (160 - h);
        P1->floorCol = 0;
    }
}

static void foxMainLoop(int64_t elapsedUs)
{
    setFrameRateUs(fps);
    
    Player1();
    debug();
}
#include "FoxsGame.h"
#include <math.h>
#include <stdbool.h>

static const char foxName[] = "Fox";
static void foxMainLoop(int64_t elapsedUs);
static void foxEnterMode();
static void foxExitMode();

int fps = 1000000 / 30;
uint16_t btnState;
int counter[3];

typedef struct{
    int pos[2];
    int lastPos[2];
    int speed[2];
    int states[2];
    int drawBox;
    int thisState;
    int lastState;
    int stateTime;
    int floorCol;
    int facing;
    int extraStateTime;
} P1_t;

typedef struct{
    int pos[2];
    int lastPos[2];
    int speed[2];
    int extraStuff[4];
    int thisState;
    int hitState;
    int stun;
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

    Dummy->pos[0] = 200;
    Dummy->pos[1] = 150;
    Dummy->facing = 0;
}

static void foxExitMode()
{
    free(P1);
}

void BGFill(){
    fillDisplayArea(0, 0, 280, 240, c000);
}

int hitBoxes[3][4][2] = {
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
    },
    {
        {0, 0},
        {20, 0},
        {20, 30},
        {0, 30}
    }
}; 
int hurtBoxes[5][4][2] = {
    {{0, 15}, {40, 15}, {40, 20}, {0, 20}},
    {{15, 20}, {40, 20}, {40, 29}, {15, 29}},
    {{0, 20}, {35, 20}, {35, 29}, {0, 29}},
    {{0, 10}, {30, 10}, {30, 30}, {0, 30}},
    {{-10, -10}, {40, -10}, {40, 40}, {-10, 40}}
}; int h = 30; int dh = 30;

int length = sizeof(hurtBoxes) / sizeof(hurtBoxes[0]);
int restoreBox[12][4];

static void debug(){
    BGFill();

    for(int z = 0; z < 4; z++){
        if( z <= 2 ){
            drawLineFast(hitBoxes[0][z][0] + P1->pos[0], hitBoxes[0][z][1] + P1->pos[1], hitBoxes[0][z + 1][0] + P1->pos[0], hitBoxes[0][z + 1][1] + P1->pos[1], c555);
            drawLineFast(hitBoxes[1][z][0], hitBoxes[1][z][1], hitBoxes[1][z + 1][0], hitBoxes[1][z + 1][1], c555);
            
            if (Dummy->stun > 0){
                drawLineFast(hitBoxes[2][z][0] + Dummy->pos[0], hitBoxes[2][z][1] + Dummy->pos[1], hitBoxes[2][z + 1][0] + Dummy->pos[0], hitBoxes[2][z + 1][1] + Dummy->pos[1], c125);
            } else {
                drawLineFast(hitBoxes[2][z][0] + Dummy->pos[0], hitBoxes[2][z][1] + Dummy->pos[1], hitBoxes[2][z + 1][0] + Dummy->pos[0], hitBoxes[2][z + 1][1] + Dummy->pos[1], c555);
            }
        } else if ( z == 3 ){
            drawLineFast(hitBoxes[0][z][0] + P1->pos[0], hitBoxes[0][z][1] + P1->pos[1], hitBoxes[0][0][0] + P1->pos[0], hitBoxes[0][1][1] + P1->pos[1], c555);
            drawLineFast(hitBoxes[1][z][0], hitBoxes[1][z][1], hitBoxes[1][0][0], hitBoxes[1][0][1], c555);
            
            if(Dummy->stun > 0){
                drawLineFast(hitBoxes[2][z][0] + Dummy->pos[0], hitBoxes[2][z][1] + Dummy->pos[1], hitBoxes[2][0][0] + Dummy->pos[0], hitBoxes[2][1][1] + Dummy->pos[1], c125);
            } else {
                drawLineFast(hitBoxes[2][z][0] + Dummy->pos[0], hitBoxes[2][z][1] + Dummy->pos[1], hitBoxes[2][0][0] + Dummy->pos[0], hitBoxes[2][1][1] + Dummy->pos[1], c555);
            }      
        }
    }

    if( P1->drawBox != 200 ){
        for(int z = 0; z < 4; z++){
            if( z <= 2 ){
                drawLineFast(hurtBoxes[P1->drawBox][z][0] + P1->pos[0], hurtBoxes[P1->drawBox][z][1] + P1->pos[1], hurtBoxes[P1->drawBox][z + 1][0] + P1->pos[0], hurtBoxes[P1->drawBox][z + 1][1] + P1->pos[1], c555);
            } else if ( z == 3 ){
                drawLineFast(hurtBoxes[P1->drawBox][z][0] + P1->pos[0], hurtBoxes[P1->drawBox][z][1] + P1->pos[1], hurtBoxes[P1->drawBox][0][0] + P1->pos[0], hurtBoxes[P1->drawBox][1][1] + P1->pos[1], c555);
            }
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

    if(P1->floorCol == 0  && P1->thisState <= 6){
        if(btnState & PB_A){
            if(P1->states[0] == 0  && P1->states[1] == 0){
                P1->thisState = 7;
            }
            if(P1->states[0] == 2  && P1->states[1] == 0){
                P1->thisState = 8;
            }
            if(P1->states[0] == 2  && P1->states[1] == 1 && P1->facing == 1){
                P1->thisState = 9;
            }
            if(P1->states[0] == 2  && P1->states[1] == 2 && P1->facing == -1){
                P1->thisState = 9;
            }
        }

        if(btnState & PB_B){
            if(P1->states[0] == 0  && P1->states[1] == 0){
                P1->thisState = 10;
            }
            if(P1->states[0] == 2  && P1->states[1] == 0){
                P1->thisState = 11;
            }
            if(P1->states[0] == 2  && P1->states[1] == 1 && P1->facing == 1){
                P1->thisState = 12;
            }
            if(P1->states[0] == 2  && P1->states[1] == 2 && P1->facing == -1){
                P1->thisState = 12;
            }
        }
    }

    if(P1->floorCol == 1  && P1->thisState <= 6){
        if(btnState & PB_A){
            if(P1->states[0] == 0  && P1->states[1] == 0){
                P1->thisState = 13;
            }
            /*
            if(P1->states[0] == 2  && P1->states[1] == 0){
                P1->thisState = 8;
            }
            if(P1->states[0] == 2  && P1->states[1] == 1 && P1->facing == 1){
                P1->thisState = 9;
            }
            if(P1->states[0] == 2  && P1->states[1] == 2 && P1->facing == -1){
                P1->thisState = 9;
            }
            */
        }

        
        if(btnState & PB_B){
            if(P1->states[0] == 0  && P1->states[1] == 0){
                P1->thisState = 16;
            }
            /*
            if(P1->states[0] == 2  && P1->states[1] == 0){
                P1->thisState = 11;
            }
            if(P1->states[0] == 2  && P1->states[1] == 1 && P1->facing == 1){
                P1->thisState = 12;
            }
            if(P1->states[0] == 2  && P1->states[1] == 2 && P1->facing == -1){
                P1->thisState = 12;
            }
            */
        }
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
    P1->drawBox = 200;
    if(P1->lastState != P1->thisState){
        Dummy->hitState = 0;
        P1->stateTime = 0;
    }
    if(P1->lastState == P1->thisState){
        if(P1->thisState == 7){
            if(P1->stateTime > 3){
                P1->drawBox = 0;
            }
            if(P1->stateTime >= 6){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 8){
            hitBoxes[0][0][1] = 15;
            hitBoxes[0][1][1] = 15;

            if(P1->stateTime > 3){
                P1->drawBox = 1;
            }
            if(P1->stateTime >= 6){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 9){
            if(P1->stateTime > 5){
                P1->drawBox = 3;
            }
            if(P1->stateTime > 5 && P1->stateTime < 6){
                P1->pos[0] += 1 * P1->facing;
            }
            if(P1->stateTime >= 8){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 10){
            if(P1->stateTime > 4 && P1->stateTime < 8){
                P1->drawBox = 1;
            }
            if(P1->stateTime > 5){
                P1->pos[0] += 1 * P1->facing;
            }

            if(P1->stateTime == 7){
                Dummy->hitState = 0;
            }
            if(P1->stateTime > 8){
                P1->drawBox = 0;
            }

            if(P1->stateTime >= 11){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 11){
            hitBoxes[0][0][1] = 15;
            hitBoxes[0][1][1] = 15;

            if(P1->stateTime > 4){
                P1->drawBox = 1;
                Dummy->hitState = 0;
            }
            if(P1->stateTime >= 6){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 12){
            if(P1->stateTime > 4 && P1->stateTime < 15){
                P1->pos[0] += 1 * P1->facing;
            }
            if(P1->stateTime > 8 && P1->stateTime < 10){
                P1->drawBox = 1;
            }
            if(P1->stateTime == 12){
                Dummy->hitState = 0;
            }
            if(P1->stateTime > 14){
                P1->drawBox = 0;
            }

            if(P1->stateTime >= 16){
                P1->thisState = 0;
            }
        }
        if(P1->thisState == 13){
            if(P1->stateTime > 5){
                P1->pos[0] += 1 * P1->facing;
                P1->speed[1] = 0;
                P1->drawBox = 4;

                if(P1->extraStateTime > 3){
                    Dummy->hitState = 0;
                    P1->extraStateTime = 0;
                }    
            }

            if(P1->stateTime >= 17){
                P1->thisState = 0;
            }
            P1->extraStateTime += 1;
        }
        if(P1->thisState == 16){
            if(P1->stateTime > 2){
                P1->pos[0] += 1 * P1->facing;
                P1->speed[1] += 2;
                P1->drawBox = 3;
            }

            if(P1->stateTime >= 6){
                P1->thisState = 0;
            }
        }
    }
    P1->stateTime += 1;
}

static bool collHitCheck() {
    int hitBoxMinX, hitBoxMaxX, hitBoxMinY, hitBoxMaxY;
    int hurtBoxMinX, hurtBoxMaxX, hurtBoxMinY, hurtBoxMaxY;

    hitBoxMinX = hitBoxes[2][0][0] + Dummy->pos[0];
    hitBoxMaxX = hitBoxes[2][1][0] + Dummy->pos[0];
    hitBoxMinY = hitBoxes[2][0][1] + Dummy->pos[1];
    hitBoxMaxY = hitBoxes[2][2][1] + Dummy->pos[1];

    if( P1->facing == -1){
        hurtBoxMinX = hurtBoxes[P1->drawBox][0][0] + P1->pos[0] - 20;
        hurtBoxMaxX = hurtBoxes[P1->drawBox][1][0] + P1->pos[0] - 20;
    } else {
        hurtBoxMinX = hurtBoxes[P1->drawBox][0][0] + P1->pos[0];
        hurtBoxMaxX = hurtBoxes[P1->drawBox][1][0] + P1->pos[0];
    }
    hurtBoxMinY = hurtBoxes[P1->drawBox][0][1] + P1->pos[1];
    hurtBoxMaxY = hurtBoxes[P1->drawBox][2][1] + P1->pos[1];

    bool collisionX = (hitBoxMinX < hurtBoxMaxX) && (hitBoxMaxX > hurtBoxMinX);
    bool collisionY = (hitBoxMinY < hurtBoxMaxY) && (hitBoxMaxY > hurtBoxMinY);
    bool collision = collisionX && collisionY;

    return collision;
}
static bool collCheck() {
    int hitMinX = hitBoxes[2][0][0] + Dummy->pos[0];
    int hitMaxX = hitBoxes[2][1][0] + Dummy->pos[0];
    int hitMinY = hitBoxes[2][0][1] + Dummy->pos[1];
    int hitMaxY = hitBoxes[2][2][1] + Dummy->pos[1];

    int hurtMinX = hitBoxes[0][0][0] + P1->pos[0];
    int hurtMaxX = hitBoxes[0][1][0] + P1->pos[0];
    int hurtMinY = hitBoxes[0][0][1] + P1->pos[1];
    int hurtMaxY = hitBoxes[0][2][1] + P1->pos[1];

    bool collisionX = (hitMinX < hurtMaxX) && (hitMaxX > hurtMinX);
    bool collisionY = (hitMinY < hurtMaxY) && (hitMaxY > hurtMinY);
    bool collision = collisionX && collisionY;

    return collision;
}

static void botStates() {
    if( P1->thisState > 6 && P1->thisState < 100 && Dummy->hitState == 0){
        if (collHitCheck()) {
            if(P1->thisState == 7){
                Dummy->pos[0] += 12 * P1->facing;
                Dummy->stun = 19;
            }

            if(P1->thisState == 8){
                Dummy->pos[0] += 5 * P1->facing;
                Dummy->stun = 20;
            }

            if(P1->thisState == 9){
                Dummy->pos[0] += 5 * P1->facing;
                if(Dummy->extraStuff[1] < 2){
                    Dummy->speed[1] -= 8;
                    Dummy->stun = 30;
                } else {
                    Dummy->speed[1] -= 8;
                    Dummy->stun = 20;
                }
                Dummy->extraStuff[1] += 1;
            }

            if(P1->thisState == 10){
                Dummy->pos[0] += 12 * P1->facing;
                Dummy->stun = 22;
            }
            if(P1->thisState == 11){
                Dummy->pos[0] += 1 * P1->facing;
                Dummy->speed[1] -= 2;
                Dummy->stun = 22;
            }
            if(P1->thisState == 12){
                if(Dummy->extraStuff[0] == 0 && P1->drawBox == 0){
                    Dummy->pos[0] += 10000 * P1->facing;
                    Dummy->stun = 112;
                    Dummy->extraStuff[0] = 1;
                } else {
                    Dummy->pos[0] += 8 * P1->facing;
                    Dummy->stun = 24;
                }
                
            }
            if(P1->thisState == 13){
                Dummy->pos[0] += 1 * P1->facing;
                Dummy->pos[1] -= 1;
                Dummy->speed[1] = -1;
                Dummy->stun = 18;
            }
            /*
            if(P1->thisState == 14){
                Dummy->pos[0] += 1 * P1->facing;
                Dummy->speed[1] -= 2;
                Dummy->stun = 15;
            }
            if(P1->thisState == 15){
                Dummy->pos[0] += 1 * P1->facing;
                Dummy->speed[1] -= 2;
                Dummy->stun = 15;
            }
            */
            if(P1->thisState == 16){
                Dummy->pos[0] += 5 * P1->facing;
                Dummy->speed[1] += 10;
                Dummy->stun = 28;
            }

            if( Dummy->pos[0] > 260 ){
                P1->pos[0] -= 8;
            }
            if( Dummy->pos[0] < 1 ){
                P1->pos[0] += 8;
            }

            if( Dummy->pos[0] > 260 ){
                Dummy->pos[0] = 260;
            }
            if( Dummy->pos[0] < 1 ){
                Dummy->pos[0] = 1;
            }

            if(Dummy->floorCol > 0){
                Dummy->speed[1] -= 2;
            }
            
            counter[2] += 1;
            if(counter[2] >= 10){
                counter[2] = 0;
                counter[1] += 1;
            }
            if(counter[1] >= 10){
                counter[1] = 0;
                counter[0] += 1;
            }
            if(counter[0] == 9 && counter[1] == 9 && counter[2] == 9){
                counter[2] = 9;
                counter[1] = 9;
                counter[0] = 9;
            }
            Dummy->hitState = 1;
        }
    }

    if(Dummy->stun > 0){
        Dummy->stun -= 1;
    } else {
        Dummy->extraStuff[0] = 0;
        Dummy->extraStuff[1] = 0;
        Dummy->extraStuff[2] = 0;
        Dummy->extraStuff[3] = 0;
        Dummy->stun = 0;
        Dummy->hitState = 0;
    }
}
static void Dummyz() {
    Dummy->lastPos[0] = Dummy->pos[0];
    Dummy->lastPos[1] = Dummy->pos[1];

    hitBoxes[2][0][1] = 0;
    hitBoxes[2][1][1] = 0;

    botStates();

    Dummy->pos[0] += Dummy->speed[0];
    Dummy->pos[1] += Dummy->speed[1];

    if ( Dummy->pos[1] < (160 - dh) ) {
        Dummy->speed[1] += 1;
        Dummy->floorCol = 1;
    }
    if( Dummy->pos[1] >= (160 - dh) ) {
        Dummy->speed[1] = 0;
        Dummy->pos[1] = (160 - dh);
        Dummy->floorCol = 0;
    }
    if( Dummy->pos[0] > 260 ){
        Dummy->pos[0] = 260;
    }
    if( Dummy->pos[0] < 1 ){
        Dummy->pos[0] = 1;
    }
}

static void Player1() {
    P1->lastPos[0] = P1->pos[0];
    P1->lastPos[1] = P1->pos[1];

    hitBoxes[0][0][1] = 0;
    hitBoxes[0][1][1] = 0;

    btnStateMaker();
    stateMachine();

    if(P1->thisState <= 6 && P1->floorCol == 0){
        if (P1->states[0] == 2 && P1->floorCol == 0) {
            hitBoxes[0][0][1] = 15;
            hitBoxes[0][1][1] = 15;
        }
        if (P1->states[1] == 2 && P1->states[0] != 2) {
            P1->speed[0] = -2;
        }
        if (P1->states[1] == 1 && P1->states[0] != 2) {
            P1->speed[0] = 2;
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

    P1->pos[0] += P1->speed[0];
    P1->pos[1] += P1->speed[1];

    if ( P1->pos[1] < (160 - h) ) {
        P1->speed[1] += 1;
        P1->floorCol = 1;
    }
    if( P1->pos[1] >= (160 - h) ) {
        P1->speed[1] = 0;
        P1->pos[1] = (160 - h);
        P1->speed[0] = 0;
        if(P1->floorCol == 1){
            P1->thisState = 0;
            P1->stateTime = 0;
        }
        P1->floorCol = 0;
    }
    if( P1->pos[0] > 260 ){
        P1->pos[0] = 260;
    }
    if( P1->pos[0] < 1 ){
        P1->pos[0] = 1;
    }
}

static void btns(){
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnState = evt.state;
        printf("state: %04X, button: %d, down: %s\n",
            evt.state, evt.button, evt.down ? "down" : "up");
    }
}

static void foxMainLoop(int64_t elapsedUs)
{
    setFrameRateUs(fps);
    
    btns();
    Player1();
    Dummyz();
    
    if (collCheck()) {
        bool collisionLeft = (P1->pos[0] + 10 > Dummy->pos[0] + 10);
        
        if (collisionLeft) {
            P1->pos[0] = Dummy->pos[0] + 20;
        } else {
            P1->pos[0] = Dummy->pos[0] - 20;
        }
    }

    if( P1->pos[0] < Dummy->pos[0] && P1->floorCol == 0){
        P1->facing = 1;
    } else if( P1->pos[0] > Dummy->pos[0] && P1->floorCol == 0){
        P1->facing = -1;
    }

    if( P1->facing == -1 ){
        for(int i = 0; i < length; i++) {
            if( i != 5 ){    
                for(int j = 0; j < 4; j++) {
                    restoreBox[i][j] = hurtBoxes[i][j][0];
                    hurtBoxes[i][j][0] *= -1;
                    hurtBoxes[i][j][0] += 20;
                }
            }
        }
    }

    runStates();
    botStates();
    debug();

    if( P1->facing == -1 ){
        for(int i = 0; i < length; i++) {
            for(int j = 0; j < 4; j++) {
                hurtBoxes[i][j][0] = restoreBox[i][j];
            }
        }
    }
}
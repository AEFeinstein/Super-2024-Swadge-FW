#include "Cross.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "files/Map.c"
#include "files/collCheck.c"
#include "files/Player.c"

static const char crossName[] = "Cross";
static void crossMainLoop(int64_t elapsedUs);
static void crossEnterMode();
static void crossExitMode();

int fps = 1000000 / 30;
uint16_t btnState;

typedef struct {
    int pos[2];
    int w_h[2];
    int states[3];
} P1_t;

swadgeMode_t CrossMode = {
    .modeName = crossName,
    .wifiMode = ESP_NOW,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .overrideSelectBtn = false,
    .fnMainLoop = crossMainLoop,
    .fnEnterMode = crossEnterMode,
    .fnExitMode = crossExitMode
};

P1_t* P1 = NULL;
Player player;

static void crossEnterMode() {
    P1 = calloc(1, sizeof(P1_t));
    initPlayer(&player, 110, 150, 0, 0);
    P1->pos[0] = 110;
    P1->pos[1] = 150;
    P1->w_h[0] = 32;
    P1->w_h[1] = 36;
}

static void crossExitMode() {
    free(P1);
}

void renderWorld() {
    fillDisplayArea(0, 0, 280, 240, c000);
    for (int i = 0; i < sizeof(Map) / sizeof(Map[0]); i++) {
        int* collider = Map[i];

        fillDisplayArea(collider[0] + player.camera[0], collider[1] + player.camera[1], collider[2], collider[3], c555);
    }
}

static void btns() {
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnState = evt.state;
        printf("state: %04X, button: %d, down: %s\n",
            evt.state, evt.button, evt.down ? "down" : "up");
    }
    
    // this code will be sent over as buttonStates for Player.c
    P1->states[0] = 0;
    P1->states[1] = 0;
    P1->states[2] = 0;

    if(btnState & PB_LEFT){
        P1->states[0] = 1;
    }
    if(btnState & PB_RIGHT){
        P1->states[0] = 2;
    }
    if(btnState & PB_DOWN){
        P1->states[1] = 1;
    }
    if(btnState & PB_UP){
        P1->states[1] = 2;
    }
    if(btnState & PB_A){
        P1->states[2] = 1;
    }
    if(btnState & PB_B){
        P1->states[2] = 2;
    }
}

static void handle_collision(Player* p) {
    int pw = P1->w_h[0];
    int ph = P1->w_h[1];
    for (int i = 0; i < sizeof(Map) / sizeof(Map[0]); i++) {
        int* collider = Map[i];
        if (collCheck(p->positions[0], p->positions[1], pw, ph, collider[0], collider[1], collider[2], collider[3])) {
            if (p->positions[1] + ph >= collider[1] && p->positions[1] < collider[1]) {
                // feet collision
                p->positions[1] = collider[1] - ph - 1;
                p->speed[1] = 1;
                p->falling = 0;
                p->dash = 1;
                if (!(btnState & PB_LEFT) && !(btnState & PB_RIGHT)) {
                    p->anim = "Idle";
                } else {
                    p->anim = "Walk";
                }
            } else if (p->positions[1] <= collider[1] + collider[3] && p->positions[1] > collider[1]) {
                // head collision
                p->positions[1] = collider[1] + collider[3] + 1;
                p->speed[1] = 0;
            } else if (p->positions[0] + pw >= collider[0] && p->positions[0] < collider[0]) {
                // left side collision
                p->positions[0] = collider[0] - pw - 1;
                p->speed[0] = 0;
            } else if (p->positions[0] <= collider[0] + collider[2] && p->positions[0] > collider[0]) {
                // right side collision
                p->positions[0] = collider[0] + collider[2] + 1;
                p->speed[0] = 0;
            }
        }
    }
}



static void crossMainLoop(int64_t elapsedUs) {
    setFrameRateUs(fps);
    
    btns();
    
    movePlayer(&player, P1->w_h[1], P1->states);
    handle_collision(&player);

    renderWorld();
}

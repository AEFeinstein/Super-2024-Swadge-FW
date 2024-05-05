#include "FoxsGame.h"
#include <math.h>

static const char foxName[] = "Fox";
static void foxMainLoop(int64_t elapsedUs);

int fps = 1000000 / 30;
uint16_t btnState;
bool runner = true;

int x1 = 0; 
int y1 = 100;
double x1Speed = 0;
double y1Speed = 0; 
int state1 = 0; 
int falling1 = 0;

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
    .fnMainLoop = foxMainLoop
};

static void debug(){
    for(int z = 0; z < 2; z++){
        drawLineFast(int(hitBoxes[z][0][0] + x1), int(hitBoxes[z][0][1] + y1), int(hitBoxes[z][0][2] + x1), int(hitBoxes[z][0][3] + y1), c555);
        drawLineFast(int(hitBoxes[z][1][0] + x1), int(hitBoxes[z][1][1] + y1), int(hitBoxes[z][1][2] + x1), int(hitBoxes[z][1][3] + y1), c555);
        drawLineFast(int(hitBoxes[z][2][0] + x1), int(hitBoxes[z][2][1] + y1), int(hitBoxes[z][2][2] + x1), int(hitBoxes[z][2][3] + y1), c555);
        drawLineFast(int(hitBoxes[z][3][0] + x1), int(hitBoxes[z][3][1] + y1), int(hitBoxes[z][3][2] + x1), int(hitBoxes[z][3][3] + y1), c555);
    }
}

static void move() {
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnState = evt.state;
        printf("state: %04X, button: %d, down: %s\n",
               evt.state, evt.button, evt.down ? "down" : "up");
    }

    if (btnState & PB_UP && falling1 < 2) {
        y1Speed -= 5;
    }
    if (btnState & PB_LEFT) {
        x1Speed -= 0.5;
    }
    if (btnState & PB_RIGHT) {
        x1Speed += 0.5;
    }

    /*
    if (btnState & PB_DOWN) {
    }
    */

    if ( y1 < 150 )
    {
        y1Speed += 0.5;
        falling1 += 1;
    }
    else if( y1 >= 150 ){
        y1 = 150;
        y1Speed = 0;
        falling1 = 0;
    }

    if (x1Speed > 0.2)
    {
        x1Speed /= 1.5;
    }
    if (x1Speed < 0.2)
    {
        x1Speed /= 1.5;
    }

    x1 += int(x1Speed);
    y1 += int(y1Speed);
}

static void foxMainLoop(int64_t elapsedUs)
{
    setFrameRateUs(fps);

    if(runner == true){
        runner = false;
    }
    
    move();
    debug(hitBoxes);
}
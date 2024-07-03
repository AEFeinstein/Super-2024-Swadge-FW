#include "FoxsGame.h"
#include "esp_random.h"

int16_t x = 0;
int16_t y = 0;
int16_t count = 0;
uint16_t btnState;


static const char foxName[]  = "Fox";

/*
static void foxEnterMode(void);
static void foxExitMode(void);
*/

static void foxMainLoop(int64_t elapsedUs);


/*
static void foxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
*/

swadgeMode_t foxMode = {
    .modeName                 = foxName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnMainLoop               = foxMainLoop
};

/*
static void foxEnterMode(void)
{

}
 
static void foxExitMode(void)
{
}
*/

static void draw()
{
    drawRect(x, y, 45, 45, c050);
    fillDisplayArea(0, 0, 45, 220, c554);
}

static void player(int16_t x, int16_t y)
{
    buttonEvt_t evt;
    if (evt.state & PB_UP)
    {
        y += 5;
    }
    else if (evt.state & PB_DOWN)
    {
        y -= 5;
    }
}

static void foxMainLoop(int64_t elapsedUs)
{
    player(x, y);
    draw();
}
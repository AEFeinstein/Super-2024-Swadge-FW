/**
 * @file mode_bigbug.c
 * @author James Albracht (James A on slack)
 * @brief Big Bug game
 * @date 2024-05-05
 */

//==============================================================================
// Includes
//==============================================================================

#include "mode_bigbug.h"
#include <math.h>

//==============================================================================
// Defines
//==============================================================================
#define DECIMAL_BITS 4

#define TILE_FIELD_WIDTH 16
#define TILE_FIELD_HEIGHT 1024
#define GARBOTNIK_RADIUS   (14 << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define HALF_WIDTH   (FIELD_WIDTH / 2)
#define HALF_HEIGHT  (FIELD_HEIGHT / 2)

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum of screens that may be shown in bigbug mode
 */
typedef enum
{
    BIGBUG_MENU,
    BIGBUG_GAME,
} bigbugScreen_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    bigbugScreen_t screen;                      ///< The screen being displayed

    uint8_t score[2];            ///< The score for the game

    rectangle_t paddleL; ///< The left paddle
    rectangle_t paddleR; ///< The right paddle
    circle_t garbotnik;  ///< Garbotnik (player character)
    vec_t garbotnikVel;  ///< Garbotnik's velocity
    vec_t garbotnikAccel;///< Garbotnik's acceleration
    vec_t previousPos;   ///< Garbotnik's position on the previous frame (for resolving collisions)

    rectangle_t camera; ///< The camera
    int8_t tiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT]; ///< The array of tiles. 1 is tile, 0 is not. Future feature: more variety

    font_t ibm_vga8;

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for paddle control
    bool paddleRMovingUp;   ///< The CPU's paddle direction on easy mode
    bool isPaused;          ///< true if the game is paused, false if it is running

    wsg_t dirtWsg;        ///< A graphic for the dirt tile
    wsg_t garbotnikWsg;   ///< A graphic for garbotnik
    wsg_t uiWileOutlineWsg;

    song_t bgm;  ///< Background music
    song_t hit1; ///< Sound effect for one paddle's hit
    song_t hit2; ///< Sound effect for the other paddle's hit

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs
} bigbug_t;

//==============================================================================
// Function Prototypes
//==============================================================================

//required by adam
static void bigbugEnterMode(void);
static void bigbugExitMode(void);
static void bigbugMainLoop(int64_t elapsedUs);
static void bigbugAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void bigbugBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bigbugEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void bigbugEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t bigbugAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);
static void bigbugDacCb(uint8_t *samples, int16_t len);

//big bug logic
static void bigbugControlGarbotnik(int64_t elapsedUs);
static void bigbugDrawField(void);
static void bigbugGameLoop(int64_t elapsedUs);
static void bigbugReset(void);
static void bigbugSetLeds(void);
static void bigbugUpdatePhysics(int64_t elapsedUs);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */
static const char bigbugName[]  = "Big Bug";

//==============================================================================
// Variables
//==============================================================================


swadgeMode_t bigbugMode = {
    .modeName                 = bigbugName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .overrideSelectBtn        = false,
    .fnEnterMode              = bigbugEnterMode,
    .fnExitMode               = bigbugExitMode,
    .fnMainLoop               = bigbugMainLoop,
    .fnAudioCallback          = bigbugAudioCallback,
    .fnBackgroundDrawCallback = bigbugBackgroundDrawCallback,
    .fnEspNowRecvCb           = bigbugEspNowRecvCb,
    .fnEspNowSendCb           = bigbugEspNowSendCb,
    .fnAdvancedUSB            = bigbugAdvancedUSB,
    .fnDacCb                  = NULL
};

/// All state information for bigbug mode. This whole struct is calloc()'d and free()'d so that bigbug is only
/// using memory while it is being played
bigbug_t* bigbug = NULL;

//==============================================================================
// Required Functions
//==============================================================================

static void bigbugEnterMode(void)
{
    bigbug = calloc(1, sizeof(bigbug_t));

    // Load graphics
    loadWsg("dirt.wsg", &bigbug->dirtWsg, false);
    loadWsg("garbotnik-small.wsg", &bigbug->garbotnikWsg, false);
    loadWsg("button-outline.wsg", &bigbug->uiWileOutlineWsg, false);

    // Set the mode to game mode
    bigbug->screen = BIGBUG_GAME;

    // Load font
    loadFont("ibm_vga8.font", &bigbug->ibm_vga8, false);

    bigbugReset();
}
 
static void bigbugExitMode(void)
{
    // Free font
    freeFont(&bigbug->ibm_vga8);
}
 
static void bigbugMainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (bigbug->screen)
    {
        case BIGBUG_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                bigbug->menu = menuButton(bigbug->menu, evt);
            }

            // Draw the menu
            drawMenuLogbook(bigbug->menu, bigbug->menuLogbookRenderer, elapsedUs);
            break;
        }
        case BIGBUG_GAME:
        {
            // Run the main game loop. This will also process button events
            bigbugGameLoop(elapsedUs);
            break;
        }
    }
}
 
static void bigbugAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    // Fill this in
}
 
static void bigbugBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    //accelIntegrate(); only needed if using accelerometer for something
    //SETUP_FOR_TURBO(); only needed if drawing individual pixels

    fillDisplayArea(x, y, x + w, y + h, c100);//sonic sky is like c001
}
 
static void bigbugEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Fill this in
}
 
static void bigbugEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Fill this in
}
 
static int16_t bigbugAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    // Fill this in
       return 0;
}
 
static void bigbugDacCb(uint8_t *samples, int16_t len)
{
    // Fill this in
}

//==============================================================================
// Big Bug Functions
//==============================================================================

static void bigbugControlGarbotnik(int64_t elapsedUs)
{
    vec_t accel;
    accel.x = 0;
    accel.y = 0;
    // Update garbotnik's velocity if a button is currently down
    switch(bigbug->btnState){
        //up
        case 0b0001:
            accel.y = -50;
            break;
        case 0b1101:
            accel.y = -50;
            break;

        //down
        case 0b0010:
            accel.y = 50;
            break;
        case 0b1110:
            accel.y = 50;
            break;

        //left
        case 0b0100:
            accel.x = -50;
            break;
        case 0b0111:
            accel.x = -50;
            break;

        //right
        case 0b1000:
            accel.x = 50;
            break;
        case 0b1011:
            accel.x = 50;
            break;

        //up,left
        case 0b0101:
            accel.x = -35; //magnitude is sqrt(1/2) * 100000
            accel.y = -35;
            break;
        
        //up,right
        case 0b1001:
            accel.x = 35;//35 707 7035
            accel.y = -35;
            break;

        //down,right
        case 0b1010:
            accel.x = 35;
            accel.y = 35;
            break;

        //down,left
        case 0b0110:
            accel.x = -35;
            accel.y = 35;
            break;
        default:
            break;
    }

    // printf("accel x: %d\n", accel.x);
    // printf("elapsed: %d", (int32_t) elapsedUs);
    // printf("offender: %d\n", (int32_t) elapsedUs / 100000);
    // printf("now   x: %d\n", mulVec2d(accel, elapsedUs) / 100000).x);

    bigbug->garbotnikAccel = divVec2d(mulVec2d(accel, elapsedUs), 100000);
    // if (bigbug->btnState & PB_UP)
    // {
    //     bigbug->garbotnikVel.y = bigbug->garbotnikVel.y - 5 * elapsedUs / 100000;
    // }
    // else if (bigbug->btnState & PB_DOWN)
    // {
    //     bigbug->garbotnikVel.y = bigbug->garbotnikVel.y + 5 * elapsedUs / 100000;
    // }
    // else if (bigbug->btnState & PB_LEFT)
    // {
    //     bigbug->garbotnikVel.x = bigbug->garbotnikVel.x - 5 * elapsedUs / 100000;
    // }
    // else if (bigbug->btnState & PB_RIGHT)
    // {
    //     bigbug->garbotnikVel.x = bigbug->garbotnikVel.x + 5 * elapsedUs / 100000;
    // }
}

/**
 * @brief Draw the bigbug field to the TFT
 */
static void bigbugDrawField(void)
{
    //printf("camera x: %d\n", (bigbug->camera.pos.x >> DECIMAL_BITS));
    //printf("width: %d\n", FIELD_WIDTH);
    int16_t iStart = (bigbug->camera.pos.x >> DECIMAL_BITS) / 64;
    int16_t iEnd = iStart + TILE_FIELD_WIDTH;
    int16_t jStart = (bigbug->camera.pos.y >> DECIMAL_BITS) / 64;
    int16_t jEnd = jStart + TILE_FIELD_HEIGHT;
    if ((bigbug->camera.pos.x >> DECIMAL_BITS) < 0){
        iStart -= 1;
        if ((bigbug->camera.pos.x  + FIELD_WIDTH) >> DECIMAL_BITS < 0){
            iEnd -= 1;
        }
    }
    if ((bigbug->camera.pos.y >> DECIMAL_BITS) < 0){
        jStart -= 1;
        if ((bigbug->camera.pos.y + FIELD_HEIGHT) >> DECIMAL_BITS < 0){
            jEnd -= 1;
        }
    }

    
    if(iEnd >= 0 && iStart <= TILE_FIELD_WIDTH && jEnd >= 0 && jStart <= TILE_FIELD_HEIGHT){
        if(0 > iStart){
            iStart = 0;
        }
        if(4 < iEnd){
            iEnd = TILE_FIELD_WIDTH;
        }
        if(0 > jStart){
            jStart = 0;
        }
        if(3 < jEnd){
            jEnd = TILE_FIELD_HEIGHT;
        }

        // printf("iStart: %d\n", iStart);
        // printf("iEnd: %d\n", iEnd);
        // printf("jStart: %d\n", jStart);
        // printf("jEnd: %d\n", jEnd);

        for (uint32_t i = iStart; i <= iEnd; i++){
            for (uint32_t j = jStart; j <= jEnd; j++){
                // Draw dirt tile
                if(bigbug->tiles[i][j] >= 1){
                    drawWsgTile(&bigbug->dirtWsg, i * 64 - (bigbug->camera.pos.x >> DECIMAL_BITS), j * 64 - (bigbug->camera.pos.y >> DECIMAL_BITS));
                }
            }
        }
    }

    

    // printf("garbotnik.pos.y: %d\n", bigbug->garbotnik.pos.y);
    // printf("garbotnik.radius: %d\n", bigbug->garbotnik.radius);
    // printf("camera.pos.y: %d\n", bigbug->camera.pos.y);
    // printf("render y: %d\n", (bigbug->garbotnik.pos.y - bigbug->garbotnik.radius - bigbug->camera.pos.y) >> DECIMAL_BITS);

    // Draw garbotnik
    drawWsgSimple(&bigbug->garbotnikWsg, ((bigbug->garbotnik.pos.x  - bigbug->camera.pos.x )>> DECIMAL_BITS) - 19,
                  ((bigbug->garbotnik.pos.y - bigbug->camera.pos.y) >> DECIMAL_BITS) - 21);

    // Draw UI
    char buttons[] = {'Z','\0','A','\0','B','\0','C'};
    for (int i = 1; i < 4; i++){
        int xPos = i * TFT_WIDTH / 4;
        drawWsgSimple(&bigbug->uiWileOutlineWsg, xPos - 20, TFT_HEIGHT - 46);
        drawText(&bigbug->ibm_vga8, c555, &buttons[2*i], xPos - 3, TFT_HEIGHT - 12);
    }
    
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bigbugGameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        printf("state: %04X, button: %d, down: %s\n",
        evt.state, evt.button, evt.down ? "down" : "up");
        
        // Save the button state
        bigbug->btnState = evt.state;


        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            bigbug->isPaused = !bigbug->isPaused;
        }
    }

    // If the game is not paused, do game logic
    if (bigbug->isPaused == false)
    {
        // record the previous frame's position before any logic.
        bigbug->previousPos = bigbug->garbotnik.pos;
        // bigbugFadeLeds(elapsedUs);
        bigbugControlGarbotnik(elapsedUs);
        // bigbugControlCpuPaddle();
        bigbugUpdatePhysics(elapsedUs);
    }

    // Set the LEDs
    bigbugSetLeds();
    // Draw the field
    bigbugDrawField();
}

static void bigbugReset(void){
    // Set garbotnik variables
    bigbug->garbotnik.pos.x  = 128 << DECIMAL_BITS;
    bigbug->garbotnik.pos.y  =  -(90 << DECIMAL_BITS);

    printf("The width is: %d\n", FIELD_WIDTH);
    printf("The height is: %d\n", FIELD_HEIGHT);

    bigbug->camera.width = FIELD_WIDTH;
    bigbug->camera.height = FIELD_HEIGHT;

    bigbug->garbotnik.radius = GARBOTNIK_RADIUS;

    //Set all tiles to dirt
    for(int i = 0; i < TILE_FIELD_WIDTH; i++){
        for(int j = 0; j < TILE_FIELD_HEIGHT; j++){
            bigbug->tiles[i][j] = 2;
        }
    }
}

/**
 * @brief Set the LEDs
 */
static void bigbugSetLeds(void)
{
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Copy the LED colors for left and right to the whole array
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
    {
        leds[i]                         = bigbug->ledL;
        leds[i + (CONFIG_NUM_LEDS / 2)] = bigbug->ledR;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);
}

static void bigbugUpdatePhysics(int64_t elapsedUs)
{
    // Apply garbotnik's drag
    int32_t sqMagVel= sqMagVec2d(bigbug->garbotnikVel);
    int32_t speed = sqrt(sqMagVel);
    int32_t drag = sqMagVel / 500; //smaller denominator for bigger drag.

    if (drag > speed * 0.9){
        drag = speed * 0.9;
    }
    if (drag < 5){
        drag = 5.0;
    }
    // printf("speed: %d\n", speed);
    // printf("drag: %d\n", drag);
    if(speed > 0){
        bigbug->garbotnikAccel.x += (bigbug->garbotnikVel.x / (double)speed) * -drag * elapsedUs / 100000;
        bigbug->garbotnikAccel.y += (bigbug->garbotnikVel.y / (double)speed) * -drag * elapsedUs / 100000;
        // bigbug->garbotnikAccel = addVec2d(bigbug->garbotnikAccel, mulVec2d(divVec2d(bigbug->garbotnikVel, speed), -drag * elapsedUs / 100000));
    }
    
    // Update garbotnik's velocity
    bigbug->garbotnikVel.x += bigbug->garbotnikAccel.x;
    bigbug->garbotnikVel.y += bigbug->garbotnikAccel.y;

    // Update garbotnik's position
    bigbug->garbotnik.pos.x += bigbug->garbotnikVel.x * elapsedUs / 100000;
    bigbug->garbotnik.pos.y += bigbug->garbotnikVel.y * elapsedUs / 100000;

    // Look up 4 nearest tiles for collision checks
    // a tile's width is 32 pixels << 4 = 1024. half width is 512.
    int32_t xIdx = ((bigbug->garbotnik.pos.x - 512)/1024) - (bigbug->garbotnik.pos.x < 0);//the x index
    int32_t yIdx = (bigbug->garbotnik.pos.y  - 512)/1024 - (bigbug->garbotnik.pos.y < 0);//the y index

    int32_t best_i = -1;//negative means no worthy candidates found.
    int32_t best_j = -1;
    int32_t closestSqDist = 1063842;//(307.35+724.077)^2 if it's further than this, there's no way it's a collision.
    for(int32_t i = xIdx; i <= xIdx+1; i++){
        for(int32_t j = yIdx; j <= yIdx+1; j++){
            if(i >= 0 && i < TILE_FIELD_WIDTH && j >=0 && j < TILE_FIELD_HEIGHT){
                if(bigbug->tiles[i][j] >= 1){
                    //Initial circle check for preselecting the closest dirt tile
                    int32_t sqDist = sqMagVec2d(subVec2d(bigbug->garbotnik.pos, (vec_t){i * 1024 + 512, j * 1024 + 512}));
                    if(sqDist < closestSqDist){
                        //Good candidate found!
                        best_i = i;
                        best_j = j;
                        closestSqDist = sqDist;
                    }
                }
            }
        }
    }
    if(best_i > -1){
        vec_t tilePos = {best_i * 1024 + 512, best_j * 1024 + 512};
        //AABB-AABB collision detection begins here
        //https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
        if(bigbug->garbotnik.pos.x + 240 > tilePos.x - 512 &&
           bigbug->garbotnik.pos.x - 240 < tilePos.x + 512 &&
           bigbug->garbotnik.pos.y + 192 > tilePos.y - 512 &&
           bigbug->garbotnik.pos.y - 192 < tilePos.y + 512)
        {
            //Collision detected!
            //Update the dirt by decrementing if greater than 0.
            bigbug->tiles[best_i][best_j] = bigbug->tiles[best_i][best_j] > 0 ? bigbug->tiles[best_i][best_j] - 1 : 0;
            printf("hit");

            //Resolve garbotnik's position somewhat based on his position previously.
            vec_t normal = subVec2d(bigbug->previousPos, tilePos);
            //Snap the previous frame offset to an orthogonal direction.
            if((normal.x < 0?-normal.x:normal.x) > (normal.y < 0?-normal.y:normal.y)){
                if(normal.x > 0){
                    normal.x = 1;
                    normal.y = 0;
                    bigbug->garbotnik.pos.x = tilePos.x + 752;
                }
                else{
                    normal.x = -1;
                    normal.y = 0;
                    bigbug->garbotnik.pos.x = tilePos.x - 752;
                }
                

            }
            else{
                if(normal.y > 0){
                    normal.x = 0;
                    normal.y = 1;
                    bigbug->garbotnik.pos.y = tilePos.y + 704;
                }
                else{
                    normal.x = 0;
                    normal.y = -1;
                    bigbug->garbotnik.pos.y = tilePos.y - 704;
                }
            }
            
            //Mirror garbotnik's velocity
            // Reflect the velocity vector along the normal
            // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
            bigbug->garbotnikVel = subVec2d(bigbug->garbotnikVel, mulVec2d(normal, (2* dotVec2d(bigbug->garbotnikVel, normal))));
        }
    }

    // Update the camera's position to catch up to the player
    bigbug->camera.pos.x = bigbug->garbotnik.pos.x - HALF_WIDTH;
    bigbug->camera.pos.y = bigbug->garbotnik.pos.y - HALF_HEIGHT;
}


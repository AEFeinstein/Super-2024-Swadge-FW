/**
 * @file mode_bigbug.c
 * @author James Albracht (James A on slack)
 * @brief Big Bug game
 * @date 2024-05-05
 */

//==============================================================================
// Includes
//==============================================================================

#include "gameData_bigbug.h"
#include "mode_bigbug.h"
#include "gameData_bigbug.h"
#include "tilemap_bigbug.h"
#include "entityManager_bigbug.h"
#include "esp_heap_caps.h"
#include <math.h>

//==============================================================================
// Defines
//==============================================================================

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
} bb_screen_t;

//==============================================================================
// Structs
//==============================================================================

struct bb_t
{
    menu_t* menu;         ///< The menu structure
    font_t font;          ///< The font used in the menu and game
    bb_screen_t screen;   ///< The screen being displayed

    bb_gameData_t gameData;
    bb_tilemap_t tilemap;
    bb_entityManager_t entityManager;
    bb_soundManager_t soundManager;

    vec_t garbotnikPos;   ///< Garbotnik (player character)
    vec_t garbotnikVel;   ///< Garbotnik's velocity
    vec_t garbotnikAccel; ///< Garbotnik's acceleration
    vec_t previousPos;    ///< Garbotnik's position on the previous frame (for resolving collisions)
    vec_t garbotnikRotation; ///<x is Yaw to left or right. Y is change to yaw over time. Tends towards left or right.

    rectangle_t camera;   ///< The camera

    bool isPaused;        ///< true if the game is paused, false if it is running

    wsg_t garbotnikWsg[3];   ///< An array of graphics for garbotnik.

    song_t bgm;  ///< Background music
    song_t hit1; ///< A sound effect

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs
};


//==============================================================================
// Function Prototypes
//==============================================================================

//required by adam
static void bb_EnterMode(void);
static void bb_ExitMode(void);
static void bb_MainLoop(int64_t elapsedUs);
static void bb_AudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void bb_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t bb_AdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);
static void bb_DacCb(uint8_t *samples, int16_t len);

//big bug logic
static void bb_ControlGarbotnik(int64_t elapsedUs);
static void bb_DrawScene(void);
static void bb_GameLoop(int64_t elapsedUs);
static void bb_Reset(void);
static void bb_SetLeds(void);
static void bb_UpdateTileSupport(void);
static void bb_UpdatePhysics(int64_t elapsedUs);


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
    .fnEnterMode              = bb_EnterMode,
    .fnExitMode               = bb_ExitMode,
    .fnMainLoop               = bb_MainLoop,
    .fnAudioCallback          = bb_AudioCallback,
    .fnBackgroundDrawCallback = bb_BackgroundDrawCallback,
    .fnEspNowRecvCb           = bb_EspNowRecvCb,
    .fnEspNowSendCb           = bb_EspNowSendCb,
    .fnAdvancedUSB            = bb_AdvancedUSB,
    .fnDacCb                  = NULL
};

/// All state information for bigbug mode. This whole struct is calloc()'d and free()'d so that bigbug is only
/// using memory while it is being played
bb_t* bigbug = NULL;

//==============================================================================
// Required Functions
//==============================================================================

static void bb_EnterMode(void)
{
    printf("a\n");
    bigbug = heap_caps_calloc(1, sizeof(bb_t), MALLOC_CAP_SPIRAM);
    
    printf("b\n");

    bb_initializeGameData(&(bigbug->gameData), &(bigbug->soundManager));
    printf("c\n");
    bb_initializeTileMap(&(bigbug->tilemap));
    printf("d\n");
    bb_initializeEntityManager(&(bigbug->entityManager),
                            &(bigbug->gameData),
                            &(bigbug->soundManager));
    printf("e\n");
    // Load graphics
    loadWsg("garbotnik-0.wsg", &bigbug->garbotnikWsg[0], true);
    loadWsg("garbotnik-1.wsg", &bigbug->garbotnikWsg[1], true);
    loadWsg("garbotnik-2.wsg", &bigbug->garbotnikWsg[2], true);
    printf("f\n");

    // Set the mode to game mode
    bigbug->screen = BIGBUG_GAME;
    printf("g\n");

    // Load font
    loadFont("ibm_vga8.font", &bigbug->font, false);
    printf("h\n");

    bb_Reset();
    printf("i\n");
}
 
static void bb_ExitMode(void)
{
    // Free font
    freeFont(&bigbug->font);
}
 
static void bb_MainLoop(int64_t elapsedUs)
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
            break;
        }
        case BIGBUG_GAME:
        {
            // Run the main game loop. This will also process button events
            bb_GameLoop(elapsedUs);
            break;
        }
    }
}
 
static void bb_AudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    // Fill this in
}
 
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    //accelIntegrate(); only needed if using accelerometer for something
    //SETUP_FOR_TURBO(); only needed if drawing individual pixels
    if(bigbug->camera.pos.y<100){
        fillDisplayArea(x, y, x + w, y + h, c455);
    }
    else{
        fillDisplayArea(x, y, x + w, y + h, c000);
    }
    
}
 
static void bb_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Fill this in
}
 
static void bb_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Fill this in
}
 
static int16_t bb_AdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    // Fill this in
       return 0;
}
 
static void bb_DacCb(uint8_t *samples, int16_t len)
{
    // Fill this in
}

//==============================================================================
// Big Bug Functions
//==============================================================================

static void bb_ControlGarbotnik(int64_t elapsedUs)
{
    vec_t accel;
    accel.x = 0;
    accel.y = 0;
    // Update garbotnik's velocity if a button is currently down
    switch(bigbug->gameData.btnState){
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
}

/**
 * @brief Draw the bigbug field to the TFT
 */
static void bb_DrawScene(void)
{
    vec_t garbotnikDrawPos = {
        .x = (bigbug->garbotnikPos.x >> DECIMAL_BITS) - bigbug->camera.pos.x - 18,
        .y = (bigbug->garbotnikPos.y >> DECIMAL_BITS) - bigbug->camera.pos.y - 17
    };
    bb_drawTileMap(&bigbug->tilemap, &bigbug->camera, &garbotnikDrawPos, &bigbug->garbotnikRotation);

    // printf("garbotnikPos.y: %d\n", bigbug->garbotnikPos.y);
    // printf("garbotnik.radius: %d\n", bigbug->garbotnik.radius);
    // printf("camera.pos.y: %d\n", bigbug->camera.pos.y);
    // printf("render y: %d\n", (bigbug->garbotnikPos.y - bigbug->garbotnik.radius - bigbug->camera.pos.y) >> DECIMAL_BITS);

    bb_drawEntities(&bigbug->entityManager, &bigbug->camera);

    // Draw garbotnik
    if(bigbug->garbotnikRotation.x < -1400){
        drawWsgSimple(&bigbug->garbotnikWsg[0], garbotnikDrawPos.x, garbotnikDrawPos.y);
    }
    else if(bigbug->garbotnikRotation.x < -400){
        drawWsgSimple(&bigbug->garbotnikWsg[1], garbotnikDrawPos.x, garbotnikDrawPos.y);
    }
    else if(bigbug->garbotnikRotation.x < 400){
        drawWsgSimple(&bigbug->garbotnikWsg[2], garbotnikDrawPos.x, garbotnikDrawPos.y);
    }
    else if(bigbug->garbotnikRotation.x < 1400){
        drawWsg(&bigbug->garbotnikWsg[1], garbotnikDrawPos.x, garbotnikDrawPos.y, true, false, 0);
    }
    else{
        drawWsg(&bigbug->garbotnikWsg[0], garbotnikDrawPos.x, garbotnikDrawPos.y, true, false, 0);
    }
    
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bb_GameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        // printf("state: %04X, button: %d, down: %s\n",
        // evt.state, evt.button, evt.down ? "down" : "up");
        
        // Save the button state
        bigbug->gameData.btnState = evt.state;


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
        bigbug->previousPos = bigbug->garbotnikPos;
        bb_UpdateTileSupport();
        // bigbugFadeLeds(elapsedUs);
        bb_ControlGarbotnik(elapsedUs);
        // bigbugControlCpuPaddle();
        bb_UpdatePhysics(elapsedUs);
    }

    // Set the LEDs
    bb_SetLeds();
    // Draw the field
    bb_DrawScene();
}

static void bb_Reset(void){
    // Set garbotnik variables
    bigbug->garbotnikPos.x  = 128 << DECIMAL_BITS;
    bigbug->garbotnikPos.y  =  -(90 << DECIMAL_BITS);
    bigbug->garbotnikRotation.x = 0 << DECIMAL_BITS;
    bigbug->garbotnikRotation.y = 0 << DECIMAL_BITS;

    printf("The width is: %d\n", FIELD_WIDTH);
    printf("The height is: %d\n", FIELD_HEIGHT);

    bigbug->camera.width = FIELD_WIDTH;
    bigbug->camera.height = FIELD_HEIGHT;
}

/**
 * @brief Set the LEDs
 */
static void bb_SetLeds(void)
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

/**
 * @brief Finds unsupported dirt over many frames and crumbles it.
 */
static void bb_UpdateTileSupport(void){
    if(bigbug->gameData.unsupported->first != NULL){
        for(int i = 0; i < 50; i++)//arbitrarily large loop to get to the dirt tiles.
        {
            //remove the first item from the list
            uint32_t* shiftedVal = shift(bigbug->gameData.unsupported);
            //check that it's still dirt, because a previous pass may have crumbled it.
            if(bigbug->tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]] > 0)
            {
                //set it to air
                bigbug->tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]] = 0;
                //create a crumble animation
                bb_createEntity(&(bigbug->entityManager), ONESHOT_ANIMATION, CRUMBLE_ANIM, shiftedVal[0]*32+16, shiftedVal[1]*32+16);

                //queue neighbors for crumbling
                for(uint8_t neighborIdx = 0; neighborIdx < 4; neighborIdx++)
                {
                    if((int32_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] >= 0
                    && (int32_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] < TILE_FIELD_WIDTH
                    && (int32_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] >= 0
                    && (int32_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] < TILE_FIELD_HEIGHT)
                    {
                        uint32_t* val = heap_caps_calloc(2,sizeof(uint32_t), MALLOC_CAP_SPIRAM);
                        val[0] = shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0];
                        val[1] = shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1];

                        push(bigbug->gameData.unsupported, (void*)val);
                    }
                }
                break;
            }
        }
    }
}

static void bb_UpdatePhysics(int64_t elapsedUs)
{
    bigbug->garbotnikRotation.y += bigbug->garbotnikAccel.x;
    if(bigbug->garbotnikRotation.x < 0){
        bigbug->garbotnikRotation.y -=  5.0 * elapsedUs / 100000;
    }
    else{
        bigbug->garbotnikRotation.y += 5.0  * elapsedUs / 100000;
    }
    bigbug->garbotnikRotation.x += bigbug->garbotnikRotation.y;
    if(bigbug->garbotnikRotation.x < -1440){
        bigbug->garbotnikRotation.x = -1440;
        bigbug->garbotnikRotation.y = 0;
    }
    else if(bigbug->garbotnikRotation.x > 1440){
        bigbug->garbotnikRotation.x = 1440;
        bigbug->garbotnikRotation.y = 0;
    }
    // printf("rotation: %d\n",bigbug->garbotnikRotation.x);

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
    bigbug->garbotnikPos.x += bigbug->garbotnikVel.x * elapsedUs / 100000;
    bigbug->garbotnikPos.y += bigbug->garbotnikVel.y * elapsedUs / 100000;

    // Look up 4 nearest tiles for collision checks
    // a tile's width is 16 pixels << 4 = 512. half width is 256.
    int32_t xIdx = (bigbug->garbotnikPos.x - BITSHIFT_HALF_TILE)/BITSHIFT_TILE_SIZE - (bigbug->garbotnikPos.x < 0);//the x index
    int32_t yIdx = (bigbug->garbotnikPos.y  - BITSHIFT_HALF_TILE)/BITSHIFT_TILE_SIZE - (bigbug->garbotnikPos.y < 0);//the y index

    int32_t best_i = -1;//negative means no worthy candidates found.
    int32_t best_j = -1;
    int32_t closestSqDist = 1063842;//(307.35+724.077)^2 if it's further than this, there's no way it's a collision.
    for(int32_t i = xIdx; i <= xIdx+1; i++){
        for(int32_t j = yIdx; j <= yIdx+1; j++){
            if(i >= 0 && i < TILE_FIELD_WIDTH && j >=0 && j < TILE_FIELD_HEIGHT){
                if(bigbug->tilemap.fgTiles[i][j] >= 1){
                    //Initial circle check for preselecting the closest dirt tile
                    int32_t sqDist = sqMagVec2d(subVec2d(bigbug->garbotnikPos, (vec_t){i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE, j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE}));
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
        vec_t tilePos = {best_i * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE, best_j * BITSHIFT_TILE_SIZE + BITSHIFT_HALF_TILE};
        //AABB-AABB collision detection begins here
        //https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/
        if(bigbug->garbotnikPos.x + 240 > tilePos.x - BITSHIFT_HALF_TILE &&
           bigbug->garbotnikPos.x - 240 < tilePos.x + BITSHIFT_HALF_TILE &&
           bigbug->garbotnikPos.y + 192 > tilePos.y - BITSHIFT_HALF_TILE &&
           bigbug->garbotnikPos.y - 192 < tilePos.y + BITSHIFT_HALF_TILE)
        {
            ///////////////////////
            //Collision detected!//
            ///////////////////////
            //printf("hit\n");
            //Resolve garbotnik's position somewhat based on his position previously.
            vec_t normal = subVec2d(bigbug->previousPos, tilePos);
            //Snap the previous frame offset to an orthogonal direction.
            if((normal.x < 0?-normal.x:normal.x) > (normal.y < 0?-normal.y:normal.y)){
                if(normal.x > 0){
                    normal.x = 1;
                    normal.y = 0;
                    bigbug->garbotnikPos.x = tilePos.x + 240 + BITSHIFT_HALF_TILE;
                }
                else{
                    normal.x = -1;
                    normal.y = 0;
                    bigbug->garbotnikPos.x = tilePos.x - 240 - BITSHIFT_HALF_TILE;
                }
                
            }
            else{
                if(normal.y > 0){
                    normal.x = 0;
                    normal.y = 1;
                    bigbug->garbotnikPos.y = tilePos.y + 192 + BITSHIFT_HALF_TILE;
                }
                else{
                    normal.x = 0;
                    normal.y = -1;
                    bigbug->garbotnikPos.y = tilePos.y - 192 - BITSHIFT_HALF_TILE;
                }
            }
            //printf("dot product: %d\n",dotVec2d(bigbug->garbotnikVel, normal));
            if(dotVec2d(bigbug->garbotnikVel, normal) < -95)//velocity angle is opposing garbage normal vector. Tweak number for different threshold.
            {
                /////////////////////
                //digging detected!//
                /////////////////////

                //crumble test
                // uint32_t* val = calloc(2,sizeof(uint32_t));
                // val[0] = 5;
                // val[1] = 3;
                // push(bigbug->gameData.unsupported, (void*)val);

                //Update the dirt by decrementing it.
                bigbug->tilemap.fgTiles[best_i][best_j] -= 1;
                
                //Create a crumble animation
                bb_createEntity(&(bigbug->entityManager), ONESHOT_ANIMATION, CRUMBLE_ANIM, tilePos.x>>DECIMAL_BITS, tilePos.y>>DECIMAL_BITS);

                ///////////////////////////////
                //Mirror garbotnik's velocity//
                ///////////////////////////////
                // Reflect the velocity vector along the normal
                // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
                printf("hit squared speed: %" PRId32 "\n", sqMagVec2d(bigbug->garbotnikVel));
                int32_t bounceScalar = sqMagVec2d(bigbug->garbotnikVel)/-11075 + 3;
                if(bounceScalar > 3){
                    bounceScalar = 3;
                }
                else if(bounceScalar < 1){
                    bounceScalar = 1;
                }
                bigbug->garbotnikVel = mulVec2d(subVec2d(bigbug->garbotnikVel, mulVec2d(normal, (2* dotVec2d(bigbug->garbotnikVel, normal)))), bounceScalar);

                /////////////////////////////////
                //check neighbors for stability//
                /////////////////////////////////
                for(uint8_t neighborIdx = 0; neighborIdx < 4; neighborIdx++)
                {
                    uint32_t check_x = best_i + bigbug->gameData.neighbors[neighborIdx][0];
                    uint32_t check_y = best_j + bigbug->gameData.neighbors[neighborIdx][1];
                    //Check if neighbor is in bounds of map (also not on left, right, or bottom, perimiter) and if it is dirt.
                    if(check_x > 0 && check_x < TILE_FIELD_WIDTH - 1 && check_y > 0 && check_y < TILE_FIELD_HEIGHT - 1 && bigbug->tilemap.fgTiles[check_x][check_y] > 0)
                    {
                        uint32_t* val = calloc(3, sizeof(uint32_t));
                        val[0] = check_x;
                        val[1] = check_y;
                        val[2] = 1; //1 is for foreground. 0 is midground.
                    }
                }

                
            }
        }
    }

    // Update the camera's position to catch up to the player
    if(((bigbug->garbotnikPos.x - HALF_WIDTH) >> DECIMAL_BITS) - bigbug->camera.pos.x<-15){
        bigbug->camera.pos.x = ((bigbug->garbotnikPos.x - HALF_WIDTH) >> DECIMAL_BITS) + 15;
    }
    else if(((bigbug->garbotnikPos.x - HALF_WIDTH) >> DECIMAL_BITS) - bigbug->camera.pos.x>15){
        bigbug->camera.pos.x = ((bigbug->garbotnikPos.x - HALF_WIDTH) >> DECIMAL_BITS) - 15;
    }

    if(((bigbug->garbotnikPos.y - HALF_HEIGHT)  >> DECIMAL_BITS) - bigbug->camera.pos.y<-10){
        bigbug->camera.pos.y = ((bigbug->garbotnikPos.y - HALF_HEIGHT) >> DECIMAL_BITS) + 10;
    }
    else if(((bigbug->garbotnikPos.y - HALF_HEIGHT)  >> DECIMAL_BITS) - bigbug->camera.pos.y>10){
        bigbug->camera.pos.y = ((bigbug->garbotnikPos.y - HALF_HEIGHT) >> DECIMAL_BITS) - 10;
    }
}

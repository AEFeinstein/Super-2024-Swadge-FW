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

//==============================================================================
// Defines
//==============================================================================
#define DECIMAL_BITS 4

#define TILE_FIELD_WIDTH 8
#define TILE_FIELD_HEIGHT 16
#define GARBOTNIK_RADIUS   (19 << DECIMAL_BITS)
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

    rectangle_t camera; ///< The camera
    int8_t tiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT]; ///< The array of tiles. 1 is tile, 0 is not. Future feature: more variety

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for paddle control
    bool paddleRMovingUp;   ///< The CPU's paddle direction on easy mode
    bool isPaused;          ///< true if the game is paused, false if it is running

    wsg_t dirtWsg;        ///< A graphic for the dirt tile
    wsg_t garbotnikWsg;   ///< A graphic for garbotnik

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
    loadWsg("eggman.wsg", &bigbug->garbotnikWsg, false);

    // Set the mode to game mode
    bigbug->screen = BIGBUG_GAME;

    bigbugReset();
}
 
static void bigbugExitMode(void)
{
    // Fill this in
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
    int x_div = 0;
    int y_div = 0;
    // Update garbotnik's velocity if a button is currently down
    switch(bigbug->btnState){
        //up
        case 0b0001:
            y_div = -20000;
            break;
        case 0b1101:
            y_div = -20000;
            break;

        //down
        case 0b0010:
            y_div = 20000;
            break;
        case 0b1110:
            y_div = 20000;
            break;

        //left
        case 0b0100:
            x_div = -20000;
            break;
        case 0b0111:
            x_div = -20000;
            break;

        //right
        case 0b1000:
            x_div = 20000;
            break;
        case 0b1011:
            x_div = 20000;
            break;

        //up,left
        case 0b0101:
            x_div = -28284; //20000 / (1/sqrt(2)) = 20000 * sqrt(2) = 28284
            y_div = -28284;
            break;
        
        //up,right
        case 0b1001:
            x_div = 28284;
            y_div = -28284;
            break;

        //down,right
        case 0b1010:
            x_div = 28284;
            y_div = 28284;
            break;

        //down,left
        case 0b0110:
            x_div = -28284;
            y_div = 28284;
            break;
        default:
            break;
    }
    if (x_div != 0)
    {
        bigbug->garbotnikAccel.x = elapsedUs / x_div;
    }
    else{
        bigbug->garbotnikAccel.x = 0;
    }
    if (y_div != 0)
    {
        bigbug->garbotnikAccel.y = elapsedUs / y_div;
    }
    else{
        bigbug->garbotnikAccel.y = 0;
    }
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

    
    if(iEnd >= 0 && iStart <= TILE_FIELD_WIDTH && jEnd >= 0 && jStart <= FIELD_HEIGHT){
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
            jEnd = TILE_FIELD_WIDTH;
        }

        // printf("iStart: %d\n", iStart);
        // printf("iEnd: %d\n", iEnd);
        // printf("jStart: %d\n", jStart);
        // printf("jEnd: %d\n", jEnd);

        for (int i = iStart; i <= iEnd; i++){
            for (int j = jStart; j <= jEnd; j++){
                // Draw dirt tile
                drawWsgTile(&bigbug->dirtWsg, i * 64 - (bigbug->camera.pos.x >> DECIMAL_BITS), j * 64 - (bigbug->camera.pos.y >> DECIMAL_BITS));
            }
        }
    }

    

    // printf("garbotnik.pos.y: %d\n", bigbug->garbotnik.pos.y);
    // printf("garbotnik.radius: %d\n", bigbug->garbotnik.radius);
    // printf("camera.pos.y: %d\n", bigbug->camera.pos.y);
    // printf("render y: %d\n", (bigbug->garbotnik.pos.y - bigbug->garbotnik.radius - bigbug->camera.pos.y) >> DECIMAL_BITS);

    // Draw garbotnik
    drawWsgSimple(&bigbug->garbotnikWsg, (bigbug->garbotnik.pos.x - bigbug->garbotnik.radius - bigbug->camera.pos.x) >> DECIMAL_BITS,
                  (bigbug->garbotnik.pos.y - bigbug->garbotnik.radius - bigbug->camera.pos.y) >> DECIMAL_BITS);
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
    bigbug->garbotnik.pos.y  = -90 << DECIMAL_BITS;

    printf("The width is: %d\n", FIELD_WIDTH);
    printf("The height is: %d\n", FIELD_HEIGHT);

    bigbug->camera.width = FIELD_WIDTH;
    bigbug->camera.height = FIELD_HEIGHT;

    bigbug->garbotnik.radius = GARBOTNIK_RADIUS;

    //Set all tiles to dirt
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 3; j++){
        bigbug->tiles[i][j] = 1;
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
    // Update garbotnik's velocity
    bigbug->garbotnikVel.x += bigbug->garbotnikAccel.x;
    bigbug->garbotnikVel.y += bigbug->garbotnikAccel.y;

    // Update garbotnik's position
    bigbug->garbotnik.pos.x += bigbug->garbotnikVel.x * elapsedUs / 100000;
    bigbug->garbotnik.pos.y += bigbug->garbotnikVel.y * elapsedUs / 100000;

    // Update the camera's position to catch up to the player

    bigbug->camera.pos.x = bigbug->garbotnik.pos.x - HALF_WIDTH;
    bigbug->camera.pos.y = bigbug->garbotnik.pos.y - HALF_HEIGHT;
}


//==============================================================================
// Includes
//==============================================================================

#include "mode_sand.h"

//==============================================================================
// Defines
//==============================================================================

#define WORLD_WIDTH  TFT_WIDTH
#define WORLD_HEIGHT TFT_HEIGHT

#define SIM_TIME_STEP 50000

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    EMPTY,
    SAND,
    NUM_SAND_TYPES,
} sandType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    midiFile_t bgm;
    font_t ibm;
    sandType_t** world;
    int32_t simTimer;
} sand_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void sandEnterMode(void);
static void sandExitMode(void);
static void sandMainLoop(int64_t elapsedUs);
static void sandBackgroundDraw(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Const Variables
//==============================================================================

const char sandName[] = "Sand";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t sandMode = {
    .modeName                 = sandName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = sandEnterMode,
    .fnExitMode               = sandExitMode,
    .fnMainLoop               = sandMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = sandBackgroundDraw,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

sand_t* sand;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter 2048 mode and set everything up
 */
static void sandEnterMode(void)
{
    setFrameRateUs(0); // Unlimited

    // Init Mode & resources
    sand = heap_caps_calloc(sizeof(sand_t), 1, MALLOC_CAP_8BIT);

    // Load sounds
    loadMidiFile("hd_credits.mid", &sand->bgm, true);

    // Load font
    loadFont("ibm_vga8.font", &sand->ibm, true);

    // Set and play the song
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiGmOn(player);
    midiSetFile(player, &sand->bgm);
    player->loop = true;
    midiPause(player, false);

    // Create world
    sand->world = heap_caps_calloc(WORLD_WIDTH, sizeof(sandType_t*), MALLOC_CAP_SPIRAM);
    for (int32_t x = 0; x < WORLD_WIDTH; x++)
    {
        sand->world[x] = heap_caps_calloc(WORLD_HEIGHT, sizeof(sandType_t), MALLOC_CAP_SPIRAM);
        for (int32_t y = 0; y < WORLD_HEIGHT; y++)
        {
            if (esp_random() % 2)
            {
                sand->world[x][y] = SAND;
            }
        }
    }
}

/**
 * @brief Exit 2048 mode and free all resources
 */
static void sandExitMode(void)
{
    soundStop(true);
    unloadMidiFile(&sand->bgm);

    freeFont(&sand->ibm);

    // free world
    for (int32_t w = 0; w < WORLD_WIDTH; w++)
    {
        heap_caps_free(sand->world[w]);
    }
    heap_caps_free(sand->world);

    heap_caps_free(sand);
}

static void simulateSand(void)
{
    for (int32_t y = WORLD_HEIGHT - 2; y >= 0; y--)
    {
        for (int32_t x = 0; x < WORLD_WIDTH; x++)
        {
            // If there's sand here
            if (SAND == sand->world[x][y])
            {
                // Try moving down first
                if (EMPTY == sand->world[x][y + 1])
                {
                    sand->world[x][y]     = EMPTY;
                    sand->world[x][y + 1] = SAND;
                }
                // Then down-right
                else if (x < WORLD_WIDTH - 1 && EMPTY == sand->world[x + 1][y + 1])
                {
                    sand->world[x][y]         = EMPTY;
                    sand->world[x + 1][y + 1] = SAND;
                }
                // then down-left
                else if (x > 0 && EMPTY == sand->world[x - 1][y + 1])
                {
                    sand->world[x][y]         = EMPTY;
                    sand->world[x - 1][y + 1] = SAND;
                }
            }
        }
    }
}

/**
 * @brief The main game loop, responsible for input handling, game logic, and animation
 *
 * @param elapsedUs The time since this was last called, for animation
 */
static void sandMainLoop(int64_t elapsedUs)
{
    // Handle inputs
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ;
    }

    RUN_TIMER_EVERY(sand->simTimer, SIM_TIME_STEP, elapsedUs, simulateSand(););

    SETUP_FOR_TURBO();
    for (int32_t y = 0; y < WORLD_HEIGHT; y++)
    {
        for (int32_t x = 0; x < WORLD_WIDTH; x++)
        {
            switch (sand->world[x][y])
            {
                default:
                case EMPTY:
                {
                    break;
                }
                case SAND:
                {
                    TURBO_SET_PIXEL(x, y, c300);
                    break;
                }
            }
        }
    }

    DRAW_FPS_COUNTER(sand->ibm);
}

/**
 * @brief Integrate the accelerometer in the background
 *
 * @param x Unused
 * @param y  Unused
 * @param w  Unused
 * @param h  Unused
 * @param up  Unused
 * @param upNum  Unused
 */
static void sandBackgroundDraw(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up __attribute__((unused)),
                               int16_t upNum __attribute__((unused)))
{
    if (y < TFT_HEIGHT / 2)
    {
        fillDisplayArea(x, y, x + w, y + h, c003);
    }
    else
    {
        fillDisplayArea(x, y, x + w, y + h, c030);
    }
}

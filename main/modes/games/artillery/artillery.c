//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_phys.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_PLAYERS 2

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    physSim_t* phys;
    physCirc_t* players[NUM_PLAYERS];
    int32_t autofire;
} artilleryData_t;

//==============================================================================
// Function Declarations
//==============================================================================

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t artilleryMode = {
    .modeName                 = "Artillery",
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = artilleryEnterMode,
    .fnExitMode               = artilleryExitMode,
    .fnMainLoop               = artilleryMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = artilleryBackgroundDrawCallback,
    .fnEspNowRecvCb           = artilleryEspNowRecvCb,
    .fnEspNowSendCb           = artilleryEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = NULL,
};

artilleryData_t* ad;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void artilleryEnterMode(void)
{
    setFrameRateUs(1000000 / 60);

    ad = heap_caps_calloc(1, sizeof(artilleryData_t), MALLOC_CAP_8BIT);

    ad->phys = initPhys(TFT_WIDTH, TFT_HEIGHT, 0, 100 / (1000000.0f * 1000000.0f));

#define GROUND_LEVEL 200

    // Add some ground
    vecFl_t groundPoints[] = {
        {.x = 0, .y = GROUND_LEVEL},         {.x = TFT_WIDTH / 4, .y = GROUND_LEVEL},
        {.x = TFT_WIDTH / 2, .y = 100},      {.x = 3 * TFT_WIDTH / 4, .y = GROUND_LEVEL},
        {.x = TFT_WIDTH, .y = GROUND_LEVEL},
    };
    for (int idx = 0; idx < ARRAY_SIZE(groundPoints) - 1; idx++)
    {
        physAddLine(ad->phys, groundPoints[idx].x, groundPoints[idx].y, groundPoints[idx + 1].x,
                    groundPoints[idx + 1].y);
    }

    // Add an obstacle
    physAddCircle(ad->phys, TFT_WIDTH / 2, 30, 10, CT_OBSTACLE);

    // Add some players
#define PLAYER_RADIUS 8
    ad->players[0] = physAddCircle(ad->phys, TFT_WIDTH / 8, GROUND_LEVEL - PLAYER_RADIUS - 1, PLAYER_RADIUS, CT_TANK);
    ad->players[1]
        = physAddCircle(ad->phys, (7 * TFT_WIDTH) / 8, GROUND_LEVEL - PLAYER_RADIUS - 1, PLAYER_RADIUS, CT_TANK);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
    deinitPhys(ad->phys);
    heap_caps_free(ad);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void artilleryMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_A:
                {
                    fireShot(ad->phys, ad->players[0]);
                    break;
                }
                case PB_LEFT:
                case PB_RIGHT:
                {
                    float bDiff = 4 * ((PB_LEFT == evt.button) ? -(M_PI / 180.0f) : (M_PI / 180.0f));
                    setBarrelAngle(ad->players[0], ad->players[0]->barrelAngle + bDiff);
                    break;
                }
                case PB_UP:
                case PB_DOWN:
                {
                    float pDiff = (PB_UP == evt.button) ? 0.00001f : -0.00001f;
                    setShotPower(ad->players[0], ad->players[0]->shotPower + pDiff);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    // Uncomment for autofire
    // RUN_TIMER_EVERY(ad->autofire, 100000, elapsedUs, { fireShot(ad->phys, ad->players[0]); });

    physStep(ad->phys, elapsedUs);
    drawPhysOutline(ad->phys);

    font_t* f = getSysFont();
    DRAW_FPS_COUNTER((*f));

    char fireParams[64];
    snprintf(fireParams, sizeof(fireParams) - 1, "Angle %0.3f, Power %.3f", ad->players[0]->barrelAngle,
             ad->players[0]->shotPower * 100);
    drawText(f, c555, fireParams, 40, TFT_HEIGHT - f->height - 2);
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c000);
}

/**
 * @brief This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
}

/**
 * @brief This function is called whenever an ESP-NOW packet is sent. It is just a status callback whether or not
 * the packet was actually sent. This will be called after calling espNowSend().
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
}

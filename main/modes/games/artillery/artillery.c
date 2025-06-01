#include "artillery.h"

typedef struct
{
    list_t groundLines;
    list_t projectiles;
} artilleryData_t;

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

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

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void artilleryEnterMode(void)
{
    ad = heap_caps_calloc(1, sizeof(artilleryData_t), MALLOC_CAP_8BIT);

    vecFl_t groundPoints[] = {
        {.x = 0, .y = (3 * TFT_HEIGHT) / 4},
        {.x = TFT_WIDTH / 2, .y = (TFT_HEIGHT) / 4},
        {.x = TFT_WIDTH, .y = (3 * TFT_HEIGHT) / 4},
    };

    for (int idx = 0; idx < ARRAY_SIZE(groundPoints) - 1; idx++)
    {
        lineFl_t* newLine = heap_caps_calloc(1, sizeof(lineFl_t), MALLOC_CAP_8BIT);
        newLine->p1.x     = groundPoints[idx].x;
        newLine->p1.y     = groundPoints[idx].y;
        newLine->p2.x     = groundPoints[idx + 1].x;
        newLine->p2.y     = groundPoints[idx + 1].y;
        push(&ad->groundLines, newLine);
    }

    vecFl_t projectiles[] = {
        {.x = TFT_WIDTH / 4, .y = 0},
        {.x = (2 * TFT_WIDTH) / 4, .y = 0},
        {.x = (3 * TFT_WIDTH) / 4, .y = 0},
    };

    for (int idx = 0; idx < ARRAY_SIZE(projectiles); idx++)
    {
        circleFl_t* projectile = heap_caps_calloc(1, sizeof(circleFl_t), MALLOC_CAP_8BIT);
        projectile->pos.x      = projectiles[idx].x;
        projectile->pos.y      = projectiles[idx].y;
        projectile->radius     = 5;
        push(&ad->projectiles, projectile);
    }
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void artilleryExitMode(void)
{
    while (ad->groundLines.first)
    {
        heap_caps_free(pop(&ad->groundLines));
    }
    while (ad->projectiles.first)
    {
        heap_caps_free(pop(&ad->projectiles));
    }
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
        ; // TODO button presses
    }

    clearPxTft();

    node_t* groundNode = ad->groundLines.first;
    while (groundNode)
    {
        lineFl_t* gLine = (lineFl_t*)groundNode->val;
        drawLine(gLine->p1.x, gLine->p1.y, gLine->p2.x, gLine->p2.y, c555, 0);
        groundNode = groundNode->next;
    }

    node_t* projectileNode = ad->projectiles.first;
    while (projectileNode)
    {
        circleFl_t* projectile = (circleFl_t*)projectileNode->val;
        drawCircle(projectile->pos.x, projectile->pos.y, projectile->radius, c555);
        projectileNode = projectileNode->next;
    }
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

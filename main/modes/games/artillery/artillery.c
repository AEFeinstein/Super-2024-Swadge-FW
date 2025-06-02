#include "artillery.h"

// There are 32 zones split into an 8x4 grid
#define NUM_ZONES_BIG 8
#define NUM_ZONES_LIL 4
#define NUM_ZONES     (NUM_ZONES_BIG * NUM_ZONES_LIL)

typedef struct
{
    vecFl_t g;
    vecFl_t bounds;
    rectangleFl_t zones[NUM_ZONES];
} physSim_t;

typedef struct
{
    int32_t zonemask;
    bool fixed;
    vecFl_t g;
    circleFl_t c;
} physCirc_t;

typedef struct
{
    int32_t zonemask;
    bool fixed;
    vecFl_t g;
    lineFl_t l;
} physLine_t;

typedef struct
{
    physSim_t phys;
    list_t groundLines;
    list_t projectiles;
} artilleryData_t;

void artilleryEnterMode(void);
void artilleryExitMode(void);
void artilleryMainLoop(int64_t elapsedUs);
void artilleryBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void artilleryEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void artilleryEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl);
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc);

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

    // Set bounds for the physics sim
    ad->phys.bounds.x = TFT_WIDTH;
    ad->phys.bounds.y = TFT_HEIGHT;

    // Figure out zones
    vec_t zoneCnt;
    if (ad->phys.bounds.x > ad->phys.bounds.y)
    {
        zoneCnt.x = NUM_ZONES_BIG;
        zoneCnt.y = NUM_ZONES_LIL;
    }
    else
    {
        zoneCnt.x = NUM_ZONES_LIL;
        zoneCnt.y = NUM_ZONES_BIG;
    }

    // Calculate the zone size
    vecFl_t zoneSize = {
        .x = ad->phys.bounds.x / zoneCnt.x,
        .y = ad->phys.bounds.y / zoneCnt.y,
    };

    // Create the zones
    for (int32_t y = 0; y < zoneCnt.y; y++)
    {
        for (int32_t x = 0; x < zoneCnt.x; x++)
        {
            rectangleFl_t* zone = &ad->phys.zones[(y * zoneCnt.x) + x];
            zone->pos.x         = x * zoneSize.x;
            zone->pos.y         = y * zoneSize.y;
            zone->width         = zoneSize.x;
            zone->height        = zoneSize.y;
        }
    }

    vecFl_t groundPoints[] = {
        {.x = 0, .y = (3 * TFT_HEIGHT) / 4},
        {.x = TFT_WIDTH / 2, .y = (TFT_HEIGHT) / 4},
        {.x = TFT_WIDTH, .y = (3 * TFT_HEIGHT) / 4},
    };

    for (int idx = 0; idx < ARRAY_SIZE(groundPoints) - 1; idx++)
    {
        physLine_t* pl = heap_caps_calloc(1, sizeof(physLine_t), MALLOC_CAP_8BIT);
        pl->fixed      = true;
        pl->l.p1.x     = groundPoints[idx].x;
        pl->l.p1.y     = groundPoints[idx].y;
        pl->l.p2.x     = groundPoints[idx + 1].x;
        pl->l.p2.y     = groundPoints[idx + 1].y;
        physSetZoneMaskLine(&ad->phys, pl);
        push(&ad->groundLines, pl);
    }

    vecFl_t projectiles[] = {
        {.x = TFT_WIDTH / 4, .y = 0},
        {.x = (2 * TFT_WIDTH) / 4, .y = 0},
        {.x = (3 * TFT_WIDTH) / 4, .y = 0},
    };

    for (int idx = 0; idx < ARRAY_SIZE(projectiles); idx++)
    {
        physCirc_t* pc = heap_caps_calloc(1, sizeof(physCirc_t), MALLOC_CAP_8BIT);
        pc->c.pos.x    = projectiles[idx].x;
        pc->c.pos.y    = projectiles[idx].y;
        pc->c.radius   = 5;
        physSetZoneMaskCirc(&ad->phys, pc);
        push(&ad->projectiles, pc);
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
        physLine_t* gLine = (physLine_t*)groundNode->val;
        drawLine(gLine->l.p1.x, gLine->l.p1.y, gLine->l.p2.x, gLine->l.p2.y, c555, 0);
        groundNode = groundNode->next;
    }

    node_t* projectileNode = ad->projectiles.first;
    while (projectileNode)
    {
        physCirc_t* projectile = (physCirc_t*)projectileNode->val;
        drawCircle(projectile->c.pos.x, projectile->c.pos.y, projectile->c.radius, c555);
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

/**
 * @brief TODO
 *
 * @param phys
 * @param line
 */
void physSetZoneMaskLine(physSim_t* phys, physLine_t* pl)
{
    pl->zonemask = 0;
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (rectLineFlIntersection(phys->zones[zIdx], pl->l, NULL))
        {
            pl->zonemask |= (1 << zIdx);
        }
    }
}

/**
 * @brief TODO
 *
 * @param phys
 * @param pc
 */
void physSetZoneMaskCirc(physSim_t* phys, physCirc_t* pc)
{
    pc->zonemask = 0;
    for (int32_t zIdx = 0; zIdx < NUM_ZONES; zIdx++)
    {
        if (circleRectFlIntersection(pc->c, phys->zones[zIdx], NULL))
        {
            pc->zonemask |= (1 << zIdx);
        }
    }
}

//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_p2p.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_bsp.h"
#include "artillery_game.h"

#define NUM_TERRAIN_POINTS_A (64 + 18)
#define NUM_TERRAIN_POINTS_B (NUM_TERRAIN_POINTS - NUM_TERRAIN_POINTS_A)

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    P2P_SET_WORLD,
    P2P_ADD_TERRAIN,
    P2P_SET_PLAYERS,
    P2P_FIRE_SHOT,
} artilleryP2pPacketType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint16_t width;
    uint16_t height;
    struct
    {
        vecFl_t pos;
        float barrelAngle;
    } players[NUM_PLAYERS];

    uint16_t terrainPoints[NUM_TERRAIN_POINTS_A];
} artPktWorld_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint16_t terrainPoints[NUM_TERRAIN_POINTS_B];
    uint8_t numObstacles;
    struct
    {
        uint16_t x;
        uint16_t y;
        uint8_t r;
    } obstacles[MAX_NUM_OBSTACLES];

} artPktTerrain_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
    struct
    {
        vecFl_t pos;
        float barrelAngle;
    } players[NUM_PLAYERS];
} artPktPlayers_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
} artPktShot_t;

//==============================================================================
// Const Variables
//==============================================================================

const char str_conStarted[]     = "Connection Started";
const char str_conRxAck[]       = "Rx Game Start Ack";
const char str_conRxMsg[]       = "Rx Game Start Msg";
const char str_conEstablished[] = "Connection Established";
const char str_conLost[]        = "Connection Lost";

//==============================================================================
// Function Declarations
//==============================================================================

static uint8_t getSizeFromType(artilleryP2pPacketType_t type);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This typedef is for the function callback which delivers connection statuses to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
void artillery_p2pConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    artilleryData_t* ad = getArtilleryData();
    // TODO select connection text
    switch (evt)
    {
        case CON_STARTED:
        {
            ad->conStr = str_conStarted;
            break;
        }
        case RX_GAME_START_ACK:
        {
            ad->conStr = str_conRxAck;
            break;
        }
        case RX_GAME_START_MSG:
        {
            ad->conStr = str_conRxMsg;
            break;
        }
        case CON_ESTABLISHED:
        {
            ad->conStr = str_conEstablished;

            // If going first, generate and transmit the world
            if (GOING_FIRST == p2pGetPlayOrder(p2p))
            {
                artilleryInitGame(AG_WIRELESS, true);
                artilleryTxWorld(ad);
            }
            break;
        }
        case CON_LOST:
        default:
        {
            ad->conStr = str_conLost;
            break;
        }
    }
}

/**
 * @brief This typedef is for the function callback which delivers received p2p packets to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
void artillery_p2pMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    ESP_LOGI("VT", "RX type %d, len %d", payload[0], len);

    artilleryData_t* ad = getArtilleryData();

    switch (payload[0])
    {
        case P2P_SET_WORLD:
        {
            const artPktWorld_t* pkt = (const artPktWorld_t*)payload;

            // Deinit physics if it happens to be initialized
            if (ad->phys)
            {
                deinitPhys(ad->phys);
            }

            // If the game hasn't started yet, start it
            if (AMS_GAME != ad->mState)
            {
                artilleryInitGame(AG_WIRELESS, false);
            }

            // Add lines from packet
            physAddTerrainPoints(ad->phys, 0, pkt->terrainPoints, ARRAY_SIZE(pkt->terrainPoints));

            // Add players from packet
            for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
            {
                ad->players[pIdx] = physAddPlayer(ad->phys, pkt->players[pIdx].pos, pkt->players[pIdx].barrelAngle);
            }
            return;
        }
        case P2P_ADD_TERRAIN:
        {
            const artPktTerrain_t* pkt = (const artPktTerrain_t*)payload;

            // Add more lines from packet
            physAddTerrainPoints(ad->phys, NUM_TERRAIN_POINTS_A, pkt->terrainPoints, ARRAY_SIZE(pkt->terrainPoints));

            // Now that terrain is received, create BSP zones
            createBspZones(ad->phys);

            // Mark simulation as ready
            ad->phys->isReady = true;

            // Show the menu
            artillerySwitchToState(ad, AGS_MENU);
            return;
        }
        case P2P_SET_PLAYERS:
        {
            const artPktPlayers_t* pkt = (const artPktPlayers_t*)payload;
            return;
        }
        case P2P_FIRE_SHOT:
        {
            const artPktShot_t* pkt = (const artPktShot_t*)payload;
            return;
        }
    }
}

/**
 * @brief This typedef is for the function callback which delivers acknowledge status for transmitted messages to the
 * Swadge mode
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void artillery_p2pMsgTxCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
}

/**
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryTxWorld(artilleryData_t* ad)
{
    artPktWorld_t* pkt1          = heap_caps_calloc(1, sizeof(artPktWorld_t), MALLOC_CAP_SPIRAM);
    pkt1->type                   = P2P_SET_WORLD;
    pkt1->width                  = ad->phys->bounds.x;
    pkt1->height                 = ad->phys->bounds.y;
    pkt1->players[0].pos         = ad->players[0]->c.pos;
    pkt1->players[0].barrelAngle = ad->players[0]->barrelAngle;
    pkt1->players[1].pos         = ad->players[1]->c.pos;
    pkt1->players[1].barrelAngle = ad->players[1]->barrelAngle;

    artPktTerrain_t* pkt2 = heap_caps_calloc(1, sizeof(artPktTerrain_t), MALLOC_CAP_SPIRAM);
    pkt2->type            = P2P_ADD_TERRAIN;

    // Build a list of Y values
    // Iterate through lines and assume that they're already sorted left to right
    uint16_t tIdx = 0;
    node_t* lNode = ad->phys->lines.first;
    while (lNode)
    {
        physLine_t* line = lNode->val;
        // If this is terrain
        if (line->isTerrain)
        {
            if (tIdx < ARRAY_SIZE(pkt1->terrainPoints))
            {
                if (0 == tIdx)
                {
                    // Only add p1 for the first point
                    pkt1->terrainPoints[tIdx++] = (uint16_t)line->l.p1.y;
                }
                pkt1->terrainPoints[tIdx++] = (uint16_t)line->l.p2.y;
            }
            else
            {
                pkt2->terrainPoints[(tIdx++) - ARRAY_SIZE(pkt1->terrainPoints)] = (uint16_t)line->l.p2.y;
            }
        }
        lNode = lNode->next;
    }

    // TODO Enqueue the sizes, somehow
    push(&ad->p2pQueue, pkt1);
    push(&ad->p2pQueue, pkt2);
}

/**
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryCheckTxQueue(artilleryData_t* ad)
{
    // If there is a message to send, and transmission is idle
    if (ad->p2pQueue.first && p2pIsTxIdle(&ad->p2p))
    {
        uint8_t* payload = shift(&ad->p2pQueue);
        // Send over p2p
        p2pSendMsg(&ad->p2p, payload, getSizeFromType(payload[0]), artillery_p2pMsgTxCb);
        // Free queued message
        heap_caps_free(payload);
    }
}

/**
 * @brief TODO doc
 *
 * @param type
 * @return uint8_t
 */
static uint8_t getSizeFromType(artilleryP2pPacketType_t type)
{
    switch (type)
    {
        case P2P_SET_WORLD:
        {
            return sizeof(artPktWorld_t);
        }
        case P2P_ADD_TERRAIN:
        {
            return sizeof(artPktTerrain_t);
        }
        case P2P_SET_PLAYERS:
        {
            return sizeof(artPktPlayers_t);
        }
        case P2P_FIRE_SHOT:
        {
            return sizeof(artPktShot_t);
        }
    }
    return 0;
}

//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_p2p.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_bsp.h"
#include "artillery_game.h"
#include "artillery_paint.h"

#define NUM_TERRAIN_POINTS_A (64 + 18)
#define NUM_TERRAIN_POINTS_B (NUM_TERRAIN_POINTS - NUM_TERRAIN_POINTS_A)

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    P2P_SET_COLOR,
    P2P_SET_WORLD,
    P2P_ADD_TERRAIN,
    P2P_SET_PLAYERS,
    P2P_FIRE_SHOT,
    P2P_PASS_TURN,
} artilleryP2pPacketType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint8_t colorIdx;
} artPktColor_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint16_t width;
    uint16_t height;
    struct
    {
        vecFl_t pos;
        float barrelAngle;
        int32_t score;
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
        int32_t score;
    } players[NUM_PLAYERS];
    vec_t camera;
    int32_t moveTimeLeftUs;
} artPktPlayers_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
    artilleryAmmoType_t ammo;
    float barrelAngle;
    float shotPower;
} artPktShot_t;

typedef struct __attribute__((packed))
{
    uint8_t type;
} artPktPassTurn_t;

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
 * TODO doc
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
void artillery_p2pConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    artilleryData_t* ad = getArtilleryData();
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
                artilleryTxColor(ad);
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
 * TODO doc
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
        case P2P_SET_COLOR:
        {
            const artPktColor_t* pkt = (const artPktColor_t*)payload;
            ad->theirColorIdx        = pkt->colorIdx;

            // If going second
            if (GOING_SECOND == p2pGetPlayOrder(p2p))
            {
                // Reply with our color
                artilleryTxColor(ad);
                // Prepare to receive terrain
                artilleryInitGame(AG_WIRELESS, false);
            }
            // If going first
            else
            {
                // Both colors transmitted, start the game
                artilleryInitGame(AG_WIRELESS, true);
                artilleryTxWorld(ad);
            }
            break;
        }
        case P2P_SET_WORLD:
        {
            const artPktWorld_t* pkt = (const artPktWorld_t*)payload;

            // Remove everything before receiving
            physRemoveAllObjects(ad->phys);
            // Immediately add world bounds, not in the transmitted packet
            physAddWorldBounds(ad->phys);

            // Add lines from packet
            physAddTerrainPoints(ad->phys, 0, pkt->terrainPoints, NUM_TERRAIN_POINTS_A);

            // Set the color indices depending on the play order
            uint8_t colorIndices[2] = {0};
            if (GOING_FIRST == p2pGetPlayOrder(p2p))
            {
                colorIndices[0] = ad->theirColorIdx;
                colorIndices[1] = ad->myColorIdx;
            }
            else
            {
                colorIndices[0] = ad->myColorIdx;
                colorIndices[1] = ad->theirColorIdx;
            }

            // Add players from packet
            for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
            {
                paletteColor_t base;
                paletteColor_t accent;
                artilleryGetTankColors(colorIndices[pIdx], &base, &accent);
                ad->players[pIdx]
                    = physAddPlayer(ad->phys, pkt->players[pIdx].pos, pkt->players[pIdx].barrelAngle, base, accent);
                ad->players[pIdx]->score = pkt->players[pIdx].score;
            }

            // Mark as not ready until the other packet is received
            ad->phys->isReady = false;

            return;
        }
        case P2P_ADD_TERRAIN:
        {
            const artPktTerrain_t* pkt = (const artPktTerrain_t*)payload;

            // Add more lines from packet
            physAddTerrainPoints(ad->phys, NUM_TERRAIN_POINTS_A - 1, pkt->terrainPoints, NUM_TERRAIN_POINTS_B);

            // Now that terrain is received, create BSP zones
            createBspZones(ad->phys);

            // Mark simulation as ready
            ad->phys->isReady = true;

            // After receiving terrain, open the menu
            artillerySwitchToGameState(ad, AGS_MENU);
            return;
        }
        case P2P_SET_PLAYERS:
        {
            const artPktPlayers_t* pkt = (const artPktPlayers_t*)payload;

            // Update player location and barrel angle
            for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
            {
                ad->players[pIdx]->c.pos = pkt->players[pIdx].pos;
                setBarrelAngle(ad->players[pIdx], pkt->players[pIdx].barrelAngle);
                ad->players[pIdx]->score = pkt->players[pIdx].score;
            }

            // Update camera and don't track objects
            ad->phys->camera = pkt->camera;
            clear(&ad->phys->cameraTargets);

            // Update gas gauge
            ad->moveTimerUs = pkt->moveTimeLeftUs;

            return;
        }
        case P2P_FIRE_SHOT:
        {
            const artPktShot_t* pkt = (const artPktShot_t*)payload;

            ad->players[ad->plIdx]->barrelAngle = pkt->barrelAngle;
            ad->players[ad->plIdx]->ammo        = pkt->ammo;
            ad->players[ad->plIdx]->shotPower   = pkt->shotPower;

            fireShot(ad->phys, ad->players[ad->plIdx]);
            return;
        }
        case P2P_PASS_TURN:
        {
            // const artPktPassTurn_t* pkt = (const artPktPassTurn_t*)payload;
            artilleryPassTurn(ad);
            return;
        }
    }
}

/**
 * @brief This typedef is for the function callback which delivers acknowledge status for transmitted messages to the
 * Swadge mode
 * TODO doc
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
void artilleryTxColor(artilleryData_t* ad)
{
    // Allocate a packet
    artPktColor_t* pkt = heap_caps_calloc(1, sizeof(artPktShot_t), MALLOC_CAP_SPIRAM);

    // Write the type
    pkt->type = P2P_SET_COLOR;

    // Write the data
    pkt->colorIdx = ad->myColorIdx;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief TODO doc
 *
 * @param ad All the artillery mode data
 */
void artilleryTxWorld(artilleryData_t* ad)
{
    artPktWorld_t* pkt1          = heap_caps_calloc(1, sizeof(artPktWorld_t), MALLOC_CAP_SPIRAM);
    pkt1->type                   = P2P_SET_WORLD;
    pkt1->width                  = ad->phys->bounds.x;
    pkt1->height                 = ad->phys->bounds.y;
    pkt1->players[0].pos         = ad->players[0]->c.pos;
    pkt1->players[0].barrelAngle = ad->players[0]->barrelAngle;
    pkt1->players[0].score       = ad->players[0]->score;
    pkt1->players[1].pos         = ad->players[1]->c.pos;
    pkt1->players[1].barrelAngle = ad->players[1]->barrelAngle;
    pkt1->players[1].score       = ad->players[1]->score;

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
 * @param ad All the artillery mode data
 */
void artilleryTxPlayers(artilleryData_t* ad)
{
    // Remove any other enqueued packet of this type first
    node_t* pktNode = ad->p2pQueue.first;
    while (pktNode)
    {
        node_t* pktNodeNext           = pktNode->next;
        artilleryP2pPacketType_t type = *((uint8_t*)pktNode->val);
        if (P2P_SET_PLAYERS == type)
        {
            heap_caps_free(removeEntry(&ad->p2pQueue, pktNode));
        }
        pktNode = pktNodeNext;
    }

    // Allocate a packet
    artPktPlayers_t* pkt = heap_caps_calloc(1, sizeof(artPktPlayers_t), MALLOC_CAP_SPIRAM);

    // Write the type
    pkt->type = P2P_SET_PLAYERS;

    // Write player location and barrel angle
    for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
    {
        pkt->players[pIdx].pos         = ad->players[pIdx]->c.pos;
        pkt->players[pIdx].barrelAngle = ad->players[pIdx]->barrelAngle;
        pkt->players[pIdx].score       = ad->players[pIdx]->score;
    }

    // Update camera
    pkt->camera = ad->phys->camera;

    // Write gas gauge
    pkt->moveTimeLeftUs = ad->moveTimerUs;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param player
 */
void artilleryTxShot(artilleryData_t* ad, physCirc_t* player)
{
    // Allocate a packet
    artPktShot_t* pkt = heap_caps_calloc(1, sizeof(artPktShot_t), MALLOC_CAP_SPIRAM);

    // Write the type
    pkt->type = P2P_FIRE_SHOT;

    // Write the data
    pkt->ammo        = player->ammo;
    pkt->barrelAngle = player->barrelAngle;
    pkt->shotPower   = player->shotPower;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryTxPassTurn(artilleryData_t* ad)
{
    // Allocate a packet
    artPktPassTurn_t* pkt = heap_caps_calloc(1, sizeof(artPktPassTurn_t), MALLOC_CAP_SPIRAM);

    // Write the type
    pkt->type = P2P_PASS_TURN;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief TODO doc
 *
 * @param ad All the artillery mode data
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
        case P2P_SET_COLOR:
        {
            return sizeof(artPktColor_t);
        }
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
        case P2P_PASS_TURN:
        {
            return sizeof(artPktPassTurn_t);
        }
    }
    return 0;
}

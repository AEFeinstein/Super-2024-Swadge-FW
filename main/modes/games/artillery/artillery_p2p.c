/**
 * @file artillery_p2p.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief TODO file summary
 * @date 2025-11-26
 * @startuml{conn_seq.png} "Connection Sequence"
== Setup ==

"Going First" -> "Going Second": artilleryTxColor(), P2P_SET_COLOR
note over "Going Second": artilleryInitGame(false)
"Going Second" -> "Going First": artilleryTxColor(), P2P_SET_COLOR
note over "Going First": artilleryInitGame(true)
"Going First" -> "Going Second": artilleryTxWorld(), P2P_SET_WORLD
note over "Going Second": physRemoveAllObjects()\nphysAddWorldBounds()\nphysAddTerrainPoints()
"Going First" -> "Going Second": artilleryTxWorld(), P2P_ADD_TERRAIN
note over "Going Second": physAddTerrainPoints()
"Going First" -> "Going Second": artilleryTxWorld(), P2P_SET_CLOUDS
note over "Going Second": Set Clouds
note over "Going First": artillerySwitchToGameState(AGS_TOUR)
note over "Going Second": artillerySwitchToGameState(AGS_TOUR)

== Optional ==

note over "Going First", "Going Second": "Either side may send this message end the tour early
"Going First" -> "Going Second": P2P_FINISH_TOUR
"Going Second" -> "Going First": P2P_FINISH_TOUR

== Loop for Seven Turns ==

group Optional, Repeated
    "Going Second" -> "Going First": P2P_SET_STATE
end
"Going Second" -> "Going First": P2P_FIRE_SHOT
group Optional, Repeated
    "Going First" -> "Going Second": P2P_SET_STATE
end
"Going First" -> "Going Second": P2P_FIRE_SHOT
 * @enduml
 */

//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_p2p.h"
#include "artillery_paint.h"
#include "artillery_phys.h"
#include "artillery_phys_terrain.h"
#include "artillery_phys_bsp.h"

#define NUM_TERRAIN_POINTS_A (64 + 18)
#define NUM_TERRAIN_POINTS_B (NUM_TERRAIN_POINTS - NUM_TERRAIN_POINTS_A)

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief TODO doc
 *
 */
typedef struct // __attribute__((packed))
{
    uint8_t type;
    uint8_t colorIdx;
} artPktColor_t;

/**
 * @brief TODO doc
 *
 */
typedef struct //__attribute__((packed))
{
    uint8_t type;
    uint8_t bgmIdx;
    uint16_t width;
    uint16_t height;
    struct
    {
        vecFl_t pos;
        int16_t barrelAngle;
        int32_t score;
    } players[NUM_PLAYERS];

    uint16_t terrainPoints[NUM_TERRAIN_POINTS_A];
} artPktWorld_t;

/**
 * @brief TODO doc
 *
 */
typedef struct //__attribute__((packed))
{
    uint8_t type;
    uint8_t numObstacles;
    struct
    {
        uint16_t x;
        uint16_t y;
        uint8_t r;
    } obstacles[MAX_NUM_OBSTACLES];

    uint16_t terrainPoints[NUM_TERRAIN_POINTS_B];
} artPktTerrain_t;

/**
 * @brief TODO doc
 * This doesn't use structs for better byte packing
 */
typedef struct
{
    uint8_t type;
    uint8_t padding;
    uint16_t x[NUM_CLOUDS * CIRC_PER_CLOUD];
    uint16_t y[NUM_CLOUDS * CIRC_PER_CLOUD];
    uint8_t r[NUM_CLOUDS * CIRC_PER_CLOUD];
} artPktCloud_t;

/**
 * @brief TODO doc
 *
 */
typedef struct
{
    uint8_t type;
} artPktFinishTour_t;

/**
 * @brief TODO doc
 *
 */
typedef struct // __attribute__((packed))
{
    uint8_t type;
    bool looking;
    int32_t moveTimeLeftUs;
    vec_t camera;
    struct
    {
        vecFl_t pos;
        int16_t barrelAngle;
        int32_t score;
    } players[NUM_PLAYERS];
} artPktState_t;

/**
 * @brief TODO doc
 *
 */
typedef struct // __attribute__((packed))
{
    uint8_t type;
    uint8_t ammoIdx;
    int16_t barrelAngle;
    float shotPower;
} artPktShot_t;

//==============================================================================
// Const Variables
//==============================================================================

const char str_conStarted[]     = "Searching for Swadge";
const char str_conRxAck[]       = "RX Game Start Ack";
const char str_conRxMsg[]       = "RX Game Start Msg";
const char str_conEstablished[] = "Connection Ready";
const char str_conLost[]        = "Connection Lost";

//==============================================================================
// Function Declarations
//==============================================================================

static uint8_t getSizeFromType(artilleryP2pPacketType_t type);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Receive connection statues for p2p
 *
 * TODO update strings
 * TODO more when CON_LOST is received
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
            ad->conStr   = str_conEstablished;
            ad->gameType = AG_WIRELESS;

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
            ad->mState = AMS_MENU;
            break;
        }
    }
}

/**
 * @brief Receive and process a p2p packet. If the packet is not expected in the current state, it is discarded
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
void artillery_p2pMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // ESP_LOGI("VT", "RX type %d, len %d", payload[0], len);

    artilleryData_t* ad = getArtilleryData();

    switch (payload[0])
    {
        case P2P_SET_COLOR:
        {
            if (P2P_SET_COLOR != ad->expectedPacket)
            {
                return;
            }
            else
            {
                if (GOING_FIRST == p2pGetPlayOrder(&ad->p2p))
                {
                    ad->expectedPacket = P2P_SET_STATE;
                }
                else
                {
                    ad->expectedPacket = P2P_SET_WORLD;
                }

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
            }
            return;
        }
        case P2P_SET_WORLD:
        {
            if (P2P_SET_WORLD != ad->expectedPacket)
            {
                return;
            }
            else
            {
                ad->expectedPacket = P2P_ADD_TERRAIN;

                const artPktWorld_t* pkt = (const artPktWorld_t*)payload;

                // Remove everything before receiving
                physRemoveAllObjects(ad->phys);
                // Immediately add world bounds, not in the transmitted packet
                physAddWorldBounds(ad->phys);

                // Add lines from packet
                physAddTerrainPoints(ad->phys, 0, pkt->terrainPoints, NUM_TERRAIN_POINTS_A);

                // Set the color indices
                uint8_t colorIndices[2] = {0};
                colorIndices[0]         = ad->myColorIdx;
                colorIndices[1]         = ad->theirColorIdx;

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

                // Play the music from the packet
                ad->bgmIdx = pkt->bgmIdx;
                globalMidiPlayerPlaySong(&ad->bgms[ad->bgmIdx], MIDI_BGM);
                globalMidiPlayerGet(MIDI_BGM)->loop = true;

                // Mark as not ready until the other packet is received
                ad->phys->isReady = false;
            }
            return;
        }
        case P2P_ADD_TERRAIN:
        {
            if (P2P_ADD_TERRAIN != ad->expectedPacket)
            {
                return;
            }
            else
            {
                ad->expectedPacket = P2P_SET_CLOUDS;

                const artPktTerrain_t* pkt = (const artPktTerrain_t*)payload;

                // Add more lines from packet
                physAddTerrainPoints(ad->phys, NUM_TERRAIN_POINTS_A - 1, pkt->terrainPoints, NUM_TERRAIN_POINTS_B);

                // Now that terrain is received, create BSP zones
                createBspZones(ad->phys);
            }
            return;
        }
        case P2P_SET_CLOUDS:
        {
            if (P2P_SET_CLOUDS != ad->expectedPacket)
            {
                return;
            }
            else
            {
                ad->expectedPacket = P2P_SET_STATE;

                const artPktCloud_t* pkt = (const artPktCloud_t*)payload;
                // Add clouds from packet
                for (uint16_t c = 0; c < NUM_CLOUDS * CIRC_PER_CLOUD; c++)
                {
                    ad->phys->clouds[c].pos.x  = pkt->x[c];
                    ad->phys->clouds[c].pos.y  = pkt->y[c];
                    ad->phys->clouds[c].radius = pkt->r[c];
                }

                // Mark simulation as ready
                ad->phys->isReady = true;

                // After receiving clouds, go on a tour
                artillerySwitchToGameState(ad, AGS_TOUR);
            }
            return;
        }
        case P2P_FINISH_TOUR:
        {
            // P2P_FINISH_TOUR can also be received while expecting P2P_SET_STATE
            if (P2P_SET_STATE != ad->expectedPacket)
            {
                return;
            }
            else if (AGS_TOUR == ad->gState)
            {
                artilleryFinishTour(ad);
            }
            return;
        }
        case P2P_SET_STATE:
        {
            if (P2P_SET_STATE != ad->expectedPacket && !artilleryIsMyTurn(ad))
            {
                return;
            }
            else
            {
                const artPktState_t* pkt = (const artPktState_t*)payload;

                // Update player location and barrel angle
                for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
                {
                    ad->players[pIdx]->c.pos = pkt->players[pIdx].pos;
                    setBarrelAngle(ad->players[pIdx], pkt->players[pIdx].barrelAngle);
                    ad->players[pIdx]->score = pkt->players[pIdx].score;
                }

                // Update gas gauge
                ad->moveTimerUs = pkt->moveTimeLeftUs;
                // Clear all camera targets
                clear(&ad->phys->cameraTargets);

                // If not looking around, focus on the player
                if (!pkt->looking)
                {
                    push(&ad->phys->cameraTargets, ad->players[ad->plIdx]);
                }
                else
                {
                    // Set the camera
                    ad->phys->camera = pkt->camera;
                }
            }
            return;
        }
        case P2P_FIRE_SHOT:
        {
            // P2P_FIRE_SHOT can also be received while expecting P2P_SET_STATE
            if (P2P_SET_STATE != ad->expectedPacket && !artilleryIsMyTurn(ad))
            {
                return;
            }
            else
            {
                if (!ad->phys->shotFired)
                {
                    const artPktShot_t* pkt = (const artPktShot_t*)payload;

                    // Get player references
                    physCirc_t* player   = ad->players[ad->plIdx];
                    physCirc_t* opponent = ad->players[(ad->plIdx + 1) % NUM_PLAYERS];

                    // Set the player's shot
                    player->barrelAngle = pkt->barrelAngle;
                    player->ammoIdx     = pkt->ammoIdx;
                    player->shotPower   = pkt->shotPower;

                    // Switch the game to firing
                    artillerySwitchToGameState(ad, AGS_FIRE);

                    // Fire the shot from here so that artilleryTxShot() isn't called from artilleryGameLoop()
                    ad->phys->shotFired = true;
                    fireShot(ad->phys, player, opponent, true);
                }
            }
            return;
        }
    }
}

/**
 * @brief Receive a TX status for a transmitted message.
 * Start playing music after P2P_SET_WORLD is ACKed.
 *
 * TODO something if MSG_FAILED is received?
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
void artillery_p2pMsgTxCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    if (MSG_ACKED == status)
    {
        artilleryData_t* ad = getArtilleryData();
        switch (ad->lastTxType)
        {
            case P2P_SET_WORLD:
            {
                // Start playing music
                globalMidiPlayerPlaySong(&ad->bgms[ad->bgmIdx], MIDI_BGM);
                globalMidiPlayerGet(MIDI_BGM)->loop = true;
                break;
            }
            case P2P_SET_COLOR:
            case P2P_ADD_TERRAIN:
            case P2P_SET_CLOUDS:
            case P2P_FINISH_TOUR:
            case P2P_SET_STATE:
            case P2P_FIRE_SHOT:
            {
                break;
            }
        }
    }
}

/**
 * @brief Enqueue the P2P_SET_COLOR packet to transmit.
 * This packet will only be sent once.
 *
 * @param ad All the artillery mode data
 */
void artilleryTxColor(artilleryData_t* ad)
{
    if (AG_WIRELESS != ad->gameType)
    {
        return;
    }

    // Allocate a packet
    artPktColor_t* pkt = heap_caps_calloc(1, sizeof(artPktShot_t), MALLOC_CAP_8BIT);

    // Write the type
    pkt->type = P2P_SET_COLOR;

    // Write the data
    pkt->colorIdx = ad->myColorIdx;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief Enqueue the P2P_SET_WORLD, P2P_ADD_TERRAIN, and P2P_SET_CLOUDS packets to transmit
 * the entire initial game state from one Swadge to another.
 * These packets will only be sent once
 *
 * @param ad All the artillery mode data
 */
void artilleryTxWorld(artilleryData_t* ad)
{
    if (AG_WIRELESS != ad->gameType)
    {
        return;
    }

    artPktWorld_t* pkt1          = heap_caps_calloc(1, sizeof(artPktWorld_t), MALLOC_CAP_8BIT);
    pkt1->type                   = P2P_SET_WORLD;
    pkt1->width                  = ad->phys->bounds.x;
    pkt1->height                 = ad->phys->bounds.y;
    pkt1->players[0].pos         = ad->players[0]->c.pos;
    pkt1->players[0].barrelAngle = ad->players[0]->barrelAngle;
    pkt1->players[0].score       = ad->players[0]->score;
    pkt1->players[1].pos         = ad->players[1]->c.pos;
    pkt1->players[1].barrelAngle = ad->players[1]->barrelAngle;
    pkt1->players[1].score       = ad->players[1]->score;
    pkt1->bgmIdx                 = ad->bgmIdx;

    artPktTerrain_t* pkt2 = heap_caps_calloc(1, sizeof(artPktTerrain_t), MALLOC_CAP_8BIT);
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

    // Build the cloud packet
    artPktCloud_t* pktCloud = heap_caps_calloc(1, sizeof(artPktCloud_t), MALLOC_CAP_8BIT);
    pktCloud->type          = P2P_SET_CLOUDS;
    for (uint16_t c = 0; c < NUM_CLOUDS * CIRC_PER_CLOUD; c++)
    {
        pktCloud->x[c] = ad->phys->clouds[c].pos.x;
        pktCloud->y[c] = ad->phys->clouds[c].pos.y;
        pktCloud->r[c] = ad->phys->clouds[c].radius;
    }

    // Enqueue packets for transmission
    push(&ad->p2pQueue, pkt1);
    push(&ad->p2pQueue, pkt2);
    push(&ad->p2pQueue, pktCloud);

    // After queueing world to send, go on a tour
    artillerySwitchToGameState(ad, AGS_TOUR);
}

/**
 * @brief Enqueue the P2P_FINISH_TOUR message to finish the tour early.
 * This may not be used at all, but if it is, it is only sent once.
 *
 * @param ad All the artillery mode data
 */
void artilleryTxFinishTour(artilleryData_t* ad)
{
    if (AG_WIRELESS != ad->gameType)
    {
        return;
    }

    // Allocate a packet
    artPktShot_t* pkt = heap_caps_calloc(1, sizeof(artPktFinishTour_t), MALLOC_CAP_8BIT);

    // Write the type
    pkt->type = P2P_FINISH_TOUR;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief Enqueue the P2P_SET_STATE message for transmission (camera, players, move timer).
 * This may be sent multiple times per turn.
 * This will remove prior enqueued P2P_SET_STATE messages so that only the latest state is transmitted.
 *
 * @param ad All the artillery mode data
 */
void artilleryTxState(artilleryData_t* ad)
{
    if (AG_WIRELESS != ad->gameType)
    {
        return;
    }

    // Remove any other enqueued packet of this type first
    node_t* pktNode = ad->p2pQueue.first;
    while (pktNode)
    {
        node_t* pktNodeNext           = pktNode->next;
        artilleryP2pPacketType_t type = *((uint8_t*)pktNode->val);
        if (P2P_SET_STATE == type)
        {
            heap_caps_free(removeEntry(&ad->p2pQueue, pktNode));
        }
        pktNode = pktNodeNext;
    }

    // Allocate a packet
    artPktState_t* pkt = heap_caps_calloc(1, sizeof(artPktState_t), MALLOC_CAP_8BIT);

    // Write the type
    pkt->type = P2P_SET_STATE;

    // Write player location and barrel angle
    for (int32_t pIdx = 0; pIdx < ARRAY_SIZE(pkt->players); pIdx++)
    {
        pkt->players[pIdx].pos         = ad->players[pIdx]->c.pos;
        pkt->players[pIdx].barrelAngle = ad->players[pIdx]->barrelAngle;
        pkt->players[pIdx].score       = ad->players[pIdx]->score;
    }

    // Write gas gauge
    pkt->moveTimeLeftUs = ad->moveTimerUs;

    // Write the data
    pkt->camera  = ad->phys->camera;
    pkt->looking = (AGS_LOOK == ad->gState);

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief Enqueue the P2P_FIRE_SHOT message for transmission.
 * This will only be sent once per turn and the turn will pass after the shot has finished.
 *
 * @param ad All the artillery mode data
 * @param player The player that fired the shot
 */
void artilleryTxShot(artilleryData_t* ad, physCirc_t* player)
{
    if (AG_WIRELESS != ad->gameType)
    {
        return;
    }

    // Allocate a packet
    artPktShot_t* pkt = heap_caps_calloc(1, sizeof(artPktShot_t), MALLOC_CAP_8BIT);

    // Write the type
    pkt->type = P2P_FIRE_SHOT;

    // Write the data
    pkt->ammoIdx     = player->ammoIdx;
    pkt->barrelAngle = player->barrelAngle;
    pkt->shotPower   = player->shotPower;

    // Push into the queue
    push(&ad->p2pQueue, pkt);
}

/**
 * @brief Check the transmit queue for any p2p packets to send.
 * Packets are only sent when p2p is idle
 *
 * @param ad All the artillery mode data
 */
void artilleryCheckTxQueue(artilleryData_t* ad)
{
    // If there is a message to send, and transmission is idle
    if (ad->p2pQueue.first && p2pIsTxIdle(&ad->p2p))
    {
        uint8_t* payload = shift(&ad->p2pQueue);
        // ESP_LOGI("VT", "TX type %d, len %d", payload[0], getSizeFromType(payload[0]));

        // Send over p2p
        p2pSendMsg(&ad->p2p, payload, getSizeFromType(payload[0]), artillery_p2pMsgTxCb);
        ad->lastTxType = payload[0];
        // Free queued message
        heap_caps_free(payload);
    }
}

/**
 * @brief Get the size of a p2p packet from the type of the packet
 *
 * @param type The type of the packet
 * @return The size of a packet of the given type
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
        case P2P_SET_CLOUDS:
        {
            return sizeof(artPktCloud_t);
        }
        case P2P_FINISH_TOUR:
        {
            return sizeof(artPktFinishTour_t);
        }
        case P2P_SET_STATE:
        {
            return sizeof(artPktState_t);
        }
        case P2P_FIRE_SHOT:
        {
            return sizeof(artPktShot_t);
        }
    }
    return 0;
}

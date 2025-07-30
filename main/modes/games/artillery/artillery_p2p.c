#include "artillery_p2p.h"
#include "artillery.h"

const char str_conStarted[]     = "Connection Started";
const char str_conRxAck[]       = "Rx Game Start Ack";
const char str_conRxMsg[]       = "Rx Game Start Msg";
const char str_conEstablished[] = "Connection Established";
const char str_conLost[]        = "Connection Lost";

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
            if (GOING_FIRST == p2pGetPlayOrder(p2p))
            {
                artilleryTxState(ad);
            }
            ad->conStr = str_conEstablished;

            artilleryInitGame();
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
void artilleryTxState(artilleryData_t* ad)
{
// Push packets into the transmit queue
// The first byte is the length
#define TEST_PACKET_LEN 16
    for (int testIdx = 0; testIdx < TEST_PACKET_LEN - 1; testIdx++)
    {
        uint8_t* payload     = heap_caps_calloc(1 + TEST_PACKET_LEN, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
        payload[0]           = TEST_PACKET_LEN;
        payload[1 + testIdx] = testIdx;
        push(&ad->p2pQueue, payload);
    }
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
        // Remove from the queue
        uint8_t* payload = shift(&ad->p2pQueue);
        // Send over p2p
        p2pSendMsg(&ad->p2p, &payload[1], payload[0], artillery_p2pMsgTxCb);
        // Free queued message
        heap_caps_free(payload);
    }
}

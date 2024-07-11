//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <esp_random.h>
#include <esp_log.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "hdw-esp-now.h"
#include "p2pConnection.h"

//==============================================================================
// Defines
//==============================================================================

// The time we'll spend retrying messages
#define RETRY_TIME_US 3000000

// Time to wait between connection events and game rounds.
// Transmission can be 3s (see above), the round @ 12ms period is 3.636s
// (240 steps of rotation + (252/4) steps of decay) * 12ms
#define FAILURE_RESTART_US 8000000

// #define P2P_DEBUG
#ifdef P2P_DEBUG
static const char* P2P_TAG = "P2P";
    #define P2P_LOG(...) ESP_LOGI(P2P_TAG, ##__VA_ARGS__)
#else
    #define P2P_LOG(...)
#endif

//==============================================================================
// Function Prototypes
//==============================================================================

static void p2pConnectionTimeout(void* arg);
static void p2pTxAllRetriesTimeout(void* arg);
static void p2pTxRetryTimeout(void* arg);
static void p2pRestart(p2pInfo* p2p);
static void p2pRestartTmrCb(void* arg);
static void p2pStartRestartTimer(void* arg);
static void p2pProcConnectionEvt(p2pInfo* p2p, connectionEvt_t event);
static void p2pGameStartAckRecv(p2pInfo* p2p, const uint8_t* data, uint8_t dataLen);
static void p2pSendAckToMac(p2pInfo* p2p, const uint8_t* mac_addr);
static void p2pSendMsgEx(p2pInfo* p2p, uint8_t* msg, uint16_t len, bool shouldAck, p2pAckSuccessFn success,
                         p2pAckFailureFn failure);
static void p2pModeMsgSuccess(p2pInfo* p2p, const uint8_t* data, uint8_t dataLen);
static void p2pModeMsgFailure(p2pInfo* p2p);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the p2p connection protocol
 *
 * @param p2p           The p2pInfo struct with all the state information
 * @param modeId        8 bit mode ID. Must be unique per-swadge mode.
 * @param conCbFn A function pointer which will be called when connection
 *                      events occur
 * @param msgRxCbFn A function pointer which will be called when a packet
 *                      is received for the swadge mode
 * @param connectionRssi The strength needed to start a connection with another
 *                      swadge. A positive value means swadges are quite close.
 *                      I've seen RSSI go as low as -70. 0 is practically touching
 */
void p2pInitialize(p2pInfo* p2p, uint8_t modeId, p2pConCbFn conCbFn, p2pMsgRxCbFn msgRxCbFn, int8_t connectionRssi)
{
    P2P_LOG("%s", __func__);
    //  Make sure everything is zero!
    memset(p2p, 0, sizeof(p2pInfo));

    // Don't start processing anything yet
    p2p->cnc.isActive = false;

    // Set the callback functions for connection and message events
    p2p->conCbFn   = conCbFn;
    p2p->msgRxCbFn = msgRxCbFn;

    // Set the initial sequence number at 255 so that a 0 received is valid.
    p2p->cnc.lastSeqNum = 255;

    // Set the connection Rssi, the higher the value, the closer the swadges
    // need to be.
    p2p->connectionRssi = connectionRssi;

    // Set the three character message ID
    p2p->modeId         = modeId;
    p2p->incomingModeId = modeId;

    // Get and save the string form of our MAC address
    esp_wifi_get_mac(WIFI_IF_STA, p2p->cnc.myMac);

    // Set up the connection message
    p2p->conMsg.startByte   = P2P_START_BYTE;
    p2p->conMsg.modeId      = modeId;
    p2p->conMsg.messageType = P2P_MSG_CONNECT;

    // Set up dummy ACK message
    p2p->ackMsg.hdr.startByte   = P2P_START_BYTE;
    p2p->ackMsg.hdr.modeId      = modeId;
    p2p->ackMsg.hdr.messageType = P2P_MSG_ACK;
    p2p->ackMsg.hdr.seqNum      = 0;
    memset(p2p->ackMsg.hdr.macAddr, 0xFF, sizeof(p2p->ackMsg.hdr.macAddr));

    // Set up dummy start message
    p2p->startMsg.startByte   = P2P_START_BYTE;
    p2p->startMsg.modeId      = modeId;
    p2p->startMsg.messageType = P2P_MSG_START;
    p2p->startMsg.seqNum      = 0;
    memset(p2p->startMsg.macAddr, 0xFF, sizeof(p2p->startMsg.macAddr));

    // Set up a timer for acknowledging messages
    esp_timer_create_args_t p2pTxRetryTimeoutArgs = {
        .callback              = p2pTxRetryTimeout,
        .arg                   = p2p,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "p2pt_rt",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&p2pTxRetryTimeoutArgs, &p2p->tmr.TxRetry);

    // Set up a timer for when a message never gets ACKed
    esp_timer_create_args_t p2pTxAllRetriesTimeoutArgs = {
        .callback              = p2pTxAllRetriesTimeout,
        .arg                   = p2p,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "p2pt_art",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&p2pTxAllRetriesTimeoutArgs, &p2p->tmr.TxAllRetries);

    // Set up a timer to restart after abject failure
    esp_timer_create_args_t p2pRestartArgs = {
        .callback              = p2pRestartTmrCb,
        .arg                   = p2p,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "p2pt_r",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&p2pRestartArgs, &p2p->tmr.Reinit);

    // Set up a timer to do an initial connection
    esp_timer_create_args_t p2pConnectionTimeoutArgs = {
        .callback              = p2pConnectionTimeout,
        .arg                   = p2p,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "p2pt_ct",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&p2pConnectionTimeoutArgs, &p2p->tmr.Connection);
}

/**
 * @brief Set the p2p connection protocol to listen for a different mode ID from its own
 *
 * @param p2p The p2pInfo struct with all the state information
 * @param incomingModeId The unique mode ID used by the other end of this connection
 */
void p2pSetAsymmetric(p2pInfo* p2p, uint8_t incomingModeId)
{
    p2p->incomingModeId = incomingModeId;
}

/**
 * @brief Start the connection process by sending broadcasts and notify the mode
 *
 * @param p2p The p2pInfo struct with all the state information
 */
void p2pStartConnection(p2pInfo* p2p)
{
    P2P_LOG("%s", __func__);

    p2p->cnc.isActive  = true;
    p2p->cnc.playOrder = NOT_SET;

    esp_timer_start_once(p2p->tmr.Connection, 1000);

    if (NULL != p2p->conCbFn)
    {
        p2p->conCbFn(p2p, CON_STARTED);
    }
}

/**
 * @brief Stop up all timers
 *
 * @param p2p The p2pInfo struct with all the state information
 */
void p2pDeinit(p2pInfo* p2p)
{
    P2P_LOG("%s", __func__);

    if (NULL != p2p->tmr.Connection)
    {
        esp_timer_stop(p2p->tmr.Connection);
        esp_timer_stop(p2p->tmr.TxRetry);
        esp_timer_stop(p2p->tmr.Reinit);
        esp_timer_stop(p2p->tmr.TxAllRetries);
    }
}

/**
 * @brief Send a broadcast connection message
 *
 * Called periodically, with some randomness mixed in from the tmr.Connection
 * timer. The timer is set when connection starts and is stopped when we
 * receive a response to our connection broadcast
 *
 * @param arg The p2pInfo struct with all the state information
 */
static void p2pConnectionTimeout(void* arg)
{
    P2P_LOG("%s", __func__);

    p2pInfo* p2p = (p2pInfo*)arg;
    // Send a connection broadcast
    p2pSendMsgEx(p2p, (uint8_t*)&p2p->conMsg, sizeof(p2p->conMsg), false, NULL, NULL);

    // esp_random returns a 32 bit number, so this is [500ms,1500ms]
    uint32_t timeoutUs = 1000 * (100 * (5 + (esp_random() % 11)));

    // Start the timer again
    P2P_LOG("retry broadcast in %" PRIu32 "us", timeoutUs);
    esp_timer_start_once(p2p->tmr.Connection, timeoutUs);
}

/**
 * @brief Retries sending a message to be acked
 *
 * Called from the tmr.TxRetry timer. The timer is set when a message to be
 * ACKed is sent and cleared when an ACK is received
 *
 * @param arg The p2pInfo struct with all the state information
 */
static void p2pTxRetryTimeout(void* arg)
{
    P2P_LOG("%s", __func__);

    p2pInfo* p2p = (p2pInfo*)arg;

    if (p2p->ack.msgToAckLen > 0)
    {
        P2P_LOG("Retrying message");
        p2pSendMsgEx(p2p, (uint8_t*)&p2p->ack.msgToAck, p2p->ack.msgToAckLen, true, p2p->ack.SuccessFn,
                     p2p->ack.FailureFn);
    }
}

/**
 * @brief Stops a message transmission attempt after all retries have been exhausted
 * and calls p2p->ack.FailureFn() if a function was given
 *
 * Called from the tmr.TxAllRetries timer. The timer is set when a message to
 * be ACKed is sent for the first time and cleared when the message is ACKed.
 *
 * @param arg The p2pInfo struct with all the state information
 */
static void p2pTxAllRetriesTimeout(void* arg)
{
    P2P_LOG("%s", __func__);

    p2pInfo* p2p = (p2pInfo*)arg;

    // Disarm all timers
    esp_timer_stop(p2p->tmr.TxRetry);
    esp_timer_stop(p2p->tmr.TxAllRetries);

    // Call the failure function
    P2P_LOG("Message totally failed");
    if (NULL != p2p->ack.FailureFn)
    {
        p2p->ack.FailureFn(p2p);
    }

    // Clear out the ack variables
    memset(&p2p->ack, 0, sizeof(p2p->ack));
}

/**
 * @brief Send a message from one Swadge to another. This must not be called before
 * the CON_ESTABLISHED event occurs. Message addressing, ACKing, and retries
 * all happen automatically
 *
 * @param p2p       The p2pInfo struct with all the state information
 * @param payload   A byte array to be copied to the payload for this message
 * @param len       The length of the byte array
 * @param msgTxCbFn A callback function when this message is ACKed or dropped
 */
void p2pSendMsg(p2pInfo* p2p, const uint8_t* payload, uint16_t len, p2pMsgTxCbFn msgTxCbFn)
{
    P2P_LOG("%s", __func__);

    p2pDataMsg_t builtMsg = {0};
    uint8_t builtMsgLen   = sizeof(p2pCommonHeader_t);

    // Build the header
    builtMsg.hdr.startByte   = P2P_START_BYTE;
    builtMsg.hdr.modeId      = p2p->modeId;
    builtMsg.hdr.messageType = P2P_MSG_DATA;
    builtMsg.hdr.seqNum      = 0;
    memcpy(builtMsg.hdr.macAddr, p2p->cnc.otherMac, sizeof(builtMsg.hdr.macAddr));

    // Copy the payload if it exists and fits
    if (NULL != payload && len != 0 && len < P2P_MAX_DATA_LEN)
    {
        memcpy(builtMsg.data, payload, len);
        builtMsgLen += len;
    }

    // Send it
    p2p->msgTxCbFn = msgTxCbFn;
    p2pSendMsgEx(p2p, (uint8_t*)&builtMsg, builtMsgLen, true, p2pModeMsgSuccess, p2pModeMsgFailure);
}

/**
 * @brief Callback function for when a message sent by the Swadge mode, not during
 * the connection process, is ACKed
 *
 * @param p2p The p2pInfo struct with all the state information
 * @param data The data that was acknowledged
 * @param dataLen The length of the data
 */
static void p2pModeMsgSuccess(p2pInfo* p2p, const uint8_t* data, uint8_t dataLen)
{
    P2P_LOG("%s", __func__);

    if (NULL != p2p->msgTxCbFn)
    {
        p2p->msgTxCbFn(p2p, MSG_ACKED, data, dataLen);
    }
}

/**
 * @brief Callback function for when a message sent by the Swadge mode, not during
 * the connection process, is dropped
 *
 * @param p2p The p2pInfo struct with all the state information
 */
static void p2pModeMsgFailure(p2pInfo* p2p)
{
    P2P_LOG("%s", __func__);

    if (NULL != p2p->msgTxCbFn)
    {
        p2p->msgTxCbFn(p2p, MSG_FAILED, NULL, 0);
    }
}

/**
 * @brief Wrapper for sending an ESP-NOW message. Handles ACKing and retries for
 * non-broadcast style messages
 *
 * @param p2p       The p2pInfo struct with all the state information
 * @param msg       The message to send, may contain destination MAC
 * @param len       The length of the message to send
 * @param shouldAck true if this message should be acked, false if we don't care
 * @param success   A callback function if the message is acked. May be NULL
 * @param failure   A callback function if the message isn't acked. May be NULL
 */
static void p2pSendMsgEx(p2pInfo* p2p, uint8_t* msg, uint16_t len, bool shouldAck, p2pAckSuccessFn success,
                         p2pAckFailureFn failure)
{
#ifdef P2P_DEBUG
    char dbgStr[12 + 2 + 2 + (len * 3)];
    sprintf(dbgStr, "%12s: ", __func__);
    for (int i = 0; i < len; i++)
    {
        char tmp[8];
        sprintf(tmp, "%02X ", msg[i]);
        strcat(dbgStr, tmp);
    }
    P2P_LOG("%s", dbgStr);
#endif

    // If this is a first time message and longer than a connection message
    if (((void*)&(p2p->ack.msgToAck) != (void*)msg) && sizeof(p2pConMsg_t) < len)
    {
        // Insert a sequence number
        ((p2pCommonHeader_t*)msg)->seqNum = p2p->cnc.mySeqNum;

        // Increment the sequence number, 0-255, will wraparound naturally
        p2p->cnc.mySeqNum++;
    }

    if (shouldAck)
    {
        // Set the state to wait for an ack
        p2p->ack.isWaitingForAck = true;

        // If this is not a retry
        if ((void*)&(p2p->ack.msgToAck) != (void*)msg)
        {
            P2P_LOG("sending for the first time");

            // Store the message for potential retries
            memcpy(&(p2p->ack.msgToAck), msg, len);
            p2p->ack.msgToAckLen = len;
            p2p->ack.SuccessFn   = success;
            p2p->ack.FailureFn   = failure;

            // Start a timer to retry for 3s total
            esp_timer_stop(p2p->tmr.TxAllRetries);
            esp_timer_start_once(p2p->tmr.TxAllRetries, RETRY_TIME_US);
        }
        else
        {
            P2P_LOG("this is a retry");
        }

        // Mark the time this transmission started, the retry timer gets
        // started in p2pSendCb()
        p2p->ack.timeSentUs = esp_timer_get_time();
    }
    espNowSend((const char*)msg, len);
}

/**
 * @brief This function must be called whenever an ESP NOW packet is received
 *
 * @param p2p      The p2pInfo struct with all the state information
 * @param mac_addr The MAC of the swadge that sent the data
 * @param data     The data
 * @param len      The length of the data
 * @param rssi     The rssi of the received data
 */
void p2pRecvCb(p2pInfo* p2p, const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Don't process anything if we're not connecting or connected
    if (false == p2p->cnc.isActive)
    {
        return;
    }

#ifdef P2P_DEBUG
    char dbgStr[64 + (len * 3)];
    sprintf(dbgStr, "%12s - From %02X:%02X:%02X:%02X:%02X:%02X - ", __func__, mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);
    for (int i = 0; i < len; i++)
    {
        char tmp[8];
        sprintf(tmp, "%02X ", data[i]);
        strcat(dbgStr, tmp);
    }
    P2P_LOG("%s", dbgStr);
#endif

    // Check if this message matches our message ID
    if (len < sizeof(p2pConMsg_t) || ((const p2pConMsg_t*)data)->startByte != P2P_START_BYTE
        || ((const p2pConMsg_t*)data)->modeId != p2p->incomingModeId)
    {
        // This message is too short, or does not match our message ID
        P2P_LOG("DISCARD: Not a message for '%c'", p2p->modeId);
        return;
    }

    // Make a pointer for convenience
    const p2pCommonHeader_t* p2pHdr = (const p2pCommonHeader_t*)data;

    // If this message has a MAC, check it
    if (len >= sizeof(p2pCommonHeader_t) && 0 != memcmp(p2pHdr->macAddr, p2p->cnc.myMac, sizeof(p2p->cnc.myMac)))
    {
#ifdef P2P_DEBUG
        // This MAC isn't for us
        P2P_LOG("DISCARD: Not for our MAC.");
        P2P_LOG("         Our MAC: %02X:%02X:%02X:%02X:%02X:%02X", p2p->cnc.myMac[0], p2p->cnc.myMac[1],
                p2p->cnc.myMac[2], p2p->cnc.myMac[3], p2p->cnc.myMac[4], p2p->cnc.myMac[5]);
        const p2pCommonHeader_t* hdr = (const p2pCommonHeader_t*)data;
        P2P_LOG("         rx  MAC: %02X:%02X:%02X:%02X:%02X:%02X", hdr->macAddr[0], hdr->macAddr[1], hdr->macAddr[2],
                hdr->macAddr[3], hdr->macAddr[4], hdr->macAddr[5]);
#endif
        return;
    }

    // If this is anything besides a broadcast, check the other MAC
    if (p2p->cnc.otherMacReceived && len >= sizeof(p2pCommonHeader_t)
        && 0 != memcmp(mac_addr, p2p->cnc.otherMac, sizeof(p2p->cnc.otherMac)))
    {
        // This isn't from the other known swadge
        P2P_LOG("DISCARD: Not from the other MAC");
        return;
    }

    // By here, we know the received message matches our message ID, either a
    // broadcast or for us. If this isn't an ack message, ack it
    if (len >= sizeof(p2pCommonHeader_t) && p2pHdr->messageType != P2P_MSG_ACK
        && p2pHdr->messageType != P2P_MSG_DATA_ACK)
    {
        p2pSendAckToMac(p2p, mac_addr);
    }

    // After ACKing the message, check the sequence number to see if we should
    // process it or ignore it (we already did!)
    if (len >= sizeof(p2pCommonHeader_t))
    {
        // Check it against the last known sequence number
        if (p2pHdr->seqNum == p2p->cnc.lastSeqNum)
        {
            P2P_LOG("DISCARD: Duplicate sequence number");
            return;
        }
        else
        {
            p2p->cnc.lastSeqNum = p2pHdr->seqNum;
            P2P_LOG("Store lastSeqNum %" PRIu8, p2p->cnc.lastSeqNum);
        }
    }

    // Check if this is an ACK
    bool isAck = (P2P_MSG_ACK == p2pHdr->messageType) || (P2P_MSG_DATA_ACK == p2pHdr->messageType);

    // ACKs can be received in any state
    if (p2p->ack.isWaitingForAck && isAck)
    {
        P2P_LOG("ACK Received when waiting for one");

        // Save the function pointer to call it after clearing ACK vars
        p2pAckSuccessFn tmpSuccessFn = p2p->ack.SuccessFn;

        // Clear ack timeout variables
        esp_timer_stop(p2p->tmr.TxRetry);
        // Disarm the whole transmission ack timer
        esp_timer_stop(p2p->tmr.TxAllRetries);
        // Clear out ACK variables
        memset(&p2p->ack, 0, sizeof(p2p->ack));

        p2p->ack.isWaitingForAck = false;

        // Call the callback after clearing out variables
        if (NULL != tmpSuccessFn)
        {
            if (P2P_MSG_DATA_ACK == p2pHdr->messageType)
            {
                tmpSuccessFn(p2p, ((const p2pDataMsg_t*)data)->data, len - sizeof(p2pCommonHeader_t));
            }
            else
            {
                tmpSuccessFn(p2p, NULL, 0);
            }
        }

        // Ack handled
        return;
    }
    else if (p2p->ack.isWaitingForAck || isAck)
    {
        // Don't process anything else when waiting for or receiving an ack
        return;
    }

    if (false == p2p->cnc.isConnected)
    {
        // Received another broadcast, Check if this RSSI is strong enough
        if (!p2p->cnc.broadcastReceived && rssi > p2p->connectionRssi && sizeof(p2pConMsg_t) == len
            && ((const p2pConMsg_t*)data)->messageType == p2p->conMsg.messageType
            && ((const p2pConMsg_t*)data)->modeId == p2p->incomingModeId
            && ((const p2pConMsg_t*)data)->startByte == p2p->conMsg.startByte)
        {
            // We received a broadcast, don't allow another
            p2p->cnc.broadcastReceived = true;

            // Save the other ESP's MAC
            memcpy(p2p->cnc.otherMac, mac_addr, sizeof(p2p->cnc.otherMac));
            p2p->cnc.otherMacReceived = true;

            // Send a message to that ESP to start the game.
            p2p->startMsg.startByte   = P2P_START_BYTE;
            p2p->startMsg.modeId      = p2p->modeId;
            p2p->startMsg.messageType = P2P_MSG_START;
            p2p->startMsg.seqNum      = 0;
            memcpy(p2p->startMsg.macAddr, mac_addr, sizeof(p2p->startMsg.macAddr));

            // If it's acked, call p2pGameStartAckRecv(), if not reinit with p2pRestart()
            p2pSendMsgEx(p2p, (uint8_t*)&p2p->startMsg, sizeof(p2p->startMsg), true, p2pGameStartAckRecv, p2pRestart);
        }
        // Received a response to our broadcast
        else if (!p2p->cnc.rxGameStartMsg && sizeof(p2p->startMsg) == len && p2pHdr->messageType == P2P_MSG_START)
        {
            P2P_LOG("Game start message received, ACKing");

            // This is another swadge trying to start a game, which means
            // they received our p2p->conMsg. First disable our p2p->conMsg
            esp_timer_stop(p2p->tmr.Connection);

            // And process this connection event
            p2pProcConnectionEvt(p2p, RX_GAME_START_MSG);
        }
        return;
    }
    else if (len >= sizeof(p2pCommonHeader_t))
    {
        P2P_LOG("cnc.isconnected is true");
        //  Let the mode handle it
        if (NULL != p2p->msgRxCbFn)
        {
            P2P_LOG("letting mode handle message");

            // Call the callback
            p2p->msgRxCbFn(p2p, ((const p2pDataMsg_t*)data)->data, len - sizeof(p2pCommonHeader_t));
        }
    }
}

/**
 * @brief Set data to be automatically used as the payload all future ACKs, until the
 * data is cleared.
 *
 * This data will not be transmitted until the next ACK is
 * sent, whenever that is. Normally an ACK does not contain any payload data,
 * but an application can have a more efficient continuous request and response
 * flow by embedding the response in the ACK.
 *
 * @param p2p The p2pInfo struct with all the state information
 * @param ackData The data to be included in the next ACK, will be copied here
 * @param ackDataLen The length of the data to be included in the next ACK
 */
void p2pSetDataInAck(p2pInfo* p2p, const uint8_t* ackData, uint8_t ackDataLen)
{
    // Make sure there aren't overflows
    if (ackDataLen > P2P_MAX_DATA_LEN)
    {
        ackDataLen = P2P_MAX_DATA_LEN;
    }

    // Set the message type to indicate data
    p2p->ackMsg.hdr.messageType = P2P_MSG_DATA_ACK;
    // Copy the data to the ackMsg, save the length
    memcpy(p2p->ackMsg.data, ackData, ackDataLen);
    p2p->dataInAckLen = ackDataLen;
}

/**
 * @brief Clear any data which was set to be used as the payload in the next ACK
 *
 * @param p2p The p2pInfo struct with all the state information
 */
void p2pClearDataInAck(p2pInfo* p2p)
{
    // Set the message type to indicate no data and clear the length
    p2p->ackMsg.hdr.messageType = P2P_MSG_ACK;
    p2p->dataInAckLen           = 0;
}

/**
 * @brief Helper function to send an ACK message to the given MAC
 *
 * @param p2p      The p2pInfo struct with all the state information
 * @param mac_addr The MAC to address this ACK to
 */
static void p2pSendAckToMac(p2pInfo* p2p, const uint8_t* mac_addr)
{
    P2P_LOG("%s", __func__);

    // Write the destination MAC address
    // Everything else should already be written
    memcpy(p2p->ackMsg.hdr.macAddr, mac_addr, sizeof(p2p->ackMsg.hdr.macAddr));
    // Send the ACK
    p2pSendMsgEx(p2p, (uint8_t*)&p2p->ackMsg, sizeof(p2pCommonHeader_t) + p2p->dataInAckLen, false, NULL, NULL);
}

/**
 * @brief This is called when p2p->startMsg is acked and processes the connection event
 *
 * @param p2p The p2pInfo struct with all the state information
 * @param data The start message which was acked
 * @param dataLen The length of the start message which was acked
 */
static void p2pGameStartAckRecv(p2pInfo* p2p, const uint8_t* data, uint8_t dataLen)
{
    P2P_LOG("%s", __func__);

    p2pProcConnectionEvt(p2p, RX_GAME_START_ACK);
}

/**
 * @brief This function processes connection events
 *
 * Two steps are necessary to establish a connection in no particular order.
 * 1. This swadge has to receive a start message from another swadge
 * 2. This swadge has to receive an ack to a start message sent to another swadge
 * The order of events determines who is the 'client' and who is the 'server'
 *
 * @param p2p   The p2pInfo struct with all the state information
 * @param event The event that occurred
 */
static void p2pProcConnectionEvt(p2pInfo* p2p, connectionEvt_t event)
{
    P2P_LOG("%s evt: %" PRId16 ", p2p->cnc.rxGameStartMsg %s, p2p->cnc.rxGameStartAck %s", __func__, event,
            p2p->cnc.rxGameStartMsg ? "true" : "false", p2p->cnc.rxGameStartAck ? "true" : "false");

    switch (event)
    {
        case RX_GAME_START_MSG:
        {
            // Already received the ack, become the client
            if (!p2p->cnc.rxGameStartMsg && p2p->cnc.rxGameStartAck)
            {
                p2p->cnc.playOrder = GOING_SECOND;
            }
            // Mark this event
            p2p->cnc.rxGameStartMsg = true;
            break;
        }
        case RX_GAME_START_ACK:
        {
            // Already received the msg, become the server
            if (!p2p->cnc.rxGameStartAck && p2p->cnc.rxGameStartMsg)
            {
                p2p->cnc.playOrder = GOING_FIRST;
            }
            // Mark this event
            p2p->cnc.rxGameStartAck = true;
            break;
        }
        case CON_STARTED:
        case CON_ESTABLISHED:
        case CON_LOST:
        default:
        {
            break;
        }
    }

    if (NULL != p2p->conCbFn)
    {
        p2p->conCbFn(p2p, event);
    }

    // If both the game start messages are good, start the game
    if (p2p->cnc.rxGameStartMsg && p2p->cnc.rxGameStartAck)
    {
        // Connection was successful, so disarm the failure timer
        esp_timer_stop(p2p->tmr.Reinit);

        p2p->cnc.isConnected = true;

        // tell the mode it's connected
        if (NULL != p2p->conCbFn)
        {
            p2p->conCbFn(p2p, CON_ESTABLISHED);
        }
    }
    else
    {
        // Start a timer to reinit if we never finish connection
        p2pStartRestartTimer(p2p);
    }
}

/**
 * @brief This starts a timer to call p2pRestart(), used in case of a failure
 *
 * The timer is set when one half of the necessary connection messages is received
 * The timer is disarmed when the connection is established
 *
 * @param arg The p2pInfo struct with all the state information
 */
static void p2pStartRestartTimer(void* arg)
{
    P2P_LOG("%s", __func__);

    p2pInfo* p2p = (p2pInfo*)arg;
    // If the connection isn't established in FAILURE_RESTART_MS, restart
    esp_timer_start_once(p2p->tmr.Reinit, FAILURE_RESTART_US);
}

/**
 * @brief Wrapper for p2pRestart() for a timer callback
 *
 * Restart by deinitializing then initializing. Persist the msgId and p2p->conCbFn fields
 *
 * @param arg The p2pInfo struct with all the state information
 */
static void p2pRestartTmrCb(void* arg)
{
    p2pRestart(arg);
}

/**
 * @brief Restart by deinitializing then initializing. Persist the msgId and p2p->conCbFn fields
 *
 * @param p2p The p2pInfo struct with all the state information
 */
static void p2pRestart(p2pInfo* p2p)
{
    P2P_LOG("%s", __func__);

    if (NULL != p2p->conCbFn)
    {
        p2p->conCbFn(p2p, CON_LOST);
    }

    uint8_t modeId         = p2p->modeId;
    uint8_t incomingModeId = p2p->incomingModeId;
    p2pDeinit(p2p);
    p2pInitialize(p2p, modeId, p2p->conCbFn, p2p->msgRxCbFn, p2p->connectionRssi);

    if (incomingModeId != modeId)
    {
        p2pSetAsymmetric(p2p, incomingModeId);
    }
}

/**
 * @brief This must be called by whatever function is registered to the Swadge mode's
 * fnEspNowSendCb
 *
 * This is called after an attempted transmission. If it was successful, and the
 * message should be acked, start a retry timer. If it wasn't successful, just
 * try again
 *
 * @param p2p      The p2pInfo struct with all the state information
 * @param mac_addr unused
 * @param status   Whether the transmission succeeded or failed
 */
void p2pSendCb(p2pInfo* p2p, const uint8_t* mac_addr __attribute__((unused)), esp_now_send_status_t status)
{
    P2P_LOG("%s - %s", __func__, status == ESP_NOW_SEND_SUCCESS ? "ESP_NOW_SEND_SUCCESS" : "ESP_NOW_SEND_FAIL");

    switch (status)
    {
        case ESP_NOW_SEND_SUCCESS:
        {
            if (0 != p2p->ack.timeSentUs)
            {
#ifdef P2P_DEBUG
                uint32_t transmissionTimeUs = esp_timer_get_time() - p2p->ack.timeSentUs;
                P2P_LOG("Transmission time %" PRIu32 "us", transmissionTimeUs);
#endif
                // //  The timers are all millisecond, so make sure that
                // //  transmissionTimeUs is at least 1ms
                // if (transmissionTimeUs < 1000)
                // {
                //     transmissionTimeUs = 1000;
                // }

                // Use a fixed retry of 5ms
                uint32_t waitTimeUs = 5000;

                // Round it to the nearest Ms, add 69ms (the measured worst case)
                // then add some randomness [0ms to 15ms random]
                // uint32_t waitTimeUs = 1000 * (((transmissionTimeUs + 500) / 1000) + 69 + (esp_random() & 0b1111));

                // Start the timer
                P2P_LOG("ack timer set for %" PRIu32 "us", waitTimeUs);
                esp_timer_start_once(p2p->tmr.TxRetry, waitTimeUs);
            }
            break;
        }
        default:
        case ESP_NOW_SEND_FAIL:
        {
            // If a message is stored
            if (p2p->ack.msgToAckLen > 0)
            {
                // try again in 1ms
                esp_timer_start_once(p2p->tmr.TxRetry, 1000);
            }
            break;
        }
    }
}

/**
 * @brief After the swadge is connected to another, return whether this Swadge is
 * player 1 or player 2. This can be used to determine client/server roles
 *
 * @param p2p The p2pInfo struct with all the state information
 * @return    ::GOING_SECOND, ::GOING_FIRST, or ::NOT_SET
 */
playOrder_t p2pGetPlayOrder(p2pInfo* p2p)
{
    if (p2p->cnc.isConnected)
    {
        return p2p->cnc.playOrder;
    }
    else
    {
        return NOT_SET;
    }
}

/**
 * @brief Override whether the Swadge is player 1 or player 2. You probably shouldn't
 * do this, but you might want to for single player modes
 *
 * @param p2p The p2pInfo struct with all the state information
 * @param order The order to set
 */
void p2pSetPlayOrder(p2pInfo* p2p, playOrder_t order)
{
    p2p->cnc.playOrder = order;
}

// clang-format off

/*! \file p2pConnection.h
 *
 * \section p2p_design Design Philosophy
 *
 * p2pConnection is a connection protocol with a little bit of message reliability that sits on top of ESP-NOW.
 * Think of ESP-NOW like UDP, where you can quickly and unreliably broadcast and receive packets, and p2pConnection as TCP, which trades some speed for reliability.
 * p2pConnection messages have sequence numbers for deduplication, are acknowledged, and are retried if not acknowledged.
 * 
 * Connections are made when two Swadges broadcast connection messages to each other, then send start messages to each other, and acknowledge each other's start message.
 * The play order is determined by who acknowledges the start message first, which is suitably random.
 * A connection sequence looks like this:
 *
 * @startuml
 * == Connection ==
 * 
 * group Part 1
 * "Swadge_AB:AB:AB:AB:AB:AB" ->  "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x00 {P2P_MSG_CONNECT}]"
 * "Swadge_12:12:12:12:12:12" ->  "Swadge_AB:AB:AB:AB:AB:AB" : "['p', {mode ID}, 0x01 {P2P_MSG_START}, 0x00 {seqNum}, (0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB)]"
 * note left: Stop Broadcasting, set p2p->cnc.rxGameStartMsg
 * "Swadge_AB:AB:AB:AB:AB:AB" ->  "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x02 {P2P_MSG_ACK}, 0x00 {seqNum}, (0x12, 0x12, 0x12, 0x12, 0x12, 0x12)]
 * note right: set p2p->cnc.rxGameStartAck
 * end
 * 
 * group Part 2
 * "Swadge_12:12:12:12:12:12" ->  "Swadge_AB:AB:AB:AB:AB:AB" : "['p', {mode ID}, 0x00 {P2P_MSG_CONNECT}]"
 * "Swadge_AB:AB:AB:AB:AB:AB" ->  "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x01 {P2P_MSG_START}, 0x01 {seqNum}, (0x12, 0x12, 0x12, 0x12, 0x12, 0x12)]"
 * note right: Stop Broadcasting, set p2p->cnc.rxGameStartMsg, become CLIENT
 * "Swadge_12:12:12:12:12:12" ->  "Swadge_AB:AB:AB:AB:AB:AB" : "['p', {mode ID}, 0x02 {P2P_MSG_ACK}, 0x01 {seqNum}, (0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB)]
 * note left: set p2p->cnc.rxGameStartAck, become SERVER
 * end
 * @enduml
 * 
 * After connection, Swadges are free to send messages to each other.
 * An example of unreliable communication with retries and duplication is as follows.
 * 
 * @startuml
 * == Unreliable Communication Example ==
 * 
 * group Retries & Sequence Numbers
 * "Swadge_AB:AB:AB:AB:AB:AB" ->x "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x03 {P2P_MSG_DATA}, 0x04 {seqNum}, (0x12, 0x12, 0x12, 0x12, 0x12, 0x12), 'd', 'a', 't', 'a']
 * note right: msg not received
 * "Swadge_AB:AB:AB:AB:AB:AB" ->  "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x03 {P2P_MSG_DATA}, 0x04 {seqNum}, (0x12, 0x12, 0x12, 0x12, 0x12, 0x12), 'd', 'a', 't', 'a']
 * note left: first retry, up to five retries
 * "Swadge_12:12:12:12:12:12" ->x "Swadge_AB:AB:AB:AB:AB:AB" : "['p', {mode ID}, 0x02 {P2P_MSG_ACK}, 0x04 {seqNum}, (0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB)]
 * note left: ack not received
 * "Swadge_AB:AB:AB:AB:AB:AB" ->  "Swadge_12:12:12:12:12:12" : "['p', {mode ID}, 0x03 {P2P_MSG_DATA}, 0x04 {seqNum}, (0x12, 0x12, 0x12, 0x12, 0x12, 0x12), 'd', 'a', 't', 'a']
 * note left: second retry
 * note right: duplicate seq num, ignore message
 * "Swadge_12:12:12:12:12:12" ->  "Swadge_AB:AB:AB:AB:AB:AB" : "['p', {mode ID}, 0x02 {P2P_MSG_ACK}, 0x05 {seqNum}, (0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB)]
 * end
 * @enduml
 *
 * \section p2p_usage Usage
 *
 * p2pSendCb() and p2pRecvCb() must be called from the ESP-NOW callbacks to pass data to and from p2p.
 * 
 * p2pInitialize() should be called to initialize p2p.
 * p2pDeinit() should be called when the Swadge mode is done to clean up.
 * 
 * The connection won't actually start until p2pStartConnection() is called.
 * Connection statues will be delivered to the Swadge mode through the 
 * 
 * p2pGetPlayOrder() can be called after connection to figure out of this Swadge is player one or two.
 * 
 * p2pSendMsg() can be called to send a message from one Swadge to another.
 * 
 * p2pSetDataInAck() can be called to set up data to be delivered in the next acknowledge message.
 * This is useful for high-bandwith transactional messages.
 * For example, one Swadge may send it's button state to the other, and receive the game state in the acknowledge message.
 * This turns four messages (button state, ack, game state, ack) into two messages (button state, ack[game state]).
 * Note that the acknowledge message with data is not acknowledged itself, but the Swadge sending the inital message will retry until it receives the acknowledge.
 * p2pClearDataInAck() can be called to clear the data to be sent in the acknowledge.
 *
 * \section p2p_example Example
 *
 * \code{.c}
 * static void demoEspNowRecvCb(const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi);
 * static void demoEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
 * static void demoConCb(p2pInfo* p2p, connectionEvt_t evt);
 * static void demoMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
 * static void demoMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
 * 
 * // Make sure the Swadge mode callbacks are set
 * swadgeMode_t demoMode = {
 *     .wifiMode       = ESP_NOW,
 *     .fnEspNowRecvCb = demoEspNowRecvCb,
 *     .fnEspNowSendCb = demoEspNowSendCb,
 *     ...
 * };
 * 
 * // Variable which contains all the state information
 * p2pInfo_t p2p;
 * 
 * ...
 * 
 * // Initialize and start connection
 * p2pInitialize(&p2p, 'd', demoConCb, demoMsgRxCb, -70);
 * p2pStartConnection(&p2p);
 * 
 * ...
 * 
 * // Send a message
 * const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
 * p2pSendMsg(&p2p, testMsg, ARRAY_SIZE(testMsg), demoMsgTxCbFn);
 * 
 * ...
 * 
 * static void demoEspNowRecvCb(const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi)
 * {
 *     p2pRecvCb(&p2p, mac_addr, data, len, rssi);
 * }
 * 
 * static void demoEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
 * {
 *     p2pSendCb(&p2p, mac_addr, status);
 * }
 * 
 * static void demoConCb(p2pInfo* p2p, connectionEvt_t evt)
 * {
 * 	// Do something when a connection event happens
 * }
 * 
 * static void demoMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
 * {
 * 	// Do something when a message is received
 * }
 * 
 * static void demoMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
 * {
 * 	// Do something when a message is acknowledged
 * }
 * \endcode
 */
// clang-format on

#ifndef _P2P_CONNECTION_H_
#define _P2P_CONNECTION_H_

#include <stdint.h>
#include <stdbool.h>

#include <esp_timer.h>

/// The maximum payload of a p2p packet is 245 bytes
#define P2P_MAX_DATA_LEN 245

/// After connecting, one Swadge will be ::GOING_FIRST and one will be ::GOING_SECOND
typedef enum
{
    NOT_SET,      ///< Swadges haven't connected yet
    GOING_SECOND, ///< This Swadge goes second (player two)
    GOING_FIRST   ///< This swadge goes first (player one)
} playOrder_t;

/// These are the states a Swadge will go through when connecting to another
typedef enum
{
    CON_STARTED,       ///< Connection has started
    RX_GAME_START_ACK, ///< This Swadge's start message has been ACKed
    RX_GAME_START_MSG, ///< Another Swadge's start message has been received
    CON_ESTABLISHED,   ///< Connection has been established
    CON_LOST           ///< Connection was lost
} connectionEvt_t;

/// Message statuses after transmission
typedef enum
{
    MSG_ACKED, ///< The message was acknowledged after transmission
    MSG_FAILED ///< The message transmission failed
} messageStatus_t;

typedef struct _p2pInfo p2pInfo;

/**
 * @brief This typedef is for the function callback which delivers connection statuses to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
typedef void (*p2pConCbFn)(p2pInfo* p2p, connectionEvt_t evt);

/**
 * @brief This typedef is for the function callback which delivers received p2p packets to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
typedef void (*p2pMsgRxCbFn)(p2pInfo* p2p, const uint8_t* payload, uint8_t len);

/**
 * @brief This typedef is for the function callback which delivers acknowledge status for transmitted messages to the
 * Swadge mode
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
typedef void (*p2pMsgTxCbFn)(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

/**
 * @brief This typedef is for a function callback called when a message is acknowledged
 *
 * @param p2p The p2pInfo
 * @param data The data which was acknowledged
 * @param len The length of the data which was acknowledged
 */
typedef void (*p2pAckSuccessFn)(p2pInfo* p2p, const uint8_t* data, uint8_t len);

/**
 * @brief This typedef is for a function callback called when a message is not acknowledged
 *
 * @param p2p The p2pInfo
 * @param data The data which was not acknowledged
 * @param len The length of the data which was not acknowledged
 */
typedef void (*p2pAckFailureFn)(p2pInfo*);

/// A start byte for all p2p packets
#define P2P_START_BYTE 'p'

typedef enum __attribute__((packed))
{
    P2P_MSG_CONNECT,  ///< The connection broadcast
    P2P_MSG_START,    ///< The start message, used during connection
    P2P_MSG_ACK,      ///< An acknowledge message
    P2P_MSG_DATA_ACK, ///< An acknowledge message with extra data
    P2P_MSG_DATA      ///< A data message
} p2pMsgType_t;

typedef struct
{
    uint8_t startByte;
    uint8_t modeId;
    p2pMsgType_t messageType;
} p2pConMsg_t;

typedef struct
{
    uint8_t startByte;
    uint8_t modeId;
    p2pMsgType_t messageType;
    uint8_t seqNum;
    uint8_t macAddr[6];
} p2pCommonHeader_t;

typedef struct
{
    p2pCommonHeader_t hdr;
    uint8_t data[P2P_MAX_DATA_LEN];
} p2pDataMsg_t;

// Variables to track acking messages
typedef struct _p2pInfo
{
    // Messages that every mode uses
    uint8_t modeId;
    uint8_t incomingModeId;
    p2pConMsg_t conMsg;
    p2pDataMsg_t ackMsg;
    p2pCommonHeader_t startMsg;

    // Callback function pointers
    p2pConCbFn conCbFn;
    p2pMsgRxCbFn msgRxCbFn;
    p2pMsgTxCbFn msgTxCbFn;

    int8_t connectionRssi;

    // Variables used for acking and retrying messages
    struct
    {
        bool isWaitingForAck;
        p2pDataMsg_t msgToAck;
        uint16_t msgToAckLen;
        uint32_t timeSentUs;
        p2pAckSuccessFn SuccessFn;
        p2pAckFailureFn FailureFn;
        uint8_t dataInAckLen;
    } ack;

    // Connection state variables
    struct
    {
        bool isActive;
        bool isConnected;
        bool broadcastReceived;
        bool rxGameStartMsg;
        bool rxGameStartAck;
        playOrder_t playOrder;
        uint8_t myMac[6];
        uint8_t otherMac[6];
        bool otherMacReceived;
        uint8_t mySeqNum;
        uint8_t lastSeqNum;
    } cnc;

    // The timers used for connection and acking
    struct
    {
        esp_timer_handle_t TxRetry;
        esp_timer_handle_t TxAllRetries;
        esp_timer_handle_t Connection;
        esp_timer_handle_t Reinit;
    } tmr;
} p2pInfo;

// All the information for a packet to store between the receive callback and
// the task it's actually processed in
typedef struct
{
    int8_t rssi;
    uint8_t mac[6];
    uint8_t len;
    p2pDataMsg_t data;
} p2pPacket_t;

void p2pInitialize(p2pInfo* p2p, uint8_t modeId, p2pConCbFn conCbFn, p2pMsgRxCbFn msgRxCbFn, int8_t connectionRssi);
void p2pSetAsymmetric(p2pInfo* p2p, uint8_t incomingModeId);
void p2pDeinit(p2pInfo* p2p);

void p2pStartConnection(p2pInfo* p2p);

void p2pSendMsg(p2pInfo* p2p, const uint8_t* payload, uint16_t len, p2pMsgTxCbFn msgTxCbFn);
void p2pSendCb(p2pInfo* p2p, const uint8_t* mac_addr, esp_now_send_status_t status);
void p2pRecvCb(p2pInfo* p2p, const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi);
void p2pSetDataInAck(p2pInfo* p2p, const uint8_t* ackData, uint8_t ackDataLen);
void p2pClearDataInAck(p2pInfo* p2p);

playOrder_t p2pGetPlayOrder(p2pInfo* p2p);
void p2pSetPlayOrder(p2pInfo* p2p, playOrder_t order);

#endif

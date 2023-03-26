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
 * @brief This typedef is for a function callback called when a message is acknowledged.
 * It make also contain a data packet which was appended to the ACK.
 *
 * @param p2p The p2pInfo
 * @param data A data payload returned along with the ACK
 * @param len The length of the data payload appended to the ACK
 */
typedef void (*p2pAckSuccessFn)(p2pInfo* p2p, const uint8_t* data, uint8_t len);

/**
 * @brief This typedef is for a function callback called when a message is not acknowledged
 *
 * @param p2p The p2pInfo
 */
typedef void (*p2pAckFailureFn)(p2pInfo* p2p);

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

/**
 * @brief The byte format for a connection message for a P2P session
 */
typedef struct
{
    uint8_t startByte;        ///< Start byte, must be ::P2P_START_BYTE
    uint8_t modeId;           ///< Mode byte, must be unique per-mode
    p2pMsgType_t messageType; ///< Message type byte
} p2pConMsg_t;

/**
 * @brief The byte format for a common header for all P2P packets
 */
typedef struct
{
    uint8_t startByte;        ///< Start byte, must be ::P2P_START_BYTE
    uint8_t modeId;           ///< Mode byte, must be unique per-mode
    p2pMsgType_t messageType; ///< Message type byte
    uint8_t seqNum;           ///< A sequence number for this packet
    uint8_t macAddr[6];       ///< The MAC address destination for this packet
} p2pCommonHeader_t;

/**
 * @brief The byte format for a P2P data packet
 */
typedef struct
{
    p2pCommonHeader_t hdr;          ///< The common header bytes for a P2P packet
    uint8_t data[P2P_MAX_DATA_LEN]; ///< The data bytes sent or received
} p2pDataMsg_t;

/**
 * @brief All the state variables required for a P2P session with another Swadge
 */
typedef struct _p2pInfo
{
    // Messages that every mode uses
    uint8_t modeId; ///< The mode ID set by the mode using P2P
    uint8_t
        incomingModeId; ///< A mode ID to listen for which is different than initialized mode ID. See p2pSetAsymmetric()
    p2pConMsg_t conMsg; ///< The connection message to transmit
    p2pCommonHeader_t startMsg; ///< The start message to transmit
    p2pDataMsg_t ackMsg;        ///< The acknowledge message to transmit
    uint8_t dataInAckLen;       ///< The length of any extra data which was appended to the ACK, see p2pSetDataInAck()

    // Callback function pointers
    p2pConCbFn conCbFn;     ///< A callback function called during the connection process
    p2pMsgRxCbFn msgRxCbFn; ///< A callback function called when receiving a message
    p2pMsgTxCbFn msgTxCbFn; ///< A callback function called when transmitting a message

    int8_t connectionRssi; ///< The minimum RSSI required to begin a connection

    /**
     * @brief Variables used for acknowledging and retrying messages
     */
    struct
    {
        bool isWaitingForAck;      ///< true if waiting for an ACK after transmitting a message
        p2pDataMsg_t msgToAck;     ///< A transmitted message which is waiting for an ACK
        uint16_t msgToAckLen;      ///< The length of the message which is waiting for an ACK
        uint32_t timeSentUs;       ///< The time the message is waiting for an ACK was transmitted
        p2pAckSuccessFn SuccessFn; ///< A callback function to be called if the message is ACKed
        p2pAckFailureFn FailureFn; ///< A callback function to be called if the message is not ACKed
    } ack;

    /**
     * @brief Connection state variables
     */
    struct
    {
        playOrder_t playOrder;  ///< Either ::GOING_FIRST or ::GOING_SECOND depending on how the handshake went
        uint8_t myMac[6];       ///< This Swadge's MAC address
        uint8_t otherMac[6];    ///< The other Swadge's MAC address
        bool isActive;          ///< true if the connection process has started
        bool isConnected;       ///< true if connected to another Swadge
        bool broadcastReceived; ///< true if a broadcast was received to start the connection handshake
        bool rxGameStartMsg;    ///< true if the other Swadge's game start message was received
        bool rxGameStartAck;    ///< True if this Swadge's game start message was acknowledged
        bool otherMacReceived;  ///< true if the other Swadge's MAC address has been received
        uint8_t mySeqNum;       ///< The current sequence number used for transmissions
        uint8_t lastSeqNum;     ///< The last sequence number used for transmissions
    } cnc;

    /**
     * @brief The timers used for connection and ACKing
     */
    struct
    {
        esp_timer_handle_t TxRetry;      ///< A timer used to retry a transmission multiple times if not acknowledged
        esp_timer_handle_t TxAllRetries; ///< A timer used to cancel a transmission if all attempts failed
        esp_timer_handle_t Connection;   ///< A timer used to cancel a connection if the handshake fails
        esp_timer_handle_t Reinit;       ///< A timer used to restart P2P after any complete failures
    } tmr;
} p2pInfo;

/**
 * @brief All the information for a packet to store between the receive callback and the task it's actually processed in
 */
typedef struct
{
    int8_t rssi;       ///< The received signal strength indicator for the packet
    uint8_t mac[6];    ///< The MAC address of the sender
    uint8_t len;       ///< The length of the received bytes
    p2pDataMsg_t data; ///< The received bytes
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

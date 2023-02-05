#ifndef _P2P_CONNECTION_H_
#define _P2P_CONNECTION_H_

#include <stdint.h>
#include <stdbool.h>

#define P2P_MAX_DATA_LEN 245

typedef enum
{
    NOT_SET,
    GOING_SECOND,
    GOING_FIRST
} playOrder_t;

typedef enum
{
    CON_STARTED,
    RX_GAME_START_ACK,
    RX_GAME_START_MSG,
    CON_ESTABLISHED,
    CON_LOST
} connectionEvt_t;

typedef enum
{
    MSG_ACKED,
    MSG_FAILED
} messageStatus_t;

typedef struct _p2pInfo p2pInfo;

typedef void (*p2pConCbFn)(p2pInfo* p2p, connectionEvt_t);
typedef void (*p2pMsgRxCbFn)(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
typedef void (*p2pMsgTxCbFn)(p2pInfo* p2p, messageStatus_t status, const uint8_t*, uint8_t);

typedef void (*p2pAckSuccessFn)(p2pInfo*, const uint8_t*, uint8_t);
typedef void (*p2pAckFailureFn)(p2pInfo*);

#define P2P_START_BYTE 'p'

typedef enum __attribute__((packed))
{
    P2P_MSG_CONNECT,
    P2P_MSG_START,
    P2P_MSG_ACK,
    P2P_MSG_DATA_ACK,
    P2P_MSG_DATA
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

#pragma once

#include "p2pConnection.h"
#include "artillery.h"

void artillery_p2pConCb(p2pInfo* p2p, connectionEvt_t evt);
void artillery_p2pMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
void artillery_p2pMsgTxCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

void artilleryTxColor(artilleryData_t* ad);
void artilleryTxWorld(artilleryData_t* ad);
void artilleryTxFinishTour(artilleryData_t* ad);
void artilleryTxState(artilleryData_t* ad);
void artilleryTxShot(artilleryData_t* ad, physCirc_t* player);
void artilleryCheckTxQueue(artilleryData_t* ad);

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTT.h"
#include "ultimateTTTgame.h"

//==============================================================================
// Function Declarations
//==============================================================================

void tttDrawConnecting(ultimateTTT_t* ttt, int64_t elapsedUs);
void tttHandleConnectingInput(ultimateTTT_t* ttt, buttonEvt_t* evt);

void tttHandleCon(ultimateTTT_t* ttt, connectionEvt_t evt);
void tttHandleMsgRx(ultimateTTT_t* ttt, const uint8_t* payload, uint8_t len);
void tttHandleMsgTx(ultimateTTT_t* ttt, messageStatus_t status, const uint8_t* data, uint8_t len);

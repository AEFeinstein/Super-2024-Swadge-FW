#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"

//==============================================================================
// Function Declarations
//==============================================================================

void dn_DrawConnecting(dn_gameData_t* gameData, int64_t elapsedUs);
void dn_HandleConnectingInput(dn_gameData_t* gameData, buttonEvt_t* evt);

void dn_HandleCon(dn_gameData_t* gameData, connectionEvt_t evt);
void dn_HandleMsgRx(dn_gameData_t* gameData, const uint8_t* payload, uint8_t len);
void dn_HandleMsgTx(dn_gameData_t* gameData, messageStatus_t status, const uint8_t* data, uint8_t len);

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "fairyCreationData.h"

//==============================================================================
// Functions declarations
//==============================================================================

void fcInitCreation(fcCreationData_t* fcdd, fairy_t* fairy, profileCard_t* card);

bool fcRunCreation(fcCreationData_t* fcdd);

void fcDrawCreation(fcCreationData_t* fcdd, font_t* font);
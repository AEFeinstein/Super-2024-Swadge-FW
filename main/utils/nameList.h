/**
 * @file nameList.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides index-based usernames that will avoid the issues inherent in free text entry.
 * @date 2025-05-02
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>

#define USERNAME_MAX_LEN 64

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    LIST1,
    LIST2,
    LIST3
} nameListEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t nameIdxs[3];
    uint8_t randCode;
} nameStruct_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Generates a name from Swadge's MAC address
 *
 * @param ns Name object new name is generated from
 * @param buffer Buffer to write name to
 * @param buffLen Length of the buffer
 */
void generateMACName(nameStruct_t* ns, char* buffer, int buffLen);

/**
 * @brief Grabs the text from the specified list
 *
 * @param listIdx Which list to grab from
 * @param idx Index of the text asked for
 * @param buffer Buffer to store text
 * @param buffLen Length of the buffer
 */
void getTextFromList(int listIdx, int idx, char* buffer, int buffLen);
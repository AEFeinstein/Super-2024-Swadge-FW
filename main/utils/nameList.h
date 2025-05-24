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
#include "hdw-btn.h"

#define MAX_ADJ1_LEN 10 // Length of longest word in list
#define MAX_ADJ2_LEN 10 // Length of longest word in list
#define MAX_NOUN_LEN 10 // Length of longest word in list

// +3 for number, +1 for end, +3 for '-' = +7
#define USERNAME_MAX_LEN (MAX_ADJ1_LEN + MAX_ADJ2_LEN + MAX_NOUN_LEN + 7)

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int8_t idxs[3];
    uint8_t randCode;
    int arrayIdx;
    char nameBuffer[USERNAME_MAX_LEN];
    bool user;
} nameData_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Call this to initialize the MAC variable. Call inside the swadge2024.h file.
 *
 */
void initUsernameSystem(void);

/**
 * @brief Generates a username based on the MAC of the swadge
 *
 * @param nd Data struct the name is saved to
 */
void generateMACUsername(nameData_t* nd);

/**
 * @brief Generates a random username.
 *
 * @param nd Data struct the name is saved to
 */
void generateRandUsername(nameData_t* nd);

/**
 * @brief Sets the username from a predefined nd
 *
 * @param nd nd containing the data
 */
void setUsernameFromND(nameData_t* nd);

/**
 * @brief Set the Username From indices. Useful for loading data from swadgepass
 *
 * @param nd Data struct to receive the objects
 * @param idx1 First index
 * @param idx2 Second index
 * @param idx3 Third index
 * @param randomCode numbers at the end of the swadge
 */
void setUsernameFromIdxs(nameData_t* nd, int idx1, int idx2, int idx3, int randomCode);

/**
 * @brief Handles the input of the username.
 *
 * @param evt The button event object
 * @param nd Data structure to store data in
 * @return true If the user presses A to finalize the name
 * @return false If the user is still selecting a name
 */
bool handleUsernamePickerInput(buttonEvt_t* evt, nameData_t* nd);

/**
 * @brief Draws the picker input
 *
 * @param nd The data
 */
void drawUsernamePicker(nameData_t* nd);

/**
 * @brief Get the System Username object
 * 
 * @return nameData_t* The username for the system at large
 */
nameData_t* getSystemUsername(void);
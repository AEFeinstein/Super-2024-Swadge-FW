/**
 * @file cVectors.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Implements the C++ style vectors (dynamic arrays)
 * @version 1.0
 * @date 2025-01-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

// TODO: Docs

//==============================================================================
// Structs
//==============================================================================

typedef struct 
{
    void **data;
    int capacity;
    int total;
} cVector_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Initializes the vector object
 * 
 * @param vec Vector to initialize
 * @note Initializes with 'malloc' with initial size of 4
 */
void initVector(cVector_t* vec);

/**
 * @brief Get the size of the Vector
 * 
 * @param vec Vector to evaluate
 * @return int length of the vector
 */
int vectorTotal(cVector_t* vec);

/**
 * @brief Add an item to a vector
 * 
 * @param vec Vector to add item to
 * @param item Item to add
 */
void vectorAdd(cVector_t* vec, void* item);

/**
 * @brief Sets a specifc value
 * 
 * @param vec Vector to use
 * @param idx Index to load values into
 * @param item Item to add
 */
void vectorSet(cVector_t* vec, int idx, void* item);

/**
 * @brief Retrieve value stored in vector
 * 
 * @param vec Vector to get data from
 * @param idx Index to fetch data from
 * @return void* Data stored at the index. NULL when idex is invalid
 */
void* vectorGet(cVector_t* vec, int idx);

void vectorRm(cVector_t* vec, int idx);

/**
 * @brief Frees the memory
 * 
 * @param vec Vector to free
 */
void freeVector(cVector_t* vec);
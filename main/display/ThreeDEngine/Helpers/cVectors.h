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

/*! \file cVectors.h
    \brief C++ Style vectors used wherever dynamic arrays are required.

    Implements the following funcitons:
    - vectorInit(&vec): Initializes the vector
    - vectorTotal(&vec): Get the length of the vector
    - vectorAdd(&vec, &item): Add item at end
    - vectorSet(&vec, idx, &item): Save item at index
    - vectorGet(&vec, idx): Get data at idx
    - vectorRemove(&vec, idx): Remove item at idx
    - vectorFree(&vec): Free the Vector

    \section vecExample Examples
    \code{.c}
    // Vars
    int a = 5;
    int b = 9;
    int c = 42;
    int d = 88;

    // Init
    cVector v;
    vectorInit(&v);

    // Add items
    vectorAdd(&v, &a);
    vectorAdd(&v, &b);
    vectorAdd(&v, &c);

    // Set item
    vectorSet(&v, 1, &d);

    // Get item
    int* i = vectorGet(&v, 2);

    // Remove item
    vectorRemove(&v, 0);

    vectorFree(&v);
    \endcode
*/

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Vector data structure.
 *
 */
typedef struct
{
    void** data;
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
void vectorInit(cVector_t* vec);

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

/**
 * @brief Removes data from the vector
 *
 * @param vec Vector to remove data from
 * @param idx Index to remove data at
 */
void vectorRemove(cVector_t* vec, int idx);

/**
 * @brief Frees the memory
 *
 * @param vec Vector to free
 */
void vectorFree(cVector_t* vec);
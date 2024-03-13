/*!
 * \file hashMap.h
 * \date 2024-03-10
 * \author Dylan Whichard
 * \brief A generic map for storing arbitrary key-value pairs
 *
 * \section hashMap_design Design Philosophy
 *
 * This is a generic hash map data structure, which supports the basic put, get, remove, and iterate operations.
 * By default, keys are assumed to be null-terminated strings, but any type can be used if alternate hash and
 * equality functions are supplied. The hash map will automatically be resized once its item count reaches 75%
 * of its array size, but setting a sufficiently large initial size will improve efficiency overall.
 *
 * Basic operations against the hash map generally have O(1) time in the average case, with O(n) time in the worst case.
 * The exception is hashIterRemove(), which has O(1) time in the worst case. Hash collisions are handled by storing
 * multiple items within the same bucket in a linked list. Resizing is O(k), where k is the number of buckets in the
 * hash map, and should occur once in every k put operations.
 *
 * \section hashMap_usage Usage
 *
 * hashInit() allocates a new hash map for string keys, and hashInit
 *
 * hashPut() adds a new entry to the map or updates the value of an existing one.
 *
 * hashGet() retrieves a value from the map.
 *
 * hashRemove() removes an entry from the map by its key.
 *
 * hashDeinit() deallocates a hash map and all its entries.
 *
 * \section hashMap_example Example
 *
 * \subsection hashMap_example_iteration Iteration
 *
 * \section hashMap_caveats Caveats
 * This utility has some drawbacks, mainly
 */
#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "linked_list.h"

/**
 * @brief A function that takes a pointer to key data and returns its hash value
 *
 * @param data The data to hash
 * @return An int32_t representing the hash of this data
 */
typedef uint32_t (*hashFunction_t)(const void* data);

/**
 * @brief A function that takes two pointers to keys and returns true if the values they point to are equal
 */
typedef bool (*eqFunction_t)(const void* a, const void* b);

// Forward-declared internal structs
typedef struct hashBucket hashBucket_t;
typedef struct hashIterState hashIterState_t;

/**
 * @brief Struct used for iterating through a hash map efficiently
 */
typedef struct
{
    /// @brief The key of the current key-value pair
    void* key;
    /// @brief The value of the current key-value pair
    void* value;

    /// @internal @brief Internal iterator state
    hashIterState_t* _state;
} hashIterator_t;

/**
 * @brief A hash map for storing key-value pairs
 */
typedef struct
{
    /// @brief The total number of allocated buckets in the hash map
    int size;

    /// @brief The actual number of items stored in the hash map
    int count;

    /// @brief The array of bucket values
    hashBucket_t* values;

    /// @brief The key hash function to use, or NULL to use hashString()
    hashFunction_t hashFunc;

    /// @brief The key equality function to use, or NULL to use strEq()
    eqFunction_t eqFunc;
} hashMap_t;

uint32_t hashString(const char* str);
bool strEq(const void* a, const void* b);

// Helper functions for using custom key types
uint32_t hashBytes(const uint8_t* bytes, size_t length);
bool bytesEq(const uint8_t* a, size_t aLength, const uint8_t* b, size_t bLength);

void hashPutVoid(hashMap_t* map, const void* key, void* value);
void hashPut(hashMap_t* map, const char* key, void* value);
void* hashGetHash(hashMap_t* map, const void* key);
void* hashGet(hashMap_t* map, const char* key);
bool hashIterRemove(hashMap_t* map, hashIterator_t* iter);
void* hashRemoveHash(hashMap_t* map, const void* key);
void* hashRemove(hashMap_t* map, const char* key);
void hashInit(hashMap_t* map, int initialSize);
void hashInitStrKey(hashMap_t* map, int initialSize, hashFunction_t hashFunc, eqFunction_t eqFunc);
void hashDeinit(hashMap_t* map);
bool hashIterate(hashMap_t* map, hashIterator_t* iterator);
void hashIterReset(hashIterator_t* iterator);

#endif

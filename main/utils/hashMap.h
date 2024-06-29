/*!
 * \file hashMap.h
 * \date 2024-03-10
 * \author Dylan Whichard
 * \brief A generic map for storing arbitrary key-value pairs
 *
 * \section hashMap_design Design Philosophy
 *
 * This is a generic hash map data structure, which supports the basic put, get, and remove operations, as well
 * as iteration. By default, keys are assumed to be nul-terminated strings, but any type can be used if
 * alternate hash and equality functions are supplied. The hash map will automatically resize itself once it
 * becomes about 75% full, but setting a sufficiently large initial capacity will improve efficiency overall.
 *
 * Keys inserted into the map must remain valid for the duration they are in the map, as the map maintains a reference
 * rather than copying them. This also means that modifying the key itself after it has been inserted into the
 * map will cause it to become inaccessible, so using immutable data as pointers is also recommended. To change
 * the key of an entry in the map, remove it and add it with the new key instead.
 *
 * Basic operations against the hash map have O(1) time in the average case, with O(n) time in the worst case.
 * Hash collisions are handled by storing multiple items within the same bucket in a linked list. Resizing is O(k),
 * where k is the number of buckets in the hash map, and should occur once in every k put operations.
 *
 * Iteration over the map is O(k) best case, O(k+n) worst case, and O(k) average case, as we do not know ahead of time
 * which buckets or keys are in use. This could theoretically be improved to O(n) worst case if keys were also kept in
 * a separate array, but this would be a marginal improvement with a lot of complexity.
 *
 * \section hashMap_usage Usage
 *
 * hashInit() allocates a new hash map for use with string keys.
 *
 * hashPut() adds a new entry to the map or updates the value of an existing one.
 *
 * hashGet() retrieves a value from the map.
 *
 * hashRemove() removes an entry from the map by its key.
 *
 * hashDeinit() deallocates a hash map and all its entries.
 *
 * hashIterate() can be used to loop over a hash map's entries.
 *
 * hashIterReset() is used to reset an iterator if iteration stopped before hashIterate() returned false.
 *
 * hashIterRemove() can be used to safely remove entries during iteration.
 *
 * hashPutBin(), hashGetBin(), and hashRemoveBin() are variants of the normal hash functions which accept
 * a void pointer in the \c key argument, rather than a char pointer. You must provide hash and equality
 * functions to hashInitBin() in order to safely use keys which are not nul-terminated strings.
 *
 * hashString(), strEq(), hashBytes(), and bytesEq() are helper functions useful when implementing custom
 * hash and comparison functions.
 *
 * \section hashMap_example Examples
 *
 * Create a hash map, add and retrieve some values:
 * \code{.c}
 * // Declare a hash map
 * hashMap_t map = {0};
 * // Initialize it with an initial capacity of 16
 * hashInit(&map, 16);
 *
 * hashPut(&map, "greeting", "Hello");
 * hashPut(&map, "name", "Swadgeman");
 * // Prints 'Hello! Your name is Swadgeman!'
 * printf("%s! Your name is %s!\n", hashGet(&map, "greeting"), hashGet(&map, "name"));
 *
 * hashPut(&map, "greeting", "Goodbye");
 * // Prints 'Goodbye, Swadgeman.'
 * printf("%s, %s.", hashGet(&map, "greeting"), hashGet(&map, "name"));
 * \endcode
 *
 * \subsection hashMap_example_iteration Iteration
 * Iterate over items in the map:
 * \code{.c}
 * hashMap_t map = {0};
 * hashInit(&map, 16);
 *
 * hashPut(&map, "title", "HashMap");
 * hashPut(&map, "description", "A cool new data structure");
 * hashPut(&map, "filename", "hashMap.h");
 * hashPut(&map, "directory", "utils");
 * hashPut(&map, "somethingElse", "donut");
 * hashPut(&map, "tag-0", "data-structure");
 * hashPut(&map, "tag-1", "map");
 * hashPut(&map, "tag-2", "dictionary");
 *
 * hashIterator_t iter = {0};
 * while (hashIterate(&map, &iter))
 * {
 *     printf("%s: %s\n", iter.key, iter.value);
 * }
 * // No need to use hashIterReset() here as the loop finished normally
 * hashDeinit(&map);
 * \endcode
 *
 * Remove items while iterating:
 * \code{.c}
 * while (hashIterate(&map, &iter))
 * {
 *     // Remove any items with a key starting with "tag-"
 *     if (!strncmp(iter.key, "tag-", 4))
 *     {
 *         printf("Removing tag %s\n", iter.value);
 *         if (!hashIterRemove(&map, &iter))
 *         {
 *             //
 *             break;
 *         }
 *     }
 * }
 * // Still no need to use hashIterReset(), as it will be handled
 * // automatically by either hashIterate() or hashIterRemove()
 * \endcode
 *
 * Breaking iteration early:
 * \code{.c}
 * // Find and print any tag, then stop
 * while (hashIterate(&map, &iter))
 * {
 *     if (!strncmp(iter.key, "tag-", 4))
 *     {
 *         printf("Found a tag: %s\n", iter.key);
 *         break;
 *     }
 * }
 *
 * // This time, hashIterReset() is necessary!
 * // We might not have reached the end of iteration, so the iterator might not have reset automatically
 * // If we did happen to reach the end anyway, resetting it again won't hurt anything
 * hashIterReset(&iter);
 *
 * \subsection hashMap_example_nonStringKeys Non-string Keys
 *
 * The hash map also supports using structs or even primitives as key types, instead of strings.
 * This can be done using hashInitBin() to supply custom hashing and comparison functions
 * appropriate for the type of key being used.
 *
 * \subsubsection hashMap_example_nonStringKeys_enum
 *
 * This example shows how to use a hash map where the keys are enum values. This method will
 * work with any numeric type, as long as \c{sizeof(type) <= sizeof(void*)} -- that is, as
 * long as it fits inside a pointer. In practice this means an \c int32_t can be used as a
 * key in a hash map in this way, but an \c int64_t cannot.
 *
 * \code{.c}
 * #include "hdw-btn.h"
 * #include "spiffs_wsg.h"
 * #include <stdbool.h>
 * #include <stdint.h>
 *
 * // Hash function for using ::buttonBit_t as a key
 * uint32_t hashEnum(const void* data)
 * {
 *     // This might be cursed but the C spec says it's totally fine
 *     buttonBit_t enumValue = (buttonBit_t)(data);
 *     return hashData(&enumValue, sizeof(buttonBit_t));
 * }
 *
 * // Compare ::buttonBit_t values stuffed into void pointers
 * bool enumEq(const void* a, const void* b)
 * {
 *     return a == b;
 * }
 *
 * // Just a simple struct to hold a WSG and its rotation for the values
 * typedef struct
 * {
 *     int16_t rot;
 *     wsg_t wsg;
 * } wsgRot_t;
 *
 * hashmap_t map = {0};
 * wsgRot_t* wsgs = NULL;
 *
 * void setupButtonIconMap(void)
 * {
 *     hashInitBin(&map, 16, hashEnum, enumEq);
 *
 *     wsgs = calloc(8, sizeof(wsgRot_t));
 *     wsgRot_t* wsg = wsgs;
 *
 *     // Load up a bunch of WSGs
 *     loadWsg("button_up.wsg", &wsg->wsg, false);
 *     hashPutBin(&map, PB_UP, wsg++);
 *
 *     loadWsg("button_up.wsg", &wsg->wsg, false);
 *     wsg->rot = 180;
 *     hashPutBin(&map, PB_DOWN, wsg++);
 *
 *     loadWsg("button_up.wsg", &wsg->wsg, false);
 *     wsg->rot = 90;
 *     hashPutBin(&map, PB_LEFT, wsg++);
 *
 *     loadWsg("button_up.wsg", &wsg->wsg, false);
 *     wsg->rot = 270;
 *     hashPutBin(&map, PB_RIGHT, wsg++);
 *
 *     loadWsg("button_a.wsg", &wsg->wsg, false);
 *     hashPutBin(&map, PB_A, wsg++);
 *
 *     loadWsg("button_b.wsg", &wsg->wsg, false);
 *     hashPutBin(&map, PB_B, wsg++);
 *
 *     loadWsg("button_menu.wsg", &wsg->wsg, false);
 *     hashPutBin(&map, PB_MENU, wsg++);
 *
 *     loadWsg("button_pause.wsg", &wsg->wsg, false);
 *     hashPutBin(&map, PB_PAUSE, wsg++);
 * }
 *
 * void runButtonIconMap(void)
 * {
 *     // Use the WSGs
 *     buttonEvt_t evt = {0};
 *     while (checkButtonQueueWrapper(&evt))
 *     {
 *         if (evt.down)
 *         {
 *             wsgRot_t* draw = hashGet(&map, evt.button);
 *             drawWsg(&draw->wsg, (TFT_WIDTH - draw->w) / 2, (TFT_HEIGHT - draw->h) / 2, false, false, draw->rot);
 *         }
 *     }
 * }
 *
 * void cleanupButtonIconMap(void)
 * {
 *     // Clean up the WSGs using the hash map
 *     hashIterator_t iter = {0};
 *     while (hashIterate(&map, &iter))
 *     {
 *         wsgRot_t* value = iter.value;
 *         freeWsg(&value->wsg);
 *     }
 *
 *     // Clean up the hash map and WSG struct
 *     hashDeinit(&map);
 *
 *     // Clean up the values
 *     free(wsgs);
 * }
 * \endcode
 *
 * \subsubsection hashMap_example_nonStringKeys_struct
 * This example shows how to use
 * \code{.c}
 * // A struct to use for the key
 * typedef struct
 * {
 *     int32_t x, y;
 * } keyStruct_t;
 *
 * int32_t hashStruct(const void* data)
 * {
 *     // Helper function provided with hash map
 *     return hashBytes((const uint8_t*)data, sizeof(keyStruct_t));
 * }
 *
 * bool structEq(const void* a, const void* b)
 * {
 *     // Another hash map helper function
 *     return bytesEq((const uint8_t*)a, sizeof(keyStruct_t), (const uint8_t*)b, sizeof(keyStruct_t));
 * }
 *
 * hashMap_t map = {0};
 * keyStruct_t keys[5] = {0};
 *
 * void setupStructKeyExample(void)
 * {
 *     hashInitBin(&map, 10, hashStruct, structEq);
 *
 *     keys[0].x = 10;
 *     keys[0].y = 16;
 *
 *     keys[1].x = 5;
 *     keys[1].y = 5;
 *
 *     keys[2].x = 7;
 *     keys[2].y = 12;
 *
 *     keys[3].x = 1;
 *     keys[3].y = 3;
 *
 *     keys[4].x = 16;
 *     keys[4].y = 4;
 *
 *     hashPutBin(&map, &keys[0], "Star");
 *     hashPutBin(&map, &keys[1], "Square");
 *     hashPutBin(&map, &keys[2], "Circle");
 *     hashPutBin(&map, &keys[3], "Diamond");
 *     hashPutBin(&map, &keys[4], "Triangle");
 * }
 *
 * void runStructKeyExample(void)
 * {
 *     static int x = 0;
 *     static int y = 0;
 *
 *     buttonEvt_t evt;
 *     while (checkButtonQueueWrapper(&evt))
 *     {
 *         if (evt.down)
 *         {
 *             if (evt.button == PB_UP)
 *             {
 *                 y++;
 *             }
 *             else if (evt.button == PB_DOWN)
 *             {
 *                 y--;
 *             }
 *             else if (evt.button == PB_LEFT)
 *             {
 *                 x--;
 *             }
 *             else if (evt.button == PB_RIGHT)
 *             {
 *                 x++;
 *             }
 *             else if (evt.button == PB_A)
 *             {
 *                 keyStruct_t key;
 *                 key.x = x;
 *                 key.y = y;
 *                 const char* found = hashGetBin(&map, &key);
 *                 if (found)
 *                 {
 *                     printf("Found a %s at %d, %d!\n", found, x, y);
 *                 }
 *                 else
 *                 {
 *                     printf("Didn't find anything at %d, %d :(\n", x, y);
 *                 }
 *             }
 *         }
 *     }
 * }
 * \endcode
 */
#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

// Default hash functions
uint32_t hashString(const char* str);
bool strEq(const void* a, const void* b);

// Helper functions for using custom key types
uint32_t hashBytes(const uint8_t* bytes, size_t length);
bool bytesEq(const uint8_t* a, size_t aLength, const uint8_t* b, size_t bLength);

void hashPut(hashMap_t* map, const char* key, void* value);
void* hashGet(hashMap_t* map, const char* key);
void* hashRemove(hashMap_t* map, const char* key);

void hashPutBin(hashMap_t* map, const void* key, void* value);
void* hashGetBin(hashMap_t* map, const void* key);
void* hashRemoveBin(hashMap_t* map, const void* key);

void hashInit(hashMap_t* map, int initialSize);
void hashInitBin(hashMap_t* map, int initialSize, hashFunction_t hashFunc, eqFunction_t eqFunc);
void hashDeinit(hashMap_t* map);

bool hashIterate(hashMap_t* map, hashIterator_t* iterator);
bool hashIterRemove(hashMap_t* map, hashIterator_t* iter);
void hashIterReset(hashIterator_t* iterator);

#endif

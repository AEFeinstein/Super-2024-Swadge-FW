//==============================================================================
// Includes
//==============================================================================

#include "hashMap.h"

#include <malloc.h>
#include <stddef.h>
#include <string.h>

#include <esp_log.h>

//==============================================================================
// Defines
//==============================================================================

// #define HASH_LOG(...) ESP_LOGI("HashMap", __VA_ARGS__)
#define HASH_LOG(...)

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A single key-value pair in the hash map
 *
 */
typedef struct
{
    ///<  The key's hash value
    uint32_t hash;

    ///<  The key of this pair, or NULL if this node is empty
    const void* key;

    ///<  The value of this pair
    void* value;
} hashNode_t;

/**
 * @brief A single element of the hash map array, holding either one value or a list of values.
 *
 * While holding a single value, the item's key, hash, and value are held directly in the struct.
 * While holding multiple values, each item is stored as a value in a linked list, heap-allocated.
 */
typedef struct hashBucket
{
    ///<  Whether the bucket contains multiple or a single item
    bool hasMulti;

    union
    {
        ///<  The node's single-item contents, when hasMulti is false
        hashNode_t single;

        ///<  The node's multi-item contents, when hasMulti is true
        list_t multi;
    };
} hashBucket_t;

/**
 * @brief The internal state of a hash map iterator
 *
 */
typedef struct hashIterState
{
    ///<  The bucket containing the current item
    hashBucket_t* curBucket;

    ///<  The list node containing the current item, if within a multi bucket
    node_t* curListNode;

    ///<  The node containing the current item
    hashNode_t* curNode;

    ///<  The number of items returned by the iterator
    int returned;

    ///<  Whether the iterator has already been advanced, after removing the previous item
    bool removed;
} hashIterState_t;

//==============================================================================
// Static Function Prototypes
//==============================================================================

static inline void hashCheckSize(hashMap_t* map);
static inline hashNode_t* hashFindNode(hashMap_t* map, const void* key, uint32_t* hashOut, hashBucket_t** bucketOut,
                                       node_t** multiOut);
static inline hashNode_t* bucketPut(hashMap_t* map, hashBucket_t* bucket, const void* key, void* value, uint32_t hash,
                                    int* count);
static inline hashNode_t bucketRemove(hashMap_t* map, hashBucket_t* bucket, hashNode_t* node, node_t* multiNode,
                                      int* count);
static bool hashIterNext(const hashMap_t* map, hashIterator_t* iter);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check the hash map's size and resize it if necessary.
 *
 * Runtime: O(n)
 *
 * @param map The hash map to resize
 */
static inline void hashCheckSize(hashMap_t* map)
{
    // if count >= size * .75
    if (map->count * 4 >= 3 * map->size)
    {
        int size = map->size * 2;
        HASH_LOG("Resizing backing array from %d to %d (count=%d)", map->size, size, map->count);
        hashBucket_t* newArray = reallocarray(map->values, size, sizeof(hashBucket_t));

        if (newArray != NULL)
        {
            // Zero the new memory as reallocarray does not do it for us
            memset(newArray + map->size, 0, (size - map->size) * sizeof(hashBucket_t));

            int moved = 0;
            // Rehash everything in the old portion of the array
            for (int i = 0; i < map->size; i++)
            {
                hashBucket_t* bucket = &newArray[i];
                if (bucket->hasMulti)
                {
                    // Need to store the next list node here before deleting the current one
                    node_t* nextNode = NULL;
                    for (node_t* listNode = bucket->multi.first; listNode != NULL; listNode = nextNode)
                    {
                        nextNode = listNode->next;

                        hashNode_t* node = listNode->val;
                        if ((node->hash % size) != i)
                        {
                            // The node belongs in a different bucket after the resize
                            hashNode_t tmp = bucketRemove(map, bucket, node, listNode, NULL);
                            node = bucketPut(map, &newArray[tmp.hash % size], tmp.key, tmp.value, tmp.hash, NULL);
                            moved++;
                        }
                    }
                }
                else
                {
                    hashNode_t* node = &bucket->single;
                    if (node->key && (node->hash % size != i))
                    {
                        // The node belongs in a different bucket after the resize
                        hashNode_t tmp = bucketRemove(map, bucket, node, NULL, NULL);
                        node           = bucketPut(map, &newArray[tmp.hash % size], tmp.key, tmp.value, tmp.hash, NULL);
                        moved++;
                    }
                }
            }

            HASH_LOG("Moved %d nodes after resizing", moved);

            map->size   = size;
            map->values = newArray;
        }
        else
        {
            ESP_LOGE("HashMap", "Failed to resize HashMap");
        }
    }
}

/**
 * @brief Find and return a pointer to the node for a given key, and optionally returns its hash and pointers to its
 * bucket and list node
 *
 * Runtime: O(1) average case, O(n) worst case (all items in the same bucket) but not common
 *
 * @param map The hash map to search
 * @param key The key to use for searching
 * @param[out] hashOut A pointer to an int32_t to write the calculated hash into
 * @param[out] bucketOut A pointer to a pointer to a bucket to be set to the containing bucket
 * @param[out] multiOut A pointer to a pointer to a list node to be set to the containing list item, if any
 * @return hashNode_t* A pointer to the node corresponding to the given key, if any was found
 */
static inline hashNode_t* hashFindNode(hashMap_t* map, const void* key, uint32_t* hashOut, hashBucket_t** bucketOut,
                                       node_t** multiOut)
{
    uint32_t hash        = map->hashFunc ? map->hashFunc(key) : hashString((const char*)key);
    int index            = hash % map->size;
    hashBucket_t* bucket = &map->values[index];
    eqFunction_t eqFn    = map->eqFunc ? map->eqFunc : strEq;
    // We can always dereference the first pointer because if the list is emptied, hasMulti should be false
    // (It could still have only 1 entry but that's fine)
    hashNode_t* node    = NULL;
    node_t* listNodeOut = NULL;

    if (bucket->hasMulti)
    {
        node = bucket->multi.first->val;
    }
    else
    {
        node = &bucket->single;
    }

    int tries = 1;
    if (node->key != NULL && (node->hash != hash || !eqFn(node->key, key)))
    {
        // Node doesn't match!
        node = NULL;

        if (bucket->hasMulti)
        {
            for (node_t* listNode = bucket->multi.first; listNode != NULL; listNode = listNode->next)
            {
                tries++;
                hashNode_t* newNode = listNode->val;
                if (newNode->hash == hash && eqFn(newNode->key, key))
                {
                    node        = newNode;
                    listNodeOut = listNode;
                    break;
                }
            }
        }
    }

    if (node)
    {
        if (!map->hashFunc || map->hashFunc == hashString)
        {
            HASH_LOG("Found %s node %d for key %s after %d %s", (node->key) ? "in-use" : "new", index, (const char*)key,
                     tries, (tries == 1) ? "try" : "tries");
        }
        else
        {
            HASH_LOG("Found %s node %d for key %p after %d %s", (node->key) ? "in-use" : "new", index, (const void*)key,
                     tries, (tries == 1) ? "try" : "tries");
        }
    }
    else
    {
        if (!map->hashFunc || map->hashFunc == hashString)
        {
            HASH_LOG("Did not find node for key %s after %d %s", (const char*)key, tries,
                     (tries == 1) ? "try" : "tries");
        }
        else
        {
            HASH_LOG("Did not find node for key %p after %d %s", (const void*)key, tries,
                     (tries == 1) ? "try" : "tries");
        }
    }

    if (hashOut)
    {
        *hashOut = hash;
    }

    if (bucketOut)
    {
        *bucketOut = bucket;
    }

    if (multiOut)
    {
        *multiOut = listNodeOut;
    }

    return node;
}

/**
 * @brief Insert an entry into the given bucket, converting the bucket to a multi-bucket if necessary.
 *
 * Runtime: O(1) average case, O(n) worst case (all items in same bucket)
 *
 * @param map The map the bucket is part of
 * @param bucket The bucket to insert into
 * @param key The key of the new entry
 * @param value The value of the new entry
 * @param hash The hash of the new entry key
 * @param[out] count If non-NULL, will be incremented if the value is inserted
 * @return hashNode_t* A pointer to the newly-inserted node
 */
static inline hashNode_t* bucketPut(hashMap_t* map, hashBucket_t* bucket, const void* key, void* value, uint32_t hash,
                                    int* count)
{
    eqFunction_t eqFn = map->eqFunc ? map->eqFunc : strEq;
    // We can always dereference the first pointer because if the list is emptied, hasMulti should be false
    // (It could still have only 1 entry but that's fine)
    hashNode_t* node = bucket->hasMulti ? (hashNode_t*)bucket->multi.first->val : &bucket->single;

    int tries = 1;
    if (node->key != NULL && (node->hash != hash || !eqFn(node->key, key)))
    {
        // Node is non-empty and doesn't match hash
        hashNode_t* newNode;

        // This bucket doesn't have multiple entries yet, so we need to convert it
        if (!bucket->hasMulti)
        {
            // Gotta make it a multi-bucket
            HASH_LOG("Bucket collision; converting bucket to multi-value");

            // Copy the first node itself
            newNode = malloc(sizeof(hashNode_t));
            memcpy(newNode, node, sizeof(hashNode_t));

            // Init the list data
            bucket->hasMulti = true;
            memset(&bucket->multi, 0, sizeof(list_t));

            // And add the new first node to the list
            push(&bucket->multi, newNode);

            // Now, we should also add an empty node
            newNode = calloc(1, sizeof(hashNode_t));
            push(&bucket->multi, newNode);

            if (count)
            {
                (*count)++;
            }
        }
        else
        {
            // There should always be a last item in the list if it's multi
            newNode = bucket->multi.last->val;
        }

        if (newNode->key == NULL)
        {
            // If the 'new node' is empty, it should be the correct one already
            node = newNode;
        }
        else
        {
            // Search through the nodes until we find a matching one
            // I don't think we would ever come across an empty one here since it should always be last if it's there
            node = NULL;
            for (node_t* listNode = bucket->multi.first; listNode != NULL; listNode = listNode->next)
            {
                tries++;
                newNode = listNode->val;
                if (newNode->hash == hash && eqFn(newNode->key, key))
                {
                    node = newNode;
                    break;
                }
            }

            if (node == NULL)
            {
                // Gotta add one
                node = calloc(1, sizeof(hashNode_t));
                push(&bucket->multi, node);

                if (count)
                {
                    (*count)++;
                }
            }
        }
    }
    else if (!node->key && count)
    {
        (*count)++;
    }

    node->hash  = hash;
    node->key   = key;
    node->value = value;

    return node;
}

/**
 * @brief Helper function to remove an entry from a bucket, but not necessarily from a map
 *
 * Runtime: O(1) average case, O(n) worst case (all items in same bucket)
 *
 * @param map The map containing the bucket
 * @param bucket The bucket containing the item
 * @param node The hash map node for the item to remove
 * @param multiNode If the bucket is a multi-item bucket, the list node
 * @param[out] count The count of items, to be decremented if set
 * @return A copy of the node that was removed
 */
static inline hashNode_t bucketRemove(hashMap_t* map, hashBucket_t* bucket, hashNode_t* node, node_t* multiNode,
                                      int* count)
{
    hashNode_t result = {0};
    if (bucket->hasMulti)
    {
        hashNode_t* value = removeEntry(&bucket->multi, multiNode);
        if (value->key)
        {
            if (count)
            {
                (*count)--;
            }
            result = *value;
        }
        free(value);
        value = NULL;

        // Un-multi-ify the bucket if its list is empty
        if (bucket->multi.length == 0)
        {
            bucket->hasMulti = false;
            memset(&bucket->single, 0, sizeof(hashNode_t));
        }
    }
    else
    {
        if (node->key != NULL && count)
        {
            (*count)--;
        }

        result = *node;

        node->hash  = 0;
        node->key   = NULL;
        node->value = NULL;
    }

    if (!map->hashFunc || map->hashFunc == hashString)
    {
        HASH_LOG("Removed node for key %s", (const char*)result.key);
    }
    else
    {
        HASH_LOG("Removed node for key %p", result.key);
    }

    return result;
}

/**
 * @brief Convert a NULL-terminated string to a hash value
 *
 * Uses the 'djb2' algorithm, described at http://www.cse.yorku.ca/~oz/hash.html
 *
 * @param str The string to hash
 * @return uint32_t The hash value
 */
uint32_t hashString(const void* str)
{
    uint32_t hash    = 5381;
    const char* strv = (const char*)str;
    int c;

    while ((c = *strv++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

/**
 * @brief Compare two NUL-terminated strings.
 *
 * @param a The first string to compare
 * @param b The second string to compare
 * @return true when the two strings are equal or the same pointer
 * @return false when the two strings differ
 */
bool strEq(const void* a, const void* b)
{
    return (a == b) || (0 == strcmp((const char*)a, (const char*)b));
}

/**
 * @brief Convert an arbitrary byte sequence to a hash value
 *
 * Uses the 'djb2' algorithm, described at http://www.cse.yorku.ca/~oz/hash.html
 *
 * @param bytes A pointer to a byte array to hash
 * @param length The length of the byte array
 * @return uint32_t The hash value
 */
uint32_t hashBytes(const uint8_t* bytes, size_t length)
{
    const uint8_t* end = (bytes + length);
    uint32_t hash      = 5381;

    while (bytes < end)
    {
        hash = ((hash << 5) + hash) + (*bytes++);
    }

    return hash;
}

/**
 * @brief Compare two byte arrays
 *
 * @param a A pointer to the first byte array
 * @param aLength The length of the first byte array
 * @param b A pointer to the second byte array
 * @param bLength The length of the second byte array
 * @return true when the byte arrays are equal or the same pointer
 * @return false when the byte arrays differ
 */
bool bytesEq(const uint8_t* a, size_t aLength, const uint8_t* b, size_t bLength)
{
    return aLength == bLength && ((a == b) || (0 == memcmp(a, b, aLength)));
}

/**
 * @brief Create or update a key-value pair in the hash map with a string key
 *
 * @warning A reference to the key will be stored in the map until the entry is removed
 *
 * @param map The hash map to update
 * @param key The string key to associate the value with
 * @param value The value to add to the map
 */
void hashPut(hashMap_t* map, const char* key, void* value)
{
    hashPutBin(map, key, value);
}

/**
 * @brief Return the value in the hash map associated with the given string key
 *
 * @param map The hash map to search
 * @param key The string key to retrieve the value for
 * @return void* A pointer to the mapped value, or NULL if it was not found
 */
void* hashGet(hashMap_t* map, const char* key)
{
    return hashGetBin(map, (const void*)key);
}

/**
 * @brief Remove the value with a given string key from the hash map
 *
 * @param map The hash map to remove from
 * @param key The string key to remove the value for
 * @return void* The item that was removed, or NULL if no item was found for the given key
 */
void* hashRemove(hashMap_t* map, const char* key)
{
    return hashRemoveBin(map, (const void*)key);
}

/**
 * @brief Create or update a key-value pair in the hash map with a non-string key
 *
 * @param map The hash map to update
 * @param key The key to associate the value with
 * @param value The value to add to the map
 */
void hashPutBin(hashMap_t* map, const void* key, void* value)
{
    hashCheckSize(map);

    uint32_t hash = map->hashFunc ? map->hashFunc(key) : hashString((const char*)key);
    int index     = (hash % map->size);
    int newCount  = map->count;
    bucketPut(map, &map->values[index], key, value, hash, &newCount);

    if (map->count != newCount)
    {
        map->count = newCount;

        if (!map->hashFunc || map->hashFunc == hashString)
        {
            HASH_LOG("Put: %s = %p, count now %d", (const char*)key, value, map->count);
        }
        else
        {
            HASH_LOG("Put: %p = %p, count now %d", key, value, map->count);
        }
    }
    else
    {
        if (!map->hashFunc || map->hashFunc == hashString)
        {
            HASH_LOG("Set: %s = %p", (const char*)key, value);
        }
        else
        {
            HASH_LOG("Set: %p = %p", key, value);
        }
    }
}

/**
 * @brief Return the value in the hash map associated with the given key
 *
 * @param map The hash map to search
 * @param key The key to retrieve the value for
 * @return void* A pointer to the mapped value, or NULL if it was not found
 */
void* hashGetBin(hashMap_t* map, const void* key)
{
    hashNode_t* node = hashFindNode(map, key, NULL, NULL, NULL);

    if (node == NULL || node->key == NULL)
    {
        return NULL;
    }
    else
    {
        return node->value;
    }
}

/**
 * @brief Remove the value with a given non-string key from the hash map
 *
 * @param map The hash map to remove from
 * @param key The non-string key to remove the value for
 * @return void* The item that was removed, or NULL if no item was found for the given key
 */
void* hashRemoveBin(hashMap_t* map, const void* key)
{
    uint32_t hash;
    hashBucket_t* bucket;
    node_t* multiNode;
    hashNode_t* node = hashFindNode(map, key, &hash, &bucket, &multiNode);

    if (node != NULL)
    {
        hashNode_t removed = bucketRemove(map, bucket, node, multiNode, &map->count);
        return removed.value;
    }
    else
    {
        // Nothing to remove, key not found
        return NULL;
    }
}

/**
 * @brief Initialize a hash map for string keys
 *
 * @param map A pointer to a hashMap_t struct to be initialized
 * @param initialSize The initial size of the hash map
 */
void hashInit(hashMap_t* map, int initialSize)
{
    map->count    = 0;
    map->size     = initialSize;
    map->values   = calloc(map->size, sizeof(hashBucket_t));
    map->hashFunc = NULL;
    map->eqFunc   = NULL;
}

/**
 * @brief Initialize a hash map for non-string keys, using the given functions for hashing and comparison
 *
 * @param map A pointer to a hashMap_t struct to be initialized
 * @param initialSize The initial size of the hash map
 * @param hashFunc The hash function to use for the key datatype
 * @param eqFunc The comparison function to use for the key datatype
 */
void hashInitBin(hashMap_t* map, int initialSize, hashFunction_t hashFunc, eqFunction_t eqFunc)
{
    hashInit(map, initialSize);

    map->hashFunc = hashFunc;
    map->eqFunc   = eqFunc;
}

/**
 * @brief Deinitialize and free all memory associated with the given hash map
 *
 * @param map The map to deinitialize
 */
void hashDeinit(hashMap_t* map)
{
    if (map->count > 0)
    {
        for (hashBucket_t* bucket = map->values; bucket < (map->values + map->size); bucket++)
        {
            if (bucket->hasMulti && bucket->multi.length > 0)
            {
                hashNode_t* node = NULL;
                while (NULL != (node = pop(&bucket->multi)))
                {
                    free(node);
                }
            }
        }
    }

    free(map->values);
    map->size     = 0;
    map->count    = 0;
    map->eqFunc   = NULL;
    map->hashFunc = NULL;
}

/**
 * @brief Advance the iterator's internal state to the next item, allocating it if it has not yet been initialized
 *
 * @param map The hash map
 * @param iterator The iterator to advance
 * @return true if the iterator was advanced
 * @return false if the iterator was not advanced because it reached the end of the hash map
 */
static bool hashIterNext(const hashMap_t* map, hashIterator_t* iterator)
{
    hashIterState_t* state = iterator->_state;
    do
    {
        bool nextBucket = false;

        if (state == NULL)
        {
            // Start the iteration
            iterator->_state = state = calloc(1, sizeof(hashIterState_t));
            state->curBucket         = map->values;
            // Set nextBucket without incrementing curBucket
            // This just sets up the vars for the initial bucket
            nextBucket = true;
        }
        else
        {
            if (state->curListNode)
            {
                // This is a multi item node, try to go to the next list node
                state->curListNode = state->curListNode->next;

                // There was no next list node! Go to the next bucket instead
                if (!state->curListNode)
                {
                    state->curBucket++;
                    nextBucket = true;
                }
                else
                {
                    state->curNode = (hashNode_t*)state->curListNode->val;
                }
            }
            else
            {
                // This is a single item node, so just go to the next bucket
                state->curBucket++;
                nextBucket = true;
            }
        }

        // Set up after moving to a new bucket
        if (nextBucket)
        {
            // Check for the end of the array before dereferencing curBucket
            if (state->curBucket - map->values >= map->size)
            {
                // End of the line
                return false;
            }

            if (state->curBucket->hasMulti)
            {
                // Use the first node of the next bucket
                state->curListNode = state->curBucket->multi.first;
                state->curNode     = (hashNode_t*)state->curListNode->val;
            }
            else
            {
                // Use the single-item node of the bucket
                state->curNode     = &state->curBucket->single;
                state->curListNode = NULL;
            }
        }

        // If the node we've picked is an empty bucket, continue until we find a non-empty one
    } while (!state->curNode->key);

    return true;
}

/**
 * @brief Advance the given iterator to the next item, or return false if there is no next item
 *
 * The \c iterator should point to a zero-initialized struct at the start of iteration. Once iteration
 * completes and this function returns \c false, \c iterator will be reset. If iteration is stopped
 * before this function returns \c false, the iterator must be reset with hashIterReset() to prevent
 * memory leaks.
 *
 * Items in the hash map are not returned in any particular order.
 *
 * It is possible to remove items during iteration, but this must be done with hashIterRemove(). Using
 * hashRemove() or hashRemoveBin() during iteration is not allowed and will cause undefined behavior.
 *
 * Adding or udpating items during iteration is permitted, with the caveat that a new item inserted during
 * iteration may or may not later be returned by the iterator.
 *
 * @param[in] map The map to iterate over
 * @param[in,out] iterator A pointer to a hashIterator_t struct
 * @return true if the iterator returned an item
 * @return false if iteration is complete and no item was returned
 */
bool hashIterate(const hashMap_t* map, hashIterator_t* iterator)
{
    hashIterState_t* state = iterator->_state;

    // Exit early
    if (state != NULL && state->returned == map->count)
    {
        hashIterReset(iterator);
        return false;
    }

    // If we just removed a node, don't advance the iterator -- it was already advanced in order to prevent bad data
    if ((iterator->_state && iterator->_state->removed) || hashIterNext(map, iterator))
    {
        state = iterator->_state;
        // Finally, set the iterator values
        iterator->key   = state->curNode->key;
        iterator->value = state->curNode->value;

        // Keep track of how many we've returned via this iterator
        state->returned++;

        // Reset the 'removed' flag
        state->removed = false;

        HASH_LOG("Iterated over %d/%d", state->returned, map->count);

        // Return true to indicate iteration shoud continue
        return true;
    }
    else
    {
        HASH_LOG("Done iterating after %d/%d!", iterator->_state ? iterator->_state->returned : 0, map->count);
        // No next item!
        hashIterReset(iterator);

        return false;
    }
}

/**
 * @brief Remove the last item returned by the iterator from the hash map
 *
 * This function does not need to search and so it always runs in constant time.
 *
 * If you want to remove an item from the hash map while iterating over it,
 * this function is the only safe way to do it.
 *
 * @warning If this function returns false, iteration is complete.
 *
 * @param map The hash map to remove the item from
 * @param iter The iterator whose current item to remove from the hash map
 * @return true if there are still items remaining in the iterator
 * @return false if there are no more items remaining in the iterator
 */
bool hashIterRemove(hashMap_t* map, hashIterator_t* iter)
{
    if (!iter || !iter->_state)
    {
        // Can't do anything with nulls...
        return false;
    }

    hashIterState_t* state = iter->_state;
    bool result            = false;

    int newCount         = map->count;
    node_t* nextListNode = state->curBucket->hasMulti ? state->curListNode->next : NULL;
    bucketRemove(map, state->curBucket, state->curNode, state->curListNode, &newCount);
    if (newCount != map->count)
    {
        int diff = map->count - newCount;
        state->returned -= diff;
        map->count     = newCount;
        state->removed = true;
        HASH_LOG("Removed %d nodes in iterator, map now has %d nodes", diff, map->count);
    }
    else
    {
        ESP_LOGW("HashMap", "WARN: Count didn't change on remove for some reason?");
    }

    state->curListNode = nextListNode;
    if (nextListNode == NULL)
    {
        result = hashIterNext(map, iter);
    }
    else
    {
        state->curNode = nextListNode->val;
        result         = true;
    }

    return result;
}

/**
 * @brief Reset the given iterator struct, freeing any memory associated with it
 *
 * @param iterator A pointer to the iterator struct to be reset
 */
void hashIterReset(hashIterator_t* iterator)
{
    if (iterator->_state)
    {
        free(iterator->_state);
    }

    iterator->_state = NULL;
    iterator->key    = NULL;
    iterator->value  = NULL;
}

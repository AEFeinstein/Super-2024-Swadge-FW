//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <esp_log.h>
#include <esp_random.h>
#include <esp_heap_caps.h>

#include "linked_list.h"

//==============================================================================
// Defines
//==============================================================================

#ifdef TEST_LIST
    #define VALIDATE_LIST(func, line, nl, list, target) validateList(func, line, nl, list, target)
#else
    #define VALIDATE_LIST(func, line, nl, list, target)
#endif

//==============================================================================
// Function Prototypes
//==============================================================================

#ifdef TEST_LIST
static void validateList(const char* func, int line, bool nl, list_t* list, node_t* target);
#endif

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Add to the end of the list
 *
 * @param list The list to add to
 * @param val The value to be added
 */
void push(list_t* list, void* val)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    node_t* newLast = heap_caps_malloc(sizeof(node_t), MALLOC_CAP_8BIT);
    newLast->val    = val;
    newLast->next   = NULL;
    newLast->prev   = list->last;

    if (list->length == 0)
    {
        list->first = newLast;
        list->last  = newLast;
    }
    else
    {
        list->last->next = newLast;
        list->last       = newLast;
    }
    list->length++;
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
}

/**
 * @brief Remove from the end of the list
 *
 * @param list The list to remove the last node from
 * @return The value from the last node
 */
void* pop(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    void* retval = NULL;

    // Get a direct pointer to the node we're removing
    node_t* target = list->last;

    // If the list any nodes at all
    if (target != NULL)
    {
        // Adjust the node before the removed node, then adjust the list's last pointer
        if (target->prev != NULL)
        {
            target->prev->next = NULL;
            list->last         = target->prev;
        }
        // If the list has only one node, clear the first and last pointers
        else
        {
            list->first = NULL;
            list->last  = NULL;
        }

        // Get the last node val, then free it and update length
        retval = target->val;
        free(target);
        list->length--;
    }

    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
    return retval;
}

/**
 * @brief Add to the front of the list
 *
 * @param list The list to add to
 * @param val The value to add to the list
 */
void unshift(list_t* list, void* val)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    node_t* newFirst = heap_caps_malloc(sizeof(node_t), MALLOC_CAP_8BIT);
    newFirst->val    = val;
    newFirst->next   = list->first;
    newFirst->prev   = NULL;

    if (list->length == 0)
    {
        list->first = newFirst;
        list->last  = newFirst;
    }
    else
    {
        list->first->prev = newFirst;
        list->first       = newFirst;
    }
    list->length++;
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
}

/**
 * @brief Remove from the front of the list
 *
 * @param list The list to remove from
 * @return The value from the first node
 */
void* shift(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    void* retval = NULL;

    // Get a direct pointer to the node we're removing
    node_t* target = list->first;

    // If the list any nodes at all
    if (target != NULL)
    {
        // Adjust the node after the removed node, then adjust the list's first pointer
        if (target->next != NULL)
        {
            target->next->prev = NULL;
            list->first        = target->next;
        }
        // If the list has only one node, clear the first and last pointers
        else
        {
            list->first = NULL;
            list->last  = NULL;
        }

        // Get the first node val, then free it and update length
        retval = target->val;
        free(target);
        list->length--;
    }

    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
    return retval;
}

/**
 * @brief Add at an index in the list
 *
 * @param list The list to add to
 * @param val The value to add
 * @param index The index to add the value at
 * @return true if the value was added, false if the index was invalid
 */
bool addIdx(list_t* list, void* val, uint16_t index)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    // If the index is 0, we're adding to the start of the list
    if (index == 0)
    {
        unshift(list, val);
    }
    // Else if the index is the length, we're adding to the end of the list
    else if (index == list->length - 1)
    {
        push(list, val);
    }
    // Else if the index we're trying to add to is before the end of the list
    else if (index < list->length - 1)
    {
        node_t* newNode = heap_caps_malloc(sizeof(node_t), MALLOC_CAP_8BIT);
        newNode->val    = val;
        newNode->next   = NULL;
        newNode->prev   = NULL;

        node_t* current = list->first;
        for (uint16_t i = 0; i < index - 1; i++)
        {
            current = current->next;
        }

        // We need to adjust the newNode, and the nodes before and after it
        // current is set to the node before it

        current->next->prev = newNode;
        newNode->next       = current->next;

        current->next = newNode;
        newNode->prev = current;

        list->length++;
    }
    else
    {
        return false;
    }
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
    return true;
}

/**
 * @brief Insert a value into the list immediately before the given node.
 *
 * If the given node is NULL, inserts at the end of the list
 *
 * @param list  The list to add the entry to
 * @param val   The new value to add to the list
 * @param entry The existing entry, after which to insert the value
 */
void addBefore(list_t* list, void* val, node_t* entry)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);

    if (entry == list->first)
    {
        // Add at head
        unshift(list, val);
    }
    else if (entry == NULL)
    {
        push(list, val);
    }
    else
    {
        node_t* prev    = entry->prev;
        node_t* newNode = heap_caps_malloc(sizeof(node_t), MALLOC_CAP_8BIT);
        newNode->val    = val;
        newNode->prev   = prev;
        newNode->next   = entry;

        if (prev)
        {
            prev->next = newNode;
        }
        entry->prev = newNode;
        list->length++;
    }
}

/**
 * @brief Insert a value into the list immediately after the given node.
 *
 * If the given node is NULL, inserts at the beginning of the list
 *
 * @param list  The list to add the entry to
 * @param val   The new value to add to the list
 * @param entry The existing entry, after which to insert the value
 */
void addAfter(list_t* list, void* val, node_t* entry)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);

    if (entry == list->last)
    {
        push(list, val);
    }
    else if (entry == NULL)
    {
        // Add at head
        unshift(list, val);
    }
    else
    {
        node_t* next    = entry->next;
        node_t* newNode = heap_caps_malloc(sizeof(node_t), MALLOC_CAP_8BIT);
        newNode->val    = val;
        newNode->prev   = entry;
        newNode->next   = next;

        if (next)
        {
            next->prev = newNode;
        }
        entry->next = newNode;
        list->length++;
    }
}

/**
 * @brief Remove at an index in the list
 *
 * @param list The list to remove from
 * @param index The index to remove the value from
 * @return The value that was removed. May be NULL if the index was invalid
 */
void* removeIdx(list_t* list, uint16_t index)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, (void*)((intptr_t)index));
    // If the list is null or empty, dont touch it
    if (NULL == list || list->length == 0)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return NULL;
    }
    // Else if the index we're trying to remove from is the start of the list
    else if (index == 0)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return shift(list);
    }
    // Else if the index we're trying to remove from is the end of the list
    else if (index == list->length - 1)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return pop(list);
    }
    // Else if the index we're trying to remove from is before the end of the list
    else if (index < list->length - 1)
    {
        void* retval = NULL;

        node_t* current = list->first;
        for (uint16_t i = 0; i < index - 1; i++)
        {
            current = current->next;
        }

        // We need to free the removed node, and adjust the nodes before and after it
        // current is set to the node before it

        node_t* target = current->next;
        retval         = target->val;

        current->next       = target->next;
        current->next->prev = current;

        free(target);
        target = NULL;

        list->length--;
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return retval;
    }
    else
    {
        // Index was invalid
        return NULL;
    }
}

/**
 * Remove a specific entry from the linked list by ::node_t.
 * This relinks the entry's neighbors, but does not validate that it was part of the given ::list_t.
 * If the given ::node_t was not part of the given ::list_t, the list length will desync.
 *
 * @param list The list to remove an entry from
 * @param entry The entry to remove
 * @return The removed value from the entry, may be NULL if the entry was invalid
 */
void* removeEntry(list_t* list, node_t* entry)
{
    // Don't do anything with a NULL entry
    if (NULL == entry)
    {
        return NULL;
    }

    VALIDATE_LIST(__func__, __LINE__, true, list, entry);

    // Get references to this node's previous and next nodes
    node_t* prev = entry->prev;
    node_t* next = entry->next;

    // Adjust first and last, if necessary
    if (list->first == entry)
    {
        list->first = entry->next;
    }
    if (list->last == entry)
    {
        list->last = entry->prev;
    }

    // Relink previous and next nodes, if able
    if (NULL != prev)
    {
        prev->next = next;
    }
    if (NULL != next)
    {
        next->prev = prev;
    }

    // Decrement list length
    list->length--;

    // Save the value
    void* retVal = entry->val;

    VALIDATE_LIST(__func__, __LINE__, false, list, entry);

    // free the memory
    free(entry);

    // Return the value
    return retVal;
}

/**
 * @brief Remove all items from the list
 *
 * @param list The list to clear
 */
void clear(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    while (list->first != NULL)
    {
        pop(list);
    }
    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
}

#ifdef TEST_LIST

/**
 * @brief Debug print and validate the list
 *
 * @param func The calling function
 * @param line The calling line
 * @param nl true to print a newline before the list, false to not
 * @param list The list to validate
 * @param target The target for the operation, may be NULL
 */
static void validateList(const char* func, int line, bool nl, list_t* list, node_t* target)
{
    if (nl)
    {
        printf("\n");
    }
    ESP_LOGD("VL", "%s::%d, len: %d (%p)", func, line, list->length, target);
    node_t* currentNode = list->first;
    node_t* prev        = NULL;
    int countedLen      = 0;

    if (NULL != list->first && NULL != list->first->prev)
    {
        ESP_LOGE("VL", "Node before first not NULL");
        exit(1);
    }

    if (NULL != list->last && NULL != list->last->next)
    {
        ESP_LOGE("VL", "Node after last not NULL");
        exit(1);
    }

    while (currentNode != NULL)
    {
        ESP_LOGD("VL", "%p -> %p -> %p", currentNode->prev, currentNode, currentNode->next);
        if (prev != currentNode->prev)
        {
            ESP_LOGE("VL", "Linkage error %p != %p", currentNode->prev, prev);
            exit(1);
        }
        prev        = currentNode;
        currentNode = currentNode->next;
        countedLen++;
    }

    if (countedLen != list->length)
    {
        ESP_LOGE("VL", "List mismatch, list says %d, counted %d", list->length, countedLen);
        exit(1);
    }
}

/**
 * @brief Exercise the linked list code by doing lots of random operations
 */
void listTester(void)
{
    list_t testList = {.first = NULL, .last = NULL, .length = 0};
    list_t* l       = &testList;

    // Seed the list
    for (int i = 0; i < 25; i++)
    {
        push(l, NULL);
    }

    for (int64_t i = 0; i < 100000000; i++)
    {
        if (0 == i % 10000)
        {
            printf("link tester %" PRIu64 "\n", i);
        }
        switch (esp_random() % 8)
        {
            case 0:
            {
                push(l, NULL);
                break;
            }
            case 1:
            {
                pop(l);
                break;
            }
            case 2:
            {
                unshift(l, NULL);
                break;
            }
            case 3:
            {
                shift(l);
                break;
            }
            case 4:
            {
                // Add to random valid index
                int idx = 0;
                if (l->length)
                {
                    idx = esp_random() % l->length;
                }
                addIdx(l, NULL, idx);
                break;
            }
            case 5:
            {
                // Remove from random valid index
                int idx = 0;
                if (l->length)
                {
                    idx = esp_random() % l->length;
                }
                removeIdx(l, idx);
                break;
            }
            case 6:
            {
                // Remove random valid node
                int idx = 0;
                if (l->length)
                {
                    idx = esp_random() % l->length;
                }
                node_t* node = l->first;
                while (idx--)
                {
                    node = node->next;
                }
                removeEntry(l, node);
                break;
            }
            case 7:
            {
                // clear(l);
                break;
            }
        }
    }
    ESP_LOGD("LV", "List validated");
}

#endif
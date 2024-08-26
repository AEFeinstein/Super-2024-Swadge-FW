/*! \file linked_list.h
 * \date Sept 12, 2019
 * \author Jonathan Moriarty
 *
 * \section linked_list_design Design Philosophy
 *
 * This is a basic doubly linked list data structure.
 * Each entry in the list has a \c void* which can point to any data.
 *
 * Data can be added or removed from the head or tail in O(1) time, making this suitable as a queue or stack.
 *
 * \section linked_list_usage Usage
 *
 * push() and pop() add and remove from the tail of the list.
 *
 * unshift() and shift() add and remove from the head of the list.
 *
 * addIdx() and removeIdx() add and remove from the middle of the list, by index. These run in O(N), not O(1).
 *
 * removeEntry() can remove a specific entry.
 *
 * Links are allocated, so when done with a list, be sure to call clear() when done.
 *
 * \section linked_list_example Example
 *
 * Creating an empty list:
 * \code{.c}
 * // Use calloc to ensure members are all 0 or NULL
 * list_t* myList = calloc(1, sizeof(list_t));
 * \endcode
 *
 * Adding values to a list:
 * \code{.c}
 * // Malloc the value to be persistent
 * // push to tail
 * uint32_t* val1 = malloc(sizeof(uint32_t));
 * *val1          = 1;
 * push(myList, (void*)val1);
 * // unshift to head
 * uint32_t* val2 = malloc(sizeof(uint32_t));
 * *val2          = 2;
 * unshift(myList, (void*)val2);
 * \endcode
 *
 * Iterating over a list:
 * \code{.c}
 * // Iterate over all nodes
 * node_t* currentNode = myList->first;
 * while (currentNode != NULL)
 * {
 *     // Print the nodes
 *     printf("%d, ", *((uint32_t*)currentNode->val));
 *     currentNode = currentNode->next;
 * }
 * \endcode
 *
 * Removing values from a list:
 * \code{.c}
 * // Remove from head
 * uint32_t* shiftedVal = shift(myList);
 * // Remove from tail
 * uint32_t* poppedVal = pop(myList);
 * \endcode
 */

#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief A node in a doubly linked list with pointers to the previous and next values (which may be NULL), and a \c
 * void* to arbritray data
 */
typedef struct node
{
    void* val;         ///< A pointer to the data for this node.
    struct node* next; ///< The next node in the list
    struct node* prev; ///< The previous node in the list
} node_t;

/**
 * @brief A doubly linked list with pointers to the first and last nodes
 */
typedef struct
{
    node_t* first; ///< The first node in the list
    node_t* last;  ///< The last node in the list
    int length;    ///< The number of nodes in the list
} list_t;

void push(list_t* list, void* val);
void* pop(list_t* list);
void unshift(list_t* list, void* val);
void* shift(list_t* list);
bool addIdx(list_t* list, void* val, uint16_t index);
void addBefore(list_t* list, void* val, node_t* entry);
void addAfter(list_t* list, void* val, node_t* entry);
void* removeIdx(list_t* list, uint16_t index);
void* removeEntry(list_t* list, node_t* entry);
void clear(list_t* list);

#ifdef TEST_LIST
// Exercise the linked list functions
void listTester(void);
#endif

#endif

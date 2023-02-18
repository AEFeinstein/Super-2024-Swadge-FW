/*! \file linked_list.h
 * \date Sept 12, 2019
 * \author Jonathan Moriarty
 *
 * \section linked_list_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section linked_list_usage Usage
 *
 * TODO doxygen
 *
 * \section linked_list_example Example
 *
 * Creating an empty list:
 * \code{.c}
 * list_t * myList = malloc(sizeof(list_t));
 * myList->first = NULL;
 * myList->last = NULL;
 * myList->length = 0;
 * \endcode
 *
 * Iterating through the list:
 * \code{.c}
 * node_t * currentNode = landedTetrads->first;
 * while (currentNode != NULL)
 * {
 *     //Perform desired operations with current node.
 *     currentNode = currentNode->next;
 * }
 * \endcode
 */

#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

// Doubly-linked list.
typedef struct node
{
    void* val;         ///< A pointer to the data for this node.
    struct node* next; ///< The next node in the list
    struct node* prev; ///< The previous node in the list
} node_t;

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
void add(list_t* list, void* val, int index);
void* removeIdx(list_t* list, int index);
void* removeEntry(list_t* list, node_t* entry);
void clear(list_t* list);

#ifdef TEST_LIST
// Exercise the linked list functions
void listTester(void);
#endif

#endif

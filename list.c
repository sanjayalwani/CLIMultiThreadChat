//Assignment 1: List Datatype
//By Sanjay Alwani
//Implements the functions from list.h header file
#include "list.h"
//stddef or stdlib needed for NULL
#include <stddef.h>
#include <stdlib.h>

//These two bad boys will be initialized once and be handled by our static 'memory API'
static List List_pool[LIST_MAX_NUM_HEADS] = { {0} };
static Node Node_pool[LIST_MAX_NUM_NODES] = { {0} };

//MEMORY API, Releases and absorbs free nodes and heads
//Heads are set to active or inactive by this layer

//Two separate types of managers used due to differing behaviour, modularity not worth it for the scope of this assignment
//Free nodes tracking data structure static to this file
typedef struct NodeManager_s NodeManager;
struct NodeManager_s {
    Node* top;
    int size;
};
//Free heads tracking data structure static to this file
typedef struct HeadManager_s HeadManager;
struct HeadManager_s {
    List* top;
    int size;
};

//Static declaration of structs that track da stack
static NodeManager Free_nodes_controller;
static HeadManager Free_heads_controller;

//Setup Free_nodes and Free_heads once with a boolean flag
static bool setupComplete = false;
static void oneTimeSetup()
{
    /*
    FREE NODE SETUP
    */
    //Node manager setup links up to the last node
    for(int n = 0; n < (LIST_MAX_NUM_NODES-1); n++){
        Node_pool[n].next = (Node_pool + n +1);
    }
    //For last node set next to null
    Node_pool[(LIST_MAX_NUM_NODES-1)].next = NULL;
    //We don't really care about prev pointer in FREE MODE as nodes belong to the stack 

    //member declarations
        //Returns first node address
    Free_nodes_controller.top = Node_pool;
        //Number of free nodes is max nodes
    Free_nodes_controller.size = LIST_MAX_NUM_NODES;

    /*
    FREE HEAD SETUP
    */
    //Link heads by head pointer which is assigned a void* for this dual-purpose
    for(int n = 0; n < (LIST_MAX_NUM_HEADS-1); n++){
        List_pool[n].head = (List_pool + n+1);
    }
    //For last head set head to null
    List_pool[(LIST_MAX_NUM_HEADS-1)].head = NULL;
    //We don't really care about tail pointer for free space as we treat it like a stack

    //member declarations
        //Returns first index address
    Free_heads_controller.top = List_pool;
        //Number of free nodes is max nodes
    Free_heads_controller.size = LIST_MAX_NUM_HEADS;

    setupComplete = true;
}

static Node* popNode( )
{
    //Check if nodes are exhausted
    if( Free_nodes_controller.size == 0 ){
        return NULL;
    }
    //Else basic stack pop operation
    Node* current_top = Free_nodes_controller.top;
    Free_nodes_controller.top = Free_nodes_controller.top->next;
    Free_nodes_controller.size--;
    return current_top;
}

static void pushNode( Node* pNode )
{
    //Free node's next element is the current top of stack
    pNode->next = Free_nodes_controller.top;
    //Freed node is now the top
    Free_nodes_controller.top = pNode;
    //Size increases
    Free_nodes_controller.size++;
    return; 
}

static List* popHead( )
{
    //Check if nodes are exhausted
    if( Free_heads_controller.size == 0 ){
        return NULL;
    }
    //Else simple stack pop implementation with active switch trigerred

    List* current_top = Free_heads_controller.top;
    Free_heads_controller.top = Free_heads_controller.top->head;
    Free_heads_controller.size--;
    current_top->active = true;
    return current_top;
}

static void pushHead( List* pList )
{
    //Add pList to top of stack and set as unactive
    pList->active = false;
    pList->head = Free_heads_controller.top;
    pList->tail = NULL;
    pList->size = 0;
    Free_heads_controller.top = pList;
    Free_heads_controller.size++;
    return; 
}

// Makes a new, empty list, and returns its reference on success. 
// Returns a NULL pointer on failure.
List* List_create()
{
    //Setup logic
    if(!setupComplete){
        oneTimeSetup();
    }
    //Pop head
    List* NewListPointer = popHead();
    //If no lists available NULL returned
    if (NewListPointer==NULL){
        return NewListPointer;
    }
    
    //Instantiation of new list, active status handled by controller
    //Reset head to null not part of free head chain anymore
    NewListPointer->head = NULL;
    NewListPointer->tail = NULL;
    NewListPointer->currentNode = NULL;
    NewListPointer->currentPos = 0;
    NewListPointer->size = 0;

    return NewListPointer;
}

// Returns the number of items in pList.
int List_count(List* pList)
{
    return pList->size;
}

// Returns a pointer to the first item in pList and makes the first item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_first(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }
    
    //Returns NULL if empty
    Node* currentNodePointer = pList->head;
    if ( currentNodePointer == NULL ){
        pList->currentNode = NULL;
        return NULL;
    }
    
    //Passes first node pointer or NULL
    pList->currentNode = currentNodePointer;
    pList->currentPos = 0;
    //returns list head pointer value
    return currentNodePointer->item;
}

// Returns a pointer to the last item in pList and makes the last item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_last(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }

    //Returns NULL if empty
    Node* currentNodePointer = pList->tail;
    if ( currentNodePointer == NULL ){
        pList->currentNode = NULL;
        return NULL;
    }
    //Sets currentNode as tail
    pList->currentNode = currentNodePointer;
    pList->currentPos = 0;
    //returns list tail pointer value
    return currentNodePointer->item;
}

// Advances pList's current item by one, and returns a pointer to the new current item.
// If this operation advances the current item beyond the end of the pList, a NULL pointer 
// is returned and the current item is set to be beyond end of pList.
void* List_next(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }
    //Pointer is beyond list
    if( pList->currentPos == 1 ){
        return NULL;
        //nothing changes
    }
    //Pointer is before list
    if( pList->currentPos == -1 ){
        return List_first(pList);
    }
    //Pointer is within list
    Node* currentNodePointer = pList->currentNode;
    //To prevent accesing a member of NULL, in the case of new list
    if( currentNodePointer == NULL){
        return NULL;
    }
    //Move current pointer up 1 link
    currentNodePointer = currentNodePointer->next;
    pList->currentNode = currentNodePointer;
    //If it has moved beyond list change position beyond list; return null;
    if( currentNodePointer == NULL){
        pList->currentPos = 1;
        return NULL;
    }
    return currentNodePointer->item;
}

// Backs up pList's current item by one, and returns a pointer to the new current item. 
// If this operation backs up the current item beyond the start of the pList, a NULL pointer 
// is returned and the current item is set to be before the start of pList.
void* List_prev(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }

    //Pointer is before list
    if( pList->currentPos == -1 ){
        return NULL;
        //nothing changes
    }
    //Pointer is beyond list
    if( pList->currentPos == 1 ){
        return List_last(pList);
    }
    //Pointer is within list
    Node* currentNodePointer = pList->currentNode;
    //To prevent accesing a member of NULL
    if( currentNodePointer == NULL){
        return NULL;
    }
    //Move pointer down 1 link
    currentNodePointer = currentNodePointer->prev;
    pList->currentNode = currentNodePointer;
   //If moved before list change currentPos to -1; return null;
    if( currentNodePointer == NULL){
        pList->currentPos=-1;
        return NULL;
    }
    return currentNodePointer->item;
}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }
    //If current node is null... return null
    if( pList->currentNode==NULL ){
        return NULL;
    }
    return pList->currentNode->item;
}

// Adds the new item to pList directly after the current item, and makes item the current item. 
// If the current pointer is before the start of the pList, the item is added at the start. If 
// the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_add(List* pList, void* pItem)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return -1;
    }
    //Node setup before adding to list
    Node* newNode = popNode();
    if( newNode == NULL ){
        return -1;
    }
    //Node was assigned, nothing can go wrong now!
    pList->size++;
    newNode->item = pItem;
    
    //If empty list
    if( pList->head==NULL )
    {
        newNode->next=NULL;
        newNode->prev=NULL;
        pList->head = newNode;
        pList->tail = newNode;
        pList->currentNode = newNode;
        pList->currentPos = 0;
        return 0;
    }
    //List is in bounds
    if( pList->currentPos==0 )
    {
        newNode->next = pList->currentNode->next;
        
        newNode->prev = pList->currentNode;
        //If we have inserted ourselves ahead of tail
        if(pList->tail==pList->currentNode){
            pList->tail = newNode;
        }
        else{ //walk forward is ok
            newNode->next->prev = newNode;
        }
        //walk backward always ok
        newNode->prev->next = newNode;

        pList->currentNode = newNode;
    }
    //current pointer is beyond list
    if( pList->currentPos==1 )
    {
        List_last( pList );
        newNode->next = pList->currentNode->next;
        newNode->prev = pList->currentNode;
        //walk backward
        newNode->prev->next = newNode;
        //we have inserted ourselves ahead of tail
        pList->tail = newNode;
        pList->currentNode = newNode;
        pList->currentPos = 0;
    }
    //current pointer is beyond list
    if( pList->currentPos==-1 )
    {
        List_first( pList );
        newNode->next = pList->currentNode;
        newNode->prev = pList->currentNode->prev;
        //walk forward
        newNode->next->prev = newNode;
        //we have inserted ourselves at start
        pList->head = newNode;
        pList->currentNode = newNode;
        pList->currentPos = 0;
    }
    return 0;
}

// Adds item to pList directly before the current item, and makes the new item the current one. 
// If the current pointer is before the start of the pList, the item is added at the start. 
// If the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert(List* pList, void* pItem)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return -1;
    }
    //Node setup before adding to list
    Node* newNode = popNode();
    if( newNode == NULL ){
        return -1;
    }
    //Node was assigned, nothing can go wrong now!
    pList->size++;
    newNode->item = pItem;

    //If empty list
    if( pList->head==NULL )
    {
        newNode->next=NULL;
        newNode->prev=NULL;
        pList->head = newNode;
        pList->tail = newNode;
        pList->currentNode = newNode;
        pList->currentPos = 0;
        return 0;
    }
    //List is in bounds
    if( pList->currentPos==0 )
    {
        //assign pointers for new node
        newNode->next = pList->currentNode;
        newNode->prev = pList->currentNode->prev;

        //If we have inserted ourselves behind head node
        if(pList->currentNode == pList->head){
            pList->head = newNode;
        }
        else{ //walk backward is ok
            newNode->prev->next = newNode;
        }
        //walk forward always ok
        newNode->next->prev = newNode;

        pList->currentNode = newNode;
    }
    //current pointer is beyond list
    if( pList->currentPos==1 )
    {
        List_last( pList );
        newNode->next = pList->currentNode;
        newNode->prev = pList->currentNode->prev;
        
        //If list has only one node ( which is also the head )
        if(pList->currentNode == pList->head){
            pList->head = newNode;
        }
        else{ //walk backward is ok
            newNode->prev->next = newNode;
        }
        //walk forward
        newNode->next->prev = newNode;

        pList->currentNode = newNode;
        pList->currentPos = 0;
    }
    //current pointer is beyond list
    if( pList->currentPos==-1 )
    {
        List_first( pList );
        newNode->next = pList->currentNode;
        newNode->prev = pList->currentNode->prev;
        //walk forward
        newNode->next->prev = newNode;
        //we have inserted ourselves at start
        pList->head = newNode;
        pList->currentNode = newNode;
        pList->currentPos = 0;
    }
    return 0;
}

// Adds item to the end of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_append(List* pList, void* pItem)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return -1;
    }
    List_last( pList );
    return List_add( pList, pItem);
}
// Adds item to the front of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_prepend(List* pList, void* pItem)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return -1;
    }
    List_first( pList );
    return List_insert( pList, pItem );
}

// Return current item and take it out of pList. Make the next item the current one.
// If the current pointer is before the start of the pList, or beyond the end of the pList,
// then do not change the pList and return NULL.
void* List_remove(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }

    if( pList->currentNode == NULL ){
        return NULL;
    }
    Node* nodeToRemove = pList->currentNode;
    void* currentItem = nodeToRemove->item;
    
    //--List relinking process
    
    //If node is at end of list
    if( nodeToRemove->next == NULL ){
        pList->currentPos = 1;
        pList->tail = nodeToRemove->prev;
    } else {
        pList->currentPos = 0;
        nodeToRemove->next->prev = nodeToRemove->prev;
    }
    //If node is at the start
    if( nodeToRemove->prev == NULL ){
        pList->head = nodeToRemove->next;
    } else {
        nodeToRemove->prev->next = nodeToRemove->next;
    }
    pList->currentNode = nodeToRemove->next;

    //--Node freeing process
    pushNode( nodeToRemove );
    pList->size--;
    return currentItem;
}

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1. 
// pList2 no longer exists after the operation; its head is available
// for future operations.
void List_concat(List* pList1, List* pList2)
{
    //If you try to abuse my freed list pointer
    if ( (!pList1->active)||(!pList2->active) ){
        return;
    }

    //If list1 is empty
    if(pList1->head==NULL)
    {
        pList1->head = pList2->head;
        pList1->size = pList2->size;
        pList1->tail = pList2->tail;
        pushHead( pList2 );
        return;
    }
    //If list2 is empty
    else if(pList2->head==NULL)
    {
        //DO NOTHING
    }
    else
    {
        //Node link through lists
        pList1->tail->next = pList2->head;
        ((Node*)pList2->head)->prev = pList1->tail;
        //Increment size
        pList1->size += pList2->size;
        pList1->tail = pList2->tail;
    }

    pushHead( pList2 );
}

// Delete pList. pItemFreeFn is a pointer to a routine that frees an item. 
// It should be invoked (within List_free) as: (*pItemFreeFn)(itemToBeFreedFromNode);
// pList and all its nodes no longer exists after the operation; its head and nodes are 
// available for future operations.
// UPDATED: Changed function pointer type, May 19
//typedef void (*FREE_FN)(void* pItem);

void List_free(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return;
    }

    //If list is not empty then we must free items and nodes
    if( pList->head != NULL )
    {
        Node* curr = pList->head;
        while(curr!=NULL)
        {
            //Free item
            free(curr->item);
            curr->item = NULL;
            //Save address of node to be freed as pushNode mutates Nodes
            Node* toFree = curr;
            //Acess next pointer of Node
            curr = curr->next;
            //Then free node
            pushNode(toFree);
        }
    }

    pushHead( pList );
}

// Return last item and take it out of pList. Make the new last item the current one.
// Return NULL if pList is initially empty.
void* List_trim(List* pList)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }

    //If list empty
    if(pList->head==NULL){
        return NULL;
    }
    //Make last item current
    List_last( pList );
    //Remove last item and save item value
    void* toReturn =  List_remove( pList );
    //Reset current to new Last item
    List_last( pList);
    return toReturn;
}

// Search pList, starting at the current item, until the end is reached or a match is found. 
// In this context, a match is determined by the comparator parameter. This parameter is a
// pointer to a routine that takes as its first argument an item pointer, and as its second 
// argument pComparisonArg. Comparator returns 0 if the item and comparisonArg don't match, 
// or 1 if they do. Exactly what constitutes a match is up to the implementor of comparator. 
// 
// If a match is found, the current pointer is left at the matched item and the pointer to 
// that item is returned. If no match is found, the current pointer is left beyond the end of 
// the list and a NULL pointer is returned.
// 
// UPDATED: Added clarification of behaviour May 19
// If the current pointer is before the start of the pList, then start searching from
// the first node in the list (if any).
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg)
{
    //If you try to abuse my freed list pointer
    if ( !pList->active ){
        return NULL;
    }

    //if empty list
    if( pList->head == NULL ){
        return NULL;
    }
    bool found = false;
    void* currentItem = NULL;
    
    //If before list set current pointer to first
    if(pList->currentPos == -1){
        currentItem = List_first( pList );
    } else { //Else set to current
        currentItem = List_curr( pList );
    }
     
    while( (currentItem != NULL ) && !found )
    {
        if( (*pComparator)( currentItem, pComparisonArg) == 1 ){
            found = true;
        } else {
            currentItem = List_next( pList );
        }
    }
    
    return currentItem;
}

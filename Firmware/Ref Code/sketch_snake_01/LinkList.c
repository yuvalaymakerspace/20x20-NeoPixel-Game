#ifndef _LL_FUNC_C_
#define _LL_FUNC_C_

#include<stdio.h>
#include<stdlib.h>
#include "LinkList.h"

typedef struct ll      // creating structure to handle structures of link list
{
  void *node;     // stores the node pointer of other structures of link list
  struct ll *next;    // stores the value of next node point in link list
}club;


void * InsertAtFirst(void *link,void *nptr ){
  club *cptr;
  cptr=(club*)malloc(sizeof(club));    // allocation memory for structure

  if(NULL == cptr)
    return NULL;
    
  cptr->node=nptr;        // assigning node structure pointer
  cptr->next=link;
  printf("Inserted Node");
  return cptr;
}
void * RemoveTail(void *link){ 
   if(NULL == link)
    return NULL;
    
  club * ptr = link;
  club * prev = NULL;

  while(NULL != ptr->next)
  {
    prev = ptr;
    ptr = ptr->next;
  }

  if(NULL != prev)
    prev->next = NULL;

  free(ptr);
 
  if(ptr == link)
    return NULL;
  else
    return link;
    
  
}

void * InsertAtLast(void *link,void *nptr)
{
  club *ptr,*cptr;
  cptr=(club*)malloc(sizeof(club));    // allocation memory for structure
  cptr->node=nptr;        // assigning node structure pointer
  cptr->next=NULL;
  ptr=link;

  if(NULL == ptr)
  {   
    link=cptr;      
  }
  else
  {
    while(NULL != ptr->next)
    {
      ptr = ptr->next;
    } 
    ptr->next=cptr;
  }
    
  return link;          //returning the starting pointer of link list
}

unsigned int ListLength(void * link)
{
  if(NULL == link)
    return 0;

  club *ptr = link;
  unsigned int listMem = 1;
  while(NULL != ptr->next)
  {
    listMem++;
    ptr = ptr->next;
  }
  
  return listMem;
}

void * RemoveHead(void * link)
{
  if(NULL == link)
    return link;

   club * ptr = link;

   link = ptr->next;
   free(ptr);

   return link;
}

void * GetNext(void * link)
{
   if(NULL == link)
    return link;

   club * ptr = link;

   return ptr->next;
  
}
void * GetData(void * link){
  if(NULL == link)
    return link;
   club * ptr = link;

   return ptr->node;
}

#endif //_LL_FUNC_C_

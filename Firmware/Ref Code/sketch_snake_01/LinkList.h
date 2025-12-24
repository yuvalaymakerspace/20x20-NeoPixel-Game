#ifndef _LL_H_
#define _LL_H_

unsigned int ListLength(void * link);
void * InsertAtLast(void *,void* );
void * RemoveHead(void *);

void * InsertAtFirst(void *,void* );
void * RemoveTail(void *);

void * GetNext(void *);
void * GetData(void *);

#endif _LL_H_

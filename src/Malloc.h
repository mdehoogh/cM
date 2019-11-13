#include <stdlib.h>
#include <stdbool.h>

// a user can mark the allocation by calling Mmark() and using the returned position to unmark
// typically all unmark calls should unmark the most recent mark (otherwise an unmark is missing)
size_t addallocationtype(char allocationtype);
bool allocationrecordinginitialized();
size_t allocationmark();
size_t unmarkallocation(size_t mark);
void allocationreport(size_t mark); // report on the current allocation status
void syncallocations();

// changed to always use my Mmalloc, Mcalloc, Mfree unless a truely production version is intended
// i.e. replacing __ADEBUG__ by __PRODUCTION__ and changing the sign
#ifndef __PRODUCTION__
void* Mmalloc(size_t size,char type);
void* Mcalloc(size_t nitems,size_t size,char type);
void Mfree(void* ptr,char type);
// use the substitutes
#define MALLOC(size,type) Mmalloc(size,type)
#define CALLOC(nitems,size,type) Mcalloc(nitems,size,type)
#define FREE(ptr,type) Mfree(ptr,type)
#else
// use the system methods
#define MALLOC(size,type) malloc(size)
#define CALLOC(nitems,size,type) calloc(nitems,size)
#define FREE(ptr,type) free(ptr)
#endif
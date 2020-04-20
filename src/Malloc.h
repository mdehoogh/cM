#include <stdlib.h>
#include <stdbool.h>

#include "Mmessage.h"

// MDH@14APR2020
typedef unsigned long long t_count;

// MDH@13APR2020: we're going to keep histograms for each of the allocation type
//                we can make a union to distinguish between fixed size and variable size allocations
typedef struct{
    size_t  size; // the 'id' of the allocation class
    t_count count;
}t_allocationsize;

// when dealing with a variable size allocation type, we're storing 
typedef union{
    size_t size; // the size of any fixed size allocation type
    t_allocationsize* _allocationsizes;
}t_allocationsizeunion;

typedef struct{
    char type;
    t_count occupied; // number of bytes occupied
    t_count freed; // number of bytes freed
    t_count mark_occupied; // marked number of bytes occupied
    t_count mark_freed; // marked number of bytes freed
    t_count count; // either the total number of fixed size allocations, or the total number of size categories
    t_allocationsizeunion allocationsizeunion; // either the size of a fixed size allocation type of a pointer to the allocation sizes of a variable type allocation type
}t_allocationtype;

// a user can mark the allocation by calling Mmark() and using the returned position to unmark
// typically all unmark calls should unmark the most recent mark (otherwise an unmark is missing)
t_count addAllocation(char allocationType,size_t size,t_count count); // MDH@09APR2020: perhaps nitems should always be 1 somehow?????????
// t_count registerAllocation(char allocationType,size_t size,t_count count);

bool allocationRecordingInitialized();
t_count allocationmark();
t_count unmarkallocation(t_count mark);
void allocationreport(t_count mark); // report on the current allocation status
void syncallocations();

// MDH@15NOV2019: keeping track of the allocation counts and the allocation types
t_count getNumberOfAllocationTypes();
t_count* _getAllocationCounts();
t_allocationtype* _getAllocationTypes();
bool resetAllocationTypes();
void markAllocationCounts();
long long getAllocationTypeAllocated(char allocationType);
long long getAllocationTypeFreed(char allocationType);

// changed to always use my Mmalloc, Mcalloc, Mfree unless a truely production version is intended
// i.e. replacing __ADEBUG__ by __PRODUCTION__ and changing the sign
#ifndef __PRODUCTION__
void* Mmalloc(size_t size,char type);
void* Mcalloc(size_t size,char type);
void* Mrealloc(void* ptr,t_count from_count,t_count to_count,size_t size,char type);
void Mfree(void* ptr,char type); // releasing a single item of a fixed size allocation type
// use the substitutes
#define MALLOC(size,type) Mmalloc((size),(type))
#define CALLOC(size,type) Mcalloc((size),(type))
#define REALLOC(ptr,from_count,to_count,size,type) Mrealloc((ptr),(from_count),(to_count),(size),(type))
#define FREE(ptr,type) Mfree((ptr),(type))
#else
// use the system methods
#define MALLOC(size,type) malloc((nitems)*(size))
#define CALLOC(size,type) calloc(1,(size))
#define FREE(ptr,type) free(ptr)
#define REALLOC(ptr,from_nitems,to_nitems,size,type) realloc((ptr),(to_nitems)*(size))
#endif


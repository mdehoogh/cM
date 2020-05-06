#include <stdlib.h>
#include <stdbool.h>

#include "Mmessage.h"

// MDH@14APR2020
// typedef unsigned t_count long long;

// MDH@13APR2020: we're going to keep histograms for each of the allocation type
//                we can make a union to distinguish between fixed size and variable size allocations
typedef struct{
    size_t class; // the 'id' of the allocation class
    unsigned long long count; // how many we have of this 'size'
}Mallocationsize;

// when dealing with a variable size allocation type, we're storing 
typedef union{
    size_t size; // the size of any fixed size allocation type
    Mallocationsize* _allocationsizes;
}Mallocationsizeunion;

typedef struct{
    char type;
    unsigned long long occupied; // number of bytes occupied
    unsigned long long freed; // number of bytes freed
    unsigned long long mark_occupied; // marked number of bytes occupied
    unsigned long long mark_freed; // marked number of bytes freed
    long long count; // counting up for fixed-size allocation, and down for variable-size allocation (so we can distinguish between them!!!)
    Mallocationsizeunion allocationsizeunion; // either the size of a fixed size allocation type of a pointer to the allocation sizes of a variable type allocation type
}Mallocationtype;

// a user can mark the allocation by calling Mmark() and using the returned position to unmark
// typically all unmark calls should unmark the most recent mark (otherwise an unmark is missing)
long long addAllocation(char allocationType/*,size_t size*//*,long long count*/); // MDH@09APR2020: perhaps nitems should always be 1 somehow?????????
// long long registerAllocation(char allocationType,size_t size,long long count);

bool allocationRecordingInitialized();
long long allocationmark();
long long unmarkallocation(long long mark);
void allocationreport(long long mark); // report on the current allocation status
void syncallocations();

// MDH@15NOV2019: keeping track of the allocation counts and the allocation types
long long getNumberOfAllocationTypes();
long long* _getAllocationCounts();
Mallocationtype* _getAllocationTypes();
bool resetAllocationTypes();
void markAllocationCounts();
long long getAllocationTypeAllocated(char allocationType);
long long getAllocationTypeFreed(char allocationType);

// changed to always use my Mmalloc, Mcalloc, Mfree unless a truely production version is intended
// i.e. replacing __ADEBUG__ by __PRODUCTION__ and changing the sign
#ifndef __PRODUCTION__
void* Mmalloc(size_t size,char type);
void* Mcalloc(size_t size,char type);
void* Mrealloc(void* ptr,long long from_count,long long to_count,size_t size,char type);
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

// MDH@04MAY2020: asking for the allocation type sizes
typedef struct{
    char type;
    unsigned long long size;
}Mallocationtypesize;

// for requesting some or all allocation type sizes
Mallocationtypesize* _getAllocationTypeSizes(char const * const types,unsigned long long *numberOfAllocationTypes);




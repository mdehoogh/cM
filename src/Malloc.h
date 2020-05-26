#include <stdlib.h>
#include <stdbool.h>

#include "Mmessage.h"

// MDH@26MAY2020: all variable-size allocation types should be negative, all fixed-size allocation types should be positive
//                to start with this should be used for the Mmalloc, Mcalloc, Mfree functions therefore the type is declared as signed char
//                but the type itself is always stored as unsigned char i.e. abs(type)

// MDH@14APR2020
// typedef unsigned t_count long long;

// MDH@13APR2020: we're going to keep histograms for each of the allocation type
//                we can make a union to distinguish between fixed size and variable size allocations
typedef struct{
    size_t class; // the 'id' of the allocation class
    unsigned long long count; // how many we have of this 'size'
}Mallocationsize;

/*
// when dealing with a variable size allocation type, we're storing 
typedef union{
    size_t size; // the size of any fixed size allocation type
    Mallocationsize* _allocationsizes;
}Mallocationsizeunion;
*/

// MDH@07MAY2020: it's a good idea to be able to keep a stack of occupied/free elements
//                unfortunately each Mallocationtype needs to have a fixed size as all the types are stored in the same structure
//                technically we could store all allocation marks in a separate single length structure, one for each of the allocation types
//                then we can give each allocation mark sequence the same number of marks
//                BUT we want to prevent a lot of shifting
typedef struct{
    unsigned long long occupied; // number of bytes occupied
    unsigned long long freed; // number of bytes freed
}Mallocationmark;

typedef struct{
    signed char type;
    /*
    unsigned long long mark_occupied; // marked number of bytes occupied
    unsigned long long mark_freed; // marked number of bytes freed
    */
    long long count; // counting up for fixed-size allocation, and down for variable-size allocation (so we can distinguish between them!!!)
    size_t size; // the size of each record
    Mallocationsize* _allocationsizes; // only used for variable-size allocations
    /*
    unsigned long long lastMarkIndex; // MDH@07MAY2020: the last mark index (which should be initialized to 0)
    Mallocationmark* _allocationmarks; // a single element to store the pointer to the allocation marks (at least one)
    */
}Mallocationtype;

typedef struct{
    uint8_t disowned:1; // whether or not it's a disowned allocation (so it can get a new owner)
    uint8_t level:7; // the subpointer level
    uint32_t id:24; // the owner id (typically a function or a module itself)
}Mallocationowner;

// a user can mark the allocation by calling Mmark() and using the returned position to unmark
// typically all unmark calls should unmark the most recent mark (otherwise an unmark is missing)
long long addAllocation(signed char allocationType,Mallocationowner owner); // MDH@09APR2020: perhaps nitems should always be 1 somehow?????????
// long long registerAllocation(char allocationType,size_t size,long long count);

bool allocationRecordingInitialized();
/*
long long allocationmark();
long long unmarkallocation(long long mark);
void allocationreport(long long mark); // report on the current allocation status
void syncallocations();
*/
// MDH@15NOV2019: keeping track of the allocation counts and the allocation types
unsigned long long getNumberOfAllocationMarks(); // MDH@11MAY2020: expose the number of allocation marks
long long getNumberOfAllocationTypes();
long long* _getAllocationCounts();
Mallocationtype* _getAllocationTypes();

bool resetAllocationTypes();

long long getAllocationTypeOccupied(signed char allocationType,unsigned long long history);
long long getAllocationTypeFreed(signed char allocationType,unsigned long long history);
// MDH@11MAY2020: allow adding an allocation mark and dropping the oldest one
bool allocationMarkAdded();
bool oldestAllocationMarkDropped();

// changed to always use my Mmalloc, Mcalloc, Mfree unless a truely production version is intended
// i.e. replacing __ADEBUG__ by __PRODUCTION__ and changing the sign

// MDH@18MAY2020: for passing along (pointer) ownership DISOWNED and OWNED are introduced

Mallocationowner Msubowner(Mallocationowner owner,uint8_t level);

#ifndef __PRODUCTION__
// MDH@22MAY2020: the structure used for indicating allocation ownership allowing for a total of 1022 modules (with 0 being the program module), and 2^20-1 function lines per module

//Mallocationowner getOwner(uint16_t module,uint32_t functionId);
void* Mmalloc(size_t size,long long count,signed char type,Mallocationowner owner);
void* Mcalloc(size_t size,long long count,signed char type,Mallocationowner owner);
void* Mrealloc(void* ptr,long long from_count,long long to_count,size_t size,signed char type/*,Mallocationowner owner*/); // MDH@26MAY2020 from now on only to be used to reallocate variable-size types (with negative type)
void Mfree(void* ptr,long long count,signed char type,Mallocationowner owner); // releasing a single item of a fixed size allocation type
// use the substitutes
#define MALLOC(size,count,type,owner) Mmalloc((size),(count),(type),(owner))
#define CALLOC(size,count,type,owner) Mcalloc((size),(count),(type),(owner))
#define FREE(ptr,count,type,owner) Mfree((ptr),(count),(type),(owner))
#define REALLOC(ptr,from_count,to_count,size,type) Mrealloc((ptr),(from_count),(to_count),(size),(type))
#define DISOWNED(ptr,owner) Mdisowned((ptr),(owner))
#define OWNED(ptr,owner) Mowned((ptr),(owner))
#define OWNED_BY(subptr,ptr) Mownedby((subptr),(ptr))
#define SUBOWNED(ptr,level) Msubowned((ptr),(level))
#else
// use the system methods
#define MALLOC(size,type,owner) malloc((nitems)*(size))
#define CALLOC(size,type,owner) calloc(1,(size))
#define FREE(ptr,type,owner) free(ptr)
#define REALLOC(ptr,from_nitems,to_nitems,size,type) realloc((ptr),(to_nitems)*(size))
#define DISOWNED(ptr,owner) (ptr)
#define OWNED(ptr,owner) (ptr)
#define OWNED_BY(ptr,owner) (ptr)
#define SUBOWNED(ptr,level) (ptr)
#endif

// some shortcuts
#define MALLOC_1(size,type,owner) MALLOC((size),1,(type),(counter))
#define CALLOC_1(size,type,owner) CALLOC((size),1,(type),(counter))
#define FREE_1(ptr,type,owner) FREE((ptr),(type),(owner))

// MDH@04MAY2020: asking for the allocation type sizes
// MDH@07MAY2020: we could pass back all the allocation values of all the marks
typedef struct{
    // char type; // we'll be storing the type in the first element of sizes!!!!
    unsigned long long sizes[1]; // at least one size being returned
}Mallocationtypesize;

// for requesting some or all allocation type sizes
long long * _getAllocationTypeSizes(char const * const types,unsigned long long *_numberOfAllocationTypes,long long *_numberOfAllocationMarks);

void outputAllocationTypeMarks();

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "Malloc.h"

// MDH@15NOV2019: want to keep track of the number of allocations for each type (with character id)
t_allocationtype* _allocationTypes=NULL; // the unique allocation type characters
t_count numberOfAllocationTypes=0; // keep track of the number of allocation types
// MDH@14APR2020 now being stored as part of the allocationtypes: size_t* _allocationcounts=NULL; // the size of the type is stored every odd size_t

// helper functions
// we need a local something to store the allocation types in
struct{
    t_count l; // the number of allocation types stored
    char* _chars; // the allocation type characters
}allocations;

// you HAVE to call this method to be able to register allocations
bool allocationRecordingInitialized(){
    printf("Initializing allocation recording...\n");
    allocations.l=0;
#ifndef __PRODUCTION__
    allocations._chars=malloc(16); // starting out with one block
    if(allocations._chars){
        _allocationTypes=calloc(1,sizeof(t_allocationtype));
        if(_allocationTypes){
            numberOfAllocationTypes=1;
            _allocationTypes[0].type='*';
        } // always keep track of the total allocation count, which we mark with a wildcard *
        // MDH@14APR2020: if(numberofallocationtypes>0)_allocationcounts=calloc(5,sizeof(size_t)); // start out with two size_t items one to store the count and one to store the size!!
    }
#else
    allocations._chars=NULL;
#endif
    return(/* superfluous: allocations._chars&&*/_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/);
    // replacing: if(!allocations._chars)printf("ERROR: Failed to initialize recording allocations.\n");else printf("Allocation recording initialized.\n");
}

t_count getNumberOfAllocationTypes(){return (_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/?numberOfAllocationTypes:0);}

static t_count getNewAllocationTypeIndex(char allocationType,size_t size,t_count count,bool fixedsize){
    t_count newAllocationTypeIndex=0;
    if(_allocationTypes){ // meaning we have both _allocationtypes and _allocationcounts
        t_allocationsize* _allocationTypeSizeHistogram=(fixedsize?NULL:calloc(1,sizeof(t_allocationsize)));
        if(fixedsize||_allocationTypeSizeHistogram){
            // how about allocating memory for the histogram beforehand?
            printf("************** New allocation type #%llu: '%c' of size %zd!\n",numberOfAllocationTypes,allocationType,size);
            void* newAllocationTypes=realloc(_allocationTypes,sizeof(t_allocationtype)*(numberOfAllocationTypes+1));
            if(newAllocationTypes){           
                _allocationTypes[numberOfAllocationTypes].type=allocationType;
                _allocationTypes[numberOfAllocationTypes].occupied=0;
                _allocationTypes[numberOfAllocationTypes].freed=0;
                // MDH@14APR2020: _allocationcounts=realloc(_allocationcounts,(sizeof(size_t)*(allocationtypeindex+1))*5); // for every type we store 5 size_t values, one to keep the item count, and one to keep the size
                _allocationTypes=newAllocationTypes;
                if(fixedsize){
                    _allocationTypes[numberOfAllocationTypes].count=count; // the initial count
                    _allocationTypes[numberOfAllocationTypes].allocationsizeunion.size=size;
                }else{
                    _allocationTypes[numberOfAllocationTypes].allocationsizeunion._allocationsizes=_allocationTypeSizeHistogram;
                    _allocationTypes[numberOfAllocationTypes].count=1;
                    _allocationTypes[numberOfAllocationTypes].allocationsizeunion._allocationsizes[0].size=size;
                    _allocationTypes[numberOfAllocationTypes].allocationsizeunion._allocationsizes[0].count=count;            
                }
                newAllocationTypeIndex=numberOfAllocationTypes++; // good to go
            }else{
                if(!fixedsize)free(_allocationTypeSizeHistogram); // MDH@14APR2020: don't forget to free what we've allocated beforehand
                printf("ERROR: Failed to register new allocation type '%c'.\n",allocationType);
            }
        }
        /* MDH@14APR2020:
        // register the size and initialize the count to 0!!!
        _allocationcounts[allocationtypeindex*5]=size; // storing the size in the second element of the pair
        _allocationcounts[(allocationtypeindex*5)+1]=0; // number of allocations
        _allocationcounts[(allocationtypeindex*5)+2]=0; // number of frees
        _allocationcounts[(allocationtypeindex*5)+3]=0; // mark number of allocations
        _allocationcounts[(allocationtypeindex*5)+4]=0; // mark number of frees
        */
    }
    return newAllocationTypeIndex;
}

// MDH@09APR2020: distinguish between adding an allocation (local) and adding an allocationtype (global)
t_count addAllocation(char allocationType,size_t size,t_count count){
    // I suppose that the allocation might fail but we do NOT want to loose allocations._chars over it
    // MDH@14APR2020: there's room for improvement here
    if(allocations._chars){
        t_count newl=allocations.l+count;
        while(newl>allocations.l){
            if(!(allocations.l&0xF)){ // allocations.l is a multiple of 16, so allocation._chars is full and we need a new block
                printf("Expanding allocations.\n");
                char* allocationchars=realloc(allocations._chars,(allocations.l+16));
                if(!allocationchars)return 0; // realloc failure
                allocations._chars=allocationchars;
            }
            allocations._chars[allocations.l++]=allocationType;
            printf("Allocation #%llu of type '%c' registered.\n",allocations.l,allocationType);
        }
    }
    return allocations.l; // returning the current length of allocations (which should be nonzero for sure!!!)
}

static long long getAllocationTypeIndex(char allocationType){
    long long allocationTypeIndex=numberOfAllocationTypes;
    while(--allocationTypeIndex>=0&&_allocationTypes[allocationTypeIndex].type!=allocationType);
    return allocationTypeIndex;
}

// MDH@14APR2020: addAllocationType renamed to registerAllocation
static t_count registerAllocation(char allocationType,size_t size,t_count count,bool fixedsize){
    t_count allocationIndex=0;
    if(allocationType!='\0'&&allocationType!='*'){
        // how about registering the type first if we need to???????
        // increase size if necessary
        if(size>0&&count>0){
            long long allocationTypeIndex=getAllocationTypeIndex(allocationType); // the number of registered allocation types
            if(allocationTypeIndex<0)allocationTypeIndex=getNewAllocationTypeIndex(allocationType,size,count,fixedsize);
            if(allocationTypeIndex>=0){ // yes, we should already have at least one allocation type
                allocationIndex=addAllocation(allocationType,size,count); // this is for registering the allocation BUT TODO should this be done here?????
                /* MDH@14APR2020
                if(allocationIndex>0){
                    // MDH@15NOV2019: add another size_t to _allocationcounts array if we need to
                    _allocationcounts[allocationTypeIndex*5+1]+=nitems; // another nitems allocated
                    _allocationcounts[0]+=(nitems*size); // keep track of the total amount of bytes used
                    _allocationcounts[1]+=nitems; // another nitems allocated
                }
                */
            }
        }
    }
    return allocationIndex;
}

// MDH@15NOV2019: allow access to the allocation counts
// MDH@25NOV2019: let's return copies, so that we can get a frozen snapshot instead of something that can change
t_count* _getAllocationCounts(){
    /* MDH@14APR2020: not used anymore
    size_t sizeallocationcounts=(_allocationcounts?numberofallocationtypes*sizeof(size_t)*5:0);
    return(sizeallocationcounts>0?memcpy((char*)malloc(sizeallocationcounts),_allocationcounts,sizeallocationcounts):NULL);
    */
    return NULL;
}
t_allocationtype* _getAllocationTypes(){
    size_t allocationTypesSize=(_allocationTypes?numberOfAllocationTypes*sizeof(t_allocationtype):0);
    return(allocationTypesSize>0?memcpy(malloc(allocationTypesSize),_allocationTypes,allocationTypesSize):NULL);
}
// MDH@19NOV2019: 
bool resetAllocationTypes(){
    /////printf("Resetting allocation type counts.\n");
    // best to free the lot, the reinitialize
    if(_allocationTypes)free(_allocationTypes);
    // MDH@14APR2020: if(_allocationcounts)free(_allocationcounts);
    if(allocations._chars)free(allocations._chars);
    ////////printf("Allocation type counts reset.\n");
    return allocationRecordingInitialized();
}
// MDH@25NOV2019: markAllcoationTypes() remembers the current allocation type counts in the 4th and 5th element
void markAllocationCounts(){
    t_count numberOfAllocationTypes=getNumberOfAllocationTypes();
    while(numberOfAllocationTypes>0){
        numberOfAllocationTypes--;
        _allocationTypes[numberOfAllocationTypes].mark_occupied=_allocationTypes[numberOfAllocationTypes].occupied;
        _allocationTypes[numberOfAllocationTypes].mark_freed=_allocationTypes[numberOfAllocationTypes].freed;
        /* MDH@14APR2020 replacing:
        _allocationcounts[5*numberOfAllocationTypes+3]=_allocationcounts[5*numberOfAllocationTypes+1];
        _allocationcounts[5*numberOfAllocationTypes+4]=_allocationcounts[5*numberOfAllocationTypes+2];
        */
    }
}
long long getAllocationTypeAllocated(char allocationType){
    long long allocationTypeAllocated=-2; // if there's some error
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        allocationTypeAllocated=(allocationTypeIndex>=0?_allocationTypes[allocationTypeIndex].occupied:-1);
    }
    return allocationTypeAllocated;
}
long long getAllocationTypeFreed(char allocationType){
    long long allocationTypeFreed=-2; // if there's some error
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        allocationTypeFreed=(allocationTypeIndex>=0?_allocationTypes[allocationTypeIndex].freed:-1);
    }
    return allocationTypeFreed;
}

// 'public' functions
t_count allocationmark(){return addAllocation(' ',0,0);} // MDH@09APR2020: I think calling addallocation() suffices here, instead of addallocationtype

// unmark allocation returns the total number of encountered allocations
t_count unmarkallocation(t_count mark){
    if(!allocations._chars){printf("No allocation recording!\n");return 0;}
    if(mark==0){printf("No mark!\n");return 0;}
#ifndef __PRODUCTION__
    if(mark>allocations.l){printf("Mark %llu too large.\n",mark);return 0;}
    if(allocations._chars[mark-1]!=' '){printf("No mark at position %llu!\n",mark);return 0;}
    allocations.l=mark-1;
#endif
    return allocations.l; // returning what's left!!!
}
void allocationreport(t_count mark){
    if(mark==0||allocations._chars==NULL)return;
#ifndef __PRODUCTION__
    printf("%s","Allocations: '");
    size_t pos=mark-1;
    while(pos<allocations.l)printf("%c",allocations._chars[pos++]);
    printf("%c",'\'');
    printf("\n");
#endif
}
void syncallocations(){
#ifndef __PRODUCTION__
   if(!allocations._chars)return;
    while(allocations.l>0){if(allocations._chars[allocations.l-1]!='.')break;allocations.l--;}
    allocationreport(1);
#endif
}

// MDH@08APR2020: if ptr starts with an allocation_index size_t field we can store the result of addallocation into it
//                so we have to ascertain that in the non-production version every structure that we allocate this way starts with
void* Mmalloc(t_count nitems,size_t size,char type){
    void* ptr=(size>0&&nitems>0?malloc(size*nitems):NULL);
    // MDH@09APR2020: addallocation() is now addallocationtype()
#ifndef __PRODUCTION__
    if(ptr){
        // MDH@13APR2020: all Mmalloc calls represent fixed size allocations
        t_count allocationIndex=registerAllocation(type,size,nitems,true); // NOTE we do now how many items that are being allocated, so we assume size items of a single byte!!
        *((t_count*)ptr)=allocationIndex;
    }
#endif
    return ptr;
}

void* Mcalloc(t_count nitems,size_t size,char type){
    void* ptr=(nitems>0&&size>0?calloc(nitems,size):NULL);
    // MDH@09APR2020: addallocation() is now addallocationtype()
#ifndef __PRODUCTION__
    if(ptr){
        t_count allocationIndex=registerAllocation(type,size,nitems,true);
        *((t_count*)ptr)=allocationIndex;
    }
#endif
    return ptr;
}

void Mfree(void* ptr,char allocationType){
    if(!ptr)return;
    /////printf("Freeing type '%c' data",type);
    // determine the amount of items to free which depends on the type size!!
    // MDH@14APR2020: size_t nitems=0,typesize=0,allocationtypecountoffset=0;
    char* _allocationtype=NULL;
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        // assume a size 1 thing if it's not there yet????? (typically only for testing though!!!)
        if(allocationTypeIndex>=0){ // MDH@07APR2020: better to NOT create the new allocation type if not currently known!!!!
            _allocationTypes[allocationTypeIndex].count--;
            // assuming this is a fixed size allocation type
            _allocationTypes[allocationTypeIndex].freed+=_allocationTypes[allocationTypeIndex].allocationsizeunion.size;
            /* MDH@14APR2020 replacing:
            allocationtypecountoffset=5*allocationTypeIndex;
            if(allocationtypecountoffset>=0){ // success (and not the accumulative (zero) one!!!)
                typesize=_allocationTypes[ // replacing: allocationcounts[allocationtypecountoffset]; // where the size is stored!!!
                if(typesize>0)nitems=sizeof(*ptr)/typesize;
            }
            */
        }else
            printf("BUG: Memory of unknown type '%c' to be freed!\n",allocationType);
    }
    // MDH@14APR2020 NOTE: the following is about removing the allocation
#ifndef __PRODUCTION__
    t_count allocationIndex=*((t_count*)ptr);
    if(allocationIndex>0)allocations._chars[--allocationIndex]=' '; // MDH@13APR2020: can't use ' ' as that's used for a command
    free(ptr);
    //////printf("!");
    // undo the allocation of the given type
    if(!allocations._chars){printf("Allocation types not recorded!\n");return;}
    if(allocations.l==0){printf("Nothing allocated to free.\n");return;}
    t_count pos=allocations.l-1;
    while(pos>0){
        if(allocations._chars[pos]==' '){/*printf("Allocation of type '%c' not encountered.\n",type);*/break;}
        if(allocations._chars[pos]==allocationType){allocations._chars[pos]='.';break;}
        pos--;
    }
    /* MDH@14APR2020 removing:
    ///////printf("!");
    // MDH@15NOV2019: if this is an existing type
    if(nitems>0){ // we know both how many items AND where
        _allocationcounts[allocationtypecountoffset+2]+=nitems; // increment the number of freed data type items
        _allocationcounts[0]-=(nitems*typesize);
        _allocationcounts[2]+=nitems; // another nitems freed!!
    }
    */
    ////////printf("!\n");
#endif
}

// MDH@27NOV2019: now passing the number of items in as well, and the current number of items
// MDH@09APR2020: from now on (v0.1.2) REALLOC is only to be used for all variable dynamic memory allocations
//                and also for freeing (i.e. when occupied equals zero)
void* Mrealloc(void* ptr,t_count from_count,t_count to_count,size_t size,char allocationType){
    //////printf(".");
    // kind of like 'freeing' the space ptr is using now
    // step 1. take out what has been registered before...
    void* newptr=ptr; // by default return the original pointer!!!
    t_count freed=from_count*size; /////// replacing: (ptr?sizeof(*ptr):0); // best to determine it here
    t_count occupied=to_count*size; //// replacing: (newptr?sizeof(*newptr):0); // what we need to add
    if(freed!=occupied){ // amount changed
#ifndef __PRODUCTION__
        t_count allocationIndex;
        if(occupied==0){ // a deallocation, no reason to assume that will fail!!!
            allocationIndex=*((size_t*)ptr);
            if(allocationIndex>0)allocations._chars[allocationIndex-1]=' ';
        }
#endif
        // MDH@14APR2020: if a (re)alloc use malloc if first time otherwise use realloc
        newptr=(occupied>0?(freed>0?realloc(ptr,occupied):malloc(occupied)):NULL); // we have to reallocate nitems each of the given size
        printf("Object of type '%c' reallocated from %llu to %llu!\n",allocationType,freed,occupied);
        // newptr is allowed to be NULL if occupied equals 
        if(occupied==0||newptr){ // success (newptr will be NULL when occupied==0, but that also indicates success)
            if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
                // MDH@09APR2020: this could be the first call to REALLOC with a given type
#ifndef __PRODUCTION__
                if(occupied>0){ // an allocation which means that type should be present in _allocationtypes, and if it is not we're going to register it
                    if(freed==0){ // initial allocation, ALWAYS register a single allocation (TODO perhaps nitems should always be considered 1)
                        allocationIndex=registerAllocation(allocationType,size,to_count,false);
                        *((t_count*)newptr)=allocationIndex; // TODO not sure whether realloc() will initialize to '\0' so we also write when allocation_index is 0!!!!
                    }
                }
#endif
                long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
                if(allocationTypeIndex<0)allocationTypeIndex=getNewAllocationTypeIndex(allocationType,size,to_count,false);
                if(allocationTypeIndex>=0){
                    _allocationTypes[allocationTypeIndex].occupied+=occupied;
                    _allocationTypes[allocationTypeIndex].freed+=freed;
                    // MDH@14APR2020: we have to remove from the histogram (as realloc should always be called on variable size stuff)
                    //                to do so we have to find the t_allocationsize record
                    t_allocationsize* allocationSizes=_allocationTypes[allocationTypeIndex].allocationsizeunion._allocationsizes;
                    t_count allocationSizeCount=_allocationTypes[allocationTypeIndex].count; // the number of allocation sizes
                    while(allocationSizeCount>0){
                        allocationSizeCount--;
                        if(allocationSizes[allocationSizeCount].size==size){
                            allocationSizes[allocationSizeCount].count-=from_count;
                            allocationSizes[allocationSizeCount].count+=to_count;
                            break;
                        }
                    }
                    /* MDH@14APR2020 replacing:
                    size_t allocationtypecountoffset=allocationTypeIndex*5; // double the index to get at the first position of the size_t pair
                    if(allocationtypecountoffset>0){
                        if(_allocationcounts[allocationtypecountoffset]!=size){
                            printf("WARNING: Mrealloc() called on a data type that does not occupy a %zd bytes.",size);
                            freed/=_allocationcounts[allocationtypecountoffset]; // which might round and we are in trouble!!!!
                            occupied/=_allocationcounts[allocationtypecountoffset];
                        }
                        _allocationcounts[allocationtypecountoffset+1]+=occupied; // increment what was occupied
                        _allocationcounts[allocationtypecountoffset+2]+=freed; // increment what was freed
                        _allocationcounts[1]+=occupied;
                        _allocationcounts[2]+=freed;
                        _allocationcounts[0]+=(_allocationcounts[allocationtypecountoffset+1]*(occupied-freed)); // update the number of bytes we've changed!!!
                    }else
                        printf("BUG: Mrealloc() called on the global data type (*).\n");
                    */
                }else 
                    printf("BUG: Mrealloc() called on unknown data type %c.\n",allocationType);
            }
        }
    }
    return newptr;
}

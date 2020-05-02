#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "Malloc.h"

extern char const * const M_ERROR_PREFIX;
extern char const * const M_WARNING_PREFIX;
extern char const * const M_BUG_PREFIX;

static void info(char const * fmt,...){
    // va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);
}
static void warning(char const * fmt,...){printf("%s",M_WARNING_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);}
static void error(char const * fmt,...){printf("%s",M_ERROR_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);}
static void bug(char const * fmt,...){printf("%s",M_BUG_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);}

#ifndef __PRODUCTION__
typedef struct{
    char allocationType;
    long long allocationIndex;
}Malloc;
#endif

static void dump(char* _c,size_t count,size_t size){
    printf("Contents: '");
    size_t l=0;
    while(l<count){if(l>0){if(size>1&&l>0&&l%size)printf("%c",'|');else printf("%c",' ');}printf("%c=%hhu",*_c,*_c);_c++;l++;}
#ifndef __PRODUCTION__
    printf(" + %c",*_c);l=1;while(l<sizeof(Malloc)){_c++;printf(".%x",*_c);l++;}
#endif
    printf("%s","'.\n");
}

// MDH@15NOV2019: want to keep track of the number of allocations for each type (with character id)
t_allocationtype* _allocationTypes=NULL; // the unique allocation type characters
long long numberOfAllocationTypes=0; // keep track of the number of allocation types
// MDH@14APR2020 now being stored as part of the allocationtypes: size_t* _allocationcounts=NULL; // the size of the type is stored every odd size_t

// helper functions
// we need a local something to store the allocation types in
struct{
    long long l; // the number of allocation types stored
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
    return(/* superfluous: allocations._chars&&*/_allocationTypes!=NULL/* MDH@14APR2020: &&_allocationcounts*/);
    // replacing: if(!allocations._chars)info("ERROR: Failed to initialize recording allocations.\n");else info("Allocation recording initialized.\n");
}

static long long getAllocationTypeIndex(char allocationType){
    long long allocationTypeIndex=numberOfAllocationTypes;
    while(--allocationTypeIndex>=0&&_allocationTypes[allocationTypeIndex].type!=allocationType)
    ;
    if(allocationTypeIndex>=0)info("Index of allocation type '%c': %lld.\n",allocationType,allocationTypeIndex);
    return allocationTypeIndex;
}

long long getNumberOfAllocationTypes(){return (_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/?numberOfAllocationTypes:0);}

// MDH@21APR2020: getNewAllocationTypeIndex() changed to also check whether the provided allocationType already exists 
static long long getNewAllocationTypeIndex(char allocationType,size_t size,long long count,bool fixedsize){
    if(_allocationTypes){
        long long newAllocationTypeIndex=getAllocationTypeIndex(allocationType);
        if(newAllocationTypeIndex<0){ // doesn't exist yet
            info("New allocation type #%lld: '%c' of %s size %zd!\n",numberOfAllocationTypes,allocationType,(fixedsize?"variable":"fixed"),size);
            t_allocationsize* _allocationTypeSizeHistogram=(fixedsize?NULL:calloc(1,sizeof(t_allocationsize)));
            if(fixedsize||_allocationTypeSizeHistogram){
                // how about allocating memory for the histogram beforehand?
                void* newAllocationTypes=realloc(_allocationTypes,sizeof(t_allocationtype)*(numberOfAllocationTypes+1));
                if(newAllocationTypes){    
                    info("New allocation type record created.\n");
                    // MDH@14APR2020: _allocationcounts=realloc(_allocationcounts,(sizeof(size_t)*(allocationtypeindex+1))*5); // for every type we store 5 size_t values, one to keep the item count, and one to keep the size
                    _allocationTypes=newAllocationTypes;
                    _allocationTypes[numberOfAllocationTypes].type=allocationType;
                    _allocationTypes[numberOfAllocationTypes].occupied=0;
                    _allocationTypes[numberOfAllocationTypes].freed=0;
                    info("New allocation type '%c' registered!\n",allocationType);       
                    if(fixedsize){
                        _allocationTypes[numberOfAllocationTypes].count=count; // the initial count
                        _allocationTypes[numberOfAllocationTypes].allocationsizeunion.size=size;
                    }else{
                        _allocationTypes[numberOfAllocationTypes].allocationsizeunion._allocationsizes=_allocationTypeSizeHistogram;
                        _allocationTypes[numberOfAllocationTypes].count=1;
                        // this is a bit 'verwarrend' but count is the size of what's allocated, and it's always a single allocation (as stored in the count field)
                        _allocationTypeSizeHistogram->size=count;
                        _allocationTypeSizeHistogram->count=1;            
                    }
                    newAllocationTypeIndex=numberOfAllocationTypes++; // good to go
                    info("Number of allocation types: %lld.\n",numberOfAllocationTypes);
                }else{
                    if(!fixedsize)free(_allocationTypeSizeHistogram); // MDH@14APR2020: don't forget to free what we've allocated beforehand
                    error("Failed to register new allocation type '%c'.\n",allocationType);
                }
            }else
                error("Failed to allocate memory for the histogram data of variable size type '%c'.\n",allocationType);
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
    error("No allocation types!");
    return -1;
}

// MDH@09APR2020: distinguish between adding an allocation (local) and adding an allocationtype (global)
// MDH@21APR2020: addAllocation() doesn't use size so we remove it from the parameter list
long long addAllocation(char allocationType/*,size_t size,*//*,long long count*/){
    // I suppose that the allocation might fail but we do NOT want to loose allocations._chars over it
    // MDH@14APR2020: there's room for improvement here
    // if(count<=0)return -2; // invalid input
    if(!allocations._chars)return -1; // no allocation characters
    info("Remembering an allocation of type '%c'.\n"/*,count*/,allocationType);
    // MDH@21APR2020: consuming count is easier I suppose
    // removing: long long newl=allocations.l+count;
    // while(--count>=0){ // replacing: newl>allocations.l
        if(!(allocations.l&0xF)){ // allocations.l is a multiple of 16, so allocation._chars is full and we need a new block
            info("Expanding allocations.\n");
            char* allocationchars=realloc(allocations._chars,(allocations.l+16));
            if(!allocationchars){error("Allocation could not be remembered.\n");return 0;} // realloc failure
            allocations._chars=allocationchars;
        }
        allocations._chars[allocations.l]=allocationType;
        info("Allocation of type '%c' remembered at position %llu.\n",allocationType,allocations.l);
    // }
    return allocations.l++; // returning the position where the allocation is stored, and incrementing the length of the allocations unless we replace allocations.l by allocations.lastIndex
}

// MDH@14APR2020: addAllocationType renamed to registerAllocation
static long long registerAllocation(char type,size_t size,long long count,bool fixedsize){
    long long allocationIndex=0;
    if(type!='\0'&&type!='*'){
        // how about registering the type first if we need to???????
        // increase size if necessary
        if(size>0&&count>0){
            long long allocationTypeIndex=getNewAllocationTypeIndex(type,size,count,fixedsize);
            if(allocationTypeIndex>=0){ // yes, we should already have at least one allocation type
                allocationIndex=addAllocation(type/*,size*//*,count*/); // this is for registering the allocation BUT TODO should this be done here?????
                if(allocationIndex>=0){
                    //t_allocationtype allocationType=_allocationTypes[allocationTypeIndex];
                    if(!fixedsize){
                        // store in histogram
                        t_allocationsize* histogram=_allocationTypes[allocationTypeIndex].allocationsizeunion._allocationsizes;
                        if(!histogram)_allocationTypes[allocationTypeIndex].count=0; // MDH@29APR2020: precaution in case histogram pointer is undefined
                        long long category=_allocationTypes[allocationTypeIndex].count;
                        while(--category>=0&&histogram[category].size!=count)
                        ;
                        if(category<0){ // does not yet exist
                            printf("Adding category #%lld as %lld units (of size %zd) to the histogram of allocation type '%c'.\n",_allocationTypes[allocationTypeIndex].count+1,count,size,type);
                            if(histogram)
                                histogram=realloc(histogram,sizeof(t_allocationsize)*(_allocationTypes[allocationTypeIndex].count+1));
                            else
                                histogram=malloc(sizeof(t_allocationsize));
                            if(histogram){
                                _allocationTypes[allocationTypeIndex].allocationsizeunion._allocationsizes=histogram; // MDH@29APR2020 ADDITION: Oops, suppose this is important as well
                                category=_allocationTypes[allocationTypeIndex].count++; // another histogram category (and count represents the number of categories)
                                histogram[category].count=0; // will be incremented below!!!!
                                histogram[category].size=count;
                                printf("Category #%lld of size %lld added to the histogram of allocation type '%c'.\n",_allocationTypes[allocationTypeIndex].count+1,count,type);
                            }
                        }
                        if(category>=0)
                            histogram[category].count++;
                        else
                            error("Failed to count %lld allocation(s) of type '%c' and size %zd.\n",count,type,size);
                    }else // fixed size allocation, so increment count with the number of allocations (1 in general for static allocations)
                        _allocationTypes[allocationTypeIndex].count+=count;
                }else
                    error("Failed to add %lld allocation(s) of type '%c' and size %zd.\n",count,type,size);
                /* MDH@14APR2020
                if(allocationIndex>0){
                    // MDH@15NOV2019: add another size_t to _allocationcounts array if we need to
                    _allocationcounts[allocationTypeIndex*5+1]+=nitems; // another nitems allocated
                    _allocationcounts[0]+=(nitems*size); // keep track of the total amount of bytes used
                    _allocationcounts[1]+=nitems; // another nitems allocated
                }
                */
            }else
                error("Failed to register allocation of type '%c' and size %zd (error code: %lld).\n",type,size,allocationTypeIndex);
        }
    }
    return allocationIndex;
}
static bool unregisterAllocation(long long allocationTypeIndex,size_t size,bool fixedsize){
    // NOTE the given size is actually the count i.e. the number of units allocated
    if(allocationTypeIndex>=0&&allocationTypeIndex<numberOfAllocationTypes){
        t_allocationtype allocationType=_allocationTypes[allocationTypeIndex];
        if(size>0){
            if(!fixedsize){
                // lookup the size as size in the histogram (should be there)
                t_allocationsize* histogram=(allocationType.count>0?allocationType.allocationsizeunion._allocationsizes:NULL);
                if(histogram){
                    long long category=allocationType.count;
                    while(--category>=0&&histogram[category].size!=size)
                    printf(" %lld*%zd",histogram[category].count,histogram[category].size);
                    ;
                    printf("\n");
                    if(category>=0){ // found it
                        if(histogram[category].count>0){
                            histogram[category].count--; // one down
                            return true;
                        }
                        bug("Unable to unregister the allocation of size %zd of type '%c': no registered allocation count.\n",size,allocationType.type);
                    }else
                        bug("Unable to unregister the allocation of size %zd of type '%c': allocation type category unknown.\n",size,allocationType.type);
                }
            }else{
                if(allocationType.count>=size){
                    allocationType.count-=size;
                    return true;
                }
                bug("Failed to unregister %zd fixed-size allocation%s of type '%c'.\n",size,(size>1?"s":""),allocationType.type);
            }
        }
    }
    return false;
}
// MDH@15NOV2019: allow access to the allocation counts
// MDH@25NOV2019: let's return copies, so that we can get a frozen snapshot instead of something that can change
/* MDH@14APR2020: not used anymore
long long* _getAllocationCounts(){
    size_t sizeallocationcounts=(_allocationcounts?numberofallocationtypes*sizeof(size_t)*5:0);
    return(sizeallocationcounts>0?memcpy((char*)malloc(sizeallocationcounts),_allocationcounts,sizeallocationcounts):NULL);
    return NULL;
}
*/
t_allocationtype* _getAllocationTypes(){
    size_t allocationTypesSize=(_allocationTypes?numberOfAllocationTypes*sizeof(t_allocationtype):0);
    return(allocationTypesSize>0?memcpy(malloc(allocationTypesSize),_allocationTypes,allocationTypesSize):NULL);
}
// MDH@19NOV2019: 
bool resetAllocationTypes(){
    /////info("Resetting allocation type counts.\n");
    // best to free the lot, the reinitialize
    if(_allocationTypes)free(_allocationTypes);
    // MDH@14APR2020: if(_allocationcounts)free(_allocationcounts);
    if(allocations._chars)free(allocations._chars);
    ////////info("Allocation type counts reset.\n");
    return allocationRecordingInitialized();
}
// MDH@25NOV2019: markAllcoationTypes() remembers the current allocation type counts in the 4th and 5th element
void markAllocationCounts(){
    long long numberOfAllocationTypes=getNumberOfAllocationTypes();
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
// MDH@21APR2020 the mark changed to \0 but we might consider using another character to indicate such a mark
long long allocationmark(){return addAllocation('\0'/*,0*//*,1*/);} // MDH@09APR2020: I think calling addallocation() suffices here, instead of addallocationtype

// unmark allocation returns the total number of encountered allocations
long long unmarkallocation(long long mark){
    if(!allocations._chars){info("No allocation recording!\n");return 0;}
    if(mark==0){info("No mark!\n");return 0;}
// MDH@21APR2020: TODO don't think we actually need __PRODUCTION__ here!!!
#ifndef __PRODUCTION__
    if(mark>allocations.l){info("Mark %llu too large.\n",mark);return 0;}
    if(allocations._chars[mark-1]!='\0'){info("No mark at position %llu!\n",mark);return 0;}
    allocations.l=mark-1;
#endif
    return allocations.l; // returning what's left!!!
}
void allocationreport(long long mark){
    if(mark==0||allocations._chars==NULL)return;
    printf("%s","Allocations: '");
    size_t pos=mark-1;
    while(pos<allocations.l)printf("%c",allocations._chars[pos++]);
    printf("%c",'\'');
    printf("%c",'\n');
}
void syncallocations(){
#ifndef __PRODUCTION__
   if(!allocations._chars)return;
    while(allocations.l>0){if(allocations._chars[allocations.l-1]!='.')break;allocations.l--;}
    allocationreport(1);
#endif
}

#ifndef __PRODUCTION__
// MDH@21APR2020: general function to store allocation info with the dynamically allocated memory
static void attachAllocationInfo(void* ptr,char allocationType,size_t size){
    // ASSERT all arguments supposedly valid i.e. ptr!=NULL, size>0
    // MDH@13APR2020: all Mmalloc calls represent fixed size allocations
    long long allocationIndex=registerAllocation(allocationType,size,1,true); // NOTE we do now how many items that are being allocated, so we assume size items of a single byte!!
    Malloc* _alloc=(Malloc*)((char*)ptr+size);
    _alloc->allocationType=allocationType; // register the type
    _alloc->allocationIndex=allocationIndex;
}
#endif

// MDH@08APR2020: if ptr starts with an allocation_index size_t field we can store the result of addallocation into it
//                so we have to ascertain that in the non-production version every structure that we allocate this way starts with
void* Mmalloc(size_t size,char type){
    void* ptr=NULL;
    if(size>0){
// MDH@09APR2020: addallocation() is now addallocationtype()
#ifndef __PRODUCTION__
        info("\n*************************** Allocating %zd bytes of dynamic memory of type '%c' ***************************\n",size,type);
        ptr=malloc(size+sizeof(Malloc));
        if(ptr)attachAllocationInfo(ptr,type,size);
#else
        ptr=malloc(size);
#endif
    }
    return ptr;
}

void* Mcalloc(size_t size,char type){
    void* ptr=NULL;
    if(size>0){
#ifndef __PRODUCTION__
        info("\n*************************** Allocating %zd initialized bytes of dynamic memory of type '%c' ***************************\n",size,type);
        ptr=calloc(1,size+sizeof(Malloc)); // MDH@20APR2020: calloc doesn't care about the items!!!!!
        if(ptr)attachAllocationInfo(ptr,type,size);
#else
        ptr=calloc(1,size);
#endif
    }
    return ptr;
}

// MDH@20APR2020: unfortunately we need to know the size of what was allocated which is easy for fixed size allocation but problematic for variable size records
//                unless we assume that Mfree is always called on fixed size allocations which require that a single item is allocated each time, so we don't need nitems on Mmalloc and Mcalloc
void Mfree(void* ptr,char allocationType){
    if(!ptr)return;
    info("\n*************************** Freeing dynamic memory of type '%c' ***************************\n",allocationType);
    /////info("Freeing type '%c' data",type);
    // determine the amount of items to free which depends on the type size!!
    // MDH@14APR2020: size_t nitems=0,typesize=0,allocationtypecountoffset=0;
    size_t size=0; // MDH@20APR2020: we need to determine the size from what we stored with the allocation type
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        // assume a size 1 thing if it's not there yet????? (typically only for testing though!!!)
        if(allocationTypeIndex>=0){ // MDH@07APR2020: better to NOT create the new allocation type if not currently known!!!!
            size=_allocationTypes[allocationTypeIndex].allocationsizeunion.size; // extract the (fixed) size
            if(!unregisterAllocation(allocationTypeIndex,1,true))
                bug("Failed to free the dynamic memory of an allocation of type '%c' (size: %zd).\n",allocationType,size); // MDH@02MAY2020: we have to decrement the count (representing the number of allocated instances) by 1
            /* replacing what would no longer work:
            _allocationTypes[allocationTypeIndex].count--;
            // assuming this is a fixed size allocation type
            size=_allocationTypes[allocationTypeIndex].allocationsizeunion.size;
            _allocationTypes[allocationTypeIndex].freed+=size;
            */
            /* MDH@14APR2020 replacing:
            allocationtypecountoffset=5*allocationTypeIndex;
            if(allocationtypecountoffset>=0){ // success (and not the accumulative (zero) one!!!)
                typesize=_allocationTypes[ // replacing: allocationcounts[allocationtypecountoffset]; // where the size is stored!!!
                if(typesize>0)nitems=sizeof(*ptr)/typesize;
            }
            */
        }else
            bug("Memory of unknown type '%c' to be freed!\n",allocationType);
    }
    // MDH@14APR2020 NOTE: the following is about removing the allocation
#ifndef __PRODUCTION__
    Malloc* _alloc=(Malloc*)(((char*)ptr)+size);
    if(_alloc->allocationType!=allocationType){
        bug("Allocation type of dynamic memory '%c' (%u) freed of size %zd does not match provided allocation type '%c'.\n",_alloc->allocationType,_alloc->allocationType,size,allocationType);
        dump(ptr,size,size);
        free(ptr);
        return;
    }
    // if(_alloc->allocationIndex<=0)bug("No allocation index registered for allocation of type '%c'.\n",allocationType);
#endif
    if(!allocations._chars)error("Allocation types not recorded!\n");
    if(allocations.l==0)warning("Nothing allocated to free.\n");
#ifndef __PRODUCTION__
    if(_alloc->allocationIndex>=0&&_alloc->allocationIndex<allocations.l){
        if(allocations._chars[_alloc->allocationIndex]!=allocationType){
            bug("Allocation type of dynamic memory '%c' (=%u) (at index %llu) does not match provided allocation type '%c'.\n",allocations._chars[_alloc->allocationIndex],allocations._chars[_alloc->allocationIndex],_alloc->allocationIndex,allocationType);
            dump(ptr,size,size);
        }else
            allocations._chars[_alloc->allocationIndex]=' '; // MDH@13APR2020: can't use ' ' as that's used for a command
    }else{
        bug("Retrieved allocation position %llu out of range [0,%llu).",_alloc->allocationIndex,allocations.l);
        dump(ptr,size,size);
    }
#else
    long long pos=allocations.l-1;
    while(pos>0){
        if(allocations._chars[pos]==allocationType){allocations._chars[pos]=' ';break;}
        pos--;
    }
#endif
    free(ptr);
    //////info("!");
    // undo the allocation of the given type
    /* MDH@14APR2020 removing:
    ///////info("!");
    // MDH@15NOV2019: if this is an existing type
    if(nitems>0){ // we know both how many items AND where
        _allocationcounts[allocationtypecountoffset+2]+=nitems; // increment the number of freed data type items
        _allocationcounts[0]-=(nitems*typesize);
        _allocationcounts[2]+=nitems; // another nitems freed!!
    }
    */
    ////////info("!\n");
}

// MDH@20APR2020: 
void Mresized(void* ptr,long long from_count,long long to_count,size_t size,char allocationType){

}

// MDH@27NOV2019: now passing the number of items in as well, and the current number of items
// MDH@09APR2020: from now on (v0.1.2) REALLOC is only to be used for all variable dynamic memory allocations
//                and also for freeing (i.e. when occupied equals zero)
void* Mrealloc(void* ptr,long long from_count,long long to_count,size_t size,char allocationType){
    if(from_count<0||to_count<0){bug("%s.\n","Number of bytes to free or occupy negative");return NULL;}
    // info("Size of Malloc: %zd, size of long long: %zd.\n",sizeof(Malloc),sizeof(long long));
    void* newptr=ptr; // by default return the original pointer!!!
    //////info(".");
    // kind of like 'freeing' the space ptr is using now
    // step 1. take out what has been registered before...
    // MDH@29APR2020: ascertaining that freed equals 0 when ptr is NULL (no matter what from_count is!!!!)
    size_t freed=(ptr?size*from_count:0); /////// replacing: (ptr?sizeof(*ptr):0); // best to determine it here
    size_t occupied=size*to_count; //// replacing: (newptr?sizeof(*newptr):0); // what we need to add
#ifndef __PRODUCTION__
    info("\n*************************** Reallocating %llu blocks of size %zd to %llu blocks of dynamic memory of type '%c' ***************************\n",from_count,size,to_count,allocationType);
#endif
    if(freed!=occupied){ // amount changed
        Malloc* _alloc=(freed>0?(Malloc*)(((char*)ptr)+freed):NULL); // pointer to Malloc allocation registration appendix
        long long allocationIndex=-1;
#ifndef __PRODUCTION__
        Malloc newAllocation={allocationType,0}; // default to the given allocation type
        if(_alloc){
            newAllocation.allocationType=_alloc->allocationType;
            newAllocation.allocationIndex=_alloc->allocationIndex;
            // we need to get the allocation type and index out BEFORE memory is reallocated!!!!!
            if(_alloc->allocationType!=allocationType){
                bug("The allocation type '%c' stored with the data at byte %zd does not match the provided allocation type '%c'.\n",_alloc->allocationType,freed,allocationType);
                // let's dump the current contents as text?
                dump(ptr,freed,size);
            }
            // MDH@20APR2020 ASSERT: freed>0 as freed!=occupied
            allocationIndex=_alloc->allocationIndex;
            if(allocationIndex>=0&&allocationIndex<allocations.l){
                if(allocations._chars[allocationIndex]!=_alloc->allocationType){
                    bug("Type '%c' of remembered allocation #%llu does not match the provided allocation type '%c'.\n",allocations._chars[allocationIndex],allocationIndex,_alloc->allocationType);
                    dump(ptr,freed,size);
                }
                // MDH@22APR2020 BUG FIX: do NOT clear the remembered allocation type unless the memory is freed!!!!
                if(occupied==0) // MDH@22APR2020 ADDITION
                    allocations._chars[allocationIndex]=' ';
            }else{
                bug("Allocation index %zd stored with the data at position %llu (resized to %llu) of size %zd is out of range [0,%llu)!\n",allocationIndex,freed,occupied,size,allocations.l);
                dump(ptr,freed,size);
            }
        }
#endif
        // MDH@14APR2020: if a (re)alloc use malloc if first time otherwise use realloc
        if(occupied>0){
#ifndef __PRODUCTION__
            if(freed>0)
                newptr=realloc(ptr,occupied+sizeof(Malloc));
            else
                newptr=malloc(occupied+sizeof(Malloc)); // we have to reallocate nitems each of the given size
#else
            newptr=(freed>0?realloc(ptr,occupied):malloc(occupied)); // we have to reallocate nitems each of the given size
#endif
        }else
            free(ptr);
        printf("Object of type '%c' resized from %zd to %zd!\n",allocationType,freed,occupied);
        // newptr is allowed to be NULL if occupied equals 
        if(newptr){ // success (newptr will be NULL when occupied==0, but that also indicates success)
#ifndef __PRODUCTION__
            if(freed>0){
                unregisterAllocation(getAllocationTypeIndex(allocationType),from_count,false); // MDH@28APR2020
            }
            if(occupied>0){
                info("Number of dynamically allocated bytes: %zd.\n",occupied+sizeof(Malloc));
                if(!_alloc){ // first time allocation (i.e. freed equals zero)
                    info("Storing allocation information...\n");
                    _alloc=(Malloc*)(((char*)newptr)+occupied); // MDH@21APR2020 BUG FIX: it said ptr instead of newptr here before which obviously was terribly wrong as ptr would be NULL on the first allocation
                    // MDH@21APR2020 OK, mapping ptr to char* as we do seems to work: info("Number of bytes between start of dynamic data and allocation information: %zd.\n",(char*)_alloc-(char*)ptr);
                    _alloc->allocationType=allocationType;
                    info("Allocation type stored!\n");
                    _alloc->allocationIndex=registerAllocation(allocationType,size,to_count,false);
                    printf("Allocation index %llu stored.\n",_alloc->allocationIndex);
                    // info("Allocation information stored...\n");
                }else{ // not a first time allocation, so we can simply copy the allocation over
                    Malloc* _newalloc=(Malloc*)(((char*)newptr)+occupied);
                    _newalloc->allocationType=newAllocation.allocationType;
                    _newalloc->allocationIndex=newAllocation.allocationIndex;
                    // replacing: memcpy(_newalloc,_alloc,sizeof(Malloc)); // replacing:  *((Malloc*)(((char*)newptr)+occupied))=*_alloc; // copying the allocation structure over // OOPS ptr replaced by newptr (what it should be I guess)
                    info("%zd allocation information bytes copied...\n",sizeof(Malloc));
                    // printf("New allocation type %c (%c) - allocation index %llu (%llu).\n",_newalloc->allocationType,_alloc->allocationType,_newalloc->allocationIndex,_alloc->allocationIndex);
                }
            }
#endif
            if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
                // MDH@09APR2020: this could be the first call to REALLOC with a given type
                long long allocationTypeIndex=getNewAllocationTypeIndex(allocationType,size,to_count,false);
                if(allocationTypeIndex>=0){
                    _allocationTypes[allocationTypeIndex].occupied+=occupied;
                    _allocationTypes[allocationTypeIndex].freed+=freed;
                    // MDH@14APR2020: we have to remove from the histogram (as realloc should always be called on variable size stuff)
                    //                to do so we have to find the t_allocationsize record
                    t_allocationsize* allocationSizes=_allocationTypes[allocationTypeIndex].allocationsizeunion._allocationsizes;
                    long long allocationSizeCount=_allocationTypes[allocationTypeIndex].count; // the number of allocation sizes
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
                            info("WARNING: Mrealloc() called on a data type that does not occupy a %zd bytes.",size);
                            freed/=_allocationcounts[allocationtypecountoffset]; // which might round and we are in trouble!!!!
                            occupied/=_allocationcounts[allocationtypecountoffset];
                        }
                        _allocationcounts[allocationtypecountoffset+1]+=occupied; // increment what was occupied
                        _allocationcounts[allocationtypecountoffset+2]+=freed; // increment what was freed
                        _allocationcounts[1]+=occupied;
                        _allocationcounts[2]+=freed;
                        _allocationcounts[0]+=(_allocationcounts[allocationtypecountoffset+1]*(occupied-freed)); // update the number of bytes we've changed!!!
                    }else
                        info("BUG: Mrealloc() called on the global data type (*).\n");
                    */
                }else 
                    info("BUG: Mrealloc() called on unknown data type %c.\n",allocationType);
            }
        }
    }
    return newptr;
}

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// MDH@15NOV2019: want to keep track of the number of allocations for each type (with character id)
char* _allocationtypes=NULL; // the unique allocation type characters
size_t numberofallocationtypes=0; // keep track of the number of allocation types
size_t* _allocationcounts=NULL; // the size of the type is stored every odd size_t

// helper functions
// we need a local something to store the allocation types in
struct{
    size_t l; // the number of allocation types stored
    char* _chars; // the allocation type characters
}allocations;

// you HAVE to call this method to be able to register allocations
bool allocationRecordingInitialized(){
    printf("Initializing allocation recording...\n");
    allocations.l=0;
#ifndef __PRODUCTION__
    allocations._chars=malloc(16); // starting out with one block
    _allocationtypes=malloc(sizeof(char));if(_allocationtypes){numberofallocationtypes=1;*_allocationtypes='*';} // always keep track of the total allocation count, which we mark with a wildcard *
    if(numberofallocationtypes>0)_allocationcounts=calloc(5,sizeof(size_t)); // start out with two size_t items one to store the count and one to store the size!!
#else
    allocations._chars=NULL;
#endif
    return(allocations._chars&&_allocationtypes&&_allocationcounts);
    // replacing: if(!allocations._chars)printf("ERROR: Failed to initialize recording allocations.\n");else printf("Allocation recording initialized.\n");
}

size_t getNumberOfAllocationTypes(){return (_allocationtypes&&_allocationcounts?numberofallocationtypes:0);}

size_t getNewAllocationTypeIndex(char allocationtype,size_t size){
    size_t allocationtypeindex=getNumberOfAllocationTypes(); // at least one!!!
    if(allocationtypeindex>0){ // meaning we have both _allocationtypes and _allocationcounts
        // printf("New allocation type #%zd: '%c' of size %zd!\n",allocationtypeindex,allocationtype,size);
        _allocationtypes=realloc(_allocationtypes,sizeof(char)*(allocationtypeindex+1));
        _allocationcounts=realloc(_allocationcounts,(sizeof(size_t)*(allocationtypeindex+1))*5); // for every type we store 5 size_t values, one to keep the item count, and one to keep the size
        if(_allocationtypes&&_allocationcounts){
            // still got them
            _allocationtypes[allocationtypeindex]=allocationtype;
            numberofallocationtypes=allocationtypeindex+1; // keep track of how many we've got
            // register the size and initialize the count to 0!!!
            _allocationcounts[allocationtypeindex*5]=size; // storing the size in the second element of the pair
            _allocationcounts[(allocationtypeindex*5)+1]=0; // number of allocations
            _allocationcounts[(allocationtypeindex*5)+2]=0; // number of frees
            _allocationcounts[(allocationtypeindex*5)+3]=0; // mark number of allocations
            _allocationcounts[(allocationtypeindex*5)+4]=0; // mark number of frees
        }
    }
    return allocationtypeindex;
}

size_t addallocation(char allocationtype,size_t size,size_t nitems){
    if(!allocationtype)return 0; // force using allocationtype to prevent unused-parameter warning
#ifndef __PRODUCTION__
    // increase size if necessary
    size_t newl=allocations.l+nitems;
    while(newl>allocations.l){
        if(allocations._chars&&!(allocations.l&0xF))allocations._chars=realloc(allocations._chars,(allocations.l+16)); // add a 'block' if now full
        if(!allocations._chars)return 0;
        allocations._chars[allocations.l]=allocationtype;
        allocations.l++;
    }
    if(nitems>0&&allocationtype!='*'){
        // MDH@15NOV2019: add another size_t to _allocationcounts array if we need to
        size_t allocationtypeindex=getNumberOfAllocationTypes(); // the number of registered allocation types
        if(allocationtypeindex>0){ // yes, we should already have at least one allocation type
            char* _allocationtype=strchr(_allocationtypes,allocationtype);
            if(_allocationtype){ // already got it
                // if(allocationtype!='S'&&allocationtype!='s')printf("Adding %zd allocations of type %c with size %zd.\n",nitems,allocationtype,size);
                allocationtypeindex=(_allocationtype-_allocationtypes);
                // NOTE text with variable length is allocated as type '"' and should not be checked!!
                if(allocationtype!='s'&&size!=_allocationcounts[allocationtypeindex*5]){
                    // printf("Allocation types: '%s'.\n",_allocationtypes);
                    printf("*****************\nAllocation types: '%s'.\nBUG: Different size (%zd) of data type '%c' (size: %zd, count: %zd) received!\n*****************\n",_allocationtypes,size,allocationtype,_allocationcounts[5*allocationtypeindex],_allocationcounts[5*allocationtypeindex+1]);
                }
            }else // haven't got this one yet!!!
                allocationtypeindex=getNewAllocationTypeIndex(allocationtype,size);
            if(allocationtypeindex>0){
                _allocationcounts[allocationtypeindex*5+1]+=nitems; // another nitems allocated
                _allocationcounts[0]+=(nitems*size); // keep track of the total amount of bytes used
                _allocationcounts[1]+=nitems; // another nitems allocated
            } // increment the allocation type count and the total allocation count
        }
    }
    return allocations.l;
#else
    return 0;
#endif
}

// MDH@15NOV2019: allow access to the allocation counts
// MDH@25NOV2019: let's return copies, so that we can get a frozen snapshot instead of something that can change
size_t* _getAllocationCounts(){
    size_t sizeallocationcounts=(_allocationcounts?numberofallocationtypes*sizeof(size_t)*5:0);
    return(sizeallocationcounts>0?memcpy((char*)malloc(sizeallocationcounts),_allocationcounts,sizeallocationcounts):NULL);
}
char* _getAllocationTypes(){
    size_t sizeallocationtypes=(_allocationtypes?numberofallocationtypes*sizeof(char):0);
    return(sizeallocationtypes>0?memcpy((char*)malloc(sizeallocationtypes),_allocationtypes,sizeallocationtypes):NULL);
}
// MDH@19NOV2019: 
bool resetAllocationTypes(){
    /////printf("Resetting allocation type counts.\n");
    // best to free the lot, the reinitialize
    if(_allocationtypes)free(_allocationtypes);
    if(_allocationcounts)free(_allocationcounts);
    if(allocations._chars)free(allocations._chars);
    ////////printf("Allocation type counts reset.\n");
    return allocationRecordingInitialized();
}
// MDH@25NOV2019: markAllcoationTypes() remembers the current allocation type counts in the 4th and 5th element
void markAllocationCounts(){
    if(_allocationtypes&&_allocationcounts){
        size_t numberofallocationtypes=getNumberOfAllocationTypes();
        while(numberofallocationtypes>0){
            numberofallocationtypes--;
            _allocationcounts[5*numberofallocationtypes+3]=_allocationcounts[5*numberofallocationtypes+1];
            _allocationcounts[5*numberofallocationtypes+4]=_allocationcounts[5*numberofallocationtypes+2];
        }
    }
}
long long getAllocationTypeAllocated(char allocationtype){
    long long allocationTypeAllocated=-2; // if there's some error
    if(_allocationtypes&&_allocationcounts){
        char* _allocationtype=strchr(_allocationtypes,allocationtype);
        allocationTypeAllocated=(_allocationtype?_allocationcounts[1+(_allocationtype-_allocationtypes)*5]:-1);
    }
    return allocationTypeAllocated;
}
long long getAllocationTypeFreed(char allocationtype){
    long long allocationTypeFreed=-2; // if there's some error
    if(_allocationtypes&&_allocationcounts){
        char* _allocationtype=strchr(_allocationtypes,allocationtype);
        allocationTypeFreed=(_allocationtype?_allocationcounts[2+(_allocationtype-_allocationtypes)*5]:-1);
    }
    return allocationTypeFreed;
}

// 'public' functions
size_t allocationmark(){return addallocation(' ',0,0);}

// unmark allocation returns the total number of encountered allocations
size_t unmarkallocation(size_t mark){
    if(!allocations._chars){printf("No allocation recording!\n");return 0;}
    if(mark==0){printf("No mark!\n");return 0;}
#ifndef __PRODUCTION__
    if(mark>allocations.l){printf("Mark %zu too large.\n",mark);return 0;}
    if(allocations._chars[mark-1]!=' '){printf("No mark at position %zu!\n",mark);return 0;}
    allocations.l=mark-1;
#endif
    return allocations.l; // returning what's left!!!
}
void allocationreport(size_t mark){
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
#ifndef __PRODUCTION__
void* Mmalloc(size_t nitems,size_t size,char type){
    void* ptr=(size>0&&nitems>0?malloc(size*nitems):NULL);
    if(ptr)addallocation(type,size,nitems); // NOTE we do now how many items that are being allocated, so we assume size items of a single byte!!
    return ptr;
}

void* Mcalloc(size_t nitems,size_t size,char type){
    void* ptr=(nitems>0&&size>0?calloc(nitems,size):NULL);
    if(ptr)addallocation(type,size,nitems);
    return ptr;
}

void Mfree(void* ptr,char type){
    if(!ptr)return;
    /////printf("Freeing type '%c' data",type);
    // determine the amount of items to free which depends on the type size!!
    size_t nitems=0,typesize=0,allocationtypecountoffset=0;
    char* _allocationtype=NULL;
    if(_allocationtypes&&_allocationcounts){
        char* _allocationtype=strchr(_allocationtypes,type);
        // assume a size 1 thing if it's not there yet????? (typically only for testing though!!!)
        if(_allocationtype){ // MDH@07APR2020: better to NOT create the new allocation type if not currently known!!!!
            allocationtypecountoffset=5*(_allocationtype?_allocationtype-_allocationtypes:getNewAllocationTypeIndex(type,1));
            if(allocationtypecountoffset>0){ // success (and not the accumulative (zero) one!!!)
                typesize=_allocationcounts[allocationtypecountoffset]; // where the size is stored!!!
                if(typesize>0)nitems=sizeof(*ptr)/typesize;
            }
        }else
            printf("BUG: Memory of unknown type '%c' to be freed!\n",type);
    }
    free(ptr);
    //////printf("!");
    // undo the allocation of the given type
    if(!allocations._chars){printf("Allocation types not recorded!\n");return;}
    if(allocations.l==0){printf("Nothing allocated to free.\n");return;}
    size_t pos=allocations.l-1;
    while(pos>0){
        if(allocations._chars[pos]==' '){/*printf("Allocation of type '%c' not encountered.\n",type);*/break;}
        if(allocations._chars[pos]==type){allocations._chars[pos]='.';break;}
        pos--;
    }
    ///////printf("!");
    // MDH@15NOV2019: if this is an existing type
    if(nitems>0){ // we know both how many items AND where
        _allocationcounts[allocationtypecountoffset+2]+=nitems; // increment the number of freed data type items
        _allocationcounts[0]-=(nitems*typesize);
        _allocationcounts[2]+=nitems; // another nitems freed!!
    }
    ////////printf("!\n");
}

// MDH@27NOV2019: now passing the number of items in as well, and the current number of items
void* Mrealloc(void* ptr,size_t from_nitems,size_t to_nitems,size_t size,char type){
    //////printf(".");
    // kind of like 'freeing' the space ptr is using now
    // step 1. take out what has been registered before...
    void* newptr=ptr; // by default return the original pointer!!!
    long long freed=from_nitems*size; /////// replacing: (ptr?sizeof(*ptr):0); // best to determine it here
    long long occupied=to_nitems*size; //// replacing: (newptr?sizeof(*newptr):0); // what we need to add
    if(freed!=occupied){ // amount changed
        newptr=realloc(ptr,occupied); // we have to reallocate nitems each of the given size
        if(_allocationtypes&&_allocationcounts){
            char* _allocationtype=strchr(_allocationtypes,type);
            if(_allocationtype){
                size_t allocationtypecountoffset=(_allocationtype-_allocationtypes)*5; // double the index to get at the first position of the size_t pair
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
            }else 
                printf("BUG: Mrealloc() called on an unknown data type pointer.\n");
        }
    }
    return newptr;
}
#endif

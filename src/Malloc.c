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
bool allocationrecordinginitialized(){
    allocations.l=0;
#ifndef __PRODUCTION__
    allocations._chars=malloc(16); // starting out with one block
    _allocationtypes=malloc(sizeof(char));if(_allocationtypes){numberofallocationtypes=1;*_allocationtypes='*';} // always keep track of the total allocation count, which we mark with a wildcard *
    if(numberofallocationtypes>0)_allocationcounts=calloc(2,sizeof(size_t)); // start out with two size_t items one to store the count and one to store the size!!
#else
    allocations._chars=NULL;
#endif
    return(allocations._chars&&_allocationtypes&&_allocationcounts);
    // replacing: if(!allocations._chars)printf("ERROR: Failed to initialize recording allocations.\n");else printf("Allocation recording initialized.\n");
}

size_t getNumberOfAllocationTypes(){return (_allocationtypes&&_allocationcounts?numberofallocationtypes:0);}
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
            if(!_allocationtype){ // haven't got this one yet!!!
                ////////printf("New allocation type #%zd: '%c'!\n",allocationtypeindex,allocationtype);
                _allocationtypes=realloc(_allocationtypes,sizeof(char)*(allocationtypeindex+1));
                _allocationcounts=realloc(_allocationcounts,(sizeof(size_t)*(allocationtypeindex+1))<<1); // for every type we store 2 size_t values, one to keep the item count, and one to keep the size
                if(_allocationtypes&&_allocationcounts){ // still got them
                    numberofallocationtypes=allocationtypeindex+1; // keep track of how many we've got
                    _allocationtypes[allocationtypeindex]=allocationtype;
                    // register the size and initialize the count to 0!!!
                    _allocationcounts[allocationtypeindex<<1]=0;
                    _allocationcounts[(allocationtypeindex<<1)+1]=size; // storing the size in the second element of the pair
                }else
                    allocationtypeindex=0; // too bad
            }else{ // already got it
                allocationtypeindex=(_allocationtype-_allocationtypes);
                // NOTE text with variable length is allocated as type '"' and should not be checked!!
                if(allocationtype!='"'&&size!=_allocationcounts[1+(allocationtypeindex<<1)])
                    printf("*****************\nBUG: Different size (%zd) of data type '%c' (size: %zd) received!\n*****************\n",size,allocationtype,_allocationcounts[1+(allocationtypeindex<<1)]);
            }
            if(allocationtypeindex>0){
                _allocationcounts[allocationtypeindex<<1]+=nitems;
                _allocationcounts[0]+=nitems;
                _allocationcounts[1]+=(nitems*size); // keep track of the total amount of bytes used
            } // increment the allocation type count and the total allocation count
        }
    }
    return allocations.l;
#else
    return 0;
#endif
}

// MDH@15NOV2019: allow access to the allocation counts
size_t* getAllocationCounts(){return _allocationcounts;}
char* getAllocationTypes(){return _allocationtypes;}

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

#ifndef __PRODUCTION__
void* Mmalloc(size_t size,char type){
    void* ptr=(size>0?malloc(size):NULL);
    if(ptr)addallocation(type,1,size); // NOTE we do now how many items that are being allocated, so we assume size items of a single byte!!
    return ptr;
}

void* Mcalloc(size_t nitems,size_t size,char type){
    void* ptr=(nitems>0&&size>0?calloc(nitems,size):NULL);
    if(ptr)addallocation(type,size,nitems);
    return ptr;
}

void Mfree(void* ptr,char type){
    if(!ptr)return;
    // determine the amount of items to free which depends on the type size!!
    size_t nitems=0,typesize=0,allocationtypecountoffset=0;
    char* _allocationtype=(_allocationtypes&&_allocationcounts?strchr(_allocationtypes,type):NULL);
    if(_allocationtype){
        allocationtypecountoffset=(_allocationtype-_allocationtypes)<<1; // double the index to get at the first position of the size_t pair
        if(allocationtypecountoffset>0){
            typesize=_allocationcounts[allocationtypecountoffset+1]; // where the size is stored!!!
            if(typesize>0)nitems=sizeof(*ptr)/typesize;
        }
    }
    free(ptr);
    // undo the allocation of the given type
    if(!allocations._chars){printf("Allocation types not recorded!\n");return;}
    if(allocations.l==0){printf("Nothing allocated to free.\n");return;}
    size_t pos=allocations.l-1;
    while(pos>0){
        if(allocations._chars[pos]==' '){/*printf("Allocation of type '%c' not encountered.\n",type);*/break;}
        if(allocations._chars[pos]==type){allocations._chars[pos]='.';break;}
        pos--;
    }
    // MDH@15NOV2019: if this is an existing type
    if(nitems>0){ // we know both how many items AND where
        _allocationcounts[allocationtypecountoffset]-=nitems; // subtract the count
        _allocationcounts[0]-=nitems;
        _allocationcounts[1]-=(nitems*typesize);
    }
}

void* Mrealloc(void* ptr,size_t size,char type){
    // kind of like 'freeing' the space ptr is using now
    // step 1. take out what has been registered before...
    long long freed=(ptr?sizeof(*ptr):0); // best to determine it here
    void* newptr=realloc(ptr,size);
    long long more=(newptr?sizeof(*newptr):0)-freed; // what we need to add
    if(more!=0){ // the difference between what we had and what we have now
        if(_allocationtypes&&_allocationcounts){
            char* _allocationtype=strchr(_allocationtypes,type);
            if(_allocationtype){
                size_t allocationtypecountoffset=(_allocationtype-_allocationtypes)<<1; // double the index to get at the first position of the size_t pair
                if(allocationtypecountoffset>0){
                    if(_allocationcounts[allocationtypecountoffset+1]!=1){
                        printf("WARNING: Mrealloc() called on a data type that does not occupy a single byte.");
                        more/=_allocationcounts[allocationtypecountoffset+1]; // which might round and we are in trouble!!!!
                    }
                    _allocationcounts[allocationtypecountoffset]+=more; // subtract the count
                    _allocationcounts[0]+=more;
                    _allocationcounts[1]+=more;
                }else
                    printf("BUG: Mrealloc() called on the global data type (*).\n");
            }else 
                printf("BUG: Mrealloc() called on an unknown data type pointer.\n");
        }
    }
    return newptr;
}
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// helper functions
// we need a local something to store the allocation types in
struct{
    size_t l; // the number of allocation types stored
    char* chars; // the allocation type characters
}allocationtypes;

// you HAVE to call this method to be able to register allocations
bool allocationrecordinginitialized(){
    allocationtypes.l=0;
#ifdef __ADEBUG__
    allocationtypes.chars=malloc(16); // starting out with one block
#else
    allocationtypes.chars=NULL;
#endif
    return(allocationtypes.chars!=NULL);
    // replacing: if(!allocationtypes.chars)printf("ERROR: Failed to initialize recording allocations.\n");else printf("Allocation recording initialized.\n");
}

size_t addallocationtype(char allocationtype){
    if(!allocationtype)return 0; // force using allocationtype to prevent unused-parameter warning
#ifdef __ADEBUG__
    // increase size if necessary
    if(allocationtypes.chars&&!(allocationtypes.l&0xF))allocationtypes.chars=realloc(allocationtypes.chars,(allocationtypes.l+16)); // add a 'block' if now full
    if(!allocationtypes.chars)return 0;
    allocationtypes.chars[allocationtypes.l++]=allocationtype;
    return allocationtypes.l;
#else
    return 0;
#endif
}

// 'public' functions
size_t allocationmark(){return addallocationtype(' ');}

// unmark allocation returns the total number of encountered allocations
size_t unmarkallocation(size_t mark){
    if(!allocationtypes.chars){printf("No allocation recording!\n");return 0;}
    if(mark==0){printf("No mark!\n");return 0;}
#ifdef __ADEBUG__
    if(mark>allocationtypes.l){printf("Mark %zu too large.\n",mark);return 0;}
    if(allocationtypes.chars[mark-1]!=' '){printf("No mark at position %zu!\n",mark);return 0;}
    allocationtypes.l=mark-1;
#endif
    return allocationtypes.l; // returning what's left!!!
}
void allocationreport(size_t mark){
    if(mark==0||allocationtypes.chars==NULL)return;
#ifdef __ADEBUG__
    printf("%s","Allocations: '");
    size_t pos=mark-1;
    while(pos<allocationtypes.l)printf("%c",allocationtypes.chars[pos++]);
    printf("%c",'\'');
    printf("\n");
#endif
}
void syncallocations(){
#ifdef __ADEBUG__
    if(!allocationtypes.chars)return;
    while(allocationtypes.l>0){if(allocationtypes.chars[allocationtypes.l-1]!='.')break;allocationtypes.l--;}
    allocationreport(1);
#endif
}

#ifdef __ADEBUG__
void* Mmalloc(size_t size,char type){
    void* ptr=malloc(size);
    if(ptr)addallocationtype(type);
    return ptr;
}

void* Mcalloc(size_t nitems,size_t size,char type){
    void* ptr=calloc(nitems,size);
    if(ptr)addallocationtype(type);
    return ptr;
}

void Mfree(void* ptr,char type){
    if(!ptr)return;
    free(ptr);
    // undo the allocation of the given type
    if(!allocationtypes.chars){printf("Allocation types not recorded!\n");return;}
    if(allocationtypes.l==0){printf("Nothing allocated to free.\n");return;}
    size_t pos=allocationtypes.l-1;
    while(1){
        if(allocationtypes.chars[pos]==' '){printf("Allocation of type '%c' not encountered.\n",type);break;}
        if(allocationtypes.chars[pos]==type){allocationtypes.chars[pos]='.';break;}
        if(pos==0)break;
        pos--;
    }
}
#endif

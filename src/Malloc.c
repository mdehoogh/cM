#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "Malloc.h"

static uint16_t const MODULE_ID=3;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}

static char const * const MODULE_NAMES[]={"M","Moutput","Mmessage","Malloc","Mchars","Mstring","Msettings","Mexpression","Mmemory","Mexecution","Mbiginteger","Mrational","Mdecimal","Mvalue","Mlist","Mfunctions","Menvironment","Mshell"};
static char const * const DISOWNED_FLAG_TEXTS[]={"","-"};
static char const * const GLOBAL_FLAG_TEXTS[]={"f","m"};
static char const * const FREED_FLAG_TEXTS[]={"","X"};

extern char const * const M_ERROR_PREFIX;
extern char const * const M_WARNING_PREFIX;
extern char const * const M_BUG_PREFIX;

static char const * const HMS_FORMAT_STRING="%H:%M:%S";

static void info(char const * fmt,...){
    // va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);
}
static void warning(char const * fmt,...){printf("%s",M_WARNING_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);printf("%c",'\n');}
static void error(char const * fmt,...){printf("%s",M_ERROR_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);printf("%c",'\n');}
static void bug(char const * fmt,...){printf("%s",M_BUG_PREFIX);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);printf("%c",'\n');}

// MDH@26MAY2020: all char types changed to signed char so we can use the sign bit to indicate that it's a variable sized allocation unless we make all types variable sized
#ifndef __PRODUCTION__
typedef struct{
    int32_t allocationIndex; // MDH@19MAY2020: assuming 32 bits will suffice
    Mallocationowner owner; // MDH@19MAY2020: storing the owner id as well
    signed char allocationType;
}Malloc;
// MDH@22MAY2020: assuming that the moduleId is below 1024, and the functionId below 1024^2
//Mallocationowner getOwner(uint16_t moduleId,uint32_t functionId){return(Mallocationowner){0,moduleId,0,functionId};}
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
static Mallocationtype* _allocationTypes=NULL; // the unique allocation type characters
static unsigned long long numberOfAllocationTypes=0; // keep track of the number of allocation types
// MDH@14APR2020 now being stored as part of the allocationtypes: size_t* _allocationcounts=NULL; // the size of the type is stored every odd size_t

// MDH@07MAY2020: keep track of all allocation marks
static unsigned long long firstActiveAllocationMark=0,lastActiveAllocationMark=-1; // MDH@11MAY2020: as marks get deleted (from the back the firstAllocationMark changes)
static unsigned long long numberOfAllocationMarks=0; // we need at least one mark (TODO this could change if we decide to not do this in the production version)
static unsigned long long numberOfAllocationMarkTypes=0; // keep track of the number of types we have marks of
static Mallocationmark* _allocationTypeMarks=NULL;
static char* *_allocationTypeMarkIds=NULL; // the pointer to the list of allocation type mark ids

// as we're expecting more marks to be added then types we do mark 1 type 1, mark 1 type 2, etc.
// every time a new allocation type is created we need to expand the allocation marks
void outputAllocationTypeMarks(char* linePrefix){
    size_t size;
    unsigned long long freed,occupied,totaloccupied,totalfreed;
    // output("%llu allocation marks of %llu types:\n",numberOfAllocationMarks,numberOfAllocationMarkTypes);
    // I suppose it's best to write the types first
    output("%s#\tTime\tType ->",linePrefix);
    for(unsigned long long allocationMarkTypeIndex=0;allocationMarkTypeIndex<numberOfAllocationMarkTypes;allocationMarkTypeIndex++)
        if(_allocationTypes[allocationMarkTypeIndex].size>0)
            output("\t%c",_allocationTypes[allocationMarkTypeIndex].type);
    output("\tTotal (bytes)\n");
    // how about showing the oldest until the newest
    unsigned long long allocationMarkIndex=(lastActiveAllocationMark+1)%numberOfAllocationMarks; // the successor of the last active allocation mark
    while(1){
        output("%s%llu\t%s",linePrefix,allocationMarkIndex+1,_allocationTypeMarkIds[allocationMarkIndex]);
        if(allocationMarkIndex>=firstActiveAllocationMark||allocationMarkIndex<=lastActiveAllocationMark)outputChar('*'); // mark an active one with an asterisk
        totaloccupied=totalfreed=0; // the total we're reporting for the mark (at the end)
        for(unsigned long long allocationMarkTypeIndex=0;allocationMarkTypeIndex<numberOfAllocationMarkTypes;allocationMarkTypeIndex++){
            size=_allocationTypes[allocationMarkTypeIndex].size;
            if(size==0)continue; // only the case for the * allocation type mark
            outputChar('\t');// output(" %c:",_allocationTypes[allocationMarkTypeIndex].type);
            // how about showing the number of elements instead of the size??????
            occupied=_allocationTypeMarks[allocationMarkIndex*numberOfAllocationMarkTypes+allocationMarkTypeIndex].occupied;
            freed=_allocationTypeMarks[allocationMarkIndex*numberOfAllocationMarkTypes+allocationMarkTypeIndex].freed;
            output("%llu",(occupied-freed)/size);
            totaloccupied+=occupied;
            totalfreed+=freed;
        }
        output("\t%llu = %llu - %llu",totaloccupied-totalfreed,totaloccupied,totalfreed);
        outputChar('\n');
        if(allocationMarkIndex==lastActiveAllocationMark)break; // final active allocation mark output
        allocationMarkIndex=(allocationMarkIndex+1)%numberOfAllocationMarks;
    }
}
static bool updateAllocationTypeMarks(){
    // ascertain that numberOfAllocationMarkTypes is at least numberOfAllocationTypes
    // ASSERT numberOfAllocationMarkTypes should NEVER be larger than numberOfAllocationTypes
    if(numberOfAllocationMarks>0){
        unsigned long long numberOfNewAllocationMarkTypes=numberOfAllocationTypes-numberOfAllocationMarkTypes; // the number of new allocation type marks we need
        if(numberOfNewAllocationMarkTypes>0){
            // outputAllocationTypeMarks();
            unsigned long long totalnumberOfAllocationMarks=numberOfAllocationTypes*numberOfAllocationMarks; // the number of allocation type mark elements we need
            size_t allocationMarksSize=sizeof(Mallocationmark)*totalnumberOfAllocationMarks; // the maximum size we need
            Mallocationmark* newAllocationTypeMarks=(!_allocationTypeMarks?malloc(allocationMarksSize):realloc(_allocationTypeMarks,allocationMarksSize));
            if(newAllocationTypeMarks){
                _allocationTypeMarks=newAllocationTypeMarks;
                // we'll have to do some shifting...
                // because we do NOT need to shift the first mark we can use unsigned long long for allocationTypeMark
                unsigned long long shift=numberOfAllocationMarks*numberOfNewAllocationMarkTypes;
                unsigned long long allocationTypeMarkIndex=totalnumberOfAllocationMarks; // one above the last one we have to change
                unsigned long long allocationMarkType;
                while(1){
                    shift-=numberOfNewAllocationMarkTypes;
                    allocationMarkType=numberOfAllocationTypes; // ASSERT must be positive
                    while(1){
                        allocationTypeMarkIndex--;
                        if(allocationTypeMarkIndex<numberOfAllocationMarkTypes)break; // no need to initialize/move the first mark values
                        if(allocationMarkType>numberOfAllocationMarkTypes){ // haven't got it yet
                            _allocationTypeMarks[allocationTypeMarkIndex].occupied=0;
                            _allocationTypeMarks[allocationTypeMarkIndex].freed=0;
                        }else
                            _allocationTypeMarks[allocationTypeMarkIndex]=_allocationTypeMarks[allocationTypeMarkIndex-shift];
                        allocationMarkType--;
                        if(allocationMarkType==0)break;
                    }
                    if(shift==0)break; // done with last initialization/move
                }
                numberOfAllocationMarkTypes=numberOfAllocationTypes;
                // outputAllocationTypeMarks();
            }else
                error("Failed to update the allocation marks.\n");
        }
    }
    // if we have it we return true, false otherwise
    return(numberOfAllocationMarkTypes>=numberOfAllocationTypes);
}
// MDH@07MAY2020 END

// helper functions
// we need a local something to store the allocation types in
// MDH@18MAY2020: we are not just going to store the allocation type (a char) but also the id of the owner (i.e. every allocation will be owned)
typedef struct{
    Mallocationowner owner;
    signed char type;
}Mallocationownertype;

struct{
    long long l; // the number of allocation types stored
    Mallocationownertype* _owners; // the allocation type owners
}allocations={0,NULL};

// you HAVE to call this method to be able to register allocations
bool allocationRecordingInitialized(){
    
    bool result=false;

    printf("Initializing allocation recording...\n");

    // MDH@13MAY2020: remove whatever is currently allocated
    if(allocations._owners){
        free(allocations._owners);
        allocations._owners=NULL;
        printf("\tHistory of allocation type ids released...\n");
    }
    if(allocations.l>0){printf("\t%lld registered allocation type ids released...\n",allocations.l);allocations.l=0;}

    if(_allocationTypeMarks){free(_allocationTypeMarks);_allocationTypeMarks=NULL;printf("\tHistory of allocation marks released...\n");} 
    if(numberOfAllocationMarks*numberOfAllocationMarkTypes>0)printf("\t%lld allocation marks of %lld types released...\n",numberOfAllocationMarks,numberOfAllocationMarkTypes);
    numberOfAllocationMarks=0;numberOfAllocationMarkTypes=0;

#ifndef __PRODUCTION__
    allocations._owners=calloc(16,sizeof(Mallocationownertype)); // starting out with one block
    if(!allocations._owners)return false;
    printf("\tHistory of allocation type ids initialized...\n");

    // keep all current allocation types (if any)
    if(numberOfAllocationTypes==0){
        _allocationTypes=calloc(1,sizeof(Mallocationtype));
        if(_allocationTypes){
            numberOfAllocationTypes=1;
            _allocationTypes[0].type='*';
        } // always keep track of the total allocation count, which we mark with a wildcard *
    }
    if(numberOfAllocationTypes==0)return false;
    printf("\tAllocation type registration initialized...\n");
        // MDH@14APR2020: if(numberofallocationtypes>0)_allocationcounts=calloc(5,sizeof(size_t)); // start out with two size_t items one to store the count and one to store the size!!
#endif
    // MDH@12MAY2020: I suppose that if we have allocation types we can create them
    // assuming we have a single (global) allocation type (i.e. *)
    _allocationTypeMarkIds=calloc(1,sizeof(char*)); // a single char* that is initialized to NULL!!!!
    if(!_allocationTypeMarkIds)return false;

    time_t now=time(NULL);struct tm * nowlocal=localtime(&now);char hms[9];strftime(hms,9,HMS_FORMAT_STRING,nowlocal);
    _allocationTypeMarkIds[0]=strdup(hms);
    if(!_allocationTypeMarkIds[0]){free(_allocationTypeMarkIds);return false;}

    _allocationTypeMarks=calloc(numberOfAllocationTypes,sizeof(Mallocationmark));
    if(!_allocationTypeMarks){free(_allocationTypeMarkIds);return false;}
    
    numberOfAllocationMarks=1;
    firstActiveAllocationMark=0;
    lastActiveAllocationMark=0;
    numberOfAllocationMarkTypes=numberOfAllocationTypes;
    printf("\tAllocation marks registration initialized...\n");

    return true;
    // replacing: if(!allocations._chars)info("ERROR: Failed to initialize recording allocations.\n");else info("Allocation recording initialized.\n");
}

static long long getAllocationTypeIndex(signed char allocationType){
    long long allocationTypeIndex=numberOfAllocationTypes;
    while(--allocationTypeIndex>=0&&_allocationTypes[allocationTypeIndex].type!=allocationType)
    ;
    if(allocationTypeIndex>=0)info("Index of allocation type '%c'(=%i): %lld.\n",allocationType,allocationType,allocationTypeIndex);
    return allocationTypeIndex;
}

long long getNumberOfAllocationTypes(){return (_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/?numberOfAllocationTypes:0);}

// MDH@21APR2020: getNewAllocationTypeIndex() changed to also check whether the provided allocationType already exists 
static long long getNewAllocationTypeIndex(signed char allocationType,size_t size,long long count/*,bool fixedsize*/){
    if(_allocationTypes){
        long long newAllocationTypeIndex=getAllocationTypeIndex(allocationType);
        if(newAllocationTypeIndex<0){ // doesn't exist yet
            bool fixedsize=(allocationType>0);
            info("New allocation type #%lld: '%c'(=%i) of %s size %zd!\n",numberOfAllocationTypes,allocationType,allocationType,(fixedsize?"variable":"fixed"),size);
            Mallocationsize* _allocationTypeSizeHistogram=(fixedsize?NULL:calloc(1,sizeof(Mallocationsize)));
            if(fixedsize||_allocationTypeSizeHistogram){
                // how about allocating memory for the histogram beforehand?
                void* newAllocationTypes=realloc(_allocationTypes,sizeof(Mallocationtype)*(numberOfAllocationTypes+1));
                if(newAllocationTypes){   
                    info("New allocation type record created.\n");
                    newAllocationTypeIndex=numberOfAllocationTypes++; 
                    info("Number of allocation types: %lld.\n",numberOfAllocationTypes);
                    // MDH@14APR2020: _allocationcounts=realloc(_allocationcounts,(sizeof(size_t)*(allocationtypeindex+1))*5); // for every type we store 5 size_t values, one to keep the item count, and one to keep the size
                    _allocationTypes=newAllocationTypes;
                    _allocationTypes[newAllocationTypeIndex].type=allocationType;
                    _allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/.size=size;
                    // output("Size of allocation type #%lld ('%c'): %zd.\n",newAllocationTypeIndex,allocationType,_allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/.size);
                    updateAllocationTypeMarks(); // MDH@07MAY2020: to be called each time a new allocation type is added (i.e. when numberOfAllocationTypes is incremented!!!!!)
                    /* replacing:
                    // MDH@06MAY2020: essential to ascertain that occupied and freed are initialized to zero!!!!
                    _allocationTypes[newAllocationTypeIndex].occupied=0;
                    _allocationTypes[newAllocationTypeIndex].freed=0;
                    */
                    info("New allocation type '%c' registered!\n",allocationType);       
                    if(!fixedsize){
                        // output("Size of allocation type #%lld ('%c'): %zd.\n",newAllocationTypeIndex,allocationType,_allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/.size);
                        _allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/._allocationsizes=_allocationTypeSizeHistogram;
                        // output("Size of allocation type #%lld ('%c'): %zd.\n",newAllocationTypeIndex,allocationType,_allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/.size);
                        _allocationTypes[newAllocationTypeIndex].count=-1; // MDH@04MAY2020: counting down to indicate variable-size allocations
                        // this is a bit 'verwarrend' but count is the size of what's allocated, and it's always a single allocation (as stored in the count field)
                        _allocationTypeSizeHistogram->class=count;
                        _allocationTypeSizeHistogram->count=1;            
                    }else
                        _allocationTypes[newAllocationTypeIndex].count=count;
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
        // if(newAllocationTypeIndex>=0)output("Final size of allocation type #%lld ('%c'): %zd.\n",newAllocationTypeIndex,allocationType,_allocationTypes[newAllocationTypeIndex]/*.allocationsizeunion*/.size);
        return newAllocationTypeIndex;
    }
    error("No allocation types!");
    return -1;
}

// MDH@09APR2020: distinguish between adding an allocation (local) and adding an allocationtype (global)
// MDH@21APR2020: addAllocation() doesn't use size so we remove it from the parameter list
long long addAllocation(signed char type,Mallocationowner owner){
    // I suppose that the allocation might fail but we do NOT want to loose allocations._chars over it
    // MDH@14APR2020: there's room for improvement here
    // if(count<=0)return -2; // invalid input
    if(!allocations._owners)return -1; // no allocation characters
    info("Remembering an allocation of type '%c'.\n"/*,count*/,type);
    // MDH@21APR2020: consuming count is easier I suppose
    // removing: long long newl=allocations.l+count;
    // while(--count>=0){ // replacing: newl>allocations.l
        if(!(allocations.l&0xF)){ // allocations.l is a multiple of 16, so allocation._chars is full and we need a new block
            info("Expanding allocations.\n");
            Mallocationownertype* newAllocationOwners=realloc(allocations._owners,(allocations.l+16)*sizeof(Mallocationownertype));
            if(!newAllocationOwners){error("Allocation could not be remembered.\n");return 0;} // realloc failure
            allocations._owners=newAllocationOwners;
        }
        Mallocationownertype allocationowner={owner,type};
        allocations._owners[allocations.l]=allocationowner;
        info("Allocation of type '%c' (=%i) owned by %s:%u(%s%u%s%s) remembered at position %llu.\n",allocationowner.type,allocationowner.type
            ,MODULE_NAMES[allocationowner.owner.module],allocationowner.owner.id,GLOBAL_FLAG_TEXTS[allocationowner.owner.global],allocationowner.owner.level,DISOWNED_FLAG_TEXTS[allocationowner.owner.disowned],FREED_FLAG_TEXTS[allocationowner.owner.freed]
            ,allocations.l);
    // }
    return allocations.l++; // returning the position where the allocation is stored, and incrementing the length of the allocations unless we replace allocations.l by allocations.lastIndex
}

static void incrementAllocationTypeOccupied(long long allocationTypeIndex,unsigned long long increment){
    if(allocationTypeIndex<0)return; // should never happen though TODO make a bug
    if(!_allocationTypeMarks||numberOfAllocationMarks==0)return; // too bad
    if(allocationTypeIndex>=numberOfAllocationMarkTypes){warning("Type #%lld not markable.");return;}
    unsigned long long allocationTypeMarkIndex=lastActiveAllocationMark*numberOfAllocationMarkTypes+allocationTypeIndex;
    // output("Allocations of type '%c' at index %llu incremented from %llu",_allocationTypes[allocationTypeIndex].type,allocationTypeMarkIndex,_allocationTypeMarks[allocationTypeMarkIndex].occupied);
    _allocationTypeMarks[allocationTypeMarkIndex].occupied+=increment;
    // output(" to %llu.\n",_allocationTypeMarks[allocationTypeMarkIndex].occupied);
    // replacing: _allocationTypes[allocationTypeIndex].occupied+=increment; ///(histogram[category].class*_allocationTypes[allocationTypeIndex].allocationsizeunion.size);
}
static void incrementAllocationTypeFreed(long long allocationTypeIndex,unsigned long long increment){
    if(allocationTypeIndex<0)return; // should never happen though TODO make a bug
    if(!_allocationTypeMarks||numberOfAllocationMarks==0)return; // too bad
    if(allocationTypeIndex>=numberOfAllocationMarkTypes){warning("Type #%lld not markable.");return;}
    unsigned long long allocationTypeMarkIndex=lastActiveAllocationMark*numberOfAllocationMarkTypes+allocationTypeIndex;
    // output("Deallocations of type '%c' at index %llu incremented from %llu",_allocationTypes[allocationTypeIndex].type,allocationTypeMarkIndex,_allocationTypeMarks[allocationTypeMarkIndex].freed);
    _allocationTypeMarks[allocationTypeMarkIndex].freed+=increment;
    // output(" to %llu.\n",_allocationTypeMarks[allocationTypeMarkIndex].freed);
    // MDH@14MAY2020: if occupied is below freed something terribly wrong
    if(_allocationTypeMarks[allocationTypeMarkIndex].occupied<_allocationTypeMarks[allocationTypeMarkIndex].freed)
        bug("More memory freed than allocated for allocation type '%c'.",_allocationTypes[allocationTypeIndex].type);
    // replacing: _allocationTypes[allocationTypeIndex].occupied+=increment; ///(histogram[category].class*_allocationTypes[allocationTypeIndex].allocationsizeunion.size);
}
// MDH@14APR2020: addAllocationType renamed to registerAllocation
// MDH@03MAY2020: it's preferable to distinguish between a fixed-size allocation (always new), and a variable-size
//                (re)allocation possibly new (allocationIndex<0)
static long long registerReallocation(signed char type/*,Mallocationowner owner*/,size_t size,long long count,long long allocationIndex){
    if(type!=0&&size>0&&count>0){
        // MDH@05JUN2020 if this is a true reallocation (as we assume it is, no need to use getNewAllocationTypeIndex,
        long long allocationTypeIndex=getAllocationTypeIndex(type); // replacing: getNewAllocationTypeIndex(type,size,count/*,false*/);
        if(allocationTypeIndex>=0){ // yes, we should already have at least one allocation type
            // output("Current size of allocation type #%lld ('%c'): %zd.\n",allocationTypeIndex,_allocationTypes[allocationTypeIndex].type,_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size);
            // MDH@26MAY2020 should NOT happen : if(allocationIndex<0)allocationIndex=addAllocation(type,owner); // if new, register the allocation
            if(allocationIndex>=0){
                //t_allocationtype allocationType=_allocationTypes[allocationTypeIndex];
                // store in histogram
                Mallocationsize* histogram=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/._allocationsizes;
                if(!histogram)_allocationTypes[allocationTypeIndex].count=0; // MDH@29APR2020: precaution in case histogram pointer is undefined
                if(_allocationTypes[allocationTypeIndex].count>0)
                    bug("Invalid histogram category count of type %c: %lld.",type,_allocationTypes[allocationTypeIndex].count);
                long long numberOfHistogramCategories=llabs(_allocationTypes[allocationTypeIndex].count); // MDH@03JUN2020: TODO why won't - work????
                long long category=numberOfHistogramCategories; // MDH@04MAY2020: negate because we're counting backwards for variable-size allocations now
                while(--category>=0&&histogram[category].class!=count)
                ;
                if(category<0){ // does not yet exist
                    info("Adding category #%lld as %lld units (of size %zd) to the histogram of allocation type '%c'.\n",numberOfHistogramCategories+1,count,size,type);
                    if(histogram)
                        histogram=realloc(histogram,sizeof(Mallocationsize)*(numberOfHistogramCategories+1));
                    else
                        histogram=malloc(sizeof(Mallocationsize));
                    if(!histogram)return -1;
                    if(histogram){
                        _allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/._allocationsizes=histogram; // MDH@29APR2020 ADDITION: Oops, suppose this is important as well
                        category=numberOfHistogramCategories; // MDH@04MAY2020: the negative value of the count represents the number of histogram categories
                        _allocationTypes[allocationTypeIndex].count--; // another histogram category (and count represents the number of categories)
                        histogram[category].count=0; // will be incremented below!!!!
                        histogram[category].class=count;
                        info("Category #%lld of size %lld added to the histogram of allocation type '%c'.\n",-_allocationTypes[allocationTypeIndex].count,count,type);
                    }else
                        allocationIndex=-1;
                }
                if(category<0){
                    allocationIndex=-1;
                    error("Failed to count %lld allocation(s) of type '%c' and size %zd.\n",count,type,size);
                }else{
                    histogram[category].count++;
                    // MDH@04MAY2020: register the additional bytes (where class represents the number of units of what was reallocated and size the unit size)
                    // output("Size of allocation type #%lld ('%c'): %zd.\n",allocationTypeIndex,_allocationTypes[allocationTypeIndex].type,_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size);
                    unsigned long long increment=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size; //histogram[category].class;
                    // output("Occupied of allocation type #%lld ('%c'): %llu to be incremented by %zd.\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,increment);
                    increment*=histogram[category].class;
                    // output("Occupied of allocation type #%lld ('%c'): %llu to be incremented by %zd.\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,increment);
                    incrementAllocationTypeOccupied(allocationTypeIndex,increment);
                    // output("Occupied of allocation type #%lld ('%c'): %llu (incremented by %llu * %llu).\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,histogram[category],_allocationTypes[allocationTypeIndex].allocationsizeunion.size);
                }
            }else{
                error("Failed to add %lld allocation(s) of type '%c' and size %zd.\n",count,type,size);
                allocationIndex=-1;
            }
        }else{
            error("Failed to register allocation of type '%c' and size %zd (error code: %lld).\n",type,size,allocationTypeIndex);
            allocationIndex=-1;
        }
    }
    return allocationIndex;
}
// MDH@03MAY2020: now a fixed-size (always new) allocation 
// MDH@26MAY2020: now also accepts variable-size allocations (when type is negative), wait a minute I think registerAllocation is still to be called for fixed size allocations (with type>0), and registerReallocation() for variable-sized (re)allocations
//                wait a minute, registerReallocation does NOT require an owner (so should only be called from Mrealloc whereas registerAllocation() should be used for fixed AND variable-sized allocations!!)
static long long registerAllocation(signed char type,Mallocationowner owner,size_t size,long long count){
    long long allocationIndex=-1;
    if(type!=0&&type!='*'){
        // how about registering the type first if we need to???????
        // increase size if necessary
        if(size>0&&count>0){
            long long allocationTypeIndex=getNewAllocationTypeIndex(type,size,count/*,true*/);
            if(allocationTypeIndex>=0){ // yes, we should already have at least one allocation type
                allocationIndex=addAllocation(type,owner); // this is for registering the allocation BUT TODO should this be done here?????
                if(allocationIndex>=0){
                    if(type>0){ // fixed-size memory allocation
                        _allocationTypes[allocationTypeIndex].count+=count;
                        // MDH@04MAY2020: keep track of what we have allocated right now
                        incrementAllocationTypeOccupied(allocationTypeIndex,count*_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size); // MDH@07MAY2020
                        // replacing: _allocationTypes[allocationTypeIndex].occupied+=(count*_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size);
                    }else{ // variable-sized memory allocation
                        // store in histogram
                        Mallocationsize* histogram=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/._allocationsizes;
                        if(!histogram)_allocationTypes[allocationTypeIndex].count=0; // MDH@29APR2020: precaution in case histogram pointer is undefined
                        if(_allocationTypes[allocationTypeIndex].count>0)
                            bug("Invalid count for type %c: %lld.",type,_allocationTypes[allocationTypeIndex].count);
                        long long numberOfHistogramCategories=llabs(_allocationTypes[allocationTypeIndex].count); // MDH@03JUN2020: why doesn't - work????
                        long long category=numberOfHistogramCategories; // MDH@04MAY2020: negate because we're counting backwards for variable-size allocations now
                        while(--category>=0&&histogram[category].class!=count)
                        ;
                        if(category<0){ // does not yet exist
                            info("Adding category #%lld as %lld units (of size %zd) to the histogram of allocation type '%c'.\n",numberOfHistogramCategories+1,count,size,type);
                            if(histogram)
                                histogram=realloc(histogram,sizeof(Mallocationsize)*(numberOfHistogramCategories+1));
                            else
                                histogram=malloc(sizeof(Mallocationsize));
                            if(!histogram)return -1;
                            if(histogram){
                                _allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/._allocationsizes=histogram; // MDH@29APR2020 ADDITION: Oops, suppose this is important as well
                                category=numberOfHistogramCategories; // MDH@04MAY2020: the negative value of the count represents the number of histogram categories
                                _allocationTypes[allocationTypeIndex].count--; // another histogram category (and count represents the number of categories)
                                histogram[category].count=0; // will be incremented below!!!!
                                histogram[category].class=count;
                                info("Category #%lld of size %lld added to the histogram of allocation type '%c'.\n",-_allocationTypes[allocationTypeIndex].count,count,type);
                            }else
                                allocationIndex=-1;
                        }
                        if(category<0){
                            allocationIndex=-1;
                            error("Failed to count %lld allocation(s) of type '%c' and size %zd.\n",count,type,size);
                        }else{
                            histogram[category].count++;
                            // MDH@04MAY2020: register the additional bytes (where class represents the number of units of what was reallocated and size the unit size)
                            // output("Size of allocation type #%lld ('%c'): %zd.\n",allocationTypeIndex,_allocationTypes[allocationTypeIndex].type,_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size);
                            unsigned long long increment=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size; //histogram[category].class;
                            // output("Occupied of allocation type #%lld ('%c'): %llu to be incremented by %zd.\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,increment);
                            increment*=histogram[category].class;
                            // output("Occupied of allocation type #%lld ('%c'): %llu to be incremented by %zd.\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,increment);
                            incrementAllocationTypeOccupied(allocationTypeIndex,increment);
                            // output("Occupied of allocation type #%lld ('%c'): %llu (incremented by %llu * %llu).\n",allocationTypeIndex,type,_allocationTypes[allocationTypeIndex].occupied,histogram[category],_allocationTypes[allocationTypeIndex].allocationsizeunion.size);
                        }
                    }
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
static bool unregisterAllocation(long long allocationTypeIndex,long long count,bool fixedsize){
    // NOTE the given size is actually the count i.e. the number of units allocated
    bool result=false;
    if(allocationTypeIndex>=0&&allocationTypeIndex<numberOfAllocationTypes){
        if(count>0){
            if(!fixedsize){
                Mallocationtype allocationType=_allocationTypes[allocationTypeIndex];
                // lookup the size as size in the histogram (should be there)
                Mallocationsize* histogram=(allocationType.count<0?allocationType/*.allocationsizeunion*/._allocationsizes:NULL);
                if(histogram){
                    long long category=-allocationType.count;
                    while(--category>=0&&histogram[category].class!=count)
                    // printf(" %lld*%zd",histogram[category].count,histogram[category].class);
                    ;
                    // printf("\n");
                    if(category>=0){ // found it
                        if(histogram[category].count>0){
                            histogram[category].count--; // one down
                            result=true;
                        }else
                            bug("Unable to unregister the allocation of count %zd elements of type #%i '%c' (=%i): no registered allocation count.\n",count,allocationTypeIndex,allocationType.type,allocationType.type);
                    }else
                        bug("Unable to unregister the allocation of count %zd elements of type #%i '%c'(=%i): allocation type category unknown.\n",count,allocationTypeIndex,allocationType.type,allocationType.type);
                }
            }else{
                if(_allocationTypes[allocationTypeIndex].count>=count){
                    _allocationTypes[allocationTypeIndex].count-=count;
                    result=true;
                }else
                    bug("Failed to unregister %zd fixed-size allocation%s of type '%c'.\n",count,(count>1?"s":""),_allocationTypes[allocationTypeIndex].type);
            }
            // MDH@04MAY2020: increment freed with size times the fixed size of this allocation type
            // MDH@06MAY2020 BUG FIX: do NOT use allocationType here as that would be a copy not the original!!!!!
            if(result)incrementAllocationTypeFreed(allocationTypeIndex,_allocationTypes[allocationTypeIndex].size*count); 
            // replacing: _allocationTypes[allocationTypeIndex].freed+=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size*size; 
        }
    }
    return result;
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
Mallocationtype* _getAllocationTypes(){
    size_t allocationTypesSize=(_allocationTypes?numberOfAllocationTypes*sizeof(Mallocationtype):0);
    return(allocationTypesSize>0?memcpy(malloc(allocationTypesSize),_allocationTypes,allocationTypesSize):NULL);
}
// MDH@19NOV2019: 
bool resetAllocationTypes(){
    /////info("Resetting allocation type counts.\n");
    // best to free the lot, the reinitialize
    /* MDH@13MAY2020: now all addressed by allocationRecordingInitialized() (NOTE that we got a free error because we didn't NULL allocations._chars in the last line commented out)
    if(_allocationTypes)free(_allocationTypes);
    // MDH@14APR2020: if(_allocationcounts)free(_allocationcounts);
    if(allocations._chars)free(allocations._chars);
    */
    ////////info("Allocation type counts reset.\n");
    free(_allocationTypeMarks);numberOfAllocationMarks=0;free(_allocationTypeMarkIds);

    return allocationRecordingInitialized();

}

// MDH@25NOV2019: markAllocationTypes() remembers the current allocation type counts in the 4th and 5th element
// MDH@11MAY2020: either we can use a reusable allocation mark or append one
bool allocationMarkAdded(){
    if(_allocationTypeMarks){
        // if we can't increment the last active allocation mark without bumping into the first active allocation mark we have to add an allocation mark
        unsigned long long newLastActiveAllocationMark=(lastActiveAllocationMark+1)%numberOfAllocationMarks;
        if(firstActiveAllocationMark==newLastActiveAllocationMark){ // the first active allocation mark is right behind the last active allocation mark and has to be moved up
            char** newAllocationTypeMarkIds=realloc(_allocationTypeMarkIds,(numberOfAllocationMarks+1)*sizeof(char*));
            if(!newAllocationTypeMarkIds)return false;
            _allocationTypeMarkIds=newAllocationTypeMarkIds;
            // MDH@07MAY2020: we have to add a new mark
            size_t newNumberOfAllocationTypeMarks=(numberOfAllocationMarks+1)*numberOfAllocationMarkTypes;

            Mallocationmark* newAllocationTypeMarks=realloc(_allocationTypeMarks,newNumberOfAllocationTypeMarks*sizeof(Mallocationmark));
            if(!newAllocationTypeMarks)return false; // failure if unable to reallocate!!!!
            _allocationTypeMarks=newAllocationTypeMarks;
            if(newLastActiveAllocationMark>0){ // we need room at where the first active allocation mark is now (the oldest allocation mark)
                // we move all allocation marks one mark up starting at firstActiveAllocationMark up until numberOfAllocationMarks
                unsigned long long numberOfAllocationTypeMarks=(firstActiveAllocationMark+1)*numberOfAllocationMarkTypes; // the last allocation info to copy
                while(--newNumberOfAllocationTypeMarks>=numberOfAllocationTypeMarks)
                    _allocationTypeMarks[newNumberOfAllocationTypeMarks]=_allocationTypeMarks[newNumberOfAllocationTypeMarks-numberOfAllocationMarkTypes];
            }else // instead of writing the new mark at position 0, we can simply write it at the room we created!!!
                newLastActiveAllocationMark=numberOfAllocationMarks;
            numberOfAllocationMarks++;
        }else // free the mark id that is going to be replaced!!!!
        if(_allocationTypeMarkIds[newLastActiveAllocationMark])free(_allocationTypeMarkIds[newLastActiveAllocationMark]);

        // we have to make room for the new allocation and copy the current last active mark over
        // moving over lastActiveAllocationMark to newLastActiveAllocationMark (nonoverlapping allocation type marks so we can use memcpy)
        memcpy(_allocationTypeMarks+(newLastActiveAllocationMark*numberOfAllocationMarkTypes)
            ,_allocationTypeMarks+(lastActiveAllocationMark*numberOfAllocationMarkTypes)
            ,numberOfAllocationMarkTypes*sizeof(Mallocationmark));
        /* replacing:
        unsigned long long allocationMarkType=numberOfAllocationMarkTypes;
        while(1){
            allocationMarkType--;
            _allocationTypeMarks[newLastActiveAllocationMark*numberOfAllocationMarkTypes+allocationMarkType]=_allocationTypeMarks[lastActiveAllocationMark*numberOfAllocationMarkTypes+allocationMarkType];
            if(allocationMarkType==0)break;
        }
        */
        lastActiveAllocationMark=newLastActiveAllocationMark;

   }else{

        _allocationTypeMarkIds=calloc(1,sizeof(char*));
        if(!_allocationTypeMarkIds)return false;

        _allocationTypeMarks=calloc(numberOfAllocationTypes,sizeof(Mallocationmark));
        if(!_allocationTypeMarks){free(_allocationTypeMarkIds);return false;}

        numberOfAllocationMarks=1;firstActiveAllocationMark=0;lastActiveAllocationMark=0;numberOfAllocationMarkTypes=numberOfAllocationTypes;
        printf("\tAllocation marks registration initialized...\n");        
    }
 
    time_t now=time(NULL);struct tm * nowlocal=localtime(&now);char hms[9];strftime(hms,9,HMS_FORMAT_STRING,nowlocal);
    _allocationTypeMarkIds[lastActiveAllocationMark]=strdup(hms); // TODO for now assume that strdup() will NOT fail!!!! of course if it does it will return NULL so that's OK
 
    return true;
    /* replacing:
    long long numberOfAllocationTypes=getNumberOfAllocationTypes();
    while(numberOfAllocationTypes>0){
        numberOfAllocationTypes--;
        _allocationTypes[numberOfAllocationTypes].mark_occupied=_allocationTypes[numberOfAllocationTypes].occupied;
        _allocationTypes[numberOfAllocationTypes].mark_freed=_allocationTypes[numberOfAllocationTypes].freed;
        // // MDH@14APR2020 replacing:
        // _allocationcounts[5*numberOfAllocationTypes+3]=_allocationcounts[5*numberOfAllocationTypes+1];
        // _allocationcounts[5*numberOfAllocationTypes+4]=_allocationcounts[5*numberOfAllocationTypes+2];
    }
    */
}
long long getAllocationTypeOccupied(signed char allocationType,unsigned long long history){
    long long allocationTypeAllocated=-2; // if there's some error
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        if(allocationTypeIndex<0)return -1;
        // MDH@07MAY2020
        if(!_allocationTypeMarks||allocationTypeIndex>=numberOfAllocationMarkTypes)return -3;
        if(history>=numberOfAllocationMarks)return -4;
        allocationTypeAllocated=_allocationTypeMarks[numberOfAllocationMarkTypes*(numberOfAllocationMarks-1-history)+allocationTypeIndex].occupied;
        // replacing: allocationTypeAllocated=_allocationTypes[allocationTypeIndex].occupied;
    }
    return allocationTypeAllocated;
}
long long getAllocationTypeFreed(signed char allocationType, unsigned long long history){
    long long allocationTypeFreed=-2; // if there's some error
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        if(allocationTypeIndex<0)return -1;
        // MDH@07MAY2020
        if(!_allocationTypeMarks||allocationTypeIndex>=numberOfAllocationMarkTypes)return -3;
        if(history>=numberOfAllocationMarks)return -4;
        allocationTypeFreed=_allocationTypeMarks[numberOfAllocationMarkTypes*(numberOfAllocationMarks-1-history)+allocationTypeIndex].freed;
        // replacing: allocationTypeFreed=(allocationTypeIndex>=0?_allocationTypes[allocationTypeIndex].freed:-1);
    }
    return allocationTypeFreed;
}

// 'public' functions
// MDH@11MAY2020 some new functions
bool oldestAllocationMarkDropped(){
    if(firstActiveAllocationMark>=0){
        firstActiveAllocationMark=(firstActiveAllocationMark+1)%numberOfAllocationMarks;
        return true;
    }
    return false;
}
unsigned long long getNumberOfAllocationMarks(){return numberOfAllocationMarks;}

/* MDH@11MAY2020: replaced by other functions 
// MDH@21APR2020 the mark changed to \0 but we might consider using another character to indicate such a mark
long long allocationmark(){return addAllocation('\0');} // MDH@09APR2020: I think calling addallocation() suffices here, instead of addallocationtype

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
*/

#ifndef __PRODUCTION__
// MDH@21APR2020: general function to store allocation info with the dynamically allocated memory
static bool attachAllocationInfo(void* ptr,signed char type,Mallocationowner owner,size_t size,long long count){
    // ASSERT all arguments supposedly valid i.e. ptr!=NULL, size>0
    // MDH@13APR2020: all Mmalloc calls represent fixed size allocations
    if(ptr==NULL||type==0||size==0||count<=0)return false;
    // MDH@26MAY2020: now we should also accomodate for variable size allocations in which case we call registerReallocation with -1 for the allocationIndex (as this is a new allocation), see Mvalloc for a rewrite of Mrealloc with from_count=0 and to_count=1 with the required functionality)
    long long allocationIndex=registerAllocation(type,owner,size,count); // NOTE we do now how many items that are being allocated, so we assume size items of a single byte!!
    if(allocationIndex<0)return false;
    Malloc* _alloc=(Malloc*)ptr/*((char*)ptr+size)*/;
    _alloc->allocationType=type; // register the type
    _alloc->allocationIndex=allocationIndex;
    _alloc->owner=owner;
    /* MDH@03JUN2020: we do NOT want to disown something that is allocated, i.e. the receiving pointer module function actually owns it until disowning when returning the pointer as result
    _alloc->owner.disowned=1; // immediately disown the thing
    allocations._owners[allocationIndex].owner.disowned=1; // don't forget it in the registration!!!
    */
    return true;
}
#endif

/* MDH@26MAY2020: Mvalloc is a rewrite of Mrealloc for the case that from_count is 0 and to_count is 1 (i.e. freed is 0)
static void* Mvalloc(size_t size,unsigned char allocationType,Mallocationowner owner){
    void* newptr=NULL;
    if(0!=size){ // amount changed
        long long allocationIndex=-1;
        // MDH@14APR2020: if a (re)alloc use malloc if first time otherwise use realloc
        newptr=calloc(1,size+sizeof(Malloc)); // we have to reallocate nitems each of the given size
        if(!newptr)return NULL;
#ifndef __PRODUCTION__
        info("Number of dynamically allocated bytes: %zd.\n",size+sizeof(Malloc));
        // MDH@03MAY2020: we ALWAYS need to register the allocation
        long long allocationIndex=registerReallocation(allocationType,owner,size,1,-1);
        if(allocationIndex>=0){ // success
            info("New index of variable-size (re)allocation of type '%c': %lld\n",allocationIndex,allocationType);
            info("Storing allocation information...\n");
            Malloc* _alloc=(Malloc*)(((char*)newptr)+size); // MDH@21APR2020 BUG FIX: it said ptr instead of newptr here before which obviously was terribly wrong as ptr would be NULL on the first allocation
            // MDH@21APR2020 OK, mapping ptr to char* as we do seems to work: info("Number of bytes between start of dynamic data and allocation information: %zd.\n",(char*)_alloc-(char*)ptr);
            _alloc->allocationType=allocationType;
            _alloc->allocationIndex=allocationIndex;
            info("New allocation information stored...\n");
        }else
            bug("Failed to register the (re)allocation of a variable-size allocation of type '%c' from %lld to %lld.\n",allocationType,0,size);
#endif
    }
    return ((char*)newptr)+sizeof(Malloc);
}
*/
// MDH@08APR2020: if ptr starts with an allocation_index size_t field we can store the result of addallocation into it
//                so we have to ascertain that in the non-production version every structure that we allocate this way starts with
void* Mmalloc(size_t size,long long count,signed char type,Mallocationowner owner){
    void* ptr=NULL;
    if(size>0&&count>0&&type!=0){
        info("\n*************************** Allocating %zd * %lld bytes of dynamic memory of type '%c' ***************************\n",size,count,type);
// MDH@09APR2020: addallocation() is now addallocationtype()
#ifndef __PRODUCTION__
        ptr=malloc((size*count)+sizeof(Malloc));
#else
        ptr=malloc(size*count);
#endif
    }
    if(!ptr)return NULL;
#ifndef __PRODUCTION__
    if(!attachAllocationInfo(ptr,type,owner,size,count))
        bug("Failed to register %lld %s-sized allocation%s of type '%c'.\n",count,(type>0?"fixed":"variable"),abs(type),(count>1?"s":""));
#endif
    info("%p created by Mmalloc() owned by %s:%u(%s%u%s%s).\n",ptr
        ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
        );
    return ((char*)ptr)+sizeof(Malloc);
}

void* Mcalloc(size_t size,long long count,signed char type,Mallocationowner owner){
    void* ptr=NULL;
    if(size>0&&count>0&&type!=0){
        info("\n*************************** Allocating %zd * %lld initialized bytes of dynamic memory of type '%c' ***************************\n",size,count,type);
#ifndef __PRODUCTION__
        ptr=calloc(1,(size*count)+sizeof(Malloc)); // MDH@20APR2020: calloc doesn't care about the items!!!!!
#else
        ptr=calloc(count,size);
#endif
    }
    if(!ptr)return NULL;
#ifndef __PRODUCTION__
    if(!attachAllocationInfo(ptr,type,owner,size,count))
        bug("Failed to register %lld %s-size allocation%s of type '%c'.",count,(type>0?"fixed":"variable"),abs(type),(count>1?"s":""));
#endif
    info("%p created by Mcalloc() owned by %s:%u(%s%u%s%s).\n",ptr
        ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
        );
    return ((char*)ptr)+sizeof(Malloc);
}

// MDH@18MAY2020: passing along ownership is done through macros DISOWNED and OWNED 
//                unfortunately we need to know the size so we can find the allocation id
// MDH@02JUN2020: yes, we do need to register current ownership 'globally' i.e. not just in the memory 'record' itself because we do not know where the pointer is
#ifndef __PRODUCTION__
void* Mdisowned(void* ptr/*,size_t size*/,Mallocationowner owner){
    if(!ptr)return NULL;
    Malloc* _alloc=(Malloc*)(((char*)ptr)-sizeof(Malloc)/*+size*/);
    info("%p: Releasing owner %s:%u(%s%u%s%s) by %s:%u(%s%u%s%s).\n",_alloc
        ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
        ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
        );
    if(_alloc->allocationIndex>0){
        // info("Disowned: %p\n",_alloc);
        info("\tAllocation #%i=%s:%u(%s%u%s%s).\n",_alloc->allocationIndex
            ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,_alloc->owner.global,_alloc->owner.level,_alloc->owner.disowned,_alloc->owner.freed);
        // you can only disown what you own!!
        if(owner.disowned==0&&owner.id>0){
            // let's toggle the ownership if it matches
            Mallocationowner *_owner=&(allocations._owners[_alloc->allocationIndex].owner);
            if(_owner->id==owner.id){
                allocations._owners[_alloc->allocationIndex].owner.disowned=1; // replacing: _owner->disowned=1;
                _alloc->owner.disowned=1; // TODO we might have to comment this out in due course
            }else
                bug("\tUnable to remove ownership %s:%u(%s%u%s%s) of a memory allocation: it is owned by %s:%u(%s%u%s%s)."
                    ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
                    ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]
                    );
        }else
            bug("\tCan't release the ownership of an invalid owner.");
    }else
        bug("\tFailed to disown a memory allocation: it is not registered.");
    info("\tDisowned by (%s:%u,%u,%u,%u,%u).\n",MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]);
    return ptr;
}
void* Mowned(void* ptr/*,size_t size*/,Mallocationowner owner){
    if(!ptr)return NULL;
    Malloc* _alloc=(Malloc*)(((char*)ptr)-sizeof(Malloc)/* MDH@20MAY2020: +size*/);
    // info("\tPointer allocation=(%i,%i,%x).\n",_alloc->allocationIndex,_alloc->owner.level,_alloc->owner.disowned,_alloc->owner.id);
    info("%p: %s:%u(%s%u%s%s) taking over allocation owned by %s:%u(%s%u%s%s).\n",_alloc
        ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
        ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
        );
    // printf("S");
    // MDH@20MAY2020: you can only own something if disowned by the previous owner (in which case ownerId should be negative)
    // info("Owned %p:\n",_alloc);
    // printf("%s","U");
    if(_alloc->allocationIndex>0){
        Mallocationowner *_owner=&(allocations._owners[_alloc->allocationIndex].owner); // MDH@02JUN2020: pointing to where the owner of the allocation is registered
        info("\tAllocation #%i=%s:%u(%s%u%s%s).\n",_alloc->allocationIndex
            ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]
            );
        if(owner.disowned==0&&owner.id>0&&owner.freed==0){
            // pass ownership to owner if ptr is currently disowned
            // printf("%s","X");
            if(_alloc->owner.disowned!=_owner->disowned)bug("\tUnsynced ownership flags.");
            if(_owner->disowned!=0){
                *_owner=owner;
                // printf("\tOwnership of %s:%u(%s%u%s%s)"
                //     ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
                // );
                _alloc->owner=*_owner; // TODO we might have to comment this out in due course
                // printf(" taken by %s:%u(%s%u%s%s).\n"
                //     ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]
                //     );
            }else
            if(_owner->id==0)
                bug("\tOwner %s:%u(%s%u%s%s) cannot take over ownership of a memory allocation: it is not owned anymore."
                    ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
                    );
            else
                bug("\tOwner %s:%u(%s%u%s%s) cannot take over ownership: it is still owned by %s:%u(%s%u%s%s)."
                    ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
                    ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]
                    );
            // printf("%s","Y");
        }else
            bug("\tCan't set the ownership of a memory allocation to invalid owner %s:%u(%s%u%s%s)."
                ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
                );
    }else
        bug("\tFailed to disown a memory allocation: it is not registered.");
    info("\tOwned by %s:%u(%s%u%s%s).\n",MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]);
    return ptr;
}
void* Msubowned(void* ptr,uint8_t level){
    if(!ptr)return NULL;
    Malloc* _alloc=(Malloc*)(((char*)ptr)-sizeof(Malloc)/* MDH@20MAY2020: +size*/);
    info("%p: Incrementing level of owner %s:%u(%s%u%s%s) by %i.\n",_alloc
        ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
        ,level);
    //replacing: info("Incrementing subownership of %p by %i.\n",_alloc,level);
    // info("Subowning %p:\n",ptr);
    if(_alloc->allocationIndex>=0&&_alloc->allocationIndex<allocations.l){
        Mallocationowner* _owner=&(allocations._owners[_alloc->allocationIndex].owner);
        info("\tAllocation #%i=%s:%u(%s%u%s%s).\n",_alloc->allocationIndex
            ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]);
        if(_owner->id>0&&_owner->disowned==0){
            if(_owner->level==256-level)return NULL; // MDH@22MAY2020: shouldn't happen though!!!
            _owner->level+=level; // simply increment the owner level
            _alloc->owner.level=_owner->level; // TODO might be removed in due course
            info("\tSubownership established by owner %s:%u(%s%u%s%s).\n"
                ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]);
        }else
            bug("\tCan't subown a disowned or unowned memory allocation %s:%u(%s%u%s%s)."
                ,MODULE_NAMES[_owner->module],_owner->id,GLOBAL_FLAG_TEXTS[_owner->global],_owner->level,DISOWNED_FLAG_TEXTS[_owner->disowned],FREED_FLAG_TEXTS[_owner->freed]);
    }else
        bug("\tFailed to subown memory allocation %s:%u(%s%u%s%s): it is not registered (index: %llu)."
                ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
                ,_alloc->allocationIndex);
    return ptr;
}
// MDH@25MAY2020 careful here Msubowner result is supposed to be a local variable (on the program stack) so it will be disposed off 'automagically'
Mallocationowner Msubowner(Mallocationowner owner,uint8_t level){
    return (Mallocationowner){owner.module,owner.id,owner.global,owner.level+level,owner.disowned,owner.freed};
}
/*
void* Mownedby(void* ptr,Mallocationowner owner){
    if(!ptr)return NULL;
    Malloc* _alloc=(Malloc*)(((char*)ptr)-sizeof(Malloc));
    if(_alloc->allocationIndex>0){
        if(owner.id>0&&owner.disowned==0){
            Mallocationowner* _owner=&(allocations._owners[_alloc->allocationIndex].owner);
            *_owner=owner; // MDH@02JUN2020 TODO should work!
            if(owner.level==255)return NULL; // MDH@22MAY2020: shouldn't happen though!!!
            _owner->level++; // this is easiest (could've called Msubowned but )
        }else
            bug("Can't set subownership of an undefined or disowned or unowned memory location.");
    }else
        bug("Failed to register a memory allocation a subowned: it is not registered.");
    return ptr;
}
*/
// int32_t Mowner(void* ptr,size_t size){
//     if(ptr&&size>0){
//         Malloc* _alloc=(Malloc*)(((char*)ptr)+size);
//         return _alloc->ownerId;
//     }
//     return 0;
// }
#endif

/* Mvfree is a rewrite of the original Mrealloc (for all memory (de)allocation of variable-sized allocations) with from_count>0 and to_count=0
void Mvfree(){

}
*/
// MDH@20APR2020: unfortunately we need to know the size of what was allocated which is easy for fixed size allocation but problematic for variable size records
//                unless we assume that Mfree is always called on fixed size allocations which require that a single item is allocated each time, so we don't need nitems on Mmalloc and Mcalloc
void Mfree(void const * const ptr,long long count,signed char allocationType,Mallocationowner owner){
    if(!ptr||allocationType==0||count<=0)return;
    Malloc* _alloc=(Malloc*)(((char*)ptr)-sizeof(Malloc)); // MDH@20MAY2020 added because we have moved the allocation record to the start instead of the end!!!
    info("\n*************************** Freeing dynamic memory of type '%c' ***************************\n",abs(allocationType));
    info("%p: owned by %s:%u(%s%u%s%s) being freed by %s:%u(%s%u%s%s).\n",_alloc
        ,MODULE_NAMES[_alloc->owner.module],_alloc->owner.id,GLOBAL_FLAG_TEXTS[_alloc->owner.global],_alloc->owner.level,DISOWNED_FLAG_TEXTS[_alloc->owner.disowned],FREED_FLAG_TEXTS[_alloc->owner.freed]
        ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]);
    /////info("Freeing type '%c' data",type);
    // determine the amount of items to free which depends on the type size!!
    // MDH@14APR2020: size_t nitems=0,typesize=0,allocationtypecountoffset=0;
    size_t size=0; // MDH@20APR2020: we need to determine the size from what we stored with the allocation type
    if(_allocationTypes/* MDH@14APR2020: &&_allocationcounts*/){
        long long allocationTypeIndex=getAllocationTypeIndex(allocationType);
        // assume a size 1 thing if it's not there yet????? (typically only for testing though!!!)
        if(allocationTypeIndex>=0){ // MDH@07APR2020: better to NOT create the new allocation type if not currently known!!!!
            size=_allocationTypes[allocationTypeIndex]/*.allocationsizeunion*/.size; // extract the (fixed) size
            if(!unregisterAllocation(allocationTypeIndex,count,allocationType>0))
                bug("Failed to free the dynamic memory of %llu allocation%s of type '%c' (size: %zd).",count,(count>1?"s":""),allocationType,size); // MDH@02MAY2020: we have to decrement the count (representing the number of allocated instances) by 1
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
            bug("Memory of unknown type '%c' to be freed!",allocationType);
    }
    // MDH@14APR2020 NOTE: the following is about removing the allocation
#ifndef __PRODUCTION__
    if(_alloc->allocationType!=allocationType){
        bug("\tAllocation type of dynamic memory '%c' (=%i) freed of size %zd does not match provided allocation type '%c' (=%i).",_alloc->allocationType,_alloc->allocationType,size,allocationType,allocationType);
        dump(ptr,count*size,size);
        free(_alloc);
        return;
    }
    // if(_alloc->allocationIndex<=0)bug("No allocation index registered for allocation of type '%c'.\n",allocationType);
#endif
    if(!allocations._owners)error("Allocation owners not recorded!\n");
    if(allocations.l==0)warning("Nothing allocated to free.\n");
#ifndef __PRODUCTION__
    // MDH@04JUN2020: we can check ownership here BUT when a subowned allocation is freed by the superowner, which might have changed ownership the subowned allocation can still be freed as long as the levels match
    if(_alloc->allocationIndex>=0&&_alloc->allocationIndex<allocations.l){
        if(allocations._owners[_alloc->allocationIndex].owner.freed){
            allocations._owners[_alloc->allocationIndex].owner.freed=0;
            bug("\tAllocation of dynamic memory '%c' (=%i) of size %zd was already marked as free.");
        }
        if(allocations._owners[_alloc->allocationIndex].type!=allocationType){
            bug("\tAllocation type of dynamic memory '%c' (=%i) (at index %i) does not match provided allocation type '%c' (=%i).",allocations._owners[_alloc->allocationIndex].type,allocations._owners[_alloc->allocationIndex].type,_alloc->allocationIndex,allocationType,allocationType);
            dump(ptr,size*count,size);
        }else
        if(allocations._owners[_alloc->allocationIndex].owner.level!=owner.level+1&&(allocations._owners[_alloc->allocationIndex].owner.id!=owner.id||allocations._owners[_alloc->allocationIndex].owner.module!=owner.module)){
            Mallocationowner* _allocationowner=&(allocations._owners[_alloc->allocationIndex].owner);
            bug("\tAllocation owner of dynamic memory %s:%u(%s%u%s%s) (at index %i) does not match owner %s:%u(%s%u%s%s) trying to free the memory of type '%c' (=%i)."
            ,MODULE_NAMES[_allocationowner->module],_allocationowner->id,GLOBAL_FLAG_TEXTS[_allocationowner->global],_allocationowner->level,DISOWNED_FLAG_TEXTS[_allocationowner->disowned],FREED_FLAG_TEXTS[_allocationowner->freed]
            ,_alloc->allocationIndex
            ,MODULE_NAMES[owner.module],owner.id,GLOBAL_FLAG_TEXTS[owner.global],owner.level,DISOWNED_FLAG_TEXTS[owner.disowned],FREED_FLAG_TEXTS[owner.freed]
            ,allocationType,allocationType);
            dump(ptr,size*count,size);
        }else{
            // MDH@07JUN2020: it's going to suffice (see below) to set the freed flag once done (see the line below)
            /* replacing:
            // what we do here is the same as what Mdisown does!!!!
            allocations._owners[_alloc->allocationIndex].type=' '; // MDH@13APR2020: can't use ' ' as that's used for a command
            allocations._owners[_alloc->allocationIndex].owner.id=0; // MDH@18MAY2020
            allocations._owners[_alloc->allocationIndex].owner.module=0; // MDH@03JUN2020: TODO we might not want to do this
            */
        }
    }else{
        bug("\tRetrieved allocation position %llu out of range [0,%llu).",_alloc->allocationIndex,allocations.l);
        dump(ptr,size*count,size);
    }
#else
    long long pos=allocations.l-1;
    while(pos>0){
        if(allocations._chars[pos]==allocationType){allocations._chars[pos]=' ';break;}
        pos--;
    }
#endif
    free(_alloc);
#ifndef __PRODUCTION__
    // MDH@07JUN2020: if we get here we know free was sucessful and we should definitely mark the thing as freed
    // we may safely assume that _alloc was freed but its good that to set the freed flag so we know that the pointer was freed actually but still know the type
    // technically it might also be a good idea to have an additional flag that we can use to 
    if(_alloc->allocationIndex>=0&&_alloc->allocationIndex<allocations.l)
        allocations._owners[_alloc->allocationIndex].owner.freed=1; // MDH@13APR2020: can't use ' ' as that's used for a command
#endif
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

// MDH@27NOV2019: now passing the number of items in as well, and the current number of items
// MDH@09APR2020: from now on (v0.1.2) REALLOC is only to be used for all variable dynamic memory allocations
//                and also for freeing (i.e. when occupied equals zero)
// MDH@26MAY2020: rewrite assuming from_count and to_count are positive
void* Mrealloc(void* ptr,long long from_count,long long to_count,size_t size,signed char allocationType/*,Mallocationowner owner*/){
    // info("Size of Malloc: %zd, size of long long: %zd.\n",sizeof(Malloc),sizeof(long long));
    void* newptr=ptr;
    if(from_count>0&&to_count>0){ // not a (new) (de-)allocation

        if(from_count!=to_count){ // a change in the number of allocation elements
        
            newptr=((char*)newptr)-sizeof(Malloc);

            size_t freed=size*from_count,occupied=size*to_count;

            Malloc* _alloc=(Malloc*)newptr; // pointer to Malloc allocation registration appendix
            long long allocationIndex=-1;
#ifndef __PRODUCTION__
            // MDH@05JUN2020 realloc takes care of this: Malloc newAllocation=*_alloc; // MDH@05JUN2020: copy the entire record over
            // we need to get the allocation type and index out BEFORE memory is reallocated!!!!!
            if(_alloc->allocationType!=allocationType){
                bug("The allocation type '%c'(=%i) stored with the data at byte %zd does not match the provided allocation type '%c'.\n",_alloc->allocationType,_alloc->allocationType,freed,allocationType);
                // let's dump the current contents as text?
                dump(ptr,freed,size);
            }
            // MDH@20APR2020 ASSERT: freed>0 as freed!=occupied
            allocationIndex=_alloc->allocationIndex;
            if(allocationIndex>=0&&allocationIndex<allocations.l){
                if(allocations._owners[allocationIndex].type!=_alloc->allocationType){
                    bug("Type '%c' of remembered allocation #%llu does not match the provided allocation type '%c'.",allocations._owners[allocationIndex].type,allocationIndex,_alloc->allocationType);
                    dump(ptr,freed,size);
                }
                // MDH@22APR2020 BUG FIX: do NOT clear the remembered allocation type unless the memory is freed!!!!
                if(occupied==0) // MDH@22APR2020 ADDITION
                    allocations._owners[allocationIndex].type=' ';
            }else{
                bug("Allocation index %lld stored with the data at position %llu (resized to %llu) of size %zd is out of range [0,%llu)!",allocationIndex,freed,occupied,size,allocations.l);
                dump(ptr,freed,size);
            }
#endif
        // MDH@14APR2020: if a (re)alloc use malloc if first time otherwise use realloc
        // MDH@05JUN2020 NOTE: realloc will also copy the allocation record (_alloc) over, which means that we do not need to do it ourselves anymore
#ifndef __PRODUCTION__
            newptr=realloc(newptr,occupied+sizeof(Malloc));
#else
            newptr=realloc(newptr,occupied); // we have to reallocate nitems each of the given size
#endif
            info("Variable-size allocation of type '%c'(=%i) resized from %zd to %zd!\n",allocationType,allocationType,freed,occupied);
            // newptr is allowed to be NULL if occupied equals 
            if(newptr){ // success (newptr will be NULL when occupied==0, but that also indicates success)           
#ifndef __PRODUCTION__
                unregisterAllocation(getAllocationTypeIndex(allocationType),from_count,false); // MDH@28APR2020
                info("Number of dynamically allocated bytes: %zd.\n",occupied+sizeof(Malloc));
                // MDH@03MAY2020: we ALWAYS need to register the allocation
                long long allocationIndex=registerReallocation(allocationType,size,to_count,_alloc->allocationIndex);
                if(allocationIndex>=0){ // success
                    info("New index of variable-size (re)allocation of type '%c': %lld\n",allocationIndex,allocationType);
                    /* MDH@05JUN2020: no need for the following anymore
                    if(!_alloc){ // first time allocation (i.e. freed equals zero)
                        info("Storing allocation information...\n");
                        _alloc=(Malloc*)(((char*)newptr)+occupied); // MDH@21APR2020 BUG FIX: it said ptr instead of newptr here before which obviously was terribly wrong as ptr would be NULL on the first allocation
                        // MDH@21APR2020 OK, mapping ptr to char* as we do seems to work: info("Number of bytes between start of dynamic data and allocation information: %zd.\n",(char*)_alloc-(char*)ptr);
                        _alloc->allocationType=allocationType;
                        _alloc->allocationIndex=allocationIndex;
                        info("New allocation information stored...\n");
                    }else{ // not a first time allocation, so we can simply copy the allocation over
                        Malloc* _newalloc=(Malloc*)newptr;
                        _newalloc->allocationType=newAllocation.allocationType;
                        _newalloc->allocationIndex=newAllocation.allocationIndex;
                        // replacing: memcpy(_newalloc,_alloc,sizeof(Malloc)); // replacing:  *((Malloc*)(((char*)newptr)+occupied))=*_alloc; // copying the allocation structure over // OOPS ptr replaced by newptr (what it should be I guess)
                        info("Allocation information bytes copied...\n");
                        // printf("New allocation type %c (%c) - allocation index %llu (%llu).\n",_newalloc->allocationType,_alloc->allocationType,_newalloc->allocationIndex,_alloc->allocationIndex);
                    }
                    */
                }else
                    bug("Failed to register the (re)allocation of a variable-size allocation of type '%c' (=%i) from %lld to %lld.\n",allocationType,allocationType,freed,occupied);
#endif
                //                well, it might be a replacement
                newptr=((char*)newptr)+sizeof(Malloc);
            }
        }
    }else
        bug("%s.","Number of bytes to reallocate non-positive");
    return newptr;
}

/* replacing the original Mrealloc
void* Mrealloc(void* ptr,long long from_count,long long to_count,size_t size,signed char allocationType,Mallocationowner owner){
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
        Malloc* _alloc=(freed>0?(Malloc*)ptr:NULL); // pointer to Malloc allocation registration appendix
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
                if(allocations._owners[allocationIndex].type!=_alloc->allocationType){
                    bug("Type '%c' of remembered allocation #%llu does not match the provided allocation type '%c'.\n",allocations._owners[allocationIndex].type,allocationIndex,_alloc->allocationType);
                    dump(ptr,freed,size);
                }
                // MDH@22APR2020 BUG FIX: do NOT clear the remembered allocation type unless the memory is freed!!!!
                if(occupied==0) // MDH@22APR2020 ADDITION
                    allocations._owners[allocationIndex].type=' ';
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
            else // use calloc instead of malloc will automatically initialize occupied and freed as it should
                newptr=calloc(1,occupied+sizeof(Malloc)); // we have to reallocate nitems each of the given size
#else
            newptr=(freed>0?realloc(ptr,occupied):malloc(occupied)); // we have to reallocate nitems each of the given size
#endif
        }else
            free(ptr);
        info("Variable-size allocation of type '%c' resized from %zd to %zd!\n",allocationType,freed,occupied);
        // newptr is allowed to be NULL if occupied equals 
        if(newptr){ // success (newptr will be NULL when occupied==0, but that also indicates success)
            
#ifndef __PRODUCTION__
            if(freed>0){
                unregisterAllocation(getAllocationTypeIndex(allocationType),from_count,false); // MDH@28APR2020
            }
            if(occupied>0){
                info("Number of dynamically allocated bytes: %zd.\n",occupied+sizeof(Malloc));
                // MDH@03MAY2020: we ALWAYS need to register the allocation
                long long allocationIndex=registerReallocation(allocationType,owner,size,to_count,(_alloc?_alloc->allocationIndex:-1));
                if(allocationIndex>=0){ // success
                    info("New index of variable-size (re)allocation of type '%c': %lld\n",allocationIndex,allocationType);
                    if(!_alloc){ // first time allocation (i.e. freed equals zero)
                        info("Storing allocation information...\n");
                        _alloc=(Malloc*)(((char*)newptr)+occupied); // MDH@21APR2020 BUG FIX: it said ptr instead of newptr here before which obviously was terribly wrong as ptr would be NULL on the first allocation
                        // MDH@21APR2020 OK, mapping ptr to char* as we do seems to work: info("Number of bytes between start of dynamic data and allocation information: %zd.\n",(char*)_alloc-(char*)ptr);
                        _alloc->allocationType=allocationType;
                        _alloc->allocationIndex=allocationIndex;
                        info("New allocation information stored...\n");
                    }else{ // not a first time allocation, so we can simply copy the allocation over
                        Malloc* _newalloc=(Malloc*)newptr;
                        _newalloc->allocationType=newAllocation.allocationType;
                        _newalloc->allocationIndex=newAllocation.allocationIndex;
                        // replacing: memcpy(_newalloc,_alloc,sizeof(Malloc)); // replacing:  *((Malloc*)(((char*)newptr)+occupied))=*_alloc; // copying the allocation structure over // OOPS ptr replaced by newptr (what it should be I guess)
                        info("Allocation information bytes copied...\n");
                        // printf("New allocation type %c (%c) - allocation index %llu (%llu).\n",_newalloc->allocationType,_alloc->allocationType,_newalloc->allocationIndex,_alloc->allocationIndex);
                    }
                }else
                    bug("Failed to register the (re)allocation of a variable-size allocation of type '%c' from %lld to %lld.\n",allocationType,freed,occupied);
                //                well, it might be a replacement
            }
#endif

        }
    }
    return ((char*)newptr)+sizeof(Malloc);
}
*/
static long long getAllocationTypeSize(unsigned long long allocationTypeIndex,unsigned long long allocationMarkIndex){
    // output("Allocated by type '%c': %lld - %lld.\n",allocationType.type,allocationType.occupied,allocationType.freed);
    if(allocationTypeIndex<0||!_allocationTypeMarks||allocationTypeIndex>=numberOfAllocationMarkTypes||allocationMarkIndex>=numberOfAllocationMarks)return 0;
    Mallocationmark allocationMark=_allocationTypeMarks[allocationMarkIndex*numberOfAllocationMarkTypes+allocationTypeIndex];
    long long size=allocationMark.occupied;
    size-=allocationMark.freed;
    return size;
    // replacing: return(_allocationTypes[allocationTypeIndex].occupied-_allocationTypes[allocationTypeIndex].freed); // MDH@04MAY2020: assuming occupied and freed are kept up to date all the time
}
// MDH@07MAY2020: passing in the number of allocation marks requested (<0=one, 0=all, otherwise the number given, returning what is actually returned)
long long * _getAllocationTypeSizes(char const * const types,unsigned long long *_numberOfAllocationTypes,long long *_numberOfAllocationMarks){
    // *_numberOfAllocationMarks is the requested number of allocation marks, the total number of returned allocation types is returned in *_numberOfAllocationTypes
    
    if(!_numberOfAllocationTypes||!_numberOfAllocationMarks)return NULL;
    if(!_allocationTypeMarks||numberOfAllocationMarks==0)return NULL; // no marked allocations

    // compute the number of allocation types of which we will return marks
    unsigned long long numberOfAllocationTypesToReturn=(types?(strlen(types)==0?numberOfAllocationTypes:strlen(types)):0)+1;
    // compute the number of allocation marks to return (add 1 because we always return the total count)
    unsigned long long numberOfAllocationMarksToReturn=(_numberOfAllocationMarks<0?1:(*_numberOfAllocationMarks>0?*_numberOfAllocationMarks:numberOfAllocationMarks));
    if(numberOfAllocationMarksToReturn>numberOfAllocationMarks)numberOfAllocationMarksToReturn=numberOfAllocationMarks; // can't return more than we have
    
    // store the total number of allocation type records we return
    *_numberOfAllocationTypes=(numberOfAllocationMarksToReturn+1)*numberOfAllocationTypesToReturn; // per type we will be returning an additional long long in which the type character is stored
    
    // allocate exactly what we need (calloc will ascertain to initialize to zero all overall sizes in the first 'record')
    long long * _allocationTypeSizes=calloc(*_numberOfAllocationTypes,sizeof(unsigned long long));
    if(_allocationTypeSizes){
        *_numberOfAllocationMarks=numberOfAllocationMarksToReturn;
        // register the sizes fior each of the marks to return
        unsigned long long allocationTypeMark=0,allocationTypeSizeIndex=0;
        long long allocationTypeSize=0;
        while(++allocationTypeMark<=numberOfAllocationMarksToReturn){
            allocationTypeSizeIndex=0;
            for(unsigned long long allocationTypeIndex=0;allocationTypeIndex<numberOfAllocationTypes;allocationTypeIndex++){
                allocationTypeSize=getAllocationTypeSize(allocationTypeIndex,numberOfAllocationMarks-allocationTypeMark);
                // MDH@25MAY2020: abs'ing the stored type to also catch variable-size allocation types
                if(types&&(strlen(types)==0||strchr(types,abs(_allocationTypes[allocationTypeIndex].type)))){
                    allocationTypeSizeIndex++;
                    _allocationTypeSizes[allocationTypeMark+allocationTypeSizeIndex*(numberOfAllocationMarksToReturn+1)]=allocationTypeSize;
                }
                _allocationTypeSizes[allocationTypeMark]+=allocationTypeSize; // always register the size with the total record...
            }
        }
        // register the types
        _allocationTypeSizes[0]=(int)'*'; // if you insist
        allocationTypeSizeIndex=0;
        unsigned long long allocationTypeInfo=0;
        for(unsigned long long allocationTypeIndex=0;allocationTypeIndex<numberOfAllocationTypes;allocationTypeIndex++){
            if(types&&(strlen(types)==0||strchr(types,_allocationTypes[allocationTypeIndex].type))){
                allocationTypeSizeIndex++;
                // we can store a little more than just the type
                allocationTypeInfo=_allocationTypes[allocationTypeIndex].type;
                if(_allocationTypes[allocationTypeIndex]._allocationsizes)allocationTypeInfo+=128; // variable size
                allocationTypeInfo+=(_allocationTypes[allocationTypeIndex].size<<8); // shift in the size of the allocation type
                _allocationTypeSizes[allocationTypeSizeIndex*(numberOfAllocationMarksToReturn+1)]=allocationTypeInfo;
            }
            // if(allocationTypeSizeIndex==numberOfAllocationTypesToReturn)break;
        }
    }else
        *_numberOfAllocationTypes=0; // none returned!!!!
    return _allocationTypeSizes;
}
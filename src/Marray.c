#include "Marray.h"
// MDH@22NOV2020
static uint16_t const MODULE_ID=14;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}
extern long long M_LL_INVALID,M_TRUE,M_FALSE;

Mvalue* marray(Mvalue* length_value){Mallocationowner owner=getOwner(__LINE__);
    // returns an array that can store length_value values (if possible)
    long long length=getValueInteger(length_value);
    if(length>0){
        Marray* _array=owned_array(__array("marray"),owner);
        if(_array){
            _array->values=CALLOC(sizeof(Mvalue*),length,'a',Msubowner(owner,1));
            if(_array->values){_array->numberOfElements=length;return _getValueOfArray(disowned_array(_array,owner));}
        }
        FREE_ARRAY(_array,owner); // failed to initialize the array
    }
    return NULL;
}
// fill with value
Mvalue* mfill(Mvalue* array_value,Mvalue* value){
    Marray* array=(array_value&&array_value->type==VT_ARRAY?array_value->value._array:NULL);
    if(array){
        unsigned long long l=array->numberOfElements;
        if(l>0){
            if(array->values){
                // we use assignValue here we get a copy of maps and lists (and arrays for that matter)
                do{assignValue(&array->values[--l],value);}while(l>0);
            }
        }
    }
    return array_value;
}

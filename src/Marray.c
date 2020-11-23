#include "Marray.h"
// MDH@22NOV2020
static uint16_t const MODULE_ID=14;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}
extern long long M_LL_INVALID,M_TRUE,M_FALSE;

Mvalue* marray(Mvalue* length_value,Mvalue* fill_value){Mallocationowner owner=getOwner(__LINE__);
    // returns an array that can store length_value values (if possible)
    long long length=getValueInteger(length_value);
    if(length>=0){
        Marray* _array=owned_array(_getArray("marray",length),owner);
        if(_array){
            if(fill_value)
                while(length>0)assignValue(&_array->values[--length],fill_value);
            return _getValueOfArray(disowned_array(_array,owner));
        }
    }
    return NULL;
}
// fill with value
Mvalue* mfill(Mvalue* array_value,Mvalue* value){
    Marray* array=(array_value&&array_value->type==VT_ARRAY?array_value->value._array:NULL);
    if(array){
        unsigned long long l=array->numberOfElements;
        // we use assignValue here we get a copy of maps and lists (and arrays for that matter)
        while(l>0)assignValue(&array->values[--l],value);
    }
    return array_value;
}
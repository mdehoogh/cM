#include "Marray.h"

extern unsigned long long M_MODULE_DEBUGGING;

// MDH@22NOV2020
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_ARRAY,id};}

extern long long M_LL_INVALID,M_TRUE,M_FALSE;

// iterator support
static Msequenceelement arrayNext(void * const iterator){
    Msequenceelement sequenceelement={};
    if(iterator){
        Miterator* it=(Miterator*)iterator;
        if(it->valueholder){
            sequenceelement=(Msequenceelement){it->index,*(it->valueholder)};
            if(it->index<it->lastindex){ // not the last array element yet
                // we need the last value holder to know when we are done!!!!
                it->index++;
                it->valueholder++;
            }else // end of array reached
                it->valueholder=NULL;
        }
    }
    return sequenceelement;
}
static unsigned long long arrayNextIndex(void * const iterator){
    if(iterator){
        Miterator* it=(Miterator*)iterator;
        if(it->valueholder)return it->index;
    }
    return 0;
}
Miterator getArrayiterator(Marray* array){
    Miterator arrayiterator={arrayNext,arrayNextIndex,1};
    if(array){
        arrayiterator.valuetype=array->valuetype;
        if(array->numberOfElements){
            arrayiterator.lastindex=array->numberOfElements;
            arrayiterator.valueholder=(void**)array->values; // NOTE leading in determining whether end of sequence was reached (instead of index)
        }
    }
    return arrayiterator;
}

Mvalue* marray(Mvalue* length_value,Mvalue* fill_value){Mallocationowner owner=getOwner(__LINE__);
    // how about allowing length_value to be a list or array to fill the array with 
    // returns an array that can store length_value values (if possible)
    if(length_value){
        long long length;
        if(length_value->type==VT_MAP){
            Mmap* _map=length_value->value._map;
            length=_map->numberOfElements<<1;
            Marray* _result=owned_array(_getArray("marray",length),owner); // each key value pair uses 2 array elements
            if(_result){
                Mvariable* variable;
                Mmapelement* mapelement=_map->_first;
                unsigned long long arrayindex=0;
                while(mapelement&&arrayindex<length){
                    // NOTE the name of the variable is an Mchars which essentially means it does not know its quote character so in essence we need to wrap it first
                    Mstring* _name=owned_string(_getString("'"),owner);
                    if(_name){
                        if(string_append(_name,mapelement->_variable->_name->chars)){
                            assignValue(&_result->values[arrayindex++],_getTextValue(string(_name)));
                            assignValue(&_result->values[arrayindex++],mapelement->_variable->_value);
                        }
                        FREE_STRING(_name,owner);
                    }
                    mapelement=mapelement->_next;
                }
                return _getValueOfArray(disowned_array(_result,owner));
            }
        }else
        if(length_value->type==VT_ARRAY){
            Marray* _array=length_value->value._array;
            length=_array->numberOfElements;
            Marray* _result=owned_array(_getArray("marray",length),owner);
            if(_result){
                while(--length>=0)assignValue(&_result->values[length],_array->values[length]);
                return _getValueOfArray(disowned_array(_result,owner));
            }
        }else
        if(length_value->type==VT_LIST){
            Mlist* _list=length_value->value._list;
            length=_list->numberOfElements;
            Marray* _result=owned_array(_getArray("marray",length),owner);
            if(_result){
                Mlistelement* listelement=_list->_first;
                unsigned long long arrayindex=0;
                while(listelement&&arrayindex<length){assignValue(&_result->values[arrayindex++],listelement->_value);listelement=listelement->_next;}
                return _getValueOfArray(disowned_array(_result,owner));
            }
        }else{ // not composite, interpret as a length
            long long length=getValueInteger(length_value);
            if(length>=0){
                Marray* _result=owned_array(_getArray("marray",length),owner);
                if(_result){
                    if(fill_value)
                        while(length>0)assignValue(&_result->values[--length],fill_value);
                    return _getValueOfArray(disowned_array(_result,owner));
                }
            }
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

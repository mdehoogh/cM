#include "Mlist.h"

#include "Malloc.h"

extern long long M_LL_INVALID,M_TRUE,M_FALSE;

Mvalue* Mempty(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value)
    switch(value->type){
        case VT_MAP:result=(value->value._map&&value->value._map->_first?M_FALSE:M_TRUE);break;
        case VT_LIST:result=(value->value._list&&value->value._list->_first?M_FALSE:M_TRUE);break;
        default:break;
    }
    return _getIntegerValue(result);
}
Mvalue* Mkeys(Mvalue* value){
    if(value)
    switch(value->type){
        case VT_MAP:return _getValueOfList(_getMapAttributes(value->value._map),true);
        case VT_LIST:return _getValueOfList(_getListIndices(value->value._list),true);
        default:break;
    }
    return NULL;
}

// TODO check whether the list is immutable????
// how about returning true on success and false on failure
Mvalue* Mpush(Mvalue* listValue,Mvalue* value){ // append a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=(appendedToList(listValue->value._list,value,M_LL_INVALID)>0?M_TRUE:M_FALSE);
    return _getIntegerValue(result);
}
Mvalue* Mshove(Mvalue* listValue,Mvalue* value){ // prepend a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=(appendedToList(listValue->value._list,value,0)>0?M_TRUE:M_FALSE);
    return _getIntegerValue(result);
}

Mvalue* Mpop(Mvalue* listValue){ // remove and return the last value i.e. opposite of push/drop
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list&&list->_first&&list->numberOfElements>0){ // list is not empty, so something to pop
            Mvalue* lastValue=NULL;
            // unfortunately a list is single linked so we have to traverse the entire list to reach the end
            Mlistelement *beforelast=NULL,*last=list->_first;
            while(last->_next){beforelast=last;last=last->_next;}
            // given the fact that the value we return also needs to be removed from the list
            lastValue=last->_value;
            // now we have to unlink the last element i.e. free it
            if(beforelast)beforelast->_next=NULL;else list->_first=NULL; // if we have a beforelast we're NOT removing the first element, else we are (and list->_first should become NULL)
            list->_last=beforelast;list->numberOfElements--; // unlink, update last
            assignValue(&last->_value,NULL); // by assigning NULL last->_value will have one less reference count
            FREE(last,'l'); // free the list element we unlinked
            return lastValue;
        }
    }
    return NULL;
}
Mvalue* Mpull(Mvalue* listValue){ // remove and return the first value
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        Mlistelement* first=(list&&list->numberOfElements>0?list->_first:NULL);
        if(first){ // the list has a first element
            Mvalue* firstValue=first->_value; // remember the pointer to the first value
            assignValue(&first->_value,NULL); // dereference the current value
            list->_first=first->_next; // make the list start with the successor of the original first
            if(!list->_first)list->_last=NULL; // if no list first now, also no list last anymore
            list->numberOfElements--; // obviously one less element
            FREE(first,'l'); // free the list element we unlinked
            return firstValue;
        }
    }
    return NULL;
}

Mvalue* Mfirst(Mvalue* listValue){ // return the first value
    return(listValue&&listValue->type==VT_LIST&&listValue->value._list&&listValue->value._list->_first?listValue->value._list->_first->_value:NULL);
}
Mvalue* Mlast(Mvalue* listValue){ // return the last value
    return(listValue&&listValue->type==VT_LIST&&listValue->value._list&&listValue->value._list->_last?listValue->value._list->_last->_value:NULL);
}
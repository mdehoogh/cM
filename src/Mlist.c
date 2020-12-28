#include "Mlist.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_LIST,id};}

extern long long M_LL_INVALID,M_TRUE,M_FALSE;
extern long double M_LD_NAN;
extern char const * const M_ERROR_PREFIX;

// iterator support
// NOTE for a list use getListIterator() to obtain a valid list iterator (which guarantees that it's actually an iterator pointer being passed in)
static Msequenceelement listNext(void * const iterator){
    Msequenceelement result={}; // initialize to the default i.e. {0,NULL}
    if(iterator){
        Miterator* it=(Miterator*)iterator;
        Mlistelement* listelement=(Mlistelement*)it->valueholder;
        if(listelement){ // there's still a value being pointed to
            result=(Msequenceelement){listelement->index,listelement->_value};
            // prepare for returning the next element
            // knowing that the valueholder is a listelement, we can get the next list element
            if(listelement->index>=it->lastindex)it->valueholder=NULL;else it->valueholder=(void**)listelement->_next;
        }
    }
    return result;
}
static unsigned long long listNextindex(void * const iterator){
    Mlistelement* listelement=(iterator?(Mlistelement*)((Miterator*)iterator)->valueholder:NULL);
    return(listelement?listelement->index:0);
}
Miterator getListiterator(Mlist* list){
    // NOTE ascertain to have at least the next and nextindex fields
    Miterator listiterator=(Miterator){listNext,listNextindex};
    if(list){
        listiterator.valuetype=list->valuetype;
        // for security we check whether the list has a last, and if so there's something to iterate over
        // if no last, no valueholder
        if(list->_last&&!list->_last->_next){
            listiterator.valueholder=(void**)list->_first;
            // no need to set index because we can always get the index from the current value holder!!!
            listiterator.lastindex=list->_last->index;
        }else
        if(list->_last)
            outputBug("Assumed last list element points to another list element in constructing a list iterator!");
    }
    return listiterator;
}

Mvalue* Mclear(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value)
    switch(value->type){
        case VT_MAP:
            if(value->value._map){
                result=0;
                Mmap* map=value->value._map;
                if(map){
                    if(!map->immutable){
                        result=map->numberOfElements-free_mapelement(disowned_mapelement(map->_first,getValueOwner()),map->weak);
                        if(result<=0){
                            map->_first=NULL;
                            map->_last=NULL;
                            map->numberOfElements=0;
                        }
                    }
                }
            }
            break;
        case VT_LIST:
            {
                result=0;
                Mlist* list=value->value._list;
                if(list){
                    if(!list->immutable){
                        result=list->numberOfElements-free_listelement(disowned_listelement(list->_first,getValueOwner()),list->weak);
                        if(result<=0){
                            list->_first=NULL;
                            list->_last=NULL;
                            list->numberOfElements=0;
                        }
                    }
                }
            }
        default:
            break;
    }
    return _getIntegerValue(result);
}
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
Mvalue* Mkeys(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _keysList=NULL;
    if(value){
        if(value->type==VT_MAP)_keysList=owned_list(_getMapAttributes(value->value._map),owner);else
        if(value->type==VT_LIST)_keysList=owned_list(_getListIndices(value->value._list),owner);
    }
    if(!_keysList)return NULL;
    return _getValueOfList(disowned_list(_keysList,owner));
}

// TODO check whether the list is immutable????
// DONE appendedToList() will check for that in the list (as a list has an immutable flag now) AND it will also return M_LL_INVALID on failure
// how about returning true on success and false on failure
Mvalue* Mpush(Mvalue* listValue,Mvalue* value){ // append a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=appendedToList(listValue->value._list,Msubowner(getValueOwner(),1),value,M_LL_INVALID); // now returning the result of appendedList() instead of M_TRUE and M_FALSE (nevertheless positive values indicate success)
    return _getIntegerValue(result);
}
Mvalue* Mshove(Mvalue* listValue,Mvalue* value){ // prepend a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=appendedToList(listValue->value._list,Msubowner(getValueOwner(),1),value,0);
    return _getIntegerValue(result);
}
// MDH@23NOV2020: inserting means adding a value with a given index but not replacing it, which means we need to increment all successive indices
Mvalue* Minsert(Mvalue* listValue,Mvalue* value,Mvalue* indexValue){
    long long result=getValueInteger(indexValue);
    if(result!=M_LL_INVALID)result=(listValue&&listValue->type==VT_LIST?insertedIntoList(listValue->value._list,Msubowner(getValueOwner(),1),value,result):M_LL_INVALID);
    return _getIntegerValue(result);
}
Mvalue* Mpop(Mvalue* listValue){ // remove and return the last value i.e. opposite of push/drop
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            if(list->_first){
                if(!list->immutable){
                    Mvalue* lastValue=NULL;
                    // unfortunately a list is single linked so we have to traverse the entire list to reach the end
                    Mlistelement *beforelast=NULL,*last=list->_first;
                    while(last->_next){beforelast=last;last=last->_next;}
                    // given the fact that the value we return also needs to be removed from the list
                    lastValue=last->_value;
                    // now we have to unlink the last element i.e. free it
                    if(beforelast)beforelast->_next=NULL;else list->_first=NULL; // if we have a beforelast we're NOT removing the first element, else we are (and list->_first should become NULL)
                    list->_last=beforelast;list->numberOfElements--; // unlink, update last
                    last->_next=NULL;/* prevents releasing all following!*/FREE_LISTELEMENT(last,list->weak,Msubowner(getValueOwner(),2)); // free the list element we unlinked
                    /////// replacing: assignValue(&last->_value,NULL);FREE(last,"l'); // by assigning NULL last->_value will have one less reference count
                    return lastValue;
                }
                outputError("It is not allowed to pop elements from an immutable list");
            }else
                outputError("No elements in list to pop");
        }else
            outputError("No list to pop from");
    }
    return NULL;
}
Mvalue* Mpull(Mvalue* listValue){ // remove and return the first value
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            Mlistelement* first=list->_first;
            if(first){
                if(!list->immutable){
                    Mvalue* firstValue=first->_value; // remember the pointer to the first value
                    assignValue(&first->_value,NULL); // dereference the current value
                    list->_first=first->_next; // make the list start with the successor of the original first
                    if(!list->_first)list->_last=NULL; // if no list first now, also no list last anymore
                    list->numberOfElements--; // obviously one less element
                    first->_next=NULL;/* prevents releasing all following!*/FREE_LISTELEMENT(first,list->weak,Msubowner(getValueOwner(),2)); // replacing: FREE(first,"l'); // free the list element we unlinked
                    return firstValue;
                }
                outputError("It is not allowed to pull elements from an immutable list");
            }else
                outputError("No elements in list to pull");
        }else
            outputError("No list to pull from");
    }
    return NULL;
}
Mvalue* removedFromList(Mlist* list,Mallocationowner owner_list,long long listIndex){
    Mvalue* removedValue=NULL;
    if(list){
        if(listIndex>0){
            if(!list->immutable){
                Mlistelement *previouslistelement=NULL,*listelement=list->_first;
                while(listelement){
                    if(listelement->index==listIndex){ // got it
                        // connect through
                        if(!listelement->_next)list->_last=previouslistelement; // if the last element is being removed (i.e. listelement has no successor, replace _last by the previous element (which could also be NULL of course)
                        // the previous list element must now point to the successor of listelement, if there's no successor we get a new first
                        if(previouslistelement)previouslistelement->_next=listelement->_next;else list->_first=listelement->_next;
                        list->numberOfElements--; // one less element in the list
                        removedValue=listelement->_value; // BEFORE freeing the element (and dereferencing the value well if this is not a weak list which it most likely will not be)
                        listelement->_next=NULL;/* prevents releasing all following!*/FREE_LISTELEMENT(listelement,list->weak,Msubowner(owner_list,1)); // takes care of dereferencing the value
                        break;
                    }
                    previouslistelement=listelement; // remember the previous list element (in case we find a match)
                    listelement=previouslistelement->_next;
                }
            }else
                outputError("Cannot remove a list element: the list is immutable");
        }else
            output("%sInvalid list index %lld.\n",M_ERROR_PREFIX,listIndex);
    }else
        outputError("No list to remove from");
    return removedValue;
}
/**
 * \brief removes any element with the given listIndex, if no such element is present M_LL_INVALID is returned
 * \returns the removed element
 */
Mvalue* Mremoved(Mvalue* listValue,Mvalue* listIndexValue){Mallocationowner owner=getOwner(__LINE__);
    Mvalue* removedValue=NULL;
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            if(!list->immutable){
                if(listIndexValue&&listIndexValue->type!=VT_MAP){ // list index/indices defined and not a map
                    if(listIndexValue->type==VT_LIST){ // multiple
                        Mlist* indexList=listIndexValue->value._list;
                        if(indexList){
                            Mlist* _removedElementsList=owned_list(_getListOfType(list->valuetype),owner); // get a list of the same type as the list from which elements are removed!!!
                            if(_removedElementsList){
                                Mlistelement* indexListelement=indexList->_first;
                                Mvalue* removedFromListValue;
                                while(indexListelement){
                                    removedFromListValue=removedFromList(list,Msubowner(getValueOwner(),1),getValueInteger(indexListelement->_value));
                                    if(removedFromListValue&&appendedToList(_removedElementsList,owner,removedFromListValue,M_LL_INVALID)<=0)
                                        outputError("Failed to remember a removed list element");
                                    indexListelement=indexListelement->_next;
                                }
                                removedValue=_getValueOfList(disowned_list(_removedElementsList,owner)); // MDH@12JUN2020: by passing the list disowned, it will be freed when failing to bind it
                            }else 
                                outputError("Failed to create a list for storing the removed list elements");
                        }else 
                            outputBug("Missing index list");
                    }else // something else
                        removedValue=removedFromList(list,Msubowner(getValueOwner(),1),getValueInteger(listIndexValue));
                }else 
                    outputError("No or invalid list index/indices second argument to the removed() function");
            }else 
                outputError("Will not remove elements from a list that is immutable");
        }else 
            outputBug("Missing list");
    }else
        outputError("List argument to removed() invalid");
    return removedValue;
}
// TODO find certain elements in a list
/**
 * \brief returns the indices of the elements in \p listValue equal to \p listElementValue but at most \p maximumNumberOfElementsValue
 */
Mvalue* Mfind(Mvalue* listValue,Mvalue* listElementValue,Mvalue* maximumNumberOfElementsToFindValue){Mallocationowner owner=getOwner(__LINE__);
    Mvalue* _findValue=NULL;
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            long long maximumNumberOfElementsToFind=getValueInteger(maximumNumberOfElementsToFindValue);
            Mlist* _foundElementsIndicesList=owned_list(_getListOfType(VT_INTEGER),owner); // get a list of the same type as the list from which elements are removed!!!
            if(_foundElementsIndicesList){
                Mlistelement* listelement=list->_first;
                while(listelement){
                    if(areValuesEqual(listelement->_value,listElementValue)){
                        if(appendedToList(_foundElementsIndicesList,owner,_getIntegerValue(listelement->index),M_LL_INVALID)<=0)
                            outputError("Failed to store the index of a list element found");else
                        if(maximumNumberOfElementsToFind>0
                                &&_foundElementsIndicesList->numberOfElements>=maximumNumberOfElementsToFind)
                            break;
                    }
                    listelement=listelement->_next;
                }
                _findValue=_getValueOfList(disowned_list(_foundElementsIndicesList,owner));
            }else
                outputError("Failed to create the list to store the indices of the element to find");
        }else
            outputBug("Missing list");
    }else 
        outputError("No list to search for a particular value.");
    return _findValue;
}

Mvalue* Mfirst(Mvalue* listValue){ // return the first value
    return(listValue&&listValue->type==VT_LIST&&listValue->value._list&&listValue->value._list->_first?listValue->value._list->_first->_value:NULL);
}
Mvalue* Mlast(Mvalue* listValue){ // return the last value
    return(listValue&&listValue->type==VT_LIST&&listValue->value._list&&listValue->value._list->_last?listValue->value._list->_last->_value:NULL);
}

// MDH@03NOV2019: allowing computing sample statistics on lists with values of the same (numeric) scalar type 
// what statistics do we want to compute of a given sample of numbers? count, sum, sumofsquares, mode, minimum, maximum, missing
// count and missing are integers, sum, sumofsquares, mode, minimum and maximum are in the same unit as the input values
// if we use sum and sumofsquares to compute the mean and variance we can store these in a rational for integer input values
Mmap* _getIntegerSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _statisticsMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
    if(_statisticsMap){
        if(amVerbose())output("Computing integer sample statistics.\n");
        long long missings=0,errors=0,integer=M_LL_INVALID;
        unsigned long long index=0;
        Mvalue* value;
        while((index=iter_nextindex(iterator))){
            value=iter_next(iterator);
            if(value&&value->type==VT_INTEGER){
                integer=getValueInteger(value);
                if(integer!=M_LL_INVALID)break;
                errors++;
            }else
                missings++;
        }
        if(integer!=M_LL_INVALID){ // at least one valid integer in the list
            long long count=1,minimumindex=index,maximumindex=index; // counting the missings and the number of sample values (that are NOT missing)
            long long sum=integer,squaressum=sum*sum,mode=integer,minimum=integer,maximum=integer;
            while((index=iter_nextindex(iterator))){
                value=iter_next(iterator);
                if(value&&value->type==VT_INTEGER){
                    integer=getValueInteger(value);
                    if(integer!=M_LL_INVALID){
                        count++;
                        sum+=integer;
                        squaressum+=(integer*integer);
                        if(integer<minimum){minimum=integer;minimumindex=index;}
                        if(integer>maximum){maximum=integer;maximumindex=index;}
                    }else errors++;
                }else missings++;
            }
            // ready to compose the map elements
            appendedToMap(_statisticsMap,owner,"count",_getIntegerValue(count));
            appendedToMap(_statisticsMap,owner,"sum",_getIntegerValue(sum));
            appendedToMap(_statisticsMap,owner,"squaressum",_getIntegerValue(squaressum));
            appendedToMap(_statisticsMap,owner,"minimum",_getIntegerValue(minimum));
            appendedToMap(_statisticsMap,owner,"maximum",_getIntegerValue(maximum));
            appendedToMap(_statisticsMap,owner,"minimumindex",_getIntegerValue(minimumindex));
            appendedToMap(_statisticsMap,owner,"maximumindex",_getIntegerValue(maximumindex));
            // with these values we are able to compute the mean and the variance and the standard deviation
            if(count>0){
                // we need big integers of all the relevant values
                Mbiginteger *_count=owned_biginteger(_getBiginteger(count),owner),*_sum=owned_biginteger(_getBiginteger(sum),owner),*_squaressum=owned_biginteger(_getBiginteger(squaressum),owner);
                if(_count&&_sum&&_squaressum){
                    Mvalue* _meanValue=_getValueOfRational(_getRational(_sum,_count,M_LD_NAN,true));
                    if(!_meanValue||appendedToMap(_statisticsMap,owner,"mean",_meanValue)<=0)outputError("Failed to store the sample mean in the statistics map");
                    // the sum of squared deviations (of sum of squares) is defined as squaressum-(sum*sum)/count
                    Mrational* _squaressumRational=owned_rational(_getRational(_squaressum,NULL,M_LD_NAN,false),owner);
                    if(_squaressumRational){
                        // I need to subtract another rational
                        Mbiginteger* _squaredsum=owned_biginteger(__biginteger(),owner);
                        if(_squaredsum){
                            if(mp_sqr(MP_INT_POINTER(_sum),MP_INT_POINTER(_squaredsum))==MP_OKAY){
                                Mrational* _tosubtract=owned_rational(_getRational(_squaredsum,_count,M_LD_NAN,false),owner);
                                if(_tosubtract){
                                    Mrational* _sumofsquaresRational=owned_rational(_getRationalDifference(_squaressumRational,_tosubtract),owner);
                                    if(_sumofsquaresRational){
                                        Mvalue* _sumofsquaresRationalValue=_getValueOfRational(disowned_rational(_sumofsquaresRational,owner));
                                        if(_sumofsquaresRationalValue){ // _sumofsquaresRational now bound to the value
                                            if(appendedToMap(_statisticsMap,owner,"sumofsquares",_sumofsquaresRationalValue)<=0)
                                                outputError("Failed to store the sample sum of squares in the statistics map");
                                            // next to divide by the count minus 1 to give us the variance
                                            Mbiginteger* _countminus1=owned_biginteger(_getBigintegerCopy(_count),owner);
                                            if(_countminus1){
                                                if(mp_decr(MP_INT_POINTER(_countminus1))==MP_OKAY){
                                                    Mrational* _varianceRational=owned_rational(_getRationalBigintegerQuotient(_sumofsquaresRational,_countminus1),owner);
                                                    if(_varianceRational){
                                                        Mvalue* _varianceRationalValue=_getValueOfRational(disowned_rational(_varianceRational,owner));
                                                        if(!_varianceRationalValue||appendedToMap(_statisticsMap,owner,"variance",_varianceRationalValue)<=0)outputError("Failed to store the sample variance in the statistics map");
                                                        // TODO and finally the standard deviation
                                                    }
                                                }
                                                FREE_BIGINTEGER(_countminus1,owner);
                                            }
                                        }
                                    }
                                }                       
                            }
                            FREE_BIGINTEGER(_squaredsum,owner);
                        }
                    }else 
                        outputError("Failed to initialize the sum of squares");
                }else 
                    outputMemoryError("Failed to store the sample size and/or sum in a big integer");
                FREE_BIGINTEGER(_sum,owner);
                FREE_BIGINTEGER(_count,owner);
                FREE_BIGINTEGER(_squaressum,owner);
            }
        }
        appendedToMap(_statisticsMap,owner,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,owner,"errors",_getIntegerValue(errors));
        return disowned_map(_statisticsMap,owner);
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getBigintegerSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _statisticsMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
    if(_statisticsMap){
        if(amVerbose())output("Computing big integer sample statistics.\n");
        Mbiginteger* biginteger=NULL;
        long long missings=0,errors=0; // NOTE errors is the number of times converting a value to a big integer failed
        unsigned long long index=0;
        // ok, go and try to find the first valid big integer
        Mvalue* value;
        while((index=iter_nextindex(iterator))){
            value=iter_next(iterator);
            if(value&&value->type==VT_BIGINTEGER){
                biginteger=_getValueBiginteger(value);
                if(biginteger)break;
                errors++;
            }else 
                missings++;
        }
        if(biginteger){ // at least one valid integer in the list
            long long count=1,minimumindex=index,maximumindex=index; // counting the missings and the number of sample values (that are NOT missing)
            Mbiginteger *sumofsquares=owned_biginteger(__biginteger(),owner),*sum=owned_biginteger(_getBigintegerCopy(biginteger),owner)
                       ,*minimum=owned_biginteger(_getBigintegerCopy(biginteger),owner),*maximum=owned_biginteger(_getBigintegerCopy(biginteger),owner);
            if(sum&&minimum&&maximum&&sumofsquares&&mp_mul(MP_INT_POINTER(biginteger),MP_INT_POINTER(biginteger),MP_INT_POINTER(sumofsquares))==MP_OKAY){
                // every time we get a big integer to use to update the cumulative sample statistics we're going to update the helpers first
                Mbiginteger *_newsum=owned_biginteger(__biginteger(),owner)
                           ,*_newssq=owned_biginteger(__biginteger(),owner)
                           ,*_newminimum=owned_biginteger(__biginteger(),owner)
                           ,*_newmaximum=owned_biginteger(__biginteger(),owner)
                           ,*_square=owned_biginteger(__biginteger(),owner);
                if(_newsum&&_newssq&&_newminimum&&_newmaximum){
                    bool someerror;
                    while((index=iter_nextindex(iterator))){
                        value=iter_next(iterator);
                        if(value&&value->type==VT_BIGINTEGER){
                            biginteger=_getValueBiginteger(value);
                            if(biginteger){
                                someerror=false;
                                if(mp_mul(MP_INT_POINTER(biginteger),MP_INT_POINTER(biginteger),MP_INT_POINTER(_square))!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_cmp(MP_INT_POINTER(biginteger),MP_INT_POINTER(minimum))==MP_LT&&mp_copy(MP_INT_POINTER(biginteger),MP_INT_POINTER(_newminimum))!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_cmp(MP_INT_POINTER(maximum),MP_INT_POINTER(biginteger))==MP_LT&&mp_copy(MP_INT_POINTER(biginteger),MP_INT_POINTER(_newmaximum))!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_add(MP_INT_POINTER(sum),MP_INT_POINTER(biginteger),MP_INT_POINTER(_newsum))!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_add(MP_INT_POINTER(sumofsquares),MP_INT_POINTER(_square),MP_INT_POINTER(_newssq))!=MP_OKAY)someerror=true;
                                if(!someerror){
                                    // now we need to copy the new values over
                                    // if any of them fails we're in an unrecoverable situation
                                    if(mp_copy(MP_INT_POINTER(_newminimum),MP_INT_POINTER(minimum))==MP_OKAY&&mp_copy(MP_INT_POINTER(_newmaximum),MP_INT_POINTER(maximum))==MP_OKAY&&mp_copy(MP_INT_POINTER(_newsum),MP_INT_POINTER(sum))==MP_OKAY&&mp_copy(MP_INT_POINTER(_newssq),MP_INT_POINTER(sumofsquares))==MP_OKAY)
                                        count++;
                                    else
                                        someerror=true;
                                }else
                                    errors++;
                            }
                        }else
                            missings++;
                        if(someerror)break;
                    }
                    if(!someerror){
                        appendedToMap(_statisticsMap,owner,"count",_getIntegerValue(count));
                        appendedToMap(_statisticsMap,owner,"sum",_getValueOfBiginteger(disowned_biginteger(sum,owner)));
                        appendedToMap(_statisticsMap,owner,"sumofsquares",_getValueOfBiginteger(disowned_biginteger(sumofsquares,owner)));
                        appendedToMap(_statisticsMap,owner,"minimum",_getValueOfBiginteger(disowned_biginteger(minimum,owner)));
                        appendedToMap(_statisticsMap,owner,"maximum",_getValueOfBiginteger(disowned_biginteger(maximum,owner)));
                    }else{
                        Mstring* _error=owned_string(__string(),owner);
                        if(_error){
                            if(value){
                                string_append(_error,"Some error occurred while updating the sample statistics with the list element at index ");
                                appendll(_error,index);string_append(_error,".");
                            }else
                                string_append(_error,"Failed to compute the big integer sample statistics.");
                            appendedToMap(_statisticsMap,owner,"error",_getTextValue(string(_error)));
                            FREE_STRING(_error,owner);
                        }else // NOTE _getTextValue will _strdup the text given, so we do not need to do that here!!!
                            appendedToMap(_statisticsMap,owner,"error",_getTextValue("Some error occurred computing the big integer sample statistics."));
                    }
                }else
                    outputError("Failed to create all big integer helpers in computing big integer sample statistics");
                FREE_BIGINTEGER(_newsum,owner);
                FREE_BIGINTEGER(_newssq,owner);
                FREE_BIGINTEGER(_newminimum,owner);
                FREE_BIGINTEGER(_newmaximum,owner);
                FREE_BIGINTEGER(_square,owner);
                // ready to compose the map elements
            }else 
                outputError("Failed to initialize the big integer sample statistics");
        }
        appendedToMap(_statisticsMap,owner,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,owner,"errors",_getIntegerValue(errors));
        return disowned_map(_statisticsMap,owner);
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getDecimalSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _statisticsMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
    if(_statisticsMap){
        if(amVerbose())output("Computing decimal sample statistics.\n");
        return disowned_map(_statisticsMap,owner);
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getRationalSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _statisticsMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
    if(_statisticsMap){
        if(amVerbose())output("Computing rational sample statistics.\n");
        return disowned_map(_statisticsMap,owner);
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getFloatSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _statisticsMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
    if(_statisticsMap){
        if(amVerbose())output("Computing float sample statistics.\n");
        long long missings=0,errors=0;
        long double ld=M_LD_NAN;
        unsigned long long index=0;
        Mvalue* value;
        while((index=iter_nextindex(iterator))){
            value=iter_next(iterator);
            if(value&&value->type==VT_FLOAT){
                ld=getValueLongDouble(value);
                if(!isLongDoubleUndefined(ld))break;
                errors++;
            }else 
                missings++;
        }
        if(!isLongDoubleUndefined(ld)){ // at least one valid integer in the list
            long long count=1,minimumindex=index,maximumindex=index;
            long double sum=ld,sumofsquares=(ld*ld),minimum=ld,maximum=ld;
            unsigned long long index;
            Mvalue* value;
            while((index=iter_nextindex(iterator))){
                value=iter_next(iterator);
                ld=getValueLongDouble(value);
                if(!isLongDoubleUndefined(ld)){
                    sum+=ld;
                    sumofsquares+=(ld*ld);
                    if(ld<minimum){minimum=ld;minimumindex=index;}
                    if(ld>maximum){maximum=ld;maximumindex=index;}
                    count++;
                }else
                    missings++;
            }
            // ready to compose the map elements
            appendedToMap(_statisticsMap,owner,"count",_getIntegerValue(count));
            appendedToMap(_statisticsMap,owner,"sum",_getFloatValue(sum));
            appendedToMap(_statisticsMap,owner,"sumofsquares",_getFloatValue(sumofsquares));
            appendedToMap(_statisticsMap,owner,"minimum",_getFloatValue(minimum));
            appendedToMap(_statisticsMap,owner,"maximum",_getFloatValue(minimum));
            appendedToMap(_statisticsMap,owner,"minimumindex",_getIntegerValue(minimumindex));
            appendedToMap(_statisticsMap,owner,"maximumindex",_getIntegerValue(maximumindex));
        }
        appendedToMap(_statisticsMap,owner,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,owner,"errors",_getIntegerValue(errors));
        return disowned_map(_statisticsMap,owner);
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
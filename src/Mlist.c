#include "Mlist.h"

#include "Malloc.h"
#include "Msettings.h"
#include "Moutput.h"

extern long long M_LL_INVALID,M_TRUE,M_FALSE;
extern long double M_LD_NAN;
extern char const * const ERROR_PREFIX;

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
// DONE appendedToList() will check for that in the list (as a list has an immutable flag now) AND it will also return M_LL_INVALID on failure
// how about returning true on success and false on failure
Mvalue* Mpush(Mvalue* listValue,Mvalue* value){ // append a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=appendedToList(listValue->value._list,value,M_LL_INVALID); // now returning the result of appendedList() instead of M_TRUE and M_FALSE (nevertheless positive values indicate success)
    return _getIntegerValue(result);
}
Mvalue* Mshove(Mvalue* listValue,Mvalue* value){ // prepend a value to the list
    long long result=M_LL_INVALID;
    if(listValue&&listValue->type==VT_LIST)result=appendedToList(listValue->value._list,value,0);
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
                    last->_next=NULL;/* prevents releasing all following!*/free_listelement(last,list->weak); // free the list element we unlinked
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
                    first->_next=NULL;/* prevents releasing all following!*/free_listelement(first,list->weak); // replacing: FREE(first,"l'); // free the list element we unlinked
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
Mvalue* removedFromList(Mlist* list,long long listIndex){
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
                        listelement->_next=NULL;/* prevents releasing all following!*/free_listelement(listelement,list->weak); // takes care of dereferencing the value
                        break;
                    }
                    previouslistelement=listelement; // remember the previous list element (in case we find a match)
                    listelement=previouslistelement->_next;
                }
            }else
                outputError("Cannot remove a list element: the list is immutable");
        }else
            output("%sInvalid list index %lld.\n",ERROR_PREFIX,listIndex);
    }else
        outputError("No list to remove from");
    return removedValue;
}
/**
 * \brief removes any element with the given listIndex, if no such element is present M_LL_INVALID is returned
 * \returns the removed element
 */
Mvalue* Mremoved(Mvalue* listValue,Mvalue* listIndexValue){
    Mvalue* removedValue=NULL;
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            if(!list->immutable){
                if(listIndexValue&&listIndexValue->type!=VT_MAP){ // list index/indices defined and not a map
                    if(listIndexValue->type==VT_LIST){ // multiple
                        Mlist* indexList=listIndexValue->value._list;
                        if(indexList){
                            Mlist* _removedElementsList=_getListOfType(list->valuetype); // get a list of the same type as the list from which elements are removed!!!
                            if(_removedElementsList){
                                Mlistelement* indexListelement=indexList->_first;
                                Mvalue* removedFromListValue;
                                while(indexListelement){
                                    removedFromListValue=removedFromList(list,getValueInteger(indexListelement->_value));
                                    if(removedFromListValue&&!appendedToList(_removedElementsList,removedFromListValue,M_LL_INVALID))outputError("Failed to remember a removed list element");
                                    indexListelement=indexListelement->_next;
                                }
                                return _getValueOfList(_removedElementsList,true);
                            }else 
                                outputError("Failed to create a list for storing the removed list elements");
                        }else 
                            outputBug("Missing index list");
                    }else // something else
                        removedValue=removedFromList(list,getValueInteger(listIndexValue));
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
Mvalue* Mfind(Mvalue* listValue,Mvalue* listElementValue,Mvalue* maximumNumberOfElementsToFindValue){
    if(listValue&&listValue->type==VT_LIST){
        Mlist* list=listValue->value._list;
        if(list){
            long long maximumNumberOfElementsToFind=getValueInteger(maximumNumberOfElementsToFindValue);
            Mlist* _foundElementsIndicesList=_getListOfType(VT_INTEGER); // get a list of the same type as the list from which elements are removed!!!
            if(_foundElementsIndicesList){
                Mlistelement* listelement=list->_first;
                while(listelement){
                    if(areValuesEqual(listelement->_value,listElementValue)){
                        if(appendedToList(_foundElementsIndicesList,_getIntegerValue(listelement->index),M_LL_INVALID)<=0)outputError("Failed to store the index of a list element found");else
                        if(maximumNumberOfElementsToFind>0&&_foundElementsIndicesList->numberOfElements>=maximumNumberOfElementsToFind)break;
                    }
                    listelement=listelement->_next;
                }
                return _getValueOfList(_foundElementsIndicesList,true);
            }else
                outputError("Failed to create the list to store the indices of the element to find");
        }else
            outputBug("Missing list");
    }else 
        outputError("No list to search for a particular value.");
    return NULL;
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
Mmap* _getIntegerSampleStatisticsMap(Mlist* list){
    Mmap* _statisticsMap=_getMapOfType(VT_UNDEFINED);
    if(_statisticsMap){
        if(amVerbose())output("Computing integer sample statistics.\n");
        long long missings=0,errors=0;
        long long integer=M_LL_INVALID;
        Mlistelement* listelement=(list?list->_first:NULL);
        while(listelement){
            if(listelement->_value&&listelement->_value->type==VT_INTEGER){
                integer=getValueInteger(listelement->_value);
                if(integer!=M_LL_INVALID)break;
                errors++;
                listelement=listelement->_next;
            }else
                missings++;
        }
        if(integer!=M_LL_INVALID){ // at least one valid integer in the list
            long long count=1,minimumindex=listelement->index,maximumindex=listelement->index; // counting the missings and the number of sample values (that are NOT missing)
            long long sum=integer,squaressum=sum*sum,mode=integer,minimum=integer,maximum=integer;
            while(listelement->_next){
                listelement=listelement->_next;
                if(listelement->_value&&listelement->_value->type==VT_INTEGER){
                    integer=getValueInteger(listelement->_value);
                    if(integer!=M_LL_INVALID){
                        count++;
                        sum+=integer;
                        squaressum+=(integer*integer);
                        if(integer<minimum){minimum=integer;minimumindex=listelement->index;}
                        if(integer>maximum){maximum=integer;maximumindex=listelement->index;}
                    }else errors++;
                }else missings++;
            }
            // ready to compose the map elements
            appendedToMap(_statisticsMap,"count",_getIntegerValue(count));
            appendedToMap(_statisticsMap,"sum",_getIntegerValue(sum));
            appendedToMap(_statisticsMap,"squaressum",_getIntegerValue(squaressum));
            appendedToMap(_statisticsMap,"minimum",_getIntegerValue(minimum));
            appendedToMap(_statisticsMap,"maximum",_getIntegerValue(maximum));
            appendedToMap(_statisticsMap,"minimumindex",_getIntegerValue(minimumindex));
            appendedToMap(_statisticsMap,"maximumindex",_getIntegerValue(maximumindex));
            // with these values we are able to compute the mean and the variance and the standard deviation
            if(count>0){
                // we need big integers of all the relevant values
                Mbiginteger *_count=_getBiginteger(count),*_sum=_getBiginteger(sum),*_squaressum=_getBiginteger(squaressum);
                if(_count&&_sum&&_squaressum){
                    Mrational* _mean=_getRational(_getBigintegerCopy(_sum),_getBigintegerCopy(_count),M_LD_NAN,true,true);
                    if(_mean)appendedToMap(_statisticsMap,"mean",_getRationalValue(_mean,true));
                    // the sum of squared deviations (of sum of squares) is defined as squaressum-(sum*sum)/count
                    Mrational* _squaressumRational=_getRational(_getBigintegerCopy(_squaressum),NULL,M_LD_NAN,false,true);
                    if(_squaressumRational){
                        // I need to subtract another rational
                        Mbiginteger* _squaredsum=__biginteger();
                        if(_squaredsum){
                            if(mp_sqr(_sum,_squaredsum)==MP_OKAY){
                                Mrational* _tosubtract=_getRational(_getBigintegerCopy(_squaredsum),_getBigintegerCopy(_count),M_LD_NAN,false,true);
                                if(_tosubtract){
                                    Mrational* _sumofsquaresRational=_getRationalDifference(_squaressumRational,_tosubtract);
                                    appendedToMap(_statisticsMap,"sumofsquares",_getRationalValue(_sumofsquaresRational,true));
                                    // next to divide by the count minus 1 to give us the variance
                                    Mbiginteger* _countminus1=_getBigintegerCopy(_count);
                                    if(_countminus1&&mp_decr(_countminus1)==MP_OKAY){
                                        Mrational* _varianceRational=_getRationalBigintegerQuotient(_sumofsquaresRational,_countminus1);
                                        if(_varianceRational){
                                            appendedToMap(_statisticsMap,"variance",_getRationalValue(_varianceRational,true));
                                            // and finally the standard deviation
                                        }
                                    }
                                }                       
                            }
                            free_biginteger(_squaredsum);
                        }
                    }else 
                        outputError("Failed to initialize the sum of squares");
                }else 
                    outputMemoryError("Failed to store the sample size and/or sum in a big integer");
                free_biginteger(_sum);free_biginteger(_count);free_biginteger(_squaressum);
            }
        }
        appendedToMap(_statisticsMap,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,"errors",_getIntegerValue(errors));
        return _statisticsMap;
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getBigintegerSampleStatisticsMap(Mlist* list){
    Mmap* _statisticsMap=_getMapOfType(VT_UNDEFINED);
    if(_statisticsMap){
        if(amVerbose())output("Computing big integer sample statistics.\n");
        Mbiginteger* biginteger=NULL;
        long long missings=0,errors=0; // NOTE errors is the number of times converting a value to a big integer failed
        // ok, go and try to find the first valid big integer
        Mlistelement* listelement=(list?list->_first:NULL);
        while(listelement){
            if(listelement->_value&&listelement->_value->type==VT_BIGINTEGER){
                biginteger=_getValueBiginteger(listelement->_value);
                if(biginteger)break;
                errors++;
            }else 
                missings++;
            listelement=listelement->_next;
        }
        if(biginteger){ // at least one valid integer in the list
            long long count=1,minimumindex=listelement->index,maximumindex=listelement->index; // counting the missings and the number of sample values (that are NOT missing)
            Mbiginteger *sumofsquares=__biginteger(),*sum=_getBigintegerCopy(biginteger),*minimum=_getBigintegerCopy(biginteger),*maximum=_getBigintegerCopy(biginteger);
            if(sum&&minimum&&maximum&&sumofsquares&&mp_mul(biginteger,biginteger,sumofsquares)==MP_OKAY){
                // every time we get a big integer to use to update the cumulative sample statistics we're going to update the helpers first
                Mbiginteger *_newsum=__biginteger(),*_newssq=__biginteger(),*_newminimum=__biginteger(),*_newmaximum=__biginteger(),*_square=__biginteger();
                if(_newsum&&_newssq&&_newminimum&&_newmaximum){
                    bool someerror;
                    while(listelement->_next){
                        listelement=listelement->_next;
                        if(listelement->_value&&listelement->_value->type==VT_BIGINTEGER){
                            biginteger=_getValueBiginteger(listelement->_value);
                            if(biginteger){
                                someerror=false;
                                if(mp_mul(biginteger,biginteger,_square)!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_cmp(biginteger,minimum)==MP_LT&&mp_copy(biginteger,_newminimum)!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_cmp(maximum,biginteger)==MP_LT&&mp_copy(biginteger,_newmaximum)!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_add(sum,biginteger,_newsum)!=MP_OKAY)someerror=true;
                                if(!someerror)if(mp_add(sumofsquares,_square,_newssq)!=MP_OKAY)someerror=true;
                                if(!someerror){
                                    // now we need to copy the new values over
                                    // if any of them fails we're in an unrecoverable situation
                                    if(mp_copy(_newminimum,minimum)==MP_OKAY&&mp_copy(_newmaximum,maximum)==MP_OKAY&&mp_copy(_newsum,sum)==MP_OKAY&&mp_copy(_newssq,sumofsquares)==MP_OKAY)
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
                        appendedToMap(_statisticsMap,"count",_getIntegerValue(count));
                        appendedToMap(_statisticsMap,"sum",_getBigintegerValue(sum,true));
                        appendedToMap(_statisticsMap,"sumofsquares",_getBigintegerValue(sumofsquares,true));
                        appendedToMap(_statisticsMap,"minimum",_getBigintegerValue(minimum,true));
                        appendedToMap(_statisticsMap,"maximum",_getBigintegerValue(maximum,true));
                    }else{
                        Mstring* _error=__string();
                        if(_error){
                            if(listelement){string_append(_error,"Some error occurred while updating the sample statistics with the list element at index ");appendll(_error,listelement->index);string_append(_error,".");}
                            else string_append(_error,"Failed to compute the big integer sample statistics.");
                            appendedToMap(_statisticsMap,"error",_getTextValue(strdup(string(_error)),false));
                            free_string(_error);
                        }else
                            appendedToMap(_statisticsMap,"error",_getTextValue(strdup("Some error occurred computing the big integer sample statistics."),false));
                    }
                }else 
                    outputError("Failed to create all big integer helpers in computing big integer sample statistics");
                free_biginteger(_newsum);free_biginteger(_newssq);free_biginteger(_newminimum);free_biginteger(_newmaximum);free_biginteger(_square);
                // ready to compose the map elements
            }else 
                outputError("Failed to initialize the big integer sample statistics");
        }
        appendedToMap(_statisticsMap,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,"errors",_getIntegerValue(errors));
        return _statisticsMap;
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getDecimalSampleStatisticsMap(Mlist* list){
    Mmap* _statisticsMap=_getMapOfType(VT_UNDEFINED);
    if(_statisticsMap){
        if(amVerbose())output("Computing decimal sample statistics.\n");
        return _statisticsMap;
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getRationalSampleStatisticsMap(Mlist* list){
    Mmap* _statisticsMap=_getMapOfType(VT_UNDEFINED);
    if(_statisticsMap){
        if(amVerbose())output("Computing rational sample statistics.\n");
        return _statisticsMap;
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mmap* _getFloatSampleStatisticsMap(Mlist* list){
    Mmap* _statisticsMap=_getMapOfType(VT_UNDEFINED);
    if(_statisticsMap){
        if(amVerbose())output("Computing float sample statistics.\n");
        long long missings=0,errors=0;
        long double ld=M_LD_NAN;
        Mlistelement* listelement=(list?list->_first:NULL);
        while(listelement){
            if(listelement->_value&&listelement->_value->type==VT_FLOAT){
                ld=getValueLongDouble(listelement->_value);
                if(!isLongDoubleUndefined(ld))break;
                errors++;
            }else 
                missings++;
            listelement=listelement->_next;
        }
        if(!isLongDoubleUndefined(ld)){ // at least one valid integer in the list
            long long count=1,minimumindex=listelement->index,maximumindex=listelement->index;
            long double sum=ld,sumofsquares=(ld*ld),minimum=ld,maximum=ld;
            while(listelement->_next){
                listelement=listelement->_next;
                ld=getValueLongDouble(listelement->_value);
                if(!isLongDoubleUndefined(ld)){
                    sum+=ld;
                    sumofsquares+=(ld*ld);
                    if(ld<minimum){minimum=ld;minimumindex=listelement->index;}
                    if(ld>maximum){maximum=ld;maximumindex=listelement->index;}
                    count++;
                }else
                    missings++;
            }
            // ready to compose the map elements
            appendedToMap(_statisticsMap,"count",_getIntegerValue(count));
            appendedToMap(_statisticsMap,"sum",_getFloatValue(sum));
            appendedToMap(_statisticsMap,"sumofsquares",_getFloatValue(sumofsquares));
            appendedToMap(_statisticsMap,"minimum",_getFloatValue(minimum));
            appendedToMap(_statisticsMap,"maximum",_getFloatValue(minimum));
            appendedToMap(_statisticsMap,"minimumindex",_getIntegerValue(minimumindex));
            appendedToMap(_statisticsMap,"maximumindex",_getIntegerValue(maximumindex));
        }
        appendedToMap(_statisticsMap,"missings",_getIntegerValue(missings));
        appendedToMap(_statisticsMap,"errors",_getIntegerValue(errors));
        return _statisticsMap;
    }
    outputMemoryError("Failed to create a map to store statistics in.");
    return NULL;
}
Mvalue* Mstats(Mvalue* listValue){
    if(listValue){
        Mlist* list=listValue->value._list;
        if(list){
            if(list->valuetype!=VT_MAP&&list->valuetype!=VT_REFERENCE&&list->valuetype!=VT_LIST&&list->valuetype!=VT_UNDEFINED){
                // the values in the list need to be scalars of the same type
                if(list->valuetype==VT_INTEGER)return _getValueOfMap(_getIntegerSampleStatisticsMap(list),true);
                if(list->valuetype==VT_BIGINTEGER)return _getValueOfMap(_getBigintegerSampleStatisticsMap(list),true);
                if(list->valuetype==VT_RATIONAL)return _getValueOfMap(_getRationalSampleStatisticsMap(list),true);
                if(list->valuetype==VT_DECIMAL)return _getValueOfMap(_getDecimalSampleStatisticsMap(list),true);
                if(list->valuetype==VT_FLOAT)return _getValueOfMap(_getFloatSampleStatisticsMap(list),true);
            }else
                output("All values in the list should be of the same numeric type (integer, float, rational or decimal).\n");
        }else 
            outputBug("List vanished!");
    }else
        outputError("No sample list to compute statistics of");
    return NULL;
}
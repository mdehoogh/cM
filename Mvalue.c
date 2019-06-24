#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "mstring.h"
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"

#include "Mvalue.h"

extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* const ERROR_PREFIX;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!

void free_variable(Mvariable* _variable){
    if(_variable){
        if(_variable->_name)free(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
        if(_variable->_value)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
        free(_variable);
    }
}
Mvariable* new_variable(const char* name,Mvaluetype valuetype,bool immutable){
    if(!name||!strlen(name)){
        outputError("No variable name defined");
        return NULL;
    }
    Mvariable* _variable=(Mvariable*)calloc(1,sizeof(Mvariable)); // all pointers will be NULL!!
    if(!_variable){
        outputErrorAndText("Failed to allocate memory to store variable ",name);
        return NULL;
    }
    _variable->immutable=immutable;
    _variable->_name=_strdup(name); // create a dynamic pointer on the heap
    if(!_variable->_name){
        free_variable(_variable);
        output("%sFailed to allocate memory to store name '%s' of the new variable.\n",ERROR_PREFIX,name);
        return NULL;
    }
    _variable->valuetype=valuetype;
    /* MDH@01MAY2019: we're NOT setting the value here!!!!
    // a ha, here we have an issue: we cannot just use the char pointer, if we want to free the name later on
    if(_variable->valuetype!=VT_UNDEFINED){
        _variable->_value=(Mvalue*)calloc(1,sizeof(Mvalue));
        if(!_variable->_value){
            free_variable(_variable);
            printf("\nERROR: Failed to allocate memory to store the value of variable '%s'.",name);
            return NULL;
        }
        _variable->_value->type=valueType;
        // I do NOT need to set the values of the atomic elements (integer, real), so basically these are not initialized to start with
        // NOTE using calloc() instead of malloc() is essential for everything with pointers in it that should be NULL initially!!
        switch(_variable->valueType){
            case VT_UNDEFINED:
            case VT_INTEGER:
            case VT_REAL:break;
            case VT_STRING:_variable->_value->value._string=(Mstring*)calloc(1,sizeof(Mstring));break;
            case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
            case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
        }
    }else
        _variable->_value=NULL;
    */
    return _variable;
}

void free_listelement(Mlistelement* _listelement){
    if(_listelement){
        if(_listelement->_next)free_listelement(_listelement->_next);
        if(_listelement->_value)decrementReferenceCount(_listelement->_value); ///////// replacing: free_value(_listelement->_value);
        free(_listelement);
    }
}
void free_list(Mlist* _list){
    if(_list){
        if(_list->numberOfElements){
            free_listelement(_list->_first);
            _list->numberOfElements=0; // just a precaution to not do it again
        }
        free(_list);
    }
}
void free_mapelement(Mmapelement* _mapelement){
    if(_mapelement){
        free_mapelement(_mapelement->_next);
        free_variable(_mapelement->_variable);
        free(_mapelement);
    }
}
void free_map(Mmap* _map){
    if(_map){
        if(_map->numberOfElements){
            free_mapelement(_map->_first);
            _map->numberOfElements=0;
        }
        free(_map);
    }
}

// MDH@01MAY2019: 'local' function for freeing a value
void free_value(Mvalue* _value){
    if(_value){
        if(amVerbose())output("Value of type %u to free.",_value->type);
        // I do not need to free the value itself, only the pointers inside it
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_TOKEN:if(_value->value._token)free_token(_value->value._token);break;
            case VT_INTEGER:if(_value->value._integer)free_integer(_value->value._integer);break;
            case VT_BIGINTEGER:if(_value->value._biginteger)free_biginteger(_value->value._biginteger);break;
            case VT_DECIMAL:if(_value->value._decimal)free_decimal(_value->value._decimal);break;
            case VT_RATIONAL:if(_value->value._rational)free_rational(_value->value._rational);break;
            case VT_REAL:if(_value->value._real)free_real(_value->value._real);break;
            case VT_STRING:free_string(_value->value._string);break;
            case VT_LIST:free_list(_value->value._list);break;
            case VT_MAP:free_map(_value->value._map);break;
        }
        if(amVerbose())output("Type-specific value freed.");
        free(_value);
    }else
        output("BUG: No value to free!");
}

// keep a list of allocated values
Mlist* _valueList=NULL;
Mvalue* new_value(){
    Mvalue* _value=NULL;
    if(!_valueList)_valueList=(Mlist*)calloc(1,sizeof(Mlist));
    if(_valueList){
        Mlistelement* _valueListelement=(Mlistelement*)calloc(1,sizeof(Mlistelement)); // both pointers NULL
        if(_valueListelement){
            _value=(Mvalue*)calloc(1,sizeof(Mvalue));
            if(_value){
                // shouldn't pose a problem now...
               _valueListelement->_value=_value;
                 if(_valueList->_last)_valueList->_last->_next=_valueListelement;else _valueList->_first=_valueListelement;
                _valueList->_last=_valueListelement;
                _valueList->numberOfElements++;
            }else // couldn't get a new value, so free the value list element immediately
                free(_valueListelement);
        }
    }
    if(!_value)if(amVerbose())outputLine("ERROR: Failed to create value!");
    return _value;
}

const char* VALUETYPENAMES[]={"unknown","integer","real","string","list","map"};

// can be asked to remove unused values
size_t getNumberOfRemovedValues(){
    unsigned long long removed=0;
    unsigned long long tofree=0; // how many value elements we should free
    if(_valueList){
        Mlistelement* _valueListelement=_valueList->_first;
        if(amVerbose())output("Number of values to check: %llu.\n",_valueList->numberOfElements);
        unsigned long long checked=0;
        while(_valueListelement){
            checked++;
            if(amVerbose())output("Checking value #%llu.",checked);
            if(_valueListelement->_value){
                if(amVerbose())output("\tChecking the count!");
                if(_valueListelement->_value->count==0){ // unused
                    if(amVerbose())output("About to free unused value #%llu of type '%s'.\n",checked,VALUETYPENAMES[_valueListelement->_value->type]);
                    free_value(_valueListelement->_value);
                    _valueListelement->_value=NULL; // just in case
                    tofree++;
                }else
                if(amVerbose())outputLine("\tStill in use!");
            }else
                output("%sNo value stored in value #%llu.\n",ERROR_PREFIX,checked);
            _valueListelement=_valueListelement->_next;
        }
        if(amVerbose())output("Number of values checked: %llu.\nNumber of value list elements to free: %llu.\n",checked,tofree);
        // the list is now intact, are we going to correct the links??????
        if(tofree){ // some values were freed
            Mlistelement* _firstValueListelement=NULL; // the first value list element to remain
            Mlistelement* _lastValueListelement=NULL; // the last value list element remaining
            Mlistelement* _nextValueListelement;
            _valueListelement=_valueList->_first;
            while(_valueListelement){
                _nextValueListelement=_valueListelement->_next; // remember the next before freeing
                if(_valueListelement->_value){ // this one is too remain in the list
                    if(!_firstValueListelement)_firstValueListelement=_valueListelement;
                    if(_lastValueListelement)_lastValueListelement->_next=_valueListelement;
                    _valueListelement->_next=NULL; // we can do this because we've already remembered the next one in the list at the start!!
                    _lastValueListelement=_valueListelement; // remember the last element in the list (now pointing to nothing as the last element should!!!
                }else{ // this one is to be removed
                    removed++;
                    // let's be careful here!!!
                    if(_valueList->numberOfElements)_valueList->numberOfElements--;else output("BUG: Trying to free a value list element that is not counted!");  // one less element in the list!!!
                    free(_valueListelement);
                }
                // next to check!!!
                _valueListelement=_nextValueListelement;
            }
            // update the first and last in the list (could both be NULL!!!)
            _valueList->_first=_firstValueListelement;
            _valueList->_last=_lastValueListelement;
        }
    }
    if(tofree){
        if(tofree>removed)output("WARNING: Failed to free %llu unused value list elements.\n",(tofree-removed));else if(amVerbose())outputLine("All unused value list elements freed!");
    }
    return removed;
}

unsigned long long getNumberOfValues(){return _valueList->numberOfElements;}

bool decrementReferenceCount(Mvalue* _value){
    if(_value){
        if(_value->count){
            (_value->count)--;
            /* all values referenced in list and maps should have their count decremented as soon as becoming zero
            // however it's more convenient to let the 'garbage collection' take care of that, in fact the free_map/free_list should do that!!!!
            if(!_value->count){
                if(!_value->type==VT_LIST){

                }
            }
            */
            return true;
        }
        mstring* _valueText=_getValueText(_value,false);
        output("BUG: Reference count of '%s' of type '%c' already zero.",_valueText,MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
        free_mstring(_valueText);
    }else
        if(amVerbose())outputLine("No value to decrement the reference count of.");
    return true;
}
bool incrementReferenceCount(Mvalue* _value){
    if(_value){
        (_value->count)++;
        return true;
    }
    if(amVerbose())outputLine("No value to increment the reference count of.");
    return false;
}
// interface functions that use the above functions
// wrapping the different value type instances
Mvalue* _getUndefinedValue(){return (Mvalue*)calloc(1,sizeof(Mvalue));}

Mvalue* _getDecimalValue(Mdecimal* _decimal,bool freeonfailure){
    if(!_decimal)return NULL;
    Mvalue* _decimalValue=new_value();
    if(_decimalValue){_decimalValue->type=VT_DECIMAL;_decimalValue->value._decimal=_decimal;}else if(freeonfailure)free_decimal(_decimal);
    return _decimalValue;
}
Mvalue* _getIntegerValue(long long ll){
    if(amVerbose())output("Wrapping integer '%ll'.\n",ll);
    Mvalue* _integerValue=new_value();
    if(_integerValue){
        _integerValue->value._integer=new_integer(ll);
        if(!_integerValue->value._integer){free_value(_integerValue);_integerValue=NULL;}else _integerValue->type=VT_INTEGER;
    }
    return _integerValue;
}
Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,bool freeonfailure){
    if(!_biginteger)return NULL;
    Mvalue* _bigintegerValue=new_value();
    if(_bigintegerValue){_bigintegerValue->type=VT_BIGINTEGER;_bigintegerValue->value._biginteger=_biginteger;}else if(freeonfailure)free_biginteger(_biginteger);
    return _bigintegerValue;
}
Mvalue* _getRealValue(long double ld){
    if(amVerbose())output("Wrapping real '%.*Lf'.\n",DBL_DIG,ld);
    Mvalue* _realValue=new_value();
    if(_realValue){
        _realValue->value._real=new_real(ld);
        if(!_realValue->value._real){free_value(_realValue);_realValue=NULL;}else _realValue->type=VT_REAL;
    }
    return _realValue;
}
Mvalue* _getStringValue(char* _s,bool freeonfailure){
    if(!_s)return NULL; // when no input, no go
    Mvalue* _stringValue=NULL; // the result, when NULL check freeonfailure
    Mstring* _string=new_string(_s); // for mstring* sources pass string(mstring*) into getStringValue() (which points to mstring->chars which always start with the quote char used in declaring the literal)
    if(_string){
        _stringValue=new_value();
        if(_stringValue){_stringValue->type=VT_STRING;_stringValue->value._string=_string;}else free_string(_string); // always free the Mstring if it is not bound!!!
    }
    if(!_stringValue)if(freeonfailure)free(_s);
    return _stringValue;
}
Mvalue* _getCharStringValue(char _c){
    Mstring* _string=(_c?new_charstring(_c):NULL); // for mstring* sources pass string(mstring*) into getStringValue() (which points to mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _stringvalue=(_string?new_value():NULL);
    if(_stringvalue){_stringvalue->type=VT_STRING;_stringvalue->value._string=_string;}
    return _stringvalue;
}
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype){
    Mlist* _list=(Mlist*)calloc(1,sizeof(Mlist));
    _list->valuetype=listValuetype; // register what type of elements this list should have
    Mvalue* _listvalue=(_list?new_value():NULL);
    if(_listvalue){_listvalue->type=VT_LIST;_listvalue->value._list=_list;}
    return _listvalue;
}
Mvalue* _getMapValue(Mvaluetype mapValuetype){
    Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
    _map->valuetype=mapValuetype;
    Mvalue* _mapvalue=(_map?new_value():NULL);
    if(_mapvalue){_mapvalue->type=VT_MAP;_mapvalue->value._map=_map;}
    return _mapvalue;
}

// some other wrappers
Mvalue* _getValueOfInteger(Minteger* _integer,bool freeonfailure){if(!_integer)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_INTEGER;_value->value._integer=_integer;}else if(freeonfailure)free_integer(_integer);return _value;}
Mvalue* _getValueOfReal(Mreal* _real,bool freeonfailure){if(!_real)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_REAL;_value->value._real=_real;}else if(freeonfailure)free_real(_real);return _value;}
Mvalue* _getValueOfMap(Mmap* _map,bool freeonfailure){if(!_map)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_MAP;_value->value._map=_map;}else if(freeonfailure)free_map(_map);return _value;}
Mvalue* _getValueOfToken(Mtoken* _token,bool freeonfailure){if(!_token)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_TOKEN;_value->value._token=_token;}else if(freeonfailure)free_token(_token);return _value;}

// helper function to create a parameter map with a single value
// the following is a nuisance
Mmap* _getRealMap(char* name,Mvalue* _realValue){
    if(name&&_realValue){
        Mvariable* _realVariable=new_variable(name,VT_REAL,true);
        if(_realVariable){
            assignValue(&_realVariable->_value,_realValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_realVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amDebugging())outputLine("Returning the single real map!");
                    return _map;
                }
                outputError("Failed to create the real variable map");
               free_mapelement(_mapelement);
            }else{
                outputError("Failed to create the real variable map element");
                free_variable(_realVariable);
            }
        }else
            outputError("Failed to create the real variable");
    }
    return NULL;
}
Mmap* _getMap(char *name){
    Mvariable* _variable=new_variable(name,VT_UNDEFINED,true);
    Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
    if(_mapelement){
        _mapelement->_variable=_variable;
        Mmap* _map=calloc(1,sizeof(Mmap));
        _map->_first=_mapelement;
        _map->_last=_mapelement;
        _map->numberOfElements=1;
        return _map;
    }
    return NULL;
}
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue){
    if(name&&_integerValue){
        Mvariable* _integerVariable=new_variable(name,VT_INTEGER,true);
        if(_integerVariable){
            assignValue(&_integerVariable->_value,_integerValue); ///////  incrementReferenceCount(_integerValue); // now bound to the integer variable
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_integerVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amVerbose())outputLine("Returning the single integer map!");
                    return _map;
                }
                outputError("Failed to create the integer variable map");
               free_mapelement(_mapelement);
            }else{
                outputError("Failed to create the integer variable map element");
                free_variable(_integerVariable);
            }
        }else
            outputError("Failed to create the integer variable");
    }
    return NULL;
}
Mmap* _getStringStringMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&!strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            Mmapelement* _mapelement2=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement1&&_mapelement2){
                _mapelement1->_variable=new_variable(name1,VT_STRING,true);
                _mapelement2->_variable=new_variable(name2,VT_STRING,true);
                if(_mapelement1->_variable&&_mapelement2->_variable){
                    Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                    if(_map){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                }
            }
        }
    }
    return NULL;
}
Mmap* _getListMap(char* name,Mvalue* _listValue){
    if(name&&_listValue){
        Mvariable* _listVariable=new_variable(name,VT_LIST,true);
        if(_listVariable){
            assignValue(&_listVariable->_value,_listValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_listVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amDebugging())outputLine("Returning the single list map");
                    return _map;
                }
                outputError("Failed to create the list variable map");
               free_mapelement(_mapelement);
            }else{
                outputError("Failed to create the list variable map element");
                free_variable(_listVariable);
            }
        }else
            outputError("Failed to create the list variable");
    }
    return NULL;
}
// end helper functions 

// LIST STUFF
Mvalue* _getValueOfList(Mlist* _list,bool freeonfailure){if(!_list)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_LIST;_value->value._list=_list;}else if(freeonfailure)free_list(_list);return _value;}
void checkList(Mlist* _list){
    if(_list){
        long long l=_list->numberOfElements;
        if(l>0){
            Mlistelement* _listelement=_list->_first;
            if(_listelement){
                // the number of elements in the list should match the number of counted elements
                long long listelementindex=0;
                while(true){
                    if(l==0)outputError("More elements in list than accounted for");
                    l--;
                    if(_listelement->index<=listelementindex)output("%sList element index (%lld) below the expected list element index (%lld).",ERROR_PREFIX,_listelement->index,listelementindex);
                    listelementindex=_listelement->index;
                    if(!_listelement->_next){
                        if(_list->_last!=_listelement)outputError("Registered last list element not equal to the actual last list element");
                        break;                        
                    }
                    output("List element with index %llu OK.",_listelement->index);
                    _listelement=_listelement->_next;
                }
                if(l>0)outputError("Less elements in list than accounted for");else 
                if(l<0)output("%s%lld more elements in list than counted.",ERROR_PREFIX,(-l));
            }else
                output("%sList with %lld elements does not have a first element!",ERROR_PREFIX,l);
        }else{
            if(_list->_first)outputError("Empty list with first element");
            if(_list->_last)outputError("Empty list with last element");
        }
    }
}
// instead of returning a boolean we could return the assigned index (0 on failure)
// MDH@02JUN2019: check (and correct) prepending
unsigned long long appendedToList(Mlist* const _list,Mvalue* const _value,long long index){
    if(!_list||!_value){outputError("No list to append to or no value to append");return 0;}
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    if(index<=0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    if(index<=0){output("%sIndex %lld of (new) list element too small.\n",index);return 0;}
    if(amVerbose())outputValue("\nAppending '",_value,"' to list.");
    // MDH@23MAY2019: let's allow inserting or replacing as well
    // determine _listelement as element to host the value, store the successor in _nextlistelement
    Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index<=lastindex?_list->_first:NULL);
    if(_listelement){ // we are not appending and we have a first element, so this might be an insert or replace
        // NOTE testing _listelement is just a fail-safe as that should never happen
        while(index>_listelement->index){
            _prevListelement=_listelement;
            if(!_listelement->_next){outputValue("\nBUG: Index of list element '",_listelement->_value,"' probably out of order.");return 0;}
            _listelement=_listelement->_next;
        }
        // if we're going to insert there will be a successor
        if(_listelement->index!=index){_nextListelement=(_prevListelement?_prevListelement->_next:_list->_first);_listelement=NULL;} // so that we are forced to create one
    }else // we'll be appending, so the current last is the predecessor (and no successor)
        _prevListelement=_list->_last;
    // if we do not have a list element ascertain to have one
    if(!_listelement){ // not yet present in list, so we have to create a new element
        _listelement=(Mlistelement*)calloc(1,sizeof(Mlistelement));
        if(!_listelement){outputError("Failed to create a list element to insert/append");return 0;} // failure
    }
    assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
    // if replacing i.e. the index of _listelement matches index, we're done
    if(_listelement->index!=index){ // insert or append
        _listelement->index=index;
        (_list->numberOfElements)++; // an additional element
        // linking
        if(_prevListelement)_prevListelement->_next=_listelement;else _list->_first=_listelement;
        if(!_nextListelement){if(_list->_last)_list->_last->_next=_listelement;_list->_last=_listelement;}else _listelement->_next=_nextListelement;
    }
    if(amVerbose())checkList(_list);
    return _listelement->index;
}

Mvalue* getValueAtIndex(Mlist* _list,long long index){
    // NOTE if index is equal to zero definitely no value there!!!
    if(_list&&index){
        if(_list->_last){
            long long maxindex=_list->_last->index;
            if(index<0)index+=(maxindex+1);
            if(index>0){
                if(index==maxindex)return _list->_last->_value;
                if(index<maxindex){
                    Mlistelement* _listelement=_list->_first;
                    while(_listelement){
                        if(_listelement->index==index)return _listelement->_value;
                        if(_listelement->index>index)break; // couldn't find it!!!
                        _listelement=_listelement->_next;
                    }
                }
            }
        }
    }
    return NULL;
}
// END LIST STUFF

// MAP STUFF
// MDH@24MAY2019: if already in the map should replace the current value
bool appendedToMap(Mmap* _map,const char* attributeName,Mvalue* _attributeValue){
    if(!_map||!attributeName)return false;
    if(amVerbose()){output("Setting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.");}
    Mmapelement* _mapelement=_map->_first;
    while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name,attributeName))_mapelement=_mapelement->_next;
    if(!_mapelement){ // not found
        _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement)); // NOTE no need to set _next because it is now NULL
        if(!_mapelement)return false;
        _mapelement->_variable=(Mvariable*)calloc(1,sizeof(Mvariable));
        if(!_mapelement->_variable)return false;
        _mapelement->_variable->_name=_strdup(attributeName); // if we change name into _name (as mstring*) we won't have to free attributeName which holds the character array 
        if(_map->numberOfElements)_map->_last->_next=_mapelement;else _map->_first=_mapelement;
        _map->_last=_mapelement;
        _map->numberOfElements++;
    }
    assignValue(&_mapelement->_variable->_value,_attributeValue); // replace the current attribute value with the new value
    return true;
}

Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName){
    if(_map&&attributeName&&strlen(attributeName)){
        Mmapelement* _mapelement=_map->_first;
        if(_mapelement){            
            // keep looking until there is a match
            while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name,attributeName))_mapelement=_mapelement->_next;
            // if there is a match return the associated value
            if(_mapelement&&_mapelement->_variable)return _mapelement->_variable->_value;
        }
    }
    return NULL;
}
// END MAP STUFF

Mvalue* _getRationalValue(Mrational* _rational,bool freeonfailure){if(!_rational)return NULL;Mvalue* _value=new_value();if(_value){_value->type=VT_RATIONAL;_value->value._rational=_rational;}else if(freeonfailure)free_rational(_rational);return _value;}

//////////mstring* _getValueText(Mvalue* _value); // forward prototype used in getListText() and getMapText()
mstring* _getListText(Mlist* _list){
    ///////output("List to output.");char c;inputCharRead(&c);
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'l');
	if(p){
		p=string_append_char(p,'['); // switch to using p in appends
		/////////size_t l=_list->numberOfElements;
        unsigned long long listindex=1;
		Mlistelement* _listelement=_list->_first;
		Mvalue* _listelementValue;
		while(p&&_listelement){
            ///////outputChar('$');
            // increment listindex until it is equal to _listelement->index
            if(!_listelement->index)break;
            while(listindex<_listelement->index){listindex++;p=string_append_char(p,',');}
            /////////p=appendull(p,_listelement->index);p=string_append_char(p,':');if(!p)break;
	        //////////outputChar('.');
			_listelementValue=_listelement->_value;
			if(_listelementValue){
				mstring* _listelementValueText=_getValueText(_listelementValue,false);
				if(_listelementValueText){
					p=string_append(p,string(_listelementValueText));
					free_mstring(_listelementValueText); // release AFTER copying over
				}
			}
			_listelement=_listelement->_next;
		}
		p=string_append_char(p,']');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}
mstring* _getMapText(Mmap* _map){
	mstring* s=string_create();
	mstring* p=s;
    if(amDebugging())p=string_append_char(p,'m');
	if(p){
        p=string_append_char(p,'{');
		//////output("%s",string(p));
		Mmapelement* _mapelement=_map->_first;
		while(p&&_mapelement){
			//////output("%s","start");
			Mvariable* _variable=_mapelement->_variable;
			if(!_variable)continue;
            // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
            p=string_append_char(p,'\'');
			p=string_append(p,_variable->_name);
            p=string_append_char(p,'\'');
			/////output("%s",string(p));
			p=string_append_char(p,':'); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
			/////output("%s",string(p));
			mstring* mapelementValueText=_getValueText(_variable->_value,false);
			/////output("Map element: %s",string(p));
			if(!mapelementValueText)continue;
			p=string_append(p,string(mapelementValueText)); // append 
			free_mstring(mapelementValueText); // release AFTER copying over
			_mapelement=_mapelement->_next;
			if(_mapelement)p=string_append(p,", "); // only when there's a next map element to process
			////output("%s","next");
		}
		//////output("%s(%d)",string(p),string_length(p));
		p=string_append_char(p,'}');
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}

mstring* _getValueText(const Mvalue* const _value,bool dequoted){
	// NOTE whatever is returned should be freed
	mstring* valueText=NULL;
    //////outputChar('.');
	if(_value){
        ////////outputChar('+');
		////////printf("\nTYPE: %d",_value->type);
		switch(_value->type){
			case VT_INTEGER:valueText=_getIntegerText(_value->value._integer);break;
            case VT_BIGINTEGER:valueText=_getBigintegerText(_value->value._biginteger);break; // how many characters do we need????
            case VT_DECIMAL:valueText=_getDecimalText(_value->value._decimal,false);break; // fixedpoint to obligatory (i.e. e-notation allowed for very big/small (positive) numbers)
            case VT_RATIONAL:valueText=_getRationalText(_value->value._rational);break;
			case VT_REAL:valueText=_getRealText(_value->value._real);break;
			case VT_STRING:valueText=_getStringText(_value->value._string,dequoted);break; // TODO don't dequote the text!!
			case VT_MAP:valueText=_getMapText(_value->value._map);break;
			case VT_LIST:valueText=_getListText(_value->value._list);break;
            case VT_TOKEN:valueText=string_copy(_value->value._token->text);break; // we need to return a copy because that copy will be freed typically (and we do not want to free the original now do we?)
			default:break;
		}
	}
    if(valueText)if(amAssisting())valueText=appendll(string_append_char(valueText,'#'),_value->count); // show the reference count as well
    ////////outputChar('.');
    return(valueText?valueText:getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(string_create(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}
void outputValue(const char* const prefix,const Mvalue* _value,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_value){
        mstring* _valueText=_getValueText(_value,false);
        output("%s",string(_valueText));free_mstring(_valueText);
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}

long long getValueInteger(const Mvalue* const _value){
    // ASSERTION if _value can be converted to an integer,it should not equal invalid!!!!
    if(_value){
        switch(_value->type){
            case VT_INTEGER:return _value->value._integer->ll;
		    case VT_REAL:return double2long(_value->value._real->ld);
            case VT_BIGINTEGER:return biginteger2long(_value->value._biginteger);
            case VT_STRING:
                {
                    // alternative which doesn't check for 0 explicitly I think: _strtoll(_value->value._string->_c,M_LL_INVALID);
                    // we should check for 0 explicitly, which should always be represented by a single '0' character i.e. without signs!!!
                    // TODO are we allowing other zero representations as well???
                    if(strIsZero(_value->value._string->_c))return 0;
                    long long ll=atoll(_value->value._string->_c); // invalid if zero
                    if(ll)return ll; // valid if non-zero
                }
                break;
            case VT_RATIONAL:
                {
                    Mbiginteger* _biginteger=_rational2biginteger(_value->value._rational);
                    if(_biginteger){
                        long long ll=biginteger2long(_biginteger);
                        free_biginteger(_biginteger);
                        return ll;
                    }
                }
   		    default:break;
        }
        // TODO sometimes reals can also represent integers!!!
    }
    return M_LL_INVALID;
}
long double getValueReal(const Mvalue* const _value){return(_value&&_value->type==VT_REAL?_value->value._real->ld:M_LD_NAN);}
long double getRealLongDouble(const Mreal* const _real){return(_real?_real->ld:M_LD_NAN);}

Mbiginteger* _getValueBiginteger(const Mvalue* const _value){
    if(_value)
        switch(_value->type){
        case VT_BIGINTEGER:return _value->value._biginteger;
        case VT_INTEGER:return _getBiginteger(_value->value._integer->ll);
        case VT_RATIONAL:return _rational2biginteger(_value->value._rational); // TODO how can we be certain that the returned big integer is actually used? well, it should as this is _getValueBiginteger meaning you have to free it if you don't use it!!!
        case VT_REAL:
            {
                // this is a bit of a nuisance when the double is out of the VT_INTEGER range
                Mbiginteger* _biginteger=new_biginteger();
                if(mp_set_longdouble(_biginteger,_value->value._real->ld)==MP_OKAY)return _biginteger;
                outputValue("\nERROR: Failed to convert `",_value,"` to a big integer.");
                mp_clear(_biginteger);
            }
            break;
        case VT_STRING:
            {
                Mbiginteger* _biginteger=new_biginteger();
                if(mp_read_radix(_biginteger,_value->value._string->_c,10)==MP_OKAY)return _biginteger;
                outputValue("\nERROR: Failed to convert `",_value,"` to a big integer.");
                mp_clear(_biginteger); // not used so free immediately
            }
        default:break;
    }
    return NULL;
}
// (map) list conversions
bool listAppendedToMap(Mmap* const _map,const Mlist* const _list){ // appends a list to a (possibly empty) map using the indices as attribute name
    bool result=(_map!=NULL); // no map, no result!
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(_listelement){
                mstring* _indexValueText=appendll(string_create(),_listelement->index);
                if(_indexValueText){
                    if(!appendedToMap(_map,string(_indexValueText),_listelement->_value))result=false;
                    free_mstring(_indexValueText);
                }else{
                    result=false;
                    output("Failed to convert index %llu to attribute name.",_listelement->index);
                }
                if(!result)break;
                _listelement=_listelement->_next;
            }
        }
    }
    return result;
}
bool listAppendedToMaplist(Mlist* const _maplist,const Mlist* const _list){
    bool result=(_maplist&&_maplist->valuetype==VT_LIST); // the destination list should only allow for list elements
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(result&&_listelement){
                // index and value of the list element are stored in a new list!!
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED);
                Mlist* _maplistelement=(_maplistelementValue?_maplistelementValue->value._list:NULL);
                // if we fail to construct the maplist element or to add it
                if(!_maplistelement||!appendedToList(_maplistelement,_getIntegerValue(_listelement->index),0)||!appendedToList(_maplistelement,_listelement->_value,0)||!appendedToList(_maplist,_maplistelementValue,0))
                    result=false;
                else
                    _listelement=_listelement->_next;
            }
        }
    }
    return result;
}
bool maplistAppendedToList(Mlist* const _list,const Mlist* const _maplist){
    bool result=(_list!=NULL); // no list, no result!
    if(result){
        if(_maplist){ // something to copy
            Mlistelement* _maplistelement=_maplist->_first;
            Mvalue* _maplistelementValue;
            while(result&&_maplistelement){
                _maplistelementValue=_maplistelement->_value;
                // each map list element should be a list with at least two elements
                if(_maplistelementValue&&_maplistelementValue->type==VT_LIST&&_maplistelementValue->value._list->numberOfElements>1){
                    // the first element in the map list element should be a positive integer that we can use as index
                    long long index=getValueInteger(_maplistelementValue->value._list->_first->_value);
                    // if the index is positive AND we fail to copy the second list element over, we failed!!!
                    if(index>0&&!appendedToList(_list,_maplistelementValue->value._list->_first->_next->_value,index))
                        result=false;
                }
                _maplistelement=_maplistelement->_next;
            }
        }
    }
    return result;
}
bool maplistAppendedToMap(Mmap* const _map,const Mlist* const _maplist){
    bool result=(_map!=NULL);
    if(result){
        if(_maplist){
            Mlistelement* _maplistelement=_maplist->_first;
            Mvalue* _maplistelementValue;
            while(result&&_maplistelement){
                _maplistelementValue=_maplistelement->_value;
                // each map list element should be a list with at least two elements
                if(_maplistelementValue&&_maplistelementValue->type==VT_LIST&&_maplistelementValue->value._list->numberOfElements>1){
                    // the first element becomes the key the second element the attribute value
                    // BUT I suppose composite keys (maps or lists) are not allowed
                    Mvalue* _attributeNameValue=_maplistelementValue->value._list->_first->_value;
                    mstring* _attributeNameValueText=NULL;
                    if(_attributeNameValue){
                        if(_attributeNameValue->type==VT_INTEGER)_attributeNameValueText=_getIntegerText(_attributeNameValue->value._integer);else
                        if(_attributeNameValue->type==VT_REAL)_attributeNameValueText=_getRealText(_attributeNameValue->value._real);else
                        if(_attributeNameValue->type==VT_STRING)_attributeNameValueText=_getStringText(_attributeNameValue->value._string,true); // effectively cutting of the presuffix character TODO other solution????????
                    }
                    if(_attributeNameValueText){
                        if(!string_length(_attributeNameValueText)||!appendedToMap(_map,string(_attributeNameValueText),_maplistelementValue->value._list->_first->_next->_value))result=false;
                        free_mstring(_attributeNameValueText);
                    }
                }
                _maplistelement=_maplistelement->_next;
            }
        }
    }
    return result;
}
// map to (map) list conversions
bool mapAppendedToList(Mlist* const _list,Mmap* const _map){
    bool result=(_list!=NULL);
    if(result){
        if(_map&&_map->_first){
            Mmapelement* _mapelement=_map->_first;
            while(result&&_mapelement){
                // only add those map elements of which the key can be converted to a positive integer
                long long index=atoll(_mapelement->_variable->_name);
                if(index>0&&!appendedToList(_list,_mapelement->_variable->_value,index))result=false;
                _mapelement=_mapelement->_next;
            }
        }
    }
    return result;
}
bool mapAppendedToMaplist(Mlist* const _maplist,Mmap* const _map){
    bool result=(_maplist&&_maplist->valuetype==VT_LIST);
    if(result){
        if(_map&&_map->_first){
            Mmapelement* _mapelement=_map->_first;
            while(result&&_mapelement){
               // index and value of the list element are stored in a new list!!
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED);
                Mlist* _maplistelement=(_maplistelementValue?_maplistelementValue->value._list:NULL);
                if(_maplistelement){
                    // if we fail to construct the maplist element or to add it
                    // NOTE the attribute name does not start with a quote character wich we need to call _getStringValue
                    mstring* _attributeName=string_create();
                    if(_attributeName){
                        string_append_char(_attributeName,'\'');
                        string_append(_attributeName,_mapelement->_variable->_name);
                        Mvalue* _attributeNameValue=_getStringValue(string(_attributeName),false);
                        if(_attributeNameValue&&appendedToList(_maplistelement,_attributeNameValue,0)){
                            if(!appendedToList(_maplistelement,_mapelement->_variable->_value,0)||!appendedToList(_maplist,_maplistelementValue,0))result=false;
                        }else
                            result=false;
                        free_mstring(_attributeName);
                    }else
                        result=false;                
                }else
                    result=false;
                _mapelement=_mapelement->_next;
            }
        }
    }
    return result;
}

Mlist* _getListOfType(Mvaluetype valuetype){Mlist* _list=calloc(1,sizeof(Mlist));_list->valuetype=valuetype;return _list;}
Mmap* _getMapOfType(Mvaluetype valuetype){Mmap* _map=calloc(1,sizeof(Mmap));_map->valuetype=valuetype;return _map;}

bool isValueZero(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==0;
        if(_value->type==VT_BIGINTEGER)return isBigintegerZero(_value->value._biginteger);
        if(_value->type==VT_REAL)return ldIsZero(_value->value._real->ld);
        if(_value->type==VT_DECIMAL)return isDecimalZero(_value->value._decimal);
        if(_value->type==VT_RATIONAL)return isBigintegerZero(_value->value._rational->num);
    }
    return false;
}
bool isValueOne(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==1;
        if(_value->type==VT_BIGINTEGER)return isBigintegerOne(_value->value._biginteger);
        if(_value->type==VT_DECIMAL)return isDecimalOne(_value->value._decimal);
        if(_value->type==VT_RATIONAL)return isRationalOne(_value->value._rational);
        if(_value->type==VT_REAL)return _value->value._real->ld==1;
    }
    return false;
}

// null test for the value to be considered NULL
bool isNull(Mvalue* _value){
    if(_value)
    switch(_value->type){
        case VT_INTEGER:return !_value->value._integer;
        case VT_BIGINTEGER:return !_value->value._biginteger;
        case VT_DECIMAL:return !_value->value._decimal;
        case VT_RATIONAL:return !_value->value._rational;
        case VT_REAL:return !_value->value._real;
        case VT_STRING:return !_value->value._string;
        case VT_LIST:return !_value->value._list;
        case VT_MAP:return !_value->value._map;
        case VT_TOKEN:return !_value->value._token;
        default:break;
    }
    return true;
}

// MDH@04JUN2019: based on https://stackoverflow.com/questions/4637967/algorithm-challenge-generate-continued-fractions-for-a-float/56444882#56444882
Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter){
    // if iterations, you're supposed to return all iteration results
    Mlist* _iterationsList=_getListOfType(VT_UNDEFINED);
    if(_iterationsList){
        Mrational* _rational=NULL; // the last (computed) rational
        if(!ldIsNaN(ld)&&!ldIsInf(ld)){ // neither a NaN nor Inf      
            if(!ldIsZero(ld)){
                bool neg=(ld<0);if(neg)ld=-ld; // remember if negative
                // ASSERT ld is positive 
                long long pmin1=1,pmin2=0,qmin1=0,qmin2=1;
                long long a,p,q;
                long double delta,rem=ld;
                // ascertain to execute the following at least once (so when maxiter<=1 we at least get the integer part of the rational)
                for(int i=1;i<=MAX(maxiter,1);i++){
                    a=lrint(floorl(rem));
                    p=a*pmin1+pmin2;
                    q=a*qmin1+qmin2;
                    ////////printf("\nIteration #%u: %lld:  %lld/%lld", i, a, p, q);
                    ///////printf(" - delta: %.*Lf, rem: %.*Lf",LDBL_DIG,delta,LDBL_DIG,rem);
                    ///// doesn't work!!!!! if(fabsl(rem)<eps)return;
                    delta=(ld*q)-p;
                    if(ldIsZero(delta))break; ///// MDH@07JUN2019: when a list is returned like this don't stop below the system's epsilon but only when the delta is zero!!!!
                    ////////////replacing (see above): if(fabsl(delta)<=M_LD_Q_EPS)break; // if the p and q we've got are fine, stop!!!
                    _rational=_getRational(_getBiginteger(neg?-p:p),_getBiginteger(q),delta,false,true); // construct the intermediate result without normalizing
                    if(!_rational){output("%sFailed to construct the intermediate rational %lld/%lld",ERROR_PREFIX,p,q);break;}
                    // NOT being able to append the intermediate result to the list shouldn't be enough reason to abort, as long as we manage to add the end result
                    Mvalue* _rationalValue=_getRationalValue(_rational,true);
                    if(!_rationalValue){outputError("Failed to value wrap the intermediate rational approximation to a real");break;}
                    if(!appendedToList(_iterationsList,_rationalValue,i)){free_rational(_rational);outputError("Failed to register a intermediate rational approximation");/*break;*/}
                    rem-=a;
                    rem=1/rem;
                    // shift the lot
                    pmin2=pmin1;qmin2=qmin1;
                    pmin1=p;qmin1=q;
                }
                // construct the last rational (i.e. the result) from p and q
                Mbiginteger* _numerator=_getBiginteger(p),*_denominator=_getBiginteger(q);
                if(_numerator&&_denominator)if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)_rational=_getRational(_numerator,_denominator,delta,true,true);
            }else // long double is zero, TODO should we store 0 as the delta, or just NaN???? what would be the difference??????
                _rational=_getRational(new_biginteger(),NULL,M_LD_NAN,false,true);
        }
        // _rational should contain the 'last' computed rational
        if(_rational){
            Mvalue* _rationalValue=_getRationalValue(_rational,true);
            if(_rationalValue){
                // how about reporting backwards????
                if(appendedToList(_iterationsList,_rationalValue,0))return _iterationsList;
                // ASSERT failed to append the rational approximation to the iterations list
                // free whatever's NOT being returned NOTE the list itself will be freed below
                free_value(_rationalValue);
                outputError("Failed to append the rational of a real to the result list");
            }else // failed to wrap the rational
                outputError("Failed to store the rational approximation");
        }
        if(_iterationsList)free_list(_iterationsList);
    }
    return NULL;
}

void assignValue(Mvalue** _valueholder,Mvalue* const _value){
    if(*_valueholder)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
    *_valueholder=_value; // replace what's being pointed to
    if(*_valueholder)incrementReferenceCount(*_valueholder); // increment what it's pointing to now (if not NULL)
}
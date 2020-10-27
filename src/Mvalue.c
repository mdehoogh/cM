#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <dirent.h>

#include "Mvalue.h"

static uint16_t const MODULE_ID=13;
static Mallocationowner getOwner(int16_t id){return(Mallocationowner){MODULE_ID,id};}

extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_POSITIVE,M_NEGATIVE,M_ZERO;
extern const char * const VALUETYPENAMES[]; // the characters associated with each of the value types
extern const char * const MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const M_ERROR_PREFIX;
extern const char * const M_WARNING_PREFIX;
extern const char * const M_BUG_PREFIX;
extern const char * const M_NULL_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value that is NULL
extern const char * const M_UNDEFINED_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value of type VT_UNDEFINED
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const long double LD_PI; // for Mfacd()
extern Mdecimalcontext * const M_DECIMALCONTEXT; // the application-wide (default) decimal context
extern const char M_DEREFERENCE_CHARACTER; // MDH@11MAR2020

Mvariable* disowned_variable(Mvariable* _variable,Mallocationowner owner_variable){
    if(_variable->_name)disowned_chars(_variable->_name,owner_variable); // dynamically allocated (indicated by _) so we should free it...
    return(Mvariable*)DISOWNED(_variable,owner_variable);
}
Mvariable* owned_variable(Mvariable* _variable,Mallocationowner owner_variable){
    if(_variable->_name)owned_chars(_variable->_name,Msubowner(owner_variable,1)); // dynamically allocated (indicated by _) so we should free it...
    return(Mvariable*)OWNED(_variable,owner_variable);
}
void free_variable(Mvariable* _variable,bool weak){
    if(_variable->_name)freeChars(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
    if(!weak)if(_variable->_value)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
    FREE_1(_variable,'V');
}/* VALIDATED */

// MDH@09JUN2020: if we want the name of the variable to be subowned by the caller, it's better to pass in a properly owned Mchars name, which we can subown
//                it's a bit of a nuisance that we create it in such a way that the caller has to take care of the ownership of _name but that makes sense because we pass in Mchars (which is already managed)
// MDH@11JUN2020: by allowing _name to be disowned to start with we can free it when we fail to bind it
Mvariable* _getVariable(Mchars const * const _name,Mvaluetype valuetype,bool immutable){Mallocationowner owner=getOwner(__LINE__);
    // MDH@14NOV2019: maps might have attributes with no name (i.e. the empty string)
    if(_name){
        Mvariable* _variable=(Mvariable*)CALLOC_1(sizeof(Mvariable),'V',owner); // all pointers will be NULL!!
        if(_variable){
            _variable->_name=(Misdisowned(_name)?owned_chars(_name,Msubowner(owner,1)):_name); // MDH@17APR2020 _strdup() replaced by _getChars(): // create a dynamic pointer on the heap
            _variable->immutable=immutable;
            _variable->valuetype=valuetype;
            // output("Returning new disowned variable '%s'.\n",_name); // DEBUG
            return disowned_variable(_variable,owner);
        }
        if(Misdisowned(_name))freeChars(_name);
        outputError("Failed to create a variable.");
    }else
        outputError("No variable name defined");
    /*
     if(!_variable->_name){
        freeChars(_name,owner);
        free_variable(_variable,true,owner);
        output("%sFailed to allocate memory to store name '%s' of the new variable.\n",M_ERROR_PREFIX,_name->chars);
        return NULL;
    }
    _variable->valuetype=valuetype;
    */
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
            case VT_FLOAT:break;
            case VT_TEXT:_variable->_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
            case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
            case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
        }
    }else
        _variable->_value=NULL;
    */
    // MDH@04JUN2020: given that _variable is already disowned (as returned by CALLOC_1), no need to disown it again
    return NULL; // replacing: return DISOWNED(_variable,owner);
}/* VALIDATED */

Mlistelement* owned_listelement(Mlistelement* _listelement,Mallocationowner owner_listelement){
    if(!_listelement)return NULL;
    owned_listelement(_listelement->_next,owner_listelement);
    return OWNED(_listelement,owner_listelement);
}
Mlistelement* disowned_listelement(Mlistelement* _listelement,Mallocationowner owner_listelement){
    if(!_listelement)return NULL;
    if(_listelement->_next)disowned_listelement(_listelement->_next,owner_listelement);
    return DISOWNED(_listelement,owner_listelement);
}
// MDH@18JUN2020: how about returning the number of list elements removed or perhaps the last list element removed????
//                let's return the number of list elements freed
long long free_listelement(Mlistelement* _listelement,bool weak/*,Mallocationowner owner*/){
    long long result=M_LL_INVALID; // when there's nothing to free!!!!
    if(_listelement){
        // free_listelement on a non-null next will ALWAYS return a positive value
        result=(_listelement->_next?free_listelement(_listelement->_next,weak):0);_listelement->_next=NULL;
        if(_listelement->_value){if(!weak)decrementReferenceCount(_listelement->_value);_listelement->_value=NULL;} ///////// replacing: free_value(_listelement->_value);
        FREE_1(_listelement,'l'/*,owner*/);
        result+=1;
    }
    return result;
}/* VALIDATED */

Mlist* owned_list(Mlist* _list,Mallocationowner owner_list){
    if(!_list)return NULL;
    if(_list->_creator)owned_chars(_list->_creator,Msubowner(owner_list,1));
    if(_list->_first)owned_listelement(_list->_first,owner_list);
    return OWNED(_list,owner_list);
}
Mlist* disowned_list(Mlist* _list,Mallocationowner owner_list){
    if(!_list)return NULL;
    if(_list->_creator)disowned_chars(_list->_creator,owner_list);
    if(_list->_first)disowned_listelement(_list->_first,owner_list);
    return DISOWNED(_list,owner_list);
}
void free_list(Mlist* _list){
    if(amVerboseDebugging())
    {output("Freeing a %s list",_list->weak?"weak":"strong");if(_list->_creator)output(" created by '%s'",_list->_creator->chars);outputChar('.');outputChar('\n');}
    if(_list->_first){
        // MDH@17APR2020: assuming we allocated exactly the number of characters for storing the characters
        if(_list->_creator)freeChars(_list->_creator/*,owner*/);// MDH@17APR2020 replacing: FREE_DISOWNED_1(_list->_creator,'"');
        free_listelement(_list->_first,_list->weak/*,owner*/);
        _list->_first=NULL;
    }
    FREE_1(_list,'L'/*,owner*/);
}/* VALIDATED */
Mlist* __list(char* source/*,Mallocationowner owner_list*/){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=CALLOC_1(sizeof(Mlist),'L',owner);
    if(source){
        _list->_creator=owned_chars(_getChars(source),Msubowner(owner,1)); // replacing: keep disowned, so easy to reown... SUBOWNED(OWNED(_getChars(source),owner),1); // MDH@17APR2020 replacing: _strdup(source);
        if(!_list->_creator)
            output("%sFailed to register list creator '%s'.\n",M_ERROR_PREFIX,source);
        else
        if(amVerboseDebugging())
            output("List creator: '%s'.\n",_list->_creator->chars);
    }
    return DISOWNED_LIST(_list,owner);
}
#ifndef __PRODUCTION__
Mmapelement* owned_mapelement(Mmapelement* _mapelement,Mallocationowner owner_mapelement){
    if(!_mapelement)return NULL;
    owned_mapelement(_mapelement->_next,owner_mapelement);
    owned_variable(_mapelement->_variable,Msubowner(owner_mapelement,1));
    return OWNED(_mapelement,owner_mapelement);
}
Mmapelement* disowned_mapelement(Mmapelement* _mapelement,Mallocationowner owner_mapelement){
    if(!_mapelement)return NULL;
    disowned_mapelement(_mapelement->_next,owner_mapelement);
    disowned_variable(_mapelement->_variable,owner_mapelement);
    return DISOWNED(_mapelement,owner_mapelement);
}
#endif
long long free_mapelement(Mmapelement* _mapelement,bool weak/*,Mallocationowner owner*/){
    long long result=M_LL_INVALID;
    if(_mapelement){
        if(amVerboseDebugging())
            output("About to free a %s map attribute!\n",(weak?"weak":"strong"));
        result=(_mapelement->_next?free_mapelement(_mapelement->_next,weak):0);
        _mapelement->_next=NULL;
        if(_mapelement->_variable){
            if(_mapelement->_variable->_name){
                if(amVerboseDebugging())
                    output("About to free %s map attribute '%s'.\n",(weak?"weak":"strong"),_mapelement->_variable->_name);
            }else
                outputWarning("Unnamed map attribute!");
            free_variable(_mapelement->_variable,weak);
            _mapelement->_variable=NULL; // MDH@11NOV2019: for safety purposes (won't wanna try it again)
        }else
            outputWarning("No map attribute to free!");
        FREE_1(_mapelement,'m'/*,owner*/);
        result+=1;
        if(amVerboseDebugging())
            outputInfo("\tMap element freed!");
    }
    return result;
}/* VALIDATED */

Mmap* owned_map(Mmap* _map,Mallocationowner owner_map){
    if(!_map)return NULL; // MDH@22OCT2020: this was missing before, which was causing crashes
    if(_map->_first)owned_mapelement(_map->_first,Msubowner(owner_map,1));
    return OWNED(_map,owner_map);
}
Mmap* disowned_map(Mmap* _map,Mallocationowner owner_map){
    if(!_map)return NULL;
    if(_map->_first)disowned_mapelement(_map->_first,owner_map);
    return DISOWNED(_map,owner_map);
}
void free_map(Mmap* _map/*,Mallocationowner owner*/){
    if(!_map)return;
    if(amVerboseDebugging())
        output("About to free a (%s) map with %llu attributes!\n",(_map->weak?"weak":"strong"),_map->numberOfElements);
    if(_map->_first){
        free_mapelement(_map->_first,_map->weak);
        _map->_first=NULL;
    }else
    if(amVerboseDebugging()) 
        outputInfo("No map attributes to free!");
    FREE_1(_map,'M'/*,owner*/);
}/* VALIDATED */

// MDH@26OCT2019: when freeing a value reference we NULL the fields just in case (TODO why?)
Mvaluereference* disowned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference){
    if(!_valuereference)return NULL;
    disowned_chars(_valuereference->_name,owner_valuereference);
    return DISOWNED(_valuereference,owner_valuereference);
}
Mvaluereference* owned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference){
    if(!_valuereference)return NULL;
    owned_chars(_valuereference->_name,owner_valuereference);
    return OWNED(_valuereference,owner_valuereference);
}
void free_valuereference(Mvaluereference* _valuereference/*,Mallocationowner owner*/){
    if(!_valuereference)return;
    if(_valuereference->_name){freeChars(_valuereference->_name/*,owner*/);_valuereference->_name=NULL;}
    /* MDH@02NOV2019: all values now 'weak' assigned i.e. no need to dereference anymore
    if(_valuereference->_value){assignValue(&_valuereference->_value,NULL);_valuereference->_value=NULL;} // get rid of the reference
    */
    if(_valuereference->_itemid){assignValue(&_valuereference->_itemid,NULL);_valuereference->_itemid=NULL;}
    FREE_1(_valuereference,'5'/*,owner*/); // MDH@19NOV2019: type changed from @ to 5 (See M.c for the allocations)
}/* VALIDATED */
// #define FREE_VALUEREFERENCE(_valuereference,owner_valuereference) free_reference(disowned_valuereference(_valuereference,owner_valuereference))

// manage a list of created values
// if we make a map out of it, we can annote the value with a name????
static Mlist* _valueList=NULL;
static unsigned long long valueCount=0; // MDH@16JUN2020: keeping track of the total number of values
// MDH@28MAY2020: in one go we can set the owner of the value list, the owner of every element in the value list, the owner of each value in every element of the value list and finally that of any data bound to the value
static Mallocationowner owner_valueList=(Mallocationowner){MODULE_ID,__LINE__,1},owner_valueListelement=(Mallocationowner){MODULE_ID,__LINE__,1,1},owner_value=(Mallocationowner){MODULE_ID,__LINE__,1,2},owner_value_data=(Mallocationowner){MODULE_ID,__LINE__,1,3};
// MDH@28MAY2020: if someone want to add something to a value (s)he should use getValueOwner() to retrieve the owner of the value
Mallocationowner getValueOwner(){return owner_value;}
Mvalue* __value(char const * const descriptor){Mallocationowner owner=getOwner(__LINE__);
    Mvalue* _value=NULL;
    if(!_valueList){
        _valueList=owned_list(__list("global value list"),owner_valueList); // MDH@19MAY2020: _valueList is global and we use 0 as function owner id (which is the rule for module global variables)
        if(!_valueList){outputBug("Failed to create the global value list.");return NULL;}
        // MDH@28MAY2020: it doesn't really matter what owner we pass to CALLOC_1 because appendedToList() will reposses it
        //                as an alternative we could call __value now to add an empty value representing undefined except that in that case it would get X as value type not U
        long long valueIndex=appendedToList(_valueList,owner_valueList,(Mvalue*)DISOWNED(CALLOC_1(sizeof(Mvalue),'U',owner),owner),M_LL_INVALID);
        if(valueIndex<=0){outputBug("Failed to store the global undefined value.");return NULL;}
        valueCount=valueIndex;
        _valueList->weak=true; // MDH@11NOV2019: from now on a weak list i.e. elements are not added using assignValue but directly
    }
    Mlistelement* _valueListelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',owner_valueListelement); // both pointers NULL
    if(_valueListelement){
        _value=(Mvalue*)CALLOC_1(sizeof(Mvalue),'X',owner_value); // MDH@07APR2020: should be 'X' not 'Y' // MDH@18MAY2020: immediately disown the Mvalue, because we return it 
        if(_value){
            // shouldn't pose a problem now...
            _valueListelement->_value=_value;
            if(_valueList->_last)_valueList->_last->_next=_valueListelement;else _valueList->_first=_valueListelement;
            _valueList->_last=_valueListelement;
            _valueList->numberOfElements++;
            // MDH@11NOV2019: by remembering the number of elements as index, removing intermediate elements will NOT prevent informing about what element was removed!!!
            _valueListelement->index=(++valueCount); // MDH@17JUN2020 replacing: _valueList->numberOfElements;
            if(descriptor)
            if(amVerboseDebugging())
            output("Descriptor of value with id #%llu: '%s'.\n",_valueListelement->index,descriptor);
        }else // couldn't get a new value, so free the value list element immediately
            FREE_DISOWNED_1(_valueListelement,'l',owner);
    }
    if(!_value)outputError("Failed to create value!"); // serious enough to report
    return _value;
}/* VALIDATED */
// MDH@01MAY2019: 'local' function for freeing a value

// MDH@28MAY2020: because we NEVER want anybody else then this module to call free_value, we made it static now
static void free_value(Mvalue* _value/*,Mallocationowner owner*/){
    //////if(amVerbose()){output("Value of type '%s'",VALUETYPENAMES[_value->type]);outputValue(" to free: '",_value,"'.\n");}
    // I do not need to free the value itself, only the pointers inside it
    if(_value){
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_TOKEN:if(_value->value._token){FREE_TOKEN(_value->value._token,owner_value_data);_value->value._token=NULL;}break;
            case VT_INTEGER:if(_value->value._integer){FREE_INTEGER(_value->value._integer,owner_value_data);_value->value._integer=NULL;}break;
            case VT_BIGINTEGER:if(_value->value._biginteger){FREE_BIGINTEGER(_value->value._biginteger,owner_value_data);_value->value._biginteger=NULL;}break;
            case VT_DECIMAL:if(_value->value._decimal){FREE_DECIMAL(_value->value._decimal,owner_value_data);_value->value._decimal=NULL;}break;
            case VT_RATIONAL:if(_value->value._rational){FREE_RATIONAL(_value->value._rational,owner_value_data);_value->value._rational=NULL;}break;
            case VT_FLOAT:if(_value->value._float){FREE_FLOAT(_value->value._float,owner_value_data);_value->value._float=NULL;}break;
            case VT_TEXT:if(_value->value._text){FREE_TEXT(_value->value._text,owner_value_data);_value->value._text=NULL;}break;
            case VT_LIST:if(_value->value._list){FREE_LIST(_value->value._list,owner_value_data);_value->value._list=NULL;}break;
            case VT_MAP:if(_value->value._map){FREE_MAP(_value->value._map,owner_value_data);_value->value._map=NULL;}break;
            case VT_REFERENCE:if(_value->value._reference){FREE_REFERENCE(_value->value._reference,owner_value_data);_value->value._reference=NULL;}break; // MDH@04NOV2019: decrement the reference count to the variable
            case VT_FUNCTION:if(_value->value._function){FREE_FUNCTION(_value->value._function,owner_value_data);_value->value._function=NULL;}break;
            case VT_ENVIRONMENT:if(_value->value._environment){FREE_ENVIRONMENT(_value->value._environment,owner_value_data);_value->value._environment=NULL;}break;
            case VT_FILE:if(_value->value._file){FREE_FILE(_value->value._file,owner_value_data);_value->value._file=NULL;}break; // MDH@28SEP2020
            //case VT_USERFUNCTION:if(_value->value._userfunction)free_userfunction(_value->value._userfunction);break;
        }
        FREE_DISOWNED_1(_value,'X',owner_value);
        if(amVerboseDebugging())output("\tValue of type '%s' freed.\n",VALUETYPENAMES[_value->type]);
    }else
        outputBug("No value to free!");
}/* VALIDATED */

extern const char* const VALUETYPENAMES[];

// can be asked to remove unused values
// TODO check whether it functions correctly (think so though)
size_t getNumberOfRemovedValues(bool showInfo){Mallocationowner owner=getOwner(__LINE__);
    unsigned long long tofree=0,removed=0;
    if(_valueList){
        if(showInfo)output("Garbage collecting unused values.\n");
        Mallocationowner owner_valueListelement=Msubowner(owner_valueList,1);
        Mallocationowner owner_value=Msubowner(owner_valueList,2);
        Mlistelement* _valueListelement=_valueList->_first;
        if(showInfo)output("Number of values to check: %llu.\n",_valueList->numberOfElements); // MDH@11NOV2019: no longer the actual number of elements to check
        unsigned long long checked=0;
        while(_valueListelement){
            checked++;
            if(_valueListelement->_value){
                if(showInfo){
                    output("Checking value #%llu with id %llu",checked,_valueListelement->index);
                    outputValue(": '",_valueListelement->_value,"'.\n");
                }
                // if(showInfo)outputInfo("\tChecking the count!");
                if(_valueListelement->_value->count==0){ // unused
                    if(showInfo)output("\tFreeing unused value #%llu of type '%s'.\n",checked,VALUETYPENAMES[_valueListelement->_value->type]);
                    free_value(_valueListelement->_value);_valueListelement->_value=NULL; // essential to NULL so removing the value list elements below becomes possible
                    tofree++;
                }else
                if(showInfo)outputInfo("\tStill in use!");
            }else
                output("%sNo value stored in value #%llu.\n",M_BUG_PREFIX,checked); // technically a bug not an error
            _valueListelement=_valueListelement->_next;
        }
        if(showInfo)output("Number of values checked: %llu.\nNumber of value list elements to free: %llu.\n",checked,tofree);
        // the list is now intact, are we going to correct the links??????
        if(tofree>0){ // some values were freed
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
                    /* MDH@11NOV2019: let's decide NOT to decrement numberOfElements meaning that we NOW use numberOfElements to always have a unique index for every value ever added to it!!!
                    if(_valueList->numberOfElements>0)_valueList->numberOfElements--;else output("BUG: Trying to free a value list element that is not counted!");  // one less element in the list!!!
                    */
                    FREE_DISOWNED_1(_valueListelement,'l',owner_valueListelement);
                }
                // next to check!!!
                _valueListelement=_nextValueListelement;
            }
            if(showInfo)output("Actual number of values freed: %llu.\n",removed);
            // update the first and last in the list (could both be NULL!!!)
            _valueList->_first=_firstValueListelement;
            _valueList->_last=_lastValueListelement;
        }
    }
    if(tofree>0){
        if(tofree>removed)
            output("%sFailed to free %llu unused value list elements.\n",M_WARNING_PREFIX,(tofree-removed));
        else 
        if(showInfo)
            outputInfo("All unused value list elements freed!");
    }
    return removed;
}/* VALIDATED */

unsigned long long getNumberOfValues(){
    return (_valueList?_valueList->numberOfElements:0);
}/* VALIDATED */

bool decrementReferenceCount(Mvalue * const _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->count>0){(_value->count)--;return true;}
        Mstring* _valueText=owned_string(_getValueText(_value,false),owner);
        output("%sReference count of '%s' of type '%c' already zero.\n",M_BUG_PREFIX,string(_valueText),MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
        FREE_STRING(_valueText,owner);
    }else
    if(amVerbose())
        outputInfo("No value to decrement the reference count of.");
    return false;
}/* VALIDATED */
bool incrementReferenceCount(Mvalue* _value){
    if(_value){(_value->count)++;return true;}
    if(amVerbose())
        outputInfo("No value to increment the reference count of.");
    return false;
}/* VALIDATED */

// interface functions that use the above functions
// wrapping the different value type instances
// MDH@23MAY2020: why not return a global representing an undefined value (might be the first value in _valuelist)
Mvalue* _getUndefinedValue(){
    return (_valueList?DISOWNED(_valueList->_first->_value,owner_valueList):NULL);
    // MDH@23MAY2020 replacing: return (Mvalue*)CALLOC_1(sizeof(Mvalue),'U',-getOwner(__LINE__));
}/* VALIDATED */
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
    if(!_userfunction)return NULL;
    Mvalue* _userfunctionValue=__value();
    if(_userfunctionValue){_userfunctionValue->type=VT_USERFUNCTION;_userfunctionValue->value._userfunction=_userfunction;}else if(freeonfailure)free_userfunction(_userfunction);
    return _userfunctionValue;
}// VALIDATED 
*/
// MDH@04NOV2019: no matter where the variable originates we can store it so it can be used elsewhere
Mreference* owned_reference(Mreference* _reference,Mallocationowner owner_reference){return DISOWNED(_reference,owner_reference);}
Mreference* disowned_reference(Mreference* _reference,Mallocationowner owner_reference){return OWNED(_reference,owner_reference);}
void free_reference(Mreference* reference/*,Mallocationowner owner_reference*/){
    if(!reference)return;
    if(reference->variable){
        if(reference->variable->referencecount==0)
            outputBug("Count of referenced variable already zero.");
        else
            reference->variable->referencecount--;
    }
    FREE_1(reference,'Q'/*,owner_reference*/);
}
Mreference* _getReference(Mvariable* variable){Mallocationowner owner=getOwner(__LINE__);
    // MDH@11MAR2020: variable can now be NULL
    Mreference* _reference=CALLOC_1(sizeof(Mreference),'Q',owner);
    if(_reference){_reference->variable=SUBOWNED(variable,1);if(variable)_reference->referenceindex=(++variable->referencecount);} // MDH@11MAR2020: if variable is undefined, no reference count we can increment and assign
    return DISOWNED(_reference,owner);
}

Mvalue* _getValueOfReference(Mreference* _reference/*,Mallocationowner owner_reference*/){
    // MDH@19MAY2020: should we check whether _reference is ownable???????
    if(!_reference){outputWarning("No reference to wrap.");return NULL;}
    Mvalue* _referenceValue=__value("reference");
    if(_referenceValue){
        _referenceValue->type=VT_REFERENCE;
        _referenceValue->value._reference=(Misdisowned(_reference)?owned_reference(_reference,owner_value_data):_reference);
    }else
    if(Misdisowned(_reference))free_reference(_reference);
    return _referenceValue;
}
// MDH@26MAY2020: new contract, if NULL is returned _decimal is NOT bound and should be freed if desirable...
Mvalue* _getValueOfDecimal(Mdecimal* _decimal/*,Mallocationowner owner_decimal*/){
    if(!_decimal)return NULL;
    Mvalue* _decimalValue=__value("decimal");
    if(_decimalValue){
        // outputDecimal("Wrapping decimal '",_decimal,"'.\n"); // DEBUG
        _decimalValue->type=VT_DECIMAL;
        _decimalValue->value._decimal=(Misdisowned(_decimal)?owned_decimal(_decimal,owner_value_data):_decimal);
    }else
    if(Misdisowned(_decimal))free_decimal(_decimal);
    return _decimalValue;
}/* VALIDATED */

Mvalue* _getValueOfBiginteger(Mbiginteger* _biginteger/*,Mallocationowner owner_biginteger*/){
    if(!_biginteger)return NULL;
    Mvalue* _bigintegerValue=__value("biginteger");
    if(_bigintegerValue){
        _bigintegerValue->type=VT_BIGINTEGER;
        _bigintegerValue->value._biginteger=(Misdisowned(_biginteger)?owned_biginteger(_biginteger,owner_value_data):_biginteger);
    }else
    if(Misdisowned(_biginteger))free_biginteger(_biginteger);
    return _bigintegerValue;
}/* VALIDATED */

Mvalue* _getFloatValue(long double ld){
    if(amVerboseDebugging())output("Wrapping long double (float) '%.*Lf'.\n",DBL_DIG,ld);
    Mvalue* _floatValue=__value("long double");
    if(!_floatValue)return NULL;
    _floatValue->type=VT_FLOAT;
    _floatValue->value._float=owned_float(_getFloat(ld),owner_value_data);
    return _floatValue;
}/* VALIDATED */
Mvalue* _getIntegerValue(long long ll){
    if(amVerboseDebugging())output("Wrapping integer '%lld'.\n",ll);
    Mvalue* _integerValue=__value("integer");
    if(!_integerValue)return NULL;
    _integerValue->type=VT_INTEGER;
    _integerValue->value._integer=owned_integer(_getInteger(ll),owner_value_data);
    return _integerValue;
}/* VALIDATED */
// MDH@25MAY2020: s is a constant character array that does not need change ownership (because it is supposed to be owned elsewhere or not owned)
Mvalue* _getTextValue(char const * const s/*,bool freeonfailure*/){
    if(!s)return NULL; // when no input, no go
    // output("Retrieving the text value of '%s' of length %zd.\n",s,strlen(s)); // DEBUG
    Mvalue* _textValue=__value("text"); // the result, when NULL check freeonfailure
    if(!_textValue)return NULL;
    _textValue->type=VT_TEXT;
    _textValue->value._text=owned_text(_getText(s),owner_value_data);
    return _textValue;
}/* VALIDATED */
Mvalue* _getCharTextValue(char _c){
    Mvalue* _textValue=__value("char");
    if(!_textValue)return NULL;
    _textValue->type=VT_TEXT;
    _textValue->value._text=owned_text(_getCharText(_c),owner_value_data);
    return _textValue;
}/* VALIDATED */
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype,bool weak,char const * const source){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=owned_list(__list(source?source:"_getListValue"),owner);
    if(!_list){
        // if(amVerboseDebugging())
            outputError("Failed to create a list.\n");
        return NULL;
    }
    Mvalue* _listValue=__value(weak?"weak list":"strong list");
    if(!_listValue){
        FREE_LIST(_list,owner);
        return NULL;
    }
    _list->weak=weak;
    _list->valuetype=listValuetype; // register what type of elements this list should have    
    _listValue->type=VT_LIST;
    _listValue->value._list=owned_list(disowned_list(_list,owner),owner_value_data);
    return _listValue;
}/* VALIDATED */

Mlist* _getListIndices(Mlist const * const list){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=owned_list(_getListOfType(VT_INTEGER),owner);
    if(!_list)return NULL;
    Mlistelement* listelement=list->_first;
    while(listelement){
        Mvalue* indexValue=_getIntegerValue(listelement->index);
        if(appendedToList(_list,owner,indexValue,M_LL_INVALID)<=0){
            // NO need to free indexValue because it is a Value!!!
            outputError("Failed to append list element index");break;
        }
        listelement=listelement->_next;
    }
    return disowned_list(_list,owner);
}/* VALIDATED */

// MDH@19JUN2020: if something goes wrong reversing the list NULL is returned!!!
Mlist* _getReversedList(Mlist const * const list){if(!list)return NULL;Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=owned_list(_getListOfType(list->valuetype),owner); // make a list of the same type as the list argument
    if(!_list)return NULL;
    Mlistelement* listelement=list->_first;
    while(listelement){
        if(appendedToList(_list,owner,listelement->_value,0)<=0){FREE_LIST(_list,owner);return NULL;}
        listelement=listelement->_next;
    }
    return disowned_list(_list,owner);
}

// MDH@30MAR2020: the general idea of flattening a list is that all elements of the list are not lists anymore
//                let's assume that NULL means something went wrong, caller should take care of situation where value is NULL unless ?TODO? we allow putting a NULL value in the list
//                reversed tells _getFlattenedList to prepend instead of append
// MDH@06APR2020: passing in a flatten level that is decremented on each element and when it reaches 0 no flattening has to occur
Mlist* _getFlattenedList(Mvalue const * const value,unsigned int flattenLevel,bool reversed){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=NULL;
    if(value){
        // if(amVerboseDebugging())
            outputValue("Flattening '",value,"'.\n");
        _list=owned_list(_getListOfType(VT_UNDEFINED),owner);
        if(_list){
            bool success=true;
            if(value->type==VT_LIST){
                // we have to flatten all elements in the list obviously before we can actually add them to _list
                // if the list is empty we do NOT consider this an error, we simply do nothing
                Mlistelement* valueListelement=(value->value._list?value->value._list->_first:NULL);
                while(valueListelement){
                    // if there is a value associated _getFlattenedList should NOT return NULL (so NULL would represent an error)
                    if(valueListelement->_value){
                        // MDH@31MAR2020: a little more effort to check if the value is a list in which case flattening is required, otherwise it is not
                        // MDH@06APR2020: if flattenLevel>0 we need to add all list elements individually instead of all together
                        if(flattenLevel>0&&valueListelement->_value->type==VT_LIST){
                            Mlist* _subList=owned_list(_getFlattenedList(valueListelement->_value,(flattenLevel>0?flattenLevel-1:0),reversed),owner);
                            if(_subList){
                                Mlistelement* valueSubListelement=_subList->_first;
                                while(valueSubListelement){
                                    // if supposed to return the prepend instead of append pass in 0 instead of M_LL_INVALID
                                    if(valueSubListelement->_value)if(appendedToList(_list,owner,valueSubListelement->_value,(reversed?0:M_LL_INVALID))<=0){success=false;break;}
                                    valueSubListelement=valueSubListelement->_next;
                                }
                                // ALWAYS free _sublist
                                FREE_LIST(_subList,owner);
                            }else
                                success=false;
                        }else
                        if(appendedToList(_list,owner,valueListelement->_value,(reversed?0:M_LL_INVALID))<=0)success=false;
                        if(!success)break; // do NOT continue with appending when failing to do so
                    }
                    valueListelement=valueListelement->_next;
                }
            }else // a single element to add to the list
            if(appendedToList(_list,owner,value,M_LL_INVALID)<=0)success=false;
            if(!success){FREE_LIST(_list,owner);_list=NULL;} // on failure release the list
        }    
    }
    // if(amVerboseDebugging())
    {
        if(_list){if(flattenLevel>0)outputList("Flattened to '",_list,"'.\n");else outputList("Converted to '",_list,"'.\n");}
    }
    return disowned_list(_list,owner);
}

Mvalue* getFirstScalarValue(Mvalue* value){
    if(!value)return NULL;
    if(value->type==VT_MAP)return NULL; // can't go into a map
    if(value->type!=VT_LIST)return value;
    Mlistelement* listelement=(value->value._list?value->value._list->_first:NULL);
    while(listelement){
        Mvalue* firstScalarValue=getFirstScalarValue(listelement->_value);
        if(firstScalarValue)return firstScalarValue; // if this element is a scalar (i.e. defined and not a map or a list)
        listelement=listelement->_next;
    }
    return NULL;
}

Mlist* _getMapAttributes(Mmap const * const map){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _list=owned_list(_getListOfType(VT_TEXT),owner);
    if(!_list)return NULL;
    Mmapelement* mapelement=map->_first;
    while(mapelement){
        // I guess we'll have to duplicate the attribute name because it will be wrapped inside a Value
        // this is a bit of an issue because typically text should be enquoted
        Mstring* _attributeName=owned_string(_getString("'"),owner);
        if(!_attributeName){outputError("Failed to duplicate a map attribute name");break;}
        string_append(_attributeName,mapelement->_variable->_name->chars); // MDH@17APR2020: char* _name replaced by Mchars* _name // append the attribute name
        Mvalue* attributeValue=_getTextValue(string(_attributeName));
        FREE_STRING(_attributeName,owner);
        if(!attributeValue){outputError("Failed to store a map attribute name");break;}
        if(appendedToList(_list,owner,attributeValue,M_LL_INVALID)<=0){
            // NO need to free indexValue because it is a Value!!!
            outputError("Failed to append map attribute name");break;
        }
        mapelement=mapelement->_next;
    }
    return disowned_list(_list,owner);
}

// MDH@23MAY2020: although a value is (weakly but permanently) stored in _valuelist we can set its owner to the function that created it
Mvalue* _getMapValue(Mvaluetype mapValuetype,bool weak){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
    if(!_map)return NULL;
    Mvalue* _mapValue=__value(weak?"weak map":"strong map");
    if(_mapValue){
        _map->weak=weak;
        _map->valuetype=mapValuetype;
        _mapValue->type=VT_MAP;
        _mapValue->value._map=owned_map(disowned_map(_map,owner),owner_value_data);
    }else
        FREE_MAP(_map,owner);
    return _mapValue;
}/* VALIDATED */

// some other wrappers
// if something is wrapped in a value it should get the ownership of the value
// the question now is what owner_integer actually means
// the point is that we cannot free an integer or obtain ownership if owner_integer is not correct
// however, it should be possibly to bind _integer to the value that is created in which case we should be able to obtain ownership
// meaning that whatever we receive should be a disowned integer unless we do a super own
Mvalue* _getValueOfInteger(Minteger* _integer/*,Mallocationowner owner_integer*/){
    if(!_integer)return NULL;
    Mvalue* _value=__value("integer"); // make the value create have the same owner as the integer
    if(_value)
    {_value->value._integer=(Misdisowned(_integer)?owned_integer(_integer,owner_value_data):_integer);_value->type=VT_INTEGER;}
    else 
    if(Misdisowned(_integer))
        free_integer(_integer);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfFloat(Mfloat* _float/*,Mallocationowner owner_float*/){
    if(!_float)return NULL;
    Mvalue* _value=__value("float");
    if(_value)
    {_value->value._float=(Misdisowned(_float)?owned_float(_float,owner_value_data):_float);_value->type=VT_FLOAT;}
    else 
    if(Misdisowned(_float))
        free_float(_float);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfMap(Mmap* _map/*,Mallocationowner owner_map*/){
    if(!_map)return NULL;
    Mvalue* _value=__value("map");
    if(_value)
    {_value->value._map=(Misdisowned(_map)?owned_map(_map,owner_value_data):_map);_value->type=VT_MAP;}
    else 
    if(Misdisowned(_map))free_map(_map);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfToken(Mtoken* _token/*,Mallocationowner owner_token*/){
    if(!_token)return NULL;
    Mvalue* _value=__value("token");
    if(_value)
    {_value->value._token=(Misdisowned(_token)?owned_token(_token,owner_value_data):_token);_value->type=VT_TOKEN;}
    else 
    if(Misdisowned(_token))free_token(_token);
    return _value;
}/* VALIDATED */

/* TODO move elsewhere
Mvalue* _getTokenValue(Mtoken* _token,bool freeonfailure){
	if(!_token)return NULL;
	Mvalue* _tokenValue=__value();
	if(_tokenValue){_tokenValue->type=VT_TOKEN;_tokenValue->value._token=_token;}else if(freeonfailure)FREE_TOKEN(_token);
	return _tokenValue;
}*/

// MDH@09JUN2020: interface between _getVariable (now requiring a Mchars name) and all that still use a char* thing
static Mvariable* _getVariableWithName(char const * const name,Mvaluetype valuetype,bool immutable,Mallocationowner owner_variable){
    // MDH@21OCT2020: A HA we should allow the name of a variable to be empty (as in maps)
    if(!name/*||strlen(name)==0*/)return NULL;//Mallocationowner owner=getOwner(__LINE__);
    return owned_variable(_getVariable(_getChars(name),valuetype,immutable),owner_variable);
}
// helper function to create a parameter map with a single value
// the following is a nuisance
Mmap* _getFloatMap(char* name,Mvalue* _floatValue){Mallocationowner owner=getOwner(__LINE__);
    if(name&&_floatValue){
        Mvariable* _realVariable=_getVariableWithName(name,VT_FLOAT,true,Msubowner(owner,2)); // MDH@11JUN2020: _getChars(name) returns a disowned pointer that we need in _getVariable()
        if(_realVariable){
            Mmapelement* _mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            if(_mapelement){
                Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    assignValue(&_realVariable->_value,_floatValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
                    _mapelement->_variable=_realVariable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    if(amVerboseDebugging())
                        outputInfo("Returning the single float map!");
                    return disowned_map(_map,owner);
                }
                outputError("Failed to create the float variable map");
                FREE_MAPELEMENT(_mapelement,false,owner);
            }else
                outputError("Failed to create the float variable map element");
            FREE_VARIABLE(_realVariable,false,owner);
        }else
            outputError("Failed to create the float variable name");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getMap(char* name){if(!name)return NULL;Mallocationowner owner=getOwner(__LINE__);
    Mvariable* _variable=_getVariableWithName(name,VT_UNDEFINED,true,Msubowner(owner,2));
    if(_variable){
        Mmapelement* _mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
        if(_mapelement){
            Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
            if(_map){
                _map->numberOfElements=1;
                _mapelement->_variable=_variable; // TODO we can do this better given the new disowned_map usage
                _map->_first=_mapelement;
                _map->_last=_mapelement;
                return disowned_map(_map,owner);
            }
            FREE_MAPELEMENT(_mapelement,false,owner); // MDH@11NOV2019: no need to call free_mapelement() 
        }
        FREE_VARIABLE(_variable,false,owner);
    }else
        output("%sFailed to create map '%s'.\n",M_ERROR_PREFIX,name);
    return NULL;
}/* VALIDATED */

Mmap* _getMapCopy(Mmap const * const map){Mallocationowner owner=getOwner(__LINE__); // creates a 'deep' copy
    if(map){
        Mmap* _map=owned_map(_getMapOfType(map->valuetype),owner);
        if(_map){
            Mvariable *mapelementVariable,*_mapelementVariable=NULL;
            Mmapelement *mapelement=map->_first,*_mapelement=NULL;
            while(mapelement){
                mapelementVariable=mapelement->_variable;
                if(mapelementVariable){
                    _mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1)); // new map element to hold a copy
                    if(_mapelement){
                        // create a variable with the same name and value as the variable in mapelement
                        // MDH@12MAR2020 OOPS: why would we make the copy ALWAYS immutable: replacing true by mapelementVariable->immutable
                        _mapelement->_variable=_getVariableWithName(mapelementVariable->_name->chars,mapelementVariable->valuetype,mapelementVariable->immutable/*true*/,Msubowner(owner,2));
                        if(_mapelement->_variable){
                            assignValue(&_mapelement->_variable->_value,mapelementVariable->_value); // 'copy' the value over
                            if(_map->_last)_map->_last->_next=_mapelement; // make the current last point to the new last
                            _map->_last=_mapelement; // replace current last by the new last
                            if(!_map->_first)_map->_first=_map->_last; // initialize first if necessary
                            _map->numberOfElements++; // count one more
                        }else
                            output("%sFailed to copy the map attribute name '%s'.\n",M_ERROR_PREFIX,mapelementVariable->_name->chars);
                    }else
                        output("%sFailed to copy map attribute '%s'.\n",M_ERROR_PREFIX,mapelementVariable->_name->chars);
                }
                mapelement=mapelement->_next;
            }
            return disowned_map(_map,owner);
        }else
            outputError("Failed to create a map");
    }
    return NULL;
}
Mlist* _getListCopy(Mlist const * const list){Mallocationowner owner=getOwner(__LINE__); // creates a 'deep' copy
    if(list){
        Mlist* _list=owned_list(_getListOfType(list->valuetype),owner);
        if(_list){
            Mlistelement *listelement=list->_first,*_listelement=NULL;
            while(listelement){
                _listelement=CALLOC_1(sizeof(Mlistelement),'l',Msubowner(owner,1));
                if(_listelement){
                    // 'copy' the value over, here we have the same problem as with copying any other value: if the value to copy is composite (a list or a map) we should copy by value i.e. point to a new map or list and not to the original
                    // technically we could let assignValue() take care of that 
                    assignValue(&_listelement->_value,listelement->_value); // no need to 'copy' the value itself, we only need to make another (strong) reference to that value
                    _listelement->index=listelement->index;
                    if(_list->_last)_list->_last->_next=_listelement;
                    _list->_last=_listelement;
                    if(!_list->_first)_list->_first=_list->_last;
                    _list->numberOfElements++;
                }else 
                    outputError("Failed to copy a list element");
                listelement=listelement->_next;
            }
            return disowned_list(_list,owner);
        }else
            outputError("Failed to create a list");
    }
    return NULL;
}
// MDH@24MAY2020: more convenient to be able to make any map with a certain number of arguments with names and types
static Mmap* _getOneArgumentMap(char* name,Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
    // NOTE wait with filling the single integer value map until we have all the ingredients
    if(name&&strlen(name)>0){
        Mvariable* _variable=_getVariableWithName(name,valuetype,true,Msubowner(owner,2));
        if(_variable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    _mapelement->_variable=_variable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amVerboseDebugging())outputInfo("Returning the single integer map!");
                    return disowned_map(_map,owner);
                }
                outputError("Failed to create the one variable map");
                FREE_MAPELEMENT(_mapelement,false,owner);
            }else
                outputError("Failed to create the one variable map element");
            FREE_VARIABLE(_variable,false,owner);
        }else
            outputError("Failed to create the variable of the one variable map");
    }
    return NULL;
}
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue){Mallocationowner owner=getOwner(__LINE__);
    // NOTE wait with filling the single integer value map until we have all the ingredients
    if(!name||strlen(name)==0)return NULL;
    Mmap* _integerMap=owned_map(_getOneArgumentMap(name,VT_INTEGER),owner);
    if(_integerMap&&_integerValue){
        assignValue(&_integerMap->_first->_variable->_value,_integerValue);
        return disowned_map(_integerMap,owner);
    }
    if(_integerMap)FREE_MAP(_integerMap,owner);
    return NULL;
}/* VALIDATED */
Mmap* _getListMap(char* name,Mvalue* _listValue){Mallocationowner owner=getOwner(__LINE__);
    if(!name||strlen(name)==0)return NULL;
    if(!_listValue){
        // if(amVerboseDebugging())
            output("Unable to create list map: no list value to put in list.\n");
        return NULL;
    }
    Mmap* _listMap=owned_map(_getOneArgumentMap(name,VT_LIST),owner);
    if(!_listMap){
        // if(amVerboseDebugging())
           output("%sFailed to create a one argument list map.\n",M_ERROR_PREFIX);
        return NULL;
    }
    assignValue(&_listMap->_first->_variable->_value,_listValue);
    return disowned_map(_listMap,owner);
}/* VALIDATED */

static Mmap* _getTwoArgumentMap(char* name1,char* name2,Mvaluetype valuetype1,Mvaluetype valuetype2){Mallocationowner owner=getOwner(__LINE__);
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    _mapelement1->_variable=_getVariableWithName(name1,valuetype1,true,Msubowner(owner,2));
                    _mapelement2->_variable=_getVariableWithName(name2,VT_INTEGER,true,Msubowner(owner,2));
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return disowned_map(_map,owner);
                    }
                    outputError("Failed to create both map element variables");
                    FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }else
                    outputError("Failed to create a map");
            }else
                outputError("Failed to create both integer map elements");
            // either map element might have been created and we need to release them
            FREE_MAPELEMENT(_mapelement1,false,owner);
            FREE_MAPELEMENT(_mapelement2,false,owner);
        }else
            output("%sTwo argument map element names '%s' and '%s' undefined or the same.\n",M_ERROR_PREFIX,name1,name2);
    }else
        outputError("Not both two map element names defined");
    return NULL;    
}
Mmap* _getIntegerBooleanMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_INTEGER,VT_INTEGER);}/* VALIDATED */
Mmap* _getStringStringMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_TEXT,VT_TEXT);}/* VALIDATED */
Mmap* _getFloatFloatMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_FLOAT,VT_FLOAT);}/* VALIDATED */
Mmap* _getMapTokenMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_MAP,VT_TOKEN);}/* VALIDATED */
Mmap* _getTokenTokenMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_TOKEN,VT_TOKEN);}/* VALIDATED */

Mmap* _getThreeArgumentMap(char* name1,char* name2,char* name3,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3){Mallocationowner owner=getOwner(__LINE__);
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    _mapelement1->_variable=_getVariableWithName(name1,VT_TEXT,true,Msubowner(owner,2));
                    _mapelement2->_variable=_getVariableWithName(name2,VT_MAP,true,Msubowner(owner,2));
                    _mapelement3->_variable=_getVariableWithName(name3,VT_TOKEN,true,Msubowner(owner,2));
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _map->_last=_mapelement3;
                        _map->numberOfElements=3;
                        // output("Returning disowned map of '%s', '%s' and '%s'.\n",name1,name2,name3); // DEBUG
                        return disowned_map(_map,owner);
                    }
                    FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            FREE_MAPELEMENT(_mapelement1,false,owner);
            FREE_MAPELEMENT(_mapelement2,false,owner);
            FREE_MAPELEMENT(_mapelement3,false,owner);
        }
    }
    return NULL;
}
Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_TEXT,VT_MAP,VT_TOKEN);}/* VALIDATED */
Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_UNDEFINED,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_INTEGER,VT_INTEGER,VT_INTEGER);}/* VALIDATED */
Mmap* _getListValueIntegerMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_LIST,VT_UNDEFINED,VT_INTEGER);}/* VALIDATED */

Mmap* _getFourArgumentMap(char* name1,char* name2,char* name3,char *name4,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3,Mvaluetype valuetype4){Mallocationowner owner=getOwner(__LINE__);
    if(name1&&name2&&name3&&name4){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
            strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name3,name4)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            Mmapelement* _mapelement4=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
            if(_mapelement1&&_mapelement2&&_mapelement3&&_mapelement4){
                Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    _mapelement1->_variable=_getVariableWithName(name1,valuetype1,true,Msubowner(owner,2));
                    _mapelement2->_variable=_getVariableWithName(name2,VT_TOKEN,true,Msubowner(owner,2));
                    _mapelement3->_variable=_getVariableWithName(name3,VT_TOKEN,true,Msubowner(owner,2));
                    _mapelement4->_variable=_getVariableWithName(name4,VT_TOKEN,true,Msubowner(owner,2));
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable&&_mapelement4->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _mapelement3->_next=_mapelement4;
                        _map->_last=_mapelement4;
                        _map->numberOfElements=4;
                        return disowned_map(_map,owner);
                    }
                    FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            FREE_MAPELEMENT(_mapelement1,false,owner);
            FREE_MAPELEMENT(_mapelement2,false,owner);
            FREE_MAPELEMENT(_mapelement3,false,owner);
            FREE_MAPELEMENT(_mapelement4,false,owner);
        }
    }
    return NULL;
}
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4){return _getFourArgumentMap(name1,name2,name3,name4,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN);}/* VALIDATED */

Mmap* _getFiveArgumentMap(char* name1,char* name2,char* name3,char *name4,char *name5,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3,Mvaluetype valuetype4,Mvaluetype valuetype5){Mallocationowner owner=getOwner(__LINE__);
    if(name1&&name2&&name3&&name4&&name5){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
            strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name1,name5)&&
            strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name2,name5)&&
            strcmp(name3,name4)&&strcmp(name3,name5)&&
            strcmp(name4,name5)){
            Mallocationowner owner_mapelement=Msubowner(owner,1);
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
            Mmapelement* _mapelement4=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
            Mmapelement* _mapelement5=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
            if(_mapelement1&&_mapelement2&&_mapelement3&&_mapelement4&&_mapelement5){
                Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
                if(_map){
                    Mallocationowner owner_variable=Msubowner(owner,2);
                    _mapelement1->_variable=_getVariableWithName(name1,VT_TOKEN,true,owner_variable);
                    _mapelement2->_variable=_getVariableWithName(name2,VT_TOKEN,true,owner_variable);
                    _mapelement3->_variable=_getVariableWithName(name3,VT_TOKEN,true,owner_variable);
                    _mapelement4->_variable=_getVariableWithName(name4,VT_TOKEN,true,owner_variable);
                    _mapelement5->_variable=_getVariableWithName(name5,VT_TOKEN,true,owner_variable);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable&&_mapelement4->_variable&&_mapelement5->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _mapelement3->_next=_mapelement4;
                        _mapelement4->_next=_mapelement5;
                        _map->_last=_mapelement5;
                        _map->numberOfElements=5;
                        return disowned_map(_map,owner);
                    }
                    FREE_MAP(_map,owner); // failed to create the five map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            FREE_MAPELEMENT(_mapelement1,false,owner);
            FREE_MAPELEMENT(_mapelement2,false,owner);
            FREE_MAPELEMENT(_mapelement3,false,owner);
            FREE_MAPELEMENT(_mapelement4,false,owner);
            FREE_MAPELEMENT(_mapelement5,false,owner);
        }
    }
    return NULL;
}
Mmap* _getTokenTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4,char *name5){return _getFiveArgumentMap(name1,name2,name3,name4,name5,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
// end helper functions 

// LIST STUFF
Mvalue* _getValueOfList(Mlist* _list/*,Mallocationowner owner_list*/){//Mallocationowner owner=getOwner(__LINE__);
    if(!_list)return NULL;
    bool disowned_list=Misdisowned(_list);
    if(amVerbose())
        output("Wrapping a %s list.\n",(disowned_list?"disowned":"owned"));
    Mvalue* _value=__value(_list->weak?"weak list":"strong list");
    if(!_value){
        if(Misdisowned(_list))free_list(_list);
        return NULL;
    }
    _value->value._list=(disowned_list?owned_list(_list,owner_value_data):_list); // MDH@09JUN2020: _value is to take over ownership of _list
    _value->type=VT_LIST;
    if(amVerbose())
        output("%s list wrapped.\n",(disowned_list?"disowned":"owned"));
    return _value;
}/* VALIDATED */

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
                    if(_listelement->index<=listelementindex)output("%sList element index (%lld) below the expected list element index (%lld).\n",M_ERROR_PREFIX,_listelement->index,listelementindex);
                    listelementindex=_listelement->index;
                    if(!_listelement->_next){
                        if(_list->_last!=_listelement)outputError("Registered last list element not equal to the actual last list element");
                        break;                        
                    }
                    output("List element with index %llu OK.\n",_listelement->index);
                    _listelement=_listelement->_next;
                }
                if(l>0)outputError("Less elements in list than accounted for");else 
                if(l<0)output("%s%lld more elements in list than counted.\n",M_ERROR_PREFIX,(-l));
            }else
                output("%sList with %lld elements does not have a first element!\n",M_ERROR_PREFIX,l);
        }else{
            if(_list->_first)outputError("Empty list with first element");
            if(_list->_last)outputError("Empty list with last element");
        }
    }
}/* VALIDATED */
// instead of returning a boolean we could return the assigned index (0 on failure)
// MDH@02JUN2019: check (and correct) prepending
// MDH@17OCT2019: passing in 0 should NOT do appending but prepending (use index len(l)+1 for appending!!!!!!)
//                OOPS we used to use 0 to force an append, so we now use M_LL_INVALID to force that!!!!
// MDH@05NOV2019: originally we returned 0 on failure and an unsigned result (leaving only 0 as possible return value), but by allowing to return negative values as well we can distinguish different types of failures
//                e.g. returning a negative value if we fail to insert the given element
//                NOTE: index==0 means prepend, index==M_LL_INVALID means append, index<0 means insert from the back (i.e. relative to the maximum index)
//                TODO: determine the situations where we want to return either M_LL_INVALID or 0 or a negative value to indicate failure
//                DOING: I suppose returning M_LL_INVALID when there's something wrong with the input, 0 when unable to comply somehow (e.g. when the list is immutable)
/*unsigned*/ long long appendedToList(Mlist * const _list,Mallocationowner owner_list,Mvalue const * const _value,long long index){Mallocationowner owner=getOwner(__LINE__);
    if(!_list){outputError("No list to append to");return M_LL_INVALID;} // MDH@18OCT2019: let's allow NULLing list elements (i.e. accepting _value to be NULL)
    if(_list->immutable){outputError("Unable to change the list: it is immutable");return 0;}
    // MDH@05NOV2019: let's always allow adding NULL or undefined values to a list
    if(_value&&_value->type!=VT_UNDEFINED&&_list->valuetype!=VT_UNDEFINED)
    if(_value->type!=_list->valuetype)
    {output("%s",M_ERROR_PREFIX);outputValue("Unable to add '",_value,"' to a list: it is of the wrong type.\n");return 0;}
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    // MDH@17OCT2019: index 0 now does not indicate to append to the end anymore but now indicates that the given value should be prepended!!!!
    // MDH@05NOV2019: if supposed to append the value, and the current last index is already equal to the maximum possible index, we consider the list to be full
    if(index==M_LL_INVALID){if(lastindex==M_LL_MAX){outputError("Unable to append to a list: it is full");return 0;};index=lastindex+1;} // MDH@17OCT2019: we need to be able to append as well (can't use 0 anymore!!!!)
    if(index<0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    // MDH@17OCT2019: a negative index might still end up with index 0, this happens with -len(x)-1, ok, for now just accept this when it happens
    if(index<0){output("%sIndex %lld of (new) list element too small.\n",M_ERROR_PREFIX,index);return M_LL_INVALID;} // MDH@17OCT2019: can't return negative value!!! // MDH@05NOV2019: to indicate invalid input
    // if(amVerboseDebugging()){outputValue("Adding '",_value,"' to a list");output(" at index %lld.\n",index);}
    // MDH@23MAY2019: let's allow inserting or replacing as well
    // determine _listelement as element to host the value, store the successor in _nextlistelement
    Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index>0&&index<=lastindex?_list->_first:NULL);
    if(_listelement){ // we are not appending and we have a first element, so this might be an insert or replace
        // NOTE testing _listelement is just a fail-safe as that should never happen
        while(index>_listelement->index){
            _prevListelement=_listelement;
            if(!_listelement->_next){output("%sIndex (%llu) ",M_BUG_PREFIX,_listelement->index);outputValue("of existing list element '",_listelement->_value,"' probably out of order.\n");return M_LL_INVALID;}
            _listelement=_listelement->_next;
        }
        // if we're going to insert there will be a successor
        if(_listelement->index!=index){_nextListelement=(_prevListelement?_prevListelement->_next:_list->_first);_listelement=NULL;} // so that we are forced to create one
    }else // we'll be insertingappending/prepending, so the current last is the predecessor (and no successor)
    if(index>0) // MdH@17OCT2019: when not prepending...
        _prevListelement=_list->_last;
    // if we do not have a list element ascertain to have one
    if(!_listelement){ // not yet present in list, so we have to create a new element
        _listelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',owner);
        if(!_listelement){outputError("Failed to create a list element to insert");return 0;} // failure
    }
    // MDH@02NOV2019: if the list is flagged as weak we do not (de)reference values (and copy lists and maps as assignValue() does)
    if(_list->weak)_listelement->_value=_value;else assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
    // outputValue("Count of value '",_value,"' added to list:");output("%zd\n",_listelement->_value->count); // DEBUG
    // if replacing i.e. the index of _listelement matches index, we're done
    // if index equals 0 it WILL be equal to _listelement->index (which is initialized to 0 for sure)
    if(_listelement->index!=index){ // insert or append
        SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list
        _listelement->index=index;
        // linking
        if(_prevListelement)_prevListelement->_next=_listelement;else _list->_first=_listelement;
        if(!_nextListelement){if(_list->_last)_list->_last->_next=_listelement;_list->_last=_listelement;}else _listelement->_next=_nextListelement;
        (_list->numberOfElements)++; // an additional element
    }else
    if(index==0){ // prepending
        SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list
        // linking into the list
        _listelement->_next=_list->_first;
        _list->_first=_listelement;
        if(!_list->_last)_list->_last=_list->_first;
        (_list->numberOfElements)++;
        // if(amVerboseDebugging())outputValue("\tPrepending '",_listelement->_value,"'.\n");
        // we should increment the index of all elements (consuming _listelement on the go which is OK)
        _nextListelement=_listelement;
        while(_nextListelement){
            // if(amVerboseDebugging())
            // {outputValue("\tIncrementing the index of '",_nextListelement->_value,"'.\n");} // DEBUG
            (_nextListelement->index)++;
            // if(amVerboseDebugging()){outputValue("\tIndex of '",_nextListelement->_value,"' incremented");output(" to %llu.\n",_nextListelement->index);} // DEBUG
            _nextListelement=_nextListelement->_next;
            // if(amVerboseDebugging()){if(_nextListelement)outputInfo("\tA next element to consider!");else outputInfo("\tNo next element to consider!");}
        }
        //if(amVerboseDebugging()){outputValue("\t'",_listelement->_value,"' prepended to a list");output(" (now) with %llu elements.\n",_list->numberOfElements);}
    }
    if(amDebugging())checkList(_list);
    return _listelement->index;
}/* VALIDATED */

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
}/* VALIDATED */
Mvalue** getValueHolderAtIndex(Mlist* _list,long long index){
    // NOTE if index is equal to zero definitely no value there!!!
    if(_list&&index){
        // if(amVerboseDebugging()){outputList("Determining the value holder in '",_list,"'");output(" at index %lld.\n",index);}
        if(_list->_last){
            long long maxindex=_list->_last->index;
            if(index<0)index+=(maxindex+1);
            if(index>0){
                if(index==maxindex)return &(_list->_last->_value);
                if(index<maxindex){
                    Mlistelement* _listelement=_list->_first;
                    while(_listelement){
                        if(_listelement->index==index){
                            // if(amVerboseDebugging())outputValue("\tFound with value '",_listelement->_value,"'.\n");
                            return &(_listelement->_value);
                        }
                        if(_listelement->index>index)break; // couldn't find it!!!
                        _listelement=_listelement->_next;
                    }
                }
            }
        }
    }
    return NULL;
}/* VALIDATED */// END LIST STUFF

// MAP STUFF
// MDH@22OCT2020: useful to know if a map contains a certain attribute 
Mmapelement* getMapelement(Mmap const * const map,char const * const attributeName){
    Mmapelement* mapelement=(map&&attributeName?map->_first:NULL); // if both map and attribute name are defined, initialize map element to the first map element
    // as long as the map element is defined, and either it does not hold a variable or the variable's name does not match the given attribute name, select the next map element
    while(mapelement&&(!mapelement->_variable||strcmp(mapelement->_variable->_name->chars,attributeName)))mapelement=mapelement->_next;
    return mapelement;
}
// MDH@24MAY2019: if already in the map should replace the current value
long long appendedToMap(Mmap* const _map,Mallocationowner owner_map,char const * const attributeName,Mvalue const * const _attributeValue){Mallocationowner owner=getOwner(__LINE__);
    long long result=(_map&&attributeName?M_FALSE:M_LL_INVALID);
    if(result!=M_LL_INVALID){
        if(!_map->immutable){ // the map is mutable
            // MDH@05NOV2019: let's always allow adding NULL or undefined values to a map, but otherwise the type of _attributeValue should match the type of values the map allows
            if(!_attributeValue||_attributeValue->type==VT_UNDEFINED||_map->valuetype==VT_UNDEFINED||_attributeValue->type==_map->valuetype){
                if(amVerbose())
                    {output("Setting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.\n");}
                // MDH@22OCT2020: get the map element associated with the given attribute name
                Mmapelement* _mapelement=getMapelement(_map,attributeName);
                /* replacing:
                Mmapelement* _mapelement=_map->_first;
                while(_mapelement&&(!_mapelement->_variable||strcmp(_mapelement->_variable->_name->chars,attributeName)))_mapelement=_mapelement->_next;
                */
                if(!_mapelement){ // not found
                    if(amVerbose())
                        outputInfo("Attribute not found");
                    _mapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner); // NOTE no need to set _next because it is now NULL
                    if(_mapelement){
                        // MDH@09JUN2020: we can immediately set the owner of _variable to be in the map because if we succeed in creating it that's where it will go
                        // MDH@12MAR2020: I suppose we would like to be able to change the map property value (now using dot notation as well), so the mutable flag should be true not false
                        // MDH@25MAY2020: we're disowning _variable because we 
                        Mvariable* _variable=_getVariableWithName(attributeName,VT_UNDEFINED,false,Msubowner(owner_map,2)); // TODO why would this 'variable' be mutable, and allowing all values????
                        if(_variable){ // the variable was created so attach in map
                            if(amVerbose())
                                outputInfo("Map element created");
                             // pass ownership of _mapelement to _map at the first sublevel
                            _mapelement->_variable=_variable; // pass ownership of _variable to the mapelement at the second sublevel in the map
                            if(_map->numberOfElements)_map->_last->_next=_mapelement;else _map->_first=_mapelement;
                            _map->_last=SUBOWNED(OWNED(DISOWNED(_mapelement,owner),owner_map),1);
                            _map->numberOfElements++;
                            // result=M_TRUE; // success
                        }else{ // we have a map element BUT no variable, so no go
                            FREE_MAPELEMENT(_mapelement,false,owner);_mapelement=NULL;
                            outputError("Failed to create a new attribute");
                        }
                    }else
                        outputError("Failed to create new map element");
                }
                if(_mapelement){
                    if(_map->weak)
                        _mapelement->_variable->_value=_attributeValue;
                    else // MDH@02NOB2019: if the map is weak assign directly!!
                        assignValue(&_mapelement->_variable->_value,_attributeValue); // replace the current attribute value with the new value
                    result=M_TRUE;
                }
            }else{
                output("%s",M_ERROR_PREFIX);outputValue("Unable to add '",_attributeValue,"' to a map: it is of the wrong type.\n");
            }
        }else 
            outputError("Unable to change the map: it is immutable");
    }else
        outputError("No map or atribute name specified");
    if(amVerbose())
        output("Value %sappended to map.\n",(result==M_TRUE?"":"NOT "));
    return result;
}/* VALIDATED */
long long removedFromMap(Mmap* _map,Mallocationowner owner_map,char const * const attributeName){
    long long result=(_map&&attributeName?M_FALSE:M_LL_INVALID);
    if(result!=M_LL_INVALID){
        if(!_map->immutable){ // the map is mutable
            Mmapelement *mapelement=_map->_first,*previousmapelement=NULL;
            while(mapelement&&mapelement->_variable&&strcmp(mapelement->_variable->_name->chars,attributeName)){previousmapelement=mapelement;mapelement=previousmapelement->_next;}
            if(mapelement){ // found
                if(previousmapelement)previousmapelement->_next=mapelement->_next;else _map->_first=mapelement->_next; // disconnect the map element
                if(mapelement->_next==NULL)_map->_last=previousmapelement;else mapelement->_next=NULL;
                _map->numberOfElements--; // one less element in the map now
                FREE_MAPELEMENT(mapelement,_map->weak,owner_map); // free (all parts of) the map element
            }
            result=M_TRUE;
        }
    }
    return result;
}/* VALIDATED */

Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName){
    // MDH@14NOV2019: there's no reason why the map couldn't have an attribute with an empty name!!!!
    if(_map&&attributeName){
        Mmapelement* _mapelement=_map->_first;
        if(_mapelement){            
            // keep looking until there is a match
            while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name->chars,attributeName))_mapelement=_mapelement->_next;
            // if there is a match return the associated value
            if(_mapelement&&_mapelement->_variable)return _mapelement->_variable->_value;
        }
    }
    return NULL;
}/* VALIDATED */
// MDH@25MAR2020: sometimes we need the value holder
Mvalue** getValueHolderOfAttribute(Mmap* _map,char* attributeName){
    // MDH@14NOV2019: there's no reason why the map couldn't have an attribute with an empty name!!!!
    if(_map&&attributeName){
        Mmapelement* _mapelement=_map->_first;
        if(_mapelement){            
            // keep looking until there is a match
            while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name->chars,attributeName))_mapelement=_mapelement->_next;
            // if there is a match return the associated value
            if(_mapelement&&_mapelement->_variable)return &(_mapelement->_variable->_value);
        }
    }
    return NULL;
}/* VALIDATED */

// END MAP STUFF

Mvalue* _getValueOfRational(Mrational* _rational/*,Mallocationowner owner_rational*/){
    if(!_rational)return NULL;
    Mvalue* _rationalValue=__value("rational");
    if(_rationalValue){
        _rationalValue->type=VT_RATIONAL;
        _rationalValue->value._rational=(Misdisowned(_rational)?owned_rational(_rational,owner_value_data):_rational);
    }else
    if(Misdisowned(_rational))free_rational(_rational);
    return _rationalValue;
}/* VALIDATED */

//////////Mstring* _getValueText(Mvalue* _value); // forward prototype used in getListText() and getMapText()
Mstring* _getListText(Mlist const * const _list){Mallocationowner owner=getOwner(__LINE__);
    ///////output("List to output.");char c;inputCharRead(&c);
	Mstring* result=owned_string(__string(),owner);
    if(result){
        Mstring* p=result;
        if(amDebugging())p=string_append_char(p,'l');
		p=string_append_char(p,'['); // switch to using p in appends
		/////////size_t l=_list->numberOfElements;
        unsigned long long listindex=1;
		Mlistelement* _listelement=(_list?_list->_first:NULL);
		Mvalue* _listelementValue;
		while(p&&_listelement){
            ///////outputChar('$');
            // increment listindex until it is equal to _listelement->index
            if(_listelement->index==0)break; // VERY UNLIKELY AS field index should be monotonically increasing
            while(listindex<_listelement->index){listindex++;p=string_append_char(p,',');}
            if(amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
            // MDH@17JUN2020: because weak list values can be freed without the list knowing about it we cannot display it without possibly crashing...
            if(!_list->weak){
	            //////////outputChar('.');
		    	_listelementValue=_listelement->_value;
                if(_listelementValue){
				    Mstring* _listelementValueText=owned_string(_getValueText(_listelementValue,false),owner); // to be freed asap
				    if(_listelementValueText){
					    p=string_append(p,string(_listelementValueText));
					    FREE_STRING(_listelementValueText,owner); // release AFTER copying over
				    }
			    }
            }else
            if(!amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
			_listelement=_listelement->_next;
		}
		p=string_append_char(p,']');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(!p){FREE_STRING(result,owner);result=NULL;}
	}
	return disowned_string(result,owner);
}/* VALIDATED */
// MDH@02MAR2020: utility function to output a map
void outputList(char const * const prefix,Mlist const * const list,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
    Mstring* _listText=owned_string(_getListText(list),owner);
    output("%s%s%s",(prefix?prefix:""),string(_listText),(suffix?suffix:""));
    FREE_STRING(_listText,owner);
}/* VALIDATED */

Mstring* _getMapText(Mmap const * const _map,bool showcurlybraces,bool showquotes,bool showmissings){Mallocationowner owner=getOwner(__LINE__);
	Mstring* result=owned_string(__string(),owner);
    if(result){
	    Mstring* p=result;
        if(amDebugging())p=string_append_char(p,'m');
        if(_map){
            if(showcurlybraces)p=string_append_char(p,'{');
            if(_map->numberOfElements>0){
                //////output("%s",string(p));
                Mmapelement* _mapelement=_map->_first;
                while(p&&_mapelement){
                    //////output("%s","start");
                    Mvariable* _mapVariable=_mapelement->_variable;
                    if(_mapVariable){
                        // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
                        if(showquotes)p=string_append_char(p,'\'');
                        p=string_append(p,_mapVariable->_name->chars);
                        if(showquotes)p=string_append_char(p,'\'');
                        if(showmissings||!isValueUndefined(_mapVariable->_value)){
                            /////output("%s",string(p));
                            p=string_append_char(p,':'); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
                            /////output("%s",string(p));
                            Mstring* _mapelementValueText=owned_string(_getValueText(_mapVariable->_value,false),owner); // free asap
                            /////output("Map element: %s",string(p));
                            // TODO technically NULL is also a value, so shouldn't be use the undefined value text????
                            if(_mapelementValueText){
                                p=string_append(p,string(_mapelementValueText)); // append 
                                FREE_STRING(_mapelementValueText,owner); // release AFTER copying over
                            }
                        }
                    }
                    _mapelement=_mapelement->_next;
                    if(!_mapelement)break;
                    p=string_append(p,","); // only when there's a next map element to process
                    ////output("%s","next");
                }
            }else
            if(_map->_first)p=string_append_char(p,'?');
            //////output("%s(%d)",string(p),string_length(p));
            if(showcurlybraces)p=string_append_char(p,'}');
        }
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){FREE_STRING(result,owner);result=NULL;}
	}
	return disowned_string(result,owner);
}/* VALIDATED */
// MDH@02MAR2020: utility function to output a map
void outputMap(char const * const prefix,Mmap const * const map,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
    Mstring* _mapText=owned_string(_getMapText(map,true,true,true),owner);
    output("%s%s%s",(prefix?prefix:""),(_mapText?string(_mapText):""),(suffix?suffix:""));
    FREE_STRING(_mapText,owner);
}/* VALIDATED */ 

// MDH@24OCT2019: if you want the value representation or perhaps the name of a constant depends on whether name is defined
/*
static Mstring* _nullValueTextRepresentation=NULL;
Mstring* _getNullValueTextRepresentation(){if(!_nullValueTextRepresentation)_nullValueTextRepresentation=_getString(M_NULL_VALUE_TEXT_REPRESENTATION);return _getNullValueTextRepresentation;}
*/
Mmap* getFilePropertyMap(Mfile* _file); // prototype
Mstring* _getValueText(const Mvalue* const _value,bool dequoted){Mallocationowner owner=getOwner(__LINE__);
	// NOTE whatever is returned should be freed
	Mstring* valueText=NULL;
    ////outputChar('.');
	if(_value){
        ////outputChar('+');
		////output("TYPE: %d\n",_value->type);
		switch(_value->type){
            case VT_UNDEFINED:valueText=owned_string(_getString(M_UNDEFINED_VALUE_TEXT),owner);break; // calling _getString() will create a new string every time but I think we have to do that because _getValueText() typically returns something that is freed elsewhere
			case VT_INTEGER:valueText=owned_string(_getIntegerText(_value->value._integer),owner);break;
            case VT_BIGINTEGER:valueText=owned_string(_getBigintegerText(_value->value._biginteger),owner);break; // how many characters do we need????
            case VT_DECIMAL:valueText=owned_string(_getDecimalText(_value->value._decimal,false),owner);break; // fixedpoint to obligatory (i.e. e-notation allowed for very big/small (positive) numbers)
            case VT_RATIONAL:valueText=owned_string(_getRationalText(_value->value._rational),owner);break;
			case VT_FLOAT:valueText=owned_string(_getFloatText(_value->value._float),owner);break;
			case VT_TEXT:valueText=owned_string(_getStringText(_value->value._text,dequoted),owner);break; // TODO don't dequote the text!!
			case VT_MAP:valueText=owned_string(_getMapText(_value->value._map,true,true,true),owner);break;
			case VT_LIST:valueText=owned_string(_getListText(_value->value._list),owner);break;
            case VT_TOKEN:
                { // can't just show the single token because we could have following ones
                    valueText=owned_string(__string(),owner);
                    if(valueText){
                        Mstring* p=valueText;
                        Mtoken* token=_value->value._token;
                        while(p&&token){
                            if(token->text)p=string_append(p,string(token->text));
                            token=token->next;
                        }
                        if(!p){FREE_STRING(valueText,owner);valueText=NULL;}
                    }
                    // replacing: valueText=_stringCopy(_value->value._token->text,0);
                }
                break; // we need to return a copy because that copy will be freed typically (and we do not want to free the original now do we?)
			case VT_REFERENCE:
                {
                    valueText=owned_string(__string(),owner);
                    if(valueText){
                        Mstring* p=valueText;
                        p=string_append_char(p,M_DEREFERENCE_CHARACTER);
                        if(_value->value._reference&&_value->value._reference->variable){ // MDH@11MAR2020: possibly a reference without a variable yet associated (typically used with reference function parameters)
                            p=string_append(p,_value->value._reference->variable->_name->chars);
                            // append the reference index so we know which one it is
                            p=string_append_char(p,':');
                            p=appendll(p,_value->value._reference->referenceindex);
                        }
                        /* replacing:
                        if(_value->value._reference->_name)p=string_append(p,_value->value._reference->_name);
                        if(_value->value._reference->_itemid){
                            p=string_append_char(p,'[');
                            Mstring* _valueText=_getValueText(_value->value._reference->_itemid,dequoted);
                            if(_valueText){p=string_append(p,string(_valueText));FREE_STRING(_valueText);}
                            p=string_append_char(p,']');
                        }
                        if(_value->value._reference->_value){
                            p=string_append_char(p,'=');
                            Mstring* _valueText=_getValueText(_value->value._reference->_value,dequoted);
                            if(_valueText){p=string_append(p,string(_valueText));FREE_STRING(_valueText);}
                        }
                        */
                        if(!p){FREE_STRING(valueText,owner);valueText=NULL;}
                    }
                }
                break;
            case VT_FUNCTION:
                {
                    valueText=owned_string(_getString("function"),owner);
                    if(valueText){
                        Mstring* p=valueText;
                        Mfunction* function=_value->value._function;
                        if(function){
                            p=string_append_char(p,'(');
                            if(p&&function->_parameterMap){
                                Mmapelement* _parameterMapelement=function->_parameterMap->_first;
                                while(p&&_parameterMapelement){
                                    if(_parameterMapelement->_variable){
                                        p=string_append(p,_parameterMapelement->_variable->_name->chars);
                                        p=string_append_char(p,':');
                                        Mstring* _parameterValueText=owned_string(_getValueText(_parameterMapelement->_variable->_value,false),owner);
                                        p=string_append(p,string(_parameterValueText));
                                        FREE_STRING(_parameterValueText,owner);
                                    }
                                    _parameterMapelement=_parameterMapelement->_next;
                                    if(_parameterMapelement)p=string_append_char(p,',');
                                }
                            }
                            p=string_append_char(p,')');
                        }else
                            p=string_append_char(p,'?');
                        if(!p){FREE_STRING(valueText,owner);valueText=NULL;}
                    }
                }
                break;
            case VT_ENVIRONMENT:
                {
                    valueText=owned_string(_getString("environment"),owner);
                    if(valueText){
                        Mstring* p=valueText;
                        Menvironment* environment=_value->value._environment;
                        if(environment){
                            p=string_append_char(p,'(');
                            p=string_append_char(p,'\'');
                            Mstring* _environmentName=owned_string(_getEnvironmentName(environment),owner);
                            p=string_append(p,string(_environmentName));
                            FREE_STRING(_environmentName,owner);
                            p=string_append_char(p,'\'');
                            p=string_append_char(p,')');
                        }else
                            p=string_append_char(p,'?');
                        if(!p){FREE_STRING(valueText,owner);valueText=NULL;}
                    }
                }
                break;
            case VT_FILE: // MDH@28SEP2020: get the file property map and make text of it
                {
                    valueText=owned_string(_getMapText(getFilePropertyMap(_value->value._file),true,true,true),owner);
                }
            default:break;
		}
	}else // the text we use for an value that is NULL!
        valueText=owned_string(_getString(M_NULL_VALUE_TEXT),owner);
    if(_value)if(amAssisting())if(valueText)valueText=owned_string(appendll(string_append_char(valueText,'#'),_value->count),owner); // show the reference count as well
    /////outputChar('.');
    return(valueText?disowned_string(valueText,owner):_getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}/* VALIDATED */
// MDH@13MAR2020: now returning the number of characters written
size_t outputValue(char const * const prefix,Mvalue const * const value,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
    size_t written=0;
    if(prefix)written=output("%s",prefix);
    if(value){
        // output("%u",value->type); // DEBUG
        Mstring* _valueText=owned_string(_getValueText(value,false),owner); // free asap
        if(_valueText){written+=output("%s",string(_valueText));FREE_STRING(_valueText,owner);_valueText=NULL;}
    }else
        written+=outputChar('-');
    if(suffix)written+=output("%s",suffix);
    return written;
}/* VALIDATED */

long long getValueInteger(const Mvalue* const _value){Mallocationowner owner=getOwner(__LINE__);
    // ASSERTION if _value can be converted to an integer,it should not equal invalid!!!!
    if(_value){
        switch(_value->type){
            case VT_INTEGER:return _value->value._integer->ll;
		    case VT_FLOAT:return double2long(_value->value._float->ld);
            case VT_BIGINTEGER:return biginteger2long(_value->value._biginteger);
            case VT_DECIMAL:return decimal2long(_value->value._decimal);
            case VT_TEXT:
                {
                    // alternative which doesn't check for 0 explicitly I think: _strtoll(_value->value._text->_c,M_LL_INVALID);
                    // we should check for 0 explicitly, which should always be represented by a single '0' character i.e. without signs!!!
                    // TODO are we allowing other zero representations as well???
                    if(strIsZero(_value->value._text->_c))return 0;
                    long long ll=atoll(_value->value._text->_c); // invalid if zero
                    if(ll!=0)return ll; // valid if non-zero
                }
                break;
            case VT_TOKEN:
                {
                    Mstring* tokenText=_value->value._token->text;
                    if(strIsZero(string(tokenText)))return 0;
                    long long ll=atoll(string(tokenText)); // invalid if zero
                    if(ll!=0)return ll; // valid if non-zero
                }
                break;
            case VT_RATIONAL:
                {
                    Mbiginteger* _biginteger=owned_biginteger(_rational2biginteger(_value->value._rational),owner);
                    if(_biginteger){
                        long long ll=biginteger2long(_biginteger);
                        FREE_BIGINTEGER(_biginteger,owner);
                        return ll;
                    }
                }
            case VT_REFERENCE: // TODO this might be hard
                break;
   		    default:break;
        }
        // TODO sometimes reals can also represent integers!!!
    }
    return M_LL_INVALID;
}/* VALIDATED */

// MDH@09OCT2019: TODO=DONE is there a better way than through parsing?????
long double getBigintegerLongDouble(Mbiginteger* biginteger){
    return mp_get_long_double(biginteger); // MDH@28OCT2019: definitely
    /* replacing:
    long double ldBiginteger=M_LD_NAN;
   	Mstring* _bigintegerText=(biginteger?_getBigintegerText(biginteger):NULL);
    if(_bigintegerText){ldBiginteger=_strtold(string(_bigintegerText),ldBiginteger);FREE_STRING(_bigintegerText);}
	return ldBiginteger;
    */
}
long double getRealLongDouble(const Mfloat* const _real){return(_real?_real->ld:M_LD_NAN);}/* VALIDATED */

long double getValueLongDouble(const Mvalue* const _value){
    // MDH@09OCT2019: a little more complicated then just getting out the real in that all scalar numeric values are convertable
    long double valueLongDouble=M_LD_NAN;
    if(_value)
    switch(_value->type){ // all scalar types should be convertible
        case VT_INTEGER:valueLongDouble=_value->value._integer->ll;break;
        case VT_BIGINTEGER:valueLongDouble=getBigintegerLongDouble(_value->value._biginteger);break;
        case VT_FLOAT:valueLongDouble=_value->value._float->ld;break;
        case VT_DECIMAL:valueLongDouble=getDecimalLongDouble(_value->value._decimal);break;
        case VT_RATIONAL:valueLongDouble=getRationalLongDouble(_value->value._rational);break;
        case VT_TEXT:valueLongDouble=_strtold(_value->value._text->_c,M_LD_NAN);break; // MDH@28OCT2019: OOPS text is also a 'scalar' 
        case VT_TOKEN: valueLongDouble=_strtold(string_remainder(_value->value._token->text,1),M_LD_NAN);break; // MDH@28OCT2019: OOPS a token is also a 'scalar' (NOTE string_remainder simply skips the quote character)
        case VT_REFERENCE: // TODO this might be hard
            break;
        default:break;
    }
    return valueLongDouble;
}/* VALIDATED */

Mbiginteger* _getValueBiginteger(Mvalue const * const _value){Mallocationowner owner=getOwner(__LINE__);
    if(!_value)return NULL;
    if(_value&&_value->type==VT_BIGINTEGER)return _value->value._biginteger;
    Mbiginteger* _resultBiginteger=NULL;
    switch(_value->type){
        case VT_INTEGER:_resultBiginteger=owned_biginteger(_getBiginteger(_value->value._integer->ll),owner);break;
        case VT_RATIONAL:_resultBiginteger=owned_biginteger(_rational2biginteger(_value->value._rational),owner);break; // TODO how can we be certain that the returned big integer is actually used? well, it should as this is _getValueBiginteger meaning you have to free it if you don't use it!!!
        case VT_FLOAT:
            {
                // this is a bit of a nuisance when the double is out of the VT_INTEGER range
                _resultBiginteger=owned_biginteger(__biginteger(),owner);
                if(_resultBiginteger&&mp_set_longdouble(_resultBiginteger,_value->value._float->ld)!=MP_OKAY)
                {FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
                if(!_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
            }
            break;
        case VT_TEXT:
            {
                _resultBiginteger=__biginteger();
                if(_resultBiginteger&&mp_read_radix(MP_INT_POINTER(_resultBiginteger),_value->value._text->_c,10)!=MP_OKAY)
                {FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
                if(!_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
            }
            break;
        case VT_TOKEN:
            {
                _resultBiginteger=owned_biginteger(__biginteger(),owner);
                if(_resultBiginteger&&mp_read_radix(MP_INT_POINTER(_resultBiginteger),string(_value->value._token->text),10)!=MP_OKAY)
                {FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
                if(!_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
            }
        case VT_REFERENCE: // TODO this may be hard
            break;
        default:break;
    }
    return disowned_biginteger(_resultBiginteger,owner);
}/* VALIDATED */

// (map) list conversions
bool listAppendedToMap(Mmap* const _map,Mallocationowner owner_map,const Mlist* const _list){Mallocationowner owner=getOwner(__LINE__); // appends a list to a (possibly empty) map using the indices as attribute name
    bool result=(_map!=NULL); // no map, no result!
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(_listelement){
                Mstring* _indexValueText=owned_string(__string(),owner);
                if(_indexValueText){ // free asap
                    if(!appendll(_indexValueText,_listelement->index)||appendedToMap(_map,owner_map,string(_indexValueText),_listelement->_value)<=0){outputError("Failed to append a list element to a map");result=false;}
                    FREE_STRING(_indexValueText,owner); // freeing
                }else{
                    result=false;
                    output("ERROR: Failed to convert list element index %llu to an attribute name.\n",_listelement->index);
                }
                if(!result)break;
                _listelement=_listelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
bool listAppendedToMaplist(Mlist* const _maplist,Mallocationowner owner_maplist,Mlist const * const _list){Mallocationowner owner=getOwner(__LINE__);
    bool result=(_maplist&&_maplist->valuetype==VT_LIST); // the destination list should only allow for list elements
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(result&&_listelement){
                // index and value of the list element are stored in a new list!!
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED,false,"listAppendedToMapList"); // this will be a new value that (being managed) will be freed automatically when not bound, including the list contained by it!!
                if(!_maplistelementValue){outputError("Failed to create an empty list");result=false;break;}
                Mlist* _maplistelement=_maplistelementValue->value._list;
                if(!_maplistelement)result=false;else
                if(appendedToList(_maplistelement,owner,_getIntegerValue(_listelement->index),M_LL_INVALID)<=0)result=false;else
                if(appendedToList(_maplistelement,owner,_listelement->_value,M_LL_INVALID)<=0)result=false;else
                if(appendedToList(_maplist,owner_maplist,_maplistelementValue,M_LL_INVALID)<=0)result=false;
                if(!result){
                    // no need to clean up because all created values will be garbage collected including their contents
                    break;
                }
                /* MDH@26MAY2020 replacing: if we fail to construct the maplist element or to add it
                if(!_maplistelement||appendedToList(_maplistelement,owner_maplist,_getIntegerValue(_listelement->index),M_LL_INVALID)<=0||!appendedToList(_maplistelement,_listelement->_value,M_LL_INVALID)||!appendedToList(_maplist,_maplistelementValue,M_LL_INVALID)){
                    outputError("Failed to create or populate map list element");
                    result=false;
                }else
                */
                    _listelement=_listelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
bool maplistAppendedToList(Mlist* const _list,Mallocationowner owner_list,const Mlist* const _maplist){
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
                    // TODO we're appending the value as is (i.e. without copying so any change will also be present in the list)
                    // DONE this is ok because values are immutable in essence
                    if(index>0&&appendedToList(_list,owner_list,_maplistelementValue->value._list->_first->_next->_value,index)<=0)result=false;
                }
                _maplistelement=_maplistelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
bool maplistAppendedToMap(Mmap * const _map,Mallocationowner owner_map,Mlist const * const _maplist){Mallocationowner owner=getOwner(__LINE__);
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
                    Mstring* _attributeNameValueText=owned_string(_getValueText(_attributeNameValue,true),owner); // using common _getValueText to text the first element
                    /* replacing:
                    if(_attributeNameValue){
                        if(_attributeNameValue->type==VT_INTEGER)_attributeNameValueText=_getIntegerText(_attributeNameValue->value._integer);else
                        if(_attributeNameValue->type==VT_FLOAT)_attributeNameValueText=_getFloatText(_attributeNameValue->value._float);else
                        if(_attributeNameValue->type==VT_TEXT)_attributeNameValueText=_getStringText(_attributeNameValue->value._text,true); // effectively cutting of the presuffix character TODO other solution????????
                    }
                    */
                    if(_attributeNameValueText){
                        // TODO again the value is appended as is, but 
                        // DONE that should be OK, because values are essentially immutable!!!
                        if(!string_length(_attributeNameValueText)||!appendedToMap(_map,owner_map,string(_attributeNameValueText),_maplistelementValue->value._list->_first->_next->_value)){
                            outputError("Failed to append a list element to a map (using the index text as attribute name)");
                            result=false;
                        }
                        FREE_STRING(_attributeNameValueText,owner);
                        // TODO is this Ok?
                        // DONE yes, because appendedToMap will _strdup the char* (i.e. string(_attributeNameValueText) )
                    }
                }
                _maplistelement=_maplistelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
// map to (map) list conversions
bool mapAppendedToList(Mlist* const _list,Mallocationowner owner_list,const Mmap* const _map){
    bool result=(_list!=NULL);
    if(result){
        if(_map&&_map->_first){
            Mmapelement* _mapelement=_map->_first;
            while(result&&_mapelement){
                // only add those map elements of which the key can be converted to a positive integer
                long long index=atoll(_mapelement->_variable->_name->chars);
                if(index>0&&appendedToList(_list,owner_list,_mapelement->_variable->_value,index)<=0){
                    outputError("Failed to append a map element to a list (using the integer value of the name as index)");
                    result=false;
                }
                _mapelement=_mapelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
bool mapAppendedToMaplist(Mlist* const _maplist,Mallocationowner owner_maplist,const Mmap* const _map){Mallocationowner owner=getOwner(__LINE__);
    bool result=(_maplist&&_maplist->valuetype==VT_LIST);
    if(result){
        Mallocationowner owner_maplistelement=Msubowner(owner_maplist,1);
        if(_map&&_map->_first){
            Mmapelement* _mapelement=_map->_first;
            while(result&&_mapelement){
               // index and value of the list element are stored in a new list!!
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED,false,"mapAppededToMapList");
                Mlist* _maplistelement=(_maplistelementValue?_maplistelementValue->value._list:NULL);
                if(_maplistelement){
                    // if we fail to construct the maplist element or to add it
                    // NOTE the attribute name does not start with a quote character wich we need to call _getTextValue
                    // TODO the following could be restructured I suppose
                    Mstring* _attributeName=owned_string(__string(),owner);
                    if(_attributeName){ // should be freed
                        Mstring* p=_attributeName;
                        p=string_append_char(p,'\'');
                        p=string_append(p,_mapelement->_variable->_name->chars);
                        if(p){
                            Mvalue* _attributeNameValue=_getTextValue(string(_attributeName));
                            if(_attributeNameValue&&appendedToList(_maplistelement,owner_maplistelement,_attributeNameValue,M_LL_INVALID)>0){
                                if(appendedToList(_maplistelement,owner_maplist,_mapelement->_variable->_value,M_LL_INVALID)<=0||appendedToList(_maplist,owner_maplist,_maplistelementValue,M_LL_INVALID)<=0){
                                    result=false;
                                    outputError("Failed to append the attribute value in constructing a map list element");
                                }
                            }else{
                                result=false;
                                outputError("Failed to create or add the attribute name in constructing a map list element");
                            }
                        }else{
                            result=false;
                            outputError("Failed to construct the attribute name text in constructing a map list element");
                        }
                        FREE_STRING(_attributeName,owner); // freed!
                    }else{
                        result=false;
                        outputError("Failed to create a text");
                    }               
                }else{
                    result=false;
                    outputError("Failed to create a map list element list");
                }
                _mapelement=_mapelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */

// DECIMAL EXTRACTION
Mdecimal* _getValueTextDecimal(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
    // MDH@09OCT2019: delegating to _getTextDecimal() is preferable over doing it ourselves
    Mdecimal* _valueTextDecimal=NULL;
    if(value){
        if(value->type!=VT_DECIMAL){
    	    Mstring* _valueText=owned_string(_getValueText(value,true),owner); // TODO will there be any brackets around a repeating part of a 
	        if(_valueText){
                _valueTextDecimal=owned_decimal(_getTextDecimal(string(_valueText),0),owner);
                FREE_STRING(_valueText,owner);
            }else
                outputError("Failed to create the text trying to convert a value to a decimal");
        }else
            _valueTextDecimal=owned_decimal(_getDecimalCopy(value->value._decimal),owner); // shouldn't happen though
    }
    return disowned_decimal(_valueTextDecimal,owner);
    /* replacing:
    Mdecimal* _parsedValueDecimal=__decimal(NULL,0,0);
    if(_parsedValueDecimal){

		Mstring* _valueText=_getValueText(value,true);
		if(_valueText){
            uint32_t status=0;
            mpd_qset_string(_parsedValueDecimal->mpd,string(_valueText),get_default_mpd_context(),&status);
            FREE_STRING(_valueText);
            if((status&0xEFBF)!=0){FREE_DECIMAL(_parsedValueDecimal);_parsedValueDecimal=NULL;outputError("Failed to parse the decimal text");}
        }
    }else
        outputError("Failed to create a decimal");
    return _parsedValueDecimal;
    */
}

// the work horse of converting any value (if possible) to a decimal
// TODO shouldn't we use this function in d()???????
Mdecimal* _getValueDecimal(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(value){
		if(value->type!=VT_LIST&&value->type!=VT_MAP){
			switch(value->type){
				case VT_DECIMAL:_decimal=owned_decimal(_getDecimalCopy(value->value._decimal),owner);break;
				case VT_INTEGER:_decimal=owned_decimal(__decimal(NULL,value->value._integer->ll,0),owner);break; // MDH@29AUG2019: replacing a call to _getDecimal()
                case VT_BIGINTEGER:
                    {
                        // NOTE we need to make a copy of the big integer because otherwise FREE_RATIONAL() below would free the big integer wrapped inside the value, which would be a terrible mistake
                        // MDH@26MAY2020 but now that _getRational does not use the given numerator and denominator anymore and makes a copy if necessary we can simply pass in value->value._biginteger as is
                        Mrational* _rational=_getRational(value->value._biginteger,NULL,M_LD_NAN,false); // much neater, isn't it?
                        /* replacing:
                        Mbiginteger* _numerator=OWNED(_getBigintegerCopy(value->value._biginteger),owner);
                        Mrational* _rational=_getRational(_numerator,NULL,M_LD_NAN,false);
                        FREE_BIGINTEGER(_numerator,owner);
                        */
                        if(_rational){
                            _decimal=owned_decimal(_getRationalDecimal(_rational),owner);
                            FREE_RATIONAL(_rational,owner);
                        }
                    }
                    break;
				case VT_RATIONAL:
					_decimal=owned_decimal(_getRationalDecimal(value->value._rational),owner);
					break;
				default:
                    _decimal=owned_decimal(_getValueTextDecimal(value),owner); // delegates to _getValueTextDecimal() which always parses the decimal from the text representation of the value
					break;
			}
		}
	}
	return disowned_decimal(_decimal,owner);
}
Mdecimal* getValueDecimal(Mvalue* value){
	return(value?(value->type==VT_DECIMAL?value->value._decimal:_getValueDecimal(value)):NULL);
}
// END DECIMAL EXTRACTION

// MDH@03FEB2020: extract specific data elements wrapped in values
Menvironment* getValueEnvironment(Mvalue* value){return(value&&value->type==VT_ENVIRONMENT?value->value._environment:NULL);}

Mlist* _getListOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
    // output("Allocating list (size: %zd).\n",sizeof(Mlist));
    Mlist* _list=owned_list(__list("getListOfType"),owner);
    if(!_list)return NULL;
    _list->valuetype=valuetype;
    return disowned_list(_list,owner);
}/* VALIDATED */

Mmap* _getMapOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
    if(!_map)return NULL;
    _map->valuetype=valuetype;
    return disowned_map(_map,owner);
}/* VALIDATED */

Mlist* listMadeWeak(Mlist* list){if(list)list->weak=true;return list;}
Mmap* mapMadeWeak(Mmap* map){if(map)map->weak=true;return map;}

long long getIntegerSign(long long integer){return(integer>0?1:(integer<0?-1:0));}
long long getValueSign(Mvalue const * const value){
    if(value){
        if(value->type==VT_INTEGER)return getIntegerSign(value->value._integer->ll);
        if(value->type==VT_BIGINTEGER)return getBigintegerSign(value->value._biginteger);
        if(value->type==VT_FLOAT)return getLongDoubleSign(value->value._float->ld);
        if(value->type==VT_DECIMAL)return getDecimalSign(value->value._decimal);
        if(value->type==VT_RATIONAL)return getRationalSign(value->value._rational);
    }
    return M_LL_INVALID;
}
// TODO turn the result type of isValueZero() into long long
long long isValueZero(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value){
        if(amVerboseDebugging())
            outputValue("Checking whether '",value,"' is zero");
        if(value->type==VT_INTEGER)result=isIntegerZero(value->value._integer);else
        if(value->type==VT_BIGINTEGER)result=isBigintegerZero(value->value._biginteger);else
        if(value->type==VT_FLOAT)result=isFloatZero(value->value._float);else
        if(value->type==VT_DECIMAL)result=isDecimalZero(value->value._decimal);else
        if(value->type==VT_RATIONAL)result=isRationalZero(value->value._rational);
    }
    if(amVerboseDebugging())
        output(": %s.\n",(result==M_TRUE?"YES":"NO"));
    return result;
}/* VALIDATED */
long long isValueOne(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value){
        if(amVerboseDebugging())
            outputValue("Checking whether '",value,"' equals one");
        if(value->type==VT_INTEGER)result=isIntegerOne(value->value._integer);else
        if(value->type==VT_BIGINTEGER)result=isBigintegerOne(value->value._biginteger);else
        if(value->type==VT_FLOAT)result=isFloatOne(value->value._float);else
        if(value->type==VT_DECIMAL)result=isDecimalOne(value->value._decimal);else
        if(value->type==VT_RATIONAL)result=isRationalOne(value->value._rational);
        if(amVerboseDebugging())
            output(": %s.\n",(result==M_TRUE?"YES":"NO"));
    }
    return result;
}/* VALIDATED */
long long isValuePositive(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value){
        if(amVerboseDebugging())
            outputValue("Checking whether '",value,"' is positive");
        if(value->type==VT_INTEGER)result=isIntegerPositive(value->value._integer);else
        if(value->type==VT_BIGINTEGER)result=isBigintegerPositive(value->value._biginteger);else
        if(value->type==VT_FLOAT)result=isFloatPositive(value->value._float);else
        if(value->type==VT_DECIMAL)result=isDecimalPositive(value->value._decimal);else
        if(value->type==VT_RATIONAL)result=isRationalPositive(value->value._rational); // assuming the numerator is never NULL and the denominator is always positive
        if(amVerboseDebugging())
            output(": %s.\n",(result==M_TRUE?"YES":"NO"));
    }
    return result;
}/* VALIDATED */
long long isValueNegative(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value){
        if(amVerboseDebugging())
            outputValue("Checking whether '",value,"' is negative");
        if(value->type==VT_INTEGER)result=isIntegerNegative(value->value._integer);else
        if(value->type==VT_BIGINTEGER)result=isBigintegerNegative(value->value._biginteger);else
        if(value->type==VT_FLOAT)result=isFloatNegative(value->value._float);else
        if(value->type==VT_DECIMAL)result=isDecimalNegative(value->value._decimal);else
        if(value->type==VT_RATIONAL)result=isRationalNegative(value->value._rational);
        if(amVerboseDebugging())
            output(": %s.\n",(result==M_TRUE?"YES":"NO"));
    }
    return result;
}/* VALIDATED */
long long isValueScalar(Mvalue* value){
    if(value)switch(value->type){case VT_INTEGER:case VT_BIGINTEGER:case VT_DECIMAL:case VT_RATIONAL:case VT_FLOAT:case VT_TEXT:case VT_TOKEN:return M_TRUE;default:return M_FALSE;}
    return M_FALSE;
}/* VALIDATED */

// MDH@26OCT2019: TODO perhaps a reference is invalid if it is no longer pointing somewhere??????
long long isReferenceUndefined(Mvaluereference* valuereference){
    return(valuereference?M_FALSE:M_TRUE);
}

// null test for the value to be considered NULL
long long isValueNull(Mvalue* value){
    long long result=M_LL_INVALID;
    if(value)
    switch(value->type){
        case VT_INTEGER:result=(value->value._integer?M_FALSE:M_TRUE);break;
        case VT_BIGINTEGER:result=(value->value._biginteger?M_FALSE:M_TRUE);break;
        case VT_DECIMAL:result=(value->value._decimal?M_FALSE:M_TRUE);break;
        case VT_RATIONAL:result=(value->value._rational?M_FALSE:M_TRUE);break;
        case VT_FLOAT:result=(value->value._float?M_FALSE:M_TRUE);break;
        case VT_TEXT:result=(value->value._text?M_FALSE:M_TRUE);break;
        case VT_LIST:result=(value->value._list?M_FALSE:M_TRUE);break;
        case VT_MAP:result=(value->value._map?M_FALSE:M_TRUE);break;
        case VT_TOKEN:result=(value->value._token?M_FALSE:M_TRUE);break;
        case VT_UNDEFINED:result=M_TRUE;break;
        case VT_REFERENCE:result=(value->value._reference?M_FALSE:M_TRUE);break;
        case VT_FUNCTION:result=(value->value._function?M_FALSE:M_TRUE);break;
        case VT_ENVIRONMENT:result=(value->value._environment?M_FALSE:M_TRUE);break;
        case VT_FILE:result=(value->value._file?M_FALSE:M_TRUE);break; // MDH@28SEP2020
    }
    return result;
}/* VALIDATED */
// MDH@18JUL2019: we consider certain non-null values as undefined, this is to fill the gap between non-null values that represent missings
//                TODO is a map or list undefined when empty???????
long long isValueUndefined(Mvalue* value){
    long long result=M_LL_INVALID;
    // values that are considered NULL are also undefined (even if value is NULL)
    if(value) // we have to check the value
    switch(value->type){
        case VT_INTEGER:result=isIntegerUndefined(value->value._integer);break; // replacing: value->value._integer->ll==M_LL_INVALID;
        case VT_FLOAT:result=isFloatUndefined(value->value._float);break; // replacing: return ldIsNaN(value->value._float->ld);
        case VT_BIGINTEGER:result=isBigintegerUndefined(value->value._biginteger);break;
        case VT_DECIMAL:result=isDecimalUndefined(value->value._decimal);break; // replacing: mpd_isnan((mpd_t*)value->value._decimal); // sames right but no idea how to set/get this // decimal points directly to mpd_t so we can cast
        case VT_RATIONAL:result=isRationalUndefined(value->value._rational);break;
        case VT_TEXT:result=isTextUndefined(value->value._text);break; ////strlen(_value->value._text->_c)==0;
        case VT_LIST:result=isListUndefined(value->value._list);break; ////Mlen(_value)==0;
        case VT_MAP:result=isMapUndefined(value->value._map);break; ////Mlen(_value)==0;
        case VT_TOKEN:result=isTokenUndefined(value->value._token);break; /////string_length(_value->value._token->text)==0;
        case VT_UNDEFINED:result=M_TRUE;break;
        case VT_REFERENCE:result=(value->value._reference?M_FALSE:M_TRUE);break;
        case VT_FUNCTION:result=(value->value._function?M_FALSE:M_TRUE);break;
        case VT_ENVIRONMENT:result=(value->value._environment?M_FALSE:M_TRUE);break;
        case VT_FILE:result=(value->value._file?M_FALSE:M_TRUE);break; // MDH@28SEP2020
    }
    return result;
}/* VALIDATED */

// MDH@04JUN2019: based on https://stackoverflow.com/questions/4637967/algorithm-challenge-generate-continued-fractions-for-a-float/56444882#56444882
Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter){Mallocationowner owner=getOwner(__LINE__);
    // if iterations, you're supposed to return all iteration results
    Mlist* _iterationsList=owned_list(_getListOfType(VT_UNDEFINED),owner);
    if(_iterationsList){ // should be freed when NOT returned!!
        Mrational* _rational=NULL; // the last (computed) rational
        if(isLongDoubleUndefined(ld)!=M_TRUE){ // not a NaN (might still be Infinity though), but Infinity has a sign too   
            long long ldSign=getLongDoubleSign(ld);
            if(ldSign!=M_ZERO){ // ld is not zero 
                bool neg=(ldSign==M_NEGATIVE);if(neg)ld=-ld; // remember if negative
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
                    ////////////replacing (see above): if(fabsl(delta)<=M_LD_Q_EPS)break; // if the p and q we've got are fine, stop!!!
                    Mbiginteger *_numerator=owned_biginteger(_getBiginteger(neg?-p:p),owner)
                               ,*_denominator=owned_biginteger(_getBiginteger(q),owner);
                    _rational=_getRational(_numerator,_denominator,delta,false/*,true*/); // construct the intermediate result without normalizing
                    FREE_BIGINTEGER(_numerator,owner);FREE_BIGINTEGER(_denominator,owner); // MDH@26MAY2020 now always!!!
                    if(!_rational){
                        output("%sFailed to construct the rational approximation %lld/%lld.\n",M_ERROR_PREFIX,p,q);
                        break;
                    }
                    // NOTE once we have the created big integer numerator and denominator bound in _rational we're responsible of freeing _rational when not bound
                    // NOT being able to append the intermediate result to the list shouldn't be enough reason to abort, as long as we manage to add the end result
                    Mvalue* _rationalValue=_getValueOfRational(disowned_rational(_rational,owner));
                    if(!_rationalValue){FREE_RATIONAL(_rational,owner);outputError("Failed to value wrap the intermediate rational approximation to a real");break;}
                    // NOTE probably best to break if we can't append approximations!!
                    // NOTE no need to free _rational even then as it is bound in _rationalValue so it will be freed anyway
                    if(appendedToList(_iterationsList,owner,_rationalValue,i)<=0)
                    {/*FREE_RATIONAL(_rational);*/outputError("Failed to register a rational approximation");break;}
                    // if we get here success in updating the iterations list!!!!
                    // if delta is now zero, we're done!!!
                    if(isLongDoubleZero(delta))break; ///// MDH@07JUN2019: when a list is returned like this don't stop below the system's epsilon but only when the delta is zero!!!!
                    rem-=a;
                    rem=1/rem;
                    // shift the lot
                    pmin2=pmin1;qmin2=qmin1;
                    pmin1=p;qmin1=q;
                }
                /* NO NEED FOR THIS ANYMORE NOW WE'VE PLACED THE CHECK FOR delta IS zero AFTER APPENDING THE RATIONAL
                if(ldIsZero(delta)){ // success but the final rational has not yet been appended to the list!!
                    // construct the last rational (i.e. the result) from p and q
                    Mbiginteger* _numerator=_getBiginteger(p),*_denominator=_getBiginteger(q); // have to be freed when not bound in _rational
                    if(_numerator&&_denominator)if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)_rational=_getRational(_numerator,_denominator,delta,true,false);
                    if(!_rational){FREE_BIGINTEGER(_numerator);FREE_BIGINTEGER(_denominator);} // DIY freeing if failing to construct the rational
                }
                */
            }else{ // long double is zero, TODO should we store 0 as the delta, or just NaN???? what would be the difference??????
                Mbiginteger* _numerator=owned_biginteger(__biginteger(),owner);
                Mrational* _rational=(Mrational*)OWNED(_getRational(_numerator,NULL,M_LD_NAN,false),owner);
                FREE_BIGINTEGER(_numerator,owner); // ALWAYS!!
                Mvalue* _rationalValue=_getValueOfRational(disowned_rational(_rational,owner)); // OK free __biginteger() if failing to get that _rational
                if(!_rationalValue)FREE_RATIONAL(_rational,owner);else 
                if(appendedToList(_iterationsList,owner,_rationalValue,0)<=0)
                    outputError("Failed to append rational approximation to the result list"); // no need to free _rational because it's value wrapper will be garbage collected!!
            }
        }
        /* 
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
        free_list(_iterationsList);
        */
    }else
        outputError("Failed to create a list for storing the rational approximations");
    return disowned_list(_iterationsList,owner);
}/* VALIDATED */

void assignValue(Mvalue** _valueholder, Mvalue const * _value){//Mallocationowner owner=getOwner(__LINE__);
    // {outputValue("Storing '",_value,"'");output(" in %p.\n",_valueholder);} // DEBUG
    // ASSERT not a composite value (so like an end node)
    if(*_valueholder)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
    // MDH@01NOV2019: it's a leap of faith to let assignValue() create copies of composite values i.e. instead of assigning _value to the *_valueholder we assign a new map or list value
    if(_value){
        // create a copy of the map or list and assign it to the value holder however it would then copy the map again, and that's not what should happen!!!
        // NOTE the new map and list value get a reference count of 1 below as soon as they are bound to the value holder (as should be the case)
        //      wait a minute a forgot to take care of the reference count of the values in _getMapCopy() and _getListCopy(), NO no need to that if they use assignValue() to 'copy' the values
        if(_value->type==VT_MAP){
            // if(amVerbose()&&amDebugging())outputValue("Copying map ",_value,".\n");
            _value=_getValueOfMap(_getMapCopy(_value->value._map));
            // if(!_value)free_map(_mapCopy,owner);
        }else
        if(_value->type==VT_LIST){
            // if(amVerbose()&&amDebugging())outputValue("Copying list ",_value,".\n");
            _value=_getValueOfList(_getListCopy(_value->value._list));
            // if(!_value)free_list(_listCopy,owner);
        }
    }
    *_valueholder=_value; // replace what's being pointed to
    if(*_valueholder)incrementReferenceCount(*_valueholder); // increment what it's pointing to now (if not NULL)
}/* VALIDATED */

// functions
// helpers
Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction){Mallocationowner owner=getOwner(__LINE__);
    Mlist* _result=NULL;
    if(_list){
        _result=owned_list(_getListOfType(_list->valuetype),owner);
        Mlistelement* _listelement=_list->_first;
        while(_listelement&&appendedToList(_result,owner,oneArgumentFunction(_listelement->_value),_listelement->index)>0)_listelement=_listelement->_next;
    }
    return disowned_list(_result,owner);
}/* VALIDATED */
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction){Mallocationowner owner=getOwner(__LINE__);
    Mmap* _result=NULL;
    if(_map){
        _result=owned_map(_getMapOfType(_map->valuetype),owner);
        Mmapelement* _mapelement=_map->_first;
        while(_mapelement&&appendedToMap(_result,owner,_mapelement->_variable->_name->chars,oneArgumentFunction(_mapelement->_variable->_value))==1)_mapelement=_mapelement->_next;
    }
    return disowned_map(_result,owner);
}/* VALIDATED */

Mbiginteger* _getRoundedRationalInteger(Mrational* _rational){Mallocationowner owner=getOwner(__LINE__);
    // we have to multiply the numerator by 2 and add the denominator
    // NO we should compare the remainder with the denominator divided by two
    // BUT if we multiply the numerator with 2 we can divide and compare with 
    // TODO ignores delta for now
    if(_rational){
        // denominator equal to 1?
        if(!_rational->den||mp_cmp(MP_INT_POINTER(_rational->den),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return disowned_biginteger(owned_biginteger(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1),owner),owner);
        // if the numerator equals 1, the result is 0
        if(!_rational->num||mp_cmp(MP_INT_POINTER(_rational->num),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return disowned_biginteger(owned_biginteger(_getBiginteger(0),owner),owner); // with the numerator at least equal to 2 the result will always be 0
        bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _dividend=NULL;
        Mbiginteger* _absnum=owned_biginteger(__biginteger(),owner); // to be freed asap
        if(_absnum){ // freeable
            if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))==MP_OKAY){
                Mbiginteger* _twicenum=owned_biginteger(__biginteger(),owner);
                if(_twicenum){ // freeable
                    if(mp_mul_2(MP_INT_POINTER(_absnum),MP_INT_POINTER(_twicenum))==MP_OKAY){
                        Mbiginteger* _twiceden=owned_biginteger(__biginteger(),owner);
                        if(_twiceden){
                            if(mp_mul_2(MP_INT_POINTER(_rational->den),MP_INT_POINTER(_twiceden))==MP_OKAY){
                                Mbiginteger* _remainder=owned_biginteger(__biginteger(),owner);
                                if(_remainder){
                                    _dividend=owned_biginteger(__biginteger(),owner);
                                    if(_dividend){
                                        bool success=(mp_div(MP_INT_POINTER(_twicenum),MP_INT_POINTER(_twiceden),MP_INT_POINTER(_dividend),MP_INT_POINTER(_remainder))==MP_OKAY);
                                        // increment _dividend if _remainder larger than denominator
                                        if(success&&mp_cmp(MP_INT_POINTER(_remainder),MP_INT_POINTER(_rational->den))==MP_GT&&mp_incr(MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
                                        if(success&&neg&&mp_neg(MP_INT_POINTER(_dividend),MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
                                        if(!success){FREE_BIGINTEGER(_dividend,owner);_dividend=NULL;}
                                    }else 
                                        outputError("Failed to create big integer dividend");
                                    FREE_BIGINTEGER(_remainder,owner);
                                }else 
                                    outputError("Failed to create big integer remainder");
                            }else
                                outputError("Failed to double big integer denominator");
                            FREE_BIGINTEGER(_twiceden,owner);
                        }
                    }else
                        outputError("Failed to double big integger numerator");
                    FREE_BIGINTEGER(_twicenum,owner); // freed!
                }else
                    outputError("Failed to create big integer");
            }else
                outputError("Failed to compute the absolute of a big integer");
            FREE_BIGINTEGER(_absnum,owner); // freed!
        }
        return disowned_biginteger(_dividend,owner);
    }
    return NULL;
}/* VALIDATED */
/*
 * \parameter floor  when true, returns the integer part of the rational, which actually is the trunc value
 * \parameter towardszero is true, returns the integer part of the rational, which actually is the trunc version
 */
Mbiginteger* _getRationalInteger(Mrational* _rational,bool floor,bool towardszero){Mallocationowner owner=getOwner(__LINE__);
    // TODO ignores delta for now
    if(_rational){
        if(!_rational->num)return NULL; // if the numerator is undefined, the rational is undefined!!!!
        if(!_rational->den)return _getBigintegerCopy(_rational->num); // cannot get it much simpler if the rational already is integer
        // ASSERT both numerator and denominator are NOT NULL
        bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _absnum=owned_biginteger(__biginteger(),owner); // to be freed asap
        if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))!=MP_OKAY)
        {FREE_BIGINTEGER(_absnum,owner);outputError("Failed to compute the absolute of a big integer");return NULL;}
        Mbiginteger *_dividend=owned_biginteger(__biginteger(),owner),*_remainder=owned_biginteger(__biginteger(),owner);
        bool success=(mp_div(MP_INT_POINTER(_absnum),MP_INT_POINTER(_rational->den),MP_INT_POINTER(_dividend),MP_INT_POINTER(_remainder))==MP_OKAY);
        if(success&&mp_iszero(MP_INT_POINTER(_remainder))!=MP_YES){ // division succeeded with a non-zero remainder
            if(neg){
                if(mp_neg(MP_INT_POINTER(_dividend),MP_INT_POINTER(_dividend))==MP_OKAY){
                    // if flooring (instead of ceiling) we have to subtract one
                    if(floor&&!towardszero&&mp_decr(MP_INT_POINTER(_dividend))!=MP_OKAY){
                        success=false;
                        outputError("Failed to decrement the truncated negative big integer");
                    }
                }else{
                    success=false;
                    outputError("Failed to negate the big integer dividend");
                }
            }else{
                if(!floor&&!towardszero&&mp_incr(MP_INT_POINTER(_dividend))!=MP_OKAY){
                    success=false;
                    outputError("Failed to increment truncated positive big integer");
                }
            }
            if(!success){FREE_BIGINTEGER(_dividend,owner);_dividend=NULL;}
        }
        FREE_BIGINTEGER(_remainder,owner);
        FREE_BIGINTEGER(_absnum,owner);
        return disowned_biginteger(_dividend,owner);
    }
    return NULL;
}

// MDH@18OCT2019: same for decimals
// TODO should we store the result in a big integer or an integer (if possible????)
//      theoretically we should return a decimal!!!
Mdecimal* _getDecimalInteger(Mdecimal* _decimal,bool floor,bool towardszero){Mallocationowner owner=getOwner(__LINE__);
    // NOTE decimals can have repeating parts BUT those repeating digits are only behind the decimal comma, so won't have effect on the integers
    // ceil: false,false / trunc: true,true / floor: true,false / ?: false,true
    if(_decimal){
        Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
        if(mpd_context){
            if(towardszero){ // always means truncing!!!
                Mdecimal* _truncDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
                if(_truncDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qtrunc(_truncDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return disowned_decimal(_truncDecimal,owner);
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to truncate decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    FREE_DECIMAL(_truncDecimal,owner);
                }
            }else
            if(floor){
                Mdecimal* _floorDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
                if(_floorDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qfloor(_floorDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return disowned_decimal(_floorDecimal,owner);
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to floor decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    FREE_DECIMAL(_floorDecimal,owner);
                }                    
            }else{
                Mdecimal* _ceilDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
                if(_ceilDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qceil(_ceilDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return disowned_decimal(_ceilDecimal,owner);
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to ceil decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    FREE_DECIMAL(_ceilDecimal,owner);
                }
            }
        }else
            outputError("No context available for converting a decimal to an integer");
    }
    return NULL;
}

Mdecimal* _getRoundedDecimal(Mdecimal* _decimal){Mallocationowner owner=getOwner(__LINE__);
    if(_decimal){
        Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
        if(mpd_context){
            Mdecimal* _roundDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
            if(_roundDecimal){
                uint32_t status=0;
                mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
                if((status&0xEFBF)==0)return disowned_decimal(_roundDecimal,owner);
                output("%s",M_ERROR_PREFIX);outputDecimal("Failed to round decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                FREE_DECIMAL(_roundDecimal,owner);
            }
        }else
            outputError("No context available for rounding a decimal");
    }
    return NULL;
}

long long isListUndefined(Mlist* list){return(list?M_FALSE:M_TRUE);}
long long isMapUndefined(Mmap* map){return(map?M_FALSE:M_TRUE);}

// MDH@24OCT2019: when representing variable values the value might match the value of a constant in which case we use the name of that constant variable instead (which is like a symbol)
//                obviously the type of the value should match as well
bool areValuesEqual(Mvalue const * const value1,Mvalue const * const value2){
    if(!value1&&!value2)return false; // if both NULL not considered to be the same
    if(value1==value2)return true; // the same value pointed to
    if(!value1||!value2)return false; // if either is NULL, not the same of course
    // ASSERT both not NULL
    if(value1->type==value2->type) // if the types are different definitely not the same
    switch(value1->type){
        case VT_INTEGER:return(value1->value._integer->ll==value2->value._integer->ll);
        case VT_FLOAT:return(areFloatsEqual(value1->value._float,value2->value._float)); // MDH@25OCT2019: replacing ldEqual() with a call to areFloatsEqual()
        case VT_BIGINTEGER:return(mp_cmp(MP_INT_POINTER(value1->value._biginteger),MP_INT_POINTER(value2->value._biginteger))==MP_EQ);
        case VT_TEXT:return(value1->value._text->presuffix==value2->value._text->presuffix&&strcmp(value1->value._text->_c,value2->value._text->_c)==0);
        case VT_TOKEN:return string_equal(value1->value._token->text,value2->value._token->text);
        case VT_LIST:case VT_MAP:break;
        case VT_DECIMAL:case VT_RATIONAL:break;
        case VT_UNDEFINED:return true; // there's only ONE undefined value around??????
        case VT_REFERENCE: // TODO this might be hard
            break;
        case VT_FUNCTION:return(value1->value._function==value2->value._function);
        case VT_ENVIRONMENT:return(strcmp(value1->value._environment->_name->chars,value2->value._environment->_name->chars)==0); // TODO we might need to use the full name of the environment here though
        case VT_FILE:return string_equal(value1->value._file->_name,value2->value._file->_name); // MDH@28SEP2020: the same if files are the same (TODO should be canonical of course)
    }
    return false;
}

// MDH@03MAR2020: moved over from Menvironment.c
void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement){
    if(_expressionlistelement){
        free_expressionlistelement(_expressionlistelement->_next);
        free_value(_expressionlistelement->_value);
        free(_expressionlistelement);
    }
}/* VALIDATED */
void free_expressionlist(Mexpressionlist* _expressionlist){
    if(_expressionlist){
        free_expressionlistelement(_expressionlist->_next);
        free_value(_expressionlist->_value);
        free(_expressionlist);
    }
}/* VALIDATED */

// NOTE typically you're not supposed to free internal functions safe M function definitions 
// TODO if a function map is freed, we shouldn't free internal functions BUT those are only present in the main environment which is never released!!
Mfunction* owned_function(Mfunction* _function,Mallocationowner owner_function){
    if(!_function)return NULL;
    /////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
    if(_function->_parameterMap)owned_map(_function->_parameterMap,Msubowner(owner_function,1)); // MDH@22OCT2020: with a parameter map possibly missing, testing might help
    if(_function->type==FT_USER)owned_userfunction(_function->functionunion._userfunction,Msubowner(owner_function,1)); // TODO ?????
    return OWNED(_function,owner_function);
}
Mfunction* disowned_function(Mfunction* _function,Mallocationowner owner_function){
    if(!_function)return NULL;
    /////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
    if(_function->_parameterMap)disowned_map(_function->_parameterMap,owner_function);
    if(_function->type==FT_USER)disowned_userfunction(_function->functionunion._userfunction,owner_function); // TODO ?????
    return DISOWNED(_function,owner_function);
}
void free_function(Mfunction* _function/*,Mallocationowner owner_function*/){
    if(!_function)return;
        /////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
    if(_function->_parameterMap)free_map(_function->_parameterMap);
    if(_function->type==FT_USER)free_userfunction(_function->functionunion._userfunction);
    FREE_1(_function,'F');
}/* VALIDATED */

Mfunctionmapelement* owned_functionmapelement(Mfunctionmapelement* _functionmapelement,Mallocationowner owner_functionmapelement){
    if(!_functionmapelement)return NULL;
    owned_functionmapelement(_functionmapelement->_next,owner_functionmapelement);
    owned_function(_functionmapelement->_function,Msubowner(owner_functionmapelement,1));
    owned_string(_functionmapelement->_name,Msubowner(owner_functionmapelement,1));
    return OWNED(_functionmapelement,owner_functionmapelement);
}
Mfunctionmapelement* disowned_functionmapelement(Mfunctionmapelement* _functionmapelement,Mallocationowner owner_functionmapelement){
    if(!_functionmapelement)return NULL;
    disowned_functionmapelement(_functionmapelement->_next,owner_functionmapelement);
    disowned_function(_functionmapelement->_function,owner_functionmapelement);
    disowned_string(_functionmapelement->_name,owner_functionmapelement);
    return DISOWNED(_functionmapelement,owner_functionmapelement);
}
void free_functionmapelement(Mfunctionmapelement* _functionmapelement){
    if(!_functionmapelement)return;
    free_functionmapelement(_functionmapelement->_next);
    free_function(_functionmapelement->_function);
    free_string(_functionmapelement->_name);
    FREE_1(_functionmapelement,'f');
}// VALIDATED
#define FREE_FUNCTIONMAPELEMENT(_functionmapelement,owner_functionmapelement) free_functionmapelement(disowned_functionmapelement(_functionmapelement,owner_functionmapelement))

Mfunctionmap* owned_functionmap(Mfunctionmap* _functionmap,Mallocationowner owner_functionmap){
    if(!_functionmap)return NULL;
    owned_functionmapelement(_functionmap->_first,Msubowner(owner_functionmap,1));
    return OWNED(_functionmap,owner_functionmap);
}
Mfunctionmap* disowned_functionmap(Mfunctionmap* _functionmap,Mallocationowner owner_functionmap){
    if(!_functionmap)return NULL;
    disowned_functionmapelement(_functionmap->_first,owner_functionmap);
    return DISOWNED(_functionmap,owner_functionmap);
}
void free_functionmap(Mfunctionmap* _functionmap){
    if(!_functionmap)return;
    free_functionmapelement(_functionmap->_first);
    FREE_1(_functionmap,'F');
}// VALIDATED
#define FREE_FUNCTIONMAP(_functionmap,owner_functionmap) free_functionmap(disowned_functionmap(_functionmap,owner_functionmap))

// MDH@20JUL2019: might never get called, wel perhaps on internal functions when it goes out of scope???????
Muserfunction* owned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction){
    if(!_userfunction)return NULL;
    owned_list(_userfunction->_bodyCommandList,Msubowner(owner_userfunction,1));
    // replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
    return OWNED(_userfunction,owner_userfunction);
}
Muserfunction* disowned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction){
    if(!_userfunction)return NULL;
    disowned_list(_userfunction->_bodyCommandList,owner_userfunction);
    // replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
    return DISOWNED(_userfunction,owner_userfunction);
}
void free_userfunction(Muserfunction* _userfunction){
    if(!_userfunction)return;
    ///////////if(_userfunction->_parameterMap)free_map(_userfunction->_parameterMap);
    // NOTE do NOT call free_value() on the body token value, instead NULL it so the reference count of the value is decremented!!!!
    free_list(_userfunction->_bodyCommandList);
    // replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
    FREE_1(_userfunction,'U');
}/* VALIDATED */
// END RELEASERS

// Menvironment stuff
Menvironment* owned_environment(Menvironment* _environment,Mallocationowner owner_environment){
    if(!_environment)return NULL;
    if(amVerboseDebugging())output("Taking over ownership environment.\n");
    owned_chars(_environment->_name,Msubowner(owner_environment,1));
    owned_map(_environment->_variableMap,Msubowner(owner_environment,1));
    return OWNED(_environment,owner_environment);
}
Menvironment* disowned_environment(Menvironment* _environment,Mallocationowner owner_environment){
    if(!_environment)return NULL;
    if(amVerboseDebugging())output("Releasing ownership environment.\n");
    disowned_chars(_environment->_name,owner_environment);
    if(amVerboseDebugging())output("Environment name ownership released.\n");
    disowned_map(_environment->_variableMap,owner_environment);
    if(amVerboseDebugging())output("Environment variable map ownership released.\n");
    // disowned_map(_environment->_functionMap);
    return DISOWNED(_environment,owner_environment);
}
void free_environment(Menvironment* _environment/*,Mallocationowner owner_environment*/){
    if(_environment){
        if(_environment->_name){
            output("Freeing environment name '%s'.\n",_environment->_name->chars);
            freeChars(_environment->_name/*,owner_environment*/);
            _environment->_name=NULL;
        }
        output("Releasing the parent.\n"); // DEBUG
        assignValue(&_environment->_parent,NULL); // MDH@03FEB2020 replacing:
        output("Releasing the execution.\n"); // DEBUG
        assignValue(&_environment->execution,NULL); // MDH@03FEB2020 replacing: _environment->_execution=NULL;
        output("Freeing the variable map.\n"); // DEBUG
        free_map(_environment->_variableMap/*,owner_environment*/);
        // free_map(_environment->_functionMap); // MDH@04MAR2020: TODO do we need this??????
        /* MDH@10JUL2019: only Menvironment has a function map!!   
           MDH@20JUL2019: NO user functions may also contain a function map, which is referenced in a user function execution environment
                          and indeed being a referenced they should not be freed (otherwise we would loose these nested functions on
                          freeing the execution environment)
        if(_environment->_functionMap)free_functionmap(_environment->_functionMap);
        */
        FREE_1(_environment,'E'/*,owner_environment*/);
    }
}/* VALIDATED */
Menvironment* __environment(){Mallocationowner owner=getOwner(__LINE__);
    Menvironment* _environment=CALLOC_1(sizeof(Menvironment),'E',owner);
    if(!_environment){outputError("Failed to create an environment");return NULL;}
    _environment->_variableMap=CALLOC_1(sizeof(Mmap),'M',Msubowner(owner,1)); // ascertain that the environment contains a variable map
    if(!_environment->_variableMap){FREE_ENVIRONMENT(_environment,owner);_environment=NULL;outputError("Failed to create the new environment variable map");}
    return disowned_environment(_environment,owner);
}/* VALIDATED */

Menvironment* getEnvironmentParent(Menvironment* _environment){
    return(_environment&&_environment->_parent?getValueEnvironment(_environment->_parent):NULL);
}/* VALIDATED */
Mstring* _getEnvironmentName(Menvironment* _environment){Mallocationowner owner=getOwner(__LINE__);
    Mstring* _environmentName=owned_string(__string(),owner);
    if(_environmentName){
        Mstring* p=_environmentName;
        while(p&&_environment){
            if(string_length(p)>0)p=string_insert_char(p,0,'.');
            /////// printf("Prepending '%s'.\n",_environment->_name);
            if(_environment->_name)p=string_prepend(p,_environment->_name->chars);
            _environment=getEnvironmentParent(_environment); // MDH@03MAR2020 replacing: _environment->_parent;
        }
        if(!p){FREE_STRING(_environmentName,owner);_environmentName=NULL;}
    }
    return disowned_string(_environmentName,owner);
}

// additional function for wrapping environments and functions
Mvalue* _getValueOfFunction(Mfunction* _function/*,Mallocationowner owner_function*/){
    if(!_function)return NULL;
    bool disowned_function=Misdisowned(_function);
    if(amVerbose())
        output("Wrapping a %s function.\n",(disowned_function?"disowned":"owned"));
    Mvalue* _value=__value("function");
    if(!_value){
        if(disowned_function)free_function(_function);
        return NULL;
    }
    if(amVerbose())
        outputInfo("Binding the function to the value");
    _value->type=VT_FUNCTION;
    _value->value._function=(disowned_function?owned_function(_function,owner_value_data):_function);
    if(amVerbose())
        output("%s function wrapped.\n",(disowned_function?"Disowned":"Owned"));
    return _value;
}/* VALIDATED */

Mvalue* _getValueOfEnvironment(Menvironment* _environment/*,Mallocationowner owner_environment*/){
    if(!_environment)return NULL;
    Mvalue* _value=__value("environment");
    if(!_value){
        if(Misdisowned(_environment))free_environment(_environment);
        return NULL;
    }
    _value->type=VT_ENVIRONMENT;
    // MDH@12JUN2020: TODO supposedly this is a bit of a problem actually taking over the ownership of an environment completely
    _value->value._environment=(Misdisowned(_environment)?owned_environment(_environment,owner_value_data):_environment);
    return _value;
}/* VALIDATED */

// Mfile support
#include "unistd.h"
#include "time.h"
Mmap* getFilePropertyMap(Mfile* _file){Mallocationowner owner=getOwner(__LINE__);

    if(_file){

        Mmap* _map=owned_map(_getMapOfType(VT_UNDEFINED),owner);

        if(_map){
            
            if(_file->_name){
                Mstring* _filename=owned_string(_getString("'"),owner);
                if(_filename){
                    string_append(_filename,string(_file->_name));
                    appendedToMap(_map,owner,"name",_getTextValue(string(_filename)));
                    FREE_STRING(_filename,owner);
                }
            }

            if(_file->_f){ // the file is currently open
                // show the mode the file was opened in
                Mstring* _openmode=owned_string(_getString("'"),owner);
                if(_openmode){
                    if(_file->mode[0])string_append_char(_openmode,_file->mode[0]);
                    if(_file->mode[1])string_append_char(_openmode,_file->mode[1]);
                    if(_file->mode[2])string_append_char(_openmode,_file->mode[2]);
                    appendedToMap(_map,owner,"mode",_getTextValue(string(_openmode)));
                    FREE_STRING(_openmode,owner);
                }
                fpos_t filepos;fgetpos(_file->_f,&filepos);
                Minteger* _integer=owned_integer(_getInteger(filepos),owner);
                appendedToMap(_map,owner,"position",_getValueOfInteger(disowned_integer(_integer,owner)));
            } 

            struct stat* stats=_file->_stat;

            if(stats){

                struct tm dt;

                // File permissions
                Mstring* _access=owned_string(_getString("'"),owner);

                // File access property
                if(S_ISDIR(stats->st_mode))string_append_char(_access,'d'); // TODO we might need to consider other types as well like links, devices and the like
                if(stats->st_mode & R_OK)string_append_char(_access,'r');
                if(stats->st_mode & W_OK)string_append_char(_access,'w');
                if(stats->st_mode & X_OK)string_append_char(_access,'x');
                appendedToMap(_map,owner,"access",_getTextValue(string(_access)));
                FREE_STRING(_access,owner);

                // File size property
                Minteger* _integer=owned_integer(_getInteger(stats->st_size),owner);
                appendedToMap(_map,owner,"size",_getValueOfInteger(disowned_integer(_integer,owner)));

                // Get file creation time in seconds and convert seconds to date and time format
                dt = *(gmtime(&stats->st_ctime));
                Mstring* _created=owned_string(__string(),owner);
                if(string_setlength(_created,50))string_setlength(_created,strftime(_created->_chars->chars,50,"%Y-%m-%d %H:%M:%S",&dt));
                string_insert_char(_created,0,'\'');
                appendedToMap(_map,owner,"created",_getTextValue(string(_created)));
                FREE_STRING(_created,owner);
                // from: printf("\nCreated on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900,dt.tm_hour, dt.tm_min, dt.tm_sec);

                // File modification time
                dt = *(gmtime(&stats->st_mtime));
                Mstring* _modified=owned_string(__string(),owner);
                if(string_setlength(_modified,50))string_setlength(_modified,strftime(_modified->_chars->chars,50,"%Y-%m-%d %H:%M:%S",&dt));
                string_insert_char(_modified,0,'\'');
                appendedToMap(_map,owner,"modified",_getTextValue(string(_modified)));
                FREE_STRING(_modified,owner);
            }

             // from: printf("\nModified on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, dt.tm_hour, dt.tm_min, dt.tm_sec);
            return disowned_map(_map,owner);
        
        }
    }
    return NULL;

}

// MDH@02OCT2020: when opening a file check whether the file is readable or writeable depending on the opening mode
bool isFileReadable(Mfile* _file){
    if(_file){
        if(_file->_f)return(_file->mode[1]=='+'||_file->mode[0]!='w'); // an open file is readable if it can be read from
        // an unopened file is readable when it exists, is not a directory and has the 'r' access flag set
        // I suppose an open file is also readable when it has not been opened in write-only mode
        return(_file->_stat&&!S_ISDIR(_file->_stat->st_mode)&&_file->_stat->st_mode&R_OK);
    }
    return false;
}
bool isFileWriteable(Mfile* _file){
    if(_file){
        if(_file->_f)return(_file->mode[1]=='+'||_file->mode[0]!='r');
        return(_file->_stat&&!S_ISDIR(_file->_stat->st_mode)&&_file->_stat->st_mode&W_OK);
    }
    return false;
    // a file is writeable when it exists, is not open yet, is not a directory and has the 'w' access flag set
}
Mvalue* _getValueOfFile(Mfile* _file){
    if(!_file)return NULL;
    Mvalue* _value=__value("file");
    if(!_value){
        if(Misdisowned(_file))free_file(_file);
        return NULL;
    }
    _value->type=VT_FILE;
    // MDH@12JUN2020: TODO supposedly this is a bit of a problem actually taking over the ownership of an environment completely
    _value->value._file=(Misdisowned(_file)?owned_file(_file,owner_value_data):_file);
    return _value;
}
Mvalue* mfile(Mvalue* filename_value){Mallocationowner owner=getOwner(__LINE__);
    if(filename_value!=NULL){
        Mstring* _filename=owned_string(_getValueText(filename_value,true),owner); // supposedly always a copy of the input argument so we can bind it to _file in _file->_name
        if(_filename){
            Mfile* _file=owned_file(__file(),owner); // get an owned new file instance
            // the file might not exist in which case we could get rid of _file->_stat???
            if(_file){
                _file->_name=SUBOWNED(_filename,1); // bind _filename to _file->_name (ownership one level down)
                if(stat(string(_filename),_file->_stat)!=0){FREE_1(_file->_stat,'f');_file->_stat=NULL;}
                return _getValueOfFile(disowned_file(_file,owner));
            }
            free_string(_filename); // not bound to _file->_name so to be released
        }
    }
    return NULL;
}
// delete a file
Mvalue* mfdelete(Mvalue* file_value){
    if(file_value!=NULL&&file_value->type==VT_FILE){
        // can only delete an existing file that is not currently open
        Mfile* _file=file_value->value._file;
        if(_file&&_file->_name&&!_file->_f){ // a file with a name that is not currently open
            int success=remove(string(_file->_name));
            if(success)return _getIntegerValue(M_FALSE); // if success is not zero return failure
            // update stat accordingly, if we succeed we should free _stat
            if(stat(string(_file->_name),_file->_stat)==0){FREE_1(_file->_stat,'f');_file->_stat=NULL;}
            return _getIntegerValue(M_TRUE);
        }
    }
    return NULL;
}
// opening a file might mean that afterwards the file exists, and we then should update _file->_stat accordingly!!!
static void openFile(Mfile* _file,char* mode){
    // only open when defined and currently not open
    if(_file&&mode){ // valid input
        if(!_file->_f){ // not opened yet
            if(!_file->_stat||!S_ISDIR(_file->_stat->st_mode)){ // never try to open a directory (TODO perhaps we should not try to open other things here as well)
                _file->_f=fopen(string(_file->_name),mode);
                if(_file->_f){ // now opened
                    _file->mode[0]=mode[0];_file->mode[1]=mode[1];_file->mode[2]=mode[2]; // register the opening mode (which consists of exactly three characters)
                    if(stat(string(_file->_name),_file->_stat)!=0){FREE_1(_file->_stat,'f');_file->_stat=NULL;} // update stat (even if already set, because the file existed to start with)
                }
            }
        }
    }
}
// things we can do with a Mfile
Mvalue* mfopen(Mvalue* file_value,Mvalue* mode_value){Mallocationowner owner=getOwner(__LINE__);
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file){ // there's a Mfile 
        Mstring* mode_str=owned_string(_getString("'"),owner);
        if(mode_str){
            if(!_file->_f){
                // how about checking whether the mode_value argument is correct first??????
                char openmode=(mode_value?(mode_value->type==VT_TEXT?mode_value->value._text->_c[0]:'\0'):(_file->_stat?'r':'w')); // if mode_value is defined, it must be of type VT_TEXT, and the first character should be 'a', 'r' or 'w' to be a valid mode
                // let's NOT allow overwriting an existing file!!!
                if(openmode=='w'||openmode=='r'||openmode=='a'){ // a valid open mode
                    if(openmode!='w'||_file->_stat==NULL){ // but do NOT allow deleting existing content
                        // let's initialize the default mode
                        char mode[4]={openmode}; // initialize mode to openmode NOTE mode always needs to end with '\0'
                        if(mode_value){ // accepting two additional mode characters '+' and 'b'
                            // let's simply copy the characters (even if they are wrong)
                            mode[1]=mode_value->value._text->_c[1];
                            if(mode[1])mode[2]=mode_value->value._text->_c[2];
                        }else // for optimal flexibility allow for writing as well
                            mode[1]='+';
                        // try to open the file
                        openFile(_file,mode); 
                        /* replacing and augmenting:
                        _file->_f=fopen(string(_file->_name),mode);
                        if(_file->_f){_file->mode[0]=mode[0];_file->mode[1]=mode[1];_file->mode[2]=mode[2];} // remember the opening mode when the file was successfully opened
                        */
                    }
                }
            }
            if(_file->_f){ // the file is (now) open
                if(_file->mode[0])string_append_char(mode_str,_file->mode[0]);
                if(_file->mode[1])string_append_char(mode_str,_file->mode[1]);
                if(_file->mode[2])string_append_char(mode_str,_file->mode[2]);
            }
            Mvalue* result=_getTextValue(string(mode_str));
            FREE_STRING(mode_str,owner);
            return result;
        }
    }
    return NULL;
}
/*
// reading from a file by specifying the number of bytes to read
static void openFileForReadingText(Mfile* _file){
    // ASSERT _file should NOT be NULL and _file->_stat should not be NULL (i.e. it's an existing file) and _file->_f should be NULL
    // if the file mode has not been set yet, or if it has been set to something that is supposedly readable
    if(_file&&!_file->_f){ // defined, but not open yet
        if(_file->mode[0]=='\0'||_file->mode[0]!='w'||_file->mode[1]=='+'){
            _file->_f=fopen(string(_file->_name),(_file->mode[0]?_file->mode:"r+")); // an array is a pointer!!!!!
            if(_file->_f)if(!_file->mode[0]){_file->mode[0]='r';_file->mode[1]='+';} // if the file mode was not initialized, use r+ as access mode
        }
    }
}*/
// to allow for continued reading we should keep track of the position in the file?????? I guess we can keep a reference to the FILE pointer I suppose
Mvalue* mfread(Mvalue* file_value,Mvalue* numberofbytes_value){Mallocationowner owner=getOwner(__LINE__);
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file){
        long long numberofbytes=(numberofbytes_value?getValueInteger(numberofbytes_value):1); // the default is to read a single byte
        if(numberofbytes>=0){ // only non-negative values are considered valid
            // before checking the mode to see if the file can be read from, we might need to open it
            // it's easiest to check whether it exists to start with, because if it doesn't it can't be read from anyway
            if(_file->_stat){ // an existing file
                // if the file wasn't opened before, try to open it for reading 'text' (i.e. not binary)
                if(!_file->_f)openFile(_file,"r+");
                // if the file can be read from, we do
                if(_file->mode[1]=='+'||_file->mode[0]=='a'||_file->mode[0]=='r'){ // the mode is defined (i.e. unequal to it's initial value '\0')
                    Mstring* _bytesread=owned_string(_getString("'"),owner); // start the string with a single quote for create a Mtext from it
                    if(_bytesread){
                        Mstring* p=_bytesread;
                        // the file could be empty to start with
                        while(numberofbytes!=0&&!feof(_file->_f)){
                            p=string_append_char(p,fgetc(_file->_f));
                            if(!p)break;
                            if(numberofbytes>0)numberofbytes--;
                        }
                        Mvalue* result=(p?_getTextValue(string(_bytesread)):NULL); // an immutable version of the bytes obtained
                        FREE_STRING(_bytesread,owner);
                        return result;
                    }
                }
            }
        }
    }
    return NULL; // some error
}
Mvalue* mfreadline(Mvalue* file_value){Mallocationowner owner=getOwner(__LINE__); // reads all bytes until a new line character is encountered 
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file){
        if(_file->_stat){ // an existing file
            if(!_file->_f)openFile(_file,"r+");
            // if the file is not binary and can be read from
            if(_file->mode[2]!='b'&&(_file->mode[1]=='+'||_file->mode[0]=='a'||_file->mode[0]=='r')){ // the mode is defined (i.e. unequal to it's initial value '\0')
                Mstring* _bytesread=owned_string(_getString("'"),owner); // start the string with a single quote for create a Mtext from it
                if(_bytesread){
                    Mstring* p=_bytesread;
                    // the file could be empty to start with
                    char c='\0';
                    while(!feof(_file->_f)){
                        c=fgetc(_file->_f);
                        if(c=='\n'||c=='\r')break; // either LF or CR would stop the reading
                        p=string_append_char(p,c);
                        if(!p)break;
                    }
                    // skip the optional linefeed following any carriage return, if something else push back again
                    if(c=='\r')if(!feof(_file->_f)){c=fgetc(_file->_f);if(c!='\n')ungetc(c,_file->_f);}
                    Mvalue* result=(p?_getTextValue(string(_bytesread)):NULL); // an immutable version of the bytes obtained
                    FREE_STRING(_bytesread,owner);
                    return result;
                }
            }
        }
    }
    return NULL; // some error
}
// MDH@01OCT2020: how about allowing to read a number of lines in one go??????
Mvalue* mfreadlines(Mvalue* file_value,Mvalue* numberoflines_value){Mallocationowner owner=getOwner(__LINE__);
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file&&_file->_stat&&!S_ISDIR(_file->_stat->st_mode)){ // an existing (non directory) file
        long long numberoflines=(numberoflines_value?getValueInteger(numberoflines_value):1); // the default is to read a single byte
        if(numberoflines>=0){ // only non-negative values are considered valid
        Mlist* lines_list=owned_list(_getListOfType(VT_TEXT),owner);
        if(lines_list){
            // before checking the mode to see if the file can be read from, we might need to open it
            // it's easiest to check whether it exists to start with, because if it doesn't it can't be read from anyway
                // if the file wasn't opened before, try to open it for reading 'text' (i.e. not binary)
                if(!_file->_f)openFile(_file,"r+");
                // if the file can be read from, we do
                if(_file->mode[1]=='+'||_file->mode[0]=='a'||_file->mode[0]=='r'){ // the mode is defined (i.e. unequal to it's initial value '\0')
                    if(_file->mode[2]!='b'){
                        long long lineindex=0;
                        char buffer[128]; // the buffer to use with fgets
                        size_t line_length;
                        bool eoln;
                        while(!feof(_file->_f)){ // there are still additional lines
                            Mstring* _line=owned_string(_getString("'"),owner);
                            buffer[0]='\0'; // ascertain for the buffer to have length 0 when we start
                            lineindex=0;
                            // keep reading until all of the line is read (or some error occurs)
                            while(fgets(buffer,128,_file->_f)!=NULL){
                                // the problem is that buffer might not end with a new line
                                line_length=strlen(buffer);
                                if(line_length==0)break; // if nothing was read we're done (technically won't happen as fgets will also append the end of line!!!)
                                eoln=(buffer[line_length-1]=='\n');
                                if(eoln)buffer[line_length-1]='\0';
                                string_append(_line,buffer); // append the buffer to _line
                                if(eoln){
                                    lineindex=appendedToList(lines_list,owner,_getTextValue(string(_line)),M_LL_INVALID);
                                    break;
                                }
                            }
                            FREE_STRING(_line,owner);
                            if(lineindex<=0)break; // either an error or failed to append the line to the list!!!!
                        }
                        return _getValueOfList(disowned_list(lines_list,owner));
                    }
                }
            }
        }
    }
    return NULL;
}

Mvalue* mfclose(Mvalue* file_value){
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file&&_file->_f){
        if(fclose(_file->_f)==0){_file->_f=NULL;return _getIntegerValue(M_TRUE);}return _getIntegerValue(M_FALSE);
    }
    return _getIntegerValue(M_LL_INVALID);
}
/*
static void openFileForWriting(Mfile* _file){
    // ASSERT _file should NOT be NULL and _file->_f should be NULL (but _file->stat might be NULL)
    // if the file mode has not been set yet, or if it has been set to something that is supposedly readable
    if(_file->mode[0]=='\0'||_file->mode[0]!='w'||_file->mode[1]=='+'){
        _file->_f=fopen(string(_file->_name),(_file->mode[0]?_file->mode:"r+")); // an array is a pointer!!!!!
        if(_file->_f)if(!_file->mode[0]){_file->mode[0]='r';_file->mode[1]='+';} // if the file mode was not initialized, use r+ as access mode
    }
}*/
Mvalue* mfwrite(Mvalue* file_value,Mvalue* write_value){Mallocationowner owner=getOwner(__LINE__); // reads all bytes until a new line character is encountered 
    // how about returning the number of bytes NOT written...
    Mfile* _file=(file_value&&file_value->type==VT_FILE?file_value->value._file:NULL);
    if(_file){ // something to write to
        if(write_value&&write_value->type==VT_TEXT&&write_value->value._text->_c[0]){ // something to write
            if(!_file->_f)openFile(_file,"w+"); // TODO I guess opening explicitly for writing seems to be the right choice
            if(_file->mode[1]=='+'||_file->mode[0]=='a'||_file->mode[0]=='w'){ // the mode is defined (i.e. unequal to it's initial value '\0')
                long long notwritten=0;
                char* p=write_value->value._text->_c; // _c is an array so a pointer
                size_t towrite=strlen(p);
                if(towrite>0){
                    if(_file->mode[2]=='b'){ // binary write
                        notwritten=towrite-fwrite(p,sizeof(char),strlen(p),_file->_f);
                    }else{ // text write (i.e. as characters)
                        // in case there are escape sequences in the text, we need to resolve these which _getStringText() does
                        Mstring* _towrite=_getStringText(write_value->value._text,true);
                        if(_towrite){
                            char* p=string(_towrite);
                            while(*p){if(fputc(*p,_file->_f)==EOF)break;p++;}
                            while(*p){notwritten++;p++;}
                        }else
                            notwritten=towrite;
                    }
                }
                return _getIntegerValue(notwritten);
            }
        }
    }
    return _getIntegerValue(M_LL_INVALID);
}

Mvalue* mfiles(Mvalue* file_value){Mallocationowner owner=getOwner(__LINE__);
    Mfile* _file=(file_value?file_value->value._file:NULL);
    if(_file&&_file->_name&&_file->_stat&&S_ISDIR(_file->_stat->st_mode)){ // the file is a directory
        char* directoryname=string(_file->_name);
        DIR* dr=(directoryname?opendir(directoryname):NULL);
        if(dr){
            Mlist* files_list=owned_list(_getListOfType(VT_TEXT),owner);
            struct dirent *en;
            while((en=readdir(dr))!=NULL){
                Mstring* _filename=owned_string(_getString("'"),owner);
                if(_filename){
                    // let's NOT prepend the directory name!!!! string_append(_filename,directoryname);
                    string_append(_filename,en->d_name);
                    appendedToList(files_list,owner,_getTextValue(string(_filename)),M_LL_INVALID);
                    FREE_STRING(_filename,owner);
                }
            }
            closedir(dr); //close all directory
            return _getValueOfList(disowned_list(files_list,owner));
        }
   }
   return NULL;
}
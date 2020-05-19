#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "Mvalue.h"

static int32_t const MODULE_ID=(13<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_POSITIVE,M_NEGATIVE,M_ZERO;
extern const char * const VALUETYPENAMES[]; // the characters associated with each of the value types
extern const char * const MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const M_ERROR_PREFIX;
extern const char * const M_WARNING_PREFIX;
extern const char * const M_NULL_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value that is NULL
extern const char * const M_UNDEFINED_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value of type VT_UNDEFINED
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const long double LD_PI; // for Mfacd()
extern Mdecimalcontext * const M_DECIMALCONTEXT; // the application-wide (default) decimal context
extern const char M_DEREFERENCE_CHARACTER; // MDH@11MAR2020

void free_variable(Mvariable* _variable,bool weak,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(1));
    if(_variable->_name)free(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
    if(!weak)if(_variable->_value)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
    FREE(_variable,'V',foid);
}/* VALIDATED */
Mvariable* _getVariable(const char* name,Mvaluetype valuetype,bool immutable,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(1));
    // MDH@14NOV2019: maps might have attributes with no name (i.e. the empty string)
    if(!name){
        outputError("No variable name defined");
        return NULL;
    }
    Mvariable* _variable=(Mvariable*)CALLOC(sizeof(Mvariable),'V',foid); // all pointers will be NULL!!
    if(!_variable){
        outputErrorAndText("Failed to allocate memory to store variable ",name);
        return NULL;
    }
    _variable->immutable=immutable;
    _variable->_name=_getChars(name,foid); // MDH@17APR2020 _strdup() replaced by _getChars(): // create a dynamic pointer on the heap
    if(!_variable->_name){
        free_variable(_variable,true,foid);
        output("%sFailed to allocate memory to store name '%s' of the new variable.\n",M_ERROR_PREFIX,name);
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
            case VT_FLOAT:break;
            case VT_TEXT:_variable->_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
            case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
            case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
        }
    }else
        _variable->_value=NULL;
    */
    return(oid>0?_variable:DISOWNED(_variable,sizeof(Mvariable),foid));
}/* VALIDATED */

bool free_listelement(Mlistelement* _listelement,bool weak,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(2));
    if(OWNED(_listelement,sizeof(Mlistelement),foid)){
        if(_listelement->_next){free_listelement(_listelement->_next,weak);_listelement->_next=NULL;}
        if(_listelement->_value){if(!weak)decrementReferenceCount(_listelement->_value);_listelement->_value=NULL;} ///////// replacing: free_value(_listelement->_value);
        FREE(_listelement,'l',foid);
        return true;
    }
    return false;
}/* VALIDATED */
Mlist* __list(char* source,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(3));
    Mlist* _list=CALLOC(sizeof(Mlist),'L',foid);
    if(source){
        _list->_creator=OWNED(_getChars(source),strlen(source)+1,foid); // MDH@17APR2020 replacing: _strdup(source);
        if(_list->_creator){
            if(amVerboseDebugging())output("List creator: '%s'.\n",_list->_creator->chars);
        }else
            output("%sFailed to register list creator '%s'.\n",M_ERROR_PREFIX,source);
    }
    return _list;
}
void free_list(Mlist* _list,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(3));
    if(amVerboseDebugging())
    {output("Freeing a %s list",_list->weak?"weak":"strong");if(_list->_creator)output(" created by '%s'",_list->_creator->chars);outputChar('.');outputChar('\n');}
    if(_list->_first){
        // MDH@17APR2020: assuming we allocated exactly the number of characters for storing the characters
        if(_list->_creator)freeChars(_list->_creator,len(_list->_creator->chars)+1,foid);// MDH@17APR2020 replacing: FREE(_list->_creator,'"');
        free_listelement(_list->_first,_list->weak,foid);
        _list->_first=NULL;
    }
    FREE(_list,'L',foid);
}/* VALIDATED */

bool free_mapelement(Mmapelement* _mapelement,bool weak,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(4));
    if(amVerbose()&&amDebugging())output("About to free a %s map attribute!\n",(weak?"weak":"strong"));
    if(_mapelement->_next){
        if(!free_mapelement(_mapelement->_next,weak))outputError("Failed to free a map element!");//////else outputInfo("Next map element freed!");
        _mapelement->_next=NULL;
    }
    if(_mapelement->_variable){
        if(_mapelement->_variable->_name){
            if(amVerboseDebugging())output("About to free %s map attribute '%s'.\n",(weak?"weak":"strong"),_mapelement->_variable->_name);
        }else
            outputWarning("Unnamed map attribute!");
        free_variable(_mapelement->_variable,weak,foid);
        _mapelement->_variable=NULL; // MDH@11NOV2019: for safety purposes (won't wanna try it again)
    }else
        outputWarning("No map attribute to free!");
    FREE(_mapelement,'m',foid);
    if(amVerboseDebugging())outputInfo("\tMap element freed!");
    return true;
}/* VALIDATED */
void free_map(Mmap* _map,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(5));
    if(amVerboseDebugging())output("About to free a (%s) map with %llu attributes!\n",(_map->weak?"weak":"strong"),_map->numberOfElements);
    if(_map->_first){free_mapelement(_map->_first,_map->weak,foid);_map->_first=NULL;}else outputInfo("No map attributes to free!");
    FREE(_map,'M',foid);
}/* VALIDATED */

// MDH@26OCT2019: when freeing a value reference we NULL the fields just in case (TODO why?)
void free_valuereference(Mvaluereference* _valuereference,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(7));
    if(_valuereference->_name){freeChars(_valuereference->_name);_valuereference->_name=NULL;}
    /* MDH@02NOV2019: all values now 'weak' assigned i.e. no need to dereference anymore
    if(_valuereference->_value){assignValue(&_valuereference->_value,NULL);_valuereference->_value=NULL;} // get rid of the reference
    */
    if(_valuereference->_itemid){assignValue(&_valuereference->_itemid,NULL);_valuereference->_itemid=NULL;}
    FREE(_valuereference,'5',foid); // MDH@19NOV2019: type changed from @ to 5 (See M.c for the allocations)
}/* VALIDATED */

// manage a list of created values
// if we make a map out of it, we can annote the value with a name????
Mlist* _valueList=NULL;
Mvalue* __value(char const * const descriptor,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(10));
    Mvalue* _value=NULL;
    if(!_valueList)_valueList=__list("global value list",getOwnerId(0)); // MDH@19MAY2020: _valueList is global and we use 0 as function owner id (which is the rule for module global variables)
    if(_valueList){
        _valueList->weak=true; // MDH@11NOV2019: don't think this actually matters, as I'm the only one that accesses it and the list will be around for the remainder of the session!!!
        Mlistelement* _valueListelement=(Mlistelement*)CALLOC(sizeof(Mlistelement),'l',foid); // both pointers NULL
        if(_valueListelement){
            _value=(Mvalue*)CALLOC(sizeof(Mvalue),'X',-foid); // MDH@07APR2020: should be 'X' not 'Y' // MDH@18MAY2020: immediately disown the Mvalue, because we return it 
            if(_value){
                // shouldn't pose a problem now...
                _valueListelement->_value=_value;
                if(_valueList->_last)_valueList->_last->_next=_valueListelement;else _valueList->_first=_valueListelement;
                _valueList->_last=_valueListelement;
                _valueList->numberOfElements++;
                // MDH@11NOV2019: by remembering the number of elements as index, removing intermediate elements will NOT prevent informing about what element was removed!!!
                _valueListelement->index=_valueList->numberOfElements;
                if(descriptor)if(amVerbose()&&amDebugging())output("Descriptor of value with id #%llu: '%s'.\n",_valueListelement->index,descriptor);
            }else // couldn't get a new value, so free the value list element immediately
                FREE(_valueListelement,'l',foid);
        }
    }
    if(!_value)outputError("Failed to create value!"); // serious enough to report
    return(oid>0?_value:DISOWNED(_value,sizeof(Mvalue),foid));
}/* VALIDATED */
// MDH@01MAY2019: 'local' function for freeing a value
void free_value(Mvalue* _value,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(10));
    //////if(amVerbose()){output("Value of type '%s'",VALUETYPENAMES[_value->type]);outputValue(" to free: '",_value,"'.\n");}
    // I do not need to free the value itself, only the pointers inside it
    if(_value){
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_TOKEN:if(_value->value._token){free_token(_value->value._token,foid);_value->value._token=NULL;}break;
            case VT_INTEGER:if(_value->value._integer){free_integer(_value->value._integer,foid);_value->value._integer=NULL;}break;
            case VT_BIGINTEGER:if(_value->value._biginteger){free_biginteger(_value->value._biginteger,foid);_value->value._biginteger=NULL;}break;
            case VT_DECIMAL:if(_value->value._decimal){free_decimal(_value->value._decimal,foid);_value->value._decimal=NULL;}break;
            case VT_RATIONAL:if(_value->value._rational){free_rational(_value->value._rational,foid);_value->value._rational=NULL;}break;
            case VT_FLOAT:if(_value->value._float){free_float(_value->value._float,foid);_value->value._float=NULL;}break;
            case VT_TEXT:if(_value->value._text){free_text(_value->value._text,foid);_value->value._text=NULL;}break;
            case VT_LIST:if(_value->value._list){free_list(_value->value._list,foid);_value->value._list=NULL;}break;
            case VT_MAP:if(_value->value._map){free_map(_value->value._map,foid);_value->value._map=NULL;}break;
            case VT_REFERENCE:if(_value->value._reference){free_reference(_value->value._reference,foid);_value->value._reference=NULL;}break; // MDH@04NOV2019: decrement the reference count to the variable
            case VT_FUNCTION:if(_value->value._function){free_function(_value->value._function,foid);_value->value._function=NULL;}break;
            case VT_ENVIRONMENT:if(_value->value._environment){free_environment(_value->value._environment,foid);_value->value._environment=NULL;}break;
            //case VT_USERFUNCTION:if(_value->value._userfunction)free_userfunction(_value->value._userfunction);break;
        }
        FREE(_value,'X',foid);
        if(amVerboseDebugging())output("\tValue of type '%s' freed.\n",VALUETYPENAMES[_value->type]);
    }else
        outputBug("No value to free!");
}/* VALIDATED */


extern const char* const VALUETYPENAMES[];

// can be asked to remove unused values
// TODO check whether it functions correctly (think so though)
size_t getNumberOfRemovedValues(bool showInfo){int32_t foid=getOwnerId(11);
    unsigned long long tofree=0,removed=0;
    if(_valueList){
        if(showInfo)output("Garbage collecting unused values.\n");
        Mlistelement* _valueListelement=_valueList->_first;
        if(showInfo)output("Number of values to check: %llu.\n",_valueList->numberOfElements); // MDH@11NOV2019: no longer the actual number of elements to check
        unsigned long long checked=0;
        while(_valueListelement){
            checked++;
            if(_valueListelement->_value){
                if(showInfo){output("Checking value #%llu ",checked);outputValue("(",_valueListelement->_value,")");output(" with id %llu.\n",_valueListelement->index);}
                // if(showInfo)outputInfo("\tChecking the count!");
                if(_valueListelement->_value->count==0){ // unused
                    if(showInfo)output("\tAbout to free unused value #%llu of type '%s'.\n",checked,VALUETYPENAMES[_valueListelement->_value->type]);
                    free_value(_valueListelement->_value);
                    _valueListelement->_value=NULL; // just in case
                    tofree++;
                }else
                if(showInfo)outputInfo("\tStill in use!");
            }else
                output("%sNo value stored in value #%llu.\n",M_ERROR_PREFIX,checked);
            _valueListelement=_valueListelement->_next;
        }
        if(showInfo)output("Number of values checked: %llu.\nNumber of value list elements to free: %llu.\n",checked,tofree);
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
                    /* MDH@11NOV2019: let's decide NOT to decrement numberOfElements meaning that we NOW use numberOfElements to always have a unique index for every value ever added to it!!!
                    if(_valueList->numberOfElements>0)_valueList->numberOfElements--;else output("BUG: Trying to free a value list element that is not counted!");  // one less element in the list!!!
                    */
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
        if(tofree>removed)output("%sFailed to free %llu unused value list elements.\n",M_WARNING_PREFIX,(tofree-removed));else 
        if(showInfo)outputInfo("All unused value list elements freed!");
    }
    return removed;
}/* VALIDATED */

unsigned long long getNumberOfValues(){
    int32_t foid=getOwnerId(12);
    return (_valueList?_valueList->numberOfElements:0);
}/* VALIDATED */

bool decrementReferenceCount(Mvalue* _value){int32_t foid=getOwnerId(13);
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
        Mstring* _valueText=_getValueText(_value,false,foid);
        output("BUG: Reference count of '%s' of type '%c' already zero.\n",string(_valueText),MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
        free_string(_valueText,foid);
    }else
    if(amVerbose())outputInfo("No value to decrement the reference count of.");
    return true;
}/* VALIDATED */
bool incrementReferenceCount(Mvalue* _value){int32_t foid=getOwnerId(14);
    if(_value){
        (_value->count)++;
        return true;
    }
    if(amVerbose())outputInfo("No value to increment the reference count of.");
    return false;
}/* VALIDATED */
// interface functions that use the above functions
// wrapping the different value type instances
Mvalue* _getUndefinedValue(int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(15));
    Mvalue* _undefinedValue=(Mvalue*)CALLOC(sizeof(Mvalue),'U',foid);
    return(oid>0?_undefinedValue:DISOWNED(_undefinedValue,sizeof(Mvalue),foid));
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
Mreference* _getReference(Mvariable* variable,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(16));
    // MDH@11MAR2020: variable can now be NULL
    Mreference* _reference=CALLOC(sizeof(Mreference),'Q',foid);
    if(_reference){_reference->variable=variable;if(variable)_reference->referenceindex=(++variable->referencecount);} // MDH@11MAR2020: if variable is undefined, no reference count we can increment and assign
    return(oid>0?_reference:DISOWNED(_reference,sizeof(Mreference),foid));
}
void free_reference(Mreference* reference,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(16));
    if(!reference)return;
    if(reference->variable)reference->variable->referencecount--;
    FREE(reference,'Q',foid);
}
Mvalue* _getReferenceValue(Mreference* _reference,bool freeonfailure,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(17));
    // MDH@19MAY2020: should we check whether _reference is ownable???????
    if(!_reference){outputWarning("No reference to wrap.");return NULL;}
    Mvalue* _referenceValue=__value("reference",foid);
    if(_referenceValue){
        _referenceValue->type=VT_REFERENCE;
        // I suppose the value should become the owner of the reference!!
        _referenceValue->value._reference=OWNED(_reference,sizeof(Mreference),foid);
    }
    if(!_referenceValue||!_referenceValue->value._reference)if(freeonfailure)free_reference(_reference,foid);
    return(oid>0?_referenceValue:DISOWNED(_referenceValue,sizeof(Mreference),foid));
}

Mvalue* _getDecimalValue(Mdecimal* _decimal,bool freeonfailure,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(18));
    if(!_decimal){outputInfo("No decimal to wrap.");return NULL;}
    Mvalue* _decimalValue=__value("decimal",foid);
    //////////outputDecimal("Wrapping decimal '",_decimal,"'.\n");
    if(_decimalValue){
        _decimalValue->type=VT_DECIMAL;
        _decimalValue->value._decimal=OWNED(_decimal,sizeof(Mdecimal),foid);
    }
    if(!_decimalValue||!_decimalValue->value._decimal)if(freeonfailure)free_decimal(_decimal,foid);
    return(oid>0?_decimalValue:DISOWNED(_decimalValue,sizeof(Mvalue),foid));
}/* VALIDATED */

Mvalue* _getIntegerValue(long long ll,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(19));
    if(amVerboseDebugging())output("Wrapping integer '%lld'.\n",ll);
    Mvalue* _integerValue=__value("integer",foid);
    if(_integerValue){
        _integerValue->value._integer=_getInteger(ll);
        if(!_integerValue->value._integer){free_value(_integerValue,foid);_integerValue=NULL;}else _integerValue->type=VT_INTEGER;
    }
    return(oid>0?_integerValue:DISOWNED(_integerValue,sizeof(Mvalue),foid));
}/* VALIDATED */

Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,bool freeonfailure,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(20));
    Mvalue* _bigintegerValue=(_biginteger?__value("biginteger",foid):NULL);
    if(_bigintegerValue){
        _bigintegerValue->type=VT_BIGINTEGER;
        _bigintegerValue->value._biginteger=OWNED(_biginteger,sizeof(Mbiginteger),foid);
    }
    if(!_bigintegerValue||!_bigintegerValue->value._biginteger){
        if(freeonfailure)free_biginteger(_biginteger,foid);
        if(amVerboseDebugging())
            {output("%s",M_ERROR_PREFIX);outputBiginteger("Failed to wrap big integer '",_biginteger,"'.\n");}
    }else
    if(amVerboseDebugging())
        outputInfo("No big integer to wrap.");
    return(oid>0?_bigintegerValue:DISOWNED(_bigintegerValue,sizeof(Mvalue),foid));
}/* VALIDATED */

Mvalue* _getFloatValue(long double ld){
    if(amVerbose())output("Wrapping long double (float) '%.*Lf'.\n",DBL_DIG,ld);
    Mvalue* _floatValue=__value("long double");
    if(_floatValue){
        _floatValue->value._float=_getFloat(ld);
        if(!_floatValue->value._float){free_value(_floatValue);_floatValue=NULL;}else _floatValue->type=VT_FLOAT;
    }else
    if(amVerboseDebugging())
        outputInfo("No float to wrap.");
    return _floatValue;
}/* VALIDATED */
Mvalue* _getTextValue(char* _s,bool freeonfailure){
    if(!_s)return NULL; // when no input, no go
    Mvalue* _textValue=NULL; // the result, when NULL check freeonfailure
    Mtext* _text=_getText(_s); // for Mstring* sources pass string(Mstring*) into getStringValue() (which points to Mstring->chars which always start with the quote char used in declaring the literal)
    if(_text){
        _textValue=__value("text");
        if(_textValue){_textValue->type=VT_TEXT;_textValue->value._text=_text;}else free_text(_text); // always free the Mtext if it is not bound!!!
    }
    if(!_textValue)if(freeonfailure)free(_s);
    return _textValue;
}/* VALIDATED */
Mvalue* _getCharTextValue(char _c){
    Mtext* _text=(_c?_getCharText(_c):NULL); // for Mstring* sources pass string(Mstring*) into getStringValue() (which points to Mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _textValue=(_text?__value("char"):NULL);
    if(_textValue){_textValue->type=VT_TEXT;_textValue->value._text=_text;}else if(_text)free_text(_text); // ah do NOT forget to free _text if we haven't been able to create a value!!
    return _textValue;
}/* VALIDATED */
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype,bool weak,char const * const source){
    Mlist* _list=__list((source?source:"_getListValue"));
    if(!_list)return NULL;
    _list->weak=weak;
    _list->valuetype=listValuetype; // register what type of elements this list should have
    Mvalue* _listvalue=__value(weak?"weak list":"strong list");
    if(_listvalue){_listvalue->type=VT_LIST;_listvalue->value._list=_list;}else free_list(_list);
    return _listvalue;
}/* VALIDATED */
Mlist* _getListIndices(Mlist const * const list){
    Mlist* _list=_getListOfType(VT_INTEGER);
    if(!_list)return NULL;
    Mlistelement* listelement=list->_first;
    while(listelement){
        Mvalue* indexValue=_getIntegerValue(listelement->index);
        if(appendedToList(_list,indexValue,M_LL_INVALID)==0){
            // NO need to free indexValue because it is a Value!!!
            outputError("Failed to append list element index");break;
        }
        listelement=listelement->_next;
    }
    return _list;
}/* VALIDATED */

// MDH@30MAR2020: the general idea of flattening a list is that all elements of the list are not lists anymore
//                let's assume that NULL means something went wrong, caller should take care of situation where value is NULL unless ?TODO? we allow putting a NULL value in the list
//                reversed tells _getFlattenedList to prepend instead of append
// MDH@06APR2020: passing in a flatten level that is decremented on each element and when it reaches 0 no flattening has to occur
Mlist* _getFlattenedList(Mvalue const * const value,unsigned int flattenLevel,bool reversed){
    Mlist* _list=NULL;
    if(value){
        if(amVerboseDebugging())
            outputValue("Flattening '",value,"'.\n");
        _list=_getListOfType(VT_UNDEFINED);
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
                            Mlist* _subList=_getFlattenedList(valueListelement->_value,(flattenLevel>0?flattenLevel-1:0),reversed);
                            if(_subList){
                                Mlistelement* valueSubListelement=_subList->_first;
                                while(valueSubListelement){
                                    // if supposed to return the prepend instead of append pass in 0 instead of M_LL_INVALID
                                    if(valueSubListelement->_value)if(appendedToList(_list,valueSubListelement->_value,(reversed?0:M_LL_INVALID))<=0){success=false;break;}
                                    valueSubListelement=valueSubListelement->_next;
                                }
                                // ALWAYS free _sublist
                                free_list(_subList);
                            }else
                                success=false;
                        }else
                            if(appendedToList(_list,valueListelement->_value,(reversed?0:M_LL_INVALID))<=0)success=false;
                        if(!success)break; // do NOT continue with appending when failing to do so
                    }
                    valueListelement=valueListelement->_next;
                }
            }else // a single element to add to the list
                if(appendedToList(_list,value,M_LL_INVALID)<=0)success=false;
            if(!success){free_list(_list);_list=NULL;} // on failure release the list
        }    
    }
    if(amVerboseDebugging())
        {if(_list){if(flattenLevel>0)outputList("Flattened to '",_list,"'.\n");else outputList("Converted to '",_list,"'.\n");}}
    return _list;
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

Mlist* _getMapAttributes(Mmap const * const map){
    Mlist* _list=_getListOfType(VT_TEXT);
    if(!_list)return NULL;
    Mmapelement* mapelement=map->_first;
    while(mapelement){
        // I guess we'll have to duplicate the attribute name because it will be wrapped inside a Value
        // this is a bit of an issue because typically text should be enquoted
        Mstring* _attributeName=_getString("'");
        if(!_attributeName){outputError("Failed to duplicate a map attribute name");break;}
        string_append(_attributeName,mapelement->_variable->_name->chars); // MDH@17APR2020: char* _name replaced by Mchars* _name // append the attribute name
        Mvalue* attributeValue=_getTextValue(string(_attributeName),false);
        free_string(_attributeName);
        if(!attributeValue){outputError("Failed to store a map attribute name");break;}
        if(appendedToList(_list,attributeValue,M_LL_INVALID)==0){
            // NO need to free indexValue because it is a Value!!!
            outputError("Failed to append map attribute name");break;
        }
        mapelement=mapelement->_next;
    }
    return _list;
}

Mvalue* _getMapValue(Mvaluetype mapValuetype,bool weak){
    Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
    if(!_map)return NULL;
    _map->weak=weak;
    _map->valuetype=mapValuetype;
    Mvalue* _mapvalue=__value(weak?"weak map":"strong map");
    if(_mapvalue){_mapvalue->type=VT_MAP;_mapvalue->value._map=_map;}else free_map(_map);
    return _mapvalue;
}/* VALIDATED */

// some other wrappers
Mvalue* _getValueOfInteger(Minteger* _integer,bool freeonfailure){
    if(!_integer)return NULL;
    Mvalue* _value=__value("integer");
    if(_value){_value->type=VT_INTEGER;_value->value._integer=_integer;}else if(freeonfailure)free_integer(_integer);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfFloat(Mfloat* _float,bool freeonfailure){
    if(!_float)return NULL;
    Mvalue* _value=__value("float");
    if(_value){_value->type=VT_FLOAT;_value->value._float=_float;}else if(freeonfailure)free_float(_float);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfMap(Mmap* _map,bool freeonfailure){
    if(!_map)return NULL;
    Mvalue* _value=__value("map");
    if(_value){_value->type=VT_MAP;_value->value._map=_map;}else if(freeonfailure)free_map(_map);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfToken(Mtoken* _token,bool freeonfailure){
    if(!_token)return NULL;
    Mvalue* _value=__value("token");
    if(_value){_value->type=VT_TOKEN;_value->value._token=_token;}else if(freeonfailure)free_token(_token);
    return _value;
}/* VALIDATED */
/* TODO move elsewhere
Mvalue* _getTokenValue(Mtoken* _token,bool freeonfailure){
	if(!_token)return NULL;
	Mvalue* _tokenValue=__value();
	if(_tokenValue){_tokenValue->type=VT_TOKEN;_tokenValue->value._token=_token;}else if(freeonfailure)free_token(_token);
	return _tokenValue;
}*/

// helper function to create a parameter map with a single value
// the following is a nuisance
Mmap* _getFloatMap(char* name,Mvalue* _floatValue){
    if(name&&_floatValue){
        Mvariable* _realVariable=_getVariable(name,VT_FLOAT,true);
        if(_realVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_realVariable->_value,_floatValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
                    _mapelement->_variable=_realVariable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    if(amVerboseDebugging())
                        outputInfo("Returning the single float map!");
                    return _map;
                }
                outputError("Failed to create the float variable map");
                free_mapelement(_mapelement,false);
            }else
                outputError("Failed to create the float variable map element");
            free_variable(_realVariable,false);
        }else
            outputError("Failed to create the float variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getMap(char* name){
    Mvariable* _variable=_getVariable(name,VT_UNDEFINED,true);
    if(_variable){
        Mmapelement* _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
        if(_mapelement){
            Mmap* _map=CALLOC(sizeof(Mmap),'M');
            if(_map){
                _map->numberOfElements=1;
                _mapelement->_variable=_variable;
                _map->_first=_mapelement;
                _map->_last=_mapelement;
                return _map;
            }
            FREE(_mapelement,'m'); // MDH@11NOV2019: no need to call free_mapelement() 
        }
        free_variable(_variable,false);
    }
    return NULL;
}/* VALIDATED */

Mmap* _getMapCopy(Mmap const * const map){ // creates a 'deep' copy
    if(map){
        Mmap* _map=_getMapOfType(map->valuetype);
        if(_map){
            Mvariable *mapelementVariable,*_mapelementVariable=NULL;
            Mmapelement *mapelement=map->_first,*_mapelement=NULL;
            while(mapelement){
                mapelementVariable=mapelement->_variable;
                if(mapelementVariable){
                    _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m'); // new map element to hold a copy
                    if(_mapelement){
                        // create a variable with the same name and value as the variable in mapelement
                        // MDH@12MAR2020 OOPS: why would we make the copy ALWAYS immutable: replacing true by mapelementVariable->immutable
                        _mapelement->_variable=_getVariable(mapelementVariable->_name->chars,mapelementVariable->valuetype,mapelementVariable->immutable/*true*/);
                        assignValue(&_mapelement->_variable->_value,mapelementVariable->_value); // 'copy' the value over
                        if(_map->_last)_map->_last->_next=_mapelement; // make the current last point to the new last
                        _map->_last=_mapelement; // replace current last by the new last
                        if(!_map->_first)_map->_first=_map->_last; // initialize first if necessary
                        _map->numberOfElements++; // count one more
                    }else
                        output("%sFailed to copy map attribute '%s'.\n",M_ERROR_PREFIX,mapelementVariable->_name);
                }
                mapelement=mapelement->_next;
            }
            return _map;
        }else
            outputError("Failed to create a map");
    }
    return NULL;
}
Mlist* _getListCopy(Mlist const * const list){ // creates a 'deep' copy
    if(list){
        Mlist* _list=_getListOfType(list->valuetype);
        if(_list){
            Mlistelement *listelement=list->_first,*_listelement=NULL;
            while(listelement){
                _listelement=(Mlistelement*)CALLOC(sizeof(Mlistelement),'l');
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
            return _list;
        }else
            outputError("Failed to create a list");
    }
    return NULL;
}

Mmap* _getIntegerMap(char* name,Mvalue* _integerValue){
    // NOTE wait with filling the single integer value map until we have all the ingredients
    if(name&&_integerValue){
        Mvariable* _integerVariable=_getVariable(name,VT_INTEGER,true);
        if(_integerVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_integerVariable->_value,_integerValue); ///////  incrementReferenceCount(_integerValue); // now bound to the integer variable
                    _mapelement->_variable=_integerVariable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amVerboseDebugging())
                        outputInfo("Returning the single integer map!");
                    return _map;
                }
                outputError("Failed to create the integer variable map");
                free_mapelement(_mapelement,false);
            }else
                outputError("Failed to create the integer variable map element");
            free_variable(_integerVariable,false);
        }else
            outputError("Failed to create the integer variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getIntegerBooleanMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_INTEGER,true);
                    _mapelement2->_variable=_getVariable(name2,VT_INTEGER,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                    outputError("Failed to create both map element variables");
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }else
                    outputError("Failed to create a map");
            }else
                outputError("Failed to create both integer map elements");
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
        }else
            output("%sMap element attribute keys '%s' and '%s' undefined or the same.\n",M_ERROR_PREFIX,name1,name2);
    }else
        outputError("Not both map element attribute keys defined");
    return NULL;
}/* VALIDATED */
Mmap* _getStringStringMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_TEXT,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TEXT,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                    outputError("Failed to create both map element variables");
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }else
                    outputError("Failed to create a map");
            }else
                outputError("Failed to create both string map elements");
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
        }else
            output("%sMap element attribute keys '%s' and '%s' undefined or the same.\n",M_ERROR_PREFIX,name1,name2);
    }else
        outputError("Not both map element attribute keys defined");
    return NULL;
}/* VALIDATED */
Mmap* _getFloatFloatMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_FLOAT,true);
                    _mapelement2->_variable=_getVariable(name2,VT_FLOAT,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getMapTokenMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_MAP,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getListMap(char* name,Mvalue* _listValue){
    if(name&&_listValue){
        Mvariable* _listVariable=_getVariable(name,VT_LIST,true);
        if(_listVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_listVariable->_value,_listValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
                    _mapelement->_variable=_listVariable;
                   _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    if(amDebugging())outputInfo("Returning the single list map!");
                    return _map;
                }
                outputError("Failed to create the list variable map");
                free_mapelement(_mapelement,false);
            }else
                outputError("Failed to create the list variable map element");
            free_variable(_listVariable,false);
       }else
            outputError("Failed to create the list variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_TEXT,true);
                    _mapelement2->_variable=_getVariable(name2,VT_MAP,true);
                    _mapelement3->_variable=_getVariable(name3,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _map->_last=_mapelement3;
                        _map->numberOfElements=3;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getTokenTokenMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_TOKEN,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _map->_last=_mapelement2;
                        _map->numberOfElements=2;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3){
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_UNDEFINED,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TOKEN,true);
                    _mapelement3->_variable=_getVariable(name3,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _map->_last=_mapelement3;
                        _map->numberOfElements=3;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3){
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_INTEGER,true);
                    _mapelement2->_variable=_getVariable(name2,VT_INTEGER,true);
                    _mapelement3->_variable=_getVariable(name3,VT_INTEGER,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _map->_last=_mapelement3;
                        _map->numberOfElements=3;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getListValueIntegerMap(char* name1,char* name2,char* name3){
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_LIST,true); // must be a list
                    _mapelement2->_variable=_getVariable(name2,VT_UNDEFINED,true); // any value will do
                    _mapelement3->_variable=_getVariable(name3,VT_INTEGER,true); // must be an integer
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _map->_last=_mapelement3;
                        _map->numberOfElements=3;
                        return _map;
                    }
                    free_map(_map); // failed to create the three map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4){
    if(name1&&name2&&name3&&name4){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
            strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name3,name4)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement4=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3&&_mapelement4){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_TOKEN,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TOKEN,true);
                    _mapelement3->_variable=_getVariable(name3,VT_TOKEN,true);
                    _mapelement4->_variable=_getVariable(name4,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable&&_mapelement4->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _mapelement3->_next=_mapelement4;
                        _map->_last=_mapelement4;
                        _map->numberOfElements=4;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
            free_mapelement(_mapelement4,false);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getTokenTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4,char *name5){
    if(name1&&name2&&name3&&name4){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
            strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name1,name5)&&
            strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name2,name5)&&
            strcmp(name3,name4)&&strcmp(name3,name5)&&
            strcmp(name4,name5)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement4=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            Mmapelement* _mapelement5=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3&&_mapelement4&&_mapelement5){
                Mmap* _map=(Mmap*)CALLOC(sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_TOKEN,true);
                    _mapelement2->_variable=_getVariable(name2,VT_TOKEN,true);
                    _mapelement3->_variable=_getVariable(name3,VT_TOKEN,true);
                    _mapelement4->_variable=_getVariable(name4,VT_TOKEN,true);
                    _mapelement5->_variable=_getVariable(name5,VT_TOKEN,true);
                    if(_mapelement1->_variable&&_mapelement2->_variable&&_mapelement3->_variable&&_mapelement4->_variable&&_mapelement5->_variable){
                        _map->_first=_mapelement1;
                        _mapelement1->_next=_mapelement2;
                        _mapelement2->_next=_mapelement3;
                        _mapelement3->_next=_mapelement4;
                        _mapelement4->_next=_mapelement5;
                        _map->_last=_mapelement5;
                        _map->numberOfElements=5;
                        return _map;
                    }
                    free_map(_map); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
                }
            }
            // either map element might have been created and we need to release them
            free_mapelement(_mapelement1,false);
            free_mapelement(_mapelement2,false);
            free_mapelement(_mapelement3,false);
            free_mapelement(_mapelement4,false);
            free_mapelement(_mapelement5,false);
        }
    }
    return NULL;
}/* VALIDATED */
// end helper functions 

// LIST STUFF
Mvalue* _getValueOfList(Mlist* _list,bool freeonfailure){
    if(!_list)return NULL;
    Mvalue* _value=__value(_list->weak?"weak list":"strong list");
    if(_value){_value->type=VT_LIST;_value->value._list=_list;}else if(freeonfailure)free_list(_list);
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
/*unsigned*/ long long appendedToList(Mlist * const _list,Mvalue const * const _value,long long index){
    if(!_list){outputError("No list to append to");return M_LL_INVALID;} // MDH@18OCT2019: let's allow NULLing list elements (i.e. accepting _value to be NULL)
    if(_list->immutable){outputError("Unable to change the list: it is immutable");return 0;}
    // MDH@05NOV2019: let's always allow adding NULL or undefined values to a list
    if(_value&&_value->type!=VT_UNDEFINED&&_list->valuetype!=VT_UNDEFINED)
    if(_value->type!=_list->valuetype)
    {output("%s",M_ERROR_PREFIX);outputValue("Unable to add '",_value,"' to a list: it is of the wrong type.");return 0;}
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    // MDH@17OCT2019: index 0 now does not indicate to append to the end anymore but now indicates that the given value should be prepended!!!!
    // MDH@05NOV2019: if supposed to append the value, and the current last index is already equal to the maximum possible index, we consider the list to be full
    if(index==M_LL_INVALID){if(lastindex==M_LL_MAX){outputError("Unable to append to a list: it is full");return 0;};index=lastindex+1;} // MDH@17OCT2019: we need to be able to append as well (can't use 0 anymore!!!!)
    if(index<0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    // MDH@17OCT2019: a negative index might still end up with index 0, this happens with -len(x)-1, ok, for now just accept this when it happens
    if(index<0){output("%sIndex %lld of (new) list element too small.\n",M_ERROR_PREFIX,index);return M_LL_INVALID;} // MDH@17OCT2019: can't return negative value!!! // MDH@05NOV2019: to indicate invalid input
    if(amVerbose()&&amDebugging())outputValue((index>0?"Appending '":"Prepending '"),_value,"' to a list.\n");
    // MDH@23MAY2019: let's allow inserting or replacing as well
    // determine _listelement as element to host the value, store the successor in _nextlistelement
    Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index>0&&index<=lastindex?_list->_first:NULL);
    if(_listelement){ // we are not appending and we have a first element, so this might be an insert or replace
        // NOTE testing _listelement is just a fail-safe as that should never happen
        while(index>_listelement->index){
            _prevListelement=_listelement;
            if(!_listelement->_next){output("BUG: Index (%llu) ",_listelement->index);outputValue("of existing list element '",_listelement->_value,"' probably out of order.\n");return M_LL_INVALID;}
            _listelement=_listelement->_next;
        }
        // if we're going to insert there will be a successor
        if(_listelement->index!=index){_nextListelement=(_prevListelement?_prevListelement->_next:_list->_first);_listelement=NULL;} // so that we are forced to create one
    }else // we'll be insertingappending/prepending, so the current last is the predecessor (and no successor)
    if(index>0) // MdH@17OCT2019: when not prepending...
        _prevListelement=_list->_last;
    // if we do not have a list element ascertain to have one
    if(!_listelement){ // not yet present in list, so we have to create a new element
        _listelement=(Mlistelement*)CALLOC(sizeof(Mlistelement),'l');
        if(!_listelement){outputError("Failed to create a list element to insert");return 0;} // failure
    }
    // MDH@02NOV2019: if the list is flagged as weak we do not (de)reference values (and copy lists and maps as assignValue() does)
    if(_list->weak)_listelement->_value=_value;else assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
    // if replacing i.e. the index of _listelement matches index, we're done
    // if index equals 0 it WILL be equal to _listelement->index (which is initialized to 0 for sure)
    if(_listelement->index!=index){ // insert or append
        _listelement->index=index;
        // linking
        if(_prevListelement)_prevListelement->_next=_listelement;else _list->_first=_listelement;
        if(!_nextListelement){if(_list->_last)_list->_last->_next=_listelement;_list->_last=_listelement;}else _listelement->_next=_nextListelement;
        (_list->numberOfElements)++; // an additional element
    }else
    if(index==0){ // prepending
        // linking into the list
        _listelement->_next=_list->_first;
        _list->_first=_listelement;
        if(!_list->_last)_list->_last=_list->_first;
        (_list->numberOfElements)++;
        if(amVerbose())outputValue("Prepending '",_listelement->_value,"'.\n");
        // we should increment the index of all elements (consuming _listelement on the go which is OK)
        _nextListelement=_listelement;
        while(_nextListelement){
            if(amDebugging()){outputValue("Incrementing the index of '",_nextListelement->_value,"'.\n");}
            (_nextListelement->index)++;
            if(amDebugging()){outputValue("Index of '",_nextListelement->_value,"' incremented");output(" to %llu.\n",_nextListelement->index);}
            _nextListelement=_nextListelement->_next;
            if(amDebugging()){if(_nextListelement)outputInfo("A element to consider!");else outputInfo("No next element to consider!");}
        }
        if(amVerbose())outputValue("'",_listelement->_value,"' prepended.\n");
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
        if(_list->_last){
            long long maxindex=_list->_last->index;
            if(index<0)index+=(maxindex+1);
            if(index>0){
                if(index==maxindex)return &(_list->_last->_value);
                if(index<maxindex){
                    Mlistelement* _listelement=_list->_first;
                    while(_listelement){
                        if(_listelement->index==index)return &(_listelement->_value);
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
// MDH@24MAY2019: if already in the map should replace the current value
long long appendedToMap(Mmap* const _map,char const * const attributeName,Mvalue const * const _attributeValue){
    long long result=(_map&&attributeName?M_FALSE:M_LL_INVALID);
    if(result!=M_LL_INVALID){
        if(!_map->immutable){ // the map is mutable
            // MDH@05NOV2019: let's always allow adding NULL or undefined values to a map, but otherwise the type of _attributeValue should match the type of values the map allows
            if(!_attributeValue||_attributeValue->type==VT_UNDEFINED||_map->valuetype==VT_UNDEFINED||_attributeValue->type==_map->valuetype){
                if(amVerbose()&&amDebugging())
                {output("Setting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.\n");}
                Mmapelement* _mapelement=_map->_first;
                while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name->chars,attributeName))_mapelement=_mapelement->_next;
                if(!_mapelement){ // not found
                    _mapelement=(Mmapelement*)CALLOC(sizeof(Mmapelement),'m'); // NOTE no need to set _next because it is now NULL
                    if(_mapelement){
                        // MDH@12MAR2020: I suppose we would like to be able to change the map property value (now using dot notation as well), so the mutable flag should be true not false
                        _mapelement->_variable=_getVariable(attributeName,VT_UNDEFINED,false); // TODO why would this 'variable' be mutable, and allowing all values????
                        if(_mapelement->_variable){ // the variable was created so attach in map
                            if(_map->numberOfElements)_map->_last->_next=_mapelement;else _map->_first=_mapelement;
                            _map->_last=_mapelement;
                            _map->numberOfElements++;
                            // result=M_TRUE; // success
                        }else{ // we have a map element BUT no variable, so no go
                            free_mapelement(_mapelement,false);_mapelement=NULL;
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
                output("%s",M_ERROR_PREFIX);outputValue("Unable to add '",_attributeValue,"' to a list: it is of the wrong type.\n");
            }
        }else 
            outputError("Unable to change the map: it is immutable");
    }else
        outputError("No map or atribute name specified");
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

Mvalue* _getRationalValue(Mrational* _rational,bool freeonfailure){
    if(!_rational)return NULL;
    Mvalue* _value=__value("rational");
    if(_value){
        // MDH@03APR2020: if a rational has a denominator that equals 1 we can safely return a big integer instead but only when freeonfailure is true
        if(freeonfailure){
            if(!_rational->den){
                _value->type=VT_BIGINTEGER;
                _value->value._biginteger=_getBigintegerCopy(_rational->num);
                if(_value->value._biginteger){ // success
                    free_rational(_rational);
                    return _value;
                }
            }
        }
        _value->type=VT_RATIONAL;_value->value._rational=_rational;
    }else
    if(freeonfailure)free_rational(_rational);
    return _value;
}/* VALIDATED */

//////////Mstring* _getValueText(Mvalue* _value); // forward prototype used in getListText() and getMapText()
Mstring* _getListText(Mlist* _list){
    ///////output("List to output.");char c;inputCharRead(&c);
	Mstring* result=__string();
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
	        //////////outputChar('.');
			_listelementValue=_listelement->_value;
			/////if(_listelementValue){
				Mstring* _listelementValueText=_getValueText(_listelementValue,false); // to be freed asap
				if(_listelementValueText){
					p=string_append(p,string(_listelementValueText));
					free_string(_listelementValueText); // release AFTER copying over
				}
			/////}
			_listelement=_listelement->_next;
		}
		p=string_append_char(p,']');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(!p){free_string(result);result=NULL;}
	}
	return result;
}/* VALIDATED */
// MDH@02MAR2020: utility function to output a map
void outputList(char const * const prefix,Mlist* list,char const * const suffix){
    Mstring* _listText=_getListText(list);
    output("%s%s%s",(prefix?prefix:""),string(_listText),(suffix?suffix:""));
    free_string(_listText);
}/* VALIDATED */

Mstring* _getMapText(Mmap* _map,bool showcurlybraces,bool showquotes,bool showmissings){
	Mstring* result=__string();
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
                            Mstring* _mapelementValueText=_getValueText(_mapVariable->_value,false); // free asap
                            /////output("Map element: %s",string(p));
                            // TODO technically NULL is also a value, so shouldn't be use the undefined value text????
                            if(_mapelementValueText){
                                p=string_append(p,string(_mapelementValueText)); // append 
                                free_string(_mapelementValueText); // release AFTER copying over
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
		if(!p){free_string(result);result=NULL;}
	}
	return result;
}/* VALIDATED */
// MDH@02MAR2020: utility function to output a map
void outputMap(char const * const prefix,Mmap* map,char const * const suffix){
    Mstring* _mapText=_getMapText(map,true,true,true);
    output("%s%s%s",(prefix?prefix:""),(_mapText?string(_mapText):""),(suffix?suffix:""));
    free_string(_mapText);
}/* VALIDATED */ 

// MDH@24OCT2019: if you want the value representation or perhaps the name of a constant depends on whether name is defined
/*
static Mstring* _nullValueTextRepresentation=NULL;
Mstring* _getNullValueTextRepresentation(){if(!_nullValueTextRepresentation)_nullValueTextRepresentation=_getString(M_NULL_VALUE_TEXT_REPRESENTATION);return _getNullValueTextRepresentation;}
*/
Mstring* _getValueText(const Mvalue* const _value,bool dequoted){
	// NOTE whatever is returned should be freed
	Mstring* valueText=NULL;
    ////outputChar('.');
	if(_value){
        ////outputChar('+');
		////output("TYPE: %d\n",_value->type);
		switch(_value->type){
            case VT_UNDEFINED:valueText=_getString(M_UNDEFINED_VALUE_TEXT);break; // calling _getString() will create a new string every time but I think we have to do that because _getValueText() typically returns something that is freed elsewhere
			case VT_INTEGER:valueText=_getIntegerText(_value->value._integer);break;
            case VT_BIGINTEGER:valueText=_getBigintegerText(_value->value._biginteger);break; // how many characters do we need????
            case VT_DECIMAL:valueText=_getDecimalText(_value->value._decimal,false);break; // fixedpoint to obligatory (i.e. e-notation allowed for very big/small (positive) numbers)
            case VT_RATIONAL:valueText=_getRationalText(_value->value._rational);break;
			case VT_FLOAT:valueText=_getFloatText(_value->value._float);break;
			case VT_TEXT:valueText=_getStringText(_value->value._text,dequoted);break; // TODO don't dequote the text!!
			case VT_MAP:valueText=_getMapText(_value->value._map,true,true,true);break;
			case VT_LIST:valueText=_getListText(_value->value._list);break;
            case VT_TOKEN:
                { // can't just show the single token because we could have following ones
                    valueText=__string();
                    if(valueText){
                        Mstring* p=valueText;
                        Mtoken* token=_value->value._token;
                        while(p&&token){
                            p=string_append(p,string(token->text));
                            token=token->next;
                        }
                        if(!p){free_string(valueText);valueText=NULL;}
                    }
                    // replacing: valueText=_stringCopy(_value->value._token->text,0);
                }
                break; // we need to return a copy because that copy will be freed typically (and we do not want to free the original now do we?)
			case VT_REFERENCE:
                {
                    valueText=__string();
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
                            if(_valueText){p=string_append(p,string(_valueText));free_string(_valueText);}
                            p=string_append_char(p,']');
                        }
                        if(_value->value._reference->_value){
                            p=string_append_char(p,'=');
                            Mstring* _valueText=_getValueText(_value->value._reference->_value,dequoted);
                            if(_valueText){p=string_append(p,string(_valueText));free_string(_valueText);}
                        }
                        */
                        if(!p){free_string(valueText);valueText=NULL;}
                    }
                }
                break;
            case VT_FUNCTION:
                {
                    valueText=_getString("function");
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
                                        Mstring* _parameterValueText=_getValueText(_parameterMapelement->_variable->_value,false);
                                        p=string_append(p,string(_parameterValueText));
                                        free_string(_parameterValueText);
                                    }
                                    _parameterMapelement=_parameterMapelement->_next;
                                    if(_parameterMapelement)p=string_append_char(p,',');
                                }
                            }
                            p=string_append_char(p,')');
                        }else
                            p=string_append_char(p,'?');
                        if(!p){free_string(valueText);valueText=NULL;}
                    }
                }
                break;
            case VT_ENVIRONMENT:
                {
                    valueText=_getString("environment");
                    if(valueText){
                        Mstring* p=valueText;
                        Menvironment* environment=_value->value._environment;
                        if(environment){
                            p=string_append_char(p,'(');
                            p=string_append_char(p,'\'');
                            Mstring* _environmentName=_getEnvironmentName(environment);
                            p=string_append(p,string(_environmentName));
                            free_string(_environmentName);
                            p=string_append_char(p,'\'');
                            p=string_append_char(p,')');
                        }else
                            p=string_append_char(p,'?');
                        if(!p){free_string(valueText);valueText=NULL;}
                    }
                }
                break;
            default:break;
		}
	}else // the text we use for an value that is NULL!
        valueText=_getString(M_NULL_VALUE_TEXT);
    if(_value)if(amAssisting())if(valueText)valueText=appendll(string_append_char(valueText,'#'),_value->count); // show the reference count as well
    /////outputChar('.');
    return(valueText?valueText:_getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}/* VALIDATED */
// MDH@13MAR2020: now returning the number of characters written
size_t outputValue(const char* const prefix,const Mvalue* const value,const char* const suffix){
    size_t written=0;
    if(prefix)written=output("%s",prefix);
    if(value){
        Mstring* _valueText=_getValueText(value,false); // free asap
        if(_valueText){written+=output("%s",string(_valueText));free_string(_valueText);}
    }else
        written+=outputChar('-');
    if(suffix)written+=output("%s",suffix);
    return written;
}/* VALIDATED */

long long getValueInteger(const Mvalue* const _value){
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
                    Mbiginteger* _biginteger=_rational2biginteger(_value->value._rational);
                    if(_biginteger){
                        long long ll=biginteger2long(_biginteger);
                        free_biginteger(_biginteger);
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
    if(_bigintegerText){ldBiginteger=_strtold(string(_bigintegerText),ldBiginteger);free_string(_bigintegerText);}
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

Mbiginteger* _getValueBiginteger(Mvalue const * const _value){
    if(_value)
        switch(_value->type){
        case VT_BIGINTEGER:return _value->value._biginteger;
        case VT_INTEGER:return _getBiginteger(_value->value._integer->ll);
        case VT_RATIONAL:return _rational2biginteger(_value->value._rational); // TODO how can we be certain that the returned big integer is actually used? well, it should as this is _getValueBiginteger meaning you have to free it if you don't use it!!!
        case VT_FLOAT:
            {
                // this is a bit of a nuisance when the double is out of the VT_INTEGER range
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_set_longdouble(_biginteger,_value->value._float->ld)!=MP_OKAY)
                {free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
        case VT_TEXT:
            {
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_read_radix(MP_INT_POINTER(_biginteger),_value->value._text->_c,10)!=MP_OKAY)
                {free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
        case VT_TOKEN:
            {
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_read_radix(MP_INT_POINTER(_biginteger),string(_value->value._token->text),10)!=MP_OKAY)
                {free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
        case VT_REFERENCE: // TODO this may be hard
            break;
        default:break;
    }
    return NULL;
}/* VALIDATED */

// (map) list conversions
bool listAppendedToMap(Mmap* const _map,const Mlist* const _list){ // appends a list to a (possibly empty) map using the indices as attribute name
    bool result=(_map!=NULL); // no map, no result!
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(_listelement){
                Mstring* _indexValueText=appendll(__string(),_listelement->index); // free asap
                if(_indexValueText){
                    if(!appendedToMap(_map,string(_indexValueText),_listelement->_value)){outputError("Failed to append a list element to a map");result=false;}
                    free_string(_indexValueText); // freeing
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
bool listAppendedToMaplist(Mlist* const _maplist,const Mlist* const _list){
    bool result=(_maplist&&_maplist->valuetype==VT_LIST); // the destination list should only allow for list elements
    if(result){
        if(_list){
            Mlistelement* _listelement=_list->_first;
            while(result&&_listelement){
                // index and value of the list element are stored in a new list!!
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED,false,"listAppendedToMapList"); // this will be a new value that (being managed) will be freed automatically when not bound, including the list contained by it!!
                if(!_maplistelementValue){outputError("Failed to create an empty list");result=false;break;}
                Mlist* _maplistelement=_maplistelementValue->value._list;
                // if we fail to construct the maplist element or to add it
                if(!_maplistelement||!appendedToList(_maplistelement,_getIntegerValue(_listelement->index),M_LL_INVALID)||!appendedToList(_maplistelement,_listelement->_value,M_LL_INVALID)||!appendedToList(_maplist,_maplistelementValue,M_LL_INVALID)){
                    outputError("Failed to create or populate map list element");
                    result=false;
                }else
                    _listelement=_listelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
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
                    // TODO we're appending the value as is (i.e. without copying so any change will also be present in the list)
                    // DONE this is ok because values are immutable in essence
                    if(index>0&&!appendedToList(_list,_maplistelementValue->value._list->_first->_next->_value,index))result=false;
                }
                _maplistelement=_maplistelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
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
                    Mstring* _attributeNameValueText=_getValueText(_attributeNameValue,true); // using common _getValueText to text the first element
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
                        if(!string_length(_attributeNameValueText)||!appendedToMap(_map,string(_attributeNameValueText),_maplistelementValue->value._list->_first->_next->_value)){
                            outputError("Failed to append a list element to a map (using the index text as attribute name)");
                            result=false;
                        }
                        free_string(_attributeNameValueText);
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
bool mapAppendedToList(Mlist* const _list,const Mmap* const _map){
    bool result=(_list!=NULL);
    if(result){
        if(_map&&_map->_first){
            Mmapelement* _mapelement=_map->_first;
            while(result&&_mapelement){
                // only add those map elements of which the key can be converted to a positive integer
                long long index=atoll(_mapelement->_variable->_name->chars);
                if(index>0&&!appendedToList(_list,_mapelement->_variable->_value,index)){
                    outputError("Failed to append a map element to a list (using the integer value of the name as index)");
                    result=false;
                }
                _mapelement=_mapelement->_next;
            }
        }
    }
    return result;
}/* VALIDATED */
bool mapAppendedToMaplist(Mlist* const _maplist,const Mmap* const _map){
    bool result=(_maplist&&_maplist->valuetype==VT_LIST);
    if(result){
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
                    Mstring* _attributeName=__string();
                    if(_attributeName){ // should be freed
                        Mstring* p=_attributeName;
                        p=string_append_char(p,'\'');
                        p=string_append(p,_mapelement->_variable->_name->chars);
                        if(p){
                            Mvalue* _attributeNameValue=_getTextValue(string(_attributeName),false);
                            if(_attributeNameValue&&appendedToList(_maplistelement,_attributeNameValue,M_LL_INVALID)){
                                if(!appendedToList(_maplistelement,_mapelement->_variable->_value,M_LL_INVALID)||!appendedToList(_maplist,_maplistelementValue,M_LL_INVALID)){
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
                        free_string(_attributeName); // freed!
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
Mdecimal* _getValueTextDecimal(Mvalue* value){
    // MDH@09OCT2019: delegating to _getTextDecimal() is preferable over doing it ourselves
    Mdecimal* _valueTextDecimal=NULL;
    if(value){
        if(value->type!=VT_DECIMAL){
    	    Mstring* _valueText=_getValueText(value,true); // TODO will there be any brackets around a repeating part of a 
	        if(_valueText){
                _valueTextDecimal=_getTextDecimal(string(_valueText),0);
                free_string(_valueText);
            }else
                outputError("Failed to create the text trying to convert a value to a decimal");
        }else
            _valueTextDecimal=_getDecimalCopy(value->value._decimal); // shouldn't happen though
    }
    return _valueTextDecimal;
    /* replacing:
    Mdecimal* _parsedValueDecimal=__decimal(NULL,0,0);
    if(_parsedValueDecimal){

		Mstring* _valueText=_getValueText(value,true);
		if(_valueText){
            uint32_t status=0;
            mpd_qset_string(_parsedValueDecimal->mpd,string(_valueText),get_default_mpd_context(),&status);
            free_string(_valueText);
            if((status&0xEFBF)!=0){free_decimal(_parsedValueDecimal);_parsedValueDecimal=NULL;outputError("Failed to parse the decimal text");}
        }
    }else
        outputError("Failed to create a decimal");
    return _parsedValueDecimal;
    */
}

// the work horse of converting any value (if possible) to a decimal
// TODO shouldn't we use this function in d()???????
Mdecimal* _getValueDecimal(Mvalue* value){
	Mdecimal* _decimal=NULL;
	if(value){
		if(value->type!=VT_LIST&&value->type!=VT_MAP){
			switch(value->type){
				case VT_DECIMAL:_decimal=_getDecimalCopy(value->value._decimal);break;
				case VT_INTEGER:_decimal=__decimal(NULL,value->value._integer->ll,0);break; // MDH@29AUG2019: replacing a call to _getDecimal()
                case VT_BIGINTEGER:
                    {
                        // NOTE we need to make a copy of the big integer because otherwise free_rational() below would free the big integer wrapped inside the value, which would be a terrible mistake
                        Mrational* _rational=_getRational(_getBigintegerCopy(value->value._biginteger),NULL,M_LD_NAN,false,true);
                        if(_rational){
                            _decimal=_getRationalDecimal(_rational);
                            free_rational(_rational);
                        }
                    }
                    break;
				case VT_RATIONAL:
					_decimal=_getRationalDecimal(value->value._rational);
					break;
				default:
                    _decimal=_getValueTextDecimal(value); // delegates to _getValueTextDecimal() which always parses the decimal from the text representation of the value
					break;
			}
		}
	}
	return _decimal;
}
Mdecimal* getValueDecimal(Mvalue* value){
	return(value?(value->type==VT_DECIMAL?value->value._decimal:_getValueDecimal(value)):NULL);
}
// END DECIMAL EXTRACTION

// MDH@03FEB2020: extract specific data elements wrapped in values
Menvironment* getValueEnvironment(Mvalue* value){return(value&&value->type==VT_ENVIRONMENT?value->value._environment:NULL);}

Mlist* _getListOfType(Mvaluetype valuetype){
    // output("Allocating list (size: %zd).\n",sizeof(Mlist));
    Mlist* _list=__list("getListOfType");
    _list->valuetype=valuetype;
    return _list;
}/* VALIDATED */
Mmap* _getMapOfType(Mvaluetype valuetype){
    Mmap* _map=CALLOC(sizeof(Mmap),'M');
    _map->valuetype=valuetype;
    return _map;
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
    }
    return result;
}/* VALIDATED */

// MDH@04JUN2019: based on https://stackoverflow.com/questions/4637967/algorithm-challenge-generate-continued-fractions-for-a-float/56444882#56444882
Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter){
    // if iterations, you're supposed to return all iteration results
    Mlist* _iterationsList=_getListOfType(VT_UNDEFINED);
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
                    _rational=_getRational(_getBiginteger(neg?-p:p),_getBiginteger(q),delta,false,true); // construct the intermediate result without normalizing
                    if(!_rational){output("%sFailed to construct the rational approximation %lld/%lld",M_ERROR_PREFIX,p,q);break;}
                    // NOTE once we have the created big integer numerator and denominator bound in _rational we're responsible of freeing _rational when not bound
                    // NOT being able to append the intermediate result to the list shouldn't be enough reason to abort, as long as we manage to add the end result
                    Mvalue* _rationalValue=_getRationalValue(_rational,true);
                    if(!_rationalValue){outputError("Failed to value wrap the intermediate rational approximation to a real");break;}
                    // NOTE probably best to break if we can't append approximations!!
                    // NOTE no need to free _rational even then as it is bound in _rationalValue so it will be freed anyway
                    if(!appendedToList(_iterationsList,_rationalValue,i)){/*free_rational(_rational);*/outputError("Failed to register a rational approximation");break;}
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
                    if(!_rational){free_biginteger(_numerator);free_biginteger(_denominator);} // DIY freeing if failing to construct the rational
                }
                */
            }else{ // long double is zero, TODO should we store 0 as the delta, or just NaN???? what would be the difference??????
                Mvalue* _rationalValue=_getRationalValue(_getRational(__biginteger(),NULL,M_LD_NAN,false,true),true); // OK free __biginteger() if failing to get that _rational
                if(_rationalValue&&!appendedToList(_iterationsList,_rationalValue,0))outputError("Failed to append rational approximation to the result list");
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
    return _iterationsList;
}/* VALIDATED */

void assignValue(Mvalue** _valueholder, Mvalue const * _value){
    // ASSERT not a composite value (so like an end node)
    if(*_valueholder)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
    // MDH@01NOV2019: it's a leap of faith to let assignValue() create copies of composite values i.e. instead of assigning _value to the *_valueholder we assign a new map or list value
    if(_value){
        // create a copy of the map or list and assign it to the value holder however it would then copy the map again, and that's not what should happen!!!
        // NOTE the new map and list value get a reference count of 1 below as soon as they are bound to the value holder (as should be the case)
        //      wait a minute a forgot to take care of the reference count of the values in _getMapCopy() and _getListCopy(), NO no need to that if they use assignValue() to 'copy' the values
        if(_value->type==VT_MAP){
            // if(amVerbose()&&amDebugging())outputValue("Copying map ",_value,".\n");
            _value=_getValueOfMap(_getMapCopy(_value->value._map),true);
        }else
        if(_value->type==VT_LIST){
            // if(amVerbose()&&amDebugging())outputValue("Copying list ",_value,".\n");
            _value=_getValueOfList(_getListCopy(_value->value._list),true);
        }
    }
    *_valueholder=_value; // replace what's being pointed to
    if(*_valueholder)incrementReferenceCount(*_valueholder); // increment what it's pointing to now (if not NULL)
}/* VALIDATED */

// functions
// helpers
Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction){
    Mlist* _result=NULL;
    if(_list){
        _result=_getListOfType(_list->valuetype);
        Mlistelement* _listelement=_list->_first;
        while(_listelement&&appendedToList(_result,oneArgumentFunction(_listelement->_value),_listelement->index)>0)_listelement=_listelement->_next;
    }
    return _result;
}/* VALIDATED */
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction){
    Mmap* _result=NULL;
    if(_map){
        _result=_getMapOfType(_map->valuetype);
        Mmapelement* _mapelement=_map->_first;
        while(_mapelement&&appendedToMap(_result,_mapelement->_variable->_name->chars,oneArgumentFunction(_mapelement->_variable->_value))==1)_mapelement=_mapelement->_next;
    }
    return _result;
}/* VALIDATED */

Mbiginteger* _getRoundedRationalInteger(Mrational* _rational){
    // we have to multiply the numerator by 2 and add the denominator
    // NO we should compare the remainder with the denominator divided by two
    // BUT if we multiply the numerator with 2 we can divide and compare with 
    // TODO ignores delta for now
    if(_rational){
        // denominator equal to 1?
        if(!_rational->den||mp_cmp(MP_INT_POINTER(_rational->den),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1));
        // if the numerator equals 1, the result is 0
        if(!_rational->num||mp_cmp(MP_INT_POINTER(_rational->num),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return _getBiginteger(0); // with the numerator at least equal to 2 the result will always be 0
        bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _dividend=NULL;
        Mbiginteger* _absnum=__biginteger(); // to be freed asap
        if(_absnum){ // freeable
            if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))==MP_OKAY){
                Mbiginteger* _twicenum=__biginteger();
                if(_twicenum){ // freeable
                    if(mp_mul_2(MP_INT_POINTER(_absnum),MP_INT_POINTER(_twicenum))==MP_OKAY){
                        Mbiginteger* _twiceden=__biginteger();
                        if(_twiceden){
                            if(mp_mul_2(MP_INT_POINTER(_rational->den),MP_INT_POINTER(_twiceden))==MP_OKAY){
                                Mbiginteger* _remainder=__biginteger();
                                if(_remainder){
                                    _dividend=__biginteger();
                                    if(_dividend){
                                        bool success=(mp_div(MP_INT_POINTER(_twicenum),MP_INT_POINTER(_twiceden),MP_INT_POINTER(_dividend),MP_INT_POINTER(_remainder))==MP_OKAY);
                                        // increment _dividend if _remainder larger than denominator
                                        if(success&&mp_cmp(MP_INT_POINTER(_remainder),MP_INT_POINTER(_rational->den))==MP_GT&&mp_incr(MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
                                        if(success&&neg&&mp_neg(MP_INT_POINTER(_dividend),MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
                                        if(!success){free_biginteger(_dividend);_dividend=NULL;}
                                    }else 
                                        outputError("Failed to create big integer dividend");
                                    free_biginteger(_remainder);
                                }else 
                                    outputError("Failed to create big integer remainder");
                            }else
                                outputError("Failed to double big integer denominator");
                            free_biginteger(_twiceden);
                        }
                    }else
                        outputError("Failed to double big integger numerator");
                    free_biginteger(_twicenum); // freed!
                }else
                    outputError("Failed to create big integer");
            }else
                outputError("Failed to compute the absolute of a big integer");
            free_biginteger(_absnum); // freed!
        }
        return _dividend;
    }
    return NULL;
}/* VALIDATED */
/*
 * \parameter floor  when true, returns the integer part of the rational, which actually is the trunc value
 * \parameter towardszero is true, returns the integer part of the rational, which actually is the trunc version
 */
Mbiginteger* _getRationalInteger(Mrational* _rational,bool floor,bool towardszero){
    // TODO ignores delta for now
    if(_rational){
        if(!_rational->num)return NULL; // if the numerator is undefined, the rational is undefined!!!!
        if(!_rational->den)return _getBigintegerCopy(_rational->num); // cannot get it much simpler if the rational already is integer
        // ASSERT both numerator and denominator are NOT NULL
        bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _absnum=__biginteger(); // to be freed asap
        if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))!=MP_OKAY)
        {free_biginteger(_absnum);outputError("Failed to compute the absolute of a big integer");return NULL;}
        Mbiginteger *_dividend=__biginteger(),*_remainder=__biginteger();
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
            if(!success){free_biginteger(_dividend);_dividend=NULL;}
        }
        free_biginteger(_remainder);
        free_biginteger(_absnum);
        return _dividend;
    }
    return NULL;
}

// MDH@18OCT2019: same for decimals
// TODO should we store the result in a big integer or an integer (if possible????)
//      theoretically we should return a decimal!!!
Mdecimal* _getDecimalInteger(Mdecimal* _decimal,bool floor,bool towardszero){
    // NOTE decimals can have repeating parts BUT those repeating digits are only behind the decimal comma, so won't have effect on the integers
    // ceil: false,false / trunc: true,true / floor: true,false / ?: false,true
    if(_decimal){
        Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
        if(mpd_context){
            if(towardszero){ // always means truncing!!!
                Mdecimal* _truncDecimal=__decimal(mpd_context,0,0);
                if(_truncDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qtrunc(_truncDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return _truncDecimal;
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to truncate decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    free_decimal(_truncDecimal);
                }
            }else
            if(floor){
                Mdecimal* _floorDecimal=__decimal(mpd_context,0,0);
                if(_floorDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qfloor(_floorDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return _floorDecimal;
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to floor decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    free_decimal(_floorDecimal);
                }                    
            }else{
                Mdecimal* _ceilDecimal=__decimal(mpd_context,0,0);
                if(_ceilDecimal){
                    uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
                    mpd_qceil(_ceilDecimal->mpd,_decimal->mpd,mpd_context,&status);
                    if((status&0xEFBF)==0)return _ceilDecimal;
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to ceil decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                    free_decimal(_ceilDecimal);
                }
            }
        }else
            outputError("No context available for converting a decimal to an integer");
    }
    return NULL;
}

Mdecimal* _getRoundedDecimal(Mdecimal* _decimal){
    if(_decimal){
        Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
        if(mpd_context){
            Mdecimal* _roundDecimal=__decimal(mpd_context,0,0);
            if(_roundDecimal){
                uint32_t status=0;
                mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
                if((status&0xEFBF)==0)return _roundDecimal;
                output("%s",M_ERROR_PREFIX);outputDecimal("Failed to round decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                free_decimal(_roundDecimal);
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
bool free_function(Mfunction* _function){
    if(_function){
        /////// MDH@10JUL2019: moved over to the map element containing the function! free_string(_function->_name);
        free_map(_function->_parameterMap);
        if(_function->type==FT_USER)free_userfunction(_function->functionunion._userfunction);
        free(_function);
        return true;
    }
    return false;
}/* VALIDATED */
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement){
    if(_functionmapelement){
        if(free_functionmapelement(_functionmapelement->_next))_functionmapelement->_next=NULL;
        if(free_function(_functionmapelement->_function)){
            free_string(_functionmapelement->_name);
            free(_functionmapelement);
            return true;
        }
    }
    return false;
}// VALIDATED
void free_functionmap(Mfunctionmap* _functionmap){
    if(_functionmap){
        free_functionmapelement(_functionmap->_first);
        free(_functionmap);
    }
}// VALIDATED
// MDH@20JUL2019: might never get called, wel perhaps on internal functions when it goes out of scope???????
void free_userfunction(Muserfunction* _userfunction){
    if(_userfunction){
        ///////////if(_userfunction->_parameterMap)free_map(_userfunction->_parameterMap);
        // NOTE do NOT call free_value() on the body token value, instead NULL it so the reference count of the value is decremented!!!!
        free_list(_userfunction->_bodyCommandList);
        // replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
        free(_userfunction);
    }
}/* VALIDATED */
// END RELEASERS

// Menvironment stuff
Menvironment* __environment(int32_t oid){int32_t foid=(oid>0?oid:-getOwnerId(100));
    Menvironment* _environment=CALLOC(sizeof(Menvironment),'E',foid);
    if(!_environment){outputError("Failed to create an environment");return NULL;}
    _environment->_variableMap=CALLOC(sizeof(Mmap),'M',abs(foid)); // ascertain that the environment contains a variable map
    if(!_environment->_variableMap){free_environment(_environment,foid);_environment=NULL;outputError("Failed to create the new environment variable map");}
    return _environment;
}/* VALIDATED */
void free_environment(Menvironment* _environment,int32_t oid){int32_t foid=(oid>0?oid:-getOwnerId(100));
    if(_environment){
        if(_environment->_name){freeChars(_environment->_name);_environment->_name=NULL;}
        assignValue(&_environment->_parent,NULL); // MDH@03FEB2020 replacing:
        assignValue(&_environment->execution,NULL); // MDH@03FEB2020 replacing: _environment->_execution=NULL;
        free_map(_environment->_variableMap);
        // free_map(_environment->_functionMap); // MDH@04MAR2020: TODO do we need this??????
        /* MDH@10JUL2019: only Menvironment has a function map!!   
           MDH@20JUL2019: NO user functions may also contain a function map, which is referenced in a user function execution environment
                          and indeed being a referenced they should not be freed (otherwise we would loose these nested functions on
                          freeing the execution environment)
        if(_environment->_functionMap)free_functionmap(_environment->_functionMap);
        */
        FREE(_environment,'E');
    }
}/* VALIDATED */
Menvironment* getEnvironmentParent(Menvironment* _environment){
    return(_environment&&_environment->_parent?getValueEnvironment(_environment->_parent):NULL);
}/* VALIDATED */
Mstring* _getEnvironmentName(Menvironment* _environment){int32_t foid=getOwnerId(101);
    Mstring* _environmentName=__string(foid);
    if(_environmentName){
        Mstring* p=_environmentName;
        while(p&&_environment){
            if(string_length(p)>0)p=string_insert_char(p,0,'.');
            /////// printf("Prepending '%s'.\n",_environment->_name);
            p=string_prepend(p,_environment->_name->chars);
            _environment=getEnvironmentParent(_environment); // MDH@03MAR2020 replacing: _environment->_parent;
        }
        if(!p){free_string(_environmentName,foid);_environmentName=NULL;}
    }
    return _environmentName;
}

// additional function for wrapping environments and functions
Mvalue* _getValueOfFunction(Mfunction* _function,bool freeonfailure){
    if(!_function)return NULL;
    Mvalue* _value=__value("function");
    if(_value){_value->type=VT_FUNCTION;_value->value._function=_function;}else if(freeonfailure)free_function(_function);
    if(!_value)outputError("Failed to wrap function.");
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfEnvironment(Menvironment* _environment,bool freeonfailure){
    if(!_environment)return NULL;
    Mvalue* _value=__value("environment");
    if(_value){_value->type=VT_ENVIRONMENT;_value->value._environment=_environment;}else if(freeonfailure)free_environment(_environment);
    return _value;
}/* VALIDATED */
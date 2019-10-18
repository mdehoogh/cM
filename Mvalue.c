#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "Mstring.h"
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
extern const long double LD_PI; // for Mfacd()
extern const Mdecimalcontext* M_DECIMALCONTEXT; // the application-wide (default) decimal context

void free_variable(Mvariable* _variable){
    if(_variable){
        if(_variable->_name)free(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
        if(_variable->_value)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
        free(_variable);
    }
}/* VALIDATED */
Mvariable* _getVariable(const char* name,Mvaluetype valuetype,bool immutable){
    if(!name||!strlen(name)){
        outputError("No variable name defined");
        return NULL;
    }
    Mvariable* _variable=(Mvariable*)CALLOC(1,sizeof(Mvariable),'V'); // all pointers will be NULL!!
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
            case VT_TEXT:_variable->_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
            case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
            case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
        }
    }else
        _variable->_value=NULL;
    */
    return _variable;
}/* VALIDATED */

void free_listelement(Mlistelement* _listelement){
    if(_listelement){
        if(_listelement->_next)free_listelement(_listelement->_next);
        if(_listelement->_value)decrementReferenceCount(_listelement->_value); ///////// replacing: free_value(_listelement->_value);
        free(_listelement);
    }
}/* VALIDATED */
void free_list(Mlist* _list){
    if(_list){
        if(_list->_first)free_listelement(_list->_first);
        free(_list);
    }
}/* VALIDATED */
void free_mapelement(Mmapelement* _mapelement){
    if(_mapelement){
        if(_mapelement->_next)free_mapelement(_mapelement->_next);
        if(_mapelement->_variable)free_variable(_mapelement->_variable);
        free(_mapelement);
    }
}/* VALIDATED */
void free_map(Mmap* _map){
    if(_map){
        if(_map->_first)free_mapelement(_map->_first);
        free(_map);
    }
}/* VALIDATED */

// MDH@01MAY2019: 'local' function for freeing a value
void free_value(Mvalue* _value){
    if(_value){
        if(amVerbose())output("Value of type %u to free.\n",_value->type);
        // I do not need to free the value itself, only the pointers inside it
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_TOKEN:if(_value->value._token)free_token(_value->value._token);break;
            case VT_INTEGER:if(_value->value._integer)free_integer(_value->value._integer);break;
            case VT_BIGINTEGER:if(_value->value._biginteger)free_biginteger(_value->value._biginteger);break;
            case VT_DECIMAL:if(_value->value._decimal)free_decimal(_value->value._decimal);break;
            case VT_RATIONAL:if(_value->value._rational)free_rational(_value->value._rational);break;
            case VT_REAL:if(_value->value._real)free_real(_value->value._real);break;
            case VT_TEXT:if(_value->value._text)free_text(_value->value._text);break;
            case VT_LIST:if(_value->value._list)free_list(_value->value._list);break;
            case VT_MAP:if(_value->value._map)free_map(_value->value._map);break;
            //case VT_USERFUNCTION:if(_value->value._userfunction)free_userfunction(_value->value._userfunction);break;
        }
        if(amVerbose())output("Type-specific value freed.\n");
        free(_value);
    }else
        output("BUG: No value to free!\n");
}/* VALIDATED */

// manage a list of created values
Mlist* _valueList=NULL;
Mvalue* __value(){
    Mvalue* _value=NULL;
    if(!_valueList)_valueList=(Mlist*)CALLOC(1,sizeof(Mlist),'X');
    if(_valueList){
        Mlistelement* _valueListelement=(Mlistelement*)CALLOC(1,sizeof(Mlistelement),'x'); // both pointers NULL
        if(_valueListelement){
            _value=(Mvalue*)CALLOC(1,sizeof(Mvalue),'V');
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
}/* VALIDATED */

extern const char* const VALUETYPENAMES[];

// can be asked to remove unused values
// TODO check whether it functions correctly (think so though)
size_t getNumberOfRemovedValues(){
    bool showDebugInfo=amDebugging()&&amVerbose();
    unsigned long long tofree=0,removed=0;
    if(_valueList){
        Mlistelement* _valueListelement=_valueList->_first;
        if(showDebugInfo)output("Number of values to check: %llu.\n",_valueList->numberOfElements);
        unsigned long long checked=0;
        while(_valueListelement){
            checked++;
            if(showDebugInfo)output("Checking value #%llu.",checked);
            if(_valueListelement->_value){
                if(showDebugInfo)outputLine("\tChecking the count!");
                if(_valueListelement->_value->count==0){ // unused
                    if(showDebugInfo)output("About to free unused value #%llu of type '%s'.\n",checked,VALUETYPENAMES[_valueListelement->_value->type]);
                    free_value(_valueListelement->_value);
                    _valueListelement->_value=NULL; // just in case
                    tofree++;
                }else
                if(showDebugInfo)outputLine("\tStill in use!");
            }else
                output("%sNo value stored in value #%llu.\n",ERROR_PREFIX,checked);
            _valueListelement=_valueListelement->_next;
        }
        if(showDebugInfo)output("Number of values checked: %llu.\nNumber of value list elements to free: %llu.\n",checked,tofree);
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
        if(tofree>removed)output("WARNING: Failed to free %llu unused value list elements.\n",(tofree-removed));else if(showDebugInfo)outputLine("All unused value list elements freed!");
    }
    return removed;
}/* VALIDATED */

unsigned long long getNumberOfValues(){return (_valueList?_valueList->numberOfElements:0);}/* VALIDATED */

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
        Mstring* _valueText=_getValueText(_value,false);
        output("BUG: Reference count of '%s' of type '%c' already zero.\n",string(_valueText),MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
        free_string(_valueText);
    }else
    if(amVerbose())outputLine("No value to decrement the reference count of.");
    return true;
}/* VALIDATED */
bool incrementReferenceCount(Mvalue* _value){
    if(_value){
        (_value->count)++;
        return true;
    }
    if(amVerbose())outputLine("No value to increment the reference count of.");
    return false;
}/* VALIDATED */
// interface functions that use the above functions
// wrapping the different value type instances
Mvalue* _getUndefinedValue(){return (Mvalue*)CALLOC(1,sizeof(Mvalue),'U');}/* VALIDATED */
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
    if(!_userfunction)return NULL;
    Mvalue* _userfunctionValue=__value();
    if(_userfunctionValue){_userfunctionValue->type=VT_USERFUNCTION;_userfunctionValue->value._userfunction=_userfunction;}else if(freeonfailure)free_userfunction(_userfunction);
    return _userfunctionValue;
}// VALIDATED 
*/
Mvalue* _getDecimalValue(Mdecimal* _decimal,bool freeonfailure){
    if(!_decimal){outputLine("No decimal to wrap.");return NULL;}
    Mvalue* _decimalValue=__value();
    //////////outputDecimal("Wrapping decimal '",_decimal,"'.\n");
    if(_decimalValue){_decimalValue->type=VT_DECIMAL;_decimalValue->value._decimal=_decimal;}else if(freeonfailure)free_decimal(_decimal);
    return _decimalValue;
}/* VALIDATED */
Mvalue* _getIntegerValue(long long ll){
    if(amVerbose())output("Wrapping integer '%lld'.\n",ll);
    Mvalue* _integerValue=__value();
    if(_integerValue){
        _integerValue->value._integer=_getInteger(ll);
        if(!_integerValue->value._integer){free_value(_integerValue);_integerValue=NULL;}else _integerValue->type=VT_INTEGER;
    }
    return _integerValue;
}/* VALIDATED */
Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,bool freeonfailure){
    Mvalue* _bigintegerValue=(_biginteger?__value():NULL);
    if(_bigintegerValue){
        _bigintegerValue->type=VT_BIGINTEGER;_bigintegerValue->value._biginteger=_biginteger;
    }else
    if(_biginteger){
        if(freeonfailure)free_biginteger(_biginteger);
        if(amVerbose()){output("%s",ERROR_PREFIX);outputBiginteger("Failed to wrap big integer '",_biginteger,"'.\n");}
    }else
    if(amVerbose())outputLine("No big integer to wrap.");
    return _bigintegerValue;
}/* VALIDATED */
Mvalue* _getRealValue(long double ld){
    if(amVerbose())output("Wrapping real '%.*Lf'.\n",DBL_DIG,ld);
    Mvalue* _realValue=__value();
    if(_realValue){
        _realValue->value._real=_getReal(ld);
        if(!_realValue->value._real){free_value(_realValue);_realValue=NULL;}else _realValue->type=VT_REAL;
    }
    return _realValue;
}/* VALIDATED */
Mvalue* _getTextValue(char* _s,bool freeonfailure){
    if(!_s)return NULL; // when no input, no go
    Mvalue* _textValue=NULL; // the result, when NULL check freeonfailure
    Mtext* _text=_getText(_s); // for Mstring* sources pass string(Mstring*) into getStringValue() (which points to Mstring->chars which always start with the quote char used in declaring the literal)
    if(_text){
        _textValue=__value();
        if(_textValue){_textValue->type=VT_TEXT;_textValue->value._text=_text;}else free_text(_text); // always free the Mtext if it is not bound!!!
    }
    if(!_textValue)if(freeonfailure)free(_s);
    return _textValue;
}/* VALIDATED */
Mvalue* _getCharTextValue(char _c){
    Mtext* _text=(_c?_getCharText(_c):NULL); // for Mstring* sources pass string(Mstring*) into getStringValue() (which points to Mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _textValue=(_text?__value():NULL);
    if(_textValue){_textValue->type=VT_TEXT;_textValue->value._text=_text;}else if(_text)free_text(_text); // ah do NOT forget to free _text if we haven't been able to create a value!!
    return _textValue;
}/* VALIDATED */
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype){
    Mlist* _list=(Mlist*)CALLOC(1,sizeof(Mlist),'L');
    if(!_list)return NULL;
    _list->valuetype=listValuetype; // register what type of elements this list should have
    Mvalue* _listvalue=__value();
    if(_listvalue){_listvalue->type=VT_LIST;_listvalue->value._list=_list;}else free_list(_list);
    return _listvalue;
}/* VALIDATED */
Mvalue* _getMapValue(Mvaluetype mapValuetype){
    Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
    if(!_map)return NULL;
    _map->valuetype=mapValuetype;
    Mvalue* _mapvalue=__value();
    if(_mapvalue){_mapvalue->type=VT_MAP;_mapvalue->value._map=_map;}else free_map(_map);
    return _mapvalue;
}/* VALIDATED */

// some other wrappers
Mvalue* _getValueOfInteger(Minteger* _integer,bool freeonfailure){
    if(!_integer)return NULL;
    Mvalue* _value=__value();
    if(_value){_value->type=VT_INTEGER;_value->value._integer=_integer;}else if(freeonfailure)free_integer(_integer);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfReal(Mreal* _real,bool freeonfailure){
    if(!_real)return NULL;
    Mvalue* _value=__value();
    if(_value){_value->type=VT_REAL;_value->value._real=_real;}else if(freeonfailure)free_real(_real);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfMap(Mmap* _map,bool freeonfailure){
    if(!_map)return NULL;
    Mvalue* _value=__value();
    if(_value){_value->type=VT_MAP;_value->value._map=_map;}else if(freeonfailure)free_map(_map);
    return _value;
}/* VALIDATED */
Mvalue* _getValueOfToken(Mtoken* _token,bool freeonfailure){
    if(!_token)return NULL;
    Mvalue* _value=__value();
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
Mmap* _getRealMap(char* name,Mvalue* _realValue){
    if(name&&_realValue){
        Mvariable* _realVariable=_getVariable(name,VT_REAL,true);
        if(_realVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_realVariable->_value,_realValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
                    _mapelement->_variable=_realVariable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    if(amDebugging())outputLine("Returning the single real map!");
                    return _map;
                }
                outputError("Failed to create the real variable map");
                free_mapelement(_mapelement);
            }else
                outputError("Failed to create the real variable map element");
            free_variable(_realVariable);
        }else
            outputError("Failed to create the real variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getMap(char *name){
    Mvariable* _variable=_getVariable(name,VT_UNDEFINED,true);
    if(_variable){
        Mmapelement* _mapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
        if(_mapelement){
            Mmap* _map=CALLOC(1,sizeof(Mmap),'M');
            if(_map){
                _mapelement->_variable=_variable;
                _map->_first=_mapelement;
                _map->_last=_mapelement;
                _map->numberOfElements=1;
                return _map;
            }
            free_mapelement(_mapelement);
        }
        free_variable(_variable);
    }
    return NULL;
}/* VALIDATED */
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue){
    // NOTE wait with filling the single integer value map until we have all the ingredients
    if(name&&_integerValue){
        Mvariable* _integerVariable=_getVariable(name,VT_INTEGER,true);
        if(_integerVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_integerVariable->_value,_integerValue); ///////  incrementReferenceCount(_integerValue); // now bound to the integer variable
                    _mapelement->_variable=_integerVariable;
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    if(amVerbose())outputLine("Returning the single integer map!");
                    return _map;
                }
                outputError("Failed to create the integer variable map");
                free_mapelement(_mapelement);
            }else
                outputError("Failed to create the integer variable map element");
            free_variable(_integerVariable);
        }else
            outputError("Failed to create the integer variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getStringStringMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
        }else
            output("%sMap element attribute keys '%s' and '%s' undefined or the same.\n",ERROR_PREFIX,name1,name2);
    }else
        outputError("Not both map element attribute keys defined");
    return NULL;
}/* VALIDATED */
Mmap* _getRealRealMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
                if(_map){
                    _mapelement1->_variable=_getVariable(name1,VT_REAL,true);
                    _mapelement2->_variable=_getVariable(name2,VT_REAL,true);
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getListMap(char* name,Mvalue* _listValue){
    if(name&&_listValue){
        Mvariable* _listVariable=_getVariable(name,VT_LIST,true);
        if(_listVariable){
            Mmapelement* _mapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
                if(_map){
                    assignValue(&_listVariable->_value,_listValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
                    _mapelement->_variable=_listVariable;
                   _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    if(amDebugging())outputLine("Returning the single list map!");
                    return _map;
                }
                outputError("Failed to create the list variable map");
                free_mapelement(_mapelement);
            }else
                outputError("Failed to create the list variable map element");
            free_variable(_listVariable);
       }else
            outputError("Failed to create the list variable");
    }
    return NULL;
}/* VALIDATED */
Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
            free_mapelement(_mapelement3);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getTokenTokenMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3){
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
            free_mapelement(_mapelement3);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3){
    if(name1&&name2&&name3){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
            free_mapelement(_mapelement3);
        }
    }
    return NULL;
}/* VALIDATED */
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4){
    if(name1&&name2&&name3&&name4){
        if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
            strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name3,name4)){
            Mmapelement* _mapelement1=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement2=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement3=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            Mmapelement* _mapelement4=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
            if(_mapelement1&&_mapelement2&&_mapelement3&&_mapelement4){
                Mmap* _map=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
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
            free_mapelement(_mapelement1);
            free_mapelement(_mapelement2);
            free_mapelement(_mapelement3);
            free_mapelement(_mapelement4);
        }
    }
    return NULL;
}/* VALIDATED */
// end helper functions 

// LIST STUFF
Mvalue* _getValueOfList(Mlist* _list,bool freeonfailure){
    if(!_list)return NULL;
    Mvalue* _value=__value();
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
                    if(_listelement->index<=listelementindex)output("%sList element index (%lld) below the expected list element index (%lld).\n",ERROR_PREFIX,_listelement->index,listelementindex);
                    listelementindex=_listelement->index;
                    if(!_listelement->_next){
                        if(_list->_last!=_listelement)outputError("Registered last list element not equal to the actual last list element");
                        break;                        
                    }
                    output("List element with index %llu OK.\n",_listelement->index);
                    _listelement=_listelement->_next;
                }
                if(l>0)outputError("Less elements in list than accounted for");else 
                if(l<0)output("%s%lld more elements in list than counted.\n",ERROR_PREFIX,(-l));
            }else
                output("%sList with %lld elements does not have a first element!\n",ERROR_PREFIX,l);
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
unsigned long long appendedToList(Mlist * const _list,Mvalue const * const _value,long long index){
    if(!_list){outputError("No list to append to");return 0;} // MDH@18OCT2019: let's allow NULLing list elements (i.e. accepting _value to be NULL)
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    // MDH@17OCT2019: index 0 now does not indicate to append to the end anymore but now indicates that the given value should be prepended!!!!
    if(index==M_LL_INVALID)index=lastindex+1; // MDH@17OCT2019: we need to be able to append as well (can't use 0 anymore!!!!)
    if(index<0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    // MDH@17OCT2019: a negative index might still end up with index 0, this happens with -len(x)-1, ok, for now just accept this when it happens
    if(index<0){output("%sIndex %lld of (new) list element too small.\n",ERROR_PREFIX,index);return 0;} // MDH@17OCT2019: can't return negative value!!!
    if(amVerbose())outputValue((index>0?"Appending '":"Prepending '"),_value,"' to a list.\n");
    // MDH@23MAY2019: let's allow inserting or replacing as well
    // determine _listelement as element to host the value, store the successor in _nextlistelement
    Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index>0&&index<=lastindex?_list->_first:NULL);
    if(_listelement){ // we are not appending and we have a first element, so this might be an insert or replace
        // NOTE testing _listelement is just a fail-safe as that should never happen
        while(index>_listelement->index){
            _prevListelement=_listelement;
            if(!_listelement->_next){outputValue("\nBUG: Index of list element '",_listelement->_value,"' probably out of order.");return 0;}
            _listelement=_listelement->_next;
        }
        // if we're going to insert there will be a successor
        if(_listelement->index!=index){_nextListelement=(_prevListelement?_prevListelement->_next:_list->_first);_listelement=NULL;} // so that we are forced to create one
    }else // we'll be insertingappending/prepending, so the current last is the predecessor (and no successor)
    if(index>0) // MdH@17OCT2019: when not prepending...
        _prevListelement=_list->_last;
    // if we do not have a list element ascertain to have one
    if(!_listelement){ // not yet present in list, so we have to create a new element
        _listelement=(Mlistelement*)CALLOC(1,sizeof(Mlistelement),'l');
        if(!_listelement){outputError("Failed to create a list element to insert");return 0;} // failure
    }
    assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
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
            if(amDebugging())if(_nextListelement)outputLine("A element to consider!");else outputLine("No next element to consider!");
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
// END LIST STUFF

// MAP STUFF
// MDH@24MAY2019: if already in the map should replace the current value
bool appendedToMap(Mmap* const _map,const char* const attributeName,const Mvalue* const _attributeValue){
    if(_map&&attributeName){
        if(amVerbose()){output("Setting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.\n");}
        Mmapelement* _mapelement=_map->_first;
        while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name,attributeName))_mapelement=_mapelement->_next;
        if(!_mapelement){ // not found
            _mapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m'); // NOTE no need to set _next because it is now NULL
            if(_mapelement){
                _mapelement->_variable=_getVariable(attributeName,VT_UNDEFINED,false);
                if(_mapelement->_variable){ // the variable was created so attach in map
                    if(_map->numberOfElements)_map->_last->_next=_mapelement;else _map->_first=_mapelement;
                    _map->_last=_mapelement;
                    _map->numberOfElements++;
                }else{ // we have a map element BUT no variable, so no go
                    free_mapelement(_mapelement);_mapelement=NULL;
                }
            }else
            if(amVerbose())outputError("Failed to create new map element");
        }
        if(_mapelement){
            assignValue(&_mapelement->_variable->_value,_attributeValue); // replace the current attribute value with the new value
            return true;
        }
    }
    return false;
}/* VALIDATED */

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
}/* VALIDATED */
// END MAP STUFF

Mvalue* _getRationalValue(Mrational* _rational,bool freeonfailure){
    if(!_rational)return NULL;
    Mvalue* _value=__value();
    if(_value){_value->type=VT_RATIONAL;_value->value._rational=_rational;}else if(freeonfailure)free_rational(_rational);
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
		Mlistelement* _listelement=_list->_first;
		Mvalue* _listelementValue;
		while(p&&_listelement){
            ///////outputChar('$');
            // increment listindex until it is equal to _listelement->index
            if(_listelement->index==0)break; // VERY UNLIKELY AS field index should be monotonically increasing
            while(listindex<_listelement->index){listindex++;p=string_append_char(p,',');}
            /////////p=appendull(p,_listelement->index);p=string_append_char(p,':');if(!p)break;
	        //////////outputChar('.');
			_listelementValue=_listelement->_value;
			if(_listelementValue){
				Mstring* _listelementValueText=_getValueText(_listelementValue,false); // to be freed asap
				if(_listelementValueText){
					p=string_append(p,string(_listelementValueText));
					free_string(_listelementValueText); // release AFTER copying over
				}
			}
			_listelement=_listelement->_next;
		}
		p=string_append_char(p,']');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(!p){free_string(result);result=NULL;}
	}
	return result;
}/* VALIDATED */
Mstring* _getMapText(Mmap* _map,bool showcurlybraces,bool showquotes,bool showmissings){
	Mstring* result=__string();
    if(result){
	    Mstring* p=result;
        if(amDebugging())p=string_append_char(p,'m');
        if(showcurlybraces)p=string_append_char(p,'{');
		//////output("%s",string(p));
		Mmapelement* _mapelement=_map->_first;
		while(p&&_mapelement){
			//////output("%s","start");
			Mvariable* _mapVariable=_mapelement->_variable;
			if(_mapVariable){
                // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
                if(showquotes)p=string_append_char(p,'\'');
                p=string_append(p,_mapVariable->_name);
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
			if(_mapelement)p=string_append(p,", "); // only when there's a next map element to process
			////output("%s","next");
		}
		//////output("%s(%d)",string(p),string_length(p));
		if(showcurlybraces)p=string_append_char(p,'}');
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){free_string(result);result=NULL;}
	}
	return result;
}/* VALIDATED */

Mstring* _getValueText(const Mvalue* const _value,bool dequoted){
	// NOTE whatever is returned should be freed
	Mstring* valueText=NULL;
    ////outputChar('.');
	if(_value){
        ////outputChar('+');
		////output("TYPE: %d\n",_value->type);
		switch(_value->type){
			case VT_INTEGER:valueText=_getIntegerText(_value->value._integer);break;
            case VT_BIGINTEGER:valueText=_getBigintegerText(_value->value._biginteger);break; // how many characters do we need????
            case VT_DECIMAL:valueText=_getDecimalText(_value->value._decimal,false);break; // fixedpoint to obligatory (i.e. e-notation allowed for very big/small (positive) numbers)
            case VT_RATIONAL:valueText=_getRationalText(_value->value._rational);break;
			case VT_REAL:valueText=_getRealText(_value->value._real);break;
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
			default:break;
		}
	}
    if(valueText)if(amAssisting())valueText=appendll(string_append_char(valueText,'#'),_value->count); // show the reference count as well
    /////outputChar('.');
    return(valueText?valueText:_getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}/* VALIDATED */
void outputValue(const char* const prefix,const Mvalue* const value,const char* const suffix){
    if(prefix)output("%s",prefix);
    if(value){
        Mstring* _valueText=_getValueText(value,false); // free asap
        if(_valueText){output("%s",string(_valueText));free_string(_valueText);}
    }else
        outputChar('-');
    if(suffix)output("%s",suffix);
}/* VALIDATED */

long long getValueInteger(const Mvalue* const _value){
    // ASSERTION if _value can be converted to an integer,it should not equal invalid!!!!
    if(_value){
        switch(_value->type){
            case VT_INTEGER:return _value->value._integer->ll;
		    case VT_REAL:return double2long(_value->value._real->ld);
            case VT_BIGINTEGER:return biginteger2long(_value->value._biginteger);
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
   		    default:break;
        }
        // TODO sometimes reals can also represent integers!!!
    }
    return M_LL_INVALID;
}/* VALIDATED */

// MDH@09OCT2019: TODO is there a better way than through parsing?????
long double getBigintegerLongDouble(Mbiginteger* biginteger){
    long double ldBiginteger=M_LD_NAN;
   	Mstring* _bigintegerText=(biginteger?_getBigintegerText(biginteger):NULL);
    if(_bigintegerText){ldBiginteger=_strtold(string(_bigintegerText),ldBiginteger);free_string(_bigintegerText);}
	return ldBiginteger;
}
long double getValueReal(const Mvalue* const _value){
    // MDH@09OCT2019: a little more complicated then just getting out the real in that all scalar numeric values are convertable
    long double valueReal=M_LD_NAN;
    if(_value)
    switch(_value->type){
        case VT_INTEGER:valueReal=_value->value._integer->ll;break;
        case VT_BIGINTEGER:valueReal=getBigintegerLongDouble(_value->value._biginteger);break;
        case VT_REAL:valueReal=_value->value._real->ld;break;
        case VT_DECIMAL:valueReal=getDecimalLongDouble(_value->value._decimal);break;
        default:break;
    }
    return valueReal;
}/* VALIDATED */

long double getRealLongDouble(const Mreal* const _real){return(_real?_real->ld:M_LD_NAN);}/* VALIDATED */

Mbiginteger* _getValueBiginteger(const Mvalue* const _value){
    if(_value)
        switch(_value->type){
        case VT_BIGINTEGER:return _value->value._biginteger;
        case VT_INTEGER:return _getBiginteger(_value->value._integer->ll);
        case VT_RATIONAL:return _rational2biginteger(_value->value._rational); // TODO how can we be certain that the returned big integer is actually used? well, it should as this is _getValueBiginteger meaning you have to free it if you don't use it!!!
        case VT_REAL:
            {
                // this is a bit of a nuisance when the double is out of the VT_INTEGER range
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_set_longdouble(_biginteger,_value->value._real->ld)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
        case VT_TEXT:
            {
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_read_radix(_biginteger,_value->value._text->_c,10)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
        case VT_TOKEN:
            {
                Mbiginteger* _biginteger=__biginteger();
                if(_biginteger&&mp_read_radix(_biginteger,string(_value->value._token->text),10)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;}
                if(!_biginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
                return _biginteger;
            }
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
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED); // this will be a new value that (being managed) will be freed automatically when not bound, including the list contained by it!!
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
                        if(_attributeNameValue->type==VT_REAL)_attributeNameValueText=_getRealText(_attributeNameValue->value._real);else
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
                long long index=atoll(_mapelement->_variable->_name);
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
                Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED);
                Mlist* _maplistelement=(_maplistelementValue?_maplistelementValue->value._list:NULL);
                if(_maplistelement){
                    // if we fail to construct the maplist element or to add it
                    // NOTE the attribute name does not start with a quote character wich we need to call _getTextValue
                    // TODO the following could be restructured I suppose
                    Mstring* _attributeName=__string();
                    if(_attributeName){ // should be freed
                        Mstring* p=_attributeName;
                        p=string_append_char(p,'\'');
                        p=string_append(p,_mapelement->_variable->_name);
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

Mlist* _getListOfType(Mvaluetype valuetype){Mlist* _list=CALLOC(1,sizeof(Mlist),'L');_list->valuetype=valuetype;return _list;}/* VALIDATED */
Mmap* _getMapOfType(Mvaluetype valuetype){Mmap* _map=CALLOC(1,sizeof(Mmap),'M');_map->valuetype=valuetype;return _map;}/* VALIDATED */

bool isValueZero(Mvalue* value){
    if(value){
        if(value->type==VT_INTEGER)return value->value._integer->ll==0;
        if(value->type==VT_BIGINTEGER)return isBigintegerZero(value->value._biginteger);
        if(value->type==VT_REAL)return ldIsZero(value->value._real->ld);
        if(value->type==VT_DECIMAL)return isDecimalZero(value->value._decimal);
        if(value->type==VT_RATIONAL)return isBigintegerZero(value->value._rational->num);
    }
    return false;
}/* VALIDATED */
bool isValueOne(Mvalue* value){
    if(value){
        if(value->type==VT_INTEGER)return value->value._integer->ll==1;
        if(value->type==VT_BIGINTEGER)return isBigintegerOne(value->value._biginteger);
        if(value->type==VT_REAL)return value->value._real->ld==1;
        if(value->type==VT_DECIMAL)return isDecimalOne(value->value._decimal);
        if(value->type==VT_RATIONAL)return isRationalOne(value->value._rational);
    }
    return false;
}/* VALIDATED */
bool isValuePositive(Mvalue* value){
    if(value){
        if(value->type==VT_INTEGER)return value->value._integer->ll>0;
        if(value->type==VT_BIGINTEGER)return isBigintegerPositive(value->value._biginteger);
        if(value->type==VT_REAL)return ldIsPositive(value->value._real->ld);
        if(value->type==VT_DECIMAL)return isDecimalPositive(value->value._decimal);
        if(value->type==VT_RATIONAL)return isBigintegerPositive(value->value._rational->num); // assuming the numerator is never NULL and the denominator is always positive
    }
    return false;
}/* VALIDATED */
bool isValueNegative(Mvalue* value){
    if(value){
        if(value->type==VT_INTEGER)return value->value._integer->ll<0;
        if(value->type==VT_BIGINTEGER)return isBigintegerNegative(value->value._biginteger);
        if(value->type==VT_REAL)return ldIsNegative(value->value._real->ld);
        if(value->type==VT_DECIMAL)return isDecimalNegative(value->value._decimal);
        if(value->type==VT_RATIONAL)return isBigintegerNegative(value->value._rational->num);
    }
    return false;
}/* VALIDATED */
bool isValueScalar(Mvalue* value){
    if(value)switch(value->type){case VT_INTEGER:case VT_BIGINTEGER:case VT_DECIMAL:case VT_RATIONAL:case VT_REAL:case VT_TEXT:case VT_TOKEN:return true;}
    return false;
}/* VALIDATED */

// null test for the value to be considered NULL
bool isValueNull(Mvalue* value){
    if(value)
    switch(value->type){
        case VT_INTEGER:return !value->value._integer;
        case VT_BIGINTEGER:return !value->value._biginteger;
        case VT_DECIMAL:return !value->value._decimal;
        case VT_RATIONAL:return !value->value._rational;
        case VT_REAL:return !value->value._real;
        case VT_TEXT:return !value->value._text;
        case VT_LIST:return !value->value._list;
        case VT_MAP:return !value->value._map;
        case VT_TOKEN:return !value->value._token;
        default:break;
    }
    return true;
}/* VALIDATED */
// MDH@18JUL2019: we consider certain non-null values as undefined, this is to fill the gap between non-null values that represent missings
//                TODO is a map or list undefined when empty???????
bool isValueUndefined(Mvalue* value){
    // values that are considered NULL are also undefined
    if(!isValueNull(value))
    switch(value->type){
        case VT_INTEGER:return value->value._integer->ll==M_LL_INVALID;
        case VT_BIGINTEGER:return false;
        case VT_DECIMAL:return mpd_isnan((mpd_t*)value->value._decimal); // sames right but no idea how to set/get this // decimal points directly to mpd_t so we can cast
        case VT_RATIONAL:return false;
        case VT_REAL:return ldIsNaN(value->value._real->ld);
        case VT_TEXT:return false; ////strlen(_value->value._text->_c)==0;
        case VT_LIST:return false; ////Mlen(_value)==0;
        case VT_MAP:return false; ////Mlen(_value)==0;
        case VT_TOKEN:return false; /////string_length(_value->value._token->text)==0;
        default:break;
    }
    return true;
}/* VALIDATED */

// MDH@04JUN2019: based on https://stackoverflow.com/questions/4637967/algorithm-challenge-generate-continued-fractions-for-a-float/56444882#56444882
Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter){
    // if iterations, you're supposed to return all iteration results
    Mlist* _iterationsList=_getListOfType(VT_UNDEFINED);
    if(_iterationsList){ // should be freed when NOT returned!!
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
                    ////////////replacing (see above): if(fabsl(delta)<=M_LD_Q_EPS)break; // if the p and q we've got are fine, stop!!!
                    _rational=_getRational(_getBiginteger(neg?-p:p),_getBiginteger(q),delta,false,true); // construct the intermediate result without normalizing
                    if(!_rational){output("%sFailed to construct the rational approximation %lld/%lld",ERROR_PREFIX,p,q);break;}
                    // NOTE once we have the created big integer numerator and denominator bound in _rational we're responsible of freeing _rational when not bound
                    // NOT being able to append the intermediate result to the list shouldn't be enough reason to abort, as long as we manage to add the end result
                    Mvalue* _rationalValue=_getRationalValue(_rational,true);
                    if(!_rationalValue){outputError("Failed to value wrap the intermediate rational approximation to a real");break;}
                    // NOTE probably best to break if we can't append approximations!!
                    // NOTE no need to free _rational even then as it is bound in _rationalValue so it will be freed anyway
                    if(!appendedToList(_iterationsList,_rationalValue,i)){/*free_rational(_rational);*/outputError("Failed to register a rational approximation");break;}
                    // if we get here success in updating the iterations list!!!!
                    // if delta is now zero, we're done!!!
                    if(ldIsZero(delta))break; ///// MDH@07JUN2019: when a list is returned like this don't stop below the system's epsilon but only when the delta is zero!!!!
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

void assignValue(Mvalue** _valueholder, Mvalue* const _value){
    if(*_valueholder)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
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
        while(_listelement&&appendedToList(_result,oneArgumentFunction(_listelement->_value),_listelement->index))_listelement=_listelement->_next;
    }
    return _result;
}/* VALIDATED */
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction){
    Mmap* _result=NULL;
    if(_map){
        _result=_getMapOfType(_map->valuetype);
        Mmapelement* _mapelement=_map->_first;
        while(_mapelement&&appendedToMap(_result,_mapelement->_variable->_name,oneArgumentFunction(_mapelement->_variable->_value)))_mapelement=_mapelement->_next;
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
        if(!_rational->den||mp_cmp(_rational->den,getBigintegerOne())==MP_EQ)return(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1));
        // if the numerator equals 1, the result is 0
        if(!_rational->num||mp_cmp(_rational->num,getBigintegerOne())==MP_EQ)return _getBiginteger(0); // with the numerator at least equal to 2 the result will always be 0
        bool neg=mp_isneg(_rational->num); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _dividend=NULL;
        Mbiginteger* _absnum=__biginteger(); // to be freed asap
        if(_absnum){ // freeable
            if(mp_abs(_rational->num,_absnum)==MP_OKAY){
                Mbiginteger* _twicenum=__biginteger();
                if(_twicenum){ // freeable
                    if(mp_mul_2(_absnum,_twicenum)==MP_OKAY){
                        Mbiginteger* _twiceden=__biginteger();
                        if(_twiceden){
                            if(mp_mul_2(_rational->den,_twiceden)==MP_OKAY){
                                Mbiginteger* _remainder=__biginteger();
                                if(_remainder){
                                    _dividend=__biginteger();
                                    if(_dividend){
                                        bool success=(mp_div(_twicenum,_twiceden,_dividend,_remainder)==MP_OKAY);
                                        // increment _dividend if _remainder larger than denominator
                                        if(success&&mp_cmp(_remainder,_rational->den)==MP_GT&&mp_incr(_dividend)!=MP_OKAY)success=false;
                                        if(success&&neg&&mp_neg(_dividend,_dividend)!=MP_OKAY)success=false;
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
        if(!_rational->num)return _getBiginteger(_rational->den?0:1); // if the numerator is undefined (i.e. equals 1), either return 0 or 1 (denominator 1)
        bool neg=mp_isneg(_rational->num); // determine whether negative or not
        // get the absolute value of the numerator
        Mbiginteger* _absnum=__biginteger(); // to be freed asap
        if(mp_abs(_rational->num,_absnum)!=MP_OKAY){free_biginteger(_absnum);outputError("Failed to compute the absolute of a big integer");return NULL;}
        Mbiginteger *_dividend=__biginteger(),*_remainder=__biginteger();
        bool success=(mp_div(_absnum,_rational->den,_dividend,_remainder)==MP_OKAY);
        if(success&&mp_iszero(_remainder)!=MP_YES){ // division succeeded with a non-zero remainder
            if(neg){
                if(mp_neg(_dividend,_dividend)==MP_OKAY){
                    // if flooring (instead of ceiling) we have to subtract one
                    if(floor&&!towardszero&&mp_decr(_dividend)!=MP_OKAY){
                        success=false;
                        outputError("Failed to decrement the truncated negative big integer");
                    }
                }else{
                    success=false;
                    outputError("Failed to negate the big integer dividend");
                }
            }else{
                if(!floor&&!towardszero&&mp_incr(_dividend)!=MP_OKAY){
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
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:get_default_mpd_context());
        if(mpd_context){
            if(floor){
                if(towardszero){
                    Mdecimal* _truncDecimal=__decimal(mpd_context,0,0);
                    if(_truncDecimal){
                        uint32_t status;
                        mpd_qtrunc(_truncDecimal->mpd,_decimal->mpd,mpd_context,&status);
                        if((status&0xEFBE)==0)return _truncDecimal;
                        output("%s",ERROR_PREFIX);outputDecimal("Failed to truncate decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                        free_decimal(_truncDecimal);
                    }
                }else{
                   Mdecimal* _floorDecimal=__decimal(mpd_context,0,0);
                    if(_floorDecimal){
                        uint32_t status;
                        mpd_qfloor(_floorDecimal->mpd,_decimal->mpd,mpd_context,&status);
                        if((status&0xEFBE)==0)return _floorDecimal;
                        output("%s",ERROR_PREFIX);outputDecimal("Failed to floor decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                        free_decimal(_floorDecimal);
                    }                    
                }
            }else{
                if(towardszero){

                }else{
                    Mdecimal* _ceilDecimal=__decimal(mpd_context,0,0);
                    if(_ceilDecimal){
                        uint32_t status;
                        mpd_qceil(_ceilDecimal->mpd,_decimal->mpd,mpd_context,&status);
                        if((status&0xEFBE)==0)return _ceilDecimal;
                        output("%s",ERROR_PREFIX);outputDecimal("Failed to ceil decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                        free_decimal(_ceilDecimal);
                    }
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
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:get_default_mpd_context());
        if(mpd_context){
            Mdecimal* _roundDecimal=__decimal(mpd_context,0,0);
            if(_roundDecimal){
                uint32_t status;
                mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
                if((status&0xEFBE)==0)return _roundDecimal;
                output("%s",ERROR_PREFIX);outputDecimal("Failed to round decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
                free_decimal(_roundDecimal);
            }
        }else
            outputError("No context available for rounding a decimal");
    }
    return NULL;
}

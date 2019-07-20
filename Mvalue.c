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
            case VT_TEXT:if(_value->value._text)free_text(_value->value._text);break;
            case VT_LIST:if(_value->value._list)free_list(_value->value._list);break;
            case VT_MAP:if(_value->value._map)free_map(_value->value._map);break;
            //case VT_USERFUNCTION:if(_value->value._userfunction)free_userfunction(_value->value._userfunction);break;
        }
        if(amVerbose())output("Type-specific value freed.");
        free(_value);
    }else
        output("BUG: No value to free!");
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
    unsigned long long tofree=0,removed=0;
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
        output("BUG: Reference count of '%s' of type '%c' already zero.\n",_valueText,MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
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
    if(!_decimal)return NULL;
    Mvalue* _decimalValue=__value();
    if(_decimalValue){_decimalValue->type=VT_DECIMAL;_decimalValue->value._decimal=_decimal;}else if(freeonfailure)free_decimal(_decimal);
    return _decimalValue;
}/* VALIDATED */
Mvalue* _getIntegerValue(long long ll){
    if(amVerbose())output("Wrapping integer '%ll'.\n",ll);
    Mvalue* _integerValue=__value();
    if(_integerValue){
        _integerValue->value._integer=_getInteger(ll);
        if(!_integerValue->value._integer){free_value(_integerValue);_integerValue=NULL;}else _integerValue->type=VT_INTEGER;
    }
    return _integerValue;
}/* VALIDATED */
Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,bool freeonfailure){
    if(!_biginteger)return NULL;
    Mvalue* _bigintegerValue=__value();
    if(_bigintegerValue){_bigintegerValue->type=VT_BIGINTEGER;_bigintegerValue->value._biginteger=_biginteger;}else if(freeonfailure)free_biginteger(_biginteger);
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
                    if(amDebugging())outputLine("Returning the single list map");
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
                    output("List element with index %llu OK.",_listelement->index);
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
unsigned long long appendedToList(Mlist* const _list,Mvalue const * const _value,long long index){
    if(!_list||!_value){outputError("No list to append to or no value to append");return 0;}
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    if(index<=0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    if(index<=0){output("%sIndex %lld of (new) list element too small.\n",ERROR_PREFIX,index);return 0;}
    if(amVerbose())outputValue("Appending '",_value,"' to list.\n");
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
        _listelement=(Mlistelement*)CALLOC(1,sizeof(Mlistelement),'l');
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
                if(showmissings||!isUndefined(_mapVariable->_value)){
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
    ////////outputChar('.');
    return(valueText?valueText:_getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}/* VALIDATED */
void outputValue(const char* const prefix,const Mvalue* const _value,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_value){
        Mstring* _valueText=_getValueText(_value,false); // free asap
        if(_valueText){output("%s",string(_valueText));free_string(_valueText);}
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
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
long double getValueReal(const Mvalue* const _value){return(_value&&_value->type==VT_REAL?_value->value._real->ld:M_LD_NAN);}/* VALIDATED */
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
                if(!_maplistelement||!appendedToList(_maplistelement,_getIntegerValue(_listelement->index),0)||!appendedToList(_maplistelement,_listelement->_value,0)||!appendedToList(_maplist,_maplistelementValue,0)){
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
                            if(_attributeNameValue&&appendedToList(_maplistelement,_attributeNameValue,0)){
                                if(!appendedToList(_maplistelement,_mapelement->_variable->_value,0)||!appendedToList(_maplist,_maplistelementValue,0)){
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

Mlist* _getListOfType(Mvaluetype valuetype){Mlist* _list=CALLOC(1,sizeof(Mlist),'L');_list->valuetype=valuetype;return _list;}/* VALIDATED */
Mmap* _getMapOfType(Mvaluetype valuetype){Mmap* _map=CALLOC(1,sizeof(Mmap),'M');_map->valuetype=valuetype;return _map;}/* VALIDATED */

bool isValueZero(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==0;
        if(_value->type==VT_BIGINTEGER)return isBigintegerZero(_value->value._biginteger);
        if(_value->type==VT_REAL)return ldIsZero(_value->value._real->ld);
        if(_value->type==VT_DECIMAL)return isDecimalZero(_value->value._decimal);
        if(_value->type==VT_RATIONAL)return isBigintegerZero(_value->value._rational->num);
    }
    return false;
}/* VALIDATED */
bool isValueOne(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==1;
        if(_value->type==VT_BIGINTEGER)return isBigintegerOne(_value->value._biginteger);
        if(_value->type==VT_REAL)return _value->value._real->ld==1;
        if(_value->type==VT_DECIMAL)return isDecimalOne(_value->value._decimal);
        if(_value->type==VT_RATIONAL)return isRationalOne(_value->value._rational);
    }
    return false;
}/* VALIDATED */

// null test for the value to be considered NULL
bool isNull(Mvalue* _value){
    if(_value)
    switch(_value->type){
        case VT_INTEGER:return !_value->value._integer;
        case VT_BIGINTEGER:return !_value->value._biginteger;
        case VT_DECIMAL:return !_value->value._decimal;
        case VT_RATIONAL:return !_value->value._rational;
        case VT_REAL:return !_value->value._real;
        case VT_TEXT:return !_value->value._text;
        case VT_LIST:return !_value->value._list;
        case VT_MAP:return !_value->value._map;
        case VT_TOKEN:return !_value->value._token;
        default:break;
    }
    return true;
}/* VALIDATED */
// MDH@18JUL2019: we consider certain non-null values as undefined, this is to fill the gap between non-null values that represent missings
//                TODO is a map or list undefined when empty???????
bool isUndefined(Mvalue* _value){
    // values that are considered NULL are also undefined
    if(!isNull(_value))
    switch(_value->type){
        case VT_INTEGER:return _value->value._integer->ll==M_LL_INVALID;
        case VT_BIGINTEGER:return false;
        case VT_DECIMAL:return mpd_isnan(_value->value._decimal); // sames right but no idea how to set/get this
        case VT_RATIONAL:return false;
        case VT_REAL:return ldIsNaN(_value->value._real->ld);
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
// applying unary operators by means of functions
// math functions: independent of the execution environment but still receive it...
// TODO how about applying the functions to decimals and rationals
/*
returns the largest integer equal to or smaller than \p _value
\parameter _value the value to floor
*/
Mvalue* Mfloor(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(floorl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mfloor),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mfloor),true);
    }
    return NULL;
}
Mvalue* Mtrunc(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(truncl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,true),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtrunc),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtrunc),true);
    }
    return NULL;
}
/*
 * returns nearest integer value
 */
Mvalue* Mround(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(roundl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRoundedRationalInteger(_value->value._rational),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil),true);
    }
    return NULL;
}
/*
 * returns integer equal to or larger than
 */
Mvalue* Mceil(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(ceill(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,false,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil),true);
    }
    return NULL;
}
Mvalue* Msin(Mvalue* _value){
    if(_value){
        if(amVerbose()){outputValue("Applying sin() to '",_value,"' of type ");output("%s(%u).\n",VALUETYPENAMES[_value->type],_value->type);}
        if(_value->type==VT_REAL)return _getRealValue(sinl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sin(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msin),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msin),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcos(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(cosl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(cos(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcos),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcos),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtan(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(tanl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(tan(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtan),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtan),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcosh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(coshl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(cosh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcosh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcosh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Msinh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(sinhl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sinh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msinh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msinh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtanh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(tanhl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(tanh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtanh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtanh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mexp(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(expl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp),true);
    }
    return NULL;
}
Mvalue* Mlog(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(logl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(log(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog),true);
    }
    return NULL;
}
Mvalue* Mlog10(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(log10l(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(log10(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog10),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog10),true);
    }
    return NULL;
}
Mvalue* Msqrt(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(sqrtl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sqrt(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msqrt),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msqrt),true);
    }
    return NULL;
}
// two-argument power function (complicates things considerably)
// TODO check different types convert to long double and use powl to compute the power!!
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue){
    if(_value&&_exponentValue){
        if(_value->type==VT_REAL&&_exponentValue->type==VT_REAL)return _getRealValue(powl(_value->value._real->ld,_exponentValue->value._real->ld));
        if(_value->type==VT_INTEGER&&_exponentValue->type==VT_INTEGER)return _getRealValue(pow(_value->value._integer->ll,_exponentValue->value._integer->ll));
    }
    return NULL;
}
// end math functions

Mvalue* Mneg(Mvalue* _value){ // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_REAL)return _getRealValue(-_value->value._real->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot),true);
    }
    return NULL;
}/* VALIDATED */
// TODO can we not a string??????
Mvalue* Mbnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mnull(Mvalue* _value){
    return _getIntegerValue(isNull(_value)?1:0);
}/* VALIDATED */
Mvalue* Mundefined(Mvalue* _value){
    return _getIntegerValue(isUndefined(_value)?1:0); // MDH@18JUL2019: isUndefined() now comes in handy
}/* VALIDATED */

// TODO the length of a text is the number of characters in a text????
Mvalue* Mlen(Mvalue* _value){
    long long result=0;
    if(_value){
        switch(_value->type){
            case VT_INTEGER:case VT_BIGINTEGER:case VT_REAL:case VT_TEXT:result=1;break;
            case VT_LIST:result=_value->value._list->numberOfElements;break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            default:break;
        }
    }
    return _getIntegerValue(result);
}/* VALIDATED */

// MDH@29MAY2019: how about forcing the result to be a big integer instead of a long double?????
Mvalue* Mfacd(Mvalue* _value){
    // Stirling formula to compute the number of factorial digits in n!: return 
    // get the integer out of the value
    long long ll=getValueInteger(_value);
    return (ll>0?_getIntegerValue(floor( ((ll+0.5)*log(ll) - ll + 0.5*log(2*LD_PI))/log(10) ) + 1):NULL);
}/* VALIDATED */

// TODO remember intermediate values in some list, that we can use as starting point
Mvalue* Mfac(Mvalue* _value){
    if(!_value){if(amVerbose())outputLine("No argument to factorial() function!");return NULL;}
    if(amVerbose())outputValue("Argument of factorial() function: '",_value,"'.\n");
    if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER){outputValue("\nERROR: Non-integer argument '",_value,"' to factorial() function!");return NULL;}
    // some special cases (i.e. the input number is smaller than 2)
    Mbiginteger* _finalmultiplier=NULL;
    if(_value->type==VT_INTEGER){
        if(_value->value._integer->ll<0){outputError("Invalid (negative) integer argument to factorial() function");return NULL;}
        if(_value->value._integer->ll<3)return _getIntegerValue(_value->value._integer->ll);
        _finalmultiplier=_getBiginteger(_value->value._integer->ll);
    }else{
        if(mp_isneg(_value->value._biginteger)){outputError("Invalid (negative) big integer argument to factorial() function");return NULL;}
        if(mp_cmp(_value->value._biginteger,getBigintegerThree())==MP_LT)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        _finalmultiplier=_getBigintegerCopy(_value->value._biginteger);
    }
    if(!_finalmultiplier){output("%s",ERROR_PREFIX);outputValue("Failed to convert '",_value,"' to a big integer!\n");return NULL;}
    if(amVerbose()&&amDebugging())outputBiginteger("\nFinal multiplier: '",_finalmultiplier,"'.");
    Mbiginteger* _result=_getBiginteger(6); // the smallest value to return
    if(_result){
        // we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
        // we could start at some intermediate value????
        Mbiginteger *_multiplier=_getBiginteger(3);
        if(_multiplier){
            while(mp_cmp(_multiplier,_finalmultiplier)==MP_LT){
                if(mp_incr(_multiplier)!=MP_OKAY){outputError("Failed to increment a big integer");_result=NULL;break;} // if we fail to increment break
                if(mp_mul(_result,_multiplier,_result)!=MP_OKAY){outputError("Failed to multiply a big integer");_result=NULL;break;}
                //////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
            }
            // get rid of intermediate big integers
            free_biginteger(_multiplier);
        }else        
            outputError("Failed to create big integer 3");
    }else
        outputError("Failed to create big integer 6");
    free_biginteger(_finalmultiplier);
    if(amVerbose())outputBiginteger("Result of applying the factorial() function: '",_result,"'.\n");
    return (_result?_getBigintegerValue(_result,true):NULL);
    /* replacing:
    // 39 is about the maximum that we can store in a long long
    if(n<40){
        long long result=n;while(--n>1)result*=n; // TODO should we use multiply here NO I guess not, although we could get overflow at some point!!!
        return _getIntegerValue(result);
    }
    long double result=n;
    while(--n>1)result*=n;
    return _getRealValue(result);
    */
}/* VALIDATED */


#include <limits.h>
#include <math.h>
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"
// MDH@31MAY2019 (married for 8 year now): switched from libzahl to tommath
#include "tommath.h"
#include "Mexecution.h"

const char* MUTABLEVALUETYPECHARS="utirslm"; // the characters associated with each of the value types
const char* IMMUTABLEVALUETYPECHARS="UTIRSLM"; // the characters associated with each of the value types

/* initialization for big integer arithmetic
jmp_buf env;
bool initExecution(){
    if(setjmp(env))return false;
    zsetup(env);
    return true;
}
*/
// new_mp_int returns an initialized big integer on success, or NULL when failing
mp_int* new_mp_int(){
    mp_int* result=(mp_int*)malloc(sizeof(mp_int));
    if(!result)return NULL;
    if(mp_init(result)!=MP_OKAY){mp_clear(result);return NULL;} // ESSENTIAL to release the big integer, when failing to initialize it!!
    return result;
}
mp_int* _getBigInteger(int64_t l){
    mp_int* bi=new_mp_int();
    if(l)mp_set_i64(bi,l); // TODO mp_set_i64 can fail can't it? then why is it of type void???
    return bi;
}
// pass in NULL to _getBigIntegerCopy to get a big integer (initialized to zero)
mp_int* _getBigIntegerCopy(mp_int* bi){
    mp_int* result=new_mp_int();
    if(bi&&mp_copy(result,bi)!=MP_OKAY){mp_clear(result);return NULL;}
    return result;
}

// using constant big integers 0, 1 and 2 (do NOT wrap these constants in Mvalue's though or they will need to be created over and over again)
static mp_int *bi0=NULL,*bi1=NULL,*bi2=NULL,*bi3=NULL;
mp_int* getBigIntegerZero(){if(!bi0)bi0=_getBigInteger(0);return bi0;}
mp_int* getBigIntegerOne(){if(!bi1)bi1=_getBigInteger(1);return bi1;}
mp_int* getBigIntegerTwo(){if(!bi2)bi2=_getBigInteger(2);return bi2;}
mp_int* getBigIntegerThree(){if(!bi3)bi3=_getBigInteger(3);return bi3;}

// RELEASERS
// however we can only NULL them if we have the address of the pointer)
// but if these pointer are local to a function (which they will be typically if they are to be released in the first place) no NULLing is required!!!
void free_string(Mstring* _string){
    free(_string); // replacing (when we used a char pointer (_m) for storing the characters): if(_string){if(_string->_m)free_mstring(_string->_m);_string->_m=NULL;free(_string);}
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
void free_variable(Mvariable* _variable){
    if(_variable){
        if(_variable->_name)free(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
        if(_variable->_value)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
        free(_variable);
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
void free_integer(Minteger* _integer){
    if(_integer){
        if(amVerbose())output("\nFreeing integer %llu.",_integer->ll);
        free(_integer);
    }else
        output("\nBUG: No integer to free!");
}
void free_biginteger(mp_int* bi){
    if(bi){
        if(amVerbose())output("\nFreeing big integer."); // TODO can we display the value?
        mp_clear(bi); // directly call mp_clear on the mp_int pointer!!!
    }else
        output("\nBUG: No big integer to free!");
}
void free_real(Mreal* _real){
    if(_real){
        if(amVerbose())output("\nFreeing real %.*Lf.",23,_real->ld);
        free(_real);
    }else
        output("\nBUG: No real to free!");
}
// MDH@01MAY2019: 'local' function for freeing a value
void free_value(Mvalue* _value){
    if(_value){
        // I do not need to free the value itself, only the pointers inside it
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_TOKEN:if(_value->value._token)free_token(_value->value._token);break;
            case VT_INTEGER:if(_value->value._integer)free_integer(_value->value._integer);break;
            case VT_BIGINTEGER:if(_value->value._biginteger)free_biginteger(_value->value._biginteger);break;
            case VT_REAL:if(_value->value._real)free_real(_value->value._real);break;
            case VT_STRING:free_string(_value->value._string);break;
            case VT_LIST:free_list(_value->value._list);break;
            case VT_MAP:free_map(_value->value._map);break;
        }
        free(_value);
    }else
        output("\nBUG: No value to free!");
}

// keep a list of allocated values
Mlist* _valueList=NULL;
Mvalue* _newValue(){
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
    size_t removed=0;
    size_t tofree=0; // how many value elements we should free
    if(_valueList){
        if(amVerbose())output("\nNumber of values to check: %u.",_valueList->numberOfElements);
        Mlistelement* _valueListelement=_valueList->_first;
        size_t checked=0;
        while(_valueListelement){
            checked++;
            if(_valueListelement->_value&&!_valueListelement->_value->count){ // unused
                if(amVerbose())output("\nAbout to free unused value #%u of type '%s'.",checked,VALUETYPENAMES[_valueListelement->_value->type]);
                free_value(_valueListelement->_value);
                _valueListelement->_value=NULL; // just in case
                tofree++;
            }
            _valueListelement=_valueListelement->_next;
        }
        if(amVerbose())output("\nNumber of values checked: %lu.\nNumber of value list elements to free: %lu.",checked,tofree);
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
                    if(_valueList->numberOfElements)_valueList->numberOfElements--;else output("\nBUG: Trying to free a value list element that is not counted!");  // one less element in the list!!!
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
        if(tofree>removed)output("\nWARNING: Failed to free %lu unused value list elements.",(tofree-removed));else if(amVerbose())output("\nAll unused value list elements freed!");
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
        output("\nBUG: Reference count of '%s' of type '%c' already zero.",_valueText,MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
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

// ALLOCATORS (private)
// value wrappers
// typically an Mvalue is immutable (we might change that for variables that are strong typed e.g. when created with integer(),real(),string(),list() or map() function)
Minteger* new_integer(long long ll){
    Minteger* _integer=malloc(sizeof(Minteger));
    if(_integer)_integer->ll=ll;
    return _integer;
}
/*
Mbiginteger* new_biginteger(z_t zt){
    Mbiginteger* _biginteger=malloc(sizeof(Mbiginteger));
    if(_biginteger){
        zset(_biginteger->bi,zt);
    }
    return _biginteger;
}
*/
Mreal* new_real(long double ld){
    Mreal* _real=malloc(sizeof(Mreal));
    if(_real)_real->ld=ld;
    return _real;
}
// Mstring is an immutable version of mstring* in that it cannot be changed
Mstring* new_string(char* _text){ // _text assumed to be string(mstring*), so we can simply copy it over with the starting quote character (" or ')
    return (Mstring*)_strdup(_text);
}
Mstring* new_charstring(char _char){ // _text assumed to be string(mstring*), so we can simply copy it over with the starting quote character (" or ')
    mstring* charstring=string_create();string_append_char(charstring,'"');string_append_char(charstring,_char);
    Mstring* char_string=new_string(string(charstring));
    free(charstring);
    return char_string;
}
// interface functions that use the above functions
// wrapping the different value type instances
Mvalue* _getUndefinedValue(){return (Mvalue*)calloc(1,sizeof(Mvalue));}
Mvalue* _getIntegerValue(long long ll){
    Minteger* _integer=new_integer(ll);
    Mvalue* _integervalue=(_integer?_newValue():NULL);
    if(_integervalue){_integervalue->type=VT_INTEGER;_integervalue->value._integer=_integer;}
    return _integervalue;
}
Mvalue* _getBigintegerValue(mp_int* _biginteger){
    Mvalue* _bigintegerValue=(_biginteger?_newValue():NULL);
    if(_bigintegerValue){_bigintegerValue->type=VT_BIGINTEGER;_bigintegerValue->value._biginteger=_biginteger;}
    return _bigintegerValue;
}
Mvalue* _getRealValue(long double ld){
    Mreal* _real=new_real(ld);
    Mvalue* _realvalue=(_real?_newValue():NULL);
    if(_realvalue){_realvalue->type=VT_REAL;_realvalue->value._real=_real;}
    return _realvalue;
}
Mvalue* _getStringValue(char* _s){
    Mstring* _string=(_s?new_string(_s):NULL); // for mstring* sources pass string(mstring*) into getStringValue() (which points to mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _stringvalue=(_string?_newValue():NULL);
    if(_stringvalue){_stringvalue->type=VT_STRING;_stringvalue->value._string=_string;}
    return _stringvalue;
}
Mvalue* _getCharStringValue(char _c){
    Mstring* _string=(_c?new_charstring(_c):NULL); // for mstring* sources pass string(mstring*) into getStringValue() (which points to mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _stringvalue=(_string?_newValue():NULL);
    if(_stringvalue){_stringvalue->type=VT_STRING;_stringvalue->value._string=_string;}
    return _stringvalue;
}
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype){
    Mlist* _list=(Mlist*)calloc(1,sizeof(Mlist));
    _list->valuetype=listValuetype; // register what type of elements this list should have
    Mvalue* _listvalue=(_list?_newValue():NULL);
    if(_listvalue){_listvalue->type=VT_LIST;_listvalue->value._list=_list;}
    return _listvalue;
}
Mvalue* _getMapValue(Mvaluetype mapValuetype){
    Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
    _map->valuetype=mapValuetype;
    Mvalue* _mapvalue=(_map?_newValue():NULL);
    if(_mapvalue){_mapvalue->type=VT_MAP;_mapvalue->value._map=_map;}
    return _mapvalue;
}

// some other wrappers
Mvalue* _getValueOfList(Mlist* _list){Mvalue* _value=_newValue();_value->type=VT_LIST;_value->value._list=_list;return _value;}
Mvalue* _getValueOfInteger(Minteger* _integer){Mvalue* _value=_newValue();_value->type=VT_INTEGER;_value->value._integer=_integer;return _value;}
Mvalue* _getValueOfReal(Mreal* _real){Mvalue* _value=_newValue();_value->type=VT_REAL;_value->value._real=_real;return _value;}
Mvalue* _getValueOfMap(Mmap* _map){Mvalue* _value=_newValue();_value->type=VT_MAP;_value->value._map=_map;return _value;}
Mvalue* _getValueOfToken(Mtoken* _token){Mvalue* _value=_newValue();_value->type=VT_TOKEN;_value->value._token=_token;return _value;}

/*
void free_list(Mlist* _list);
void free_variable(Mvariable* _variable);
void free_mapelement(Mmapelement* _mapelement);
void free_map(Mmap* _map);
void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement);
void free_expressionlist(Mexpressionlist* _expressionlist);
void free_functiondefinition(Mfunctiondefinition* _functiondefinition);
// NOTE typically you're not supposed to free internal functions safe M function definitions 
bool free_function(Mfunction* _function);
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement);
void free_functionmap(Mfunctionmap* _functionmap);
void free_environment(Menvironment* _environment);
*/


void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement){
    if(_expressionlistelement){
        free_expressionlistelement(_expressionlistelement->_next);
        free_value(_expressionlistelement->_value);
        free(_expressionlistelement);
    }
}
void free_expressionlist(Mexpressionlist* _expressionlist){
    if(_expressionlist){
        free_expressionlistelement(_expressionlist->_next);
        free(_expressionlist);
    }
}
void free_functiondefinition(Mfunctiondefinition* _functiondefinition){
    if(_functiondefinition){
        if(_functiondefinition->_parameterMap)free_map(_functiondefinition->_parameterMap);
        if(_functiondefinition->_expressionlist)free_expressionlist(_functiondefinition->_expressionlist);
        free(_functiondefinition);
    }
}
// NOTE typically you're not supposed to free internal functions safe M function definitions 
bool free_function(Mfunction* _function){
    if(_function){
        if(_function->type==FT_M){
            free_functiondefinition(_function->functionunion._functiondefinition);
            free(_function);
            return true;
        }
    }
    return false;
}
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement){
    if(_functionmapelement){
        if(free_functionmapelement(_functionmapelement->_next))_functionmapelement->_next=NULL;
        if(free_function(_functionmapelement->_function)){
            free(_functionmapelement);
            return true;
        }
    }
    return false;
}
void free_functionmap(Mfunctionmap* _functionmap){
    if(_functionmap){
        free_functionmapelement(_functionmap->_first);
        free(_functionmap);
    }
}
void free_environment(Menvironment* _environment){
    if(_environment){
        free_map(_environment->_variableMap);
        free_functionmap(_environment->_functionMap);
        free(_environment);
    }
}
// END RELEASERS
// keep track of the current execution environment
static Menvironment* _executionEnvironment=NULL;
bool pushExecutionEnvironment(Menvironment* _environment){
    if(!_environment)return false;
    if(_environment->_parent)return false; // shouldn't have a parent!!!
    _environment->_parent=_executionEnvironment;
    _executionEnvironment=_environment;
    return true;
}
bool popExecutionEnvironment(){
    Menvironment* _parentExecutionEnvironment=(_executionEnvironment?_executionEnvironment->_parent:NULL);
    if(!_parentExecutionEnvironment)return false;
    _executionEnvironment->_parent=NULL; // clear the parent of the current execution environment
    free_environment(_executionEnvironment); // TODO I guess we won't be needing this execution environment any more????
    _executionEnvironment=_parentExecutionEnvironment;
    return true;
}

/*
// helper functions
// helper functions to wrap literals for storage in M
Mreal* get_real(long double ld){
	Mreal* _real=(Mreal*)malloc(sizeof(Mreal));
	if(_real)_real->ld=ld;
	return _real;
}
Mvalue* getRealValue(Mreal* _real){
    if(_real){
        Mvalue* _realValue=calloc(1,sizeof(Mvalue));
        if(_realValue){_realValue->type=VT_REAL;_realValue->value._real=_real;return _realValue;}
    }else
        printf("\nERROR: No real to wrap.");  
    return NULL;
}

Minteger* get_integer(long long ll){
	Minteger* _integer=(Minteger*)malloc(sizeof(Minteger));
	if(_integer)_integer->ll=ll;
	return _integer;
}
Mvalue* getIntegerValue(Minteger* _integer){
    if(_integer){
        Mvalue* _integerValue=calloc(1,sizeof(Mvalue));
        if(_integerValue){
            _integerValue->type=VT_INTEGER;
            _integerValue->value._integer=_integer;
            return _integerValue;
        }
    }else
        printf("\nERROR: No integer to wrap.");  
    return NULL;
}

// mstring* is assumed to start with the same prefix/suffix character
Mstring* get_string(mstring* s){
	Mstring* _string=(s?(Mstring*)malloc(sizeof(Mstring)):NULL);
	if(_string){
		_string->presuffix=string_char(s,0);
		_string->_m=string_create();
		if(_string->_m==NULL)return NULL;
		string_append(_string->_m,string_remainder(s,1)); // NOTE string_remainder might return NULL of course
	}
	return _string;
}
Mvalue* getStringValue(Mstring* _string){
    if(_string){
        Mvalue* _stringValue=calloc(1,sizeof(Mvalue));
        if(_stringValue){
            _stringValue->type=VT_STRING;
            _stringValue->value._string=_string;
            return _stringValue;
        }
    }else
        printf("\nERROR: No string to wrap!");
    return NULL;
}
// end helper functions
*/

// read access to the elements defined in an environment
uint32_t getNumberOfVariables(Menvironment* _environment){
    return(_environment?_environment->_variableMap->numberOfElements:0);
}
// the names of the variables may be requested
mstring* _getVariableNames(const Menvironment* _environment,char* sep){
    if(_environment!=NULL&&sep!=NULL){
        mstring* variableNames=string_create();
        if(variableNames!=NULL){
            // first append the names of the variables in the parent
            if(_environment->_parent){
                mstring* parentVariableNames=_getVariableNames(_environment->_parent,sep);
                if(parentVariableNames){
                    string_append(variableNames,string(parentVariableNames));
                    free_mstring(parentVariableNames); // we can do this because string_append copies the characters
                }
            }
            // we'll be appending the names of the variables in the environment itself
            Mmapelement* _variableMapelement=_environment->_variableMap->_first;
            while(_variableMapelement){
                if(strlen(sep))if(!string_empty(variableNames))string_append(variableNames,sep);
                string_append(variableNames,_variableMapelement->_variable->_name);
                _variableMapelement=_variableMapelement->_next;
            }
            return variableNames;
        }
    }
    return NULL;
}

Mvariable* getVariable(Menvironment* _environment,const char* name, bool verbose){
    if(!_environment||!name){output("\nERROR: %s","No environment or name specified.");return NULL;}
    // input valid        
    if(!_environment->_variableMap){output("\nERROR: %s","No variables in environment.");return NULL;}
    if(verbose)output("\nLooking for variable '%s'.",name);
    Mmapelement*_variableMapelement=_environment->_variableMap->_first;
    // as long as variable is defined, and the variable's name is not equal to the given name, continue
    while(_variableMapelement&&(!_variableMapelement->_variable||strcmp(_variableMapelement->_variable->_name,name)))_variableMapelement=_variableMapelement->_next;
    if(!_variableMapelement){
        if(verbose)outputLine("Found!");
        return NULL;
    }
    return _variableMapelement->_variable;
}
bool containsVariable(Menvironment* _environment,const char* name){return(getVariable(_environment,name,false)!=NULL);}

// write access
// helper function to create a new variable with a given name and of a given type
Mvariable* _createVariable(const char* name,Mvaluetype valuetype,bool immutable){
    if(!name||!strlen(name)){
        printf("\nERROR: No variable name defined.");
        return NULL;
    }
    Mvariable* _variable=(Mvariable*)calloc(1,sizeof(Mvariable)); // all pointers will be NULL!!
    if(!_variable){
        printf("\nERROR: Failed to allocate memory to store variable '%s'.",name);
        return NULL;
    }
    _variable->immutable=immutable;
    _variable->_name=_strdup(name); // create a dynamic pointer on the heap
    if(!_variable->_name){
        free_variable(_variable);
        printf("\nERROR: Failed to allocate memory to store name '%s' of the new variable.",name);
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

// addVariable returns the value map element that was created (if successful)
bool addVariable(Menvironment* _environment,const char* name,Mvaluetype valuetype,bool immutable){
    Mvariable* _variable=NULL;
    if(_environment&&name){ // input valid
        _variable=getVariable(_environment,name,false);
        if(!_variable){ // non-existing...
            _variable=_createVariable(name,valuetype,immutable);
            //////printf("\nVariable created!");
            if(_variable){
                Mmapelement* _variableMapelement=(Mmapelement*)malloc(sizeof(Mmapelement));
                if(_variableMapelement){
                    // store the references
                    _variableMapelement->_next=NULL;
                    _variableMapelement->_variable=_variable;
                    Mmapelement* _lastVariableMapelement=_environment->_variableMap->_last;
                    if(_lastVariableMapelement!=NULL)_lastVariableMapelement->_next=_variableMapelement;else _environment->_variableMap->_first=_variableMapelement;
                    _environment->_variableMap->_last=_variableMapelement;
                    _environment->_variableMap->numberOfElements++;
                    return true;
                }
                // ASSERT failed to link the variable to the variable map!!
                free_variable(_variable);
                output("\nERROR: Failed to link variable '%s'.",name);
            }else
                output("\nERROR: Failed to create variable '%s'.",name);
        }
    }
    return false;
}

bool setValue(Menvironment* _environment,const char* name,Mvalue* _value){
    // NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
    if(!_environment||!name){output("\nERROR: Cannot set the value: no environment or name.");return false;}
    Mvariable* _variable=getVariable(_environment,name,false);
    if(_variable){
        if(!_variable->_value||!_variable->immutable){
            // _value needs to be of the right type
            if(!_value||_variable->valuetype==VT_UNDEFINED||_variable->valuetype==_value->type){
                ///////////////if(_variable->_value)_variable->_value->count--; // decrement the reference count on the current value
                assignValue(&_variable->_value,_value); // 'assign' the reference (takes care of updating the reference counts)
                if(amDebugging()){mstring* _valueText=_getValueText(_value,false);output("\nValue `%s` assigned to variable `%s`.",string(_valueText),name);free_mstring(_valueText);}
                ///////////////if(_variable->_value)_variable->_value->count++; // increment the reference count
                return true; // releasing the value is my responsibility now...
            }
            output("\nERROR: Cannot set the value of variable `%s`: the new value is of the wrong type.",name);
        }else
            output("\nERROR: Cannot set the value of variable `%s`: it is not mutable!",name);
    }else
        output("\nERROR: Cannot set the value of variable `%s`:it is unknown.",name);
    return false;
}

// MDH@24MAY2019: if already in the map should replace the current value
bool appendedToMap(Mmap* _map,const char* attributeName,Mvalue* _attributeValue){
    if(!_map||!attributeName)return false;
    if(amVerbose()){output("\nSetting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.");}
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

void checkList(Mlist* _list){
    if(_list){
        long long l=_list->numberOfElements;
        if(l>0){
            Mlistelement* _listelement=_list->_first;
            if(_listelement){
                // the number of elements in the list should match the number of counted elements
                long long listelementindex=0;
                while(true){
                    if(l==0)output("\nERROR: More elements in list than accounted for.");
                    l--;
                    if(_listelement->index<=listelementindex)output("\nERROR: List element index (%lld) below the expected list element index (%lld).",_listelement->index,listelementindex);
                    listelementindex=_listelement->index;
                    if(!_listelement->_next){
                        if(_list->_last!=_listelement)output("\nERROR: Registered last list element not equal to the actual last list element.");
                        break;                        
                    }
                    output("\nList element with index %llu OK.",_listelement->index);
                    _listelement=_listelement->_next;
                }
                if(l>0)output("\nERROR: Less elements in list than accounted for.");else 
                if(l<0)output("\nERROR: %lld more elements in list than counted.",(-l));
            }else
                output("\nERROR: List with %lld elements does not have a first element!",l);
        }else{
            if(_list->_first)output("\nERROR: Empty list with first element!");
            if(_list->_last)output("\nERROR: Empty list with last element.");
        }
    }
}
// instead of returning a boolean we could return the assigned index (0 on failure)
unsigned long long appendedToList(Mlist* const _list,Mvalue* const _value,long long index){
    if(!_list||!_value){output("\nERROR: %s.","No list to append to or no value to append");return 0;}
    // check validity of index first
    long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
    if(index<=0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
    if(index<=0){output("\nERROR: Index %lld of (new) list element too small.",index);return 0;}
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
        // if we're going to insert we there will be a successor
        if(_listelement->index!=index){_nextListelement=_prevListelement->_next;_listelement=NULL;} // so that we are forced to create one
    }else // we'll be appending, so the current last is the predecessor (and no successor)
        _prevListelement=_list->_last;
    // if we do not have a list element ascertain to have one
    if(!_listelement){ // not yet present in list, so we have to create a new element
        _listelement=(Mlistelement*)calloc(1,sizeof(Mlistelement));
        if(!_listelement){output("\nERROR: Failed to create a list element to insert/append.");return 0;} // failure
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
long long appendToListVariable(Menvironment* _environment,const char* name,Mvalue* _value){
    if(!_environment||!name||!_value){output("\nERROR: %s","No environment, variable name of value specified!");return 0;}
    Mvariable* _variable=getVariable(_environment,name,amVerbose());
    if(_variable){
        Mvalue* _variableValue=_variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
        if(_variableValue->type==VT_LIST){ // yes a list we can append to
            // we should prevent circular references
            if(_variableValue==_value)_value=NULL;
            Mlist* _list=_variableValue->value._list;
            unsigned long long index=appendedToList(_list,_value,0); // NOTE always append to the end of the list with the first available index that's why I'm passing in 0 instead of a positive index value!!
            if(index)return index;
            output("\nERROR: Didn't append the value to the list stored in variable '%s': the type of the new value (%u) is wrong.",name,_value->type);
        }else
            output("\nERROR: Cannot append the value to variable '%s': it does not contain a list!",name);
    }else
        output("\nERROR: Cannot set the value of variable '%s':it is unknown.",name);
    return 0;
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
/*
Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue){
    if(_environment&&name){
        Mvariable* _variable=getVariable(_environment,name,amVerbose());
        if(_variable){
            Mvalue* _variableValue=_variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
            if(_variableValue){
                if(_variableValue->type==VT_LIST){ // yes a list we can look into
                    Mlist* _list=_variableValue->value._list;
                    return getValueAtIndex(_list,_indexValue->value._integer->ll);
                }
            }
        }
    }
    return NULL;
}
*/
Mvalue* getValue(Menvironment* _environment,const char* name){
    if(!_environment||!name){output("\nERROR: %s.","No environment or name specified.");return NULL;}
    Mvariable* _variable=getVariable(_environment,name,false);
    return(_variable?_variable->_value:NULL);
}
/*
// let's start with some simple assignments
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer){
    if(_variable){        
        Mvalue* _integerValue=getIntegerValue(_integer);
        if(_integerValue){
            free_value(_variable->_value); // free current value
            _variable->_value=_integerValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store integer '%llu' in variable '%s'.",_integer->ll,_variable->_name);
    }
    return false;
}
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real){
    if(_variable){        
        Mvalue* _realValue=getRealValue(_real); // wrap in value
        if(_realValue){
            free_value(_variable->_value); // release the current value
            _variable->_value=_realValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store real '%Lf' in variable '%s'.",_real->ld,_variable->_name);
    }
    return false;
}
bool setValueOfStringVariable(Mvariable* _variable,Mstring* _string){
    if(_variable){        
        Mvalue* _stringValue=getStringValue(_string); // wrap in value
        if(_stringValue){
            free_value(_variable->_value); // free current value
            _variable->_value=_stringValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store text '%s' in variable '%s'.",string(_string->_m),_variable->_name);
   }
   return false;
 }
*/

// functions
// the names of the variables may be requested
mstring* _getFunctionNames(const Menvironment* _environment,char* sep){
    if(_environment&&sep){
        mstring* _functionNames=string_create();
        if(_functionNames){
            // first append the names of the variables in the parent
            if(_environment->_parent){
                mstring* parentFunctionNames=_getFunctionNames(_environment->_parent,sep);
                if(parentFunctionNames){
                    string_append(_functionNames,string(parentFunctionNames));
                    free_mstring(parentFunctionNames); // we can do this because string_append copies the characters that string() points to!!
                }
            }
            // we'll be appending the names of the variables in the environment itself
            if(_environment->_functionMap){
                Mfunctionmapelement* _functionmapelement=_environment->_functionMap->_first;
                while(_functionmapelement){
                    if(strlen(sep))if(!string_empty(_functionNames))string_append(_functionNames,sep);
                    string_append(_functionNames,string(_functionmapelement->_function->_name));
                    _functionmapelement=_functionmapelement->_next;
                }
                return _functionNames;
            }
        }
    }
    return NULL;
}

Mfunction* getFunction(Menvironment* _environment,const char* functionName){
    if(_environment&&functionName&&strlen(functionName)){
        Mfunctionmap* _functionmap=_environment->_functionMap;
        if(_functionmap){
            Mfunctionmapelement* _functionmapelement=_functionmap->_first;
            // we need string() on the function name as function name is an mstring*
            while(_functionmapelement){
                if(_functionmapelement->_function&&!strcmp(string(_functionmapelement->_function->_name),functionName)){
                    //////////printf("\nFunction '%s' matches '%s'.",string(_functionmapelement->_function->_name),functionName);
                    return _functionmapelement->_function;
                }
                _functionmapelement=_functionmapelement->_next;
            }
        }
    }
    return NULL;    
}

Mmap* _getFunctionArgumentMap(Mfunction* _function,Mlist* _argumentList){
    if(_function&&_argumentList){
        Mmap* _argumentMap=(Mmap*)calloc(1,sizeof(Mmap));
        Mmap* _functionParameterMap=_function->_parameterMap;
        if(_functionParameterMap){
            if(amVerbose())output("\nMatching the function parameters!");
            Mmapelement* _functionParameterMapelement=_functionParameterMap->_first;
            Mlistelement* _argumentListelement=_argumentList->_first;
            while(_functionParameterMapelement){
                Mmapelement* _argumentmapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
                // BUG FIX I suppose we need _variable to point to something
                _argumentmapelement->_variable=(Mvariable*)calloc(1,sizeof(Mvariable));
                // probably can't simply assign??? let's use _strdup then
                _argumentmapelement->_variable->_name=_strdup(_functionParameterMapelement->_variable->_name);
                // associate the argument list element value (if available)
                if(_argumentListelement){
                    assignValue(&_argumentmapelement->_variable->_value,_argumentListelement->_value);
                    _argumentListelement=_argumentListelement->_next;
                }else // use the default!!!
                    assignValue(&_argumentmapelement->_variable->_value,_functionParameterMapelement->_variable->_value);
                // append to _argumentMap
                if(_argumentMap->_last)_argumentMap->_last->_next=_argumentmapelement;else _argumentMap->_first=_argumentmapelement;
                _argumentMap->_last=_argumentmapelement;
                _argumentMap->numberOfElements++;
                _functionParameterMapelement=_functionParameterMapelement->_next;
            }
        }
        if(amVerbose())output("\nArgument map created.");
        return _argumentMap;
    }
    return NULL;
}

Mfunction* newFunction(Menvironment* _environment,const char* name){
    Mfunction* _function=NULL;
    if(_environment&&name&&strlen(name)){
        _function=getFunction(_environment,name);
        if(!_function){ // doesn't exist yet
            _function=(Mfunction*)calloc(1,sizeof(Mfunction));
            if(_function){
                _function->_definitionEnvironment=_environment; // TODO why would we need this?????
                mstring* _functionName=string_append(string_create(),name);
                if(_functionName){
                    // try to append it to the functionMap, if we succeed store _functioName in ->_name
                    Mfunctionmap* _functionmap=_environment->_functionMap;
                    if(_functionmap){
                        Mfunctionmapelement* _functionmapelement=(Mfunctionmapelement*)calloc(1,sizeof(Mfunctionmapelement));
                        if(_functionmapelement){
                            _functionmapelement->_function=_function; // no worries here
                            Mfunctionmapelement* _lastFunctionmapelement=_functionmap->_last;
                            if(_lastFunctionmapelement){
                                _lastFunctionmapelement->_next=_functionmapelement;
                                _functionmap->_last=_functionmapelement;
                            }else
                                _functionmap->_first=_functionmapelement;
                            _functionmap->_last=_functionmapelement;
                            _functionmap->numberOfFunctions++;
                             _function->_name=_functionName; // success!!!!!
                            printf("\nFunction '%s' registered as function #%d.",string(_function->_name),_functionmap->numberOfFunctions);
                        }
                    }
                }else
                    printf("\nERROR: Failed to store function name '%s'.",name);
                // if we fail to register the name and/or the function with the environment free the function!!
                if(!_function->_name){free_function(_function);_function=NULL;}   
            }
            if(!_function)printf("\nERROR: Failed to create function '%s'.",name);
        }else
            printf("\nFunction '%s' already exists.",name);
    }
    return _function;
}

// helper function to create a parameter map with a single value
// the following is a nuisance
Mmap* _getRealMap(char* name,Mvalue* _realValue){
    if(name&&_realValue){
        Mvariable* _realVariable=_createVariable(name,VT_REAL,true);
        if(_realVariable){
            assignValue(&_realVariable->_value,_realValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_realVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    output("\n%s","Returning the single real map!");
                    return _map;
                }
                output("\nERROR: %s.","Failed to create the real variable map!");
               free_mapelement(_mapelement);
            }else{
                output("\nERROR: %s.","Failed to create the real variable map element!");
                free_variable(_realVariable);
            }
        }else
            output("\nERROR: %s.","Failed to create the real variable!");
    }
    return NULL;
}
Mmap* _getMap(char *name){
    Mvariable* _variable=_createVariable(name,VT_UNDEFINED,true);
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
        Mvariable* _integerVariable=_createVariable(name,VT_INTEGER,true);
        if(_integerVariable){
            assignValue(&_integerVariable->_value,_integerValue); ///////  incrementReferenceCount(_integerValue); // now bound to the integer variable
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_integerVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    output("\n%s","Returning the single integer map!");
                    return _map;
                }
                printf("\nERROR: %s.","Failed to create the integer variable map!");
               free_mapelement(_mapelement);
            }else{
                printf("\nERROR: %s.","Failed to create the integer variable map element!");
                free_variable(_integerVariable);
            }
        }else
            printf("\nERROR: %s.","Failed to create the integer variable!");
    }
    return NULL;
}
Mmap* _getStringStringMap(char* name1,char* name2){
    if(name1&&name2){
        if(strlen(name1)&&strlen(name2)&&!strcmp(name1,name2)){
            Mmapelement* _mapelement1=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            Mmapelement* _mapelement2=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement1&&_mapelement2){
                _mapelement1->_variable=_createVariable(name1,VT_STRING,true);
                _mapelement2->_variable=_createVariable(name2,VT_STRING,true);
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
        Mvariable* _listVariable=_createVariable(name,VT_LIST,true);
        if(_listVariable){
            assignValue(&_listVariable->_value,_listValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_listVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    output("\n%s","Returning the single list map!");
                    return _map;
                }
                output("\nERROR: %s.","Failed to create the list variable map!");
               free_mapelement(_mapelement);
            }else{
                output("\nERROR: %s.","Failed to create the list variable map element!");
                free_variable(_listVariable);
            }
        }else
            output("\nERROR: %s.","Failed to create the list variable!");
    }
    return NULL;
}
// end helper functions 


// the internal functions
/**
 * Msettype() to set the (value) type of a variable
 */
Mvalue* Msettype(Mvalue* _variableName,Mvalue* _valuetype){
    // check the types first, both should be strings
    if(_variableName->type==VT_STRING&&_valuetype->type==VT_STRING){
        char* variableName=_variableName->value._string->_c; // ignoring the presuffix exactly as we need to!!!
        if(strlen(variableName)){
            // get the value type, we can use uppercase to indicate an immutable (constant) variable?????
            bool immutable=false;
            Mvaluetype valuetype=VT_UNDEFINED;
            switch(_valuetype->value._string->_c[0]){ // use the first character (which will be '\0' if the default value is used!!!)
                case 'I':
                    immutable=true;
                case 'i':
                    valuetype=VT_INTEGER;
                    break;
                case 'R':
                    immutable=true;
                case 'r':
                    valuetype=VT_REAL;
                    break;
                case 'S':
                    immutable=true;
                case 's':
                    valuetype=VT_STRING;
                    break;
                case 'L':
                    immutable=true;
                case 'l':
                    valuetype=VT_LIST;
                    break;
                case 'M':
                    immutable=true;
                case 'm':
                    valuetype=VT_MAP;
                    break;
            }
            if(containsVariable(_executionEnvironment,variableName)||addVariable(_executionEnvironment,variableName,valuetype,immutable)){
                Mvariable* _variable=getVariable(_executionEnvironment,variableName,false); // should exist
                if(_variable){
                    // you can change the value type if the current value is (still) NULL or when it is mutable...
                    if(valuetype!=_variable->valuetype){ // a change of the value type intended (e.g. from undefined i.e. free to integer, or real or whatever)
                        if(!_variable->immutable||!_variable->_value){
                            _variable->valuetype=valuetype; // update the value type
                            assignValue(&_variable->_value,NULL); // clear the value (might already be the case but won't harm either)
                        }
                    }
                    // return the value type as text, which means we need to wrap the value type character
                    return _getCharStringValue(_variable->immutable?IMMUTABLEVALUETYPECHARS[_variable->valuetype]:MUTABLEVALUETYPECHARS[_variable->valuetype]);
                }
            }
        }
    }
    return NULL;
}
// math functions: independent of the execution environment but still receive it...
Mvalue* Msin(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(sin(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sin(_value->value._integer->ll));
    }
    return NULL;
}
Mvalue* Mcos(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(cos(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(cos(_value->value._integer->ll));
    }
    return NULL;
}

bool completedFunction(Mfunction* _function,NoArgumentFunction noArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_NO_ARGUMENTS;
        _function->functionunion.noArgumentFunction=noArgumentFunction;
        _function->_parameterMap=NULL;
        output("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedValueFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getMap("v");
        // no defaults here!!!
        output("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedRealFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getRealMap("x",_getRealValue(0.0));
        output("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedIntegerFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getIntegerMap("i",_getIntegerValue(0));
        output("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedListFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getListMap("l",_getListValue(VT_UNDEFINED));
        output("\nRegistered list function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedStringStringFunction(Mfunction* _function,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getStringStringMap("variable","type");
        output("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}

// these internal functions do NOT have a body as M defined functions have...
bool registerInternalFunctions(Menvironment* _environment){
    // variable functions

    // math functions
    if(!completedRealFunction(newFunction(_environment,"cos"),Mcos))return false;
    if(!completedRealFunction(newFunction(_environment,"sin"),Msin))return false;
    if(!completedStringStringFunction(newFunction(_environment,"settype"),Msettype))return false;
    return true;
}

mstring* appendull(mstring* const ms,unsigned long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}
// helper function
mstring* appendll(mstring* const ms,long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}
mstring* appendld(mstring* const ms,long double ld){
	char ldText[80];
	snprintf(ldText,80,"%.*Lf",LDBL_DIG,ld);
	return string_append(ms,ldText);
}
// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
mstring* _getIntegerText(Minteger* _integer){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'i');
	if(p&&_integer)p=appendll(p,_integer->ll);
    if(!p){free_mstring(s);s=NULL;}
	////////if(amVerbose())output("\nInteger '%s'.",string(s));
	return s;
}

// part of implementing _getRealText (so not present in the header)
const char* M_NAN="NaN";
const char* M_INF="Inf";
bool ldIsZero(long double ld){return fpclassify(ld)==FP_ZERO;}
bool ldIsNaN(long double ld){return fpclassify(ld)==FP_NAN;}
bool ldIsInf(long double ld){return fpclassify(ld)==FP_INFINITE;}
mstring* _getRealText(Mreal* _real){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'r');
	if(p&&_real){
		switch(fpclassify(_real->ld)){
			case FP_NAN:p=string_append(p,M_NAN);break;
			case FP_INFINITE:p=string_append(p,M_INF);break;
			default:p=appendld(p,_real->ld);break;
		}
        if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}
mstring* _getStringText(Mstring* _string,bool dequoted){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'s');
    if(p&&_string){
        if(!dequoted)p=string_append_char(p,_string->presuffix);
        p=string_append(p,_string->_c);
        if(!dequoted)p=string_append_char(p,_string->presuffix);
        if(!p){free_mstring(s);s=NULL;}
    }
	return s;
}
//////////mstring* _getValueText(Mvalue* _value); // forward prototype used in getListText() and getMapText()
mstring* _getListText(Mlist* _list){
    ///////output("\nList to output.");char c;inputCharRead(&c);
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
		/////output("\nList=%s",string(p));
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
		//////output("\n%s",string(p));
		Mmapelement* _mapelement=_map->_first;
		while(p&&_mapelement){
			//////output("\n%s","start");
			Mvariable* _variable=_mapelement->_variable;
			if(!_variable)continue;
            // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
            p=string_append_char(p,'\'');
			p=string_append(p,_variable->_name);
            p=string_append_char(p,'\'');
			/////output("\n%s",string(p));
			p=string_append_char(p,':'); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
			/////output("\n%s",string(p));
			mstring* mapelementValueText=_getValueText(_variable->_value,false);
			/////output("\nMap element: %s",string(p));
			if(!mapelementValueText)continue;
			p=string_append(p,string(mapelementValueText)); // append 
			free_mstring(mapelementValueText); // release AFTER copying over
			_mapelement=_mapelement->_next;
			if(_mapelement)p=string_append(p,", "); // only when there's a next map element to process
			////output("\n%s","next");
		}
		//////output("\n%s(%d)",string(p),string_length(p));
		p=string_append_char(p,'}');
		//////output("\n%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}

mstring* _UNDEFINED_VALUETEXT=NULL;
// the problem here is that whatever _getValueText returns will be freed on the other side, which we would not want to happen with _UNDEFINED_VALUETEXT, so perhaps we should return NULL in that case after all????
// we can solve that by returning a new undefined value text instance every time
mstring* getUndefinedValueText(){
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(string_create(),UNDEFINED_VALUETEXT);
    mstring* _undefinedValueText=string_create();
    // if copying fails (however unlikely), we ourselves need to free _undefinedValueText, and return NULL (unfortunately), which indicates memory problems!!!
    if(!string_copy(_UNDEFINED_VALUETEXT,_undefinedValueText)){if(_undefinedValueText)free_mstring(_undefinedValueText);return NULL;}
    return _undefinedValueText;
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
            case VT_BIGINTEGER: // how many characters do we need????
                {
                    char buffer[1024];
                    if(mp_toradix_n(_value->value._biginteger,buffer,10,1024)!=MP_OKAY)
                        output("\nERROR: 1024 characters can't hold the big integer decimal representation.");
                    else
                        valueText=new_mstring(buffer);
                }
                break;
			case VT_REAL:valueText=_getRealText(_value->value._real);break;
			case VT_STRING:valueText=_getStringText(_value->value._string,dequoted);break; // TODO don't dequote the text!!
			case VT_MAP:valueText=_getMapText(_value->value._map);break;
			case VT_LIST:valueText=_getListText(_value->value._list);break;
            case VT_TOKEN:valueText=string_create();string_copy(_value->value._token->text,valueText);break; // we need to return a copy because that copy will be freed typically (and we do not want to free the original now do we?)
			default:break;
		}
	}
    if(valueText)if(amAssisting())valueText=appendll(string_append_char(valueText,'#'),_value->count); // show the reference count as well
    ////////outputChar('.');
    return(valueText?valueText:getUndefinedValueText());
    /* replacing:
    if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("\nValue text: '%s'.",string(valueText));else output("\nValue not represented.");
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(string_create(),UNDEFINED_VALUETEXT);
    return _UNDEFINED_VALUETEXT;
    */
}
void outputBigInteger(const char* const prefix,const mp_int* const _bigInteger,const char* const postfix){
    if(prefix)output("%s",prefix);
    mstring* _bigIntegerText=string_create();
    char buffer[1024];
    if(mp_toradix_n(_bigInteger,buffer,10,1024)!=MP_OKAY)
        output("too large for buffer");
    else
        output("%s",buffer);
    if(postfix)output("%s",postfix);
}
void outputValue(const char* const prefix,const Mvalue* _value,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_value){
        mstring* _valueText=_getValueText(_value,false);
        output("%s",string(_valueText));free_mstring(_valueText);
    }
    if(postfix)output("%s",postfix);
}
long long getValueInteger(const Mvalue* const _value,long long invalid){
    // ASSERTION if _value can be converted to an integer,it should not equal invalid!!!!
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll;
        if(_value->type==VT_STRING){
            // we should check for 0 explicitly, which should always be represented by a single '0' character i.e. without signs!!!
            // TODO are we allowing other zero representations as well???
            if(!strcmp(_value->value._string->_c,"0"))return 0LL;
            long long ll=atoll(_value->value._string->_c);
            if(ll)return ll; // valid if non-zero
        }
        // TODO sometimes reals can also represent integers!!!
    }
    return invalid;
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
                    output("\nFailed to convert index %llu to attribute name.",_listelement->index);
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
                    long long index=getValueInteger(_maplistelementValue->value._list->_first->_value,0);
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
                        Mvalue* _attributeNameValue=_getStringValue(string(_attributeName));
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

// applying unary operators by means of functions
Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction){
    Mlist* _result=NULL;
    if(_list){
        _result=_getListOfType(_list->valuetype);
        Mlistelement* _listelement=_list->_first;
        while(_listelement&&appendedToList(_result,oneArgumentFunction(_listelement->_value),_listelement->index))_listelement=_listelement->_next;
    }
    return _result;
}
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction){
    Mmap* _result=NULL;
    if(_map){
        _result=_getMapOfType(_map->valuetype);
        Mmapelement* _mapelement=_map->_first;
        while(_mapelement&&appendedToMap(_result,_mapelement->_variable->_name,oneArgumentFunction(_mapelement->_variable->_value)))_mapelement=_mapelement->_next;
    }
    return _result;
}
bool isZero(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==0;
        if(_value->type==VT_REAL)return ldIsZero(_value->value._real->ld);
    }
    return false;
}
bool isOne(Mvalue* _value){
    if(_value){
        if(_value->type==VT_INTEGER)return _value->value._integer->ll==1;
        if(_value->type==VT_REAL)return _value->value._real->ld==1;
    }
    return false;
}
Mvalue* Mneg(Mvalue* _value){ // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_REAL)return _getRealValue(-_value->value._real->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg));
    }
    return NULL;
}
Mvalue* Mnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot));
    }
    return NULL;
}
// TODO can we not a string??????
Mvalue* Mbnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot));
    }
    return NULL;
}

// null test for the value to be considered NULL
bool isNull(Mvalue* _value){
    if(_value)
    switch(_value->type){
        case VT_INTEGER:return !_value->value._integer;
        case VT_BIGINTEGER:return !_value->value._biginteger;
        case VT_REAL:return !_value->value._real;
        case VT_STRING:return !_value->value._string;
        case VT_LIST:return !_value->value._list;
        case VT_MAP:return !_value->value._map;
        case VT_TOKEN:return !_value->value._token;
        default:break;
    }
    return true;
}
Mvalue* Mnull(Mvalue* _value){
    return _getIntegerValue(isNull(_value)?1:0);
}
Mvalue* Mundefined(Mvalue* _value){
    return _getIntegerValue(!_value?1:0);
}

Mvalue* Mlen(Mvalue* _value){
    long long result=0;
    if(_value){
        switch(_value->type){
            case VT_INTEGER:case VT_REAL:case VT_STRING:result=1;break;
            case VT_LIST:result=_value->value._list->numberOfElements;break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            default:break;
        }
    }
    return _getIntegerValue(result);
}

// MDH@29MAY2019: how about forcing the result to be a big integer instead of a long double?????
Mvalue* Mfac(Mvalue* _value){
    if(!_value){if(amVerbose())output("\nNo value!");return NULL;}
    if(amVerbose())outputValue("\nArgument of fac() function: '",_value,"'.");
    if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER){outputValue("\nERROR: Non-integer argument '",_value,"' to fac() function!");return NULL;}
    // some special cases (i.e. the input number is smaller than 2)
    mp_int* finalmultiplier=NULL;
    if(_value->type==VT_INTEGER){
        if(_value->value._integer->ll<0){output("\nERROR: Invalid (negative integer) argument to fac() function.");return NULL;}
        if(_value->value._integer->ll<3)return _getIntegerValue(_value->value._integer->ll);
        finalmultiplier=_getBigInteger(_value->value._integer->ll);
    }else{
        if(_value->value._biginteger->sign==MP_NEG){output("\nERROR: Invalid (negative integer) argument to fac() function!");return NULL;}
        if(mp_cmp(_value->value._biginteger,getBigIntegerThree())==MP_LT)return _getBigintegerValue(_getBigIntegerCopy(_value->value._biginteger));
        finalmultiplier=_value->value._biginteger;
    }
    if(!finalmultiplier){outputValue("\nERROR: Failed to convert '",_value,"' to a big integer!");return NULL;}
    if(amVerbose()&&amDebugging())outputBigInteger("\nFinal multiplier: '",finalmultiplier,"'.");
    mp_int* result=_getBigInteger(6); // the smallest value to return
    if(result){
        // we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
        // we could start at some intermediate value????
        mp_int *multiplier=_getBigInteger(3);
        if(multiplier){
            while(mp_cmp(multiplier,finalmultiplier)==MP_LT){
                if(mp_incr(multiplier)!=MP_OKAY){if(amVerbose())output("\nERROR: Failed to increment big integer!");result=NULL;break;} // if we fail to increment break
                if(mp_mul(result,multiplier,result)!=MP_OKAY){if(amVerbose())output("\nERROR: Failed to multiply big integer!");result=NULL;break;}
                //////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
            }
            // get rid of intermediate big integers
            mp_clear(multiplier);
        }
    }else
        if(amVerbose())output("\nERROR: No initial big integer 6.");
    if(_value->type==VT_INTEGER)mp_clear(finalmultiplier);
    if(amVerbose())outputBigInteger("\nResult of applying the fac() function: '",result,"'.");
    return (result?_getBigintegerValue(result):NULL);
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
}
/*
// big integer arithmetic
Mvalue* Mbi(Mvalue* _value){
    if(_value&&_value->type==VT_INTEGER){
        z_t bi;
        zinit(bi); // TODO do I need this??????
        zseti(bi,_value->value._integer->ll);
        Mvalue* _biValue=_getBigIntegerValue(bi);
    }
    return NULL;
}
*/
void assignValue(Mvalue** _valueholder,Mvalue* const _value){
    if(*_valueholder)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
    *_valueholder=_value; // replace what's being pointed to
    if(*_valueholder)incrementReferenceCount(*_valueholder); // increment what it's pointing to now (if not NULL)
}
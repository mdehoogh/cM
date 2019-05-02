#include "Mexecution.h"

#include <math.h>
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"

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

// MDH@01MAY2019: 'local' function for freeing a value
void free_value(Mvalue* _value){
    if(_value){
        // I do not need to free the value itself, only the pointers inside it
        switch(_value->type){
            case VT_UNDEFINED:break;
            case VT_INTEGER:if(_value->value._integer)free(_value->value._integer);break;
            case VT_REAL:if(_value->value._real)free(_value->value._real);break;
            case VT_STRING:free_string(_value->value._string);break;
            case VT_LIST:free_list(_value->value._list);break;
            case VT_MAP:free_map(_value->value._map);break;
        }
        free(_value);
    }
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
// can be asked to remove unused values
size_t getNumberOfRemovedValues(){
    size_t removed=0;
    if(_valueList){
        printf("\nNumber of values to check: %u.",_valueList->numberOfElements);
        Mlistelement* _lastValueListelement=NULL; // the last value list element processed that is still present
        Mlistelement* _nextValueListelement;
        Mlistelement* _valueListelement=_valueList->_first;
        size_t checked=0;
        while(_valueListelement){
            checked++;
            if(_valueListelement->_value&&!_valueListelement->_value->count){ // unused
                if(amVerbose())outputLine("NOTE: Removing a value.");
                removed++;
                free_value(_valueListelement->_value);
                _valueListelement->_value=NULL; // just in case
                // make the previous list element point to the next (skipping the unused value)
                if(_lastValueListelement) // at least one value in the value list
                    _lastValueListelement->_next=_valueListelement->_next;
                else // no values before this element in the list, so make _first point to the next element!!!
                    _valueList->_first=_valueListelement->_next;
                // one down
                _valueList->numberOfElements--;
                free(_valueListelement);
            }else // keeping the current value list element, which therefore is the last value list element
                _lastValueListelement=_valueListelement;
            // if there is no last value list element at the moment we should continue with the first element
            _valueListelement=(_lastValueListelement?_lastValueListelement->_next:_valueList->_first);
        }
        if(amVerbose())output("\nNumber of values checked: %lu.",checked);
    }
    return removed;
}
bool decrementReferenceCount(Mvalue* _value){
    if(_value){
        if(_value->count){
            _value->count--;
            return true;
        }
        outputLine("BUG: Reference count already zero."); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
    }else
        if(amVerbose())outputLine("No value to decrement the reference count of.");
    return true;
}
bool incrementReferenceCount(Mvalue* _value){
    if(_value){
        _value->count++;
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
Mreal* new_real(long double ld){
    Mreal* _real=malloc(sizeof(Mreal));
    if(_real)_real->ld=ld;
    return _real;
}
// Mstring is an immutable version of mstring* in that it cannot be changed
Mstring* new_string(char* _text){ // _text assumed to be string(mstring*), so we can simply copy it over with the starting quote character (" or ')
    return (Mstring*)_strdup(_text);
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
Mvalue* _getRealValue(long double ld){
    Mreal* _real=new_real(ld);
    Mvalue* _realvalue=(_real?_newValue():NULL);
    if(_realvalue){_realvalue->type=VT_REAL;_realvalue->value._real=_real;}
    return _realvalue;
}
Mvalue* _getStringValue(char* _s){
    Mstring* _string=(_s?new_string(_s):NULL); // for mstring* sources pass string(mstring*) into getStringValue() (which points to mstring->chars which always start with the quote char used in declaring the literal)
    Mvalue* _stringvalue=(_string?_newValue():NULL);
    if(_stringvalue){_stringvalue->type=VT_REAL;_stringvalue->value._string=_string;}
    return _stringvalue;
}
// we can force all listelements to have the same type????
Mvalue* _getListValue(Mvaluetype listValuetype){
    Mlist* _list=(Mlist*)malloc(sizeof(Mlist));
    _list->valuetype=listValuetype; // register what type of elements this list should have
    Mvalue* _listvalue=(_list?_newValue():NULL);
    if(_listvalue){_listvalue->type=VT_LIST;_listvalue->value._list=_list;}
    return _listvalue;
}
Mvalue* _getMapValue(Mvaluetype mapValuetype){
    Mmap* _map=(Mmap*)malloc(sizeof(Mmap));
    _map->valuetype=mapValuetype;
    Mvalue* _mapvalue=(_map?_newValue():NULL);
    if(_mapvalue){_mapvalue->type=VT_MAP;_mapvalue->value._map=_map;}
    return _mapvalue;
}
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
        free_map(_functiondefinition->_parameterMap);
        free_expressionlist(_functiondefinition->_expressionlist);
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
    if(!_environment||!name||!_value){output("\nERROR: Cannot set the value: no environment, name or value.");return false;}
    Mvariable* _variable=getVariable(_environment,name,false);
    if(_variable){
        if(!_variable->_value||!_variable->immutable){
            // _value needs to be of the right type
            if(!_value||_variable->valuetype==VT_UNDEFINED||_variable->valuetype==_value->type){
                if(_variable->_value)_variable->_value->count--; // decrement the reference count on the current value
                _variable->_value=_value; // store the reference
                if(_variable->_value)_variable->_value->count++; // increment the reference count
                return true; // releasing the value is my responsibility now...
            }
            printf("\nERROR: Cannot set the value of variable '%s': the new value is of the wrong type.",name);
        }else
            printf("\nERROR: Cannot set the value of variable '%s': it is not mutable!",name);
    }else
        printf("\nERROR: Cannot set the value of variable '%s':it is unknown.",name);
    return false;
}
bool appendedToList(Mlist* _list,Mvalue* _value){
    if(!_list||!_value){output("\nERROR: %s.","No list to append to or no value to append.");return false;}
    Mlistelement* _listelement=(Mlistelement*)calloc(1,sizeof(Mlistelement*));
    if(!_listelement){output("\nERROR: Failed to create a new list element to append.");return false;}
    _listelement->_value=_value;
    incrementReferenceCount(_listelement->_value); // increment the reference count of the stored value immediately
    if(_list->_last)_list->_last->_next=_listelement;else _list->_first=_listelement;
    _list->_last=_listelement;
    _list->numberOfElements++;
    return true;
}
bool appendToListVariable(Menvironment* _environment,const char* name,Mvalue* _value){
    if(!_environment||!name||!_value){output("\nERROR: %s","No environment, variable name of value specified!");return false;}
    Mvariable* _variable=getVariable(_environment,name,amVerbose());
    if(_variable){
        Mvalue* _variableValue=_variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
        if(_variableValue->type==VT_LIST){ // yes a list we can append to
            // we should prevent circular references
            if(_variableValue==_value)_value=NULL;
            Mlist* _list=_variableValue->value._list;
            if(appendedToList(_list,_value)){
                return true; // releasing the value is my responsibility now...
            }
            output("\nERROR: Didn't append the value to the list stored in variable '%s': the type of the new value (%u) is wrong.",name,_value->type);
        }else
            output("\nERROR: Cannot append the value to variable '%s': it does not contain a list!",name);
    }else
        output("\nERROR: Cannot set the value of variable '%s':it is unknown.",name);
    return false;
}

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

Mmap* getFunctionArgumentMap(Mfunction* _function,Mlist* _argumentList){
    if(_function){
        Mmap* _argumentMap=(Mmap*)calloc(1,sizeof(Mmap));
        Mmap* _functionParameterMap=_function->_parameterMap;
        Mmapelement* _functionParameterMapelement=_functionParameterMap->_first;
        Mlistelement* _argumentListelement=_argumentList->_first;
        while(_functionParameterMapelement){
            Mmapelement* _argumentmapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            _argumentmapelement->_variable->_name=_functionParameterMapelement->_variable->_name;
            // associate the argument list element value (if available)
            if(_argumentListelement){
                _argumentmapelement->_variable->_value=_argumentListelement->_value;
                _argumentListelement=_argumentListelement->_next;
            }else // use the default!!!
                _argumentmapelement->_variable->_value=_functionParameterMapelement->_variable->_value;
            // append to _argumentMap
            if(_argumentMap->_last)_argumentMap->_last->_next=_argumentmapelement;else _argumentMap->_first=_argumentmapelement;
            _argumentMap->_last=_argumentmapelement;_argumentMap->numberOfElements++;
            _functionParameterMapelement=_functionParameterMapelement->_next;
        }
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
Mmap* _getSingleRealMap(char* name,Mvalue* _realValue){
    if(name&&_realValue){
        Mvariable* _realVariable=_createVariable(name,VT_REAL,true);
        if(_realVariable){
            _realVariable->_value=_realValue;
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_realVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    printf("\n%s","Returning the single real map!");
                    return _map;
                }
                printf("\nERROR: %s.","Failed to create the real variable map!");
               free_mapelement(_mapelement);
            }else{
                printf("\nERROR: %s.","Failed to create the real variable map element!");
                free_variable(_realVariable);
            }
        }else
            printf("\nERROR: %s.","Failed to create the real variable!");
    }
    return NULL;
}
Mmap* _getSingleIntegerMap(char* name,Mvalue* _integerValue){
    if(name&&_integerValue){
        Mvariable* _integerVariable=_createVariable(name,VT_INTEGER,true);
        if(_integerVariable){
            _integerVariable->_value=_integerValue;
            Mmapelement* _mapelement=(Mmapelement*)calloc(1,sizeof(Mmapelement));
            if(_mapelement){
                _mapelement->_variable=_integerVariable;
                Mmap* _map=(Mmap*)calloc(1,sizeof(Mmap));
                if(_map){
                    _map->numberOfElements=1;
                    _map->_first=_mapelement;
                    printf("\n%s","Returning the single integer map!");
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
// end helper functions 

// the internal functions (from math) can be registered with a given (most likely root) environment
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

bool completedOneArgumentRealFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getSingleRealMap("x",_getRealValue(0.0));
        printf("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}
bool completedOneArgumentIntegerFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getSingleIntegerMap("i",_getIntegerValue(0));
        printf("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}

// these internal functions do NOT have a body as M defined functions have...
bool registerInternalFunctions(Menvironment* _environment){
    // let's try to register the sine function
    if(!completedOneArgumentRealFunction(newFunction(_environment,"cos"),Mcos))return false;
    if(!completedOneArgumentRealFunction(newFunction(_environment,"sin"),Msin))return false;
    return true;
}


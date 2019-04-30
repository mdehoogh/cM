#include <math.h>
#include "Mexecution.h"

// RELEASERS
// however we can only NULL them if we have the address of the pointer)
// but if these pointer are local to a function (which they will be typically if they are to be released in the first place) no NULLing is required!!!
void free_string(Mstring* _string){
    if(_string){if(_string->_m)free_mstring(_string->_m);_string->_m=NULL;free(_string);}
}
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
void free_listelement(Mlistelement* _listelement){
    if(_listelement){
        if(_listelement->_next)free_listelement(_listelement->_next);
        free_value(_listelement->_value);
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
        free_value(_variable->_value);
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

Mvariable* getVariable(Menvironment* _environment,const char* name){
    if(_environment&&name&&strlen(name)){ // input valid        
        Mmapelement*_variableMapelement=_environment->_variableMap->_first;
        // as long as variable is defined, and the variable's name is not equal to the given name, continue
        while(_variableMapelement!=NULL&&strcmp(_variableMapelement->_variable->_name,name))_variableMapelement=_variableMapelement->_next;
        if(_variableMapelement!=NULL)return _variableMapelement->_variable;
    }
    return NULL;
}

// write access
// helper function to create a new variable with a given name and of a given type
Mvariable* _createVariable(char* name,enum Mvaluetype valueType){
    if(!name||!strlen(name)){
        printf("\nERROR: No variable name defined.");
        return NULL;
    }
    Mvariable* _variable=(Mvariable*)malloc(sizeof(Mvariable));
    if(!_variable){
        printf("\nERROR: Failed to allocate memory to store variable '%s'.",name);
        return NULL;
    }
    _variable->_name=heap_string_copy(name);
    if(!_variable->_name){
        free_variable(_variable);
        printf("\nERROR: Failed to allocate memory to store name '%s' of the new variable.",name);
        return NULL;
    }
    // a ha, here we have an issue: we cannot just use the char pointer, if we want to free the name later on
    if(valueType!=VT_UNDEFINED){
        _variable->_value=(Mvalue*)calloc(1,sizeof(Mvalue));
        if(!_variable->_value){
            free_variable(_variable);
            printf("\nERROR: Failed to allocate memory to store the value of variable '%s'.",name);
            return NULL;
        }
        _variable->_value->type=valueType;
        // I do NOT need to set the values of the atomic elements (integer, real), so basically these are not initialized to start with
        // NOTE using calloc() instead of malloc() is essential for everything with pointers in it that should be NULL initially!!
        switch(valueType){
            case VT_UNDEFINED:
            case VT_INTEGER:
            case VT_REAL:break;
            case VT_STRING:_variable->_value->value._string=(Mstring*)calloc(1,sizeof(Mstring));break;
            case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
            case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
        }
    }else
        _variable->_value=NULL;
    return _variable;
}

// addVariable returns the value map element that was created (if successful)
Mvariable* addVariable(Menvironment* _environment,char* name,enum Mvaluetype valueType){
    Mvariable* _variable=NULL;
    if(_environment&&name){ // input valid 
        _variable=getVariable(_environment,name);
        if(!_variable){ // non-existing...
            _variable=_createVariable(name,valueType);
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
                    return _variable;
                }
                // ASSERT failed to link the variable to the variable map!!
                free(_variable);
                printf("\nERROR: Failed to link variable '%s'.",name);
            }else
                printf("\nERROR: Failed to create variable '%s'.",name);
        }
    }
    return NULL;
}

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
        Mvariable* _realVariable=_createVariable(name,VT_REAL);
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
// end helper functions 

// the internal functions (from math) can be registered with a given (most likely root) environment
Mvalue* Msin(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return getRealValue(get_real(sin(_value->value._real->ld)));
        if(_value->type==VT_INTEGER)return getRealValue(get_real(sin(_value->value._integer->ll)));
    }
    return NULL;
}
Mvalue* Mcos(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return getRealValue(get_real(cos(_value->value._real->ld)));
        if(_value->type==VT_INTEGER)return getRealValue(get_real(cos(_value->value._integer->ll)));
    }
    return NULL;
}

bool completedOneArgumentFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getSingleRealMap("x",getRealValue(get_real(0.0)));
        printf("\nRegistered function '%s' completed.",string(_function->_name));
        return true;
    }
    return false;
}

// these internal functions do NOT have a body as M defined functions have...
bool registerInternalFunctions(Menvironment* _environment){
    // let's try to register the sine function
    if(!completedOneArgumentFunction(newFunction(_environment,"cos"),Mcos))return false;
    if(!completedOneArgumentFunction(newFunction(_environment,"sin"),Msin))return false;
    return true;
}


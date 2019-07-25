/**
 * MDH@24JUN2019: everything that deals with Menvironments
 */
#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "Mstring.h"
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"

#include "Menvironment.h"

// externally (in M.c) defined constants
const char* const DEFINEUSERFUNCTION_NAME; // the name of the define user function function
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* const ERROR_PREFIX;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!

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
void free_environment(Menvironment* _environment){
    if(_environment){
        if(_environment->_name){free(_environment->_name);_environment->_name=NULL;}
        _environment->_execution=NULL;
        free_map(_environment->_variableMap);
        /* MDH@10JUL2019: only Menvironment has a function map!!   
           MDH@20JUL2019: NO user functions may also contain a function map, which is referenced in a user function execution environment
                          and indeed being a referenced they should not be freed (otherwise we would loose these nested functions on
                          freeing the execution environment)
        if(_environment->_functionMap)free_functionmap(_environment->_functionMap);
        */
        FREE(_environment,'E');
    }
}/* VALIDATED */
Menvironment* __environment(){
    Menvironment* _environment=CALLOC(1,sizeof(Menvironment),'E');
    if(!_environment)return NULL;
    _environment->_variableMap=CALLOC(1,sizeof(Mmap),'M'); // ascertain that the environment contains a variable map
    if(!_environment->_variableMap){free_environment(_environment);_environment=NULL;}
    return _environment;
}/* VALIDATED */
// keep track of the current execution environment
static Menvironment* _executionEnvironment=NULL;
void outputEnvironmentName(){
    Mstring* _environmentName=_getEnvironmentName();
    output("Current execution environment: '%s'.\n",string(_environmentName));
    free_string(_environmentName);
}
bool pushExecutionEnvironment(Menvironment* _environment){
    if(!_environment)return false;
    if(!_environment->_parent)_environment->_parent=_executionEnvironment; // if without a parent give it the current one
    _environment->_execution=_executionEnvironment; // remember to what execution environment to pop back to
    _executionEnvironment=_environment;
    if(amVerbose())outputEnvironmentName();
    return true;
}/* VALIDATED */
void popExecutionEnvironment(){
    if(!_executionEnvironment){outputLine("BUG: No environment left to pop!");return;} // nothing to pop
    // NOTE only execution environments that have a parent can be popped!!!
    Menvironment* _previousExecutionEnvironment=_executionEnvironment->_execution;
    if(!_previousExecutionEnvironment){outputLine("BUG: Can't pop top-most environment!");return;}
    free_environment(_executionEnvironment); // TODO I guess we won't be needing this execution environment any more????
    _executionEnvironment=_previousExecutionEnvironment;
    if(amVerbose())outputEnvironmentName();
}/* VALIDATED */
Menvironment* getEnvironment(){return _executionEnvironment;}/* VALIDATED */
Mstring* _getEnvironmentName(){
    Mstring* _environmentName=__string();
    if(_environmentName){
        Mstring* p=_environmentName;
        Menvironment* _environment=_executionEnvironment;
        while(p&&_environment){
            if(string_length(p)>0)p=string_insert_char(p,0,'.');
            ////////output("Prepending '%s'.\n",_environment->_name);
            p=string_prepend(p,_environment->_name);
            _environment=_environment->_parent;
        }
        if(!p){free_string(_environmentName);_environmentName=NULL;}
    }
    return _environmentName;
}
Mtoken* getEnvironmentExpressionToken(){
    // MDH@22JUL2019: let's allow breaking here
    if(kbhit())return NULL;
    return _executionEnvironment->expressionToken;
}/* VALIDATED */
Mtoken* nextEnvironmentExpressionToken(){
    if(!_executionEnvironment)return NULL;
    if(_executionEnvironment->expressionToken)_executionEnvironment->expressionToken=_executionEnvironment->expressionToken->next;
    return _executionEnvironment->expressionToken;
}/* VALIDATED */

// read access to the elements defined in an environment
uint32_t getNumberOfVariables(const Menvironment* const _environment){
    if(!_environment||!_environment->_variableMap)return 0;
    // MDH@20JUL2019: now returning the sum of the variables in the parent plus those in the environment itself!!
    return getNumberOfVariables(_environment->_parent)+_environment->_variableMap->numberOfElements;
}/* VALIDATED */

// the names of the variables may be requested
Mstring* _getVariableNames(const Menvironment* const _environment,const char* const sep){
    Mstring* _variableNames=NULL;
    if(_environment&&sep){
        _variableNames=__string();
        if(_variableNames){
            Mstring* p=_variableNames;
            // first append the names of the variables in the parent
            if(_environment->_parent){
                Mstring* _parentVariableNames=_getVariableNames(_environment->_parent,sep); // free asap
                if(_parentVariableNames){
                    p=string_append(p,string(_parentVariableNames));
                    free_string(_parentVariableNames); // we can do this because string_append copies the characters
                }
            }
            // we'll be appending the names of the variables in the environment itself
            if(_environment->_variableMap){
                Mmapelement* _variableMapelement=_environment->_variableMap->_first;
                while(p&&_variableMapelement){
                    if(_variableMapelement->_variable){
                        if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
                        p=string_append(p,_variableMapelement->_variable->_name);
                    }
                    _variableMapelement=_variableMapelement->_next;
                }
            }
            if(!p){free_string(_variableNames);_variableNames=NULL;}
        }
    }
    return _variableNames;
}/* VALIDATED */

Mvariable* getVariable(const Menvironment* const _environment,const char* const name, bool verbose){
    if(!_environment)return NULL;
    if(!name){outputError("No variable name specified");return NULL;}
    // input valid
    if(!_environment->_variableMap){if(verbose)output("%sEnvironment to find variable '%s' in is empty.\n",ERROR_PREFIX,name);return NULL;}
    ///////////if(amVerbose())output("Looking for variable '%s'.\n",name);
    Mmapelement* _variableMapelement=_environment->_variableMap->_first;
    // as long as variable is defined, and the variable's name is not equal to the given name, continue
    while(_variableMapelement&&(!_variableMapelement->_variable||strcmp(_variableMapelement->_variable->_name,name)))_variableMapelement=_variableMapelement->_next;
    // MDH@20JUL2019: if not defined locally, perhaps in parent
    return(_variableMapelement?_variableMapelement->_variable:getVariable(_environment->_parent,name,verbose));
}/* VALIDATED */
bool containsVariable(const Menvironment* const _environment,const char* const name){return(getVariable(_environment,name,false)!=NULL);}/* VALIDATED */
// use Mexists to determine if a variable exists passed in as text, we might decide to return the name of the environment it exists in
Mvalue* Mexists(Mvalue* _value){
    if(_value&&_value->type==VT_TEXT){
        return _getIntegerValue(getVariable(getEnvironment(),_value->value._text->_c,false)?1:0);
    }
    return NULL; // input invalid
}
// write access
// helper function to create a new variable with a given name and of a given type

// addVariable returns the value map element that was created (if successful)
bool addVariable(Menvironment* const _environment,const char* const name,Mvaluetype valuetype,bool immutable){
    Mvariable* _variable=NULL;
    if(_environment&&name){ // input valid
        _variable=getVariable(_environment,name,false);
        if(!_variable){ // non-existing...
            _variable=_getVariable(name,valuetype,immutable); // creates the variable, free when not bound
            //////printf("\nVariable created!");
            if(_variable){
                Mmapelement* _variableMapelement=(Mmapelement*)MALLOC(sizeof(Mmapelement),'V');
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
                outputErrorAndText("Failed to link variable ",name);
            }else
                outputErrorAndText("%sFailed to create variable ",name);
        }
    }
    return false;
}/* VALIDATED */

bool setValue(const Menvironment* const _environment,const char* const name,const Mvalue* const _value){
    // NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
    if(!_environment||!name){outputError("Cannot set the value: no environment or name");return false;}
    Mvariable* variable=getVariable(_environment,name,false);
    if(variable){
        if(!variable->_value||!variable->immutable){
            // _value needs to be of the right type
            if(!_value||variable->valuetype==VT_UNDEFINED||variable->valuetype==_value->type){
                ///////////////if(_variable->_value)_variable->_value->count--; // decrement the reference count on the current value
                assignValue(&variable->_value,_value); // 'assign' the reference (takes care of updating the reference counts)
                if(amVerbose()){Mstring* _valueText=_getValueText(variable->_value,false);output("Value '%s' with count '%u' assigned to variable '%s'.\n",string(_valueText),variable->_value->count,name);free_string(_valueText);}
                ///////////////if(_variable->_value)_variable->_value->count++; // increment the reference count
                return true; // releasing the value is my responsibility now...
            }
            output("%sCannot set the value of variable `%s`: the new value is of the wrong type.\n",ERROR_PREFIX,name);
        }else
            output("%sCannot set the value of variable `%s`: it is not mutable!\n",ERROR_PREFIX,name);
    }else
        output("%sCannot set the value of variable `%s`:it is unknown.\n",ERROR_PREFIX,name);
    return false;
}/* VALIDATED */

long long appendToListVariable(const Menvironment* const _environment,const char* const name,const Mvalue* const _value){
    if(!_environment||!name){outputError("No environment or variable name specified");return 0;}
    Mvariable* variable=getVariable(_environment,name,amVerbose());
    if(variable){
        Mvalue* variableValue=variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
        if(variableValue&&variableValue->type==VT_LIST){ // yes a list we can append to
            // we should prevent circular references
            if(variableValue!=_value){
                unsigned long long index=appendedToList(variableValue->value._list,_value,0); // NOTE always append to the end of the list with the first available index that's why I'm passing in 0 instead of a positive index value!!
                if(index>0)return index;
                output("%sFailed to append the value to the list stored in variable '%s': the type of the new value (%u) is wrong.\n",ERROR_PREFIX,name,(_value?_value->type:-1));
            }else
                outputError("Circular reference not allowed");
        }else
            output("%sCannot append the value to variable '%s': it does not contain a list!\n",ERROR_PREFIX,name);
    }else
        output("%sCannot set the value of variable '%s':it is unknown.\n",ERROR_PREFIX,name);
    return 0;
}/* VALIDATED */

Mvalue* getValue(const Menvironment* const _environment,const char* const name){
    if(!_environment||!name){outputError("No environment or name specified");return NULL;}
    Mvariable* variable=getVariable(_environment,name,false);
    return(variable?variable->_value:NULL);
}/* VALIDATED */

// FUNCTION STUFF
// the names of the variables may be requested
Mstring* _getFunctionNames(const Menvironment* const _environment,const char* const sep){
    Mstring* _functionNames=NULL;
    if(_environment&&sep){
        _functionNames=__string();
        if(_functionNames){
            Mstring* p=_functionNames;
            // first append the names of the variables in the parent
            if(_environment->_parent){
                Mstring* _parentFunctionNames=_getFunctionNames(_environment->_parent,sep);
                if(_parentFunctionNames){
                    p=string_append(p,string(_parentFunctionNames));
                    free_string(_parentFunctionNames); // we can do this because string_append copies the characters that string() points to!!
                }
            }
            // we'll be appending the names of the variables in the environment itself
            if(p&&_environment->_functionMap){
                Mfunctionmapelement* functionmapelement=_environment->_functionMap->_first;
                while(p&&functionmapelement){
                    if(functionmapelement->_function){
                        if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
                        p=string_append(p,string(functionmapelement->_name)); //////_function->_name));
                    }
                    functionmapelement=functionmapelement->_next;
                }
            }
            if(!p){free_string(_functionNames);_functionNames=NULL;}
        }
    }
    return _functionNames;
}/* VALIDATED */

Mfunction* getFunction(const Menvironment* const _environment,const char* const functionName){
    if(_environment&&functionName&&strlen(functionName)){
        Mfunctionmap* functionmap=_environment->_functionMap;
        if(functionmap){
            Mfunctionmapelement* functionmapelement=functionmap->_first;
            // we need string() on the function name as function name is an Mstring*
            while(functionmapelement){
                // MDH@10JUL2019: _name moved to the map element instead of in the function
                if(functionmapelement->_function&&!strcmp(string(functionmapelement->_name),functionName)){
                    //////////printf("\nFunction '%s' matches '%s'.",string(_functionmapelement->_function->_name),functionName);
                    return functionmapelement->_function;
                }
                functionmapelement=functionmapelement->_next;
            }
        }
        // might exist in the parent environment
        if(_environment->_parent)return getFunction(_environment->_parent,functionName);
    }
    return NULL;
}/* VALIDATED */
/*
Muserfunction* getUserfunction(const Menvironment* const _environment,const char* const userfunctionName){
    if(_environment&&userfunctionName&&strlen(userfunctionName)){
        Mvariable* _functionVariable=getVariable(_environment,userfunctionName,false);
        if(_functionVariable)if(_functionVariable->_value->type==VT_USERFUNCTION)return _functionVariable->_value->value._userfunction;
    }
    return NULL;
}// VALIDATED
*/
Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList){
    Mmap* _functionArgumentMap=NULL;
    if(_function&&_argumentList){
        _functionArgumentMap=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
        Mmap* functionParameterMap=_function->_parameterMap;
        if(functionParameterMap){
            if(amVerbose())outputLine("Matching the function parameters!");
            Mmapelement* functionParameterMapelement=functionParameterMap->_first;
            Mlistelement* argumentListelement=_argumentList->_first;
            while(functionParameterMapelement){
                Mmapelement* _argumentmapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
                if(!_argumentmapelement)break; // TODO should we return NULL?????
                // BUG FIX I suppose we need _variable to point to something
                _argumentmapelement->_variable=(Mvariable*)CALLOC(1,sizeof(Mvariable),'V');
                if(!_argumentmapelement->_variable){free_mapelement(_argumentmapelement);break;}
                // probably can't simply assign??? let's use _strdup then
                _argumentmapelement->_variable->_name=_strdup(functionParameterMapelement->_variable->_name);
                if(!_argumentmapelement->_variable->_name){free_mapelement(_argumentmapelement);break;}
                // associate the argument list element value (if available)
                if(argumentListelement){
                    assignValue(&_argumentmapelement->_variable->_value,argumentListelement->_value);
                    argumentListelement=argumentListelement->_next;
                }else // use the default!!!
                    assignValue(&_argumentmapelement->_variable->_value,functionParameterMapelement->_variable->_value);
                // append to _argumentMap
                if(_functionArgumentMap->_last)_functionArgumentMap->_last->_next=_argumentmapelement;else _functionArgumentMap->_first=_argumentmapelement;
                _functionArgumentMap->_last=_argumentmapelement;
                _functionArgumentMap->numberOfElements++;
                functionParameterMapelement=functionParameterMapelement->_next;
            }
        }
        if(amVerbose())outputLine("Argument map created.");
    }
    return _functionArgumentMap;
}/* VALIDATED */

// newFunction renamed to _getFunction(), not to be confused with getFunction()
// _getFunction() will create the function
Mfunction* _getFunction(Menvironment* const _environment,const char* const name){
    Mfunction* _function=NULL;
    if(_environment&&name&&strlen(name)){
        _function=getFunction(_environment,name);
        if(!_function){ // doesn't exist yet
            _function=(Mfunction*)CALLOC(1,sizeof(Mfunction),'F');
            if(_function){
                ///////////_function->type=functionType;
                _function->_definitionEnvironment=_environment; // TODO why would we need this?????
                Mstring* _functionName=__string();
                if(_functionName){
                    Mstring* p=_functionName;
                    p=string_append(p,name);
                    if(p){
                        Mfunctionmap* _functionmap=_environment->_functionMap;
                        if(_functionmap){
                            Mfunctionmapelement* _functionmapelement=(Mfunctionmapelement*)CALLOC(1,sizeof(Mfunctionmapelement),'f');
                            if(_functionmapelement){
                                _functionmapelement->_name=_functionName; // MDH@10JUL2019: moved over to the function map element
                                _functionmapelement->_function=_function; // no worries here
                                Mfunctionmapelement* _lastFunctionmapelement=_functionmap->_last;
                                if(_lastFunctionmapelement){
                                    _lastFunctionmapelement->_next=_functionmapelement;
                                    _functionmap->_last=_functionmapelement;
                                }else
                                    _functionmap->_first=_functionmapelement;
                                _functionmap->_last=_functionmapelement;
                                _functionmap->numberOfFunctions++;
                                ///////_function->_name=_functionName; // success!!!!!
                                if(amVerbose())output("Function '%s' registered as function #%d.\n",name,_functionmap->numberOfFunctions);
                            }else // failure
                                p=NULL;
                        }else
                            p=NULL;
                    }
                    if(!p){free_string(_functionName);_functionName=NULL;} // p==NULL indicates _functionName not bound in _function->_name
                    // try to append it to the functionMap, if we succeed store _functioName in ->_name
                }else
                    output("%sFailed to store function name '%s'.\n",ERROR_PREFIX,name);
                // if we fail to register the name and/or the function with the environment free the function!!
                if(!_functionName){free_function(_function);_function=NULL;}   
            }
            if(!_function)output("%sFailed to create function '%s'.\n",ERROR_PREFIX,name);
        }else
            output("NOTE: Function '%s' already exists.\n",name);
    }
    return _function;
}/* VALIDATED */

// END FUNCTION STUFF

// the internal functions
/**
 * Msettype() to set the (value) type of a variable
 */
Mvalue* Msettype(Mvalue* _variableName,Mvalue* _valuetype){
    // check the types first, both should be strings
    if(_variableName->type==VT_TEXT&&_valuetype->type==VT_TEXT){
        char* variableName=_variableName->value._text->_c; // ignoring the presuffix exactly as we need to!!!
        if(strlen(variableName)){
            // get the value type, we can use uppercase to indicate an immutable (constant) variable?????
            bool immutable=false;
            Mvaluetype valuetype=VT_UNDEFINED;
            switch(_valuetype->value._text->_c[0]){ // use the first character (which will be '\0' if the default value is used!!!)
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
                    valuetype=VT_TEXT;
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
                    return _getCharTextValue(_variable->immutable?IMMUTABLEVALUETYPECHARS[_variable->valuetype]:MUTABLEVALUETYPECHARS[_variable->valuetype]);
                }
            }
        }
    }
    return NULL;
}/* VALIDATED */

bool completedFunction(Mfunction* const _function,char* functionName,NoArgumentFunction noArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_NO_ARGUMENTS;
        _function->functionunion.noArgumentFunction=noArgumentFunction;
        _function->_parameterMap=NULL;
        if(amVerbose())output("Registered no-argument function '%s' completed.\n",functionName);
        return true;
    }
    return false;
}/* VALIDATED */
bool completedValueFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getMap("v");
        if(_function->_parameterMap){
            // no defaults here!!!
            if(amVerbose())output("Registered single value argument function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single value argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedRealFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getRealMap("x",_getRealValue(M_LD_NAN)); // MDH@20JUN2019: now using the invalid real value as default (to indicate a missing value)
        if(_function->_parameterMap){
            if(amVerbose())output("Registered single real argument function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single real argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedIntegerFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getIntegerMap("i",_getIntegerValue(M_LL_INVALID)); // MDH@20JUN2019: now using the invalid value as default (to indicate a missing!!!!)
        if(_function->_parameterMap){
           if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;    
        }
        output("%sFailed to register single integer argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedListFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getListMap("l",_getListValue(VT_UNDEFINED));
        if(_function->_parameterMap){
            if(amVerbose())output("Registered list function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single list argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedStringStringFunction(Mfunction* const _function,char* functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getStringStringMap("variable","type");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register double string argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedRealRealFunction(Mfunction* const _function,char* functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getRealRealMap("base","exponent");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register double real argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedStringMapTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getStringMapTokenMap("name","parameters","body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register string map list argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedTokenTokenFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getTokenTokenMap("while condition","while body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register two token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedValueTokenTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getValueTokenTokenMap("if condition","then clause","else clause");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register three token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedTokenTokenTokenTokenFunction(Mfunction* const _function,const char* const functionName,FourArgumentFunction fourArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_FOUR_ARGUMENTS;
        _function->functionunion.fourArgumentFunction=fourArgumentFunction;
        _function->_parameterMap=_getTokenTokenTokenTokenMap("for initialization","for condition","for increment","for body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register four token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */

// MDH@09JUL2019: a function is defined as a parameter map (with defaults) and a body token
// MDH@10JUL2019: the caller will need to register the function in the function map of the definition environment
//                here we only need to return the wrapped user function
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
    Mvalue* _value=__value();
    if(_value){_value->type=VT_TOKEN;_value->value._userfunction=_userfunction->_bodyTokenValue->value._token;}
    if(!_value)free_userfunction(_userfunction);
    return _value;
}
*/
unsigned long long getNumberOfFunctionCommands(const char* const functionName){
    Mfunction* function=getFunction(getEnvironment(),functionName);
    if(!function||function->type!=FT_USER){if(!function)output("%sFunction '%s' not found.\n",ERROR_PREFIX,functionName);return -1;}
    return (function->functionunion._userfunction->_bodyCommandList?function->functionunion._userfunction->_bodyCommandList->numberOfElements:0);
}
bool registerFunctionCommand(const char* const functionName,Mtoken* command){
    if(!functionName||!command)return false;
    Mfunction* function=getFunction(getEnvironment(),functionName);
    if(function&&function->type==FT_USER){
        Mvalue* _commandValue=_getValueOfToken(command,false);
        if(_commandValue){
            if(!function->functionunion._userfunction->_bodyCommandList)
                function->functionunion._userfunction->_bodyCommandList=CALLOC(1,sizeof(Mlist),'L');
            if(appendedToList(function->functionunion._userfunction->_bodyCommandList,_commandValue,0))return true;
            output("%sFailed to add command to list of body of '%s'.\n",ERROR_PREFIX,functionName);
        }else
            output("%sFailed to wrap a command of function '%s'.\n",ERROR_PREFIX,functionName);
    }else
        output("%s'%s' does not represent a user function.\n",ERROR_PREFIX,functionName);
    return false;
}

Mvalue* Mdefinefunction(Mvalue* _nameValue,Mvalue* _parameterMapValue,Mvalue* _bodyTokenValue){
    // the user specifies the body as a text (to prevent evaluation during defining the function)
    // but perhaps it could also be a list of tokens????? i.e. already tokenized (that is not evaluated)
    // of course, tokenizing is a problem later on, but this means that we need to prevent evaluation of the second argument before calling this function on it
    if(_nameValue&&_parameterMapValue){
        if(_nameValue->type==VT_TEXT&&_parameterMapValue->type==VT_MAP&&(!_bodyTokenValue||_bodyTokenValue->type==VT_TOKEN)){
            Muserfunction* _userfunction=(Muserfunction*)CALLOC(1,sizeof(Muserfunction),'U');
            if(_userfunction){
                Mtext* functionName=_nameValue->value._text;
                // user function expects a list of commands, so we have to wrap the single token (if any)
                if(_bodyTokenValue){
                    _userfunction->_bodyCommandList=_getListOfType(VT_TOKEN);
                    if(!_userfunction->_bodyCommandList||appendedToList(_userfunction->_bodyCommandList,_bodyTokenValue,0))
                        output("%sFailed to store the inline command as body of function definition of '%s'.\n",ERROR_PREFIX,functionName->_c);
                    // replacing: assignValue(&_userfunction->_bodyTokenValue,_bodyTokenValue);
                }
                //////////Mvalue* _userfunctionValue=_getUserfunctionValue(_userfunction,true); // free asap or bound
                ///////if(_userfunctionValue){
                    // MDH@17JUL2019: the map needs to be stored with the Mfunction
                Mfunction* _function=_getFunction(getEnvironment(),functionName->_c);
                if(_function){
                    _function->_parameterMap=_parameterMapValue->value._map;
                    _function->functionunion._userfunction=_userfunction;
                    // return the result of applying the function to the default parameter map

                    return _getIntegerValue(1);
                }
                ///////////free_value(_userfunctionValue); // freed
                output("%sFailed to create function '%s'.\n",ERROR_PREFIX,functionName);
                ///////}
            }
        }else
            outputError("Invalid user function name, parameter map or body");
    }else{
        if(!_nameValue)outputError("No name defined of function");
        if(!_parameterMapValue)outputError("No (formal) parameter map defined for function");
        ////////if(!_bodyTokenValue)outputError("No body (expression) defined of function");
    }
    return _getIntegerValue(0); // indicating failure...
}/*VALIDATED */

Mvalue* Mreturn(Mvalue* _value){
    // sets the value of the function execution result variable to _value
    // if you call return with NO value, the current value of $ will be used (or the value of the last executed function body command)
    // do NOT replace the value of "$" if _value is NULL (which should indicate a return without argument), this means you cannot undo the result value
    // perhaps with $=NULL though
    if(_value!=NULL&&!setValue(getEnvironment(),"$",_value)){
        outputError("Failed to set the function execution result variable");
        return NULL;
    }
    // setting the exit flag variable will get the function execution aborted
    if(!setValue(getEnvironment(),"!",_getIntegerValue(1))){
        outputError("Failed to set the function execution exit flag variable");
        return NULL;
    }
    return _value; // echo the input value
}/*VALIDATED */

// these internal functions do NOT have a body as M defined functions have...
bool registerInternalFunctions(Menvironment* const _environment){
    // variable functions
    // math functions
    if(!completedRealFunction(_getFunction(_environment,"cos"),"cos",Mcos))return false;
    if(!completedRealFunction(_getFunction(_environment,"sin"),"sin",Msin))return false;
    if(!completedRealFunction(_getFunction(_environment,"tan"),"tan",Mtan))return false;
    if(!completedRealFunction(_getFunction(_environment,"cosh"),"cosh",Mcosh))return false;
    if(!completedRealFunction(_getFunction(_environment,"sinh"),"sinh",Msinh))return false;
    if(!completedRealFunction(_getFunction(_environment,"tanh"),"tanh",Mtanh))return false;
    if(!completedRealFunction(_getFunction(_environment,"sqrt"),"sqrt",Msqrt))return false;
    if(!completedRealFunction(_getFunction(_environment,"log"),"log",Mlog))return false;
    if(!completedRealFunction(_getFunction(_environment,"log10"),"log10",Mlog10))return false;
    if(!completedRealFunction(_getFunction(_environment,"floor"),"floor",Mfloor))return false;
    if(!completedRealFunction(_getFunction(_environment,"trunc"),"trunc",Mtrunc))return false;
    if(!completedRealFunction(_getFunction(_environment,"round"),"round",Mround))return false;
    if(!completedRealFunction(_getFunction(_environment,"ceil"),"ceil",Mceil))return false;
    if(!completedRealFunction(_getFunction(_environment,"exp"),"exp",Mexp))return false;

    if(!completedStringStringFunction(_getFunction(_environment,"settype"),"settype",Msettype))return false;
    if(!completedRealRealFunction(_getFunction(_environment,"pow"),"pow",Mpow))return false;

    if(!completedStringMapTokenFunction(_getFunction(_environment,"function"),DEFINEUSERFUNCTION_NAME,Mdefinefunction))return false;
    if(!completedValueFunction(_getFunction(_environment,"return"),"return",Mreturn))return false;

    return true;
}/* VALIDATED */

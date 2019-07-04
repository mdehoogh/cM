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
void free_functiondefinition(Mfunctiondefinition* _functiondefinition){
    if(_functiondefinition){
        if(_functiondefinition->_parameterMap)free_map(_functiondefinition->_parameterMap);
        if(_functiondefinition->_expressionlist)free_expressionlist(_functiondefinition->_expressionlist);
        free(_functiondefinition);
    }
}/* VALIDATED */
// NOTE typically you're not supposed to free internal functions safe M function definitions 
// TODO if a function map is freed, we shouldn't free internal functions BUT those are only present in the main environment which is never released!!
bool free_function(Mfunction* _function){
    if(_function){
        if(_function->type==FT_M){
            free_string(_function->_name);
            free_map(_function->_parameterMap);
            // TODO should we actually free the definitiona environment as it is a reference to an environment
            // DONE I guess not
            //////////free_environment(_function->_definitionEnvironment);
            free_functiondefinition(_function->functionunion._functiondefinition);
            free(_function);
            return true;
        }
    }
    return false;
}/* VALIDATED */
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement){
    if(_functionmapelement){
        if(free_functionmapelement(_functionmapelement->_next))_functionmapelement->_next=NULL;
        if(free_function(_functionmapelement->_function)){
            free(_functionmapelement);
            return true;
        }
    }
    return false;
}/* VALIDATED */
void free_functionmap(Mfunctionmap* _functionmap){
    if(_functionmap){
        free_functionmapelement(_functionmap->_first);
        free(_functionmap);
    }
}/* VALIDATED */
void free_environment(Menvironment* _environment){
    if(_environment){
        free_map(_environment->_variableMap);
        free_functionmap(_environment->_functionMap);
        free(_environment);
    }
}/* VALIDATED */
// END RELEASERS
// keep track of the current execution environment
static Menvironment* _executionEnvironment=NULL;
bool pushExecutionEnvironment(Menvironment* _environment){
    if(!_environment)return false;
    if(_environment->_parent)return false; // shouldn't have a parent!!!
    _environment->_parent=_executionEnvironment;
    _executionEnvironment=_environment;
    return true;
}/* VALIDATED */
bool popExecutionEnvironment(){
    // NOTE only execution environments that have a parent can be popped!!!
    Menvironment* _parentExecutionEnvironment=(_executionEnvironment?_executionEnvironment->_parent:NULL);
    if(!_parentExecutionEnvironment)return false;
    _executionEnvironment->_parent=NULL; // clear the parent of the current execution environment
    free_environment(_executionEnvironment); // TODO I guess we won't be needing this execution environment any more????
    _executionEnvironment=_parentExecutionEnvironment;
    return true;
}/* VALIDATED */

// read access to the elements defined in an environment
uint32_t getNumberOfVariables(const Menvironment* const _environment){
    return(_environment&&_environment->_variableMap?_environment->_variableMap->numberOfElements:0);
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
    if(!_environment||!name){outputError("No environment or name specified");return NULL;}
    // input valid        
    if(!_environment->_variableMap){outputError("No variables in environment");return NULL;}
    if(amVerbose())output("Looking for variable '%s'.\n",name);
    Mmapelement* _variableMapelement=_environment->_variableMap->_first;
    // as long as variable is defined, and the variable's name is not equal to the given name, continue
    while(_variableMapelement&&(!_variableMapelement->_variable||strcmp(_variableMapelement->_variable->_name,name)))_variableMapelement=_variableMapelement->_next;
    if(!_variableMapelement){
        if(amVerbose())output("Not found!\n");
        return NULL;
    }
    return _variableMapelement->_variable;
}/* VALIDATED */
bool containsVariable(const Menvironment* const _environment,const char* const name){return(getVariable(_environment,name,false)!=NULL);}/* VALIDATED */

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
                if(amDebugging()){Mstring* _valueText=_getValueText(_value,false);output("Value `%s` assigned to variable `%s`.",string(_valueText),name);free_string(_valueText);}
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
                        p=string_append(p,string(functionmapelement->_function->_name));
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
                if(functionmapelement->_function&&!strcmp(string(functionmapelement->_function->_name),functionName)){
                    //////////printf("\nFunction '%s' matches '%s'.",string(_functionmapelement->_function->_name),functionName);
                    return functionmapelement->_function;
                }
                functionmapelement=functionmapelement->_next;
            }
        }
    }
    return NULL;    
}/* VALIDATED */

Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList){
    Mmap* _functionArgumentMap=NULL;
    if(_function&&_argumentList){
        _functionArgumentMap=(Mmap*)CALLOC(1,sizeof(Mmap),'M');
        Mmap* functionParameterMap=_function->_parameterMap;
        if(functionParameterMap){
            if(amVerbose())output("Matching the function parameters!");
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
        if(amVerbose())output("Argument map created.");
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
                                if(amVerbose())output("Function '%s' registered as function #%d.\n",string(_function->_name),_functionmap->numberOfFunctions);
                            }else // failure
                                p=NULL;
                        }else
                            p=NULL;
                    }
                    if(!p)free_string(_functionName); // p==NULL indicates _functionName not bound in _function->_name
                    // try to append it to the functionMap, if we succeed store _functioName in ->_name
                 }else
                    output("%sFailed to store function name '%s'.\n",ERROR_PREFIX,name);
                // if we fail to register the name and/or the function with the environment free the function!!
                if(!_function->_name){free_function(_function);_function=NULL;}   
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
Mvalue* Msettype(const Mvalue* const _variableName,const Mvalue* const _valuetype){
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
Mvalue* Mneg(Mvalue* _value){ // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_REAL)return _getRealValue(-_value->value._real->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg),true);
    }
    return NULL;
}
Mvalue* Mnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot),true);
    }
    return NULL;
}
// TODO can we not a string??????
Mvalue* Mbnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot),true);
    }
    return NULL;
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
            case VT_INTEGER:case VT_BIGINTEGER:case VT_REAL:case VT_TEXT:result=1;break;
            case VT_LIST:result=_value->value._list->numberOfElements;break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            default:break;
        }
    }
    return _getIntegerValue(result);
}

// MDH@29MAY2019: how about forcing the result to be a big integer instead of a long double?????
extern const long double LD_PI;
Mvalue* Mfacd(Mvalue* _value){
    // Stirling formula to compute the number of factorial digits in n!: return 
    // get the integer out of the value
    long long ll=getValueInteger(_value);
    return (ll>0?_getIntegerValue(floor( ((ll+0.5)*log(ll) - ll + 0.5*log(2*LD_PI))/log(10) ) + 1):NULL);
}
Mvalue* Mfac(Mvalue* _value){
    if(!_value){if(amVerbose())output("No value!");return NULL;}
    if(amVerbose())outputValue("\nArgument of fac() function: '",_value,"'.");
    if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER){outputValue("\nERROR: Non-integer argument '",_value,"' to fac() function!");return NULL;}
    // some special cases (i.e. the input number is smaller than 2)
    Mbiginteger* finalmultiplier=NULL;
    if(_value->type==VT_INTEGER){
        if(_value->value._integer->ll<0){outputError("Invalid (negative integer) argument to fac() function");return NULL;}
        if(_value->value._integer->ll<3)return _getIntegerValue(_value->value._integer->ll);
        finalmultiplier=_getBiginteger(_value->value._integer->ll);
    }else{
        if(mp_isneg(_value->value._biginteger)){outputError("Invalid (negative integer) argument to fac() function");return NULL;}
        if(mp_cmp(_value->value._biginteger,getBigintegerThree())==MP_LT)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        finalmultiplier=_value->value._biginteger;
    }
    if(!finalmultiplier){output("%s",ERROR_PREFIX);outputValue("Failed to convert '",_value,"' to a big integer!\n");return NULL;}
    if(amVerbose()&&amDebugging())outputBiginteger("\nFinal multiplier: '",finalmultiplier,"'.");
    Mbiginteger* result=_getBiginteger(6); // the smallest value to return
    if(result){
        // we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
        // we could start at some intermediate value????
        Mbiginteger *multiplier=_getBiginteger(3);
        if(multiplier){
            while(mp_cmp(multiplier,finalmultiplier)==MP_LT){
                if(mp_incr(multiplier)!=MP_OKAY){if(amVerbose())outputError("Failed to increment big integer");result=NULL;break;} // if we fail to increment break
                if(mp_mul(result,multiplier,result)!=MP_OKAY){if(amVerbose())outputError("Failed to multiply a big integer by 6");result=NULL;break;}
                //////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
            }
            // get rid of intermediate big integers
            mp_clear(multiplier);
        }else        
            outputError("Failed to create big integer 3");
    }else
        outputError("Failed to create big integer 6");
    if(_value->type==VT_INTEGER)mp_clear(finalmultiplier);
    if(amVerbose())outputBiginteger("Result of applying the fac() function: '",result,"'.\n");
    return (result?_getBigintegerValue(result,true):NULL);
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

bool completedFunction(Mfunction* _function,NoArgumentFunction noArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_NO_ARGUMENTS;
        _function->functionunion.noArgumentFunction=noArgumentFunction;
        _function->_parameterMap=NULL;
        if(amVerbose())output("Registered function '%s' completed.\n",string(_function->_name));
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
        if(amVerbose())output("Registered function '%s' completed.\n",string(_function->_name));
        return true;
    }
    return false;
}
bool completedRealFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getRealMap("x",_getRealValue(M_LD_NAN)); // MDH@20JUN2019: now using the invalid real value as default (to indicate a missing value)
        if(amVerbose())output("Registered function '%s' completed.\n",string(_function->_name));
        return true;
    }
    return false;
}
bool completedIntegerFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getIntegerMap("i",_getIntegerValue(M_LL_INVALID)); // MDH@20JUN2019: now using the invalid value as default (to indicate a missing!!!!)
        if(amVerbose())output("Registered function '%s' completed.\n",string(_function->_name));
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
        if(amVerbose())output("Registered list function '%s' completed.\n",string(_function->_name));
        return true;
    }
    return false;
}
bool completedStringStringFunction(Mfunction* _function,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getStringStringMap("variable","type");
        if(amVerbose())output("Registered function '%s' completed.\n",string(_function->_name));
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

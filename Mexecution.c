#include "Mexecution.h"

// read access to the elements defined in an environment
uint32_t getNumberOfVariables(Menvironment* _environment){
    return(_environment?_environment->_variableMap->numberOfElements:0);
}
// the names of the variables may be requested
mstring* getVariableNames(const Menvironment* _environment,char* sep){
    if(_environment!=NULL&&sep!=NULL){
        mstring* variableNames=string_create();
        if(variableNames!=NULL){
            // first append the names of the variables in the parent
            if(_environment->_parent){
                mstring* parentVariableNames=getVariableNames(_environment->_parent,sep);
                if(parentVariableNames){
                    string_append(variableNames,string(parentVariableNames));
                    free(parentVariableNames); // don't keep it hanging around!!
                }
            }
            // we'll be appending the names of the variables in the environment itself
            Mmapelement* _variableMapelement=_environment->_variableMap->_first;
            while(_variableMapelement){
                if(strlen(sep))if(!string_empty(variableNames))string_append(variableNames,sep);
                string_append(variableNames,_variableMapelement->_variable->name);
                _variableMapelement=_variableMapelement->_next;
            }
            return variableNames;
        }
    }
    return NULL;
}

Mvariable* getVariable(Menvironment* _environment,char* name){
    if(_environment!=NULL&&name!=NULL&&strlen(name)>0){ // input valid        
        Mmapelement*_variableMapelement=_environment->_variableMap->_first;
        // as long as variable is defined, and the variable's name is not equal to the given name, continue
        while(_variableMapelement!=NULL&&strcmp(_variableMapelement->_variable->name,name))_variableMapelement=_variableMapelement->_next;
        if(_variableMapelement!=NULL)return _variableMapelement->_variable;
    }
    return NULL;
}

// write access
// helper function to create a new variable with a given name and of a given type
Mvariable* createVariable(char* name,enum Mvaluetype valueType){
    Mvariable* _variable=(Mvariable*)malloc(sizeof(Mvariable));
    if(_variable!=NULL){
        _variable->name=name;
        _variable->_value=NULL;
        if(valueType!=VT_UNDEFINED){
            _variable->_value=(Mvalue*)malloc(sizeof(Mvalue));
            if(_variable->_value!=NULL){
                _variable->_value->type=valueType;
                // I do NOT need to set the values of the atomic elements (integer, real), so basically these are not initialized to start with
                switch(valueType){
                    case VT_UNDEFINED:
                    case VT_INTEGER:
                    case VT_REAL:break;
                    case VT_STRING:_variable->_value->value._string=(Mstring*)calloc(1,sizeof(Mstring));break;
                    case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
                    case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
                }
            }else{ // failed to allocate memory for the value
                free(_variable);
                _variable=NULL;
            }
        }
    }
    return _variable;
}

// addVariable returns the value map element that was created (if successful)
Mvariable* addVariable(Menvironment* _environment,char* name,enum Mvaluetype valueType){
    Mvariable* _variable=NULL;
    if(_environment!=NULL&&name!=NULL&&strlen(name)>0){ // input valid   
        _variable=getVariable(_environment,name);
        if(!_variable){ // non-existing...
            _variable=createVariable(name,valueType);
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
                }else{
                    // ASSERT creating a map element to store the variable reference in failed, so free _variable
                    free(_variable);
                    _variable=NULL;
                }
            }
        }
    }
    ///////printf("\nConnected!");
    return _variable;
}

// let's start with some simple assignments
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer){
    if(_variable==NULL||_integer==NULL)return false;
    Mvalue* _variableValue=_variable->_value;
    // if the variable does not have a value associated with it, create one
    if(_variableValue==NULL){
        _variableValue=(Mvalue*)malloc(sizeof(Mvalue));
        if(!_variableValue)return false;
        _variableValue->type=VT_INTEGER;
        _variable->_value=_variableValue;
    }else // could be wrong type, so do not store variable
    if(_variableValue->type!=VT_INTEGER)return false; // wrong type!!!
    _variableValue->value._integer=_integer;
    return true;
}
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real){
    if(_variable==NULL||_real==NULL)return false;
    Mvalue* _variableValue=_variable->_value;
    // if the variable does not have a value associated with it, create one
    if(_variableValue==NULL){
        _variableValue=(Mvalue*)malloc(sizeof(Mvalue));
        if(!_variableValue)return false;
        _variableValue->type=VT_REAL;
        _variable->_value=_variableValue;
    }else // could be wrong type, so do not store variable
    if(_variableValue->type!=VT_REAL)return false; // wrong type!!!
    _variableValue->value._real=_real;
    return true;
}
bool setValueOfStringVariable(Mvariable* _variable,Mstring* _string){
    if(_variable==NULL||_string==NULL)return false;
    Mvalue* _variableValue=_variable->_value;
    // if the variable does not have a value associated with it, create one
    if(_variableValue==NULL){
        _variableValue=(Mvalue*)malloc(sizeof(Mvalue));
        if(!_variableValue)return false;
        _variableValue->type=VT_STRING;
        _variable->_value=_variableValue;
    }else // could be wrong type, so do not store variable
    if(_variableValue->type!=VT_STRING)return false; // wrong type!!!
    _variableValue->value._string=_string;
    return true;
}
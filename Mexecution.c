#include "Mexecution.h"

uint32_t getNumberOfVariables(Menvironment* environment){
    return(environment?environment->variableMap->numberOfElements:0);
}

// helper function to create a new variable with a given name and of a given type
Mmapelement* createVariable(char* name,enum Mvaluetype valueType){
    Mmapelement* variable=(Mmapelement*)malloc(sizeof(Mmapelement));
    if(variable){
        variable->name=name;
        variable->mValue=(Mvalue*)malloc(sizeof(Mvalue));
        if(variable->mValue){
            variable->next=NULL;
            variable->mValue->type=valueType;
            // I do NOT need to set the values of the atomic elements (integer, real), so basically these are not initialized to start with
            switch(valueType){
                case VT_INTEGER:break;
                case VT_REAL:break;
                case VT_STRING:variable->mValue->value.s=(Mstring*)malloc(sizeof(Mstring));break;
                case VT_LIST:variable->mValue->value.l=(Mlist*)malloc(sizeof(Mlist));break;
                case VT_MAP:variable->mValue->value.m=(Mmap*)malloc(sizeof(Mmap));break;
          }
        }else{ // failed to allocate memory for the value
            free(variable);
            variable=NULL;
        }
    }
    return variable;
}
// addVariable returns the value map element that was created (if successful)
Mmapelement* addVariable(Menvironment* environment,char* name,enum Mvaluetype valueType){
    if(!environment)return false;
    if(!name)return false;
    uint32_t numberOfVariables=getNumberOfVariables(environment);
    ////////printf("\nNumber of variables: %d.",numberOfVariables);
    Mmapelement* variable;
    if(numberOfVariables){
        // a variable with the given name might already exist
        variable=environment->variableMap->first;
        // as long as variable is defined, and the variable's name is not equal to the given name, continue
        while(variable&&strcmp(variable->name,name))variable=variable->next;
        if(variable)return variable; // already exists!!
    }
    variable=createVariable(name,valueType);
    //////printf("\nVariable created!");
    if(variable){
        Mmapelement* lastVariable=environment->variableMap->last;
        if(lastVariable)lastVariable->next=variable;else environment->variableMap->first=variable;
        environment->variableMap->last=variable;
        environment->variableMap->numberOfElements=numberOfVariables+1;
    }
    ///////printf("\nConnected!");
    return(getNumberOfVariables(environment)>numberOfVariables?variable:NULL);
}

// let's start with some simple assignments
bool setValueOfIntegerVariable(Mmapelement* pVariable,Minteger* pMinteger){
    if(!pVariable||!pVariable->mValue)return false;
    if(pVariable->mValue->type!=VT_INTEGER)return false; // wrong type!!!
    pVariable->mValue->value.i=pMinteger;
    return true;
}
bool setValueOfRealVariable(Mmapelement* pVariable,Mreal* pMreal){
    if(!pVariable||!pVariable->mValue)return false;
    if(pVariable->mValue->type!=VT_REAL)return false; // wrong type!!!
    pVariable->mValue->value.r=pMreal;
    return true;
}
// the names of the variables may be requested
mstring* getVariableNames(const Menvironment* environment,char* sep){
    if(environment&&sep){
        mstring* variableNames=string_create();
        if(variableNames){
            // first append the names of the variables in the parent
            if(environment->parent){
                mstring* parentVariableNames=getVariableNames(environment->parent,sep);
                if(parentVariableNames){
                    string_append(variableNames,string(parentVariableNames));
                    free(parentVariableNames); // don't keep it hanging around!!
                }
            }
            // we'll be appending the names of the variables in the environment itself
            Mmapelement* variable=environment->variableMap->first;
            while(variable){
                if(strlen(sep))if(!string_empty(variableNames))string_append(variableNames,sep);
                string_append(variableNames,variable->name);
                variable=variable->next;
            }
            return variableNames;
        }
    }
    return NULL;
}

#include "Mexecution.h"

uint32_t getNumberOfVariables(Menvironment* environment){
    return(environment?environment->variableMap->numberOfValues:0);
}

// helper function to create a new variable with a given name and of a given type
Mvaluemapelement* createVariable(char* name,enum Mvaluetype valueType){
    Mvaluemapelement* variable=(Mvaluemapelement*)malloc(sizeof(Mvaluemapelement));
    if(variable){
        variable->name=name;
        variable->mValue=(Mvalue*)malloc(sizeof(Mvalue));
        if(variable->mValue){
            variable->next=NULL;
            variable->mValue->type=valueType;
            switch(valueType){
                case VT_NUMBER:variable->mValue->value.n=(Mnumber*)malloc(sizeof(Mnumber));break;
                case VT_STRING:variable->mValue->value.s=(mstring*)malloc(sizeof(mstring));break;
                case VT_LIST:variable->mValue->value.vl=(Mvaluelist*)malloc(sizeof(Mvaluelist));break;
               case VT_MAP:variable->mValue->value.vm=(Mvaluemap*)malloc(sizeof(Mvaluemap));break;
            }
        }else{ // failed to allocate memory for the value
           free(variable);
            variable=NULL;
        }
    }
    return variable;
}
bool addVariable(Menvironment* environment,char* name,enum Mvaluetype valueType){
    if(!environment)return false;
    if(!name)return false;
    uint32_t numberOfVariables=getNumberOfVariables(environment);
    printf("\nNumber of variables: %d.",numberOfVariables);
    Mvaluemapelement* variable;
    if(numberOfVariables){
        // a variable with the given name might already exist
        variable=environment->variableMap->first;
        // as long as variable is defined, and the variable's name is not equal to the given name, continue
        while(variable&&strcmp(variable->name,name))variable=variable->next;
        if(variable)return false; // already exists!!
    }
    variable=createVariable(name,valueType);
    printf("\nVariable created!");
    if(variable){
        Mvaluemapelement* lastVariable=environment->variableMap->last;
        if(lastVariable)lastVariable->next=variable;else environment->variableMap->first=variable;
        environment->variableMap->last=variable;
        environment->variableMap->numberOfValues=numberOfVariables+1;
    }
    printf("\nConnected!");
    return(getNumberOfVariables(environment)>numberOfVariables);
}

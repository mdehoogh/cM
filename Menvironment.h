/**
 * MDH@24JUN2019: everything that deals with execution of functions (in their own execution environment)
 */
#include "Mvalue.h"

 // functions of different types, internal (no body but a function to pass the arguments to) or external (with a body)
// all functions are executed in an execution environment, that descends from the environment in which the function is defined (the definition environment)
// MDH@28MAY2019: how about NOT passing the execution environment and instead keep a current execution environment instead (in case one needs it)
/////////////struct Menvironment;
typedef Mvalue* (*NoArgumentFunction)();
typedef Mvalue* (*OneArgumentFunction)(Mvalue* _argumentValue);
typedef Mvalue* (*TwoArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value);

typedef enum Mfunctiontype{FT_M,FT_INTERNAL_NO_ARGUMENTS,FT_INTERNAL_ONE_ARGUMENT,FT_INTERNAL_TWO_ARGUMENTS}Mfunctiontype;

typedef struct Mfunctiondefinition{
    Mmap* _parameterMap;
    Mexpressionlist* _expressionlist;
}Mfunctiondefinition;

typedef union Mfunctionunion{
    NoArgumentFunction noArgumentFunction;
    OneArgumentFunction oneArgumentFunction;
    TwoArgumentFunction twoArgumentFunction;
    Mfunctiondefinition* _functiondefinition; // a list of expressions to evaluate that use the parameters (and have defaults, and an environment)
}Mfunctionunion;

struct Menvironment;
typedef struct Mfunction{
    Mstring* _name;
    Mmap* _parameterMap; // a map of values defines the parameters and their default values (implicitly defining the expected types)
    struct Menvironment* _definitionEnvironment;
    Mfunctiontype type; // whether internal or external
    Mfunctionunion functionunion; // where either the internal function to call with the arguments is placed or 
}Mfunction;

typedef struct Mfunctionmapelement{
    Mfunction* _function;
    struct Mfunctionmapelement* _next;
}Mfunctionmapelement;

typedef struct MfunctionMap{
    uint32_t numberOfFunctions;  // keeping track of the total number of functions...
    Mfunctionmapelement* _first;
    Mfunctionmapelement* _last;
}Mfunctionmap;

//Mvalue* getFunction(Mfunctionlist functionlist,char* name);

// environments

// an environment is a bag of variables and functions
typedef struct Menvironment{
    Mmap* _variableMap; // variables are stored by name
    Mfunctionmap* _functionMap; // this would be the map of M functions defined in this environment (i.e. not the C functions/constants)
    struct Menvironment* _parent;
}Menvironment;

Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction);
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction);

bool pushExecutionEnvironment(Menvironment* _environment);
bool popExecutionEnvironment();

// MDH@01MAY2019: it's possible to somehow hide the structure pointers within an Menvironment that point to the variables and functions
//                which basically means that only raw data should go in and out of public functions

// function prototypes
// read access
uint32_t getNumberOfVariables(Menvironment* _environment);
Mstring* _getVariableNames(const Menvironment* _environment,char* sep); // NOTE the _ indicates that the caller should free whatever is returned!!!
/* replacing:
Mvariable* getNewVariable(Menvironment* _environment,const char* name);
Mvariable* getVariable(Menvironment* _environment,const char* name);
*/
bool containsVariable(Menvironment* _environment,const char* name);
Mvaluetype getVariableType(Menvironment* _environment,const char* name); // the type of a variable can be fixed (only values of this type can be assigned to it) or unfixed (any value can be assigned to it)
Mvaluetype getVariableValueType(Menvironment* _environment,const char* name); // same as getVariableType() if a type is defined for the given variable

// write access
bool setVariableType(Menvironment* _environment,const char* name,Mvaluetype valuetype); // NOTE changing the type is dangerous as it will clear the value if the value is not of the right type
// create a value of a certain value type initialized with either value NULL (atomic values) or an empty list or map (VT_LIST,VT_MAP)
// NOTE when using VT_UNDEFINED, the value remains NULL but any value can be stored in it subsequently
bool createVariable(Menvironment* _environment,const char* name,Mvaluetype valuetype);

long long appendToListVariable(Menvironment* _environment,const char* name,Mvalue* _value);

/*
Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue);
*/
// once you've created an Mvalue with one of the above new... functions you can link it to a variable with a given name, if unsuccessful you have to release the value yourself!!!!
// NOTE this is possible when _value is not allowed or the variable does not exists, anyway if the assignment succeeds true should be returned false otherwise
// decided to allow asking for a value of a given type that always owns what it contains (Minteger, Mreal, Mtext, Mlist or Mmap pointer)
bool setValue(Menvironment* _environment,const char* name,Mvalue* _value);
Mvalue* getValue(Menvironment* _environment,const char* name);

bool addVariable(Menvironment* _environment,const char* name,Mvaluetype valuetype,bool immutable);
/*
// if you want to set a value you have to pass in a pointer to the contents
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real);
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer);
bool setValueOfStringVariable(Mvariable* _variable,Mtext* _string);
*/

// functions
Mstring* _getFunctionNames(const Menvironment* _environment,char* sep);
Mfunction* newFunction(Menvironment* _environment,const char* functionName);
bool registerInternalFunctions(Menvironment* _environment);
// helper function to return the function
Mfunction* getFunction(Menvironment* _environment,const char* functionName);
// MDH@21MAY2019: the _ indicates that the caller has to free the map itself
Mmap* _getFunctionArgumentMap(Mfunction* _function,Mlist* _argumentList);

bool completedFunction(Mfunction* _function,NoArgumentFunction noArgumentFunction);
bool completedValueFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction);
bool completedIntegerFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction);
bool completedRealFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction);
bool completedStringStringFunction(Mfunction* _function,TwoArgumentFunction twoArgumentFunction);
bool completedListFunction(Mfunction* _function,OneArgumentFunction oneArgumentFunction);

Mvalue* Mfacd(Mvalue* _value);
Mvalue* Mfac(Mvalue* _value);
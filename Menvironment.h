/**
 * MDH@24JUN2019: everything that deals with execution of functions (in their own execution environment)
 */
#include "Mvalue.h"

 // functions of different types, internal (no body but a function to pass the arguments to) or external (with a body)
// all functions are executed in an execution environment, that descends from the environment in which the function is defined (the definition environment)
// MDH@28MAY2019: how about NOT passing the execution environment and instead keep a current execution environment instead (in case one needs it)
/////////////struct Menvironment;

typedef enum Mfunctiontype{FT_USER,FT_INTERNAL_NO_ARGUMENTS,FT_INTERNAL_ONE_ARGUMENT,FT_INTERNAL_TWO_ARGUMENTS,FT_INTERNAL_THREE_ARGUMENTS}Mfunctiontype;

typedef union Mfunctionunion{
    NoArgumentFunction noArgumentFunction;
    OneArgumentFunction oneArgumentFunction;
    TwoArgumentFunction twoArgumentFunction;
    ThreeArgumentFunction threeArgumentFunction;
    Muserfunction* _userfunction; // a list of expressions to evaluate that use the parameters (and have defaults, and an environment)
}Mfunctionunion;

struct Menvironment;
typedef struct Mfunction{
    ////////Mstring* _name;
    Mmap* _parameterMap; // a map of values defines the parameters and their default values (implicitly defining the expected types)
    struct Menvironment* _definitionEnvironment;
    Mfunctiontype type; // whether internal or external
    Mfunctionunion functionunion; // where either the internal function to call with the arguments is placed or 
}Mfunction;

typedef struct Mfunctionmapelement{
    Mstring* _name;
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
    char* _name; // the name of the environment
    Mmap* _variableMap; // variables are stored by name
    Mfunctionmap* _functionMap; // this would be the map of M functions defined in this environment (i.e. not the C functions/constants)
    Mtoken* expressionToken; // MDH@17JUL2019: the current token of the expression being evaluated in this environment
    struct Menvironment* _parent;
}Menvironment;

Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction);
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction);

Menvironment* __environment(); // creates a new (empty) environment
void free_environment(Menvironment* _environment);
bool pushExecutionEnvironment(Menvironment* _environment);
void popExecutionEnvironment(); // should never go wrong (a bug is reported if there's no environment to pop though)
Menvironment* getEnvironment(); // the current environment
Mstring* _getEnvironmentName(); // for use in prompting

// MDH@17JUL2019: getting and updating the environment expression token
Mtoken* getEnvironmentExpressionToken();
Mtoken* nextEnvironmentExpressionToken();

unsigned long long getNumberOfFunctionCommands(const char* const functionName);
bool registerFunctionCommand(const char* const functionName,Mtoken* command);

// MDH@01MAY2019: it's possible to somehow hide the structure pointers within an Menvironment that point to the variables and functions
//                which basically means that only raw data should go in and out of public functions

// function prototypes
// read access
uint32_t getNumberOfVariables(const Menvironment* const _environment);
Mstring* _getVariableNames(const Menvironment* const _environment,const char* const sep); // NOTE the _ indicates that the caller should free whatever is returned!!!
/* replacing:
Mvariable* getNewVariable(Menvironment* _environment,const char* name);
Mvariable* getVariable(Menvironment* _environment,const char* name);
*/
bool containsVariable(const Menvironment* const _environment,const char* const name);
Mvaluetype getVariableType(const Menvironment* const _environment,const char* const name); // the type of a variable can be fixed (only values of this type can be assigned to it) or unfixed (any value can be assigned to it)
Mvaluetype getVariableValueType(const Menvironment* const _environment,const char* const name); // same as getVariableType() if a type is defined for the given variable

// write access
bool setVariableType(const Menvironment* const _environment,const char* const name,Mvaluetype valuetype); // NOTE changing the type is dangerous as it will clear the value if the value is not of the right type
// create a value of a certain value type initialized with either value NULL (atomic values) or an empty list or map (VT_LIST,VT_MAP)
// NOTE when using VT_UNDEFINED, the value remains NULL but any value can be stored in it subsequently
bool createVariable(Menvironment* const _environment,const char* const name,Mvaluetype valuetype);

long long appendToListVariable(const Menvironment* const _environment,const char* const name,const Mvalue* const _value);

/*
Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue);
*/
// once you've created an Mvalue with one of the above new... functions you can link it to a variable with a given name, if unsuccessful you have to release the value yourself!!!!
// NOTE this is possible when _value is not allowed or the variable does not exists, anyway if the assignment succeeds true should be returned false otherwise
// decided to allow asking for a value of a given type that always owns what it contains (Minteger, Mreal, Mtext, Mlist or Mmap pointer)
bool setValue(const Menvironment* const _environment,const char* const name,const Mvalue* const _value);
Mvalue* getValue(const Menvironment* const _environment,const char* const name);

bool addVariable(Menvironment* const _environment,const char* const name,Mvaluetype valuetype,bool immutable);
/*
// if you want to set a value you have to pass in a pointer to the contents
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real);
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer);
bool setValueOfStringVariable(Mvariable* _variable,Mtext* _string);
*/

// functions
Mstring* _getFunctionNames(const Menvironment* const _environment,const char* const sep);
bool registerInternalFunctions(Menvironment* const _environment);

// helper function to return the function
Mfunction* getFunction(const Menvironment* const _environment,const char* const functionName);
Muserfunction* getUserfunction(const Menvironment* const _environment,const char* const userfunctionName);

// MDH@21MAY2019: the _ indicates that the caller has to free the map itself
Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList);
Mfunction* _getFunction(Menvironment* const _environment,const char* const functionName); // creates the function if it does not exist yet

bool completedFunction(Mfunction* const _function,char* functionName,NoArgumentFunction noArgumentFunction);
bool completedValueFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction);
bool completedIntegerFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction);
bool completedRealFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction);
bool completedListFunction(Mfunction* const _function,char* functionName,OneArgumentFunction oneArgumentFunction);

bool completedStringStringFunction(Mfunction* const _function,char* functionName,TwoArgumentFunction twoArgumentFunction);
bool completedRealRealFunction(Mfunction* const _function,char* functionName,TwoArgumentFunction twoArgumentFunction);

// MDH@09JUL2019: a user function is defined as a two-parameter function containing the parameter map and a body (list)
bool completedStringMapTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction twoArgumentFunction);
Mvalue* Mdefinefunction(Mvalue* _name,Mvalue* _parameterMap,Mvalue* _body);
//                the return function returns its value as result of the function it is executing
Mvalue* Mreturn(Mvalue* _value);
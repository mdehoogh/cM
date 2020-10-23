/**
 * MDH@24JUN2019: everything that deals with execution of functions (in their own execution environment)
 */
#include "Mfunctions.h"

// MDH@03FEB2020: every 'execution environment' is effectively a stack of (wrapped) environments
//                therefore an executing environment remember the previous executing environment to return to once it is no longer executing
//                but an executing environment has it's own parent executing environment

// definitions of Mfunction and Menvironment moved over to the end of Mvalue.h as we use it there as well
bool pushExecutionEnvironment(Menvironment* _environment);
void popExecutionEnvironment(); // should never go wrong (a bug is reported if there's no environment to pop though)
Menvironment* getExecutionEnvironment(); // the current environment
Mallocationowner getOwnerExecutionEnvironment(); // MDH@31MAY2020
Mstring* _getExecutionEnvironmentName();
void outputExecutionEnvironmentName(char* prefix,char* suffix);

// MDH@17JUL2019: getting and updating the environment expression token
Mtoken* getEnvironmentExpressionToken();
Mtoken* nextEnvironmentExpressionToken();

unsigned long long getNumberOfFunctionCommands(char const * const functionName);
// bool registerFunctionCommand(char const * const functionName,Mtoken* _command,Mallocationowner owner_command);

// MDH@01MAY2019: it's possible to somehow hide the structure pointers within an Menvironment that point to the variables and functions
//                which basically means that only raw data should go in and out of public functions

// function prototypes
// read access
uint32_t getNumberOfVariables(Menvironment const * const _environment);
Mstring* _getVariableNames(Menvironment const * const _environment,char const * const sep); // NOTE the _ indicates that the caller should free whatever is returned!!!
/* replacing:
Mvariable* getNewVariable(Menvironment* _environment,const char* name);
*/
Mvariable* getVariable(Menvironment const * const _environment,char /*const*/ * const name,bool verbose); // we need to be able to do this in M.c MDH@10MAR2020: name characters are alterable so we do not need to create a dynamic copy of it
Mvalue* Mexists(Mvalue* _value);
// int8_t containsVariable(const Menvironment* const _environment,char /*const*/ * const name, int8_t report);
Mvaluetype getVariableType(const Menvironment* const _environment,const char* const name); // the type of a variable can be fixed (only values of this type can be assigned to it) or unfixed (any value can be assigned to it)
Mvaluetype getVariableValueType(const Menvironment* const _environment,const char* const name); // same as getVariableType() if a type is defined for the given variable

Mstring* _getCompletion(const char* const name,bool functionidentifiersaswell); // returns the remainder of variables/functions shared by all variables/functions that start with name, if not shared by all variables/functions all first continuation characters are returned

// write access
bool setVariableType(Menvironment const * const _environment,char const * const name,Mvaluetype valuetype); // NOTE changing the type is dangerous as it will clear the value if the value is not of the right type
// create a value of a certain value type initialized with either value NULL (atomic values) or an empty list or map (VT_LIST,VT_MAP)
// NOTE when using VT_UNDEFINED, the value remains NULL but any value can be stored in it subsequently
bool createVariable(Menvironment * const _environment,char const * const name,Mvaluetype valuetype);

long long appendToListVariable(Menvironment const * const _environment,char const * const name,Mvalue const * const _value);

/*
Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue);
*/
// once you've created an Mvalue with one of the above new... functions you can link it to a variable with a given name, if unsuccessful you have to release the value yourself!!!!
// NOTE this is possible when _value is not allowed or the variable does not exists, anyway if the assignment succeeds true should be returned false otherwise
// decided to allow asking for a value of a given type that always owns what it contains (Minteger, Mfloat, Mtext, Mlist or Mmap pointer)
bool setValue(const Menvironment* const _environment,char /*const*/ * const name,const Mvalue* const _value);

// MDH@14NOV2019: same as setValue but does not use assignValue (which will copy the value passed in)
bool setVariable(Menvironment * const _environment,char /*const*/ * const name,Mvalue const * const _value);

Mvalue* getValue(Menvironment const * const _environment,char /*const*/ * const name);
// MDH@26MAR2020: if we need the address of the value pointer
Mvalue** getValueHolder(Menvironment const * const _environment,char /*const*/ * const name);

char* getConstantWithValue(Menvironment const * const environment,char * name,Mvalue* value); // MDH@24OCT2019: if we want to find a constant with the same value we can use that as a 'symbol'
// MDH@24OCT2019: if we want to see the variables in an environment vall getVariableMapText(), which will also represent values by the names of constants with the same value (representing symbols)
Mstring* _getVariableMapText(Menvironment const * const environment,bool showcurlybraces,bool showquotes,bool showmissings,bool showhiddenfiles);

Mmap* _getVariableNamesMap(Menvironment const * const environment); // MDH@14NOV2019: returns a map with the names of all local variables (in attribute '') and the names of the variables in the parent environment with the name of the parent environment!

void outputTable(Mlist* table); // MDH@25NOV2019: certain lists are now constructed and recognized as 'tables'

Mmap* _getValuesMap(Mvalue* variableNamesMapValue);
Mlist* _getValuesTable(Mvalue* variableNamesMapValue); // MDH@25NOV2019: storing the memory allocations in a table makes it more displayable

bool addVariable(Menvironment * const _environment,Mallocationowner owner_environment,char * const name,Mvaluetype valuetype,bool immutable);
/*
// if you want to set a value you have to pass in a pointer to the contents
bool setValueOfRealVariable(Mvariable* _variable,Mfloat* _real);
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer);
bool setValueOfStringVariable(Mvariable* _variable,Mtext* _string);
*/

// functions
Mstring* _getFunctionNames(Menvironment const * const _environment,const char* const sep);
bool registerInternalFunctions(Menvironment* const _environment,Mallocationowner owner_environment);

// helper function to return the function
Mfunction* getFunction(Menvironment const * const _environment,const char* const functionName);
Muserfunction* getUserfunction(Menvironment const * const _environment,const char* const userfunctionName);

// MDH@21MAY2019: the _ indicates that the caller has to free the map itself
Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList,Mallocationowner owner_functionargumentmap);

Mfunction* _getFunction(Menvironment* const _environment,Mallocationowner owner_environment,char const * const name); // creates the function if it does not exist yet

bool completedFunction(Mfunction* const _function,const char* const functionName,NoArgumentFunction noArgumentFunction);
bool completedValueFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction);
bool completedIntegerFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction);
bool completedFloatFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction);
bool completedListFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction);
bool completedTokenListFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction);

bool completedStringStringFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);
bool completedFloatFloatFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);

bool completedTokenTokenFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);
bool completedValueValueFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);
bool completedListValueFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);
bool completedIntegerBooleanFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction);

// MDH@09JUL2019: a user function is defined as a two-parameter function containing the parameter map and a body (list)
bool completedStringMapTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction);
bool completedValueTokenTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction);
bool completedValueValueValueFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction);

bool completedTokenTokenTokenTokenFunction(Mfunction* const _function,const char* const functionName,FourArgumentFunction fourArgumentFunction);
bool completedTokenTokenTokenTokenTokenFunction(Mfunction* const _function,const char* const functionName,FiveArgumentFunction fiveArgumentFunction);

bool completedListValueIntegerFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction);

Mvalue* Mdefinefunction(Mvalue* _name,Mvalue* _parameterMap,Mvalue* _body);
//                the return function returns its value as result of the function it is executing
Mvalue* Mreturn(Mvalue* _value);

// MDH@23OCT2020: a get and set function might come in handy
Mvalue* Mset(Mvalue* _variableNameValue,Mvalue* _value);
Mvalue* Mget(Mvalue* _variableNameValue);
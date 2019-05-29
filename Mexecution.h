/**
 * MDH@01MAY2019:
 * - we need rules of 'engagement': 
 *   . all dynamic memory management of structure pointers defined here has to be done inside Mexecution.c, so only raw data should be provided to the methods
 *   . external parties can use the structures but should not change them
 *   . if M needs a list to store stuff in it should request value of that type, than it can use that to populate the list, but NEVER ever change the list itself i.e. treat a value as immutable
 *     this way free_value() can always free the integer, real, string, list or map it contains
 * 
 * MDH@29APR2019:
 * - every struct that allocates dynamic memory needs to have an explicit free function, otherwise you could free a pointer to it without freeing internal pointers
 * 
 * MDH@25APR2019:
 * - all M<something>* fields will be called _<something> so _ is like short for pointer to M<something>
 *
 * MDH@18APR2019:
 * - Any command executes inside an environment in which variables and functions are defined
 *   A variable is an identifier and an associated (literal) value!
 *   Because any value can also be a list, it might be a good idea to store a single value as a list of one element
 *   Functions have an associated identifier (different from the names of variables) that can be executed on a list of argument values
 *   The pointer to the name of the variable/function does not need to be 
 */
#include <math.h>
#include <float.h>

// for heap_string_copy() to copy char* stuff
#include "Mmemory.h"

// Token and Mexpression is provided in Mexpression.h
#include "Mexpression.h"

// defining VALUE_TYPES as an enum defining all possible value types
// VT_UNDEFINED indicates that no value is currently to be associated
typedef enum Mvaluetype {VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_REAL,VT_STRING,VT_LIST,VT_MAP}Mvaluetype;

// we define the names of 'standard' function but it is a good idea to classify them by the number of arguments

// TODO Minteger could become a union if we're storing multiple types of integers in it
typedef struct Minteger{
    long long ll; // signed 64-bit integer (for now)
}Minteger;

// TODO Mreal could become a union if we're storing multiple types of reals in it
typedef struct Mreal{
    long double ld; // double precision floating point binary number (for now)
}Mreal;

typedef struct Mstring{
    char presuffix;
    char _c[]; // by using an array and not a pointer, it's easy to make one out of an mstring* by strcpy from string(mstring)
}Mstring;

struct Mlist;
struct Mmap;
typedef union Mvalueunion{
    Mtoken* _token;
    Minteger* _integer;
    Mreal* _real;
    Mstring* _string;
    struct Mlist* _list;
    struct Mmap* _map;
}Mvalueunion;

// a Value is either a number (numeric literal), a string literal, a list of values or a map
// you could say that a map is a list of variables, as such Menvironment holds a map of variables and a map of functions
// and we could make a separate struct to hold a map
typedef struct Mvalue{
    size_t count; // keep track of the number of reference
    Mvaluetype type;
    Mvalueunion value;
}Mvalue;

typedef struct Mlistelement{
    unsigned long long index; // MDH@03MAY2019: keep track of the index in the list of this list element
    Mvalue* _value;
    struct Mlistelement* _next;
}Mlistelement;

// MDH@03MAY2019: we're going to allow a list to be sparse i.e. with each element we keep an offset
//                this may come in handy if we fail to store something in a list
typedef struct Mlist{
    unsigned long long numberOfElements; // keep track of the total number of elements
    Mvaluetype valuetype; // we can force a list to have elements of the same type
    Mlistelement* _first;
    Mlistelement* _last;
}Mlist;

typedef struct Mvariable{
    char* _name;
    bool immutable; // whether or not mutable
    Mvaluetype valuetype; // MDH@01MAY2019: fixed type variables can only be assigned once, after that any value that is assigned to it has to have the same type as the first value
    Mvalue* _value;
}Mvariable;

typedef struct Mmapelement{
    Mvariable* _variable;
    struct Mmapelement* _next;
}Mmapelement;

typedef struct Mmap{
    unsigned long long numberOfElements; // keep track of the total number of variables
    Mvaluetype valuetype; // the type all values in the map should have
    Mmapelement* _first;
    Mmapelement* _last;
}Mmap;

//Mvalue* getVariableValue(Mvariablelist variablelist,char* name);

typedef struct Mexpressionlistelement{
    char* binop; // the binary operator to apply to the operands
    Mvalue* _value; // the second operand
    struct Mexpressionlistelement* _next;
}Mexpressionlistelement;

typedef struct Mexpressionlist{
    Mvalue* _value; // the first value in the expression list
    Mexpressionlistelement* _next;
}Mexpressionlist;

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
    mstring* _name;
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

bool pushExecutionEnvironment(Menvironment* _environment);
bool popExecutionEnvironment();

////////Menvironment* getExecutionEnvironment();

// MDH@20MAY2019: we need free_map to free the function argument maps!!
void free_map(Mmap* _map);
/* MDH@01MAY2019: we do not want helper functions to free structure pointers visible to the outside
// pointer to these structs releasers
void free_string(Mstring* _string);
void free_value(Mvalue* _value);
void free_listelement(Mlistelement* listelement);
void free_list(Mlist* _list);
void free_variable(Mvariable* _variable);
void free_mapelement(Mmapelement* _mapelement);
void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement);
void free_expressionlist(Mexpressionlist* _expressionlist);
void free_functiondefinition(Mfunctiondefinition* _functiondefinition);
// NOTE typically you're not supposed to free internal functions safe M function definitions 
bool free_function(Mfunction* _function);
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement);
void free_functionmap(Mfunctionmap* _functionmap);
void free_environment(Menvironment* _environment);

// helper function
Mreal* get_real(long double ld);

Mvalue* getRealValue(Mreal* _real);
Minteger* get_integer(long long ll);
Mvalue* getIntegerValue(Minteger* _integer);
// mstring* is assumed to start with the same prefix/suffix character
Mstring* get_string(mstring* s);
Mvalue* getStringValue(Mstring* _string);
*/

// MDH@01MAY2019: it's possible to somehow hide the structure pointers within an Menvironment that point to the variables and functions
//                which basically means that only raw data should go in and out of public functions

// function prototypes
// read access
uint32_t getNumberOfVariables(Menvironment* _environment);
mstring* _getVariableNames(const Menvironment* _environment,char* sep); // NOTE the _ indicates that the caller should free whatever is returned!!!
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

// anybody can ask for a specific type of value (wrapping certain contents) and the pointer in it should be considered immutable i.e. Mvalue itself should be considered immutable
// NOTE this doesn't mean that 
Mvalue* _getUndefinedValue(); // it's also possible to ask for an undefined value!!!
Mvalue* _getIntegerValue(long long ll);
Mvalue* _getRealValue(long double ld);
Mvalue* _getStringValue(char* text);
Mvalue* _getListValue(Mvaluetype listValuetype); // returning an empty list with all values to be of type listValuetype
Mvalue* _getMapValue(Mvaluetype mapValuetype); // returning an empty map with all values to be of type mapValuetype
//////Mvalue* _getTokenValue(char* text);

Mvalue* _getValueOfList(Mlist* _list);
Mvalue* _getValueOfInteger(Minteger* _integer);
Mvalue* _getValueOfReal(Mreal* _real);
Mvalue* _getValueOfMap(Mmap* _map);
Mvalue* _getValueOfToken(Mtoken* _token);

Mlist* _getListOfType(Mvaluetype valuetype);

// MDH@02MAY2019: not allowed to call free_value from the outside
bool decrementReferenceCount(Mvalue* _value);
bool incrementReferenceCount(Mvalue* _value);

//////void free_value(Mvalue* _value);

unsigned long long appendedToList(Mlist* const _list,Mvalue* const _value,long long index); // helper function to append to a list with a certain index (possibly undefined), index must not be negative, if zero first available index will be used, otherwise it should be at least the first available index!!
bool appendedToMap(Mmap* _map,const char* attributeName,Mvalue* _attributeValue);

Mvalue* getValueAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName);

long long appendToListVariable(Menvironment* _environment,const char* name,Mvalue* _value);

/*
Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue);
*/
// once you've created an Mvalue with one of the above new... functions you can link it to a variable with a given name, if unsuccessful you have to release the value yourself!!!!
// NOTE this is possible when _value is not allowed or the variable does not exists, anyway if the assignment succeeds true should be returned false otherwise
// decided to allow asking for a value of a given type that always owns what it contains (Minteger, Mreal, Mstring, Mlist or Mmap pointer)
bool setValue(Menvironment* _environment,const char* name,Mvalue* _value);
Mvalue* getValue(Menvironment* _environment,const char* name);

bool addVariable(Menvironment* _environment,const char* name,Mvaluetype valuetype,bool immutable);
/*
// if you want to set a value you have to pass in a pointer to the contents
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real);
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer);
bool setValueOfStringVariable(Mvariable* _variable,Mstring* _string);
*/

// functions
mstring* _getFunctionNames(const Menvironment* _environment,char* sep);
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

size_t getNumberOfRemovedValues();
unsigned long long getNumberOfValues();

// some helper functions (TODO or should we use this on Mreal values?????)
bool ldIsZero(long double ld);
bool ldIsNaN(long double ld);
bool ldIsInf(long double ld);
// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
mstring* _getIntegerText(Minteger* _integer);
mstring* _getRealText(Mreal* _real);
mstring* _getStringText(Mstring* _string,bool dequoted);
mstring* _getListText(Mlist* _list);
mstring* _getMapText(Mmap* _map);

mstring* _getValueText(const Mvalue* const _value,bool dequoted); // flag only applicable to string values!!!
// getValueInteger() should return a value unequal to invalid iff _value can be converted to an integer (therefore should NOT equal invalid itself!!!!)
long long getValueInteger(const Mvalue* const _value,long long invalid);
void outputValue(const char* const prefix,const Mvalue* _value,const char* const postfix);

// list to map (list) conversions
bool listAppendedToMap(Mmap* const _map,const Mlist* const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool listAppendedToMaplist(Mlist* const _maplist,const Mlist* const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool maplistAppendedToList(Mlist* const _list,const Mlist* const _maplist);
bool maplistAppendedToMap(Mmap* const _map,const Mlist* const _maplist);
// map to (map) list conversions
bool mapAppendedToList(Mlist* const _list,Mmap* const _map);
bool mapAppendedToMaplist(Mlist* const _maplist,Mmap* const _map);

bool isZero(Mvalue* _value);
bool isOne(Mvalue* _value);

// unary functions
Mvalue* Mneg(Mvalue* _value); // negate a value
Mvalue* Mbnot(Mvalue* _value); // binary not a value
Mvalue* Mnot(Mvalue* _value); // not a value

bool isNull(Mvalue* _value); // expose as well
Mvalue* Mnull(Mvalue* _value); // whether null!!!
Mvalue* Mundefined(Mvalue* _value); // whether undefined!!!

Mvalue* Msum(Mvalue* _value); // sum (typically of a list)
Mvalue* Mlen(Mvalue* _value); // length (typically of a list)
Mvalue* Mfac(Mvalue* _value); // faculty (for an integer)

// MDH@20MAY2019: it's best to store a value at a single location (to replace all assignments to _value structure elements)
void assignValue(Mvalue** _valueholder,Mvalue* const _value);


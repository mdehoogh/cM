/**
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
enum Mvaluetype {VT_UNDEFINED,VT_INTEGER,VT_REAL,VT_STRING,VT_LIST,VT_MAP};

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
    mstring* _m;
}Mstring;

struct Mlist;
struct Mmap;
typedef union Mvalueunion{
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
    enum Mvaluetype type;
    Mvalueunion value;
}Mvalue;

typedef struct Mlistelement{
    Mvalue* _value;
    struct Mlistelement* _next;
}Mlistelement;

typedef struct Mlist{
    Mlistelement* _first;
    Mlistelement* _last;
    uint32_t numberOfElements; // keep track of the total number of elements
}Mlist;

typedef struct Mvariable{
    char* _name;
    Mvalue* _value;
}Mvariable;

typedef struct Mmapelement{
    Mvariable* _variable;
    struct Mmapelement* _next;
}Mmapelement;

typedef struct Mmap{
    Mmapelement* _first;
    Mmapelement* _last;
    uint32_t numberOfElements; // keep track of the total number of variables
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
typedef Mvalue* (*NoArgumentFunction)(void);
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

typedef struct Mfunction{
    mstring* _name;
    Mmap* _parameterMap; // a map of values defines the parameters and their default values (implicitly defining the expected types)
    Mfunctiontype type; // whether internal or external
    Mfunctionunion functionunion; // where either the internal function to call with the arguments is placed or 
}Mfunction;

typedef struct Mfunctionmapelement{
    Mfunction* _function;
    struct Mfunctionmapelement* _next;
}Mfunctionmapelement;

typedef struct MfunctionMap{
    Mfunctionmapelement* _first;
    Mfunctionmapelement* _last;
    uint32_t numberOfFunctions;  // keeping track of the total number of functions...
}Mfunctionmap;

//Mvalue* getFunction(Mfunctionlist functionlist,char* name);

// environments

// an environment is a bag of variables and functions
typedef struct Menvironment{
    Mmap* _variableMap; // variables are stored by name
    Mfunctionmap* _functionMap; // this would be the map of M functions defined in this environment (i.e. not the C functions/constants)
    struct Menvironment* _parent;
}Menvironment;

// pointer to these structs releasers
void free_string(Mstring* _string);
void free_value(Mvalue* _value);
void free_listelement(Mlistelement* listelement);
void free_list(Mlist* _list);
void free_variable(Mvariable* _variable);
void free_mapelement(Mmapelement* _mapelement);
void free_map(Mmap* _map);
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

// function prototypes
// read access
uint32_t getNumberOfVariables(Menvironment* _environment);
mstring* _getVariableNames(const Menvironment* _environment,char* sep);
Mvariable* getNewVariable(Menvironment* _environment,const char* name);
Mvariable* getVariable(Menvironment* _environment,const char* name);

// write access
Mvariable* addVariable(Menvironment* _environment,char* name,enum Mvaluetype valueType);
Mvalue* getValueOfVariable(Menvironment* _environment,char* name);

// if you want to set a value you have to pass in a pointer to the contents
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real);
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer);
bool setValueOfStringVariable(Mvariable* _variable,Mstring* _string);

// functions
mstring* _getFunctionNames(const Menvironment* _environment,char* sep);
Mfunction* newFunction(Menvironment* _environment,const char* functionName);
bool registerInternalFunctions(Menvironment* _environment);
// helper function to return the function
Mfunction* getFunction(Menvironment* _environment,const char* functionName);
Mmap* getFunctionArgumentMap(Mfunction* _function,Mlist* _argumentList);

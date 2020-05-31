/**
 * MDH@24JUN2019: Mvalue stuff wrapping raw data structures defined in Mexecution
 */
// additional data structures from
#include "Mdecimal.h"

struct Mlist;
struct Mmap;
struct Mreference;
// MDH@03MAR2020: if we want to be able to wrap a function or an environment in a value we have to add them here
struct Mfunction;
struct Menvironment;
// MDH@04NOV2019: we need a variable reference not a value reference as used in M.c, so I've introduced Mreference in Mexecution.c instead!!!! struct Mvaluereference; // MDH@26OCT2019: being able to store a value reference in a value coming up next...
typedef union Mvalueunion{
    Mtoken* _token;
    Minteger* _integer;
    Mbiginteger* _biginteger; // most convenient to immediately point to the Mbiginteger structure
    Mdecimal* _decimal; // no longer pointing to the mpd_t structure, as we're going to store the number of repeating decimals as well!!!
    Mrational* _rational;
    Mfloat* _float;
    Mtext* _text;
    struct Mlist* _list;
    struct Mmap* _map;
    struct Mreference* _reference; // MDH@04NOV2019: for now a reference is simply a pointer to a variable
    struct Mfunction* _function; // MDH@03MAR2020
    struct Menvironment* _environment; // MDH@03MAR2020
    //////////struct Muserfunction* _userfunction;
}Mvalueunion;

// a Value is either a number (numeric literal), a string literal, a list of values or a map
// you could say that a map is a list of variables, as such Menvironment holds a map of variables and a map of functions
// and we could make a separate struct to hold a map
typedef struct Mvalue{
    size_t count; // keep track of the number of reference
    Mvaluetype type;
    Mvalueunion value;
}Mvalue;

// MDH@26OCT2019: moved over from M.c so that we can store value references in values as well (like pointers, well not exactly like pointers)
typedef struct Mvaluereference{
	Mchars* _name; // the name of the host variable or NULL if we're in a substructure
	Mvalue* _value; // either the host value (if no variable name is defined), or the value of the host variable
	Mvalue* _itemid; // the item referenced!!!
}Mvaluereference;

void free_valuereference(Mvaluereference* _valuereference,Mallocationowner owner);

// a variable is a named value of a certain value type
typedef struct Mvariable{
    Mchars* _name;
    Mvaluetype valuetype; // MDH@01MAY2019: fixed type variables can only be assigned once, after that any value that is assigned to it has to have the same type as the first value
    Mvalue* _value;
    size_t referencecount; // MDH@04NOV2019: keep track of all its references
    bool immutable:1; // whether or not mutable
}Mvariable;

typedef struct Mreference{
    Mvariable* variable;
    size_t referenceindex;
}Mreference;

Mreference* _getReference(Mvariable* variable);
void free_reference(Mreference* reference,Mallocationowner owner);

typedef struct Mlistelement{
    unsigned long long index; // MDH@03MAY2019: keep track of the index in the list of this list element
    Mvalue* _value;
    struct Mlistelement* _next;
}Mlistelement;

// MDH@03MAY2019: we're going to allow a list to be sparse i.e. with each element we keep an offset
//                this may come in handy if we fail to store something in a list
// MDH@17APR2020: variable size dynamic allocations should be 'managed' so we can tell how big they are
//                of course we know the size of char* obviously BUT we should NOT use REALLOC on it, in which case we loose track of it
typedef struct Mlist{
    Mchars* _creator; // MDH@17APR2020: replacing: char *_creator;
    unsigned long long numberOfElements; // keep track of the total number of elements
    Mvaluetype valuetype; // we can force a list to have elements of the same type
    Mlistelement* _first;
    Mlistelement* _last;
    bool weak:1;
    bool immutable:1;
}Mlist;

typedef struct Mmapelement{
    Mvariable* _variable;
    struct Mmapelement* _next;
}Mmapelement;

typedef struct Mmap{
    unsigned long long numberOfElements; // keep track of the total number of variables
    Mvaluetype valuetype; // the type all values in the map should have
    Mmapelement* _first;
    Mmapelement* _last;
    bool weak:1;
    bool immutable:1;
}Mmap;

//Mvalue* getVariableValue(Mvariablelist variablelist,char* name);

typedef struct Mexpressionlistelement{
    Mchars* _binop; // the binary operator to apply to the operands
    Mvalue* _value; // the second operand
    struct Mexpressionlistelement* _next;
}Mexpressionlistelement;

typedef struct Mexpressionlist{
    Mvalue* _value; // the first value in the expression list
    Mexpressionlistelement* _next;
}Mexpressionlist;

void free_map(Mmap* _map,Mallocationowner owner);

Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter); // convert a long double to its rational equivalent and wraps it in a value

Mmap* _getMap(char *name);
Mmap* _getMapCopy(Mmap const * const map);
Mmap* _getFloatMap(char* name,Mvalue* _floatValue);
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue);
Mmap* _getListMap(char* name,Mvalue* _listValue);
Mmap* _getStringStringMap(char* name1,char* name2);
Mmap* _getFloatFloatMap(char* name1,char* name2);
long long isMapUndefined(Mmap* map);

// in order to find out if a big integer is out of the long long range we need the smallest and largest long long big integer values
// data wrappers
Mvalue* _getUndefinedValue(); // it's also possible to ask for an undefined value!!!
// and allow asking for a reference value wrapper
Mvalue* _getIntegerValue(long long ll);
Mvalue* _getCharTextValue(char _c);

// MDH@13JUN2019: anything that receives a pointer and might fail, should allow freeing the input pointer
// MDH@28MAY2020 TODO shouldn't we rename these to _getValueOfReference etc.
Mvalue* _getReferenceValue(Mreference* _reference,Mallocationowner owner_reference); // MDH@04NOV2019: wrap a variable name as a reference (I suppose it ought to reference a variable though)
Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,Mallocationowner owner_biginteger); // MDH@31MAY2019: we cannot use a big integer long here
Mvalue* _getRationalValue(Mrational* _rational,Mallocationowner owner_rational);
Mvalue* _getDecimalValue(Mdecimal* _decimal,Mallocationowner owner_decimal);

Mvalue* _getFloatValue(long double ld);
Mvalue* _getTextValue(char const * const text);
Mvalue* _getListValue(Mvaluetype listValuetype,bool weak,char const * const source); // returning an empty list with all values to be of type listValuetype
Mvalue* _getMapValue(Mvaluetype mapValuetype,bool weak); // returning an empty map with all values to be of type mapValuetype
//////Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure);
//////Mvalue* _getTokenValue(char* text);

Mvalue* _getValueOfList(Mlist* _list,Mallocationowner owner_list);
Mvalue* _getValueOfInteger(Minteger* _integer,Mallocationowner owner_integer);
Mvalue* _getValueOfReal(Mfloat* _real,Mallocationowner owner_real);
Mvalue* _getValueOfMap(Mmap* _map,Mallocationowner owner_map);
Mvalue* _getValueOfToken(Mtoken* _token,Mallocationowner owner_token);

Mlist* _getListOfType(Mvaluetype valuetype);
Mmap* _getMapOfType(Mvaluetype valuetype);

Mlist* listMadeWeak(Mlist* list);
Mmap* mapMadeWeak(Mmap* map);

// MDH@02MAY2019: not allowed to call free_value from the outside
bool decrementReferenceCount(Mvalue* _value);
bool incrementReferenceCount(Mvalue* _value);

// MDH@28MAY2020: because every value is owned by the same owner i.e. 'the value owner' which is one level down _valueList there's no need to call free_value with an owner
//                technically this means that a value once created does (and should) never change ownership as opposed to locally created stuff not bound to a global variable
Mallocationowner getValueOwner();
// MDH@28MAY2020 better not to let the outside free values ever (except this module's garbage collector of course): void free_value(Mvalue* _value/*,Mallocationowner owner*/);
//////////Mstring* appendld(Mstring* mstr,long double ld);

/*unsigned */long long appendedToList(Mlist* const _list,Mallocationowner owner_list,const Mvalue* const _value,long long index); // helper function to append to a list with a certain index (possibly undefined), index must not be negative, if zero first available index will be used, otherwise it should be at least the first available index!!

Mlist* __list(char* source);
void free_list(Mlist* _list,Mallocationowner owner);
Mlist* _getListCopy(Mlist const * const _list);

Mlist* _getListIndices(Mlist const * const _list);
Mlist* _getMapAttributes(Mmap const * const _map);

Mlist* _getFlattenedList(Mvalue const * const _value,unsigned int flattenLevel,bool reversed); // MDH@30MAR2020: to apply index element that can be lists, we need to flatten the list
Mvalue* getFirstScalarValue(Mvalue* value);

long long isListUndefined(Mlist* list);

long long appendedToMap(Mmap* const _map,Mallocationowner owner_map,const char* const attributeName,const Mvalue* const _attributeValue);
long long removedFromMap(Mmap* const _map,Mallocationowner owner_map,const char* const attributeName);

long double getValueLongDouble(Mvalue const * const _value);
Mbiginteger* _getValueBiginteger(Mvalue const * const _value); // converts a value to a big integer (if possible)

long long getValueSign(Mvalue const * const value); // return -1 for negative values, 1 for positive values, 0 for zero values, and M_LL_INVALID for non-scalar values obviously

Mvalue* getValueAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName);

Mvalue** getValueHolderAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue** getValueHolderOfAttribute(Mmap* _map,char* attributeName);

Mstring* _getListText(Mlist* _list);
Mstring* _getMapText(Mmap* _map,bool showcurlybraces,bool showquotes,bool showmissings);

void outputList(char const * const prefix,Mlist* list,char const * const suffix); // MDH@02MAR2020: utility function to output a list
void outputMap(char const * const prefix,Mmap* map,char const * const suffix); // MDH@02MAR2020: utility function to output a map

Mstring* _getValueText(const Mvalue* const _value,bool dequoted); // flag only applicable to string values!!!

// getValueInteger() should return a value unequal to invalid iff _value can be converted to an integer (therefore should NOT equal invalid itself!!!!)
long long getValueInteger(const Mvalue* const _value);
size_t outputValue(const char* const prefix,const Mvalue* value,const char* const suffix);

Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3);
Mmap* _getMapTokenMap(char* name1,char* name2);
Mmap* _getTokenTokenMap(char* name1,char* name2);
Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3);
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3);
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char* name4);
Mmap* _getTokenTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4,char *name5);

Mmap* _getListValueIntegerMap(char* name1,char* name2,char* name3);
Mmap* _getIntegerBooleanMap(char* name1,char* name2);

// list to map (list) conversions
bool listAppendedToMap(Mmap * const _map,Mallocationowner owner_map,Mlist const * const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool listAppendedToMaplist(Mlist * const _maplist,Mallocationowner owner_maplist,Mlist const * const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool maplistAppendedToList(Mlist * const _list,Mallocationowner owner_list,Mlist const * const _maplist);
bool maplistAppendedToMap(Mmap * const _map,Mallocationowner owner_map,Mlist const * const _maplist);
// map to (map) list conversions
bool mapAppendedToList(Mlist * const _list,Mallocationowner owner_list,Mmap const * const _map);
bool mapAppendedToMaplist(Mlist * const _maplist,Mallocationowner owner_maplist,Mmap const * const _map);

// all is... methods should now return a long long equal to M_TRUE, M_FALSE or M_LL_INVALID
long long isValueZero(Mvalue* value);
long long isValueOne(Mvalue* value);
long long isValuePositive(Mvalue* value);
long long isValueNegative(Mvalue* value);
long long isValueScalar(Mvalue* value);
long long isValueNull(Mvalue* value); // expose as well
long long isValueUndefined(Mvalue* value); // expose as well

// MDH@20MAY2019: it's best to store a value at a single location (to replace all assignments to _value structure elements)
void assignValue(Mvalue** _valueholder,Mvalue const * _value);

Mvalue* __value(char const * const descriptor); // TODO expose __value()????? yes

size_t getNumberOfRemovedValues(bool showInfo);
unsigned long long getNumberOfValues();

Mvariable* _getVariable(const char* name,Mvaluetype valuetype,bool immutable);
void free_variable(Mvariable* _variable,bool weak,Mallocationowner owner);

bool free_mapelement(Mmapelement* _mapelement,bool weak,Mallocationowner owner);
bool free_listelement(Mlistelement* _listelement,bool weak,Mallocationowner owner);

typedef Mvalue* (*NoArgumentFunction)();
typedef Mvalue* (*OneArgumentFunction)(Mvalue* _argumentValue);
typedef Mvalue* (*TwoArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value);
typedef Mvalue* (*ThreeArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value);
typedef Mvalue* (*FourArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value,Mvalue* _argument4Value);
typedef Mvalue* (*FiveArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value,Mvalue* _argument4Value,Mvalue* _argument5Value);

Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction);
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction);

// some conversion functions that might be moved to some more specialized 'module'
Mbiginteger* _getRationalInteger(Mrational* _rational,bool floor,bool towardszero); // TODO probably to be moved to Mrational.h/c
Mbiginteger* _getRoundedRationalInteger(Mrational* _rational);

Mdecimal* _getDecimalInteger(Mdecimal* _decimal,bool floor,bool towardszero);
Mdecimal* _getRoundedDecimal(Mdecimal* _decimal);

Mdecimal* _getValueDecimal(Mvalue* _value);
Mdecimal* getValueDecimal(Mvalue* _value);

Mdecimal* _getValueTextDecimal(Mvalue* value); // MDH@09OCT2019: delegates to _getTextDecimal() in Mdecimal.h/c, guarantees to return a new decimal from parsing the value text representation (unless the value wraps a decimal itself)

bool areValuesEqual(Mvalue const * const value1,Mvalue const * const value2); //MDH@05NOV2019: moved over from Menvironment.h/c as we need it in Mlist's find() function

// MDH@03MAR2020: moved over from Menvironment.h
 // functions of different types, internal (no body but a function to pass the arguments to) or external (with a body)
// all functions are executed in an execution environment, that descends from the environment in which the function is defined (the definition environment)
// MDH@28MAY2019: how about NOT passing the execution environment and instead keep a current execution environment instead (in case one needs it)
/////////////struct Menvironment;
// MDH@10JUL2019: user functions are stored differently than M functions
// MDH@20JUL2019: moved over from Mvalue but this means that we cannot currently store a function in a value...
struct Mfunctionmap; // prototype of Mfunctionap
typedef struct Muserfunction{
    //////////struct Mmap* _parameterMap;
    struct Mfunctionmap* _functionMap; // to contain the list of (user) functions defined inside the function
    struct Mlist* _bodyCommandList; // a list of body commands
}Muserfunction;
void free_userfunction(Muserfunction* _userfunction,Mallocationowner owner_userfunction);

typedef enum Mfunctiontype{FT_USER,FT_INTERNAL_NO_ARGUMENTS,FT_INTERNAL_ONE_ARGUMENT,FT_INTERNAL_TWO_ARGUMENTS,FT_INTERNAL_THREE_ARGUMENTS,FT_INTERNAL_FOUR_ARGUMENTS,FT_INTERNAL_FIVE_ARGUMENTS}Mfunctiontype;

typedef union Mfunctionunion{
    NoArgumentFunction noArgumentFunction;
    OneArgumentFunction oneArgumentFunction;
    TwoArgumentFunction twoArgumentFunction;
    ThreeArgumentFunction threeArgumentFunction;
    FourArgumentFunction fourArgumentFunction;
    FiveArgumentFunction fiveArgumentFunction;
    Muserfunction* _userfunction; // a list of expressions to evaluate that use the parameters (and have defaults, and an environment)
}Mfunctionunion;

// struct Menvironment;
// MDH@03FEB2020: the definition environment (in which the function is created is wrapped in a value so that it can persist even when popped from the execution stack (JavaScript like))
typedef struct Mfunction{
    ////////Mstring* _name;
    Mmap* _parameterMap; // a map of values defines the parameters and their default values (implicitly defining the expected types)
    Mvalue* _definitionEnvironmentValue; // MDH@03FEB2020 replacing: struct Menvironment* _definitionEnvironment;
    Mfunctiontype type; // whether internal or external
    Mfunctionunion functionunion; // where either the internal function to call with the arguments is placed or 
}Mfunction;

typedef struct Mfunctionmapelement{
    Mstring* _name;
    Mfunction* _function;
    struct Mfunctionmapelement* _next;
}Mfunctionmapelement;

typedef struct Mfunctionmap{
    uint32_t numberOfFunctions;  // keeping track of the total number of functions...
    Mfunctionmapelement* _first;
    Mfunctionmapelement* _last;
}Mfunctionmap;

//Mvalue* getFunction(Mfunctionlist functionlist,char* name);

// environments

// an environment is a bag of variables and functions
// MDH@03FEB2020: all environments should be wrapped in Mvalue instances, so they won't get released until they can
typedef struct Menvironment{
    Mchars* _name; // the name of the environment
    Mmap* _variableMap; // variables are stored by name
    Mfunctionmap* _functionMap; // this would be the map of M functions defined in this environment (i.e. not the C functions/constants)
    Mtoken* expressionToken; // MDH@17JUL2019: the current token of the expression being evaluated in this environment
    Mvalue* _parent; // MDH@03FEB2020 replacing: struct Menvironment* _parent; // typically the definition environment
    Mvalue* execution; // MDH@03FEB2020 replacing: struct Menvironment* _execution; // the environment that was executing before this one was popped!!
}Menvironment;

Menvironment* __environment(); // creates a new (empty) environment
Mstring* _getEnvironmentName(Menvironment* _environment); // for use in prompting
void free_environment(Menvironment* _environment,Mallocationowner owner_environment);
Menvironment* getEnvironmentParent(Menvironment* _environment);

bool free_function(Mfunction* _function,Mallocationowner owner_function);

Menvironment* getValueEnvironment(Mvalue* _value); // MDH@03FEB2020: the first additional function to obtain a specific data type value

Mvalue* _getValueOfFunction(Mfunction* _function,Mallocationowner owner_function);
Mvalue* _getValueOfEnvironment(Menvironment* _environment,Mallocationowner owner_environment);
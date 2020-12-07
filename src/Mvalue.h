/**
 * MDH@24JUN2019: Mvalue stuff wrapping raw data structures defined in Mexecution
 */
// additional data structures from
#include "Mdecimal.h"

struct Mlist;
struct Marray; // MDH@04NOV2020: we're going to have an array after all (so we can speed up sorting)
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
    struct Marray* _array;
    struct Mmap* _map;
    struct Mreference* _reference; // MDH@04NOV2019: for now a reference is simply a pointer to a variable
    struct Mfunction* _function; // MDH@03MAR2020
    struct Menvironment* _environment; // MDH@03MAR2020
    struct Mfile* _file; // MDH@28SEP2020 (defined in Mexecution.h)
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

void free_valuereference(Mvaluereference* _valuereference/*,Mallocationowner owner*/);
#ifndef __PRODUCTION__
Mvaluereference* owned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference);
Mvaluereference* disowned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference);
#define FREE_VALUEREFERENCE(_valuereference,owner_valuereference) free_valuereference(disowned_valuereference(_valuereference,owner_valuereference))
#else
#define FREE_VALUEREFERENCE(_valuereference,owner_valuereference) free_valuereference(_valuereference)
#endif


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
void free_reference(Mreference* _reference);
#ifndef __PRODUCTION__
Mreference* owned_reference(Mreference* reference,Mallocationowner owner);
Mreference* disowned_reference(Mreference* reference,Mallocationowner owner);
#define FREE_REFERENCE(_reference,owner_reference) free_reference(disowned_reference(_reference,owner_reference))
#define __REFERENCE(_variable,owner_reference) owned_reference(_getReference(_variable),owner_reference)
#else
#define FREE_REFERENCE(_reference,owner_reference) free_reference(_reference)
#define __REFERENCE(_variable,owner_reference) _getReference(_variable)
#endif


typedef struct Mlistelement{
    Mvalue* _value; // should be the first field in Mlistelement, so that we can use Mlistelement* as a value holder (Mvalue**)
    unsigned long long index; // MDH@03MAY2019: keep track of the index in the list of this list element
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

Mlist* __list(char* source/*,Mallocationowner owner_list*/);
void free_list(Mlist* _list/*,Mallocationowner owner*/);
#ifndef __PRODUCTION__
Mlist* owned_list(Mlist * const _list,Mallocationowner owner_list);
Mlist* disowned_list(Mlist * const _list,Mallocationowner owner_list);
#define OWNED_LIST(_list,owner_list) owned_list(_list,owner_list)
#define __LIST(source,owner_list) owned_list(__list(source),owner_list)
#define DISOWNED_LIST(_list,owner_list) disowned_list(_list,owner_list)
#define FREE_LIST(_list,owner_list) free_list(disowned_list(_list,owner_list))
#else
#define OWNED_LIST(_list,owner_list) _list
#define __LIST(source,owner_list) __list(source)
#define DISOWNED_LIST(_list,owner_list) _list
#define FREE_LIST(_list,owner_list) free_list(_list)
#endif

Mlist* _getListOfType(Mvaluetype valuetype);
Mlist* listMadeWeak(Mlist * const list);
Mlist* _getListCopy(Mlist const * const _list);
Mlist* _getListIndices(Mlist const * const _list);
Mlist* _getReversedList(Mlist const * const list); // MDH@19JUN2020: convenience method to reverse a list returns NULL on failure
Mlist* _getFlattenedList(Mvalue const * const _value,unsigned int flattenLevel,bool reversed); // MDH@30MAR2020: to apply index element that can be lists, we need to flatten the list
long long isListUndefined(Mlist* list);

/*unsigned */long long appendedToList(Mlist * const _list,Mallocationowner owner_list,Mvalue const * const _value,long long index); // helper function to append to a list with a certain index (possibly undefined), index must not be negative, if zero first available index will be used, otherwise it should be at least the first available index!!
long long insertedIntoList(Mlist * const _list,Mallocationowner owner_list,Mvalue const * const _value,long long index); // MDH@23NOV2020: helper function to insert into a list with a certain index

// MDH@04NOV2020: similar definitions for Marray
typedef struct Marray{
    Mchars* _creator; // MDH@17APR2020: replacing: char *_creator;
    unsigned long long numberOfElements; // keep track of the total number of elements
    Mvaluetype valuetype; // we can force a list to have elements of the same type
    Mvalue** values; // I suppose we need to have a pointer to an array of Mvalue pointers, alternatively if the number of elements is fixed, we could point to the Mvalue structures themselves?????
    bool weak:1;
    bool immutable:1;
}Marray;
long long isArrayUndefined(Marray* array);

Marray* __array(char* source/*,Mallocationowner owner_list*/);
Marray* _getArray(char* source,unsigned long long numberOfValues);
void free_array(Marray* _array/*,Mallocationowner owner*/);
#ifndef __PRODUCTION__
Marray* owned_array(Marray * const _array,Mallocationowner owner_array);
Marray* disowned_array(Marray * const _array,Mallocationowner owner_array);
#define OWNED_ARRAY(_array,owner_array) owned_array((_array),(owner_array))
#define __ARRAY(source,owner_array) owned_array(__array(source),owner_array)
#define DISOWNED_ARRAY(_array,owner_array) disowned_array((_array),(owner_array))
#define FREE_ARRAY(_array,owner_array) free_array(disowned_array(_array,owner_array))
#else
#define OWNED_ARRAY(_array,owner_array) _array
#define __ARRAY(source,owner_array) __array(source)
#define DISOWNED_ARRAY(_array,owner_array) _array
#define FREE_ARRAY(_array,owner_array) free_array(_array)
#endif
Marray* _getArrayCopy(Marray const * const array);
void outputArray(char const * const prefix,Marray const * const array,char const * const suffix);

typedef struct Mmapelement{
    Mvariable* _variable;
    struct Mmapelement* _next;
}Mmapelement;

long long free_mapelement(Mmapelement* _mapelement,bool weak);
#ifndef __PRODUCTION__
Mmapelement* owned_mapelement(Mmapelement * const _mapelement,Mallocationowner owner_mapelement);
Mmapelement* disowned_mapelement(Mmapelement * const _mapelement,Mallocationowner owner_mapelement);
#define FREE_MAPELEMENT(_mapelement,weak,owner_mapelement) free_mapelement(disowned_mapelement(_mapelement,owner_mapelement),weak)
#else
#define FREE_MAPELEMENT(_mapelement,weak,owner_mapelement) free_mapelement(_mapelement,weak)
#endif

typedef struct Mmap{
    Mchars* _creator; // MDH@01NOV2020: replacing: char *_creator;
    unsigned long long numberOfElements; // keep track of the total number of variables
    Mvaluetype valuetype; // the type all values in the map should have
    Mmapelement* _first;
    Mmapelement* _last;
    bool weak:1;
    bool immutable:1;
}Mmap;

Mmap* __map(char* source); // MDH@01NOV2020
void free_map(Mmap* _map);
#ifndef __PRODUCTION__
Mmap* owned_map(Mmap* _map,Mallocationowner owner_map);
Mmap* disowned_map(Mmap* _map,Mallocationowner owner_map);
#define FREE_MAP(_map,owner_map) free_map(disowned_map(_map,owner_map))
#else
#define FREE_MAP(_map,owner_map) free_map(_map)
#endif

Mmap* _getMap(char *name);
Mmap* _getMapCopy(Mmap const * const map);
Mmap* _getFloatMap(char* name,Mvalue* _floatValue);
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue);
Mmap* _getListMap(char* name,Mvalue* _listValue);
Mmap* _getStringStringMap(char* name1,char* name2);
Mmap* _getFloatFloatMap(char* name1,char* name2);
Mlist* _getMapAttributes(Mmap const * const _map);
long long isMapUndefined(Mmap* map);

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

Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter); // convert a long double to its rational equivalent and wraps it in a value

// in order to find out if a big integer is out of the long long range we need the smallest and largest long long big integer values
// data wrappers
Mvalue* _getUndefinedValue(); // it's also possible to ask for an undefined value!!!
// and allow asking for a reference value wrapper
Mvalue* _getIntegerValue(long long ll);
Mvalue* _getCharTextValue(char _c);

// MDH@13JUN2019: anything that receives a pointer and might fail, should allow freeing the input pointer
// MDH@28MAY2020 TODO shouldn't we rename these to _getValueOfReference etc.
Mvalue* _getFloatValue(long double ld);
Mvalue* _getTextValue(char const * const text);
Mvalue* _getListValue(Mvaluetype listValuetype,bool weak,char const * const source); // returning an empty list with all values to be of type listValuetype
Mvalue* _getMapValue(Mvaluetype mapValuetype,bool weak); // returning an empty map with all values to be of type mapValuetype
//////Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure);
//////Mvalue* _getTokenValue(char* text);

Mvalue* _getValueOfArray(Marray* _array);
Mvalue* _getValueOfList(Mlist* _list/*,Mallocationowner owner_list*/);
Mvalue* _getValueOfInteger(Minteger* _integer/*,Mallocationowner owner_integer*/);
Mvalue* _getValueOfReal(Mfloat* _real/*,Mallocationowner owner_real*/);
Mvalue* _getValueOfMap(Mmap* _map/*,Mallocationowner owner_map*/);
Mvalue* _getValueOfToken(Mtoken* _token/*,Mallocationowner owner_token*/);
Mvalue* _getValueOfText(Mtext* _text); // MDH@07DEC2020: finally!!!
Mvalue* _getValueOfReference(Mreference* _reference); // MDH@04NOV2019: wrap a variable name as a reference (I suppose it ought to reference a variable though)
Mvalue* _getValueOfBiginteger(Mbiginteger* _biginteger); // MDH@31MAY2019: we cannot use a big integer long here
Mvalue* _getValueOfRational(Mrational* _rational);
Mvalue* _getValueOfDecimal(Mdecimal* _decimal);

Mmap* _getMapOfType(Mvaluetype valuetype);
Mmap* mapMadeWeak(Mmap * const map);

// MDH@02MAY2019: not allowed to call free_value from the outside
bool decrementReferenceCount(Mvalue * const _value);
bool incrementReferenceCount(Mvalue * const _value);

// MDH@28MAY2020: because every value is owned by the same owner i.e. 'the value owner' which is one level down _valueList there's no need to call free_value with an owner
//                technically this means that a value once created does (and should) never change ownership as opposed to locally created stuff not bound to a global variable
Mallocationowner getValueOwner();
// MDH@28MAY2020 better not to let the outside free values ever (except this module's garbage collector of course): void free_value(Mvalue* _value/*,Mallocationowner owner*/);
//////////Mstring* appendld(Mstring* mstr,long double ld);
Mvalue* getFirstScalarValue(Mvalue* value);

Mmapelement* getMapelement(Mmap const * const map,char const * const attributeName); // MDH@22OCT2020: useful not only in appendedByMap() but also in _getFunctionArgumentMap()
long long appendedToMap(Mmap * const _map,Mallocationowner owner_map,char const * const attributeName,Mvalue const * const _attributeValue);
long long removedFromMap(Mmap * const _map,Mallocationowner owner_map,char const * const attributeName);

long double getValueLongDouble(Mvalue const * const _value);
Mbiginteger* _getValueBiginteger(Mvalue const * const _value); // converts a value to a big integer (if possible)

long long getValueSign(Mvalue const * const value); // return -1 for negative values, 1 for positive values, 0 for zero values, and M_LL_INVALID for non-scalar values obviously

Mvalue* getValueAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName);

Mvalue** getValueHolderAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue** getValueHolderOfAttribute(Mmap* _map,char* attributeName);

// MDH@02NOV2020: limiting the number of values from the list to actually show...
Mstring* _getListText(Mlist const * const _list,long long showAtStart,long long showAtEnd);
Mstring* _getMapText(Mmap const * const _map,bool showcurlybraces,bool showquotes,bool showmissings);

void outputList(char const * const prefix,Mlist const * const list,char const * const suffix); // MDH@02MAR2020: utility function to output a list
void outputMap(char const * const prefix,Mmap const * const map,char const * const suffix); // MDH@02MAR2020: utility function to output a map

Mstring* _getValueText(Mvalue const * const _value,bool dequoted); // flag only applicable to string values!!!
Mvalue* _getStringValue(Mstring const * const _string); // MDH@28SEP2020: why wasn't this here so far?????


// getValueInteger() should return a value unequal to invalid iff _value can be converted to an integer (therefore should NOT equal invalid itself!!!!)
long long getValueInteger(Mvalue const * const _value);
size_t outputValue(char const * const prefix,Mvalue const * value,char const * const suffix);

Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3);
Mmap* _getMapTokenMap(char* name1,char* name2);
Mmap* _getMapListMap(char* name1,char* name2); // MDH@28OCT2020
Mmap* _getTokenTokenMap(char* name1,char* name2);
Mmap* _getListFunctionMap(char* name1,char* name2);
Mmap* _getIntegerBooleanMap(char* name1,char* name2);
Mmap* _getListTextMap(char* name1,char* name2);

Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3);
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3);
Mmap* _getListValueIntegerMap(char* name1,char* name2,char* name3);
Mmap* _getMapMapListMap(char* name1,char* name2,char* name3);
Mmap* _getListFunctionValueMap(char* name1,char* name2,char* name3);

Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char* name4);
Mmap* _getTokenTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4,char *name5);

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

void free_variable(Mvariable* _variable,bool weak);
#ifndef __PRODUCTION__
Mvariable* disowned_variable(Mvariable* _variable,Mallocationowner owner_variable);
Mvariable* owned_variable(Mvariable* _variable,Mallocationowner owner_variable);
#define FREE_VARIABLE(_variable,weak,owner_variable) free_variable(disowned_variable(_variable,owner_variable),weak)
#else
#define FREE_VARIABLE(_variable,weak,owner_variable) free_variable(_variable,weak)
#endif

Mvariable* _getVariable(Mchars const * const _name,Mvaluetype valuetype,bool immutable);

long long free_listelement(Mlistelement* _listelement,bool weak/*,Mallocationowner owner*/); // returning the number of successive list elements freed

#ifndef __PRODUCTION__
Mlistelement* owned_listelement(Mlistelement * const _listelement,Mallocationowner owner_listelement);
Mlistelement* disowned_listelement(Mlistelement * const _listelement,Mallocationowner owner_listelement);
#define FREE_LISTELEMENT(_listelement,weak,owner_listelement) free_listelement(disowned_listelement(_listelement,owner_listelement),weak)
#else
#define FREE_LISTELEMENT(_listelement,weak,owner_listelement) free_listelement(_listelement,weak)
#endif

typedef Mvalue* (*NoArgumentFunction)();
typedef Mvalue* (*OneArgumentFunction)(Mvalue* _argumentValue);
typedef Mvalue* (*TwoArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value);
typedef Mvalue* (*ThreeArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value);
typedef Mvalue* (*FourArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value,Mvalue* _argument4Value);
typedef Mvalue* (*FiveArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value,Mvalue* _argument4Value,Mvalue* _argument5Value);

Marray* appliedToArray(Marray* _array,OneArgumentFunction oneArgumentFunction); // MDH@22NOV2020
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
    Mmap* _localMap; // MDH@29OCT2020: the local map is typically only defined on user functions
    struct Mfunctionmap* _functionMap; // to contain the list of (user) functions defined inside the function
    struct Mlist* _bodyCommandList; // a list of body commands
}Muserfunction;

void free_userfunction(Muserfunction* _userfunction);
#ifndef __PRODUCTION__
Muserfunction* disowned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction);
Muserfunction* owned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction);
#define FREE_USERFUNCTION(_userfunction,owner_userfunction) free_userfunction(disowned_userfunction(_userfunction,owner_userfunction))
#else
#define FREE_USERFUNCTION(_userfunction,owner_userfunction) free_userfunction(_userfunction)
#endif

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

void free_function(Mfunction* _function);
#ifndef __PRODUCTION__
Mfunction* owned_function(Mfunction* _function,Mallocationowner owner_function);
Mfunction* disowned_function(Mfunction* _function,Mallocationowner owner_function);
#define FREE_FUNCTION(_function,owner_function) free_function(disowned_function(_function,owner_function))
#else
#define FREE_FUNCTION(_function,owner_function) free_function(_function)
#endif

Mvalue* _getValueOfFunction(Mfunction* _function/*,Mallocationowner owner_function*/);

typedef struct Mfunctionmapelement{
    Mstring* _name;
    Mfunction* _function;
    struct Mfunctionmapelement* _next;
}Mfunctionmapelement;
// Mfunctionmapelement* disowned_functionmapelement(Mfunctionmapelement * const _functionmapelement,Mallocationowner owner_functionmapelement);

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
Menvironment* _getNewEnvironment(); // MDH@25OCT2020: create environment with a nameless variable

void free_environment(Menvironment* _environment/*,Mallocationowner owner_environment*/);
#ifndef __PRODUCTION__
Menvironment* owned_environment(Menvironment* _environment,Mallocationowner owner_environment);
Menvironment* disowned_environment(Menvironment* _environment,Mallocationowner owner_environment);
#define FREE_ENVIRONMENT(_environment,owner_environment) free_environment(disowned_environment(_environment,owner_environment))
#define __ENVIRONMENT(owner_environment) owned_environment(__environment(),owner_environment)
#else
#define FREE_ENVIRONMENT(_environment,owner_environment) free_environment(_environment)
#define __ENVIRONMENT(owner_environment) __environment()
#endif

Mstring* _getEnvironmentName(Menvironment* _environment); // for use in prompting
Menvironment* getEnvironmentParent(Menvironment* _environment);
Menvironment* getValueEnvironment(Mvalue* _value); // MDH@03FEB2020: the first additional function to obtain a specific data type value

Mvalue* _getValueOfEnvironment(Menvironment* _environment/*,Mallocationowner owner_environment*/);

// Mfile is added starting from v0.1.4
Mvalue* _getValueOfFile(Mfile* _file);

Mvalue* mfile(Mvalue* filename_value);
Mvalue* mfdelete(Mvalue* file_value);
Mvalue* mfopen(Mvalue* file_value,Mvalue* mode_value);
Mvalue* mfclose(Mvalue* file_value);
Mvalue* mfread(Mvalue* file_value,Mvalue* numberofbytes_value); // reads at most count_value 
Mvalue* mfreadline(Mvalue* file_value); // reads all bytes until a new line character is encountered 
Mvalue* mfreadlines(Mvalue* file_value,Mvalue* numberoflines_value); // reads all bytes until a new line character is encountered 
Mvalue* mfwrite(Mvalue* file_value,Mvalue* write_value);
Mvalue* mfiles(Mvalue* wildcard_value);
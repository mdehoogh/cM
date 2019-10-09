/**
 * MDH@24JUN2019: Mvalue stuff wrapping raw data structures defined in Mexecution
 */
// additional data structures from
#include "Mdecimal.h"

struct Mlist;
struct Mmap;
typedef union Mvalueunion{
    Mtoken* _token;
    Minteger* _integer;
    Mbiginteger* _biginteger; // most convenient to immediately point to the Mbiginteger structure
    Mdecimal* _decimal; // no longer pointing to the mpd_t structure, as we're going to store the number of repeating decimals as well!!!
    Mrational* _rational;
    Mreal* _real;
    Mtext* _text;
    struct Mlist* _list;
    struct Mmap* _map;
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

// a variable is a named value of a certain value type
typedef struct Mvariable{
    char* _name;
    bool immutable; // whether or not mutable
    Mvaluetype valuetype; // MDH@01MAY2019: fixed type variables can only be assigned once, after that any value that is assigned to it has to have the same type as the first value
    Mvalue* _value;
}Mvariable;

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

void free_map(Mmap* _map);

Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter); // convert a long double to its rational equivalent and wraps it in a value

Mmap* _getMap(char *name);
Mmap* _getRealMap(char* name,Mvalue* _realValue);
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue);
Mmap* _getListMap(char* name,Mvalue* _listValue);
Mmap* _getStringStringMap(char* name1,char* name2);
Mmap* _getRealRealMap(char* name1,char* name2);

// in order to find out if a big integer is out of the long long range we need the smallest and largest long long big integer values
// data wrappers
Mvalue* _getUndefinedValue(); // it's also possible to ask for an undefined value!!!
Mvalue* _getIntegerValue(long long ll);
Mvalue* _getCharTextValue(char _c);
// MDH@13JUN2019: anything that receives a pointer and might fail, should allow freeing the input pointer
Mvalue* _getBigintegerValue(Mbiginteger* _biginteger,bool freeonfailure); // MDH@31MAY2019: we cannot use a big integer long here
Mvalue* _getRationalValue(Mrational* _rational,bool freeonfailure);
Mvalue* _getDecimalValue(Mdecimal* _decimal,bool freeonfailure);
Mvalue* _getRealValue(long double ld);
Mvalue* _getTextValue(char* text,bool freeonfailure);
Mvalue* _getListValue(Mvaluetype listValuetype); // returning an empty list with all values to be of type listValuetype
Mvalue* _getMapValue(Mvaluetype mapValuetype); // returning an empty map with all values to be of type mapValuetype
//////Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure);
//////Mvalue* _getTokenValue(char* text);

Mvalue* _getValueOfList(Mlist* _list,bool freeonfailure);
Mvalue* _getValueOfInteger(Minteger* _integer,bool freeonfailure);
Mvalue* _getValueOfReal(Mreal* _real,bool freeonfailure);
Mvalue* _getValueOfMap(Mmap* _map,bool freeonfailure);
Mvalue* _getValueOfToken(Mtoken* _token,bool freeonfailure);

Mlist* _getListOfType(Mvaluetype valuetype);
Mmap* _getMapOfType(Mvaluetype valuetype);

// MDH@02MAY2019: not allowed to call free_value from the outside
bool decrementReferenceCount(Mvalue* _value);
bool incrementReferenceCount(Mvalue* _value);

void free_value(Mvalue* _value);
Mstring* appendld(Mstring* mstr,long double ld);

unsigned long long appendedToList(Mlist* const _list,const Mvalue* const _value,long long index); // helper function to append to a list with a certain index (possibly undefined), index must not be negative, if zero first available index will be used, otherwise it should be at least the first available index!!
void free_list(Mlist* _list);

bool appendedToMap(Mmap* const _map,const char* const attributeName,const Mvalue* const _attributeValue);

long double getValueReal(const Mvalue* const _value);
Mbiginteger* _getValueBiginteger(const Mvalue* const _value); // converts a value to a big integer (if possible)

Mvalue* getValueAtIndex(Mlist* _list,long long index); // helper function that can be used on any Mlist even if defined outside an environment (as getResult() defined in M.c does!!!)
Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName);

Mstring* _getListText(Mlist* _list);
Mstring* _getMapText(Mmap* _map,bool showcurlybraces,bool showquotes,bool showmissings);

Mstring* _getValueText(const Mvalue* const _value,bool dequoted); // flag only applicable to string values!!!

// getValueInteger() should return a value unequal to invalid iff _value can be converted to an integer (therefore should NOT equal invalid itself!!!!)
long long getValueInteger(const Mvalue* const _value);
void outputValue(const char* const prefix,const Mvalue* _value,const char* const postfix);

Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3);
Mmap* _getTokenTokenMap(char* name1,char* name2);
Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3);
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char* name4);

// list to map (list) conversions
bool listAppendedToMap(Mmap* const _map,const Mlist* const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool listAppendedToMaplist(Mlist* const _maplist,const Mlist* const _list); // append a list to a (possibly empty) map using the indices as attribute name
bool maplistAppendedToList(Mlist* const _list,const Mlist* const _maplist);
bool maplistAppendedToMap(Mmap* const _map,const Mlist* const _maplist);
// map to (map) list conversions
bool mapAppendedToList(Mlist* const _list,const Mmap* const _map);
bool mapAppendedToMaplist(Mlist* const _maplist,const Mmap* const _map);

bool isValueZero(Mvalue* _value);
bool isValueOne(Mvalue* _value);
 
bool isNull(Mvalue* _value); // expose as well
bool isUndefined(Mvalue* _value); // expose as well

// MDH@20MAY2019: it's best to store a value at a single location (to replace all assignments to _value structure elements)
void assignValue(Mvalue** _valueholder,Mvalue* const _value);

Mvalue* __value(); // TODO expose __value()????? yes

size_t getNumberOfRemovedValues();
unsigned long long getNumberOfValues();

Mvariable* _getVariable(const char* name,Mvaluetype valuetype,bool immutable);
void free_variable(Mvariable* _variable);

void free_mapelement(Mmapelement* _mapelement);

typedef Mvalue* (*NoArgumentFunction)();
typedef Mvalue* (*OneArgumentFunction)(Mvalue* _argumentValue);
typedef Mvalue* (*TwoArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value);
typedef Mvalue* (*ThreeArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value);
typedef Mvalue* (*FourArgumentFunction)(Mvalue* _argument1Value,Mvalue* _argument2Value,Mvalue* _argument3Value,Mvalue* _argument4Value);

Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction);
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction);

// some conversion functions that might be moved to some more specialized 'module'
Mbiginteger* _getRationalInteger(Mrational* _rational,bool floor,bool towardszero); // TODO probably to be moved to Mrational.h/c
Mbiginteger* _getRoundedRationalInteger(Mrational* _rational);

Mdecimal* _getValueDecimal(Mvalue* _value);
Mdecimal* getValueDecimal(Mvalue* _value);

Mdecimal* _getValueTextDecimal(Mvalue* value); // MDH@09OCT2019: delegates to _getTextDecimal() in Mdecimal.h/c, guarantees to return a new decimal from parsing the value text representation (unless the value wraps a decimal itself)
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

// MDH@31MAY2019: big integer support switched from libzahl to libtommatch
#include "tommath.h"

// MDH@10JUN2019: decimal support by libmpdec
#include "mpdecimal.h"

#include "Mexpression.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void outputError(const char* const error);
void outputErrorAndText(const char* const error,const char* const text);

// defining VALUE_TYPES as an enum defining all possible value types
// VT_UNDEFINED indicates that no value is currently to be associated
typedef enum Mvaluetype {VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_REAL,VT_TEXT,VT_LIST,VT_MAP/*VT_USERFUNCTION*/}Mvaluetype;

// we define the names of 'standard' function but it is a good idea to classify them by the number of arguments

// TODO Minteger could become a union if we're storing multiple types of integers in it
typedef struct Minteger{
    long long ll; // signed 64-bit integer (for now)
}Minteger;
/*
typedef struct Mbiginteger{
    Mbiginteger* _mi;
}Mbiginteger;
*/
// TODO Mreal could become a union if we're storing multiple types of reals in it
typedef struct Mreal{
    long double ld; // double precision floating point binary number (for now)
}Mreal;

typedef mp_int Mbiginteger; // MDH@17JUN2019: use Mbiginteger the same as we would Mbiginteger, one-to-one correspondence with the structure used in libtommath 

typedef struct Mrational{
    Mbiginteger* num;
    Mbiginteger* den;
    Mreal* delta; // any deviation from the original long double
    bool normalized; // MDH@05JUN2019: remember whether or not normalized...
}Mrational;

typedef struct Mtext{
    char presuffix;
    char _c[]; // by using an array and not a pointer, it's easy to make one out of an Mtext* by strcpy from string(Mstring)
}Mtext;

// MDH@17JUN2019: if we want to know when a decimal contains repeating fractions we should be able to remember how many decimals repeat themselves
typedef struct Mdecimal{
    mpd_t* mpd; // ok, for now use a pointer
    mpd_ssize_t repeating; // the number of decimals that repeat themselves at the end
    mpd_ssize_t prec; // MDH@25AUG2019: the precision used for creating this decimal (thus allowing to automatically set the decimal precision to use in computations)
}Mdecimal;

bool isLittleEndian();

Mstring* _getUndefinedValueText();

Minteger* _getInteger(long long ll);

void free_text(Mtext* _text);
Mtext* _getText(char* _c);
Mtext* _getCharText(char _char);

////////Menvironment* getExecutionEnvironment();
void free_integer(Minteger* _integer);
void free_real(Mreal* _real);

bool strIsZero(char* str);
Mstring* appendll(Mstring* const ms,long long ll);
Mstring* appendld(Mstring* const ms,long double ld);

// MDH@20MAY2019: we need free_map to free the function argument maps!!
/* MDH@01MAY2019: we do not want helper functions to free structure pointers visible to the outside
// pointer to these structs releasers
void free_string(Mtext* _string);
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
// Mstring* is assumed to start with the same prefix/suffix character
Mtext* get_string(Mstring* s);
*/

// anybody can ask for a specific type of value (wrapping certain contents) and the pointer in it should be considered immutable i.e. Mvalue itself should be considered immutable
// NOTE this doesn't mean that 
/*
const long long M_LL_INVALID=LLONG_MIN; // the invalid long long defaults to LLONG_MIN
// it's preferable if the allowed range of integer (long long) values, does not include LLONG_MIN
const long long M_LL_MIN=LLONG_MIN+1;
const long long M_LL_MAX=LLONG_MAX;
*/

long long double2long(long double ld); // convert long double to long long

#define M_LL_INVALID LLONG_MIN // the invalid long long defaults to LLONG_MIN
// it's preferable if the allowed range of integer (long long) values, does not include LLONG_MIN
#define M_LL_MIN LLONG_MIN+1
#define M_LL_MAX LLONG_MAX

// GENERAL FUNCTIONS
Mstring* _getUint64BinaryText(uint64_t l,char presuffix);
Mstring* _getUint16BinaryText(uint16_t s,char presuffix);

Minteger* _getInteger(long long ll);

void extractMantisseAndExponent(long double ld,uint64_t *mantisse,uint16_t *exponent); // so we can also put these into the decimal representation of a double!!!
///////////long long getInteger(const Mvalue* const _value); // TODO check how this differs from getValueInteger()!!!
long double getRealLongDouble(const Mreal* const _real);
Mreal* _getReal(long double ld);

// BIG INTEGER STUFF
void free_biginteger(Mbiginteger* _biginteger);
Mbiginteger* __biginteger();
Mbiginteger* _getBigintegerCopy(Mbiginteger const * const _biginteger);
Mbiginteger* _getBiginteger(int64_t l);

Mbiginteger* _getBigintegerNeg(Mbiginteger const * const _biginteger); // NOTE there's a replicate called getNegatedBiginteger in Mbiginteger.h/c but I need it here

Mbiginteger* getBigintegerLLMin();
Mbiginteger* getBigintegerLLMax();
bool isBigintegerZero(Mbiginteger* _biginteger);
bool isBigintegerOne(Mbiginteger* _biginteger);
// big integer conversions
mp_err mp_set_long_double(Mbiginteger *a, long double b); // MDH@01MAY2019: which I made myself
long double mp_get_long_double(const Mbiginteger* const a); // MDH@07JUN2019: same here
long long biginteger2long(const Mbiginteger* const _biginteger);
mp_err mp_set_longdouble(Mbiginteger *a, long double b);
const Mbiginteger* getBigintegerOne();
const Mbiginteger* getBigintegerTwo();
const Mbiginteger* getBigintegerThree();

// RATIONAL STUFF
// some helper functions (TODO or should we use this on Mreal values?????)
bool ldIsZero(long double ld);
bool ldIsNaN(long double ld);
bool ldIsInf(long double ld);
bool ldIsPositive(long double ld);
bool ldIsNegative(long double ld);

// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
Mstring* _getIntegerText(Minteger* _integer);
Mstring* _getBigintegerText(const Mbiginteger* const _biginteger);
Mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint);
Mstring* _getRealText(Mreal* _real);
Mstring* _getStringText(Mtext* _string,bool dequoted);


void outputBiginteger(const char* const prefix,const Mbiginteger* const _biginteger,const char* const postfix);
void outputDecimal(const char* const prefix,const Mdecimal* const _decimal,const char* const postfix);

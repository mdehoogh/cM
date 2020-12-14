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
#include <time.h>

// MDH@31MAY2019: big integer support switched from libzahl to libtommatch
// MDH@13MAR2020: switched to using Mtommath.h instead of tommath.h itself because sometimes we want to use a different tommath.h (like Mtommath-develop.h)
#include "M.tommath.h"

// MDH@10JUN2019: decimal support by libmpdec
#include "mpdecimal.h"

// for heap_string_copy() to copy char* stuff
#include "Mmemory.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// defining VALUE_TYPES as an enum defining all possible value types
// VT_UNDEFINED indicates that no value is currently to be associated
// VT_REF coming up next for storing (second-level) references (main variables are the first named values)
// MDH@02NOV2020: VT_UNKNOWN added to indicate that the type is unknown in advance
// MDH@08DEC2020: VT_DATE added
typedef enum Mvaluetype {/*VT_UNKNOWN=-1,*/VT_UNDEFINED=0,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_FLOAT,VT_TEXT,VT_ARRAY,VT_LIST,VT_MAP/*VT_USERFUNCTION*/,VT_REFERENCE,VT_FUNCTION,VT_ENVIRONMENT,VT_FILE,VT_TIME}Mvaluetype;

// MDH@02NOV2020: if we can somehow define the value type to use when applying a binary operator to two values of a certain type 
//                this is in particularly applicable to numeric data in which we can predict the type of the outcome based on the type of the two values
//                NOT IMPLEMENTED YET

// we define the names of 'standard' function but it is a good idea to classify them by the number of arguments

// TODO Minteger could become a union if we're storing multiple types of integers in it
typedef struct Minteger{
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    long long ll; // signed 64-bit integer (for now)
}Minteger;
/*
typedef struct Mbiginteger{
    Mbiginteger* _mi;
}Mbiginteger;
*/
// TODO Mfloat could become a union if we're storing multiple types of reals in it
typedef struct Mfloat{
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    long double ld; // double precision floating point binary number (for now)
}Mfloat;

// MDH@09APR2020: keeping track of the allocations means we know need a pointer to an mp_int (which is internally allocated by libtommath)
//                TODO it's probably prudent to in the future ALWAYS use Mbiginteger as a separate struct because mp_int is also of variable length (dynamically)
//                     which should be managed by REALLOC somehow...
#ifndef __PRODUCTION__
typedef struct Mbiginteger{
    // t_count allocationIndex;
    mp_int* _bi;
}Mbiginteger;
#define MP_INT_POINTER(biginteger) (biginteger)->_bi
#else
    // MDH@17JUN2019: use Mbiginteger the same as we would Mbiginteger, one-to-one correspondence with the structure used in libtommath 
typedef mp_int Mbiginteger;
#define MP_INT_POINTER(biginteger) biginteger
#endif

typedef struct Mrational{
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    Mbiginteger* num;
    Mbiginteger* den;
    Mfloat* delta; // any deviation from the original long double
    bool normalized; // MDH@05JUN2019: remember whether or not normalized...
}Mrational;

// MDH@09APR2020: apart from the allocation index, if we want to know what size Mtext is (with _c being of unknown size, we need to add size unless we never allocate more than we need
//                i.e. _c always ends with '\0' so strlen(_c) actually gives us the actual length
typedef struct Mtext{
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    char presuffix;
    char _c[]; // by using an array and not a pointer, it's easy to make one out of an Mtext* by strcpy from string(Mstring)
}Mtext;

// MDH@17JUN2019: if we want to know when a decimal contains repeating fractions we should be able to remember how many decimals repeat themselves
typedef struct Mdecimal{
    mpd_t* mpd; // ok, for now use a pointer
    mpd_ssize_t repeating; // the number of decimals that repeat themselves at the end
    mpd_ssize_t prec; // MDH@25AUG2019: the precision used for creating this decimal (thus allowing to automatically set the decimal precision to use in computations)
}Mdecimal;

// MDH@08DEC2020: dealing with dates
typedef struct Mtime{
    time_t t;
    int16_t tzsec; // if tzsec equals INT16_MIN it is not set and tznindex should not be zero!!!!
    int16_t tznindex; // non-zero values indicate the timezone index, where negative values indicate a DST local time
}Mtime;

bool isLittleEndian();

Mstring* _getUndefinedValueText();

Minteger* _getInteger(long long ll);

Mtext* disowned_text(Mtext* _text,Mallocationowner owner_text);
void free_text(Mtext* _text/*,Mallocationowner owner*/);
#define FREE_TEXT(_text,owner_text) free_text(disowned_text(_text,owner_text))
Mtext* owned_text(Mtext* _text,Mallocationowner owner_text);

Mtext* _getText(char const * const _c);
Mtext* _getCharText(char _char);

// MDH@07DEC2020: convenience method to create a Mstring from a character string
Mstring* _getQuotedTextString(char const * const c,char quote); // specifically used by
Mtext* _getSingleQuotedText(char const * const text);
Mstring* _getQuotedTextCharString(char _char,char quote); // specifically used by
Mtext* _getSingleQuotedCharText(char _char);

////////Menvironment* getExecutionEnvironment();
Minteger* disowned_integer(Minteger* _integer,Mallocationowner owner_integer);
Minteger* owned_integer(Minteger* _integer,Mallocationowner owner_integer);
void free_integer(Minteger* _integer/*,Mallocationowner owner*/);
#define FREE_INTEGER(_integer,owner_integer) free_integer(disowned_integer(_integer,owner_integer))

Mfloat* disowned_float(Mfloat* _float,Mallocationowner owner_float);
Mfloat* owned_float(Mfloat* _float,Mallocationowner owner_float);
void free_float(Mfloat* _float/*,Mallocationowner owner*/);
#define FREE_FLOAT(_float,owner_float) free_float(disowned_float(_float,owner_float))

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
Mfloat* get_real(long double ld);

Mvalue* getRealValue(Mfloat* _real);
Minteger* get_integer(long long ll);
Mvalue* getIntegerValue(Minteger* _integer);
// Mstring* is assumed to start with the same prefix/suffix character
Mtext* get_string(Mstring* s);
*/

// anybody can ask for a specific type of value (wrapping certain contents) and the pointer in it should be considered immutable i.e. Mvalue itself should be considered immutable
// NOTE this doesn't mean that 
/* moved over to initializing these in M.c where all the global constants go
const long long M_LL_INVALID=LLONG_MIN; // the invalid long long defaults to LLONG_MIN
// it's preferable if the allowed range of integer (long long) values, does not include LLONG_MIN
const long long M_LL_MIN=LLONG_MIN+1;
const long long M_LL_MAX=LLONG_MAX;
*/

long long double2long(long double ld); // convert long double to long long

// GENERAL FUNCTIONS
Mstring* _getUint64BinaryText(uint64_t l,char presuffix);
Mstring* _getUint16BinaryText(uint16_t s,char presuffix);

Minteger* _getInteger(long long ll);

void extractMantisseAndExponent(long double ld,uint64_t *mantisse,uint16_t *exponent); // so we can also put these into the decimal representation of a double!!!
///////////long long getInteger(const Mvalue* const _value); // TODO check how this differs from getValueInteger()!!!
long double getFloatLongDouble(Mfloat const * const _float);
Mfloat* _getFloat(long double ld);

bool floatIsUndefined(Mfloat* afloat);
bool floatIsUndefinedOrZero(Mfloat* afloat);
// copying Mfloat's
Mfloat* _getFloatCopy(Mfloat* afloat);
Mfloat* _getFloatNeg(Mfloat* afloat);

// BIG INTEGER STUFF
// TODO should be moved over to Mbiginteger.h/c
Mbiginteger* disowned_biginteger(Mbiginteger* _biginteger,Mallocationowner owner_biginteger);
Mbiginteger* owned_biginteger(Mbiginteger* _biginteger,Mallocationowner owner_biginteger);

Mbiginteger* __biginteger();
void free_biginteger(Mbiginteger* _biginteger/*,Mallocationowner owner_biginteger*/);
#define FREE_BIGINTEGER(_biginteger,owner_biginteger) free_biginteger(disowned_biginteger(_biginteger,owner_biginteger))

Mbiginteger* _getBiginteger(int64_t l);

Mbiginteger* _getBigintegerCopy(Mbiginteger const * const _biginteger);
Mbiginteger* _getBigintegerNeg(Mbiginteger const * const _biginteger); // NOTE there's a replicate called getNegatedBiginteger in Mbiginteger.h/c but I need it here

Mbiginteger* getBigintegerLLMin();
Mbiginteger* getBigintegerLLMax();

long long isBigintegerOne(Mbiginteger* _biginteger);

// big integer conversions
mp_err mp_set_long_double(Mbiginteger *a, long double b); // MDH@01MAY2019: which I made myself
long double mp_get_long_double(const Mbiginteger* const a); // MDH@07JUN2019: same here
long long biginteger2long(const Mbiginteger* const _biginteger);
mp_err mp_set_longdouble(Mbiginteger *a, long double b);
const Mbiginteger* getBigintegerOne();
const Mbiginteger* getBigintegerTwo();
const Mbiginteger* getBigintegerThree();

long long isIntegerOne(Minteger* integer);
long long isIntegerUndefined(Minteger* integer);
long long isIntegerZero(Minteger* integer);
long long isIntegerPositive(Minteger* integer);
long long isIntegerNegative(Minteger* integer);

// direct long double functions
long double ldShift(long double ld,long long shift); // 'shifting' a double means either doubling or halving a number of times

long long isLongDoubleUndefined(long double ld); // for use in isFloatUndefined() and long double functions below...
long long getLongDoubleSign(long double ld);
long long isLongDoubleZero(long double ld);
long long isLongDoublePositive(long double ld);
long long isLongDoubleNegative(long double ld);
long long isLongDoubleOne(long double ld);

long long isFloatUndefined(Mfloat* afloat);
long long isFloatZero(Mfloat* afloat);
long long isFloatOne(Mfloat* afloat);
long long isFloatInfinite(Mfloat* afloat);
long long isFloatPositive(Mfloat* afloat);
long long isFloatNegative(Mfloat* afloat);
long long areFloatsEqual(Mfloat* float1,Mfloat* float2);

// long double stuff
/* MDH@25OCT2019: only use the ld functions internally
// some helper functions (TODO or should we use this on Mfloat values?????)
// tests that use fpclassify() directly (NOTE that ld)
bool ldIsSubnormal(long double ld);
bool ldIsSupernormal(long double ld);
bool ldIsNaN(long double ld);
bool ldIsInf(long double ld);
// any long double is considered valid when either zero or normal (so subnormals are considered invalid although they are considered zero!!)
bool ldIsInvalid(long double ld);
bool ldIsValid(long double ld);
// used in determining the sign of a real
bool ldIsZero(long double ld);
bool ldIsPositive(long double ld);
bool ldIsNegative(long double ld);
bool ldEqual(long double ld1,long double ld2);
bool ldIsOne(long double ld);
*/

// RATIONAL STUFF
// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
Mstring* _getLongLongText(long long ll);
Mstring* _getIntegerText(Minteger* _integer);
Mstring* _getTimeText(Mtime* _time);
Mstring* _getBigintegerText(const Mbiginteger* const _biginteger);
Mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint);
Mstring* _getFloatText(Mfloat* _real);
Mstring* _getStringText(Mtext* _string,bool dequoted);

long long isBigintegerUndefined(Mbiginteger* biginteger);
long long isTextUndefined(Mtext* text);
long long isTokenUndefined(Mtoken* token);

size_t outputBiginteger(const char* const prefix,const Mbiginteger* const _biginteger,const char* const postfix);
size_t outputDecimal(const char* const prefix,const Mdecimal* const _decimal,const char* const postfix);

// starting with v0.1.4 we have file I/O support
#include "sys/stat.h"

// Mfile holds all information related to a single file
typedef struct Mfile{
    struct stat* _stat;
    Mstring* _name; // if the file exists _name will contain the name of the file
    // keep track of open file attributes
    FILE* _f; // the pointer to the opened file
    off_t pos; // the current position in the file
    char mode[3]; // the '\0' terminated mode array which will contain 'r','a','w','r+','a+' or w+'
}Mfile;
Mfile* disowned_file(Mfile* _file,Mallocationowner owner_file);
Mfile* owned_file(Mfile* _file,Mallocationowner owner_file);

Mfile* __file();
void free_file(Mfile* _file);
#define FREE_FILE(_file,owner_file) free_file(disowned_file(_file,owner_file))

Mtime* owned_time(Mtime* _time,Mallocationowner owner_time);
Mtime* disowned_time(Mtime* _time,Mallocationowner owner_time);
Mtime* __time();
Mtime* _getTime(char const * const source,time_t t,int16_t tzsec,int16_t tznindex);
void free_time(Mtime* _time);
#define FREE_TIME(_time,owner_time) free_time(disowned_time(_time,owner_time))
/**
 * MDH@24JUN2019:
 * - memory safe checkup on each function based on the following guidelines:
 *   1. all local variables to point to data in dynamic storage (heap) should be declared at the start (or just after input check)
 *   2. all these local variables should be freed before leaving the function (so technically there should be one exit point)
 *   3. if the pointer contents is passed along (in)to the result of the function the pointer should be NULLed to prevent releasing the memory pointed to (which needs to persist function execution)
 *   4. preferably this is done by calling the transfer<Mtype> function that will NULL the calling pointer
 *   5. prefix the function name with _ if it returns a dynamically allocated pointer whose ownerships transfers to the caller
 *   6. prefix a function validated with the validated comment: VALIDATED@<timestamp>
 */
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#include "Mexecution.h"

static int32_t const MODULE_ID=(9<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

// externally (in M.c) defined constants
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_ZERO,M_POSITIVE,M_NEGATIVE;
extern const char* const M_ERROR_PREFIX;
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const char* const M_UNDEFINED_VALUE_TEXT; // TODO might be called M_NULL_VALUETEXT though
extern const mpd_context_t* _decimalContext;

static int8_t littleEndian=-1;
// NOTE force execution immediately (don't think this is working)
/**
 * \brief determines whether the hardware stores data in little or big endian format
 */
__attribute__((constructor)) void initExecution(){
    int i=1;
	char* c=(char*)&i;
	littleEndian=*c;
	if(amVerbose())output("On this system data is stored in %s endian format.\n",(littleEndian?"little":"big"));
}

/**
 * \brief returns true if the hardware supports little endian, false otherwise
 */
bool isLittleEndian(){
    if(littleEndian<0)initExecution();
    return(littleEndian>0);
}

// NOTE when binaryText is returned, it does not need to be freed internally, otherwise it does
/**
 * \brief determines the binary text representation of the 64-bit unsigned integer \p ul using text prefix and postfix \p presuffix
 * \param ul the 64-bit positive integer to represent in 0s and 1s
 * \param presuffix character to enquote the binary representation when displayed
 * \return the newly created Mstring containing the binary representation on success, NULL on failure
 * needs to be freed with free_string() after use (e.g. of its string representation) as indicated by the _ that starts the name
 */
Mstring* _getUint64BinaryText(uint64_t ul,char presuffix){
    Mstring* _binaryText=__string();
    if(_binaryText){
        Mstring* _p=_binaryText;
        int l=64;
        while(--l>=0&&_p){_p=string_append_char(_p,ul&1?'1':'0');ul>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
        if(presuffix)_p=string_append_char(_p,presuffix);
        if(!_p){free_string(_binaryText);_binaryText=NULL;}else string_reverse(_p);
    }
    return _binaryText;
}/* VALIDATED */
/**
 * \brief determines the binary text representation of the 16-bit unsigned integer \p us using text prefix and postfix \p presuffix
 * \param us the 16-bit unsigned integer to represent in 0s and 1s
 * \param presuffix character to enquote the binary representation when displayed
 * \return the newly created Mstring containing the binary representation on success, NULL on failure
 * needs to be freed with free_string() after use (e.g. of its string representation) as indicated by the _ that starts the name
 */
Mstring* _getUint16BinaryText(uint16_t us,char presuffix){
    Mstring* _binaryText=__string();
    if(_binaryText){
        Mstring* _p=_binaryText;
        int l=16;
        while(--l>=0&&_p){_p=string_append_char(_p,us&1?'1':'0');us>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
        if(presuffix)_p=string_append_char(_p,presuffix);
        if(!_p){free_string(_binaryText);_binaryText=NULL;}else string_reverse(_p);
    }
    return _binaryText;
}/* VALIDATED */

/* initialization for big integer arithmetic
jmp_buf env;
bool initExecution(){
    if(setjmp(env))return false;
    zsetup(env);
    return true;
}
*/

// BIG INTEGER STUFF
/** \brief __biginteger creates a new big integer (on the heap) ready to be used, if successful, NULL otherwise
 *  \return a newly created big integer
 */
void free_biginteger(Mbiginteger* biginteger){
    if(biginteger){
        if(amVerbose()&&amDebugging())outputInfo("Freeing a big integer."); // TODO can we display the value?
#ifndef __PRODUCTION__
        mp_clear(biginteger->_bi);
        FREE(biginteger->_bi,'b');
#else
        mp_clear(biginteger); // directly call mp_clear on the Mbiginteger pointer!!!
#endif
        FREE(biginteger,'B'); // MDH@15NOV2019: this is a big gamble but if I understand the library correctly this should be Ok because the big integer is allocated on the heap!!!
    }else
    if(amVerbose())outputInfo("No big integer to free!");
}/* VALIDATED */
Mbiginteger* __biginteger(){
    Mbiginteger* _biginteger=(Mbiginteger*)CALLOC(sizeof(Mbiginteger),'B');
    if(_biginteger){
#ifndef __PRODUCTION__
        _biginteger->_bi=(mp_int*)CALLOC(sizeof(mp_int),'b');
        if(_biginteger->_bi&&mp_init(_biginteger->_bi)!=MP_OKAY){FREE(_biginteger->_bi,'b');_biginteger->_bi=NULL;} // initialize the mp_int, when failing free the mp_int*
        if(!_biginteger->_bi){FREE(_biginteger,'B');_biginteger=NULL;} // if we fail to allocate and/or initialize an mp_int dynamically, get rid of the biginteger too
#else
        if(mp_init((mp_int*)_biginteger)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;} // ESSENTIAL to release the big integer, when failing to initialize it!!
#endif
    }
    return _biginteger;
}/* VALIDATED */
// end of block that uses __PRODUCTION__ flag

// MDH@09APR2020: for all methods that call mp_int methods now require calling MP_INT_POINTER() on Mbiginteger instances
Mbiginteger* _getBiginteger(int64_t ll){
    Mbiginteger* biginteger=__biginteger();
    // MDH@09APR2020: in the non-production version we're keeping track of the allocations and get_mpint on Mbiginteger will return what is required
    if(biginteger)mp_set_i64(MP_INT_POINTER(biginteger),ll); // even if l equals 0 set it TODO check is that necessary???
    return biginteger;
}/* VALIDATED */

// replace in due course by _getBigintegerNeg in Mbiginteger.c/h but that would require moving _getRational and some other functions as well from Mexecution.h/c
Mbiginteger* _getBigintegerNeg(Mbiginteger const * const _biginteger){
    Mbiginteger* _bigintegerNeg=(_biginteger?__biginteger():NULL); // the result we will be returning
    if(_bigintegerNeg&&mp_neg(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_bigintegerNeg))!=MP_OKAY){free_biginteger(_bigintegerNeg);_bigintegerNeg=NULL;}
    return _bigintegerNeg;
}// VALIDATED

// pass in NULL to _getBigIntegerCopy to get a big integer (initialized to zero)
Mbiginteger* _getBigintegerCopy(Mbiginteger const * const biginteger){
    Mbiginteger* bigintegerCopy=(biginteger?__biginteger():NULL);
    if(bigintegerCopy&&mp_copy(MP_INT_POINTER(biginteger),MP_INT_POINTER(bigintegerCopy))!=MP_OKAY){free_biginteger(bigintegerCopy);bigintegerCopy=NULL;}
    return bigintegerCopy;
}/* VALIDATED */

// using constant big integers 0, 1 and 2 (do NOT wrap these constants in Mvalue's though or they will need to be created over and over again)
static Mbiginteger *bi0=NULL,*bi1=NULL,*bi2=NULL,*bi3=NULL;
// NOTE do NOT start with underscore (_) to indicate that the result is to be left alone!!
const Mbiginteger* getBigintegerZero(){if(!bi0)bi0=_getBiginteger(0);return bi0;}/* VALIDATED */
const Mbiginteger* getBigintegerOne(){if(!bi1)bi1=_getBiginteger(1);return bi1;}/* VALIDATED */
const Mbiginteger* getBigintegerTwo(){if(!bi2)bi2=_getBiginteger(2);return bi2;}/* VALIDATED */
const Mbiginteger* getBigintegerThree(){if(!bi3)bi3=_getBiginteger(3);return bi3;}/* VALIDATED */

long long isBigintegerOne(Mbiginteger* biginteger){
    return(biginteger?(mp_cmp(MP_INT_POINTER(biginteger),MP_INT_POINTER(getBigintegerOne()))==MP_EQ?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
// END BIG INTEGER STUFF

/////////mp_int* __mp_int(){return (mp_int*)MALLOC(sizeof(mp_int),'I');}
// long double to rational or representation
typedef struct {
    uint64_t mantisse;
    uint16_t exponent;
} littleEndianLongDouble;
typedef struct {
    uint16_t exponent;
    uint64_t mantisse;
} bigEndianLongDouble;
typedef union {
    long double ld;
    littleEndianLongDouble lELD;
} littleEndianLongDoubleUnion;
typedef union {
    long double ld;
    bigEndianLongDouble bELD;
} bigEndianLongDoubleUnion;
void extractMantisseAndExponent(long double ld,uint64_t *mantisse,uint16_t *exponent){
    if(isLittleEndian()){
        littleEndianLongDoubleUnion lELDU;
        lELDU.ld=ld;
        *mantisse=lELDU.lELD.mantisse;
        *exponent=lELDU.lELD.exponent;
    }else{
        bigEndianLongDoubleUnion bELDU;
        bELDU.ld=ld;
        *mantisse=bELDU.bELD.mantisse;
        *exponent=bELDU.bELD.exponent;
    }
}/* VALIDATED */

///////Mstring* _getBigintegerText(const Mbiginteger* const _biginteger); // prototype declaration

/* replacing:
Mrational* _getLongDoubleRational(long double ld){
    // a non zero long double
    uint16_t exponent;
    uint64_t mantisse;
    extractMantisseAndExponent(ld,&mantisse,&exponent);
    int32_t exp=(exponent&0x7FFF);
    // if exponent is all ones, the long double represents a NaN or Inf
    if(exp==0x7FFF){if(amVerbose())output("Can't convert an invalid or infinite real to a rational!");return NULL;}
    // if exponent is all zeroes, the long double represents zero
    Mbiginteger* _numerator=__biginteger();
    if(exp==0)return _getRational(_numerator,NULL,false);
    // set the numerator to the mantisse (which luckily is uint64_t)
    mp_set_u64(_numerator,mantisse);
    exp-=0x403E; // determine the 'true' exponent
    // if the true exponent is negative, the multiplier is below 1 and cannot be used as denominator, instead the numerator should be multiplied by 2 to the power -exp
    if(exp<0){
        Mbiginteger* _shiftedout=__biginteger();
        mp_err err=mp_div_2d(_numerator,-exp,_numerator,_shiftedout);
        if(err!=MP_OKAY){
            free_biginteger(_shiftedout);
            outputError("Failed to adjust the numerator of the rational by the negative exponent of the real.");
            free_biginteger(_numerator);
            return NULL;
        }
        // we may assume that what got shifted out fits in an uint64_t
        uint64_t shiftedout=mp_get_u64(_shiftedout);Mstring* _shiftedoutText=_getUint64BinaryText(shiftedout,'\0');output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));free_string(_shiftedoutText);
        // replacing: Mstring* _shiftedoutText=_getBigintegerText(_shiftedout);output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));free_string(_shiftedoutText);
        free_biginteger(_shiftedout);
    }
    // make the numerator negative if the long double is negative (this is when bit 15 of the exponent equals 1)
    if(exponent>>15)if(mp_neg(_numerator,_numerator)!=MP_OKAY){outputError("Failed to negate the rational of the real.");free_biginteger(_numerator);return NULL;}
    // if the exponent is non-positive (zero or negative) there's no denominator (i.e. denominator remains 1)
    if(exp<=0)return _getRational(_numerator,NULL,false);
    // ASSERT a positive exponent that we can use for the denominator
    Mbiginteger* _denominator=_getBiginteger(1);
    if(!_denominator||mp_mul_2d(_denominator,exp,_denominator)!=MP_OKAY){outputError("Failed to compute the denominator of the rational of a real.");free_biginteger(_denominator);return NULL;}
    return _getRational(_numerator,_denominator,true);
}
*/
// RELEASERS
// however we can only NULL them if we have the address of the pointer)
// but if these pointer are local to a function (which they will be typically if they are to be released in the first place) no NULLing is required!!!
void free_text(Mtext* _text){
    // MDH@15NOV2019: text is now created using the _strdup() function which will manage the dynamic memory of the static text allocation
    // MDH@07APR2020: BUT the problem is that currently _text is NOT under allocation control TODO we should fix that somehow...
    //                ok, changed _strdup to call MALLOC() and use memcpy to copy the characters over
    if(_text){
        // MDH@09APR2020: from now on use REALLOC instead of FREE for anything with variable dynamic memory allocation
        //                _text->_c is an array and yes strlen() can be applied to any char*
        //                TODO let me think, should I use sizeof(Mtext), I suppose so assuming it will also include allocation_index (if present)
        REALLOC(_text,strlen(_text->_c)+sizeof(Mtext),0,sizeof(char),'"');
        // replacing: FREE(_text,'"'); // replacing (when we used a char pointer (_m) for storing the characters): if(_string){if(_string->_m)free_string(_string->_m);_string->_m=NULL;free(_string);}
    }else
    if(amDebugging())
        outputInfo("No text to free!");
}/* VALIDATED */
void free_integer(Minteger* _integer){
    if(_integer){
        if(amVerbose()&&amDebugging())output("Freeing integer %llu.\n",_integer->ll);
        FREE(_integer,'I');
    }else
    if(amDebugging())outputInfo("No integer to free!");
}/* VALIDATED */
void free_float(Mfloat* _float){
    if(_float){
        if(amVerbose()&&amDebugging())output("Freeing real %.*Lf.\n",LDBL_DIG,_float->ld);
        FREE(_float,'F');
    }else
    if(amDebugging())outputInfo("No real to free!");
}/* VALIDATED */

// ALLOCATORS (private)
// value wrappers
// typically an Mvalue is immutable (we might change that for variables that are strong typed e.g. when created with integer(),real(),string(),list() or map() function)
Minteger* _getInteger(long long ll){
    Minteger* _integer=MALLOC(sizeof(Minteger),'I');
    if(_integer)_integer->ll=ll;
    return _integer;
}/* VALIDATED */
/*
Mbiginteger*__biginteger(z_t zt){
    Mbiginteger* _biginteger=malloc(sizeof(Mbiginteger));
    if(_biginteger){
        zset(_biginteger->bi,zt);
    }
    return _biginteger;
}
*/
long double getFloatLongDouble(Mfloat const * const _float){return(_float?_float->ld:M_LD_NAN);}

Mfloat* _getFloat(long double ld){
    Mfloat* _float=MALLOC(sizeof(Mfloat),'F'); // change MALLOC to also allow passing in the number of items, although the production version doesn't care!!!!
    if(_float)_float->ld=ld;
    return _float;
}/* VALIDATED */

// special integer values to consider
long long isIntegerOne(Minteger* integer){return(integer?(integer->ll==1?M_TRUE:M_FALSE):M_LL_INVALID);}
// DISCUSSION it's debatable whether a NULL integer can be tested as it is NULL but isValueUndefined() will prevent call isIntegerUndefined() with a NULL pointer!!!
long long isIntegerUndefined(Minteger* integer){return(integer?(integer->ll==M_LL_INVALID?M_TRUE:M_FALSE):M_TRUE);}
long long isIntegerZero(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll==0?M_TRUE:M_FALSE));}
long long isIntegerPositive(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll>0?M_TRUE:M_FALSE));}
long long isIntegerNegative(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll<0?M_TRUE:M_FALSE));}

// long double support functions (only for internal use)
const char* M_NAN="NaN";
const char* M_INF="Inf";

#ifndef FP_SUPERNORMAL
#define FP_SUPERNORMAL 6
#endif

// functions that operate purely on long doubles
// MDH@25OCT2019: subnormal numbers are considered zero
bool ldIsNaN(long double ld){return fpclassify(ld)==FP_NAN;}/* VALIDATED */
bool ldIsInf(long double ld){return fpclassify(ld)==FP_INFINITE;}/* VALIDATED */
// MDH@25OCT2019: a long double is invalid if it is not zero and not normal
//                NOTE a subnormal number is considered invalid but will be treated as a zero (i.e. ldIsZero() will return true on a subnormal number)
bool ldIsSubnormal(long double ld){return fpclassify(ld)==FP_SUBNORMAL;}/* VALIDATED */
bool ldIsSupernormal(long double ld){return fpclassify(ld)==FP_SUPERNORMAL;}/* VALIDATED */
bool ldIsZero(long double ld){return(fpclassify(ld)==FP_ZERO);}/* VALIDATED */

bool ldIsInvalid(long double ld){return(fpclassify(ld)!=FP_NORMAL&&fpclassify(ld)!=FP_ZERO);}
bool ldIsValid(long double ld){return(fpclassify(ld)==FP_NORMAL||fpclassify(ld)==FP_ZERO);}

// MDH@18OCT2019: lettting the comparison take care of the result!!!
// MDH@25OCT2019: consider subnormal long doubles to be zero (to test BEFORE calling ldIsValid)
// replaced by isLongDoubleZero (see below)... bool ldIsZero(long double ld){return(fpclassify(ld)==FP_SUBNORMAL?true:(ldIsValid(ld)?ld==0?false));}/* VALIDATED */
bool ldIsPositive(long double ld){return(ldIsValid(ld)?ld>0:false);}/* VALIDATED */
bool ldIsNegative(long double ld){return(ldIsValid(ld)?ld<0:false);}/* VALIDATED */
/* MDH@25OCT2019: replaced by the ...LongDouble... functions
bool ldEqual(long double ld1,long double ld2){
    if(ldIsNaN(ld1)&&ldIsNaN(ld2))return true;
    if(ldIsNaN(ld1)||ldIsNaN(ld2))return false;
    // ASSERT both not NaN
    if(ldIsInf(ld1)&&ldIsInf(ld2)&&signbit(ld1)==signbit(ld2))return true; // both infinite with the same sign
    if(ldIsInf(ld1)||ldIsInf(ld2))return false; // either is infinite or both with a different sign
    // ASSERT both not NaN and not Inf
    return(ld1==ld2);
}
bool ldIsOne(long double ld){return(ldIsValid(ld)?false:ld==1);}
*/
// end long double support functions

// special real values
// long double helper functions
// long doubles that classify as either subnormal or zero are considered zero, as well as all normal long doubles that equal zero
// testing for zero is quite complicated, as theoretically it should always return either M_TRUE or M_FALSE, but we decide to return M_LL_INVALID if we cannot determine what the number is
// 
long long isLongDoubleUndefined(long double ld){ // returns M_TRUE or M_FALSE (never M_LL_INVALID)
    // if the sign has any meaning we should return M_TRUE otherwise M_FALSE
    switch(fpclassify(ld)){
        case FP_NAN:return M_TRUE;
        case FP_INFINITE:return M_FALSE;
        case FP_ZERO:return M_FALSE;
        case FP_NORMAL:return M_FALSE;
        case FP_SUBNORMAL:return M_FALSE;
        case FP_SUPERNORMAL:return M_TRUE;
    }
    return M_LL_INVALID; // should never happen though (if we have all possible values covered!!!)
}
long long isLongDoubleZero(long double ld){
    if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
    switch(fpclassify(ld)){
        case FP_INFINITE:return M_FALSE;
        case FP_SUBNORMAL:case FP_ZERO:return M_TRUE;
        default:break;
    }
    return(ld==0);
}
long long isLongDoublePositive(long double ld){
    if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
    switch(fpclassify(ld)){
        case FP_INFINITE:return(signbit(ld)?M_FALSE:M_TRUE);
        case FP_SUBNORMAL:case FP_ZERO:return M_FALSE;
        default:break;
    }
    return(ld>0);   
}
long long isLongDoubleNegative(long double ld){
    if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
    switch(fpclassify(ld)){
        case FP_INFINITE:return(signbit(ld)?M_TRUE:M_FALSE);
        case FP_SUBNORMAL:case FP_ZERO:return M_FALSE;
        default:break;
    }
    return(ld<0);   
}
long long isLongDoubleOne(long double ld){
    if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
    switch(fpclassify(ld)){
        case FP_INFINITE:case FP_SUBNORMAL:case FP_ZERO:return M_FALSE;
        default:break;
    }
    return(ld==1);
}

// testing for special values TODO we need to make M functions to test for these special values like zero, inf, and undefined
// if a real is undefined, testing for a specific value or sign does not make any sense
// isFloatUndefined() always returns either M_TRUE or M_FALSE (never M_LL_INVALID)
long long isFloatUndefined(Mfloat* afloat){return(afloat?isLongDoubleUndefined(afloat->ld):M_TRUE);} // a real is undefined if it is NULL or the contained long double is undefined i.e. is NaN
// use isFloatUndefined() first in the following specific functions
// in general for undefined reals we cannot determine the sign, therefore one should test for undefined first, of course one can test for invalid result of the comparison of course
long long isFloatZero(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleZero(afloat->ld));}
long long isFloatPositive(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoublePositive(afloat->ld));}
long long isFloatNegative(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleNegative(afloat->ld));}
long long isFloatOne(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleOne(afloat->ld));}
long long isFloatInfinite(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:(ldIsInf(afloat->ld)?M_TRUE:M_FALSE));}
long long areFloatsEqual(Mfloat* afloat1,Mfloat* afloat2){
    // TODO we are considering two NULL values equal for now although that's questionable
    if(!afloat1&&!afloat2)return M_TRUE; // both NULL
    if(!afloat1||!afloat2)return M_FALSE; // only one of them NULL
    // ASSERT neither NULL
    if(fpclassify(afloat1->ld)!=fpclassify(afloat2->ld))return M_FALSE; // if they classify differently definitely not the same
    switch(fpclassify(afloat1->ld)){
        case FP_NAN:return M_TRUE; // both NaN TODO check for signbit as well here?????
        case FP_INFINITE:return(signbit(afloat1->ld)==signbit(afloat2->ld)?M_TRUE:M_FALSE); // both infinite but perhaps the wrong sign
        case FP_ZERO:return M_TRUE; // both zero or subnormal zero TODO check for signbit as well here?????
        case FP_SUBNORMAL:case FP_SUPERNORMAL:return M_LL_INVALID; // can't tell
        default:break;
    }
    return(afloat1->ld==afloat2->ld);
}
// TODO for now leave these two methods return a boolean, although we should decide whether or not they are derived or not
bool floatIsUndefined(Mfloat* afloat){return(!afloat||ldIsNaN(afloat->ld));}
bool floatIsUndefinedOrZero(Mfloat* afloat){return(!afloat||ldIsNaN(afloat->ld)||isFloatZero(afloat)==M_TRUE);}

Mfloat* _getFloatCopy(Mfloat* afloat){return(!floatIsUndefined(afloat)?_getFloat(afloat->ld):NULL);}/* VALIDATED */ // only when not undefined return a copy (even when zero), NULL otherwise
Mfloat* _getFloatNeg(Mfloat* afloat){return(!floatIsUndefined(afloat)?_getFloat(-afloat->ld):NULL);}/* VALIDATED */ // just switching the sign of what _getRealCopy returns

long long isBigintegerUndefined(Mbiginteger* biginteger){return(biginteger?M_FALSE:M_TRUE);}
long long isTextUndefined(Mtext* text){return(text?M_FALSE:M_TRUE);}
long long isTokenUndefined(Mtoken* token){return(token?M_FALSE:M_TRUE);}

// Mtext is an immutable version of Mstring* in that it cannot be changed
Mtext* _getText(char* _text){ // _text assumed to be string(Mstring*), so we can simply copy it over with the starting quote character (" or ')
    return (Mtext*)_strdup(_text);
}/* VALIDATED */

Mtext* _getCharText(char c){ // _text assumed to be string(Mstring*), so we can simply copy it over with the starting quote character (" or ')
    Mtext* _charText=NULL;
    Mstring* _charString=_getString("\"");
    if(_charString){
        if(string_append_char(_charString,c))_charText=_getText(string(_charString));
        free_string(_charString);
    }
    return _charText;
}/* VALIDATED */
/*
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
*/


/*
// helper functions
// helper functions to wrap literals for storage in M
Mfloat* get_real(long double ld){
	Mfloat* _real=(Mfloat*)malloc(sizeof(Mfloat));
	if(_real)_real->ld=ld;
	return _real;
}
Mvalue* getRealValue(Mfloat* _real){
    if(_real){
        Mvalue* _realValue=calloc(1,sizeof(Mvalue));
        if(_realValue){_realValue->type=VT_FLOAT;_realValue->value._float=_real;return _realValue;}
    }else
        printf("\nERROR: No real to wrap.");  
    return NULL;
}

Minteger* get_integer(long long ll){
	Minteger* _integer=(Minteger*)malloc(sizeof(Minteger));
	if(_integer)_integer->ll=ll;
	return _integer;
}
Mvalue* getIntegerValue(Minteger* _integer){
    if(_integer){
        Mvalue* _integerValue=calloc(1,sizeof(Mvalue));
        if(_integerValue){
            _integerValue->type=VT_INTEGER;
            _integerValue->value._integer=_integer;
            return _integerValue;
        }
    }else
        printf("\nERROR: No integer to wrap.");  
    return NULL;
}

// Mstring* is assumed to start with the same prefix/suffix character
Mtext* get_string(Mstring* s){
	Mtext* _string=(s?(Mtext*)malloc(sizeof(Mtext)):NULL);
	if(_string){
		_string->presuffix=string_char(s,0);
		_string->_m=__string();
		if(_string->_m==NULL)return NULL;
		string_append(_string->_m,string_remainder(s,1)); // NOTE string_remainder might return NULL of course
	}
	return _string;
}
// end helper functions

Mvalue* getListValueAtIndex(Menvironment* _environment,const char* name,Mvalue* _indexValue){
    if(_environment&&name){
        Mvariable* _variable=getVariable(_environment,name,amVerbose());
        if(_variable){
            Mvalue* _variableValue=_variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
            if(_variableValue){
                if(_variableValue->type==VT_LIST){ // yes a list we can look into
                    Mlist* _list=_variableValue->value._list;
                    return getValueAtIndex(_list,_indexValue->value._integer->ll);
                }
            }
        }
    }
    return NULL;
}
*/
/*
// let's start with some simple assignments
bool setValueOfIntegerVariable(Mvariable* _variable,Minteger* _integer){
    if(_variable){        
        Mvalue* _integerValue=getIntegerValue(_integer);
        if(_integerValue){
            free_value(_variable->_value); // free current value
            _variable->_value=_integerValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store integer '%llu' in variable '%s'.",_integer->ll,_variable->_name);
    }
    return false;
}
bool setValueOfRealVariable(Mvariable* _variable,Mfloat* _real){
    if(_variable){        
        Mvalue* _realValue=getRealValue(_real); // wrap in value
        if(_realValue){
            free_value(_variable->_value); // release the current value
            _variable->_value=_realValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store real '%Lf' in variable '%s'.",_real->ld,_variable->_name);
    }
    return false;
}
bool setValueOfStringVariable(Mvariable* _variable,Mtext* _string){
    if(_variable){        
        Mvalue* _stringValue=getStringValue(_string); // wrap in value
        if(_stringValue){
            free_value(_variable->_value); // free current value
            _variable->_value=_stringValue; // store new value
            return true;
        }
        printf("\nERROR: Failed to store text '%s' in variable '%s'.",string(_string->_m),_variable->_name);
   }
   return false;
 }
*/
/*
Minteger* _getInteger(long long ll){
    Minteger* integer=MALLOC(sizeof(Minteger),'i');if(integer)integer->ll=ll;return integer;
}
*/

Mstring* appendull(Mstring* const ms,unsigned long long ull){return string_append_ull(ms,ull);}/* VALIDATED */
Mstring* appendll(Mstring* const ms,long long ll){return string_append_ll(ms,ll);}/* VALIDATED */
Mstring* appendld(Mstring* const ms,long double ld){return string_append_ld(ms,ld);}/* VALIDATED */

// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
Mstring* _getIntegerText(Minteger* _integer){
	Mstring* _s=__string();
    if(_s){
        Mstring* _p=_s;
        if(amDebugging())_p=string_append_char(_p,'i');
	    if(_p&&_integer)_p=appendll(_p,_integer->ll);
        if(!_p){free_string(_s);_s=NULL;}
    }
	////////if(amVerbose())output("Integer '%s'.",string(s));
	return _s;
}/* VALIDATED */
/* replaced by getValueInteger()
long long getInteger(Mvalue* _value){
    if(_value)
    switch(_value->type){
        case VT_INTEGER:return _value->value._integer->ll;
        case VT_BIGINTEGER:
			if(mp_cmp(_value->value._biginteger,getBigintegerLLMin())!=MP_LT&&mp_cmp(_value->value._biginteger,getBigintegerLLMax())!=MP_GT)
                return mp_get_i64(_value->value._biginteger);
            break;
		case VT_FLOAT:return double2long(_value->value._float->ld);
		case VT_STRING:return _strtoll(_value->value._string->_c,M_LL_INVALID);
		default:break;
    }
    return M_LL_INVALID;
}
*/

// BigInteger stuff
// MDH@09APR2020: certain functions only know the mp_int* and not the big integer
static Mstring* _getMpintText(mp_int const * const _mpint){
    Mstring* _mpintText=__string();
    ////outputChar('A');
    if(_mpintText){
        /// output("Initial big integer text length: %zu.\n",_bigintegerText->length);
        ///outputChar('B');
        if(_mpint){
            // determine the required size
// MDH@13MAR2020: this is unfortunate because I would have wanted to solve everything with tommath.h
#ifdef M_MP_DEVELOP
            size_t arepsize=0; // MDH@13MAR2020: changing type int to size_t (which is larger), so that we should be able to use it with libtommath-develop
#else
            int arepsize=0;
#endif
            ///outputChar('C');
            clock_t then=clock();
            ///////if(amVerbose())outputInfo("Determining a big integer text representation.");
            // output("Big integer text length: %zu.\n",_bigintegerText->length);
            // output("Before calling mp_radix_size: ");Mstring* str_info=_string_info(_bigintegerText);output("Big integer text info: '%s'.\n",string(str_info));free_string(str_info);
            if(mp_radix_size(_mpint,10,&arepsize)==MP_OKAY){
#ifdef M_MP_DEVELOP
                // if(arepsize>0)output("Length of big integer text representation: %zu.\n",arepsize-1);
                if(arepsize<=SIZE_MAX){
#else
                // if(arepsize>0)output("Length of big integer text representation: %d.\n",arepsize-1);
                if(arepsize<=INT_MAX){
#endif
                // output("After calling mp_radix_size: ");Mstring* str_info=_string_info(_bigintegerText);output("Big integer text info: '%s'.\n",string(str_info));free_string(str_info);
                // outputChar('D');
                // if(amVerbose()&&amDebugging())
                    // outputChar('E');
                    // MDH@13MAR2020: arepsize actually includes the '\0' character at the end of any text representation which means that the text itself is one byte shorter
                    //                however string_synclength() didn't like the length being set to arepsize-1 I suppose because there would be no '\0' at that position in the text
                    //                as mp_toradix would write
                    uint8_t failure=0;
                    if(string_setlength(_mpintText,arepsize)){
                        if(mp_toradix(_mpint,_mpintText->_chars->chars,10)==MP_OKAY){ // MDH@17APR2020: TODO we should NOT actually use the internal structure of Mstring here!!
                            if(string_synclength(_mpintText)){
                                size_t trailingZeroCount=string_trailing(_mpintText,'0');
                                if(trailingZeroCount>=3){
                                    if(string_shorten(_mpintText,trailingZeroCount)){ // 'remove' the trailing zeroes
                                        if(string_append_char(_mpintText,'e')){
                                            if(!string_append_ll(_mpintText,trailingZeroCount))
                                                failure=6;
                                        }else
                                            failure=5;
                                    }else
                                        failure=4;
                                }
                            }else
                                failure=3;
                        }else
                            failure=2;
                    }else
                        failure=1;
                    if(failure>0){
                        free_string(_mpintText);_mpintText=NULL;
                        switch(failure){
                            case 1:output("%sFailed to initialize the length of the big integer text representation to %d.",M_ERROR_PREFIX,arepsize);break;
                            case 2:outputError("Failed to determine the big integer representation");break;
                            case 3:outputError("Failed to sync the length of the big integer representation");break;
                            case 4:outputError("Failed to remove the trailing zeroes from the big integer representation.");break;
                            case 5:outputError("Failed to append 'e' to the big integer representation.");break;
                            case 6:outputError("Failed to append the exponent part to the big integer representation.");break;
                        }
                    }
                }else
                    output("%sCan't store more than %u characters in a string.\n",M_ERROR_PREFIX,SIZE_MAX);
            }else
                outputError("Couldn't determine the size of a big integer");
            if(amVerboseDebugging())
                output("Determining the big integer representation took %lld ms.\n",(clock()-then)/1000);
        }else
            outputError("No big integer to represent");
    }else
        output("%sFailed to create a text for storing the representation of a big integer.\n",M_ERROR_PREFIX);
    ///outputChar('H');
    return _mpintText;
}
Mstring* _getBigintegerText(const Mbiginteger* _biginteger){
    if(!_biginteger)return __string();
    return _getMpintText(MP_INT_POINTER(_biginteger));
}

Mbiginteger *_biLLMin=NULL,*_biLLMax=NULL;

Mbiginteger* getBigintegerLLMin(){
    if(!_biLLMin){
        if(amVerboseDebugging())
            outputInfo("Determining the big integer equivalent of the smallest small integer.");
        _biLLMin=_getBiginteger(M_LL_MIN);
        if(amVerboseDebugging())
            outputBiginteger("Smallest valid small integer '",_biLLMin,".\n");
    }
    return _biLLMin;
}/* VALIDATED */
Mbiginteger* getBigintegerLLMax(){
    if(!_biLLMax){
        if(amVerboseDebugging())
            outputInfo("Determining the big integer equivalent of the largest small integer.");
        _biLLMax=_getBiginteger(M_LL_MAX);
        if(amVerboseDebugging())
            outputBiginteger("Largest valid small integer '",_biLLMax,".\n");
    }
    return _biLLMax;
}/* VALIDATED */

// MDH@01JUN2019: my own version of converting a (IEEE754 extended precision) long double to a big integer 
/*
#  define MP_ZERO_DIGITS(mem, digits)                   \
do {                                                    \
   int zd_ = (digits);                                  \
   mp_digit* zm_ = (mem);                               \
   while (zd_-- > 0) {                                  \
      *zm_++ = 0;                                       \
   }                                                    \
} while (0)
typedef unsigned __int128 uint128_t;
const uint128_t ONE128=1;
void mp_set_u128(Mbiginteger* a,uint128_t b){
    int i=0;
    while(b!=0u){
        a->dp[i++]=((mp_digit)b&MP_MASK);
        if(128<=MP_DIGIT_BIT)break;
        b>>=((128<=MP_DIGIT_BIT)?0:MP_DIGIT_BIT);
    }
    a->used=i;
    a->sign=MP_ZPOS;
    MP_ZERO_DIGITS(a->dp+a->used,a->alloc-a->used);
}
*/

// we can use the method below to come up with the numerator and denominator of a given double that matches the double exactly
// for dealing with long double to big integer conversion
// MDH@17JUN2019: although a typically is of type Mbiginteger, for a function that starts with mp_ we can use the primitive type mp_int instead of the alias Mbiginteger
mp_err mp_set_me(mp_int* a,uint64_t mantisse,uint16_t exponent){
    if(!a)return MP_ERR;
    int32_t exp=(exponent&0x7FFF); // cut off the sign
    if(exp==0x7FFF)return MP_VAL; // +-inf, NaN
    if(exp!=0){
        mp_set_u64(a,mantisse);
        exp-=0x403E; // same as exp-=(16383+63); // the actual exponent (as 63 out of 64 mantisse bits are 'significant', bit 63 equals 1 for normalized numbers) 
        if(exp!=0){
            mp_err err=(exp>0?mp_mul_2d(a,exp,a):mp_div_2d(a,-exp,a,NULL));
            if(err!=MP_OKAY)return err;
        }
        if(exponent>>15&&mp_iszero(a)==MP_NO)a->sign=MP_NEG;
    }else // all zeros in exponent
        mp_zero(a);
    return MP_OKAY;
}/* VALIDATED */
mp_err mp_set_me_verbose(mp_int* a,uint64_t mantisse,uint16_t exponent){
    if(!a)return MP_ERR;
    int32_t exp=(exponent&0x7FFF); // cut off the sign
    if(exp!=0){
        mp_set_u64(a,mantisse);
        if(amVerbose()){
            Mstring* _mantisseBigIntegerText=_getMpintText(a); // MDH@09APR2020: ask _getMpintText(), replacing _getBigintegerText()
            output("Value after setting the fraction: %s.\n",string(_mantisseBigIntegerText));
            free_string(_mantisseBigIntegerText);
        }
        if(amVerbose())output("Long double exponent part: %d - mantisse: %llu.\n",exp,mantisse);
        if(exp==0x7FFF){if(amVerbose())output("NOTE: Cannot convert an invalid or infinite real value to a big integer.");return MP_VAL;} // +-inf, NaN
        exp-=0x403E; // same as exp-=(16383+63); // the actual exponent (as 63 out of 64 mantisse bits are 'significant', bit 63 equals 1 for normalized numbers) 
        //////////frac=(frac<<1)>>1;/// replacing: &0x7FFFFFFFuLL; // I have to cut off bit 63
        if(amVerbose())output("Power of two exponent: %d.\n",exp);  
        if(exp!=0){
            mp_err err=(exp>0?mp_mul_2d(a,exp,a):mp_div_2d(a,-exp,a,NULL));
            if(err!=MP_OKAY){outputError("Failed to use the exponent of a real value in the conversion to a big integer");return err;}
        }
        if(amVerbose()){
            Mstring* _bigIntegerText=_getMpintText(a); // MDH@09APR2020
            output("Value after applying the exponent: %s.\n",string(_bigIntegerText));
            free_string(_bigIntegerText);
        }
        if(exponent>>15){ // negative
            // take over the sign from the long double (bit 15 in the signandexponent part)
            if(mp_iszero(a)==MP_NO){ // TODO preferable NOT to use used directly!!
                a->sign=MP_NEG;
                if(amVerbose())output("Sign part of real used to set the sign of the big integer.\n");
            }else
                if(amVerbose())output("No need to set the sign on a big integer equal to zero.\n");           
        }
    }else // all zeros in exponent
        mp_zero(a);
    return MP_OKAY;
}/* VALIDATED */
mp_err mp_set_longdouble(Mbiginteger *a, long double b){
    // always assume 10-byte long double (extended precision)
    uint64_t mantisse;
    uint16_t exponent; // including bit 63
    extractMantisseAndExponent(b,&mantisse,&exponent);
    // determine the sign, and the 15-bit power of two exponent
    ////////return mp_set_me_verbose(a,mantisse,exponent); 
    return (amVerbose()?mp_set_me_verbose(MP_INT_POINTER(a),mantisse,exponent):mp_set_me(MP_INT_POINTER(a),mantisse,exponent));
    /*
    if(sizeof(long double)==16){
        int exp;
        mp_err err;
        union {
            long double dbl;
            uint128_t bits;
        } cast;
        cast.dbl=b;
        exp=(int)((unsigned)(cast.bits>>112)&0x7FFFu); // get rid of mantisse and sign
        uint128_t frac=(cast.bits&((ONE128<<112)-1uLL))|(ONE128<<112);
        if(exp==0x7FFF)return MP_VAL; // +-inf, NaN
        exp-=16383+112;
        mp_set_u128(a,frac);
        err=(exp<0)?mp_div_2d(a,-exp,a,NULL):mp_mul_2d(a,exp,a);
        if(err!=MP_OKAY)return err;
        if(((cast.bits>>127)!=0uLL)&&!isBigintegerZero(a))a->sign=MP_NEG;
        return MP_OKAY;
    }
    if(sizeof(long double)==10){
        uint64_t frac;
        int exp;
        mp_err err;
        union {
            long double dbl;
            ldintparts ldints;
        } cast;
        cast.dbl=b;
        if((cast.ldints.mantisse>>63)==0){ // a normalized number
            exp=(int)((unsigned)(cast.ldints.signandexponent)&0x7FFFu);
            if(exp==0x7FFF){if(amVerbose())output("NOTE: Cannot convert an invalid or infinite real value to a big integer.");return MP_VAL;}; // +-inf, NaN
            exp-=16383+63; // 63 out of 64 mantisse bits are 'significant', bit 63 equals 1 for normalized numbers    
            frac=(cast.ldints.mantisse<<1)>>1;/// replacing: &0x7FFFFFFFuLL; // I have to cut off bit 63
            mp_set_u64(a,frac);
            if(amVerbose())output("Fraction part %16x used to initialize the big integer.",frac);
            err=(exp<0?mp_div_2d(a,-exp,a,NULL):mp_mul_2d(a,exp,a));
            if(err!=MP_OKAY){outputError("Failed to use the exponent of a real value in the conversion to a big integer.");return err;}
            // take over the sign from the long double (bit 15 in the signandexponent part)
            if(((cast.ldints.signandexponent>>15)!=0uLL)&&!isBigintegerZero(a))a->sign=MP_NEG;
            if(amVerbose())output("Sign part of real used to set the sign of the big integer.");
        }else // a denormalized number, which all map to zero!!
            mp_zero(a); // NOTE it probably is already zero!!
        return MP_OKAY;
    }else
    if(sizeof(long double)==8){
        if(amVerbose())output("NOTE: Long double has same size as a double!");
        return mp_set_double(a,(double)b);
    }
     if(amVerbose())output("The size of a long double is %u.",sizeof(long double));
    return MP_VAL;
   */
}/* VALIDATED */
// MDH@07JUN2019: based on mp_get_double in libtommath:
/*
double mp_get_double(const Mbiginteger *a)
{
   int i;
   double d = 0.0, fac = 1.0;
   for (i = 0; i < MP_DIGIT_BIT; ++i) {
      fac *= 2.0;
   }
   for (i = a->used; i --> 0;) {
      d = (d * fac) + (double)a->dp[i];
   }
   return (a->sign == MP_NEG) ? -d : d;
}
*/
long double M_LD_DIGIT_MULTIPLIER=0.0; // NAN is the builtin NaN value defined in math.h
long double mp_get_long_double(const Mbiginteger* const a){
    mp_int* mpi_a=MP_INT_POINTER(a); // MDH@09APR2020: get the mp_int pointer from the big integer
    if(!mpi_a)return M_LD_NAN; // if a undefined, return NaN
    int i=mpi_a->used;
    if(i==0)return 0.0; // if a zero, return 0
    --i; // 0 if only one big integer digit, otherwise positive
    if(i&&!M_LD_DIGIT_MULTIPLIER){ // if we need the digit multiplier, get it
        M_LD_DIGIT_MULTIPLIER=1.0;
        int j=MP_DIGIT_BIT;
        while(--j>=0)M_LD_DIGIT_MULTIPLIER*=2.0;
    }
    long double d=(long double)mpi_a->dp[i]; // initialize d to the most significant big integer digit
    if(amVerbose())output("Long double of big integer digit %lld initialized to '%.*Lf' yet to shift by %u big integer digits.\n",MP_INT_POINTER(a)->dp[i],LDBL_DIG,d,i);
    while(--i>=0){
        if(amVerbose())output("Multiplying '%.*Lf' by %Lf.\n",d,M_LD_DIGIT_MULTIPLIER);
        d*=M_LD_DIGIT_MULTIPLIER;
        if(amVerbose())output("Result of multiplying by '%Lf': '%.*Lf'.\n",M_LD_DIGIT_MULTIPLIER,LDBL_DIG,d);
        d+=(long double)mpi_a->dp[i];
        if(amVerbose())output("Result of adding '%lld': '%.*Lf'.\n",mpi_a->dp[i],LDBL_DIG,d);
    }
    if(mpi_a->sign==MP_NEG&&!ldIsNaN(d))return -d;
    if(amVerbose())output("Conversion of big integer to long double '%.*Lf' done!\n",LDBL_DIG,d);
    return d;
    // replacing: return(a->sign==MP_NEG&&!ldIsNaN(d)?-d:d);
}/* VALIDATED */

// part of implementing _getRealText (so not present in the header)

// 'shifting' a double means either doubling or halving a number of times
long double ldShift(long double ld,long long shift){
    long double result=(shift==M_LL_INVALID?M_LD_NAN:ld); // initialize the result to what we received, unless the shift value is invalid
    // if something to shift by, and we can expect a change to the result go ahead...
    if(shift!=0&&!ldIsNaN(result)&&!ldIsZero(result)&&!ldIsInf(result)){ // something to operate on (that allows doubling/halving), as well as something to shift by
        if(shift>0){
            while(--shift>=0)result*=2; // keep doubling
        }else{
            while(++shift<=0)result/=2; // keep halving
        }
    }
    return result;
}/* VALIDATED */

long long getLongDoubleSign(long double ld){
    if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
    if(ldIsPositive(ld))return M_POSITIVE;
    if(ldIsNegative(ld))return M_NEGATIVE;
    return M_ZERO;
}/* VALIDATED */

long long double2long(long double ld){
    if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
    // TODO perhaps there are some other 
    if(ldIsZero(ld))return 0;
    long double tld=truncl(ld); // extract the integer part i.e. floor towards zero (which is called truncate)
    if(tld<M_LL_MIN||tld>M_LL_MAX)return M_LL_INVALID; // out of range
    return(long long)tld;
}/* VALIDATED */

// STRINGIFY FUNCTIONS
Mstring* _getFloatText(Mfloat* _float){
	Mstring* _floatText=(_float?__string():NULL);
    if(_floatText){
        Mstring* _p=_floatText;
        if(amDebugging())_p=string_append_char(_p,'r');
        if(_p){
            switch(fpclassify(_float->ld)){
                case FP_NAN:_p=string_append(_p,M_NAN);break;
                case FP_INFINITE:_p=string_append(_p,M_INF);break;
                default:_p=appendld(_p,_float->ld);break;
            }
        }
        if(!_p){free_string(_floatText);_floatText=NULL;}
    }
	return _floatText;
}/* VALIDATED */

char hexdigit(char c){
    if(c>=97&&c<=102)return hexdigit(c-32);
    if(c>=65&&c<=70)return c-55;
    if(c>=48&&c<=57)return c-48;
    return '\0';
}
Mstring* _getStringText(Mtext* _text,bool dequoted){
	Mstring* _stringText=(_text?__string():NULL);
    if(_stringText){
        Mstring* _p=_stringText;
        if(amDebugging())_p=string_append_char(_p,'s');
        if(_p){
            // MDH@02OCT2019: are we going to resolve escape sequence characters? yes if we're supposed to dequote (e.g. when using the Mout function)
            if(dequoted){
                char c;
                size_t lastindex=strlen(_text->_c);
                if(lastindex>0){
                    lastindex--;
                    for(size_t index=0;index<=lastindex;index++){
                        c=_text->_c[index];
                        if(index<lastindex&&c=='\\'){
                            c=_text->_c[++index];
                            switch(c){
                                case 'a':_p=string_append_char(_p,0x07);break;
                                case 'b':_p=string_append_char(_p,0x08);break;
                                case 'e':_p=string_append_char(_p,0x1B);break;
                                case 'f':_p=string_append_char(_p,0x0C);break;
                                case 'n':_p=string_append_char(_p,0x0A);break;
                                case 'r':_p=string_append_char(_p,0x0D);break;
                                case 't':_p=string_append_char(_p,0x09);break;
                                case 'v':_p=string_append_char(_p,0x0B);break;
                                case '\\':_p=string_append_char(_p,0x5C);break;
                                case '\'':_p=string_append_char(_p,0x27);break;
                                case '"':_p=string_append_char(_p,0x22);break;
                                case '?':_p=string_append_char(_p,0x3F);break;
                                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': // octal
                                    { // octal representations can't have 8 or 9 in it
                                        char oct=(c-48);
                                        // check successive characters if they are octal digits
                                        while(index+1<=lastindex){
                                            c=_text->_c[index+1];
                                            if(c<48||c>55)break; // not an octal digit
                                            oct=(oct<<3)+(c-48); // update oct by multiplying oct by 8 and adding c-48!!
                                            index++;
                                        }
                                        _p=string_append_char(_p,oct);
                                        // replacing: _p=string_append_char(_p,8*(8*(c-48)+(_text->_c[++index]-48))+(_text->_c[++index]-48));
                                    }
                                    break; // assume octal
                                case 8:case 9:break; // this would be invalid
                                case 'x':case 'X':
                                    {
                                        if(index+2<=lastindex){_p=string_append_char(_p,(hexdigit(_text->_c[index+1])<<4)+hexdigit(_text->_c[index+2]));}
                                        index+=2;
                                    }
                                    break; // TODO for now skip, so still left to do
                            }
                        }else
                            _p=string_append_char(_p,c);
                    }
                }
            }else{
                _p=string_append_char(_p,_text->presuffix);
                _p=string_append(_p,_text->_c);
                _p=string_append_char(_p,_text->presuffix);
            }
        }
        if(!_p){free_string(_stringText);_stringText=NULL;}
    }
	return _stringText;
}/* VALIDATED */

/////////////Mstring* _UNDEFINED_VALUETEXT=NULL;
// the problem here is that whatever _getValueText returns will be freed on the other side, which we would not want to happen with _UNDEFINED_VALUETEXT, so perhaps we should return NULL in that case after all????
// we can solve that by returning a new undefined value text instance every time
Mstring* _getUndefinedValueText(){
    return _getString(M_UNDEFINED_VALUE_TEXT); // just wrapping UNDEFINED_VALUETEXT again...
    /* replacing:
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return string_copy(_UNDEFINED_VALUETEXT);
    */
}/* VALIDATED */

size_t outputBiginteger(const char* const prefix,const Mbiginteger* const _biginteger,const char* const postfix){
    size_t written=0;
    if(prefix)written=output("%s",prefix);
    if(_biginteger){
        Mstring* _bigintegerText=_getBigintegerText(_biginteger);
        if(_bigintegerText){
            written+=output("%s",string(_bigintegerText));
            free_string(_bigintegerText);
        }else
            written+=output("no big integer text representation");
    }else
        written+=outputChar('?');
    if(postfix)written+=output("%s",postfix);
    return written;
}/* VALIDATED */
size_t outputDecimal(const char* const prefix,const Mdecimal* const _decimal,const char* const postfix){
    size_t written=0;
    if(prefix)written=output("%s",prefix);
    if(_decimal){
        Mstring* _decimalText=_getDecimalText(_decimal,false);
        if(_decimalText){
            written+=output("%s",string(_decimalText));
            free_string(_decimalText);
        }else
            written+=output("no decimal text representation");
    }else
        written+=outputChar('?');
    if(postfix)written+=output("%s",postfix);
    return written;
}/* VALIDATED */

// conversion from big integer to the long long it contains (when in range)
long long biginteger2long(const Mbiginteger* const _biginteger){
	return(_biginteger&&mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMin()))!=MP_LT
                        &&mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT?mp_get_i64(MP_INT_POINTER(_biginteger)):M_LL_INVALID);
}/* VALIDATED */
bool strIsZero(char* str){
    size_t l=strlen(str);
    //// NOTE do not accept integer literal postfixes when checking for 1: if(l>0&&str[l-1]=='i'||str[l-1]=='I'||str[l-1]=='q'||str[l-1]=='r')l-=1; // skip any accepted integer postfix!!
    // if l already is zero str[0] will equal '\0' which (see below) is not considered a zero integer!!!!
    while(l>0){l--;if(str[l]!='0')break;} // stop as soon as the character does not match '0' (any sign is only allowed at position 0)
    return(!l?false:str[l]=='-'||str[l]=='+'||str[l]=='0');
}/* VALIDATED */

/* moved to Mdecimal.h/c
bool isDecimalZero(Mdecimal* _decimal){
    if(!_decimal)return false;
    Mstring* _decimalText=_getDecimalText(_decimal,true); // free asap
    ///////output("Is decimal '%s' zero?",string(_decimalText));
    free_string(_decimalText); // freed
    bool result=(_decimal->mpd?mpd_iszero(_decimal->mpd)==MP_YES:false); // TODO apparently 0 means true, something else means false
    //////output(" %s.\n",(result?"YES":"NO"));
    return result;
}// VALIDATED 
*/


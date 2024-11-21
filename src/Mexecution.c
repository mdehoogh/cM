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
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>

#include "Mexecution.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return(Mallocationowner){MI_EXECUTION,id};}

// externally (in M.c) defined constants
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_ZERO,M_POSITIVE,M_NEGATIVE;
extern const char* const M_ERROR_PREFIX;
extern const char* const M_WARNING_PREFIX;
extern const char* const M_INFO_PREFIX;
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
 * needs to be freed with FREE_STRING() after use (e.g. of its string representation) as indicated by the _ that starts the name
 */
Mstring* _getUint64BinaryText(uint64_t ul,char presuffix){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _binaryText=owned_string(__string(),owner);
	if(_binaryText){
		Mstring* _p=_binaryText;
		int l=64;
		while(--l>=0&&_p){_p=string_append_char(_p,ul&1?'1':'0');ul>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
		if(presuffix)_p=string_append_char(_p,presuffix);
		if(!_p){FREE_STRING(_binaryText,owner);_binaryText=NULL;}else string_reverse(_p);
	}
	return disowned_string(_binaryText,owner);
}/* VALIDATED */
/**
 * \brief determines the binary text representation of the 16-bit unsigned integer \p us using text prefix and postfix \p presuffix
 * \param us the 16-bit unsigned integer to represent in 0s and 1s
 * \param presuffix character to enquote the binary representation when displayed
 * \return the newly created Mstring containing the binary representation on success, NULL on failure
 * needs to be freed with FREE_STRING() after use (e.g. of its string representation) as indicated by the _ that starts the name
 */
Mstring* _getUint16BinaryText(uint16_t us,char presuffix){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _binaryText=owned_string(__string(),owner);
	if(_binaryText){
		Mstring* _p=_binaryText;
		int l=16;
		while(--l>=0&&_p){_p=string_append_char(_p,us&1?'1':'0');us>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
		if(presuffix)_p=string_append_char(_p,presuffix);
		if(!_p){FREE_STRING(_binaryText,owner);_binaryText=NULL;}else string_reverse(_p);
	}
	return disowned_string(_binaryText,owner);
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
/**
 * @brief disownes the big integer pointed to by \p _biginteger from its current owner \p owner_biginteger
 * 
 * @param _biginteger the pointer to the big integer
 * @param owner_biginteger the current owner of the big integer
 * @return Mbiginteger* the disowned big integer pointer
 */
Mbiginteger* disowned_biginteger(Mbiginteger* _biginteger,Mallocationowner owner_biginteger){
	if(NULL==_biginteger)return NULL;
	if(_biginteger->_bi!=NULL)DISOWNED(_biginteger->_bi,Msubowner(owner_biginteger,1));
	DISOWNED(_biginteger,owner_biginteger);
	if(NULL==Mdisowned(_biginteger,owner_biginteger))outputError("Failed to disown big integer");
	return _biginteger;
}
/**
 * @brief sets the ownership of the big integer pointed to by \p _biginteger to \p owner_biginteger
 * 
 * @param _biginteger the pointer to the M big integer
 * @param owner_biginteger the new owner of the big integer pointed to by \p _biginteger
 * @return Mbiginteger* the owned big integer pointer
 */
Mbiginteger* owned_biginteger(Mbiginteger* _biginteger,Mallocationowner owner_biginteger){
	if(NULL==_biginteger)return NULL;
	if(_biginteger->_bi!=NULL)OWNED(_biginteger->_bi,Msubowner(owner_biginteger,1));
	return OWNED(_biginteger,owner_biginteger);
}
/**
 * @brief returns a pointer to a new big integer
 * 
 * @return Mbiginteger* a pointer to a newly created big integer
 */
Mbiginteger* __biginteger(){Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _biginteger=(Mbiginteger*)CALLOC_1(sizeof(Mbiginteger),'B',owner);
	if(_biginteger!=NULL){
#ifndef __PRODUCTION__
		_biginteger->_bi=(mp_int*)SUBOWNED(CALLOC_1(sizeof(mp_int),'b',owner),1);
		if(_biginteger->_bi!=NULL&&mp_init(_biginteger->_bi)!=MP_OKAY){
			FREE_DISOWNED_1(_biginteger->_bi,'b',owner);
			_biginteger->_bi=NULL;
		} // initialize the mp_int, when failing free the mp_int*
		if(NULL==_biginteger->_bi){
			FREE_DISOWNED_1(_biginteger,'B',owner);
			_biginteger=NULL;
		} // if we fail to allocate and/or initialize an mp_int dynamically, get rid of the biginteger too
#else
		if(mp_init((mp_int*)_biginteger)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;} // ESSENTIAL to release the big integer, when failing to initialize it!!
#endif
	}
	return(_biginteger!=NULL?disowned_biginteger(_biginteger,owner):NULL);
}/* VALIDATED */

// end of block that uses __PRODUCTION__ flag
/** \brief frees the big integer pointed to by \p __biginteger
 * 
 */
void free_biginteger(Mbiginteger* _biginteger/*,Mallocationowner owner_biginteger*/){
	if(_biginteger){
		if(amVerboseDebugging())outputInfo("Freeing a big integer."); // TODO can we display the value?
#ifndef __PRODUCTION__
		mp_clear(_biginteger->_bi);
		FREE_1(_biginteger->_bi,'b'/*,owner_biginteger*/);
#else
		mp_clear(biginteger); // directly call mp_clear on the Mbiginteger pointer!!!
#endif
		FREE_1(_biginteger,'B'/*,owner_biginteger*/); // MDH@15NOV2019: this is a big gamble but if I understand the library correctly this should be Ok because the big integer is allocated on the heap!!!
	}else
	if(amVerbose())outputInfo("No big integer to free!");
}/* VALIDATED */

// MDH@09APR2020: for all methods that call mp_int methods now require calling MP_INT_POINTER() on Mbiginteger instances
/**
 * @brief creates and returns the pointer to a M big integer initialized to \p ll
 * 
 * @param ll the int64_t long integer to wrap
 * @return Mbiginteger* the pointer to the M big integer created
 */
Mbiginteger* _getBiginteger(int64_t ll){Mallocationowner owner=getOwner(__LINE__);
	// if(ll==M_LL_INVALID)return NULL; // MDH@16DEC2020: essentially a long long could be larger so technically do not call _getBiginteger() with an invalid long long!!!!!
	Mbiginteger* _biginteger=owned_biginteger(__biginteger(),owner);
	// MDH@09APR2020: in the non-production version we're keeping track of the allocations and get_mpint on Mbiginteger will return what is required
	if(_biginteger!=NULL)mp_set_i64(MP_INT_POINTER(_biginteger),ll); // even if l equals 0 set it TODO check is that necessary???
	return disowned_biginteger(_biginteger,owner);
}/* VALIDATED */

// replace in due course by _getBigintegerNeg in Mbiginteger.c/h but that would require moving _getRational and some other functions as well from Mexecution.h/c
/**
 * @brief creates and returns the negation of the M big integer pointed to by \p _biginteger
 * 
 * @param _biginteger the pointer to the M big integer to negate
 * @return Mbiginteger* the negation of \p _biginteger
 */
Mbiginteger* _getBigintegerNeg(Mbiginteger const * const _biginteger){if(!_biginteger)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _bigintegerNeg=(Mbiginteger*)owned_biginteger(__biginteger(),owner); // the result we will be returning
	if(!_bigintegerNeg)return NULL;
	if(mp_neg(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_bigintegerNeg))!=MP_OKAY){FREE_BIGINTEGER(_bigintegerNeg,owner); return NULL;}
	return disowned_biginteger(_bigintegerNeg,owner);
}// VALIDATED

// pass in NULL to _getBigIntegerCopy to get a big integer (initialized to zero)
/**
 * @brief returns a copy of the M big integer pointed to by \p _biginteger
 * 
 * @param _biginteger the pointer to a M big integer
 * @return Mbiginteger* a copy of \p _biginteger
 */
Mbiginteger* _getBigintegerCopy(Mbiginteger const * const _biginteger){if(!_biginteger)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _bigintegerCopy=(Mbiginteger*)owned_biginteger(__biginteger(),owner);
	if(!_bigintegerCopy)return NULL;
	if(mp_copy(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_bigintegerCopy))!=MP_OKAY){FREE_BIGINTEGER(_bigintegerCopy,owner);return NULL;}
	return disowned_biginteger(_bigintegerCopy,owner);
}/* VALIDATED */

// using constant big integers 0, 1 and 2 (do NOT wrap these constants in Mvalue's though or they will need to be created over and over again)
static Mbiginteger *bi0=NULL,*bi1=NULL,*bi2=NULL,*bi3=NULL;static Mallocationowner owner_biginteger=(Mallocationowner){MI_EXECUTION,__LINE__,1};
// NOTE do NOT start with underscore (_) to indicate that the result is to be left alone!!
/**
 * @brief returns constant big integer representing 0
 * 
 * @return const Mbiginteger* 
 */
const Mbiginteger* getBigintegerZero(){if(bi0==NULL)bi0=owned_biginteger(_getBiginteger(0),owner_biginteger);return bi0;}/* VALIDATED */
/**
 * @brief returns constant big integer representing 1
 * 
 * @return const Mbiginteger* 
 */
const Mbiginteger* getBigintegerOne(){if(bi1==NULL)bi1=owned_biginteger(_getBiginteger(1),owner_biginteger);return bi1;}/* VALIDATED */
/**
 * @brief returns const big integer representing 2
 * 
 * @return const Mbiginteger* 
 */
const Mbiginteger* getBigintegerTwo(){if(bi2==NULL)bi2=owned_biginteger(_getBiginteger(2),owner_biginteger);return bi2;}/* VALIDATED */
/**
 * @brief returns const big integer representing 3
 * 
 * @return const Mbiginteger* 
 */
const Mbiginteger* getBigintegerThree(){if(bi3==NULL)bi3=owned_biginteger(_getBiginteger(3),owner_biginteger);return bi3;}/* VALIDATED */

// MDH@24OCT2019: we need a method that can convert a big integer to an integer
// TODO probably best to move this to Mbiginteger.c/h NO we need it here so we can use it in Mvalue.c/h
/**
 * @brief get the long long equivalent of big integer \p biginteger
 * @details returns M_LL_INVALID on failure
 * @param biginteger 
 * @return long long the equivalent of big integer \p biginteger
 */
long long getBigintegerInteger(Mbiginteger const * const biginteger){
	long long result=M_LL_INVALID;
	if(biginteger!=NULL){
		if(amVerboseDebugging())
			outputBiginteger("Trying to convert big integer '",biginteger,"' to a small integer.\n");
		if(mp_cmp(MP_INT_POINTER(biginteger),MP_INT_POINTER(getBigintegerLLMin()))!=MP_LT&&
				mp_cmp(MP_INT_POINTER(biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT){
			result=mp_get_i64(MP_INT_POINTER(biginteger));
			if(amVerboseDebugging())
				outputInfo("Big integer converted to a small integer.");
		}else
			if(amVerboseDebugging())
				outputInfo("Big integer cannot be converted to a small integer.");
	}
	if(amVerboseDebugging())
		output("Small integer result: %lld.\n",result);
	return result;
}

/**
 * @brief returns long integer value 1 if the M big integer pointed to by \p biginteger equals 1, otherwise 0, or M_LL_INVALID if the \p biginteger is NULL
 * 
 * @param biginteger the pointer to the M big integer
 * @return long long M_TRUE if \p biginteger points to 1, M_FALSE otherwise, or M_LL_INVALID if equal to NULL
 */
long long isBigintegerOne(Mbiginteger* biginteger){
    return(biginteger?(mp_cmp(MP_INT_POINTER(biginteger),MP_INT_POINTER(getBigintegerOne()))==MP_EQ?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
// END BIG INTEGER STUFF

/////////mp_int* __mp_int(){return (mp_int*)MALLOC(sizeof(mp_int),'I');}
// long double to rational or representation
/**
 * @brief littleEndianLongDouble represents the little endian representation of a long double
 * 
 */
typedef struct {
	uint64_t mantisse;
	uint16_t exponent;
} littleEndianLongDouble;
/**
 * @brief bigEndianLongDouble represents the big endian internal representation of a long double
 * 
 */
typedef struct {
	uint16_t exponent;
	uint64_t mantisse;
} bigEndianLongDouble;
/**
 * @brief littleEndianLongDoubleUnion maps a long double to its little endian internal representation
 * 
 */
typedef union {
	long double ld;
	littleEndianLongDouble lELD;
} littleEndianLongDoubleUnion;
/**
 * @brief bigEndianLongDoubleUnion maps a long double to its big endian internal representation
 * 
 */
typedef union {
	long double ld;
	bigEndianLongDouble bELD;
} bigEndianLongDoubleUnion;
/**
 * @brief sets the mantisse pointed to by \p mantisse and the exponent pointed to by \p exponent to the mantisse and exponent of the long double \p ld
 * 
 * @param ld the long double to map
 * @param mantisse the pointer to the mantisse of \p ld on return 
 * @param exponent the pointer to the exponent of \p ld on return
 */
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
        uint64_t shiftedout=mp_get_u64(_shiftedout);Mstring* _shiftedoutText=_getUint64BinaryText(shiftedout,'\0');output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));FREE_STRING(_shiftedoutText);
        // replacing: Mstring* _shiftedoutText=_getBigintegerText(_shiftedout);output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));FREE_STRING(_shiftedoutText);
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

/**
 * @brief sets the ownership the M text pointed to by \p _text to \p owner_text
 * 
 * @param _text the pointer to the M text
 * @param owner_text the new owner of the M text
 * @return Mtext* the pointer to the M text owned by \p owner_text
 */
Mtext* owned_text(Mtext* _text,Mallocationowner owner_text){return(_text?(Mtext*)OWNED(_text,owner_text):NULL);}
/**
 * @brief disowns the M text pointed to by \p _text owned by \p owner_text
 * 
 * @param _text the pointer to the M text
 * @param owner_text the current owner of the M text
 * @return Mtext* the disowned M text pointer
 */
Mtext* disowned_text(Mtext* _text,Mallocationowner owner_text){return(_text?(Mtext*)DISOWNED(_text,owner_text):NULL);}
void free_text(Mtext* _text/*,Mallocationowner owner*/){
	// MDH@15NOV2019: text is now created using the _strdup() function which will manage the dynamic memory of the static text allocation
	// MDH@07APR2020: BUT the problem is that currently _text is NOT under allocation control TODO we should fix that somehow...
	//                ok, changed _strdup to call MALLOC() and use memcpy to copy the characters over
	if(_text){
		if(amVerboseDebugging())
				output("Freeing text %c%s%c.\n",_text->presuffix,_text->_c,_text->presuffix);
		// MDH@09APR2020: from now on use REALLOC instead of FREE for anything with variable dynamic memory allocation
		//                _text->_c is an array and yes strlen() can be applied to any char*
		//                TODO let me think, should I use sizeof(Mtext), I suppose so assuming it will also include allocation_index (if present)
		// MDH@25OCT2020: given that most of the time _strdup() is used to create an Mtext instance
		//                I suppose we should release strlen(_text->_c)+2*sizeof(char)
		FREE(_text,sizeof(char)*(strlen(_text->_c)+2)/*sizeof(Mtext)*/,-'"'/*,owner*/); // MDH@26MAY2020 replacing: REALLOC(_text,strlen(_text->_c)+sizeof(Mtext),0,sizeof(char),'"',owner);
		// replacing: FREE(_text,'"'); // replacing (when we used a char pointer (_m) for storing the characters): if(_string){if(_string->_m)FREE_STRING(_string->_m);_string->_m=NULL;free(_string);}
	}else
	if(amVerboseDebugging())
		outputInfo("No text to free!");
}/* VALIDATED */

/**
 * @brief sets the ownership of the M integer pointed to by \p _integer to \p owner_integer
 * 
 * @param _integer the pointer to the M integer
 * @param owner_integer the new owner of the M integer
 * @return Minteger* the owned M text pointer
 */
Minteger* owned_integer(Minteger* _integer,Mallocationowner owner_integer){return(_integer?(Minteger*)OWNED(_integer,owner_integer):NULL);}
/**
 * @brief disowns the M integer pointed to by \p _integer from its current owner \p owner_integer
 * 
 * @param _integer the pointer to the M integer
 * @param owner_integer the current owner of the M integer
 * @return Minteger* the disowned M integer pointer
 */
Minteger* disowned_integer(Minteger* _integer,Mallocationowner owner_integer){return(_integer?(Minteger*)DISOWNED(_integer,owner_integer):NULL);}
void free_integer(Minteger* _integer/*,Mallocationowner owner*/){
	if(_integer){
		if(amVerboseDebugging())output("Freeing integer %llu.\n",_integer->ll);
		FREE_1(_integer,'I'/*,owner*/);
	}else
	if(amVerboseDebugging())outputInfo("No integer to free!");
}/* VALIDATED */
#define FREE_INTEGER(_integer,owner_integer) free_integer(disowned_integer(_integer,owner_integer))

/**
 * @brief sets the ownership of the M float pointed to by \p _float to \p owner_float
 * 
 * @param _float the pointer to the M float
 * @param owner_float the new owner of the M float
 * @return Mfloat* the newly owned M float pointer
 */
Mfloat* owned_float(Mfloat* _float,Mallocationowner owner_float){return(_float?(Mfloat*)OWNED(_float,owner_float):NULL);}
/**
 * @brief disowns the M float pointed to by \p _float from its current owner \p owner_float
 * 
 * @param _float the pointer to the M float
 * @param owner_float the current owner of the M float
 * @return Mfloat* the disowned M float pointer
 */
Mfloat* disowned_float(Mfloat* _float,Mallocationowner owner_float){return(_float?(Mfloat*)DISOWNED(_float,owner_float):NULL);}
void free_float(Mfloat* _float/*,Mallocationowner owner*/){
	if(_float){
		if(amVerboseDebugging())output("Freeing real %.*Lf.\n",LDBL_DIG,_float->ld);
		FREE_1(_float,'F'/*,owner*/);
	}else
	if(amVerboseDebugging())outputInfo("No real to free!");
}/* VALIDATED */
/**
 * @brief the FREE_FLOAT macro that will first disown the M float pointed to and then free the pointer to the disowned M float
 * 
 */
#define FREE_FLOAT(_float,owner_float) free_float(disowned_float(_float,owner_float))

// ALLOCATORS (private)
// value wrappers
// typically an Mvalue is immutable (we might change that for variables that are strong typed e.g. when created with integer(),real(),string(),list() or map() function)
/**
 * @brief returns a pointer to a M integer initialized to long integer \p ll
 * 
 * @param ll the initial value of the M integer
 * @return Minteger* the pointer to the M integer initialized to \p ll
 */
Minteger* _getInteger(long long ll){Mallocationowner owner=getOwner(__LINE__);
	Minteger* _integer=MALLOC_1(sizeof(Minteger),'I',owner);
	if(!_integer)return NULL;
	_integer->ll=ll;
	// output("Integer: %lld.\n",_integer->ll); // DEBUG
	return disowned_integer(_integer,owner);
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
/**
 * @brief returns the long double wrapped in the M float pointed to by \p _float
 * @param _float the pointer to the M float
 * @return the long double stored in the M float if \p _float is not NULL, or M_LD_NAN otherwise
*/
long double getFloatLongDouble(Mfloat const * const _float){return(_float?_float->ld:M_LD_NAN);}

/**
 * @brief returns a pointer to a new M float wrapping the long double \p ld
 * 
 * @param ld the long double to wrap
 * @return Mfloat* the pointer to the new M float initialized to \p ld
 */
Mfloat* _getFloat(long double ld){Mallocationowner owner=getOwner(__LINE__);
	Mfloat* _float=MALLOC_1(sizeof(Mfloat),'F',owner); // change MALLOC to also allow passing in the number of items, although the production version doesn't care!!!!
	if(!_float)return NULL;
	_float->ld=ld;
	return disowned_float(_float,owner);
}/* VALIDATED */

// special integer values to consider
/**
 * @brief returns M_TRUE if \p integer points to a M integer equal to 1, M_FALSE otherwise, unless \p integer equals NULL in which case M_LL_INVALID is returned
 * 
 * @param integer the pointer to the M integer
 * @return long long if \p integer is NULL M_LL_INVALID is returned, otherwise M_TRUE if the M integer pointed to by \p integer equals 1, or M_FALSE otherwise
 */
long long isIntegerOne(Minteger* integer){return(integer?(integer->ll==1?M_TRUE:M_FALSE):M_LL_INVALID);}
// DISCUSSION it's debatable whether a NULL integer can be tested as it is NULL but isValueUndefined() will prevent call isIntegerUndefined() with a NULL pointer!!!
/**
 * @brief returns M_TRUE if the M integer pointed to by \p integer equals M_LL_INVALID, M_FALSE if it does not, or M_LL_INVALID if \p integer equals NULL
 * 
 * @param integer the pointer to the M integer
 * @return long long M_TRUE if the M integer pointed to equals M_LL_INVALID, M_FALSE otherwise, except when \p integer equals NULL, then M_LL_INVALID is returned
 */
long long isIntegerUndefined(Minteger* integer){return(integer?(integer->ll==M_LL_INVALID?M_TRUE:M_FALSE):M_TRUE);}
/**
 * @brief returns M_TRUE if \p integer points to a M integer equal to 0, M_FALSE otherwise, unless \p integer equals NULL in which case M_LL_INVALID is returned
 * 
 * @param integer the pointer to the M integer
 * @return long long if \p integer is NULL M_LL_INVALID is returned, otherwise M_TRUE if the M integer pointed to by \p integer equals 0, or M_FALSE otherwise

 */
long long isIntegerZero(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll==0?M_TRUE:M_FALSE));}
/**
 * @brief if \p integer is not null and not undefined, returns M_TRUE if the M integer is positive, or M_FALSE if it is not, but M_LL_INVALID otherwise
 * 
 * @param integer the pointer to the M integer
 * @return long long see @brief
 */
long long isIntegerPositive(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll>0?M_TRUE:M_FALSE));}
/**
 * @brief if \p integer is not null and not undefined, returns M_TRUE if the M integer is negative, or M_FALSE if it is not, but M_LL_INVALID otherwise
 * 
 * @param integer the pointer to the M integer
 * @return long long see @brief
 */long long isIntegerNegative(Minteger* integer){return(isIntegerUndefined(integer)==M_TRUE?M_LL_INVALID:(integer->ll<0?M_TRUE:M_FALSE));}

// long double support functions (only for internal use)
/**
 * @brief the text representation of a M float that is not a number
 * 
 */
const char* M_NAN="NaN";
/**
 * @brief the text representation of a M float equal to infinity
 * 
 */
const char* M_INF="Inf";

#ifndef FP_SUPERNORMAL
#define FP_SUPERNORMAL 6
#endif

// functions that operate purely on long doubles
// MDH@25OCT2019: subnormal numbers are considered zero
/**
 * @brief returns true if \p ld is a NaN value, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsNaN(long double ld){return fpclassify(ld)==FP_NAN;}/* VALIDATED */
/**
 * @brief returns true if \p ld is infinite, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsInf(long double ld){return fpclassify(ld)==FP_INFINITE;}/* VALIDATED */
// MDH@25OCT2019: a long double is invalid if it is not zero and not normal
//                NOTE a subnormal number is considered invalid but will be treated as a zero (i.e. ldIsZero() will return true on a subnormal number)
/**
 * @brief returns true if \p ld is subnormal, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsSubnormal(long double ld){return fpclassify(ld)==FP_SUBNORMAL;}/* VALIDATED */
/**
 * @brief returns true if \p ld is supernormal, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsSupernormal(long double ld){return fpclassify(ld)==FP_SUPERNORMAL;}/* VALIDATED */
/**
 * @brief returns true if \p ld is zero, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsZero(long double ld){return(fpclassify(ld)==FP_ZERO);}/* VALIDATED */
/**
 * @brief return true if \p ld is invalid (i.e. not normal and not zero), false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsInvalid(long double ld){return(fpclassify(ld)!=FP_NORMAL&&fpclassify(ld)!=FP_ZERO);}
/**
 * @brief returns true if \p ld is valid, i.e. normal or zero.
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsValid(long double ld){return(fpclassify(ld)==FP_NORMAL||fpclassify(ld)==FP_ZERO);}

// MDH@18OCT2019: lettting the comparison take care of the result!!!
// MDH@25OCT2019: consider subnormal long doubles to be zero (to test BEFORE calling ldIsValid)
// replaced by isLongDoubleZero (see below)... bool ldIsZero(long double ld){return(fpclassify(ld)==FP_SUBNORMAL?true:(ldIsValid(ld)?ld==0?false));}/* VALIDATED */
/**
 * @brief returns true if \p ld is positive, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
bool ldIsPositive(long double ld){return(ldIsValid(ld)?ld>0:false);}/* VALIDATED */
/**
 * @brief returns true if \p ld is negative, false otherwise
 * 
 * @param ld 
 * @return true 
 * @return false 
 */
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
/**
 * @brief returns M_TRUE if \p ld is undefined, FALSE if \p ld is not undefined, and M_LL_INVALID if \p ld is unknown classified
 * 
 * @param ld 
 * @return long long 
 */
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
/**
 * @brief if \p ld is not considered undefined, returns M_TRUE if \p ld is either zero or subnormal, M_FALSE otherwise
 * 
 * @param ld 
 * @return long long M_LL_INVALID if \p ld is undefined, otherwise M_TRUE of M_FALSE (see @brief)
 */
long long isLongDoubleZero(long double ld){
	if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
	switch(fpclassify(ld)){
		case FP_INFINITE:return M_FALSE;
		case FP_SUBNORMAL:case FP_ZERO:return M_TRUE;
		default:break;
	}
	return(ld==0);
}
/**
 * @brief returns M_TRUE if \p ld is positive, M_FALSE otherwise, unless ld is not defined in which case M_LL_INVALID is returned
 * 
 * @param ld 
 * @return long long M_LL_INVALID if \p ld is not defined, M_TRUE or M_FALSE otherwise (see @brief)
 */
long long isLongDoublePositive(long double ld){
	if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
	switch(fpclassify(ld)){
		case FP_INFINITE:return(signbit(ld)?M_FALSE:M_TRUE);
		case FP_SUBNORMAL:case FP_ZERO:return M_FALSE;
		default:break;
	}
	return(ld>0);
}
/**
 * @brief returns M_TRUE if \p ld is negative, M_FALSE otherwise, unless \p ld is not defined in which case M_LL_INVALID is returned
 * 
 * @param ld 
 * @return long long M_LL_INVALID if \p ld is not defined, M_TRUE or M_FALSE otherwise (see @brief)
 */
long long isLongDoubleNegative(long double ld){
	if(isLongDoubleUndefined(ld)!=M_FALSE)return M_LL_INVALID;
	switch(fpclassify(ld)){
		case FP_INFINITE:return(signbit(ld)?M_TRUE:M_FALSE);
		case FP_SUBNORMAL:case FP_ZERO:return M_FALSE;
		default:break;
	}
	return(ld<0);
}
/**
 * @brief returns M_TRUE if \p ld equals 1, M_FALSE otherwise, unless \p ld is not defined in which case M_LL_INVALID is returned
 * 
 * @param ld 
 * @return long long M_LL_INVALID if \p ld is not defined, M_TRUE or M_FALSE otherwise
 */
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
/**
 * @brief returns M_TRUE if the M float pointed to by \p afloat is undefined or \p afloat equals NULL
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat is undefined or \p afloat equals NULL
 */
long long isFloatUndefined(Mfloat* afloat){return(afloat?isLongDoubleUndefined(afloat->ld):M_TRUE);} // a real is undefined if it is NULL or the contained long double is undefined i.e. is NaN
// use isFloatUndefined() first in the following specific functions
// in general for undefined reals we cannot determine the sign, therefore one should test for undefined first, of course one can test for invalid result of the comparison of course
/**
 * @brief returns M_TRUE if the M float pointed to by \p afloat is zero or M_LL_INVALID if \p afloat equals NULL or the M float pointed to is undefined
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat is zero or M_LL_INVALID if \p afloat equals NULL or the M float pointed to is undefined
 */
long long isFloatZero(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleZero(afloat->ld));}
/**
 * @brief returns M_TRUE if the M float pointed to by \p afloat is positive or M_LL_INVALID if \p afloat equals NULL
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat is positive, or M_LL_INVALID if \p afloat equals NULL or the M float pointed to is undefined
 */
long long isFloatPositive(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoublePositive(afloat->ld));}
/**
 * @brief returns M_TRUE if the M float pointed to by \p afloat is negative or M_LL_INVALID if \p afloat equals NULL
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat is negative, or M_LL_INVALID if \p afloat equals NULL or the M float pointed to is undefined
 */

long long isFloatNegative(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleNegative(afloat->ld));}
/**
 * @brief returns M_LL_INVALID if \p afloat is undefined, or the result of testing if \p afloat equals 1
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat equals 1, or M_LL_INVALID or M_FALSE otherwise
 */
long long isFloatOne(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:isLongDoubleOne(afloat->ld));}
/**
 * @brief returns M_LL_INVALID if \p afloat is undefined, or M_TRUE if \p afloat equals infinity, M_FALSE otherwise
 * 
 * @param afloat the pointer to an M float
 * @return long long returns M_TRUE if the Mfloat pointed to by \p afloat equals infinity, M_FALSE or M_LL_INVALID otherwise (see @brief)
 */
long long isFloatInfinite(Mfloat* afloat){return(isFloatUndefined(afloat)==M_TRUE?M_LL_INVALID:(ldIsInf(afloat->ld)?M_TRUE:M_FALSE));}

/**
 * @brief returns M_TRUE if the M floats pointed to by \p afloat1 and \p afloat2 are equal
 * @details \p afloat1 and \p afloat2 are considered equal when:
 * - when both are NULL, M_TRUE is returned
 * - when only one is NULL, M_FALSE is returned
 * - when their classification is different, M_FALSE is returned
 * - when (both) subnormal or supernormal, M_LL_INVALID is returned
 * - when (both) NaN, M_TRUE is returned
 * - when (both) infinite, M_TRUE if the signs are the same, M_FALSE otherwise
 * - when (both) normal, M_TRUE when the wrapped long doubles are equal, M_FALSE otherwise
 * @param afloat1 
 * @param afloat2 
 * @return long long M_TRUE if the M floats pointed to are equal, M_FALSE or M_LL_INVALID otherwise
 */
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
	return(afloat1->ld==afloat2->ld?M_TRUE:M_FALSE);
}
// TODO for now leave these two methods return a boolean, although we should decide whether or not they are derived or not
/**
 * @brief returns true if \p afloat is NULL or the M float pointed to is NaN, false otherwise
 * 
 * @param afloat 
 * @return true 
 * @return false 
 */
bool floatIsUndefined(Mfloat* afloat){return(afloat==NULL||ldIsNaN(afloat->ld));}
/**
 * @brief returns true if \p afloat is NULL or when the M float pointed to is NaN, or zero, false otherwise
 * 
 * @param afloat 
 * @return true 
 * @return false 
 */
bool floatIsUndefinedOrZero(Mfloat* afloat){return(afloat==NULL||ldIsNaN(afloat->ld)||isFloatZero(afloat)==M_TRUE);}

/**
 * @brief returns a copy of the M float pointed to by \p afloat
 * 
 * @param afloat 
 * @return Mfloat* a copy of the M float pointed to by \p afloat
 */
Mfloat* _getFloatCopy(Mfloat* afloat){//Mallocationowner owner=getOwner(__LINE__);
	return(floatIsUndefined(afloat)?NULL:_getFloat(afloat->ld));
}/* VALIDATED */ // only when not undefined return a copy (even when zero), NULL otherwise
/**
 * @brief returns the negation of the M float pointed to by \p afloat
 * 
 * @param afloat 
 * @return Mfloat* the negated M float pointer
 */
Mfloat* _getFloatNeg(Mfloat* afloat){//Mallocationowner owner=getOwner(__LINE__);
    return(floatIsUndefined(afloat)?NULL:_getFloat(-afloat->ld));
}/* VALIDATED */ // just switching the sign of what _getRealCopy returns

/**
 * @brief return M_TRUE if \p biginteger is NULL or does not hold an integer, M_FALSE otherwise
 * 
 * @param biginteger 
 * @return long long 
 */
long long isBigintegerUndefined(Mbiginteger const * const biginteger){return(biginteger&&biginteger->_bi?M_FALSE:M_TRUE);}
/**
 * @brief returns M_TRUE if \p text is NULL, M_FALSE otherwise
 * 
 * @param text 
 * @return long long 
 */
long long isTextUndefined(Mtext const * const text){return(text!=NULL?M_FALSE:M_TRUE);}
/**
 * @brief returns M_TRUE if \p str is NULL, M_FALSE otherwise
 * 
 * @param str 
 * @return long long 
 */
long long isStringUndefined(Mstring const * const str){return(str!=NULL?M_FALSE:M_TRUE);}
/**
 * @brief returns M_TRUE if \p token is NULL, M_FALSE otherwise
 * 
 * @param token 
 * @return long long 
 */
long long isTokenUndefined(Mtoken const * const token){return(token!=NULL?M_FALSE:M_TRUE);}

// Mtext is an immutable version of Mstring* in that it cannot be changed
/**
 * @brief returns a M text pointer with the M text initialized with C string \p text
 * 
 * @param text 
 * @return Mtext* the M text pointer
 */
Mtext* _getText(char const * const text){//Mallocationowner owner=getOwner(__LINE__);
	return(text!=NULL?(Mtext*)_strdup(text):NULL); // TODO assuming that _strdup will return a disowned text!!!
	// _text assumed to be string(Mstring*), so we can simply copy it over with the starting quote character (" or ')
}/* VALIDATED */

/**
 * @brief returns a M text pointer initialized to a single character C string initialized to \p c
 * 
 * @param c 
 * @return Mtext* the M text pointer holding a single character C string initialized to \p c
 */
Mtext* _getCharText(char c,char quote){Mallocationowner owner=getOwner(__LINE__); // _text assumed to be string(Mstring*), so we can simply copy it over with the starting quote character (" or ')
	Mtext* _charText=NULL;
	Mstring* _charString=owned_string(__string(),owner);
	if(_charString!=NULL){
		if((!quote||string_append_char(_charString,quote))&&string_append_char(_charString,c))_charText=owned_text(_getText(string(_charString)),owner);
		FREE_STRING(_charString,owner);
	}
	return disowned_text(_charText,owner);
}/* VALIDATED */

/**
 * @brief returns a M string pointer initialized to the C string \p text enclosed in \c quote characters
 * 
 * @param text the C string to enquote
 * @param quote the quote character
 * @return Mstring* the M string pointer initialized to the C string \text wrapped in \c quote characters
 */
Mstring* _getQuotedTextString(char const * const text,char quote){Mallocationowner owner=getOwner(__LINE__);
	if(text!=NULL){
			Mstring* _quotedTextString=owned_string(__string("_getQuotedTextString()"),owner);
			if(_quotedTextString!=NULL){
				if((!quote||string_append_char(_quotedTextString,quote))&&string_append(_quotedTextString,text))
					return disowned_string(_quotedTextString,owner);
				FREE_STRING(_quotedTextString,owner);
			}else
				outputError("Failed to create a quoted text string");
	}
	return NULL;
}/* VALIDATED */
// MDH@07DEC2020: more often than not we're going to need an Mtext that starts with a certain quote character
/**
 * @brief returns a M text enclosing C string \p text with single quotes
 * 
 * @param text the C string to enquote
 * @return Mtext* the M string pointer with \t text enquoted
 */
Mtext* _getSingleQuotedText(char const * const text){Mallocationowner owner=getOwner(__LINE__);
	if(!text)return NULL;
	Mstring* _singleQuotedTextString=owned_string(_getQuotedTextString(text,'\''),owner);
	if(!_singleQuotedTextString)return NULL;
	Mtext* _singleQuotedText=owned_text(_getText(string(_singleQuotedTextString)),owner);
	FREE_STRING(_singleQuotedTextString,owner);
	return disowned_text(_singleQuotedText,owner);
} /* VALIDATED */
/**
 * @brief returns a pointer to a new M string containing \p _char enquoted with \p quote
 * @details a quoted M string starts with the quote character but does not end with it
 * @param _char the character to enquote
 * @param quote the quote character
 * @return Mstring* the quoted M string pointer
 */
Mstring* _getQuotedTextCharString(char _char,char quote){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _quotedTextCharString=owned_string(__string("_getQuotedTextCharString()"),owner);
	if(_quotedTextCharString){
		if((!quote||string_append_char(_quotedTextCharString,quote))&&(!_char||string_append_char(_quotedTextCharString,_char)))
			return disowned_string(_quotedTextCharString,owner);
		FREE_STRING(_quotedTextCharString,owner);
	}else
		outputError("Failed to create a quoted character string");
	return NULL;
}/* VALIDATED */
/**
 * @brief returns a quoted M text \p _char single quoted
 * 
 * @param _char the character to enquote
 * @return Mtext* the quoted M text pointer
 */
Mtext* _getSingleQuotedCharText(char _char){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _singleQuotedTextCharString=owned_string(_getQuotedTextCharString(_char,'\''),owner);
	if(!_singleQuotedTextCharString)return NULL;
	Mtext* _singleQuotedCharText=owned_text(_getText(string(_singleQuotedTextCharString)),owner);
	FREE_STRING(_singleQuotedTextCharString,owner);
	return disowned_text(_singleQuotedCharText,owner);
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

/**
 * @brief appends the unsigned long integer \p ull to the M string pointed to by \p ms
 * 
 * @param ms the M string to append \p ull to
 * @param ull the long integer to append
 * @return Mstring* the M string pointer with \p ull appended to \p ms
 */
Mstring* appendull(Mstring* const ms,unsigned long long ull){return string_append_ull(ms,ull);}/* VALIDATED */
/**
 * @brief appends long integer \p ll to the M string pointed to by \p ms
 * 
 * @param ms the pointer to the M string to append \p ll to
 * @param ll the long integer to append
 * @return Mstring* the pointer to the M string with \p ll appended
 */
Mstring* appendll(Mstring* const ms,long long ll){return string_append_ll(ms,ll);}/* VALIDATED */
/**
 * @brief appends long double \p ld to the M string pointed to by \p ms
 * 
 * @param ms the M string to append \p ld to
 * @param ld the long double to append
 * @return Mstring* the pointer to the M string with \p ld appended to \p ms
 */
Mstring* appendld(Mstring* const ms,long double ld){return string_append_ld(ms,ld);}/* VALIDATED */

/**
 * @brief returns the pointer to a new M string containing the text representation of the long integer \p ll
 * 
 * @param ll the long integer to append
 * @return Mstring* the pointer to the new M string containing the text representation of long integer \p ll
 */
Mstring* _getLongLongText(long long ll){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _s=owned_string(__string(),owner);
	if(!_s)return NULL;
	Mstring* p=appendll(_s,ll);
	if(!p){FREE_STRING(_s,owner);return NULL;}
	////////if(amVerbose())output("Integer '%s'.",string(s));
	return disowned_string(_s,owner);
}
// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
/**
 * @brief returns the pointer to a new M string containing the text representation of M integer \p _integer
 * 
 * @param _integer the pointer to the M integer
 * @return Mstring* the pointer to a new M string containing the text representation of \p _integer
 */
Mstring* _getIntegerText(Minteger const * const _integer){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _s=owned_string(__string(),owner);
	if(!_s)return NULL;
	Mstring* p=_s;
	if(amVerboseDebugging())
			p=string_append_char(p,'i');
	// MDH@21APR2024: represent M_LL_INVALID as NAI 
	if(p!=NULL&&_integer!=NULL){
		if(_integer->ll<M_LL_MIN)
			p=string_append(p,"NAI");
		else
			p=appendll(p,_integer->ll); // p=string_append(p,LL_SEP(_integer->ll)); // MDH@05DEC2020 replacing: p=appendll(p,_integer->ll);
	}
	if(NULL==p){FREE_STRING(_s,owner);return NULL;}
	////////if(amVerbose())output("Integer '%s'.",string(s));
	return disowned_string(_s,owner);
}/* VALIDATED */

/**
 * @brief the timezone names
 * 
 */
extern char** Mtimezonenames;
/**
 * @brief returns the pointer to a new M string containing the text representation of the M time pointed to by \p _time
 * 
 * @param _time the pointer to a M time
 * @return Mstring* the pointer to a new M string containing the text representation of the M time pointed to by \p _time
 */
Mstring* _getTimeText(Mtime const * const _time){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _s=owned_string(__string(),owner);
	if(!_s)return NULL;
	Mstring* p=_s;
	if(amVerboseDebugging())
		p=string_append_char(p,'i');
	if(p&&_time){
		if(_time->t>=0){
			long long lltime=_time->t;
			if(_time->tzsec!=INT16_MIN)lltime-=_time->tzsec;
			p=appendll(p,lltime); // p=string_append(p,LL_SEP(_integer->ll)); // MDH@05DEC2020 replacing: p=appendll(p,_integer->ll);
			// MDH@14DEC2020: with the timezone name index now stored in _time->tznindex we're in trouble, because methods to change the timezone are not registered until Mtime.c/h
			//                essentially the difference is between an Unix time that is a local time (with associated timezone name) and a non-local time (essentially the epoch time)
			if(_time->tznindex!=0){
				p=string_append_char(p,'@');
				p=string_append(p,Mtimezonenames[abs(_time->tznindex)-1]);
			}
		}
	}
	if(!p){FREE_STRING(_s,owner);return NULL;}
	////////if(amVerbose())output("Integer '%s'.",string(s));
	return disowned_string(_s,owner);
}
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
// MDH@16OCT2023: _getMpintText() below it too slow because it calls mp_radix_size and mp_to_radix which both iterate _mpint 
//                and also divide continued by 10 putting the result in the same mp_int which can be optimized because now it keeps computing the dividend
//                each time and copying the result back, instead of directly putting it back in the original mp_int
//                the question is whether or not we may consume the used mpint???????? but ok, we might still make a single copy at the start which is reducable
static int getMpintSize(mp_int const * const _mpint){
	////////output("mp_int size: '%i'",_mpint->used);
	// depends on the number of bits per 'digit' which is MP_DIGIT_BIT (could be as large as 60)
	return(_mpint!=NULL?(_mpint->used*MP_DIGIT_BIT)/3+(_mpint->sign!=MP_ZPOS?1:0)+1:0);
}
/**
 * @brief returns the decimal text representation of \p _mpint in \p str using the current length of \p str in \p size
 * 
 * @param a
 * @param str where to place the decimal digits into
 * @param _size the final length of str on return
 * @return int MP_OKAY on sucess
 */
static mp_err _getMpintDecimalText(mp_int const * const a,char * str,int* size){
	if(size<2)return MP_VAL;
	// now let's compute an approximation of how many decimal digits we're going to need to allocated
	// how about allocating this amount
	// start of mp_to_radix function adapted to speed it up
	/* quick out if its zero */
	if(a->used<=0){ // replacing: MP_IS_ZERO(a)
		*str='0';
		*(str+1)='\0';
		*size=1;
		return MP_OKAY;
	}
	// MDH16OCT2023 NOTE: copying the original a into t, which we only do this once, so a will be unaffected all in all
	static char* digitchars="0123456789";
	mp_err err;
	mp_int t;
	if((err=mp_init_copy(&t,a))==MP_OKAY){
		char *_s=str;
		int digs;
		mp_digit d;
		// MDH: we can append the sign AFTER computing all the digits
		bool negative=(t.sign==MP_NEG);
		/* if it is negative output we have one less position for decimal digits */
		if(negative){*size--;t.sign=MP_ZPOS;} // size is the number of digits we 
		digs=*size; // the number of decimal digit positions available for the magnitude
		mp_word remainder; // called w in the original code
		mp_digit digit;
		// NOTE why divide the entire number a by 10 until it ends up equal to zero when we can
		//      simply divide each term until it is zero before we divide the next term
		//      that would actually save us a lot of iterations
		//      does that mean that we can keep dividing until what's left is below 10 in each digit?
		do{
			// when there are a lot of decimal digits, we would be calling mp_div_d a lot, so it's best to put the division by 10 code here
			// mp_div_d has arguments a=&t, b=10, c=&t and d=&d, and the result should be placed in err to test it next 
			// in the original mp_div_d q is initialized to a, and at the end it copies q to c, we can simply continue with using t instead of q and NOT copy q back to t
			// we can do that because t.dp[ix] is retrieved to set t.dp[ix] later on, and both do NOT interfere!!!!! except for the remaining 'digits' which I think we should zero
			remainder=0;
			// how about using pointers in the following instead of array elements????
			int ix=t.used;
			mp_digit* _mp_digit=t.dp+ix;
			////////bool zeroed=true; // to prevent needing to clamp afterwards
			while(--ix>=0){
				// the following is 3 to 4 as fast as what we had before, and twice when we use both % and /
				digit=(remainder<<(mp_word)MP_DIGIT_BIT)|(mp_word)*(--_mp_digit); // this is the current 'digit' with the remainder prefixed
				*_mp_digit=digit/10;
				remainder=digit-(*_mp_digit)*10;
				/////////if(!*_mp_digit)t.used--;
				/* replacing:

				*/
			}
			/* replacing:
			int ix=t.used;
			while(--ix>=0){
				w=(w<<(mp_word)MP_DIGIT_BIT)|(mp_word)t.dp[ix];
				if(w>=10){
					digit=(mp_digit)(w/10);
					w-=(mp_word)digit*(mp_word)10;
					t.dp[ix]=digit;
				}else
					t.dp[ix]=0;
			}
			*/
			// MDH the actual decimal digit result is 'w' (not d)
			*_s++=(remainder+48); // this is a little faster than using digitchars[w]
			// one less position for decimal digits
			if(!--digs)break;
			// clamp but a bit faster than calling mp_clamp(&t)
			int digitindex=t.used;while(--digitindex>=0&&t.dp[digitindex]==0u);t.used=digitindex+1;
			// no need to change the sign of t (as mp_clamp() does)
			// replacing: mp_clamp(&t);
		}while(t.used); // replacing !MP_IS_ZERO(&t)
		if(!t.used){
			if(negative){
				*_s++='-';
				--digs;
			}
			*_s='\0'; /* append a NULL so the string is properly terminated */
			// digs is the number of decimal positions left, and we update *size by subtracting digs
			*size-=digs;
			/* reverse the digits of the string */
			//////output("Reversing '%i' characters.",digs);
			char *x=str,*y=_s-1;char c;while(x<y){c=*x;*x++=*y;*y--=c;}
			// replacing: s_mp_reverse((unsigned char *)str,*size);
			/////////output("Reversed: '%s'",str);
		}else // only possible when digs is not zero2
			err=MP_VAL;
		mp_clear(&t);
	}
	return err;
}

// MDH@27OCT2023: we can do binary to bcd using double dabble, let's indeed call it double dabble
//                let's return the result in a string????
Mtext* doubledabble_ll(long long integer){Mallocationowner owner=getOwner(__LINE__);
	bool zerointeger=(integer==0);
	Mstring *_p=NULL,*_result=owned_string(__string(),owner);
	if(_result!=NULL){
		_p=_result;
		if(integer!=0){
			bool negative=(integer<0);
			if(negative)integer=-integer;
			char c;
			// how many bytes are we going to need
			int bits=8*sizeof(long long);
			int bcd_bytes=(bits+4*((bits/3)+1))<<3; // (bits/3)+1 should equal ceil(bits/3)
			_p=string_setlength(_p,bcd_bytes);
			if(_p!=NULL){
				char* bcd_chars=_p->_chars->chars; // easiest to directly manipulate the stored characters
				char c;
				int bit_in,bit_out,bcd_digit,bcd_digits=1; // the number of BDC digits we have constructed (every 4 shifts we get another 1)
				// we can now perform bits shifts possibly prepended by a number of ADD-3 operations
				while(--bits>0){
					// check whether we need to perform ADD-3 on any of the BCD digits 
					bcd_digit=bcd_digits;
					while(--bcd_digit>=0){
						c=bcd_chars[bcd_digit];
						if(c>=5)bcd_chars[bcd_digit]+=3;
					}
					// ready to shift
					bit_in=(integer>>bits);
					// shift in 
					while(++bcd_digit<bcd_digits){
						c=bcd_chars[bcd_digit];
						bit_out=(c>>3);
						bcd_chars[bcd_digit]=bit_in+((c&7)<<1); // TODO + has higher priority then << so I've enclosed the shift in parentheses but not certain if that is correct
						bit_in=bit_out;
					}
					// after 4 shifts we will get a new bcd_digit
					if(bits%4==0)bcd_digits++;
				}
				if(negative)_p=string_prepend(_p,"-");
			}
		}else
			_p=string_append_char(_result,'0');
	}
	Mtext* _text=(_p!=NULL?_getText(string(_p)):NULL);
	if(_result!=NULL)FREE_STRING(_result,owner);
	return(_text!=NULL?disowned_text(_result,owner):NULL);
}

// MDH@09APR2020: certain functions only know the mp_int* and not the big integer
/**
 * @brief returns the pointer to a new M string containing the text representation of the multiple precision integer pointed to by \p _mpint
 * 
 * @param _mpint the pointer to a multiple precision integer
 * @return Mstring* the pointer to a new M string containing the text representation of the multiple precision integer pointed to by \p _mpint
 */
static Mstring* _getMpintText(mp_int const * const _mpint){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _mpintText=owned_string(__string(),owner);
	////outputChar('A');
	if(_mpintText!=NULL){
		/// output("Initial big integer text length: %zu.\n",_bigintegerText->length);
		///outputChar('B');
		if(_mpint!=NULL){
			// determine the required size
// MDH@13MAR2020: this is unfortunate because I would have wanted to solve everything with tommath.h
#ifdef M_MP_DEVELOP
			size_t arepsize=0; // MDH@13MAR2020: changing type int to size_t (which is larger), so that we should be able to use it with libtommath-develop
#else
			int arepsize=0;
#endif
			//////outputChar('C');
			clock_t then=0; /////=clock();
			// if(amVerboseDebugging())
			///////then=clock(); // DEBUG
			///////if(amVerbose())outputInfo("Determining a big integer text representation.");
			// output("Big integer text length: %zu.\n",_bigintegerText->length);
			// output("Before calling mp_radix_size: ");Mstring* str_info=_string_info(_bigintegerText);output("Big integer text info: '%s'.\n",string(str_info));FREE_STRING(str_info);
			// MDH@16OCT2023: replacing the call to mp_radix_size with a call to getMpintSize() which approximates the number of decimal digit characters we're going to need!!!!
			arepsize=getMpintSize(_mpint);
			if(arepsize>0){ // replacing: mp_radix_size(_mpint,10,&arepsize)==MP_OKAY){
#ifdef M_MP_DEVELOP
				// if(arepsize>0)output("Length of big integer text representation: %zu.\n",arepsize-1);
				if(arepsize<=SIZE_MAX){
#else
				// if(arepsize>0)output("Length of big integer text representation: %d.\n",arepsize-1);
				if(arepsize<=INT_MAX){
#endif
					// output("After calling mp_radix_size: ");Mstring* str_info=_string_info(_bigintegerText);output("Big integer text info: '%s'.\n",string(str_info));FREE_STRING(str_info);
					/////outputChar('D');
					/////output("Decimal representation size: %i",arepsize);
					// if(amVerbose()&&amDebugging())
					// outputChar('E');
					// MDH@13MAR2020: arepsize actually includes the '\0' character at the end of any text representation which means that the text itself is one byte shorter
					//                however string_synclength() didn't like the length being set to arepsize-1 I suppose because there would be no '\0' at that position in the text
					//                as mp_toradix would write
					uint8_t failure=0;
					if(string_setlength(_mpintText,arepsize)){
						//////outputChar('E');
						// MDH@16OCT2023: replacing the call to mp_toradix with a call to my own speedup version
						if(_getMpintDecimalText(_mpint,_mpintText->_chars->chars,&arepsize)==MP_OKAY){
						// replacing: if(mp_toradix(_mpint,_mpintText->_chars->chars,10)==MP_OKAY){ // MDH@17APR2020: TODO we should NOT actually use the internal structure of Mstring here!!
							///////output("BEFORE SYNCLENGTH: '%s'",string(_mpintText));
							//////outputChar('F');
							if(string_setlength(_mpintText,arepsize)){ //// replacing (since we know the exact length now!!!): string_synclength(_mpintText)){
								////////output("'%s'",string(_mpintText));
								//////outputChar('G');
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
						FREE_STRING(_mpintText,owner);_mpintText=NULL;
						switch(failure){
							case 1:outputMessage(M_ERROR_PREFIX,"Failed to initialize the length of the big integer text representation to %d.",arepsize);break;
							case 2:outputError("Failed to determine the big integer representation");break;
							case 3:outputError("Failed to sync the length of the big integer representation");break;
							case 4:outputError("Failed to remove the trailing zeroes from the big integer representation.");break;
							case 5:outputError("Failed to append 'e' to the big integer representation.");break;
							case 6:outputError("Failed to append the exponent part to the big integer representation.");break;
						}
					}
				}else
					outputMessage(M_ERROR_PREFIX,"Can't store more than %u characters in a string.",SIZE_MAX);
			}else
				outputError("Couldn't determine the size of a big integer");
			if(then)
				outputMessage(M_INFO_PREFIX,"Determining the big integer representation took %lld ms.\n",(clock()-then)/M_CLOCKS_PER_MS);
		}else
				outputError("No big integer to represent");
	}else
		outputError("Failed to create a text for storing the representation of a big integer");
	///outputChar('H');
	return disowned_string(_mpintText,owner);
}
/**
 * @brief returns the pointer to a new M string containing the text representation of the M big integer \p _biginteger
 * 
 * @param _biginteger the pointer to a M big integer
 * @return Mstring* the pointer to a new M string containing the text representation of the M big integer \p _biginteger
 */
Mstring* _getBigintegerText(const Mbiginteger* _biginteger){//Mallocationowner owner=getOwner(__LINE__);
	return(_biginteger?_getMpintText(MP_INT_POINTER(_biginteger)):__string());
}

Mallocationowner owner_biLLextreme=(Mallocationowner){MI_EXECUTION,__LINE__,1};
/**
 * @brief _biLLMin is to hold the M big integer containing the long integer minimum M_LL_MIN
 * 
 */
Mbiginteger *_biLLMin=NULL;
/**
 * @brief _biLLMax is to hold the M big integer containing the long integer maximum M_LL_MAX
 * 
 */
Mbiginteger *_biLLMax=NULL;
// MDH@11JUN2020: if we return something that is owned instead of something that is disowned we prevent external freeing (i.e. unwarned that is)
/**
 * @brief returns the pointer to the constant big integer representing the long integer minimum M_LL_MIN
 * 
 * @return Mbiginteger* 
 */
Mbiginteger* getBigintegerLLMin(){//Mallocationowner owner=getOwner(__LINE__);
	if(!_biLLMin){
		if(amVerboseDebugging())outputInfo("Determining the big integer equivalent of the smallest small integer.");
		_biLLMin=owned_biginteger(_getBiginteger(M_LL_MIN),owner_biLLextreme);
		if(amVerboseDebugging())outputBiginteger("Smallest valid small integer '",_biLLMin,".\n");
	}
	return _biLLMin;
}/* VALIDATED */
/**
 * @brief returns the pointer to the constant big integer containing the long integer maximum M_LL_MAX
 * 
 * @return Mbiginteger* 
 */
Mbiginteger* getBigintegerLLMax(){
	if(!_biLLMax){
		if(amVerboseDebugging())outputInfo("Determining the big integer equivalent of the largest small integer.");
		_biLLMax=owned_biginteger(_getBiginteger(M_LL_MAX),owner_biLLextreme);
		if(amVerboseDebugging())outputBiginteger("Largest valid small integer '",_biLLMax,".\n");
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
/**
 * @brief sets multiple precision integer pointed to by \p a to the mantisse \p mantisse and exponent \p exponent of a long double
 * 
 * @param a the multiple precision integer to set
 * @param mantisse the mantisse of the multiple precision integer
 * @param exponent the base 2 exponent of the multiple precision integer
 * @return mp_err MP_ERR on failure, MP_OKAY on success
 */
static mp_err mp_set_me(mp_int* a,uint64_t mantisse,uint16_t exponent){
	if(a==NULL)return MP_ERR;
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
/**
 * @brief sets multiple precision integer pointed to by \p a to the mantisse times 2 to the power of \p exponent 
 * @details the verbose version of mp_set_me
 * @param a the multiple precision integer to set
 * @param mantisse the mantisse of the multiple precision integer
 * @param exponent the base 2 exponent of the multiple precision integer
 * @return mp_err MP_ERR on failure, MP_OKAY on success
 */
static mp_err mp_set_me_verbose(mp_int* a,uint64_t mantisse,uint16_t exponent){Mallocationowner owner=getOwner(__LINE__);
	if(!a)return MP_ERR;
	int32_t exp=(exponent&0x7FFF); // cut off the sign
	if(exp!=0){
		mp_set_u64(a,mantisse);
		///if(amVerbose()){
			Mstring* _mantisseBigIntegerText=owned_string(_getMpintText(a),owner); // MDH@09APR2020: ask _getMpintText(), replacing _getBigintegerText()
			output("Value after setting the fraction: %s.\n",string(_mantisseBigIntegerText));
			FREE_STRING(_mantisseBigIntegerText,owner);
		///}
		///if(amVerbose())
		output("Long double exponent part: %d - mantisse: %llu.\n",exp,mantisse);
		if(exp==0x7FFF){
			///////if(amVerbose())
			output("NOTE: Cannot convert an invalid or infinite real value to a big integer.");
			return MP_VAL;
		} // +-inf, NaN
		exp-=0x403E; // same as exp-=(16383+63); // the actual exponent (as 63 out of 64 mantisse bits are 'significant', bit 63 equals 1 for normalized numbers) 
		//////////frac=(frac<<1)>>1;/// replacing: &0x7FFFFFFFuLL; // I have to cut off bit 63
		/////if(amVerbose())
		output("Power of two exponent: %d.\n",exp);  
		if(exp!=0){
			mp_err err=(exp>0?mp_mul_2d(a,exp,a):mp_div_2d(a,-exp,a,NULL));
			if(err!=MP_OKAY){outputError("Failed to use the exponent of a real value in the conversion to a big integer");return err;}
		}
		///if(amVerbose()){
			Mstring* _bigIntegerText=owned_string(_getMpintText(a),owner); // MDH@09APR2020
			output("Value after applying the exponent: %s.\n",string(_bigIntegerText));
			FREE_STRING(_bigIntegerText,owner);
		///}
		if(exponent>>15){ // negative
			// take over the sign from the long double (bit 15 in the signandexponent part)
			if(mp_iszero(a)==MP_NO){ // TODO preferable NOT to use used directly!!
				a->sign=MP_NEG;
				///if(amVerbose())
				output("Sign part of real used to set the sign of the big integer.\n");
			}else
				/////if(amVerbose())
				output("No need to set the sign on a big integer equal to zero.\n");
		}
	}else // all zeros in exponent
		mp_zero(a);
	return MP_OKAY;
}/* VALIDATED */
/**
 * @brief sets the M big integer pointed to by \p a to the long double \p b
 * 
 * @param a the pointer to the M big integer
 * @param b the long double
 * @return mp_err MP_ERR on failure, MP_OKAY on success
 */
mp_err mp_set_longdouble(Mbiginteger *a, long double b){
	if(a==NULL)return MP_ERR;
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
/**
 * @brief returns the long double represented by the M big integer pointed to by \p a
 * 
 * @param a the pointer to the M big integer
 * @return long double the long double represented by \p a
 */
long double mp_get_long_double(Mbiginteger const * const a){
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_EXECUTION));
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
	if(report)
		output("Long double of big integer digit %lld initialized to '%.*Lf' yet to shift by %u big integer digits.\n",MP_INT_POINTER(a)->dp[i],LDBL_DIG,d,i);
	while(--i>=0){
		if(report)
				output("Multiplying '%.*Lf' by %Lf.\n",d,M_LD_DIGIT_MULTIPLIER);
		d*=M_LD_DIGIT_MULTIPLIER;
		if(report)
			output("Result of multiplying by '%Lf': '%.*Lf'.\n",M_LD_DIGIT_MULTIPLIER,LDBL_DIG,d);
		d+=(long double)mpi_a->dp[i];
		if(report)
			output("Result of adding '%lld': '%.*Lf'.\n",mpi_a->dp[i],LDBL_DIG,d);
	}
	if(mpi_a->sign==MP_NEG&&!ldIsNaN(d))d=-d;
	if(report)
		output("Conversion of big integer to long double '%.*Lf' done!\n",LDBL_DIG,d);
	return d;
	// replacing: return(a->sign==MP_NEG&&!ldIsNaN(d)?-d:d);
}/* VALIDATED */

// part of implementing _getRealText (so not present in the header)

// 'shifting' a double means either doubling or halving a number of times
/**
 * @brief returns the long double \p ld shifted by long integer \p shift
 * 
 * @param ld the long double to shift
 * @param shift the number of shift positions
 * @return long double the shifted long double
 */
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

/**
 * @brief returns the sign of long double \p ld
 * 
 * @param ld 
 * @return long long M_LL_INVALID if \p ld is NaN or infinity, M_POSITIVE if \p ld is positive, and M_NEGATIVE if \p ld is negative
 */
long long getLongDoubleSign(long double ld){
	if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
	if(ldIsPositive(ld))return M_POSITIVE;
	if(ldIsNegative(ld))return M_NEGATIVE;
	return M_ZERO;
}/* VALIDATED */

/**
 * @brief returns the long integer from the truncated value of long double \p ld
 * 
 * @param ld 
 * @return long long the long integer represented by long double \p ld but M_LL_INVALID if \p ld is NaN or infinity or out of the M integer range
 */
long long double2long(long double ld){
	if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
	// TODO perhaps there are some other 
	if(ldIsZero(ld))return 0;
	long double tld=truncl(ld); // extract the integer part i.e. floor towards zero (which is called truncate)
	if(tld<M_LL_MIN||tld>M_LL_MAX)return M_LL_INVALID; // out of range
	return(long long)tld;
}/* VALIDATED */

// STRINGIFY FUNCTIONS
/**
 * @brief returns the pointer to a new M string containing the text representation of M float pointed to by \p _float
 * @details preprends r in verbose debugging mode to indicate the number type
 * @param _float 
 * @return Mstring* the pointer to a new M string containing the text representation of M float pointed to by \p _float on success, NULL on failure
 */
Mstring* _getFloatText(Mfloat const * const _float){Mallocationowner owner=getOwner(__LINE__);
	if(!_float)return NULL;
	Mstring* _floatText=owned_string(__string(),owner);
	if(_floatText){
		Mstring* p=_floatText;
		if(amVerboseDebugging())
			p=string_append_char(p,'r');
		if(p!=NULL){
			// DONE TODO we should distinguish between +INF and -INF
			switch(fpclassify(_float->ld)){
				case FP_NAN:p=string_append(p,M_NAN);break;
				case FP_INFINITE:if(signbit(_float->ld))p=string_append_char(p,'-');p=string_append(p,M_INF);break;
				default:p=appendld(p,_float->ld);break;
			}
		}
		if(!p){FREE_STRING(_floatText,owner);return NULL;}
	}
	return disowned_string(_floatText,owner);
}/* VALIDATED */

/**
 * @brief returns the hexadecimal value (0-15) of hexadecimal character \p c
 * 
 * @param c 
 * @return char the hexadecimal value that hexadecimal character \p c represents
 */
char hexdigit(char c){
	if(c>=97&&c<=102)return hexdigit(c-32);
	if(c>=65&&c<=70)return c-55;
	if(c>=48&&c<=57)return c-48;
	return '\0';
}

// MDH@01MAY2024: convenient to have a _getStringOfText(Chars that we may call directly on a C string
/**
 * @brief returns a new M string containing the (dequoted) text pointed to by \p chars
 * 
 * @param chars the C string to decode
 * @param dequoted 
 * @return Mstring* the decoded C string
 */
Mstring* _getStringOfChars(char const * const chars,char quoteChar){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _stringText=owned_string(__string(),owner);
	if(_stringText!=NULL){
		Mstring* p=_stringText;
		/*
		if(amVerboseDebugging())
			p=string_append_char(p,'s');
			*/
		// MDH@02OCT2019: are we going to resolve escape sequence characters? yes if we're supposed to dequote (e.g. when using the Mout function)
		if(!quoteChar){ // not to return enquoted but simply decoded...
			// TODO can we do the following using pointers somehow????
			char c;
			size_t lastindex=strlen(chars);
			if(lastindex>0){
				lastindex--;
				for(size_t index=0;index<=lastindex;index++){
					c=chars[index];
					if(index<lastindex&&c=='\\'){
						c=chars[++index];
						switch(c){
							case 'a':p=string_append_char(p,0x07);break;
							case 'b':p=string_append_char(p,0x08);break;
							case 't':p=string_append_char(p,0x09);break;
							case 'n':p=string_append_char(p,0x0A);break;
							case 'v':p=string_append_char(p,0x0B);break;
							case 'f':p=string_append_char(p,0x0C);break;
							case 'r':p=string_append_char(p,0x0D);break;
							case 'e':p=string_append_char(p,0x1B);break;
							case '"':p=string_append_char(p,0x22);break;
							case '\'':p=string_append_char(p,0x27);break;
							case '?':p=string_append_char(p,0x3F);break;
							case '\\':p=string_append_char(p,0x5C);break;
							case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': // octal
								{ // octal representations can't have 8 or 9 in it
									char oct=(c-48);
									// check successive characters if they are octal digits
									while(index+1<=lastindex){
										c=chars[index+1];
										if(c<48||c>55)break; // not an octal digit
										oct=(oct<<3)+(c-48); // update oct by multiplying oct by 8 and adding c-48!!
										index++;
									}
									p=string_append_char(p,oct);
									// replacing: _p=string_append_char(_p,8*(8*(c-48)+(chars[++index]-48))+(chars[++index]-48));
								}
								break; // assume octal
							case 8:case 9:break; // this would be invalid
							case 'x':case 'X':
								{
										if(index+2<=lastindex){p=string_append_char(p,(hexdigit(chars[index+1])<<4)+hexdigit(chars[index+2]));}
										index+=2;
								}
								break; // TODO for now skip, so still left to do
						}
					}else
						p=string_append_char(p,c);
				}
			}
		}else{ // just return enquoted inside the quoteChar
			p=string_append_char(p,quoteChar);
			p=string_append(p,chars);
			p=string_append_char(p,quoteChar);
		}
		if(NULL==p){FREE_STRING(_stringText,owner);return NULL;}
		///else output("Unescaped text of '%s': '%s'.\n",chars,string(_stringText));
	}
	return disowned_string(_stringText,owner);
}

static unsigned char HEX_CHARS[]="0123456789ABCDEF";

/**
 * @brief returns the escaped string representation of Mstring* \p str
 * 
 * @param str 
 * @return Mstring* the escaped string representation of Mstring* \p str
 */
Mstring* _getPrintableString(Mstring const * const str,char quoteChar,char quoteTypeChar){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==str||NULL==str->_chars)return NULL;
	// it would be convenient to create a string with the same length as str
	size_t strlength=string_length(str);
	Mstring* _printableString=owned_string(_getStringOfLength(strlength),owner);
	if(_printableString!=NULL){
		Mstring* p=_printableString;
		if(quoteTypeChar)p=string_append_char(p,quoteTypeChar);
		if(quoteChar)p=string_append_char(p,quoteChar);
		if(strlength){
			char* chars=str->_chars->chars;
			char c;
			do{
				c=*chars++; // TODO hopefully this works as intended, c=*chars then *chars++
				if(c<=31||c>=127){
					p=string_append_char(p,'\\');
					switch(c){
						case 7:p=string_append_char(p,'a');break;
						case 8:p=string_append_char(p,'b');break;
						case 9:p=string_append_char(p,'t');break;
						case 10:p=string_append_char(p,'n');break;
						case 11:p=string_append_char(p,'v');break;
						case 12:p=string_append_char(p,'f');break;
						case 13:p=string_append_char(p,'r');break;
						case 27:p=string_append_char(p,'e');break;
						default:{
							p=string_append_char(p,'x');
							p=string_append_char(p,HEX_CHARS[c>>4]);
							p=string_append_char(p,HEX_CHARS[c&15]);
						}
						break;
					}
				}else{
					if(c=='\\')p=string_append_char(p,'\\');
					p=string_append_char(p,c);
				}
				if(NULL==p)break;
				strlength--;
			}while(strlength);
		}
		if(quoteChar)p=string_append_char(p,quoteChar);
		if(p==NULL){outputError("Failed to create a readable bytes representation");FREE_STRING(_printableString,owner);return NULL;}
		return disowned_string(_printableString,owner);
	}
}

/**
 * @brief returns the escaped text representation of \p _text
 * 
 * @param _text 
 * @return Mstring* 
 */
Mstring* _getEscapedStringOfText(Mtext const * const _text,bool dequoted){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _escapedString=(_text!=NULL?owned_string(__string(),owner):NULL);
	if(_escapedString!=NULL){
		/*
		if(dequoted){

		}else{*/
			Mstring* p=_escapedString;
			p=string_append_char(p,_text->presuffix);
			if(_text->presuffix=='b')p=string_append_char(p,'\'');else
			if(_text->presuffix=='B')p=string_append_char(p,'"');
			unsigned char c;
			size_t index=0;
			// problem when _text->_c contains NUL characters somehow!!!! those should already be escaped with \0 NOT \x00
			while((c=_text->_c[index++])){
				if(c<=31||c==127){
					p=string_append(p,"\\x");
					p=string_append_char(p,HEX_CHARS[c>>4]);
					p=string_append_char(p,HEX_CHARS[c&15]);
				}else{ // escape backslashes
					p=string_append_char(p,c);
					if(c=='\'')p=string_append_char(p,c);
				}
			}
			if(_text->presuffix=='B')p=string_append_char(p,'"');else 
			if(_text->presuffix=='b')p=string_append_char(p,'\'');else p=string_append_char(p,_text->presuffix);
			if(NULL==p){FREE_STRING(_escapedString,owner);return NULL;}
		//}
	}
	return disowned_string(_escapedString,owner);
}
/**
 * @brief returns the pointer to a new M string containing the (dequoted) text pointed to by \p _text
 * 
 * @param _text the M text to be store in the new M string (dequoted or not)
 * @param dequoted the flag indicating whether or not to dequote \p _text
 * @return Mstring* the (dequoted) text represented by the M text pointed to by \p _text, NULL on failure e.g. when \p _text is NULL
 */
Mstring* _getStringOfText(Mtext const * const _text,bool dequoted){if(NULL==_text)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mstring* _stringText=owned_string(__string(),owner);
	if(_stringText!=NULL){
		Mstring* p=_stringText;
		/*
		if(amVerboseDebugging())
			p=string_append_char(p,'s');*/
		// MDH@02OCT2019: are we going to resolve escape sequence characters? yes if we're supposed to dequote (e.g. when using the Mout function)
		if(dequoted){
			// TODO can we do the following using pointers somehow????
			char c;
			size_t lastindex=strlen(_text->_c);
			if(lastindex>0){
				lastindex--;
				for(size_t index=0;index<=lastindex;index++){
					c=_text->_c[index];
					if(index<lastindex&&c=='\\'){
						c=_text->_c[++index];
						switch(c){
							case 'a':p=string_append_char(p,0x07);break;
							case 'b':p=string_append_char(p,0x08);break;
							case 't':p=string_append_char(p,0x09);break;
							case 'n':p=string_append_char(p,0x0A);break;
							case 'v':p=string_append_char(p,0x0B);break;
							case 'f':p=string_append_char(p,0x0C);break;
							case 'r':p=string_append_char(p,0x0D);break;
							case 'e':p=string_append_char(p,0x1B);break;
							case '"':p=string_append_char(p,0x22);break;
							case '\'':p=string_append_char(p,0x27);break;
							case '?':p=string_append_char(p,0x3F);break;
							case '\\':p=string_append_char(p,0x5C);break;
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
									p=string_append_char(p,oct);
									// replacing: _p=string_append_char(_p,8*(8*(c-48)+(_text->_c[++index]-48))+(_text->_c[++index]-48));
								}
								break; // assume octal
							case 8:case 9:break; // this would be invalid
							case 'x':case 'X':
								{
										if(index+2<=lastindex){p=string_append_char(p,(hexdigit(_text->_c[index+1])<<4)+hexdigit(_text->_c[index+2]));}
										index+=2;
								}
								break; // TODO for now skip, so still left to do
						}
					}else
						p=string_append_char(p,c);
				}
			}
		}else{
			if(_text->presuffix=='b')p=string_append(p,"b'");else
			if(_text->presuffix=='B')p=string_append(p,"b\"");else
			p=string_append_char(p,_text->presuffix);
			p=string_append(p,_text->_c);
			if(_text->presuffix=='b')p=string_append_char(p,'\'');else
			if(_text->presuffix=='B')p=string_append_char(p,'"');else
			p=string_append_char(p,_text->presuffix);
		}
		if(NULL==p){FREE_STRING(_stringText,owner);return NULL;}
		////else output("Unescaped text of '%c%s': '%s'.\n",_test->presuffix,_text->_c,string(_stringText));
	}
	return disowned_string(_stringText,owner);
}/* VALIDATED */

/////////////Mstring* _UNDEFINED_VALUETEXT=NULL;
// the problem here is that whatever _getValueText returns will be freed on the other side, which we would not want to happen with _UNDEFINED_VALUETEXT, so perhaps we should return NULL in that case after all????
// we can solve that by returning a new undefined value text instance every time
/**
 * @brief returns the pointer to a new M string wrapping M_UNDEFINED_VALUE_TEXT
 * 
 * @return Mstring* the pointer to a new M string holding the M_UNDEFINED_VALUE_TEXT
 */
Mstring* _getUndefinedValueText(){//Mallocationowner owner=getOwner(__LINE__);
	return _getString(M_UNDEFINED_VALUE_TEXT); // just wrapping UNDEFINED_VALUETEXT again...
	/* replacing:
	if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
	return string_copy(_UNDEFINED_VALUETEXT);
	*/
}/* VALIDATED */

/**
 * @brief outputs the M big integer pointed to by \p _biginteger, with prefix \p prefix and suffix \p postfix
 * 
 * @param prefix 
 * @param _biginteger 
 * @param postfix 
 * @return size_t the number of characters written
 */
size_t outputBiginteger(char const * const prefix,Mbiginteger const * const _biginteger,char const * const postfix){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	if(prefix)written=output("%s",prefix);
	if(_biginteger){
		Mstring* _bigintegerText=owned_string(_getBigintegerText(_biginteger),owner);
		if(_bigintegerText){
			written+=output("%s",string(_bigintegerText));
			FREE_STRING(_bigintegerText,owner);
		}else
				written+=output("no big integer text representation");
	}else
			written+=outputChar('?');
	if(postfix)written+=output("%s",postfix);
	return written;
}/* VALIDATED */
/**
 * @brief outputs the M decimal pointed to by \p _decimal prefixed by the C string pointed to by \p prefix and suffixed by the C string pointed to by \p postfix
 * 
 * @param prefix 
 * @param _decimal 
 * @param postfix 
 * @return size_t the number of characters written
 */
size_t outputDecimal(char const * const prefix,Mdecimal const * const _decimal,char const * const postfix){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	if(prefix)written=output("%s",prefix);
	if(_decimal!=NULL){
		Mstring* _decimalText=owned_string(_getDecimalText(_decimal,false),owner);
		if(_decimalText){
			written+=output("%s",string(_decimalText));
			FREE_STRING(_decimalText,owner);
		}else
			written+=output("no decimal text representation");
	}else
		written+=outputChar('?');
	if(postfix)written+=output("%s",postfix);
	return written;
}/* VALIDATED */

// conversion from big integer to the long long it contains (when in range)
/**
 * @brief returns the long integer value of the M big integer pointed to by \p _biginteger
 * @details if something goes wrong e.g. when \p _biginteger is NULL or out of long integer range, M_LL_INVALID is returned
 * @param _biginteger 
 * @return long long the long integer represented by the M big integer pointed to by \p _biginteger
 */
long long biginteger2long(Mbiginteger const * const _biginteger){
	return(_biginteger&&mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMin()))!=MP_LT
		&&mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT?mp_get_i64(MP_INT_POINTER(_biginteger)):M_LL_INVALID);
}/* VALIDATED */
/**
 * @brief returns true if the C string pointed to by \p str represents zero, false otherwise
 * 
 * @param str the C string pointer
 * @return true 
 * @return false 
 */
bool strIsZero(char const * const str){
	size_t l=strlen(str);
	//// NOTE do not accept integer literal postfixes when checking for 1: if(l>0&&str[l-1]=='i'||str[l-1]=='I'||str[l-1]=='q'||str[l-1]=='r')l-=1; // skip any accepted integer postfix!!
	// if l already is zero str[0] will equal '\0' which (see below) is not considered a zero integer!!!!
	while(l>0)if(str[--l]!='0')break; // stop as soon as the character does not match '0' (any sign is only allowed at position 0)
	return(!l?false:str[l]=='-'||str[l]=='+'||str[l]=='0');
}/* VALIDATED */

/* moved to Mdecimal.h/c
bool isDecimalZero(Mdecimal* _decimal){
	if(!_decimal)return false;
	Mstring* _decimalText=_getDecimalText(_decimal,true); // free asap
	///////output("Is decimal '%s' zero?",string(_decimalText));
	FREE_STRING(_decimalText); // freed
	bool result=(_decimal->mpd?mpd_iszero(_decimal->mpd)==MP_YES:false); // TODO apparently 0 means true, something else means false
	//////output(" %s.\n",(result?"YES":"NO"));
	return result;
}// VALIDATED 
*/

// MDH@28SEP2020: file access
/**
 * @brief returns the pointer to M file \p _file disowned from its current owner \p owner_file
 * 
 * @param _file the pointer to a M file
 * @param owner_file the current owner
 * @return Mfile* the disowned \p _file
 */
Mfile* disowned_file(Mfile* _file,Mallocationowner owner_file){
	if(NULL==_file)return NULL;
	///output("Disowning file stat!\n");
	/*if(_file->_stat!=NULL){
		DISOWNED(_file->_stat,owner_file);
	}*/
	if(_file->_name!=NULL){
		///output("Disowning file name!\n");
		DISOWNED(_file->_name,owner_file); // MDH@27DEC2020: oops, need to do this too!
		///if(Misdisowned(_file->_name))output("File name disowned!\n");else outputError("File not disowned");
	}
	///output("Disowning the file!\n");
	return DISOWNED(_file,owner_file);
}
/**
 * @brief sets the ownership of the M file pointed to by \p _file to \p owner_file
 * 
 * @param _file 
 * @param owner_file the new owner of \p _file
 * @return Mfile* the owned \p _file
 */
Mfile* owned_file(struct Mfile* _file,Mallocationowner owner_file){
	if(NULL==_file)return NULL;
	if(_file->_name!=NULL)OWNED(_file->_name,Msubowner(owner_file,1)); // MDH@28APR2024: OOPS forgot this line before!!!
	///if(_file->_stat!=NULL)OWNED(_file->_stat,Msubowner(owner_file,1));
	return OWNED(_file,owner_file);
}
/**
 * @brief returns a pointer to a new M file
 * @details both the M file and its statistics pointer is zero initialized
 * @return Mfile* the pointer to a new M file
 */
Mfile* __file(){Mallocationowner owner=getOwner(__LINE__);
	Mfile* _file=CALLOC_1(sizeof(struct Mfile),'F',owner);
	if(_file==NULL)return NULL;
	_file->staterrno=INT_MIN; // this is to indicate that the stat field has not yet been set
	/* MDH@01MAY2024: no need to have a _file->_stat at this point
	_file->_stat=(struct stat*)SUBOWNED(CALLOC_1(sizeof(struct stat),'f',owner),1); // allocate memory to store the file statistics
	if(NULL==_file->_stat){FREE_FILE(_file,owner);return NULL;} // MDH@01MAY2024: having a stat is crucial!!
	*/
	return disowned_file(_file,owner);
}

/* Mmap is not yet defined here, so we should report in another way!!!
static Mstring* _fInfoText(Mfile* file){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _filePropertyMap=owned_map(_getFilePropertyMap(file),owner);
	Mstring* result=_getValueText(_getFilePropertyMap(file),true,true,true);
	FREE_MAP(_filePropertyMap,owner);
	return result;
}
*/

Mstring* _getFilePermissionsText(mode_t perms){Mallocationowner owner=getOwner(__LINE__);
	Mstring *_result=owned_string(_getString("'"),owner);
	if(_result!=NULL){
		Mstring* p=string_append_char(_result,(S_ISDIR(perms)) ? 'd' : ' ');
		p=string_append_char(_result,(perms & S_IRUSR) ? 'r' : '-');
		p=string_append_char(_result,(perms & S_IWUSR) ? 'w' : '-');
		p=string_append_char(_result,(perms & S_IXUSR) ? 'x' : '-');
		p=string_append_char(_result,(perms & S_IRGRP) ? 'r' : '-');
		p=string_append_char(_result,(perms & S_IWGRP) ? 'w' : '-');
		p=string_append_char(_result,(perms & S_IXGRP) ? 'x' : '-');
		p=string_append_char(_result,(perms & S_IROTH) ? 'r' : '-');
		p=string_append_char(_result,(perms & S_IWOTH) ? 'w' : '-');
		p=string_append_char(_result,(perms & S_IXOTH) ? 'x' : '-');
		if(NULL==p){FREE_STRING(_result,owner);return NULL;}
	}
	return disowned_string(_result,owner);
}

/**
 * @brief updates the stats stored of \p file stored in file->stat calling the C stat() function
 * @details stores 0 in file->staterrno on success, or the error number (from errno) on failure
 * @param file 
 */
void fUpdateStats(Mfile * const file,bool report){Mallocationowner owner=getOwner(__LINE__);
	if(file!=NULL&&file->_name!=NULL){
		int stat_errno=file->staterrno; // the current status of the stats info
		// MDH@08MAY2024: use fstat() instead of stat() on an opened file!!
		file->staterrno=((file->_f!=NULL?fstat(file->_f,&file->stat):stat(string(file->_name),&file->stat))!=0?errno:0); // failure, some error occurred
		if(file->staterrno==0){
			///if(report){
				/*
				Mstring* _fileInfoText=owned_string(_fInfoText(file),owner);
				if(_fileInfoText!=NULL){output("File '%s' stats: %s\n",string(_fileInfoText));FREE_STRING(_fileInfoText,owner);}
				*/
				struct passwd *pwd;struct group *grp;char datestring[256];struct tm *tm;
				output("File '%s':\n",string(file->_name));
				output("  permissions:");
				Mstring* _permissionsText=owned_string(_getFilePermissionsText(file->stat.st_mode),owner);
				if(_permissionsText!=NULL){
					output(" %10.10s",string_remainder(_permissionsText,1));
					FREE_STRING(_permissionsText,owner);
				}

				output(" (%o)\n",file->stat.st_mode);

				output("  link       : '%4d'\n",file->stat.st_nlink);
				
				if((pwd=getpwuid(file->stat.st_uid))!=NULL)output("  user       : %-8.8s\n",pwd->pw_name);else output("  user       : %-8d\n",file->stat.st_uid);
				
				if((grp=getgrgid(file->stat.st_gid))!=NULL)output("  group      : %-8.8s\n",grp->gr_name);else output("  group      : %-8d\n",file->stat.st_gid);
				
				output("  size       : %llu\n",file->stat.st_size);
				
				tm=localtime(&file->stat.st_mtime);strftime(datestring,sizeof(datestring),nl_langinfo(D_T_FMT),tm);
				output("  modified   : %s\n",datestring);
			////}
		}
		if(stat_errno!=file->staterrno){ // some change
			if(file->staterrno)
				outputMessage(M_ERROR_PREFIX,"%s (error code: %d) updating the status information of file '%s'.",strerror(file->staterrno),file->staterrno,string(file->_name));
			else
				outputMessage(M_INFO_PREFIX,"The status information of file '%s' updated successfully.",string(file->_name));
		}
	}else
	if(file!=NULL)
		outputError("Cannot update the file stats of an unnamed file");
	else
		outputError("No file specified");
}

// MDH@02OCT2020: when opening a file check whether the file is readable or writeable depending on the opening mode
/**
 * @brief closes the M file pointed to by \p _file
 * @details error conditions:
 *          - \p _file is NULL
 *          - \p _file->_name is NULL
 *          - \p _file->_f is NULL
 *          - Failing to close \p _file->_f using fclose
 * @param _file 
 * @return true on success
 * @return false on failure
 */
bool closeFile(Mfile* const _file,bool report){
	/////////bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_EXECUTION));
	if(NULL==_file){outputWarning("No file to close");return false;} // nothing to close
	if(NULL==_file->_name){outputWarning("File has no name");return true;} // must be closed
	//////assert(_file->_name); // MDH@28DEC2020: we need a name!!!!
	if(_file->_f==NULL){
		outputMessage(M_WARNING_PREFIX,"File '%s' already closed.",string(_file->_name));
		return true;
	} // already closed
	if(report)
		outputMessage(M_INFO_PREFIX,"Closing file '%s'.\n",string(_file->_name));
	if(fclose(_file->_f)==0){ // success
		_file->_f=NULL;
		if(report)
			outputMessage(M_INFO_PREFIX,"'%s' closed.\n",string(_file->_name));
		return true;
	}
	outputMessage(M_ERROR_PREFIX,"Failed to close '%s'.",string(_file->_name));
	return false;
}
/**
 * @brief frees \p _fileposition
 * 
 * @param _fileposition 
 */
static void free_fileposition(Mfileposition*  _fileposition){
	assert(_fileposition!=NULL);
	if(_fileposition->next!=NULL)free_fileposition(_fileposition->next);
	FREE_1(_fileposition,'P');
}
/**
 * @brief frees the M file pointed to by _file
 * @details always attempts to close the M file pointed to by \p _file first
 * @param _file 
 */
void free_file(Mfile* _file){
	if(_file!=NULL){
		if(_file->_f!=NULL)closeFile(_file,false); // I suppose this is typically what we have to do to not have pending resources
		////if(_file->_stat!=NULL){FREE_1(_file->_stat,'f');_file->_stat=NULL;}
		if(_file->_name!=NULL){FREE_1(_file->_name,'S');_file->_name=NULL;}
		if(_file->_mode!=NULL){
			////output("File mode '%s'",_file->_mode);
			FREE(_file->_mode,1+strlen(_file->_mode),-'"'); // _file->_mode was assigned using _strdup() which IS managed!!! so don't use free()
			////output(" freed!\n");
			_file->_mode=NULL;
			if(_file->_filepositionstack!=NULL)free_fileposition(_file->_filepositionstack);
		} // MDH@04MAY2024: _file->_mode is currently unmanaged!!
		FREE_1(_file,'F');
	}
}
// opening a file might mean that afterwards the file exists, and we then should update _file->_stat accordingly!!!
/**
 * @brief opens the M file pointed to by \p _file in mode \p mode
 * 
 * @param _file the pointer to the M file
 * @param mode the mode in which to open the M file
 */
void openFile(Mfile* _file,Mallocationowner owner_file,char* mode,bool report){
	//////bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_EXECUTION));
	// only open when defined and currently not open
	if(_file!=NULL&&mode!=NULL&&*mode){ // valid input
		if(NULL==_file->_f){ // not opened yet
			if(report)
				output("Opening file '%s' in mode '%s'.\n",string(_file->_name),mode);
			if(_file->staterrno!=0)fUpdateStats(_file,report);
			if(_file->staterrno!=0||!S_ISDIR(_file->stat.st_mode)){ // never try to open a directory (TODO perhaps we should not try to open other things here as well)
				output("Opening file '%s'.\n",string(_file->_name));
				//////assert(_file->_name); // MDH@28DEC2020: we need a name!!!!
				_file->_f=fopen(string(_file->_name),mode);
				if(_file->_f!=NULL){ // now opened
					// update stat (even if already set, because the file existed to start with)
					_file->staterrno=INT_MIN; // replacing: fUpdateStats(_file,report); // TODO or should we just make the staterrno dirty?
					if(report)
						output("File '%s' opened!\n",string(_file->_name));
					/* MDH@03MAY2024: fOpened() will take care of copying the mode the file was opened in
					// TODO find a better way to copy *mode
					_file->mode[0]=*mode;if(*mode){_file->mode[1]=*(++mode);if(*mode){_file->mode[2]=*(++mode);if(*mode)_file->mode[3]='\0';}} // register the opening mode (which consists of exactly three characters)
					*/
					/*
					if(NULL==_file->_stat)_file->_stat=CALLOC_1(sizeof(struct stat),'f',Msubowner(owner_file,1));
					int updateStatsErrorCode=stat(string(_file->_name),_file->_stat);
					if(updateStatsErrorCode){ // updating stat failed
						outputMessage(M_ERROR_PREFIX,"Failed to update the stats of file '%s' in mode '%s' (error code: %d).",string(_file->_name),_file->mode,updateStatsErrorCode);
						// perhaps we should never do the following???? although somehow we are using _file->_stat for certain purposes!!!
						FREE_DISOWNED_1(_file->_stat,'f',owner_file);
						_file->_stat=NULL;
					}else
					if(report)
						outputMessage(M_INFO_PREFIX,"Stats of file '%s' updated.\n",string(_file->_name));
						*/
				}else
					outputMessage(M_ERROR_PREFIX,"Failed to open file '%s'.",string(_file->_name));
			}else
			if(_file->staterrno==0)
				outputMessage(M_ERROR_PREFIX,"Can't open a '%s': it is a directory!",string(_file->_name));
			else
				outputMessage(M_WARNING_PREFIX,"File '%s' already opened!",string(_file->_name));
		}else
			outputMessage(M_WARNING_PREFIX,"'%s' already open!",string(_file->_name));
	}
}

// MDH@08DEC2020: and not times
/**
 * @brief sets the ownership of the M time pointed to by \p _time to \p owner_time
 * 
 * @param _time 
 * @param owner_time 
 * @return Mtime* the owned \p _time
 */
Mtime* owned_time(Mtime* _time,Mallocationowner owner_time){
	if(!_time)return NULL;
	return OWNED(_time,owner_time);
}
/**
 * @brief disowns the M time pointed to by \p _time from its current owner \p owner_time
 * 
 * @param _time 
 * @param owner_time 
 * @return Mtime* the disowned \p _time
 */
Mtime* disowned_time(Mtime* _time,Mallocationowner owner_time){
	if(!_time)return NULL;
	return DISOWNED(_time,owner_time);
}
// __time() returns an time initialized as zero UTC (because tznindex and tzsec will be zero)
/**
 * @brief returns a pointer to a new M time
 * @details the M time is zero initialized (0 UTC)
 * 
 * @return Mtime* the pointer to a new M time
 */
Mtime* __time(){Mallocationowner owner=getOwner(__LINE__);
	Mtime* _time=CALLOC_1(sizeof(struct Mtime),'T',owner);
	return disowned_time(_time,owner);
}
// NOTE _getTime() will return a result even if the tzsec and tznindex are an incorrect combination!!!
/**
 * @brief returns a pointer to a new M time set to \p t in the timezone with index \p tznindex with timezone offset \p tzsec
 * @exception returns NULL on failure to allocate sufficient memory
 * @param source the caller identification
 * @param t the time integer
 * @param tzsec the timezone offset in seconds
 * @param tznindex the timezone index
 * @return Mtime* 
 */
Mtime* _getTime(char const * const source,time_t t,int16_t tzsec,int16_t tznindex){Mallocationowner owner=getOwner(__LINE__);
	Mtime* _time=owned_time(__time(),owner);
	if(_time==NULL)return NULL;
	_time->t=t;
	_time->tzsec=tzsec; // store the timezone seconds deviation
	_time->tznindex=tznindex;
	return disowned_time(_time,owner);
}
/**
 * @brief frees the M time pointed to by \p _time
 * 
 * @param _time 
 */
void free_time(Mtime* _time){
	if(_time!=NULL){
		FREE_1(_time,'T');
	}
}
/**
 * @brief returns the long integer representing the UTC seconds of the represented M time pointed to by \p _time
 * @details returns M_LL_INVALID if \p _time is NULL
 * @param _time 
 * @return long long the time stored in the M time corrected by its timezone offset
 */
long long getTimeLongLong(Mtime* _time){
	return(_time!=NULL?_time->t-(_time->tzsec==INT16_MIN?0:_time->tzsec):M_LL_INVALID);
}

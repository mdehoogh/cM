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
#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "Mstring.h"
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"

#include "Mexecution.h"

// externally (in M.c) defined constants
extern const char* const ERROR_PREFIX;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const mpd_context_t* _decimalContext;

void outputError(const char* const error){if(error)output("%s%s.\n",ERROR_PREFIX,error);}
void outputErrorAndText(const char* const error,const char* const text){if(error)output("%s%s",ERROR_PREFIX,error);if(text)output(text);output(".\n");}

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
        if(amVerbose())outputLine("Freeing a big integer."); // TODO can we display the value?
        mp_clear(biginteger); // directly call mp_clear on the Mbiginteger pointer!!!
    }else
    if(amVerbose())outputLine("No big integer to free!");
}/* VALIDATED */
Mbiginteger* __biginteger(){
    Mbiginteger* biginteger=(Mbiginteger*)malloc(sizeof(mp_int));
    if(biginteger&&mp_init(biginteger)!=MP_OKAY){free_biginteger(biginteger);biginteger=NULL;} // ESSENTIAL to release the big integer, when failing to initialize it!!
    return biginteger;
}/* VALIDATED */
Mbiginteger* _getBiginteger(int64_t ll){
    Mbiginteger* biginteger=__biginteger();
    if(biginteger)mp_set_i64((mp_int*)biginteger,ll); // even if l equals 0 set it TODO check is that necessary???
    return biginteger;
}/* VALIDATED */

// replace in due course by _getBigintegerNeg in Mbiginteger.c/h but that would require moving _getRational and some other functions as well from Mexecution.h/c
Mbiginteger* _getBigintegerNeg(Mbiginteger* _biginteger){
    Mbiginteger* _bigintegerNeg=(_biginteger?__biginteger():NULL); // the result we will be returning
    if(_bigintegerNeg&&mp_neg(_biginteger,_bigintegerNeg)!=MP_OKAY){free_biginteger(_bigintegerNeg);_bigintegerNeg=NULL;}
    return _bigintegerNeg;
}// VALIDATED

// pass in NULL to _getBigIntegerCopy to get a big integer (initialized to zero)
Mbiginteger* _getBigintegerCopy(Mbiginteger* biginteger){
    if(!biginteger)return NULL;
    Mbiginteger* bigintegerCopy=__biginteger();
    if(bigintegerCopy&&mp_copy(biginteger,bigintegerCopy)!=MP_OKAY){free_biginteger(bigintegerCopy);bigintegerCopy=NULL;}
    return bigintegerCopy;
}/* VALIDATED */
mp_int* _mp_int(){return (mp_int*)__biginteger();}/* VALIDATED */

// using constant big integers 0, 1 and 2 (do NOT wrap these constants in Mvalue's though or they will need to be created over and over again)
static Mbiginteger *bi0=NULL,*bi1=NULL,*bi2=NULL,*bi3=NULL;
// NOTE do NOT start with underscore (_) to indicate that the result is to be left alone!!
const Mbiginteger* getBigintegerZero(){if(!bi0)bi0=_getBiginteger(0);return bi0;}/* VALIDATED */
const Mbiginteger* getBigintegerOne(){if(!bi1)bi1=_getBiginteger(1);return bi1;}/* VALIDATED */
const Mbiginteger* getBigintegerTwo(){if(!bi2)bi2=_getBiginteger(2);return bi2;}/* VALIDATED */
const Mbiginteger* getBigintegerThree(){if(!bi3)bi3=_getBiginteger(3);return bi3;}/* VALIDATED */

bool isBigintegerZero(Mbiginteger* biginteger){return(biginteger?mp_iszero((mp_int*)biginteger)==MP_YES:false);}/* VALIDATED */
bool isBigintegerOne(Mbiginteger* biginteger){return(biginteger?mp_cmp((mp_int*)biginteger,getBigintegerOne())==MP_EQ:false);}/* VALIDATED */
// END BIG INTEGER STUFF


// MDH@07JUN2019: we are going to store real text representations (like 100.1) as rationals from now on with delta equal to 0 (so we know where they came from, and that the denominator is a power of 10)
//                because if we convert them to a long double we might loose precision in converting the decimal representation to the binary (internal) representation
// MDH@14JUN2019: now also possible that the text has an e-part (which will change the denominator!!!!)
// MDH@26JUN2019: decimalText is adjusted in the process, so freeonfailure is ommitted as that would complicate matters significantly NOTE that it's pass by value so even if decimalText pointer is adjusted locally, the variable itself is not adjusted 
Mrational* _getDecimalTextRational(char* decimalText/*,bool freeonfailure*/){
	// the text should represent an integer or a real
	if(!decimalText)return NULL;
    if(amVerbose())output("Converting decimal text '%s' to a rational.",decimalText);
	Mrational* rational=NULL; // where the result is stored!!!
	int l=strlen(decimalText); // NOT needed, only used once
	if(l>0){
		bool neg=(*decimalText=='-');if(neg)decimalText++; // get the sign
		// get the e-part (if any)
		char* exponentText=strchr(decimalText,'e'); // assume lowercase e
		Mbiginteger* _exponent=NULL;
		if(exponentText){
			*exponentText='\0'; // 'cuf off' the e-part!!!!
			l=(int)(exponentText-decimalText); // this will be the new l we need below!!!
			if(amVerbose())output("Decimal text with exponent removed: '%s'.",decimalText);
			exponentText++; // point to the first character of the exponent
            _exponent=__biginteger(); // need it before calling mp_read_radix()
			if(mp_read_radix(_exponent,exponentText,10)!=MP_OKAY){
				output("%sFailed to extract the exponent from its text representation '%s'.\n",ERROR_PREFIX,exponentText);
				free_biginteger(_exponent);
				_exponent=NULL;
			}else
			if(amVerbose())outputBiginteger("Exponent '",_exponent,"'.\n");
		}
		if(!exponentText||_exponent){ // either we do not have an exponentText or we have an exponent big integer (to apply later on)
			// TODO if we would just have an eval to get the value out of the token text
			char* decimalPartText=strchr(decimalText,'.');
			int decimalPartIndex=0;
			if(decimalPartText)*decimalPartText='\0'; // 'cut off' the decimal part (for now)
			if(amVerbose())output("With decimal part removed: '%s'.",decimalText);
			// now ready to check the integer part 
			Mbiginteger *_numerator=__biginteger(),*_denominator=NULL; // two big integers to free if unbound!!
			if(mp_read_radix(_numerator,decimalText,10)==MP_OKAY){ // apparently a valid (big) integer
				Mbiginteger* _decimalPartBiginteger=NULL; // freeable...
				if(decimalPartText){
					int decimalPartIndex=(int)(decimalPartText-decimalText);
					decimalPartText++; // point to the first character of the decimal part
					_decimalPartBiginteger=__biginteger();
					if(_decimalPartBiginteger&&mp_read_radix(_decimalPartBiginteger,decimalPartText,10)==MP_OKAY){
                        if(!isBigintegerZero(_decimalPartBiginteger)){
						    // compute the power of ten denominator
						    Mbiginteger* _bi10=_getBiginteger(10); // must be freed (see three lines down)
                            if(_bi10){
                                // make a denominator, and keep multiplying by 10, but if something goes wrong free and NULL it again to indicate an error
    						    _denominator=_getBiginteger(1);
	    					    while(++decimalPartIndex<l){if(!_denominator)break;if(mp_mul(_denominator,_bi10,_denominator)!=MP_OKAY){free_biginteger(_denominator);_denominator=NULL;}}
		    				    free_biginteger(_bi10);
                            }
        					if(amVerbose())if(_denominator)outputBiginteger("Denominator: '",_denominator,"'.\n");
                        }else // the decimal part is zero therefore we do not officially have a decimal part (but we do want the associated rational even with _denominator NULL)
                            decimalPartText=NULL;
    					if(amVerbose())outputBiginteger("Decimal part integer: '",_decimalPartBiginteger,"'.\n");
					}
				}
				// if we have a decimalPartText we need a denominator
				if(!decimalPartText||_denominator){
					// if we have a _denominator and we fail to compute the appropriate numerator, we have to free all big integers
					// NOTE do NOT free the numerator and denominator in the call to _getRational, as we free them if _rational ends of being NULL afterwards
					if(!_denominator||(mp_mul(_numerator,_denominator,_numerator)==MP_OKAY&&mp_add(_numerator,_decimalPartBiginteger,_numerator)==MP_OKAY)){
						if(amVerbose())outputBiginteger("Numerator before applying the exponent: '",_numerator,"'.\n");
						if(amVerbose())if(_denominator)outputBiginteger("Denominator before applying the exponent: '",_denominator,"'.\n");
						// if we have an non-zero exponent, we have to adjust the numerator or denominator BEFORE trying to create the rational!!!
						if(exponentText&&mp_iszero(_exponent)==MP_NO){
							Mbiginteger* _bi10=_getBiginteger(10);
							if(_bi10){
								if(mp_isneg(_exponent)==MP_YES){ // a negative exponent goes into the denominator
									if(!_denominator)_denominator=_getBiginteger(1);
									if(_denominator){
										while(mp_iszero(_exponent)==MP_NO){
											if(mp_mul(_denominator,_bi10,_denominator)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
											if(mp_incr(_exponent)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
										}
									}else{free_biginteger(_exponent);_exponent=NULL;}
								}else{ // a positive exponent goes into the numerator
									while(mp_iszero(_exponent)==MP_NO){
										if(mp_mul(_numerator,_bi10,_numerator)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
										if(mp_decr(_exponent)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
									}
								}
								free_biginteger(_bi10);
							}else{free_biginteger(_exponent);_exponent=NULL;}
						}
						// check again whether we still have an exponent (when we should)
						if(!exponentText||_exponent)
                            if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)
                                rational=_getRational(_numerator,_denominator,0,true,false); // NOTE the 0 explicitly tells the rational that it represents a decimal representation!!!!!
					}
				}
                free_biginteger(_decimalPartBiginteger);
			}else
				output("%sInteger part of rational text '%s' invalid.\n",ERROR_PREFIX,decimalText);
			// if we haven't got a rational that binded _numerator and _denominator free both of them
			if(!rational){free_biginteger(_numerator);free_biginteger(_denominator);}
		}
        free_biginteger(_exponent); // if it's still around, release _exponent
	}
    ///////////////////////if(!rational)if(freeonfailure)free(rationalText);
	return rational;
}/* VALIDATED */

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
    if(_text){
        free(_text); // replacing (when we used a char pointer (_m) for storing the characters): if(_string){if(_string->_m)free_string(_string->_m);_string->_m=NULL;free(_string);}
    }else
    if(amDebugging())outputLine("No text to free!");
}/* VALIDATED */
void free_integer(Minteger* _integer){
    if(_integer){
        if(amVerbose())output("Freeing integer %llu.\n",_integer->ll);
        free(_integer);
    }else
    if(amDebugging())outputLine("No integer to free!");
}/* VALIDATED */
void free_real(Mreal* _real){
    if(_real){
        if(amVerbose())output("Freeing real %.*Lf.\n",LDBL_DIG,_real->ld);
        free(_real);
    }else
    if(amDebugging())outputLine("No real to free!");
}/* VALIDATED */
void free_rational(Mrational* _rational){
    if(_rational){
        if(_rational->num)free_biginteger(_rational->num);
        if(_rational->den)free_biginteger(_rational->den);
        if(_rational->delta)free_real(_rational->delta);
        free(_rational);
    }else
    if(amDebugging())outputLine("No rational to free!");
}/* VALIDATED */

// ALLOCATORS (private)
// value wrappers
// typically an Mvalue is immutable (we might change that for variables that are strong typed e.g. when created with integer(),real(),string(),list() or map() function)
Minteger* _getInteger(long long ll){
    Minteger* _integer=MALLOC(sizeof(Minteger),'i');
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

Mreal* _getReal(long double ld){
    Mreal* _real=MALLOC(sizeof(Mreal),'R');
    if(_real)_real->ld=ld;
    return _real;
}/* VALIDATED */

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
Mreal* get_real(long double ld){
	Mreal* _real=(Mreal*)malloc(sizeof(Mreal));
	if(_real)_real->ld=ld;
	return _real;
}
Mvalue* getRealValue(Mreal* _real){
    if(_real){
        Mvalue* _realValue=calloc(1,sizeof(Mvalue));
        if(_realValue){_realValue->type=VT_REAL;_realValue->value._real=_real;return _realValue;}
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
bool setValueOfRealVariable(Mvariable* _variable,Mreal* _real){
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

Mstring* appendull(Mstring* const ms,unsigned long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}/* VALIDATED */
// helper function
Mstring* appendll(Mstring* const ms,long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}/* VALIDATED */
Mstring* appendld(Mstring* const ms,long double ld){
	char ldText[80];
    // how about using scientific notation here?????
	snprintf(ldText,80,"%.*Le",LDBL_DIG,ld); //////snprintf(ldText,80,"%.*Le",LDBL_DIG,ld); // replaced f with e to get scientific notation!!
    // alternatively we could shift
    char* exp=strchr(ldText,'e');
    int l=strlen(ldText); // where we will be searching for decimal zeroes
    int exponent=0;
    if(exp){
        int e=(int)(exp-ldText);
        l=e++;
        ldText[l]='\0'; // cut off the exponent (we can still extract the exponent though)
        //////output("With exponent: '%s'",ldText);
        // extract the exponent
        bool neg=(ldText[e]=='-');if(neg||ldText[e]=='+')e++;
        while(ldText[e]!='\0'){exponent=10*exponent+(ldText[e]-'0');e++;}
        if(neg)exponent=-exponent;
        //////output("Exponent: %u.",exponent);
        /* replacing:
        while(--l>0&&ldText[l]=='0');
        if(ldText[l]=='-'||ldText[l]=='+')l--;
        if(ldText[l]=='e'){ // the e-part is zero
            ///output("Zero exponent!");
            exp=NULL;
        }else{
            while(--l>0&&ldText[l]!='e'); // move to the 'e'
        }
        */
    }///////else output("Without exponent: '%s'",ldText);

    // ASSERT l is now on the 'e' of the exponent (if any)
    
    char* period=strchr(ldText,'.');
    if(period){ // there's a decimal period
        int p=(int)(period-ldText); // p is the position of the decimal point
        // l-p-1 is the number of decimals if the exponent is smaller than that
        if(exponent>0){ // move the period up as far as necessary
            if(exponent<l-p)while(exponent>0){ldText[p]=ldText[p+1];ldText[++p]='.';exponent--;}
        }else
        if(exponent<0){ // move the period back as far as possible
            if(exponent+p>=0)while(exponent<0){ldText[p]=ldText[p-1];ldText[--p]='.';exponent++;}
        }
        // ASSERT p is the index of the period
        while(ldText[--l]=='0'); // a bit naughty to simply replacing '0' with '\0' to pretend to end the text!!!
        ldText[l+1]='\0';
    }
	if(!string_append(ms,ldText))return NULL;
    if(exponent!=0)if(!string_append_char(ms,'e')||!appendll(ms,exponent))return NULL; // append the exponent
    return ms;
}/* VALIDATED */

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
		case VT_REAL:return double2long(_value->value._real->ld);
		case VT_STRING:return _strtoll(_value->value._string->_c,M_LL_INVALID);
		default:break;
    }
    return M_LL_INVALID;
}
*/

// BigInteger stuff
Mstring* _getBigintegerText(const Mbiginteger* _biginteger){
    Mstring* _bigintegerText=__string();
    ////outputChar('A');
    if(_bigintegerText){
        ///outputChar('B');
        if(_biginteger){
            // determine the required size
            int arepsize=0;
            ///outputChar('C');
            ///////if(amVerbose())outputLine("Determining a big integer text representation.");
            if(mp_radix_size(_biginteger,10,&arepsize)==MP_OKAY){
                ///outputChar('D');
                ////////output("Representation size: %d.\n",arepsize);
                if(arepsize<=0xFFFFFFFF){
                    ///outputChar('E');
                    string_setlength(_bigintegerText,arepsize);
                    ///outputChar('F');
                    if(mp_toradix(_biginteger,_bigintegerText->chars,10)==MP_OKAY)string_synclength(_bigintegerText);
                    ///outputChar('G');
                }else
                    output("%sCan't store more than %u characters in a string.\n",ERROR_PREFIX,0xFFFFFFFF);
            }else
                outputError("Couldn't determine the size of a big integer");
        }else
            outputError("No big integer to represent");
    }else
        output("%sFailed to create a text for storing the representation of a big integer.\n",ERROR_PREFIX);
    ///outputChar('H');
    return _bigintegerText;
}/* VALIDATED */

Mbiginteger *_biLLMin=NULL,*_biLLMax=NULL;

Mbiginteger* getBigintegerLLMin(){if(!_biLLMin)_biLLMin=_getBiginteger(M_LL_MIN);return _biLLMin;}/* VALIDATED */
Mbiginteger* getBigintegerLLMax(){if(!_biLLMax)_biLLMax=_getBiginteger(M_LL_MAX);return _biLLMax;}/* VALIDATED */

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
// TODO should we free the given big integers when they are NOT bound to the rational that is being returned????
void normalizeRational(Mrational* rational){
    if(!rational)return;
    // checking on the validity of the flag (which would actually be a bug)
    if(!rational->normalized&&!rational->den){output("BUG: Normalized flag of rational not set although the denominator equals 1; flag set.\n");rational->normalized=true;}
    if(rational->normalized)return; // apparently already normalized
    // normalization means dividing by the gcd unless the gcd is one
    Mbiginteger* _gcd=__biginteger(); // to be freed in all cases!
    if(!_gcd){outputError("Can't normalize a rational: failed to create the big integer to store the GCD");return;}
    // ASSERT at the end of the following block always free _gcd
    if(mp_gcd(rational->num,rational->den,_gcd)==MP_OKAY){
        if(mp_cmp(_gcd,getBigintegerOne())!=MP_EQ){ // equal to 1 apparently no need to divide num and den by the gcd and then consider normalized
            // won't do an in-place division as we need both to succeed, if only one does we would be in trouble
            Mbiginteger *_newnum=__biginteger(),*_newden=__biginteger(); // to be freed if failing to bind them!!!
            if(_newnum&&_newden&&mp_div(rational->num,_gcd,_newnum,NULL)==MP_OKAY&&mp_div(rational->den,_gcd,_newden,NULL)==MP_OKAY){
                free_biginteger(rational->num);rational->num=_newnum;
                free_biginteger(rational->den);rational->den=NULL;if(isBigintegerOne(_newden))free_biginteger(_newden);else rational->den=_newden; // if _newden equals 1, get rid of it, otherwise assign
                rational->normalized=true;
            }else{ // if the normalization failed, newnum and newden are not bound to the rational!!
                free_biginteger(_newnum);
                free_biginteger(_newden);
                outputError("Normalization of rational failed");
            }
        }else // the GCD equals 1 which means that the thing is normalized!!!
            rational->normalized=true;
    }else
        outputError("Can't normalize a rational: failed to compute the GCD");
    free_biginteger(_gcd); // OOPS essential!!
}/* VALIDATED */

// MDH@07JUN2019: _getRational does NOT free the numerator and denominator supplied!!!
//                as it does not know whether _numerator or _denominator should be released on failure
Mrational* __rational(){
    Mrational* _rational=(Mrational*)CALLOC(1,sizeof(Mrational),'R');
    if(!_rational)outputError("Failed to create a rational");
    if(_rational->num||_rational->den||_rational->delta)outputLine("BUG: New rational numerator and/or denominator and/or delta not considered undefined.");
    return _rational;
}/* VALIDATED */
Mrational* _getRational(Mbiginteger* _numerator,Mbiginteger* _denominator,long double delta,bool normalize,bool freeonfailure){
    // if the given numerator is NULL assume 1
    Mrational* _rational=NULL;
    if(amVerbose()){outputBiginteger("Determining the rational with numerator ",_numerator,NULL);outputBiginteger(" and denominator ",_denominator,".\n");}
    if(!_denominator||!isBigintegerZero(_denominator)){ // we have a numerator (any would do), and either NO denominator or a non-zero denominator
        Mreal* _delta=NULL;
        if(!ldIsNaN(delta)&&!ldIsInf(delta)){ // we need a delta
            _delta=_getReal(delta); // store the delta if a valid value
            if(_delta)_rational=__rational();else outputError("Failed to create a new delta");
        }else
            _rational=__rational();
        if(_rational){
            if(_delta)_rational->delta=_delta; // _delta now bound to the rational, and will be freed if we free the rational, so we don't have to take care of that ourselves
            // TODO what if a delta is defined and the denominator is undefined (i.e. 1)
            // force using a nonnullnumerator, if NULL was provided (typically when inverting a rational)
            Mbiginteger* _nonnullnumerator=(_numerator?_numerator:_getBiginteger(1));
            if(_nonnullnumerator){
                // MDH@15AUG2019: we prefer the numerator to be negative instead of the denominator
                Mbiginteger *_negatedNumerator=NULL,*_negatedDenominator=NULL;
                // NOTE if there's no denominator the denominator equals 1 and therefore it is not negative
                if(_denominator&&mp_isneg(_denominator)==MP_YES){ // the given denominator is negative
                    if(amVerbose())outputLine("Moving the sign from the denominator to the numerator of the rational.");
                    // we have to get negated versions of both the numerator and the denominator
                    // if we succeed in doing so we use those otherwise we stick to using the current ones
                    _negatedNumerator=_getBigintegerNeg(_nonnullnumerator);
                    _negatedDenominator=_getBigintegerNeg(_denominator);
                    // if both are defined, we should use these otherwise we should use neither
                }
                if(_negatedNumerator&&_negatedDenominator){ // succeeded in negating the numerator and denominator
                    if(amVerbose())outputLine("Using the negated numerator and denominator.");
                    _rational->num=_negatedNumerator;
                    _rational->den=_negatedDenominator;
                    // NOTE that we do NOT free _numerator explicitly but if _numerator is not null, _nonnullnumerator will be equal to it and we're freeing the right thing, if _numerator is NULL we're freeing the created _getBiginteger(1) which is also the right thing to do
                    //      I have to NULL both _numerator and _denominator here because if I don't an attempt to free them again when e.g. normalization fails could be catastrophic
                    //      alternatively I could simply toggle freeonfailure PREFERRED APPROACH
                    // NOTE even if freeonfailure is already false, we are returning a rational that does NOT use the presented numerator and denominator in which case we should free the numerator and denominator because the caller won't
                    //      if freeonfailure is true we should of course prevent freeing below (which won't happen though)
                    freeonfailure=false;
                    free_biginteger(_nonnullnumerator);free_biginteger(_denominator);
                }else{
                    _rational->num=_nonnullnumerator; // could be NULL now when it's the inverse of another rational
                    _rational->den=_denominator;
                    // if using the original numerator and denominator we still have to free any negated version we created
                    free_biginteger(_negatedNumerator);free_biginteger(_negatedDenominator);
                }
                // NOTE _nonnullnumerator and _denominator NOW bound, so no need to free them anymore
                /* STORING 1 AS DENOMINATOR ISN'T WRONG per se 
                if(_denominator&&mp_cmp(_denominator,getBigintegerOne())==MP_EQ){
                    free_biginteger(_denominator); // won't store denominator equal to 1
                    _rational->den=NULL; // probably already is though
                }else // either NULL or not equal to 1
                    _rational->den=_denominator;
                */
                _rational->normalized=(!_rational->den||isBigintegerOne(_rational->num)); // if either numerator or denominator is NULL assume normalized!!!
                if(normalize&&!_rational->normalized){
                    if(amVerbose())outputRational("Rational before normalization: ",_rational,".\n");
                    normalizeRational(_rational); // normalize the rational if we are supposed to
                    if(_denominator&&!_rational->normalized)outputError("Failed to normalize a rational");else if(amVerbose())outputRational("Rational after normalization: ",_rational,".\n");
                }else
                if(amVerbose())outputRational("Rational initialized: ",_rational,".\n");
                ///////// AS LONG AS WE FREE THE RATIONAL IN THE ELSE PART NO NEED TO DO: return _rational; // return whether normalized or not
            }else{ // either _numerator NULL or _getBiginteger(1) NULL, in the last case nothing created that needs to be freed (except for _rational)
                free_rational(_rational);_rational=NULL;
                outputError(_numerator?"Undefined rational numerator":"Failed to create big integer 1");
            }
            // NOTE if we get here we failed to create the big integer 1 to use as numerator!!!
        }else{
            if(_delta)free_real(_delta);
            outputError("Failed to create a rational");
        }
    }
    /* NOT OUR RESPONSIBILITY unless we decide to do that
    // if we get here we failed storing _numerator and _denominator, so we free them
    if(_numerator)free_biginteger(_numerator);
    if(_denominator)free_biginteger(_denominator);
    */
    if(!_rational)if(freeonfailure){free_biginteger(_numerator);free_biginteger(_denominator);}
    return _rational;
}/* VALIDATED */

// _getInverseRational() will take care of releasing the newly created rational parts when failing to wrap them in a rational
Mrational* _getInverseRational(Mrational const * const _rational){
    if(!_rational){outputError("No rational to invert");return NULL;}
    Mrational* _inverseRational=NULL;
    // for now only allow inverting pure rationals!!!
    if(!_rational->delta||ldIsZero(_rational->delta->ld)||ldIsNaN(_rational->delta->ld)){
        Mbiginteger *_inverseNumerator=(_rational->den?_getBigintegerCopy(_rational->den):_getBiginteger(1)),*_inverseDenominator=(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1)); // free on failure
        if(_inverseNumerator&&_inverseDenominator)_inverseRational=_getRational(_inverseNumerator,_inverseDenominator,M_LD_NAN,!_rational->normalized,false);
        if(!_inverseRational){free_biginteger(_inverseNumerator);free_biginteger(_inverseDenominator);outputError("Failed to create the inverse rational");}else if(_rational->normalized)_inverseRational->normalized=true; // nasty TODO check if this is correct
    }else
        outputError("Can't invert an unpure rational");
    return _inverseRational;
}/* VALIDATED */

// MDH@08JUN2019 NOTE: adapted so that if the numerator is NULL will assume the numerator to equal 1
// BUT _getRational has been adapted to NOT allow a NULL numerator, i.e. replacing NULL with big integer 1, so actually a NULL numerator is unlikely to occur!!!
// rationals can equal zero or one but only when the delta value equals 0 (or is not defined which is the same)
bool isRationalZero(Mrational* _rational){
    // if the rational does not have a num, the numerator equals 1, and obviously is NOT zero
    return(_rational&&_rational->num?isBigintegerZero(_rational->num)&&(!_rational->delta||ldIsZero(_rational->delta->ld)||ldIsNaN(_rational->delta->ld)):false); // the delta needs to be undefined (i.e. zero)
}/* VALIDATED */
bool isRationalOne(Mrational* _rational){
    // when the numerator is NULL, it is considered to be equal to 1
    if(!_rational)return false;
    // basically a rational equals 1 if the numerator and denominator are the same
    if(_rational->delta)if(!ldIsZero(_rational->delta->ld)&&!ldIsNaN(_rational->delta->ld))return false; // TODO if the rational delta is not zero, do not consider to be equal to 1 (although theoretically it could be)
    return (_rational->den?mp_cmp(_rational->num,_rational->den)==MP_EQ:!_rational->num||isBigintegerOne(_rational->num));
    // replacing: return(_rational?!(_rational->num||isBigintegerOne(_rational->num))&&(!_rational->den||isBigintegerOne(_rational->den))&&(!_rational->delta||ldIsZero(_rational->delta->ld)):false);
}/* VALIDATED */

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
            Mstring* _mantisseBigIntegerText=_getBigintegerText(a);
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
            Mstring* _bigIntegerText=_getBigintegerText(a);
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
    return (amVerbose()?mp_set_me_verbose(a,mantisse,exponent):mp_set_me(a,mantisse,exponent));
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
    if(!a)return M_LD_NAN; // if a undefined, return NaN
    int i=a->used;
    if(i==0)return 0.0; // if a zero, return 0
    --i; // 0 if only one big integer digit, otherwise positive
    if(i&&!M_LD_DIGIT_MULTIPLIER){ // if we need the digit multiplier, get it
        M_LD_DIGIT_MULTIPLIER=1.0;
        int j=MP_DIGIT_BIT;
        while(--j>=0)M_LD_DIGIT_MULTIPLIER*=2.0;
    }
    long double d=(long double)a->dp[i]; // initialize d to the most significant big integer digit
    if(amVerbose())output("Real of big integer digit %lld initialized to '%.*Lf' yet to shift by %u big integer digits.\n",a->dp[i],LDBL_DIG,d,i);
    while(--i>=0){
        if(amVerbose())output("Multiplying '%.*Lf' by %Lf.\n",d,M_LD_DIGIT_MULTIPLIER);
        d*=M_LD_DIGIT_MULTIPLIER;
        if(amVerbose())output("Result of multiplying by '%Lf': '%.*Lf'.\n",M_LD_DIGIT_MULTIPLIER,LDBL_DIG,d);
        d+=(long double)a->dp[i];
        if(amVerbose())output("Result of adding '%lld': '%.*Lf'.\n",a->dp[i],LDBL_DIG,d);
    }
    if(a->sign==MP_NEG&&!ldIsNaN(d))return -d;
    if(amVerbose())output("Conversion of big integer to long double '%.*Lf' done!\n",LDBL_DIG,d);
    return d;
    // replacing: return(a->sign==MP_NEG&&!ldIsNaN(d)?-d:d);
}/* VALIDATED */

// part of implementing _getRealText (so not present in the header)
const char* M_NAN="NaN";
const char* M_INF="Inf";
bool ldIsZero(long double ld){return fpclassify(ld)==FP_ZERO;}/* VALIDATED */
bool ldIsNaN(long double ld){return fpclassify(ld)==FP_NAN;}/* VALIDATED */
bool ldIsInf(long double ld){return fpclassify(ld)==FP_INFINITE;}/* VALIDATED */
long long double2long(long double ld){
    if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
    // TODO perhaps there are some other 
    if(ldIsZero(ld))return 0;
    long double tld=truncl(ld); // extract the integer part i.e. floor towards zero (which is called truncate)
    if(tld<M_LL_MIN||tld>M_LL_MAX)return M_LL_INVALID; // out of range
    return(long long)tld;
}/* VALIDATED */

// STRINGIFY FUNCTIONS
Mstring* _getRealText(Mreal* _real){
	Mstring* _realText=(_real?__string():NULL);
    if(_realText){
        Mstring* _p=_realText;
        if(amDebugging())_p=string_append_char(_p,'r');
        if(_p){
            switch(fpclassify(_real->ld)){
                case FP_NAN:_p=string_append(_p,M_NAN);break;
                case FP_INFINITE:_p=string_append(_p,M_INF);break;
                default:_p=appendld(_p,_real->ld);break;
            }
        }
        if(!_p){free_string(_realText);_realText=NULL;}
    }
	return _realText;
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

// TODO good idea to always return something (if we can), as in _getBigintegerText()
Mstring* _getRationalText(const Mrational* const _rational){
    Mstring*  _rationalText=NULL;
    ///outputChar('a');
    if(_rational){
        ///outputChar('b');
        _rationalText=__string();
        ///outputChar('c');
        if(_rationalText){
            ///outputChar('d');
            Mstring* _p=_rationalText;
            _p=string_append_char(_p,'(');
            ///outputChar('e');
            Mstring* _numeratorBigintegerText=_getBigintegerText(_rational->num);
            ///outputChar('f');
            if(_numeratorBigintegerText){_p=string_append(_p,string(_numeratorBigintegerText));free_string(_numeratorBigintegerText);}
            ///outputChar('g');
            if(_rational->den){
                _p=string_append_char(_p,'/');
                Mstring* _denominatorBigintegerText=_getBigintegerText(_rational->den);
                if(_denominatorBigintegerText){_p=string_append(_p,string(_denominatorBigintegerText));free_string(_denominatorBigintegerText);}
            }
            ///outputChar('k');
            _p=string_append_char(_p,')');
            ///outputChar('l');
            // if a delta is known, append that as well!!!
            if(_rational->delta){
                // always show a sign
                if(_rational->delta>=0)string_append_char(_p,'+');
                _p=appendld(_p,_rational->delta->ld);
            }
            ///outputChar('n');
            if(!_p){free_string(_rationalText);_rationalText=NULL;} // if something went wrong, return NULL and free _rationalText
            ///outputChar('o');
        }
    }else
        if(amVerbose())outputLine("No rational to determine the text representation of.");
    outputChar('p');
    return _rationalText;
}/* VALIDATED */

/////////////Mstring* _UNDEFINED_VALUETEXT=NULL;
// the problem here is that whatever _getValueText returns will be freed on the other side, which we would not want to happen with _UNDEFINED_VALUETEXT, so perhaps we should return NULL in that case after all????
// we can solve that by returning a new undefined value text instance every time
Mstring* _getUndefinedValueText(){
    return _getString(UNDEFINED_VALUETEXT); // just wrapping UNDEFINED_VALUETEXT again...
    /* replacing:
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
    return string_copy(_UNDEFINED_VALUETEXT);
    */
}/* VALIDATED */

void outputBiginteger(const char* const prefix,const Mbiginteger* const _biginteger,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_biginteger){
        Mstring* _bigintegerText=_getBigintegerText(_biginteger);
        if(_bigintegerText){
            output("%s",string(_bigintegerText));
            free_string(_bigintegerText);
        }else
            output("no big integer text representation");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}/* VALIDATED */
void outputDecimal(const char* const prefix,const Mdecimal* const _decimal,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_decimal){
        Mstring* _decimalText=_getDecimalText(_decimal,false);
        if(_decimalText){
            output("%s",string(_decimalText));
            free_string(_decimalText);
        }else
            output("no decimal text representation");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}/* VALIDATED */
void outputRational(const char* const prefix,const Mrational* const _rational,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_rational){
        Mstring* _rationalText=_getRationalText(_rational);
        if(_rationalText){
            output("%s",string(_rationalText));
            free_string(_rationalText);
        }else
            output("no rational text representation");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}/* VALIDATED */

// conversion from big integer to the long long it contains (when in range)
long long biginteger2long(const Mbiginteger* const _biginteger){
	return(_biginteger&&mp_cmp(_biginteger,getBigintegerLLMin())!=MP_LT&&mp_cmp(_biginteger,getBigintegerLLMax())!=MP_GT?mp_get_i64(_biginteger):M_LL_INVALID);
}/* VALIDATED */
bool strIsZero(char* str){
    size_t l=strlen(str);
    //// NOTE do not accept integer literal postfixes when checking for 1: if(l>0&&str[l-1]=='i'||str[l-1]=='I'||str[l-1]=='q'||str[l-1]=='r')l-=1; // skip any accepted integer postfix!!
    // if l already is zero str[0] will equal '\0' which (see below) is not considered a zero integer!!!!
    while(l>0){l--;if(str[l]!='0')break;} // stop as soon as the character does not match '0' (any sign is only allowed at position 0)
    return(!l?false:str[l]=='-'||str[l]=='+'||str[l]=='0');
}/* VALIDATED */

// NOTE the _ indicates that what is returned has to be freed after being used
Mbiginteger* _rational2biginteger(Mrational* _rational){
    Mbiginteger* _biginteger=(_rational?__biginteger():NULL);
    if(_biginteger){        
        if(_rational->den){
            // if the numerator is NULL or 0 _biginteger should remain what it is (i.e. 0)
            if(_rational->num&&!isBigintegerZero(_rational->num)){
                Mbiginteger* absnum=NULL;
                bool neg=mp_isneg(_rational->num);
                if(neg){absnum=__biginteger();if(absnum&&mp_neg(_rational->num,absnum)!=MP_OKAY){free_biginteger(absnum);absnum=NULL;}}else absnum=_rational->num;
                // we need absnum, if we haven't got one, negating the negative numerator failed
                if(!absnum||mp_div(absnum,_rational->den,_biginteger,NULL)!=MP_OKAY){
                    outputError("Failed to (integer) divide the rational numerator by its denominator");
                    free_biginteger(_biginteger);
                    _biginteger=NULL;
                }else
                if(absnum&&neg){ // we have to negate _biginteger
                    if(mp_neg(_biginteger,_biginteger)!=MP_OKAY){free_biginteger(_biginteger);_biginteger=NULL;}
                    free_biginteger(absnum); // free absnum
                }
            }
        }else // copy the numerator
            if(mp_copy(_rational->num,_biginteger)!=MP_OKAY){outputError("Failed to copy the rational numerator");free_biginteger(_biginteger);_biginteger=NULL;}
    }
    return _biginteger;
}/* VALIDATED */

bool isDecimalZero(Mdecimal* _decimal){
    if(!_decimal)return false;
    Mstring* _decimalText=_getDecimalText(_decimal,true); // free asap
    ///////output("Is decimal '%s' zero?",string(_decimalText));
    free_string(_decimalText); // freed
    bool result=(_decimal->mpd?mpd_iszero(_decimal->mpd)==MP_YES:false); // TODO apparently 0 means true, something else means false
    //////output(" %s.\n",(result?"YES":"NO"));
    return result;
}/* VALIDATED */

// when only interested in the end result, calling _getLongDoubleRational is the way to go
Mrational* _getLongDoubleRational(long double ld,uint32_t maxiter){
    // if iterations, you're supposed to return all iteration results
    Mrational* _rational=NULL; // the last (computed) rational
    if(!ldIsNaN(ld)&&!ldIsInf(ld)){ // neither a NaN nor Inf      
        if(!ldIsZero(ld)){
            bool neg=(ld<0);if(neg)ld=-ld; // remember if negative
            // ASSERT ld is positive 
            // if we use p for pmin1 and q for qmin1 we do not need pmin1 and qmin1
            long long pmin1=1,qmin1=0,pmin2=0,qmin2=1;
            long long a,p,q; // p and q now store the initial values of pmin1 and qmin1
            long double delta,rem=ld;
            // ascertain to execute the following at least once (so when maxiter<=1 we at least get the integer part of the rational)
            // MDH@09OCT2019: if maxiter equals zero never stop
            int i=0;
            while(!maxiter||i<maxiter){
                i++;
                a=lrint(floorl(rem));
                p=a*pmin1+pmin2;
                q=a*qmin1+qmin2;
                ////////printf("\nIteration #%u: %lld:  %lld/%lld", i, a, p, q);
                ///////printf(" - delta: %.*Lf, rem: %.*Lf",LDBL_DIG,delta,LDBL_DIG,rem);
                ///// doesn't work!!!!! if(fabsl(rem)<eps)return;
                delta=(ld*q)-p;
                if(fabsl(delta)<=M_LD_Q_EPS)break;
                rem-=a;
                rem=1/rem;
                // shift the lot
                pmin2=pmin1;qmin2=qmin1;
                pmin1=p;qmin1=q;
            }
            // construct the last rational (i.e. the result) from p and q
            Mbiginteger *_numerator=_getBiginteger(p),*_denominator=_getBiginteger(q);
            if(_numerator&&_denominator) // we've got both of them
                if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)
                    _rational=_getRational(_numerator,_denominator,delta,true,false); // NOTE there should always be a delta!!!!
            if(!_rational){free_biginteger(_numerator);free_biginteger(_denominator);}
        }else // long double is zero
            _rational=_getRational(__biginteger(),NULL,M_LD_NAN,false,true);
    }
    // _rational should contain the 'last' computed rational
    return _rational;
}/* VALIDATED */
// MDH@07JUN2019: converting a rational to a double
// MDH@19SEP2019: TODO the conversion of the numerator or denominator big integer might fail if the big integer is too large, therefore actually performing the division of the big integers seems a better approach 
long double getRationalLongDouble(const Mrational* const _rational){
    if(_rational){
        // as you can see up we're storing the delta in our rationals as well, so if we want to get ld back out of it the formula is: (numerator+delta)/denominator
        // but with the numerator and denominator possibly big integers adding delta to the numerator means adding a double to a big integer (of course delta typically is very small)
        // it's easiest to turn the numerator big integer into a long double and add delta to it, and divide by the long double stored in the denominator
        // TODO find a better way to do this
        long double ldNumerator=mp_get_long_double(_rational->num); // NOTE also shortcuts when _rational->num equals 0 but we have to add the delta, so we have to do it this way
        if(amVerbose())output("Rational numerator converted to real '%.*Lf'.\n",LDBL_DIG,ldNumerator);
        if(!ldIsNaN(ldNumerator)&&!ldIsInf(ldNumerator)){ // TODO checking with ldIsInf probably NOT needed although the big integer might be too big!!!
            // if we do NOT have a denominator (i.e. the denominator equals one we only need to add the delta (if any))
            if(!_rational->den){if(_rational->delta)ldNumerator+=_rational->delta->ld;return ldNumerator;}
            // ASSERT a denominator is present
            long double ldDenominator=mp_get_long_double(_rational->den);
            if(_rational->delta)ldNumerator+=(ldDenominator*_rational->delta->ld); // add the denominator multiplied by the delta to the numerator
            // a denominator which is not equal to 1
            if(amVerbose())output("Rational denominator converted to real '%.*Lf'.\n",LDBL_DIG,ldDenominator);
            if(!ldIsNaN(ldDenominator)&&!ldIsInf(ldDenominator))return ldNumerator/ldDenominator; // NOTE the denominator won't equal 0 so this should be Ok
            if(amVerbose())outputError("Failed to convert a rational denominator to a real");
        }
        if(amVerbose())outputError("Failed to convert a rational numerator to a real");
    }
    return M_LD_NAN; // if something went wrong
}/* VALIDATED */
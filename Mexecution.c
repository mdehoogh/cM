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

// are we keeping a map of mpd contexts????
// trap handler (how to set it????)
void MMdecimalraphandler(mpd_context_t* mpd_context){
}
size_t mpd_context_count=0;

const size_t MAXIMUM_NUMBER_OF_CONTEXTS=2; // quick fix to ascertain to use the same context over and over again

mpd_context_t** mpd_contexts=NULL; // keep track of all decimal contexts
/**
 * \brief returns the multiple-precision decimal context with the \p decimalprecision requested
 * \param decimalprecision the number of decimal digits a decimal should (minimally) hold
 * \return NULL on failing to obtain such a decimal context, otherwise that decimal context
 * if the number of stored decimal context equals the maximum number of contexts, the last context will be reused
 * if creating a new context succeeded it will be returned even when failing to remember the decimal context
 */
mpd_context_t* get_mpd_context(mpd_ssize_t decimalprecision){
    mpd_context_t* mpd_context=NULL; // the context we will be returning
    if(amVerbose())output("Retrieving the decimal context with precision %lld.\n",decimalprecision);
    // locate the context with the requested decimal precision
    int mpd_context_index=mpd_context_count;
    while(--mpd_context_index>=0)if(mpd_getprec(mpd_contexts[mpd_context_index])==decimalprecision){mpd_context=mpd_contexts[mpd_context_index];break;}
    // if we haven't found a match, try to get one
    if(!mpd_context){ // wasn't found
        // TODO perhaps re-using is not such a good idea...
        if(mpd_context_count>=MAXIMUM_NUMBER_OF_CONTEXTS){ // re-use the last one
            mpd_context=mpd_contexts[mpd_context_count-1];
            output("Changing the decimal precision of the last remembered decimal context to %llu.\n",decimalprecision);
            mpd_qsetprec(mpd_contexts[mpd_context_count-1],decimalprecision);
        }else{
            if(amVerbose())output("About to create the decimal context with precision %lld.\n",decimalprecision);
            mpd_context=(mpd_context_t*)malloc(sizeof(mpd_context_t));
            if(mpd_context){
                if(amVerbose())output("New decimal context with precision %lld created.\n",decimalprecision);
                // initialize the new context to the default context (specification)
                mpd_init(mpd_context,decimalprecision);
                if(amVerbose())output("Decimal context with precision %u initialized.\n",mpd_getprec(mpd_context));
                // try to append this context
                mpd_context_t** new_mpd_contexts=(mpd_context_count>0?realloc(mpd_contexts,(mpd_context_count+1)*sizeof(mpd_context_t*)):malloc(sizeof(mpd_context_t))); // realloc will work anyway
                if(new_mpd_contexts){
                    mpd_contexts=new_mpd_contexts;
                    mpd_contexts[mpd_context_count++]=mpd_context;
                    if(amVerbose())output("Decimal context with precision %u remembered.\n",mpd_getprec(mpd_context));
                }else
                    output("%sFailed to return a decimal context with precision %u.\n",ERROR_PREFIX,decimalprecision);
            }else
                outputError("Failed to create a new decimal context");
        }
        ////Mdecimalraphandler=MMdecimalraphandler;
    }
    // reset the status, so we can use the context as if it were new
    if(mpd_context)mpd_qsetstatus(mpd_context,0); // using the setter is preferred over ->status=0 assignment
    return mpd_context;
}/* VALIDATED */

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

// DECIMAL STUFF
extern mpd_context_t* _decimalContext; // M.c takes care of creating the application-wide decimal context
// MDH@17JUN2019: convenient to expose _get_mpd (e.g. for use in approximating pi with a decimal)
/**
 * \brief returns an mpd_t instance (from the mpdecimal libray) with the precision given by \p mpd_context equal to \p value
 * \param value the (initial) value of the returned mpd_t instance
 * \return on success the mpd_t instance equal to \p value, NULL otherwise
 */
mpd_t* __mpd(mpd_context_t* mpd_context,int64_t value){
    // using mpd_qnew over mpd_new because we want to return NULL on failure!!!
    mpd_t* mpd=mpd_qnew(); // replacing: mpd_context mpd_context?mpd_context:_decimalContext);
    // NOTE it is essential to initialize the stored value even when 0 as we would otherwise get errors on mpd_to_sci calls
    if(mpd){
        mpd_set_i64(mpd,value,(mpd_context?mpd_context:_decimalContext));
        if(mpd->len==0){
            output("%sFailed to create decimal with value " PRId64 ".\n",ERROR_PREFIX,value);
            free_mpd(mpd);mpd=NULL;
        }
    }else
        outputError("Failed to create a decimal");
    /////////outputDecimal("Decimal '",(Mdecimal*)_mpd,"' created!");
    return mpd;
}/* VALIDATED */
/**
 * \brief frees \p mpd, calling mpd_del()
 * \param mpd the mpdecimal instance to free
 */
void free_mpd(mpd_t* mpd){if(mpd)mpd_del(mpd);}/* VALIDATED */

/**
 * \brief frees \p decimal, delegating to free_mpd() for freeing the contained mpdecimal instance
 */
void free_decimal(Mdecimal* decimal){
    if(decimal){
        if(amVerbose())output("Freeing decimal.\n");
        free_mpd(decimal->mpd);
        FREE(decimal,'D');
    }else
    if(amVerbose())output("No decimal to free.\n");
}/* VALIDATED */

/**
 * \brief returns an uninitialized but cleared decimal (i.e. without an initialized mpd pointer)
 */
Mdecimal* __adecimal(){return (Mdecimal*)CALLOC(1,sizeof(Mdecimal),'D');} /* VALIDATED */

/**
 * \brief returns a decimal initialized to \p value with the precision specified by \p mpd_context and number of repeating digits equal to \p repeating
 * \param mpd_context the decimal context to use
 * \param value the initial (integer) value of the decimal
 * \param repeating the number of repeating decimal digits at the end
 */
Mdecimal* __decimal(mpd_context_t* mpd_context,int64_t value,uint64_t repeating){
    Mdecimal* decimal=(Mdecimal*)MALLOC(sizeof(Mdecimal),'D');
    if(decimal){
        decimal->mpd=__mpd(mpd_context,value); // initialize to zero by default
        if(!decimal->mpd){outputError("Failed to create a decimal");FREE(decimal,'D');decimal=NULL;}else decimal->repeating=repeating;
    }
    return decimal;
}/* VALIDATED */
/**
 * \brief returns a decimal with mpdecimal instance with the default decimal context equal to \p mpd and number of repeating digits equal to \p repeating, freeing the _mpd on failure
 */
Mdecimal* _getDecimal(mpd_t* mpd,uint64_t repeating,bool freeonfailure){
    if(!mpd)return NULL; // can do this as won't have to free mpd anyway
    Mdecimal* decimal=__decimal(NULL,0,repeating); // always using the default decimal context
    if(decimal)decimal->mpd=mpd;else if(freeonfailure)free_mpd(mpd);
    return decimal;
}/* VALIDATED */
/**
 * \brief returns a decimal parsed from \p decimalText using the default decimal context and repeating number of digits \p repeating
 */
Mdecimal* _getTextDecimal(const char* const decimalText,uint64_t repeating){
    if(!decimalText)return NULL;
    Mdecimal* decimal=NULL;
    if(strlen(decimalText)){
        if(amVerbose())output("Parsing decimal text '%s'.\n",decimalText);
        // can we find a repeating fraction????? this would be the case if behind the period we'd have xxxx<yyy><yyy><yyy>
        // the rounding at the end of course could prove to be problematic
        decimal=__decimal(NULL,0,repeating);
        if(decimal){
            mpd_set_string(decimal->mpd,decimalText,_decimalContext); // NOTE here we have to pass in the default decimal context
            if(!decimal->mpd){free_decimal(decimal);decimal=NULL;} // if we failed to get a mpdecimal instance from the text, the text is probably wrong!!!
        }
        if(!decimal)output("%sFailed to create a decimal from '%s'.\n",ERROR_PREFIX,decimalText);
    }else
        outputError("No decimal text to parse");
    return decimal;
}/* VALIDATED */

// MDH@24JUN2019: used an Mlist before to store the remainders, but because Mlist uses Mvalue instances, which we do not have access to here anymore (we have to create our own list for storing the remainders)
typedef struct MbigintegerListelement{
    Mbiginteger* _biginteger;
    struct MbigintegerListelement* _next;
}MbigintegerListelement;
void free_bigintegerListelement(MbigintegerListelement* _bile){
    // ASSERT assume _bile to not be NULL
    if(_bile->_next)free_bigintegerListelement(_bile->_next);
    if(_bile->_biginteger)free_biginteger(_bile->_biginteger);
}/* VALIDATED */

Mdecimal* _getRationalDecimal(const Mrational* const _rational){
    if(!_rational){outputError("No rational to convert to a decimal");return NULL;}
    // _decimalText is a local variable that when set should be freed before returning!!!
    Mstring* _decimalText=NULL;
    Mbiginteger *numerator=_rational->num,*denominator=_rational->den; // shortcut to the rational numerator and denominator
    uint64_t repeating=0;
    if(denominator){
        // local variables to be freed at the end (so NOT before)
        Mbiginteger *_digit=__biginteger(),*_remainder=__biginteger(),*_bi10=_getBiginteger(10);
        if(!_bi10){outputError("Failed to create big integer 10");return NULL;}
        if(_digit&&_remainder&&_bi10&&mp_div(numerator,denominator,_digit,_remainder)==MP_OKAY){
            // the integer part is _dividend
            _decimalText=_getBigintegerText(_digit);
            if(_decimalText&&!isBigintegerZero(_remainder)){ // we've got a fraction to add!!!
                Mstring* _p=string_append_char(_decimalText,'.'); // append the decimal period, storing the result in _p so we will know when that failed...
                // in order to find the repeating fraction we have to continue computing the remainders
                // and we have to register the remainders and compare the one we find with all remembered remainders, so far
                MbigintegerListelement *_firstRemainderListelement=NULL,*lastRemainderListelement=NULL;
                ////////////////////////if(!_lastRemainderListelement){outputError("Failed to create the list to store the remainders");return NULL;}
                /* replacing, using an Mlist):
                Mlist* _remainderList=_getListOfType(VT_BIGINTEGER);
                Mlistelement* _remainderListelement=NULL;
                */
                uint64_t remainderIndex,remainderCount=0; // where we found a match
                long long decimalsLeft=_decimalContext->prec+2; // stop as soon as we have sufficient decimals
                if(amVerbose())output("Number of decimals to determine: %llu.\n",decimalsLeft);
                Mstring* _digitText; // for storing the dividend digit character
                MbigintegerListelement* _remainderListelement=NULL;
                bool failure=false;
                while(--decimalsLeft>=0&&_p){
                    // ASSERT the current remainder is nonzero, therefore we have to store it (if different from any we have so far)
                    // compare first, if not present store and continue
                    // if the new remainder is not-zero and present in the list, we will know how many repeating fractions we have
                    _remainderListelement=_firstRemainderListelement; //_remainderList->_first;
                    remainderIndex=0;
                    while(_remainderListelement){
                        ///////////////if(amVerbose()){outputBiginteger("Comparing '",_remainderListelement->_value->value._biginteger,"'");outputBiginteger(" with remainder '",_remainder,"'.\n");}
                        if(mp_cmp(_remainderListelement->_biginteger,_remainder)==MP_EQ)break;
                        // replacing: if(mp_cmp(_remainderListelement->_value->value._biginteger,_remainder)==MP_EQ)break;
                        remainderIndex++;
                        _remainderListelement=_remainderListelement->_next;
                    }
                    if(_remainderListelement){ // we know the repeating part, so no need to add the remainder anymore!!!
                        repeating=(remainderCount-remainderIndex); // replacing: _remainderList->numberOfElements-remainderIndex);
                        if(amVerbose())output("Number of repeating decimals: %llu.\n",repeating);
                        break;
                    }

                    // remainder hasn't appeared before so store it in the list of remainders
                    _remainderListelement=(MbigintegerListelement*)calloc(1,sizeof(MbigintegerListelement)); // NOTE re-use of _remainderListelement
                    if(!_remainderListelement){outputError("Failed to create a big integer list element for storing the new remainder");_p=NULL;break;}

                    // at this point we have a new remainder list element (in _remainderListelement) that should be bound or freed
                    _remainderListelement->_biginteger=_getBigintegerCopy(_remainder); // NOTE we have to copy _remainder as we will be computing with _remainder further (see below)
                    if(!_remainderListelement->_biginteger){
                        outputError("Failed to store the remainder");
                        free(_remainderListelement); // we have to free _remainderListelement here because it's not going to be remembered (and freed later on) in the list of remainders
                        _p=NULL;break;
                    }
                    // 'store' _remainderListelement in the list (this means that we can be certain that _remainderListelement will be freed after the loop ends)
                    if(!lastRemainderListelement)_firstRemainderListelement=_remainderListelement;else lastRemainderListelement->_next=_remainderListelement;
                    lastRemainderListelement=_remainderListelement; // replace lastRemainderListelement with the new big integer list element
                    remainderCount++;
                    /* replacing:
                    Mvalue* remainderValue=_getBigintegerValue(_getBigintegerCopy(_remainder),true);
                    if(!remainderValue){outputError("Failed to store the remainder");break;}
                    if(appendedToList(_remainderList,remainderValue,0)<=0){free_value(remainderValue);outputError("Failed to remember the remainder in order to recognized the repeating fraction");break;}
                    */
                    if(mp_mul(_remainder,_bi10,_remainder)!=MP_OKAY){outputError("Failed to multiply the remainder by 10");_p=NULL;break;}
                    if(amVerbose()){outputBiginteger("Dividing '",_remainder,"'");outputBiginteger(" by '",denominator,"'.\n");}
                    // _digit and _remainder are getting re-used here as well, which does not pose a problem (so we've created them once)
                    if(mp_div(_remainder,denominator,_digit,_remainder)!=MP_OKAY){outputError("Failed to perform a long division to obtain the next decimal digit");_p=NULL;break;}
                    if(amVerbose()){outputBiginteger("Digit: '",_digit,"'");outputBiginteger(" and remainder '",_remainder,"'.\n");}

                    // append the dividend to the decimal text
                    // NOTE that _digitText is freed as soon as possible
                    _digitText=_getBigintegerText(_digit);
                    if(!_digitText){outputError("Failed to store the next decimal character");_p=NULL;break;}
                    _p=string_append(_p,string(_digitText));
                    free_string(_digitText);

                    if(amVerbose())if(_p)output("Decimal text so far: '%s'.\n",string(_p));

                    // if the remainder is zero (NOW stored in _remainderListelement->_biginteger instead of _remainder), we're done (it's a finite decimal fraction)
                    if(isBigintegerZero(_remainderListelement->_biginteger)){if(amVerbose())outputLine("Remainder is zero, so the decimal is finished.");break;}
                }
                if(_firstRemainderListelement)free_bigintegerListelement(_firstRemainderListelement); // replacing: free_list(_remainderList);
                if(!_p){free_string(_decimalText);_decimalText=NULL;} // some failure occurred
            }
        }
        // free all locally used pointers to dynamic memory
        free_biginteger(_digit);free_biginteger(_remainder);free_biginteger(_bi10);
    }else
        _decimalText=_getBigintegerText(numerator);
    // parse _decimalText to a decimal
    Mdecimal* _decimal=NULL;
    if(_decimalText){
        _decimal=_getTextDecimal(string(_decimalText),repeating);
        if(!_decimal)outputError("Failed to parse the decimal text of the corresponding rational");else if(amVerbose())outputDecimal("Decimal of rational: '",_decimal,"'.\n");
        free_string(_decimalText);
    }
    return _decimal;
}/* VALIDATED */

static mpd_t* decimalOne=NULL;
// getDecimalOne() return a decimal but this is a decimal that should never be freed
const mpd_t* getDecimalOne(){if(!decimalOne)decimalOne=__mpd(NULL,1);return decimalOne;}/* VALIDATED */
bool isDecimalOne(Mdecimal* _decimal){
    // MDH@17JUN2019: something that is repeating is definitely not equal to 1 (TODO unless it's 0.[9])
    return (_decimal->repeating&&mpd_cmp(_decimal->mpd,getDecimalOne(),_decimalContext)==MP_EQ);
}/* VALIDATED */

/**
 * \brief returns a copy of \p _decimal
 * \param _decimal the decimal to copy
 * \return a copy of \p _decimal on success, or NULL otherwise
 */
Mdecimal* _getDecimalCopy(Mdecimal* _decimal){
    if(!_decimal)return NULL;
    Mdecimal* _decimalCopy=__decimal(_decimalContext,0,_decimal->repeating);
    if(_decimalCopy){
        _decimalCopy->mpd=NULL; // TODO check if __decimal uses calloc or malloc
        mpd_copy(_decimalCopy->mpd,_decimal->mpd,_decimalContext); // copy attempt
        if(!_decimalCopy->mpd){free_decimal(_decimalCopy);_decimalCopy=NULL;} // on failure, release the decimal
    }else
        outputError("Failed to copy a decimal");
    return _decimalCopy;
}/* VALIDATED */

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
// MDH@17JUN2019: convert a decimal (back) to a rational
Mrational* _getDecimalRational(Mdecimal* decimal){
    Mrational* rational=NULL;
    if(decimal){
        // get the decimal text in fixed point format if it is not repeating, otherwise we always get in in fixed point but then without the closing ]
        Mstring* _decimalText=_getDecimalText(decimal,decimal->repeating>0); // replacing: mpd_to_sci(_decimal->mpd,0);
        if(_decimalText){
            char* decimalText=string(_decimalText); // a pointer to the chars array in _decimalString, so you can't free _decimalText until being finished with decimalText
            if(amVerbose())output("Decimal text to parse to rational: '%s'.\n",decimalText);
            if(decimal->repeating){
                // we have _decimal->repeating characters at the end of _decimalText that are repeated
                // having a decimal part (behind decimal period) is obligatory
                char* periodText=strchr(decimalText,'.');
                if(periodText){
                    ////////output("Period text: '%s'.\n",periodText);
                    bool neg=false;if(decimalText[0]=='-'){decimalText++;neg=true;} // 'cut off' and remember the sign
                    // 1. determine the start of the repeating digits (which is marked by [)
                    char* repeatingText=strchr(decimalText,'['); // replacing: _decimalText+(strlen(_decimalText)-_decimal->repeating); // using pointer arithmetic
                    *repeatingText='\0'; // we don't need the [ for parsing
                    repeatingText++;
                    ////////output("Repeating text: '%s'.\n",repeatingText);
                    // 2. extract the big integer representing the repeating digits (which will be the third part of the numerator)
                    // locally used dynamic variables
                    Mbiginteger *_num3=__biginteger(),*_bi10=_getBiginteger(10),*_den2=_getBiginteger(10),*_den1=_getBiginteger(1);
                    ////////output("Temporary dynamic variables created.\n");
                    if(_num3&&_bi10&&_den2&&_den1&&mp_read_radix(_num3,repeatingText,10)==MP_OKAY){
                        if(amVerbose())outputBiginteger("Repeating digits numerator part: '",_num3,"'.\n");
                        for(int i=decimal->repeating;i>1;i--)if(mp_mul(_den2,_bi10,_den2)!=MP_OKAY){outputError("Failed to multiply the second rational denominator part by 10");free_biginteger(_den2);_den2=NULL;break;}
                        if(_den2&&mp_decr(_den2)==MP_OKAY){ // _den2 computed (as 9999....9)
                            // let's determine the denominator
                            mp_int* _den=NULL;
                            int numberOfNonRepeatingDecimalDigits=(int)(repeatingText-periodText-2); // compute the number of non repeating decimals
                            ///////////////////mp_int* _den1=_getBiginteger(1);
                            if(numberOfNonRepeatingDecimalDigits>0){
                                while(_den1&&(--numberOfNonRepeatingDecimalDigits>=0))if(mp_mul(_den1,_bi10,_den1)!=MP_OKAY){outputError("Failed to multiply the first rational denominator part by 10");free_biginteger(_den1);_den1=NULL;}
                                if(_den1){
                                    _den=__biginteger();
                                    if(mp_mul(_den1,_den2,_den)!=MP_OKAY){free_biginteger(_den);_den=NULL;}else if(amVerbose())outputBiginteger("First denominator multiplier: '",_den1,"'.\n");
                                }
                            }else
                                _den=_getBigintegerCopy(_den2);
                            if(_den){ // denominator computed successfully, either to be bound or freed in this block
                                *periodText='\0'; // no harm overwriting the period with end-of-text character so _decimalText will contain the before period integer part
                                periodText++; // point periodText to the first digit behind the decimal period
                                if(amVerbose())output("Behind period text: '%s'.\n",periodText);
                                
                                // the numerator is the sum of what's in front of the repeating digits plus the integer representing the repeating digits (_num2)
                                Mbiginteger* _num=_getBigintegerCopy(_num3); // initialize _num to the repeating digits integer
                                // add the fixed part of the decimal digits (treated as integer)
                                if(_num&&strlen(periodText)){ // something between the period and the repeating digits
                                    Mbiginteger* _num2=__biginteger(); // _num2 is freed below, so that's good
                                    if(mp_read_radix(_num2,periodText,10)!=MP_OKAY||mp_mul(_num2,_den2,_num2)!=MP_OKAY||mp_add(_num,_num2,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}
                                    else 
                                    if(amVerbose())outputBiginteger("Non-repeating digits numerator part: '",_num2,"'.\n");
                                    free_biginteger(_num2);
                                }
                                if(_num){ // so far so good
                                    // _num to be bound or freed in this block!!!
                                    // add the part in front of the period multiplied by _den1 but it could be zero of course
                                    Mbiginteger* _num1=__biginteger(); // _mul1 freed below (which is reachable)
                                    if(mp_read_radix(_num1,decimalText,10)==MP_OKAY){
                                        // of course the integer part could well be zero!!!!
                                        if(!isBigintegerZero(_num1)){
                                            if(mp_mul(_num1,_den,_num1)!=MP_OKAY||mp_add(_num,_num1,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}else if(amVerbose())outputBiginteger("Integer numerator part: '",_num1,"'.\n");
                                        }
                                    }else{free_biginteger(_num);_num=NULL;}
                                    free_biginteger(_num1);    
                                }
                                // negate the numerator if the decimal is negative
                                if(_num&&neg&&mp_neg(_num,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}
                                if(_num)rational=_getRational(_num,_den,M_LD_NAN,true,false);
                                // if rational is NULL, _den and _num are not bound, otherwise they are, can't harm to try to release if not set though
                                if(!rational){free_biginteger(_den);free_biginteger(_num);}
                            }
                        }else
                            outputError("Failed to compute the second denominator multiplier");
                    }else
                        outputError("Failed to construct the integer containing the repeating digits");
                    free_biginteger(_num3);
                    free_biginteger(_bi10);
                    free_biginteger(_den2);
                    free_biginteger(_den1);
                }else{
                    if(amVerbose())outputDecimal("No fractional digits in decimal '",decimal,"'.\n");
                    Mbiginteger* _num=__biginteger();
                    if(_num){
                        if(mp_read_radix(_num,decimalText,10)==MP_OKAY)rational=_getRational(_num,NULL,M_LD_NAN,false,false);
                        if(!rational)free_biginteger(_num); // if no rational _num is unbound and must be freed
                    }
                }
            }else // we can go through the text????
                rational=_getDecimalTextRational(decimalText);
            free_string(_decimalText); // OOPS use free_string() not free()!
        }else
            outputError("Failed to convert the decimal to text");
    }
    return rational;
}/* VALIDATED */

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
Mrational* _getInverseRational(const Mrational* const _rational){
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
Mstring* _getStringText(Mtext* _text,bool dequoted){
	Mstring* _stringText=(_text?__string():NULL);
    if(_stringText){
        Mstring* _p=_stringText;
        if(amDebugging())_p=string_append_char(_p,'s');
        if(_p){
            if(!dequoted)_p=string_append_char(_p,_text->presuffix);
            _p=string_append(_p,_text->_c);
            if(!dequoted)_p=string_append_char(_p,_text->presuffix);
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
// if decimal->repeating fixedpoint will determine whether or not to append ] so pass in false in that case!!!!!
Mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint){
    Mstring* _decimalText=NULL;
    if(_decimal){
        _decimalText=__string();
        if(_decimalText){
            Mstring* _p=_decimalText;
            // NOTE not using mpd_to_sci as we do not know when we get an e-part!!!!
            // NOTE if _decimal->repeating always use fixed-point notation
            char* _decimalChars=mpd_format(_decimal->mpd,(fixedpoint||_decimal->repeating?"f":"g"),_decimalContext);
            ///////////output("Decimal rep: '%s'.\n",_decimalRep);
            if(_decimalChars){
                _p=string_append(_p,_decimalChars);
                free(_decimalChars); // NOTE assuming mpd_format dynamically created _decimalChars transferring ownership to me
                if(_p)if(_decimal->repeating){
                    _p=string_insert_char(_p,string_length(_p)-_decimal->repeating,'[');
                    if(_p&&!fixedpoint)_p=string_append_char(_p,']');
                }
            }else
                _p=NULL;
            if(!_p){free_string(_decimalText);_decimalText=NULL;}
        }
    }
    return _decimalText;
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
            for(int i=1;i<=MAX(1,maxiter);i++){
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
long double getRationalLongDouble(const Mrational* const _rational){
    if(_rational){
        // as you can see up we're storing the delta in our rationals as well, so if we want to get ld back out of it the formula is: (numerator+delta)/denominator
        // but with the numerator and denominator possibly big integers adding delta to the numerator means adding a double to a big integer (of course delta typically is very small)
        // it's easiest to turn the numerator big integer into a long double and add delta to it, and divide by the long double stored in the denominator
        // TODO find a better way to do this
        long double ldNumerator=mp_get_long_double(_rational->num); // NOTE also shortcuts when _rational->num equals 0 but we have to add the delta, so we have to do it this way
        if(amVerbose())output("Rational numerator converted to real '%.*Lf'.",LDBL_DIG,ldNumerator);
        if(!ldIsNaN(ldNumerator)&&!ldIsInf(ldNumerator)){ // TODO checking with ldIsInf probably NOT needed although the big integer might be too big!!!
            // add delta (which could be zero though) NOTE ld should not be NaN or Infinity though
            if(_rational->delta)ldNumerator+=_rational->delta->ld;
            if(!_rational->den)return ldNumerator; // if no denominator (i.e. 1) nothing to divide by!!
            // a denominator which is not equal to 1
            long double ldDenominator=mp_get_long_double(_rational->den);
            if(amVerbose())output("Rational denominator converted to real '%.*Lf'.",LDBL_DIG,ldDenominator);
            if(!ldIsNaN(ldDenominator)&&!ldIsInf(ldDenominator))return ldNumerator/ldDenominator; // NOTE the denominator won't equal 0 so this should be Ok
            if(amVerbose())outputError("Failed to convert a rational denominator to a real");
        }
        if(amVerbose())outputError("Failed to convert a rational numerator to a real");
    }
    return M_LD_NAN; // if something went wrong
}/* VALIDATED */
/**
 * MDH@24JUN2019:
 * - memory safe checkup on each function based on the following guidelines:
 *   1. all local variables to point to data in dynamic storage (heap) should be declared at the start (or just after input check)
 *   2. all these local variables should be freed before leaving the function (so technically there should be one exit point)
 *   3. if the pointer contents is passed along (in)to the result of the function the pointer should be NULLed to prevent releasing the memory pointed to (which needs to persist function execution)
 *   4. preferably this is done by calling the transfer<Mtype> function that will NULL the calling pointer
 *   5. prefix the function name with _ if it returns a dynamically allocated pointer whose ownerships transfers to the caller
 */
#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "mstring.h"
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
// NOTE force execution immediately
__attribute__((constructor)) void initExecution() {
    int i=1;
	char* c=(char*)&i;
	littleEndian=*c;
	if(amVerbose())output(littleEndian?"\nLittle endian.":"\nBig endian.");
}

bool isLittleEndian(){
    if(littleEndian<0)initExecution();
    return(littleEndian>0);
}


mstring* _getUint64BinaryText(uint64_t ul,char presuffix){
    mstring* _binaryText=string_create();
    if(_binaryText){
        mstring* _p=_binaryText;
        int l=64;
        while(--l>=0&&_p){_p=string_append_char(_p,ul&1?'1':'0');ul>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
        if(presuffix)_p=string_append_char(_p,presuffix);
        if(!_p){free_mstring(_binaryText);_binaryText=NULL;}else string_reverse(_p);
    }
    return _binaryText;
}
mstring* _getUint16BinaryText(uint16_t us,char presuffix){
    mstring* _binaryText=string_create();
    if(_binaryText){
        mstring* _p=_binaryText;
        int l=16;
        while(--l>=0&&_p){_p=string_append_char(_p,us&1?'1':'0');us>>=1;if(l)if((l%8)==0)_p=string_append_char(_p,' ');}
        if(presuffix)_p=string_append_char(_p,presuffix);
        if(!_p){free_mstring(_binaryText);_binaryText=NULL;}else string_reverse(_p);
    }
    return _binaryText;
}

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
mpd_context_t* get_mpd_context(mpd_ssize_t decimalprecision){
    if(amVerbose())output("Retrieving the decimal context with precision %lld.\n",decimalprecision);
    int mpd_context_index=mpd_context_count;
    while(--mpd_context_index>=0)if(mpd_getprec(mpd_contexts[mpd_context_index])==decimalprecision)break;
    if(mpd_context_index<0){
        if(mpd_context_count<MAXIMUM_NUMBER_OF_CONTEXTS){
            if(amVerbose())output("About to create the decimal context with precision %lld.\n",decimalprecision);
            mpd_context_t** new_mpd_contexts=(mpd_context_count>0?realloc(mpd_contexts,(mpd_context_count+1)*sizeof(mpd_context_t*)):(mpd_context_t**)malloc(sizeof(mpd_context_t*)));
            if(!new_mpd_contexts){
                output("%sFailed to return a decimal context with precision %u.\n",ERROR_PREFIX,decimalprecision);
                return NULL;
            }
            mpd_contexts=new_mpd_contexts;
            mpd_context_index=mpd_context_count;
            mpd_context_count++;
            mpd_contexts[mpd_context_index]=(mpd_context_t*)malloc(sizeof(mpd_context_t)); // TODO do we need to do this???
            if(amVerbose())output("New decimal context with precision %lld created.\n",decimalprecision);
            // initialize the new context to the default context
            mpd_init(mpd_contexts[mpd_context_index],decimalprecision);
            if(amVerbose())output("Decimal context with precision %u initialized.\n",mpd_getprec(mpd_contexts[mpd_context_index]));
        }else{ // re-use the last context
            mpd_context_index=mpd_context_count-1;
            output("Changing the decimal precision to %llu.\n",decimalprecision);
            mpd_qsetprec(mpd_contexts[mpd_context_index],decimalprecision);
        }
        ////Mdecimalraphandler=MMdecimalraphandler;
    }
    // reset the status
    mpd_qsetstatus(mpd_contexts[mpd_context_index],0); // using the setter is preferred over ->status=0 assignment
    return mpd_contexts[mpd_context_index];
}

// BIG INTEGER STUFF
// new_biginteger returns an initialized big integer on success, or NULL when failing
Mbiginteger* new_biginteger(){
    Mbiginteger* result=(Mbiginteger*)malloc(sizeof(mp_int));
    if(result&&mp_init(result)!=MP_OKAY){mp_clear((mp_int*)result);result=NULL;} // ESSENTIAL to release the big integer, when failing to initialize it!!
    return result;
}
mp_int* new_mp_int(){return (mp_int*)new_biginteger();}
Mbiginteger* _getBiginteger(int64_t l){
    Mbiginteger* _biginteger=new_biginteger();
    if(l)mp_set_i64((mp_int*)_biginteger,l); // TODO mp_set_i64 can fail can't it? then why is it of type void???
    return _biginteger;
}
Mbiginteger* _getBigintegerNeg(Mbiginteger* _biginteger){
    Mbiginteger* _bigintegerNeg=new_biginteger();
    if(mp_neg(_biginteger,_bigintegerNeg)!=MP_OKAY){free_biginteger(_bigintegerNeg);_bigintegerNeg=NULL;}
    return _bigintegerNeg;
}
// pass in NULL to _getBigIntegerCopy to get a big integer (initialized to zero)
Mbiginteger* _getBigintegerCopy(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    Mbiginteger* _result=new_biginteger();
    if(mp_copy(_biginteger,_result)!=MP_OKAY){free_biginteger(_result);return NULL;}
    return _result;
}

// using constant big integers 0, 1 and 2 (do NOT wrap these constants in Mvalue's though or they will need to be created over and over again)
static Mbiginteger *bi0=NULL,*bi1=NULL,*bi2=NULL,*bi3=NULL;
const Mbiginteger* getBigintegerZero(){if(!bi0)bi0=_getBiginteger(0);return bi0;}
const Mbiginteger* getBigintegerOne(){if(!bi1)bi1=_getBiginteger(1);return bi1;}
const Mbiginteger* getBigintegerTwo(){if(!bi2)bi2=_getBiginteger(2);return bi2;}
const Mbiginteger* getBigintegerThree(){if(!bi3)bi3=_getBiginteger(3);return bi3;}

bool isBigintegerZero(Mbiginteger* _biginteger){return(_biginteger?mp_iszero((mp_int*)_biginteger)==MP_YES:false);}
bool isBigintegerOne(Mbiginteger* _biginteger){return(_biginteger?mp_cmp((mp_int*)_biginteger,getBigintegerOne())==MP_EQ:false);}

extern mpd_context_t* _decimalContext; // M.c takes care of creating the application-wide decimal context
// MDH@17JUN2019: convenient to expose _get_mpd (e.g. for use in approximating pi with a decimal)
mpd_t* new_mpd(mpd_context_t* mpd_context,int64_t value){
    // using mpd_qnew over mpd_new because we want to return NULL on failure!!!
    mpd_t* _mpd=mpd_qnew(); // replacing: mpd_context mpd_context?mpd_context:_decimalContext);
    // NOTE it is essential to initialize the stored value even when 0 as we would otherwise get errors on mpd_to_sci calls
    if(_mpd)
        mpd_set_i64(_mpd,value,(mpd_context?mpd_context:_decimalContext));
    else
        outputError("Failed to allocate a decimal");
    /////////outputDecimal("Decimal '",(Mdecimal*)_mpd,"' created!");
    return _mpd;
}
Mdecimal* new_decimal(mpd_context_t* mpd_context,int64_t value,uint64_t repeating){
    Mdecimal* _decimal=(Mdecimal*)MALLOC(sizeof(Mdecimal),'D');
    if(_decimal){
        _decimal->mpd=new_mpd(mpd_context,value); // initialize to zero by default
        if(!_decimal->mpd){outputError("Failed to create a decimal");FREE(_decimal,'D');_decimal=NULL;}else _decimal->repeating=repeating;
    }
    return _decimal;
}
Mdecimal* _getDecimal(mpd_t* _mpd,uint64_t repeating,bool freeonfailure){
    Mdecimal* _decimal=NULL;
    if(_mpd){
        _decimal=new_decimal(NULL,0,repeating); // always using the default decimal context
        if(_decimal)_decimal->mpd=_mpd;else if(freeonfailure)mpd_del(_mpd);
    }
    return _decimal;
}
Mdecimal* _getTextDecimal(const char* const decimalText,uint64_t repeating){
    Mdecimal* _decimal=NULL;
    if(decimalText&&strlen(decimalText)){
        if(amVerbose())output("Parsing decimal text '%s'.\n",decimalText);
        // can we find a repeating fraction????? this would be the case if behind the period we'd have xxxx<yyy><yyy><yyy>
        // the rounding at the end of course could prove to be problematic
        _decimal=new_decimal(_decimalContext,0,repeating);
        if(_decimal)mpd_set_string(_decimal->mpd,decimalText,_decimalContext);
    }else
        outputError("No decimal text to parse");
    if(!_decimal)outputError("Failed to create a decimal");
    return _decimal;
}

// MDH@24JUN2019: used an Mlist before to store the remainders, but because Mlist uses Mvalue instances, which we do not have access to here anymore (we have to create our own list for storing the remainders)
typedef struct MbigintegerListelement{
    Mbiginteger* _biginteger;
    struct MbigintegerListelement* _next;
}MbigintegerListelement;
void free_bigintegerListelement(MbigintegerListelement* _bile){
    // ASSERT assume _bile to not be NULL
    if(_bile->_next)free_bigintegerListelement(_bile->_next);
    if(_bile->_biginteger)free_biginteger(_bile->_biginteger);
}
Mdecimal* _getRationalDecimal(const Mrational* const _rational){
    if(!_rational){outputError("No rational to convert to a decimal");return NULL;}
    Mbiginteger *_numerator=_rational->num,*_denominator=_rational->den;
    mstring* _decimalText=NULL;
    uint64_t repeating=0;
    if(_denominator){
        Mbiginteger *_digit=new_biginteger(),*_remainder=new_biginteger();
        if(_digit&&_remainder&&mp_div(_numerator,_denominator,_digit,_remainder)==MP_OKAY){
            // the integer part is _dividend
            _decimalText=_getBigintegerText(_digit);
            free_biginteger(_digit);
            if(_decimalText&&!isBigintegerZero(_remainder)){ // we've got a fraction to add!!!
                string_append_char(_decimalText,'.'); // the decimal period
                Mbiginteger* _bi10=_getBiginteger(10);
                if(!_bi10){outputError("Failed to create big integer constant 10");return NULL;}
                // in order to find the repeating fraction we have to continue computing the remainders
                // and we have to register the remainders and compare the one we find with all remembered remainders, so far
                MbigintegerListelement *_firstRemainderListelement=NULL,*_lastRemainderListelement=NULL;
                if(!_lastRemainderListelement){outputError("Failed to create the list to store the remainders");return NULL;}
                /* replacing, using an Mlist):
                Mlist* _remainderList=_getListOfType(VT_BIGINTEGER);
                Mlistelement* _remainderListelement=NULL;
                */
                uint64_t remainderIndex,remainderCount=0; // where we found a match
                long long decimalsLeft=_decimalContext->prec+2; // stop as soon as we have sufficient decimals
                if(amVerbose())output("Number of decimals to determine: %llu.\n",decimalsLeft);
                mstring* _digitText; // for storing the dividend digit character
                MbigintegerListelement* _remainderListelement=NULL;
                bool failure=false;
                while(--decimalsLeft>=0){
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
                    if(!_remainderListelement){outputError("Failed to create a big integer list element for storing the new remainder");failure=true;break;}
                    _remainderListelement->_biginteger=_remainder;_remainder=NULL; // _remainder transferred, so NULL
                    if(!_lastRemainderListelement)_firstRemainderListelement=_remainderListelement;else _lastRemainderListelement->_next=_remainderListelement;
                    _lastRemainderListelement=_remainderListelement; // replace lastRemainderListelement with the new big integer list element
                    remainderCount++;
                    /* replacing:
                    Mvalue* remainderValue=_getBigintegerValue(_getBigintegerCopy(_remainder),true);
                    if(!remainderValue){outputError("Failed to store the remainder");break;}
                    if(appendedToList(_remainderList,remainderValue,0)<=0){free_value(remainderValue);outputError("Failed to remember the remainder in order to recognized the repeating fraction");break;}
                    */
                    if(mp_mul(_remainder,_bi10,_remainder)!=MP_OKAY){outputError("Failed to multiply the remainder by 10");failure=true;break;}
                    if(amVerbose()){outputBiginteger("Dividing '",_remainder,"'");outputBiginteger(" by '",_denominator,"'.\n");}
                    if(mp_div(_remainder,_denominator,_digit,_remainder)!=MP_OKAY){outputError("Failed to perform a long division to obtain the next decimal digit");failure=true;break;}
                    if(amVerbose()){outputBiginteger("Digit: '",_digit,"'");outputBiginteger(" and remainder '",_remainder,"'.\n");}
                    // append the dividend to the decimal text
                    _digitText=_getBigintegerText(_digit);
                    if(!_digitText){outputError("Failed to store the next decimal character");failure=true;break;}
                    string_append(_decimalText,string(_digitText));
                    if(amVerbose())output("Decimal text so far: '%s'.\n",string(_decimalText));
                    free_mstring(_digitText);
                    free_biginteger(_digit);
                    // if the remainder is zero (NOW stored in _remainderListelement->_biginteger instead of _remainder), we're done (it's a finite decimal fraction)
                    if(isBigintegerZero(_remainderListelement->_biginteger)){if(amVerbose())outputLine("Remainder is zero, so the decimal is finished.");break;}
                }
                free_biginteger(_bi10);
                if(_firstRemainderListelement)free_bigintegerListelement(_firstRemainderListelement); // replacing: free_list(_remainderList);
                if(failure){free_mstring(_decimalText);_decimalText=NULL;}
            }
        }
        free_biginteger(_digit);free_biginteger(_remainder);
    }else
        _decimalText=_getBigintegerText(_numerator);
    // parse _decimalText to a decimal
    Mdecimal* _decimal=NULL;
    if(_decimalText){
        _decimal=_getTextDecimal(string(_decimalText),repeating);
        if(!_decimal)outputError("Failed to parse the decimal text of the corresponding rational");
        free_mstring(_decimalText);
    }
    return _decimal;
}

static mpd_t* d1=NULL;
const mpd_t* get_mpdOne(){if(!d1)d1=new_mpd(NULL,1);return d1;}
bool isDecimalOne(Mdecimal* _decimal){
    // MDH@17JUN2019: something that is repeating is definitely not equal to 1 (TODO unless it's 0.[9])
    return (_decimal->repeating&&mpd_cmp(_decimal->mpd,get_mpdOne(),_decimalContext)==MP_EQ);
}

Mdecimal* _getDecimalCopy(Mdecimal* _decimal){
    if(!_decimal)return NULL;
    Mdecimal* _decimalCopy=new_decimal(_decimalContext,0,_decimal->repeating);
    if(_decimalCopy)mpd_copy(_decimalCopy->mpd,_decimal->mpd,_decimalContext);
    return _decimalCopy;
}

// MDH@07JUN2019: we are going to store real text representations (like 100.1) as rationals from now on with delta equal to 0 (so we know where they came from, and that the denominator is a power of 10)
//                because if we convert them to a long double we might loose precision in converting the decimal representation to the binary (internal) representation
// MDH@14JUN2019: now also possible that the text has an e-part (which will change the denominator!!!!)
Mrational* _getDecimalTextRational(char* rationalText,bool freeonfailure){
	// the text should represent an integer or a real
	if(!rationalText)return NULL;
    if(amVerbose())output("Converting '%s' to a rational.",rationalText);
	Mrational* _rational=NULL;
	int l=strlen(rationalText);
	if(l){
		bool neg=(*rationalText=='-');if(neg)rationalText++; // get the sign
		// get the e-part (if any)
		char* exponentText=strchr(rationalText,'e'); // assume lowercase e
		Mbiginteger* _exponent=new_biginteger();
		if(exponentText){
			*exponentText='\0'; // 'cuf off' the e-part!!!!
			l=(int)(exponentText-rationalText); // this will be the new l we need below!!!
			if(amVerbose())output("With exponent removed: '%s'.",rationalText);
			exponentText++; // point to the first character of the exponent
			if(mp_read_radix(_exponent,exponentText,10)!=MP_OKAY){
				output("%sFailed to extract the exponent its text representation '%s'.\n",ERROR_PREFIX,exponentText);
				free_biginteger(_exponent);
				_exponent=NULL;
			}else
			//if(amVerbose())
				outputBiginteger("\nExponent '",_exponent,"'.");
		}
		if(!exponentText||_exponent){ // either we do not have an exponentText or we have an exponent big integer (to apply later on)
			// TODO if we would just have an eval to get the value out of the token text
			char* decimalPartText=strchr(rationalText,'.');
			int decimalPartIndex=0;
			if(decimalPartText)*decimalPartText='\0'; // 'cut off' the decimal part (for now)
			if(amVerbose())output("With decimal part removed: '%s'.",rationalText);
			// now ready to check the integer part 
			Mbiginteger* _numerator=new_biginteger();
			Mbiginteger* _denominator=NULL;
			if(mp_read_radix(_numerator,rationalText,10)==MP_OKAY){ // apparently a valid (big) integer
				Mbiginteger* _decimalPartBiginteger=NULL;
				if(decimalPartText){
					int decimalPartIndex=(int)(decimalPartText-rationalText);
					decimalPartText++; // point to the first character of the decimal part
					_decimalPartBiginteger=new_biginteger();
					if(mp_read_radix(_decimalPartBiginteger,decimalPartText,10)==MP_OKAY){
                        if(!isBigintegerZero(_decimalPartBiginteger)){
						    // compute the power of ten denominator
						    _denominator=_getBiginteger(1);
						    Mbiginteger* _tenBiginteger=_getBiginteger(10);
						    while(++decimalPartIndex<l)if(mp_mul(_denominator,_tenBiginteger,_denominator)!=MP_OKAY){free_biginteger(_denominator);_denominator=NULL;break;}
						    free_biginteger(_tenBiginteger);
                        }else // the decimal part is zero therefore we do not officially have a decimal part (but we do want the associated rational even with _denominator NULL)
                            decimalPartText=NULL;
					}
					if(amVerbose())outputBiginteger("\nDecimal part integer: '",_decimalPartBiginteger,"'.");
					if(amVerbose())if(_denominator)outputBiginteger("\nDenominator: '",_denominator,"'.");
				}
				// if we have a decimalPartText we need a denominator
				if(!decimalPartText||_denominator){
					// if we have a _denominator and we fail to compute the appropriate numerator, we have to free all big integers
					// NOTE do NOT free the numerator and denominator in the call to _getRational, as we free them if _rational ends of being NULL afterwards
					if(!_denominator||(mp_mul(_numerator,_denominator,_numerator)==MP_OKAY&&mp_add(_numerator,_decimalPartBiginteger,_numerator)==MP_OKAY)){
						if(amVerbose())outputBiginteger("\nNumerator before applying the exponent: '",_numerator,"'.");
						if(amVerbose())if(_denominator)outputBiginteger("\nDenominator before applying the exponent: '",_denominator,"'.");
						// if we have an non-zero exponent, we have to adjust the numerator or denominator BEFORE trying to create the rational!!!
						if(exponentText&&mp_iszero(_exponent)==MP_NO){
							Mbiginteger* _tenBiginteger=_getBiginteger(10);
							if(_tenBiginteger){
								if(mp_isneg(_exponent)==MP_YES){ // a negative exponent goes into the denominator
									if(!_denominator)_denominator=_getBiginteger(1);
									if(_denominator){
										while(mp_iszero(_exponent)==MP_NO){
											if(mp_mul(_denominator,_tenBiginteger,_denominator)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
											if(mp_incr(_exponent)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
										}
									}else{free_biginteger(_exponent);_exponent=NULL;}
								}else{ // a positive exponent goes into the numerator
									while(mp_iszero(_exponent)==MP_NO){
										if(mp_mul(_numerator,_tenBiginteger,_numerator)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
										if(mp_decr(_exponent)!=MP_OKAY){free_biginteger(_exponent);_exponent=NULL;break;}
									}
								}
								free_biginteger(_tenBiginteger);
							}else{free_biginteger(_exponent);_exponent=NULL;}
						}
						// check again whether we still have an exponent (when we should)
						if(!exponentText||_exponent)
                            if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)
                                _rational=_getRational(_numerator,_denominator,0,true,false); // NOTE the 0 explicitly tells the rational that it represents a decimal representation!!!!!
					}
				}
                if(_decimalPartBiginteger)free_biginteger(_decimalPartBiginteger);
			}else
				output("%sInteger part of rational text '%s' invalid.\n",ERROR_PREFIX,rationalText);
			// if we haven't got a rational that binded _numerator and _denominator free both of them
			if(!_rational){
                free_biginteger(_numerator);
                if(_denominator)free_biginteger(_denominator);
             }
		}
	}
    if(!_rational)if(freeonfailure)free(rationalText);
	return _rational;
}
// MDH@17JUN2019: convert a decimal (back) to a rational
Mrational* _getDecimalRational(Mdecimal* _decimal){
    if(!_decimal)return NULL;
    Mrational* _rational=NULL;
    // get the decimal text in fixed point format if it is not repeating, otherwise we always get in in fixed point but then without the closing ]
    mstring* _decimalString=_getDecimalText(_decimal,_decimal->repeating>0); // replacing: mpd_to_sci(_decimal->mpd,0);
    if(_decimalString){
        char* decimalText=string(_decimalString); // a pointer to the chars array in _decimalString
        if(_decimal->repeating){
            // we have _decimal->repeating characters at the end of _decimalText that are repeated
            // having a decimal part (behind decimal period) is obligatory
            char* periodText=strchr(decimalText,'.');
            if(periodText){
                bool neg=false;if(decimalText[0]=='-'){decimalText++;neg=true;} // 'cut off' and remember the sign
                // 1. determine the start of the repeating digits (which is marked by [)
                char* repeatingText=strchr(decimalText,'['); // replacing: _decimalText+(strlen(_decimalText)-_decimal->repeating); // using pointer arithmetic
                *repeatingText='\0'; // we don't need the [ for parsing
                repeatingText++;
                // 2. extract the big integer representing the repeating digits (which will be the third part of the numerator)
                mp_int* _num3=new_mp_int();
                if(_num3&&mp_read_radix(_num3,repeatingText,10)==MP_OKAY){
                    if(amVerbose())outputBiginteger("\nRepeating digits numerator part: '",_num3,"'.");
                    // now ready to compute _den1 and _den2
                    mp_int* _bi10=_getBiginteger(10);
                    mp_int* _den2=_getBiginteger(10);
                    for(int i=_decimal->repeating;i>1;i--)if(mp_mul(_den2,_bi10,_den2)!=MP_OKAY){outputError("Failed to multiply the second rational denominator part by 10");free_biginteger(_den2);_den2=NULL;break;}
                    if(_den2&&mp_decr(_den2)==MP_OKAY){ // _den2 computed (as 9999....9)
                        // let's determine the denominator
                        mp_int* _den=NULL;
                        int numberOfNonRepeatingDecimalDigits=(int)(repeatingText-periodText-2); // compute the number of non repeating decimals
                        mp_int* _den1=_getBiginteger(1);
                        if(numberOfNonRepeatingDecimalDigits>0){
                            while(_den1&&(--numberOfNonRepeatingDecimalDigits>=0))if(mp_mul(_den1,_bi10,_den1)!=MP_OKAY){output("WERROR: Failed to multiply the first rational denominator part by 10.");free_biginteger(_den1);_den1=NULL;}
                            if(_den1){
                                _den=new_mp_int();
                                if(mp_mul(_den1,_den2,_den)!=MP_OKAY){free_biginteger(_den);_den=NULL;}else if(amVerbose())outputBiginteger("\nFirst denominator multiplier: '",_den1,"'.");
                                free_biginteger(_den1);
                            }
                        }else
                            _den=_getBigintegerCopy(_den2);
                        if(_den){ // denominator computed successfully
                            *periodText='\0'; // no harm overwriting the period with end-of-text character so _decimalText will contain the before period integer part
                            periodText++; // point periodText to the first digit behind the decimal period
                            if(amVerbose())output("Behind period text: '%s'.",periodText);
                            // the numerator is the sum of what's in front of the repeating digits plus the integer representing the repeating digits (_num2)
                            mp_int* _num=_getBigintegerCopy(_num3); // initialize _num to the repeating digits integer
                            // add the fixed part of the decimal digits (treated as integer)
                            if(strlen(periodText)){ // something between the period and the repeating digits
                                mp_int* _num2=new_mp_int();
                                if(mp_read_radix(_num2,periodText,10)!=MP_OKAY||mp_mul(_num2,_den2,_num2)!=MP_OKAY||mp_add(_num,_num2,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}else if(amVerbose())outputBiginteger("\nNon-repeating digits numerator part: '",_num2,"'.");
                                free_biginteger(_num2);
                            }
                            if(_num){ // so far so good
                                // add the part in front of the period multiplied by _den1 but it could be zero of course
                                mp_int* _num1=new_mp_int();
                                if(mp_read_radix(_num1,decimalText,10)==MP_OKAY){
                                    // of course the integer part could well be zero!!!!
                                    if(!isBigintegerZero(_num1)){
                                        if(mp_mul(_num1,_den,_num1)!=MP_OKAY||mp_add(_num,_num1,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}else if(amVerbose())outputBiginteger("\nInteger numerator part: '",_num1,"'.");
                                    }
                                }else{
                                    free_biginteger(_num);_num=NULL;
                                }
                                free_biginteger(_num1);    
                            }
                            // negate the numerator if the decimal is negative
                            if(_num&&neg&&mp_neg(_num,_num)!=MP_OKAY){free_biginteger(_num);_num=NULL;}
                            if(_num)
                                _rational=_getRational(_num,_den,M_LD_NAN,true,true);
                            else
                                free_biginteger(_den);
                        }
                        free_biginteger(_den1);
                    }else
                        outputError("Failed to compute the second denominator multiplier");
                    free_biginteger(_bi10);
                    free_biginteger(_den2);
                }else
                    outputError("Failed to construct the integer containing the repeating digits");
                free_biginteger(_num3);
            }else{
                if(amVerbose())outputDecimal("\nNo decimal digits in decimal '",_decimal,"'.");
                mp_int* _num=new_mp_int();
                if(mp_read_radix(_num,decimalText,10)==MP_OKAY)
                    _rational=_getRational(_num,NULL,M_LD_NAN,false,true);
                else
                    free_biginteger(_num);
            }
        }else // we can go through the text????
            _rational=_getDecimalTextRational(decimalText,false);
        free(_decimalString);
    }
    return _rational;
}

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
}

///////mstring* _getBigintegerText(const Mbiginteger* const _biginteger); // prototype declaration

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
    Mbiginteger* _numerator=new_biginteger();
    if(exp==0)return _getRational(_numerator,NULL,false);
    // set the numerator to the mantisse (which luckily is uint64_t)
    mp_set_u64(_numerator,mantisse);
    exp-=0x403E; // determine the 'true' exponent
    // if the true exponent is negative, the multiplier is below 1 and cannot be used as denominator, instead the numerator should be multiplied by 2 to the power -exp
    if(exp<0){
        Mbiginteger* _shiftedout=new_biginteger();
        mp_err err=mp_div_2d(_numerator,-exp,_numerator,_shiftedout);
        if(err!=MP_OKAY){
            free_biginteger(_shiftedout);
            outputError("Failed to adjust the numerator of the rational by the negative exponent of the real.");
            free_biginteger(_numerator);
            return NULL;
        }
        // we may assume that what got shifted out fits in an uint64_t
        uint64_t shiftedout=mp_get_u64(_shiftedout);mstring* _shiftedoutText=_getUint64BinaryText(shiftedout,'\0');output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));free_mstring(_shiftedoutText);
        // replacing: mstring* _shiftedoutText=_getBigintegerText(_shiftedout);output("Shifted (by %u positions) out: '%s'.",-exp,string(_shiftedoutText));free_mstring(_shiftedoutText);
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
void free_string(Mstring* _string){
    free(_string); // replacing (when we used a char pointer (_m) for storing the characters): if(_string){if(_string->_m)free_mstring(_string->_m);_string->_m=NULL;free(_string);}
}
void free_integer(Minteger* _integer){
    if(_integer){
        if(amVerbose())output("Freeing integer %llu.",_integer->ll);
        free(_integer);
    }else
        output("BUG: No integer to free!");
}
void free_biginteger(Mbiginteger* _biginteger){
    if(_biginteger){
        if(amVerbose())output("Freeing big integer."); // TODO can we display the value?
        mp_clear(_biginteger); // directly call mp_clear on the Mbiginteger pointer!!!
    }else
        output("BUG: No big integer to free!");
}
void free_decimal(Mdecimal* _decimal){
    if(_decimal){
        if(amVerbose())output("Freeing decimal.");
        mpd_del(_decimal->mpd); // assuming -> precedes the address of operator
        FREE(_decimal,'D');
    }else
        output("BUG: No decimal to free.");
}
void free_real(Mreal* _real){
    if(_real){
        if(amVerbose())output("Freeing real %.*Lf.",23,_real->ld);
        free(_real);
    }else
        output("BUG: No real to free!");
}
void free_rational(Mrational* _rational){
    if(_rational){
        if(_rational->num)free_biginteger(_rational->num);
        if(_rational->den)free_biginteger(_rational->den);
        if(_rational->delta)free_real(_rational->delta);
        free(_rational);
    }else
        output("No rational to free!");
}

// ALLOCATORS (private)
// value wrappers
// typically an Mvalue is immutable (we might change that for variables that are strong typed e.g. when created with integer(),real(),string(),list() or map() function)
Minteger* new_integer(long long ll){
    Minteger* _integer=malloc(sizeof(Minteger));
    if(_integer)_integer->ll=ll;
    return _integer;
}
/*
Mbiginteger* new_biginteger(z_t zt){
    Mbiginteger* _biginteger=malloc(sizeof(Mbiginteger));
    if(_biginteger){
        zset(_biginteger->bi,zt);
    }
    return _biginteger;
}
*/
Mreal* new_real(long double ld){
    Mreal* _real=malloc(sizeof(Mreal));
    if(_real)_real->ld=ld;
    return _real;
}
// Mstring is an immutable version of mstring* in that it cannot be changed
Mstring* new_string(char* _text){ // _text assumed to be string(mstring*), so we can simply copy it over with the starting quote character (" or ')
    return (Mstring*)_strdup(_text);
}
Mstring* new_charstring(char _char){ // _text assumed to be string(mstring*), so we can simply copy it over with the starting quote character (" or ')
    mstring* charstring=string_create();string_append_char(charstring,'"');string_append_char(charstring,_char);
    Mstring* char_string=new_string(string(charstring));
    free(charstring);
    return char_string;
}
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

// mstring* is assumed to start with the same prefix/suffix character
Mstring* get_string(mstring* s){
	Mstring* _string=(s?(Mstring*)malloc(sizeof(Mstring)):NULL);
	if(_string){
		_string->presuffix=string_char(s,0);
		_string->_m=string_create();
		if(_string->_m==NULL)return NULL;
		string_append(_string->_m,string_remainder(s,1)); // NOTE string_remainder might return NULL of course
	}
	return _string;
}
Mvalue* getStringValue(Mstring* _string){
    if(_string){
        Mvalue* _stringValue=calloc(1,sizeof(Mvalue));
        if(_stringValue){
            _stringValue->type=VT_STRING;
            _stringValue->value._string=_string;
            return _stringValue;
        }
    }else
        printf("\nERROR: No string to wrap!");
    return NULL;
}
// end helper functions
*/


/*
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
bool setValueOfStringVariable(Mvariable* _variable,Mstring* _string){
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




mstring* appendull(mstring* const ms,unsigned long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}
// helper function
mstring* appendll(mstring* const ms,long long ll){
	char llText[80];
	snprintf(llText,80,"%lld",ll); // TODO will this fit?
	return string_append(ms,llText);
}
mstring* appendld(mstring* const ms,long double ld){
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
}

// Mvalue -> text
// whatever is returned by getIntegerText(),getRealText(),getStringText() needs to be freed!!!!
mstring* _getIntegerText(Minteger* _integer){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'i');
	if(p&&_integer)p=appendll(p,_integer->ll);
    if(!p){free_mstring(s);s=NULL;}
	////////if(amVerbose())output("Integer '%s'.",string(s));
	return s;
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
		case VT_REAL:return double2long(_value->value._real->ld);
		case VT_STRING:return _strtoll(_value->value._string->_c,M_LL_INVALID);
		default:break;
    }
    return M_LL_INVALID;
}
*/

// BigInteger stuff
mstring* _getBigintegerText(const Mbiginteger* const _biginteger){
    // determine the required size
    int arepsize;
    if(mp_radix_size(_biginteger,10,&arepsize)!=MP_OKAY){if(amVerbose())outputError("Can't determine the size of a big integer");return NULL;}
    if(arepsize>0xFFFFFFFF){output("%sCan't store more than %u characters in a string.",ERROR_PREFIX,0xFFFFFFFF);return NULL;}
    mstring* _rep=string_setlength(string_create(),arepsize);
    if(!_rep){output("%sFailed to create a string to hold %d characters.\n",ERROR_PREFIX,arepsize);return NULL;}
    if(mp_toradix(_biginteger,_rep->chars,10)==MP_OKAY){string_synclength(_rep);return _rep;} // return _rep if we succeed in storing the text representation of a
    free_mstring(_rep); // get rid of the mstring that we would have returned on success
    outputError("Failed to create the text representation of a big integer");
    return NULL;
}

Mbiginteger *_biLLMin=NULL,*_biLLMax=NULL;

Mbiginteger* getBigintegerLLMin(){if(!_biLLMin)_biLLMin=_getBiginteger(M_LL_MIN);return _biLLMin;}
Mbiginteger* getBigintegerLLMax(){if(!_biLLMax)_biLLMax=_getBiginteger(M_LL_MAX);return _biLLMax;}

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
void normalizeRational(Mrational* _rational){
    if(!_rational)return;
    // checking on the validity of the flag (which would actually be a bug)
    if(!_rational->normalized&&!_rational->den){output("BUG: Normalized flag of rational not set although the denominator equals 1; flag set.");_rational->normalized=true;}
    if(_rational->normalized)return; // apparently already normalized
    // normalization means dividing by the gcd unless the gcd is one
    Mbiginteger* _gcd=new_biginteger();
    if(!_gcd){outputError("Can't normalize a rational: failed to create the big integer to store the GCD");return;}
    // ASSERT at the end of the following block always free _gcd
    if(mp_gcd(_rational->num,_rational->den,_gcd)==MP_OKAY){
        if(mp_cmp(_gcd,getBigintegerOne())!=MP_EQ){ // equal to 1 apparently no need to divide num and den by the gcd and then consider normalized
            // won't do an in-place division as we need both to succeed, if only one does we would be in trouble
            Mbiginteger *new_num=new_biginteger(),*new_den=new_biginteger();
            if(mp_div(_rational->num,_gcd,new_num,NULL)==MP_OKAY&&mp_div(_rational->den,_gcd,new_den,NULL)==MP_OKAY){
                free_biginteger(_rational->num);_rational->num=new_num;
                free_biginteger(_rational->den);_rational->den=new_den;
                _rational->normalized=true;
            }else{
                free_biginteger(new_num);
                free_biginteger(new_den);
                outputError("Normalization of rational failed");
            }
        }else // the GCD equals 1 which means that the thing is normalized!!!
            _rational->normalized=true;
    }else
        outputError("Can't normalize a rational: failed to compute the GCD");
}

// MDH@07JUN2019: _getRational does NOT free the numerator and denominator supplied!!!
//                as it does not know whether _numerator or _denominator should be released on failure
Mrational* _getRational(Mbiginteger* _numerator,Mbiginteger* _denominator,long double delta,bool normalize,bool freeonfailure){
    // if the given numerator is NULL assume 1
    Mrational* _rational=NULL;
    if(amVerbose()){
        outputBiginteger("\nDetermining the rational with numerator ",_numerator,NULL);outputBiginteger(" and denominator ",_denominator,".");
    }
    if(!_denominator||!isBigintegerZero(_denominator)){ // we have a numerator (any would do), and either NO denominator or a non-zero denominator
        _rational=(Mrational*)calloc(1,sizeof(Mrational));
        if(_rational){
            // TODO what if a delta is defined and the denominator is undefined (i.e. 1)
            if(!ldIsNaN(delta)&&!ldIsInf(delta)&&!ldIsZero(delta))_rational->delta=new_real(delta); // store the delta if a valid value
            // force using a nonnullnumerator, if NULL was provided (typically when inverting a rational)
            Mbiginteger* _nonnullnumerator=(_numerator?_numerator:_getBiginteger(1));
            if(_nonnullnumerator){
                _rational->num=_nonnullnumerator; // could be NULL now when it's the inverse of another rational
                _rational->den=_denominator;
                if(amVerbose())outputRational("\nRational created: ",_rational,".");
                /* STORING 1 AS DENOMINATOR ISN'T WRONG per se 
                if(_denominator&&mp_cmp(_denominator,getBigintegerOne())==MP_EQ){
                    free_biginteger(_denominator); // won't store denominator equal to 1
                    _rational->den=NULL; // probably already is though
                }else // either NULL or not equal to 1
                    _rational->den=_denominator;
                */
                _rational->normalized=(!_rational->den||!_rational->num); // if either numerator or denominator is NULL assume normalized!!!
                if(amVerbose())
                outputRational("\nRational before normalization: ",_rational,".");
                if(normalize&&!_rational->normalized){
                    normalizeRational(_rational); // normalize the rational if we are supposed to
                    if(_denominator&&!_rational->normalized)output("WARNING: Failed to normalize a rational number.");
                    if(amVerbose())
                    outputRational("\nRational after normalization: ",_rational,".");
                }
                ///////// AS LONG AS WE FREE THE RATIONAL IN THE ELSE PART NO NEED TO DO: return _rational; // return whether normalized or not
            }else{
                free_rational(_rational);_rational=NULL;
                if(amVerbose())output("WARNING: Undefined rational numerator.");
            }
            // NOTE if we get here we failed to create the big integer 1 to use as numerator!!!
        }
    }
    /* NOT OUR RESPONSIBILITY unless we decide to do that
    // if we get here we failed storing _numerator and _denominator, so we free them
    if(_numerator)free_biginteger(_numerator);
    if(_denominator)free_biginteger(_denominator);
    */
    if(!_rational)if(freeonfailure){free_biginteger(_numerator);free_biginteger(_denominator);}
    return _rational;
}
// _getInverseRational() will take care of releasing the newly created rational parts when failing to wrap them in a rational
Mrational* _getInverseRational(const Mrational* const _rational){
    if(!_rational){output("WARNING: No rational to invert.");return NULL;}
    Mrational* _inverseRational=NULL;
    // for now only allow inverting pure rationals!!!
    if(!_rational->delta||ldIsZero(_rational->delta->ld)){
        Mbiginteger* _inverseRationalNumerator=NULL;
        if(_rational->den){
            _inverseRationalNumerator=_getBigintegerCopy(_rational->den);
            if(!_inverseRationalNumerator){outputError("Failed to copy the rational denominator");return NULL;}
        }
        Mbiginteger* _inverseRationalDenominator=NULL;
        if(_rational->num){
            _inverseRationalDenominator=_getBigintegerCopy(_rational->num);
            if(!_inverseRationalDenominator){outputError("Failed to copy the rational numerator");return NULL;}
        }
        // if the original is not normalized normalize, otherwise just copy the normalized flag!!
        _inverseRational=_getRational(_inverseRationalNumerator,_inverseRationalDenominator,M_LD_NAN,!_rational->normalized,true);
        if(!_inverseRational)
            outputError("Failed to create the inverse rational");
        else
        if(_rational->normalized)_inverseRational->normalized=true; // nasty TODO check if this is correct
    }else
        outputError("Can't invert an unpure rational");
    return _inverseRational;
}

// MDH@08JUN2019 NOTE: adapted so that if the numerator is NULL will assume the numerator to equal 1
// BUT _getRational has been adapted to NOT allow a NULL numerator, i.e. replacing NULL with big integer 1, so actually a NULL numerator is unlikely to occur!!!
// rationals can equal zero or one but only when the delta value equals 0 (or is not defined which is the same)
bool isRationalZero(Mrational* _rational){
    // if the rational does not have a num, the numerator equals 1, and obviously is NOT zero
    return(_rational&&_rational->num?isBigintegerZero(_rational->num)&&(!_rational->delta||ldIsZero(_rational->delta->ld)):false); // the delta needs to be undefined (i.e. zero)
}
bool isRationalOne(Mrational* _rational){
    // when the numerator is NULL, it is considered to be equal to 1
    if(!_rational)return false;
    // basically a rational equals 1 if the numerator and denominator are the same
    if(_rational->delta&&!ldIsZero(_rational->delta->ld))return false; // TODO if the rational delta is not zero, do not consider to be equal to 1 (although theoretically it could be)
    return (_rational->den?mp_cmp(_rational->num,_rational->den)==MP_EQ:isBigintegerOne(_rational->num));
    // replacing: return(_rational?!(_rational->num||isBigintegerOne(_rational->num))&&(!_rational->den||isBigintegerOne(_rational->den))&&(!_rational->delta||ldIsZero(_rational->delta->ld)):false);
}

// we can use the method below to come up with the numerator and denominator of a given double that matches the double exactly
// for dealing with long double to big integer conversion
// MDH@17JUN2019: although a typically is of type Mbiginteger, for a function that starts with mp_ we can use the primitive type mp_int instead of the alias Mbiginteger
mp_err mp_set_me(mp_int* a,uint64_t mantisse,uint16_t exponent){
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
}
mp_err mp_set_me_verbose(mp_int* a,uint64_t mantisse,uint16_t exponent){
    int32_t exp=(exponent&0x7FFF); // cut off the sign
    if(exp!=0){
        mp_set_u64(a,mantisse);
        if(amVerbose()){
            mstring* _mantisseBigIntegerText=_getBigintegerText(a);
            output("Value after setting the fraction: %s.",string(_mantisseBigIntegerText));
            free_mstring(_mantisseBigIntegerText);
        }
        if(amVerbose())output("Long double exponent part: %d - mantisse: %llu.",exp,mantisse);
        if(exp==0x7FFF){if(amVerbose())output("NOTE: Cannot convert an invalid or infinite real value to a big integer.");return MP_VAL;} // +-inf, NaN
        exp-=0x403E; // same as exp-=(16383+63); // the actual exponent (as 63 out of 64 mantisse bits are 'significant', bit 63 equals 1 for normalized numbers) 
        //////////frac=(frac<<1)>>1;/// replacing: &0x7FFFFFFFuLL; // I have to cut off bit 63
        if(amVerbose())output("Power of two exponent: %d.",exp);  
        if(exp!=0){
            mp_err err=(exp>0?mp_mul_2d(a,exp,a):mp_div_2d(a,-exp,a,NULL));
            if(err!=MP_OKAY){outputError("Failed to use the exponent of a real value in the conversion to a big integer");return err;}
        }
        if(amVerbose()){
            mstring* _bigIntegerText=_getBigintegerText(a);
            output("Value after applying the exponent: %s.",string(_bigIntegerText));
            free_mstring(_bigIntegerText);
        }
        if(exponent>>15){ // negative
            // take over the sign from the long double (bit 15 in the signandexponent part)
            if(mp_iszero(a)==MP_NO){ // TODO preferable NOT to use used directly!!
                a->sign=MP_NEG;
                if(amVerbose())output("Sign part of real used to set the sign of the big integer.");
            }else
                if(amVerbose())output("No need to set the sign on a big integer equal to zero.");           
        }
    }else // all zeros in exponent
        mp_zero(a);
    return MP_OKAY;
}
mp_err mp_set_longdouble(Mbiginteger *a, long double b){
    // always assume 10-byte long double (extended precision)
    uint64_t mantisse;
    uint16_t exponent; // including bit 63
    extractMantisseAndExponent(b,&mantisse,&exponent);
    // determine the sign, and the 15-bit power of two exponent
    return mp_set_me_verbose(a,mantisse,exponent); ///////////return (amVerbose()?mp_set_me_verbose(a,mantisse,exponent):mp_set_me(a,mantisse,exponent));
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
}
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
    if(amVerbose())output("Real of big integer digit %lld initialized to '%.*Lf' yet to shift by %u big integer digits.",a->dp[i],LDBL_DIG,d,i);
    while(--i>=0){
        if(amVerbose())output("Multiplying '%.*Lf' by %Lf.",d,M_LD_DIGIT_MULTIPLIER);
        d*=M_LD_DIGIT_MULTIPLIER;
        if(amVerbose())output("Result of multiplying by '%Lf': '%.*Lf'.",M_LD_DIGIT_MULTIPLIER,LDBL_DIG,d);
        d+=(long double)a->dp[i];
        if(amVerbose())output("Result of adding '%lld': '%.*Lf'.",a->dp[i],LDBL_DIG,d);
    }
    if(a->sign==MP_NEG&&!ldIsNaN(d))return -d;
    if(amVerbose())output("Conversion of big integer to long double '%.*Lf' done!",LDBL_DIG,d);
    return d;
    // replacing: return(a->sign==MP_NEG&&!ldIsNaN(d)?-d:d);
}

// part of implementing _getRealText (so not present in the header)
const char* M_NAN="NaN";
const char* M_INF="Inf";
bool ldIsZero(long double ld){return fpclassify(ld)==FP_ZERO;}
bool ldIsNaN(long double ld){return fpclassify(ld)==FP_NAN;}
bool ldIsInf(long double ld){return fpclassify(ld)==FP_INFINITE;}
long long double2long(long double ld){
    if(ldIsNaN(ld)||ldIsInf(ld))return M_LL_INVALID;
    // TODO perhaps there are some other 
    if(ldIsZero(ld))return 0;
    long double tld=truncl(ld); // extract the integer part i.e. floor towards zero (which is called truncate)
    if(tld<M_LL_MIN||tld>M_LL_MAX)return M_LL_INVALID; // out of range
    return(long long)tld;
}
mstring* _getRealText(Mreal* _real){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'r');
	if(p&&_real){
		switch(fpclassify(_real->ld)){
			case FP_NAN:p=string_append(p,M_NAN);break;
			case FP_INFINITE:p=string_append(p,M_INF);break;
			default:p=appendld(p,_real->ld);break;
		}
        if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}
mstring* _getStringText(Mstring* _string,bool dequoted){
	mstring* s=string_create();
    mstring* p=s;
    if(amDebugging())p=string_append_char(p,'s');
    if(p&&_string){
        if(!dequoted)p=string_append_char(p,_string->presuffix);
        p=string_append(p,_string->_c);
        if(!dequoted)p=string_append_char(p,_string->presuffix);
        if(!p){free_mstring(s);s=NULL;}
    }
	return s;
}

mstring* _getRationalText(const Mrational* const _rational){
    if(_rational){
        mstring* _rationalText=string_create();
        if(_rationalText){
            mstring* _p=_rationalText;
            _p=string_append_char(_p,'(');
            mstring* _numeratorBigintegerText=_getBigintegerText(_rational->num);
            if(_numeratorBigintegerText){_p=string_append(_p,string(_numeratorBigintegerText));free_mstring(_numeratorBigintegerText);}
            if(_rational->den){
                _p=string_append_char(_p,'/');
                mstring* _denominatorBigintegerText=_getBigintegerText(_rational->den);
                if(_denominatorBigintegerText){_p=string_append(_p,string(_denominatorBigintegerText));free_mstring(_denominatorBigintegerText);}
            }
            _p=string_append_char(_p,')');
            // if a delta is known, append that as well!!!
            if(_rational->delta){
                // always show a sign
                if(_rational->delta>=0)string_append_char(_p,'+');
                _p=appendld(_p,_rational->delta->ld);
            }
            if(_p)return _rationalText;
            free_mstring(_rationalText);
        }
    }
    return NULL;
}
// if decimal->repeating fixedpoint will determine whether or not to append ] so pass in false in that case!!!!!
mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint){
    mstring* _decimalText=NULL;
    if(_decimal){
        // NOTE not using mpd_to_sci as we do not know when we get an e-part!!!!
        // NOTE if _decimal->repeating always use fixed-point notation
        char* _decimalRep=mpd_format(_decimal->mpd,(fixedpoint||_decimal->repeating?"f":"g"),_decimalContext);
        if(_decimalRep){
            _decimalText=new_mstring(_decimalRep);
            free(_decimalRep);
            // TODO what if the decimal text representation has an e-part?????
            // bracket the repeating part
            if(_decimal->repeating){string_insert_char(_decimalText,string_length(_decimalText)-_decimal->repeating,'[');if(!fixedpoint)string_append_char(_decimalText,']');}
        }
        return _decimalText;
    }
    return NULL;
}

/////////////mstring* _UNDEFINED_VALUETEXT=NULL;
// the problem here is that whatever _getValueText returns will be freed on the other side, which we would not want to happen with _UNDEFINED_VALUETEXT, so perhaps we should return NULL in that case after all????
// we can solve that by returning a new undefined value text instance every time
mstring* getUndefinedValueText(){
    return new_mstring(UNDEFINED_VALUETEXT); // just wrapping UNDEFINED_VALUETEXT again...
    /* replacing:
    if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(string_create(),UNDEFINED_VALUETEXT);
    return string_copy(_UNDEFINED_VALUETEXT);
    */
}

void outputBiginteger(const char* const prefix,const Mbiginteger* const _biginteger,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_biginteger){
        mstring* _bigintegerText=_getBigintegerText(_biginteger);
        if(_bigintegerText){
            output("%s",string(_bigintegerText));
            free_mstring(_bigintegerText);
        }else
            output("too large for buffer");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}
void outputDecimal(const char* const prefix,const Mdecimal* const _decimal,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_decimal){
        mstring* _decimalText=_getDecimalText(_decimal,false);
        if(_decimalText){
            output("%s",string(_decimalText));
            free_mstring(_decimalText);
        }else
            output("no decimal text representation");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}
void outputRational(const char* const prefix,const Mrational* const _rational,const char* const postfix){
    if(prefix)output("%s",prefix);
    if(_rational){
        mstring* _rationalText=_getRationalText(_rational);
        if(_rationalText){
            output("%s",string(_rationalText));
            free_mstring(_rationalText);
        }else
            output("no rational text representation");
    }else
        outputChar('?');
    if(postfix)output("%s",postfix);
}

// conversion from big integer to the long long it contains (when in range)
long long biginteger2long(Mbiginteger* _biginteger){
	return(mp_cmp(_biginteger,getBigintegerLLMin())!=MP_LT&&mp_cmp(_biginteger,getBigintegerLLMax())!=MP_GT?mp_get_i64(_biginteger):M_LL_INVALID);
}
bool strIsZero(char* str){
    size_t l=strlen(str);
    //// NOTE do not accept integer literal postfixes when checking for 1: if(l>0&&str[l-1]=='i'||str[l-1]=='I'||str[l-1]=='q'||str[l-1]=='r')l-=1; // skip any accepted integer postfix!!
    // if l already is zero str[0] will equal '\0' which (see below) is not considered a zero integer!!!!
    while(l>0){l--;if(str[l]!='0')break;} // stop as soon as the character does not match '0' (any sign is only allowed at position 0)
    return(!l?false:str[l]=='-'||str[l]=='+'||str[l]=='0');
}

// NOTE the _ indicates that what is returned has to be freed after being used
Mbiginteger* _rational2biginteger(Mrational* _rational){
    if(!_rational)return NULL;
    Mbiginteger* _biginteger=new_biginteger();
    if(!_biginteger){outputError("Failed to create a big integer");return NULL;}
    if(_rational->den){
        if(mp_div(_rational->num,_rational->den,_biginteger,NULL)!=MP_OKAY){
            outputError("Failed to divide the rational numerator and denominator");
            free_biginteger(_biginteger);
            return NULL;
        }
    }else // copy the numerator
        if(mp_copy(_rational->num,_biginteger)!=MP_OKAY){outputError("Failed to copy the rational numerator");free_biginteger(_biginteger);return NULL;}
    return _biginteger;
}


bool isDecimalZero(Mdecimal* _decimal){
    return(_decimal&&mpd_iszero((mpd_t*)_decimal)==0); // TODO apparently 0 means true, something else means false
}

/*
// big integer arithmetic
Mvalue* Mbi(Mvalue* _value){
    if(_value&&_value->type==VT_INTEGER){
        z_t bi;
        zinit(bi); // TODO do I need this??????
        zseti(bi,_value->value._integer->ll);
        Mvalue* _biValue=_getBigIntegerValue(bi);
    }
    return NULL;
}
*/

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
            Mbiginteger* _numerator=_getBiginteger(p),*_denominator=_getBiginteger(q);
            if(_numerator&&_denominator){ // we've got both of them
                if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY){
                    _rational=_getRational(_numerator,_denominator,delta,true,false); // NOTE there should always be a delta!!!!
                }
            }
            if(!_rational){free_biginteger(_numerator);free_biginteger(_denominator);}
        }else // long double is zero
            _rational=_getRational(new_biginteger(),NULL,M_LD_NAN,false,true);
    }
    // _rational should contain the 'last' computed rational
    return _rational;
}
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
}
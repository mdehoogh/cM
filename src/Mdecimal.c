#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <inttypes.h>

#include "Mdecimal.h"

static uint16_t const MODULE_ID=12;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}

extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_ZERO,M_POSITIVE,M_NEGATIVE,M_TRUE,M_FALSE;
extern long double const M_LD_NAN;
extern char const * const M_ERROR_PREFIX; // TODO rename to M_ERROR_PREFIX
extern Mdecimalcontext* const M_DECIMALCONTEXT; // ASSERT should not be NULL whenever M is up and running

mpd_t* get_mpd_copy(mpd_context_t const * mpd_context,mpd_t* mpd){
	if(!mpd)return NULL;
	if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context;
	mpd_t* _mpd=__mpd(mpd_context,0);
	if(_mpd){uint32_t status=0;mpd_qcopy(_mpd,mpd,&status);if((status&0xEFBF)!=0){free_mpd(_mpd);return NULL;}}
	return _mpd;
}

Msincoselement* owned_sincoselement(Msincoselement* _sincoselement,Mallocationowner owner_sincoselement){
    if(!_sincoselement)return NULL;
	if(_sincoselement->_next)owned_sincoselement(_sincoselement->_next,owner_sincoselement); // unlikely though
	return OWNED(_sincoselement,owner_sincoselement);
}
Msincoselement* disowned_sincoselement(Msincoselement* _sincoselement,Mallocationowner owner_sincoselement){
    if(!_sincoselement)return NULL;
	if(_sincoselement->_next)disowned_sincoselement(_sincoselement->_next,owner_sincoselement); // unlikely though
	return DISOWNED(_sincoselement,owner_sincoselement);
}
void free_sincoselement(Msincoselement* _sincoselement/*,Mallocationowner owner_sincoselement*/){
    if(_sincoselement){
		if(_sincoselement->_next)free_sincoselement(_sincoselement->_next/*,owner_sincoselement*/); // unlikely though
        free_mpd(_sincoselement->_angle);
		free_mpd(_sincoselement->_sine);
		free_mpd(_sincoselement->_cosine); // MDH@20MAY2020: TODO whoever calls free_sincoselement needs to disown it first
		FREE_1(_sincoselement,'#'/*,owner_sincoselement*/);
    }
}
#define FREE_SINCOSELEMENT(_sincoselement,owner_sincoselement) free_sincoselement(disowned_sincoselement(_sincoselement,owner_sincoselement))

mpd_context_t* owned_mpd_context(mpd_context_t* _mpd_context,Mallocationowner owner_mpd_context){
	return OWNED(_mpd_context,owner_mpd_context);
}mpd_context_t* disowned_mpd_context(mpd_context_t* _mpd_context,Mallocationowner owner_mpd_context){
	return DISOWNED(_mpd_context,owner_mpd_context);
}
mpd_context_t* __mpd_context(mpd_ssize_t decimalprecision){Mallocationowner owner=getOwner(__LINE__);
	mpd_context_t* _mpd_context=(mpd_context_t*)CALLOC_1(sizeof(mpd_context_t),'c',owner); // MDH@29APR2020: calloc replaced by CALLOC for sure, because it must match FREE(,'c') on mpd_context (see below)
	if(_mpd_context){
		if(amVerbose())output("New decimal context with precision %lld created.\n",decimalprecision);
		// initialize the new context to the default context (specification)
		mpd_init(_mpd_context,decimalprecision);
		if(amVerbose())output("Decimal context with precision %u initialized.\n",mpd_getprec(_mpd_context));
	}else
		outputError("Failed to create a new decimal context");
	return disowned_mpd_context(_mpd_context,owner);
}

Mdecimalcontext* disowned_decimalcontext(Mdecimalcontext* _decimalcontext,Mallocationowner owner_decimalcontext){
	if(!_decimalcontext)return NULL;
	if(_decimalcontext->mpd_context)disowned_mpd_context(_decimalcontext->mpd_context,owner_decimalcontext);
	if(_decimalcontext->_firstCordicelement)disowned_sincoselement(_decimalcontext->_firstCordicelement,owner_decimalcontext);
	if(_decimalcontext->_firstSincoselement)disowned_sincoselement(_decimalcontext->_firstSincoselement,owner_decimalcontext);
	return DISOWNED(_decimalcontext,owner_decimalcontext);
}
Mdecimalcontext* owned_decimalcontext(Mdecimalcontext* _decimalcontext,Mallocationowner owner_decimalcontext){
	if(!_decimalcontext)return NULL;
	if(_decimalcontext->mpd_context)owned_mpd_context(_decimalcontext->mpd_context,owner_decimalcontext);
	if(_decimalcontext->_firstCordicelement)owned_sincoselement(_decimalcontext->_firstCordicelement,owner_decimalcontext);
	if(_decimalcontext->_firstSincoselement)owned_sincoselement(_decimalcontext->_firstSincoselement,owner_decimalcontext);
	return OWNED(_decimalcontext,owner_decimalcontext);
}
void free_decimalcontext(Mdecimalcontext* _decimalcontext/*,Mallocationowner owner_decimalcontext*/){
	if(!_decimalcontext)return;
	if(_decimalcontext->mpd_context)FREE_1(_decimalcontext->mpd_context,'c'/*,owner_decimalcontext*/);
	if(_decimalcontext->pi)free_mpd(_decimalcontext->pi);
	if(_decimalcontext->pimul2)free_mpd(_decimalcontext->pimul2);
	if(_decimalcontext->pidiv2)free_mpd(_decimalcontext->pidiv2);
	if(_decimalcontext->pidiv4)free_mpd(_decimalcontext->pidiv4);
	if(_decimalcontext->e)free_mpd(_decimalcontext->e);
	// MDH@29APR2020: some additional stuff to free
	if(_decimalcontext->predefinedsinedeltaangle)free_mpd(_decimalcontext->predefinedsinedeltaangle);
	for(int index=256;index>=0;index--)if(_decimalcontext->predefinedsines[index])free_mpd(_decimalcontext->predefinedsines[index]);
	if(_decimalcontext->_firstCordicelement)free_sincoselement(_decimalcontext->_firstCordicelement/*,owner_decimalcontext*/);
	if(_decimalcontext->_firstSincoselement)free_sincoselement(_decimalcontext->_firstSincoselement/*,owner_decimalcontext*/);
	FREE_1(_decimalcontext,'C'/*,owner_decimalcontext*/);
}
#define FREE_DECIMALCONTEXT(_decimalcontext,owner_decimalcontext) free_decimalcontext(disowned_decimalcontext(_decimalcontext,owner_decimalcontext))

// if we want to keep a list of all decimal contexts, we need to be able to iterate over all decimal contexts to see if it is already there
// create a single Mdecimalcontextelement
typedef struct MdecimalcontextElement{
	Mdecimalcontext* _decimalcontext;
	struct MdecimalcontextElement* _next;
}MdecimalcontextElement;

MdecimalcontextElement* owned_decimalcontextElement(MdecimalcontextElement* _decimalcontextElement,Mallocationowner owner_decimalcontextElement){
	if(!_decimalcontextElement)return NULL;
	if(_decimalcontextElement->_next)owned_decimalcontextElement(_decimalcontextElement->_next,owner_decimalcontextElement); // free whatever it is pointing to
	if(_decimalcontextElement->_decimalcontext)owned_decimalcontext(_decimalcontextElement->_decimalcontext,owner_decimalcontextElement); // free whatever decimal context it is referring to
	return(MdecimalcontextElement*)OWNED(_decimalcontextElement,owner_decimalcontextElement);
}
MdecimalcontextElement* disowned_decimalcontextElement(MdecimalcontextElement* _decimalcontextElement,Mallocationowner owner_decimalcontextElement){
	if(!_decimalcontextElement)return NULL;
	if(_decimalcontextElement->_next)disowned_decimalcontextElement(_decimalcontextElement->_next,owner_decimalcontextElement); // free whatever it is pointing to
	if(_decimalcontextElement->_decimalcontext)disowned_decimalcontext(_decimalcontextElement->_decimalcontext,owner_decimalcontextElement); // free whatever decimal context it is referring to
	return(MdecimalcontextElement*)DISOWNED(_decimalcontextElement,owner_decimalcontextElement);
}
void free_decimalcontextElement(MdecimalcontextElement* _decimalcontextElement/*,Mallocationowner owner_decimalcontextElement*/){
	if(!_decimalcontextElement)return;
	if(_decimalcontextElement->_next)free_decimalcontextElement(_decimalcontextElement->_next/*,owner_decimalcontextElement*/); // free whatever it is pointing to
	if(_decimalcontextElement->_decimalcontext)free_decimalcontext(_decimalcontextElement->_decimalcontext/*,owner_decimalcontextElement*/); // free whatever decimal context it is referring to
	FREE_1(_decimalcontextElement,'e'/*,owner_decimalcontextElement*/);
}
#define FREE_DECIMALCONTEXTELEMENT(_decimalcontextElement,owner_decimalcontextElement) free_decimalcontextElement(disowned_decimalcontextElement(_decimalcontextElement,owner_decimalcontextElement))

static MdecimalcontextElement *_firstDecimalcontextElement=NULL,*_lastDecimalcontextElement=NULL;Mallocationowner owner_decimalcontextElement={MODULE_ID,__LINE__,1};
// to get the unique decimal context with the requested precision
static Mdecimalcontext* _getExistingDecimalcontext(mpd_ssize_t prec){
	if(prec<6)return NULL; // prec needs to be at least 6
	MdecimalcontextElement *decimalcontextElement=_firstDecimalcontextElement;
	while(decimalcontextElement&&decimalcontextElement->_decimalcontext->mpd_context->prec!=prec)decimalcontextElement=decimalcontextElement->_next;
	return(decimalcontextElement?decimalcontextElement->_decimalcontext:NULL);
}
Mdecimalcontext* _getDecimalcontext(mpd_ssize_t prec){Mallocationowner owner=getOwner(__LINE__); // NOTE a global so use module level id
	Mdecimalcontext* decimalcontext=_getExistingDecimalcontext(prec);
	if(decimalcontext)return decimalcontext;
	MdecimalcontextElement* decimalcontextElement=(MdecimalcontextElement*)CALLOC_1(sizeof(MdecimalcontextElement),'e',owner);
	if(decimalcontextElement){
		decimalcontextElement->_decimalcontext=(Mdecimalcontext*)CALLOC_1(sizeof(Mdecimalcontext),'C',owner);
		if(decimalcontextElement->_decimalcontext){
			decimalcontextElement->_decimalcontext->mpd_context=owned_mpd_context(__mpd_context(prec),owner);
			if(decimalcontextElement->_decimalcontext->mpd_context){
				if(_lastDecimalcontextElement)_lastDecimalcontextElement->_next=decimalcontextElement;
				_lastDecimalcontextElement=decimalcontextElement;
				if(!_firstDecimalcontextElement)_firstDecimalcontextElement=_lastDecimalcontextElement;
				return decimalcontextElement->_decimalcontext;
			}
			// not bound!!!
			disowned_decimalcontext(decimalcontextElement->_decimalcontext,owner);
		}
		FREE_DECIMALCONTEXTELEMENT(decimalcontextElement,owner);
	}else
		output("%sFailed to create the decimal context with precision " PRIu64 ".\n",M_ERROR_PREFIX,prec);
	return NULL;
}

// if decimal->repeating fixedpoint will determine whether or not to append ] so pass in false in that case!!!!!
Mstring* _getDecimalText(Mdecimal const * const _decimal,bool fixedpoint){Mallocationowner owner=getOwner(__LINE__);
    Mstring* _decimalText=owned_string(__string(),owner);
    if(_decimalText){
		if(_decimal&&_decimal->mpd){ // MDH@16JUN2020: was getting Segmentation fault on _decimal->mpd TODO why????????
			/////////outputChar('D');
            Mstring* _p=_decimalText;
            // NOTE not using mpd_to_sci as we do not know when we get an e-part!!!!
            // NOTE if _decimal->repeating always use fixed-point notation
			// MDH@20MAY2020 _decimalChars here eludes the dynamic allocation registration so essential to validate this function
            char* _decimalChars=mpd_format(_decimal->mpd,(fixedpoint||_decimal->repeating?"f":"g"),M_DECIMALCONTEXT->mpd_context);
            // output("Decimal rep: '%s'.\n",_decimalChars);
            if(_decimalChars){
                _p=string_append(_p,_decimalChars);
				// output("Decimal characters appended!");
                if(_p){
					if(_decimal->repeating){
                    	_p=string_insert_char(_p,string_length(_p)-_decimal->repeating,'[');
                    	if(_p&&!fixedpoint)_p=string_append_char(_p,']');
					}else{
						// if there's a period in _p, and no 'e' we can insert a blank every 50 decimals
						char* _period=strchr(_decimalChars,'.');
						if(_period){
							if(!strchr(_decimalChars,'e')){
								// output("**** Inserting blanks! ****\n");
								size_t periodpos=(_period-_decimalChars); // index position of the period
								while(periodpos+51<string_length(_p)){string_insert_char(_p,periodpos+51,' ');periodpos+=51;}
								// output("**** Blanks inserted! ****\n");
							}
						}
					}
                }
                free(_decimalChars); // NOTE assuming mpd_format dynamically created _decimalChars transferring ownership to me
            }else
                _p=NULL;
            if(!_p){FREE_STRING(_decimalText,owner);_decimalText=NULL;}
        }else
			string_append_char(_decimalText,'?');
    }else outputChar('?');
    return disowned_string(_decimalText,owner);
}/* VALIDATED */

/**
 * \brief returns a copy of \p _decimal
 * \param _decimal the decimal to copy
 * \return a copy of \p _decimal on success, or NULL otherwise
 */
Mdecimal* _getDecimalCopy(Mdecimal const * const _decimal){Mallocationowner owner=getOwner(__LINE__);
	if(!_decimal)return NULL;
	Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
	// MDH@31MAY2020 undone, so no need to do this: if(!decimalcontext)decimalcontext=_getNewDecimalContext(_decimal->prec);
    if(!decimalcontext)return NULL;
	mpd_t* _mpd=get_mpd_copy(decimalcontext->mpd_context,_decimal->mpd); // make a copy
	if(!_mpd)return NULL;
	Mdecimal* _decimalCopy=owned_decimal(_getDecimal(_mpd,_decimal->prec,_decimal->repeating,true),owner);
	return disowned_decimal(_decimalCopy,owner); // if failing to wrap the mpd copy free it
	/* replacing:
	// TODO use get_mpd_copy() to copy the mpd in _decimal to speed things up, and use the
    Mdecimal* _decimalCopy=__decimal(M_DECIMALCONTEXT->mpd_context,0,_decimal->repeating);
    if(_decimalCopy){
        _decimalCopy->mpd=NULL; // TODO check if __decimal uses calloc or malloc (BTW this does not seem to be such a good idea as mpd_copy would fail)
        mpd_copy(_decimalCopy->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context); // copy attempt
        if(!_decimalCopy->mpd){free_decimal(_decimalCopy);_decimalCopy=NULL;} // on failure, release the decimal
    }else
        outputError("Failed to copy a decimal");
    return _decimalCopy;
	*/
}/* VALIDATED */

// CONVERSION FROM OTHER M TYPES
// MDH@24JUN2019: used an Mlist before to store the remainders, but because Mlist uses Mvalue instances, which we do not have access to here anymore (we have to create our own list for storing the remainders)
typedef struct MbigintegerListelement{
    Mbiginteger* _biginteger;
    struct MbigintegerListelement* _next;
}MbigintegerListelement;

// MDH@20MAY2020: you can't free what isn't yours to start with, we can make it easy by allowing passing in the owner id of the big integer list element
static void free_bigintegerListelement(MbigintegerListelement* _bile,Mallocationowner owner){
    // ASSERT assume _bile to not be NULL
    if(_bile->_next)free_bigintegerListelement(_bile->_next,owner);
    if(_bile->_biginteger)FREE_BIGINTEGER(_bile->_biginteger,owner);
	FREE_DISOWNED_1(_bile,'x',owner);
}/* VALIDATED */

// MDH@17JUN2019: convert a decimal (back) to a rational
Mrational* _getDecimalRational(Mdecimal const * const decimal){Mallocationowner owner=getOwner(__LINE__);
    Mrational* _rational=NULL;
    if(decimal){
        // get the decimal text in fixed point format if it is not repeating, otherwise we always get in in fixed point but then without the closing ]
        Mstring* _decimalText=owned_string(_getDecimalText(decimal,decimal->repeating>0),owner); // replacing: mpd_to_sci(_decimal->mpd,0);
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
                    Mbiginteger *_num3=owned_biginteger(__biginteger(),owner),*_bi10=owned_biginteger(_getBiginteger(10),owner),*_den2=owned_biginteger(_getBiginteger(10),owner),*_den1=owned_biginteger(_getBiginteger(1),owner);
                    ////////output("Temporary dynamic variables created.\n");
                    if(_num3&&_bi10&&_den2&&_den1&&mp_read_radix(MP_INT_POINTER(_num3),repeatingText,10)==MP_OKAY){
                        if(amVerbose())outputBiginteger("Repeating digits numerator part: '",_num3,"'.\n");
                        for(int i=decimal->repeating;i>1;i--)if(mp_mul(MP_INT_POINTER(_den2),MP_INT_POINTER(_bi10),MP_INT_POINTER(_den2))!=MP_OKAY){
							outputError("Failed to multiply the second rational denominator part by 10");
							FREE_BIGINTEGER(_den2,owner);
							_den2=NULL;
							break;
						}
                        if(_den2&&mp_decr(MP_INT_POINTER(_den2))==MP_OKAY){ // _den2 computed (as 9999....9)
                            // let's determine the denominator
                            Mbiginteger* _den=NULL;
                            int numberOfNonRepeatingDecimalDigits=(int)(repeatingText-periodText-2); // compute the number of non repeating decimals
                            ///////////////////mp_int* _den1=_getBiginteger(1);
                            if(numberOfNonRepeatingDecimalDigits>0){
                                while(_den1&&(--numberOfNonRepeatingDecimalDigits>=0))
									if(mp_mul(MP_INT_POINTER(_den1),MP_INT_POINTER(_bi10),MP_INT_POINTER(_den1))!=MP_OKAY)
									{outputError("Failed to multiply the first rational denominator part by 10");FREE_BIGINTEGER(_den1,owner);_den1=NULL;}
                                if(_den1){
                                    _den=owned_biginteger(__biginteger(),owner);
                                    if(mp_mul(MP_INT_POINTER(_den1),MP_INT_POINTER(_den2),MP_INT_POINTER(_den))!=MP_OKAY){
										FREE_BIGINTEGER(_den,owner);
										_den=NULL;
									}else 
									if(amVerbose())outputBiginteger("First denominator multiplier: '",_den1,"'.\n");
                                }
                            }else
                                _den=owned_biginteger(_getBigintegerCopy(_den2),owner);
                            if(_den){ // denominator computed successfully, either to be bound or freed in this block
                                *periodText='\0'; // no harm overwriting the period with end-of-text character so _decimalText will contain the before period integer part
                                periodText++; // point periodText to the first digit behind the decimal period
                                if(amVerbose())output("Behind period text: '%s'.\n",periodText);
                                
                                // the numerator is the sum of what's in front of the repeating digits plus the integer representing the repeating digits (_num2)
                                Mbiginteger* _num=owned_biginteger(_getBigintegerCopy(_num3),owner); // initialize _num to the repeating digits integer
                                // add the fixed part of the decimal digits (treated as integer)
                                if(_num&&strlen(periodText)){ // something between the period and the repeating digits
                                    Mbiginteger* _num2=owned_biginteger(__biginteger(),owner); // _num2 is freed below, so that's good
                                    if(mp_read_radix(MP_INT_POINTER(_num2),periodText,10)!=MP_OKAY||mp_mul(MP_INT_POINTER(_num2),MP_INT_POINTER(_den2),MP_INT_POINTER(_num2))!=MP_OKAY||mp_add(MP_INT_POINTER(_num),MP_INT_POINTER(_num2),MP_INT_POINTER(_num))!=MP_OKAY){
										FREE_BIGINTEGER(_num,owner);
										_num=NULL;
									}else 
                                    if(amVerbose())outputBiginteger("Non-repeating digits numerator part: '",_num2,"'.\n");
                                    FREE_BIGINTEGER(_num2,owner);
                                }
                                if(_num){ // so far so good
                                    // _num to be bound or freed in this block!!!
                                    // add the part in front of the period multiplied by _den1 but it could be zero of course
                                    Mbiginteger* _num1=owned_biginteger(__biginteger(),owner); // _mul1 freed below (which is reachable)
                                    if(mp_read_radix(MP_INT_POINTER(_num1),decimalText,10)==MP_OKAY){
                                        // of course the integer part could well be zero!!!!
                                        if(!isBigintegerZero(_num1)){
                                            if(mp_mul(MP_INT_POINTER(_num1),MP_INT_POINTER(_den),MP_INT_POINTER(_num1))!=MP_OKAY
												||mp_add(MP_INT_POINTER(_num),MP_INT_POINTER(_num1),MP_INT_POINTER(_num))!=MP_OKAY){
													FREE_BIGINTEGER(_num,owner);
													_num=NULL;
											}else
											if(amVerbose())outputBiginteger("Integer numerator part: '",_num1,"'.\n");
                                        }
                                    }else{
										FREE_BIGINTEGER(_num,owner);
										_num=NULL;
									}
                                    FREE_BIGINTEGER(_num1,owner);    
                                }
                                // negate the numerator if the decimal is negative
                                if(_num&&neg&&mp_neg(MP_INT_POINTER(_num),MP_INT_POINTER(_num))!=MP_OKAY){FREE_BIGINTEGER(_num,owner);_num=NULL;}
                                if(_num){
									_rational=owned_rational(_getRational(_num,_den,M_LD_NAN,true),owner);
									FREE_BIGINTEGER(_num,owner);
								}
                                // if rational is NULL, _den and _num are not bound, otherwise they are, can't harm to try to release if not set though
								FREE_BIGINTEGER(_den,owner);
                            }
                        }else
                            outputError("Failed to compute the second denominator multiplier");
                    }else
                        outputError("Failed to construct the integer containing the repeating digits");
                    FREE_BIGINTEGER(_num3,owner);
                    FREE_BIGINTEGER(_bi10,owner);
                    FREE_BIGINTEGER(_den2,owner);
                    FREE_BIGINTEGER(_den1,owner);
                }else{
                    if(amVerbose())outputDecimal("No fractional digits in decimal '",decimal,"'.\n");
                    Mbiginteger* _num=owned_biginteger(__biginteger(),owner);
                    if(_num){
                        if(mp_read_radix(MP_INT_POINTER(_num),decimalText,10)==MP_OKAY)
							_rational=owned_rational(_getRational(_num,NULL,M_LD_NAN,false/*,false*/),owner);
                        FREE_BIGINTEGER(_num,owner); // MDH@26MAY2020: always now
                    }
                }
            }else // we can go through the text????
                _rational=owned_rational(_getDecimalTextRational(decimalText),owner);
            FREE_STRING(_decimalText,owner); // OOPS use FREE_STRING() not free()!
        }else
            outputError("Failed to convert the decimal to text");
    }
    return disowned_rational(_rational,owner);
}/* VALIDATED */

// MDH@08OCT2019: TODO do we have this already somewhere else??????
Mdecimal* _getBigintegerDecimal(Mbiginteger const * const biginteger){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _bigintegerDecimal=NULL;
	if(biginteger){
		Mstring* _bigintegerText=owned_string(_getBigintegerText(biginteger),owner);
		if(_bigintegerText){
			_bigintegerDecimal=owned_decimal(_getTextDecimal(string(_bigintegerText),0),owner);
			FREE_STRING(_bigintegerText,owner);
		}
	}
	return disowned_decimal(_bigintegerDecimal,owner);
}/* VALIDATED */

long double getDecimalLongDouble(Mdecimal* _decimal){
	// easiest way is to transform to text first, and take if from there...
	long double ldDecimal=M_LD_NAN;
	if(_decimal){
		// MDH@20MAY2020: TODO same here _decimalText escaping memory management
		char* _decimalText=mpd_to_sci(_decimal->mpd,0); // NOTE do NOT use _getDecimalText here!!!
		if(_decimalText){ldDecimal=_strtold(_decimalText,ldDecimal);free(_decimalText);} // no need for this anymore
	}
	return ldDecimal;
}/* VALIDATED */

// MDH@09OCT2019: TODO think we forgot to take the delta into account (if any)
Mdecimal* _getRationalDecimal(Mrational const * const _rational){Mallocationowner owner=getOwner(__LINE__);
    if(!_rational){outputError("No rational to convert to a decimal");return NULL;}
    // _decimalText is a local variable that when set should be freed before returning!!!
    Mstring* _decimalText=NULL;
    Mbiginteger *numerator=_rational->num,*denominator=_rational->den; // shortcut to the rational numerator and denominator
    uint64_t repeating=0;
    if(denominator){
        // local variables to be freed at the end (so NOT before)
		// MDH@15OCT2019: take the sign into account
		Mbiginteger *_nonnegativenumerator=(mp_isneg(MP_INT_POINTER(numerator))?owned_biginteger(_getBigintegerNeg(numerator),owner):numerator);
		if(_nonnegativenumerator){
			Mbiginteger *_digit=owned_biginteger(__biginteger(),owner),*_remainder=owned_biginteger(__biginteger(),owner);
			Mbiginteger* _bi10=owned_biginteger(_getBiginteger(10),owner);
			// MDH@20MAY2020 can't do this because there would be dangling owned pointers: if(!_bi10){outputError("Failed to create big integer 10");return NULL;}
			if(_digit&&_remainder&&_bi10&&mp_div(MP_INT_POINTER(_nonnegativenumerator),MP_INT_POINTER(denominator),MP_INT_POINTER(_digit),MP_INT_POINTER(_remainder))==MP_OKAY){
				// the integer part is _dividend
				_decimalText=owned_string(_getBigintegerText(_digit),owner);
				if(_decimalText&&!isBigintegerZero(_remainder)){ // we've got a fraction to add!!!
					Mstring* _p=string_append_char(_decimalText,'.'); // append the decimal period, storing the result in _p so we will know when that failed...
					// in order to find the repeating fraction we have to continue computing the remainders
					// and we have to register the remainders and compare the one we find with all remembered remainders, so far
					MbigintegerListelement *_firstRemainderListelement=NULL,*lastRemainderListelement=NULL;
					////////////////////////if(!_lastRemainderListelement){outputError("Failed to create the list to store the remainders");return NULL;}
					/* replacing, using an Mlist):
					Mlist* _remainderList=owned_list(_getListOfType(VT_BIGINTEGER),owner);
					Mlistelement* _remainderListelement=NULL;
					*/
					uint64_t remainderIndex,remainderCount=0; // where we found a match
					long long decimalsLeft=M_DECIMALCONTEXT->mpd_context->prec+2; // stop as soon as we have sufficient decimals
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
							if(mp_cmp(MP_INT_POINTER(_remainderListelement->_biginteger),MP_INT_POINTER(_remainder))==MP_EQ)break;
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
						_remainderListelement=(MbigintegerListelement*)CALLOC_1(sizeof(MbigintegerListelement),'b',owner); // NOTE re-use of _remainderListelement
						if(!_remainderListelement){outputError("Failed to create a big integer list element for storing the new remainder");_p=NULL;break;}

						// at this point we have a new remainder list element (in _remainderListelement) that should be bound or freed
						_remainderListelement->_biginteger=owned_biginteger(_getBigintegerCopy(_remainder),owner); // NOTE we have to copy _remainder as we will be computing with _remainder further (see below)
						if(!_remainderListelement->_biginteger){
							outputError("Failed to store the remainder");
							FREE_DISOWNED_1(_remainderListelement,'b',owner); // we have to free _remainderListelement here because it's not going to be remembered (and freed later on) in the list of remainders
							_p=NULL;
							break;
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
						if(mp_mul(MP_INT_POINTER(_remainder),MP_INT_POINTER(_bi10),MP_INT_POINTER(_remainder))!=MP_OKAY){
							outputError("Failed to multiply the remainder by 10");
							_p=NULL;
							break;
						}
						if(amVerboseDebugging()){outputBiginteger("Dividing '",_remainder,"'");outputBiginteger(" by '",denominator,"'.\n");}
						
						// _digit and _remainder are getting re-used here as well, which does not pose a problem (so we've created them once)
						if(mp_div(MP_INT_POINTER(_remainder),MP_INT_POINTER(denominator),MP_INT_POINTER(_digit),MP_INT_POINTER(_remainder))!=MP_OKAY){
							outputError("Failed to perform a long division to obtain the next decimal digit");
							_p=NULL;
							break;
						}
						if(amVerboseDebugging()){outputBiginteger("Digit: '",_digit,"'");outputBiginteger(" and remainder '",_remainder,"'.\n");}

						// append the dividend to the decimal text
						// NOTE that _digitText is freed as soon as possible
						_digitText=owned_string(_getBigintegerText(_digit),owner);
						if(!_digitText){
							outputError("Failed to store the next decimal character");
							_p=NULL;
							break;
						}
						_p=string_append(_p,string(_digitText));
						FREE_STRING(_digitText,owner); // MDH@11JUN2020: disowns it and frees it

						if(amVerboseDebugging())if(_p)output("Decimal text so far: '%s'.\n",string(_p));

						// if the remainder is zero (NOW stored in _remainderListelement->_biginteger instead of _remainder), we're done (it's a finite decimal fraction)
						if(isBigintegerZero(_remainderListelement->_biginteger)){
							if(amVerbose())outputInfo("Remainder is zero, so the decimal is finished.");
							break;
						}
					}
					if(_firstRemainderListelement)free_bigintegerListelement(_firstRemainderListelement,owner); // replacing: free_list(_remainderList);
					if(!_p){FREE_STRING(_decimalText,owner);_decimalText=NULL;} // some failure occurred
				}
			}
			// prepend the sign if the numerator is negative TODO what if this fails?????
			if(mp_isneg(MP_INT_POINTER(numerator))){
				if(_decimalText&&!string_insert_char(_decimalText,0,'-')){FREE_STRING(_decimalText,owner);_decimalText=NULL;}
				FREE_BIGINTEGER(_nonnegativenumerator,owner);
			}
			// free all locally used pointers to dynamic memory
			FREE_BIGINTEGER(_digit,owner);FREE_BIGINTEGER(_remainder,owner);FREE_BIGINTEGER(_bi10,owner);
		}
    }else
        _decimalText=owned_string(_getBigintegerText(numerator),owner);
    // parse _decimalText to a decimal
    Mdecimal* _decimal=NULL;
    if(_decimalText){
        _decimal=owned_decimal(_getTextDecimal(string(_decimalText),repeating),owner);
        if(!_decimal)
			output("%sFailed to parse decimal text '%s' of the corresponding rational",M_ERROR_PREFIX,string(_decimalText));
		else 
		if(amVerboseDebugging())
			outputDecimal("Decimal of rational: '",_decimal,"'.\n");
        FREE_STRING(_decimalText,owner);
    }
    return disowned_decimal(_decimal,owner);
}/* VALIDATED */

// decimalerrorstatus() filter out the rounding and inexact 'errors'
void report_mpd_status(uint32_t mpd_status){
	if(mpd_status>0){
		output("Decimal computations errors:");
		if(mpd_status&MPD_IEEE_Invalid_operation)output(" IEEE Invalid operation error");
		if(mpd_status&MPD_Clamped)output(" Clamped error");
		if(mpd_status&MPD_Division_by_zero)output(" Division by zero error");
		if(mpd_status&MPD_Fpu_error)output(" FPU error");
		if(mpd_status&MPD_Inexact)output(" Inexact error");
		if(mpd_status&MPD_Not_implemented)output(" Unknown error");
		if(mpd_status&MPD_Overflow)output(" Overflow error");
		if(mpd_status&MPD_Rounded)output(" Rounding error");
		if(mpd_status&MPD_Subnormal)output(" Subnormal error");
		if(mpd_status&MPD_Underflow)output(" Underflow error");
		output("\n");
	}else
		outputInfo("No decimal context status.");
}

bool mpd_error(mpd_context_t const * const mpd_context){return(mpd_getstatus(mpd_context)&0xEFBF)!=0;}

// BASE STUFF
// are we keeping a map of mpd contexts????
// trap handler (how to set it????)
/* replacing:
void Mdecimalraphandler(mpd_context_t* mpd_context){
}

size_t mpd_context_count=0;

const size_t MAXIMUM_NUMBER_OF_CONTEXTS=2; // quick fix to ascertain to use the same context over and over again

mpd_context_t** mpd_contexts=NULL; // keep track of all decimal contexts
//
// * \brief returns the multiple-precision decimal context with the \p decimalprecision requested
// * \param decimalprecision the number of decimal digits a decimal should (minimally) hold
// * \return NULL on failing to obtain such a decimal context, otherwise that decimal context
// * if the number of stored decimal context equals the maximum number of contexts, the last context will be reused
// * if creating a new context succeeded it will be returned even when failing to remember the decimal context
//
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
                    output("%sFailed to return a decimal context with precision %u.\n",M_ERROR_PREFIX,decimalprecision);
            }else
                outputError("Failed to create a new decimal context");
        }
        ////Mdecimalraphandler=MMdecimalraphandler;
    }
    // reset the status, so we can use the context as if it were new
    if(mpd_context)mpd_qsetstatus(mpd_context,0); // using the setter is preferred over ->status=0 assignment
    return mpd_context;
}// VALIDATED */

// MDH@25AUG2019: now requiring mpd_context to not be NULL
// MDH@17JUN2019: convenient to expose _get_mpd (e.g. for use in approximating pi with a decimal)
/**
 * \brief returns an mpd_t instance (from the mpdecimal libray) with the precision given by \p mpd_context equal to \p value
 * \param value the (initial) value of the returned mpd_t instance
 * \return on success the mpd_t instance equal to \p value, NULL otherwise
 */
mpd_t* __mpd(mpd_context_t const * mpd_context,int64_t value){ // MDH@20MAY2020: TODO mpd_qnew NOT under memory allocation management!!!
    mpd_t* _mpd=NULL;
	if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context;
    // using mpd_qnew over mpd_new because we want to return NULL on failure!!!
    if(mpd_context){
        _mpd=mpd_qnew();
        // NOTE it is essential to initialize the stored value even when 0 as we would otherwise get errors on mpd_to_sci calls
        if(_mpd){
            mpd_set_i64(_mpd,value,mpd_context);
            if(_mpd->len==0){
                output("%sFailed to create decimal with value " PRId64 ".\n",M_ERROR_PREFIX,value);
                free_mpd(_mpd);
                _mpd=NULL;
            }
        }else
            outputError("Failed to create a decimal data instance");
    }else
        outputError("No context to store the decimal data in");
     /////////outputDecimal("Decimal '",(Mdecimal*)_mpd,"' created!");
    return _mpd;
}/* VALIDATED */
/**
 * \brief frees \p mpd, calling mpd_del()
 * \param mpd the mpdecimal instance to free
 */
void free_mpd(mpd_t* _mpd){
	if(_mpd)mpd_del(_mpd);
}/* VALIDATED */

Mdecimal* disowned_decimal(Mdecimal* _decimal,Mallocationowner owner_decimal){return(Mdecimal*)DISOWNED(_decimal,owner_decimal);}
Mdecimal* owned_decimal(Mdecimal* _decimal,Mallocationowner owner_decimal){return(Mdecimal*)OWNED(_decimal,owner_decimal);}

/**
 * \brief frees \p decimal, delegating to free_mpd() for freeing the contained mpdecimal instance
 */
void free_decimal(Mdecimal* _decimal/*,Mallocationowner owner_decimal*/){
	if(!_decimal)return;
    if(amVerboseDebugging())output("Freeing decimal.\n");
    if(_decimal->mpd){free_mpd(_decimal->mpd);_decimal->mpd=NULL;}//////else if(verbose)outputError("No data in decimal to free");
    FREE_1(_decimal,'D'/*,owner_decimal*/);
}/* VALIDATED */

/**
 * \brief returns an uninitialized but cleared decimal (i.e. without an initialized mpd pointer)
 */
Mdecimal* __adecimal(){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _adecimal=(Mdecimal*)CALLOC_1(sizeof(Mdecimal),'D',owner);
	// outputDecimal("Created decimal: '",_adecimal,"'."); // DEBUG
	return disowned_decimal(_adecimal,owner);
} /* VALIDATED */

/**
 * \brief returns a decimal initialized to \p value with the precision specified by \p mpd_context and number of repeating digits equal to \p repeating
 * \param mpd_context the decimal context to use
 * \param value the initial (integer) value of the decimal
 * \param repeating the number of repeating decimal digits at the end
 */
Mdecimal* __decimal(mpd_context_t const * mpd_context,int64_t value,uint64_t repeating){Mallocationowner owner=getOwner(__LINE__);
    Mdecimal* _decimal=NULL;
    if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context; // use the application-wide decimal context if no context is defined
    if(mpd_context){
        _decimal=owned_decimal(__adecimal(),owner); // get an uninitialized decimal
        if(_decimal){
            _decimal->mpd=__mpd(mpd_context,value); // initialize to zero by default
            if(_decimal->mpd){
                _decimal->repeating=repeating;
                _decimal->prec=mpd_context->prec;
            }else{
                FREE_DISOWNED_1(_decimal,'D',owner);
                _decimal=NULL;
            }
        }else
            outputError("Failed to create a decimal"); // TODO make an out of memory error out of this
    }else
        outputError("No context to create decimal in");
    return disowned_decimal(_decimal,owner);
}/* VALIDATED */

// MDH@29AUG2019: as mpd_t does not itself store the precision with which the decimal was created AND an Mdecimal* does store the precision I've added the prec parameter
/**
 * \brief returns a decimal with mpdecimal instance with the default decimal context equal to \p mpd and number of repeating digits equal to \p repeating, freeing the _mpd on failure
 */
Mdecimal* _getDecimal(mpd_t const * const _mpd,mpd_ssize_t prec,uint64_t repeating,bool freeonfailure){Mallocationowner owner=getOwner(__LINE__);
    if(!_mpd)return NULL; // can do this as won't have to free mpd anyway
    Mdecimal* _decimal=owned_decimal(__adecimal(),owner); // always using the default decimal context
    if(_decimal){
		////////output("Wrapping decimal with precision %u.\n",prec);
		_decimal->mpd=_mpd;_decimal->prec=prec;_decimal->repeating=repeating;
		////////output("Decimal with precision %u wrapped.\n",prec);
	}else
	if(freeonfailure)free_mpd(_mpd);
    return disowned_decimal(_decimal,owner);
}/* VALIDATED */

/**
 * \brief adds two pure decimals
 */
Mdecimal* _dadd(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		// the decimal with the highest decimal context determines the context to use
		// TODO should this double the precision???????
		mpd_ssize_t precision=MAX(d1->prec,d2->prec);
		Mdecimalcontext* decimalcontext=(precision>=6?_getDecimalcontext(precision):M_DECIMALCONTEXT);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:NULL);
		if(mpd_context){
			// compute the sum
			_decimal=owned_decimal(__decimal(mpd_context,0,false),owner);
			if(!_decimal)return NULL;
			uint32_t status=0;mpd_qadd(_decimal->mpd,d1->mpd,d2->mpd,mpd_context,&status);
			// on failure free the decimal
			if((status&0xEFBF)!=0){FREE_DECIMAL(_decimal,owner);_decimal=NULL;outputError("Failed to compute the sum of two decimals");}
		}
	}
	return disowned_decimal(_decimal,owner);
}
//MDH@19SEP2019: if we add two decimals we need to take the repeating digits into account (which we didn't do so far)
//               which will make it a little harder to compute the decimal sum
//               how about returning to the associated rational multiply and convert back to a decimal???????
Mdecimal* _getDecimalSum(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		if(d1->repeating+d2->repeating>0){ // not both pure decimals
			Mrational *_r1=owned_rational(_getDecimalRational(d1),owner),*_r2=owned_rational(_getDecimalRational(d2),owner);
			if(_r1&&_r2){
				Mrational* _r=owned_rational(_getRationalSum(_r1,_r2),owner); // compute the sum of two rationals
				if(_r){
					_decimal=owned_decimal(_getRationalDecimal(_r),owner);
					FREE_RATIONAL(_r,owner);
				}else
					outputError("Failed to compute the sum of two rational decimals");
			}else
				outputError("Failed to convert a decimal to a rational");
			FREE_RATIONAL(_r1,owner);
			FREE_RATIONAL(_r2,owner);
		}else // pure decimals
			_decimal=owned_decimal(_dadd(d1,d2),owner); // just add
	}
	return disowned_decimal(_decimal,owner);
}

/**
 * \brief subtracts two pure decimals
 */
Mdecimal* _dsub(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		// the decimal with the highest decimal context determines the context to use
		// TODO should this double the precision???????
		mpd_ssize_t precision=MAX(d1->prec,d2->prec);
		Mdecimalcontext* decimalcontext=(precision>=6?_getDecimalcontext(precision):M_DECIMALCONTEXT);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:NULL);
		if(mpd_context){
			// compute the sum
			_decimal=owned_decimal(__decimal(mpd_context,0,false),owner);
			if(!_decimal)return NULL;
			uint32_t status=0;mpd_qsub(_decimal->mpd,d1->mpd,d2->mpd,mpd_context,&status);
			// on failure free the decimal
			if((status&0xEFBF)!=0){FREE_DECIMAL(_decimal,owner);outputError("Failed to compute the difference of two decimals");return NULL;}
		}
	}
	return disowned_decimal(_decimal,owner);
}
//MDH@19SEP2019: if we add two decimals we need to take the repeating digits into account (which we didn't do so far)
//               which will make it a little harder to compute the decimal sum
//               how about returning to the associated rational multiply and convert back to a decimal???????
Mdecimal* _getDecimalDifference(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		if(d1->repeating+d2->repeating>0){ // not both pure decimals
			Mrational *_r1=owned_rational(_getDecimalRational(d1),owner),*_r2=owned_rational(_getDecimalRational(d2),owner);
			if(_r1&&_r2){
				Mrational* _r=owned_rational(_getRationalDifference(_r1,_r2),owner); // compute the product of two rationals
				if(_r){
					_decimal=owned_decimal(_getRationalDecimal(_r),owner);
					FREE_RATIONAL(_r,owner);
				}else
					outputError("Failed to compute the difference of two rationalized decimals");
			}else
				outputError("Failed to convert a decimal to a rational");
			FREE_RATIONAL(_r1,owner);
			FREE_RATIONAL(_r2,owner);
		}else // pure decimals
			_decimal=owned_decimal(_dsub(d1,d2),owner); // just subtract
	}
	return disowned_decimal(_decimal,owner);
}

/**
 * \brief computes the quotient of two pure decimals
 */
Mdecimal* _ddiv(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		// the decimal with the highest decimal context determines the context to use
		// TODO should this double the precision???????
		mpd_ssize_t precision=MAX(d1->prec,d2->prec);
		Mdecimalcontext* decimalcontext=(precision>=6?_getDecimalcontext(precision):M_DECIMALCONTEXT);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:NULL);
		if(mpd_context){
			// compute the quotient
			_decimal=owned_decimal(__decimal(mpd_context,0,false),owner);
			if(!_decimal)return NULL;
			uint32_t status=0;mpd_qdiv(_decimal->mpd,d1->mpd,d2->mpd,mpd_context,&status);
			// on failure free the decimal
			if((status&0xEFBF)!=0){
				FREE_DECIMAL(_decimal,owner);
				outputError("Failed to compute the quotient of two decimals");
				return NULL;
			}
		}
	}
	return disowned_decimal(_decimal,owner);
}
/**
 * \brief computes the quotient of any two decimals (including repeating ones)
 */
Mdecimal* _getDecimalQuotient(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		if(d1->repeating+d2->repeating>0){ // not both pure decimals
			Mrational *_r1=owned_rational(_getDecimalRational(d1),owner),*_r2=owned_rational(_getDecimalRational(d2),owner);
			if(_r1&&_r2){
				Mrational* _r=owned_rational(_getRationalQuotient(_r1,_r2),owner); // compute the quotient of two rationals
				if(_r){
					_decimal=owned_decimal(_getRationalDecimal(_r),owner);
					FREE_RATIONAL(_r,owner);
				}else
					outputError("Failed to compute the quotient of two rationals");
			}else
				outputError("Failed to convert a decimal to a rational");
			FREE_RATIONAL(_r1,owner);
			FREE_RATIONAL(_r2,owner);
		}else // pure decimals
			_decimal=owned_decimal(_ddiv(d1,d2),owner); // just multiply
	}
	return disowned_decimal(_decimal,owner);
}

/**
 * \brief computes the product of two pure decimals
 */
Mdecimal* _dmul(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		// the decimal with the highest decimal context determines the context to use
		// TODO should this double the precision???????
		mpd_ssize_t precision=MAX(d1->prec,d2->prec);
		Mdecimalcontext* decimalcontext=(precision>=6?_getDecimalcontext(precision):M_DECIMALCONTEXT);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:NULL);
		if(mpd_context){
			// compute the quotient
			_decimal=owned_decimal(__decimal(mpd_context,0,false),owner);
			if(!_decimal)return NULL;
			uint32_t status=0;mpd_qmul(_decimal->mpd,d1->mpd,d2->mpd,mpd_context,&status);
			// on failure free the decimal
			if((status&0xEFBF)!=0){
				FREE_DECIMAL(_decimal,owner);
				outputError("Failed to compute the quotient of two decimals");
				return NULL;
			}
		}
	}
	return disowned_decimal(_decimal,owner);
}
//MDH@19SEP2019: if we multiply two decimals we need to take the repeating digits into account (which we didn't do so far)
//               which will make it a little harder to compute the decimal product
//               how about returning to the associated rational multiply and convert back to a decimal???????
Mdecimal* _getDecimalProduct(Mdecimal const * const d1,Mdecimal const * const d2){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(d1&&d2){
		if(d1->repeating+d2->repeating>0){ // not both pure decimals
			Mrational *_r1=owned_rational(_getDecimalRational(d1),owner),*_r2=owned_rational(_getDecimalRational(d2),owner);
			if(_r1&&_r2){
				Mrational* _r=owned_rational(_getRationalProduct(_r1,_r2),owner); // compute the product of two rationals
				if(_r){
					_decimal=owned_decimal(_getRationalDecimal(_r),owner);
					FREE_RATIONAL(_r,owner);
				}else
					outputError("Failed to compute the product of two rational decimals");
			}else
				outputError("Failed to convert a decimal to a rational");
			FREE_RATIONAL(_r1,owner);
			FREE_RATIONAL(_r2,owner);
		}else // pure decimals
			_decimal=owned_decimal(_dmul(d1,d2),owner); // just multiply
	}
	return disowned_decimal(_decimal,owner);
}

/**
 * \brief returns a decimal parsed from \p decimalText using the default decimal context and repeating number of digits \p repeating
 */
Mdecimal* _getTextDecimal(char const * const decimalText,uint64_t repeating){Mallocationowner owner=getOwner(__LINE__);
    Mdecimal* _textDecimal=NULL;
    if(decimalText&&strlen(decimalText)){
        if(amVerbose())output("Parsing decimal text '%s'.\n",decimalText);
        // can we find a repeating fraction????? this would be the case if behind the period we'd have xxxx<yyy><yyy><yyy>
        // the rounding at the end of course could prove to be problematic
        _textDecimal=owned_decimal(__decimal(NULL,0,repeating),owner);
        if(_textDecimal){
			uint32_t status=0;
            mpd_qset_string(_textDecimal->mpd,decimalText,M_DECIMALCONTEXT->mpd_context,&status); // NOTE here we have to pass in the default decimal context
            if((status&0xEFBF)!=0){
				FREE_DECIMAL(_textDecimal,owner);
				output("%sFailed to parse a decimal (error status: %" PRIu32 ").\n",M_ERROR_PREFIX,status);
				return NULL;
			} // if we failed to get a mpdecimal instance from the text, the text is probably wrong!!!
        }else
			outputError("Failed to create a decimal");
        ////////////if(!_textDecimal)output("%sFailed to create a decimal from '%s'.\n",M_ERROR_PREFIX,decimalText);
    }else
        outputError("No decimal text to parse");
    return disowned_decimal(_textDecimal,owner);
}/* VALIDATED */

// END BASE STUFF

// MDH@09NOV2019: one can decide to compute the predefined sines table or not (when only pi is required)
Mdecimal* pi_decimal(Mdecimalcontext* decimalcontext,bool computesinetable){Mallocationowner owner=getOwner(__LINE__);

	// if decimalContext equals NULL use the global decimal context, in _decimalContext
	if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;

	mpd_context_t* mpd_context=decimalcontext->mpd_context;

	mpd_ssize_t decimalprecision=mpd_context->prec;
	if(decimalprecision<=0){outputError("Cannot approximate pi: no decimal context available");return NULL;}

	Mdecimal* _decimal=NULL; // the result

	// if pi already exists in the given decimal context return it, but we need to wrap a copy in the decimal
	if(!decimalcontext->pi){

		//if(amVerbose())
		output("Computing %lld digits of pi.\n",decimalprecision); // MDH@17JUN2020: this includes the digit in front of the period (i.e. 3)

		// initialize the variables we need for the iterations
#ifdef __ADEBUG__
		Mdecimal *lasts=owned_decimal(__decimal(mpd_context,0,0),owner),*t=owned_decimal(__decimal(mpd_context,3,0),owner),*s=owned_decimal(__decimal(mpd_context,3,0),owner);
		Mdecimal *n=owned_decimal(__decimal(mpd_context,1,0),owner),*na=owned_decimal(__decimal(mpd_context,0,0),owner),*d=owned_decimal(__decimal(mpd_context,0,0),owner);
		Mdecimal *da=owned_decimal(__decimal(mpd_context,24,0),owner);
		// some constant decimals we need
		Mdecimal *d8=owned_decimal(__decimal(mpd_context,8,0),owner),*d32=owned_decimal(__decimal(mpd_context,32,0),owner);
#else
		mpd_t *lasts=__mpd(mpd_context,0),*t=__mpd(mpd_context,3),*s=__mpd(mpd_context,3),*n=__mpd(mpd_context,1),*na=__mpd(mpd_context,0),*d=__mpd(mpd_context,0),*da=__mpd(mpd_context,24);
		// some constant decimals we need
		mpd_t *d8=__mpd(mpd_context,8),*d32=__mpd(mpd_context,32);
#endif
		clock_t now=0,then=(!amVerbose()?clock():-1); // if not running verbose, show number of iterations executed per second
		// DONE:TODO don't do this because you do need to free whatever was allocated!!!
		if(lasts&&t&&s&&n&&na&&d&&da&&d8&&d32){
			if(amVerbose())outputInfo("\tInitial helper decimals created!");
			unsigned long long iter=0;
			if(amVerbose()){
				output("\tIteration %u:",iter);
#ifdef __ADEBUG__
				char* _lasts=mpd_to_sci(lasts->mpd,0);output(" lasts=%s");free(_lasts);
				char* _t=mpd_to_sci(t->mpd,0);output(" t=%s",_t);free(_t);
				char* _s=mpd_to_sci(s->mpd,0);output(" s=%s",_s);free(_s);
				char* _n=mpd_to_sci(n->mpd,0);output(" n=%s",_n);free(_n);
				char* _na=mpd_to_sci(na->mpd,0);output(" na=%s",_na);free(_na);
				char* _d=mpd_to_sci(d->mpd,0);output(" d=%s",_d);free(_d);
				char* _da=mpd_to_sci(da->mpd,0);output(" da=%s",_da);free(_da);
#else
				char* _lasts=mpd_to_sci(lasts,0);output(" lasts=%s");free(_lasts);
				char* _t=mpd_to_sci(t,0);output(" t=%s",_t);free(_t);
				char* _s=mpd_to_sci(s,0);output(" s=%s",_s);free(_s);
				char* _n=mpd_to_sci(n,0);output(" n=%s",_n);free(_n);
				char* _na=mpd_to_sci(na,0);output(" na=%s",_na);free(_na);
				char* _d=mpd_to_sci(d,0);output(" d=%s",_d);free(_d);
				char* _da=mpd_to_sci(da,0);output(" da=%s",_da);free(_da);
#endif
				newline();
			}
			long long iterthen=iter,seconds=0; // report every second
			int cmp;
			mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
			outputInfo("In the approximation two additional decimal digits will be computed after which rounding will be applied cutting off the two extra decimals.");
			if(then>=0)output("Iterations per second (abort by pressing any key):");
			bool interrupted=false;
			while(!mpd_error(mpd_context)){
				///// doesn't work I think!!! if(kbhit()!=0)break;
				iter++;
				//if(!amVerbose()){
				if(then>=0){
					now=clock();
					if(now>=0){
						if(now-then>=1000000){
							then=now;
							seconds++;
							output(" %lld",iter-iterthen);
							if(kbhit()>0){interrupted=true;newline();outputError("Not all decimals of pi reported guaranteed to be correct, as its computation was interrupted by the user!");break;} // MDH@11NOV2019: allow breaking
							iterthen=iter;
						}/*else outputChar('.');*/
					}
				}
#ifdef __ADEBUG__
				//if(amVerbose())output("Iteration: %lld: ",iter);
				cmp=mpd_cmp(lasts->mpd,s->mpd,mpd_context); // lasts == s ?
				//if(amVerbose()){output("a\t");if(mpd_error(mpd_context))break;}
				if(!cmp){if(amVerbose())output("Done!\n");break;}
				//if(amVerbose()){output("b\t");if(mpd_error(mpd_context))break;}
				if(cmp==INT_MAX){output("Something went wrong!\n");break;}
				//if(amVerbose()){output("c\t");if(mpd_error(mpd_context))break;output("lasts = (s) = %s",Mdecimalo_sci(s,0));}
				mpd_copy(lasts->mpd,s->mpd,mpd_context); // lasts = s
				//if(amVerbose()){output("d\t",Mdecimalo_sci(lasts,0));if(mpd_error(mpd_context))break;output("n = (n=%s) + (na=)%s",Mdecimalo_sci(n,0),Mdecimalo_sci(na,0));}
				mpd_add(n->mpd,n->mpd,na->mpd,mpd_context);
				//if(amVerbose()){output(" = %s\ne\t",Mdecimalo_sci(n,0));if(mpd_error(mpd_context))break;output("na = (na=%s) + 8",Mdecimalo_sci(na,0));}
				mpd_add(na->mpd,na->mpd,d8->mpd,mpd_context); // increment n by na and na by 8
				//if(amVerbose()){output(" = %s\nf\t",Mdecimalo_sci(na,0));if(mpd_error(mpd_context))break;output("d = (d=%s) + (da=%s)",Mdecimalo_sci(d,0),Mdecimalo_sci(da,0));}
				mpd_add(d->mpd,d->mpd,da->mpd,mpd_context);
				//if(amVerbose()){output(" = %s\ng\t",Mdecimalo_sci(d,0));if(mpd_error(mpd_context))break;output("da = (da=%s) + 32",Mdecimalo_sci(da,0));}
				mpd_add(da->mpd,da->mpd,d32->mpd,mpd_context); // increment d by da and da by 32
				//if(amVerbose()){output(" = %s\nh\t",Mdecimalo_sci(da,0));if(mpd_error(mpd_context))break;output("t = (t=%s) * (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(n,0));}
				mpd_mul(t->mpd,t->mpd,n->mpd,mpd_context);
				//if(amVerbose()){output(" = %s\ni\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("t = (t=%s) / (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(d,0));}
				mpd_div(t->mpd,t->mpd,d->mpd,mpd_context); // multiply t by n and divide t by d
				//if(amVerbose()){output(" = %s\nj\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("s = (s=%s) + (t=%s)",Mdecimalo_sci(s,0),Mdecimalo_sci(t,0));}
				mpd_add(s->mpd,s->mpd,t->mpd,mpd_context); // add t to s
				//if(amVerbose()){output(" = %s\nk\t",Mdecimalo_sci(s,0));if(mpd_error(mpd_context))break;}
				if(amVerbose()){
					output("Iteration %u:",iter);
					char* _lasts=mpd_to_sci(lasts->mpd,0);output(" lasts=%s");free(_lasts);
					char* _t=mpd_to_sci(t->mpd,0);output(" t=%s",_t);free(_t);
					char* _s=mpd_to_sci(s->mpd,0);output(" s=%s",_s);free(_s);
					char* _n=mpd_to_sci(n->mpd,0);output(" n=%s",_n);free(_n);
					char* _na=mpd_to_sci(na->mpd,0);output(" na=%s",_na);free(_na);
					char* _d=mpd_to_sci(d->mpd,0);output(" d=%s",_d);free(_d);
					char* _da=mpd_to_sci(da->mpd,0);output(" da=%s",_da);free(_da);
					outputChar('\n');
				}
#else
				//if(amVerbose())output("Iteration: %lld: ",iter);
				cmp=mpd_cmp(lasts,s,mpd_context); // lasts == s ?
				//if(amVerbose()){output("a\t");if(mpd_error(mpd_context))break;}
				if(!cmp){if(amVerbose())output("\tDone!\n");break;}
				//if(amVerbose()){output("b\t");if(mpd_error(mpd_context))break;}
				if(cmp==INT_MAX){outputError("Something went wrong!");break;}
				//if(amVerbose()){output("c\t");if(mpd_error(mpd_context))break;output("lasts = (s) = %s",Mdecimalo_sci(s,0));}
				mpd_copy(lasts,s,mpd_context); // lasts = s
				//if(amVerbose()){output("d\t",Mdecimalo_sci(lasts,0));if(mpd_error(mpd_context))break;output("n = (n=%s) + (na=)%s",Mdecimalo_sci(n,0),Mdecimalo_sci(na,0));}
				mpd_add(n,n,na,mpd_context);
				//if(amVerbose()){output(" = %s\ne\t",Mdecimalo_sci(n,0));if(mpd_error(mpd_context))break;output("na = (na=%s) + 8",Mdecimalo_sci(na,0));}
				mpd_add(na,na,d8,mpd_context); // increment n by na and na by 8
				//if(amVerbose()){output(" = %s\nf\t",Mdecimalo_sci(na,0));if(mpd_error(mpd_context))break;output("d = (d=%s) + (da=%s)",Mdecimalo_sci(d,0),Mdecimalo_sci(da,0));}
				mpd_add(d,d,da,mpd_context);
				//if(amVerbose()){output(" = %s\ng\t",Mdecimalo_sci(d,0));if(mpd_error(mpd_context))break;output("da = (da=%s) + 32",Mdecimalo_sci(da,0));}
				mpd_add(da,da,d32,mpd_context); // increment d by da and da by 32
				//if(amVerbose()){output(" = %s\nh\t",Mdecimalo_sci(da,0));if(mpd_error(mpd_context))break;output("t = (t=%s) * (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(n,0));}
				mpd_mul(t,t,n,mpd_context);
				//if(amVerbose()){output(" = %s\ni\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("t = (t=%s) / (n=%s)",Mdecimalo_sci(t,0),Mdecimalo_sci(d,0));}
				mpd_div(t,t,d,mpd_context); // multiply t by n and divide t by d
				//if(amVerbose()){output(" = %s\nj\t",Mdecimalo_sci(t,0));if(mpd_error(mpd_context))break;output("s = (s=%s) + (t=%s)",Mdecimalo_sci(s,0),Mdecimalo_sci(t,0));}
				mpd_add(s,s,t,mpd_context); // add t to s
				//if(amVerbose()){output(" = %s\nk\t",Mdecimalo_sci(s,0));if(mpd_error(mpd_context))break;}
				if(amVerbose()){
					if(amDebugging()){
						output("Iteration %u:",iter);
						//char* _lasts=mpd_to_sci(lasts,0);if(_lasts){output(" lasts=%s");free(_lasts);}else output(" ?");
						char* _t=mpd_to_sci(t,0);if(_t){output(" t=%s",_t);free(_t);}else output(" ?");
						char* _s=mpd_to_sci(s,0);if(_s){output(" s=%s",_s);free(_s);}else output(" ?");
						char* _n=mpd_to_sci(n,0);if(_n){output(" n=%s",_n);free(_n);}else output(" ?");
						char* _na=mpd_to_sci(na,0);if(_na){output(" na=%s",_na);free(_na);}else output(" ?");
						char* _d=mpd_to_sci(d,0);if(_d){output(" d=%s",_d);free(_d);}else output(" ?");
						char* _da=mpd_to_sci(da,0);if(_da){output(" da=%s",_da);free(_da);}else output(" ?");
						outputChar('\n');				
					}else{
						if((iter%100)==0)output("Iteration %u.\n",iter);
					}
				}
#endif
			}
			mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2
			uint32_t status=0;
			mpd_qfinalize(s,mpd_context,&status);
			if((status&0xEFBF)!=0){
				outputError("Failed to finalize pi");
			}else
			if(!interrupted){
				// if(then>=0)if(!interrupted){outputChar('.');newline();}
				//output("Number of iterations to compute pi to %lld decimals: %lld.\n",mpd_context->prec,iter);
				if(mpd_context){
					// store pi, pi/2 and pi/4 in the decimal context (all or none) BEFORE readjusting the precision i.e. if no error occurred
					if(!mpd_error(mpd_context)){
						mpd_t *_pi=get_mpd_copy(mpd_context,s);
						if(_pi){
							// MDH@05AUG2019: it's relatively simple to compute the sin/cosine of angles like 15, 30, 45, 60, 75 so we can start with storing multiples of pi/12 which of course include all we need!!!
							//                how about storing the sine and cosines of these values along with these predefined angles?????
							mpd_t *_pidiv2=__mpd(mpd_context,0),*_pidiv4=__mpd(mpd_context,0),*_pimul2=__mpd(mpd_context,0);
							if(_pidiv2&&_pidiv4&&_pimul2){
								mpd_qdiv_u32(_pidiv2,_pi,2,mpd_context,&status);
								mpd_qdiv_u32(_pidiv4,_pi,4,mpd_context,&status);
								mpd_qadd(_pimul2,_pi,_pi,mpd_context,&status); // NOTE better to simply double pi by adding it to itself???????
							}else // _pi not bound in decimalcontext, so free
								status=0xFFFFFFFF;
							if((status&0xEFBF)==0){
								decimalcontext->pi=_pi;
								decimalcontext->pidiv2=_pidiv2;
								decimalcontext->pidiv4=_pidiv4;
								decimalcontext->pimul2=_pimul2;
							}else{
								free_mpd(_pimul2);
								free_mpd(_pi);
								free_mpd(_pidiv2);
								free_mpd(_pidiv4);
							}
						}
					}
				}
			}
			// MDH@11NOV2019: if we actually failed to store pi (when it was computed uninterrupted to start with!)
			if(!decimalcontext->pi)
				outputError("Failed to store the decimal approximation of pi in the decimal context");
			else
				outputInfo("The decimal approximation of pi was stored in the decimal context.");
			// TODO should I mpd_finalize the pi values stored? or for now leave them unrounded?????????
		}else
			outputError("Failed to create all helper decimals");

		// get rid of all the decimals we used
#ifdef __ADEBUG__
		free_decimal(lasts,owner);free_decimal(t,owner);free_decimal(n,owner);free_decimal(na,owner);free_decimal(d,owner);free_decimal(da,owner);
		free_decimal(d8,owner);free_decimal(d32,owner);
#else
		free_mpd(lasts);free_mpd(t);free_mpd(n);free_mpd(na);free_mpd(d);free_mpd(da);
		free_mpd(d8);free_mpd(d32);
#endif
		if(mpd_context){
			if(mpd_error(mpd_context)){ // something went wrong
				if(amVerbose())output("%sFailed to compute pi with precision %lld (see next line for details).\n",decimalprecision);
				report_mpd_status(mpd_getstatus(mpd_context));
				return NULL;
			}
		}
		// success
/* MDH@05AUG2019: if we turn finalization off, the first pi returned is the same as the pi returned as stored with the decimal context, so we will get the same result, which is preferred!!!)
#ifdef __ADEBUG__
		mpd_finalize(s->mpd,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#else
		mpd_finalize(s,mpd_context); // to round to the requested precision
#endif
*/
		if(amVerbose()){
#ifdef __ADEBUG__
			char* _s=mpd_to_sci(s->mpd,0);
#else
			char* _s=mpd_to_sci(s,0);
#endif
			if(_s){output("Final approximation of pi ( NOT rounded to %llu decimals but with two additional decimals): %s.\n",decimalprecision,_s);free(_s);}
		}

		_decimal=owned_decimal(_getDecimal(s,decimalprecision,0,true),owner);

		output("Number of milliseconds passed in total (for computing and storing pi): %lld.\n",(clock()-then)/1000);

	}else{ // decimalcontext->pi exists

		if(amVerbose())outputInfo("NOTE: Returning the decimal approximation of pi stored in the decimal context.");
		// a copy to return
		_decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,decimalcontext->pi),decimalprecision,0,true),owner);

	}
	
	// if we have pi stored in the decimal context and computing the predefined sine table was requested, do so
	if(decimalcontext->pi){
		if(computesinetable){
			clock_t then=clock();
			uint32_t status=0;
			mpd_t *_pi=decimalcontext->pi,*_pidiv2=decimalcontext->pidiv2,*_pidiv4=decimalcontext->pidiv4,*_pimul2=decimalcontext->pimul2;
			// MDH@11SEP2019: so far I've used 15, 30, 45, 60, 75 and 90 as relative angles of which the sine/cosine is known or exactly computable (although depending on the sqrt decimal function)
			//                but we used halving of the angle and the formulas for that to compute the CORDIC angles
			//                NOTE that doubling the angle does not require taking square roots
			//                what I want to do is precompute a number of equidistant (co)sines, if we start at a certain granularity
			//                for the sine 30 degrees has a unique sine (1/2), for the cosine that's 60 degrees, obviously we can compute the cosines from the sine and back
			//                halving these 'exact' sine angles will require square rooting a number of times unless we store the squares?????
			//                some analysis told me that if we keep halving x times we end up with x constituent binary angles that can be binary encoded as: 1000000, 01000000, 00100000, 00010000, ..., 00000001
			//                for any multiple of the smallest angle you'd get x bits and you will need to perform y-1 rotations using the original known sines where y is the number of set bits
			//                this means you can store a fixed number of sines easily in a table with 2^x entries, so we get mpd_t[257] predefinedsines for storing all sines we'll be needing from 0 degrees in predefinedsines[0]
			//                and sine of pi/2 in predefinedsines[256], any angle you get divide by 256 to get the entry to use in the predefined table!!!
			// ok, we're going to store some predefined sine/cosines
			// in particular all multiples of pi/12 below pi, NOTE that the sine/cosine of 0 does not need to be stored as that can't help us speed up sine/cosine computation
			// we start with pi/12 and then up to 6*pi/12, so we'd have in total 6 predefined sine/cosines

			mpd_t *_pidiv12=get_mpd_copy(mpd_context,_pi),*_sqrt2div2=__mpd(mpd_context,2),*_sqrt3div2=__mpd(mpd_context,3);
			// some values only need to be computed once but are used twice in the table, by precomputing them a reference is stored in the table so we won't have to free them
			// we can use _sin90 when we need 1 in computations
			Mdecimal* _intermediateResult=owned_decimal(amVerbose()?__decimal(mpd_context,0,0):NULL,owner);
			
			// MDH@11SEP2019: computing the predefined sines means starting with the sine of 90 degrees 
			mpd_t *_0=__mpd(mpd_context,0),*_1=__mpd(mpd_context,1),*_predefined=__mpd(mpd_context,0),*_rotationsine=__mpd(mpd_context,0),*_rotationcosine=__mpd(mpd_context,0),*_t1=__mpd(mpd_context,0),*_t2=__mpd(mpd_context,0),*_t3=__mpd(mpd_context,0),*_t4=__mpd(mpd_context,0);
			decimalcontext->predefinedsinedeltaangle=__mpd(mpd_context,0);
			if(decimalcontext->predefinedsinedeltaangle&&_0&&_1&&_predefined&&_rotationsine&&_rotationcosine&&_t1&&_t2&&_t3&&_t4){
				if(_intermediateResult)outputInfo("Precomputing 257 equidistant sines in [0,pi/2].");
				for(int index=256;index>=0;index--)decimalcontext->predefinedsines[index]=__mpd(mpd_context,0); // create all 257 predefined sines instances...
				// initialize the first and last sine
				mpd_qcopy(decimalcontext->predefinedsines[256],_1,&status);mpd_qcopy(decimalcontext->predefinedsines[0],_0,&status);
				if(_intermediateResult)outputInfo("Predefined sine of 0 and pi/2 radians set.");
				// now we can half index and use the predefined sine and cosine to compute the new sine and cosine
				int16_t index=256;
				// store pi/512 or (pi/2)/256 in the predefinedsinedeltaangle!!!
				mpd_qdiv_u32(decimalcontext->predefinedsinedeltaangle,decimalcontext->pidiv2,256,mpd_context,&status);
				while(1){
					// copy the cosine of the predefined angle at position index
					mpd_qcopy(_predefined,decimalcontext->predefinedsines[256-index],&status);
					// subtract 1 and switch the sign from negative to positive
					mpd_qsub_i32(_predefined,_predefined,1,mpd_context,&status);mpd_set_positive(_predefined);
					// divide by 2
					mpd_qdiv_i32(_predefined,_predefined,2,mpd_context,&status);
					// _predefined now equals the square of the sine we want to store
					index>>=1; // half the index
					if(!index)break;
					mpd_qsqrt(decimalcontext->predefinedsines[index],_predefined,mpd_context,&status); // take the square root of the square of the sine (thus the sine) and store it in the table
					if(_intermediateResult){output("Constituent predefined sine #%" PRIu16 ":",index);_intermediateResult->mpd=decimalcontext->predefinedsines[index];outputDecimal(" ",_intermediateResult,".\n");}
					// let's compute the cosine as well, well actually the sine of (pi-(index)/256)/2 to store at the other side of the table (64->192, 32->234, etc), that will be used as cosine of the angle pi/2-x
					if(index!=128){
						mpd_qsub_i32(_predefined,_predefined,1,mpd_context,&status);mpd_set_positive(_predefined);
						mpd_qsqrt(decimalcontext->predefinedsines[256-index],_predefined,mpd_context,&status); // take the square root of the square of the cosine (thus the cosine) and store it in the table
						if(_intermediateResult){output("Constituent predefined sine #%" PRIu16 ":",256-index);_intermediateResult->mpd=decimalcontext->predefinedsines[256-index];outputDecimal(" ",_intermediateResult,".\n");}
					}
				}
				if(_intermediateResult)outputInfo("Constituent predefined sines computed!");
				// now we know all the initial predefined sines and cosines that we may now use to compute all sines in between
				uint16_t rotationindex,inbetweenindex=128,skipindex=64;
				// from 127 through 3 that's all we need to do
				while(--inbetweenindex>2){
					if(inbetweenindex==skipindex){skipindex>>=1;continue;} // skip 64, 32, 16, 8 and 4
					if(_intermediateResult)output("Computing predefined sine #%" PRIu16 " using rotation of applicable constituent predefined sines.\n",inbetweenindex);
					// wait a minute, we know that the bit that corresponds with skipindex is set, so we can initialize the rotation to these angles
					mpd_qcopy(_rotationsine,decimalcontext->predefinedsines[skipindex],&status);
					mpd_qcopy(_rotationcosine,decimalcontext->predefinedsines[256-skipindex],&status);
					rotationindex=(skipindex>>1);
					while(rotationindex!=0){
						if((inbetweenindex&rotationindex)!=0){ // bit at rotationindex is set, so we should rotate by the associated sine and cosine
							mpd_qmul(_t1,_rotationsine,decimalcontext->predefinedsines[256-rotationindex],mpd_context,&status);
							mpd_qmul(_t2,_rotationcosine,decimalcontext->predefinedsines[rotationindex],mpd_context,&status);
							mpd_qmul(_t3,_rotationcosine,decimalcontext->predefinedsines[256-rotationindex],mpd_context,&status);
							mpd_qmul(_t4,_rotationsine,decimalcontext->predefinedsines[rotationindex],mpd_context,&status);
							mpd_qadd(_rotationsine,_t1,_t2,mpd_context,&status);
							mpd_qsub(_rotationcosine,_t3,_t4,mpd_context,&status);
						}
						rotationindex>>=1;
					}
					// and store the sine and cosine
					mpd_qcopy(decimalcontext->predefinedsines[inbetweenindex],_rotationsine,&status);
					// the cosine is the sine of pi/2 - x
					mpd_qcopy(decimalcontext->predefinedsines[256-inbetweenindex],_rotationcosine,&status);
				}
				if(_intermediateResult){
					outputInfo("Predefined sines:");
					for(int index=0;index<=256;index++){
						output("#%d",index);
						_intermediateResult->mpd=decimalcontext->predefinedsines[index];
						outputDecimal(": ",_intermediateResult,".\n");
					}
				}
			}
			free_mpd(_rotationsine);free_mpd(_rotationcosine);
			free_mpd(_0);free_mpd(_1);free_mpd(_predefined);
			free_mpd(_t1);free_mpd(_t2);free_mpd(_t3);free_mpd(_t4);

			mpd_t *_sin15=__mpd(mpd_context,0),*_cos15=__mpd(mpd_context,0),*_sin30=__mpd(mpd_context,0),*_cos30=__mpd(mpd_context,0),*_sin45=__mpd(mpd_context,0),*_sin90=__mpd(mpd_context,1),*_cos90=__mpd(mpd_context,0);
			if(_pidiv12&&_sqrt2div2&&_sqrt3div2&&_sin15&&_cos15&&_sin30&&_cos30&&_sin45&&_sin90&&_cos90){
				uint32_t mult=0;
				mpd_qdiv_u32(_pidiv12,_pidiv12,12,mpd_context,&status);
				// compute the square root terms we need (i.e. sqrt(2)/2 and sqrt(3)/2)
				mpd_qsqrt(_sqrt2div2,_sqrt2div2,mpd_context,&status);mpd_qdiv_u32(_sqrt2div2,_sqrt2div2,2,mpd_context,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_sqrt2div2;outputDecimal("Decimal 2 square root (",_intermediateResult,") computed successfully.\n");}
				mpd_qsqrt(_sqrt3div2,_sqrt3div2,mpd_context,&status);mpd_qdiv_u32(_sqrt3div2,_sqrt3div2,2,mpd_context,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_sqrt3div2;outputDecimal("Decimal 3 square root (",_intermediateResult,") computed successfully.\n");}
				// and all distinct sine and cosine values that go into the table
				mpd_qsub(_sin15,_sin90,_sqrt3div2,mpd_context,&status);mpd_qsqrt(_sin15,_sin15,mpd_context,&status);mpd_qmul(_sin15,_sin15,_sqrt2div2,mpd_context,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_sin15;outputDecimal("Sin(15deg)===Cos(75deg) (",_intermediateResult,") computed successfully.\n");}
				mpd_qadd(_cos15,_sin90,_sqrt3div2,mpd_context,&status);mpd_qsqrt(_cos15,_cos15,mpd_context,&status);mpd_qmul(_cos15,_cos15,_sqrt2div2,mpd_context,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_cos15;outputDecimal("Cos(15deg)===Sin(75deg) (",_intermediateResult,") computed successfully.\n");}
				mpd_qdiv_u32(_sin30,_sin90,2,mpd_context,&status); // to get 0.5
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_sin30;outputDecimal("Sin(30deg)===Cos(60deg) (",_intermediateResult,") computed successfully.\n");}
				mpd_qcopy(_cos30,_sqrt3div2,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_cos30;outputDecimal("Cos(30deg)===Sin(60deg) (",_intermediateResult,") set successfully to half the square root of 3.\n");}
				mpd_qcopy(_sin45,_sqrt2div2,&status);
				if((status&0xEFBF)==0)if(_intermediateResult){_intermediateResult->mpd=_sin45;outputDecimal("Sin(45deg)===Cos(45deg) (",_intermediateResult,") set successfully to half the square root of 2.\n");}
				// done computing the constant values being used
				while((status&0xEFBF)==0){
					Msincoselement* _sincoselement=(Msincoselement*)calloc(1,sizeof(Msincoselement));
					if(!_sincoselement){outputError("Failed to create the object to store the sine and cosine of a predefined angle");break;} // too bad
					mpd_t *_angle=__mpd(mpd_context,0);
					if(!_angle)break; // too bad as well
					mpd_qmul_u32(_angle,_pidiv12,mult,mpd_context,&status);
					if((status&0xEFBF)!=0){free_mpd(_angle);outputError("Failed to initialize the angle of a predefined sine and cosine");break;}
					// we've got all values so nothing can go wrong
					_sincoselement->_angle=_angle; // store the angle
					_sincoselement->mult=mult;
					if(_intermediateResult){
						output("Sine and cosine of predefined angle #%" PRIu32,_sincoselement->mult);
						_intermediateResult->mpd=_sincoselement->_angle;
						outputDecimal(" (",_intermediateResult,"):");
					}
					switch(_sincoselement->mult){
						case 0:_sincoselement->_sine=_cos90;_sincoselement->_cosine=_sin90;break; // 0 degrees
						case 1:_sincoselement->_sine=_sin15;_sincoselement->_cosine=_cos15;break; // pi/12=15 degrees
						case 2:_sincoselement->_sine=_sin30;_sincoselement->_cosine=_cos30;break; // pi/6=30 degrees
						case 3:_sincoselement->_sine=_sin45;_sincoselement->_cosine=_sin45;break; // pi/4=45 degrees
						case 4:_sincoselement->_sine=_cos30;_sincoselement->_cosine=_sin30;break; // pi/3=60 degrees
						case 5:_sincoselement->_sine=_cos15;_sincoselement->_cosine=_sin15;break; // 5*pi/12=75 degrees
						case 6:_sincoselement->_sine=_sin90;_sincoselement->_cosine=_cos90;break; // pi/2=90 degrees
					}
					if(_intermediateResult){
						_intermediateResult->mpd=_sincoselement->_sine;outputDecimal(" '",_intermediateResult,"'");
						_intermediateResult->mpd=_sincoselement->_cosine;outputDecimal(", '",_intermediateResult,"'.\n");
					}
					// remember
					_sincoselement->_next=decimalcontext->_firstSincoselement;
					decimalcontext->_firstSincoselement=_sincoselement;
					mult++;
					if(mult==7)break;
				}
				if(mult<7)output("%sFailed to create %u out of 7 predefined (co)sines.\n",M_ERROR_PREFIX,6-mult);
				free_mpd(_sqrt2div2);free_mpd(_sqrt3div2);
				// how about computing the CORDIC sines and cosines?????????
				uint32_t iteration=0;
				// NOTE we can use _sin15 and _cos15 as starting point to adapt the approximations (yes, but _sin15 and _cos15 themselves cannot be consumed as they are used in predefined angle (co)sines)
				mpd_t *_cordicsine=get_mpd_copy(mpd_context,_sin90),*_cordiccosine=get_mpd_copy(mpd_context,_cos90),*_cordictangent=__mpd(mpd_context,0),*_cordicangle=get_mpd_copy(mpd_context,_pidiv2);
				if(!_cordicsine||!_cordiccosine||!_cordicangle||!_cordictangent)status=0xFFFFFFFF;
				Msincoselement* _lastCordicElement=NULL;
				while((status&0xEFBF)==0){
					iteration++;
					if(iteration==mpd_context->prec*10){outputInfo("Computation of CORDIC angles stopped when exceeding the maximum number of iterations.");break;}
					mpd_qdiv_u32(_cordicangle,_cordicangle,2,mpd_context,&status); // divide the angle by 2
					// we need the cosine to compute the sine of half the angle
					// initially this cosine will equal _cos15 and we will use _cos15 to compute the new sine
					mpd_qcopy(_cordicsine,_cordiccosine,&status);
					mpd_qsub_u32(_cordicsine,_cordicsine,1,mpd_context,&status);mpd_set_positive(_cordicsine);mpd_qdiv_u32(_cordicsine,_cordicsine,2,mpd_context,&status);mpd_qsqrt(_cordicsine,_cordicsine,mpd_context,&status);
					// _sin15 now represents the sine of half the angle using the cosine of the double angle
					// now we adapt _cos15 to become the cosine of half the angle that we need for the next computation
					mpd_qadd_u32(_cordiccosine,_cordiccosine,1,mpd_context,&status);mpd_qdiv_u32(_cordiccosine,_cordiccosine,2,mpd_context,&status);mpd_qsqrt(_cordiccosine,_cordiccosine,mpd_context,&status);
					mpd_t* _sinsquared=_dsinsquared(mpd_context,_cordicangle); // compute the sine squared
					if(!_sinsquared)break;
					mpd_qsqrt(_sinsquared,_sinsquared,mpd_context,&status); // get the sine
					// NOTE apparently breaking on the CORDIC sine to match the CORDIC angle is not happening, so let's not check on that!!!!
					/*
					if(mpd_qcmp(_cordicsine,_cordicangle,&status)==0){ // if the sine matches the angle we're done
						free_mpd(_sinsquared);
						if(_intermediateResult){
							_intermediateResult->mpd=_cordicangle;
							outputDecimal("Angle at which the sine equals the angle: '",_intermediateResult,"'.");
						}
						break;
					}
					*/
					if(_intermediateResult){
						output("CORDIC angle iteration #%" PRIu32 ":",iteration);
						_intermediateResult->mpd=_cordicangle;
						outputDecimal("Power series approximation of the sine of angle ",_intermediateResult,": ");
						_intermediateResult->mpd=_sinsquared;
						outputDecimal(NULL,_intermediateResult,NULL);
						_intermediateResult->mpd=_cordicsine; // the 'true' value of the sine of half the previous CORDIC angle!!!
						outputDecimal(" should equal: '",_intermediateResult,"'");
					}
					mpd_qdiv(_cordictangent,_cordicsine,_cordiccosine,mpd_context,&status);
					if(_intermediateResult){
						_intermediateResult->mpd=_cordictangent;
						outputDecimal(" with tangens: '",_intermediateResult,"'.\n");
						// a ha _intermediateResult will break on a tangent that is zero, whereas otherwise it will not break!!!!
					}
					free_mpd(_sinsquared);
					// done if the CORDIC tangens equals zero!!!!
					if(mpd_iszero(_cordictangent))break;
					Msincoselement* _cordicElement=calloc(1,sizeof(Msincoselement));
					if(!_cordicElement){outputError("Failed to create a CORDIC element.");break;}
					_cordicElement->_angle=get_mpd_copy(mpd_context,_cordicangle);
					_cordicElement->_cosine=get_mpd_copy(mpd_context,_cordiccosine);
					_cordicElement->_sine=get_mpd_copy(mpd_context,_cordicsine);
					if(_lastCordicElement)_lastCordicElement->_next=_cordicElement;else decimalcontext->_firstCordicelement=_cordicElement;
					_lastCordicElement=_cordicElement; // remember the last cordic element!!!!
				}
				free_mpd(_cordicangle);free_mpd(_cordiccosine);free_mpd(_cordicsine);free_mpd(_cordictangent);
			}else
				status=0xFFFFFFFF;
			if(_intermediateResult){
				_intermediateResult->mpd=NULL; // OOPS you gotta do this MDH@23MAY2020 TODO do we?????????
				FREE_DECIMAL(_intermediateResult,owner);
			}
			if((status&0xEFBF)!=0){
				if(status!=0xFFFFFFFF){
					outputError("Failed to store predefined (co)sines in the decimal context");
					report_mpd_status(status);
				}else
					outputError("Failed to make preparations for storing predefined sine/cosines in the decimal context!");
			}

			output("Number of milliseconds to compute the sine and cosine of 256 predefined angles: %lld.\n",(clock()-then)/1000);

		}
	}
	return disowned_decimal(_decimal,owner); // freeonfailure=true means if we do not manage to wrap _pi in a decimal free it

	/* replacing:
	if(!mpd_context){if(decimalprecision>0)outputError("Failed to obtain the requested decimal context");else outputError("No (default) decimal context available");return NULL;}
	if(decimalprecision<0)decimalprecision=_decimalContext->prec;
	*/

}

// MDH@06SEP2019: if we turn the following into the computation of the sinequared itself, the caller itself can do the square rooting and or turning over to computing the cosine
//                NOTE this implementation is almost the same as that of _dsquarerootofsinorcossquared() except that it does not do the conversion to a cosine and square rooting
//                NOTE this has the disadvantage that sin squared is approximated with two additional decimals but conversion and square rooting will take place AFTER returning to the original precision
//                DONE we solve that by taking changing the precision to the _dsine/_dcosine functions
mpd_t* _dsinsquared(mpd_context_t const * const mpd_context,mpd_t const * const x){Mallocationowner owner=getOwner(__LINE__);
	// I suppose it's best to compute the sine squared first and turn it into a cosine before square rooting, that should guarantee that the squared sum of sine and cosine with the same x is 1
	mpd_t* _sinsquared=NULL;
	if(mpd_context&&x){
		if(amVerbose()){
			// TODO check whether we need to do something with get_mpd_copy
			Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true),owner);
			if(_decimal){
				output("Computing the square of the sine");outputDecimal(" of '",_decimal,"'.\n");
				FREE_DECIMAL(_decimal,owner);
			}
		}
		uint32_t status=0;
		/////////// REMOVED: mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2 decimal digits
		Mdecimal* _intermediateResult=owned_decimal(amVerbose()?__decimal(mpd_context,0,0):NULL,owner);
		
		// ok denominator is multiplied by 2 to start with (so dividing by 2 immediately)
		mpd_t*_1=__mpd(mpd_context,1); // fixed

		// helpers that are computed once before the iterations and remain constant from there one
		mpd_t *_4x2=__mpd(mpd_context,0),*_16x4=__mpd(mpd_context,0);

		// updated in each iteration can start with any value except that _num has to be initialized to _4x2 before the iterations
		mpd_t *_num=__mpd(mpd_context,0),*_term=__mpd(mpd_context,0),*_den2=__mpd(mpd_context,0),*_term2=__mpd(mpd_context,0);

		// helper decimals updated in the iterations that need to be initialized precisely here as they are directly used in the first computation!!!
		mpd_t *_2times2kfac=__mpd(mpd_context,4),*_2k=__mpd(mpd_context,2),*_prevsinsquared=__mpd(mpd_context,0);
		
		// each term equals: (_num/_2times2kfac)*_term2=(1-_4x2/_den2), with _num=(_4x2)**k en _den2=(2k+1)*(2k+2) for k=1,3,5,7,...		
		_sinsquared=__mpd(mpd_context,0); // any value will do

		if(_sinsquared&&_prevsinsquared&&_1&&_16x4&&_4x2&&_num&&_2times2kfac&&_term&&_2k&&_den2&&_term2){
			mpd_qmul(_4x2,x,x,mpd_context,&status); // compute x^2
			mpd_qmul_uint(_4x2,_4x2,4,mpd_context,&status); // multiply by 4 to get (2*x)^2=4*x^2
			mpd_qmul(_16x4,_4x2,_4x2,mpd_context,&status); // square _4x2 to get _16x4 which we need to update _num with
			mpd_qcopy(_num,_4x2,&status); // the initial value of _num is _4x2 (i.e. for k=1)
			unsigned long long iterations=0; // let's start with at most 100 iterations
			/////////bool sign=true; // the sine is computed in all cases
			mpd_qcopy(_sinsquared,_prevsinsquared,&status); // initialize _sinsquared to the initial value of the previous value
			while((status&0xEFBF)==0){
				// _num, _2times2kfac and _2k should now be as what they are supposed to be (see end of the iteration), the first time _num=_4x2, _2times2kfac=2.2!=4 and _2k is 2*1=2 of course
				iterations++;
				if(_intermediateResult)output("Iteration %llu: ",iterations);
				mpd_qdiv(_term,_num,_2times2kfac,mpd_context,&status); // compute the quotient of _num and _den as the new term which multiplied by _term2 is the new term to add
				// starting at 2*iterations increment _2k to become 2*k+1
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // 2 to 3, 6 to 7, 10 to 11, i.e. 2k to (2k+1)
				mpd_qcopy(_den2,_2k,&status); // set _den to _2k (=2*k+1)
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // increment _2k to 2*k+2
				mpd_qmul(_den2,_den2,_2k,mpd_context,&status); // make _den2 equal to (2*k+1)*(2*k+2)

				// if we compute the difference of two successive terms we get the following				
				mpd_qdiv(_term2,_4x2,_den2,mpd_context,&status); // divide _4x2 by _den2 to get the thingie to subtract from 1
				mpd_qsub(_term2,_1,_term2,mpd_context,&status); // subtract _term2 from 1 to get the new _term2
				mpd_qmul(_term,_term,_term2,mpd_context,&status); // multiply _term by _term2 to get the new _term
				mpd_qadd(_sinsquared,_prevsinsquared,_term,mpd_context,&status); // update _sinorcossquared by adding _term
				if(mpd_qcmp(_sinsquared,_prevsinsquared,&status)==0)break; // after adding _term no apparent change, so I guess we're done
				/* replacing:
				if(sign){
					sign=false; // toggle sign
					mpd_qadd(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Add ");
				}else{
					sign=true; // toggle sign
					mpd_qsub(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Subtract ");
				}
				if(!sign)if(mpd_qcmp(_sinorcossquared,_prevsinorcossquared,&status)==0)break; // no change anymore
				*/
				mpd_qcopy(_prevsinsquared,_sinsquared,&status); // update _prevsinorcossquared...
				if(_intermediateResult){
					mpd_qcopy(_intermediateResult->mpd,_term,&status);
					outputDecimal(" increment: '",_intermediateResult,"' -> ");
					mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);
					output("%s","Sine"); // if we want the cosine we indicate that we're computing One minus the cosine squared!!!!
					outputDecimal(" squared '",_intermediateResult,"'.\n");
				}

				// update _num (incrementing k by 2 each iteration), the numerator of the term in front of the subtraction (_term2)
				mpd_qmul(_num,_num,_16x4,mpd_context,&status); // update numerator (started at _4x2)

				// updating the denominator _2times2kfac (and _2k in the process to become 2*k+4
				mpd_qmul(_2times2kfac,_2times2kfac,_den2,mpd_context,&status); // now halfway from computing 2.(2k+4)! from (what it was) 2.(2k)! as _den2 equals (2k+1)*(2k+2)
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now (2*k+3)
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now (2*k+4) which is 2*(k+2) which is the next _2k to use
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply to get 2.(2k+4)!
			}
		}else{
			status=0xFFFFFFFF;
			outputError("Failed to create helper decimals in computing the sine of a decimal");
		}
		/* NO CONVERSION TO THE COSINE SQUARED AND SQUARE ROOTING
		if((status&0xEFBF)==0){ // so far, so good
			// if we need to return the cosine, compute 1 - 
			if(!sin)mpd_qsub(_sinsquared,_1,_sinsquared,mpd_context,&status);
			if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Squared result: '",_intermediateResult,"'.\n");}
			// take the square root
			mpd_qsqrt(_sinsquared,_sinsquared,mpd_context,&status);
			if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Result: '",_intermediateResult,"'.\n");}
		}
		*/
		// free all (10) helper decimals
		free_mpd(_den2);free_mpd(_term2);free_mpd(_2times2kfac);free_mpd(_num);free_mpd(_2k);free_mpd(_4x2);free_mpd(_1);free_mpd(_term);free_mpd(_16x4);
		free_mpd(_prevsinsquared);
		/* REMOVED decrementing the precision to use in computation again!!!
		// return the precision so we can return a rounded result (TODO should we do that??????)
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2 as soon as all computations are done
		if((status&0xEFBF)==0)mpd_qfinalize(_sinsquared,mpd_context,&status);else output("%sNo %ssine result to finalize.\n",M_ERROR_PREFIX,(sin?"":"co"));
		*/
		if((status&0xEFBF)!=0){
			output("%sSome error trying to compute the sine of a decimal.\n",M_ERROR_PREFIX);
			free_mpd(_sinsquared);_sinsquared=NULL;
		}else
		if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Final (rounded) result: '",_intermediateResult,"'.\n");}
		if(_intermediateResult){
			// _intermediateResult->mpd=NULL; // MDH@23MAY2020 added, but TODO not sure about this wasn't there before
			FREE_DECIMAL(_intermediateResult,owner);
		} // free verbose intermediate result decimal
	}
	////////if(!_sinsquared)outputError("Sine result vanished!");
	return _sinsquared;
}

// MDH@04SEP2019: if I combine two successive terms like I did below in _dsinorcos we'd always be adding positive numbers (never subtracting), and we'd be approaching from below...
/**
 * \brief computes the square root of the approximation to the square of the sine/cosine of \p x
 * \p x the argument (in radians)
 * \p sin true to return the square root of the sine squared, or the square root of the cosine squared
 * because the same terms are used to compute the sine and the cosine it is guaranteed that sin^2+cos^2=1 (as long as mpd_sqrt is correctly square rooting of course)
 */
mpd_t* _dsquarerootofsinorcossquared(mpd_context_t const * const mpd_context,mpd_t const * const x,bool sin){Mallocationowner owner=getOwner(__LINE__);
	// I suppose it's best to compute the sine squared first and turn it into a cosine before square rooting, that should guarantee that the squared sum of sine and cosine with the same x is 1
	mpd_t* _sinsquared=NULL;
	if(mpd_context&&x){
		if(amVerbose()){
			Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true),owner);
			if(_decimal){
				output("Computing the square of the %s",(sin?"sine":"cosine"));
				outputDecimal(" of '",_decimal,"'.\n");
				FREE_DECIMAL(_decimal,owner);
			}
		}
		uint32_t status=0;
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
		Mdecimal* _intermediateResult=owned_decimal(amVerbose()?__decimal(mpd_context,0,0):NULL,owner);
		
		// ok denominator is multiplied by 2 to start with (so dividing by 2 immediately)
		mpd_t*_1=__mpd(mpd_context,1); // fixed

		// helpers that are computed once before the iterations and remain constant from there one
		mpd_t *_4x2=__mpd(mpd_context,0),*_16x4=__mpd(mpd_context,0);

		// updated in each iteration can start with any value except that _num has to be initialized to _4x2 before the iterations
		mpd_t *_num=__mpd(mpd_context,0),*_term=__mpd(mpd_context,0),*_den2=__mpd(mpd_context,0),*_term2=__mpd(mpd_context,0);

		// helper decimals updated in the iterations that need to be initialized precisely here as they are directly used in the first computation!!!
		mpd_t *_2times2kfac=__mpd(mpd_context,4),*_2k=__mpd(mpd_context,2),*_prevsinsquared=__mpd(mpd_context,0);
		
		// each term equals: (_num/_2times2kfac)*_term2=(1-_4x2/_den2), with _num=(_4x2)**k en _den2=(2k+1)*(2k+2) for k=1,3,5,7,...		
		_sinsquared=__mpd(mpd_context,0); // any value will do

		if(_sinsquared&&_prevsinsquared&&_1&&_16x4&&_4x2&&_num&&_2times2kfac&&_term&&_2k&&_den2&&_term2){
			mpd_qmul(_4x2,x,x,mpd_context,&status); // compute x^2
			mpd_qmul_uint(_4x2,_4x2,4,mpd_context,&status); // multiply by 4 to get (2*x)^2=4*x^2
			mpd_qmul(_16x4,_4x2,_4x2,mpd_context,&status); // square _4x2 to get _16x4 which we need to update _num with
			mpd_qcopy(_num,_4x2,&status); // the initial value of _num is _4x2 (i.e. for k=1)
			unsigned long long iterations=0; // let's start with at most 100 iterations
			/////////bool sign=true; // the sine is computed in all cases
			mpd_qcopy(_sinsquared,_prevsinsquared,&status); // initialize _sinsquared to the initial value of the previous value
			while((status&0xEFBF)==0){
				// _num, _2times2kfac and _2k should now be as what they are supposed to be (see end of the iteration), the first time _num=_4x2, _2times2kfac=2.2!=4 and _2k is 2*1=2 of course
				iterations++;
				if(_intermediateResult)output("Iteration %llu: ",iterations);
				mpd_qdiv(_term,_num,_2times2kfac,mpd_context,&status); // compute the quotient of _num and _den as the new term which multiplied by _term2 is the new term to add
				// starting at 2*iterations increment _2k to become 2*k+1
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // 2 to 3, 6 to 7, 10 to 11, i.e. 2k to (2k+1)
				mpd_qcopy(_den2,_2k,&status); // set _den to _2k (=2*k+1)
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // increment _2k to 2*k+2
				mpd_qmul(_den2,_den2,_2k,mpd_context,&status); // make _den2 equal to (2*k+1)*(2*k+2)

				// if we compute the difference of two successive terms we get the following				
				mpd_qdiv(_term2,_4x2,_den2,mpd_context,&status); // divide _4x2 by _den2 to get the thingie to subtract from 1
				mpd_qsub(_term2,_1,_term2,mpd_context,&status); // subtract _term2 from 1 to get the new _term2
				mpd_qmul(_term,_term,_term2,mpd_context,&status); // multiply _term by _term2 to get the new _term
				mpd_qadd(_sinsquared,_prevsinsquared,_term,mpd_context,&status); // update _sinorcossquared by adding _term
				if(mpd_qcmp(_sinsquared,_prevsinsquared,&status)==0)break; // after adding _term no apparent change, so I guess we're done
				/* replacing:
				if(sign){
					sign=false; // toggle sign
					mpd_qadd(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Add ");
				}else{
					sign=true; // toggle sign
					mpd_qsub(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Subtract ");
				}
				if(!sign)if(mpd_qcmp(_sinorcossquared,_prevsinorcossquared,&status)==0)break; // no change anymore
				*/
				mpd_qcopy(_prevsinsquared,_sinsquared,&status); // update _prevsinorcossquared...
				if(_intermediateResult){
					mpd_qcopy(_intermediateResult->mpd,_term,&status);
					outputDecimal(" increment: '",_intermediateResult,"' -> ");
					mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);
					output("%s",(sin?"Sine":"One minus cosine")); // if we want the cosine we indicate that we're computing One minus the cosine squared!!!!
					outputDecimal(" squared '",_intermediateResult,"'.\n");
				}

				// update _num (incrementing k by 2 each iteration), the numerator of the term in front of the subtraction (_term2)
				mpd_qmul(_num,_num,_16x4,mpd_context,&status); // update numerator (started at _4x2)

				// updating the denominator _2times2kfac (and _2k in the process to become 2*k+4
				mpd_qmul(_2times2kfac,_2times2kfac,_den2,mpd_context,&status); // now halfway from computing 2.(2k+4)! from (what it was) 2.(2k)! as _den2 equals (2k+1)*(2k+2)
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now (2*k+3)
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now (2*k+4) which is 2*(k+2) which is the next _2k to use
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply to get 2.(2k+4)!
			}
		}else{
			status=0xFFFFFFFF;
			outputError("Failed to create helper decimals in computing the sine of a decimal");
		}
		if((status&0xEFBF)==0){ // so far, so good
			// if we need to return the cosine, compute 1 - 
			if(!sin)mpd_qsub(_sinsquared,_1,_sinsquared,mpd_context,&status);
			if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Squared result: '",_intermediateResult,"'.\n");}
			// take the square root
			mpd_qsqrt(_sinsquared,_sinsquared,mpd_context,&status);
			if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Result: '",_intermediateResult,"'.\n");}
		}
		// free all (10) helper decimals
		free_mpd(_den2);free_mpd(_term2);free_mpd(_2times2kfac);free_mpd(_num);free_mpd(_2k);free_mpd(_4x2);free_mpd(_1);free_mpd(_term);free_mpd(_16x4);
		free_mpd(_prevsinsquared);
		// return the precision so we can return a rounded result (TODO should we do that??????)
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2 as soon as all computations are done
		if((status&0xEFBF)==0)mpd_qfinalize(_sinsquared,mpd_context,&status);else output("%sNo %ssine result to finalize.\n",M_ERROR_PREFIX,(sin?"":"co"));
		if((status&0xEFBF)!=0){
			output("%sSome error trying to compute the %ssine of a decimal.\n",M_ERROR_PREFIX,(sin?"":"co"));
			free_mpd(_sinsquared);_sinsquared=NULL;
		}else
		if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinsquared,&status);outputDecimal("Final (rounded) result: '",_intermediateResult,"'.\n");}
		if(_intermediateResult){
			// _intermediateResult->mpd=NULL; // MDH@23MAY2020: TODO should we do this?????????
			FREE_DECIMAL(_intermediateResult,owner); // free verbose intermediate result decimal
		}
	}
	////////if(!_sinsquared)outputError("Sine result vanished!");
	return _sinsquared;
}
/* replacing (one term at a time):
mpd_t* _dsquarerootofsinorcossquared(mpd_context_t* mpd_context,mpd_t* x,bool sin){
	// I suppose it's best to compute the sine squared first and turn it into a cosine before square rooting, that should guarantee that the squared sum of sine and cosine with the same x is 1
	mpd_t* _sinorcossquared=NULL;
	if(mpd_context&&x){
		if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true);if(_decimal){output("Computing the square of the %s",(sin?"sine":"cosine"));outputDecimal(" of '",_decimal,"'.\n");free_decimal(_decimal);}}
		uint32_t status=0;
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
		Mdecimal* _intermediateResult=(amVerbose()?__decimal(mpd_context,0,0):NULL);
		// ok denominator is multiplied by 2 to start with (so dividing by 2 immediately)
		mpd_t *_4x2=__mpd(mpd_context,0),*_num=__mpd(mpd_context,1),*_den=__mpd(mpd_context,4),*_2n=__mpd(mpd_context,2),*_term=__mpd(mpd_context,2),*_1=__mpd(mpd_context,1);
		mpd_t *_prevsinorcossquared=__mpd(mpd_context,0); // replacing:__mpd(mpd_context,(sin?0:1)); // we're actually computing twice the square to start with so we start with 2 for the cosine (we'll divide by 2 down below!!)
		_sinorcossquared=__mpd(mpd_context,0); // any value will do
		if(_sinorcossquared&&_prevsinorcossquared&&_4x2&&_num&&_den&&_term&&_2n&&_1){
			mpd_qmul(_4x2,x,x,mpd_context,&status); // compute x^2
			mpd_qmul_uint(_4x2,_4x2,4,mpd_context,&status); // multiply by 4 to get (2*x)^2=4*x^2
			unsigned long long iterations=0; // let's start with at most 100 iterations
			bool sign=true; // the sine is computed in all cases
			mpd_qcopy(_sinorcossquared,_prevsinorcossquared,&status); // initialize _sinorcossquared to the initial value of the previous value
			while((status&0xEFBF)==0){
				iterations++;
				if(_intermediateResult)output("Iteration %llu: ",iterations);
				mpd_qmul(_num,_num,_4x2,mpd_context,&status); // update numerator (started at 1)
				mpd_qdiv(_term,_num,_den,mpd_context,&status); // compute the quotient of _num and _den as the new term to add or subtract
				if(sign){
					sign=false; // toggle sign
					mpd_qadd(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Add ");
				}else{
					sign=true; // toggle sign
					mpd_qsub(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status);
					if(_intermediateResult)output(" Subtract ");
				}
				if(!sign)if(mpd_qcmp(_sinorcossquared,_prevsinorcossquared,&status)==0)break; // no change anymore
				mpd_qcopy(_prevsinorcossquared,_sinorcossquared,&status); // update _prevsinorcossquared...
				if(_intermediateResult){
					mpd_qcopy(_intermediateResult->mpd,_term,&status);
					outputDecimal(" increment: '",_intermediateResult,"' -> ");
					mpd_qcopy(_intermediateResult->mpd,_sinorcossquared,&status);
					output("%s",(sin?"Sine":"One minus cosine")); // if we want the cosine we indicate that we're computing One minus the cosine squared!!!!
					outputDecimal(" squared '",_intermediateResult,"'.\n");
				}
				// updating the denominator (started as 2)
				mpd_qadd_u32(_2n,_2n,1,mpd_context,&status); // _2n now 3, 5, 7, ...
				mpd_qmul(_den,_den,_2n,mpd_context,&status); // multiply
				mpd_qadd_u32(_2n,_2n,1,mpd_context,&status); // _2n now 4, 6, 8, ...
				mpd_qmul(_den,_den,_2n,mpd_context,&status); // multiply
			}
		}else{
			status=0xFFFFFFFF;
			outputError("Failed to create helper decimals in computing the sine of a decimal");
		}
		if((status&0xEFBF)==0){ // so far, so good
			// if we need to return the cosine, compute 1 - 
			if(!sin)mpd_qsub(_sinorcossquared,_1,_sinorcossquared,mpd_context,&status);
			if(_intermediateResult){mpd_qcopy(_intermediateResult->mpd,_sinorcossquared,&status);outputDecimal("Squared result: '",_intermediateResult,"'.\n");}
			// take the square root
			mpd_qsqrt(_sinorcossquared,_sinorcossquared,mpd_context,&status);
		}
		if(_intermediateResult)free_decimal(_intermediateResult);
		// free all helper decimals
		free_mpd(_den);free_mpd(_term);free_mpd(_num);free_mpd(_prevsinorcossquared);free_mpd(_2n);free_mpd(_4x2);free_mpd(_1);
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2
		if((status&0xEFBF)!=0){free_mpd(_sinorcossquared);_sinorcossquared=NULL;}else mpd_finalize(_sinorcossquared,mpd_context);
	}
	return _sinorcossquared;
}
*/
typedef struct mpd_sincos_t{
	mpd_t* sin;
	mpd_t* cos;
}mpd_sincos_t;

mpd_sincos_t* owned_mpd_sincos(mpd_sincos_t* _mpd_sincos,Mallocationowner owner_mpd_sincos){
	return(mpd_sincos_t*)OWNED(_mpd_sincos,owner_mpd_sincos);
}
mpd_sincos_t* disowned_mpd_sincos(mpd_sincos_t* _mpd_sincos,Mallocationowner owner_mpd_sincos){
	return(mpd_sincos_t*)DISOWNED(_mpd_sincos,owner_mpd_sincos);
}
void free_mpd_sincos(mpd_sincos_t* _mpd_sincos){
	free_mpd(_mpd_sincos->sin);
	free_mpd(_mpd_sincos->cos);
	FREE_1(_mpd_sincos,'T');
}
#define FREE_MPD_SINCOS(_mpd_sincos,owner_mpd_sincos) free_mpd_sincos(disowned_mpd_sincos(_mpd_sincos,owner_mpd_sincos))

/**
 * \brief returns a pair of mpd_t* instances containing the sine and cosine of \p x respectively guaranteeing their sum of squares equals 1
 * \p x the decimal to compute the sine/cosine of
 */
mpd_sincos_t* _dsinandcos(mpd_context_t const * const mpd_context,mpd_t const * const x){Mallocationowner owner=getOwner(__LINE__);
	// the initial value of the sine is x, and of the cosine is 1
	mpd_sincos_t* _mpd_sinandcos=NULL;
	if(mpd_context&&x){
		_mpd_sinandcos=CALLOC_1(sizeof(mpd_sincos_t),'#',owner);
		if(_mpd_sinandcos){
			// initialize the sine and cosine to x and 1 respectively i.e. the first term of the infinite series expansion
			_mpd_sinandcos->sin=get_mpd_copy(mpd_context,x);
			_mpd_sinandcos->cos=__mpd(mpd_context,1);
			if(_mpd_sinandcos->sin&&_mpd_sinandcos->cos){
				if(amVerboseDebugging()){
					Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true),owner);
					if(_decimal){
						outputDecimal("Computing the sine and cosine of '",_decimal,"'.\n");
						FREE_DECIMAL(_decimal,owner);
					}
				}
				// we have the numerator and denominator of the term to add
				uint32_t status=0;
				mpd_t *_num=get_mpd_copy(mpd_context,x),*_den=__mpd(mpd_context,1),*_n=__mpd(mpd_context,1),*_term=__mpd(mpd_context,0);
				mpd_t *_newsine=__mpd(mpd_context,0),*_newcosine=__mpd(mpd_context,0); // starting value doesn't matter will get copied to start with anyway
				if(_n&&_num&&_den&&_term&&_newsine&&_newcosine){
					Mdecimal *_intermediateSine=(amVerboseDebugging()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL),*_intermediateCosine=(amVerboseDebugging()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
					// every other term should be negated
					bool negate=true;
					uint64_t iteration=0;
					while((status&0xEFBF)==0){
						iteration++;
						if(_intermediateSine||_intermediateCosine)
							output("Iteration #%" PRIu64 ": ",iteration);
						mpd_qadd_u32(_n,_n,1,mpd_context,&status); // make _n equal to iteration
						mpd_qmul(_den,_den,_n,mpd_context,&status);
						// cosine first
						mpd_qmul(_num,_num,x,mpd_context,&status);
						mpd_qdiv(_term,_num,_den,mpd_context,&status);
						if(negate)
							mpd_qsub(_newcosine,_mpd_sinandcos->cos,_term,mpd_context,&status);
						else
							mpd_qadd(_newcosine,_mpd_sinandcos->cos,_term,mpd_context,&status);
						if(_intermediateCosine){
							mpd_qcopy(_intermediateCosine->mpd,_term,&status);
							output(" Cosine: %s ",(negate?"minus":"plus"));
							outputDecimal("'",_intermediateCosine,"' -> ");
							mpd_qcopy(_intermediateCosine->mpd,_newcosine,&status);
							outputDecimal("'",_intermediateCosine,"'");
						}
						// and now the sine
						mpd_qadd_u32(_n,_n,1,mpd_context,&status); // make _n equal to iteration
						mpd_qmul(_den,_den,_n,mpd_context,&status);
						mpd_qmul(_num,_num,x,mpd_context,&status);
						mpd_qdiv(_term,_num,_den,mpd_context,&status);
						if(negate)
							mpd_qsub(_newsine,_mpd_sinandcos->sin,_term,mpd_context,&status);
						else
							mpd_qadd(_newsine,_mpd_sinandcos->sin,_term,mpd_context,&status);
						if(_intermediateSine){
							mpd_qcopy(_intermediateSine->mpd,_term,&status);
							output(" Sine: %s ",(negate?"minus":"plus"));
							outputDecimal("'",_intermediateSine,"' -> ");
							mpd_qcopy(_intermediateSine->mpd,_newsine,&status);
							outputDecimal("'",_intermediateSine,"'");
						}
						negate=!negate;
						if(_intermediateSine||_intermediateCosine)output(".\n");
						// now we can test whether or not were done, which we are when neither cosine nor sine changed
						// BUT only break on a positive term not on a negative term!!!
						///////if(negate)
						if(mpd_qcmp(_mpd_sinandcos->cos,_newcosine,&status)==0&&mpd_qcmp(_mpd_sinandcos->sin,_newsine,&status)==0)break;
						// copy the new sine and cosine over
						mpd_qcopy(_mpd_sinandcos->cos,_newcosine,&status);
						mpd_qcopy(_mpd_sinandcos->sin,_newsine,&status);
					}
					if(_intermediateSine)FREE_DECIMAL(_intermediateSine,owner);
					if(_intermediateCosine)FREE_DECIMAL(_intermediateCosine,owner);
				}else{
					status=1;
					outputError("Failed to create the internal helper decimals in computing the sine and cosine of a decimal");
				}
				free_mpd(_newsine);free_mpd(_newcosine);
				free_mpd(_n);free_mpd(_num);free_mpd(_den);free_mpd(_term);
				if((status&0xEFBF)==0){
					// final step: divide the sine and cosine with square root of their sum of squares
					mpd_t *_sumofsquares=NULL,*_sinesquared=__mpd(mpd_context,0),*_cosinesquared=__mpd(mpd_context,0),*_one=__mpd(mpd_context,1);
					if(_one&&_sinesquared&&_cosinesquared){
						_sumofsquares=__mpd(mpd_context,0);
						if(_sumofsquares){
							mpd_qmul(_cosinesquared,_mpd_sinandcos->cos,_mpd_sinandcos->cos,mpd_context,&status);
							mpd_qmul(_sinesquared,_mpd_sinandcos->sin,_mpd_sinandcos->sin,mpd_context,&status);
							mpd_qadd(_sumofsquares,_cosinesquared,_sinesquared,mpd_context,&status); // add the squares
							if(mpd_qcmp(_sumofsquares,_one,&status)!=0){ // sum of squares does not equal 1
								mpd_qsqrt(_sumofsquares,_sumofsquares,mpd_context,&status);
								if(amVerbose()){
									Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_sumofsquares),mpd_context->prec,0,true),owner);
									if(_decimal){outputDecimal("Sum of sine squared and cosine squared: '",_decimal,"'.\n");FREE_DECIMAL(_decimal,owner);}
								}
								mpd_qdiv(_mpd_sinandcos->sin,_mpd_sinandcos->sin,_sumofsquares,mpd_context,&status);
								mpd_qdiv(_mpd_sinandcos->cos,_mpd_sinandcos->cos,_sumofsquares,mpd_context,&status);
								if((status&0xEFBF)!=0){free_mpd(_sumofsquares);_sumofsquares=NULL;}
							}
						}
					}else
						outputError("Failed to correct the sine and cosine of a decimal");
					// if we've got them, free them
					free_mpd(_one);free_mpd(_sinesquared);free_mpd(_cosinesquared);
					//	mpd_finalize(_mpd_sinandcos->sin,mpd_context);mpd_finalize(_mpd_sinandcos->cos,mpd_context);
					if(_sumofsquares){free_mpd(_sumofsquares);return disowned_mpd_sincos(_mpd_sinandcos,owner);}
				}
			}
			FREE_MPD_SINCOS(_mpd_sinandcos,owner);
		}else
			outputError("Failed to initialize the object storing the sine and cosine of a decimal");
	}else
		outputError("Failed to create the object to store the sine and cosine of a decimal in");
	return NULL;
}

mpd_t* _dsinorcos(mpd_context_t const * const mpd_context,mpd_t const * const x,bool sin){Mallocationowner owner=getOwner(__LINE__); // convergence requires x to be below 1
	mpd_t* _sinorcos=NULL;
	if(mpd_context&&x){
		if(amVerbose()){
			Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true),owner);
			if(_decimal){output("Computing the %s",(sin?"sine":"cosine"));outputDecimal(" of '",_decimal,"'.\n");FREE_DECIMAL(_decimal,owner);}
		}
		uint32_t status=0;
		// the sine of x equals the som of an infinite number of terms multiplied by x
		// each element of the sequence has an index, say n, but let's start with n=0
		// each term then equals (x^4n)/(4n+1)!)*(1-(x^2)/(4n+2)*(4n+3)))
		// so n=0: (x^0/1!)*(1-x^2/2*3), n=1: 
		//////////mpd_qsetprec(decimalContext,mpd_getprec(decimalContext)+2); // increment the precision by 2
		Mdecimal* _intermediateResult=(amVerboseDebugging()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
		mpd_t *_x2=__mpd(mpd_context,0),*_x4=__mpd(mpd_context,0),*_1minus=__mpd(mpd_context,0),*_sub=__mpd(mpd_context,0),*_prevsincos=__mpd(mpd_context,0),*_prodacc=__mpd(mpd_context,0),*_prevprodacc=__mpd(mpd_context,0); // parts that need to be initialized
		mpd_t *_prod=__mpd(mpd_context,1),*_multnum=__mpd(mpd_context,1),*_mult=__mpd(mpd_context,1),*_1=__mpd(mpd_context,1);
		// helpers of which the value differs whether a sine or cosine approximation is requested (den is 3! for the sine, and 2! for the cosine)
		mpd_t *_den=__mpd(mpd_context,(sin?6:2)),*_4n=__mpd(mpd_context,(sin?3:2)),*_multden=__mpd(mpd_context,(sin?6:2)); // initialized helper decimals
		_sinorcos=__mpd(mpd_context,0);
		if(_sinorcos&&_prevsincos&&_x2&&_x4&&_multnum&&_multden&&_mult&&_den&&_4n&&_prod&&_1minus&&_sub&&_1&&_prodacc&&_prevprodacc){
			mpd_qmul(_x2,x,x,mpd_context,&status);
			mpd_qmul(_x4,_x2,_x2,mpd_context,&status);
			unsigned long long iterations=0; // let's start with at most 100 iterations
			while((status&0xEFBF)==0){
				iterations++;
				// compute _sub
				mpd_qdiv(_sub,_x2,_den,mpd_context,&status); // first time this would  be x^2/6
				// compute _1minus
				mpd_qsub(_1minus,_1,_sub,mpd_context,&status);
				// compute _prod as the product of _mult and _1minus
				mpd_qmul(_prod,_mult,_1minus,mpd_context,&status); // first time this would be 1 * x^2/6
				if(mpd_iszero(_prod))break; // if the product is now zero we're definitely done
				// increment sine with the new product
				if(mpd_iszero(_prevprodacc)){ // accuracy not yet reached
					// add _prod to _prevsine to become the new sine
					mpd_qadd(_sinorcos,_prevsincos,_prod,mpd_context,&status);
					if(mpd_qcmp(_prevsincos,_sinorcos,&status)==0) // _sine and _prevsine technically the same (in the given decimal context)
						mpd_qcopy(_prevprodacc,_prod,&status); // store the non-zero _prod in _prevprodacc, from now on we will keep doing that
					else // new sine differs from previous sine: required accuracy not yet reached
						mpd_qcopy(_prevsincos,_sinorcos,&status); // update _prevsine
					if(_intermediateResult){
						output("Iteration %llu: ",iterations);
						mpd_qcopy(_intermediateResult->mpd,_prod,&status);
						outputDecimal("Increment: '",_intermediateResult,"' -> ");
						if(sin)mpd_qmul(_intermediateResult->mpd,_sinorcos,x,mpd_context,&status);else mpd_qcopy(_intermediateResult->mpd,_sinorcos,&status);
						outputDecimal((sin?"Sine: ":"Cosine: '"),_intermediateResult,"'.\n");
					}
				}else{ // accuracy reached, but still some iterations left
					mpd_qadd(_prodacc,_prevprodacc,_prod,mpd_context,&status);
					if(_intermediateResult){
						output("Iteration %llu: ",iterations);
						mpd_qcopy(_intermediateResult->mpd,_prodacc,&status);
						outputDecimal("Incremental remainder: '",_intermediateResult,"'.\n");
					}
					if(mpd_qcmp(_prodacc,_prevprodacc,&status)==0)break;
					mpd_qcopy(_prodacc,_prevprodacc,&status); // copy the change accumulative remainder
				}
				// update the helpers _den, _sub, _multnum, _multden, _mult, _4n
				mpd_qmul(_multnum,_multnum,_x4,mpd_context,&status); // updating _multnum is easy as we only need to multiply it by x^4
				// NOTE _4n starts equal to 3 (as _den starts as 3!), and the faculty stored in _multden needs to be updated 4 times
				// so, we have to increment _4n four times and use each of these 4 values to update _multden to become the new faculty value to use
				mpd_qadd_uint(_4n,_4n,1,mpd_context,&status); // now equal to (4n)
				mpd_qmul(_multden,_multden,_4n,mpd_context,&status);
				mpd_qadd_uint(_4n,_4n,1,mpd_context,&status); // now equal to (4n+1)
				mpd_qmul(_multden,_multden,_4n,mpd_context,&status);
				// after two increments to _4n _multden is what we want it to be for computing the 
				// _multnum and _multden updated, so we can now update _mult
				mpd_qdiv(_mult,_multnum,_multden,mpd_context,&status);
				
				mpd_qadd_uint(_4n,_4n,1,mpd_context,&status); // now equal to (4n+2)
				mpd_qmul(_multden,_multden,_4n,mpd_context,&status);
				mpd_qcopy(_den,_4n,&status); // initialize _den to _4n
				
				mpd_qadd_uint(_4n,_4n,1,mpd_context,&status); // now equal to (4n+3)
				mpd_qmul(_multden,_multden,_4n,mpd_context,&status);
				mpd_qmul(_den,_den,_4n,mpd_context,&status); // _den now equal to (4n+2)*(4n+3) as we need it to be

				// with _den computed we can now update _sub 
				mpd_qdiv(_sub,_x4,_den,mpd_context,&status);
				// and ready to 
			}
		}else
			outputError("Failed to create helper decimals in computing the sine of a decimal");
		if(_intermediateResult)FREE_DECIMAL(_intermediateResult,owner);
		// if accuracy was reached, but we still had some more iterations left we can add the accumulated remainder
		if(!mpd_iszero(_prevprodacc)){
			mpd_qadd(_sinorcos,_sinorcos,_prevprodacc,mpd_context,&status);
		}
		free_mpd(_prevprodacc);
		free_mpd(_prodacc);
		free_mpd(_prevsincos);
		free_mpd(_x2);
		free_mpd(_x4);
		free_mpd(_4n);
		free_mpd(_1);
		free_mpd(_sub);
		free_mpd(_1minus);
		free_mpd(_multnum);
		free_mpd(_multden);
		free_mpd(_mult);
		free_mpd(_den);
		free_mpd(_prod);
		// finally multiply by x if the sine was requested!!!
		if((status&0xEFBF)==0)if(sin)mpd_qmul(_sinorcos,_sinorcos,x,mpd_context,&status);
		////////mpd_qsetprec(decimalContext,mpd_getprec(decimalContext)-2); // decrement the precision by 2
		if((status&0xEFBF)!=0){free_mpd(_sinorcos);_sinorcos=NULL;}////////else mpd_finalize(_sine,decimalContext);
	}
	return _sinorcos;
}

// MDH@06AUG2019: if we want to be able to determine the nearest stored predefined sine/cosine angle
typedef struct mpd_relative_angle{
	Msincoselement* sincoselement;
	mpd_t* _delta_angle; // the (positive) difference with the given sincoselement angle
	bool negative; // whether or not a negative relative angle
}mpd_relative_angle_t;

mpd_relative_angle_t* disowned_mpd_relative_angle(mpd_relative_angle_t* _mpd_relative_angle,Mallocationowner owner_mpd_relative_angle){
	if(!_mpd_relative_angle)return NULL;
	disowned_sincoselement(_mpd_relative_angle->sincoselement,owner_mpd_relative_angle); // MDH@11SEP2019: now we do need to free the Msincoselement*
	return (mpd_relative_angle_t*)DISOWNED(_mpd_relative_angle,owner_mpd_relative_angle);
}
mpd_relative_angle_t* owned_mpd_relative_angle(mpd_relative_angle_t* _mpd_relative_angle,Mallocationowner owner_mpd_relative_angle){
	if(!_mpd_relative_angle)return NULL;
	owned_sincoselement(_mpd_relative_angle->sincoselement,owner_mpd_relative_angle); // MDH@11SEP2019: now we do need to free the Msincoselement*
	return(mpd_relative_angle_t*)OWNED(_mpd_relative_angle,owner_mpd_relative_angle);
}
void free_mpd_relative_angle(mpd_relative_angle_t* _mpd_relative_angle/*,Mallocationowner owner_mpd_relative_angle*/){
	if(!_mpd_relative_angle)return;
	free_sincoselement(_mpd_relative_angle->sincoselement/*,owner_mpd_relative_angle*/); // MDH@11SEP2019: now we do need to free the Msincoselement*
	free_mpd(_mpd_relative_angle->_delta_angle);
	FREE_1(_mpd_relative_angle,'$'/*,owner_mpd_relative_angle*/);
}
#define FREE_MPD_RELATIVE_ANGLE(_mpd_relative_angle,owner_mpd_relative_angle) free_mpd_relative_angle(disowned_mpd_relative_angle(_mpd_relative_angle,owner_mpd_relative_angle))

// MDH@11SEP2019: we can do much faster and better now we have the predefinedsines in Mdecimalcontext's
mpd_relative_angle_t* _getPredefinedSinesRelativeAngle(Mdecimalcontext const * const decimalcontext,mpd_t const * const angle){Mallocationowner owner=getOwner(__LINE__);
	if(angle&&decimalcontext&&decimalcontext->predefinedsinedeltaangle){
		Mdecimal* _intermediateResult=(amVerboseDebugging()?owned_decimal(__decimal(decimalcontext->mpd_context,0,0),owner):NULL);
		if(_intermediateResult){
			_intermediateResult->mpd=angle;
			outputDecimal("Computing the relative angle of '",_intermediateResult,"' to the nearest predefined angles for which sines were computed.\n");
		}
		mpd_relative_angle_t* _relativeAngle=CALLOC_1(sizeof(mpd_relative_angle_t),'$',owner);
		if(_relativeAngle){
			mpd_t *_deltaAngle=__mpd(decimalcontext->mpd_context,0),*_predefinedAngleIndex=__mpd(decimalcontext->mpd_context,0);
			if(_deltaAngle&&_predefinedAngleIndex){
				uint32_t status=0;
				mpd_qdivmod(_predefinedAngleIndex,_deltaAngle,angle,decimalcontext->predefinedsinedeltaangle,decimalcontext->mpd_context,&status);
				uint64_t predefinedAngleIndex=mpd_qget_u64(_predefinedAngleIndex,&status);
				if(predefinedAngleIndex<=256){
					if(_intermediateResult)output("Predefined angle index %" PRIu32 ".\n",predefinedAngleIndex);
					_relativeAngle->negative=false;
					_relativeAngle->_delta_angle=__mpd(decimalcontext->mpd_context,0);
					if(_relativeAngle->_delta_angle){
						mpd_qcopy(_relativeAngle->_delta_angle,_deltaAngle,&status); // remember the remaining angle (which is positive!!!)
						// now we do have to construct the Msincoselement!
						_relativeAngle->sincoselement=(Msincoselement*)CALLOC_1(sizeof(Msincoselement),'#',owner);
						if(_relativeAngle->sincoselement){
							_relativeAngle->sincoselement->_sine=__mpd(decimalcontext->mpd_context,0);
							_relativeAngle->sincoselement->_cosine=__mpd(decimalcontext->mpd_context,0);
							_relativeAngle->sincoselement->_angle=__mpd(decimalcontext->mpd_context,0);
							if(_relativeAngle->sincoselement->_sine&&_relativeAngle->sincoselement->_cosine&&_relativeAngle->sincoselement->_angle){
								mpd_qmul(_relativeAngle->sincoselement->_angle,_predefinedAngleIndex,decimalcontext->predefinedsinedeltaangle,decimalcontext->mpd_context,&status);
								mpd_qcopy(_relativeAngle->sincoselement->_sine,decimalcontext->predefinedsines[predefinedAngleIndex],&status);
								mpd_qcopy(_relativeAngle->sincoselement->_cosine,decimalcontext->predefinedsines[256-predefinedAngleIndex],&status); // we know where to find the cosine of the predefined angle
							}else{
								FREE_SINCOSELEMENT(_relativeAngle->sincoselement,owner);
								_relativeAngle->sincoselement=NULL;
								status=0xFFFFFFFF;
							}
						}else{
							status=0xFFFFFFFF;
							outputError("Failed to create the relative angle offset sine/cosine data element");
						}
					}else{
						status=0xFFFFFFFF;
						outputError("Failed to create the relative angle remainder.");
					}
				}else{
					status=0xFFFFFFFF;
					outputError("Predefined sines index invalid");
				}
				if((status&0xEFBF)!=0){FREE_MPD_RELATIVE_ANGLE(_relativeAngle,owner);_relativeAngle=NULL;}
			}else 
				outputError("Failed to create a relative angle offset and remainder.");
			free_mpd(_deltaAngle);free_mpd(_predefinedAngleIndex);
		}else
			outputError("Failed to create a relative angle.");
		if(_intermediateResult){_intermediateResult->mpd=NULL;FREE_DECIMAL(_intermediateResult,owner);}
		return disowned_mpd_relative_angle(_relativeAngle,owner);
	}else
		outputError("No angle, decimal context or decimal context predefined sines step angle defined.");
	return NULL;
}
// preferable over:
mpd_relative_angle_t* _getRelativeAngle(Mdecimalcontext const * const decimalcontext,mpd_t const * const angle){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT angle must be in [0,pi/2] that way there will always be two surrounding predefined angles
	mpd_relative_angle_t* _relativeAngle=NULL;
	if(decimalcontext&&angle){
		Mdecimal* _intermediateResult=(amVerboseDebugging()?__decimal(decimalcontext->mpd_context,0,0):NULL);
		if(_intermediateResult){
			_intermediateResult->mpd=angle;
			outputDecimal("Computing the relative angle of '",_intermediateResult,"'.\n");
		}
		mpd_t *_deltaAngle1=__mpd(decimalcontext->mpd_context,0),*_deltaAngle2=__mpd(decimalcontext->mpd_context,0);
		if(_deltaAngle1&&_deltaAngle2){
			_relativeAngle=(mpd_relative_angle_t*)CALLOC_1(sizeof(mpd_relative_angle_t),'A',owner);
			if(_relativeAngle){
				uint32_t status=0;
				Msincoselement *sincoselement=decimalcontext->_firstSincoselement;
				if(sincoselement){
					Msincoselement *nextsincoselement=NULL;
					while(sincoselement){
						nextsincoselement=sincoselement->_next;
						if(nextsincoselement!=NULL){
							if(_intermediateResult){
								_intermediateResult->mpd=nextsincoselement->_angle;
								outputDecimal("Checking whether the angle lies between ",_intermediateResult," and ");
								_intermediateResult->mpd=sincoselement->_angle;
								outputDecimal(NULL,_intermediateResult,".\n");
							}
							mpd_qsub(_deltaAngle2,angle,nextsincoselement->_angle,decimalcontext->mpd_context,&status);
							if(!mpd_isnegative(_deltaAngle2)){ // angle not below the lower angle so we found the bounding angle interval
								mpd_qsub(_deltaAngle1,sincoselement->_angle,angle,decimalcontext->mpd_context,&status);
								if(mpd_qcmp(_deltaAngle1,_deltaAngle2,&status)<0){ // deltaAngle1 is smaller than deltaAngle2 and wins
									free_mpd(_deltaAngle2);
									_relativeAngle->_delta_angle=_deltaAngle1;
									_relativeAngle->negative=true; // because angle is smaller we need to return a negative delta angle
								}else{
									free_mpd(_deltaAngle1);
									sincoselement=nextsincoselement;
									_relativeAngle->_delta_angle=_deltaAngle2;
									_relativeAngle->negative=false; // because angle is smaller we need to return a negative delta angle
								}
								break;
							}
						}
						sincoselement=nextsincoselement;
					}
					if(!sincoselement)
					{FREE_DISOWNED_1(_relativeAngle,'A',owner);_relativeAngle=NULL;}else
					_relativeAngle->sincoselement=sincoselement;
				}else
					outputError("No predefined angles");
			}else
				outputError("Failed to initialize the relative angle");
		}else
			outputError("Failed to prepare for computing the relative angle");
		if(_intermediateResult){_intermediateResult->mpd=NULL;FREE_DECIMAL(_intermediateResult,owner);}
		free_mpd(_deltaAngle1);free_mpd(_deltaAngle2);
	}
	return disowned_mpd_relative_angle(_relativeAngle,owner);
}

// MDH@12SEP2019: instead of first determining the rotations to do, and then doing them, we can immediately perform the rotation
mpd_t* _getCORDICsinorcos(Mdecimalcontext const * const decimalcontext,mpd_t const * const x,bool sin){Mallocationowner owner=getOwner(__LINE__);
	// I guess an iterative procedure is better than a recursive procedure, because in a recursive procedure we have to keep passing the mpd_context...
	// this means I can't get down but then the problem is that I can't do the rotations until I find all the constituent angles
	// yes of course how many CORDIC angles do we have??????
	// we can't start with the smallest we have to start with 45 degrees, then 22.5 degrees etc.
	// if we have many of these angles we can use a big integer to store the booleans that determine whether or not to perform a rotation
	mpd_context_t* mpd_context=decimalcontext->mpd_context;
	Mdecimal* _intermediateResult=(amVerbose()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
	uint32_t status=0;
	mpd_t* _xcopy=get_mpd_copy(mpd_context,x);
	// as long as the remaining angle isn't zero keep going
	// I guess I could do the rotation immediately?????
	uint32_t count=0;
	int comp;
	// the decimals we need for actually keeping track of the intermediate rotation results
	mpd_t *_t1=__mpd(mpd_context,0),*_t2=__mpd(mpd_context,0),*_t3=__mpd(mpd_context,0),*_t4=__mpd(mpd_context,0);
	mpd_t *_resultcosine=__mpd(mpd_context,1),*_resultsine=__mpd(mpd_context,0); // starting at 0 degrees!!
	Msincoselement* _cordicElement=decimalcontext->_firstCordicelement;
	while(_xcopy&&!mpd_iszero(_xcopy)&&_cordicElement){
		count++;
		comp=mpd_qcmp(_cordicElement->_angle,_xcopy,&status);
		if(_intermediateResult){
			output("Result of comparing ");
			_intermediateResult->mpd=_cordicElement->_angle;
			outputDecimal(NULL,_intermediateResult," with ");
			_intermediateResult->mpd=_xcopy;
			outputDecimal(NULL,_intermediateResult,":");
			output("%d.\n",comp);
		}
		if(comp<=0){ // we have to do a rotation by the CORDIC angle
			mpd_qsub(_xcopy,_xcopy,_cordicElement->_angle,mpd_context,&status); // subtract the CORDIC angle from the angle left to rotate to
			// sin(a+b)=sin(a)cos(b)+cos(a)sin(b)=_t1+_t2
			// cos(a+b)=cos(a)cos(b)-sin(a)sin(b)=_t3+_t4
			mpd_qmul(_t1,_resultsine,_cordicElement->_cosine,mpd_context,&status);
			mpd_qmul(_t2,_resultcosine,_cordicElement->_sine,mpd_context,&status);
			mpd_qmul(_t3,_resultcosine,_cordicElement->_cosine,mpd_context,&status);
			mpd_qmul(_t4,_resultsine,_cordicElement->_sine,mpd_context,&status);
			mpd_qadd(_resultsine,_t1,_t2,mpd_context,&status);
			mpd_qsub(_resultcosine,_t3,_t4,mpd_context,&status);
			if(_intermediateResult){
				_intermediateResult->mpd=_cordicElement->_angle;
				outputDecimal("Rotating by CORDIC angle '",_intermediateResult,"' with ");
				_intermediateResult->mpd=_cordicElement->_sine;
				outputDecimal("sine ",_intermediateResult," and ");
				_intermediateResult->mpd=_cordicElement->_cosine;
				outputDecimal("cosine ",_intermediateResult,NULL);
				_intermediateResult->mpd=_resultsine;
				outputDecimal(" giving result sine: ",_intermediateResult,".\n");
			}
		}
		_cordicElement=_cordicElement->_next;
		if((status&0xEFBF)!=0){free_mpd(_resultsine);_resultsine=NULL;break;}
	}
	free_mpd(_xcopy);
	free_mpd(_t1);free_mpd(_t2);free_mpd(_t3);free_mpd(_t4);if(sin)free_mpd(_resultcosine);else free_mpd(_resultsine); // OOPS forgotten below
	if(_intermediateResult){_intermediateResult->mpd=NULL;FREE_DECIMAL(_intermediateResult,owner);}
	return(sin?_resultsine:_resultcosine);
}
// for testing it's a good idea to be able to ask for the CORDIC sine
Mdecimal* _dcordicsine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi||!decimalcontext->predefinedsinedeltaangle)
				FREE_DECIMAL(pi_decimal(decimalcontext,true),owner); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))
					return _getDecimal(__mpd(mpd_context,0),mpd_context->prec,0,true);
				uint32_t status=0;
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}}
					if(!_absx)
					{outputError("Failed to negate the decimal to compute the CORDIC sine of");return NULL;}
					if(amVerbose())outputInfo("Computing the CORDIC sine of a negative decimal.");
				}
				mpd_t* _CORDICsine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){
						Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true),owner);
						if(_decimal){outputDecimal("Normalized CORDIC sine (abs) argument: '",_decimal,"'.\n");FREE_DECIMAL(_decimal,owner);}
					}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint64_t xquadrant=mpd_qget_u64(_xquadrant,&status);
					if(amVerbose()){
						outputDecimal("Quadrant of CORDIC sine argument '",x,"': ");
						output("%" PRIu32 ".\n",xquadrant);
					}
					if((status&0xEFBF)!=0){
						outputError("Failed to compute the CORDIC sine of a decimal");
						report_mpd_status(status);
					}else{
						bool sin=true; // whether to compute the sine or cosine (of the transformed angle)
						mpd_t* _xsin=NULL;
						if(xquadrant!=0){
							sin=false;
							_xsin=__mpd(mpd_context,0);
							if(_xsin){
								if(xquadrant==1)
									mpd_qsub(_xsin,decimalcontext->pi,_xmod,mpd_context,&status);
								else
								if(xquadrant==2)
									mpd_qsub(_xsin,_xmod,decimalcontext->pi,mpd_context,&status);
								else
									mpd_qsub(_xsin,decimalcontext->pimul2,_xmod,mpd_context,&status);
							}else
								status=0xFFFFFFFF;
						}
						/*
						if(mpd_qcmp(_xmod,decimalcontext->pidiv4,&status)>0){ // the remainder (in [0,pi/2) is above pi/4
							sin=false;
							_xcos=__mpd(mpd_context,0);
							if(_xcos)mpd_qsub(_xcos,decimalcontext->pidiv2,_xmod,mpd_context,&status);
						}
						*/
						if((status&0xEFBF)!=0){
							outputError("Failed to compute the CORDIC sine of a decimal");
							if(status!=0xFFFFFFFF)report_mpd_status(status);
						}else{
							// if we want to use sine/cosine formulas using the predefined sine/cosine table we have to find the smallest difference with any of the predefined angles
							// looking up should return the nearest sincos element in the table
							_CORDICsine=_getCORDICsinorcos(decimalcontext,(sin?_xmod:_xsin),true);
							///*
							if(_CORDICsine){
								if((_absx!=NULL)!=(xquadrant==2||xquadrant==3)){ // NOTE equivalent to using the ^ bitwise operator!!!
									if(amVerbose())outputInfo("Negating the computed CORDIC sine!");
									mpd_set_negative(_CORDICsine); // negate the _sine
								}else
								if(amVerbose())outputInfo("Not negating the CORDIC sine!");
								if(amVerbose()){
									Mdecimal* _decimal=owned_decimal(__decimal(mpd_context,0,0),owner);
									if(_decimal){
										_decimal->mpd=_CORDICsine;
										outputDecimal("CORDIC sine: '",_decimal,"'.\n");
										_decimal->mpd=NULL; // so it won't get freed by free_decimal()
										FREE_DECIMAL(_decimal,owner);
									}else
										outputError("Failed to create a decimal for showing the CORDIC sine");
								}
							}else
								outputError("Failed to compute the CORDIC sine of the normalized decimal");
						}
						// free whatever we created...
						if(_xsin)free_mpd(_xsin);
					}
				}else
					outputError("Failed to create helper decimals for computing the CORDIC sine of a decimal");
				free_mpd(_xmod);free_mpd(_xtemp);free_mpd(_xquadrant);
				if(_absx)free_mpd(_absx);
				if(_CORDICsine){
					////////output("Returning the CORDIC decimal.\n");
					return _getDecimal(_CORDICsine,mpd_context->prec,0,true); // TODO I suppose this is ok, because it does not matter who disowns it
				}
			}
		}
		outputError("No decimal context to compute the CORDIC sine of a decimal in");
	}else
		outputError("No decimal to compute the CORDIC sine of");
	return NULL;
}
Mdecimal* _dcordiccosine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi||!decimalcontext->predefinedsinedeltaangle)
				FREE_DECIMAL(owned_decimal(pi_decimal(decimalcontext,true),owner),owner); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))
					return _getDecimal(__mpd(mpd_context,0),mpd_context->prec,0,true);
				uint32_t status=0;
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}}
					if(!_absx)
					{outputError("Failed to negate the decimal to compute the CORDIC cosine of");return NULL;}
					if(amVerbose())
						outputInfo("Computing the CORDIC cosine of a negative decimal.");
				}
				mpd_t* _CORDICcosine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){
						Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true),owner);
						if(_decimal){
							outputDecimal("Normalized CORDIC cosine (abs) argument: '",_decimal,"'.\n");
							FREE_DECIMAL(_decimal,owner);
						}
					}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint64_t xquadrant=mpd_qget_u64(_xquadrant,&status);
					if(amVerbose()){outputDecimal("Quadrant of CORDIC cosine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);}
					if((status&0xEFBF)!=0){
						outputError("Failed to compute the CORDIC cosine of a decimal");
						report_mpd_status(status);
					}else{
						bool cos=true; // whether to compute the sine or cosine (of the transformed angle)
						mpd_t* _xcos=NULL;
						if(xquadrant!=0){
							cos=false;
							_xcos=__mpd(mpd_context,0);
							if(_xcos){
								if(xquadrant==1)
									mpd_qsub(_xcos,decimalcontext->pi,_xmod,mpd_context,&status);
								else
								if(xquadrant==2)
									mpd_qsub(_xcos,_xmod,decimalcontext->pi,mpd_context,&status);
								else
									mpd_qsub(_xcos,decimalcontext->pimul2,_xmod,mpd_context,&status);
							}else
								status=0xFFFFFFFF;
						}
						/*
						if(mpd_qcmp(_xmod,decimalcontext->pidiv4,&status)>0){ // the remainder (in [0,pi/2) is above pi/4
							sin=false;
							_xcos=__mpd(mpd_context,0);
							if(_xcos)mpd_qsub(_xcos,decimalcontext->pidiv2,_xmod,mpd_context,&status);
						}
						*/
						if((status&0xEFBF)!=0){
							outputError("Failed to compute the CORDIC cosine of a decimal");
							if(status!=0xFFFFFFFF)report_mpd_status(status);
						}else{
							// if we want to use sine/cosine formulas using the predefined sine/cosine table we have to find the smallest difference with any of the predefined angles
							// looking up should return the nearest sincos element in the table
							_CORDICcosine=_getCORDICsinorcos(decimalcontext,(cos?_xmod:_xcos),false);
							///*
							if(_CORDICcosine){
								if(xquadrant==1||xquadrant==2){
									if(amVerbose())outputInfo("Negating the computed CORDIC cosine!");
									mpd_set_negative(_CORDICcosine); // negate the _sine
								}else
								if(amVerbose())
									outputInfo("Not negating the CORDIC cosine!");
								if(amVerbose()){
									Mdecimal* _decimal=owned_decimal(__decimal(mpd_context,0,0),owner);
									if(_decimal){
										_decimal->mpd=_CORDICcosine;
										outputDecimal("CORDIC cosine: '",_decimal,"'.\n");
										_decimal->mpd=NULL; // so it won't get freed by free_decimal()
										FREE_DECIMAL(_decimal,owner);
									}else
										outputError("Failed to create a decimal for showing the CORDIC cosine");
								}
							}else
								outputError("Failed to compute the CORDIC cosine of the normalized decimal");
						}
						// free whatever we created...
						if(_xcos)free_mpd(_xcos);
					}
				}else
					outputError("Failed to create helper decimals for computing the CORDIC cosine of a decimal");
				free_mpd(_xmod);free_mpd(_xtemp);free_mpd(_xquadrant);
				if(_absx)free_mpd(_absx);
				if(_CORDICcosine){
					////////output("Returning the CORDIC decimal.\n");
					return _getDecimal(_CORDICcosine,mpd_context->prec,0,true); // TODO doesn't matter who disowns it?
				}
			}
		}
		outputError("No decimal context to compute the CORDIC cosine of a decimal in");
	}else
		outputError("No decimal to compute the CORDIC cosine of");
	return NULL;
}

/* replacing:
// MDH@11SEP2019: with CORDIC predefined sine and cosines we can approximate the sine/cosine of an angle under pi/2 by determining the CORDIC angles that sum up to the angle we want
//                how can we make this function recursive????
mpd_t* _getCORDICsine(Mdecimalcontext* decimalcontext,mpd_t* x){
	// I guess an iterative procedure is better than a recursive procedure, because in a recursive procedure we have to keep passing the mpd_context...
	// this means I can't get down but then the problem is that I can't do the rotations until I find all the constituent angles
	// yes of course how many CORDIC angles do we have??????
	// we can't start with the smallest we have to start with 45 degrees, then 22.5 degrees etc.
	// if we have many of these angles we can use a big integer to store the booleans that determine whether or not to perform a rotation
	mpd_context_t* mpd_context=decimalcontext->mpd_context;
	Mdecimal* _intermediateResult=__decimal(mpd_context,0,0);
	Mbiginteger *_applyRotationFlags=__biginteger(),*_rotationFlag=_getBiginteger(1);
	Msincoselement* _cordicElement=decimalcontext->_firstCordicelement;
	uint32_t status=0;
	mpd_t* _xcopy=get_mpd_copy(mpd_context,x);
	// as long as the remaining angle isn't zero keep going
	// I guess I could do the rotation immediately?????
	uint32_t count=0;
	int comp;
	while(!mpd_iszero(_xcopy)&&_cordicElement){
		count++;
		comp=mpd_qcmp(_cordicElement->_angle,_xcopy,&status);
		if(_intermediateResult){
			output("Result of comparing ");
			_intermediateResult->mpd=_cordicElement->_angle;
			outputDecimal(NULL,_intermediateResult," with ");
			_intermediateResult->mpd=_xcopy;
			outputDecimal(NULL,_intermediateResult,":");
			output("%d.\n",comp);
		}
		if(comp<=0){
			if(mp_add(_applyRotationFlags,_rotationFlag,_applyRotationFlags)!=MP_OKAY)status=0xFFFFFFFF;else mpd_qsub(_xcopy,_xcopy,_cordicElement->_angle,decimalcontext->mpd_context,&status);
		}
		if(mp_mul_2(_rotationFlag,_rotationFlag)!=MP_OKAY)status=0xFFFFFFFF;else outputBiginteger("Rotation flag: ",_rotationFlag,".\n");
		_cordicElement=_cordicElement->_next;
		if((status&0xEFBF)!=0)break;
	}
	free_mpd(_xcopy);
	output("CORDIC rotation flags after checking %" PRIu32 " CORDIC angles",count);
	outputBiginteger(": ",_applyRotationFlags,".\n");
	// ok, rotation flags set, ready to perform the rotations based on the rotation flags if at least one is set
	mpd_t *_t1=__mpd(mpd_context,0),*_t2=__mpd(mpd_context,0),*_t3=__mpd(mpd_context,0),*_t4=__mpd(mpd_context,0);
	mpd_t *_resultsine=__mpd(decimalcontext->mpd_context,0),*_resultcosine=__mpd(decimalcontext->mpd_context,1); // starting at 0 degrees!!
	_cordicElement=decimalcontext->_firstCordicelement;
	while(mp_iszero(_applyRotationFlags)==MP_NO){ // as long as there are rotations to perform
		if(mp_isodd(_applyRotationFlags)==MP_YES){ // an angle to be applied
			// sin(a+b)=sin(a)cos(b)+cos(a)sin(b)=_t1+_t2
			// cos(a+b)=cos(a)cos(b)-sin(a)sin(b)=_t3+_t4
			if(_intermediateResult){
				_intermediateResult->mpd=_cordicElement->_angle;
				outputDecimal("Rotating by '",_intermediateResult,"' with ");
				_intermediateResult->mpd=_cordicElement->_sine;
				outputDecimal("sine: ",_intermediateResult," and ");
				_intermediateResult->mpd=_cordicElement->_cosine;
				outputDecimal("cosine: ",_intermediateResult,".\n");
			}
			mpd_qmul(_t1,_resultsine,_cordicElement->_cosine,mpd_context,&status);
			mpd_qmul(_t2,_resultcosine,_cordicElement->_sine,mpd_context,&status);
			mpd_qmul(_t3,_resultcosine,_cordicElement->_cosine,mpd_context,&status);
			mpd_qmul(_t4,_resultsine,_cordicElement->_sine,mpd_context,&status);
			mpd_qadd(_resultsine,_t1,_t2,mpd_context,&status);
			mpd_qsub(_resultcosine,_t3,_t4,mpd_context,&status);
		}
		if(mp_div_2(_applyRotationFlags,_applyRotationFlags)!=MP_OKAY)status=0xFFFFFFFF; // divide by 2 i.e. shift right
		_cordicElement=_cordicElement->_next;
		if((status&0xEFBF)!=0)break;
	}
	if(_intermediateResult){_intermediateResult->mpd=NULL;free_mpd(_intermediateResult);}
	return _resultsine;
}
*/

// MDH@26AUG2019: implementing computing the sine with a certain accuracy using Taylor series
Mdecimal* _dsine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi||!decimalcontext->predefinedsinedeltaangle)
				FREE_DECIMAL(owned_decimal(pi_decimal(decimalcontext,true),owner),owner); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))
					return _getDecimal(__mpd(mpd_context,0),mpd_context->prec,0,true);
				uint32_t status=0;
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}}
					if(!_absx){outputError("Failed to negate the decimal to compute the sine of");return NULL;}
					if(amVerbose())
						outputInfo("Computing the sine of a negative decimal.");
				}
				mpd_t* _sine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){
						Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true),owner);
						if(_decimal){outputDecimal("Normalized sine (abs) argument: '",_decimal,"'.\n");FREE_DECIMAL(_decimal,owner);}
					}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint64_t xquadrant=mpd_qget_u64(_xquadrant,&status);
					if(amVerbose()){
						outputDecimal("Quadrant of sine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);
					}
					if((status&0xEFBF)!=0){
						outputError("Failed to compute the sine of a decimal");
						report_mpd_status(status);
					}else{
						bool sin=true; // whether to compute the sine or cosine (of the transformed angle)
						mpd_t* _xsin=NULL;
						if(xquadrant!=0){
							sin=false;
							_xsin=__mpd(mpd_context,0);
							if(_xsin){
								if(xquadrant==1)
									mpd_qsub(_xsin,decimalcontext->pi,_xmod,mpd_context,&status);
								else
								if(xquadrant==2)
									mpd_qsub(_xsin,_xmod,decimalcontext->pi,mpd_context,&status);
								else
									mpd_qsub(_xsin,decimalcontext->pimul2,_xmod,mpd_context,&status);
							}else
								status=0xFFFFFFFF;
						}
						/*
						if(mpd_qcmp(_xmod,decimalcontext->pidiv4,&status)>0){ // the remainder (in [0,pi/2) is above pi/4
							sin=false;
							_xcos=__mpd(mpd_context,0);
							if(_xcos)mpd_qsub(_xcos,decimalcontext->pidiv2,_xmod,mpd_context,&status);
						}
						*/
						if((status&0xEFBF)!=0){
							outputError("Failed to compute the sine of a decimal");
							if(status!=0xFFFFFFFF)report_mpd_status(status);
						}else{
							// if we want to use sine/cosine formulas using the predefined sine/cosine table we have to find the smallest difference with any of the predefined angles
							// looking up should return the nearest sincos element in the table
							mpd_t* _CORDICsine=_getCORDICsinorcos(decimalcontext,x->mpd,true);
							if(_CORDICsine){
								Mdecimal* _decimal=(amVerbose()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
								if(_decimal){
									_decimal->mpd=_CORDICsine;
									outputDecimal("CORDIC sine: '",_decimal,"'.\n");
									_decimal->mpd=NULL; // so it won't get freed by free_decimal()
									FREE_DECIMAL(_decimal,owner);
								}else
								if(amVerbose())
									outputError("Failed to create a decimal for showing the CORDIC sine");
								free_mpd(_CORDICsine);
							}
							/*
							mpd_relative_angle_t* _relativeAngle=_getRelativeAngle(decimalcontext,x->mpd);
							if(_relativeAngle){
								if(amVerbose()){
									output("Relative angle: ");
									Mdecimal* _decimal=__decimal(mpd_context,0,0);
									if(_decimal){
										_decimal->mpd=_relativeAngle->sincoselement->_angle;
										outputDecimal(NULL,_decimal,NULL);
										outputChar(_relativeAngle->negative?'-':'+');
										_decimal->mpd=_relativeAngle->_delta_angle;
										outputDecimal(NULL,_decimal,NULL);
										_decimal->mpd=NULL; // so it won't get freed by free_decimal()
										free_decimal(_decimal);
									}else
										outputChar('?');
									outputInfo(".");
								}
								// with the relative angle we can compute the sine using sin(a+/-b)=sin(a)cos(b)+/-sin(b)cos(a)
								// which consists of two terms that need to be added or subtracted
								mpd_t *_term1=get_mpd_copy(mpd_context,_relativeAngle->sincoselement->_sine),*_term2=get_mpd_copy(mpd_context,_relativeAngle->sincoselement->_cosine);
								if(_term1&&_term2){
									Mdecimal* _decimal=(amVerbose()?__decimal(mpd_context,0,0):NULL);
									if(_decimal){
										output("Predefined angle #%" PRIu32 ":",_relativeAngle->sincoselement->mult);
										_decimal->mpd=_relativeAngle->sincoselement->_angle;outputDecimal("'",_decimal,"'");
										_decimal->mpd=_term1;outputDecimal(" with cosine '",_decimal,"'");
										_decimal->mpd=_term2;outputDecimal(" and sine '",_decimal,"'.\n");
									}
									// we still need the sine and cosine of the delta angle
									// BUT in order to be able to compare the results we should I guess use the _dsinsquared 
									mpd_t *_deltasin=__mpd(mpd_context,0),*_deltacos=__mpd(mpd_context,0);
									if(_deltasin&&_deltacos){
										mpd_t* _deltasinsquared=_dsinsquared(mpd_context,_relativeAngle->_delta_angle);
										if(_deltasinsquared){
											mpd_qsqrt(_deltasin,_deltasinsquared,mpd_context,&status);
											mpd_qsub_u32(_deltasinsquared,_deltasinsquared,1,mpd_context,&status);mpd_set_positive(_deltasinsquared);
											mpd_qsqrt(_deltacos,_deltasinsquared,mpd_context,&status);
											// ready to 'rotate'
											mpd_qmul(_term1,_term1,_deltacos,mpd_context,&status);
											mpd_qmul(_term2,_term2,_deltasin,mpd_context,&status);
											if(_decimal){_decimal->mpd=_term1;outputDecimal("First term: '",_decimal,"'.");_decimal->mpd=_term2;outputDecimal(" Second term: '",_decimal,"'.\n");_decimal->mpd=NULL;}
											if(_relativeAngle->negative)mpd_qsub(_term1,_term1,_term2,mpd_context,&status);else mpd_qadd(_term1,_term1,_term2,mpd_context,&status);
											if(_decimal){_decimal->mpd=_term1;outputDecimal("Relative angle sine: ",_decimal,".\n");}
										}
										free_mpd(_deltasinsquared);
									}
									free_mpd(_deltasin);free_mpd(_deltacos);
									if(_decimal){_decimal->mpd=NULL;free_decimal(_decimal);}
								}
								free_mpd(_term1);free_mpd(_term2);
								free_mpd_relative_angle(_relativeAngle);
							}else
								outputError("Failed to compute the relative angle");
							*/
							// MDH@11SEP2019: the following is preferred because we have precomputed 257 equidistant angle sines between 0 and pi/2
							mpd_relative_angle_t* _predefinedSinesRelativeAngle=owned_mpd_relative_angle(_getPredefinedSinesRelativeAngle(decimalcontext,x->mpd),owner);
							if(_predefinedSinesRelativeAngle){
								if(amVerbose()){
									output("Predefined sines relative angle: ");
									Mdecimal* _decimal=owned_decimal(__decimal(mpd_context,0,0),owner);
									if(_decimal){
										_decimal->mpd=_predefinedSinesRelativeAngle->sincoselement->_angle;
										outputDecimal(NULL,_decimal,NULL);
										outputChar(_predefinedSinesRelativeAngle->negative?'-':'+');
										_decimal->mpd=_predefinedSinesRelativeAngle->_delta_angle;
										outputDecimal(NULL,_decimal,NULL);
										_decimal->mpd=NULL; // so it won't get freed by free_decimal()
										FREE_DECIMAL(_decimal,owner);
									}else
										outputChar('?');
									outputInfo(".");
								}
								// with the relative angle we can compute the sine using sin(a+/-b)=sin(a)cos(b)+/-sin(b)cos(a)
								// which consists of two terms that need to be added or subtracted
								mpd_t *_term1=get_mpd_copy(mpd_context,_predefinedSinesRelativeAngle->sincoselement->_sine),*_term2=get_mpd_copy(mpd_context,_predefinedSinesRelativeAngle->sincoselement->_cosine);
								if(_term1&&_term2){
									Mdecimal* _decimal=(amVerbose()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
									if(_decimal){
										output("Predefined sines angle #%" PRIu32 ":",_predefinedSinesRelativeAngle->sincoselement->mult);
										_decimal->mpd=_predefinedSinesRelativeAngle->sincoselement->_angle;outputDecimal("'",_decimal,"'");
										_decimal->mpd=_term1;outputDecimal(" with cosine '",_decimal,"'");
										_decimal->mpd=_term2;outputDecimal(" and sine '",_decimal,"'.\n");
									}
									// we still need the sine and cosine of the delta angle
									// BUT in order to be able to compare the results we should I guess use the _dsinsquared 
									mpd_t *_deltasin=__mpd(mpd_context,0),*_deltacos=__mpd(mpd_context,0);
									if(_deltasin&&_deltacos){
										mpd_t* _deltasinsquared=_dsinsquared(mpd_context,_predefinedSinesRelativeAngle->_delta_angle);
										if(_deltasinsquared){
											mpd_qsqrt(_deltasin,_deltasinsquared,mpd_context,&status);
											mpd_qsub_u32(_deltasinsquared,_deltasinsquared,1,mpd_context,&status);mpd_set_positive(_deltasinsquared);
											mpd_qsqrt(_deltacos,_deltasinsquared,mpd_context,&status);
											// ready to 'rotate'
											mpd_qmul(_term1,_term1,_deltacos,mpd_context,&status);
											mpd_qmul(_term2,_term2,_deltasin,mpd_context,&status);
											if(_decimal){_decimal->mpd=_term1;outputDecimal("First term: '",_decimal,"'.");_decimal->mpd=_term2;outputDecimal(" Second term: '",_decimal,"'.\n");_decimal->mpd=NULL;}
											if(_predefinedSinesRelativeAngle->negative)mpd_qsub(_term1,_term1,_term2,mpd_context,&status);else mpd_qadd(_term1,_term1,_term2,mpd_context,&status);
											if(_decimal){_decimal->mpd=_term1;outputDecimal("Predefined sines relative angle sine: ",_decimal,".\n");}
										}
										free_mpd(_deltasinsquared);
									}
									free_mpd(_deltasin);free_mpd(_deltacos);
									if(_decimal){_decimal->mpd=NULL;FREE_DECIMAL(_decimal,owner);}
								}
								free_mpd(_term1);free_mpd(_term2);
								FREE_MPD_RELATIVE_ANGLE(_predefinedSinesRelativeAngle,owner);
							}else
								outputError("Failed to compute the predefined sines relative angle");

							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // approximate with two additional digits
							_sine=_dsinsquared(mpd_context,(sin?_xmod:_xsin));
							if(_sine)mpd_qsqrt(_sine,_sine,mpd_context,&status); // BEFORE returning to the original precision!!!
							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // reset precision (TODO not thread-safe if we would be running multiple versions using the same mpd_context!!!!)
							if(_sine)mpd_qfinalize(_sine,mpd_context,&status);else status=0xFFFFFFFF; // round to the original precision (NOTE if sqrt failed we didn't have to do this though!!!)
							// some extra work as we've received the sine squared
							if((status&0xEFBF)!=0){
								outputError("Failed to compute the cosine from the sine square approximation");
								free_mpd(_sine);
								_sine=NULL;
							}
							/* replacing:

							_sine=_dsquarerootofsinorcossquared(mpd_context,(sin?_xmod:_xsin),true);
							*/
							/* replacing what worked just fine:
							mpd_sincos_t* _sinandcos=_dsinandcos(mpd_context,(sin?_xmod:_xsin));
							if(_sinandcos){
								_sine=_sinandcos->sin; // we will negate if if need be below!!!
								_sinandcos->sin=NULL;
								free_mpd_sincos(_sinandcos);
							}
							*/
							/* replacing:
							_sine=_dsincos(mpd_context,(sin?_xmod:_xcos),sin);
							*/
							if(_sine){
								if((_absx!=NULL)!=(xquadrant==2||xquadrant==3)){ // NOTE equivalent to using the ^ bitwise operator!!!
									if(amVerbose())outputInfo("Negating the computed sine!");
									mpd_set_negative(_sine); // negate the _sine
								}else
								if(amVerbose())
									outputInfo("Not negating the sine!");
							}else
								outputError("Failed to compute the sine of the normalized decimal");
						}
						// free whatever we created...
						if(_xsin)free_mpd(_xsin);
					}
				}else
					outputError("Failed to create helper decimals for computing the sine of a decimal");
				free_mpd(_xmod);free_mpd(_xtemp);free_mpd(_xquadrant);
				if(_absx)free_mpd(_absx);
				if(_sine)return _getDecimal(_sine,mpd_context->prec,0,true); // TODO does it matter who disowns it?				
			}
		}else
			outputError("No decimal context to compute the sine of a decimal in");
	}else
		outputError("No decimal to compute the sine of");
	return NULL;
}

Mdecimal* _dcosine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi||!decimalcontext->predefinedsinedeltaangle)
				FREE_DECIMAL(owned_decimal(pi_decimal(decimalcontext,true),owner),owner); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))
					return _getDecimal(__mpd(mpd_context,1),mpd_context->prec,0,true); // TODO ?
				uint32_t status=0;
				// with the cosine we can forget about the sign i.e. cos(-x)=cos(x), which means we can simply ignore the sign
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0)
					{free_mpd(_absx);_absx=NULL;}}
					if(!_absx)
					{outputError("Failed to negate the decimal to compute the cosine of");return NULL;}
					if(amVerbose())
						outputInfo("Computing the cosine of a negative decimal.");
				}
				mpd_t* _cosine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){
						Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true),owner);
						if(_decimal){
							outputDecimal("Normalized cosine (abs) argument: '",_decimal,"'.\n");
							FREE_DECIMAL(_decimal,owner);
						}
					}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint64_t xquadrant=mpd_qget_u64(_xquadrant,&status);
					if(amVerbose())
						outputDecimal("Quadrant of cosine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);
					if((status&0xEFBF)!=0){
						outputError("Failed to compute the cosine of a decimal");
						report_mpd_status(status);
					}else{
						bool cos=true; // whether to compute the sine or cosine (of the transformed angle)
						mpd_t* _xcos=NULL;
						if(xquadrant!=0){
							cos=false;
							_xcos=__mpd(mpd_context,0);
							if(_xcos){
								if(xquadrant==1)
									mpd_qsub(_xcos,decimalcontext->pi,_xmod,mpd_context,&status);
								else
								if(xquadrant==2)
									mpd_qsub(_xcos,_xmod,decimalcontext->pi,mpd_context,&status);
								else
									mpd_qsub(_xcos,decimalcontext->pimul2,_xmod,mpd_context,&status);
							}else
								status=0xFFFFFFFF;
						}
						/*
						if(mpd_qcmp(_xmod,decimalcontext->pidiv4,&status)>0){  // the remainder (in [0,pi/2) is above pi/4
							cos=false;
							_xsin=__mpd(mpd_context,0);
							if(_xsin)mpd_qsub(_xsin,decimalcontext->pidiv2,_xmod,mpd_context,&status);
						}
						*/
						if((status&0xEFBF)!=0){
							outputError("Failed to compute the cosine of a decimal");
							report_mpd_status(status);
						}else{
							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // approximate with two additional digits
							_cosine=_dsinsquared(mpd_context,(cos?_xmod:_xcos));
							// some extra work as we've received the sine squared
							if(_cosine){
								mpd_qsub_u32(_cosine,_cosine,1,mpd_context,&status); // subtract 1 (assuming the value we received does never exceed 1 so the resulting value should be negative!!!)
								if(!mpd_ispositive(_cosine)){ // negated cosine should be 0 or negative
									mpd_set_positive(_cosine); // toggle the sign
									mpd_qsqrt(_cosine,_cosine,mpd_context,&status);
								}else{
									status=0xFFFFFFFF;
									outputBug("Invalid squared sine computed."); // TODO do something better with bugs!!!
								}
							}
							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // approximate with two additional digits
							if(_cosine)mpd_qfinalize(_cosine,mpd_context,&status); // round to the 'original' precision
							if((status&0xEFBF)!=0){
								outputError("Failed to compute the cosine from the sine square approximation");
								free_mpd(_cosine);
								_cosine=NULL;
							}
							/* replacing:
							_cosine=_dsquarerootofsinorcossquared(mpd_context,(cos?_xmod:_xcos),false);
							*/
							/* replacing what worked just fine
							mpd_sincos_t* _sinandcos=_dsinandcos(mpd_context,(cos?_xmod:_xcos));
							if(_sinandcos){
								_cosine=_sinandcos->cos;
								_sinandcos->cos=NULL; // so it won't get free when we free _sinandcos
								free_mpd_sincos(_sinandcos);
							}
							*/
							// replacing: _cosine=_dsincos(mpd_context,(cos?_xmod:_xsin),!cos);
							if(_cosine){
								// whether _absx is NULL or not doesn't matter as cos(-x)=cos(x)
								if(xquadrant==1||xquadrant==2)mpd_set_negative(_cosine); // negate the _cosine in quadrant 1 and 2 (from pi/2 to 3*pi/2)
							}else
								outputError("Failed to compute the cosine of the normalized decimal");
						}
						// free whatever we created...
						if(_xcos)free_mpd(_xcos);
					}
				}else
					outputError("Failed to create helper decimals for computing the cosine of a decimal");
				free_mpd(_xmod);free_mpd(_xtemp);free_mpd(_xquadrant);
				if(_absx)free_mpd(_absx);
				return(_cosine?_getDecimal(_cosine,mpd_context->prec,0,true):NULL);				
			}
		}
		outputError("No decimal context to compute the cosine of a decimal in");
	}else
		outputError("No decimal to compute the cosine of");
	return NULL;
}

// MDH@17SEP2019: computing the tangens uses _dsinsquared just like _dsine and _dcosine do
Mdecimal* _dtangent(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi||!decimalcontext->predefinedsinedeltaangle)
				FREE_DECIMAL(owned_decimal(pi_decimal(decimalcontext,true),owner),owner); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))
					return _getDecimal(__mpd(mpd_context,0),mpd_context->prec,0,true); // TODO ?
				uint32_t status=0;
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){
						mpd_qabs(_absx,x->mpd,mpd_context,&status);
						if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}
					}
					if(!_absx){
						outputError("Failed to negate the decimal to compute the sine of");return NULL;
					}
					if(amVerbose())
						outputInfo("Computing the sine of a negative decimal.");
				}
				mpd_t* _tan=NULL; // the end result
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){
						Mdecimal* _decimal=owned_decimal(_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true),owner);
						if(_decimal){outputDecimal("Normalized sine (abs) argument: '",_decimal,"'.\n");FREE_DECIMAL(_decimal,owner);}
					}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint64_t xquadrant=mpd_qget_u64(_xquadrant,&status);
					if(amVerbose())
						outputDecimal("Quadrant of sine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);
					if((status&0xEFBF)!=0){
						outputError("Failed to compute the sine of a decimal");
						report_mpd_status(status);
					}else{
						bool sin=true; // whether to compute the sine or cosine (of the transformed angle)
						mpd_t* _xsin=NULL;
						if(xquadrant!=0){
							sin=false;
							_xsin=__mpd(mpd_context,0);
							if(_xsin){
								if(xquadrant==1)
									mpd_qsub(_xsin,decimalcontext->pi,_xmod,mpd_context,&status);
								else
								if(xquadrant==2)
									mpd_qsub(_xsin,_xmod,decimalcontext->pi,mpd_context,&status);
								else
									mpd_qsub(_xsin,decimalcontext->pimul2,_xmod,mpd_context,&status);
							}else
								status=0xFFFFFFFF;
						}
						if((status&0xEFBF)!=0){
							outputError("Failed to compute the sine of a decimal");
							if(status!=0xFFFFFFFF)report_mpd_status(status);
						}else{

							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // approximate with two additional digits

							_tan=__mpd(mpd_context,1); // if we set _tan to 1 we can use it BEFORE actually setting it to the result!!!
							if(_tan){
								mpd_t* _sinsquared=_dsinsquared(mpd_context,(sin?_xmod:_xsin));
								if(_sinsquared){
									// the tangent is the squareroot of (_sine/1-sine)=1/(1-_sine)-1
									// the last part allows us to re-use _sine
									mpd_qsub_i32(_sinsquared,_sinsquared,1,mpd_context,&status);
									mpd_set_positive(_sinsquared); // denominator 1-sine
									mpd_qdiv(_sinsquared,_tan,_sinsquared,mpd_context,&status);
									mpd_qsub_i32(_sinsquared,_sinsquared,1,mpd_context,&status);
									mpd_qsqrt(_tan,_sinsquared,mpd_context,&status);
									free_mpd(_sinsquared);
								}else
									outputError("Failed to compute the squared-sine of a decimal.");
							}else
								outputError("Failed to create the decimal for storing the tangent.");
							mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // reset precision (TODO not thread-safe if we would be running multiple versions using the same mpd_context!!!!)
							if(_tan){
								if(amVerbose()){
									Mdecimal* _decimal=owned_decimal(__decimal(mpd_context,0,0),owner);
									if(_decimal){
										_decimal->mpd=_tan;
										outputDecimal("Tangent: ",_decimal,"'.\n");
										_decimal->mpd=NULL;
										FREE_DECIMAL(_decimal,owner);
									}
								}
								mpd_qfinalize(_tan,mpd_context,&status);
							}else 
								status=0xFFFFFFFF; // round to the original precision (NOTE if sqrt failed we didn't have to do this though!!!)
							// some extra work as we've received the sine squared
							if((status&0xEFBF)!=0)
							{outputError("Failed to compute the tangent from the sine square approximation");free_mpd(_tan);_tan=NULL;}

							if(_tan){
								if((_absx!=NULL)!=(xquadrant==1||xquadrant==3)){ // NOTE equivalent to using the ^ bitwise operator!!!
									if(amVerbose())outputInfo("Negating the computed tangent!");
									mpd_set_negative(_tan); // negate the _sine
								}else
								if(amVerbose())
									outputInfo("Not negating the tangent!");
							}else
								outputError("Failed to compute the tangent of the normalized decimal");
						}
						// free whatever we created...
						if(_xsin)free_mpd(_xsin);
					}
				}else
					outputError("Failed to create helper decimals for computing the tangent of a decimal");
				free_mpd(_xmod);free_mpd(_xtemp);free_mpd(_xquadrant);
				if(_absx)free_mpd(_absx);
				return(_tan?_getDecimal(_tan,mpd_context->prec,0,true):NULL);				
			}
		}
		outputError("No decimal context to compute the tangent of a decimal");
	}else
		outputError("No decimal to compute the tangent of");
	return NULL;
}

Mdecimal* _dexp(Mdecimalcontext const * decimalcontext,Mdecimal const * const x){Mallocationowner owner=getOwner(__LINE__);
	if(x){
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		mpd_context_t* mpd_context=decimalcontext->mpd_context;
		if(mpd_context){
			mpd_t* _exp=__mpd(mpd_context,1);
			if(!mpd_iszero(x->mpd)){
				if(_exp){
					Mdecimal* _intermediateResult=(amVerbose()?owned_decimal(__decimal(mpd_context,0,0),owner):NULL);
					mpd_t *_num=get_mpd_copy(mpd_context,x->mpd),*_den=__mpd(mpd_context,1),*_add=get_mpd_copy(mpd_context,x->mpd),*_i=__mpd(mpd_context,1),*_prevexp=get_mpd_copy(mpd_context,_exp); // a copy of what we need to use
					if(_num&&_den&&_add&&_i&&_prevexp){
						uint32_t status=0;
						unsigned long long iteration=0;
						while((status&0xEFBF)==0){
							mpd_qadd(_exp,_prevexp,_add,mpd_context,&status);
							if(_intermediateResult){
								iteration++;
								output("Iteration #%llu: ",iteration);
								mpd_qcopy(_intermediateResult->mpd,_exp,&status);
								outputDecimal("Exp: '",_intermediateResult,"'\n.");
							}
							// are we done????
							if(mpd_cmp(_exp,_prevexp,mpd_context)==0){if(amVerbose())outputDecimal("Done approximating exp(",x,").\n");break;}
							mpd_qcopy(_prevexp,_exp,&status);
							// update the helpers
							mpd_qadd_u32(_i,_i,1,mpd_context,&status);
							mpd_qmul(_den,_den,_i,mpd_context,&status);
							mpd_qmul(_num,_num,x->mpd,mpd_context,&status);
							mpd_qdiv(_add,_num,_den,mpd_context,&status);
						}
						if((status&0xEFBF)!=0){outputError("Something went wrong in executing dexp()");free_mpd(_exp);_exp=NULL;}
					}else
						outputError("Failed to create all dexp() execution helper decimals");
					if(_intermediateResult)FREE_DECIMAL(_intermediateResult,owner);
					free_mpd(_prevexp);
					free_mpd(_i);
					free_mpd(_num);
					free_mpd(_den);
					free_mpd(_add);
				}else
					outputError("Failed to initialize the result of dexp()");
			}else
			if(amVerbose())outputInfo("Zero argument to exp() approximation.");
			if(_exp)
				return _getDecimal(_exp,mpd_context->prec,0,true);
		}else
			outputError("No decimal context available for use in dexp().");
	}
	return NULL;
}

Mdecimal* _getInverseDecimal(Mdecimal const * const decimal){Mallocationowner owner=getOwner(__LINE__);
	if(decimal){
		Mdecimalcontext* decimalcontext=_getDecimalcontext(decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
		Mdecimal* _inverseDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
		if(_inverseDecimal){
			mpd_t* _mpd1=__mpd(mpd_context,1);
			if(_mpd1){
				uint32_t status=0;
				mpd_qdiv(_inverseDecimal->mpd,_mpd1,decimal->mpd,mpd_context,&status);
				free_mpd(_mpd1);
				if((status&0xEFBF)){
					FREE_DECIMAL(_inverseDecimal,owner);
					outputError("Failed to compute the reciprocal of a decimal");
					return NULL;
				}
			}
			return disowned_decimal(_inverseDecimal,owner);
		}
	}
	return NULL;
}

long long isDecimalUndefined(Mdecimal const * const decimal){return(decimal?(decimal->mpd?M_FALSE:M_TRUE):M_TRUE);} // used by the sign and sign testing functions
long long getDecimalSign(Mdecimal const * const decimal){return(isDecimalUndefined(decimal)==M_FALSE?(mpd_iszero(decimal->mpd)?M_ZERO:(mpd_ispositive(decimal->mpd)?M_POSITIVE:M_NEGATIVE)):M_LL_INVALID);}
long long isDecimalZero(Mdecimal const * const decimal){long long decimalSign=getDecimalSign(decimal);return(decimalSign!=M_LL_INVALID?(decimalSign==M_ZERO?M_TRUE:M_FALSE):M_LL_INVALID);}
long long isDecimalPositive(Mdecimal const * const decimal){long long decimalSign=getDecimalSign(decimal);return(decimalSign!=M_LL_INVALID?(decimalSign==M_POSITIVE?M_TRUE:M_FALSE):M_LL_INVALID);}
long long isDecimalNegative(Mdecimal const * const decimal){long long decimalSign=getDecimalSign(decimal);return(decimalSign!=M_LL_INVALID?(decimalSign==M_NEGATIVE?M_TRUE:M_FALSE):M_LL_INVALID);}

// MDH@25AUG2019: I'd see that decimalOne should be an Mdecimal? we can leave it the way it is for now but instead require the context passed to __mpd to be non-NULL!! i.e. __mpd does no longer default to _decimalContext
static mpd_t* decimalOne=NULL;
// getDecimalOne() return a decimal but this is a decimal that should never be freed
const mpd_t* getDecimalOne(){
	if(!decimalOne)decimalOne=__mpd(M_DECIMALCONTEXT->mpd_context,1);
	return decimalOne;
}/* VALIDATED */
long long isDecimalOne(Mdecimal const * const decimal){
	long long result=M_LL_INVALID;
	if(isDecimalUndefined(decimal)==M_FALSE&&decimal->repeating==0){
	    // MDH@17JUN2019: something that is repeating is definitely not equal to 1 (TODO unless it's 0.[9])
		uint32_t status=0;
		int cmpresult=mpd_qcmp(decimal->mpd,getDecimalOne(),&status);
		if((status&0xEFBF)==0)result=(cmpresult==0?M_TRUE:M_FALSE);else output("%sFailed to determine whether a decimal equals 1 (status: %" PRIu32 ").\n",M_ERROR_PREFIX,status);
	}
	return result;
}/* VALIDATED */

// MDH@18OCT2019: assuming that \p decimal already is rounded somehow to the given integer using ceil, floor, trunc or round
//                so that we should simply remove the fractional part
long long decimal2long(Mdecimal* decimal){
	uint32_t status=0;
	int64_t ll=mpd_qget_i64(decimal->mpd,&status);
	if((status&0xEFBF)==0)return ll;
	return M_LL_INVALID;
}

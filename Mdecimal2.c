#include "Mdecimal.h"

#include "Malloc.h"
#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

extern long double const M_LD_NAN;
extern const char* const ERROR_PREFIX; // TODO rename to M_ERROR_PREFIX
extern /*const*/ Mdecimalcontext* M_DECIMALCONTEXT;

mpd_t* get_mpd_copy(const mpd_context_t* mpd_context,mpd_t* mpd){
	if(!mpd)return NULL;
	if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context;
	mpd_t* _mpd=__mpd(mpd_context,0);
	if(_mpd){uint32_t status=0;mpd_qcopy(_mpd,mpd,&status);if((status&0xEFBF)!=0){free_mpd(_mpd);_mpd=NULL;}}
	return _mpd;
}

mpd_context_t* __mpd_context(mpd_ssize_t decimalprecision){
	mpd_context_t* _mpd_context=(mpd_context_t*)calloc(1,sizeof(mpd_context_t));
	if(_mpd_context){
		if(amVerbose())output("New context with precision %lld created.\n",decimalprecision);
		// initialize the new context to the default context (specification)
		mpd_init(_mpd_context,decimalprecision);
		if(amVerbose())output("Context with precision %u initialized.\n",mpd_getprec(_mpd_context));
	}else
		outputError("Failed to create a new context");
	return _mpd_context;
}

void free_decimalcontext(Mdecimalcontext* _decimalcontext){
	if(_decimalcontext->mpd_context)FREE(_decimalcontext->mpd_context,'c');
	if(_decimalcontext->pi)free_mpd(_decimalcontext->pi);
	if(_decimalcontext->pimul2)free_mpd(_decimalcontext->pimul2);
	if(_decimalcontext->pidiv2)free_mpd(_decimalcontext->pidiv2);
	if(_decimalcontext->pidiv4)free_mpd(_decimalcontext->pidiv4);
	if(_decimalcontext->e)free_mpd(_decimalcontext->e);
	FREE(_decimalcontext,'C');
}

// if we want to keep a list of all decimal contexts, we need to be able to iterate over all decimal contexts to see if it is already there
// create a single Mdecimalcontextelement
typedef struct MdecimalcontextElement{
	Mdecimalcontext* _decimalcontext;
	struct MdecimalcontextElement* _next;
}MdecimalcontextElement;
void free_decimalcontextElement(MdecimalcontextElement* _decimalcontextElement){
	if(!_decimalcontextElement)return;
	if(_decimalcontextElement->_next)free_decimalcontextElement(_decimalcontextElement->_next); // free whatever it is pointing to
	if(_decimalcontextElement->_decimalcontext)free_decimalcontext(_decimalcontextElement->_decimalcontext); // free whatever decimal context it is referring to
	FREE(_decimalcontextElement,'C');
}
static MdecimalcontextElement *_firstDecimalcontextElement=NULL,*_lastDecimalcontextElement=NULL;
// to get the unique decimal context with the requested precision
Mdecimalcontext* _getDecimalcontext(mpd_ssize_t prec){
	// do we already have it?
	if(prec<6)return NULL; // prec needs to be at least 6
	MdecimalcontextElement *decimalcontextElement=_firstDecimalcontextElement;
	while(decimalcontextElement&&decimalcontextElement->_decimalcontext->mpd_context->prec!=prec)decimalcontextElement=decimalcontextElement->_next;
	if(!decimalcontextElement){ // not existing
		decimalcontextElement=CALLOC(1,sizeof(MdecimalcontextElement),'C');
		if(decimalcontextElement){
			decimalcontextElement->_decimalcontext=CALLOC(1,sizeof(Mdecimalcontext),'c');
			if(decimalcontextElement->_decimalcontext){
				decimalcontextElement->_decimalcontext->mpd_context=__mpd_context(prec);
				if(decimalcontextElement->_decimalcontext->mpd_context){
					if(_lastDecimalcontextElement)_lastDecimalcontextElement->_next=decimalcontextElement;
					_lastDecimalcontextElement=decimalcontextElement;
					if(!_firstDecimalcontextElement)_firstDecimalcontextElement=_lastDecimalcontextElement;
				}else{
					free_decimalcontextElement(decimalcontextElement);decimalcontextElement=NULL;
				}
			}else{
				free_decimalcontextElement(decimalcontextElement);decimalcontextElement=NULL;
			}
		}else
			output("%sFailed to create the decimal context with precision " PRIu64 ".\n",ERROR_PREFIX,prec);
	}
	return(decimalcontextElement?decimalcontextElement->_decimalcontext:NULL);
}

// MDH@25AUG2019: I'd see that decimalOne should be an Mdecimal? we can leave it the way it is for now but instead require the context passed to __mpd to be non-NULL!! i.e. __mpd does no longer default to _decimalContext
static mpd_t* decimalOne=NULL;
// getDecimalOne() return a decimal but this is a decimal that should never be freed
const mpd_t* getDecimalOne(){if(!decimalOne)decimalOne=__mpd(M_DECIMALCONTEXT->mpd_context,1);return decimalOne;}/* VALIDATED */
bool isDecimalOne(Mdecimal* _decimal){
    // MDH@17JUN2019: something that is repeating is definitely not equal to 1 (TODO unless it's 0.[9])
    return (_decimal->repeating&&mpd_cmp(_decimal->mpd,getDecimalOne(),M_DECIMALCONTEXT->mpd_context)==MP_EQ);
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
            char* _decimalChars=mpd_format(_decimal->mpd,(fixedpoint||_decimal->repeating?"f":"g"),M_DECIMALCONTEXT->mpd_context);
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

/**
 * \brief returns a copy of \p _decimal
 * \param _decimal the decimal to copy
 * \return a copy of \p _decimal on success, or NULL otherwise
 */
Mdecimal* _getDecimalCopy(Mdecimal* _decimal){
	Mdecimalcontext* decimalcontext=(_decimal?_getDecimalcontext(_decimal->prec):NULL);
    if(!decimalcontext)return NULL;
	mpd_t* _mpd=get_mpd_copy(decimalcontext->mpd_context,_decimal->mpd); // make a copy
	return(_mpd?_getDecimal(_mpd,_decimal->prec,_decimal->repeating,true):NULL); // if failing to wrap the mpd copy free it
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
		outputLine("No decimal context status.");
}

bool mpd_error(const mpd_context_t* const mpd_context){return(mpd_getstatus(mpd_context)&0xEFBF)!=0;}

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
                    output("%sFailed to return a decimal context with precision %u.\n",ERROR_PREFIX,decimalprecision);
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
mpd_t* __mpd(const mpd_context_t* mpd_context,int64_t value){
    mpd_t* _mpd=NULL;
	if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context;
    // using mpd_qnew over mpd_new because we want to return NULL on failure!!!
    if(mpd_context){
        _mpd=mpd_qnew();
        // NOTE it is essential to initialize the stored value even when 0 as we would otherwise get errors on mpd_to_sci calls
        if(_mpd){
            mpd_set_i64(_mpd,value,mpd_context);
            if(_mpd->len==0){
                output("%sFailed to create decimal with value " PRId64 ".\n",ERROR_PREFIX,value);
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
void free_mpd(mpd_t* _mpd){if(_mpd)mpd_del(_mpd);}/* VALIDATED */

/**
 * \brief frees \p decimal, delegating to free_mpd() for freeing the contained mpdecimal instance
 */
void free_decimal(Mdecimal* decimal){
    if(decimal){
        if(amVerbose())output("Freeing decimal.\n");
        if(decimal->mpd)free_mpd(decimal->mpd);else outputError("No data in decimal to free");
        FREE(decimal,'D');
    }else
    if(amVerbose())outputError("No decimal to free");
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
Mdecimal* __decimal(const mpd_context_t* mpd_context,int64_t value,uint64_t repeating){
    Mdecimal* _decimal=NULL;
    if(!mpd_context)mpd_context=M_DECIMALCONTEXT->mpd_context; // use the application-wide decimal context if no context is defined
    if(mpd_context){
        _decimal=__adecimal(); // get an uninitialized decimal
        if(_decimal){
            _decimal->mpd=__mpd(mpd_context,value); // initialize to zero by default
            if(_decimal->mpd){
                _decimal->repeating=repeating;
                _decimal->prec=mpd_context->prec;
            }else{
                FREE(_decimal,'D');
                _decimal=NULL;
            }
        }else
            outputError("Failed to create a decimal"); // TODO make an out of memory error out of this
    }else
        outputError("No context to create decimal in");
    return _decimal;
}/* VALIDATED */

// MDH@29AUG2019: as mpd_t does not itself store the precision with which the decimal was created AND an Mdecimal* does store the precision I've added the prec parameter
/**
 * \brief returns a decimal with mpdecimal instance with the default decimal context equal to \p mpd and number of repeating digits equal to \p repeating, freeing the _mpd on failure
 */
Mdecimal* _getDecimal(mpd_t* mpd,mpd_ssize_t prec,uint64_t repeating,bool freeonfailure){
    if(!mpd)return NULL; // can do this as won't have to free mpd anyway
    Mdecimal* _decimal=__adecimal(); // always using the default decimal context
    if(_decimal){_decimal->mpd=mpd;_decimal->prec=prec;}else if(freeonfailure)free_mpd(mpd);
    return _decimal;
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
            mpd_set_string(decimal->mpd,decimalText,M_DECIMALCONTEXT->mpd_context); // NOTE here we have to pass in the default decimal context
            if(!decimal->mpd){free_decimal(decimal);decimal=NULL;} // if we failed to get a mpdecimal instance from the text, the text is probably wrong!!!
        }
        if(!decimal)output("%sFailed to create a decimal from '%s'.\n",ERROR_PREFIX,decimalText);
    }else
        outputError("No decimal text to parse");
    return decimal;
}/* VALIDATED */

// END BASE STUFF

Mdecimal* pi_decimal(Mdecimalcontext* decimalcontext){

	// if decimalContext equals NULL use the global decimal context, in _decimalContext
	if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;

	mpd_context_t* mpd_context=decimalcontext->mpd_context;

	mpd_ssize_t decimalprecision=mpd_context->prec;
	if(decimalprecision<=0){outputError("Cannot approximate pi: no decimal context available");return NULL;}

	Mdecimal* _decimal=NULL; // the result

	// if pi already exists in the given decimal context return it, but we need to wrap a copy in the decimal
	if(!decimalcontext->pi){

		if(amVerbose())output("Computing pi to %lld decimals.\n",decimalprecision);

		// initialize the variables we need for the iterations
#ifdef __ADEBUG__
		Mdecimal *lasts=__decimal(mpd_context,0,0),*t=__decimal(mpd_context,3,0),*s=__decimal(mpd_context,3,0),*n=__decimal(mpd_context,1,0),*na=__decimal(mpd_context,0,0),*d=__decimal(mpd_context,0,0),*da=__decimal(mpd_context,24,0);
		// some constant decimals we need
		Mdecimal *d8=__decimal(mpd_context,8,0),*d32=__decimal(mpd_context,32,0);
#else
		mpd_t *lasts=__mpd(mpd_context,0),*t=__mpd(mpd_context,3),*s=__mpd(mpd_context,3),*n=__mpd(mpd_context,1),*na=__mpd(mpd_context,0),*d=__mpd(mpd_context,0),*da=__mpd(mpd_context,24);
		// some constant decimals we need
		mpd_t *d8=__mpd(mpd_context,8),*d32=__mpd(mpd_context,32);
#endif
		if(!lasts||!t||!s||!n||!na||!d||!da||!d8||!d32){outputError("Failed to create all helper decimals");return NULL;}
		if(amVerbose())output("Initial decimals created!\n");
		unsigned long long iter=0;
		if(amVerbose()){
			output("Iteration %u:",iter);
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
			outputChar('\n');
		}
		int cmp;
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
		while(!mpd_error(mpd_context)){
			iter++;
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
			if(!cmp){if(amVerbose())output("Done!\n");break;}
			//if(amVerbose()){output("b\t");if(mpd_error(mpd_context))break;}
			if(cmp==INT_MAX){output("Something went wrong!\n");break;}
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
				output("Iteration %u:",iter);
				//char* _lasts=mpd_to_sci(lasts,0);if(_lasts){output(" lasts=%s");free(_lasts);}else output(" ?");
				char* _t=mpd_to_sci(t,0);if(_t){output(" t=%s",_t);free(_t);}else output(" ?");
				char* _s=mpd_to_sci(s,0);if(_s){output(" s=%s",_s);free(_s);}else output(" ?");
				char* _n=mpd_to_sci(n,0);if(_n){output(" n=%s",_n);free(_n);}else output(" ?");
				char* _na=mpd_to_sci(na,0);if(_na){output(" na=%s",_na);free(_na);}else output(" ?");
				char* _d=mpd_to_sci(d,0);if(_d){output(" d=%s",_d);free(_d);}else output(" ?");
				char* _da=mpd_to_sci(da,0);if(_da){output(" da=%s",_da);free(_da);}else output(" ?");
				outputChar('\n');
			}
#endif
		}

		if(mpd_context){
			// store pi, pi/2 and pi/4 in the decimal context (all or none) BEFORE readjusting the precision i.e. if no error occurred
			if(!mpd_error(mpd_context)){
				mpd_t *_pi=get_mpd_copy(mpd_context,s);
				if(_pi){
					uint32_t status=0;
					mpd_t *_pidiv2=__mpd(mpd_context,0),*_pidiv4=__mpd(mpd_context,0),*_pimul2=__mpd(mpd_context,0);
					if(_pidiv2&&_pidiv4&&_pimul2){
						mpd_qdiv_u32(_pidiv2,_pi,2,mpd_context,&status);
						mpd_qdiv_u32(_pidiv4,_pi,4,mpd_context,&status);
						mpd_qadd(_pimul2,_pi,_pi,mpd_context,&status); // NOTE better to simply double pi by adding it to itself???????
					}else // _pi not bound in decimalcontext, so free
						status=1;
					if((status&0xEFBF)==0){
						decimalcontext->pi=_pi;decimalcontext->pidiv2=_pidiv2;decimalcontext->pidiv4=_pidiv4;decimalcontext->pimul2=_pimul2;
					}else{
						free_mpd(_pimul2);free_mpd(_pi);free_mpd(_pidiv2);free_mpd(_pidiv4);
					}
				}

				if(!decimalcontext->pi)
				outputError("Failed to store the decimal approximation of pi in the decimal context");
				else
				if(amVerbose())
				outputLine("NOTE: Decimal approximation to pi stored in the decimal context.");
			}
			mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2
			// TODO should I mpd_finalize the pi values stored? or for now leave them unrounded?????????
		}

		// get rid of all the decimals we used
#ifdef __ADEBUG__
		free_decimal(lasts);free_decimal(t);free_decimal(n);free_decimal(na);free_decimal(d);free_decimal(da);
		free_decimal(d8);free_decimal(d32);
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
#ifdef __ADEBUG__
		mpd_finalize(s->mpd,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#else
		mpd_finalize(s,mpd_context); // to round to the requested precision
#endif
		if(amVerbose()){
#ifdef __ADEBUG__
			char* _s=mpd_to_sci(s->mpd,0);
#else
			char* _s=mpd_to_sci(s,0);
#endif
			if(_s){output("Final approximation of pi (rounded to %llu decimals): %s.\n",decimalprecision,_s);free(_s);}
		}

		_decimal=_getDecimal(s,decimalprecision,0,true);


	}else{ // decimalcontext->pi exists

		if(amVerbose())outputLine("NOTE: Returning the decimal approximation of pi stored in the decimal context.");
		// a copy to return
		_decimal=_getDecimal(get_mpd_copy(mpd_context,decimalcontext->pi),decimalprecision,0,true);

	}
	
	return _decimal; // freeonfailure=true means if we do not manage to wrap _pi in a decimal free it

	/* replacing:
	if(!mpd_context){if(decimalprecision>0)outputError("Failed to obtain the requested decimal context");else outputError("No (default) decimal context available");return NULL;}
	if(decimalprecision<0)decimalprecision=_decimalContext->prec;
	*/

}

// internal helper functions that compute the sine and cosine of any decimal smaller than 1 (typically in [0,pi/4))

// MDH@04SEP2019: if I combine two successive terms like I did below in _dsinorcos we'd always be adding positive numbers (never subtracting), and we'd be approaching from below...
/**
 * \brief computes the square root of the approximation to the square of the sine/cosine of \p x
 * \p x the argument (in radians)
 * \p sin true to return the square root of the sine squared, or the square root of the cosine squared
 * because the same terms are used to compute the sine and the cosine it is guaranteed that sin^2+cos^2=1 (as long as mpd_sqrt is correctly square rooting of course)
 */
mpd_t* _dsquarerootofsinorcossquared(mpd_context_t* mpd_context,mpd_t* x,bool sin){
	// I suppose it's best to compute the sine squared first and turn it into a cosine before square rooting, that should guarantee that the squared sum of sine and cosine with the same x is 1
	mpd_t* _sinorcossquared=NULL;
	if(mpd_context&&x){
		if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true);if(_decimal){output("Computing the square of the %s",(sin?"sine":"cosine"));outputDecimal(" of '",_decimal,"'.\n");free_decimal(_decimal);}}
		uint32_t status=0;
		mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)+2); // increment the precision by 2
		Mdecimal* _intermediateResult=(amVerbose()?__decimal(mpd_context,0,0):NULL);
		
		// ok denominator is multiplied by 2 to start with (so dividing by 2 immediately)
		mpd_t*_1=__mpd(mpd_context,1);
		mpd_t *_4x2=__mpd(mpd_context,0),*_16x4=__mpd(mpd_context,0);
		mpd_t *_num=__mpd(mpd_context,1),*_2times2kfac=__mpd(mpd_context,4);
		mpd_t *_2k=__mpd(mpd_context,2),*_term=__mpd(mpd_context,2);
		
		mpd_t *_den2=__mpd(mpd_context,12),*_term2=__mpd(mpd_context,0);
		
		// each term equals: _num/_den*(1-_4x2/_den2), with _num=(_4x2)*_2n

		mpd_t *_prevsinorcossquared=__mpd(mpd_context,0); // replacing:__mpd(mpd_context,(sin?0:1)); // we're actually computing twice the square to start with so we start with 2 for the cosine (we'll divide by 2 down below!!)
		_sinorcossquared=__mpd(mpd_context,0); // any value will do
		if(_sinorcossquared&&_prevsinorcossquared&&_16x4,_4x2&&_num&&_2times2kfac&&_term&&_2k&&_1){
			mpd_qmul(_4x2,x,x,mpd_context,&status); // compute x^2
			mpd_qmul_uint(_4x2,_4x2,4,mpd_context,&status); // multiply by 4 to get (2*x)^2=4*x^2
			mpd_qmul(_16x4,_4x2,_4x2,mpd_context,&status); // square _4x2 to get _16x4 which we need to update _num with
			unsigned long long iterations=0; // let's start with at most 100 iterations
			/////////bool sign=true; // the sine is computed in all cases
			mpd_qcopy(_sinorcossquared,_prevsinorcossquared,&status); // initialize _sinorcossquared to the initial value of the previous value
			while((status&0xEFBF)==0){
				iterations++;
				if(_intermediateResult)output("Iteration %llu: ",iterations);
				mpd_qmul(_num,_num,_4x2,mpd_context,&status); // update numerator (started at 1)
				mpd_qdiv(_term,_num,_2times2kfac,mpd_context,&status); // compute the quotient of _num and _den as the new term which multiplied by _term2 is the new term to add
				
				// if we compute the difference of two successive terms we get the following				
				mpd_qdiv(_term2,_4x2,_den2,mpd_context,&status);
				mpd_qsub(_term2,_1,_term2,mpd_context,&status); // _term2 has to be multiplied with _term
				mpd_qmul(_term,_term,_term2,mpd_context,&status);
				mpd_qadd(_sinorcossquared,_prevsinorcossquared,_term,mpd_context,&status); // update _sinorcossquared by adding _term
				if(mpd_qcmp(_sinorcossquared,_prevsinorcossquared,&status)==0)break; // no change anymore
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
				mpd_qcopy(_prevsinorcossquared,_sinorcossquared,&status); // update _prevsinorcossquared...
				if(_intermediateResult){
					mpd_qcopy(_intermediateResult->mpd,_term,&status);
					outputDecimal(" increment: '",_intermediateResult,"' -> ");
					mpd_qcopy(_intermediateResult->mpd,_sinorcossquared,&status);
					output("%s",(sin?"Sine":"One minus cosine")); // if we want the cosine we indicate that we're computing One minus the cosine squared!!!!
					outputDecimal(" squared '",_intermediateResult,"'.\n");
				}
				// updating the denominator (started as 2)
				mpd_qmul(_2times2kfac,_2times2kfac,_den2,mpd_context,&status); // multiply with what we just used in the second denominator
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now 3, 5, 7, ...
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply
				mpd_qadd_u32(_2k,_2k,1,mpd_context,&status); // _2k now 4, 6, 8, ...
				mpd_qmul(_2times2kfac,_2times2kfac,_2k,mpd_context,&status); // multiply
				// update _den2
				mpd_qcopy(_den2,_2k,&status);mpd_qadd_u32(_2k,_2k,1,mpd_context,&status);mpd_qmul(_den2,_den2,_2k,mpd_context,&status);
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

typedef struct mpd_sincos_t{
	mpd_t* sin;
	mpd_t* cos;
}mpd_sincos_t;
void free_mpd_sincos(mpd_sincos_t* _mpd_sincos){
	free_mpd(_mpd_sincos->sin);
	free_mpd(_mpd_sincos->cos);
	FREE(_mpd_sincos,'T');
}
/**
 * \brief returns a pair of mpd_t* instances containing the sine and cosine of \p x respectively guaranteeing their sum of squares equals 1
 * \p x the decimal to compute the sine/cosine of
 */
mpd_sincos_t* _dsinandcos(mpd_context_t* mpd_context,mpd_t* x){
	// the initial value of the sine is x, and of the cosine is 1
	mpd_sincos_t* _mpd_sinandcos=NULL;
	if(mpd_context&&x){
		_mpd_sinandcos=CALLOC(1,sizeof(mpd_sincos_t),'T');
		if(_mpd_sinandcos){
			// initialize the sine and cosine to x and 1 respectively i.e. the first term of the infinite series expansion
			_mpd_sinandcos->sin=get_mpd_copy(mpd_context,x);
			_mpd_sinandcos->cos=__mpd(mpd_context,1);
			if(_mpd_sinandcos->sin&&_mpd_sinandcos->cos){
				if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true);if(_decimal){outputDecimal("Computing the sine and cosine of '",_decimal,"'.\n");free_decimal(_decimal);}}
				// we have the numerator and denominator of the term to add
				uint32_t status=0;
				mpd_t *_num=get_mpd_copy(mpd_context,x),*_den=__mpd(mpd_context,1),*_n=__mpd(mpd_context,1),*_term=__mpd(mpd_context,0);
				mpd_t *_newsine=__mpd(mpd_context,0),*_newcosine=__mpd(mpd_context,0); // starting value doesn't matter will get copied to start with anyway
				if(_n&&_num&&_den&&_term&&_newsine&&_newcosine){
					Mdecimal *_intermediateSine=(amVerbose()?__decimal(mpd_context,0,0):NULL),*_intermediateCosine=(amVerbose()?__decimal(mpd_context,0,0):NULL);
					// every other term should be negated
					bool negate=true;
					uint64_t iteration=0;
					while((status&0xEFBF)==0){
						iteration++;
						if(_intermediateSine||_intermediateCosine)output("Iteration #%" PRIu64 ": ",iteration);
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
					if(_intermediateSine)free_decimal(_intermediateSine);
					if(_intermediateCosine)free_decimal(_intermediateCosine);
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
								if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,_sumofsquares),mpd_context->prec,0,true);if(_decimal){outputDecimal("Sum of sine squared and cosine squared: '",_decimal,"'.\n");free_decimal(_decimal);}}
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
					if(_sumofsquares){free_mpd(_sumofsquares);return _mpd_sinandcos;}
				}
			}
			free_mpd_sincos(_mpd_sinandcos);
		}else
			outputError("Failed to initialize the object storing the sine and cosine of a decimal");
	}else
		outputError("Failed to create the object to store the sine and cosine of a decimal in");
	return NULL;
}

mpd_t* _dsinorcos(mpd_context_t* mpd_context,mpd_t* x,bool sin){ // convergence requires x to be below 1
	mpd_t* _sinorcos=NULL;
	if(mpd_context&&x){
		if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,x),mpd_context->prec,0,true);if(_decimal){output("Computing the %s",(sin?"sine":"cosine"));outputDecimal(" of '",_decimal,"'.\n");free_decimal(_decimal);}}
		uint32_t status=0;
		// the sine of x equals the som of an infinite number of terms multiplied by x
		// each element of the sequence has an index, say n, but let's start with n=0
		// each term then equals (x^4n)/(4n+1)!)*(1-(x^2)/(4n+2)*(4n+3)))
		// so n=0: (x^0/1!)*(1-x^2/2*3), n=1: 
		//////////mpd_qsetprec(decimalContext,mpd_getprec(decimalContext)+2); // increment the precision by 2
		Mdecimal* _intermediateResult=(amVerbose()?__decimal(mpd_context,0,0):NULL);
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
		if(_intermediateResult)free_decimal(_intermediateResult);
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

// MDH@26AUG2019: implementing computing the sine with a certain accuracy using Taylor series
Mdecimal* _dsine(const Mdecimalcontext* decimalcontext,Mdecimal* x){
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi)free_decimal(pi_decimal(decimalcontext)); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))return _getDecimal(__mpd(mpd_context,0),mpd_context->prec,0,true);
				uint32_t status=0;
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}}
					if(!_absx){outputError("Failed to negate the decimal to compute the sine of");return NULL;}
					if(amVerbose())outputLine("Computing the sine of a negative decimal.");
				}
				mpd_t* _sine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true);if(_decimal){outputDecimal("Normalized sine (abs) argument: '",_decimal,"'.\n");free_decimal(_decimal);}}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint32_t xquadrant=mpd_qget_u32(_xquadrant,&status);
					if(amVerbose()){outputDecimal("Quadrant of sine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);}
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
							_sine=_dsquarerootofsinorcossquared(mpd_context,(sin?_xmod:_xsin),true);
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
									if(amVerbose())outputLine("Negating the computed sine!");
									mpd_set_negative(_sine); // negate the _sine
								}else
								if(amVerbose())
									outputLine("Not negating the sine!");
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
				return(_sine?_getDecimal(_sine,mpd_context->prec,0,true):NULL);				
			}
		}
		outputError("No decimal context to compute the sine of a decimal in");
	}else
		outputError("No decimal to compute the sine of");
	return NULL;
}

Mdecimal* _dcosine(const Mdecimalcontext* decimalcontext,Mdecimal* x){
	if(x){
		// use the same decimal context as used by x
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		if(decimalcontext){
			// we need pi in the given precision (now stored in any Mdecimalcontext)
			if(!decimalcontext->pi)free_decimal(pi_decimal(decimalcontext)); // compute and immediately free the returned copy
			mpd_context_t* mpd_context=(decimalcontext->pi?decimalcontext->mpd_context:NULL);
			if(mpd_context){
				if(isDecimalZero(x))return _getDecimal(__mpd(mpd_context,1),mpd_context->prec,0,true);
				uint32_t status=0;
				// with the cosine we can forget about the sign i.e. cos(-x)=cos(x), which means we can simply ignore the sign
				mpd_t* _absx=NULL;
				if(mpd_isnegative(x->mpd)){
					_absx=__mpd(mpd_context,0);
					if(_absx){mpd_qabs(_absx,x->mpd,mpd_context,&status);if((status&0xEFBF)!=0){free_mpd(_absx);_absx=NULL;}}
					if(!_absx){outputError("Failed to negate the decimal to compute the cosine of");return NULL;}
					if(amVerbose())outputLine("Computing the cosine of a negative decimal.");
				}
				mpd_t* _cosine=NULL;
				// TODO the following works for x in [0,1) but we have to ascertain to pass a value below 1 to _sincos
				mpd_t *_xmod=__mpd(mpd_context,0),*_xtemp=__mpd(mpd_context,0),*_xquadrant=__mpd(mpd_context,0);
				if(_xtemp&&_xmod&&_xquadrant){
					// normalize x to the range [0,2*pi)
					mpd_qdivmod(_xtemp,_xmod,(_absx?_absx:x->mpd),decimalcontext->pimul2,mpd_context,&status); // _xdiv is an integer number (sign)0,1,2,3,4,5,6,7,8,9,...
					if(amVerbose()){Mdecimal* _decimal=_getDecimal(get_mpd_copy(mpd_context,_xmod),mpd_context->prec,0,true);if(_decimal){outputDecimal("Normalized cosine (abs) argument: '",_decimal,"'.\n");free_decimal(_decimal);}}
					// determine the quadrant by dividing the normalized x by pi/2
					mpd_qdivmod(_xquadrant,_xtemp,_xmod,decimalcontext->pidiv2,mpd_context,&status);
					uint32_t xquadrant=mpd_qget_u32(_xquadrant,&status);
					if(amVerbose()){outputDecimal("Quadrant of cosine argument '",x,"': ");output("%" PRIu32 ".\n",xquadrant);}
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
							_cosine=_dsquarerootofsinorcossquared(mpd_context,(cos?_xmod:_xcos),false);
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

Mdecimal* _dexp(const Mdecimalcontext* decimalcontext,Mdecimal* x){
	if(x){
		if(!decimalcontext)decimalcontext=_getDecimalcontext(x->prec);
		if(!decimalcontext)decimalcontext=M_DECIMALCONTEXT;
		mpd_context_t* mpd_context=decimalcontext->mpd_context;
		if(mpd_context){
			mpd_t* _exp=__mpd(mpd_context,1);
			if(!mpd_iszero(x->mpd)){
				if(_exp){
					Mdecimal* _intermediateResult=(amVerbose()?__decimal(mpd_context,0,0):NULL);
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
					if(_intermediateResult)free_decimal(_intermediateResult);
					free_mpd(_prevexp);
					free_mpd(_i);
					free_mpd(_num);
					free_mpd(_den);
					free_mpd(_add);
				}else
					outputError("Failed to initialize the result of dexp()");
			}else
			if(amVerbose())outputLine("Zero argument to exp() approximation.");
			if(_exp)return _getDecimal(_exp,mpd_context->prec,0,true);
		}else
			outputError("No decimal context available for use in dexp().");
	}
	return NULL;
}


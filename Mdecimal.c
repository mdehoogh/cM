#include "Mdecimal.h"

#include "Malloc.h"
#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

extern long double const M_LD_NAN;
extern const char* const ERROR_PREFIX;
extern /*const*/ mpd_context_t* _decimalContext;

// MDH@25AUG2019: I'd see that decimalOne should be an Mdecimal? we can leave it the way it is for now but instead require the context passed to __mpd to be non-NULL!! i.e. __mpd does no longer default to _decimalContext
static mpd_t* decimalOne=NULL;
// getDecimalOne() return a decimal but this is a decimal that should never be freed
const mpd_t* getDecimalOne(){if(!decimalOne)decimalOne=__mpd(_decimalContext,1);return decimalOne;}/* VALIDATED */
bool isDecimalOne(Mdecimal* _decimal){
    // MDH@17JUN2019: something that is repeating is definitely not equal to 1 (TODO unless it's 0.[9])
    return (_decimal->repeating&&mpd_cmp(_decimal->mpd,getDecimalOne(),_decimalContext)==MP_EQ);
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

// decimalerrorstatus() filter out the rounding and inexact 'errors'
void report_mpd_status(const mpd_context_t* const mpd_context){
	uint32_t mpd_status=mpd_getstatus(mpd_context);
	if(mpd_status>0){
		outputLine("Decimal computations error report.");
		if(mpd_status&MPD_IEEE_Invalid_operation)outputLine("\tIEEE Invalid operation error.");
		if(mpd_status&MPD_Clamped)outputLine("\tClamped error.");
		if(mpd_status&MPD_Division_by_zero)outputLine("\tDivision by zero error.");
		if(mpd_status&MPD_Fpu_error)outputLine("\tFPU error.");
		if(mpd_status&MPD_Inexact)outputLine("\tInexact error.");
		if(mpd_status&MPD_Not_implemented)outputLine("\tNot implemented error.");
		if(mpd_status&MPD_Overflow)outputLine("\tOverflow error.");
		if(mpd_status&MPD_Rounded)outputLine("\tRounding error.");
		if(mpd_status&MPD_Subnormal)outputLine("\tSubnormal error.");
		if(mpd_status&MPD_Underflow)outputLine("\tUnderflow error.");
	}else
		outputLine("No decimal context errors.");
}

bool mpd_error(const mpd_context_t* const mpd_context){return(mpd_getstatus(mpd_context)&0xEFBF)!=0;}

// BASE STUFF
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

// MDH@25AUG2019: now requiring mpd_context to not be NULL
// MDH@17JUN2019: convenient to expose _get_mpd (e.g. for use in approximating pi with a decimal)
/**
 * \brief returns an mpd_t instance (from the mpdecimal libray) with the precision given by \p mpd_context equal to \p value
 * \param value the (initial) value of the returned mpd_t instance
 * \return on success the mpd_t instance equal to \p value, NULL otherwise
 */
mpd_t* __mpd(const mpd_context_t* mpd_context,int64_t value){
    mpd_t* _mpd=NULL;
	if(!mpd_context)mpd_context=_decimalContext;
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
    if(!mpd_context)mpd_context=_decimalContext; // use the application-wide decimal context if no context is defined
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

/**
 * \brief returns a decimal with mpdecimal instance with the default decimal context equal to \p mpd and number of repeating digits equal to \p repeating, freeing the _mpd on failure
 */
Mdecimal* _getDecimal(mpd_t* mpd,uint64_t repeating,bool freeonfailure){
    if(!mpd)return NULL; // can do this as won't have to free mpd anyway
    Mdecimal* decimal=__decimal(_decimalContext,0,repeating); // always using the default decimal context
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

// END BASE STUFF


Mdecimal* pi_decimal(const mpd_context_t* mpd_context){

	// if decimalContext equals NULL use the global decimal context, in _decimalContext
	if(!mpd_context)mpd_context=_decimalContext;

	mpd_ssize_t decimalprecision=(mpd_context?mpd_context->prec:0);
	if(decimalprecision<=0){outputError("Cannot approximate pi: no decimal context available");return NULL;}

	/* replacing:
	if(!mpd_context){if(decimalprecision>0)outputError("Failed to obtain the requested decimal context");else outputError("No (default) decimal context available");return NULL;}
	if(decimalprecision<0)decimalprecision=_decimalContext->prec;
	*/
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
	if(mpd_context)mpd_qsetprec(mpd_context,mpd_getprec(mpd_context)-2); // decrement the precision by 2
	// get rid of all the decimals we used
#ifdef __ADEBUG__
	free_decimal(lasts);free_decimal(t);free_decimal(n);free_decimal(na);free_decimal(d);free_decimal(da);
	free_decimal(d8);free_decimal(d32);
#else
	mpd_del(lasts);mpd_del(t);mpd_del(n);mpd_del(na);mpd_del(d);mpd_del(da);
	mpd_del(d8);mpd_del(d32);
#endif
	if(mpd_context){
		if(mpd_error(mpd_context)){ // something went wrong
			if(amVerbose())output("%sComputation of pi with precision %lld error status: %u.\n",ERROR_PREFIX,decimalprecision,mpd_getstatus(mpd_context));
			report_mpd_status(mpd_context);
			return NULL;
		}
	}
	// success
#ifdef __ADEBUG__
	mpd_finalize(s->mpd,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#else
	mpd_finalize(s,mpd_context?mpd_context:_decimalContext); // to round to the requested precision
#endif
	if(amVerbose()){
#ifdef __ADEBUG__
		char* _s=mpd_to_sci(s->mpd,0);
#else
		char* _s=mpd_to_sci(s,0);
#endif
		if(_s){output("Final approximation of pi (rounded to %llu decimals): %s.\n",decimalprecision,_s);free(_s);}
	}
	// wrap the mpd_t in a decimal
#ifdef __ADEBUG__
	return _getDecimalValue(s,true);
#else
	return _getDecimal(s,0,true);
#endif

}

// MDH@26AUG2019: implementing computing the sine with a certain accuracy using Taylor series
Mdecimal* _dsine(const mpd_context_t* decimalContext,Mdecimal* x){
	mpd_t* _sine=NULL;
	if(x){
		// use the same decimal context as used by x
		if(!decimalContext)decimalContext=get_mpd_context(x->prec);
		if(decimalContext){
			uint32_t status=0;
			// the sine of x equals the som of an infinite number of terms multiplied by x
			// each element of the sequence has an index, say n, but let's start with n=0
			// each term then equals (x^4n)/(4n+1)!)*(1-(x^2)/(4n+2)*(4n+3)))
			// so n=0: (x^0/1!)*(1-x^2/2*3), n=1: 
			//////////mpd_qsetprec(decimalContext,mpd_getprec(decimalContext)+2); // increment the precision by 2
			Mdecimal* _intermediateResult=(amVerbose()?__decimal(decimalContext,0,0):NULL);
			mpd_t *_x2=__mpd(decimalContext,0),*_x4=__mpd(decimalContext,0),*_1minus=__mpd(decimalContext,0),*_sub=__mpd(decimalContext,0),*_prevsine=__mpd(decimalContext,0),*_prodacc=__mpd(decimalContext,0),*_prevprodacc=__mpd(decimalContext,0); // parts that need to be initialized
			mpd_t *_prod=__mpd(decimalContext,1),*_multden=__mpd(decimalContext,6),*_multnum=__mpd(decimalContext,1),*_mult=__mpd(decimalContext,1),*_den=__mpd(decimalContext,6),*_4n=__mpd(decimalContext,3),*_1=__mpd(decimalContext,1); // initialized helper decimals
			_sine=__mpd(decimalContext,0);
			if(_sine&&_prevsine&&_x2&&_x4&&_multnum&&_multden&&_mult&&_den&&_4n&&_prod&&_1minus&&_sub&&_1&&_prodacc&&_prevprodacc){
				mpd_qmul(_x2,x->mpd,x->mpd,decimalContext,&status);
				mpd_qmul(_x4,_x2,_x2,decimalContext,&status);
				unsigned long long iterations=0; // let's start with at most 100 iterations
				while((status&0xEFBF)==0){
					iterations++;
					// compute _sub
					mpd_qdiv(_sub,_x2,_den,decimalContext,&status); // first time this would  be x^2/6
					// compute _1minus
					mpd_qsub(_1minus,_1,_sub,decimalContext,&status);
					// compute _prod as the product of _mult and _1minus
					mpd_qmul(_prod,_mult,_1minus,decimalContext,&status); // first time this would be 1 * x^2/6
					if(mpd_iszero(_prod))break; // if the product is now zero we're definitely done
					// increment sine with the new product
					if(mpd_iszero(_prevprodacc)){ // accuracy not yet reached
						// add _prod to _prevsine to become the new sine
						mpd_qadd(_sine,_prevsine,_prod,decimalContext,&status);
						if(mpd_qcmp(_prevsine,_sine,&status)==0) // _sine and _prevsine technically the same (in the given decimal context)
							mpd_qcopy(_prevprodacc,_prod,&status); // store the non-zero _prod in _prevprodacc, from now on we will keep doing that
						else // new sine differs from previous sine: required accuracy not yet reached
							mpd_qcopy(_prevsine,_sine,&status); // update _prevsine
						if(_intermediateResult){
							output("Iteration %llu: ",iterations);
							mpd_qcopy(_intermediateResult->mpd,_prod,&status);
							outputDecimal("Increment: '",_intermediateResult,"' -> ");
							mpd_qmul(_intermediateResult->mpd,_sine,x->mpd,decimalContext,&status);
							outputDecimal("Sine: '",_intermediateResult,"'.\n");
						}
					}else{ // accuracy reached, but still some iterations left
						mpd_qadd(_prodacc,_prevprodacc,_prod,decimalContext,&status);
						if(_intermediateResult){
							output("Iteration %llu: ",iterations);
							mpd_qcopy(_intermediateResult->mpd,_prodacc,&status);
							outputDecimal("Incremental remainder: '",_intermediateResult,"'.\n");
						}
						if(mpd_qcmp(_prodacc,_prevprodacc,&status)==0)break;
						mpd_qcopy(_prodacc,_prevprodacc,&status); // copy the change accumulative remainder
					}
					// update the helpers _den, _sub, _multnum, _multden, _mult, _4n
					mpd_qmul(_multnum,_multnum,_x4,decimalContext,&status); // updating _multnum is easy as we only need to multiply it by x^4
					// NOTE _4n starts equal to 3 (as _den starts as 3!), and the faculty stored in _multden needs to be updated 4 times
					// so, we have to increment _4n four times and use each of these 4 values to update _multden to become the new faculty value to use
					mpd_qadd_uint(_4n,_4n,1,decimalContext,&status); // now equal to (4n)
					mpd_qmul(_multden,_multden,_4n,decimalContext,&status);
					mpd_qadd_uint(_4n,_4n,1,decimalContext,&status); // now equal to (4n+1)
					mpd_qmul(_multden,_multden,_4n,decimalContext,&status);
					// after two increments to _4n _multden is what we want it to be for computing the 
					// _multnum and _multden updated, so we can now update _mult
					mpd_qdiv(_mult,_multnum,_multden,decimalContext,&status);
					
					mpd_qadd_uint(_4n,_4n,1,decimalContext,&status); // now equal to (4n+2)
					mpd_qmul(_multden,_multden,_4n,decimalContext,&status);
					mpd_qcopy(_den,_4n,&status); // initialize _den to _4n
					
					mpd_qadd_uint(_4n,_4n,1,decimalContext,&status); // now equal to (4n+3)
					mpd_qmul(_multden,_multden,_4n,decimalContext,&status);
					mpd_qmul(_den,_den,_4n,decimalContext,&status); // _den now equal to (4n+2)*(4n+3) as we need it to be

					// with _den computed we can now update _sub 
					mpd_qdiv(_sub,_x4,_den,decimalContext,&status);
					// and ready to 
				}
			}else
				outputError("Failed to create helper decimals in computing the sine of a decimal");
			if(_intermediateResult)free_decimal(_intermediateResult);
			// if accuracy was reached, but we still had some more iterations left we can add the accumulated remainder
			if(!mpd_iszero(_prevprodacc)){
				mpd_qadd(_sine,_sine,_prevprodacc,decimalContext,&status);
			}
			free_mpd(_prevprodacc);
			free_mpd(_prodacc);
			free_mpd(_prevsine);
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
			// finally multiply by x
			if((status&0xEFBF)==0){
				mpd_qmul(_sine,_sine,x->mpd,decimalContext,&status);
			}
			////////mpd_qsetprec(decimalContext,mpd_getprec(decimalContext)-2); // decrement the precision by 2
			if((status&0xEFBF)!=0){free_mpd(_sine);_sine=NULL;}////////else mpd_finalize(_sine,decimalContext);
		}else
			outputError("No context to compute the sine of a decimal in");
	}else
		outputError("No decimal to compute the sine of");
	return(_sine?_getDecimal(_sine,0,true):NULL);
}

Mdecimal* _dcosine(const mpd_context_t* decimalContext,Mdecimal* x){
	mpd_t* _cosine=NULL;
	if(x){
		// use the same decimal context as used by x
		if(!decimalContext)decimalContext=get_mpd_context(x->prec);
		if(decimalContext){

		}else
			outputError("No context to compute the cosine of a decimal in");
	}else
		outputError("No decimal to compute the cosine of");
	return(_cosine?_getDecimal(_cosine,0,true):NULL);
}


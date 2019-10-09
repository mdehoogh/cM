#include "Mrational.h"

#include "Malloc.h"
#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

extern const char * const ERROR_PREFIX;
extern const long double M_LD_NAN;

mp_err _bimul(Mbiginteger const * const a,Mbiginteger const * const b,Mbiginteger ** _c){
    // assuming _c equals NULL
    if(a||b){
        *_c=__biginteger();
        if(!_c)return MP_ERR;
        if(a&&b)return mp_mul(a,b,*_c); // if both big integers are defined, return the multiplication in *_c
        return mp_copy((a?a:b),*_c); // copy either a or b
    }
    // *_c should be NULL, so return MP_ERR if not NULL
    return (*_c?MP_OKAY:MP_ERR);
}
mp_err _bidiv(Mbiginteger const * const a,Mbiginteger const * const b,Mbiginteger ** _c){
    // assuming _c equals NULL
    if(a||b){
        *_c=__biginteger();
        if(!_c)return MP_ERR;
        if(a&&b)return mp_div(a,b,*_c,NULL); // if both big integers are defined, return the divisor in *_c ignoring the remainder!!!
        return mp_copy((a?a:b),*_c); // copy either a or b
    }
    // *_c should be NULL, so return MP_ERR if not NULL
    return (*_c?MP_OKAY:MP_ERR);
}
/**
 * \brief puts \p a - \b in * \p _c
 * \p a
 * \p b
 * \p _c
 */
mp_err _bisub(Mbiginteger const * const a,Mbiginteger const * const b,Mbiginteger ** _c){
    // assuming _c to not be NULL, and *_c to be NULL
    if(a&&b&&_c&&!*_c){
        *_c=__biginteger(); // get a big integer instance
        return(*_c?mp_sub(a,b,*_c):MP_ERR); // if we have an instance put the difference of a and b in it, otherwise failure
    }
    return MP_ERR;
}
mp_err _biadd(Mbiginteger const * const a,Mbiginteger const * const b,Mbiginteger ** _c){
    // assuming _c to not be NULL, and *_c to be NULL
    if(a&&b&&_c&&!*_c){
        *_c=__biginteger(); // get a big integer instance
        return(*_c?mp_add(a,b,*_c):MP_ERR); // if we have an instance put the difference of a and b in it, otherwise failure
    }
    return MP_ERR;
}

// rational equivalents of decimal binary operations
/**
 * \brief computes the product of \p a and \p b and puts the result in \p c
 */
mp_err _qmul(Mrational* const c,Mrational const * const a,Mrational const * const b){
    Mbiginteger *_num=NULL,*_den=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b->den,&_den); // multiply denominators
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    if(status==MP_OKAY)status=_bimul(a->num,b->num,&_num); // multiply numerators
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
    }else{ // numerator and denominator computed
        c->num=_num;
        c->den=_den;
        if(!c->normalized)normalizeRational(c);
    }
    return status;
}
/**
 * \brief computes the product of \p a and big integer \p b and puts the result in \p c
 */
mp_err _qmul_bi(Mrational* const c,const Mrational* const a,const Mbiginteger* const b){
    Mbiginteger *_num=NULL,*_den=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need all input pointers
    if(status==MP_OKAY)status=_bimul(a->den,NULL,&_den); // multiply denominators
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    if(status==MP_OKAY)status=_bimul(a->num,b,&_num); // multiply numerators
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
    }else{ // numerator and denominator computed
        c->num=_num;
        c->den=_den;
        if(!c->normalized)normalizeRational(c);
    }
    return status;
}

/**
 * \brief computes the quotient of \p a and \p b and puts the result in \p c
 */
mp_err _qdiv(Mrational* const c,const Mrational* const a,const Mrational* const b){
    Mbiginteger *_num=NULL,*_den=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b->num,&_den); // multiply denominator of a with numerator of a
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    if(status==MP_OKAY)status=_bimul(a->num,b->den,&_num); // multiply numerator of a with denominator of b
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
    }else{ // numerator and denominator computed
        c->num=_num;
        c->den=_den;
        if(!c->normalized)normalizeRational(c);
    }
    return status;
}
/**
 * \brief computes the quotient of rational \p a and big integer \p b and puts the result in \p c
 */
mp_err _qdiv_bi(Mrational* const c,const Mrational* const a,const Mbiginteger* const b){
    Mbiginteger *_num=NULL,*_den=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b,&_den); // multiply denominator of a with numerator of a
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    if(status==MP_OKAY)status=_bimul(a->num,NULL,&_num); // multiply numerator of a with denominator of b
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
    }else{ // numerator and denominator computed
        c->num=_num;
        c->den=_den;
        if(!c->normalized)normalizeRational(c);
    }
    return status;
}
/**
 * \brief computes the difference of \p a and \p b and puts the result in \p c
 */
mp_err _qsub(Mrational* const c,const Mrational* const a,const Mrational* const b){
    Mbiginteger *_num=NULL,*_num1=NULL,*_num2=NULL,*_den=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b->den,&_den); // multiply denominators to become the result denominator
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    if(status==MP_OKAY)status=_bimul(a->num,b->den,&_num1); // multiply numerator of a with denominator of b for the plus term of the result numerator
    if(status==MP_OKAY)status=_bimul(a->den,b->num,&_num2); // multiply denominator of a with numerator of b for the min term of the result numerator
    if(status==MP_OKAY)status=_bisub(_num1,_num2,&_num);
    // loose the numerator parts (are not stored in the result rational anyway)
    if(_num1)free_biginteger(_num1);
    if(_num2)free_biginteger(_num2);
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
    }else{ // numerator and denominator computed
        c->num=_num;
        c->den=_den;
        if(!c->normalized)normalizeRational(c);
    }
    return status;
}

// for the computation of the sum of two Mreals
long double ldsum(long double ld1,long double ld2){
    // if either is M_LD_NAN return the other
    if(ld1==M_LD_NAN)return ld2;if(ld2==M_LD_NAN)return ld1;return ld1+ld2;
}
long double realsum(Mreal* _real1,Mreal* _real2){
	if(!_real1&&!_real2)return M_LD_NAN; // both undefined
    // return the other if one is undefined
	if(!_real1)return _real2->ld;
	if(!_real2)return _real1->ld;
    // both are defined BUT then we still have the problem that either could be undefined
    return ldsum(_real1->ld,_real2->ld);
}
Mreal* _realsum(Mreal* r1,Mreal* r2){
    // if either real is defined a sum real is to be produced
    // NOTE _getReal ALWAYS returns a real (if possible) so even when M_LD_NAN is in it
    return(r1||r2?_getReal(realsum(r1,r2)):NULL);
}
// for the computation of the difference of two reals
long double lddifference(long double ld1,long double ld2){
    // if either is M_LD_NAN return the other
    if(ld1==M_LD_NAN&&ld2==M_LD_NAN)return M_LD_NAN;
    if(ld1==M_LD_NAN)return -ld2;if(ld2==M_LD_NAN)return ld1;return ld1-ld2;
}
long double realdifference(Mreal* _real1,Mreal* _real2){
	if(!_real1&&!_real2)return M_LD_NAN; // both undefined
    // return the other if one is undefined
	if(!_real1)return -_real2->ld; // return the NEGATED value of the second real
	if(!_real2)return _real1->ld;
    // both are defined BUT then we still have the problem that either could be undefined
    return lddifference(_real1->ld,_real2->ld);
}
Mreal* _realdifference(Mreal* r1,Mreal* r2){return(r1||r2?_getReal(realdifference(r1,r2)):NULL);}
// for the computation of the product of two reals (which is very simple)
long double ldproduct(long double ld1,long double ld2){return(ld1==M_LD_NAN||ld2==M_LD_NAN?M_LD_NAN:ld1*ld2);} // either undefined, product undefined
long double realproduct(Mreal* r1,Mreal* r2){return(r1&&r2?ldproduct(r1->ld,r2->ld):M_LD_NAN);} // either undefined, product undefined
Mreal* _realproduct(Mreal* r1,Mreal* r2){return(r1||r2?_getReal(realproduct(r1,r2)):NULL);} // either undefined, product undefined
// for the computation of the quotient of two reals
// NOTE if the second quotient is zero, we'd get infinity, so one should avoid this from happening, because then the quotient would be infinity!!!
long double ldquotient(long double ld1,long double ld2){return(ld1==M_LD_NAN||ld2==M_LD_NAN?M_LD_NAN:ld1/ld2);} // either undefined, quotient undefined
long double realquotient(Mreal* r1,Mreal* r2){return(r1&&r2?ldquotient(r1->ld,r2->ld):M_LD_NAN);} // either undefined, product undefined
Mreal* _realquotient(Mreal* r1,Mreal* r2){return(r1||r2?_getReal(realquotient(r1,r2)):NULL);} // either undefined, product undefined

bool realIsUndefined(Mreal* real){return(!real||ldIsNaN(real->ld));}
bool realIsUndefinedOrZero(Mreal* real){return(!real||ldIsNaN(real->ld)||ldIsZero(real->ld));}

mp_err _qadd(Mrational* c,Mrational const * const a,Mrational const * const b){
    Mbiginteger *_num=NULL,*_num1=NULL,*_num2=NULL,*_den=NULL;
    ///////Mreal* _delta=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b->den,&_den); // multiply denominators to become the result denominator
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    ////// OOPS the deltas are handled by _getRationalSum!!!! if(status==MP_OKAY){_delta=_realsum(a->delta,b->delta);if(!_delta)if(a->delta||b->delta)status=MP_ERR;} // if we failed in adding the delta's error as well
    if(status==MP_OKAY)status=_bimul(a->num,b->den,&_num1); // multiply numerator of a with denominator of b for the plus term of the result numerator
    if(status==MP_OKAY)status=_bimul(a->den,b->num,&_num2); // multiply denominator of a with numerator of b for the min term of the result numerator
    if(status==MP_OKAY)status=_biadd(_num1,_num2,&_num); // add the numerator parts
    // loose the numerator parts (are not stored in the result rational anyway)
    ////outputLine("Rational numerator parts to free.");
    if(_num1)free_biginteger(_num1);
    if(_num2)free_biginteger(_num2);
    ////outputLine("Rational numerator parts freed.");
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        ////outputLine("Freeing new numerator, denominator and delta!");
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
        //////if(_delta)free_real(_delta);
        ////outputLine("New numerator, denominator and delta freed!");
    }else{ // numerator and denominator computed
        ////outputBiginteger("Storing numerator '",_num,"'");outputBiginteger(" and denominator '",_den,"'.\n");
        // too bad we have to clear the current numerator and denominator pointers (if any)
        if(c&&c->num){if(amVerbose())outputLine("Freeing previous numerator.");free_biginteger(c->num);}
        if(c&&c->den){if(amVerbose())outputLine("Freeing previous denominator.");free_biginteger(c->den);}
        /////outputLine("Previous numerator and denominator released.");
        c->num=_num;c->den=_den;
        /////outputLine("Numerator and denominator stored.");
        // normalize the rational
        /////outputLine("Normalizing the sum rational.");
        c->normalized=false;normalizeRational(c);
        // register the delta sum
        ////////c->delta=_delta;
    }
    // TODO we have to add the 
    return status;
}
/**
 * \brief copies \p a into \p b
 */
mp_err _qcopy(Mrational* const b,Mrational const * const a){
    mp_err status=(a&&b?MP_OKAY:MP_ERR);
    if(status==MP_OKAY){
        // get copies of numerator, denominator and delta of the source
        Mbiginteger *_num=(a->num?_getBigintegerCopy(a->num):NULL),*_den=(a->den?_getBigintegerCopy(a->den):NULL);
        Mreal* _delta=(a->delta?_getReal(a->delta->ld):NULL);
        // if copying failed mark as error
        if(a->num&&!_num)status=MP_ERR;else if(a->den&&!_den)status=MP_ERR;else if(a->delta&&!_delta)status=MP_ERR;
        if(status==MP_OKAY){ // copies made successfully
            // move copies freeing all originals in the process
            if(b->num)free_biginteger(b->num);b->num=_num;
            if(b->den)free_biginteger(b->den);b->den=_den;
            if(b->delta)free_real(b->delta);b->delta=_delta;
            b->normalized=a->normalized;
        }else{
            // free all copies if made
            if(_num)free_biginteger(_num);
            if(_den)free_biginteger(_den);
            if(_delta)free_real(_delta);
        }
    }
    return status;
}
/**
 * \brief copies \p a into \p b
 */
mp_err _qcopy_bi(Mrational* const b,Mbiginteger const * const a){
    mp_err status=(a&&b?MP_OKAY:MP_ERR);
    if(status==MP_OKAY){
        // get copies of numerator, denominator and delta of the source
        Mbiginteger *_num=_getBigintegerCopy(a);
        // if copying failed mark as error
        if(_num){            
            // move copies freeing all originals in the process
            if(b->num)free_biginteger(b->num);b->num=_num;
            if(b->den)free_biginteger(b->den);b->den=NULL;
            if(b->delta)free_real(b->delta);b->delta=NULL;
            b->normalized=true;
        }else
            status=MP_ERR;
    }
    return status;
}

bool _qeq(Mrational* q1,Mrational* q2,mp_err *status){
    if(*status!=MP_OKAY)return false;
    if(q1&&q2){ // both rationals are defined
        // two rationals a/b and c/d are equal if a * d == b * c
        Mbiginteger *_prod1=NULL,*_prod2=NULL;
        *status=_bimul(q1->num,q2->den,&_prod1);
        *status=_bimul(q1->den,q2->num,&_prod2);
        bool result=(*status==MP_OKAY?mp_cmp(_prod1,_prod2)!=0:false);
        // free the help product big integers
        if(_prod1)free_biginteger(_prod1);
        if(_prod2)free_biginteger(_prod2);
        return result;
    }
    return(!q1&&!q2); // if both are NULL return true, false otherwise
}

Mrational* _getPureRationalSum(Mrational const * const q1,Mrational const * const q2){
    Mrational* _pureRationalSum=NULL;
    if(q1&&q2){ // rationals defined
        if(realIsUndefinedOrZero(q1->delta)&&realIsUndefinedOrZero(q2->delta)){ // both are pure
            _pureRationalSum=__rational();
            if(_pureRationalSum){
                if(_qadd(_pureRationalSum,q1,q2)!=MP_OKAY){
                    free_rational(_pureRationalSum);_pureRationalSum=NULL;
                    if(amVerbose())outputError("Failed to compute the sum of two pure rationals");
                }
            }else if(amVerbose())outputError("Failed to create the pure sum rational");
        }else if(amVerbose())outputError("Both rationals should be pure, and are not");
    }
    return _pureRationalSum;
}/* VALIDATED */

// the following methods take rationals with deltas into account whereas the _qmul, _qdiv, _qadd and _qsub do not
Mrational* _getRationalSum(Mrational const * const q1,Mrational const * const q2){
    // NOTE leaving it to _qadd to deal with NULL rational input (which should never happen though)
    Mrational* _rational=__rational();
    if(_rational){
        mp_err status=_qadd(_rational,q1,q2);
        if(status==MP_OKAY){
            // compute the delta
        	_rational->delta=_realsum(q1->delta,q2->delta);
            // if failed to compute the delta mark error
            if(q1->delta&&q2->delta)if(!_rational->delta)status=MP_ERR;
        }
        if(status!=MP_OKAY){free_rational(_rational);_rational=NULL;outputError("Failed to compute the sum of two rationals");}
    }else
        outputError("Failed to create the rational for storing the sum of two rationals");
    return _rational;
}
Mrational* _getRationalDifference(Mrational const * const q1,Mrational const * const q2){
    // NOTE leaving it to _qmul to deal with NULL rational input (which should never happen though)
    Mrational* _rational=__rational();
    if(_rational){
        mp_err status=_qsub(_rational,q1,q2);
        if(status==MP_OKAY){
            // compute the delta
        	_rational->delta=_realdifference(q1->delta,q2->delta);
            // if failed to compute the delta mark error
            if(q1->delta&&q2->delta)if(!_rational->delta)status=MP_ERR;
        }
        if(status!=MP_OKAY){free_rational(_rational);_rational=NULL;outputError("Failed to compute the difference of two rationals");}
    }else
        outputError("Failed to create the rational for storing the difference of two rationals");
    return _rational;
}
// computation of the actual delta is much harder in products and quotients
long double getLongDoubleRationalProduct(long double ld,Mrational* r){
    if(!r)return M_LD_NAN; // unlikely though as we're calling it ourselves with a defined rational
    if(ld!=M_LD_NAN){
        if(amVerbose())output("Product of long double %.*Lf",LDBL_DIG,ld);
        // multiply ld with the numerator to start with
        if(r->num)ld=ldproduct(ld,mp_get_long_double(r->num));
        // divide ld by the denominator
        if(r->den)ld=ldquotient(ld,mp_get_long_double(r->den));
    }
    if(amVerbose()){outputRational(" and rational ",r,":");output("%.*Lf.\n",ld);}
    return ld;
}
long double getLongDoubleRationalQuotient(long double ld,Mrational* r){
    if(!r)return M_LD_NAN; // unlikely though as we're calling it ourselves with a defined rational
    if(ld!=M_LD_NAN){
        // multiply ld with the numerator to start with
        if(r->den)ld=ldproduct(ld,mp_get_long_double(r->den));
        // divide ld by the denominator
        if(r->num)ld=ldquotient(ld,mp_get_long_double(r->num));
    }
    return ld;
}
long double getLongDoubleRationalSum(long double ld,Mrational* r){
    if(!r)return M_LD_NAN; // unlikely though as we're calling it ourselves with a defined rational
    if(ld!=M_LD_NAN){
        // multiply ld with the numerator to start with
        if(r->num)ld=ldproduct(ld,mp_get_long_double(r->num));
        // divide ld by the denominator
        if(r->den)ld=ldquotient(ld,mp_get_long_double(r->den));
    }
    return ld;
}
Mrational* _getRationalProduct(Mrational const * const q1,Mrational const * const q2){
    // NOTE leaving it to _qmul to deal with NULL rational input (which should never happen though)
    Mrational* _rational=__rational();
    if(_rational){
        mp_err status=_qmul(_rational,q1,q2);
        if(status==MP_OKAY){
        	bool delta1defined=!realIsUndefined(q1->delta),delta2defined=!realIsUndefined(q2->delta);
            // if at least one is defined, there will be a delta in the product
            if(delta1defined||delta2defined){
                // the delta is either a single term or the sum of three terms
                long double term1=(delta1defined?getLongDoubleRationalProduct(q1->delta->ld,q2):M_LD_NAN),term2=(delta2defined?getLongDoubleRationalProduct(q2->delta->ld,q1):M_LD_NAN);
                if(term1!=M_LD_NAN&&term2!=M_LD_NAN) // both terms are defined
                    _rational->delta=_getReal(term1+term2+ldproduct(q1->delta->ld,q2->delta->ld));
                else
                if(term1!=M_LD_NAN) // term1 is defined
                    _rational->delta=_getReal(term1);
                else // term2 is defined
                    _rational->delta=_getReal(term2);
                // if failed to compute the delta mark error
                if(q1->delta&&q2->delta)if(!_rational->delta)status=MP_ERR;
            }
        }
        if(status!=MP_OKAY){free_rational(_rational);_rational=NULL;outputError("Failed to compute the product of two rationals");}
    }else
        outputError("Failed to create the rational for storing the product of two rationals");
    return _rational;
}
Mrational* _getRationalQuotient(Mrational const * const q1,Mrational const * const q2){
    // NOTE leaving it to _qdiv to deal with NULL rational input (which should never happen though)
    Mrational* _rational=__rational();
    if(_rational){
        mp_err status=_qdiv(_rational,q1,q2);
        if(status==MP_OKAY){
        	bool delta1defined=!realIsUndefined(q1->delta),delta2defined=!realIsUndefined(q2->delta);
            // TODO if either delta is defined the new rational will also have a delta!!!
            if(delta2defined){
                 // the new delta is the quotient of (delta1-r*delta2) and the value of q2
                 // NOTE that if delta1 is not defined there's no delta1 term and we pass M_LD_NAN into lddifference!!!
                 // TODO check whether using M_LD_NAN instead of 0 did NOT work if delta1 is not defined!!!!
                 long double ldNumerator=lddifference(delta1defined?q1->delta->ld:0,getLongDoubleRationalProduct(q2->delta->ld,_rational));
                 long double ldDenominator=getRationalLongDouble(q2);
                 if(amVerbose())output("New rational quotient delta: numerator=%*Lf - denominator=%*Lf.\n",LDBL_DIG,ldNumerator,LDBL_DIG,ldDenominator);
                _rational->delta=_getReal(ldquotient(ldNumerator,ldDenominator));
            }else
            if(delta1defined)
                _rational->delta=_getReal(getLongDoubleRationalQuotient(q1->delta->ld,q2));
        }else{
            free_rational(_rational);_rational=NULL;outputError("Failed to compute the quotient of two rationals");
        }
    }else
        outputError("Failed to create the rational for storing the quotient of two rationals.");
    return _rational;
}

// MDH@18SEP2019: rational version of _dsinorcos (which itself is not used to compute the decimal sine/cosine)
//                the main problem here is that there will never be convergence because there is no precision given
//                obviously the term is getting smaller and smaller
/**
 * \brief rational version of sine/cosine function using the ordinary Taylor polynomial approximation
 * \p x the argument of which to compute the rational approximation to the sine/cosine
 */
Mrational* _qsinorcos(Mrational const * const x,bool sin){
	Mrational* _sinorcos=NULL;
	if(x){
		if(amVerbose()){output("Computing the %s",(sin?"sine":"cosine"));outputRational(" of '",x,"'.\n");}
		uint32_t status=0,istatus=0;
		// the sine of x equals the som of an infinite number of terms multiplied by x
		// each element of the sequence has an index, say n, but let's start with n=0
		// each term then equals (x^4n)/(4n+1)!)*(1-(x^2)/(4n+2)*(4n+3)))
		// so n=0: (x^0/1!)*(1-x^2/2*3), n=1: 
		Mrational* _intermediateResult=(amVerbose()?__rational():NULL);
	    Mrational *_x2=__rational(),*_x4=__rational(),*_1minus=__rational(),*_sub=__rational(),*_prevsincos=__rational(),*_prodacc=__rational(),*_prevprodacc=__rational(); // parts that need to be initialized
	    Mrational *_prod=_getRational(NULL,NULL,M_LD_NAN,false,false),*_multnum=_getRational(NULL,NULL,M_LD_NAN,false,false),*_mult=_getRational(NULL,NULL,M_LD_NAN,false,false),*_1=_getRational(NULL,NULL,M_LD_NAN,false,false);
		// helpers of which the value differs whether a sine or cosine approximation is requested (den is 3! for the sine, and 2! for the cosine)
		Mbiginteger *_den=_getBiginteger(sin?6:2),*_4n=_getBiginteger(sin?3:2),*_multden=_getBiginteger(sin?6:2); // initialized helper decimals
		_sinorcos=__rational();
		if(_sinorcos&&_prevsincos&&_x2&&_x4&&_multnum&&_multden&&_mult&&_den&&_4n&&_prod&&_1minus&&_sub&&_1&&_prodacc&&_prevprodacc){
            mp_err status=MP_OKAY;
            // using the rational equivalents of the mpd binary operations
            status=_qmul(_x2,x,x);
            status=_qmul(_x4,_x2,_x2);
			unsigned long long iterations=0; // let's start with at most 100 iterations
			while(status==MP_OKAY){
				iterations++;
				// compute _sub
				status=_qdiv_bi(_sub,_x2,_den); // first time this would  be x^2/6
				// compute _1minus
				status=_qsub(_1minus,_1,_sub);
				// compute _prod as the product of _mult and _1minus
				status=_qmul(_prod,_mult,_1minus); // first time this would be 1 * x^2/6
                // MDH@19SEP2019: if _prod is sufficiently small we're 'done'
				if(isRationalZero(_prod))break; // if the product is now zero we're definitely done
				// increment sine with the new product
				if(!isRationalZero(_prevprodacc)){ // accuracy not yet reached
					// add _prod to _prevsine to become the new sine
					status=_qadd(_sinorcos,_prevsincos,_prod);
					if(_qeq(_prevsincos,_sinorcos,&status)) // _sine and _prevsine technically the same (in the given decimal context)
						status=_qcopy(_prevprodacc,_prod); // store the non-zero _prod in _prevprodacc, from now on we will keep doing that
					else // new sine differs from previous sine: required accuracy not yet reached
						status=_qcopy(_prevsincos,_sinorcos); // update _prevsine
					if(_intermediateResult){
						output("Iteration %llu: ",iterations);
						istatus=_qcopy(_intermediateResult,_prod);
						if(istatus==MP_OKAY){
                            outputRational("Increment: '",_intermediateResult,"' -> ");
						    if(sin)istatus=_qmul(_intermediateResult,_sinorcos,x);else istatus=_qcopy(_intermediateResult,_sinorcos);
						    if(istatus==MP_OKAY)outputRational((sin?"Sine: ":"Cosine: '"),_intermediateResult,"'.\n");
                        }
					}
				}else{ // accuracy reached, but still some iterations left
					status=_qadd(_prodacc,_prevprodacc,_prod);
					if(_intermediateResult){
						output("Iteration %llu: ",iterations);
						istatus=_qcopy(_intermediateResult,_prodacc);
						if(istatus==MP_OKAY)outputRational("Incremental remainder: '",_intermediateResult,"'.\n");
					}
					if(_qeq(_prodacc,_prevprodacc,&status))break;
					status=_qcopy(_prodacc,_prevprodacc); // copy the change accumulative remainder
				}
				// update the helpers _den, _sub, _multnum, _multden, _mult, _4n
				status=_qmul(_multnum,_multnum,_x4); // updating _multnum is easy as we only need to multiply it by x^4
				// NOTE _4n starts equal to 3 (as _den starts as 3!), and the faculty stored in _multden needs to be updated 4 times
				// so, we have to increment _4n four times and use each of these 4 values to update _multden to become the new faculty value to use
                status=mp_incr(_4n);
				status=mp_mul(_multden,_4n,_multden);
				status=mp_incr(_4n); // now equal to (4n+1)
				status=mp_mul(_multden,_4n,_multden);
				// after two increments to _4n _multden is what we want it to be for computing the 
				// _multnum and _multden updated, so we can now update _mult
				status=_qdiv_bi(_mult,_multnum,_multden);
				
				status=mp_incr(_4n); // now equal to (4n+2)
				status=mp_mul(_multden,_4n,_multden);
				status=mp_copy(_4n,_den); // initialize _den to _4n
				
				status=mp_incr(_4n); // now equal to (4n+3)
				status=mp_mul(_multden,_4n,_multden);
				status=mp_mul(_den,_4n,_den); // _den now equal to (4n+2)*(4n+3) as we need it to be

				// with _den computed we can now update _sub 
				status=_qdiv_bi(_sub,_x4,_den);
				// and ready to 
            }

        }else{
            output("%sFailed to initialize the result of computing the %ssine",ERROR_PREFIX,(sin?"":"co"));outputRational(" of '",x,"'.\n");
        }
		if(_intermediateResult)free_rational(_intermediateResult);
		// if accuracy was reached, but we still had some more iterations left we can add the accumulated remainder
		if(!isRationalZero(_prevprodacc)){
			status=_qadd(_sinorcos,_sinorcos,_prevprodacc);
		}
		free_rational(_prevprodacc);
		free_rational(_prodacc);
		free_rational(_prevsincos);
		free_rational(_x2);
		free_rational(_x4);
		free_biginteger(_4n);
		free_rational(_1);
		free_rational(_sub);
		free_rational(_1minus);
		free_rational(_multnum);
		free_biginteger(_multden);
		free_rational(_mult);
		free_biginteger(_den);
		free_rational(_prod);
		// finally multiply by x if the sine was requested!!!
		if(status==MP_OKAY)if(sin)status=_qmul(_sinorcos,_sinorcos,x);
		if(status!=MP_OKAY){free_rational(_sinorcos);_sinorcos=NULL;}
	}
	return _sinorcos;
}
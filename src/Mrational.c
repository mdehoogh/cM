#include "Mrational.h"

// MDH@27FEB2020 replacing: #include "Msession.h"

extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_ZERO,M_POSITIVE,M_NEGATIVE;
extern const char * const ERROR_PREFIX;
extern const long double M_LD_NAN;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double

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

// MDH@21OCT2019: TODO restyled _qsub (assuming that if num is NULL it's undefined)
/**
 * \brief computes the difference of \p a and \p b and puts the result in \p c
 */
mp_err _qsub(Mrational * const c,Mrational const * const a,Mrational const * const b){
    mp_err status=(c&&(a||b)?MP_OKAY:MP_ERR);
    if(status==MP_OKAY){ // c and at least a or b provided
        // should we NULL the numerator and denominator of c?????
        if(c->num){free_biginteger(c->num);c->num=NULL;}
        // TODO do we need to NULL the denominator????? if(c->den){free_biginteger(c->den);c->den=NULL;}
        // if b is not defined, or zero, copy a into c
        if(!b||!b->num||isBigintegerZero(b->num)==M_TRUE){ // b is undefined or zero: return a
            if(amVerbose())outputInfo("Second rational argument undefined or zero.");
            if(a->num){c->num=_getBigintegerCopy(a->num);if(!c->num)return MP_ERR;}
            if(a->den){c->den=_getBigintegerCopy(a->den);if(!c->den)return MP_ERR;}
        }else
        if(!a||!a->num||isBigintegerZero(a->num)==M_TRUE){ // a is undefined or zero: return b
            if(amVerbose())outputInfo("First rational argument undefined or zero.");
            if(b->num){c->num=_getBigintegerCopy(b->num);if(!c->num)return MP_ERR;}
            if(b->den){c->den=_getBigintegerCopy(b->den);if(!c->den)return MP_ERR;}
        }else{
            if(amVerbose())outputInfo("Subtracting two pure rationals.");
            // ASSERT a and b both defined
            Mbiginteger *_num=NULL,*_num1=NULL,*_num2=NULL,*_den=NULL;
            if(a->den||b->den)status=_bimul(a->den,b->den,&_den); // we have to be careful here as _bimul requires at least one argument to be non-NULL!!!
            if(status==MP_OKAY)if(_den&&mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
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
                /////// not on pure rationals!!!! c->delta=_floatdifference(a->delta,b->delta);
                if(!c->normalized)normalizeRational(c);
            }
        }
    }else
        outputError("Not all rationals provided for computing the difference of two rationals");
    return status;
}

// for the computation of the sum of two long doubles
long double ldsum(long double ld1,long double ld2){
    // if either is M_LD_NAN return the other
    if(ld1==M_LD_NAN)return ld2;if(ld2==M_LD_NAN)return ld1;return ld1+ld2;
}
long double floatsum(Mfloat* _float1,Mfloat* _float2){
	if(!_float1&&!_float2)return M_LD_NAN; // both undefined
    // return the other if one is undefined
	if(!_float1)return _float2->ld;
	if(!_float2)return _float1->ld;
    // both are defined BUT then we still have the problem that either could be undefined
    return ldsum(_float1->ld,_float2->ld);
}
Mfloat* _floatsum(Mfloat* r1,Mfloat* r2){
    // if either real is defined a sum real is to be produced
    // NOTE _getReal ALWAYS returns a real (if possible) so even when M_LD_NAN is in it
    if(floatIsUndefinedOrZero(r2))return _getFloatCopy(r1);
    if(floatIsUndefinedOrZero(r1))return _getFloatCopy(r2);
    // ASSERT neither is undefined or zero
    return _getFloat(floatsum(r1,r2));
}
// for the computation of the difference of two reals
long double lddifference(long double ld1,long double ld2){
    // if either is M_LD_NAN return the other
    if(ld1==M_LD_NAN&&ld2==M_LD_NAN)return M_LD_NAN;
    if(ld1==M_LD_NAN)return -ld2;if(ld2==M_LD_NAN)return ld1;return ld1-ld2;
}
long double floatdifference(Mfloat* _float1,Mfloat* _float2){
	if(!_float1&&!_float2)return M_LD_NAN; // both undefined
    // return the other if one is undefined
	if(!_float1)return -_float2->ld; // return the NEGATED value of the second real
	if(!_float2)return _float1->ld;
    // both are defined BUT then we still have the problem that either could be undefined
    return lddifference(_float1->ld,_float2->ld);
}
Mfloat* _floatdifference(Mfloat* r1,Mfloat* r2){
    if(floatIsUndefinedOrZero(r2))return _getFloatCopy(r1);
    if(floatIsUndefinedOrZero(r1))return _getFloatNeg(r2);
    return _getFloat(floatdifference(r1,r2));
}
// for the computation of the product of two reals (which is very simple)
long double ldproduct(long double ld1,long double ld2){return(ld1==M_LD_NAN||ld2==M_LD_NAN?M_LD_NAN:ld1*ld2);} // either undefined, product undefined
long double floatproduct(Mfloat* r1,Mfloat* r2){return(r1&&r2?ldproduct(r1->ld,r2->ld):M_LD_NAN);} // either undefined, product undefined
Mfloat* _floatproduct(Mfloat* r1,Mfloat* r2){return(r1||r2?_getFloat(floatproduct(r1,r2)):NULL);} // either undefined, product undefined
// for the computation of the quotient of two reals
// NOTE if the second quotient is zero, we'd get infinity, so one should avoid this from happening, because then the quotient would be infinity!!!
long double ldquotient(long double ld1,long double ld2){return(ld1==M_LD_NAN||ld2==M_LD_NAN?M_LD_NAN:ld1/ld2);} // either undefined, quotient undefined
long double floatquotient(Mfloat* r1,Mfloat* r2){return(r1&&r2?ldquotient(r1->ld,r2->ld):M_LD_NAN);} // either undefined, product undefined
Mfloat* _floatquotient(Mfloat* r1,Mfloat* r2){return(r1||r2?_getFloat(floatquotient(r1,r2)):NULL);} // either undefined, product undefined

mp_err _qadd(Mrational* c,Mrational const * const a,Mrational const * const b){
    Mbiginteger *_num=NULL,*_num1=NULL,*_num2=NULL,*_den=NULL;
    ///////Mfloat* _delta=NULL;
    mp_err status=(a&&b&&c?MP_OKAY:MP_ERR); // we need both rationals
    if(status==MP_OKAY)status=_bimul(a->den,b->den,&_den); // multiply denominators to become the result denominator
    if(status==MP_OKAY)if(!_den||mp_iszero(_den)==MP_YES)status=MP_ERR; // and the denominator should be non-zero (division by zero is not possible)
    ////// OOPS the deltas are handled by _getRationalSum!!!! if(status==MP_OKAY){_delta=_floatsum(a->delta,b->delta);if(!_delta)if(a->delta||b->delta)status=MP_ERR;} // if we failed in adding the delta's error as well
    if(status==MP_OKAY)status=_bimul(a->num,b->den,&_num1); // multiply numerator of a with denominator of b for the plus term of the result numerator
    if(status==MP_OKAY)status=_bimul(a->den,b->num,&_num2); // multiply denominator of a with numerator of b for the min term of the result numerator
    if(status==MP_OKAY)status=_biadd(_num1,_num2,&_num); // add the numerator parts
    // loose the numerator parts (are not stored in the result rational anyway)
    ////outputInfo("Rational numerator parts to free.");
    if(_num1)free_biginteger(_num1);
    if(_num2)free_biginteger(_num2);
    ////outputInfo("Rational numerator parts freed.");
    if(status!=MP_OKAY){ // numerator and denominator not computed both
        ////outputInfo("Freeing new numerator, denominator and delta!");
        if(_num)free_biginteger(_num);
        if(_den)free_biginteger(_den);
        //////if(_delta)free_float(_delta);
        ////outputInfo("New numerator, denominator and delta freed!");
    }else{ // numerator and denominator computed
        ////outputBiginteger("Storing numerator '",_num,"'");outputBiginteger(" and denominator '",_den,"'.\n");
        // too bad we have to clear the current numerator and denominator pointers (if any)
        if(c&&c->num){if(amVerbose())outputInfo("Freeing previous numerator.");free_biginteger(c->num);}
        if(c&&c->den){if(amVerbose())outputInfo("Freeing previous denominator.");free_biginteger(c->den);}
        /////outputInfo("Previous numerator and denominator released.");
        c->num=_num;c->den=_den;
        /////outputInfo("Numerator and denominator stored.");
        // normalize the rational
        /////outputInfo("Normalizing the sum rational.");
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
        Mfloat* _delta=(a->delta?_getFloat(a->delta->ld):NULL);
        // if copying failed mark as error
        if(a->num&&!_num)status=MP_ERR;else if(a->den&&!_den)status=MP_ERR;else if(a->delta&&!_delta)status=MP_ERR;
        if(status==MP_OKAY){ // copies made successfully
            // move copies freeing all originals in the process
            if(b->num)free_biginteger(b->num);b->num=_num;
            if(b->den)free_biginteger(b->den);b->den=_den;
            if(b->delta)free_float(b->delta);b->delta=_delta;
            b->normalized=a->normalized;
        }else{
            // free all copies if made
            if(_num)free_biginteger(_num);
            if(_den)free_biginteger(_den);
            if(_delta)free_float(_delta);
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
            if(b->delta)free_float(b->delta);b->delta=NULL;
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
        if(floatIsUndefinedOrZero(q1->delta)&&floatIsUndefinedOrZero(q2->delta)){ // both are pure
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
        	_rational->delta=_floatsum(q1->delta,q2->delta);
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
        if(amVerbose()){outputRational("Subtracting '",q2,"'");outputRational(" from '",q1,"'.\n");}
        mp_err status=_qsub(_rational,q1,q2);
        if(status==MP_OKAY){
            if(amVerbose()){outputRational("Difference '",_rational,"'.\n");}
            // compute the delta
        	_rational->delta=_floatdifference(q1->delta,q2->delta);
            // if failed to compute the delta mark error
            if(q1->delta&&q2->delta)if(!_rational->delta){outputError("Failed to compute the difference of two rational deltas");status=MP_ERR;}
        }else
            outputError("Failed to subtract two pure rationals");
        if(status!=MP_OKAY){free_rational(_rational);_rational=NULL;outputError("Failed to compute the difference of two rationals");}
    }else
        outputError("Failed to create the rational for storing the difference of two rationals");
    return _rational;
}
/*
// MDH@23OCT2019 TODO: I believe we have a getRationalSign now, that is to replace qsign!!!!
long long qsign(Mrational* rational){
    if(!rational)return M_LL_INVALID;
    // assuming that the denominator is always positive we only have to consider the sign of (num/den)+delta=(num+den*delta/den), so the sign of num+den*delta
    // if delta is defined which is obviously not recommended at all
    if(floatIsUndefinedOrZero(rational->delta))return(isBigintegerZero(rational->num)?0:(mp_isneg(rational->num)?-1:1));
    // convert num and den to doubles
    long double lddelta=rational->delta->ld,ldnum=mp_get_long_double(rational->num),ldden=(rational->den?mp_get_long_double(rational->den):1);
    long double ldsign=(ldnum-lddelta*ldden);
    return(ldsign<0?-1:(ldsign>0?1:0));
}
// MDH@16OCT2019: what to return if a or b is not defined????? I suppose NULL is smaller than any value???????
//                qcmp() will return M_LL_INVALID if something goes wrong!!!
// MDH@23OCT2019 TODO: qcmp() also replaceable by getRationalSign() I guess
long long qcmp(Mrational const * const a,Mrational const * const b){
    mp_err status=MP_OKAY;
    mp_ord result=MP_EQ;
    if(a||b){ // not both NULL
        if(a&&b){ // both not NULL
            // a and b defined
            // taking deltas into account makes things more complex
            // now we can use _qsub and see from there
            Mrational* _rational=_getRationalDifference(a,b);
            if(_rational){
                // the sign of the rational determines what the result will be
                long long sign=qsign(_rational);
                if(sign!=M_LL_INVALID){if(sign<0)result=MP_LT;else if(sign>0)result=MP_GT;}else status=MP_ERR;
                free_rational(_rational);
            }else
                status=MP_MEM;
        }else // either a or b NULL 
        if(!a)result=MP_LT;
        else
        if(!b)result=MP_GT;
    }
    return(status==MP_OKAY?result:M_LL_INVALID);
}
*/
// computation of the actual delta is much harder in products and quotients
long double getLongDoubleRationalProduct(long double ld,Mrational const * const r){
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
long double getLongDoubleRationalQuotient(long double ld,Mrational const * const r){
    if(!r)return M_LD_NAN; // unlikely though as we're calling it ourselves with a defined rational
    if(ld!=M_LD_NAN){
        // multiply ld with the numerator to start with
        if(r->den)ld=ldproduct(ld,mp_get_long_double(r->den));
        // divide ld by the denominator
        if(r->num)ld=ldquotient(ld,mp_get_long_double(r->num));
    }
    return ld;
}
long double getLongDoubleRationalSum(long double ld,Mrational const * const r){
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
            // TODO switch over from using floatIsUndefinedOrZero() to isFloatUndefined() and isFloatZero()
        	bool delta1defined=!floatIsUndefinedOrZero(q1->delta),delta2defined=!floatIsUndefinedOrZero(q2->delta);
            // if at least one is defined, there will be a delta in the product
            if(delta1defined||delta2defined){
                // the delta is either a single term or the sum of three terms
                long double term1=(delta1defined?getLongDoubleRationalProduct(q1->delta->ld,q2):M_LD_NAN),term2=(delta2defined?getLongDoubleRationalProduct(q2->delta->ld,q1):M_LD_NAN);
                if(term1!=M_LD_NAN&&term2!=M_LD_NAN) // both terms are defined
                    _rational->delta=_getFloat(term1+term2+ldproduct(q1->delta->ld,q2->delta->ld));
                else
                if(term1!=M_LD_NAN) // term1 is defined
                    _rational->delta=_getFloat(term1);
                else // term2 is defined
                    _rational->delta=_getFloat(term2);
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
        	bool delta1defined=!floatIsUndefinedOrZero(q1->delta),delta2defined=!floatIsUndefinedOrZero(q2->delta);
            // TODO if either delta is defined the new rational will also have a delta!!!
            if(delta2defined){
                 // the new delta is the quotient of (delta1-r*delta2) and the value of q2
                 // NOTE that if delta1 is not defined there's no delta1 term and we pass M_LD_NAN into lddifference!!!
                 // TODO check whether using M_LD_NAN instead of 0 did NOT work if delta1 is not defined!!!!
                 long double ldNumerator=lddifference(delta1defined?q1->delta->ld:0,getLongDoubleRationalProduct(q2->delta->ld,_rational));
                 long double ldDenominator=getRationalLongDouble(q2);
                 if(amVerbose())output("New rational quotient delta: numerator=%*Lf - denominator=%*Lf.\n",LDBL_DIG,ldNumerator,LDBL_DIG,ldDenominator);
                _rational->delta=_getFloat(ldquotient(ldNumerator,ldDenominator));
            }else
            if(delta1defined)
                _rational->delta=_getFloat(getLongDoubleRationalQuotient(q1->delta->ld,q2));
        }else{
            free_rational(_rational);_rational=NULL;outputError("Failed to compute the quotient of two rationals");
        }
    }else
        outputError("Failed to create the rational for storing the quotient of two rationals.");
    return _rational;
}
// MDH@05NOV2019: if we want to divide a rational by a big integer
Mrational* _getRationalBigintegerQuotient(Mrational const * const q,Mbiginteger const * const b){
    Mrational* _rationalBigintegerQuotient=NULL;
    if(b&&!isBigintegerZero(b)){
        Mbiginteger* _newnum=_getBigintegerCopy(b);
        if(_newnum){
            if(!q->den||mp_mul(_newnum,q->den,_newnum)==MP_OKAY){
                long double newdelta=M_LD_NAN,qdelta=getFloatLongDouble(q->delta);
                if(qdelta!=M_LD_NAN)newdelta=qdelta/mp_get_long_double(b);
                _rationalBigintegerQuotient=_getRational(_getBigintegerCopy(q->num),_newnum,newdelta,true,true);
            }else
                outputError("Failed to multiply two big integers");
        }else 
            outputError("Failed to copy a big integer");
    }
    return _rationalBigintegerQuotient;
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

// MDH@10OCT2019: copied over from Mexecution.h/c

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
                        if(isBigintegerZero(_decimalPartBiginteger)!=M_TRUE){
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

void free_rational(Mrational* _rational){
    if(_rational){
        if(_rational->num)free_biginteger(_rational->num);
        if(_rational->den)free_biginteger(_rational->den);
        if(_rational->delta)free_float(_rational->delta);
        FREE(_rational,'R');
    }else
    if(amDebugging())outputInfo("No rational to free!");
}/* VALIDATED */

// TODO should we free the given big integers when they are NOT bound to the rational that is being returned????
void normalizeRational(Mrational* rational){
    if(!rational)return;
    // checking on the validity of the flag (which would actually be a bug)
    if(!rational->normalized&&!rational->den){outputBug("Normalized flag of rational not set although the denominator equals 1; flag set");rational->normalized=true;}
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
    if(!_rational)outputMemoryError("Failed to create a rational");
    if(_rational->num||_rational->den||_rational->delta)outputBug("New rational numerator, denominator and delta not all considered undefined");
    return _rational;
}/* VALIDATED */

// MDH@10OCT2019: guaranteeing now that any sign will always be present in the numerator and never in the denominator!
//                TODO currently always forcing the numerator to be nonnull
//                TODO currently NOT forcing the denominator to be NULL when it equals 1 (which is used throughout the code to indicate that the denominator equals 1)
Mrational* _getRational(Mbiginteger* _numerator,Mbiginteger* _denominator,long double delta,bool normalize,bool freeifunbound){
    // if the given numerator/denominator is NULL assume 1
    if(amVerbose()){outputBiginteger("Determining the rational with numerator ",_numerator,NULL);outputBiginteger(" and denominator ",_denominator,".\n");}
    // determine if the denominator is zero
    bool nonzeroDenominator=(!_denominator||isBigintegerZero(_denominator)!=M_TRUE);
    Mrational* _rational=(nonzeroDenominator?__rational():NULL);
    if(_rational){
        // get the delta in (for now we also store zero in a real i.e. the only requirement for delta is that it should be defined, i.e. not NaN or supernormal)
        if(!isLongDoubleUndefined(delta)){ // we need a delta
            _rational->delta=_getFloat(delta); // store the delta if a valid value
            if(!_rational->delta){free_rational(_rational);_rational=NULL;outputError("Failed to create the rational delta");}
        }
        if(_rational){
            Mbiginteger* _nonnullnumerator=(_numerator?_numerator:_getBiginteger(1));
            if(_nonnullnumerator){
                // MDH@15AUG2019: we prefer the numerator to be negative instead of the denominator
                // MDH@10OCT2019: TODO wouldn't it be better to be able to toggle the signs???? YES but I can't find a function in tommath to do so!!!!
                if(_denominator&&mp_isneg(_denominator)==MP_YES){ // the given denominator is negative             
                    if(amVerbose())outputInfo("Moving the sign from the denominator to the numerator of the rational.");
                    // we have to get negated versions of both the numerator and the denominator
                    // if we succeed in doing so we use those otherwise we stick to using the current ones
                    Mbiginteger *_negatedNumerator=_getBigintegerNeg(_nonnullnumerator),*_negatedDenominator=_getBigintegerNeg(_denominator);
                    // actually we need both or neither
                    if(_negatedNumerator&&_negatedDenominator){ // succeeded in negating the numerator and denominator
                        if(amVerbose())outputInfo("Using the negated numerator and denominator.");
                        _rational->num=_negatedNumerator;
                        _rational->den=_negatedDenominator;
                        // NOTE that we do NOT free _numerator explicitly but if _numerator is not null, _nonnullnumerator will be equal to it and we're freeing the right thing, if _numerator is NULL we're freeing the created _getBiginteger(1) which is also the right thing to do
                        //      I have to NULL both _numerator and _denominator here because if I don't an attempt to free them again when e.g. normalization fails could be catastrophic
                        //      alternatively I could simply toggle freeonfailure PREFERRED APPROACH
                        // NOTE even if freeonfailure is already false, we are returning a rational that does NOT use the presented numerator and denominator in which case we should free the numerator and denominator because the caller won't
                        //      if freeonfailure is true we should of course prevent freeing below (which won't happen though)
                        // MDH@10OCT2019: freeing of the 'originals' is only allowed when flag freeifunbound is set, BUT if we ourselves created the numerator we need to free it 
                        //                NOTE _nonnullnumerator is ALWAYS freed if it was created here as it should BUT I moved doing so outside!!
                        if(freeifunbound){
                            freeifunbound=false; // don't free again
                            free_biginteger(_numerator); // NOTE there might not be a non null _numerator though
                            free_biginteger(_denominator); // NOTE _denominator must be non null otherwise we wouldn't be here!!
                        }
                    }else{ // failed to create both negated versions
                        free_rational(_rational);_rational=NULL;
                        free_biginteger(_negatedNumerator);
                        free_biginteger(_negatedDenominator);
                        outputError("Failed to move the sign from the denominator to the numerator of a rational");
                    }
                    // essential to free any numerator we created ourselves!!!!
                    if(!_numerator)free_biginteger(_nonnullnumerator);
                }else{
                    _rational->num=_nonnullnumerator; // could be NULL now when it's the inverse of another rational
                    _rational->den=_denominator;
                }
                /* replacing what was obviously not always correct:
                if(_negatedNumerator&&_negatedDenominator){ // succeeded in negating the numerator and denominator
                    if(amVerbose())outputInfo("Using the negated numerator and denominator.");
                    _rational->num=_negatedNumerator;
                    _rational->den=_negatedDenominator;
                    // NOTE that we do NOT free _numerator explicitly but if _numerator is not null, _nonnullnumerator will be equal to it and we're freeing the right thing, if _numerator is NULL we're freeing the created _getBiginteger(1) which is also the right thing to do
                    //      I have to NULL both _numerator and _denominator here because if I don't an attempt to free them again when e.g. normalization fails could be catastrophic
                    //      alternatively I could simply toggle freeonfailure PREFERRED APPROACH
                    // NOTE even if freeonfailure is already false, we are returning a rational that does NOT use the presented numerator and denominator in which case we should free the numerator and denominator because the caller won't
                    //      if freeonfailure is true we should of course prevent freeing below (which won't happen though)
                    // MDH@10OCT2019: freeing of the 'originals' is only allowed when flag freeifunbound is set, BUT if we ourselves created the numerator we need to free it 
                    if(freeifunbound)free_biginteger(_denominator); // NOTE _denominator must be non null otherwise we wouldn't be here!!
                    if(!_numerator)free_biginteger(_nonnullnumerator);else if(freeifunbound)free_biginteger(_numerator);
                    freeifunbound=false; // don't free again
                }else{
                    _rational->num=_nonnullnumerator; // could be NULL now when it's the inverse of another rational
                    _rational->den=_denominator;
                    // if using the original numerator and denominator we still have to free any negated version we created
                    free_biginteger(_negatedNumerator);free_biginteger(_negatedDenominator);
                }
                */
                // NOTE _nonnullnumerator and _denominator NOW bound, so no need to free them anymore
                /* STORING 1 AS DENOMINATOR ISN'T WRONG per se 
                if(_denominator&&mp_cmp(_denominator,getBigintegerOne())==MP_EQ){
                    free_biginteger(_denominator); // won't store denominator equal to 1
                    _rational->den=NULL; // probably already is though
                }else // either NULL or not equal to 1
                    _rational->den=_denominator;
                */
               if(_rational){ // if we still have one (could have failed when we had to toggle the signs)
                    // last step: normalize if so requested
                    _rational->normalized=(!_rational->den||isBigintegerOne(_rational->num)); // if either numerator or denominator is NULL assume normalized!!!
                    if(normalize&&!_rational->normalized){
                        if(amVerbose())outputRational("Rational before normalization: ",_rational,".\n");
                        normalizeRational(_rational); // normalize the rational if we are supposed to
                        if(_denominator&&!_rational->normalized)outputError("Failed to normalize a rational");else if(amVerbose())outputRational("Rational after normalization: ",_rational,".\n");
                    }else
                    if(amVerbose())outputRational("Rational initialized: ",_rational,".\n");
               }
                ///////// AS LONG AS WE FREE THE RATIONAL IN THE ELSE PART NO NEED TO DO: return _rational; // return whether normalized or not
            }else{ // either _numerator NULL or _getBiginteger(1) NULL, in the last case nothing created that needs to be freed (except for _rational)
                free_rational(_rational);_rational=NULL;
                outputError(_numerator?"Undefined rational numerator":"Failed to create big integer 1");
            }
        }
    }else
        outputError((nonzeroDenominator?"Failed to create a rational":"Can't create a rational with a zero denominator"));
    // if we still have _rational here and the freeifunbound flag is (still) set, free the passed in numerator and denominator!!!
    if(!_rational)if(freeifunbound){free_biginteger(_numerator);free_biginteger(_denominator);}
    return _rational;
}/* VALIDATED */

// _getInverseRational() will take care of releasing the newly created rational parts when failing to wrap them in a rational
Mrational* _getInverseRational(Mrational const * const _rational){
    if(!_rational){outputError("No rational to invert");return NULL;}
    Mrational* _inverseRational=NULL;
    // for now only allow inverting pure rationals!!! with a delta that is either undefined or considered zero (might still be subnormal though)!!!
    if(isFloatUndefined(_rational->delta)==M_TRUE||isFloatZero(_rational->delta)==M_TRUE){
        Mbiginteger *_inverseNumerator=(_rational->den?_getBigintegerCopy(_rational->den):_getBiginteger(1)),*_inverseDenominator=(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1)); // free on failure
        if(_inverseNumerator&&_inverseDenominator)_inverseRational=_getRational(_inverseNumerator,_inverseDenominator,M_LD_NAN,!_rational->normalized,false);
        if(!_inverseRational){free_biginteger(_inverseNumerator);free_biginteger(_inverseDenominator);outputError("Failed to create the inverse rational");}else if(_rational->normalized)_inverseRational->normalized=true; // nasty TODO check if this is correct
    }else
        outputError("Can't invert an unpure rational");
    return _inverseRational;
}/* VALIDATED */

/* MDH@21OCT2019: see below for the corrected implementation (that actually computes the 'true' (real) numerator)
// MDH@08JUN2019 NOTE: adapted so that if the numerator is NULL will assume the numerator to equal 1
// BUT _getRational has been adapted to NOT allow a NULL numerator, i.e. replacing NULL with big integer 1, so actually a NULL numerator is unlikely to occur!!!
// rationals can equal zero or one but only when the delta value equals 0 (or is not defined which is the same)
bool isRationalZero(Mrational* _rational){
    // if the rational does not have a num, the numerator equals 1, and obviously is NOT zero
    return(_rational&&_rational->num?isBigintegerZero(_rational->num)&&floatIsUndefinedOrZero(_rational->delta):false); // the delta needs to be undefined (i.e. zero)
}// VALIDATED 

bool isRationalOne(Mrational* _rational){
    // when the numerator is NULL, it is considered to be equal to 1
    if(!_rational)return false;
    // basically a rational equals 1 if the numerator and denominator are the same
    if(_rational->delta)if(!ldIsZero(_rational->delta->ld)&&!ldIsNaN(_rational->delta->ld))return false; // TODO if the rational delta is not zero, do not consider to be equal to 1 (although theoretically it could be)
    return (_rational->den?mp_cmp(_rational->num,_rational->den)==MP_EQ:!_rational->num||isBigintegerOne(_rational->num));
    // replacing: return(_rational?!(_rational->num||isBigintegerOne(_rational->num))&&(!_rational->den||isBigintegerOne(_rational->den))&&(!_rational->delta||ldIsZero(_rational->delta->ld)):false);
}// VALIDATED
*/

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
        if(amVerbose())outputInfo("No rational to determine the text representation of.");
    outputChar('p');
    return _rationalText;
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

// NOTE the _ indicates that what is returned has to be freed after being used
Mbiginteger* _rational2biginteger(Mrational* _rational){
    Mbiginteger* _biginteger=(_rational?__biginteger():NULL);
    if(_biginteger){        
        if(_rational->den){
            // if the numerator is NULL or 0 _biginteger should remain what it is (i.e. 0)
            if(_rational->num&&isBigintegerZero(_rational->num)!=M_TRUE){
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

// when only interested in the end result, calling _getLongDoubleRational is the way to go
Mrational* _getLongDoubleRational(long double ld,int maxiter){
    // if iterations, you're supposed to return all iteration results
    Mrational* _rational=NULL; // the last (computed) rational
    if(isLongDoubleUndefined(ld)!=M_TRUE){ // not an undefined long double (might still be Inf though)
        long long ldSign=getLongDoubleSign(ld);
        if(ldSign!=M_ZERO){ // not zero
            bool neg=(ldSign==M_NEGATIVE);if(neg)ld=-ld; // if negative, reverse ld
            // ASSERT ld is positive
            // if we use p for pmin1 and q for qmin1 we do not need pmin1 and qmin1
            long long pmin1=1,qmin1=0,pmin2=0,qmin2=1;
            long long a,p,q; // p and q now store the initial values of pmin1 and qmin1
            long double delta,rem=ld;
            // ascertain to execute the following at least once (so when maxiter<=1 we at least get the integer part of the rational)
            // MDH@09OCT2019: if maxiter equals zero never stop
            int i=0;
            while(maxiter<=0||i<maxiter){
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
        if(amVerbose())output("Rational numerator converted to long double '%.*Lf'.\n",LDBL_DIG,ldNumerator);
        if(isLongDoubleUndefined(ldNumerator)==M_FALSE){ // not undefined i.e. supposedly defined (although it could still be infinity theoretically)
            // if we do NOT have a denominator (i.e. the denominator equals one we only need to add the delta (if any))
            if(!_rational->den){if(_rational->delta)ldNumerator+=_rational->delta->ld;return ldNumerator;}
            // ASSERT a denominator is present
            long double ldDenominator=mp_get_long_double(_rational->den);
            if(isFloatUndefined(_rational->delta)!=M_TRUE&&isFloatZero(_rational->delta)!=M_TRUE)ldNumerator+=(ldDenominator*_rational->delta->ld); // if delta is NOT undefined and NOT zero, add the denominator multiplied by the delta to the numerator
            // a denominator which is not equal to 1
            if(amVerbose())output("Rational denominator converted to long double '%.*Lf'.\n",LDBL_DIG,ldDenominator);
            if(isLongDoubleUndefined(ldDenominator)!=M_TRUE&&isLongDoubleZero(ldDenominator)!=M_TRUE)return ldNumerator/ldDenominator; // NOTE the denominator won't equal 0 so this should be Ok but testing it just the same
            if(amVerbose())outputError("Failed to convert a rational denominator to a long double");
        }
        if(amVerbose())outputError("Failed to convert a rational numerator to a long double");
    }
    return M_LD_NAN; // if something went wrong
}/* VALIDATED */

// functions for testing the sign that return false if rational is not defined, and assume that the sign of the numerator is the sign of the rational (i.e. the denominator should never be negative!!)
// MDH@22OCT2019: how about delegating to sign???
long double getUnpureRationalNumerator(Mbiginteger* numerator,Mbiginteger* denominator,long double ld){
    if(numerator){
        if(ld!=M_LD_NAN){ // as it should be
            // an 'unpure' (fake) rational, we're forced to use ld to compute the 'true' numerator
            if(denominator){ // the denominator isn't 1
                long double ldDenominator=mp_get_long_double(denominator);
                if(isLongDoubleUndefined(ldDenominator)==M_TRUE){outputError("Failed to convert a rational denominator to a long double.");return false;}
                if(isLongDoubleOne(ldDenominator)!=M_TRUE)ld*=ldDenominator; // if the denominator doesn't equal 1 multiply ld by it
            }
            long double ldNumerator=mp_get_long_double(numerator);
            if(isLongDoubleUndefined(ldNumerator)==M_TRUE){outputError("Failed to convert a rational numerator to a long double.");return false;}
            return ld+ldNumerator;
        }
    }
    return M_LD_NAN;
}
/* MDH@25OCT2019: replaced by getLongDoubleSign() in Mexecution.h/c
long long ldsign(long double ld){
    if(ld==M_LD_NAN)return M_LL_INVALID;
    if(ld>0)return M_POSITIVE;
    if(ld<0)return M_NEGATIVE;
    return M_ZERO;
}
*/
// if the rational has an associated delta, it's a bit more complicated, converting to a real of the numerator and denominator might fail or be inaccurate if the big integers are too large
long long isRationalUndefined(Mrational const * const rational){return(rational&&rational->num?M_FALSE:M_TRUE);}
long long getRationalSign(Mrational const * const rational){
    // ASSERT if rational->num should always be defined (even if it equals 1), if it is not it is not a valid rational!!!!
    long long rationalSign=M_LL_INVALID;
    if(isRationalUndefined(rational)==M_FALSE){ // the rational is defined (so it is not NULL and has a non NULL numerator)
        long double ld=getFloatLongDouble(rational->delta);
        rationalSign=(isLongDoubleUndefined(ld)==M_TRUE?getBigintegerSign(rational->num):getLongDoubleSign(getUnpureRationalNumerator(rational->num,rational->den,ld)));
    }
    if(amVerbose()){outputRational("Sign of rational '",rational,"':");output("%lld.\n",rationalSign);}
    return rationalSign;
}
long long isRationalPositive(Mrational const * const rational){
    long long rationalSign=getRationalSign(rational);
    return(rationalSign==M_LL_INVALID?M_LL_INVALID:(rationalSign==M_POSITIVE?M_TRUE:M_FALSE));
}
long long isRationalNegative(Mrational const * const rational){
    long long rationalSign=getRationalSign(rational);
    return(rationalSign==M_LL_INVALID?M_LL_INVALID:(rationalSign==M_NEGATIVE?M_TRUE:M_FALSE));
}
long long isRationalZero(Mrational const * const rational){
    long long rationalSign=getRationalSign(rational);
    return(rationalSign==M_LL_INVALID?M_LL_INVALID:(rationalSign==M_ZERO?M_TRUE:M_FALSE));
}

long long isRationalOne(Mrational const * const rational){
    long long result=M_LL_INVALID;
    if(isRationalUndefined(rational)!=M_TRUE){
        if(amVerbose())outputRational("Checking if '",rational,"' equals one");
        long double ld=getFloatLongDouble(rational->delta);
        // without a delta, a rational equals 1 when the numerator and denominator are the same
        if(isLongDoubleUndefined(ld)!=M_TRUE&&isLongDoubleZero(ld)!=M_TRUE) // neither undefined, nor zero
            result=(isLongDoubleOne(getUnpureRationalNumerator(rational->num,rational->den,ld))?M_TRUE:M_FALSE);
        else // undefined or zero
            result=(rational->den?(rational->num?mp_cmp(rational->den,rational->num)==MP_EQ:isBigintegerOne(rational->den)):(rational->num?isBigintegerOne(rational->num):M_TRUE));
        if(amVerbose())output(": %s.\n",(result==M_LL_INVALID?"UNKNOWN":(result==M_TRUE?"YES":"NO")));
    }
    return result;
}

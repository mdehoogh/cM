#include <stdint.h>
#include <time.h>

#include "Mfunctions.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_FUNCTIONS,id};}

extern char const * const VALUETYPENAMES[];
extern char const * const M_ERROR_PREFIX;
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_ZERO,M_NEGATIVE,M_POSITIVE;
extern const long double M_LD_NAN,M_LD_PI;
extern const Mdecimalcontext* M_DECIMALCONTEXT; // M.c takes care of creating the application-wide decimal context

void outputDecimalStatus(uint32_t status){
	if(status>0){
		outputInfo("Decimal computations error report.");
		if(status&MPD_IEEE_Invalid_operation)outputInfo("\tIEEE Invalid operation error.");
		if(status&MPD_Clamped)outputInfo("\tClamped error.");
		if(status&MPD_Division_by_zero)outputInfo("\tDivision by zero error.");
		if(status&MPD_Fpu_error)outputInfo("\tFPU error.");
		if(status&MPD_Inexact)outputInfo("\tInexact error.");
		if(status&MPD_Not_implemented)outputInfo("\tNot implemented error.");
		if(status&MPD_Overflow)outputInfo("\tOverflow error.");
		if(status&MPD_Rounded)outputInfo("\tRounding error.");
		if(status&MPD_Subnormal)outputInfo("\tSubnormal error.");
		if(status&MPD_Underflow)outputInfo("\tUnderflow error.");
	}else
		outputInfo("No decimal context errors.");
}

// applying unary operators by means of functions
// math functions: independent of the execution environment but still receive it...
// TODO how about applying the functions to decimals and rationals
/*
returns the largest integer equal to or smaller than \p _value
\parameter _value the value to floor
*/
// MDH@28MAY2020: we can either return a new value but preferably we should return the same value NOTE we can do that because values are immutable but can be reused
Mvalue* Mfloor(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
        /* replacing:
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        */
        // composite types
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mfloor));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mfloor));
        // scalar values
        if(_value->type==VT_FLOAT)return _getFloatValue(floorl(_value->value._float->ld));
        if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,true,false));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,true,false));
    }
    return NULL;
}
Mvalue* Mtrunc(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
        /* replacing:
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtrunc));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtrunc));
        if(_value->type==VT_FLOAT)return _getFloatValue(truncl(_value->value._float->ld));
        if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,true,true));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,true,true));
    }
    return NULL;
}
/*
 * returns integer equal to or larger than
 */
Mvalue* Mceil(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
        /* replacing:
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil));
        if(_value->type==VT_FLOAT)return _getFloatValue(ceill(_value->value._float->ld));
        if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,false,false));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,false,false));
    }
    return NULL;
}

/*
 * returns nearest integer value
 */
static Mbiginteger* _getRoundedDecimalInteger(Mdecimal* _decimal){Mallocationowner owner=getOwner(__LINE__);
    Mbiginteger* _roundedDecimalInteger=NULL;
    if(_decimal){
        Mdecimalcontext* decimalcontext=_getDecimalcontext(_decimal->prec);
        mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
        if(mpd_context){
            Mdecimal* _roundDecimal=(Mdecimal*)OWNED(__decimal(mpd_context,0,0),owner);
            if(_roundDecimal){
                uint32_t status=0;
                mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
                if((status&0xEFBF)==0){
                    long long dll=decimal2long(_roundDecimal);
                    _roundedDecimalInteger=owned_biginteger(_getBiginteger(dll),owner);
                }else{
                    output("%s",M_ERROR_PREFIX);outputDecimal("Failed to round decimal '",_decimal,"'");
                    output(" (status: %.8x).\n",status);
                }
                FREE_DECIMAL(_roundDecimal,owner);
            }
        }else
            outputError("No context available for rounding a decimal");
    }
    return disowned_biginteger(_roundedDecimalInteger,owner); 
}
Mvalue* Mround(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
        /* replacing:
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mround));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mround));
        if(_value->type==VT_FLOAT)return _getFloatValue(roundl(_value->value._float->ld));
        if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRoundedRationalInteger(_value->value._rational));
        if(_value->type==VT_DECIMAL)return _getValueOfBiginteger(_getRoundedDecimalInteger(_value->value._decimal));
    }
    return NULL;
}

Mvalue* Msin(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(amVerbose()){outputValue("Applying sin() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msin));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msin));
        if(_value->type==VT_RATIONAL)return _getValueOfRational(_qsinorcos(_value->value._rational,true));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dsine(NULL,_value->value._decimal)); // match the precision as used by the argument
            /* replacing what was way to slow:
            uint32_t status=0;
            Mdecimal* _sineDecimal=__decimal(_decimalContext,0,0);
            // let's create ALL the decimals we're going to need for intermediate results
            Mdecimal *_qDecimal=__decimal(_decimalContext,0,0),*_twoDecimal=__decimal(_decimalContext,2,0),*_squareDividedByPiDecimal=__decimal(_decimalContext,0,0);
            Mdecimal *_2nplus1Decimal=__decimal(_decimalContext,1,0),*_dividedByPiDecimal=__decimal(_decimalContext,0,0); // the denominator starts equal to 1 (which we can 'reuse' in computing the first numerator (1-(x/pi)^2))
            Mdecimal *_numeratorDecimal=__decimal(_decimalContext,0,0),*_denominatorDecimal=__decimal(_decimalContext,1,0),*_multiplierDecimal=__decimal(_decimalContext,0,0);
            Mdecimal *_piDecimal=pi_decimal(NULL);
            if(_sineDecimal&&_qDecimal&&_twoDecimal&&_squareDividedByPiDecimal&&_numeratorDecimal&&_denominatorDecimal&&_multiplierDecimal&&_2nplus1Decimal&&_dividedByPiDecimal&&_piDecimal){
                if(amVerbose())outputInfo("Ready to compute the sine of a decimal.");
                // to use the product formula I found on internet at matrixlab-examples.com I need to compute (x/pi)^2, I suppose I need to subtract 2*pi until the result is between -pi and pi
                // so if we divide the input by pi we get a value that should be between -1 and 1, so we have to divide it by pi and use the remainder
                Mdecimal* _decimal=_value->value._decimal;
                // determine if _decimal is negative, if it is we negate it
                int neg=mpd_isnegative(_decimal->mpd);
                Mdecimal* _negatedDecimal=(neg?__adecimal():_decimal);if(!_negatedDecimal)status=1;else if(neg)mpd_qcopy_negate(_negatedDecimal->mpd,_decimal->mpd,&status);
                if((status&0xEFBF)==0){
                    if(amVerbose())outputInfo("Determining the normalized decimal to use as argument of the sine approximation.");
                    // NOTE the remainder is the starting value of the sine approximation
                    mpd_qdivmod(_qDecimal->mpd,_sineDecimal->mpd,_negatedDecimal->mpd,_piDecimal->mpd,_decimalContext,&status);
                    if((status&0xEFBF)==0){
                        if(!isDecimalZero(_sineDecimal)){ // non-zero input
                            if(amVerbose())outputDecimal("Number of integer multiples of pi: '",_qDecimal,"'.\n");
                            // if the result of the integer division by pi is odd we have to negate the result as well
                            int odd=mpd_isodd(_qDecimal->mpd);
                            if(amVerbose())outputDecimal("Ready to approximate the sine of non-zero decimal '",_sineDecimal,"'.\n");
                            // by dividing the initial value by pi we get z/pi which we need to square to get the c part of (1-c)/1 which is the first product multiplier
                            mpd_qdiv(_dividedByPiDecimal->mpd,_sineDecimal->mpd,_piDecimal->mpd,_decimalContext,&status);
                            if((status&0xEFBF)==0){
                                if(amVerbose())outputDecimal("Divided by pi: '",_dividedByPiDecimal,"'.\n");
                                // we know the sign of the sine will be positive for angles in (0,PI), so we can safely make the result negative if dealing with a negative input value
                                // the initial value of the sine decimal is the product of _negatedDecimal and the square of _dividedByPiRemainderDecimal
                                mpd_qmul(_squareDividedByPiDecimal->mpd,_dividedByPiDecimal->mpd,_dividedByPiDecimal->mpd,_decimalContext,&status);
                                if((status&0xEFBF)==0){
                                    /////////if(amVerbose())outputInfo("Product multiplier numerator subcomputed.");
                                    mpd_qsub(_numeratorDecimal->mpd,_denominatorDecimal->mpd,_squareDividedByPiDecimal->mpd,_decimalContext,&status);
                                    if((status&0xEFBF)==0){
                                        if(amVerbose())outputDecimal("Multiplier: '",_numeratorDecimal,"' -> ");
                                        // next we multiply the initial value of the product by the numerator alone (because the denominator is still equal to 1)               
                                        mpd_qmul(_sineDecimal->mpd,_sineDecimal->mpd,_numeratorDecimal->mpd,_decimalContext,&status);
                                        ///////////////if(amVerbose())outputDecimal("Second approximation to the sine: '",_sineDecimal,"'.\n");
                                        int64_t count=M_LL_MAX;
                                        while((status&0xEFBF)==0){
                                            if(--count==0){outputInfo("Maximum number of iterations exceeded!");break;}
                                            if(amVerbose())outputDecimal("Sine approximation: '",_sineDecimal,"'.\n");
                                            mpd_qadd(_2nplus1Decimal->mpd,_2nplus1Decimal->mpd,_twoDecimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break; // add 2 to 2n+1 to get 2(n+1)+1 so becoming 3, 5, 7, ....
                                            // add _2nplus1Decimal to the numerator and denominator
                                            mpd_qadd(_numeratorDecimal->mpd,_numeratorDecimal->mpd,_2nplus1Decimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
                                            if(amVerbose())outputDecimal("Numerator: '",_numeratorDecimal,"'. ");
                                            mpd_qadd(_denominatorDecimal->mpd,_denominatorDecimal->mpd,_2nplus1Decimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
                                            if(amVerbose())outputDecimal("Denominatator: '",_denominatorDecimal,"'. ");
                                            // if the numerator equals the denominator we're actually done because that means that the multiplier will from now on equal 1
                                            if(mpd_qcmp(_numeratorDecimal->mpd,_denominatorDecimal->mpd,&status)==0)break;if((status&0xEFBF)!=0)break;
                                            mpd_qdiv(_multiplierDecimal->mpd,_numeratorDecimal->mpd,_denominatorDecimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
                                            if(amVerbose())outputDecimal("Multiplier: '",_multiplierDecimal,"' -> ");
                                            if(isDecimalOne(_multiplierDecimal))break; // we've reached the end within the given precision
                                            // multiply _sineDecimal with the multiplier
                                            mpd_qmul(_sineDecimal->mpd,_sineDecimal->mpd,_multiplierDecimal->mpd,_decimalContext,&status);
                                        }
                                        if((status&0xEFBF)!=0){free_decimal(_sineDecimal);_sineDecimal=NULL;}
                                    }
                                }
                                // we need to negate the result if neg and odd are different
                                if(_sineDecimal)if(neg^odd)mpd_set_negative(_sineDecimal->mpd);
                            }
                        }
                    }
                }
                if(neg)free_decimal(_negatedDecimal); // the negatedDecimal we created should be released
                // alternatively we can get the sign first because sin(x)=-sin(-x), so if x is negative, we determine -x and determine the remainder of x/pi which will be in (0,1) as we want it to ()
            }else
                outputError("Failed to create at least one of the help decimal in computing the sine of a decimal");
            // free all help decimals
            free_decimal(_piDecimal);
            free_decimal(_2nplus1Decimal);
            free_decimal(_multiplierDecimal);
            free_decimal(_denominatorDecimal);
            free_decimal(_numeratorDecimal);
            free_decimal(_dividedByPiDecimal); // might still be around
            free_decimal(_squareDividedByPiDecimal);
            free_decimal(_twoDecimal);
            free_decimal(_qDecimal);
            */
    }
    return NULL;
}/* NOT VALIDATED */
Mvalue* Mcordicsin(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(amVerbose()){outputValue("Applying cordicsin() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        /* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordicsin));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordicsin));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcordicsine(NULL,_value->value._decimal));
    }
    return NULL;
}/* NOT VALIDATED */
Mvalue* Mcordiccos(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(amVerbose()){outputValue("Applying cordiccos() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        /* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordicsin));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordicsin));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcordiccosine(NULL,_value->value._decimal));
    }
    return NULL;
}/* NOT VALIDATED */

Mvalue* Mcos(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(cosl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(cos(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcos));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcos));
        if(_value->type==VT_RATIONAL)return _getValueOfRational(_qsinorcos(_value->value._rational,false));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcosine(NULL,_value->value._decimal));
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtan(Mvalue*  _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        // composite application
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtan));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtan));
        // scalar arguments
        if(_value->type==VT_FLOAT)return _getFloatValue(tanl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(tan(_value->value._integer->ll));
        if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dtangent(NULL,_value->value._decimal));
        if(_value->type==VT_RATIONAL){
            Mrational* _sineRational=owned_rational(_qsinorcos(_value->value._rational,true),owner);
            Mrational* _cosineRational=owned_rational(_qsinorcos(_value->value._rational,false),owner);
            Mrational* _tanRational=owned_rational(_getRationalQuotient(_sineRational,_cosineRational),owner);
            FREE_RATIONAL(_sineRational,owner);
            FREE_RATIONAL(_cosineRational,owner);
            return _getValueOfRational(disowned_rational(_tanRational,owner));
        }
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcosh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(coshl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(cosh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcosh));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcosh));
    }
    return NULL;
}/* VALIDATED */
Mvalue* Msinh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(sinhl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sinh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msinh));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msinh));
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtanh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(tanhl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(tanh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtanh));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtanh));
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mexp(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp));
        // use decimal conversion
        Mdecimal* _decimal=getValueDecimal(_value);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
        if(_decimal){
            Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
            if(_result){
                uint32_t status=0;
                mpd_qexp(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    FREE_DECIMAL(_result,owner);_result=NULL;
                    outputError("Failed to apply the exp function to a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
            return _getValueOfDecimal(disowned_decimal(_result,owner));
        }
    }
    return NULL;
}
// internal approximation by series expansion
Mvalue* Mdexp(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp));
        // use decimal conversion
        Mdecimal* _decimal=getValueDecimal(_value);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
        if(_decimal){
            Mdecimal* _result=owned_decimal(_dexp(NULL,_decimal),owner);
            if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
            return _getValueOfDecimal(disowned_decimal(_result,owner));
        }
    }
    return NULL;
}
Mvalue* Mlog(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(logl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(log(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog));
        Mdecimal* _decimal=getValueDecimal(_value);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
        if(_decimal){
            Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
            if(_result){
                uint32_t status=0;
                mpd_qln(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    FREE_DECIMAL(_result,owner);_result=NULL;
                    outputError("Failed to compute the natural logarithm of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
            return _getValueOfDecimal(disowned_decimal(_result,owner));
        }
    }
    return NULL;
}
Mvalue* Mlog10(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(log10l(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(log10(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog10));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog10));
        Mdecimal* _decimal=getValueDecimal(_value);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
        if(_decimal){
            Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
            if(_result){
                uint32_t status=0;
                mpd_qlog10(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    FREE_DECIMAL(_result,owner);_result=NULL;
                    outputError("Failed to compute the base 10 logarithm of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
            return _getValueOfDecimal(disowned_decimal(_result,owner));
        }
    }
    return NULL;
}

Mvalue* Msqrt(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(sqrtl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sqrt(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msqrt));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msqrt));
        // the square root of big integer, decimal and rational values has to be computed by conversion to decimals first
        // TODO although for rationals we could divide the square root of the numerator by the square root of the denominator
        // we've got a function in Mdecimal.h/c to explicitly convert a value (if possible) to a decimal (if the value wraps a decimal that is returned (instead of a new copy of this wrapped decimal) and that decimal should NOT be freed (see below))
        Mdecimal* _decimal=getValueDecimal(_value);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
        if(_decimal){
            Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
            if(_result){
                uint32_t status=0;
                mpd_qsqrt(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    FREE_DECIMAL(_result,owner);_result=NULL;
                    outputError("Failed to compute the square root of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
            return _getValueOfDecimal(disowned_decimal(_result,owner));
        }
    }
    return NULL;
}
// two-argument power function (complicates things considerably)
// TODO check different types convert to long double and use powl to compute the power!!
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue){Mallocationowner owner=getOwner(__LINE__);
    if(_value&&_exponentValue){
        if(_value->type==VT_FLOAT&&_exponentValue->type==VT_FLOAT)return _getFloatValue(powl(_value->value._float->ld,_exponentValue->value._float->ld));
        if(_value->type==VT_INTEGER&&_exponentValue->type==VT_INTEGER)return _getFloatValue(pow(_value->value._integer->ll,_exponentValue->value._integer->ll));
    }
    return NULL;
}
// end math functions

Mvalue* Mneg(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__); // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_FLOAT)return _getFloatValue(-_value->value._float->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg));
        if(_value->type==VT_BIGINTEGER)return _getValueOfBiginteger(_getNegatedBiginteger(_value->value._biginteger));
        if(_value->type==VT_RATIONAL){
            // this is done by negating the numerator but if the numerator equals NULL we should use -1
            Mrational* rational=_value->value._rational;
            if(rational){
                /////////outputValue("Negating rational '",_value,"'.\n");
                Mbiginteger* _biNumerator=owned_biginteger(rational->num?_getNegatedBiginteger(rational->num):_getBiginteger(-1),owner); // negating the numerator
                if(_biNumerator){
                    ////////outputBiginteger("Denominator '",_biDenominator,"' copied!\n");
                    Mrational* _negRational=_getRational(_biNumerator,rational->den,(isFloatUndefined(rational->delta)==M_TRUE?M_LD_NAN:-rational->delta->ld),false);
                    FREE_BIGINTEGER(_biNumerator,owner);
                    return _getValueOfRational(disowned_rational(_negRational,owner));
                }
                outputError("Failed to negate the numerator of a rational");
            }
        }else
        if(_value->type==VT_DECIMAL){
            Mdecimal* decimal=_value->value._decimal;
            if(decimal){
                Mdecimal* _negDecimal=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
                if(_negDecimal){
                    uint32_t status=0;
                    mpd_qcopy_negate(_negDecimal->mpd,decimal->mpd,&status);
                    if(status&0xEFBF){
                        FREE_DECIMAL(_negDecimal,owner);_negDecimal=NULL;
                        outputError("Failed to negate a decimal");
                        outputDecimalStatus(status);
                    }else // success, ascertain to copy the repeating field over as that remains the same on negating (assumedly)
                        _negDecimal->repeating=decimal->repeating;
                    if(_negDecimal)return _getValueOfDecimal(disowned_decimal(_negDecimal,owner));
                }
            }
        }
    }
    return NULL;
}/* VALIDATED */

Mvalue* Mnot(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__); // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot));
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot));
    }
    return NULL;
}/* VALIDATED */
// TODO can we not a string??????
Mvalue* Mbnot(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__); // not a value
    if(!_value)return NULL;
    if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
    if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot));
    if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot));
    return NULL;
}/* VALIDATED */

Mvalue* Mnull(Mvalue* _value){
    return _getIntegerValue(isValueNull(_value)?M_TRUE:M_FALSE);
}/* VALIDATED */
Mvalue* Mundefined(Mvalue* _value){
    return _getIntegerValue(isValueUndefined(_value)?M_TRUE:M_FALSE);
}/* VALIDATED */ // MDH@18JUL2019: isUndefined() now comes in handy
Mvalue* Msign(Mvalue* value){
    return(_getIntegerValue(getValueSign(value)));
} /* VALIDATED */
// TODO use the sign in Mzero, Mpositive and Mnegative
Mvalue* Mzero(Mvalue* _value){
    return(_getIntegerValue(_value?(isValueZero(_value)==M_TRUE?M_TRUE:M_FALSE):M_LL_INVALID));
}/* VALIDATED */
Mvalue* Mpositive(Mvalue* _value){
    return _getIntegerValue(_value?(isValuePositive(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
Mvalue* Mnegative(Mvalue* _value){
    return _getIntegerValue(_value?(isValueNegative(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
Mvalue* Mscalar(Mvalue* _value){
    return _getIntegerValue(_value?(isValueScalar(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */

// MDH@29OCT2020: might come in handy
long long isnumeric(Mvalue* _value){
    return(_value?(_value->type==VT_BIGINTEGER||_value->type==VT_DECIMAL||_value->type==VT_FLOAT||_value->type==VT_INTEGER||_value->type==VT_RATIONAL?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
Mvalue* Misnumeric(Mvalue* _value){
    // MDH@04DEC2020: delegate to local helper function isnumeric 
    return _getIntegerValue(isnumeric(_value));
}/* VALIDATED */
Mvalue* Misalist(Mvalue* _value){
    return _getIntegerValue(_value?(_value->type==VT_LIST?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */

// TODO the length of a text is the number of characters in a text????
// MDH@17OCT2019: the length of a list should now return the index of the last element (instead of the number of non-null values)
//                because doing so means appending a value with l[len(l)+1] will do so, instead of overwriting some value!!!!
// MDH@20NOV2020: for lists, also return the number of elements
Mvalue* Mlen(Mvalue* _value){
    long long result=M_LL_INVALID;
    if(_value){
        switch(_value->type){
            case VT_ARRAY:result=_value->value._array->numberOfElements;break;
            case VT_LIST:result=_value->value._list->numberOfElements;break; //(_value->value._list->_last?_value->value._list->_last->index:0);break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            case VT_TEXT:result=strlen(_value->value._text->_c);break;
            default:break;
        }
    }
    return _getIntegerValue(result);
}/* VALIDATED */
// MDH@30NOV2020: convenient if we can change the length of an array of string
Mvalue* Msetlen(Mvalue* _value,Mvalue* newlength_value){Mallocationowner owner=getOwner(__LINE__);
    // a length should always be a nonnegative integer
    long long result=M_LL_INVALID;
    if(_value){
        long long newlength=getValueInteger(newlength_value);
        if(newlength>=0){
            switch(_value->type){
                case VT_ARRAY:
                    {
                        Marray* array=_value->value._array;
                        unsigned long long length=array->numberOfElements;
                        if(length!=newlength){
                            unsigned long long l=MAX(length,newlength); // guaranteed to be positive, will equal length if newlength equals 0!!!
                            // we can't cut off values until we managed to get new memory
                            result=newlength;
                            if(newlength>0){
                                Mvalue** newvalues=CALLOC(sizeof(Mvalue*),newlength,-'a',owner);
                                if(newvalues){
                                    do{
                                        l--;
                                        // all values ABOVE newlength will not be used anymore
                                        // all values below length will still be used
                                        if(l>=newlength)assignValue(&array->values[l],NULL);else if(l<length)newvalues[l]=array->values[l];
                                    }while(l>0);
                                    // NULL any value reference in the current array's values NOT included in the new values
                                    FREE_DISOWNED(array->values,length,-'a',Msubowner(getValueOwner(),1)); // get rid of the current values
                                    array->values=newvalues; // make values point to the new values
                                    array->numberOfElements=newlength; // remember the new length
                                    result-=length; // the change in number of elements is the result of the function
                                }
                            }else{ // deleting all values, so no need to try to allocate sufficient memory
                                result-=length;
                                while(l>0)assignValue(&array->values[--l],NULL); // getting rid of all value pointers (in effect decrementing the counts of all the values!!!!)
                                FREE(array->values,length,-'a'); // get rid of the current values                            
                                array->numberOfElements=0;
                                array->values=NULL;
                            }
                        }else // no need to change
                            result=0;
                    };break;
                case VT_LIST:
                    {
                        Mlist* list=_value->value._list;
                        unsigned long long length=list->numberOfElements;
                        result=newlength;
                        result-=length;
                        if(result>0){ // expanding a list, which essentially is not possible unless we append NULL value references
                            while(length<newlength&&appendedToList(list,Msubowner(getValueOwner(),1),NULL,M_LL_INVALID)>0)length++;
                            result-=(newlength-length); // decrement result with what we couldn't append!!!
                        }else
                        if(result<0){ // shortening a list means removing this number of elements
                            // find the first list element to free
                            if(newlength>0){
                                list->numberOfElements=newlength;
                                // determine the last element to keep
                                Mlistelement *listelement=list->_first;
                                while(--newlength>0)listelement=listelement->_next;
                                list->_last=listelement;
                                free_listelement(listelement->_next,list->weak);
                                listelement->_next=NULL;
                            }else{ // completely empty the list
                                free_listelement(list->_first,list->weak);
                                list->numberOfElements=0;
                                list->_first=NULL;
                                list->_last=NULL;
                            }
                        }
                    };break;
                case VT_TEXT:
                    {
                        unsigned long long length=strlen(_value->value._text->_c);
                        if(length!=newlength){
                            // we're going to copy the text out of it, change it and reset it
                            Mstring* _text=owned_string(__string(),owner);
                            if(_text){
                                Mstring* p=_text;
                                result=newlength;
                                result-=length;
                                p=string_append_char(p,_value->value._text->presuffix);
                                p=string_append(_text,_value->value._text->_c);
                                if(length<newlength){
                                    while(length<newlength){p=string_append_char(p,' ');if(!p)break;length++;}
                                }else
                                    p=string_setlength(p,newlength);
                                if(p){ // text successfully lengthened or shortened
                                    // now that we've copied the current text content over, we can free it BEFORE replacing it!!!
                                    free_text(_value->value._text);
                                    _value->value._text=owned_text(_getText(string(_text)),Msubowner(getValueOwner(),1));
                                    result-=(newlength-length);
                                }else // failure
                                    result=M_LL_INVALID;
                                FREE_STRING(_text,owner);
                            }
                        }
                    };break;
                default:break;
            }
        }
    }
    return _getIntegerValue(result);
}

// 25OCT2019: get the length of a text with M's tl function
Mvalue* Mtl(Mvalue* _value){
    long long result=M_LL_INVALID;
    if(_value){if(_value->type==VT_TEXT)result=strlen(_value->value._text->_c);else if(_value->type==VT_TOKEN)result=string_length(_value->value._token->text);}
    return _getIntegerValue(result);
}/* VALIDATED */

// MDH@29MAY2019: how about forcing the result to be a big integer instead of a long double?????
Mvalue* Mfacd(Mvalue* _value){
    // Stirling formula to compute the number of factorial digits in n!: return 
    // get the integer out of the value
    long long ll=getValueInteger(_value);
    return (ll>0?_getIntegerValue(floor( ((ll+0.5)*log(ll) - ll + 0.5*log(2*M_LD_PI))/log(10) ) + 1):NULL);
}/* VALIDATED */

// TODO remember intermediate values in some list, that we can use as starting point
Mvalue* Mfac(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(!_value){
        if(amVerboseDebugging())
            outputInfo("No argument to factorial() function!");
        return NULL;
    }
    if(amVerboseDebugging())
        outputValue("Argument of factorial() function: '",_value,"'.\n");
    if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER)
    {outputValue("\nERROR: Non-integer argument '",_value,"' to factorial() function!");return NULL;}
    // some special cases (i.e. the input number is smaller than 2)
    Mbiginteger* _finalmultiplier=NULL;
    if(_value->type==VT_INTEGER){
        if(_value->value._integer->ll<0)
        {outputError("Invalid (negative) integer argument to factorial() function");return NULL;}
        if(_value->value._integer->ll<3)
            return _getIntegerValue(_value->value._integer->ll);
        _finalmultiplier=owned_biginteger(_getBiginteger(_value->value._integer->ll),owner);
    }else{
        if(mp_isneg(MP_INT_POINTER(_value->value._biginteger)))
        {outputError("Invalid (negative) big integer argument to factorial() function");return NULL;}
        if(mp_cmp(MP_INT_POINTER(_value->value._biginteger),MP_INT_POINTER(getBigintegerThree()))==MP_LT)
            return _getValueOfBiginteger(_getBigintegerCopy(_value->value._biginteger));
        _finalmultiplier=owned_biginteger(_getBigintegerCopy(_value->value._biginteger),owner);
    }
    if(!_finalmultiplier){
        output("%s",M_ERROR_PREFIX);outputValue("Failed to convert '",_value,"' to a big integer!\n");
        return NULL;
    }
    if(amVerboseDebugging())
        outputBiginteger("\nFinal multiplier: '",_finalmultiplier,"'.");
    Mbiginteger* _result=owned_biginteger(_getBiginteger(6),owner);
    if(_result){
        clock_t then=(amVerbose()?clock():0);
        // we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
        // we could start at some intermediate value????
        Mbiginteger *_multiplier=owned_biginteger(_getBiginteger(3),owner);
        if(_multiplier){
            while(mp_cmp(MP_INT_POINTER(_multiplier),MP_INT_POINTER(_finalmultiplier))==MP_LT){
                if(mp_incr(MP_INT_POINTER(_multiplier))!=MP_OKAY)
                {outputError("Failed to increment a big integer");_result=NULL;break;} // if we fail to increment break
                if(mp_mul(MP_INT_POINTER(_result),MP_INT_POINTER(_multiplier),MP_INT_POINTER(_result))!=MP_OKAY)
                {outputError("Failed to multiply a big integer");_result=NULL;break;}
                //////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
            }
            // get rid of intermediate big integers
            FREE_BIGINTEGER(_multiplier,owner);
        }else        
            outputError("Failed to create big integer 3");
        if(amVerboseDebugging())
            {outputBiginteger("The computation of the factorial of ",_finalmultiplier," took ");output("%lld ms.\n",(clock()-then)/M_CLOCKS_PER_MS);}
    }else
        outputError("Failed to create big integer 6");
    FREE_BIGINTEGER(_finalmultiplier,owner);
    if(amVerboseDebugging())
        outputBiginteger("Result of applying the factorial() function: '",_result,"'.\n");
    return (_result?_getValueOfBiginteger(disowned_biginteger(_result,owner)):NULL);
    /* replacing:
    // 39 is about the maximum that we can store in a long long
    if(n<40){
        long long result=n;while(--n>1)result*=n; // TODO should we use multiply here NO I guess not, although we could get overflow at some point!!!
        return _getIntegerValue(result);
    }
    long double result=n;
    while(--n>1)result*=n;
    return _getFloatValue(result);
    */
}/* VALIDATED */

// method for writing a value to standard out
Mvalue* Mout(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    Mstring* _valueText=owned_string(_getValueText(_value,true),owner);
    size_t result=string_length(_valueText);
    if(result>0)output("%s",string(_valueText));
    FREE_STRING(_valueText,owner);
    return _getIntegerValue(result);
}

// or by defining an rgb value
Mvalue* Mbrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3){
    long long ll1=getValueInteger(_value1),ll2=getValueInteger(_value2),ll3=getValueInteger(_value3);
    if(ll1<0||ll2<0||ll3<0)return NULL;
    if(ll1==M_LL_INVALID||ll2==M_LL_INVALID||ll3==M_LL_INVALID)return NULL;
    char s[24];sprintf(s,"'\\033[48;2;%lld;%lld;%lldm",ll1%256,ll2%256,ll3%256);
    ////////output("ANSI foreground color code: '%s'.\n",s);
    return _getTextValue(s);
}
Mvalue* Mtrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3){
    long long ll1=getValueInteger(_value1),ll2=getValueInteger(_value2),ll3=getValueInteger(_value3);
    if(ll1<0||ll2<0||ll3<0)return NULL;
    if(ll1==M_LL_INVALID||ll2==M_LL_INVALID||ll3==M_LL_INVALID)return NULL;
    char s[24];sprintf(s,"'\\033[38;2;%lld;%lld;%lldm",ll1%256,ll2%256,ll3%256);
    ////////output("ANSI foreground color code: '%s'.\n",s);
    return _getTextValue(s);
}

// random functions
static Mbiginteger *birandmax_1=NULL;static Mallocationowner owner_biginteger=(Mallocationowner){MI_FUNCTIONS,__LINE__,1};
// NOTE do NOT start with underscore (_) to indicate that the result is to be left alone!!
const Mbiginteger* getBigintegerRandMaxPlusOne(){
    if(!birandmax_1){
        birandmax_1=owned_biginteger(_getBiginteger(RAND_MAX),owner_biginteger);
        if(birandmax_1)if(mp_incr(MP_INT_POINTER(birandmax_1))!=MP_OKAY)
        {FREE_BIGINTEGER(birandmax_1,owner_biginteger);birandmax_1=NULL;}
    }
    return birandmax_1;
}/* VALIDATED */
Mvalue* Mrand(){Mallocationowner owner=getOwner(__LINE__); // to return a random value between 0 and 1
    Mbiginteger* _randmaxplusone=getBigintegerRandMaxPlusOne();
    if(_randmaxplusone){
        Mbiginteger* _num=owned_biginteger(_getBiginteger(rand()),owner);
        if(_num){
            Mrational* _rational=owned_rational(_getRational(_num,_randmaxplusone,M_LD_NAN,false),owner);
            if(_rational)return _getValueOfRational(disowned_rational(_rational,owner));
            FREE_BIGINTEGER(_num,owner); // not bound to the returned rational
        }else
            outputError("Failed to create the numerator of the rational random number");
    }else
        outputError("Failed to create the numerator of the rational random number");
    return NULL;
}
static long long randominteger(long long upper){Mallocationowner owner=getOwner(__LINE__);
    // ASSERT upper should be in (0,RAND_MAX]
    long long r=M_LL_INVALID;
    Mbiginteger* _randmaxplusone=getBigintegerRandMaxPlusOne(); // will remain owned so the rational will not free it
    if(_randmaxplusone){
        // the same as what we did in Mrand() but now multiplying the numerator with upper
        Mbiginteger *_mult=owned_biginteger(_getBiginteger(upper),owner),*_rand=owned_biginteger(_getBiginteger(rand()),owner);
        if(_mult&&_rand){
            Mbiginteger* _num=owned_biginteger(__biginteger(),owner);
            if(_num){
                Mrational* _rational=NULL;
                if(mp_mul(MP_INT_POINTER(_mult),MP_INT_POINTER(_rand),MP_INT_POINTER(_num))==MP_OKAY){
                    _rational=owned_rational(_getRational(_num,_randmaxplusone,M_LD_NAN,false),owner);
                    if(_rational){
                        Mbiginteger* _biginteger=owned_biginteger(_rational2biginteger(_rational),owner);
                        if(_biginteger){
                            r=biginteger2long(_biginteger);
                            FREE_BIGINTEGER(_biginteger,owner);
                        }else
                            outputError("Failed to determine the integer part of the rational random number");
                    }
                }
                if(_rational)FREE_RATIONAL(_rational,owner);else FREE_BIGINTEGER(_num,owner);
            }else
                outputError("Failed to create the random rational numerator");
        }
        if(_mult)FREE_BIGINTEGER(_mult,owner);if(_rand)FREE_BIGINTEGER(_rand,owner);
    }
    return r;
}
Mvalue* Mirand(Mvalue* _upperValue){Mallocationowner owner=getOwner(__LINE__);
    if(_upperValue){
        if(_upperValue->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_upperValue->value._array,Mirand));
        if(_upperValue->type==VT_LIST)return _getValueOfList(appliedToList(_upperValue->value._list,Mirand));
        long long upper=getValueInteger(_upperValue);
        if(upper>0&&upper<=RAND_MAX){
            long long r=randominteger(upper);
            if(r!=M_LL_INVALID)return _getIntegerValue(r);
        }
    }
    return NULL;
}
// if you want a list of random values call Mrands()
// switched to returning an array instead of a list
Mvalue* Mrands(Mvalue* _countValue){Mallocationowner owner=getOwner(__LINE__);
    long long count=getValueInteger(_countValue);
    if(count>0){
        Marray* _randarray=owned_array(_getArray("Mrands",count),owner);
        if(_randarray){
            Mvalue** valueholder=_randarray->values;
            while(--count>=0){assignValue(valueholder,Mrand());valueholder++;}
            return _getValueOfArray(disowned_array(_randarray,owner));
        }
        output("%sFailed to create an array to hold %lld random rational numbers in [0,1).\n",M_ERROR_PREFIX,count);
        /* replacing:
        Mlist* _randList=owned_list(__list("Mrands"),owner);
        while(--count>=0&&appendedToList(_randList,owner,Mrand(),M_LL_INVALID)>0);
        return _getValueOfList(disowned_list(_randList,owner));
        */
    }
    return NULL;
}
Mvalue* Mirands(Mvalue* _countValue,Mvalue* _upperValue){Mallocationowner owner=getOwner(__LINE__);
    long long count=getValueInteger(_countValue);
    if(count>0){
        long long upper=getValueInteger(_upperValue);
        if(upper>0&&upper<=RAND_MAX){
            Marray* _randarray=owned_array(_getArray("Mrands",count),owner);
            if(_randarray){
                Mvalue** valueholder=_randarray->values;
                while(--count>=0){
                    long long r=randominteger(upper);
                    assignValue(valueholder,(r>=0?_getIntegerValue(r):NULL));
                    valueholder++;
                }
                return _getValueOfArray(disowned_array(_randarray,owner));
            }
            output("%sFailed to create an array to hold %lld random integer numbers in [0,%lld).\n",M_ERROR_PREFIX,count,upper);
        }else
        if(upper<=0)
            output("%s%llu should be positive",M_ERROR_PREFIX,upper);
        else
            output("%s%lld should not exceed %lu.",M_ERROR_PREFIX,upper,RAND_MAX);
    }
    return NULL;
}
Mvalue* Msrand(Mvalue* _seedValue){
    long long result=M_LL_INVALID;
    long long seed=(_seedValue?getValueInteger(_seedValue):time(NULL));
    long long seedmax=UINT_MAX;
    if(seed>0&&seed<=seedmax){
        srand(seed);
        result=M_TRUE;
    }else
    if(seed>0)
        result=M_FALSE;
    return _getIntegerValue(result);
}
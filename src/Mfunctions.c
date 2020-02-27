#include <stdint.h>

#include "Mfunctions.h"

extern char const * const VALUETYPENAMES[];
extern char const * const ERROR_PREFIX;
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
Mvalue* Mfloor(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(floorl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,false),true);
        if(_value->type==VT_DECIMAL)return _getDecimalValue(_getDecimalInteger(_value->value._decimal,true,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mfloor),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mfloor),true);
    }
    return NULL;
}
Mvalue* Mtrunc(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(truncl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,true),true);
        if(_value->type==VT_DECIMAL)return _getDecimalValue(_getDecimalInteger(_value->value._decimal,true,true),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtrunc),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtrunc),true);
    }
    return NULL;
}
/*
 * returns nearest integer value
 */
Mvalue* Mround(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(roundl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRoundedRationalInteger(_value->value._rational),true);
        if(_value->type==VT_DECIMAL)return _getDecimalValue(_getRoundedDecimal(_value->value._decimal),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil),true);
    }
    return NULL;
}
/*
 * returns integer equal to or larger than
 */
Mvalue* Mceil(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(ceill(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,false,false),true);
        if(_value->type==VT_DECIMAL)return _getDecimalValue(_getDecimalInteger(_value->value._decimal,false,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil),true);
    }
    return NULL;
}

Mvalue* Msin(Mvalue* _value){
    if(_value){
        if(amVerbose()){outputValue("Applying sin() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msin),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msin),true);
        if(_value->type==VT_RATIONAL){
            Mrational* _sineRational=_qsinorcos(_value->value._rational,true);
            return _getRationalValue(_sineRational,true);
        }
        if(_value->type==VT_DECIMAL){
            Mdecimal* _sineDecimal=_dsine(NULL,_value->value._decimal); // match the precision as used by the argument
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
            if(_sineDecimal)return _getDecimalValue(_sineDecimal,true);
        }
    }
    return NULL;
}/* NOT VALIDATED */
Mvalue* Mcordicsin(Mvalue* _value){
    if(_value){
        if(amVerbose()){outputValue("Applying cordicsin() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        /* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordicsin),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordicsin),true);
        if(_value->type==VT_DECIMAL){
            Mdecimal* _cordicsineDecimal=_dcordicsine(NULL,_value->value._decimal); // match the precision as used by the argument
            if(_cordicsineDecimal)return _getDecimalValue(_cordicsineDecimal,true);
        }
    }
    return NULL;
}/* NOT VALIDATED */
Mvalue* Mcordiccos(Mvalue* _value){
    if(_value){
        if(amVerbose()){outputValue("Applying cordiccos() to '",_value,"' of type ");output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);}
        /* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
        if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
        */
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordicsin),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordicsin),true);
        if(_value->type==VT_DECIMAL){
            Mdecimal* _cordiccosineDecimal=_dcordiccosine(NULL,_value->value._decimal); // match the precision as used by the argument
            if(_cordiccosineDecimal)return _getDecimalValue(_cordiccosineDecimal,true);
        }
    }
    return NULL;
}/* NOT VALIDATED */

Mvalue* Mcos(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(cosl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(cos(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcos),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcos),true);
        if(_value->type==VT_RATIONAL){
            Mrational* _cosineRational=_qsinorcos(_value->value._rational,false);
            return _getRationalValue(_cosineRational,true);
        }
        if(_value->type==VT_DECIMAL){
            Mdecimal* _cosineDecimal=_dcosine(NULL,_value->value._decimal); // match the precision as used by the argument
            if(_cosineDecimal)return _getDecimalValue(_cosineDecimal,true);
        }
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtan(Mvalue*  _value){
    if(_value){
        // composite application
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtan),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtan),true);
        // scalar arguments
        if(_value->type==VT_FLOAT)return _getFloatValue(tanl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(tan(_value->value._integer->ll));
        if(_value->type==VT_DECIMAL)return _getDecimalValue(_dtangent(NULL,_value->value._decimal),true);
        if(_value->type==VT_RATIONAL){
            Mrational* _sineRational=_qsinorcos(_value->value._rational,true);
            Mrational* _cosineRational=_qsinorcos(_value->value._rational,false);
            Mrational* _tanRational=_getRationalQuotient(_sineRational,_cosineRational);
            free_rational(_sineRational);
            free_rational(_cosineRational);
            return _getRationalValue(_tanRational,true);
        }
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcosh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(coshl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(cosh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcosh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcosh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Msinh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(sinhl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sinh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msinh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msinh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtanh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(tanhl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(tanh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtanh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtanh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mexp(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp),true);
        // use decimal conversion
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(M_DECIMALCONTEXT->mpd_context,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qexp(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    free_decimal(_result);_result=NULL;
                    outputError("Failed to apply the exp function to a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)free_decimal(_decimal);
            return _getDecimalValue(_result,true);
        }
    }
    return NULL;
}
// internal approximation by series expansion
Mvalue* Mdexp(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp),true);
        // use decimal conversion
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=_dexp(NULL,_decimal);
            if(_value->type!=VT_DECIMAL)free_decimal(_decimal);
            return _getDecimalValue(_result,true);
        }
    }
    return NULL;
}
Mvalue* Mlog(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(logl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(log(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog),true);
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(M_DECIMALCONTEXT->mpd_context,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qln(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    free_decimal(_result);_result=NULL;
                    outputError("Failed to compute the natural logarithm of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)free_decimal(_decimal);
            return _getDecimalValue(_result,true);
        }
    }
    return NULL;
}
Mvalue* Mlog10(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(log10l(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(log10(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog10),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog10),true);
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(M_DECIMALCONTEXT->mpd_context,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qlog10(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    free_decimal(_result);_result=NULL;
                    outputError("Failed to compute the base 10 logarithm of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)free_decimal(_decimal);
            return _getDecimalValue(_result,true);
        }
    }
    return NULL;
}

Mvalue* Msqrt(Mvalue* _value){
    if(_value){
        if(_value->type==VT_FLOAT)return _getFloatValue(sqrtl(_value->value._float->ld));
        if(_value->type==VT_INTEGER)return _getFloatValue(sqrt(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msqrt),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msqrt),true);
        // the square root of big integer, decimal and rational values has to be computed by conversion to decimals first
        // TODO although for rationals we could divide the square root of the numerator by the square root of the denominator
        // we've got a function in Mdecimal.h/c to explicitly convert a value (if possible) to a decimal (if the value wraps a decimal that is returned (instead of a new copy of this wrapped decimal) and that decimal should NOT be freed (see below))
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(M_DECIMALCONTEXT->mpd_context,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qsqrt(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
                if(status&0xEFBF){
                    free_decimal(_result);_result=NULL;
                    outputError("Failed to compute the square root of a decimal");
                    outputDecimalStatus(status);
                }
            }
            if(_value->type!=VT_DECIMAL)free_decimal(_decimal);
            return _getDecimalValue(_result,true);
        }
    }
    return NULL;
}
// two-argument power function (complicates things considerably)
// TODO check different types convert to long double and use powl to compute the power!!
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue){
    if(_value&&_exponentValue){
        if(_value->type==VT_FLOAT&&_exponentValue->type==VT_FLOAT)return _getFloatValue(powl(_value->value._float->ld,_exponentValue->value._float->ld));
        if(_value->type==VT_INTEGER&&_exponentValue->type==VT_INTEGER)return _getFloatValue(pow(_value->value._integer->ll,_exponentValue->value._integer->ll));
    }
    return NULL;
}
// end math functions

Mvalue* Mneg(Mvalue* _value){ // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_FLOAT)return _getFloatValue(-_value->value._float->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg),true);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getNegatedBiginteger(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL){
            // this is done by negating the numerator but if the numerator equals NULL we should use -1
            Mrational* rational=_value->value._rational;
            if(rational){
                /////////outputValue("Negating rational '",_value,"'.\n");
                Mbiginteger* _biNumerator=(rational->num?_getNegatedBiginteger(rational->num):_getBiginteger(-1));
                if(_biNumerator){
                    ////////outputBiginteger("Negated numerator '",_biNumerator,"' computed!\n");
                    Mbiginteger* _biDenominator=(rational->den?_getBigintegerCopy(rational->den):NULL);
                    ////////outputBiginteger("Denominator '",_biDenominator,"' copied!\n");
                    if(_biDenominator||!rational->den)return _getRationalValue(_getRational(_biNumerator,_biDenominator,(isFloatUndefined(rational->delta)==M_TRUE?M_LD_NAN:-rational->delta->ld),false,true),true);
                    outputError("Failed to copy the numerator of the rational to negate");
                    if(_biDenominator)free_biginteger(_biDenominator);
                    free_biginteger(_biNumerator);
                }else
                    outputError("Failed to negate the numerator of a rational");
            }
        }else
        if(_value->type==VT_DECIMAL){
            Mdecimal* decimal=_value->value._decimal;
            if(decimal){
                Mdecimal* _negDecimal=__decimal(M_DECIMALCONTEXT->mpd_context,0,0);
                if(_negDecimal){
                    uint32_t status=0;
                    mpd_qcopy_negate(_negDecimal->mpd,decimal->mpd,&status);
                    if(status&0xEFBF){
                        free_decimal(_negDecimal);_negDecimal=NULL;
                        outputError("Failed to negate a decimal");
                        outputDecimalStatus(status);
                    }else // success, ascertain to copy the repeating field over as that remains the same on negating (assumedly)
                        _negDecimal->repeating=decimal->repeating;
                    if(_negDecimal)return _getDecimalValue(_negDecimal,true);
                }
            }
        }
    }
    return NULL;
}/* VALIDATED */

Mvalue* Mnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot),true);
    }
    return NULL;
}/* VALIDATED */
// TODO can we not a string??????
Mvalue* Mbnot(Mvalue* _value){ // not a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot),true);
    }
    return NULL;
}/* VALIDATED */

Mvalue* Mnull(Mvalue* _value){return _getIntegerValue(isValueNull(_value)?M_TRUE:M_FALSE);}/* VALIDATED */
Mvalue* Mundefined(Mvalue* _value){return _getIntegerValue(isValueUndefined(_value)?M_TRUE:M_FALSE);}/* VALIDATED */ // MDH@18JUL2019: isUndefined() now comes in handy
Mvalue* Msign(Mvalue* value){return(_getIntegerValue(getValueSign(value)));} /* VALIDATED */
// TODO use the sign in Mzero, Mpositive and Mnegative
Mvalue* Mzero(Mvalue* _value){return(_getIntegerValue(_value?(isValueZero(_value)==M_TRUE?M_TRUE:M_FALSE):M_LL_INVALID));}/* VALIDATED */
Mvalue* Mpositive(Mvalue* _value){return(_value?_getIntegerValue(isValuePositive(_value)?1:0):NULL);}/* VALIDATED */
Mvalue* Mnegative(Mvalue* _value){return(_value?_getIntegerValue(isValueNegative(_value)?1:0):NULL);}/* VALIDATED */
Mvalue* Mscalar(Mvalue* _value){return(_value?_getIntegerValue(isValueScalar(_value)?1:0):NULL);}/* VALIDATED */

// TODO the length of a text is the number of characters in a text????
// MDH@17OCT2019: the length of a list should now return the index of the last element (instead of the number of non-null values)
//                because doing so means appending a value with l[len(l)+1] will do so, instead of overwriting some value!!!!
Mvalue* Mlen(Mvalue* _value){
    long long result=M_LL_INVALID;
    if(_value){
        switch(_value->type){
            case VT_LIST:result=(_value->value._list->_last?_value->value._list->_last->index:0);break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            default:break;
        }
    }
    return _getIntegerValue(result);
}/* VALIDATED */
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
Mvalue* Mfac(Mvalue* _value){
    if(!_value){if(amVerbose())outputInfo("No argument to factorial() function!");return NULL;}
    if(amVerbose())outputValue("Argument of factorial() function: '",_value,"'.\n");
    if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER){outputValue("\nERROR: Non-integer argument '",_value,"' to factorial() function!");return NULL;}
    // some special cases (i.e. the input number is smaller than 2)
    Mbiginteger* _finalmultiplier=NULL;
    if(_value->type==VT_INTEGER){
        if(_value->value._integer->ll<0){outputError("Invalid (negative) integer argument to factorial() function");return NULL;}
        if(_value->value._integer->ll<3)return _getIntegerValue(_value->value._integer->ll);
        _finalmultiplier=_getBiginteger(_value->value._integer->ll);
    }else{
        if(mp_isneg(_value->value._biginteger)){outputError("Invalid (negative) big integer argument to factorial() function");return NULL;}
        if(mp_cmp(_value->value._biginteger,getBigintegerThree())==MP_LT)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        _finalmultiplier=_getBigintegerCopy(_value->value._biginteger);
    }
    if(!_finalmultiplier){output("%s",ERROR_PREFIX);outputValue("Failed to convert '",_value,"' to a big integer!\n");return NULL;}
    if(amVerbose()&&amDebugging())outputBiginteger("\nFinal multiplier: '",_finalmultiplier,"'.");
    Mbiginteger* _result=_getBiginteger(6); // the smallest value to return
    if(_result){
        // we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
        // we could start at some intermediate value????
        Mbiginteger *_multiplier=_getBiginteger(3);
        if(_multiplier){
            while(mp_cmp(_multiplier,_finalmultiplier)==MP_LT){
                if(mp_incr(_multiplier)!=MP_OKAY){outputError("Failed to increment a big integer");_result=NULL;break;} // if we fail to increment break
                if(mp_mul(_result,_multiplier,_result)!=MP_OKAY){outputError("Failed to multiply a big integer");_result=NULL;break;}
                //////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
            }
            // get rid of intermediate big integers
            free_biginteger(_multiplier);
        }else        
            outputError("Failed to create big integer 3");
    }else
        outputError("Failed to create big integer 6");
    free_biginteger(_finalmultiplier);
    if(amVerbose())outputBiginteger("Result of applying the factorial() function: '",_result,"'.\n");
    return (_result?_getBigintegerValue(_result,true):NULL);
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
Mvalue* Mout(Mvalue* _value){
    Mstring* _valueText=_getValueText(_value,true);
    size_t result=string_length(_valueText);
    if(result>0)output("%s",string(_valueText));
    free_string(_valueText);
    return _getIntegerValue(result);
}

// MDH@17OCT2019: instead of setting the back color we can return the text to be used in out to set the back color
// set the backcolor
Mvalue* Mbc(Mvalue* _value){
    long long ll=(_value?getValueInteger(_value):-1); // all negative colors default to the back color
    if(ll==M_LL_INVALID)return NULL;
    char s[16];if(ll>=0)sprintf(s,"'\\033[48;5;%lldm",ll%256);else sprintf(s,"'\\033[48;5;%sm",getBackgroundColor());
    ////////////output("ANSI background color code: '%s'.\n",s);
    return _getTextValue(_strdup(s),true);
    /* replacing:
    Mstring* _valueText=_getValueText(_value,true);
    size_t result=string_length(_valueText);
    if(result>0)setBackColor(string(_valueText));
    free_string(_valueText);
    return _getIntegerValue(result);
    */
}
 // set text color
Mvalue* Mtc(Mvalue* _value){
    long long ll=(_value?getValueInteger(_value):-1);
    if(ll==M_LL_INVALID)return NULL;
    char s[16];if(ll>=0)sprintf(s,"'\\033[38;5;%lldm",ll%256);else sprintf(s,"'\\033[38;5;%sm",getInfoColor());
    ////////output("ANSI foreground color code: '%s'.\n",s);
    return _getTextValue(_strdup(s),true);
    /*
    Mstring* _valueText=_getValueText(_value,true);
    size_t result=string_length(_valueText);
    if(result>0)setColor(string(_valueText));
    free_string(_valueText);
    return _getIntegerValue(result);
    */
}
// or by defining an rgb value
Mvalue* Mbrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3){
    long long ll1=getValueInteger(_value1),ll2=getValueInteger(_value2),ll3=getValueInteger(_value3);
    if(ll1<0||ll2<0||ll3<0)return NULL;
    if(ll1==M_LL_INVALID||ll2==M_LL_INVALID||ll3==M_LL_INVALID)return NULL;
    char s[24];sprintf(s,"'\\033[48;2;%lld;%lld;%lldm",ll1%256,ll2%256,ll3%256);
    ////////output("ANSI foreground color code: '%s'.\n",s);
    return _getTextValue(_strdup(s),true);
}
Mvalue* Mtrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3){
    long long ll1=getValueInteger(_value1),ll2=getValueInteger(_value2),ll3=getValueInteger(_value3);
    if(ll1<0||ll2<0||ll3<0)return NULL;
    if(ll1==M_LL_INVALID||ll2==M_LL_INVALID||ll3==M_LL_INVALID)return NULL;
    char s[24];sprintf(s,"'\\033[38;2;%lld;%lld;%lldm",ll1%256,ll2%256,ll3%256);
    ////////output("ANSI foreground color code: '%s'.\n",s);
    return _getTextValue(_strdup(s),true);
}

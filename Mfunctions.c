#include "Msettings.h"
#include "Moutput.h"

#include "Mfunctions.h"

extern const char* VALUETYPENAMES;
extern const char* const ERROR_PREFIX;
extern const long double M_LD_NAN;
extern const long double M_LD_PI;

extern mpd_context_t* _decimalContext; // M.c takes care of creating the application-wide decimal context
void outputDecimalStatus(uint32_t status){
	if(status>0){
		outputLine("Decimal computations error report.");
		if(status&MPD_IEEE_Invalid_operation)outputLine("\tIEEE Invalid operation error.");
		if(status&MPD_Clamped)outputLine("\tClamped error.");
		if(status&MPD_Division_by_zero)outputLine("\tDivision by zero error.");
		if(status&MPD_Fpu_error)outputLine("\tFPU error.");
		if(status&MPD_Inexact)outputLine("\tInexact error.");
		if(status&MPD_Not_implemented)outputLine("\tNot implemented error.");
		if(status&MPD_Overflow)outputLine("\tOverflow error.");
		if(status&MPD_Rounded)outputLine("\tRounding error.");
		if(status&MPD_Subnormal)outputLine("\tSubnormal error.");
		if(status&MPD_Underflow)outputLine("\tUnderflow error.");
	}else
		outputLine("No decimal context errors.");
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
        if(_value->type==VT_REAL)return _getRealValue(floorl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mfloor),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mfloor),true);
    }
    return NULL;
}
Mvalue* Mtrunc(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(truncl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,true,true),true);
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
        if(_value->type==VT_REAL)return _getRealValue(roundl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRoundedRationalInteger(_value->value._rational),true);
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
        if(_value->type==VT_REAL)return _getRealValue(ceill(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL)return _getBigintegerValue(_getRationalInteger(_value->value._rational,false,false),true);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil),true);
    }
    return NULL;
}
Mvalue* Msin(Mvalue* _value){
    if(_value){
        if(amVerbose()){outputValue("Applying sin() to '",_value,"' of type ");output("%s(%u).\n",VALUETYPENAMES[_value->type],_value->type);}
        if(_value->type==VT_REAL)return _getRealValue(sinl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sin(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msin),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msin),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcos(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(cosl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(cos(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcos),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcos),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtan(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(tanl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(tan(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtan),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtan),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mcosh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(coshl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(cosh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcosh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcosh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Msinh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(sinhl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sinh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msinh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msinh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mtanh(Mvalue*  _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(tanhl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(tanh(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtanh),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtanh),true);
    }
    return NULL;
}/* VALIDATED */
Mvalue* Mexp(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(expl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(exp(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp),true);
        // use decimal conversion
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(_decimalContext,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qexp(_result->mpd,_decimal->mpd,_decimalContext,&status);
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
Mvalue* Mlog(Mvalue* _value){
    if(_value){
        if(_value->type==VT_REAL)return _getRealValue(logl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(log(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog),true);
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(_decimalContext,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qln(_result->mpd,_decimal->mpd,_decimalContext,&status);
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
        if(_value->type==VT_REAL)return _getRealValue(log10l(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(log10(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog10),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog10),true);
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(_decimalContext,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qlog10(_result->mpd,_decimal->mpd,_decimalContext,&status);
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
        if(_value->type==VT_REAL)return _getRealValue(sqrtl(_value->value._real->ld));
        if(_value->type==VT_INTEGER)return _getRealValue(sqrt(_value->value._integer->ll));
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msqrt),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msqrt),true);
        // the square root of big integer, decimal and rational values has to be computed by conversion to decimals first
        // TODO although for rationals we could divide the square root of the numerator by the square root of the denominator
        // we've got a function in Mdecimal.h/c to explicitly convert a value (if possible) to a decimal (if the value wraps a decimal that is returned (instead of a new copy of this wrapped decimal) and that decimal should NOT be freed (see below))
        Mdecimal* _decimal=getValueDecimal(_value);
        if(_decimal){
            Mdecimal* _result=__decimal(_decimalContext,0,0);
            if(_result){
                uint32_t status=0;
                mpd_qsqrt(_result->mpd,_decimal->mpd,_decimalContext,&status);
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
        if(_value->type==VT_REAL&&_exponentValue->type==VT_REAL)return _getRealValue(powl(_value->value._real->ld,_exponentValue->value._real->ld));
        if(_value->type==VT_INTEGER&&_exponentValue->type==VT_INTEGER)return _getRealValue(pow(_value->value._integer->ll,_exponentValue->value._integer->ll));
    }
    return NULL;
}
// end math functions

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    mp_int* _bineg=__biginteger();
    if(_bineg&&mp_neg(_biginteger,_bineg)!=MP_OKAY){free_biginteger(_bineg);_bineg=NULL;outputError("Failed to negate a big integer");}
    return _bineg;
}

Mvalue* Mneg(Mvalue* _value){ // negate a value
    if(_value){
        if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
        if(_value->type==VT_REAL)return _getRealValue(-_value->value._real->ld);
        if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg),true);
        if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg),true);
        if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getNegatedBiginteger(_value->value._biginteger),true);
        if(_value->type==VT_RATIONAL){
            // this is done by negating the numerator but if the numerator equals NULL we should use -1
            Mrational* rational=_value->value._rational;
            if(rational){
                outputValue("Negating rational '",_value,"'.\n");
                Mbiginteger* _biNumerator=(rational->num?_getNegatedBiginteger(rational->num):_getBiginteger(-1));
                if(_biNumerator){
                    Mbiginteger* _biDenominator=(rational->den?_getBigintegerCopy(rational->den):NULL);
                    if(_biDenominator||!rational->den)return _getRationalValue(_getRational(_biNumerator,_biDenominator,(!rational->delta||ldIsNaN(rational->delta->ld)?M_LD_NAN:-rational->delta->ld),false,true),true);
                    if(_biDenominator)free_biginteger(_biDenominator);
                    free_biginteger(_biNumerator);
                    outputError("Failed to copy the numerator of the rational to negate");
                }else
                    outputError("Failed to negate the numerator of a rational");
            }
        }else
        if(_value->type==VT_DECIMAL){
            Mdecimal* decimal=_value->value._decimal;
            if(decimal){
                Mdecimal* _negDecimal=__decimal(_decimalContext,0,0);
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
Mvalue* Mnull(Mvalue* _value){
    return _getIntegerValue(isNull(_value)?1:0);
}/* VALIDATED */
Mvalue* Mundefined(Mvalue* _value){
    return _getIntegerValue(isUndefined(_value)?1:0); // MDH@18JUL2019: isUndefined() now comes in handy
}/* VALIDATED */

// TODO the length of a text is the number of characters in a text????
Mvalue* Mlen(Mvalue* _value){
    long long result=0;
    if(_value){
        switch(_value->type){
            case VT_INTEGER:case VT_BIGINTEGER:case VT_REAL:case VT_TEXT:result=1;break;
            case VT_LIST:result=_value->value._list->numberOfElements;break;
            case VT_MAP:result=_value->value._map->numberOfElements;break;
            default:break;
        }
    }
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
    if(!_value){if(amVerbose())outputLine("No argument to factorial() function!");return NULL;}
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
    return _getRealValue(result);
    */
}/* VALIDATED */

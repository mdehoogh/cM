#include <stdint.h>
#include <time.h>

#include "Mfunctions.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_FUNCTIONS,id};}

extern char const * const VALUETYPENAMES[];
extern char const * const M_ERROR_PREFIX;
extern char const * const M_INFO_PREFIX;
extern char const * const M_MESSAGE_PREFIX;
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_ZERO,M_NEGATIVE,M_POSITIVE;
extern const long double M_LD_NAN,M_LD_PI;
extern const Mdecimalcontext* M_DECIMALCONTEXT; // M.c takes care of creating the application-wide decimal context

// applying unary operators by means of functions
// math functions: independent of the execution environment but still receive it...
// TODO how about applying the functions to decimals and rationals
/*
returns the largest integer equal to or smaller than \p _value
\parameter _value the value to floor
*/
// MDH@28MAY2020: we can either return a new value but preferably we should return the same value NOTE we can do that because values are immutable but can be reused
/**
 * @brief returns the floor of \p _value
 * 
 * @param _value 
 * @return Mvalue* the floor of \p _value
 */
Mvalue* Mfloor(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
		/* replacing:
		if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
		if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
		*/
		// composite types
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mfloor,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mfloor,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mfloor,VT_UNDEFINED));
		// scalar values
		if(_value->type==VT_FLOAT)return _getFloatValue(floorl(_value->value._float->ld));
		if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,true,false));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,true,false));
	}
	return NULL;
}
/**
 * @brief returns the trunc of \p _value
 * 
 * @param _value 
 * @return Mvalue* the trunc of \p _value
 */
Mvalue* Mtrunc(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
		/* replacing:
		if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
		if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
		*/
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mtrunc,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtrunc,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtrunc,VT_UNDEFINED));
		if(_value->type==VT_FLOAT)return _getFloatValue(truncl(_value->value._float->ld));
		if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,true,true));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,true,true));
	}
	return NULL;
}
/**
 * @brief returns the ceil of \p _value
 * 
 * @param _value 
 * @return the ceil of \p _value
 */
Mvalue* Mceil(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
		/* replacing:
		if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
		if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
		*/
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mceil,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mceil,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mceil,VT_UNDEFINED));
		if(_value->type==VT_FLOAT)return _getFloatValue(ceill(_value->value._float->ld));
		if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRationalInteger(_value->value._rational,false,false));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_getDecimalInteger(_value->value._decimal,false,false));
	}
	return NULL;
}

/**
 * @brief returns the big integer nearest to M decimal \p _decimal
 * 
 * @param _decimal 
 * @return Mbiginteger* the big integer nearest to M decimal \p _decimal
 */
static Mbiginteger* _getRoundedDecimalInteger(Mdecimal* _decimal){Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _roundedDecimalInteger=NULL;
	if(_decimal!=NULL){
		Mdecimalcontext* decimalcontext=getDecimalcontext(_decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext!=NULL?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
		if(mpd_context!=NULL){
			Mdecimal* _roundDecimal=(Mdecimal*)OWNED(__decimal(mpd_context,0,0),owner);
			if(_roundDecimal!=NULL){
				uint32_t status=0;
				mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
				if((status&0xEFBF)==0){
					long long dll=decimal2long(_roundDecimal);
					_roundedDecimalInteger=owned_biginteger(_getBiginteger(dll),owner);
				}else{
					q2outputmessageprefix(M_ERROR_PREFIX);
					q2outputDecimal("Failed to round decimal '",_decimal,"'"); // are we going to force wrap the decimal?? outputDecimal
					q2output(" (status: %.8x).\n",status);
				}
				FREE_DECIMAL(_roundDecimal,owner);
			}
		}else
			q2outputError("No context available for rounding a decimal");
	}
	return disowned_biginteger(_roundedDecimalInteger,owner); 
}
/**
 * @brief returns the round of \p _value
 * 
 * @param _value 
 * @return Mvalue* the round of \p _value
 */
Mvalue* Mround(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_INTEGER||_value->type==VT_BIGINTEGER)return _value;
		/* replacing:
		if(_value->type==VT_INTEGER)return _getIntegerValue(_value->value._integer->ll);
		if(_value->type==VT_BIGINTEGER)return _getBigintegerValue(_getBigintegerCopy(_value->value._biginteger),true);
		*/
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mround,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mround,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mround,VT_UNDEFINED));
		if(_value->type==VT_FLOAT)return _getFloatValue(roundl(_value->value._float->ld));
		if(_value->type==VT_RATIONAL)return _getValueOfBiginteger(_getRoundedRationalInteger(_value->value._rational));
		if(_value->type==VT_DECIMAL)return _getValueOfBiginteger(_getRoundedDecimalInteger(_value->value._decimal));
	}
	return NULL;
}
/**
 * @brief returns the sine of \p _value
 * 
 * @param _value 
 * @return Mvalue* the sine of \p _value
 */
Mvalue* Msin(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(amVerbose()){
			q2outputValue("Applying sin() to '",_value,"' of type ");
			q2output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);
		}
		if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Msin,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msin,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msin,VT_UNDEFINED));
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
				if(amVerbose())q2outputInfo("Ready to compute the sine of a decimal.");
				// to use the product formula I found on internet at matrixlab-examples.com I need to compute (x/pi)^2, I suppose I need to subtract 2*pi until the result is between -pi and pi
				// so if we divide the input by pi we get a value that should be between -1 and 1, so we have to divide it by pi and use the remainder
				Mdecimal* _decimal=_value->value._decimal;
				// determine if _decimal is negative, if it is we negate it
				int neg=mpd_isnegative(_decimal->mpd);
				Mdecimal* _negatedDecimal=(neg?__adecimal():_decimal);if(!_negatedDecimal)status=1;else if(neg)mpd_qcopy_negate(_negatedDecimal->mpd,_decimal->mpd,&status);
				if((status&0xEFBF)==0){
					if(amVerbose())q2outputInfo("Determining the normalized decimal to use as argument of the sine approximation.");
					// NOTE the remainder is the starting value of the sine approximation
					mpd_qdivmod(_qDecimal->mpd,_sineDecimal->mpd,_negatedDecimal->mpd,_piDecimal->mpd,_decimalContext,&status);
					if((status&0xEFBF)==0){
						if(!isDecimalZero(_sineDecimal)){ // non-zero input
							if(amVerbose())q2outputDecimal("Number of integer multiples of pi: '",_qDecimal,"'.\n");
							// if the result of the integer division by pi is odd we have to negate the result as well
							int odd=mpd_isodd(_qDecimal->mpd);
							if(amVerbose())q2outputDecimal("Ready to approximate the sine of non-zero decimal '",_sineDecimal,"'.\n");
							// by dividing the initial value by pi we get z/pi which we need to square to get the c part of (1-c)/1 which is the first product multiplier
							mpd_qdiv(_dividedByPiDecimal->mpd,_sineDecimal->mpd,_piDecimal->mpd,_decimalContext,&status);
							if((status&0xEFBF)==0){
								if(amVerbose())q2outputDecimal("Divided by pi: '",_dividedByPiDecimal,"'.\n");
								// we know the sign of the sine will be positive for angles in (0,PI), so we can safely make the result negative if dealing with a negative input value
								// the initial value of the sine decimal is the product of _negatedDecimal and the square of _dividedByPiRemainderDecimal
								mpd_qmul(_squareDividedByPiDecimal->mpd,_dividedByPiDecimal->mpd,_dividedByPiDecimal->mpd,_decimalContext,&status);
								if((status&0xEFBF)==0){
									/////////if(amVerbose())q2outputInfo("Product multiplier numerator subcomputed.");
									mpd_qsub(_numeratorDecimal->mpd,_denominatorDecimal->mpd,_squareDividedByPiDecimal->mpd,_decimalContext,&status);
									if((status&0xEFBF)==0){
										if(amVerbose())q2outputDecimal("Multiplier: '",_numeratorDecimal,"' -> ");
										// next we multiply the initial value of the product by the numerator alone (because the denominator is still equal to 1)			   
										mpd_qmul(_sineDecimal->mpd,_sineDecimal->mpd,_numeratorDecimal->mpd,_decimalContext,&status);
										///////////////if(amVerbose())q2outputDecimal("Second approximation to the sine: '",_sineDecimal,"'.\n");
										int64_t count=M_LL_MAX;
										while((status&0xEFBF)==0){
											if(--count==0){q2outputInfo("Maximum number of iterations exceeded!");break;}
											if(amVerbose())q2outputDecimal("Sine approximation: '",_sineDecimal,"'.\n");
											mpd_qadd(_2nplus1Decimal->mpd,_2nplus1Decimal->mpd,_twoDecimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break; // add 2 to 2n+1 to get 2(n+1)+1 so becoming 3, 5, 7, ....
											// add _2nplus1Decimal to the numerator and denominator
											mpd_qadd(_numeratorDecimal->mpd,_numeratorDecimal->mpd,_2nplus1Decimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
											if(amVerbose())q2outputDecimal("Numerator: '",_numeratorDecimal,"'. ");
											mpd_qadd(_denominatorDecimal->mpd,_denominatorDecimal->mpd,_2nplus1Decimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
											if(amVerbose())q2outputDecimal("Denominatator: '",_denominatorDecimal,"'. ");
											// if the numerator equals the denominator we're actually done because that means that the multiplier will from now on equal 1
											if(mpd_qcmp(_numeratorDecimal->mpd,_denominatorDecimal->mpd,&status)==0)break;if((status&0xEFBF)!=0)break;
											mpd_qdiv(_multiplierDecimal->mpd,_numeratorDecimal->mpd,_denominatorDecimal->mpd,_decimalContext,&status);if((status&0xEFBF)!=0)break;
											if(amVerbose())q2outputDecimal("Multiplier: '",_multiplierDecimal,"' -> ");
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
				q2outputError("Failed to create at least one of the help decimal in computing the sine of a decimal");
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
/**
 * @brief returns the cordic sine of \p value
 * 
 * @param _value 
 * @return Mvalue* the cordic sine of \p value
 */
Mvalue* Mcordicsin(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(amVerbose()){
			q2outputValue("Applying cordicsin() to '",_value,"' of type ");
			q2output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);
		}
		/* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
		if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
		*/
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mcordicsin,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordicsin,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordicsin,VT_UNDEFINED));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcordicsine(NULL,_value->value._decimal));
	}
	return NULL;
}/* NOT VALIDATED */
/**
 * @brief returns the cordic cosine of \p _value
 * 
 * @param _value 
 * @return Mvalue* 
 */
Mvalue* Mcordiccos(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value){
		if(amVerbose()){
			q2outputValue("Applying cordiccos() to '",_value,"' of type ");
			q2output("%s(%u).\n",""/*VALUETYPENAMES[_value->type]*/,_value->type);
		}
		/* TODO we can call _dcordicsine although a real or integer does not have a decimal context, but then the default decimal context is used
		if(_value->type==VT_FLOAT)return _getFloatValue(sinl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(sin(_value->value._integer->ll));
		*/
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mcordiccos,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcordiccos,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcordiccos,VT_UNDEFINED));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcordiccosine(NULL,_value->value._decimal));
	}
	return NULL;
}/* NOT VALIDATED */

/**
 * @brief returns the cosine of \p _value
 * 
 * @param _value 
 * @return Mvalue* the cosine of \p _value
 */
Mvalue* Mcos(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(cosl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(cos(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mcos,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcos,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcos,VT_UNDEFINED));
		if(_value->type==VT_RATIONAL)return _getValueOfRational(_qsinorcos(_value->value._rational,false));
		if(_value->type==VT_DECIMAL)return _getValueOfDecimal(_dcosine(NULL,_value->value._decimal));
	}
	return NULL;
}/* VALIDATED */
/**
 * @brief returns the tangens of \p _value
 * 
 * @param _value 
 * @return Mvalue* the tangens of \p _value
 */
Mvalue* Mtan(Mvalue*  _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		// composite application
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mtan,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtan,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtan,VT_UNDEFINED));
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
/**
 * @brief returns the hyperbolic cosine of \p _value 
 * 
 * @param _value 
 * @return Mvalue* the hyperbolic cosine of \p _value 
 */
Mvalue* Mcosh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(coshl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(cosh(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mcosh,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mcosh,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mcosh,VT_UNDEFINED));
	}
	return NULL;
}/* VALIDATED */
/**
 * @brief returns the hyperbolic sine of \p _value
 * 
 * @param _value 
 * @return Mvalue* the hyperbolic sine of \p _value
 */
Mvalue* Msinh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(sinhl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(sinh(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Msinh,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msinh,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msinh,VT_UNDEFINED));
	}
	return NULL;
}/* VALIDATED */
Mvalue* Mtanh(Mvalue*  _value){//Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(tanhl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(tanh(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mtanh,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mtanh,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mtanh,VT_UNDEFINED));
	}
	return NULL;
}/* VALIDATED */
/**
 * @brief returns the exp of \p _value
 * 
 * @param _value 
 * @return Mvalue* the exp of \p _value
 */
Mvalue* Mexp(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mexp,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp,VT_UNDEFINED));
		// use decimal conversion
		Mdecimal* _decimal=getValueDecimal(_value,NULL);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
		if(_decimal){
			Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
			if(_result){
				uint32_t status=0;
				mpd_qexp(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
				if(status&0xEFBF){
					FREE_DECIMAL(_result,owner);_result=NULL;
					q2outputError("Failed to apply the exp function to a decimal");
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
/**
 * @brief returns the decimal exp of \p _value
 * 
 * @param _value 
 * @return Mvalue* the decimal exp of \p _value
 */
Mvalue* Mdexp(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(expl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(exp(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mexp,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mexp,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mexp,VT_UNDEFINED));
		// use decimal conversion
		Mdecimal* _decimal=getValueDecimal(_value,NULL);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
		if(_decimal!=NULL){
			Mdecimal* _result=owned_decimal(_dexp(NULL,_decimal),owner);
			if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
			return _getValueOfDecimal(disowned_decimal(_result,owner));
		}
	}
	return NULL;
}
/**
 * @brief returns the natural logarithm of \p _value
 * 
 * @param _value 
 * @return Mvalue* returns the natural logarithm of \p _value
 */
Mvalue* Mlog(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(logl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(log(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mlog,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog,VT_UNDEFINED));
		Mdecimal* _decimal=getValueDecimal(_value,NULL);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
		if(_decimal!=NULL){
			Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
			if(_result!=NULL){
				uint32_t status=0;
				mpd_qln(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
				if(status&0xEFBF){
					FREE_DECIMAL(_result,owner);_result=NULL;
					q2outputError("Failed to compute the natural logarithm of a decimal");
					outputDecimalStatus(status);
				}
			}
			if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
			return _getValueOfDecimal(disowned_decimal(_result,owner));
		}
	}
	return NULL;
}
/**
 * @brief returns the log10 of \p _value
 * 
 * @param _value 
 * @return Mvalue* the log10 of \p _value
 */
Mvalue* Mlog10(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(log10l(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(log10(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mlog10,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mlog10,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mlog10,VT_UNDEFINED));
		Mdecimal* _decimal=getValueDecimal(_value,NULL);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
		if(_decimal!=NULL){
			Mdecimal* _result=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
			if(_result!=NULL){
				uint32_t status=0;
				mpd_qlog10(_result->mpd,_decimal->mpd,M_DECIMALCONTEXT->mpd_context,&status);
				if(status&0xEFBF){
					FREE_DECIMAL(_result,owner);_result=NULL;
					q2outputError("Failed to compute the base 10 logarithm of a decimal");
					outputDecimalStatus(status);
				}
			}
			if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
			return _getValueOfDecimal(disowned_decimal(_result,owner));
		}
	}
	return NULL;
}
/**
 * @brief returns the sqrt of \p _value
 * 
 * @param _value 
 * @return Mvalue* the sqrt of \p _value
 */
Mvalue* Msqrt(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->type==VT_FLOAT)return _getFloatValue(sqrtl(_value->value._float->ld));
		if(_value->type==VT_INTEGER)return _getFloatValue(sqrt(_value->value._integer->ll));
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Msqrt,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Msqrt,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Msqrt,VT_UNDEFINED));
		// the square root of big integer, decimal and rational values has to be computed by conversion to decimals first
		// TODO although for rationals we could divide the square root of the numerator by the square root of the denominator
		// we've got a function in Mdecimal.h/c to explicitly convert a value (if possible) to a decimal (if the value wraps a decimal that is returned (instead of a new copy of this wrapped decimal) and that decimal should NOT be freed (see below))
		Mdecimal* _decimal=getValueDecimal(_value,NULL);if(_value->type!=VT_DECIMAL)owned_decimal(_decimal,owner);
		Mdecimal* _result=owned_decimal(_getDecimalSqrt(_decimal),owner); // MDH@20MAR2023: delegate to _getDecimalSqrt added to Mdecimal.c, and take ownership!!
		if(_value->type!=VT_DECIMAL)FREE_DECIMAL(_decimal,owner);
		return _getValueOfDecimal(disowned_decimal(_result,owner));
	}
	return NULL;
}
// two-argument power function (complicates things considerably)
// TODO check different types convert to long double and use powl to compute the power!!
// theoretically we could always return a rational???
/**
 * @brief returns \p ll1 to the power of \p ll2
 * 
 * @param ll1 
 * @param ll2 
 * @return Mvalue* \p ll1 to the power of \p ll2
 */
static Mvalue* powll(long long ll1,long long ll2){
	// essentially we may either return an integer, or a big integer, or possibly a rational depending
	if(ll1==M_LL_INVALID||ll2==M_LL_INVALID)return _getIntegerValue(M_LL_INVALID);
	if(ll1==0)return _getIntegerValue(0LL);
	if(ll2==0)return _getIntegerValue(1LL);
	// ASSERT both non-zero
	// negative exponents should return 1/powll as a rational
	if(ll2<0){
		Mvalue* inverse=powll(ll1,-ll2); // the inverse
		return(inverse?_getValueOfRational(_getRational(_getBiginteger(1LL),_getValueBiginteger(inverse),0,false)):NULL);
	}
	// ASSERT ll2 now always positive
	Mbiginteger *_bi1=_getBiginteger(ll1),*_multiplier=_getBiginteger(ll1);
	Mbiginteger *_power=_getBiginteger(1LL);
	//////q2outputBiginteger("(",_multiplier,NULL);////q2outputBiginteger(",",_bi2,")");
	//////long long powerof2=1;
	// we can compute the power ourselves, by simply using the bits from ll2, and adding what we need to the result so far which we put in _power
	// which means we have to keep track of the power of 2 to multiply ll1 with (to add to the sum so far)
	while(ll2>0){
		//////output(" %lld",ll2);
		/////q2outputBiginteger("+",_bi2,NULL);
		if(ll2&1LL){
			// TODO do error handling!!!!!
			/* multiply ll1 with the power of two in _bi2
			if(mp_mul_2d(_bi1->_bi,powerof2,_multiplier->_bi)!=MP_OKAY){free_biginteger(_power);_power=NULL;break;}
			q2outputBiginteger(">",_multiplier,NULL);
			*/
			// add addendum to _power
			if(mp_mul(_power->_bi,_multiplier->_bi,_power->_bi)!=MP_OKAY){free_biginteger(_power);_power=NULL;break;}
			/////////q2outputBiginteger("=",_power,NULL);
		}
		if(mp_sqr(_multiplier->_bi,_multiplier->_bi)!=MP_OKAY){free_biginteger(_power);_power=NULL;break;}
		ll2>>=1; // half ll2
	}
	free_biginteger(_bi1);free_biginteger(_multiplier);
	// can we return a long long or should we return the big integer instead????
	return(_power!=NULL?_getValueOfBiginteger(_power):NULL);
}
/** TODO
 * @brief returns \p _value to the power of \p _exponentValue
 * 
 * @param _value 
 * @param _exponentValue 
 * @return Mvalue* \p _value to the power of \p _exponentValue
 */
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL&&_exponentValue!=NULL){
		if(_value->type==VT_INTEGER&&_exponentValue->type==VT_INTEGER)return powll(_value->value._integer->ll,_exponentValue->value._integer->ll);
		if(_value->type==VT_FLOAT&&_exponentValue->type==VT_FLOAT)return _getFloatValue(powl(_value->value._float->ld,_exponentValue->value._float->ld));
	}
	return NULL;
}
// end math functions

/**
 * @brief returns the logical not of \p _value
 * 
 * @param _value 
 * @return Mvalue* the logical not of \p _value
 */
Mvalue* Mnot(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__); // not a value
	if(_value!=NULL){
		if(_value->type==VT_INTEGER)return _getIntegerValue(!_value->value._integer->ll);
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mnot,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mnot,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mnot,VT_UNDEFINED));
	}
	return NULL;
}/* VALIDATED */
// TODO can we not a string??????
/**
 * @brief returns the binary not of \p _value
 * 
 * @param _value 
 * @return Mvalue* the binary not of \p _value
 */
Mvalue* Mbnot(Mvalue* _value){//Mallocationowner owner=getOwner(__LINE__); // not a value
	if(_value!=NULL){
		if(_value->type==VT_INTEGER)return _getIntegerValue(~_value->value._integer->ll);
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mbnot,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mbnot,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mbnot,VT_UNDEFINED));
	}
	return NULL;
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p _value equals null, M_FALSE otherwise
 * 
 * @param _value 
 * @return Mvalue* 
 */
Mvalue* Mnull(Mvalue* _value){
	return _getIntegerValue(isValueNull(_value)?M_TRUE:M_FALSE);
}/* VALIDATED */
/**
 * @brief returns M_TRUE if \p _value is undefined, M_FALSE otherwise
 * 
 * @param _value 
 * @return Mvalue* 
 */
Mvalue* Mundefined(Mvalue* _value){
	return _getIntegerValue(isValueUndefined(_value)?M_TRUE:M_FALSE);
}/* VALIDATED */ // MDH@18JUL2019: isUndefined() now comes in handy
/**
 * @brief returns the sign of \p _value
 * 
 * @param value 
 * @return Mvalue* the sign of \p _value
 */
Mvalue* Msign(Mvalue* value){
	return(_getIntegerValue(getValueSign(value)));
} /* VALIDATED */
// TODO use the sign in Mzero, Mpositive and Mnegative
/**
 * @brief returns M_TRUE if \p value equals 0, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE if \p value equals 0, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mzero(Mvalue* _value){
	return(_getIntegerValue(_value?(isValueZero(_value)==M_TRUE?M_TRUE:M_FALSE):M_LL_INVALID));
}/* VALIDATED */
/**
 * @brief returns M_TRUE if \p _value is positive, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE if \p _value is positive, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mpositive(Mvalue* _value){
	return _getIntegerValue(isValuePositive(_value)); ///// replacing: _value!=NULL?(isValuePositive(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
/**
 * @brief returns M_TRUE if \p _value is negative, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE if \p _value is negative, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mnegative(Mvalue* _value){
	return _getIntegerValue(isValueNegative(_value)); // replacing: _value!=NULL?(isValueNegative(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */
/**
 * @brief returns M_TRUE if \p _value is a scalar, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE if \p _value is a scalar, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mscalar(Mvalue* _value){
	return _getIntegerValue(isValueScalar(_value)); /// replacing: _value!=NULL?(isValueScalar(_value)?M_TRUE:M_FALSE):M_LL_INVALID);
}/* VALIDATED */

// MDH@29OCT2020: might come in handy
/**
 * @brief returns M_TRUE if \p _value is numeric, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return long long M_TRUE if \p _value is numeric, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Misnumeric(Mvalue* _value){
	// MDH@04DEC2020: delegate to local helper function isnumeric 
	return _getIntegerValue(isNumeric(_value));
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p _value wraps a list, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE if \p _value wraps a list, M_FALSE or M_LL_INVALID otherwise
Mvalue* Misalist(Mvalue* _value){
	return _getIntegerValue(_value?(_value->type==VT_LIST?M_TRUE:M_FALSE):M_LL_INVALID);
}*//* VALIDATED */

// TODO the length of a text is the number of characters in a text????
// MDH@17OCT2019: the length of a list should now return the index of the last element (instead of the number of non-null values)
//				because doing so means appending a value with l[len(l)+1] will do so, instead of overwriting some value!!!!
// MDH@20NOV2020: for lists, also return the number of elements
/**
 * @brief returns the length of \p _value
 * @details the length of an array or list or map is its number of elements
 *          the length of a text is the number of characters it contains
 * @param _value 
 * @return Mvalue* the length of \p _vakue
 */
Mvalue* Mlen(Mvalue* _value){
	long long result=M_LL_INVALID;
	if(_value!=NULL){
		switch(_value->type){
			case VT_ARRAY:
			{
				result=(_value->value._array->elements!=NULL?_value->value._array->elements->count:0);
				/* MDH@08APR2023: an array can now be defined with multiple dimensions
				if(_value->value._array->numberOfDimensionsLeft){
					Marray* _dimensions=_getArray("len",numberOfDimensionsLeft+1,NULL);
					if(_dimensions==NULL)return NULL;
					_dimensions->valuetype=VT_INTEGER;
					while(--result>=0)_dimensions->values[result]=_getIntegerValue(_value->value._array->_dimensions[result]);
					return _getArrayValue(_dimensions);
				}
				*/
				break;
			}
			case VT_LIST:result=_value->value._list->numberOfElements;break; //(_value->value._list->_last?_value->value._list->_last->index:0);break;
			case VT_MAP:result=_value->value._map->numberOfElements;break;
			case VT_TEXT:result=strlen(_value->value._text->_c);break;
			case VT_BYTES:result=string_length(_value->value._string);break;
			default:break;
		}
	}
	return _getIntegerValue(result);
}/* VALIDATED */
// MDH@30NOV2020: convenient if we can change the length of an array of string
/**
 * @brief returns M_TRUE if the length of \p _value was successfully set to \p newlength_value, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @param newlength_value 
 * @return Mvalue* M_TRUE if the length of \p _value was successfully set to \p newlength_value, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Msetlen(Mvalue* _value,Mvalue* newlength_value){Mallocationowner owner=getOwner(__LINE__);
	// a length should always be a nonnegative integer
	long long result=M_LL_INVALID;
	if(_value!=NULL){
		long long newlength=getValueInteger(newlength_value);
		if(newlength>=0){
			switch(_value->type){
				case VT_ARRAY:
					{
						Marray* array=_value->value._array;
						Marrayelements* arrayElements=(array!=NULL?array->elements:NULL);
						// TODO if this is a multidimensional array what should we do?????? should newlength_value have the same number of dimension lengths??????
						if(arrayElements!=NULL&&arrayElements->count>0){ // this is not a dimensioned array
							unsigned long long length=arrayElements->count;
							if(length!=newlength){
								unsigned long long l=MAX(length,newlength); // guaranteed to be positive, will equal length if newlength equals 0!!!
								// we can't cut off values until we managed to get new memory
								result=newlength;
								if(newlength>0){
									Mvalue** newvalues=CALLOC(sizeof(Mvalue*),newlength,-'a',owner);
									if(newvalues!=NULL){
										do{
											l--;
											// all values ABOVE newlength will not be used anymore
											// all values below length will still be used
											if(l>=newlength)
												assignValue(&arrayElements->values[l],NULL);
											else 
											if(l<length)
												newvalues[l]=arrayElements->values[l];
										}while(l>0);
										// NULL any value reference in the current array's values NOT included in the new values
										FREE_DISOWNED(arrayElements->values,length,-'a',Msubowner(getValueOwner(),1)); // get rid of the current values
										arrayElements->values=newvalues; // make values point to the new values
										arrayElements->count=newlength; // remember the new length
										result-=length; // the change in number of elements is the result of the function
									}
								}else{ // deleting all values, so no need to try to allocate sufficient memory
									result-=length;
									while(l>0)assignValue(&arrayElements->values[--l],NULL); // getting rid of all value pointers (in effect decrementing the counts of all the values!!!!)
									FREE(arrayElements->values,length,-'a'); // get rid of the current values							
									arrayElements->count=0;
									arrayElements->values=NULL;
								}
							}else // no need to change
								result=0;
						}
					};
					break;
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
				case VT_BYTES:
					if(_value->value._string!=NULL){
						if(NULL==string_setlength(_value->value._string,newlength))q2outputError("Failed to change the length of a bytes sequence");
						result=string_length(_value->value._string);
					}
					break;
				case VT_TEXT:
					{
						unsigned long long length=strlen(_value->value._text->_c);
						if(length!=newlength){
							// we're going to copy the text out of it, change it and reset it
							Mstring* _text=owned_string(__string(),owner);
							if(_text!=NULL){
								Mstring* p=_text;
								result=newlength;
								result-=length;
								p=string_append_char(p,_value->value._text->presuffix);
								p=string_append(_text,_value->value._text->_c);
								if(length<newlength){
									while(length<newlength){p=string_append_char(p,' ');if(!p)break;length++;}
								}else
									p=string_setlength(p,newlength);
								if(p!=NULL){ // text successfully lengthened or shortened
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
// MDH@06JAN2020: split string(s) by separator(s)
/**
 * @brief returns the C strings splitting \p textsValue and the number of texts returned in \p *textcount
 * 
 * @param textsValue 
 * @param textcount 
 * @return char** the C strings splitting \p textsValue and the number of texts returned in \p *textcount
 */
static char** _getTexts(Mvalue* textsValue,unsigned long long * textcount){Mallocationowner owner=getOwner(__LINE__);
	char** _texts=NULL;
	if(textcount!=NULL){
		*textcount=0;
		if(textsValue!=NULL){
			q2outputValue("Extracting text(s) from '",textsValue,"'.\n"); // DEBUGGING
			if(textsValue->type==VT_ARRAY){
				Marray* textArray=textsValue->value._array;
				if(textArray!=NULL){
					// TODO how to return the texts of a dimensioned array????
					Marrayelements* textArrayElements=textArray->elements;
					if(textArrayElements->count>0){
						*textcount=textArrayElements->count;
						if(*textcount>0){
							Mvalue** textValues=textArrayElements->values;
							if(textValues!=NULL){
								_texts=MALLOC(sizeof(char*),*textcount,'c',owner);
								if(_texts!=NULL){
									unsigned long long textindex=*textcount;
									do{
										textindex--;
										Mstring* _valueText=owned_string(_getValueText(textValues[textindex],true,true),owner);
										if(_valueText!=NULL){
											_texts[textindex]=OWNED(_strdup(string(_valueText)),Msubowner(owner,1));
											FREE_STRING(_valueText,owner);
										}else
											_texts[textindex]=NULL;
									}while(textindex>0);
								}else{
									*textcount=0;
									q2outputError("Failed to allocate memory for storing texts");
								}
							}else
								q2outputBug("Missing array values");
						}
					}
				}else 
					q2outputBug("Missing value array");
			}else
			if(textsValue->type==VT_LIST){
				Mlist* textList=textsValue->value._list;
				if(textList!=NULL){
					// we may decide to maintain sparseness????? in which case we should use the index of the last element!!!!
					// although technically we could split the texts and take over the index values later on (let's do that)
					*textcount=textList->numberOfElements;
					if(*textcount>0){
						_texts=CALLOC(sizeof(char*),*textcount,'c',owner);
						if(_texts!=NULL){
							unsigned long long textindex=0;
							Mlistelement* listelement=textList->_first;
							while(listelement!=NULL){
								if(textindex>=*textcount){q2outputBug("Number of elements of list incorrect trying to split texts");break;}
								Mstring* _valueText=owned_string(_getValueText(listelement->_value,true,true),owner);
								if(_valueText!=NULL){
									_texts[textindex++]=OWNED(_strdup(string(_valueText)),Msubowner(owner,1));
									FREE_STRING(_valueText,owner);
								}else
									_texts[textindex++]=NULL;
								listelement=listelement->_next;
							}
						}else{
							*textcount=0;
							q2outputError("Failed to allocate memory for storing texts");
						}
					}
				}else 
					q2outputBug("Missing value list");
			}else
			if(textsValue->type==VT_TEXT){ // only a single text!
				if(textsValue->value._text!=NULL){
					_texts=MALLOC_1(sizeof(char*),'c',owner);
					if(_texts!=NULL){
						*textcount=1;
						output("Duplicating '%s'.\n",textsValue->value._text->_c);
						_texts[0]=OWNED(_strdup(textsValue->value._text->_c),Msubowner(owner,1));
					}else 
						q2outputError("Failed to allocate memory for storing the text to split");
				}else 
					q2outputBug("Missing value text");
			}else 
				q2outputError("Cannot split a value that is not a text (or list and array with texts)");
			if(_texts!=NULL)return DISOWNED(_texts,owner);
			q2outputError("Failed to extract text(s)");
		}
	}
	return NULL;
}
/**
 * @brief frees \p texts containing \p textcount C strings
 * 
 * @param texts 
 * @param textcount 
 * @param owner 
 */
static void freetexts(char** const texts,unsigned long long textcount,Mallocationowner owner){
	if(NULL==texts)return;
	for(register unsigned long long textindex=0;textindex<textcount;textindex++)
	if(texts[textindex]!=NULL){
		output("Freeing text '%s'.\n",texts[textindex]); // DEBUGGING
		FREE_DISOWNED(texts[textindex],strlen(texts[textindex])+1,-'"',owner); // the reverse of the allocation by _strdup()
	}
	FREE_DISOWNED(texts,textcount,'c',owner);
}
/** TODO
 * @brief splits \p textcount C strings in \p texts using \p separatorcount C string separators in \p separators
 * @details if there are itemwrappers defined in \p itemwrappers
 * @param texts 
 * @param textcount 
 * @param separators 
 * @param separatorcount 
 * @param itemwrappers 
 * @param itemwrappercount 
 * @return Mlist* 
 */
static Mlist* splits(char** const texts,unsigned long long textcount,char** const separators,unsigned long long separatorcount,char** const itemwrappers,unsigned long long itemwrappercount){Mallocationowner owner=getOwner(__LINE__);
	if(texts!=NULL&&textcount>0&&separators!=NULL&&separatorcount>0){
		// every element in the split text list will be a list (of texts)
		Mlist* _splitTextsList=owned_list(_getListOfType(VT_LIST),owner);
		if(_splitTextsList!=NULL){
			unsigned long long textindex=textcount;
			long long separatorindex;
			// if there are item wrappers splitting will be slow
			if(itemwrappers!=NULL&&itemwrappercount>0){
				// TODO split around itemwrappers
			}else{
				char* *_text=texts;
				do{
					if(*_text!=NULL){ // something to split
						Mlist* _splitTextList=owned_list(_getListOfType(VT_TEXT),owner);
						if(_splitTextList!=NULL){
							Mstring* _splitText=owned_string(_getString("'"),owner); // local!!!
							if(_splitText!=NULL){
								// we can reuse _splitText by setting it's length to 1 every next time
								// MDH@07JAN2021: instead of iterating over the text myself I could simply try to detect the next occurrence of any of the separators?
								char* textStart=*_text; // initialize the pointer to the start of the text to search for the next separator
								char *sepstr,*firstsepstr;
								unsigned long long firstseplength;
								do{
									firstsepstr=NULL;
									char* *separator=separators; // point to the first separator
									separatorindex=separatorcount;
									while(separatorindex-->0){ // apologizing for doing it this way
										sepstr=strstr(textStart,*separator);
										if(sepstr)if(firstsepstr==NULL||sepstr<firstsepstr){firstsepstr=sepstr;firstseplength=strlen(*separator);} // we need to remember the length of the best separator
										separator++;
									}
									// if one of the separators was found, firstsepstr will point to the first character of that separator, and by zero'ing the character there the split text will be correctly appended
									if(firstsepstr)*firstsepstr='\0'; // replace the first character of the separator by the end of text, so textStart will contain the split text
									if(!string_append(_splitText,textStart)){
										q2outputMessage(M_ERROR_PREFIX,"Failed to collect split text '%s'.",textStart);
										break;
									}
									// ready to append _splitText to the split text list
									Mtext* splitText=owned_text(_getText(string(_splitText)),owner);
									if(splitText!=NULL){
										Mvalue* splitTextValue=_getValueOfText(disowned_text(splitText,owner));
										if(splitTextValue!=NULL){
											if(appendedToList(_splitTextList,owner,splitTextValue,M_LL_INVALID)<=0)
												q2outputMessage(M_ERROR_PREFIX,"Failed to add split text '%s'.",string(_splitText));
										}else
											q2outputMessage(M_ERROR_PREFIX,"Failed to wrap split text '%s'.",string(_splitText));
									}else 
										q2outputMessage(M_ERROR_PREFIX,"Failed to wrap split text '%s'.",string(_splitText));
									if(!firstsepstr)break;
									string_setlength(_splitText,1); // re-use _splitText!!!
									textStart=firstsepstr+firstseplength; // start looking for the next separator starting at strlen(*separator) further
								}while(1);
								/* replacing:
								char c;
								while((c=**_text)){
									if(!string_append_char(_splitText,c)){
										q2outputMessage(M_ERROR_PREFIX,"Failed to collect split character '%c'.",c);
										break;
									}
									// if _splitText ends with one of the separators, we cut it off and break
									separatorindex=separatorcount;
									while(--separatorindex>=0&&!string_endswith(_splitText,separators[separatorindex]));
									if(separatorindex>=0){ // matching a separator
										if(!string_shorten(_splitText,strlen(separators[separatorindex])))output("%sFailed to cut off the separator of '%s'.\n",string(_splitText));
										break;
									}
									_text++;
								}while(1);
								Mtext* splitText=owned_text(_getText(string(_splitText)),owner);
								if(splitText){
									Mvalue* splitTextValue=_getValueOfText(disowned_text(splitText,owner));
									if(splitTextValue){
										if(appendedToList(_splitTextList,owner,splitTextValue,M_LL_INVALID)<=0)
											q2outputMessage(M_ERROR_PREFIX,"Failed to add split text '%s'.",string(_splitText));
									}else
										q2outputMessage(M_ERROR_PREFIX,"Failed to wrap split text '%s'.",string(_splitText));
								}else 
									q2outputMessage(M_ERROR_PREFIX,"Failed to wrap split text '%s'.",string(_splitText));
								*/
								FREE_STRING(_splitText,owner); // freed!!!!
							}else 
								q2outputMessage(M_ERROR_PREFIX,"Failed to collect characters from text to split '%s'.",*_text);
							Mvalue* _splitTextListValue=_getValueOfList(disowned_list(_splitTextList,owner));
							// NOTE if appending fails the gc should take care of freeing this value, and the contained list!!!!
							if(appendedToList(_splitTextsList,owner,_splitTextListValue,M_LL_INVALID)<=0)
								q2outputMessage(M_ERROR_PREFIX,"Failed to append the list of split texts of '%s'.",*_text);
						}else
							q2outputMessage(M_ERROR_PREFIX,"Unable to create the list to store the split parts of '%s'.",*_text);
					}
					_text++;
				}while(--textindex>0);
			}
			return disowned_list(_splitTextsList,owner);
		}
		q2outputError("Failed to create the split text list");
	}else 
		q2outputError("Input to the split function undefined or incomplete");
	return NULL;
}
/**
 * @brief splits \p _textValue using separators in \p _separatorValue and item wrappers in \p _itemwrapperValue
 * 
 * @param _textValue 
 * @param _separatorValue 
 * @param _itemwrapperValue 
 * @return Mvalue* \p _textValue split using separators in \p _separatorValue and item wrappers in \p _itemwrapperValue
 */
Mvalue* Msplit(Mvalue* _textValue,Mvalue* _separatorValue,Mvalue* _itemwrapperValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_FUNCTIONS));
	if(_textValue!=NULL&&_separatorValue!=NULL){
		Mlist* _splitTextsList=NULL;
		unsigned long long textcount;char** _texts=OWNED(_getTexts(_textValue,&textcount),owner);
		if(_texts!=NULL){
			// if(report)
			{
				output("Splitting texts:\n");
				for(unsigned long long textindex=0;textindex<textcount;textindex++)output("\t'%s'\n",_texts[textindex]);
			}
			// let's compose the list of separators, but multiple separators is an issue
			unsigned long long separatorcount;char** _separators=OWNED(_getTexts(_separatorValue,&separatorcount),owner);
			if(_separators){
				// if(report)
				{
					output("Using separators:\n");
					for(unsigned long long separatorindex=0;separatorindex<separatorcount;separatorindex++)output("\t'%s'\n",_separators[separatorindex]);
				}
				unsigned long long itemwrappercount;char** _itemwrappers=OWNED(_getTexts(_itemwrapperValue,&itemwrappercount),owner);
				_splitTextsList=owned_list(splits(_texts,textcount,_separators,separatorcount,_itemwrappers,itemwrappercount),owner);
				if(_itemwrappers)freetexts(_itemwrappers,itemwrappercount,owner);
				freetexts(_separators,separatorcount,owner);
			}
			freetexts(_texts,textcount,owner);
		}
		if(_splitTextsList!=NULL){
			// if a single type return the first value in the list
			if(_textValue->type==VT_TEXT){
				Mvalue* splitValue=(_splitTextsList->_first!=NULL?_splitTextsList->_first->_value:NULL);
				FREE_LIST(_splitTextsList,owner); // NOTE although the reference count of splitValue might become 0, it will remove not be removed from the global value list until gc'ed
				return splitValue;
			}
			if(_textValue->type==VT_LIST){ // we'll have to update the index values of the returned list with the index values of the original list (if 'sparse')
				if(_textValue->value._list->_last!=NULL){
					if(_textValue->value._list->numberOfElements<_textValue->value._list->_last->index){
						Mlistelement *textValueListelement=_textValue->value._list->_first,*splitTextValueListelement=_splitTextsList->_first;
						while(textValueListelement!=NULL&&splitTextValueListelement!=NULL){
							splitTextValueListelement->index=textValueListelement->index;
							textValueListelement=textValueListelement->_next;
							splitTextValueListelement=splitTextValueListelement->_next;
						}
					}
				}
				return _getValueOfList(disowned_list(_splitTextsList,owner));
			}
			if(_textValue->type==VT_ARRAY){
				Marray* _splitTextsArray=owned_array(_getArray("Msplit",_splitTextsList->numberOfElements,NULL),owner);
				if(NULL==_splitTextsArray){q2outputError("Not enough memory to return the split texts in an array");return _getValueOfList(disowned_list(_splitTextsList,owner));}
				// move the values in the split text list over to splitTextArray
				unsigned long long splittextindex=0;
				Mlistelement* splitTextsListelement=_splitTextsList->_first;
				Mvalue** splitTextsArrayelement=_splitTextsArray->elements->values;
				while(splitTextsListelement!=NULL){
					if(++splittextindex>_splitTextsList->numberOfElements)break; // the number of elements in the split texts list is too small (and therefore incorrect!!!)
					assignValue(splitTextsArrayelement,splitTextsListelement->_value);
					splitTextsListelement=splitTextsListelement->_next;
					splitTextsArrayelement++;
				}
				if(splitTextsListelement!=NULL)q2outputBug("Number of split text list elements incorrect");
				FREE_LIST(_splitTextsList,owner); // no need for the list anymore after moving its values over to the split text array
				return _getValueOfArray(disowned_array(_splitTextsArray,owner));
			}
		}
	}
	return NULL;
}

// 25OCT2019: get the length of a text with M's tl function
/**
 * @brief returns the token length of \p _value
 * @details returns M_LL_INVALID if \p _value does not denote a token
 * @param _value 
 * @return Mvalue* the token length of \p _value
 */
Mvalue* Mtl(Mvalue* _value){
	long long result=M_LL_INVALID;
	if(_value){if(_value->type==VT_TEXT)result=strlen(_value->value._text->_c);else if(_value->type==VT_TOKEN)result=string_length(_value->value._token->text);}
	return _getIntegerValue(result);
}/* VALIDATED */

// MDH@29MAY2019: how about forcing the result to be a big integer instead of a long double?????
/**
 * @brief returns the number of digits in the factorial of \p _value 
 * @details uses the Stirling formula
 * @param _value 
 * @return Mvalue* the number of digits in the factorial of \p _value 
 */
Mvalue* Mfacd(Mvalue* _value){
	// Stirling formula to compute the number of factorial digits in n!: return 
	// get the integer out of the value
	long long ll=getValueInteger(_value);
	return (ll>0?_getIntegerValue(floor( ((ll+0.5)*log(ll) - ll + 0.5*log(2*M_LD_PI))/log(10) ) + 1):NULL);
}/* VALIDATED */

// TODO remember intermediate values in some list, that we can use as starting point
/**
 * @brief returns the factorial of \p _value
 * 
 * @param _value 
 * @return Mvalue* 
 */
Mvalue* Mfac(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value){
		if(amVerboseDebugging())
			q2outputInfo("No argument to factorial() function!");
		return NULL;
	}
	if(amVerboseDebugging())
		q2outputValue("Argument of factorial() function: '",_value,"'.\n");
	if(_value->type!=VT_INTEGER&&_value->type!=VT_BIGINTEGER){
		q2outputmessageprefix(M_ERROR_PREFIX);
		q2outputValue("Non-integer argument '",_value,"' to factorial() function!\n");
		return NULL;
	}
	// some special cases (i.e. the input number is smaller than 2)
	Mbiginteger* _finalmultiplier=NULL;
	if(_value->type==VT_INTEGER){
		if(_value->value._integer->ll<0)
		{q2outputError("Invalid (negative) integer argument to factorial() function");return NULL;}
		if(_value->value._integer->ll<3)
			return _getIntegerValue(_value->value._integer->ll);
		_finalmultiplier=owned_biginteger(_getBiginteger(_value->value._integer->ll),owner);
	}else{
		if(mp_isneg(MP_INT_POINTER(_value->value._biginteger)))
		{q2outputError("Invalid (negative) big integer argument to factorial() function");return NULL;}
		if(mp_cmp(MP_INT_POINTER(_value->value._biginteger),MP_INT_POINTER(getBigintegerThree()))==MP_LT)
			return _getValueOfBiginteger(_getBigintegerCopy(_value->value._biginteger));
		_finalmultiplier=owned_biginteger(_getBigintegerCopy(_value->value._biginteger),owner);
	}
	if(NULL==_finalmultiplier){
		q2outputmessageprefix(M_ERROR_PREFIX);
		q2outputValue("Failed to convert '",_value,"' to a big integer!\n");
		return NULL;
	}
	if(amVerboseDebugging())
		q2outputBiginteger("Final multiplier: '",_finalmultiplier,"'.\n");
	Mbiginteger* _result=owned_biginteger(_getBiginteger(6),owner);
	if(_result!=NULL){
		clock_t then=(amVerbose()?clock():0);
		// we could store fac values in a special list with index equal to the argument, in which case we could look up the starting value
		// we could start at some intermediate value????
		Mbiginteger *_multiplier=owned_biginteger(_getBiginteger(3),owner);
		if(_multiplier!=NULL){
			while(mp_cmp(MP_INT_POINTER(_multiplier),MP_INT_POINTER(_finalmultiplier))==MP_LT){
				if(mp_incr(MP_INT_POINTER(_multiplier))!=MP_OKAY)
				{q2outputError("Failed to increment a big integer");_result=NULL;break;} // if we fail to increment break
				if(mp_mul(MP_INT_POINTER(_result),MP_INT_POINTER(_multiplier),MP_INT_POINTER(_result))!=MP_OKAY)
				{q2outputError("Failed to multiply a big integer");_result=NULL;break;}
				//////////if(amVerbose())outputBigInteger("Result so far: '",result,"'.");
			}
			// get rid of intermediate big integers
			FREE_BIGINTEGER(_multiplier,owner);
		}else		
			q2outputError("Failed to create big integer 3");
		if(amVerboseDebugging())
			{q2outputBiginteger("The computation of the factorial of ",_finalmultiplier," took ");output("%lld ms.\n",(clock()-then)/M_CLOCKS_PER_MS);}
	}else
		q2outputError("Failed to create big integer 6");
	FREE_BIGINTEGER(_finalmultiplier,owner);
	if(amVerboseDebugging())
		q2outputBiginteger("Result of applying the factorial() function: '",_result,"'.\n");
	return (_result!=NULL?_getValueOfBiginteger(disowned_biginteger(_result,owner)):NULL);
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
/**
 * @brief outputs \p _value
 * 
 * @param _value 
 * @return Mvalue* the number of characters written
 */
Mvalue* Moutput(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _valueText=owned_string(_getValueText(_value,true,true),owner);
	long long result=string_length(_valueText);
	if(result>0)output("%s",string(_valueText));
	FREE_STRING(_valueText,owner);
	return _getIntegerValue(result);
}
/**
 * @brief outputs the elements of list or array stored in \p _value each on a separate line
 * 
 * @param _value the list/array wrapper
 * @return Mvalue* the number of characters written
 */
Mvalue* Moutputlines(Mvalue* _value){
	long long written=M_LL_INVALID;
	if(_value!=NULL){
		if(_value->type==VT_LIST){
			written=0;
			Mlist* list=_value->value._list;
			if(list!=NULL){
				Mlistelement* listelement=list->_first;
				while(listelement!=NULL){
					written+=Moutput(listelement->_value)->value._integer->ll;
					written+=newline();
					listelement=listelement->_next;
				}
			}
		}else
		if(_value->type==VT_ARRAY){
			written=0;
			Marray* array=_value->value._array;
			Marrayelements* arrayElements=(array!=NULL?array->elements:NULL);
			if(arrayElements!=NULL){
				size_t elementIndex=0;
				while(elementIndex++<array->elements->count){
					written+=Moutput(array->elements->values[elementIndex])->value._integer->ll;
					written+=newline();
				}
			}
		}else
			return Moutput(_value);

	}
	return _getIntegerValue(written);
}

// or by defining an rgb value
/**
 * @brief returns the background RGB defined by \p _value1 \p _value2 and \p _value3
 * 
 * @param _value1 
 * @param _value2 
 * @param _value3 
 * @return Mvalue* the background RGB defined by \p _value1 \p _value2 and \p _value3
 */
Mvalue* Mbrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3){
	long long ll1=getValueInteger(_value1),ll2=getValueInteger(_value2),ll3=getValueInteger(_value3);
	if(ll1<0||ll2<0||ll3<0)return NULL;
	if(ll1==M_LL_INVALID||ll2==M_LL_INVALID||ll3==M_LL_INVALID)return NULL;
	char s[24];sprintf(s,"'\\033[48;2;%lld;%lld;%lldm",ll1%256,ll2%256,ll3%256);
	////////output("ANSI foreground color code: '%s'.\n",s);
	return _getTextValue(s);
}
/**
 * @brief returns the foreground RGB defined by \p _value1 \p _value2 and \p _value3
 * 
 * @param _value1 
 * @param _value2 
 * @param _value3 
 * @return Mvalue* the foreground RGB defined by \p _value1 \p _value2 and \p _value3
 */
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
/**
 * @brief returns the global M big integer equal to RAND_MAX plus 1
 * 
 * @return const Mbiginteger* the M big integer equal to RAND_MAX plus 1
 */
const Mbiginteger* getBigintegerRandMaxPlusOne(){
	if(NULL==birandmax_1){
		birandmax_1=owned_biginteger(_getBiginteger(RAND_MAX),owner_biginteger);
		if(birandmax_1!=NULL)if(mp_incr(MP_INT_POINTER(birandmax_1))!=MP_OKAY)
		{FREE_BIGINTEGER(birandmax_1,owner_biginteger);birandmax_1=NULL;}
	}
	return birandmax_1;
}/* VALIDATED */
/**
 * @brief returns a random M big integer in [0,RAND_MAX]
 * 
 * @return Mvalue* a random M big integer in [0,RAND_MAX]
 */
Mvalue* Mrand(){Mallocationowner owner=getOwner(__LINE__); // to return a random value between 0 and 1
	Mbiginteger* _randmaxplusone=getBigintegerRandMaxPlusOne();
	if(_randmaxplusone!=NULL){
		Mbiginteger* _num=owned_biginteger(_getBiginteger(rand()),owner);
		if(_num!=NULL){
			Mrational* _rational=owned_rational(_getRational(_num,_randmaxplusone,M_LD_NAN,false),owner);
			if(_rational!=NULL)return _getValueOfRational(disowned_rational(_rational,owner));
			FREE_BIGINTEGER(_num,owner); // not bound to the returned rational
		}else
			q2outputError("Failed to create the numerator of the rational random number");
	}else
		q2outputError("Failed to create the numerator of the rational random number");
	return NULL;
}
/**
 * @brief returns a non-negative random integer below \b upper
 * 
 * @param upper the upper bound to the returned integer
 * @return long long a non-negative random integer below \b upper
 */
static long long randominteger(long long upper){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT upper should be in (0,RAND_MAX]
	long long r=M_LL_INVALID;
	Mbiginteger* _randmaxplusone=getBigintegerRandMaxPlusOne(); // will remain owned so the rational will not free it
	if(_randmaxplusone!=NULL){
		// the same as what we did in Mrand() but now multiplying the numerator with upper
		Mbiginteger *_mult=owned_biginteger(_getBiginteger(upper),owner),*_rand=owned_biginteger(_getBiginteger(rand()),owner);
		if(_mult!=NULL&&_rand!=NULL){
			Mbiginteger* _num=owned_biginteger(__biginteger(),owner);
			if(_num!=NULL){
				Mrational* _rational=NULL;
				if(mp_mul(MP_INT_POINTER(_mult),MP_INT_POINTER(_rand),MP_INT_POINTER(_num))==MP_OKAY){
					_rational=owned_rational(_getRational(_num,_randmaxplusone,M_LD_NAN,false),owner);
					if(_rational!=NULL){
						Mbiginteger* _biginteger=owned_biginteger(_rational2biginteger(_rational),owner);
						if(_biginteger!=NULL){
							r=biginteger2long(_biginteger);
							FREE_BIGINTEGER(_biginteger,owner);
						}else
							q2outputError("Failed to determine the integer part of the rational random number");
					}
				}
				if(_rational!=NULL)FREE_RATIONAL(_rational,owner);else FREE_BIGINTEGER(_num,owner);
			}else
				q2outputError("Failed to create the random rational numerator");
		}
		if(_mult!=NULL)FREE_BIGINTEGER(_mult,owner);if(_rand!=NULL)FREE_BIGINTEGER(_rand,owner);
	}
	return r;
}
/**
 * @brief returns a random integer in [0,_upperValue]
 * @details _upperValue needs to be positive and not exceed RAND_MAX
 *          if _upperValue is a list or array, this function is applied to each element
 * @param _upperValue 
 * @return Mvalue* a random integer in [0,_upperValue], or M_LL_INVALID on failure
 */
Mvalue* Mirand(Mvalue* _upperValue){Mallocationowner owner=getOwner(__LINE__);
	if(_upperValue!=NULL){
		if(_upperValue->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_upperValue->value._array,Mirand,VT_UNDEFINED));
		if(_upperValue->type==VT_LIST)return _getValueOfList(appliedToList(_upperValue->value._list,Mirand,VT_UNDEFINED));
		if(_upperValue->type==VT_MAP)return _getValueOfMap(appliedToMap(_upperValue->value._map,Mirand,VT_UNDEFINED));
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
/**
 * @brief returns a list with \p _countValue random values in [0,1)
 * @details \p countValue should be positive
 * @param _countValue 
 * @return Mvalue* a list with \p _countValue random integer values
 */
Mvalue* Mrands(Mvalue* _countValue){Mallocationowner owner=getOwner(__LINE__);
	long long count=getValueInteger(_countValue);
	if(count>0){
		Marray* _randarray=owned_array(_getArray("Mrands",count,NULL),owner);
		if(_randarray!=NULL){
			Mvalue** valueholder=_randarray->elements->values;
			while(--count>=0){assignValue(valueholder,Mrand());valueholder++;}
			return _getValueOfArray(disowned_array(_randarray,owner));
		}
		q2outputMessage(M_ERROR_PREFIX,"Failed to create an array to hold %lld random rational numbers in [0,1).",count);
		/* replacing:
		Mlist* _randList=owned_list(__list("Mrands"),owner);
		while(--count>=0&&appendedToList(_randList,owner,Mrand(),M_LL_INVALID)>0);
		return _getValueOfList(disowned_list(_randList,owner));
		*/
	}
	return NULL;
}
/**
 * @brief returns an array of \p _countValue random integers in [0,_upperValue]
 * 
 * @param _countValue 
 * @param _upperValue 
 * @return Mvalue* an array of \p _countValue random integers in [0,_upperValue]
 */
Mvalue* Mirands(Mvalue* _countValue,Mvalue* _upperValue){Mallocationowner owner=getOwner(__LINE__);
	long long count=getValueInteger(_countValue);
	if(count>0){
		long long upper=getValueInteger(_upperValue);
		if(upper>0&&upper<=RAND_MAX){
			Marray* _randarray=owned_array(_getArray("Mrands",count,NULL),owner); // guarantees elements field and elements->values to be non-NULL
			if(_randarray!=NULL){
				Mvalue** valueholder=_randarray->elements->values;
				while(--count>=0){
					long long r=randominteger(upper);
					assignValue(valueholder,(r>=0?_getIntegerValue(r):NULL));
					valueholder++;
				}
				return _getValueOfArray(disowned_array(_randarray,owner));
			}
			q2outputMessage(M_ERROR_PREFIX,"Failed to create an array to hold %lld random integer numbers in [0,%lld).",count,upper);
		}else
		if(upper<=0)
			q2outputMessage(M_ERROR_PREFIX,"%llu should be positive.",upper);
		else
			q2outputMessage(M_ERROR_PREFIX,"%lld should not exceed %lu.",upper,RAND_MAX);
	}
	return NULL;
}
/**
 * @brief sets the seed of the pseudorandom number generator to \p _seedValue
 * @details if \p _seedValue is undefined, time(NULL) is used as seed
 *          calls srand(seed) to set the seed
 *          M_LL_INVALID is returned when the seed is invalid (not positive or larger than UINT_MAX)
 * @param _seedValue 
 * @return Mvalue* M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
Mvalue* Msrand(Mvalue* _seedValue){
	long long result=M_LL_INVALID;
	long long seed=(_seedValue!=NULL?getValueInteger(_seedValue):time(NULL));
	long long seedmax=UINT_MAX;
	if(seed>0&&seed<=seedmax){
		srand(seed);
		result=M_TRUE;
	}else
	if(seed>0)
		result=M_FALSE;
	return _getIntegerValue(result);
}
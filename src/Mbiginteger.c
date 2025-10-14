#include "Mbiginteger.h"

extern unsigned long long M_MODULE_DEBUGGING;
extern const long long M_LL_INVALID,M_ZERO,M_POSITIVE,M_NEGATIVE,M_TRUE,M_FALSE;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_BIGINTEGER,id};}

/**
 * @brief returns the pointer to a new M big integer containing the negation of the M big integer pointed to by \p biginteger
 * @exception returns NULL when \p biginteger is NULL
 * @param biginteger 
 * @return Mbiginteger* the pointer to the new M big integer
 */
Mbiginteger* _getNegatedBiginteger(Mbiginteger const * const biginteger){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==biginteger)return NULL;
	Mbiginteger* _bineg=OWNED(__biginteger(),owner);
	if(_bineg!=NULL&&mp_neg(MP_INT_POINTER(biginteger),MP_INT_POINTER(_bineg))!=MP_OKAY)
	{FREE_BIGINTEGER(_bineg,owner);q2outputError("Failed to negate a big integer");return NULL;}
	return DISOWNED(_bineg,owner);
}
/**
 * @brief returns the M sign of the M big integer pointed to by \p biginteger
 * 
 * @param biginteger 
 * @return long long either M_POSITIVE, M_NEGATIVE, M_ZERO or M_LL_INVALID
 */
long long getBigintegerSign(Mbiginteger const * const biginteger){
	if(NULL==biginteger)return M_LL_INVALID;
	long long result=(mp_iszero(MP_INT_POINTER(biginteger))==MP_YES?M_ZERO:(mp_isneg(MP_INT_POINTER(biginteger))==MP_YES?M_NEGATIVE:M_POSITIVE)); // OOPS, comparing with MP_YES essential!!!
	if(amVerboseDebugging()){
		q2outputBiginteger("Sign of big integer '",biginteger,"':");
		q2output("%lld.\n",result);
	}
	return result;
}
/**
 * @brief returns the M boolean indicating whether or not \p biginteger is zero
 * 
 * @param biginteger 
 * @return long long either M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isBigintegerZero(Mbiginteger const * const biginteger){
	long long bigintegerSign=getBigintegerSign(biginteger);
	return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_ZERO?M_TRUE:M_FALSE));
}
/**
 * @brief returns the M boolean indicating whether or not \p biginteger is positive
 * 
 * @param biginteger 
 * @return long long either M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isBigintegerPositive(Mbiginteger const * const biginteger){
	long long bigintegerSign=getBigintegerSign(biginteger);
	return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_POSITIVE?M_TRUE:M_FALSE));
}
/**
 * @brief returns the M boolean indicating whether or not \p biginteger is negative
 * 
 * @param biginteger 
 * @return long long either M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isBigintegerNegative(Mbiginteger const * const biginteger){
	long long bigintegerSign=getBigintegerSign(biginteger);
	return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_NEGATIVE?M_TRUE:M_FALSE));
}

// MDH@14OCT2025
long long isBigintegerOdd(Mbiginteger const * const biginteger){
	long long result=M_LL_INVALID;
	if(biginteger!=NULL){
		mp_int mpAnd1;
		if(mp_init(&mpAnd1)==MP_OKAY){
			if(mp_and(biginteger->_bi,MP_INT_POINTER(getBigintegerOne()),&mpAnd1)==MP_OKAY)
				result=(mp_iszero(&mpAnd1)?M_FALSE:M_TRUE);
			mp_clear(&mpAnd1);
		}
	}
	return result;
}
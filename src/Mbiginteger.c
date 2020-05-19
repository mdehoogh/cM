#include "Mbiginteger.h"

static int32_t const MODULE_ID=(10<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

extern const long long M_LL_INVALID,M_ZERO,M_POSITIVE,M_NEGATIVE,M_TRUE,M_FALSE;

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    Mbiginteger* _bineg=__biginteger();
    if(_bineg&&mp_neg(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_bineg))!=MP_OKAY)
    {free_biginteger(_bineg);_bineg=NULL;outputError("Failed to negate a big integer");}
    return _bineg;
}

long long getBigintegerSign(Mbiginteger const * const biginteger){
    if(!biginteger)return M_LL_INVALID;
    long long result=(mp_iszero(MP_INT_POINTER(biginteger))==MP_YES?M_ZERO:(mp_isneg(MP_INT_POINTER(biginteger))==MP_YES?M_NEGATIVE:M_POSITIVE)); // OOPS, comparing with MP_YES essential!!!
    if(amVerbose()){outputBiginteger("Sign of big integer '",biginteger,"':");output("%lld.\n",result);}
    return result;
}
long long isBigintegerZero(Mbiginteger const * const biginteger){
    long long bigintegerSign=getBigintegerSign(biginteger);
    return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_ZERO?M_TRUE:M_FALSE));
}
long long isBigintegerPositive(Mbiginteger const * const biginteger){
    long long bigintegerSign=getBigintegerSign(biginteger);
    return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_POSITIVE?M_TRUE:M_FALSE));
}
long long isBigintegerNegative(Mbiginteger const * const biginteger){
    long long bigintegerSign=getBigintegerSign(biginteger);
    return(bigintegerSign==M_LL_INVALID?M_LL_INVALID:(bigintegerSign==M_NEGATIVE?M_TRUE:M_FALSE));
}
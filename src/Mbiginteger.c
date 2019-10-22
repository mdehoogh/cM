#include "Mbiginteger.h"

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    Mbiginteger* _bineg=__biginteger();
    if(_bineg&&mp_neg(_biginteger,_bineg)!=MP_OKAY){free_biginteger(_bineg);_bineg=NULL;outputError("Failed to negate a big integer");}
    return _bineg;
}

long long getBigintegerSign(Mbiginteger const * const biginteger){
    if(!biginteger)return M_LL_INVALID;
    if(mp_iszero(biginteger))return 0;
    return(mp_isneg(biginteger)==MP_NO?1:-1);
}

bool isBigintegerPositive(Mbiginteger const * const biginteger){long long bigintegerSign=getBigintegerSign(biginteger);return(bigintegerSign==M_LL_INVALID?false:bigintegerSign>0);}
bool isBigintegerNegative(Mbiginteger const * const biginteger){long long bigintegerSign=getBigintegerSign(biginteger);return(bigintegerSign==M_LL_INVALID?false:bigintegerSign<0);}
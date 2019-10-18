#include "Mbiginteger.h"

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    Mbiginteger* _bineg=__biginteger();
    if(_bineg&&mp_neg(_biginteger,_bineg)!=MP_OKAY){free_biginteger(_bineg);_bineg=NULL;outputError("Failed to negate a big integer");}
    return _bineg;
}
bool isBigintegerPositive(Mbiginteger* _biginteger){return(_biginteger!=NULL&&mp_iszero(_biginteger)==MP_NO&&_biginteger->sign==MP_ZPOS);}
bool isBigintegerNegative(Mbiginteger* _biginteger){return(_biginteger!=NULL&&_biginteger->sign==MP_NEG);}
#include "Mbiginteger.h"

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger){
    if(!_biginteger)return NULL;
    Mbiginteger* _bineg=__biginteger();
    if(_bineg&&mp_neg(_biginteger,_bineg)!=MP_OKAY){free_biginteger(_bineg);_bineg=NULL;outputError("Failed to negate a big integer");}
    return _bineg;
}

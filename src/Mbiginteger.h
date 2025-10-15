// Mvalue.h includes Mexecution.h where Mbiginteger is defined!!!
#include "Mexecution.h"

Mbiginteger* _getNegatedBiginteger(Mbiginteger const * const _biginteger);

long long getBigintegerSign(Mbiginteger const * const biginteger);
long long isBigintegerPositive(Mbiginteger const * const biginteger);
long long isBigintegerNegative(Mbiginteger const * const biginteger);

long long isBigintegerZero(Mbiginteger const * const biginteger);

long long isMpintOdd(mp_int const * const mp_int);
long long isBigintegerOdd(Mbiginteger const * const biginteger);
// Mvalue.h includes Mexecution.h where Mbiginteger is defined!!!
#include "Mexecution.h"

Mbiginteger* _getNegatedBiginteger(Mbiginteger* _biginteger);

long long getBigintegerSign(Mbiginteger const * const biginteger);

bool isBigintegerPositive(Mbiginteger const * const biginteger);
bool isBigintegerNegative(Mbiginteger const * const biginteger);

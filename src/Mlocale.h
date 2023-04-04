#include <stdlib.h>

#include "Mmatrix.h"

Mmap* getLocalesettingsMap(); // exposes the current locale value
bool updateLocalesettingsMap();

size_t outputLongLongLocale(const long long ll);
size_t outputIntegerLocale(Minteger const * const integer);
size_t outputBigintegerLocale(Mbiginteger const * const biginteger);
size_t outputFloatLocale(Mfloat const * const _float);
size_t outputDecimalLocale(Mdecimal const * const _decimal,bool fixedpoint);
#include <stdlib.h>

#include "Mlist.h"

Mmap* getLocalesettingsMap(); // exposes the current locale value
bool updateLocalesettingsMap();

size_t outputLongLongLocale(long long ll);
size_t outputIntegerLocale(Minteger* integer);
size_t outputBigintegerLocale(Mbiginteger* biginteger);
size_t outputFloatLocale(Mfloat* _float);
size_t outputDecimalLocale(Mdecimal* _decimal,bool fixedpoint);
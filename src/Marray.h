#include "Miterator.h"

Mvalue* marray(Mvalue* length_value,Mvalue* fill_value);
Mvalue* mfill(Mvalue* _array,Mvalue* _value);

// iterator support
Miterator getArrayIterator(Marray* array);
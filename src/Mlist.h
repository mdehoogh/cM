// some common list functions
#include "Mvalue.h"

Mvalue* Mempty(Mvalue* value); // will also work on a map though (TODO move over to functions.h/c I guess)

Mvalue* Mpush(Mvalue* listValue,Mvalue* value); // append a value to the list (also referred to as drop)
Mvalue* Mshove(Mvalue* listValue,Mvalue* value); // prepend a value to a list

Mvalue* Mpop(Mvalue* listValue); // remove and return the last value (how about takeoff)
Mvalue* Mpull(Mvalue* listValue); // remove and return the first value

Mvalue* Mfirst(Mvalue* listValue); // return the first value
Mvalue* Mlast(Mvalue* listValue); // return the last value
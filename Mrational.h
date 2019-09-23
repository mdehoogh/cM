#include "Mbiginteger.h"

long double realsum(Mreal* _real1,Mreal* _real2); // TODO should be moved to Mreal I suppose at some point 

mp_err _qadd(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qsub(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qmul(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qdiv(Mrational* c,Mrational const * const a,Mrational const * const b);

bool realIsUndefined(Mreal* _real);

Mrational* _getRationalSum(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalDifference(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalProduct(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalQuotient(Mrational const * const q1,Mrational const * const q2);

Mrational* _qsinorcos(Mrational const * const x,bool sin);
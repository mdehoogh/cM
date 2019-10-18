#include "Mvalue.h"

// unary functions
Mvalue* Mneg(Mvalue* _value); // negate a value
Mvalue* Mbnot(Mvalue* _value); // binary not a value
Mvalue* Mnot(Mvalue* _value); // not a value

Mvalue* Mnull(Mvalue* _value); // whether null!!!
Mvalue* Mundefined(Mvalue* _value); // whether undefined!!!
Mvalue* Mzero(Mvalue* _value);
Mvalue* Mpositive(Mvalue* _value);
Mvalue* Mnegative(Mvalue* _value);
Mvalue* Mscalar(Mvalue* _value);

Mvalue* Msum(Mvalue* _value); // sum (typically of a list)
Mvalue* Mlen(Mvalue* _value); // length (typically of a list)
Mvalue* Mfacd(Mvalue* _value); // faculty (for an integer)
Mvalue* Mfac(Mvalue* _value); // faculty (for an integer)

///////Mvalue* Mbi(Mvalue* _value); // convert to a big integer

// math one-argument functions
Mvalue* Mfloor(Mvalue* _value);
Mvalue* Mtrunc(Mvalue* _value);
Mvalue* Mround(Mvalue* _value);
Mvalue* Mceil(Mvalue* _value);
Mvalue* Msin(Mvalue* _value);
Mvalue* Mcordicsin(Mvalue* _value);
Mvalue* Mcos(Mvalue* _value);
Mvalue* Mcordiccos(Mvalue* _value);
Mvalue* Mtan(Mvalue* _value);
Mvalue* Msinh(Mvalue* _value);
Mvalue* Mcosh(Mvalue* _value);
Mvalue* Mtanh(Mvalue* _value);
Mvalue* Mexp(Mvalue* _value);
Mvalue* Mdexp(Mvalue* _value); // internal approximation by series expansion
Mvalue* Mlog(Mvalue* _value);
Mvalue* Mlog10(Mvalue* _value);
Mvalue* Msqrt(Mvalue* _value);

// additional one-argument functions
Mvalue* Mout(Mvalue* _value);
Mvalue* Mbc(Mvalue* _value); // set background color
Mvalue* Mtc(Mvalue* _value); // set text color
Mvalue* Mbrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3); // set background color
Mvalue* Mtrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3); // set text color

// math two-argument functions
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue);

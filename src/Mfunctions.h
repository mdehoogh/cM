#include "Mlist.h"

// unary functions
Mvalue* Mneg(Mvalue* _value); // negate a value
Mvalue* Mbnot(Mvalue* _value); // binary not a value
Mvalue* Mnot(Mvalue* _value); // not a value

Mvalue* Mnull(Mvalue* _value); // whether null!!!
Mvalue* Mundefined(Mvalue* _value); // whether undefined!!!
Mvalue* Mzero(Mvalue* _value);
Mvalue* Msign(Mvalue* value);
Mvalue* Mpositive(Mvalue* _value);
Mvalue* Mnegative(Mvalue* _value);
Mvalue* Mscalar(Mvalue* _value);

Mvalue* Msum(Mvalue* _value); // sum (typically of a list)
Mvalue* Mlen(Mvalue* _value); // length (typically of a list)
Mvalue* Mfacd(Mvalue* _value); // faculty (for an integer)
Mvalue* Mfac(Mvalue* _value); // faculty (for an integer)

Mvalue* Mtl(Mvalue* _value); // length of a text

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
Mvalue* Mout(Mvalue* _value); // for writing text to standard output (console)
Mvalue* Min(Mvalue* value); // for reading text from standard input (console)

Mvalue* Mbc(Mvalue* _value); // get a background color text representation where value indicates a color in 256-color mode, so an integer in range [0,255]
Mvalue* Mtc(Mvalue* _value); // get a text color text representation
Mvalue* Mbrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3); // get rgb background color text representation
Mvalue* Mtrgb(Mvalue* _value1,Mvalue* _value2,Mvalue* _value3); // get rgb text color text representation

// math two-argument functions
Mvalue* Mpow(Mvalue* _value,Mvalue* _exponentValue);
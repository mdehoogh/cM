#include "Mbiginteger.h"

long double realsum(Mfloat* _real1,Mfloat* _real2); // TODO should be moved to Mfloat I suppose at some point 

// MDH@10OCT2019: moved over from Mexecution.h/c
// the following two methods will use M_LD_Q_EPS as default cut-off value
Mrational* _getLongDoubleRational(long double ld,int maxiter); // convert a long double to its rational equivalent and wraps it in a value
long double getRationalLongDouble(Mrational const * const _rational);

Mrational* _getDecimalTextRational(char* decimalText/*,bool freeonfailure*/); 
// used by (now moved over to Mdecimal.h/c): Mrational* _getDecimalRational(Mdecimal* _decimal);

Mrational* __rational();
void free_rational(Mrational* _rational);
void normalizeRational(Mrational* _rational);
Mrational* _getRational(Mbiginteger* _numerator,Mbiginteger* _denominator,long double delta,bool normalize,bool freeonfailure);
Mrational* _getInverseRational(Mrational const * const _rational);
Mstring* _getRationalText(const Mrational* const _rational);
Mbiginteger* _rational2biginteger(Mrational* _rational); // computes the integer part of the rational
void outputRational(const char* const prefix,const Mrational* const _rational,const char* const postfix);

mp_err _qadd(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qsub(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qmul(Mrational* c,Mrational const * const a,Mrational const * const b);
mp_err _qdiv(Mrational* c,Mrational const * const a,Mrational const * const b);

// MDH@23OCT2019: use getRationalSign() now replacing: long long qcmp(Mrational const * const a,Mrational const * const b); // MDH@16OCT2019: what to return if a or b is not defined????? I suppose NULL is smaller than any value???????

Mrational* _getPureRationalSum(Mrational const * const q1,Mrational const * const q2); // MDH@09OCT2019: can come in handy

Mrational* _getRationalSum(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalDifference(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalProduct(Mrational const * const q1,Mrational const * const q2);
Mrational* _getRationalQuotient(Mrational const * const q1,Mrational const * const q2);

Mrational* _qsinorcos(Mrational const * const x,bool sin);

// getting the sign (M_POSITIVE, M_NEGATIVE, M_ZERO or M_LL_INVALID)
long long isRationalUndefined(Mrational const * const rational);
long long getRationalSign(Mrational const * const rational); // returns -1 for negative rationals, 1 for positive rationals, 0 for zero rationals and M_LL_INVALID for undefined rationals
long long isRationalZero(Mrational const * const rational);
long long isRationalPositive(Mrational const * const rational);
long long isRationalNegative(Mrational const * const rational);
long long isRationalOne(Mrational const * const rational);
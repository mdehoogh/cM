#include "Mrational.h"

// mpd_context_t primitives
void report_mpd_status(uint32_t mpd_status);

bool mpd_error(const mpd_context_t* const mpd_context);

mpd_context_t* get_mpd_context(mpd_ssize_t decimal_precision);
mpd_t* __mpd(const mpd_context_t* mpd_context,int64_t value);

// mpd_t primitives
void free_mpd(mpd_t* mpd);

// MDH@06SSEP2019: we also want to store predefined sine/cosines to speed up the computation and consistency of sine/cosines
typedef struct Msincoselement{
    mpd_t *_angle,*_sine,*_cosine;
    uint32_t mult;
    struct Msincoselement* _next;
}Msincoselement;

// MDH@29AUG2019: we want to store the computed value of pi and e in the decimal context as well unless we store pi and e with a postfix like PI$<prec> and E$<prec> in the current environment?????
//                the point here is that of reuse, we do not want to have to compute pi again and again for each decimal context
//                currently we store the precision with the decimal this way we can use the same precision on functions that use that argument
//                given the application-wide decimal precision stored in _decimalContext that is used to store decimals, I suppose this would be the decimal context of decimal literals
typedef struct Mdecimalcontext{
    mpd_context_t* mpd_context;
    mpd_t* e; // storing e
    mpd_t *pi,*pidiv2,*pimul2,*pidiv4; // storing pi, 2*pi, pi/2 and pi/4 and pi/512 which is the distance between two successive angles in the table of predefined sines
    mpd_t* predefinedsinedeltaangle; // MDH@11SEP2019: the difference between successive angles in the predefined sines table
    mpd_t* predefinedsines[257]; // MDH@11SEP2019: where we're going to store all predefined 257 sines (starting at 0 and up until pi/512)
    Msincoselement* _firstSincoselement; // for storing the predefined sine/cosines
    Msincoselement* _firstCordicelement; // for storing the CORDIC sine/cosines
}Mdecimalcontext;

// MDH@29AUG2019: create a decimal context with __decimalcontext passing in the required precision
Mdecimalcontext* _getDecimalcontext(mpd_ssize_t prec); // to get the unique decimal context with the requested precision

Mdecimal* disowned_decimal(Mdecimal* _decimal,Mallocationowner owner_decimal);
Mdecimal* owned_decimal(Mdecimal* _decimal,Mallocationowner owner_decimal);

Mdecimal* __adecimal(); // returning a completely blank decimal (e.g. to be used with mpd_copy_negate otherwise we'd have the old pointer hanging around with an allocated decimal that won't get freed anywhere ever)
Mdecimal* __decimal(mpd_context_t const * mpd_context,int64_t value,uint64_t repeating); // pass in NULL for mpd_context to use the application-wide decimal context!!
void free_decimal(Mdecimal* decimal/*,Mallocationowner owner_decimal*/);
#define FREE_DECIMAL(_decimal,owner_decimal) free_decimal(disowned_decimal(_decimal,owner_decimal))

// TODO if we call _getDecimal with mpd we won't know the precision of the decimal (as that is not contained in an mpd_t instance), therefore we need to change _getDecimal somehow!!!
// DONE I've added the prec parameter because mpd_t itself does not store the precision used to compute this decimal
Mdecimal* _getDecimal(mpd_t const * const _mpd,mpd_ssize_t prec,uint64_t repeating,bool freeonfailure);

Mdecimal* _getDecimalCopy(Mdecimal const * const decimal);

Mdecimal* _getDecimalSum(Mdecimal const * const d1,Mdecimal const * const d2);
Mdecimal* _getDecimalDifference(Mdecimal const * const d1,Mdecimal const * const d2);
Mdecimal* _getDecimalProduct(Mdecimal const * const d1,Mdecimal const * const d2);
Mdecimal* _getDecimalQuotient(Mdecimal const * const d1,Mdecimal const * const d2);

// compute an decimal approximation to pi which we can then store in the given decimal context unless it's already in there of course
Mdecimal* pi_decimal(Mdecimalcontext* decimalcontext,bool computesinetable);

mpd_t* _dsinsquared(mpd_context_t const * const mpd_context,mpd_t const * const x);

// ONE ARGUMENT MATH FUNCTIONS
Mdecimal* _dsine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);
Mdecimal* _dcordicsine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);
Mdecimal* _dcosine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);
Mdecimal* _dcordiccosine(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);

Mdecimal* _dtangent(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);

Mdecimal* _dexp(Mdecimalcontext const * decimalcontext,Mdecimal const * const x);

// CONVERSION
Mdecimal* _getBigintegerDecimal(Mbiginteger const * const _biginteger); // MDH@08OCT2019

long double getDecimalLongDouble(Mdecimal* _decimal); /// MDH@09OCT2019: moved over here from M.c

Mdecimal* _getRationalDecimal(Mrational const * const _rational); // converts a rational to a decimal
Mrational* _getDecimalRational(Mdecimal const * const decimal); // converts a decimal back to a rational

Mdecimal* _getTextDecimal(char const * const decimalText,uint64_t repeating);

Mstring* _getDecimalText(Mdecimal const * const _decimal,bool fixedpoint);

Mdecimal* _getInverseDecimal(Mdecimal const * const decimal);

long long decimal2long(Mdecimal* decimal);

// the now well-known sign and M boolean functions
long long isDecimalUndefined(Mdecimal const * const decimal);
long long getDecimalSign(Mdecimal const * const decimal);
long long isDecimalPositive(Mdecimal const * const decimal);
long long isDecimalNegative(Mdecimal const * const decimal);
long long isDecimalZero(Mdecimal const * const decimal);
long long isDecimalOne(Mdecimal const * const decimal);

Mstring* _getDecimalJSON(Mdecimal const * const _decimal);
Mdecimal* _getJSONDecimal(Mstring const * const _decimalJSON);

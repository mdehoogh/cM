#include "Mbiginteger.h"

// mpd_context_t primitives
void report_mpd_status(uint32_t mpd_status);

bool mpd_error(const mpd_context_t* const mpd_context);

mpd_context_t* get_mpd_context(mpd_ssize_t decimal_precision);
mpd_t* __mpd(const mpd_context_t* mpd_context,int64_t value);

// mpd_t primitives
void free_mpd(mpd_t* mpd);

// MDH@29AUG2019: we want to store the computed value of pi and e in the decimal context as well unless we store pi and e with a postfix like PI$<prec> and E$<prec> in the current environment?????
//                the point here is that of reuse, we do not want to have to compute pi again and again for each decimal context
//                currently we store the precision with the decimal this way we can use the same precision on functions that use that argument
//                given the application-wide decimal precision stored in _decimalContext that is used to store decimals, I suppose this would be the decimal context of decimal literals
typedef struct Mdecimalcontext{
    mpd_context_t* mpd_context;
    mpd_t* e; // storing e
    mpd_t *pi,*pidiv2,*pimul2,*pidiv4; // storing pi, 2*pi, pi/2 and pi/4
}Mdecimalcontext;
// MDH@29AUG2019: create a decimal context with __decimalcontext passing in the required precision
Mdecimalcontext* _getDecimalcontext(mpd_ssize_t prec); // to get the unique decimal context with the requested precision

void free_decimal(Mdecimal* decimal);
Mdecimal* __adecimal(); // returning a completely blank decimal (e.g. to be used with mpd_copy_negate otherwise we'd have the old pointer hanging around with an allocated decimal that won't get freed anywhere ever)
Mdecimal* __decimal(const mpd_context_t* mpd_context,int64_t value,uint64_t repeating); // pass in NULL for mpd_context to use the application-wide decimal context!!
// TODO if we call _getDecimal with mpd we won't know the precision of the decimal (as that is not contained in an mpd_t instance), therefore we need to change _getDecimal somehow!!!
// DONE I've added the prec parameter because mpd_t itself does not store the precision used to compute this decimal
Mdecimal* _getDecimal(mpd_t* mpd,mpd_ssize_t prec,uint64_t repeating,bool freeonfailure);

Mdecimal* _getDecimalCopy(Mdecimal* decimal);

bool isDecimalZero(Mdecimal* decimal);
bool isDecimalOne(Mdecimal* _decimal);

// compute an decimal approximation to pi which we can then store in the given decimal context unless it's already in there of course
Mdecimal* pi_decimal(Mdecimalcontext* decimalcontext);

// ONE ARGUMENT MATH FUNCTIONS
Mdecimal* _dsine(const Mdecimalcontext* decimalcontext,Mdecimal* x);
Mdecimal* _dcosine(const Mdecimalcontext* decimalcontext,Mdecimal* x);
Mdecimal* _dexp(const Mdecimalcontext* decimalcontext,Mdecimal* x);

// CONVERSION
Mdecimal* _getRationalDecimal(const Mrational* const _rational); // converts a rational to a decimal

Mdecimal* _getTextDecimal(const char* const decimalText,uint64_t repeating);

Mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint);

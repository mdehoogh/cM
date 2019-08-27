#include "Mbiginteger.h"

void report_mpd_status(const mpd_context_t* const mpd_context);

bool mpd_error(const mpd_context_t* const mpd_context);

mpd_context_t* get_mpd_context(mpd_ssize_t decimal_precision);
mpd_t* __mpd(const mpd_context_t* mpd_context,int64_t value);
void free_mpd(mpd_t* mpd);
void free_decimal(Mdecimal* decimal);
Mdecimal* __adecimal(); // returning a completely blank decimal (e.g. to be used with mpd_copy_negate otherwise we'd have the old pointer hanging around with an allocated decimal that won't get freed anywhere ever)
Mdecimal* __decimal(const mpd_context_t* mpd_context,int64_t value,uint64_t repeating); // pass in NULL for mpd_context to use the application-wide decimal context!!
Mdecimal* _getDecimal(mpd_t* mpd,uint64_t repeating,bool freeonfailure);
Mdecimal* _getDecimalCopy(Mdecimal* decimal);
bool isDecimalZero(Mdecimal* decimal);
bool isDecimalOne(Mdecimal* _decimal);

// compute an decimal approximation to pi
Mdecimal* pi_decimal(const mpd_context_t* decimalContext);

// ONE ARGUMENT MATH FUNCTIONS
Mdecimal* _dsine(const mpd_context_t* decimalContext,Mdecimal* x);
Mdecimal* _dcosine(const mpd_context_t* decimalContext,Mdecimal* x);

// CONVERSION
Mdecimal* _getRationalDecimal(const Mrational* const _rational); // converts a rational to a decimal

Mdecimal* _getTextDecimal(const char* const decimalText,uint64_t repeating);

Mstring* _getDecimalText(const Mdecimal* const _decimal,bool fixedpoint);

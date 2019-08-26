#include "Mvalue.h"

void report_mpd_status(mpd_context_t* mpd_context);

bool mpd_error(mpd_context_t* mpd_context);

Mdecimal* _getValueDecimal(Mvalue* _value);
Mdecimal* getValueDecimal(Mvalue* _value);

// compute an decimal approximation to pi
Mdecimal* pi_decimal(mpd_context_t* decimalContext);
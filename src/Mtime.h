#include <time.h>

#include "Mvalue.h"

/*
MDH@08DEC2020: 
- should not be too difficult to work with timestamps
- conversion to and from a calendar is an issue, in essence any calendar date/time is always associated with a timezone
- but can't actually expect a user to enter a unix time obviously, well perhaps
*/


Mvalue* Mnow();
Mvalue* Mcalendartime(Mvalue* _timeValue);
Mvalue* Mparsetime(Mvalue* _timetextValue);

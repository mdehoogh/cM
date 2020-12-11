#include "Mvalue.h"

Mvalue* Msystemvariables();

// singular system variables
Mvalue* Mgetenv(Mvalue* _systemVariableValue);
Mvalue* Msetenv(Mvalue* _systemVariableValue,Mvalue* _value);
Mvalue* Mclearenv();
Mvalue* Munsetenv(Mvalue* _systemVariableValue);
Mvalue* Mputenv(Mvalue* _systemVariableValue,Mvalue* _value);
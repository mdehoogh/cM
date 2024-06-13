#include "Msession.h"

Mvalue* Mpython(Mvalue const * const pythonCommandValue,Mvalue const * const sysExitValue);

// MDH@23APR2024: should be adding the methods here to be callable from e.g. a Jupyter kernel cpp program
#ifdef __CPLUSPLUS
extern "C" void initializeM();
#else
void initializeM();
#endif
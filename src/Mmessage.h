/**
 * MDH@27FEB2020: methods to output M messages to standard output as used by most modules
 *                moved over from Msession.c and Mexecution.c so we can move Msession.c up (and called from within M.c only)
 */

#include "Moutput.h"

void outputInfo(char const * const info); // replacing outputLine in all modules

void outputWarning(char const * const warning);

void outputError(char const * const error);
void outputErrorAndText(char const * const error,char const * const text);
// MDH@05NOV2019: some special error reporting (typically bugs and out of memory problems)
void outputMemoryError(char const * const memoryerror);

void outputBug(char const * const bug);

int kbhit();



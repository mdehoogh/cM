/**
 * MDH@27FEB2020: methods to output M messages to standard output as used by most modules
 *                moved over from Msession.c and Mexecution.c so we can move Msession.c up (and called from within M.c only)
 */
#include <stdio.h>
#include <stdarg.h>

#include "Moutput.h"

size_t outputInfo(char const * const info); // replacing outputLine in all modules

size_t outputWarning(char const * const warning);

size_t outputError(char const * const error);
size_t outputErrorAndText(char const * const error,char const * const text);
// MDH@05NOV2019: some special error reporting (typically bugs and out of memory problems)
size_t outputMemoryError(char const * const memoryerror);

size_t outputBug(char const * const bug);

int kbhit();

// support for multiple message streams stored in a double-linked message stream list (_messageStreamStack)
bool pushMessageStream(FILE* stream,char const * const source,char const * const messageType);
bool popMessageStream(char const * const source,char const * const messageType);
bool popAllMessageStreams(char const * const source);
bool messageStreamsInitialized(char const * const source);

size_t logMessage(char const * const messageType, char const * const messagefmt,...);

// MDH@07AUG2024: for logging any text (that may contain any message types)
size_t logText(char const * const fmt,...);
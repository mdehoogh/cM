/**
 * MDH@02MAY2019:
 * - output to the console
 */
#include <stdlib.h>
#include <stdbool.h>

#include "Mmodule.h"

#include "Mconstants.h"

// MDH@13MAR2020: let's allow echoing output to a file as well if so requested
size_t logToOutputFile(const char* fmt,...); // MDH@25OCT2021: more convenient then outputToFile!!
// moved to Mmessage.h/c: bool setOutputFilename(char const * const outputFilename);
bool setOutputFile(FILE const * newOutputFile);
bool discardOutputFile();
size_t outputToFile(char const * const prefix,char const * const str,char const * const suffix);
bool echoToOutputFile();
bool dontEchoToOutputFile();

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
size_t output(const char *fmt,...);

// convenience methods delegating to output() so all output (to stdout by default) goes through function output()
size_t outputChar(char c); // MDH@18APR2019: individual characters can use outputChar (which might have used putchar)

size_t newline();

// all output to the display has to go through output!!
size_t outputControlText(char* s);
/**
 * MDH@02MAY2019:
 * - output to the console
 */
#include <stdlib.h>

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
size_t output(const char *fmt,...);

// convenience methods delegating to output() so all output (to stdout by default) goes through function output()
void outputChar(char c); // MDH@18APR2019: individual characters can use outputChar (which might have used putchar)

// all output to the display has to go through output!!
void outputControlText(char* s);
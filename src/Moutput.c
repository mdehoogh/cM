/**
 * MDH@02MAY2019:
 * - output to the console (specifically)
 * - TODO in due course any other destination???
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "Moutput.h"

static int32_t const MODULE_ID=(1<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

static FILE* outputFile=NULL;

static bool echo_to_output_file=false;

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
// MDH@08OCT2019: it's convenient to know how many characters are actually written
size_t output(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    int result=vprintf(fmt,args);
    va_end(args);
    if(echo_to_output_file){
        va_list args;
        va_start(args,fmt);
        vfprintf(outputFile,fmt,args); // MDH@13MAR2020: echo to the output file if the flag tells us to
        fflush(outputFile);
        va_end(args);
    }
    return(result<0?0:result);
} // NOTE use vprintf here, NOT printf!!!!

bool setOutputFilename(char const * const outputFilename){
    // close any current output file
    // NOTE apparently fclose() already takes care of free'ing the file handle, so calling free(outputFile) would result in a runtime error!!!
    if(outputFile){int closeResult=fclose(outputFile);if(closeResult!=0)fprintf(stderr,"Failed to close the output file (reason: %d).\n",closeResult);/*free(outputFile);*/outputFile=NULL;}
    echo_to_output_file=false;
    if(outputFilename&&strlen(outputFilename)>0){
        outputFile=fopen(outputFilename,"a+t");
        // NOTE: we can use output here because the echo to output file flag is still false!!!
        if(outputFile){output("Output will also be written to '%s'.\n",outputFilename);echo_to_output_file=true;}
    }
    return echo_to_output_file;
}
size_t outputToFile(char const * const prefix,char const * const str,char const * const suffix){
    return(outputFile?fprintf(outputFile,"%s%s%s",(prefix?prefix:""),(str?str:""),(suffix?suffix:"")):0);
}

// convenience methods delegating to output() so all output (to stdout by default) goes through function output()
size_t outputChar(char c){
    char s[2]={c,'\0'};
    return output("%s",s); //(putchar(c)<0?0:1);//return output("%c",c);
} // MDH@18APR2019: individual characters can use outputChar (which might have used putchar)

size_t newline(){return outputChar('\n');}

// all output to the display has to go through output!!
void outputControlText(char* s){output(ES"%s",s);}

bool echoToOutputFile(){if(outputFile)echo_to_output_file=true;return echo_to_output_file;}
void dontEchoToOutputFile(){echo_to_output_file=false;}

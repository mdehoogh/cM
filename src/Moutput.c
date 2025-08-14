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

/// @brief the file to which the output is echoed
static FILE* outputFile=NULL;
/// @brief whether or not the output is echoed to the output file
static bool echo_to_output_file=false;

// MDH@25OCT2021: it's better to be able to use a format for printing
/**
 * @brief utility function to log all arguments as present in format string \p fmt
 * 
 * @param fmt the format string
 * @param ... the arguments into the format string
 * @return size_t the number of characters written to the output file
 */
size_t logToOutputFile(const char* fmt,...){
	size_t result=0;
	if(outputFile!=NULL){
	  va_list args;
  	va_start(args,fmt);
  	result=vfprintf(outputFile,fmt,args); // MDH@13MAR2020: echo to the output file if the flag tells us to
  	fflush(outputFile);
  	va_end(args);	
	}
	return result;
}

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
// MDH@08OCT2019: it's convenient to know how many characters are actually written
/**
 * @brief outputs the parameters formatted by \p fmt to stdout
 * 
 * @param fmt the format string
 * @param ... the list of arguments to \p fmt
 * @return size_t the number of characters outputted
 */
size_t output(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    int result=vprintf(fmt,args);
    fflush(stdout); // MDH@16NOV2020: let's ascertain to see it on any crash
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

// MDH@25OCT2021: now delegating to logToOutputFile which accepts a format specification and variable number of arguments
/**
 * @brief writes \p prefix, \p str, and \p suffix to the output file
 * 
 * @param prefix the first text to output
 * @param str the second text to output
 * @param suffix the last text to output
 * @return size_t the number of characters written to the output file
 */
size_t outputToFile(char const * const prefix,char const * const str,char const * const suffix){
	return logToOutputFile("%s%s%s",(prefix?prefix:""),(str?str:""),(suffix?suffix:""));
}

// convenience methods delegating to output() so all output (to stdout by default) goes through function output()
/**
 * @brief outputs single character \p c
 * 
 * @param c the character to output
 * @return size_t the number of characters written
 */
size_t outputChar(char c){
	char s[2]={c,'\0'};
	return output("%s",s); //(putchar(c)<0?0:1);//return output("%c",c);
} // MDH@18APR2019: individual characters can use outputChar (which might have used putchar)

/**
 * @brief outputs the newline character
 * 
 * @return size_t the number of characters written
 */
size_t newline(){
	return outputChar('\n');
}

// all output to the display has to go through output!!
/**
 * @brief outputs \p s as control text
 * 
 * @param s the text to output as control text
 */
size_t outputControlText(char* s){return output(ES"%s",s);}

/**
 * @brief tries to set echo_to_output_file to true
 * 
 * @return true when echo_to_output_file is (set to) true
 * @return false when echo_to_output_file is false
 */
bool echoToOutputFile(){
	if(outputFile!=NULL)echo_to_output_file=true;return echo_to_output_file;
}
/**
 * @brief sets echo_to_output_file to false, and returns the previous one
 * 
 */
bool dontEchoToOutputFile(){
	bool current_echo_to_output_file=echo_to_output_file;
	echo_to_output_file=false;
	return current_echo_to_output_file;
}
/**
 * @brief registers \p newOutputFile as the output file 
 * 
 * @param newOutputFile 
 * @return echo_to_output_file which will be true if the output file is not NULL, false otherwise
 */
bool setOutputFile(FILE const * newOutputFile){
	outputFile=newOutputFile;
	// sync echo to output file so it's false when there's no output file and true otherwise!!!
	echo_to_output_file=(outputFile!=NULL);
	return echo_to_output_file;
}

/**
 * @brief discards the current output file
 * @details returns false when failing to close the current output file, true otherwise
 * 
 * @return true when there is no current output file or when the current output file was closed successfully
 * @return false when the current output file was not closed successfully
 */
bool discardOutputFile(){
	if(NULL==outputFile)return true;
	bool closed=fclose(outputFile);
	setOutputFile(NULL); // get rid of the output file!!
	return closed;
}

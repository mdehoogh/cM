#include <string.h>

#include <unistd.h>
#include <termios.h>

#include "Mmessage.h"

// the texts to be used in certain message types
extern const char* const INFO_PREFIX;
extern const char* const ERROR_PREFIX;
extern const char* const WARNING_PREFIX;
extern const char* const BUG_PREFIX;

void outputInfo(char const * const info){
    size_t l=(info?strlen(info):0);
    if(l==0)return;
    output("%s%s",INFO_PREFIX,info);
    l--;if(info[l]!='.'&&info[l]!='!'&&info[l]!='?')outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    newline();
}

void outputWarning(char const * const warning){
    size_t l=(warning?strlen(warning):0);
    if(l==0)return;
    if(!WARNING_PREFIX)return;
    output("%s%s",WARNING_PREFIX,warning);
    l--;if(warning[l]!='.'&&warning[l]!='!'&&warning[l]!='?')outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    newline();
}

void outputError(char const * const error){
    size_t l=(error?strlen(error):0);
    if(l==0)return;
    if(!ERROR_PREFIX)return;
    output("%s%s",ERROR_PREFIX,error);
    l--;if(error[l]!='.'&&error[l]!='!'&&error[l]!='?')outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    newline();
    // replacing: if(error)output("%s%s.\n",ERROR_PREFIX,error);
}

void outputMemoryError(char const * const memoryerror){
    if(memoryerror)output("%s%s. Probable cause: out of memory!\n",ERROR_PREFIX,memoryerror);
}

// MDH@05NOV2019: might come in handy to be able to report bugs
void outputErrorAndText(char const * const error,char const * const text){if(error)output("%s%s",ERROR_PREFIX,error);if(text)output(text);output(".\n");}

void outputBug(char const * const bug){
    size_t l=(bug?strlen(bug):0);
    if(l==0)return;
    if(!BUG_PREFIX)return;
    output("%s%s",BUG_PREFIX,bug);
    l--;if(bug[l]!='.'&&bug[l]!='!'&&bug[l]!='?')outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    newline();
} 

// for now placing kbhit() here
int kbhit(){
    struct timeval tv={0L,0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1,&fds,NULL,NULL,&tv);
}
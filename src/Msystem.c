#include <stdlib.h>
#include <unistd.h> 

#include "Msystem.h"

extern char **environ;

extern char const * const M_ERROR_PREFIX;
extern long long M_MODULE_DEBUGGING;
extern long long M_TRUE;
#define DEBUGGING (M_MODULE_DEBUGGING|MM_SYSTEM)

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_SYSTEM,id};}

Mvalue* Msystemvariables(){Mallocationowner owner=getOwner(__LINE__);
    // process char** environ
    Mmap* _environMap=owned_map(__map("Mgetenvironment"),owner);
    if(_environMap){
        char* _equalpos;
        char** _environ=environ;
        while(*_environ){
            _equalpos=strchr(*_environ,'=');
            if(_equalpos){
                // part behind _equal pos is the text value
                Mstring* _environText=owned_string(_getString("'"),owner);
                if(_environText){
                    if(string_append(_environText,_equalpos+1)){
                        Mvalue* _environValue=_getTextValue(string(_environText));
                        *_equalpos='\0'; // dangerous!!!!
                        if(appendedToMap(_environMap,owner,*_environ,_environValue)!=M_TRUE)
                            output("%sFailed to store system variable '%s'.",M_ERROR_PREFIX,*_environ);
                        *_equalpos='='; // restored
                    }else
                        outputError("Failed to remember the value of a system variable");
                    FREE_STRING(_environText,owner);
                }else
                    outputError("Failed to store the system variable!");
            }
            _environ++;
        }
        return _getValueOfMap(disowned_map(_environMap,owner));
    }
    return NULL;
}
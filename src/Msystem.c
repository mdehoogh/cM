#include <stdlib.h>
#include <unistd.h> 

#include "Msystem.h"

extern long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_SYSTEM,id};}

extern char **environ;

extern char const * const M_ERROR_PREFIX;
extern char const * const M_WARNING_PREFIX;
extern long long M_TRUE;
extern long long M_FALSE;
extern long long M_LL_INVALID;

Mvalue* Msystemvariables(){Mallocationowner owner=getOwner(__LINE__);
    // process char** environ
    Mmap* _environMap=owned_map(__map("Mgetenvironment"),owner);
    if(_environMap){
        char* _equalpos;
        char** _environ=environ;
        if(_environ)
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

Mvalue* Mgetenv(Mvalue* _systemVariableValue){Mallocationowner owner=getOwner(__LINE__);
    if(_systemVariableValue&&_systemVariableValue->type==VT_TEXT){
        char* systemVariable=_systemVariableValue->value._text->_c;
        if(systemVariable)
            return _getValueOfText(_getSingleQuotedText(getenv(systemVariable)));
    }
    return NULL;
}
Mvalue* Msetenv(Mvalue* _systemVariableValue,Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    if(_systemVariableValue&&_systemVariableValue->type==VT_TEXT){
        char* systemVariable=_systemVariableValue->value._text->_c;
        if(systemVariable){
            if(_value&&_value->type==VT_TEXT){
                char* value=_value->value._text->_c;
                if(value){
                    // NOTE if value is NULL setenv will definitely fail
                    if(setenv(systemVariable,value,1)!=0)
                        output("%sFailed to set system variable '%s'.\n",M_ERROR_PREFIX,systemVariable);
                }else
                    outputError("Can't remove the system variable this way: use unsetenv() instead");
            }else
                outputError("System variable value not a text");
            // let's return the current value
            return _getValueOfText(_getSingleQuotedText(getenv(systemVariable)));
        }
    }
    return NULL;
}
Mvalue* Mclearenv(){Mallocationowner owner=getOwner(__LINE__);
    environ=NULL;
    return _getIntegerValue(environ?M_FALSE:M_TRUE);
}
Mvalue* Munsetenv(Mvalue* _systemVariableValue){Mallocationowner owner=getOwner(__LINE__);
    long long result=M_LL_INVALID;
    if(_systemVariableValue&&_systemVariableValue->type==VT_TEXT){
        char* systemVariable=_systemVariableValue->value._text->_c;
        if(systemVariable){
            if(unsetenv(systemVariable)!=0){
                result=M_FALSE;
                output("Failed to unset system variable '%s'.\n",M_ERROR_PREFIX,systemVariable);
            }else
                result=M_TRUE;
        }
    }
    return _getIntegerValue(result);
}
Mvalue* Mputenv(Mvalue* _systemVariableValue,Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
    // NOTE to call putenv I need to construct name=value text
    if(_systemVariableValue&&_systemVariableValue->type==VT_TEXT){
        char* systemVariable=_systemVariableValue->value._text->_c;
        if(systemVariable){
            if(_value){
                if(_value->type==VT_TEXT){
                    Mstring* _propertyText=owned_string(_getString(systemVariable),owner);
                    if(_propertyText){
                        if(string_append_char(_propertyText,'=')&&string_append(_propertyText,_value->value._text->_c)){
                           if(putenv(string(_propertyText))!=0)
                                output("%sFailed to set system variable '%s' to '%s'.\n",M_ERROR_PREFIX,systemVariable,_value->value._text->_c);
                        }else outputError("Failed to initialize the new system variable value");
                    }else
                        outputError("Failed to create the new system variable value");
                }else 
                    output("%sValue of system variable not of type text.\n",M_ERROR_PREFIX,systemVariable);
            }else // intending to remove it
            if(unsetenv(systemVariable)!=0)
                output("%sFailed to remove system variable '%s'.\n",M_ERROR_PREFIX,systemVariable);
            return _getValueOfText(_getSingleQuotedText(getenv(systemVariable)));
        }
    }
    return NULL;
}
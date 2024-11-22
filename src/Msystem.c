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

/**
 * @brief returns the system environmant variables in a map wrapper
 * 
 * @return Mvalue* the system environment variables in a map wrapper
 */
Mvalue* Msystemvariables(){Mallocationowner owner=getOwner(__LINE__);
	// process char** environ
	Mmap* _environMap=owned_map(__map("Mgetenvironment"),owner);
	if(_environMap!=NULL){
		char* _equalpos;
		char** _environ=environ;
		if(_environ!=NULL)
		while(*_environ!=NULL){
			_equalpos=strchr(*_environ,'=');
			if(_equalpos!=NULL){
				// part behind _equal pos is the text value
				Mstring* _environText=owned_string(_getString("'"),owner);
				if(_environText!=NULL){
						if(string_append(_environText,_equalpos+1)){
							Mvalue* _environValue=_getTextValue(string(_environText));
							*_equalpos='\0'; // dangerous!!!!
							if(appendedToMap(_environMap,owner,*_environ,_environValue)!=M_TRUE)
									outputMessage(M_ERROR_PREFIX,"Failed to store system variable '%s'.",*_environ);
							*_equalpos='='; // restored
						}else
							outputError("Failed to remember the value of a system environment variable");
						FREE_STRING(_environText,owner);
					}else
						outputError("Failed to store the system environment variable!");
				}
				_environ++;
		}
		return _getValueOfMap(disowned_map(_environMap,owner));
	}
	return NULL;
}

/**
 * @brief returns an environment value with name wrapped in \p _systemVariableValue
 * 
 * @param _systemVariableValue the M value wrapping the name of the environment variable
 * @return Mvalue* the value of environment variable \p _systemVariableValue
 */
Mvalue* Mgetenv(Mvalue* _systemVariableValue){Mallocationowner owner=getOwner(__LINE__);
	if(_systemVariableValue!=NULL&&_systemVariableValue->type==VT_TEXT){
		char* systemVariable=_systemVariableValue->value._text->_c;
		if(systemVariable!=NULL)
			return _getValueOfText(_getSingleQuotedText(getenv(systemVariable)));
	}
	return NULL;
}
/**
 * @brief returns the current value of the environment variable with name wrapped in \p _systemVariableValue set to the new value wrapped in \p _value
 * 
 * @param _systemVariableValue 
 * @param _value 
 * @return Mvalue* the current value of the environment variable with name wrapped in \p _systemVariableValue
 */
Mvalue* Msetenv(Mvalue* _systemVariableValue,Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(_systemVariableValue!=NULL&&_systemVariableValue->type==VT_TEXT){
		char* systemVariable=_systemVariableValue->value._text->_c;
		if(systemVariable!=NULL){
			if(_value!=NULL&&_value->type==VT_TEXT){
				char* value=_value->value._text->_c;
				if(value!=NULL){
					// NOTE if value is NULL setenv will definitely fail
					if(setenv(systemVariable,value,1)!=0)
						outputMessage(M_ERROR_PREFIX,"Failed to set system variable '%s'.",systemVariable);
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
/**
 * @brief returns M_TRUE if global variable environ equals NULL (after setting it to NULL)
 * 
 * @return Mvalue* M_TRUE when global variable environ is NULL, M_FALSE otherwise
 */
Mvalue* Mclearenv(){Mallocationowner owner=getOwner(__LINE__);
	environ=NULL;
	return _getIntegerValue(environ?M_FALSE:M_TRUE);
}
/**
 * @brief unsets (removes the value of) system variable with name wrapped in \p _systemVariableValue
 * 
 * @param _systemVariableValue 
 * @return Mvalue* M_TRUE when removed successfully, M_FALSE otherwise, M_LL_INVALID if the name does not denote a system variable
 */
Mvalue* Munsetenv(Mvalue* _systemVariableValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(_systemVariableValue!=NULL&&_systemVariableValue->type==VT_TEXT){
		char* systemVariable=_systemVariableValue->value._text->_c;
		if(systemVariable!=NULL){
			if(unsetenv(systemVariable)!=0){
				result=M_FALSE;
				outputMessage(M_ERROR_PREFIX,"Failed to unset system variable '%s'.",systemVariable);
			}else
				result=M_TRUE;
		}
	}
	return _getIntegerValue(result);
}
/**
 * @brief adds a new system variable with name wrapped in \p _systemVariableValue initialized to the text wrapped in \p _value
 * 
 * @param _systemVariableValue 
 * @param _value 
 * @return Mvalue* the new value of the system variable \p _systemVariableValue, or NULL on failure
 */
Mvalue* Mputenv(Mvalue* _systemVariableValue,Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	// NOTE to call putenv I need to construct name=value text
	if(_systemVariableValue&&_systemVariableValue->type==VT_TEXT){
		char* systemVariable=_systemVariableValue->value._text->_c;
		if(systemVariable!=NULL){
			if(_value!=NULL){
				if(_value->type==VT_TEXT){
					Mstring* _propertyText=owned_string(_getString(systemVariable),owner);
					if(_propertyText!=NULL){
						if(string_append_char(_propertyText,'=')&&string_append(_propertyText,_value->value._text->_c)){
							if(putenv(string(_propertyText))!=0)
								outputMessage(M_ERROR_PREFIX,"Failed to set system variable '%s' to '%s'.",systemVariable,_value->value._text->_c);
						}else 
							outputError("Failed to initialize the new system variable value");
					}else
						outputError("Failed to create the new system variable value");
				}else 
					outputMessage(M_ERROR_PREFIX,"Value of system variable not of type text.",systemVariable);
			}else // intending to remove it
			if(unsetenv(systemVariable)!=0)
				outputMessage(M_ERROR_PREFIX,"Failed to remove system variable '%s'.",systemVariable);
			return _getValueOfText(_getSingleQuotedText(getenv(systemVariable)));
		}
	}
	return NULL;
}
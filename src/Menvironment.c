/**
 * MDH@24JUN2019: everything that deals with Menvironments
 */
#include <limits.h>
#include <math.h>

#include "Malloc.h"
#include "Mstring.h"
#include "Msettings.h"
#include "Moutput.h"
// TODO find a way NOT to have to include Msession here (now for using outputLine!!)
#include "Msession.h"

#include "Menvironment.h"

// externally (in M.c) defined constants
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE;
extern const char* const DEFINEUSERFUNCTION_NAME; // the name of the define user function function
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* const ERROR_PREFIX;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!

void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement){
    if(_expressionlistelement){
        free_expressionlistelement(_expressionlistelement->_next);
        free_value(_expressionlistelement->_value);
        free(_expressionlistelement);
    }
}/* VALIDATED */
void free_expressionlist(Mexpressionlist* _expressionlist){
    if(_expressionlist){
        free_expressionlistelement(_expressionlist->_next);
        free_value(_expressionlist->_value);
        free(_expressionlist);
    }
}/* VALIDATED */

// NOTE typically you're not supposed to free internal functions safe M function definitions 
// TODO if a function map is freed, we shouldn't free internal functions BUT those are only present in the main environment which is never released!!
bool free_function(Mfunction* _function){
    if(_function){
        /////// MDH@10JUL2019: moved over to the map element containing the function! free_string(_function->_name);
        free_map(_function->_parameterMap);
        if(_function->type==FT_USER)free_userfunction(_function->functionunion._userfunction);
        free(_function);
        return true;
    }
    return false;
}/* VALIDATED */
bool free_functionmapelement(Mfunctionmapelement* _functionmapelement){
    if(_functionmapelement){
        if(free_functionmapelement(_functionmapelement->_next))_functionmapelement->_next=NULL;
        if(free_function(_functionmapelement->_function)){
            free_string(_functionmapelement->_name);
            free(_functionmapelement);
            return true;
        }
    }
    return false;
}// VALIDATED
void free_functionmap(Mfunctionmap* _functionmap){
    if(_functionmap){
        free_functionmapelement(_functionmap->_first);
        free(_functionmap);
    }
}// VALIDATED
// MDH@20JUL2019: might never get called, wel perhaps on internal functions when it goes out of scope???????
void free_userfunction(Muserfunction* _userfunction){
    if(_userfunction){
        ///////////if(_userfunction->_parameterMap)free_map(_userfunction->_parameterMap);
        // NOTE do NOT call free_value() on the body token value, instead NULL it so the reference count of the value is decremented!!!!
        free_list(_userfunction->_bodyCommandList);
        // replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
        free(_userfunction);
    }
}/* VALIDATED */
// END RELEASERS

// Menvironment stuff
void free_environment(Menvironment* _environment){
    if(_environment){
        if(_environment->_name){free(_environment->_name);_environment->_name=NULL;}
        _environment->_execution=NULL;
        free_map(_environment->_variableMap);
        /* MDH@10JUL2019: only Menvironment has a function map!!   
           MDH@20JUL2019: NO user functions may also contain a function map, which is referenced in a user function execution environment
                          and indeed being a referenced they should not be freed (otherwise we would loose these nested functions on
                          freeing the execution environment)
        if(_environment->_functionMap)free_functionmap(_environment->_functionMap);
        */
        FREE(_environment,'E');
    }
}/* VALIDATED */
Menvironment* __environment(){
    Menvironment* _environment=CALLOC(1,sizeof(Menvironment),'E');
    if(!_environment)return NULL;
    _environment->_variableMap=CALLOC(1,sizeof(Mmap),'M'); // ascertain that the environment contains a variable map
    if(!_environment->_variableMap){free_environment(_environment);_environment=NULL;}
    return _environment;
}/* VALIDATED */
// keep track of the current execution environment
static Menvironment* _executionEnvironment=NULL;
void outputEnvironmentName(){
    Mstring* _environmentName=_getEnvironmentName();
    output("Current execution environment: '%s'.\n",string(_environmentName));
    free_string(_environmentName);
}
bool pushExecutionEnvironment(Menvironment* _environment){
    if(!_environment)return false;
    if(!_environment->_parent)_environment->_parent=_executionEnvironment; // if without a parent give it the current one
    _environment->_execution=_executionEnvironment; // remember to what execution environment to pop back to
    _executionEnvironment=_environment;
    if(amVerbose())outputEnvironmentName();
    return true;
}/* VALIDATED */
void popExecutionEnvironment(){
    if(!_executionEnvironment){outputLine("BUG: No environment left to pop!");return;} // nothing to pop
    // NOTE only execution environments that have a parent can be popped!!!
    Menvironment* _previousExecutionEnvironment=_executionEnvironment->_execution;
    if(!_previousExecutionEnvironment){outputLine("BUG: Can't pop top-most environment!");return;}
    free_environment(_executionEnvironment); // TODO I guess we won't be needing this execution environment any more????
    _executionEnvironment=_previousExecutionEnvironment;
    if(amVerbose())outputEnvironmentName();
}/* VALIDATED */
Menvironment* getEnvironment(){return _executionEnvironment;}/* VALIDATED */
Mstring* _getEnvironmentName(){
    Mstring* _environmentName=__string();
    if(_environmentName){
        Mstring* p=_environmentName;
        Menvironment* _environment=_executionEnvironment;
        while(p&&_environment){
            if(string_length(p)>0)p=string_insert_char(p,0,'.');
            ////////output("Prepending '%s'.\n",_environment->_name);
            p=string_prepend(p,_environment->_name);
            _environment=_environment->_parent;
        }
        if(!p){free_string(_environmentName);_environmentName=NULL;}
    }
    return _environmentName;
}
Mtoken* getEnvironmentExpressionToken(){
    // MDH@22JUL2019: let's allow breaking here
    ////////if(kbhit())return NULL;
    return(_executionEnvironment?_executionEnvironment->expressionToken:NULL);
}/* VALIDATED */
Mtoken* nextEnvironmentExpressionToken(){
    if(!_executionEnvironment)return NULL;
    if(_executionEnvironment->expressionToken)_executionEnvironment->expressionToken=_executionEnvironment->expressionToken->next;
    return _executionEnvironment->expressionToken;
}/* VALIDATED */

// read access to the elements defined in an environment
uint32_t getNumberOfVariables(const Menvironment* const _environment){
    if(!_environment||!_environment->_variableMap)return 0;
    // MDH@20JUL2019: now returning the sum of the variables in the parent plus those in the environment itself!!
    return getNumberOfVariables(_environment->_parent)+_environment->_variableMap->numberOfElements;
}/* VALIDATED */

// the names of the variables may be requested
Mstring* _getVariableNames(const Menvironment* const _environment,const char* const sep){
    Mstring* _variableNames=NULL;
    if(_environment&&sep){
        _variableNames=__string();
        if(_variableNames){
            Mstring* p=_variableNames;
            // first append the names of the variables in the parent
            if(_environment->_parent){
                Mstring* _parentVariableNames=_getVariableNames(_environment->_parent,sep); // free asap
                if(_parentVariableNames){
                    p=string_append(p,string(_parentVariableNames));
                    free_string(_parentVariableNames); // we can do this because string_append copies the characters
                }
            }
            // we'll be appending the names of the variables in the environment itself
            if(_environment->_variableMap){
                Mmapelement* _variableMapelement=_environment->_variableMap->_first;
                while(p&&_variableMapelement){
                    if(_variableMapelement->_variable){
                        if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
                        p=string_append(p,_variableMapelement->_variable->_name);
                    }
                    _variableMapelement=_variableMapelement->_next;
                }
            }
            if(!p){free_string(_variableNames);_variableNames=NULL;}
        }
    }
    return _variableNames;
}/* VALIDATED */

// MDH@08AUG2019: when _environment is NULL, we only check the current execution environment (this makes sense because with no environment presented, we only have the current execution environment to check)
Mvariable* getVariable(Menvironment const * const _environment,char const * const name, bool verbose){
    if(!name){outputError("No variable name specified");return NULL;}
    if(verbose)output("Looking for variable '%s'.\n",name);
    // input valid
    Mmap* variableMap=(_environment?_environment->_variableMap:(_executionEnvironment?_executionEnvironment->_variableMap:NULL));
    if(!variableMap){output("%sNo variables in environment to find '%s' in.\n",ERROR_PREFIX,name);return NULL;}
    ///////////if(amVerbose())output("Looking for variable '%s'.\n",name);
    Mmapelement* _variableMapelement=variableMap->_first;
    // as long as variable is defined, and the variable's name is not equal to the given name, continue
    while(_variableMapelement&&(!_variableMapelement->_variable||strcmp(_variableMapelement->_variable->_name,name)))_variableMapelement=_variableMapelement->_next;
    // MDH@20JUL2019: if found return
    if(_variableMapelement){
        if(verbose)output("Variable '%s' found in environment '%s'.\n",name,(_environment?_environment:_executionEnvironment)->_name);
        return _variableMapelement->_variable;
    }
    // if there's an environment and it has a parent check that, otherwise (e.g. in a closure) no global variables available!!!
    return (_environment&&_environment->_parent?getVariable(_environment->_parent,name,verbose):NULL);
}/* VALIDATED */
bool containsVariable(Menvironment const * const _environment,char const * const name){return(getVariable(_environment,name,false)!=NULL);}/* VALIDATED */

// MDH@24OCT2019: when representing variable values the value might match the value of a constant in which case we use the name of that constant variable instead (which is like a symbol)
//                obviously the type of the value should match as well
bool areValuesEqual(Mvalue const * const value1,Mvalue const * const value2){
    if(!value1&&!value2)return false; // if both NULL not considered to be the same
    if(value1==value2)return true; // the same value pointed to
    if(!value1||!value2)return false; // if either is NULL, not the same of course
    // ASSERT both not NULL
    if(value1->type==value2->type) // if the types are different definitely not the same
    switch(value1->type){
        case VT_INTEGER:return(value1->value._integer->ll==value2->value._integer->ll);
        case VT_FLOAT:return(areFloatsEqual(value1->value._float,value2->value._float)); // MDH@25OCT2019: replacing ldEqual() with a call to areFloatsEqual()
        case VT_BIGINTEGER:return(mp_cmp(value1->value._biginteger,value2->value._biginteger)==MP_EQ);
        case VT_TEXT:return(value1->value._text->presuffix==value2->value._text->presuffix&&strcmp(value1->value._text->_c,value2->value._text->_c)==0);
        case VT_TOKEN:return string_equal(value1->value._token->text,value2->value._token->text);
        case VT_LIST:case VT_MAP:break;
        case VT_DECIMAL:case VT_RATIONAL:break;
        case VT_UNDEFINED:return true; // there's only ONE undefined value around??????
        case VT_REFERENCE: // TODO this might be hard
            break;
    }
    return false;
}
char* getConstantWithValue(Menvironment const * const environment,char * name,Mvalue* value){
    if(!value)return NULL; // forget about NULL
    // input valid
    Mmap* variableMap=(environment?environment->_variableMap:(_executionEnvironment?_executionEnvironment->_variableMap:NULL));
    if(!variableMap){output("%sNo variables in environment to find '%s' in.\n",ERROR_PREFIX,name);return NULL;}
    ///////////if(amVerbose())output("Looking for variable '%s'.\n",name);
    Mmapelement* _variableMapelement=variableMap->_first;
    // as long as variable is defined, and the variable's name is not equal to the given name, continue
    Mvariable* variable;
    while(_variableMapelement){
        variable=_variableMapelement->_variable;
        if(variable) // defined
            if(strcmp(variable->_name,name)) // not the same as name
                if(variable->immutable) // a constant
                    if(areValuesEqual(variable->_value,value))
                        break;
        _variableMapelement=_variableMapelement->_next;
    }
    // MDH@20JUL2019: if found return
    if(_variableMapelement){
        if(amVerbose()){output("Variable '%s' found in environment '%s'",name,(environment?environment:_executionEnvironment)->_name);outputValue(" with value '",value,"'.\n");}
        return _variableMapelement->_variable->_name;
    }
    // if there's an environment and it has a parent check that, otherwise (e.g. in a closure) no global variables available!!!
    return (environment&&environment->_parent?getConstantWithValue(environment->_parent,name,value):NULL);
}
Mstring* _getVariableMapText(Menvironment const * const environment,bool showcurlybraces,bool showquotes,bool showmissings){
    Mmap* map=(environment?environment->_variableMap:_executionEnvironment->_variableMap);
	Mstring* result=(map?__string():NULL);
    if(result){
	    Mstring* p=result;
        if(amDebugging())p=string_append_char(p,'m');
        if(showcurlybraces)p=string_append_char(p,'{');
		//////output("%s",string(p));
		Mmapelement* _mapelement=map->_first;
		while(p&&_mapelement){
			//////output("%s","start");
			Mvariable* _mapVariable=_mapelement->_variable;
			if(_mapVariable){
                // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
                if(showquotes)p=string_append_char(p,'\'');
                p=string_append(p,_mapVariable->_name);
                if(showquotes)p=string_append_char(p,'\'');
                if(showmissings||isValueUndefined(_mapVariable->_value)!=M_TRUE){
                    /////output("%s",string(p));
                    p=string_append_char(p,'='); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
                    // MDH@24OCT2019: here we deviate from _getMapText() (see Mvalue.h/c) in that we try to find a constant with the same value (like PI or E or NULL)
                    char* constantWithValue=getConstantWithValue(environment,_mapVariable->_name,_mapVariable->_value);
                    if(!constantWithValue){ // not a 'symbolic' value
                        /////output("%s",string(p));
                        Mstring* _mapelementValueText=_getValueText(_mapVariable->_value,false); // free asap
                        /////output("Map element: %s",string(p));
                        // TODO technically NULL is also a value, so shouldn't be use the undefined value text????
                        if(_mapelementValueText){
                            p=string_append(p,string(_mapelementValueText)); // append 
                            free_string(_mapelementValueText); // release AFTER copying over
                        }
                    }else // a 'symbolic' value
                        p=string_append(p,constantWithValue);
                }
            }
			_mapelement=_mapelement->_next;
			if(_mapelement)p=string_append(p,", "); // only when there's a next map element to process
			////output("%s","next");
		}
		//////output("%s(%d)",string(p),string_length(p));
		if(showcurlybraces)p=string_append_char(p,'}');
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){free_string(result);result=NULL;}
	}
	return result;
}/* VALIDATED */
// MDH@24OCT2019 END

// use Mexists to determine if a variable exists passed in as text, we might decide to return the name of the environment it exists in
Mvalue* Mexists(Mvalue* _value){
    if(_value&&_value->type==VT_TEXT){
        return _getIntegerValue(getVariable(getEnvironment(),_value->value._text->_c,false)?1:0);
    }
    return NULL; // input invalid
}

// MDH@23SEP2019: if we want to show the user how to complete the name of a variable, we can return those characters that may follow \p name to be part of a variable
//                NOTE if name is already the name of a variable we do not consider that variable
/**
 * \brief returns pointer to first non-matching character different in \p s1 and \p s2
 */
size_t getNumberOfMatchingCharacters(char const * s1,char const * s2){
    // testing *s1 and *s2 as well because we want to stop when we bump into the '\0' character and NOT counting that character
    size_t result=0;if(s1&&s2){while(*s1&&*s2&&*s1==*s2){result++;s1++;s2++;}}return result;
}
// MDH@24SEP2019: because I decided to return either a completion text, or all characters available to obtain an existing variable/function, I return an Mstring* not a char* anymore with the first character either 1 or 2
// MDH@04NOV2019: with references it is possible that the length of \p name is 0, so I change l>0 into l>=0 (forcing l to -1 when name is NULL)
Mstring* _getCompletion(char const * const name,bool functionidentifiersaswell){
    Mstring* _completion=__string(); // this result will start with '\0' indicating that there is no completion or characters from available variables/functions
    if(_completion){
        size_t completionlength=0; // the number of characters to be copied of completion (which will point to the first character in the completion string)
        int l=(name?strlen(name):-1);
        if(l>=0){
            unsigned char completiontype=1; // the completion type (either 1 or 2)
            if(string_append_char(_completion,completiontype)){ // completion type appended!!!
                // check all active environments
                char* completion=NULL; // the current completion string
                Menvironment* _environment=_executionEnvironment;
                while(_environment){
                    // check variables
                    Mmap* variableMap=_environment->_variableMap;
                    if(variableMap){
                        size_t numberOfMatchingCharacters,numberOfMatchingCompletionCharacters,vnl;
                        char *variablename;
                        // as long as variable is defined, and the variable's name is not equal to the given name, continue
                        // NOTE all variables that match should have the same characters behind the name part in order to be considered a valid completion
                        Mmapelement* variableMapelement=variableMap->_first;
                        while(variableMapelement){
                            if(variableMapelement->_variable){
                                variablename=variableMapelement->_variable->_name;
                                vnl=(variablename?strlen(variablename):0);
                                if(vnl>l){ // the variable name is larger then name is
                                    numberOfMatchingCharacters=getNumberOfMatchingCharacters(name,variablename);
                                    // wait a minute, we cannot have more than l matching characters
                                    if(numberOfMatchingCharacters==l){ // all characters in name match (at the beginning)
                                        if(completiontype==1){
                                            if(completion){
                                                numberOfMatchingCompletionCharacters=getNumberOfMatchingCharacters(variablename+l,completion);
                                                if(numberOfMatchingCompletionCharacters<completionlength)completionlength=numberOfMatchingCompletionCharacters;
                                                if(completionlength==0){ // too bad
                                                    // we should return the initial characters available
                                                    // the first character in the completion we had so far is appended to _completion
                                                    // the first character in the functionname as well
                                                    // if either fails we set the completion type to 0 otherwise to 2
                                                    if(!string_append_char(_completion,completion[0])||!string_append_char(_completion,*(variablename+l)))completiontype=0;else completiontype=2;
                                                    completion=NULL; // don't need completion anymore
                                                    // MDH@04NOV2019 don't think we should break here!!!: break;
                                                }
                                            }else{
                                                completion=variablename+l; // point to the first character after name
                                                completionlength=vnl-l; // use all following characters
                                            }
                                        }else{ // completiontype==2
                                            // don't add twice!!!
                                            if(string_find(_completion,(*(variablename+l)))<0)if(!string_append_char(_completion,*(variablename+l)))completiontype=0;               
                                        }
                                        if(completiontype==0)break; // something went wrong
                                    }
                                }
                            }
                            variableMapelement=variableMapelement->_next;
                        }
                        if(completiontype==0)break; // OOPS apparently contradictory continuations
                    }
                    if(functionidentifiersaswell){
                        // check functions
                        Mfunctionmap* functionMap=_environment->_functionMap;
                        if(functionMap){
                            size_t numberOfMatchingCharacters,numberOfMatchingCompletionCharacters,fnl;
                            char *functionname;
                            // as long as variable is defined, and the variable's name is not equal to the given name, continue
                            // NOTE all variables that match should have the same characters behind the name part in order to be considered a valid completion
                            Mfunctionmapelement* functionMapelement=functionMap->_first;
                            while(functionMapelement){
                                functionname=string(functionMapelement->_name); // TODO why is the function name an Mstring and not simply char*
                                fnl=(functionname?strlen(functionname):0);
                                if(fnl>l){ // the variable name is larger then name is
                                    numberOfMatchingCharacters=getNumberOfMatchingCharacters(name,functionname);
                                    // wait a minute, we cannot have more than l matching characters
                                    if(numberOfMatchingCharacters==l){ // all characters in name match (at the beginning)
                                        if(completiontype==1){
                                            if(completion){
                                                numberOfMatchingCompletionCharacters=getNumberOfMatchingCharacters(functionname+l,completion);
                                                if(numberOfMatchingCompletionCharacters<completionlength)completionlength=numberOfMatchingCompletionCharacters;
                                                if(completionlength==0){ // too bad: no matching characters with the current completion, meaning that the first continuation characters do not match, so that no continuation can be returned
                                                    // we should return the initial characters available
                                                    // the first character in the completion we had so far is appended to _completion
                                                    // the first character in the functionname as well
                                                    // if either fails we set the completion type to 0 otherwise to 2
                                                    if(!string_append_char(_completion,completion[0])||!string_append_char(_completion,*(functionname+l)))completiontype=0;else completiontype=2;
                                                    completion=NULL; // don't need completion anymore
                                                    // MDH@04NOV2019 don't think we should break here!!!: break;
                                                }
                                            }else{
                                                completion=functionname+l; // point to the first character after name
                                                completionlength=fnl-l; // use all following characters
                                            }
                                        }else{ // completiontype==2
                                            // don't add twice!!!
                                            if(string_find(_completion,(*(functionname+l)))<0)if(!string_append_char(_completion,*(functionname+l)))completiontype=0;               
                                        }
                                        if(completiontype==0)break; // something went wrong
                                    }
                                }
                                functionMapelement=functionMapelement->_next;
                            }
                            if(completiontype==0)break; // OOPS apparently some error
                        }
                    }
                    // and check the parent as well
                    _environment=_environment->_parent;
                }
                // either append completion (when the completion type equals 1), or replace the completion type
                if(completiontype==1)if(completion&&!string_append_chars(_completion,completion,completionlength))completiontype=0;
                // we set the completion type if not equal to 1
                // NOTE if completiontype happens to be 0, and to make this work, I've adapted string_setchar() to adapt its length to 0 when this happens (which makes perfect sense to do so)
                if(completiontype!=1)string_setchar(_completion,completiontype,0);
            }
        }
    }
    return _completion;
    /*
    return(nocompletion?NULL:strndup(completion,completionlength));
    if(!nocompletion)string_append()
    }
    */
    // I like strndup!
}

// write access
// helper function to create a new variable with a given name and of a given type

// addVariable returns the value map element that was created (if successful)
// MDH@09AUG2019: we allow checking the current environment only when _environment is NULL
bool addVariable(Menvironment* const _environment,const char* const name,Mvaluetype valuetype,bool immutable){
    Mvariable* _variable=NULL;
    if(name){ // input valid
        _variable=getVariable(_environment,name,false);
        if(!_variable){ // non-existing...
            if(amVerbose())output("Variable '%s' to be created.\n",name);
            _variable=_getVariable(name,valuetype,immutable); // creates the variable, free when not bound
            if(amVerbose())output("Variable '%s' created.\n",name);
            if(_variable){
                // get a reference to the environment to which variable map we should be appending...
                Menvironment* environment=(_environment?_environment:_executionEnvironment);
                if(environment){
                    if(amVerbose())output("Will attempt to add variable '%s' to environment '%s'.\n",name,environment->_name);
                    Mmapelement* _variableMapelement=(Mmapelement*)MALLOC(sizeof(Mmapelement),'V');
                    if(_variableMapelement){
                        // store the references
                        _variableMapelement->_next=NULL;
                        _variableMapelement->_variable=_variable;
                        Mmapelement* _lastVariableMapelement=environment->_variableMap->_last;
                        if(_lastVariableMapelement!=NULL)_lastVariableMapelement->_next=_variableMapelement;else environment->_variableMap->_first=_variableMapelement;
                        environment->_variableMap->_last=_variableMapelement;
                        environment->_variableMap->numberOfElements++;
                        if(amVerbose())output("Variable '%s' added to environment '%s'.\n",name,environment->_name);
                        return true;
                    }
                    outputErrorAndText("Failed to create a new map element for variable ",name);
                }else
                    outputErrorAndText("No environment to add the newly created variable to",name);
                // ASSERT failed to link the variable to the variable map!!
                free_variable(_variable,true); // MDH@02NOV2019: no value yet assigned so we can pass in the weak flag
                outputErrorAndText("Failed to link variable ",name);
            }else
                outputErrorAndText("Failed to create variable ",name);
        }else{
            output("WARNING: Won't add existing variable '%s'.\n",name);
            return true;
        }
    }else
        outputError("No variable name specified");
    return false;
}/* VALIDATED */

bool setValue(const Menvironment* const _environment,const char* const name,const Mvalue* const _value){
    // NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
    if(!name){outputError("Cannot set the value: no variable name");return false;}
    Mvariable* variable=getVariable(_environment,name,amVerbose());
    if(variable){
        if(!variable->_value||!variable->immutable){
            if(amVerbose())output("Variable '%s' to set.\n",variable->_name);
            // _value needs to be of the right type
            // MDH@03NOV2019: unless it's null (i.e. the type of _value->type is VT_UNDEFINED)
            if(!_value||variable->valuetype==VT_UNDEFINED||variable->valuetype==_value->type||_value->type==VT_UNDEFINED){
                ///////////////if(_variable->_value)_variable->_value->count--; // decrement the reference count on the current value
                assignValue(&variable->_value,_value); // 'assign' the reference (takes care of updating the reference counts)
                if(amVerbose()){
                    Mstring* _valueText=_getValueText(variable->_value,false);
                    if(_valueText){
                        output("Value '%s' with count %zd assigned to variable '%s'.\n",string(_valueText),(variable->_value?variable->_value->count:0),name);
                        free_string(_valueText);
                    }else
                        outputLine("No value text!");
                }
                ///////////////if(_variable->_value)_variable->_value->count++; // increment the reference count
                return true; // releasing the value is my responsibility now...
            }
            output("%sCannot set the value of variable `%s`: the new value is of the wrong type.\n",ERROR_PREFIX,name);
        }else
            output("%sCannot set the value of variable `%s`: it is not mutable!\n",ERROR_PREFIX,name);
    }else
        output("%sCannot set the value of variable `%s`: it is unknown.\n",ERROR_PREFIX,name);
    return false;
}/* VALIDATED */

long long appendToListVariable(const Menvironment* const _environment,const char* const name,const Mvalue* const _value){
    if(!_environment||!name){outputError("No environment or variable name specified");return 0;}
    Mvariable* variable=getVariable(_environment,name,amVerbose());
    if(variable){
        Mvalue* variableValue=variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
        if(variableValue&&variableValue->type==VT_LIST){ // yes a list we can append to
            // we should prevent circular references
            if(variableValue!=_value){
                unsigned long long index=appendedToList(variableValue->value._list,_value,M_LL_INVALID); // NOTE always append to the end of the list with the first available index that's why I'm passing in 0 instead of a positive index value!!
                if(index>0)return index;
                output("%sFailed to append the value to the list stored in variable '%s': the type of the new value (%u) is wrong.\n",ERROR_PREFIX,name,(_value?_value->type:-1));
            }else
                outputError("Circular reference not allowed");
        }else
            output("%sCannot append the value to variable '%s': it does not contain a list!\n",ERROR_PREFIX,name);
    }else
        output("%sCannot set the value of variable '%s': it is unknown.\n",ERROR_PREFIX,name);
    return 0;
}/* VALIDATED */

Mvalue* getValue(const Menvironment* const _environment,const char* const name){
    if(!_environment||!name){outputError("No environment or name specified");return NULL;}
    Mvariable* variable=getVariable(_environment,name,false);
    return(variable?variable->_value:NULL);
}/* VALIDATED */

// FUNCTION STUFF
// the names of the variables may be requested
Mstring* _getFunctionNames(const Menvironment* const _environment,const char* const sep){
    Mstring* _functionNames=NULL;
    if(_environment&&sep){
        _functionNames=__string();
        if(_functionNames){
            Mstring* p=_functionNames;
            // first append the names of the variables in the parent
            if(_environment->_parent){
                Mstring* _parentFunctionNames=_getFunctionNames(_environment->_parent,sep);
                if(_parentFunctionNames){
                    p=string_append(p,string(_parentFunctionNames));
                    free_string(_parentFunctionNames); // we can do this because string_append copies the characters that string() points to!!
                }
            }
            // we'll be appending the names of the variables in the environment itself
            if(p&&_environment->_functionMap){
                Mfunctionmapelement* functionmapelement=_environment->_functionMap->_first;
                while(p&&functionmapelement){
                    if(functionmapelement->_function){
                        if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
                        p=string_append(p,string(functionmapelement->_name)); //////_function->_name));
                    }
                    functionmapelement=functionmapelement->_next;
                }
            }
            if(!p){free_string(_functionNames);_functionNames=NULL;}
        }
    }
    return _functionNames;
}/* VALIDATED */

Mfunction* getFunction(const Menvironment* const _environment,const char* const functionName){
    if(_environment&&functionName&&strlen(functionName)){
        Mfunctionmap* functionmap=_environment->_functionMap;
        if(functionmap){
            Mfunctionmapelement* functionmapelement=functionmap->_first;
            // we need string() on the function name as function name is an Mstring*
            while(functionmapelement){
                // MDH@10JUL2019: _name moved to the map element instead of in the function
                if(functionmapelement->_function&&!strcmp(string(functionmapelement->_name),functionName)){
                    //////////printf("\nFunction '%s' matches '%s'.",string(_functionmapelement->_function->_name),functionName);
                    return functionmapelement->_function;
                }
                functionmapelement=functionmapelement->_next;
            }
        }
        // might exist in the parent environment
        if(_environment->_parent)return getFunction(_environment->_parent,functionName);
    }
    return NULL;
}/* VALIDATED */
/*
Muserfunction* getUserfunction(const Menvironment* const _environment,const char* const userfunctionName){
    if(_environment&&userfunctionName&&strlen(userfunctionName)){
        Mvariable* _functionVariable=getVariable(_environment,userfunctionName,false);
        if(_functionVariable)if(_functionVariable->_value->type==VT_USERFUNCTION)return _functionVariable->_value->value._userfunction;
    }
    return NULL;
}// VALIDATED
*/
Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList){
    Mmap* _functionArgumentMap=NULL;
    if(_function&&_argumentList){
        _functionArgumentMap=mapMadeWeak((Mmap*)CALLOC(1,sizeof(Mmap),'M')); // MDH@02NOV2019: force the map to be weak
        Mmap* functionParameterMap=_function->_parameterMap;
        if(_functionArgumentMap&&functionParameterMap){
            if(amVerbose())outputLine("Matching the function parameters!");
            Mmapelement* functionParameterMapelement=functionParameterMap->_first;
            Mlistelement* argumentListelement=_argumentList->_first;
            while(functionParameterMapelement){
                Mmapelement* _argumentmapelement=(Mmapelement*)CALLOC(1,sizeof(Mmapelement),'m');
                if(!_argumentmapelement)break; // TODO should we return NULL?????
                // BUG FIX I suppose we need _variable to point to something
                _argumentmapelement->_variable=(Mvariable*)CALLOC(1,sizeof(Mvariable),'V');
                if(!_argumentmapelement->_variable){free_mapelement(_argumentmapelement,true);break;}
                // probably can't simply assign??? let's use _strdup then
                _argumentmapelement->_variable->_name=_strdup(functionParameterMapelement->_variable->_name);
                if(!_argumentmapelement->_variable->_name){free_mapelement(_argumentmapelement,true);break;}
                // associate the argument list element value (if available)
                // MDH@02NOV2019: OK, using assignValue() here (after adjusting assignValue to copy maps and lists)
                //                we get a problem with functions like push() and shove() that try to adjust their argument
                //                therefore we replace the call to assignValue() to a simple assignment
                if(argumentListelement){
                    _argumentmapelement->_variable->_value=argumentListelement->_value;
                    // replacing: assignValue(&_argumentmapelement->_variable->_value,argumentListelement->_value);
                    argumentListelement=argumentListelement->_next;
                }else // use the default!!!
                    _argumentmapelement->_variable->_value=functionParameterMapelement->_variable->_value;
                    // replacing: assignValue(&_argumentmapelement->_variable->_value,functionParameterMapelement->_variable->_value);
                // append to _argumentMap
                if(_functionArgumentMap->_last)_functionArgumentMap->_last->_next=_argumentmapelement;else _functionArgumentMap->_first=_argumentmapelement;
                _functionArgumentMap->_last=_argumentmapelement;
                _functionArgumentMap->numberOfElements++;
                functionParameterMapelement=functionParameterMapelement->_next;
            }
        }
        if(amVerbose())outputLine("Argument map created.");
    }
    return _functionArgumentMap;
}/* VALIDATED */

// newFunction renamed to _getFunction(), not to be confused with getFunction()
// _getFunction() will create the function
Mfunction* _getFunction(Menvironment* const _environment,const char* const name){
    Mfunction* _function=NULL;
    if(_environment&&name&&strlen(name)){
        _function=getFunction(_environment,name);
        if(!_function){ // doesn't exist yet
            _function=(Mfunction*)CALLOC(1,sizeof(Mfunction),'F');
            if(_function){
                ///////////_function->type=functionType;
                _function->_definitionEnvironment=_environment; // TODO why would we need this?????
                Mstring* _functionName=__string();
                if(_functionName){
                    Mstring* p=_functionName;
                    p=string_append(p,name);
                    if(p){
                        Mfunctionmap* _functionmap=_environment->_functionMap;
                        if(_functionmap){
                            Mfunctionmapelement* _functionmapelement=(Mfunctionmapelement*)CALLOC(1,sizeof(Mfunctionmapelement),'f');
                            if(_functionmapelement){
                                _functionmapelement->_name=_functionName; // MDH@10JUL2019: moved over to the function map element
                                _functionmapelement->_function=_function; // no worries here
                                Mfunctionmapelement* _lastFunctionmapelement=_functionmap->_last;
                                if(_lastFunctionmapelement){
                                    _lastFunctionmapelement->_next=_functionmapelement;
                                    _functionmap->_last=_functionmapelement;
                                }else
                                    _functionmap->_first=_functionmapelement;
                                _functionmap->_last=_functionmapelement;
                                _functionmap->numberOfFunctions++;
                                ///////_function->_name=_functionName; // success!!!!!
                                if(amVerbose())output("Function '%s' registered as function #%d.\n",name,_functionmap->numberOfFunctions);
                            }else // failure
                                p=NULL;
                        }else
                            p=NULL;
                    }
                    if(!p){free_string(_functionName);_functionName=NULL;} // p==NULL indicates _functionName not bound in _function->_name
                    // try to append it to the functionMap, if we succeed store _functioName in ->_name
                }else
                    output("%sFailed to store function name '%s'.\n",ERROR_PREFIX,name);
                // if we fail to register the name and/or the function with the environment free the function!!
                if(!_functionName){free_function(_function);_function=NULL;}   
            }
            if(!_function)output("%sFailed to create function '%s'.\n",ERROR_PREFIX,name);
        }else
            output("NOTE: Function '%s' already exists.\n",name);
    }
    return _function;
}/* VALIDATED */

// END FUNCTION STUFF

// the internal functions
// helpers for Mtype()
char getValueTypeCharacter(Mvaluetype valuetype,bool immutable){
	return(immutable?IMMUTABLEVALUETYPECHARS[valuetype]:MUTABLEVALUETYPECHARS[valuetype]);
}
bool isValueImmutable(Mvalue* value){
	bool result=false;
	if(value){
		if(value->type==VT_MAP)result=value->value._map->immutable;else
		if(value->type==VT_LIST)result=value->value._list->immutable;
	}
	return result;
}
/**
 * \brief returns the (text representation of) type and immutable flag of \p value
 * \p value the value of which to return the text representing the type and immutable flag
 */
Mvalue* Mtype(Mvalue* _value){
	// every value should have a type text, even if NULL
	// MDH@03NOV2019: actually _value should be the name of a variable because it not we cannot determine whether or not
	//                the variable is mutable, that's why settype() requires the name of the variable (as text)
	char result[3]="' ";
	if(_value){
		if(_value->type==VT_REFERENCE){ // a variabler reference
			Mvariable* referencedVariable=_value->value._reference->variable;
			// NOTE for any reference we return not 'r' but the type of the referenced variable (which we wouldn't have access to otherwise)
			if(referencedVariable)result[1]=getValueTypeCharacter(referencedVariable->valuetype,referencedVariable->immutable);
		}else // a non-reference type
			result[1]=getValueTypeCharacter(_value->type,isValueImmutable(_value));
	}
	/* ewplacing:
	switch(_value->type){
		case VT_UNDEFINED:result[1]='-';break;
		case VT_TOKEN:return _getTextValue("'T",false); // can we find another character for that, so we can use t for text????
		case VT_INTEGER:return _getTextValue("'i",false);
		case VT_BIGINTEGER:return _getTextValue("'b",false);
		case VT_DECIMAL:return _getTextValue("'d",false);
		case VT_RATIONAL:return _getTextValue("'q",false);
		case VT_FLOAT:return _getTextValue("'f",false);
		case VT_TEXT:return _getTextValue("'t",false); // t for text
		case VT_LIST:return _getTextValue("'l",false);
		case VT_MAP:return _getTextValue("'m",false);
		case VT_REFERENCE:return _getTextValue("'r",false); // r now short for reference, as changing real into float
		/////case VT_USERFUNCTION:return _getTextValue("'f",false);
	}
	*/
	return _getTextValue(result,false);
}
/**
 * \brief to set the type and immutable flag of a variable or composite value
 */
Mvalue* Msettype(Mvalue* value,Mvalue* valuetypeValue,Mvalue* immutableValue){
    // check the types first, both should be strings
    Mvariable* variable=NULL;
    bool valuetypeSpecified=(valuetypeValue&&valuetypeValue->type==VT_TEXT);
    // TODO should we force value type to be text???? for now yes
    if(value&&(valuetypeSpecified||immutableValue)){
        switch(value->type){
            case VT_MAP:case VT_LIST:break;
            case VT_REFERENCE:
                {
                    variable=value->value._reference->variable;
                    // it's best NOT to create the variable if it does not yet exist although we could
                    if(!variable){output("%s",ERROR_PREFIX);outputValue("Cannot set the type of non-existing variable '",value,"'.\n");return NULL;}
                    break;
                }
            default:outputError("Can not set the type of values that are scalar or text (representing the name of a variable)");return NULL;
        }
        // set the valuetype
        // no longer allowing uppercase to be used as a shortcut to immutability?????? yes, we can still do that
        long long immutable=M_LL_INVALID; // whether we should change the immutable flag
        Mvaluetype valuetype=VT_UNDEFINED;
        if(valuetypeSpecified){ // a value type defined
            switch(valuetypeValue->value._text->_c[0]){ // use the first character (which will be '\0' if the default value is used!!!)
                case 'U':immutable=M_TRUE;
                case 'u':valuetype=VT_UNDEFINED;break;
                case 'I':immutable=M_TRUE;
                case 'i':valuetype=VT_INTEGER;break;
                case 'D':immutable=M_TRUE;
                case 'd':valuetype=VT_DECIMAL;break;
                case 'Q':case 'R':immutable=M_TRUE;
                case 'q':case 'r':valuetype=VT_RATIONAL;break;
                case 'B':immutable=M_TRUE;
                case 'b':valuetype=VT_BIGINTEGER;break;
                case 'F':immutable=M_TRUE;
                case 'f':valuetype=VT_FLOAT;break;
                case 'T':immutable=M_TRUE;
                case 't':valuetype=VT_TEXT;break;
                case 'L':immutable=M_TRUE;
                case 'l':valuetype=VT_LIST;break;
                case 'M':immutable=M_TRUE;
                case 'm':valuetype=VT_MAP;break;
                default:outputError("Unrecognized value type");return NULL;
            }
        }
        // if a third argument is specified it takes precedence over what the second argument says
        if(immutableValue)immutable=isValueOne(immutableValue); // accepting all values that represent 1 to be considered true
        if(valuetypeSpecified){
            if(variable){
                if(valuetype!=variable->valuetype){ // a change of the value type intended (e.g. from undefined i.e. free to integer, or real or whatever)
                    // you cannot change the type of a variable that its current value is not (unless the value is undefined or the type of the value is undefined)
                    if(variable->_value&&variable->_value->type!=valuetype&&variable->_value->type!=VT_UNDEFINED){
                        output("%sUnable to change the value type of %s to '%c' when its value is of type '%c'.\n",ERROR_PREFIX,variable->_name,MUTABLEVALUETYPECHARS[valuetype],MUTABLEVALUETYPECHARS[variable->_value->type]);
                        return NULL;
                    }
                    variable->valuetype=valuetype; // update the value type
                }
                if(immutable!=M_LL_INVALID)variable->immutable=(immutable==M_TRUE);
            }else
            if(value->type==VT_LIST)value->value._list->valuetype=valuetype;else value->value._map->valuetype=valuetype;
        }
        if(immutable!=M_LL_INVALID){
            if(variable)variable->immutable=(immutable==M_TRUE);else if(value->type==VT_LIST)value->value._list->immutable=(immutable==M_TRUE);else value->value._map->immutable=(immutable==M_TRUE);
        }
    }
    return Mtype(value);
}/* INVALIDATED */

bool completedFunction(Mfunction* const _function,const char* const functionName,NoArgumentFunction noArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_NO_ARGUMENTS;
        _function->functionunion.noArgumentFunction=noArgumentFunction;
        _function->_parameterMap=NULL;
        if(amVerbose())output("Registered no-argument function '%s' completed.\n",functionName);
        return true;
    }
    return false;
}/* VALIDATED */
bool completedValueFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getMap("v");
        if(_function->_parameterMap){
            // no defaults here!!!
            if(amVerbose())output("Registered single value argument function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single value argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedFloatFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        _function->_parameterMap=_getFloatMap("x",_getFloatValue(M_LD_NAN)); // MDH@20JUN2019: now using the invalid real value as default (to indicate a missing value)
        if(_function->_parameterMap){
            if(amVerbose())output("Registered single real argument function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single real argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedIntegerFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getIntegerMap("i",_getIntegerValue(M_LL_INVALID)); // MDH@20JUN2019: now using the invalid value as default (to indicate a missing!!!!)
        if(_function->_parameterMap){
           if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;    
        }
        output("%sFailed to register single integer argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedListFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getListMap("l",_getListValue(VT_UNDEFINED,false));
        if(_function->_parameterMap){
            if(amVerbose())output("Registered list function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single list argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedTokenListFunction(Mfunction* const _function,const char* const functionName,OneArgumentFunction oneArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_ONE_ARGUMENT;
        _function->functionunion.oneArgumentFunction=oneArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getListMap("l",_getListValue(VT_TOKEN,false));
        if(_function->_parameterMap){
            if(amVerbose())output("Registered token list function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register single token list argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedStringStringFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getStringStringMap("variable","type");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register double string argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedFloatFloatFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getFloatFloatMap("base","exponent");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register double real argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedStringMapTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getStringMapTokenMap("name","parameters","body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register string map list argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedValueTextValueFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getStringMapTokenMap("variable name, list or map","value type","immutable");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register a value text value argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedTokenTokenFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getTokenTokenMap("while condition","while body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register two token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedValueValueFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getTokenTokenMap("value to text","format specifier");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register two value argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedListValueFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        _function->_parameterMap=_getTokenTokenMap("list","value");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register list value function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedValueTokenTokenFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getValueTokenTokenMap("if condition","then clause","else clause");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register three token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedThreeIntegersFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getThreeIntegerMap("red","green","blue");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register three integer argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */
bool completedTokenTokenTokenTokenFunction(Mfunction* const _function,const char* const functionName,FourArgumentFunction fourArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_FOUR_ARGUMENTS;
        _function->functionunion.fourArgumentFunction=fourArgumentFunction;
        _function->_parameterMap=_getTokenTokenTokenTokenMap("for initialization","for condition","for increment","for body");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register four token argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* VALIDATED */

// MDH@09JUL2019: a function is defined as a parameter map (with defaults) and a body token
// MDH@10JUL2019: the caller will need to register the function in the function map of the definition environment
//                here we only need to return the wrapped user function
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
    Mvalue* _value=__value();
    if(_value){_value->type=VT_TOKEN;_value->value._userfunction=_userfunction->_bodyTokenValue->value._token;}
    if(!_value)free_userfunction(_userfunction);
    return _value;
}
*/
unsigned long long getNumberOfFunctionCommands(const char* const functionName){
    Mfunction* function=getFunction(getEnvironment(),functionName);
    if(!function||function->type!=FT_USER){if(!function)output("%sFunction '%s' not found.\n",ERROR_PREFIX,functionName);return -1;}
    return (function->functionunion._userfunction->_bodyCommandList?function->functionunion._userfunction->_bodyCommandList->numberOfElements:0);
}
bool registerFunctionCommand(const char* const functionName,Mtoken* command){
    if(!functionName||!command)return false;
    Mfunction* function=getFunction(getEnvironment(),functionName);
    if(function&&function->type==FT_USER){
        Mvalue* _commandValue=_getValueOfToken(command,false);
        if(_commandValue){
            if(!function->functionunion._userfunction->_bodyCommandList)
                function->functionunion._userfunction->_bodyCommandList=CALLOC(1,sizeof(Mlist),'L');
            if(appendedToList(function->functionunion._userfunction->_bodyCommandList,_commandValue,M_LL_INVALID))return true;
            output("%sFailed to add command to list of body of '%s'.\n",ERROR_PREFIX,functionName);
        }else
            output("%sFailed to wrap a command of function '%s'.\n",ERROR_PREFIX,functionName);
    }else
        output("%s'%s' does not represent a user function.\n",ERROR_PREFIX,functionName);
    return false;
}

Mvalue* Mdefinefunction(Mvalue* _nameValue,Mvalue* _parameterMapValue,Mvalue* _bodyTokenValue){
    // the user specifies the body as a text (to prevent evaluation during defining the function)
    // but perhaps it could also be a list of tokens????? i.e. already tokenized (that is not evaluated)
    // of course, tokenizing is a problem later on, but this means that we need to prevent evaluation of the second argument before calling this function on it
    if(_nameValue&&_parameterMapValue){
        if(_nameValue->type==VT_TEXT&&_parameterMapValue->type==VT_MAP&&(!_bodyTokenValue||_bodyTokenValue->type==VT_TOKEN)){
            Muserfunction* _userfunction=(Muserfunction*)CALLOC(1,sizeof(Muserfunction),'U');
            if(_userfunction){
                Mtext* functionName=_nameValue->value._text;
                // user function expects a list of commands, so we have to wrap the single token (if any)
                if(_bodyTokenValue){
                    _userfunction->_bodyCommandList=_getListOfType(VT_TOKEN);
                    if(!_userfunction->_bodyCommandList||appendedToList(_userfunction->_bodyCommandList,_bodyTokenValue,M_LL_INVALID))
                        output("%sFailed to store the inline command as body of function definition of '%s'.\n",ERROR_PREFIX,functionName->_c);
                    // replacing: assignValue(&_userfunction->_bodyTokenValue,_bodyTokenValue);
                }
                //////////Mvalue* _userfunctionValue=_getUserfunctionValue(_userfunction,true); // free asap or bound
                ///////if(_userfunctionValue){
                    // MDH@17JUL2019: the map needs to be stored with the Mfunction
                Mfunction* _function=_getFunction(getEnvironment(),functionName->_c);
                if(_function){
                    _function->_parameterMap=_parameterMapValue->value._map;
                    _function->functionunion._userfunction=_userfunction;
                    // return the result of applying the function to the default parameter map

                    return _getIntegerValue(1);
                }
                ///////////free_value(_userfunctionValue); // freed
                output("%sFailed to create function '%s'.\n",ERROR_PREFIX,functionName);
                ///////}
            }
        }else
            outputError("Invalid user function name, parameter map or body");
    }else{
        if(!_nameValue)outputError("No name defined of function");
        if(!_parameterMapValue)outputError("No (formal) parameter map defined for function");
        ////////if(!_bodyTokenValue)outputError("No body (expression) defined of function");
    }
    return _getIntegerValue(0); // indicating failure...
}/*VALIDATED */

Mvalue* Mreturn(Mvalue* _value){
    // sets the value of the function execution result variable to _value
    // if you call return with NO value, the current value of $ will be used (or the value of the last executed function body command)
    // do NOT replace the value of "$" if _value is NULL (which should indicate a return without argument), this means you cannot undo the result value
    // perhaps with $=NULL though
    if(_value!=NULL&&!setValue(getEnvironment(),"$",_value)){
        outputError("Failed to set the function execution result variable");
        return NULL;
    }
    // setting the exit flag variable will get the function execution aborted
    if(!setValue(getEnvironment(),"!",_getIntegerValue(1))){
        outputError("Failed to set the function execution exit flag variable");
        return NULL;
    }
    return _value; // echo the input value
}/*VALIDATED */

// these internal functions do NOT have a body as M defined functions have...
bool registerInternalFunctions(Menvironment* const _environment){
    // variable functions
    // math functions
    if(!completedFloatFunction(_getFunction(_environment,"cos"),"cos",Mcos))return false;
    if(!completedFloatFunction(_getFunction(_environment,"cordiccos"),"cordiccos",Mcordiccos))return false;
    if(!completedFloatFunction(_getFunction(_environment,"sin"),"sin",Msin))return false;
    if(!completedFloatFunction(_getFunction(_environment,"cordicsin"),"cordicsin",Mcordicsin))return false;
    if(!completedFloatFunction(_getFunction(_environment,"tan"),"tan",Mtan))return false;
    if(!completedFloatFunction(_getFunction(_environment,"cosh"),"cosh",Mcosh))return false;
    if(!completedFloatFunction(_getFunction(_environment,"sinh"),"sinh",Msinh))return false;
    if(!completedFloatFunction(_getFunction(_environment,"tanh"),"tanh",Mtanh))return false;
    if(!completedFloatFunction(_getFunction(_environment,"sqrt"),"sqrt",Msqrt))return false;
    if(!completedFloatFunction(_getFunction(_environment,"log"),"log",Mlog))return false;
    if(!completedFloatFunction(_getFunction(_environment,"log10"),"log10",Mlog10))return false;
    if(!completedFloatFunction(_getFunction(_environment,"floor"),"floor",Mfloor))return false;
    if(!completedFloatFunction(_getFunction(_environment,"trunc"),"trunc",Mtrunc))return false;
    if(!completedFloatFunction(_getFunction(_environment,"round"),"round",Mround))return false;
    if(!completedFloatFunction(_getFunction(_environment,"ceil"),"ceil",Mceil))return false;
    if(!completedFloatFunction(_getFunction(_environment,"exp"),"exp",Mexp))return false;
    if(!completedFloatFunction(_getFunction(_environment,"dexp"),"dexp",Mdexp))return false;
    // MDH@04NOV2019: settype now has 3 arguments the last one being the immutable flag
    if(!completedValueFunction(_getFunction(_environment,"type"),"type",Mtype))return false;
    if(!completedValueTextValueFunction(_getFunction(_environment,"settype"),"settype",Msettype))return false;
    if(!completedFloatFloatFunction(_getFunction(_environment,"pow"),"pow",Mpow))return false;

    if(!completedStringMapTokenFunction(_getFunction(_environment,"function"),DEFINEUSERFUNCTION_NAME,Mdefinefunction))return false;
    if(!completedValueFunction(_getFunction(_environment,"return"),"return",Mreturn))return false;

    if(!completedValueFunction(_getFunction(_environment,"out"),"out",Mout))return false;
    if(!completedValueFunction(_getFunction(_environment,"in"),"in",Min))return false;

    if(!completedValueFunction(_getFunction(_environment,"bc"),"bc",Mbc))return false;
    if(!completedValueFunction(_getFunction(_environment,"tc"),"tc",Mtc))return false;
    if(!completedThreeIntegersFunction(_getFunction(_environment,"brgb"),"brgb",Mbrgb))return false;
    if(!completedThreeIntegersFunction(_getFunction(_environment,"trgb"),"trgb",Mtrgb))return false;
    return true;
}/* VALIDATED */

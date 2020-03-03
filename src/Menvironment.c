/**
 * MDH@24JUN2019: everything that deals with Menvironments
 */
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "Menvironment.h"

// externally (in M.c) defined constants
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const char * const M_HIDDEN_VARIABLE_NAMES[]; // MDH@14NOV2019: the name of the M variables to NOT return when requesting the variable map text!!!
extern const unsigned long long M_NUMBER_OF_HIDDEN_VARIABLES;
// TODO make the following variables start with M_
extern const char* const DEFINEUSERFUNCTION_NAME; // the name of the define user function function
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* const ERROR_PREFIX;
extern const char * const VALUETYPENAMES[];

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
    if(amVerbose())output("Current execution environment: '%s'.\n",string(_environmentName));
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
    if(!_executionEnvironment){outputBug("No environment left to pop!");return;} // nothing to pop
    // NOTE only execution environments that have a parent can be popped!!!
    Menvironment* _previousExecutionEnvironment=_executionEnvironment->_execution;
    if(!_previousExecutionEnvironment){outputBug("Can't pop top-most environment!");return;}
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

// MDH@14NOV2019: what if someone asks for a list of variable names???
//                how about prefixing the name of the variable with the name of the environment it is part of???
Mlist* _getVariableNamesList(Menvironment* environment){
    Mlist* _variableNamesList=NULL;
    if(environment){
        _variableNamesList=_getListOfType(VT_TEXT);
        if(_variableNamesList){
            if(environment->_variableMap){
                if(amVerbose())output("Creating the list of variable names of '%s'.\n",environment->_name);
                Mmapelement* variableMapelement=environment->_variableMap->_first;
                while(variableMapelement){
                    if(variableMapelement->_variable){
                        Mstring* variableNameText=_getString("'");
                        if(variableNameText){
                            if(string_append(variableNameText,variableMapelement->_variable->_name))
                                appendedToList(_variableNamesList,_getTextValue(string(variableNameText),false),M_LL_INVALID);
                            free_string(variableNameText);
                        }
                    }
                    variableMapelement=variableMapelement->_next;
                }
            }
        }
    }
    return _variableNamesList;
}
Mmap* _getVariableNamesMap(Menvironment* environment){
    // how about returning for each environment that is active an attribute in a map?
    // first get the map of the parent
    Mmap* _variableNamesMap=(environment?_getMapOfType(VT_UNDEFINED):NULL);
    if(_variableNamesMap){
        // storing the local variables in a list that we're going to store in an attribute with name ''
        Mvalue* _localVariableNamesListValue=_getValueOfList(_getVariableNamesList(environment),true);
        if(_localVariableNamesListValue&&!appendedToMap(_variableNamesMap,"",_localVariableNamesListValue)){
            /// OOPS, no need to free values!!! free_value(_localVariableNamesListValue); // not bound to the variableNamesMap, so free immediately
            output("%sFailed to register the local variable names of '%s'.\n",ERROR_PREFIX,environment->_name);
        }
        if(environment->_parent){
            // determine the variable names map of the parent environment and wrap it
            Mvalue* _parentVariableNamesMapValue=_getValueOfMap(_getVariableNamesMap(environment->_parent),true);
            // if successfully wrapped append it to the result map but free the value when unsuccesful doing so!!
            if(_parentVariableNamesMapValue&&!appendedToMap(_variableNamesMap,environment->_parent->_name,_parentVariableNamesMapValue)){
                /// OOPS, no need to free values!!! free_value(_parentVariableNamesMapValue);
                output("%sFailed to register the variable names of the parent of '%s'.\n",ERROR_PREFIX,environment->_name);
            }
        }
    }
    return _variableNamesMap;
}
// MDH@25NOV2019: if we ask for the table, we receive a list with first element containing the column names
Mlist* _getTable(Mlist* columnNamesList,size_t numberOfRows,bool freeonfailure){
    if(columnNamesList){
        Mlist* _table=_getListOfType(VT_LIST);
        if(_table){
            _table->weak=true; // MDH@27NOV2019: if we do the following no value copying will occur!!
            // wrap the column names list in a value
            // oops this is going to be a nuisance as the list would be copied wouldn't it????????
            //      NO because _getValueOfList() will simply assign the argument to the _list property, nothing more!!!
            //      
            Mvalue* columnNamesListValue=_getValueOfList(columnNamesList,false);
            if(columnNamesListValue){
                outputValue("Column names values table: '",columnNamesListValue,"'.\n");
                if(appendedToList(_table,columnNamesListValue,M_LL_INVALID)){
                    while(numberOfRows>0){
                        numberOfRows--;
                        Mvalue* _rowValue=_getValueOfList(_getListOfType(VT_UNDEFINED),true);
                        if(!_rowValue)break;
                        if(!appendedToList(_table,_rowValue,M_LL_INVALID))break;
                    }
                    return _table;
                }
            }
        }
        // in essence the values in the list have their counts incremented when being added to it
        // and only by decrementing their counts in free_list() do the elements go free
        if(freeonfailure)free_list(columnNamesList);
    }
    return NULL;
}
// MDH@25NOV2019: the first example of a list which is constructed as a table is the the values() table
void outputTable(Mlist* table){
    if(table){
         if(table->numberOfElements>0&&table->_first){
            // the first list element is supposed to be contain the column names
           if(table->valuetype==VT_LIST){
                // the width of the column names determines the width of the columns with an additional blank in between NO not an extra blank
                Mlistelement* tableListelement=table->_first;
                Mvalue* tablerowValue=tableListelement->_value;
                Mlist* tablerowValueList=(tablerowValue&&tablerowValue->type==VT_LIST?tablerowValue->value._list:NULL);
                if(tablerowValueList){
                    if(tablerowValueList->_first){
                        size_t* columnLengths=calloc(tablerowValueList->numberOfElements,sizeof(size_t));
                        if(columnLengths){
                            // get the value text of all elements in the header list
                            Mlistelement* headerrowListelement=tablerowValueList->_first;
                            size_t columnIndex=0;
                            newline(); // start a new line before outputting the table!!!
                            while(headerrowListelement){
                                if(columnIndex==tablerowValueList->numberOfElements){outputBug("Had to break out of table header loop!");break;}
                                Mstring* _columnNameText=_getValueText(headerrowListelement->_value,true);
                                if(_columnNameText){
                                    columnLengths[columnIndex]=output("%s",string(_columnNameText));
                                    free_string(_columnNameText);
                                }
                                columnIndex++;
                                headerrowListelement=headerrowListelement->_next;
                            }
                            // ready to show the data rows 
                            while(tableListelement->_next){
                                tableListelement=tableListelement->_next;
                                tablerowValue=tableListelement->_value;
                                if(tablerowValue&&tablerowValue->type==VT_LIST){
                                    tablerowValueList=tablerowValue->value._list;
                                    if(tablerowValueList&&tablerowValueList->numberOfElements>0){
                                        newline();
                                        columnIndex=0;
                                        size_t cellLength;
                                        Mlistelement* rowListelement=tablerowValueList->_first;
                                        while(rowListelement){
                                            if(columnIndex==tablerowValueList->numberOfElements){outputBug("Had to break out of table data row loop!");break;}
                                            Mstring* _cellText=_getValueText(rowListelement->_value,true);
                                            cellLength=(_cellText?output("%s",string(_cellText)):0);
                                            while(++cellLength<=columnLengths[columnIndex])outputChar(' ');
                                            free_string(_cellText);
                                            rowListelement=rowListelement->_next;
                                            columnIndex++;
                                        }
                                    }
                                }
                            }
                            free(columnLengths); // essential bro'
                            newline(); // one newline() at the end!!
                        }else 
                            outputError("Failed to prepare for displaying the table header.");
                    }else 
                        outputError("Header table row empty!");
                }else 
                    outputError("Header of table not a list.");
            }else
                outputError("Rows of assumed table not (all) lists.");
        }else
            outputWarning("The table to output is empty.");
    }else
        outputError("No table to output.");
}
const char* HEXCHARS[]={
    "00","01","02","03","04","05","06","07","08","09","0A","0B","0C","0D","0E","0F",
    "10","11","12","13","14","15","16","17","18","19","1A","1B","1C","1D","1E","1F",
    "20","21","22","23","24","25","26","27","28","29","2A","2B","2C","2D","2E","2F",
    "30","31","32","33","34","35","36","37","38","39","3A","3B","3C","3D","3E","3F",
    "40","41","42","43","44","45","46","47","48","49","4A","4B","4C","4D","4E","4F",
    "50","51","52","53","54","55","56","57","58","59","5A","5B","5C","5D","5E","5F",
    "60","61","62","63","64","65","66","67","68","69","6A","6B","6C","6D","6E","6F",
    "70","71","72","73","74","75","76","77","78","79","7A","7B","7C","7D","7E","7F"
    };
Mlist* _getValuesTable(Mvalue* variableNamesMapValue){
    // MDH@25NOV2019: we should get the allocation types and counts asap (otherwise they will change), unless they are passed in
    char* _allocationtypes=_getAllocationTypes();
    size_t* _allocationcounts=_getAllocationCounts();
    Mlist* _valuesTable=NULL;
    // let's create the list containing the column names
    Mlist* _valuesColumnNames=_getListOfType(VT_TEXT);
    if(_valuesColumnNames
        &&appendedToList(_valuesColumnNames,_getTextValue("'Values: ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'TYPE        ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'SIZE        ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'ALLOCATED   ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'FREED       ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'M.ALLOCATED ",false),M_LL_INVALID)>0
        &&appendedToList(_valuesColumnNames,_getTextValue("'M.FREED     ",false),M_LL_INVALID)>0){
        size_t numberOfAllocationTypes=(_allocationtypes?sizeof(_allocationtypes)/sizeof(char):0);
        // get a table with the given values column names and number of rows (which are initialized to empty lists)
        // NOTE tell _getTable() to free the values column names if failing to bind them in a table!!!!
        _valuesTable=_getTable(_valuesColumnNames,numberOfAllocationTypes,true);
        if(_valuesTable){
            if(numberOfAllocationTypes){
                // we start with a general overview (the counts per type)
                // NOTE dividing by sizeof(char) is far fetched
                for(size_t i=0;i<numberOfAllocationTypes;i++){
                    Mstring* _allocationTypeText=_getString("'");
                    if(_allocationTypeText
                            &&string_append_char(_allocationTypeText,_allocationtypes[i])
                            &&string_append(_allocationTypeText,"=0x")
                            &&string_append(_allocationTypeText,HEXCHARS[_allocationtypes[i]]))
                    {
                        Mlist* _valuecountsList=_getListOfType(VT_UNDEFINED); // we're going to store the value counts in a map
                        if(_valuecountsList){
                            Mvalue* _zeroTextValue=_getTextValue("'",false); // will be garbage collected automatically when the reference count is not incremented (as in weak lists)
                            _valuecountsList->weak=true; // TODO should we do this???
                            // start with the table row index below the name of the table!!!
                            if(appendedToList(_valuecountsList,_getIntegerValue(i+1),M_LL_INVALID)
                                &&appendedToList(_valuecountsList,_getTextValue(string(_allocationTypeText),false),M_LL_INVALID)){
                                // now the 5 counts
                                appendedToList(_valuecountsList,(_allocationcounts[i*5]>0?_getIntegerValue(_allocationcounts[i*5]):_zeroTextValue),M_LL_INVALID);
                                appendedToList(_valuecountsList,(_allocationcounts[1+i*5]>0?_getIntegerValue(_allocationcounts[1+i*5]):_zeroTextValue),M_LL_INVALID);
                                appendedToList(_valuecountsList,(_allocationcounts[2+i*5]>0?_getIntegerValue(_allocationcounts[2+i*5]):_zeroTextValue),M_LL_INVALID);
                                appendedToList(_valuecountsList,(_allocationcounts[3+i*5]>0?_getIntegerValue(_allocationcounts[3+i*5]):_zeroTextValue),M_LL_INVALID);
                                appendedToList(_valuecountsList,(_allocationcounts[4+i*5]>0?_getIntegerValue(_allocationcounts[4+i*5]):_zeroTextValue),M_LL_INVALID);
                                if(!appendedToList(_valuesTable,_getValueOfList(_valuecountsList,true),M_LL_INVALID))
                                    outputError("Failed to remember a values table row.");
                            }else{
                                free_list(_valuecountsList); // have to explicitly free the list of the row that we failed to register
                                outputError("Failed to initialize a values table row.");
                            }
                            /*
                            output("Allocation info of type %c written to the table. Press any key to continue...",_allocationtypes[i]);
                            char c;inputCharRead(&c);
                            newline();
                            */
                        }else
                            outputError("Failed to create a values table row.");
                    }
                    free_string(_allocationTypeText);
                }
            }else
                outputError("No allocation types/counts registered");
        }
    }else
        outputError("Failed to create the values table header.");
    free(_allocationtypes);free(_allocationcounts); // being copies of the originals
    return _valuesTable;
}
Mmap* _getValuesMap(Mvalue* variableNamesMapValue){
    Mmap* _valuesMap=_getMapOfType(VT_UNDEFINED);
    if(_valuesMap){
        size_t numberOfAllocationTypes=getNumberOfAllocationTypes();
        appendedToMap(_valuesMap,"typecount",_getIntegerValue(numberOfAllocationTypes));
        if(numberOfAllocationTypes){
            // we start with a general overview (the counts per type)
	        char* _allocationtypes=_getAllocationTypes();
            size_t* _allocationcounts=_getAllocationCounts();
            Mstring *_allocationTypeText=__string(),*_allocationTypeCharactersText=_getString("'"); // free ASAP
            if(_allocationTypeText&&_allocationTypeCharactersText){
                Mmap* _valuecountsMap=_getMapOfType(VT_MAP); // we're going to store the value counts in a map
                // NOTE dividing by sizeof(char) is far fetched
                for(size_t i=0;i<numberOfAllocationTypes;i++){
                    if(string_append_char(_allocationTypeCharactersText,_allocationtypes[i])&&string_append_char(_allocationTypeText,_allocationtypes[i])){
                        if(_valuecountsMap){
                            Mmap* _valuecountMap=_getMapOfType(VT_INTEGER);
                            if(_valuecountMap){
                                appendedToMap(_valuecountMap,(i==0?"count sum":"count"),_getIntegerValue(_allocationcounts[i*5+1]));
                                appendedToMap(_valuecountMap,(i==0?"bytes allocated":"allocated"),_getIntegerValue(_allocationcounts[1+i*5]));
                                appendedToMap(_valuecountMap,(i==0?"bytes freed":"freed"),_getIntegerValue(_allocationcounts[2+i*5]));
                                appendedToMap(_valuecountMap,(i==0?"mark bytes allocated":"mark allocated"),_getIntegerValue(_allocationcounts[3+i*5]));
                                appendedToMap(_valuecountMap,(i==0?"mark bytes freed":"mark freed"),_getIntegerValue(_allocationcounts[4+i*5]));
                                if(!appendedToMap(_valuecountsMap,string(_allocationTypeText),_getValueOfMap(_valuecountMap,true)))
                                    output("%sFailed to store the allocation count map of '%c'.\n",ERROR_PREFIX,_allocationtypes[i]);
                            }else
                                output("%sFailed to create the allocation count map of '%c'.\n",ERROR_PREFIX,_allocationtypes[i]);
                        }
                        string_setlength(_allocationTypeText,0);
                    }
                }
                if(!appendedToMap(_valuesMap,"types",_getTextValue(string(_allocationTypeCharactersText),false)))outputError("Failed to store the data type characters.");
                if(_valuecountsMap&&!appendedToMap(_valuesMap,"counts",_getValueOfMap(_valuecountsMap,true)))outputError("Failed to store the data type counts.");
            }
            free_string(_allocationTypeCharactersText);
            free_string(_allocationTypeText); // freed
            // MDH@25NOV2019: essential!!!
            free(_allocationtypes);
            free(_allocationcounts);
        }else
            outputError("No allocation types/counts registered");
        // the variable names map with the structure as returned by _getVariableNamesMap of course
        if(variableNamesMapValue){
            // TODO what are we going to do here??????
        }
    }else
        outputError("Failed to create the values map");
    return _valuesMap;
}

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
Mstring* _getVariableMapText(Menvironment const * const environment,bool showcurlybraces,bool showquotes,bool showmissings,bool showhiddenvariablevalues){
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
            bool hidevalue;
			Mvariable* _mapVariable=_mapelement->_variable;
			if(_mapVariable){
                // let's always show the name
                if(showquotes)p=string_append_char(p,'\'');
                p=string_append(p,_mapVariable->_name);
                if(showquotes)p=string_append_char(p,'\'');
                // 
                hidevalue=!showhiddenvariablevalues;
                if(hidevalue){ // check if truely a variable to hide
                    long long hiddenvariableindex=M_NUMBER_OF_HIDDEN_VARIABLES;
                    while(--hiddenvariableindex>=0&&strcmp(M_HIDDEN_VARIABLE_NAMES[hiddenvariableindex],_mapVariable->_name)!=0);
                    if(hiddenvariableindex<0)hidevalue=false; // not a hidden variable
                }
                if(!hidevalue){
                    // MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
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
    if(name&&strlen(name)>0){ // input valid
        _variable=getVariable(_environment,name,false);
        if(!_variable){ // non-existing...
            if(amVerbose())output("Variable '%s' to be created.\n",name);
            _variable=_getVariable(name,valuetype,immutable); // creates the variable, free when not bound
            if(_variable){
                if(amVerbose())output("Variable '%s' created.\n",name);
                // get a reference to the environment to which variable map we should be appending...
                Menvironment* environment=(_environment?_environment:_executionEnvironment);
                // MDH@10NOV2019: because we now allow immutable environment variable maps, the environment to add the variable to
                //                is the first one up of which the variable map is not immutable...
                //                TODO check whether to use _execution or _parent (I suppose we should move up the execution chain)
                while(environment&&(!environment->_variableMap||environment->_variableMap->immutable))environment=environment->_execution;
                if(environment){
                    if(amVerbose())output("Will attempt to add variable '%s' to environment '%s'.\n",name,environment->_name);
                    Mmapelement* _variableMapelement=(Mmapelement*)MALLOC(1,sizeof(Mmapelement),'m');
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
                    output("%sNo environment to add newly created variable %s to.\n",ERROR_PREFIX,name);
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
                        outputInfo("No value text!");
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

// MDH@14NOV2019: sometimes we need a setValue that does not use assignValue() because we do not want to copy the (composite) value passed in
bool setVariable(const Menvironment* const _environment,const char* const name,const Mvalue* const _value){
    // NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
    if(!name||strlen(name)==0){outputError("No variable specified to set the value of");return false;}
    Mvariable* variable=getVariable(_environment,name,amVerbose());
    if(variable){
        if(!variable->_value||!variable->immutable){
            if(amVerbose())output("Variable '%s' to set.\n",variable->_name);
            // _value needs to be of the right type
            // MDH@03NOV2019: unless it's null (i.e. the type of _value->type is VT_UNDEFINED)
            if(!_value||variable->valuetype==VT_UNDEFINED||variable->valuetype==_value->type||_value->type==VT_UNDEFINED){
                if(variable->_value)decrementReferenceCount(variable->_value); // decrement the reference count on the current value
                variable->_value=_value;
                if(variable->_value)incrementReferenceCount(variable->_value);
                if(amVerbose()){
                    Mstring* _valueText=_getValueText(variable->_value,false);
                    if(_valueText){
                        output("Value '%s' (reference count: %zd) assigned to variable '%s'.\n",string(_valueText),(variable->_value?variable->_value->count:0),name);
                        free_string(_valueText);
                    }else
                    if(variable->_value)
                        outputError("No value text!");
                }
                return true; // releasing the value is my responsibility now...
            }
            output("%sCannot set variable '%s': the new value is of the wrong type.\n",ERROR_PREFIX,name);
        }else
            output("%sCannot set variable '%s': it is not mutable!\n",ERROR_PREFIX,name);
    }else
        output("%sCannot set variable '%s': it is unknown to '%s'.\n",ERROR_PREFIX,name,(_environment?_environment:_executionEnvironment)->_name);
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
// MDH@05NOV2019: if there are missing elements in _argumentList (what we allow now), there should be an associated map element with value NULL
Mmap* _getFunctionArgumentMap(const Mfunction* const _function,const Mlist* const _argumentList){
    Mmap* _functionArgumentMap=NULL;
    // MDH@03MAR2020: _argumentList should also be allowed to be NULL (because then defaults would be used)
    if(_function/*&&_argumentList*/){
        _functionArgumentMap=mapMadeWeak((Mmap*)CALLOC(1,sizeof(Mmap),'M')); // MDH@02NOV2019: force the map to be weak
        Mmap* functionParameterMap=_function->_parameterMap;
        if(_functionArgumentMap&&functionParameterMap){
            if(amVerbose())outputInfo("Matching the function parameters!");
            Mmapelement* functionParameterMapelement=functionParameterMap->_first;
            unsigned long long argumentindex=0; // MDH@05NOV2019: because _argumentList could be sparse, i.e. have missing elements, we use an index that is used to find the argument list element with that index!!!
            Mlistelement* argumentListelement=(_argumentList?_argumentList->_first:NULL);
            while(functionParameterMapelement){
                argumentindex++; // the index of the argument we need
                // if the current list element has an index below the one we need, get the next argument list element until we have found one with an index at least equal to argument index
                while(argumentListelement&&argumentListelement->index<argumentindex)argumentListelement=argumentListelement->_next;
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
                if(argumentListelement&&argumentListelement->index==argumentindex){ // we have an argument list element to use
                    _argumentmapelement->_variable->_value=argumentListelement->_value;
                    // replacing: assignValue(&_argumentmapelement->_variable->_value,argumentListelement->_value);
                    // MDH@05NOV2019: no need to do the following anymore, because we incrementing the argument list element at the start of the loop; removing: argumentListelement=argumentListelement->_next;
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
        if(amVerbose())outputInfo("Argument map created.");
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
            _function=(Mfunction*)CALLOC(1,sizeof(Mfunction),'=');
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
                            Mfunctionmapelement* _functionmapelement=(Mfunctionmapelement*)CALLOC(1,sizeof(Mfunctionmapelement),'+');
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
// helper functions for Mtype()
// NOTE these three functions should always be used to convert between a character and a value type
// TODO in time these essential methods might be moved to Mexecution.c/h
char getValueTypeCharacter(Mvaluetype valuetype,bool immutable){
	return(immutable?IMMUTABLEVALUETYPECHARS[valuetype]:MUTABLEVALUETYPECHARS[valuetype]);
}
Mvaluetype getCharacterOfMutableValueType(char valuetypechar){
    // ASSERT valuetypechar should represent the character associated with the mutable value type
    char* p=strchr(MUTABLEVALUETYPECHARS,valuetypechar);
    return (Mvaluetype)(p-MUTABLEVALUETYPECHARS);
}
char getMutableValueTypeCharacter(char valuetypechar){
    char* p=strchr(MUTABLEVALUETYPECHARS,valuetypechar);
    if(p)return valuetypechar;
    p=strchr(IMMUTABLEVALUETYPECHARS,valuetypechar);
    if(p)return MUTABLEVALUETYPECHARS[p-IMMUTABLEVALUETYPECHARS]; // NOTE I can determine the index (position) in the array by subtracting the start pointer (representing 0)!
    return '\0';
}
bool isValueImmutable(Mvalue* value){
	bool result=true; // by default any value is immutable
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
Mvalue* Mtype(Mvalue* value){
	// every value should have a type text, even if NULL
	// MDH@03NOV2019: actually _value should be the name of a variable because it not we cannot determine whether or not
	//                the variable is mutable, that's why settype() requires the name of the variable (as text)
	char result[4]="'\0\0"; // this means that all characters (except the first) are '\0', so we won't have to append an end-of-text character!!! 
	if(value){
		if(value->type==VT_REFERENCE){ // a variabler reference
			Mvariable* referencedVariable=value->value._reference->variable;
			// NOTE for any reference we return not 'r' but the type of the referenced variable (which we wouldn't have access to otherwise)
			if(referencedVariable)result[1]=getValueTypeCharacter(referencedVariable->valuetype,referencedVariable->immutable);
		}else{ // a non-reference type
            // MDH@05NOV2019: composite values (maps and lists) also have an element (value) type, and also have an immutable flags
            //                i.e. noncomposite values are immutable by definition
            //                in essence the immutability pertains to the map or list, so yes 
			result[1]=getValueTypeCharacter(value->type,isValueImmutable(value));
            if(value->type==VT_LIST){
                result[2]=getValueTypeCharacter(value->value._list->valuetype,false);
            }else
            if(value->type==VT_MAP){
                result[2]=getValueTypeCharacter(value->value._map->valuetype,false);
            }
        }
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

bool allListElementsAreOfType(Mlist* list,Mvaluetype valuetype){
    // ASSERT list must NOT be NULL
    if(list)
    if(valuetype!=VT_UNDEFINED){ // a specific type requested
        Mlistelement* listelement=list->_first;
        while(listelement){
            // allowing list element with value NULL or value type VT_UNDEFINED no matter what
            if(listelement->_value)if(listelement->_value->type!=VT_UNDEFINED&&listelement->_value->type!=valuetype)return false;
            listelement=listelement->_next;
        }
    }
    return true;
}
bool allMapAttributesAreOfType(Mmap* map,Mvaluetype valuetype){
    // ASSERT map must NOT be NULL
    if(map)
    if(valuetype!=VT_UNDEFINED){ // a specific type requested
        Mmapelement* mapelement=map->_first;
        Mvalue* attributeValue;
        Mvariable* attribute;
        while(mapelement){
            // allowing map attribute with value NULL or value type VT_UNDEFINED no matter what
            attribute=mapelement->_variable;
            if(attribute&&attribute->valuetype!=VT_UNDEFINED&&attribute->valuetype!=valuetype)return false; // TODO do we need this as well??? Can't remember why we are storing map values like this (through a variable!!!!)
            attributeValue=(attribute?attribute->_value:NULL);
            // TODO should we check the value type of the given variable as well?????
            if(attributeValue)if(attributeValue->type!=VT_UNDEFINED&&attributeValue->type!=valuetype)return false;
            mapelement=mapelement->_next;
        }
    }
    return true;
}
/**
 * \brief to set the type and immutable flag of a variable or composite value
 */
Mvalue* Msettype(Mvalue* value,Mvalue* valuetypeValue,Mvalue* immutableValue){
    // check the types first, both should be strings
    Mvariable* variable=NULL;
    bool valuetypeSpecified=(valuetypeValue&&valuetypeValue->type==VT_TEXT&&strlen(valuetypeValue->value._text->_c)>0); // the value type is specified (and there is at least one character), if not specified will NOT change the value type
    // TODO should we force value type to be text???? for now yes
    if(value&&(valuetypeSpecified||immutableValue)){
        switch(value->type){
            case VT_MAP:case VT_LIST:break;
            case VT_REFERENCE:
                {
                    variable=value->value._reference->variable;
                    // it's best NOT to create the variable if it does not yet exist although we could
                    if(!variable){output("%s",ERROR_PREFIX);outputValue("Cannot set the type of an non-existing variable through reference '",value,"'.\n");return NULL;}
                    break;
                }
            default:outputError("Can not set the type of values that are scalar or text (representing the name of a variable)");return NULL;
        }
        // set the valuetype
        // no longer allowing uppercase to be used as a shortcut to immutability?????? yes, we can still do that
        long long immutable=M_LL_INVALID; // whether we should change the immutable flag
        Mvaluetype valuetype=VT_UNDEFINED;
        if(valuetypeSpecified){ // a value type defined
            // we should locate the character in either MUTABLE
            char valuetypechar=valuetypeValue->value._text->_c[0];
            char mutablevaluetypechar=getMutableValueTypeCharacter(valuetypechar);
            if(!mutablevaluetypechar){output("%s",ERROR_PREFIX);outputValue("Invalid value type specification '",valuetypeValue,"'.\n");return NULL;}
            immutable=(mutablevaluetypechar==valuetypechar?M_FALSE:M_TRUE); // if the same we received the mutable variant
            valuetype=getCharacterOfMutableValueType(mutablevaluetypechar);
            /* replacing (which we would need to change whenever (IM)MUTABLEVALUETYPECHARS would change, which of course is easy to forget):
            switch(valuetypechar){ // use the first character (which will be '\0' if the default value is used!!!)
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
            */
        }
        // if a third argument is specified it takes precedence over what the second argument says
        if(immutableValue)immutable=isValueOne(immutableValue); // accepting all values that represent 1 to be considered true
        // only change the type when a (valid) value type was specified
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
                // see below: if(immutable!=M_LL_INVALID)variable->immutable=(immutable==M_TRUE);
            }else{
                // MDH@05NOV2019: only allow the change of the valuetype of list/map elements if all the current values in the list are already of that type (or of a supertype)
                if(value->type==VT_LIST){
                    // if changing to a more strict type (it is always possible to return to the VT_UNDEFINED type)
                    if(value->value._list->valuetype!=valuetype){
                        if(allListElementsAreOfType(value->value._list,valuetype))
                            value->value._list->valuetype=valuetype;
                        else
                            outputError("Unable to change the list value type: not all current elements are of the new type");
                    }else 
                    if(amVerbose())output("The list is already of the requested value type.");
                }else
                if(value->type==VT_MAP){
                    if(value->value._map->valuetype!=valuetype){
                        if(allMapAttributesAreOfType(value->value._map,valuetype))
                            value->value._map->valuetype=valuetype;
                        else
                            outputError("Unable to change the map value type: not all current attributes are of the new type");
                    }else 
                    if(amVerbose())output("The map is already of the requested type.");
                }
            }
        }
        if(immutable!=M_LL_INVALID){
            bool immutableflag=(immutable==M_TRUE);
            if(variable){
                variable->immutable=immutableflag;
                if(amVerbose())output("Variable '%s' is now %smutable.\n",(variable->immutable?"im":""));
            }else
            if(value->type==VT_LIST){
                value->value._list->immutable=immutableflag;
                if(amVerbose())output("List '%s' is now %smutable.\n",(value->value._list->immutable?"im":""));
            }else
            if(value->type==VT_MAP){
                value->value._map->immutable=immutableflag;
                if(amVerbose())output("List '%s' is now %smutable.\n",(value->value._list->immutable?"im":""));
            }else
            if(amVerbose())output("%sUnable to change the mutability of a value of type %s.\n",ERROR_PREFIX,VALUETYPENAMES[value->type]);
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
bool completedIntegerBooleanFunction(Mfunction* const _function,const char* const functionName,TwoArgumentFunction twoArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_TWO_ARGUMENTS;
        _function->functionunion.twoArgumentFunction=twoArgumentFunction;
        // NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
        _function->_parameterMap=_getIntegerBooleanMap("number of decimals","compute sine table");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered integer boolean function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register integer boolean argument function '%s'.\n",ERROR_PREFIX,functionName);
    }
    return false;
}/* NOT VALIDATED */
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
bool completedListValueIntegerFunction(Mfunction* const _function,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){
    if(_function){
        _function->type=FT_INTERNAL_THREE_ARGUMENTS;
        _function->functionunion.threeArgumentFunction=threeArgumentFunction;
        _function->_parameterMap=_getListValueIntegerMap("list to search","value to find","maximum number of elements");
        if(_function->_parameterMap){
            if(amVerbose())output("Registered function '%s' completed.\n",functionName);
            return true;
        }
        output("%sFailed to register list value integer function '%s'.\n",ERROR_PREFIX,functionName);
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
            Muserfunction* _userfunction=(Muserfunction*)CALLOC(1,sizeof(Muserfunction),'-');
            if(_userfunction){
                if(amVerbose()){outputValue("Defining function '",_nameValue,"' with ");outputValue(" parameters ",_parameterMapValue,".\n");}
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
                    // MDH@02MAR2020: the following is dangerous, because the value might be freed in which case the map would be freed as well!!!!
                    //                so we have to make a copy of the parameter map
                    _function->_parameterMap=_getMapCopy(_parameterMapValue->value._map); // MDH@03MAR2020: making a copy of the map wrapped in the value passed in
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

    if(!completedValueFunction(_getFunction(_environment,"bc"),"bc",Mbc))return false;
    if(!completedValueFunction(_getFunction(_environment,"tc"),"tc",Mtc))return false;
    if(!completedThreeIntegersFunction(_getFunction(_environment,"brgb"),"brgb",Mbrgb))return false;
    if(!completedThreeIntegersFunction(_getFunction(_environment,"trgb"),"trgb",Mtrgb))return false;
    return true;
}/* VALIDATED */

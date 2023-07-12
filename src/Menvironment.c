/**
 * MDH@24JUN2019: everything that deals with Menvironments
 */
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <locale.h>

#include "Menvironment.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_ENVIRONMENT,id};}

// externally (in M.c) defined constants
extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE;
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const char * const M_HIDDEN_VARIABLE_NAMES[]; // MDH@14NOV2019: the name of the M variables to NOT return when requesting the variable map text!!!
extern const char M_DEREFERENCE_CHARACTER; // MDH@10MAR2020: is set elsewhere (in Mshell.c/h)
extern const char M_PROPERTY_SEPARATOR_CHARACTER; // MDH@12MAR2020: is set elsewhere (in Mshell.c/h)
extern const unsigned long long M_NUMBER_OF_HIDDEN_VARIABLES;
// TODO make the following variables start with M_
extern const char* const DEFINEUSERFUNCTION_NAME; // the name of the define user function function
extern const char* const DEFINEANONYMOUSFUNCTION_NAME; // the name of the define user function function
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char* const M_ERROR_PREFIX;
extern const char* const M_WARNING_PREFIX;
extern const char * const VALUETYPENAMES[];
extern const char * const M_LOCALE_SETTINGS_VARIABLE_NAME; // MDH@05DEC2020

// moved over to the end of Mvalue.c

// keep track of the current execution environment
// MDH@03FEB2020: now wrapped inside a value
/**
 * @brief the global M value wrapping the execution environment
 * 
 */
static Mvalue* _executionEnvironmentValue=NULL;
/**
 * @brief returns the current execution environment
 * 
 * @return Menvironment* the current execution environment
 */
Menvironment* getExecutionEnvironment(){
	return getValueEnvironment(_executionEnvironmentValue);
} // convenience method for obtaining the current execution environment from its wrapper
// MDH@31MAY2020: the execution environment hangs inside a value
/**
 * @brief returns the owner of any execution environment
 * 
 * @return Mallocationowner the owner of any execution environment
 */
Mallocationowner getOwnerExecutionEnvironment(){return Msubowner(getValueOwner(),1);}
/**
 * @brief returns the name of the current execution environment
 * 
 * @return Mstring* the name of the current execution environment
 */
Mstring* _getExecutionEnvironmentName(){
	return _getEnvironmentName(getExecutionEnvironment());
}
/**
 * @brief outputs the current execution environment name prefixed by \p prefix, and suffixed by \p suffix
 * 
 * @param prefix 
 * @param suffix 
 * @return the number of characters output
 */
size_t outputExecutionEnvironmentName(char* prefix,char* suffix){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	Mstring* _environmentName=owned_string(_getExecutionEnvironmentName(),owner);
	if(_environmentName!=NULL){
		if(prefix!=NULL)written+=output("%s",prefix);
		written+=output("%s",string(_environmentName));
		if(suffix!=NULL)written+=output("%s",suffix);
		FREE_STRING(_environmentName,owner);
	}
	return written;
}
// MDH@14JUN2020: _environment is supposedly disowned when doing this so _getValueOfEnvironment() can take over ownership
// MDH@21OCT2020: which is a serious problem as it isn't (at least not for function execution environments) but fixed that just now
/**
 * @brief pushes M environment \p _environment on the stack of active environments
 * 
 * @param _environment 
 * @return true on success
 * @return false on failure
 */
bool pushExecutionEnvironment(Menvironment* _environment){Mallocationowner owner=getOwner(__LINE__);
	// MDH@28MAY2020: check if we actually obtain ownership of _environment at all
	// MDH@03FEB2020: wrap the _environment in a value, do NOT free when unsuccessful though (we let the caller take care of that)
	Mvalue* _environmentValue=(_environment!=NULL?_getValueOfEnvironment(_environment):NULL); // TODO check whether _environment passed in needs to be disowned or not (I think better not!!!)
	if(NULL==_environmentValue)return false;
	// MDH@04MAR2020 what WAS I thinking? to point the environment to itself but to the current execution environment
	if(NULL==_environment->_parent)assignValue(&_environment->_parent,_executionEnvironmentValue); // if without a parent give it the current one
	// keep a reference to the current execution environment (value) that we may return to if the execution environment is popped off
	assignValue(&_environment->execution,_executionEnvironmentValue); // MDH@03FEB2020 replacing: _environment->_execution=_executionEnvironment; // remember to what execution environment to pop back to
	// replace the current execution environment with the new one
	assignValue(&_executionEnvironmentValue,_environmentValue); // MDH@03FEB2020 OOPS almost forgot to use assignValue() here!!!
	if(amVerboseDebugging())
		outputExecutionEnvironmentName("New execution environment '","'.\n");
	return true;
}/* NOT VALIDATED */
// MDH@25OCT2021: the brilliant idea I had two days ago will return the value that wraps the popped environment
//				so it can be used as 'object'
/**
 * @brief returns the popped execution environment wrapper
 * 
 * @return Mvalue* the popped execution environment wrapper
 */
Mvalue* popExecutionEnvironment(){
	Menvironment* _executionEnvironment=getExecutionEnvironment();
	if(NULL==_executionEnvironment){outputBug("No environment left to pop!");return NULL;} // nothing to pop
	// NOTE only execution environments that have a parent can be popped!!!
	// MDH@03FEB2020: freeing the current execution environment value will NULL the execution field (i.e. releasing the reference to the environment it points to), so by remembering it here, we can use it AFTER the free_value call
	Mvalue* _nextExecutionEnvironmentValue=_executionEnvironment->execution; 
	if(NULL==_nextExecutionEnvironmentValue){outputBug("Can't pop the top-most environment!");return NULL;}
	// OOPS the following is wrong because only the garbage collector is allowed to free values: free_value(_executionEnvironmentValue); // MDH@03FEB2020 replacing: free_environment(_executionEnvironment); // TODO I guess we won't be needing this execution environment any more????
	// MDH@03FEB2020 by assigning to _executionEnvironmentValue the reference count to the environment is incremented again so it will not be 'garbage collected'!!!!
	Mvalue* _result=_executionEnvironmentValue; // i.e. what getEnvironment() would return!!!
	assignValue(&_executionEnvironmentValue,_nextExecutionEnvironmentValue); // MDH@03FEB2020 replacing: _executionEnvironment=_previousExecutionEnvironment;
	if(amVerboseDebugging())
		outputExecutionEnvironmentName("Returned to execution environment '","'.\n");
	return _result;
}/* NOT VALIDATED */
/**
 * @brief returns the M value wrapping the current environment
 * 
 * @return Mvalue* 
 */
Mvalue* getEnvironment(){
	return _executionEnvironmentValue;
}/* VALIDATED */
/**
 * @brief returns the expression token of the current execution environment
 * 
 * @return Mtoken* the expression token of the current execution environment
 */
Mtoken* getEnvironmentExpressionToken(){
	// MDH@22JUL2019: let's allow breaking here
	////////if(kbhit())return NULL;
	Menvironment* executionEnvironment=getExecutionEnvironment();
	return(executionEnvironment?executionEnvironment->expressionToken:NULL);
}/* VALIDATED */
/**
 * @brief returns the current environment expression token after updating it to the next expression token 
 * 
 * @return Mtoken* the current environment expression token
 */
Mtoken* nextEnvironmentExpressionToken(){
	Menvironment* _executionEnvironment=getValueEnvironment(_executionEnvironmentValue);
	if(NULL==_executionEnvironment)return NULL;
	if(_executionEnvironment->expressionToken!=NULL)_executionEnvironment->expressionToken=_executionEnvironment->expressionToken->next;
	return _executionEnvironment->expressionToken;
}/* VALIDATED */

// read access to the elements defined in an environment
/**
 * @brief returns the total number of variables in M environment \p _environment and all its parent environments
 * 
 * @param _environment 
 * @return uint32_t the total number of variables in M environment \p _environment and all its parent environments
 */
uint32_t getNumberOfVariables(Menvironment const * const _environment){
	if(NULL==_environment||NULL==_environment->_variableMap)return 0;
	// MDH@20JUL2019: now returning the sum of the variables in the parent plus those in the environment itself!!
	return getNumberOfVariables(getValueEnvironment(_environment->_parent))+_environment->_variableMap->numberOfElements;
}/* VALIDATED */

// the names of the variables may be requested
// TODO check whether to use (Mstring*)OWNED or owned_string
/**
 * @brief returns an M string containing the names of all variables in M environment \p _environment and all its parent environments separated by \p sep
 * 
 * @param _environment 
 * @param sep 
 * @return Mstring* the names of all variables in M environment \p _environment and all its parent environments separated by \p sep
 */
Mstring* _getVariableNames(Menvironment const * const _environment,const char* const sep){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _variableNames=NULL;
	if(_environment!=NULL&&sep){
		_variableNames=owned_string(__string(),owner);
		if(_variableNames!=NULL){
			Mstring* p=_variableNames;
			// first append the names of the variables in the parent
			if(_environment->_parent!=NULL){
				Mstring* _parentVariableNames=owned_string(_getVariableNames(getValueEnvironment(_environment->_parent),sep),owner); // free asap
				if(_parentVariableNames!=NULL){
					p=string_append(p,string(_parentVariableNames));
					FREE_STRING(_parentVariableNames,owner); // we can do this because string_append copies the characters
				}
			}
			// we'll be appending the names of the variables in the environment itself
			if(_environment->_variableMap!=NULL){
				Mmapelement* _variableMapelement=_environment->_variableMap->_first;
				while(p!=NULL&&_variableMapelement!=NULL){
					if(_variableMapelement->_variable!=NULL){
						if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
						p=string_append(p,_variableMapelement->_variable->_name->chars);
					}
					_variableMapelement=_variableMapelement->_next;
				}
			}
			if(NULL==p){FREE_STRING(_variableNames,owner);return NULL;}
		}
	}
	return disowned_string(_variableNames,owner);
}/* VALIDATED */

// MDH@14NOV2019: what if someone asks for a list of variable names???
//				how about prefixing the name of the variable with the name of the environment it is part of???
/**
 * @brief returns an M list containing the names of all variables in M environment \p environment
 * 
 * @param environment 
 * @return Mlist* the names of all variables in M environment \p environment
 */
Mlist* _getVariableNamesList(Menvironment* environment){Mallocationowner owner=getOwner(__LINE__);
	if(environment!=NULL){
		Mlist* _variableNamesList=owned_list(_getListOfType(VT_TEXT),owner);
		if(_variableNamesList!=NULL){
			if(environment->_variableMap!=NULL){
				if(amVerbose())output("Creating the list of variable names of '%s'.\n",environment->_name);
				Mmapelement* variableMapelement=environment->_variableMap->_first;
				while(variableMapelement!=NULL){
					if(variableMapelement->_variable!=NULL){
						Mstring* variableNameText=owned_string(_getString("'"),owner);
						if(variableNameText!=NULL){
							if(string_append(variableNameText,variableMapelement->_variable->_name->chars))
								if(appendedToList(_variableNamesList,owner,_getTextValue(string(variableNameText)),M_LL_INVALID)<=0)
									outputError("Failed to store a variable name requesting environment variable names");
							FREE_STRING(variableNameText,owner);
						}
					}
					variableMapelement=variableMapelement->_next;
				}
				return disowned_list(_variableNamesList,owner);
			}
		}
	}
	return NULL;
}
/**
 * @brief returns an M map containing the names of all variables in M environment \p environment and all its parents by environment name
 * 
 * @param environment 
 * @return Mmap* the names of all variables in M environment \p environment and all its parents by environment name
 */
Mmap* _getVariableNamesMap(Menvironment const * const environment){Mallocationowner owner=getOwner(__LINE__);
	// how about returning for each environment that is active an attribute in a map?
	// first get the map of the parent
	Mmap* _variableNamesMap=(environment!=NULL?owned_map(_getMapOfType(VT_UNDEFINED),owner):NULL);
	if(_variableNamesMap!=NULL){
		// storing the local variables in a list that we're going to store in an attribute with name ''
		Mvalue* _localVariableNamesListValue=_getValueOfList(_getVariableNamesList(environment));
		if(_localVariableNamesListValue!=NULL&&appendedToMap(_variableNamesMap,owner,"",_localVariableNamesListValue)!=M_TRUE){
			/// OOPS, no need to free values!!! free_value(_localVariableNamesListValue); // not bound to the variableNamesMap, so free immediately
			output("%sFailed to register the local variable names of '%s'.\n",M_ERROR_PREFIX,environment->_name);
		}
		if(environment->_parent){
			// determine the variable names map of the parent environment and wrap it
			Mvalue* _parentVariableNamesMapValue=_getValueOfMap(_getVariableNamesMap(getValueEnvironment(environment->_parent)));
			// if successfully wrapped append it to the result map but free the value when unsuccesful doing so!!
			if(_parentVariableNamesMapValue!=NULL&&appendedToMap(_variableNamesMap,owner,getValueEnvironment(environment->_parent)->_name->chars,_parentVariableNamesMapValue)!=M_TRUE){
				/// OOPS, no need to free values!!! free_value(_parentVariableNamesMapValue);
				output("%sFailed to register the variable names of the parent of '%s'.\n",M_ERROR_PREFIX,environment->_name);
			}
		}
		return disowned_map(_variableNamesMap,owner);
	}
	return NULL;
}

// MDH@25NOV2019: if we ask for the table, we receive a list with first element containing the column names
/**
 * @brief returns a M list containing a table like data structure with columns with names \p columnNamesList and \p numberOfRows rows
 * 
 * @param columnNamesList the names of the columns
 * @param numberOfRows the number of table rows
 * @param owner_columnNamesList the owner of the column names list
 * @return Mlist* the table
 */
Mlist* _getTable(Mlist* columnNamesList,size_t numberOfRows,Mallocationowner owner_columnNamesList){Mallocationowner owner=getOwner(__LINE__);
	if(columnNamesList!=NULL){
		Mlist* _table=owned_list(_getListOfType(VT_LIST),owner);
		if(_table!=NULL){
			_table->weak=true; // MDH@27NOV2019: if we do the following no value copying will occur!!
			// wrap the column names list in a value
			// oops this is going to be a nuisance as the list would be copied wouldn't it????????
			//	  NO because _getValueOfList() will simply assign the argument to the _list property, nothing more!!!
			//	  
			Mvalue* columnNamesListValue=_getValueOfList(disowned_list(columnNamesList,owner_columnNamesList));
			if(columnNamesListValue!=NULL){
				outputValue("Column names values table: '",columnNamesListValue,"'.\n");
				if(appendedToList(_table,owner,columnNamesListValue,M_LL_INVALID)>0){
					// MDH@23MAR2023: should we return lists or arrays????? perhaps better to return arrays
					while(numberOfRows>0){
						numberOfRows--;
						Mvalue* _rowValue=_getValueOfArray(_getArray("table",columnNamesList->numberOfElements,NULL));
						if(NULL==_rowValue){outputError("Failed to create a new table row; the table will be incomplete");break;}
						if(appendedToList(_table,owner,_rowValue,M_LL_INVALID)<=0){outputError("Failed to append a new table row; the table will be incomplete");break;}
					}
					/* replacing:
					while(numberOfRows>0){
						numberOfRows--;
						Mvalue* _rowValue=_getValueOfList(_getListOfType(VT_UNDEFINED));
						if(NULL==_rowValue){outputError("Failed to create a new table row");break;}
						if(appendedToList(_table,owner,_rowValue,M_LL_INVALID)<=0){outputError("Failed to append a new table row");break;}
					}
					*/
					return disowned_list(_table,owner);
				}
				outputError("Failed to store the column names in a new table");
			}
		}
		// in essence the values in the list have their counts incremented when being added to it
		// and only by decrementing their counts in free_list() do the elements go free
		//  TODO do we need this???? if(owner_columnNamesList.level==0)FREE_LIST(columnNamesList,owner_columnNamesList);
	}
	return NULL;
}
// MDH@25NOV2019: the first example of a list which is constructed as a table is the the values() table
/**
 * @brief outputs an M list \p table containing a table
 * 
 * @param table the table to output
 * @return the number of characters written
 */
size_t outputTable(Mlist const * const table){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	if(table!=NULL){
		 if(table->numberOfElements>0&&table->_first!=NULL){
			// the first list element is supposed to be contain the column names
			if(table->valuetype==VT_LIST){
				// the width of the column names determines the width of the columns with an additional blank in between NO not an extra blank
				Mlistelement* tableListelement=table->_first;
				Mvalue* tablerowValue=tableListelement->_value;
				Mlist* tablerowValueList=(tablerowValue!=NULL&&tablerowValue->type==VT_LIST?tablerowValue->value._list:NULL);
				if(tablerowValueList!=NULL){
					if(tablerowValueList->_first){
						size_t* columnLengths=calloc(tablerowValueList->numberOfElements,sizeof(size_t));
						if(columnLengths!=NULL){
							// get the value text of all elements in the header list
							Mlistelement* headerrowListelement=tablerowValueList->_first;
							size_t columnIndex=0;
							written+=newline(); // start a new line before outputting the table!!!
							while(headerrowListelement!=NULL){
								if(columnIndex==tablerowValueList->numberOfElements){outputBug("Had to break out of table header loop!");break;}
								Mstring* _columnNameText=owned_string(_getValueText(headerrowListelement->_value,true),owner);
								if(_columnNameText!=NULL){
									columnLengths[columnIndex]=output("%s",string(_columnNameText));
									written+=columnLengths[columnIndex];
									FREE_STRING(_columnNameText,owner);
								}
								columnIndex++;
								headerrowListelement=headerrowListelement->_next;
							}
							// ready to show the data rows 
							while(tableListelement->_next!=NULL){
								tableListelement=tableListelement->_next;
								tablerowValue=tableListelement->_value;
								if(tablerowValue!=NULL){
									// allowing the rows to be lists as well (which was the original implementation of table rows)
									size_t cellLength;
									if(tablerowValue->type==VT_LIST){
										tablerowValueList=tablerowValue->value._list;
										if(tablerowValueList!=NULL&&tablerowValueList->numberOfElements>0){
											written+=newline();
											columnIndex=0;
											Mlistelement* rowListelement=tablerowValueList->_first;
											while(rowListelement){
												if(columnIndex==tablerowValueList->numberOfElements){outputBug("Had to break out of table data row loop!");break;}
												Mstring* _cellText=owned_string(_getValueText(rowListelement->_value,true),owner);
												cellLength=(_cellText!=NULL?output("%s",string(_cellText)):0);
												written+=cellLength;
												while(++cellLength<=columnLengths[columnIndex])written+=outputChar(' ');
												FREE_STRING(_cellText,owner);
												rowListelement=rowListelement->_next;
												columnIndex++;
											}
										}
									}else
									if(tablerowValue->type==VT_ARRAY){
										Mvalue* cellValue;
										Marray* tablerowValueArray=tablerowValue->value._array;
										if(tablerowValueArray!=NULL){
											size_t numberOfColumns=tablerowValueArray->numberOfElements;
											for(size_t columnIndex=0;columnIndex<numberOfColumns;++columnIndex){
												Mstring* _cellText=owned_string(_getValueText(tablerowValueArray->values[columnIndex],true),owner);
												cellLength=(_cellText!=NULL?output("%s",string(_cellText)):0);
												written+=cellLength;
												while(++cellLength<=columnLengths[columnIndex])written+=outputChar(' ');
												FREE_STRING(_cellText,owner);
											}
										}
									}
								}
							}
							free(columnLengths); // essential bro'
							written+=newline(); // one newline() at the end!!
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
	return written;
}
/**
 * @brief the hexadecimal characters strings of integer 0 through 127
 * 
 */
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
/** TODO
 * @brief returns an M list containing a table using the keys in \p variableNamesMapValue as variable names
 * 
 * @param variableNamesMapValue 
 * @return Mlist* 
 */
Mlist* _getValuesTable(Mvalue* variableNamesMapValue){Mallocationowner owner=getOwner(__LINE__);
	// MDH@25NOV2019: we should get the allocation types and counts asap (otherwise they will change), unless they are passed in
	Mallocationtype* _allocationTypes=_getAllocationTypes();
	// MDH@14APR2020 now present in the allocation types: t_count* _allocationcounts=_getAllocationCounts();
	Mlist* _valuesTable=NULL;
	// let's create the list containing the column names
	Mlist* _valuesColumnNames=owned_list(_getListOfType(VT_TEXT),owner);
	if(_valuesColumnNames!=NULL
		&&appendedToList(_valuesColumnNames,owner,_getTextValue("'Values: "),M_LL_INVALID)>0
		&&appendedToList(_valuesColumnNames,owner,_getTextValue("'TYPE		"),M_LL_INVALID)>0
		&&appendedToList(_valuesColumnNames,owner,_getTextValue("'SIZE		"),M_LL_INVALID)>0
		&&appendedToList(_valuesColumnNames,owner,_getTextValue("'ALLOCATED   "),M_LL_INVALID)>0
		&&appendedToList(_valuesColumnNames,owner,_getTextValue("'FREED	   "),M_LL_INVALID)>0
		// &&appendedToList(_valuesColumnNames,_getTextValue("'M.ALLOCATED ",false),M_LL_INVALID)>0
		// &&appendedToList(_valuesColumnNames,_getTextValue("'M.FREED	 ",false),M_LL_INVALID)>0
		){
		long long numberOfAllocationTypes=getNumberOfAllocationTypes();
		// get a table with the given values column names and number of rows (which are initialized to empty lists)
		// NOTE tell _getTable() to free the values column names if failing to bind them in a table!!!!
		_valuesTable=owned_list(_getTable(_valuesColumnNames,numberOfAllocationTypes,owner),owner);
		if(_valuesTable!=NULL){
			if(numberOfAllocationTypes){
				// we start with a general overview (the counts per type)
				// NOTE dividing by sizeof(char) is far fetched
				for(long long i=0;i<numberOfAllocationTypes;i++){
					Mstring* _allocationTypeText=owned_string(_getString("'"),owner);
					if(_allocationTypeText!=NULL
							&&string_append_char(_allocationTypeText,_allocationTypes[i].type)
							&&string_append(_allocationTypeText,"=0x")
							&&string_append(_allocationTypeText,HEXCHARS[_allocationTypes[i].type]))
					{
						Mlist* _valuecountsList=owned_list(_getListOfType(VT_UNDEFINED),owner); // we're going to store the value counts in a map
						if(_valuecountsList!=NULL){
							Mvalue* _zeroTextValue=_getTextValue("'"); // will be garbage collected automatically when the reference count is not incremented (as in weak lists)
							_valuecountsList->weak=true; // TODO should we do this???
							// start with the table row index below the name of the table!!!
							if(appendedToList(_valuecountsList,owner,_getIntegerValue(i+1),M_LL_INVALID)>0
								&&appendedToList(_valuecountsList,owner,_getTextValue(string(_allocationTypeText)),M_LL_INVALID)>0){
								// now the 5 counts
								// TODO how about the count???????
								appendedToList(_valuecountsList,owner,_getIntegerValue(_allocationTypes[i]/*.allocationsizeunion*/.size),M_LL_INVALID);
								appendedToList(_valuecountsList,owner,_getIntegerValue(getAllocationTypeOccupied(_allocationTypes[i].type,0)),M_LL_INVALID);
								appendedToList(_valuecountsList,owner,_getIntegerValue(getAllocationTypeFreed(_allocationTypes[i].type,0)),M_LL_INVALID);
								appendedToList(_valuecountsList,owner,_getIntegerValue(getAllocationTypeOccupied(_allocationTypes[i].type,1)),M_LL_INVALID);
								appendedToList(_valuecountsList,owner,_getIntegerValue(getAllocationTypeFreed(_allocationTypes[i].type,1)),M_LL_INVALID);
								if(appendedToList(_valuesTable,owner,_getValueOfList(disowned_list(_valuecountsList,owner)),M_LL_INVALID)<=0)
									outputError("Failed to remember a values table row.");
							}else{
								FREE_LIST(_valuecountsList,owner); // have to explicitly free the list of the row that we failed to register
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
					FREE_STRING(_allocationTypeText,owner);
				}
			}else
				outputError("No allocation types/counts registered");
		}
	}else
		outputError("Failed to create the values table header.");
	free(_allocationTypes);
	return disowned_list(_valuesTable,owner);
}
/** TODO
 * @brief returns the values map
 * 
 * @param variableNamesMapValue 
 * @return Mmap* 
 */
Mmap* _getValuesMap(Mvalue* variableNamesMapValue){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _valuesMap=(Mmap*)OWNED(_getMapOfType(VT_UNDEFINED),owner);
	if(_valuesMap){
		size_t numberOfAllocationTypes=getNumberOfAllocationTypes();
		appendedToMap(_valuesMap,owner,"typecount",_getIntegerValue(numberOfAllocationTypes));
		if(numberOfAllocationTypes){
			// we start with a general overview (the counts per type)
			Mallocationtype* _allocationTypes=_getAllocationTypes(); // TODO put under memory management control?
			// MDH@14APR2020 replacing: size_t* _allocationcounts=_getAllocationCounts();
			Mstring *_allocationTypeText=owned_string(__string(),owner)
				   ,*_allocationTypeCharactersText=owned_string(_getString("'"),owner); // free ASAP
			if(_allocationTypeText&&_allocationTypeCharactersText){
				Mmap* _valuecountsMap=owned_map(_getMapOfType(VT_MAP),owner); // we're going to store the value counts in a map
				// NOTE dividing by sizeof(char) is far fetched
				for(size_t i=0;i<numberOfAllocationTypes;i++){
					if(string_append_char(_allocationTypeCharactersText,_allocationTypes[i].type)&&string_append_char(_allocationTypeText,_allocationTypes[i].type)){
						if(_valuecountsMap){
							Mmap* _valuecountMap=owned_map(_getMapOfType(VT_INTEGER),owner);
							if(_valuecountMap){
								appendedToMap(_valuecountMap,owner,(i==0?"count sum":"count"),_getIntegerValue(_allocationTypes[i]/*.allocationsizeunion*/.size));
								appendedToMap(_valuecountMap,owner,(i==0?"bytes allocated":"allocated"),_getIntegerValue(getAllocationTypeOccupied(_allocationTypes[i].type,0)/*_allocationTypes[i].occupied*/));
								appendedToMap(_valuecountMap,owner,(i==0?"bytes freed":"freed"),_getIntegerValue(getAllocationTypeFreed(_allocationTypes[i].type,0)/*_allocationTypes[i].freed*/));
								appendedToMap(_valuecountMap,owner,(i==0?"mark bytes allocated":"mark allocated"),_getIntegerValue(getAllocationTypeOccupied(_allocationTypes[i].type,1)/*_allocationTypes[i].mark_occupied*/));
								appendedToMap(_valuecountMap,owner,(i==0?"mark bytes freed":"mark freed"),_getIntegerValue(getAllocationTypeOccupied(_allocationTypes[i].type,1)/*_allocationTypes[i].mark_freed*/));
								if(appendedToMap(_valuecountsMap,owner,string(_allocationTypeText),_getValueOfMap(disowned_map(_valuecountMap,owner)))<=0)
									output("%sFailed to store the allocation count map of '%c'.\n",M_ERROR_PREFIX,_allocationTypes[i].type);
							}else
								output("%sFailed to create the allocation count map of '%c'.\n",M_ERROR_PREFIX,_allocationTypes[i].type);
						}
						string_setlength(_allocationTypeText,0);
					}
				}
				if(appendedToMap(_valuesMap,owner,"types",_getTextValue(string(_allocationTypeCharactersText)))<=0)
					outputError("Failed to store the data type characters.");
				if(_valuecountsMap){
					if(appendedToMap(_valuesMap,owner,"counts",_getValueOfMap(disowned_map(_valuecountsMap,owner)))<=0){
						outputError("Failed to store the data type counts.");
						FREE_MAP(_valuecountsMap,owner);
					}
				}
			}
			FREE_STRING(_allocationTypeCharactersText,owner);
			FREE_STRING(_allocationTypeText,owner); // freed
			// MDH@25NOV2019: essential!!!
			free(_allocationTypes);
			// MDH@14APR2020: free(_allocationcounts);
		}else
			outputError("No allocation types/counts registered");
		// the variable names map with the structure as returned by _getVariableNamesMap of course
		if(variableNamesMapValue){
			// TODO what are we going to do here??????
		}
	}else
		outputError("Failed to create the values map");
	return DISOWNED(_valuesMap,owner);
}

// MDH@08AUG2019: when _environment is NULL, we only check the current execution environment (this makes sense because with no environment presented, we only have the current execution environment to check)
// MDH@10MAR2020: if `name` ends with @ we should return the variable that the thing in front of it references (i.e. if y=@x then y@ represents x)
// MDH@12MAR2020: if `name` ends with . the part in front of it needs to be an existing variable that contains a map otherwise it is actually not allowed but that is actually handled by the parser (not allowing . in new variable)
/**
 * @brief returns the M variable in \p environment with name \p name
 * @details returns NULL if \p environment does not contain a variable with name \p name
 * @param environment
 * @param name
 * @param report
 * @return the M variable with name \p name in M environment \p environment
 */
Mvariable* getVariable(Menvironment const * const _environment,char /*const*/ * const name, bool report){ // MDH@10MAR2020: because we might want to cut off the last character name characters can not be const any more (between char and *)
	if(NULL==name){if(report)outputError("No variable name specified requesting the variable");return NULL;}
	// MDH@10MAR2020: taking care of variable names that end with @ which acts as dereference operator
	size_t l=strlen(name);
	// MDH@25OCT2020 removing: if(l==0){if(report)outputError("Undefined variable name");return NULL;}
	// MDH@12MAR2020: if `name` refers to a property in a map variable we have to determine if that property exists or not and return it if it does
	//				in combination with containsVariable() we decided to 
	char* nextPropertySeparator=strchr(name,M_PROPERTY_SEPARATOR_CHARACTER);
	if(nextPropertySeparator!=NULL){
		char* propertySeparator=name;
		// the 'root' name must be a variable in the current (environment) variable map
		propertySeparator[nextPropertySeparator-propertySeparator]='\0'; // pointer arithmetic
		Mvariable* mapVariable=getVariable(_environment,propertySeparator,report);
		propertySeparator[nextPropertySeparator-propertySeparator]=M_PROPERTY_SEPARATOR_CHARACTER;
		if(NULL==mapVariable)return NULL;
		if(NULL==mapVariable->_value)return NULL;
		if(mapVariable->_value->type!=VT_MAP)return NULL;
		// it's a map so now we can check all properties
		Mmap* map=mapVariable->_value->value._map;
		Mmapelement* mapelement;
		// starting out with a map and a list of 'dot' seperated property names
		while(map!=NULL){
			propertySeparator=nextPropertySeparator+1; // the first position after the 'dot' so containing the 
			nextPropertySeparator=strchr(propertySeparator,M_PROPERTY_SEPARATOR_CHARACTER);
			if(nextPropertySeparator!=NULL)propertySeparator[nextPropertySeparator-propertySeparator]='\0';
			if(report)output("Looking for property '%s' in '%s'.\n",propertySeparator,name);
			mapelement=map->_first;while(mapelement!=NULL&&(NULL==mapelement->_variable||strcmp(mapelement->_variable->_name->chars,propertySeparator)))mapelement=mapelement->_next;
			if(nextPropertySeparator!=NULL)propertySeparator[nextPropertySeparator-propertySeparator]=M_PROPERTY_SEPARATOR_CHARACTER; // put the property separator character back where it belongs
			if(NULL==mapelement)return NULL; // if we did not find a matching map element (property) definitely not an existing property
			// ASSERT matching 'property' found
			if(NULL==nextPropertySeparator)return mapelement->_variable; // if no next property to look for we can return the associated (map) variable
			// ASSERT because there is a next property the property must have a map associated with it, so let's update map
			map=(mapelement->_variable!=NULL&&mapelement->_variable->_value!=NULL&&mapelement->_variable->_value->type==VT_MAP?mapelement->_variable->_value->value._map:NULL);
		}
		// if we get here we definitely did not find the final property
		if(report)output("Property '%s' not found.\n",name);
		return NULL;
	}
	// testing with: outputChar(M_DEREFERENCE_CHARACTER);
	if(name[l-1]==M_DEREFERENCE_CHARACTER){
		// I need to cut off the last character, get the associated variable of that part
		name[l-1]='\0';
		if(report)output("Looking for the variable referenced by '%s'.\n",name);
		Mvariable* variable=getVariable(_environment,name,report);
		name[l-1]=M_DEREFERENCE_CHARACTER;
		if(NULL==variable)return NULL;
		// OOPS testing the valuetype of the variable is NOT enough
		//	  because the variable itself could be unbound (i.e. untyped)
		//	  whereas the value being held could be a reference!!!!!
		//	  this means we need to add the following
		if(NULL==variable->_value)return NULL; // no value, so also no variable referenced to start with
		if(variable->_value->type!=VT_REFERENCE)return NULL; // not something referenced
		// replacing (see explanation above): if(variable->valuetype!=VT_REFERENCE)return NULL;
		variable=variable->_value->value._reference->variable;
		if(NULL==variable)return NULL;
		if(report)output("Referenced variable '%s'.\n",variable->_name);
		return variable;
	}
	// MDH@10MAR2020 END
	// input valid
	Menvironment* environment=(_environment!=NULL?_environment:getExecutionEnvironment());
	Mmap* variableMap=(environment!=NULL?environment->_variableMap:NULL);
	if(NULL==variableMap){
		if(report)output("%sNo variables in environment to find '%s' in.\n",M_ERROR_PREFIX,name);
		return NULL;
	}
	if(report)output("Looking for variable '%s' in '%s'.\n",name,environment->_name->chars);
	///////////if(amVerbose())output("Looking for variable '%s'.\n",name);
	Mmapelement* _variableMapelement=variableMap->_first;
	// as long as variable is defined, and the variable's name is not equal to the given name, continue
	while(_variableMapelement!=NULL&&
			(NULL==_variableMapelement->_variable||strcmp(_variableMapelement->_variable->_name->chars,name)))
		_variableMapelement=_variableMapelement->_next;
	// MDH@20JUL2019: if found return
	if(_variableMapelement!=NULL){
		if(report)
			output("Variable '%s' found in environment '%s'.\n",name,environment->_name->chars);
		return _variableMapelement->_variable;
	}
	if(report)output("Variable '%s' NOT found in environment '%s'.\n",name,environment->_name->chars);
	// if there's an environment and it has a parent check that, otherwise (e.g. in a closure) no global variables available!!!
	return (environment!=NULL&&environment->_parent!=NULL?getVariable(getValueEnvironment(environment->_parent),name,report):NULL);
}/* VALIDATED */

/**
 * @brief returns the name of the constant with name \p name in M environment \p _environment or one of its parents
 * @details if \p _environment is NULL the current execution environment is used
 * @param _environment 
 * @param name 
 * @param value 
 * @return char* the name of the constant with name \p name in M environment \p _environment or one of its parents
 */
char* getConstantWithValue(Menvironment const * const _environment,char * name,Mvalue* value){
	if(NULL==value)return NULL; // forget about NULL
	// input valid
	Menvironment* environment=(_environment!=NULL?_environment:getExecutionEnvironment());
	Mmap* variableMap=(environment!=NULL?environment->_variableMap:NULL);
	if(NULL==variableMap){output("%sNo variables in environment to find '%s' in.\n",M_ERROR_PREFIX,name);return NULL;}
	///////////if(amVerbose())output("Looking for variable '%s'.\n",name);
	Mmapelement* _variableMapelement=variableMap->_first;
	// as long as variable is defined, and the variable's name is not equal to the given name, continue
	Mvariable* variable;
	while(_variableMapelement!=NULL){
		variable=_variableMapelement->_variable;
		if(variable!=NULL) // defined
			if(strcmp(variable->_name->chars,name)) // not the same as name
				if(variable->immutable) // a constant
					if(areValuesEqual(variable->_value,value))
						break;
		_variableMapelement=_variableMapelement->_next;
	}
	// MDH@20JUL2019: if found return
	if(_variableMapelement!=NULL){
		if(amVerboseDebugging()){output("Variable '%s' found in environment '%s'",name,environment->_name->chars);outputValue(" with value '",value,"'.\n");}
		return _variableMapelement->_variable->_name->chars;
	}
	// if there's an environment and it has a parent check that, otherwise (e.g. in a closure) no global variables available!!!
	return (environment!=NULL&&environment->_parent!=NULL?getConstantWithValue(getValueEnvironment(environment->_parent),name,value):NULL);
}
/**
 * @brief returns the text representation of the variables map in M environment \p _environment
 * 
 * @param _environment 
 * @param showcurlybraces 
 * @param showquotes 
 * @param showmissings 
 * @param showhiddenvariablevalues 
 * @return Mstring* the text representation of the variables map in M environment \p _environment
 */
Mstring* _getVariableMapText(Menvironment const * const _environment,bool showcurlybraces,bool showquotes,bool showmissings,bool showhiddenvariablevalues){Mallocationowner owner=getOwner(__LINE__);
	Menvironment* environment=(_environment!=NULL?_environment:getExecutionEnvironment());
	Mmap* map=(environment!=NULL?environment->_variableMap:NULL);
	Mstring* result=(map!=NULL?owned_string(__string(),owner):NULL);
	if(result!=NULL){
		Mstring* p=result;
		if(amDebugging())p=string_append_char(p,'m');
		if(showcurlybraces)p=string_append_char(p,'{');
		//////output("%s",string(p));
		Mmapelement* _mapelement=map->_first;
		while(p!=NULL&&_mapelement!=NULL){
			//////output("%s","start");
			bool hidevalue;
			Mvariable* _mapVariable=_mapelement->_variable;
			if(_mapVariable!=NULL){
				// let's always show the name
				if(showquotes)p=string_append_char(p,'\'');
				p=string_append(p,_mapVariable->_name->chars);
				if(showquotes)p=string_append_char(p,'\'');
				// 
				hidevalue=!showhiddenvariablevalues;
				if(hidevalue){ // check if truely a variable to hide
					long long hiddenvariableindex=M_NUMBER_OF_HIDDEN_VARIABLES;
					while(--hiddenvariableindex>=0&&strcmp(M_HIDDEN_VARIABLE_NAMES[hiddenvariableindex],_mapVariable->_name->chars)!=0);
					if(hiddenvariableindex<0)hidevalue=false; // not a hidden variable
				}
				if(!hidevalue){
					// MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
					if(showmissings||isValueUndefined(_mapVariable->_value)!=M_TRUE){
						/////output("%s",string(p));
						p=string_append_char(p,'='); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
						// MDH@24OCT2019: here we deviate from _getMapText() (see Mvalue.h/c) in that we try to find a constant with the same value (like PI or E or NULL)
						char* constantWithValue=getConstantWithValue(environment,_mapVariable->_name->chars,_mapVariable->_value);
						if(NULL==constantWithValue){ // not a 'symbolic' value
							/////output("%s",string(p));
							Mstring* _mapelementValueText=owned_string(_getValueText(_mapVariable->_value,false),owner); // free asap
							/////output("Map element: %s",string(p));
							// TODO technically NULL is also a value, so shouldn't be use the undefined value text????
							if(_mapelementValueText!=NULL){
								p=string_append(p,string(_mapelementValueText)); // append 
								FREE_STRING(_mapelementValueText,owner); // release AFTER copying over
							}
						}else // a 'symbolic' value
							p=string_append(p,constantWithValue);
					}
				}
			}
			_mapelement=_mapelement->_next;
			if(_mapelement!=NULL)p=string_append(p,", "); // only when there's a next map element to process
			////output("%s","next");
		}
		//////output("%s(%d)",string(p),string_length(p));
		if(showcurlybraces)p=string_append_char(p,'}');
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(NULL==p){FREE_STRING(result,owner);return NULL;}
	}
	return disowned_string(result,owner);
}/* VALIDATED */
// MDH@24OCT2019 END

// use Mexists to determine if a variable exists passed in as text, we might decide to return the name of the environment it exists in
/**
 * @brief returns M_TRUE if variable with name in \p nameValue exists in the current execution environment, M_FALSE or M_LL_INVALID otherwise
 * @details returns M_LL_INVALID if \p nameValue does not wrap a text
 * @param _nameValue 
 * @return Mvalue* M_TRUE if variable with name in \p nameValue exists in the current execution environment, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mexists(Mvalue* _nameValue){
	long long result=M_LL_INVALID;
	if(_nameValue!=NULL&&_nameValue->type==VT_TEXT)
		result=(getVariable(getExecutionEnvironment(),_nameValue->value._text->_c,false)!=NULL?M_TRUE:M_FALSE);
	return _getIntegerValue(result); // input invalid
}

// MDH@23SEP2019: if we want to show the user how to complete the name of a variable, we can return those characters that may follow \p name to be part of a variable
//				NOTE if name is already the name of a variable we do not consider that variable
/**
 * @brief returns the number of equal characters from the start in \p s1 and \p s2
 * @param s1
 * @param s2
 * @return the number of equal characters from the start in both \p s1 and \p s2
 */
size_t getNumberOfMatchingCharacters(char const * s1,char const * s2){
	// testing *s1 and *s2 as well because we want to stop when we bump into the '\0' character and NOT counting that character
	size_t result=0;if(s1!=NULL&&s2!=NULL){while(*s1&&*s2&&*s1==*s2){result++;s1++;s2++;}}return result;
}
// MDH@24SEP2019: because I decided to return either a completion text, or all characters available to obtain an existing variable/function, I return an Mstring* not a char* anymore with the first character either 1 or 2
// MDH@04NOV2019: with references it is possible that the length of \p name is 0, so I change l>0 into l>=0 (forcing l to -1 when name is NULL)
/**
 * @brief return the completion of variable name \p name in the current execution environment and its parents
 * 
 * @param name 
 * @param functionidentifiersaswell 
 * @return Mstring* the completion of variable name \p name in the current execution environment and its parents
 */
Mstring* _getCompletion(char const * const name,bool functionidentifiersaswell){
	Mstring* _completion=__string(); // this result will start with '\0' indicating that there is no completion or characters from available variables/functions
	if(_completion!=NULL){
		size_t completionlength=0; // the number of characters to be copied of completion (which will point to the first character in the completion string)
		int l=(name!=NULL?strlen(name):-1);
		if(l>=0){
			unsigned char completiontype=1; // the completion type (either 1 or 2)
			if(string_append_char(_completion,completiontype)){ // completion type appended!!!
				// check all active environments
				char* completion=NULL; // the current completion string
				Menvironment* _environment=getExecutionEnvironment();
				while(_environment!=NULL){
					// check variables
					Mmap* variableMap=_environment->_variableMap;
					if(variableMap!=NULL){
						size_t numberOfMatchingCharacters,numberOfMatchingCompletionCharacters,vnl;
						char *variablename;
						// as long as variable is defined, and the variable's name is not equal to the given name, continue
						// NOTE all variables that match should have the same characters behind the name part in order to be considered a valid completion
						Mmapelement* variableMapelement=variableMap->_first;
						while(variableMapelement!=NULL){
							if(variableMapelement->_variable!=NULL){
								variablename=variableMapelement->_variable->_name->chars;
								vnl=(variablename!=NULL?strlen(variablename):0);
								if(vnl>l){ // the variable name is larger then name is
									numberOfMatchingCharacters=getNumberOfMatchingCharacters(name,variablename);
									// wait a minute, we cannot have more than l matching characters
									if(numberOfMatchingCharacters==l){ // all characters in name match (at the beginning)
										if(completiontype==1){
											if(completion!=NULL){
												numberOfMatchingCompletionCharacters=getNumberOfMatchingCharacters(variablename+l,completion);
												if(numberOfMatchingCompletionCharacters<completionlength)completionlength=numberOfMatchingCompletionCharacters;
												if(completionlength==0){ // too bad
													// we should return the initial characters available
													// the first character in the completion we had so far is appended to _completion
													// the first character in the functionname as well
													// if either fails we set the completion type to 0 otherwise to 2
													if(NULL==string_append_char(_completion,completion[0])||NULL==string_append_char(_completion,*(variablename+l)))completiontype=0;else completiontype=2;
													completion=NULL; // don't need completion anymore
													// MDH@04NOV2019 don't think we should break here!!!: break;
												}
											}else{
												completion=variablename+l; // point to the first character after name
												completionlength=vnl-l; // use all following characters
											}
										}else{ // completiontype==2
											// don't add twice!!!
											if(string_find_char(_completion,(*(variablename+l)),0)<0)if(NULL==string_append_char(_completion,*(variablename+l)))completiontype=0;			   
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
						if(functionMap!=NULL){
							size_t numberOfMatchingCharacters,numberOfMatchingCompletionCharacters,fnl;
							char *functionname;
							// as long as variable is defined, and the variable's name is not equal to the given name, continue
							// NOTE all variables that match should have the same characters behind the name part in order to be considered a valid completion
							Mfunctionmapelement* functionMapelement=functionMap->_first;
							while(functionMapelement!=NULL){
								functionname=string(functionMapelement->_name); // TODO why is the function name an Mstring and not simply char*
								fnl=(functionname!=NULL?strlen(functionname):0);
								if(fnl>l){ // the variable name is larger then name is
									numberOfMatchingCharacters=getNumberOfMatchingCharacters(name,functionname);
									// wait a minute, we cannot have more than l matching characters
									if(numberOfMatchingCharacters==l){ // all characters in name match (at the beginning)
										if(completiontype==1){
											if(completion!=NULL){
												numberOfMatchingCompletionCharacters=getNumberOfMatchingCharacters(functionname+l,completion);
												if(numberOfMatchingCompletionCharacters<completionlength)completionlength=numberOfMatchingCompletionCharacters;
												if(completionlength==0){ // too bad: no matching characters with the current completion, meaning that the first continuation characters do not match, so that no continuation can be returned
													// we should return the initial characters available
													// the first character in the completion we had so far is appended to _completion
													// the first character in the functionname as well
													// if either fails we set the completion type to 0 otherwise to 2
													if(NULL==string_append_char(_completion,completion[0])||NULL==string_append_char(_completion,*(functionname+l)))completiontype=0;else completiontype=2;
													completion=NULL; // don't need completion anymore
													// MDH@04NOV2019 don't think we should break here!!!: break;
												}
											}else{
												completion=functionname+l; // point to the first character after name
												completionlength=fnl-l; // use all following characters
											}
										}else{ // completiontype==2
											// don't add twice!!!
											if(string_find_char(_completion,(*(functionname+l)),0)<0)if(NULL==string_append_char(_completion,*(functionname+l)))completiontype=0;			   
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
					_environment=getValueEnvironment(_environment->_parent);
				}
				// either append completion (when the completion type equals 1), or replace the completion type
				if(completiontype==1)if(completion!=NULL&&NULL==string_append_chars(_completion,completion,completionlength))completiontype=0;
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
// MDH@12MAR2020: we have to also now take care of adding properties for name containing 'dots' i.e. property references
/**
 * @brief adds a variable with name \p name to environment \p _environment with owner \p owner_environment of type \p valuetype and immutability \p immutable
 * @details uses the current execution environment when \p _environment is NULL
 * @param _environment 
 * @param owner_environment 
 * @param name 
 * @param valuetype 
 * @param immutable 
 * @return true on success
 * @return false on failure
 */
bool addVariable(Menvironment * const _environment,Mallocationowner owner_environment,char * const name,Mvaluetype valuetype,bool immutable){Mallocationowner owner=getOwner(__LINE__);
	Mvariable* _variable=NULL;
	if(name!=NULL){ // input valid MDH@25OCT2020: variable without name (i.e. strlen(name)==0) now allowed
		// MDH@19MAR2020: if a property reference was accepted (even though the hosting variable does not exist or it's value is currently NULL) we can still create it, the map and the property in the map!!!!
		//				we would need to go from left to right essential at the top level we start with _variableMap of environment
		// MDH@12MAR2020: it's more convenient to take care of adding properties separately because otherwise there would be a lot duplication of code in getVariable() in _getVariable()
		//				we can re-use some of the code that is present in containsVariable()
		// MDH@19MAR2020: _environment determines which environment name should be defined in
		char* property=name; // initialize the property to create to name
		char* propertySeparator=strchr(property,M_PROPERTY_SEPARATOR_CHARACTER);
		if(propertySeparator)property[propertySeparator-property]='\0'; // a property reference and we need to start with the top-level variable
		// determine whether this 'property' exists in the environment
		Menvironment* environment=(_environment!=NULL?_environment:getExecutionEnvironment());
		_variable=getVariable(environment,property,false);
		// if it does not yet exist, try to create it
		if(NULL==_variable){ // the variable doesn't exist in the given environment (and also not in any of it's parents)
			// TODO why would we add the variable to the first immutable variable map?????
			while(environment!=NULL&&(NULL==environment->_variableMap||environment->_variableMap->immutable))environment=getValueEnvironment(environment->_parent);
			Mmap* map=(environment!=NULL?environment->_variableMap:NULL); // the map to add the variable
			if(map){
				if(amVerbose())output("Will attempt to add variable '%s' to environment '%s'.\n",name,environment->_name);
				Mmapelement* _variableMapelement=(Mmapelement*)MALLOC_1(sizeof(Mmapelement),'m',owner);
				if(_variableMapelement!=NULL){
					if(amVerboseDebugging())output("Variable '%s' to be created.\n",name);
					// we may now safely create the variable BUT the type should be a map if this is NOT the last property BUT NO the type of a variable would limit what can be stored in it
					// MDH@08JUN2020: _getVariable() adjusted to accept an Mchars* properly owned to start with (and freed automatically on failure)
					_variable=owned_variable(_getVariable(_getChars(name),(propertySeparator?VT_UNDEFINED:valuetype),immutable),owner); // creates the variable, free when not bound BUT that will NOT happen
					if(_variable!=NULL){
						if(amVerboseDebugging())output("Variable '%s' created.\n",name);
						// store the references
						_variableMapelement->_next=NULL;
						_variableMapelement->_variable=_variable;
						/*
						owned_variable(disowned_variable(_variable,owner),Msubowner(owner_environment,3)); // TODO is this the best way to do that?
						if(amVerboseDebugging())output("Variable '%s' owned by new map element.\n",name);
						*/
						/*
						owned_chars(disowned_chars(_variable->_name,owner),Msubowner(owner_environment,4)); // bound name one level below what we did to the variable
						if(amVerboseDebugging())output("Variable name '%s' owned by new map element.\n",name);
						*/
						Mmapelement* _lastVariableMapelement=map->_last;
						if(_lastVariableMapelement!=NULL)
							_lastVariableMapelement->_next=_variableMapelement;
						else 
							map->_first=_variableMapelement;
						map->_last=_variableMapelement;
						if(amVerboseDebugging())output("New map element added to environment variable map.\n");
						owned_mapelement(disowned_mapelement(_variableMapelement,owner),Msubowner(owner_environment,2));
						map->numberOfElements++;
						if(amVerbose())
							output("Variable '%s' added to environment '%s'.\n",name,environment->_name);
					}
				}else
					output("%sFailed to create a new map element for variable '%s'.",M_ERROR_PREFIX,name);
			}else
				output("%sNo (environment) variable map to add variable '%s' to.\n",M_ERROR_PREFIX,name);
		}
		if(NULL==propertySeparator)return(_variable!=NULL?true:false); // if not a property reference we're done anyway (and the result depends on whether or not _variable is NULL)
		// ASSERT some property reference, and we have to undo the '\0' character placement
		property[propertySeparator-property]=M_PROPERTY_SEPARATOR_CHARACTER;
		Mallocationowner owner_variablemapelement=Msubowner(owner_environment,2),owner_variable=Msubowner(owner_environment,3),owner_variablename=Msubowner(owner_environment,4); // MDH@09JUN2020: the owner of the variable name
		while(_variable!=NULL){
			// if the variable does not have a value
			if(NULL==_variable->_value)assignValue(&_variable->_value,_getMapValue(VT_UNDEFINED,false,"addVariable"));
			Mmap* map=(_variable->_value!=NULL&&_variable->_value->type==VT_MAP?_variable->_value->value._map:NULL);
			if(NULL==map){_variable=NULL;break;} // if its value is NOT a map failure...
			// update property
			property=propertySeparator+1; // make property point to the start of the next separator
			// we need a map to put the next property in
			// on to the next part
			propertySeparator=strchr(property,M_PROPERTY_SEPARATOR_CHARACTER);
			if(propertySeparator!=NULL)property[propertySeparator-property]='\0';
			// add the given property to the map BUT it might already be defined in the map!!!!!
			Mmapelement* mapelement=map->_first;
			while(mapelement!=NULL&&(NULL==mapelement->_variable||strcmp(property,mapelement->_variable->_name->chars)))
				mapelement=mapelement->_next;
			if(NULL==mapelement){ // property does not yet exist
				Mmapelement* _variableMapelement=(Mmapelement*)MALLOC_1(sizeof(Mmapelement),'m',owner);
				if(_variableMapelement!=NULL){
					if(amVerboseDebugging())output("Variable '%s' to be created.\n",name);
					Mchars* _variablename=owned_chars(_getChars(property),owner); // technically I am the one that has to disown it
					if(_variablename!=NULL){
						// we may now safely create the variable BUT the type should be a map if this is NOT the last property BUT NO the type of a variable would limit what can be stored in it
						_variable=owned_variable(_getVariable(_variablename,(propertySeparator?VT_UNDEFINED:valuetype),immutable),owner); // creates the variable, free when not bound BUT that will NOT happen
						// store the references
						if(_variable!=NULL){
							_variableMapelement->_next=NULL;
							_variableMapelement->_variable=owned_variable(disowned_variable(_variable,owner),owner_variable);
							Mmapelement* _lastVariableMapelement=map->_last;
							if(_lastVariableMapelement!=NULL)_lastVariableMapelement->_next=_variableMapelement;else map->_first=_variableMapelement;
							map->_last=_variableMapelement;
							owned_mapelement(disowned_mapelement(_variableMapelement,owner),owner_variablemapelement);
							owned_chars(_variable->_name,owner_variablename); // MDH@09JUNE2020: assuming _variablename is now bound
							map->numberOfElements++;
							if(amVerbose())
								output("Property '%s' added.\n",property);
						}else{
							FREECHARS(_variablename,owner); // _variablename NOT bound, so has to be freed
							output("%sFailed to add property '%s'.\n",M_ERROR_PREFIX,property);
						}
					}else{
						_variable=NULL;
						outputError("Failed to create the property variable name.");
					}
				}else{ // failure
					_variable=NULL;
					output("%sFailed to create a new map element to store property '%s'.",M_ERROR_PREFIX,property);
				}
			}else // property already exists
				_variable=mapelement->_variable;
			if(NULL==propertySeparator)break;
			// put the separator back
			property[propertySeparator-property]=M_PROPERTY_SEPARATOR_CHARACTER;
		}
		/* MDH@19MAR2020 replacing:
		if(environment){
			Mmap* map=(_variable&&_variable->_value&&_variable->_value->type==VT_MAP?_variable->_value->value._map:NULL);
			bool result=false;
			if(!map)output("%s'%s' does not hold a map value.",M_ERROR_PREFIX,name);else
			if(map->immutable)output("%sCannot add a property to the immutable map stored in '%s'.",M_ERROR_PREFIX,name);else result=true; // TODO more specific please
			name[firstPropertySeparator-name]=M_PROPERTY_SEPARATOR_CHARACTER;
			if(result)if(!appendedToMap(map,firstPropertySeparator+1,NULL)){result=false;output("%sFailed to add property '%s'.",M_ERROR_PREFIX,firstPropertySeparator+1);}
			return result;
		}
		_variable=getVariable(environment,name,false);
		if(!_variable){ // non-existing...
			if(amVerbose())output("Variable '%s' to be created.\n",name);
			_variable=_getVariable(name,valuetype,immutable); // creates the variable, free when not bound
			if(_variable){
				if(amVerbose())output("Variable '%s' created.\n",name);
				// get a reference to the environment to which variable map we should be appending...
				// MDH@10NOV2019: because we now allow immutable environment variable maps, the environment to add the variable to
				//				is the first one up of which the variable map is not immutable...
				//				TODO check whether to use _execution or _parent (I suppose we should move up the execution chain)
				//				DONE MDH@03FEB2020: that should definitely be the parent (as e.g. each function execution has its own environment stack)
				while(environment&&(!environment->_variableMap||environment->_variableMap->immutable))environment=getValueEnvironment(environment->_parent);
				if(environment){
					if(amVerbose())output("Will attempt to add variable '%s' to environment '%s'.\n",name,environment->_name);
					Mmapelement* _variableMapelement=(Mmapelement*)MALLOC(sizeof(Mmapelement),'m');
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
					output("%sNo environment to add newly created variable %s to.\n",M_ERROR_PREFIX,name);
				// ASSERT failed to link the variable to the variable map!!
				free_variable(_variable,true); // MDH@02NOV2019: no value yet assigned so we can pass in the weak flag
				outputErrorAndText("Failed to link variable ",name);
			}else
				outputErrorAndText("Failed to create variable ",name);
		}else
			if(amVerbose())output("%sWon't add existing variable '%s'\n.",M_WARNING_PREFIX,name);
		*/
	}else
		outputError("No variable name specified");
	return(_variable!=NULL);
}/* VALIDATED */

// MDH@25OCT2020: create environment with a nameless variable
/**
 * @brief returns a new M environment
 * 
 * @return Menvironment* a new M environment
 */
Menvironment* _getNewEnvironment(){Mallocationowner owner=getOwner(__LINE__); 
	Menvironment* _environment=owned_environment(__environment(),owner);
	if(_environment!=NULL){
		// always ascertain to have a no-length variable in it
		if(addVariable(_environment,owner,"",VT_UNDEFINED,false))
			return disowned_environment(_environment,owner);
		FREE_ENVIRONMENT(_environment,owner);_environment=NULL;
	}
	return NULL;
} /* VALIDATED */

// MDH@23DEC2020 TODO perhaps it's better to replace environment+name by a variable that's either created or obtained beforehand??????
/**
 * @brief assigns \p value to the variable \p name in M environment \p _environment
 * @param _environment
 * @param name
 * @param _value
 * @returns true on success, false on failure
 */
bool setValue(Menvironment const * const _environment,char /*const*/ * const name,Mvalue const * const _value){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_ENVIRONMENT));
	// NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
	if(NULL==name||strlen(name)==0){outputError("Cannot set the value: no variable name");return false;}
	if(report)
	{output("Setting the value of '%s'",name);outputValue(" to '",_value,"'.\n");}
	Mvariable* variable=getVariable(_environment,name,report);
	if(variable!=NULL){
		if(NULL==variable->_value||!variable->immutable){
			if(report)
				output("Value of variable '%s' to set.\n",variable->_name);
			// _value needs to be of the right type
			// MDH@03NOV2019: unless it's null (i.e. the type of _value->type is VT_UNDEFINED)
			if(NULL==_value||variable->valuetype==VT_UNDEFINED||variable->valuetype==_value->type||_value->type==VT_UNDEFINED){
				///////////////if(_variable->_value)_variable->_value->count--; // decrement the reference count on the current value
				assignValue(&variable->_value,_value); // 'assign' the reference (takes care of updating the reference counts)
				if(report){
					Mstring* _valueText=owned_string(_getValueText(variable->_value,false),owner);
					if(_valueText!=NULL){
						output("Value '%s' with count %zd assigned to variable '%s'.\n",string(_valueText),(variable->_value?variable->_value->count:0),name);
						FREE_STRING(_valueText,owner);
					}else
						outputInfo("No value text!");
				}
				///////////////if(_variable->_value)_variable->_value->count++; // increment the reference count
				return true; // releasing the value is my responsibility now...
			}
			output("%sCannot set the value of variable `%s`: the new value is of the wrong type.\n",M_ERROR_PREFIX,name);
		}else
		if(variable->_value!=NULL){
			output("%sCannot change the value of variable '%s'",M_ERROR_PREFIX,name);
			outputValue(" from '",variable->_value,"'");outputValue(" to '",_value,"': it is not mutable!\n");
		}else
			output("%sCannot initialize the value of variable '%s': it is not mutable!\n",M_ERROR_PREFIX,name);
	}else
		output("%sCannot set the value of variable '%s': it is unknown.\n",M_ERROR_PREFIX,name);
	return false;
}/* VALIDATED */

// MDH@14NOV2019: sometimes we need a setValue that does not use assignValue() because we do not want to copy the (composite) value passed in
/**
 * @brief sets the value of variable with name \p name in M environment \p _environment to \p _value directly
 * 
 * @param _environment 
 * @param name 
 * @param _value 
 * @return true on success
 * @return false on failure
 */
bool setVariable(Menvironment * const _environment,char * const name,Mvalue const * const _value){Mallocationowner owner=getOwner(__LINE__);
	// NOTE _value is NOT allowed to be NULL, only created and not yet initialized variables have a _value equal to NULL
	// MDH@25OCT2020: the name of the variable may now be "" but of course it would need to exist!!!
	if(!name){outputError("No variable specified to set the value of");return false;}
	Mvariable* variable=getVariable(_environment,name,amVerbose());
	if(variable!=NULL){
		if(NULL==variable->_value||!variable->immutable){
			if(amVerbose())output("Variable '%s' to set.\n",variable->_name);
			// _value needs to be of the right type
			// MDH@03NOV2019: unless it's null (i.e. the type of _value->type is VT_UNDEFINED)
			if(NULL==_value||variable->valuetype==VT_UNDEFINED||variable->valuetype==_value->type||_value->type==VT_UNDEFINED){
				if(variable->_value!=NULL)decrementReferenceCount(variable->_value); // decrement the reference count on the current value
				variable->_value=_value;
				if(variable->_value!=NULL)incrementReferenceCount(variable->_value);
				if(amVerbose()){
					Mstring* _valueText=owned_string(_getValueText(variable->_value,false),owner);
					if(_valueText!=NULL){
						output("Value '%s' (reference count: %zd) assigned to variable '%s'.\n",string(_valueText),(variable->_value?variable->_value->count:0),name);
						FREE_STRING(_valueText,owner);
					}else
					if(variable->_value!=NULL)
						outputError("No value text!");
				}
				return true; // releasing the value is my responsibility now...
			}
			output("%sCannot set variable '%s': the new value is of the wrong type.\n",M_ERROR_PREFIX,name);
		}else
		if(variable->_value!=NULL){
			output("%sCannot change the value of variable '%s'",M_ERROR_PREFIX,name);
			outputValue("from '",variable->_value,"'");
			outputValue(" to '",_value,"': it is not mutable.\n");
		}else
			output("%sCannot initialize the value of variable '%s': it is not mutable!\n",M_ERROR_PREFIX,name);
	}else
		output("%sCannot set variable '%s': it is unknown to '%s'.\n",M_ERROR_PREFIX,name,(_environment?_environment:getExecutionEnvironment())->_name);
	return false;
}/* VALIDATED */

/**
 * @brief appends \p _value to list variable with name \p name in M environment \p _environment
 * 
 * @param _environment 
 * @param name 
 * @param _value 
 * @return long long the (positive) index of the added element, or 0 on failure
 */
long long appendToListVariable(Menvironment const * const _environment,const char* const name,const Mvalue* const _value){
	if(NULL==_environment||NULL==name){outputError("No environment or variable name specified");return 0;}
	Mvariable* variable=getVariable(_environment,name,amVerbose());
	if(variable!=NULL){
		Mvalue* variableValue=variable->_value; // OOPS shouldn't assign to _value (that's the parameter name DUMMY)
		if(variableValue!=NULL&&variableValue->type==VT_LIST){ // yes a list we can append to
			// we should prevent circular references
			if(variableValue!=_value){
				long long index=appendedToList(variableValue->value._list,Msubowner(getValueOwner(),1),_value,M_LL_INVALID); // NOTE always append to the end of the list with the first available index that's why I'm passing in 0 instead of a positive index value!!
				if(index>0)return index;
				output("%sFailed to append the value to the list stored in variable '%s': the type of the new value (%u) is wrong.\n",M_ERROR_PREFIX,name,(_value?_value->type:-1));
			}else
				outputError("Circular reference not allowed");
		}else
			output("%sCannot append the value to variable '%s': it's value is not a list!\n",M_ERROR_PREFIX,name);
	}else
		output("%sCannot append the value to list variable '%s': it is unknown.\n",M_ERROR_PREFIX,name);
	return 0;
}/* VALIDATED */

/**
 * @brief returns the value of the existing variable with name \p name in M environment \p _environment
 * @param _environment
 * @param name
 * @return the value of the existing variable with name \p name in M environment \p _environment
 */
Mvalue* getValue(Menvironment const * const _environment,char /*const*/ * const name){
	if(!_environment||!name){outputError("No environment or name specified");return NULL;}
	Mvariable* variable=getVariable(_environment,name,false);
	if(NULL==variable){output("%sVariable '%s' not found.\n",M_ERROR_PREFIX,name);return NULL;}
	return variable->_value;
}/* VALIDATED */

// MDH@26MAR2020: sometimes we need the address of the value pointer (in the variable)
/**
 * @brief returns the holder (=address) of the value of variable with name \p name in M environment \p _environment
 * @param _environment
 * @param name
 * @return the holder (=address) of the value of variable with name \p name in M environment \p _environment
 */
Mvalue** getValueHolder(Menvironment const * const _environment,char /*const*/ * const name){
	if(NULL==_environment||NULL==name){outputError("No environment or name specified");return NULL;}
	Mvariable* variable=getVariable(_environment,name,false);
	if(NULL==variable){output("%sVariable '%s' not found.\n",M_ERROR_PREFIX,name);return NULL;}
	return &(variable->_value);
}/* VALIDATED */

// FUNCTION STUFF
// the names of the variables may be requested
/**
 * @brief returns a M string containing the names of the functions in environment \p _environment and its parents separated by \p sep
 * 
 * @param _environment 
 * @param sep 
 * @return Mstring* a M string containing the names of the functions in environment \p _environment and its parents separated by \p sep
 */
Mstring* _getFunctionNames(Menvironment const * const _environment,const char* const sep){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _functionNames=NULL;
	if(_environment!=NULL&&sep!=NULL){
		_functionNames=owned_string(__string(),owner);
		if(_functionNames!=NULL){
			Mstring* p=_functionNames;
			// first append the names of the variables in the parent
			if(_environment->_parent!=NULL){
				Mstring* _parentFunctionNames=owned_string(_getFunctionNames(getValueEnvironment(_environment->_parent),sep),owner);
				if(_parentFunctionNames!=NULL){
					p=string_append(p,string(_parentFunctionNames));
					FREE_STRING(_parentFunctionNames,owner); // we can do this because string_append copies the characters that string() points to!!
				}
			}
			// we'll be appending the names of the variables in the environment itself
			if(p!=NULL&&_environment->_functionMap!=NULL){
				Mfunctionmapelement* functionmapelement=_environment->_functionMap->_first;
				while(p!=NULL&&functionmapelement!=NULL){
					if(functionmapelement->_function!=NULL){
						if(strlen(sep))if(!string_empty(p))p=string_append(p,sep);
						p=string_append(p,string(functionmapelement->_name)); //////_function->_name));
					}
					functionmapelement=functionmapelement->_next;
				}
			}
			if(NULL==p){FREE_STRING(_functionNames,owner);return NULL;}
		}
	}
	return disowned_string(_functionNames,owner);
}/* VALIDATED */

// MDH@04MAR2020: getFunction() is used to determine if some identifier name represents a function, which can now also be a variable which value is a(n anonymous) function
/**
 * @brief returns the M function with name \p functionName in M environment \p _environment or its ancestors
 * 
 * @param _environment 
 * @param functionName 
 * @return Mfunction* the M function with name \p functionName in M environment \p _environment
 */
Mfunction* getFunction(Menvironment const * const _environment,char const * const functionName){
	if(_environment!=NULL&&functionName!=NULL&&strlen(functionName)){
		Mfunctionmap* functionmap=_environment->_functionMap;
		if(functionmap!=NULL){
			Mfunctionmapelement* functionmapelement=functionmap->_first;
			// we need string() on the function name as function name is an Mstring*
			while(functionmapelement!=NULL){
				// MDH@10JUL2019: _name moved to the map element instead of in the function
				if(functionmapelement->_function!=NULL&&!strcmp(string(functionmapelement->_name),functionName)){
						//////////printf("\nFunction '%s' matches '%s'.",string(_functionmapelement->_function->_name),functionName);
					return functionmapelement->_function;
				}
				functionmapelement=functionmapelement->_next;
			}
		}
		// MDH@04MAR2020: could now also be an anonymous function stored as variable value
		Mmap* variableMap=_environment->_variableMap;
		if(variableMap!=NULL){
			Mmapelement* variablemapelement=variableMap->_first;
			Mvariable* variable;
			while(variablemapelement!=NULL){
				variable=variablemapelement->_variable;
				if(variable!=NULL&&!strcmp(variable->_name->chars,functionName)&&variable->_value&&variable->_value->type==VT_FUNCTION)
					return variable->_value->value._function;
				variablemapelement=variablemapelement->_next;
			}
		}
		// might exist in the parent environment
		if(_environment->_parent!=NULL)return getFunction(getValueEnvironment(_environment->_parent),functionName);
	}
	return NULL;
}/* VALIDATED */

/**
 * @brief returns the number of function parameters of the function with name \p functionName in the current execution environment
 * @details returns -1 if such a function does not exist or has no parameter map
 * @param functionName 
 * @return long long the number of function parameters of the function with name \p functionName in the current execution environment
 */
long long getNumberOfFunctionParameters(char const * const functionName){
	// MDH@20DEC2022: we're going to need this to keep track of the expected number of function arguments
	Mfunction* function=getFunction(getExecutionEnvironment(),functionName);
	return(function!=NULL&&function->_parameterMap!=NULL?function->_parameterMap->numberOfElements:-1);
}

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
// MDH@22OCT2020: in order to allow for additional arguments all additional arguments provided will be returned in a list associated with key '' in the returned function argument map
// MDH@29OCT2020: with a local variables map now stored with the user function as well, we need to add these local variables to the returned function 'argument' map in which case they will mask any external variable with the same name
/**
 * @brief returns a new argument map for the function \p _function using the list of argument names in \p _argumentList
 * 
 * @param _function 
 * @param _argumentList 
 * @param owner_functionargumentmap 
 * @return Mmap* a new argument map for the function \p _function
 */
Mmap* _getFunctionArgumentMap(Mfunction const * const _function,const Mlist* const _argumentList,Mallocationowner owner_functionargumentmap){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging();
	Mmap* _functionArgumentMap=NULL;
	// MDH@03MAR2020: _argumentList should also be allowed to be NULL (because then defaults would be used)
	if(_function!=NULL/*&&_argumentList*/){
		_functionArgumentMap=mapMadeWeak((Mmap*)CALLOC_1(sizeof(Mmap),'M',owner_functionargumentmap)); // MDH@02NOV2019: force the map to be weak
		if(_functionArgumentMap!=NULL){
			Mmap* functionParameterMap=_function->_parameterMap; // MDH@22OCT2020: now allowing the function parameter map to be undefined (if all arguments are considered additional)
			if(amVerboseDebugging())
				outputInfo("Matching the function parameters!");
			Mmapelement* functionParameterMapelement=(functionParameterMap?functionParameterMap->_first:NULL);
			unsigned long long argumentindex=0; // MDH@05NOV2019: because _argumentList could be sparse, i.e. have missing elements, we use an index that is used to find the argument list element with that index!!!
			Mlistelement* argumentListelement=(_argumentList!=NULL?_argumentList->_first:NULL);
			while(functionParameterMapelement!=NULL){
				argumentindex++; // the index of the argument we need
				// if the current list element has an index below the one we need, get the next argument list element until we have found one with an index at least equal to argument index
				while(argumentListelement!=NULL&&argumentListelement->index<argumentindex)argumentListelement=argumentListelement->_next;
				// NOTE if argumentListelement exists, it's index is at least argumentindex
				Mmapelement* _argumentmapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_functionargumentmap);
				if(NULL==_argumentmapelement)break; // TODO should we return NULL?????
				// BUG FIX I suppose we need _variable to point to something
				_argumentmapelement->_variable=(Mvariable*)CALLOC_1(sizeof(Mvariable),'V',owner_functionargumentmap);
				if(NULL==_argumentmapelement->_variable){FREE_MAPELEMENT(_argumentmapelement,true,owner_functionargumentmap);break;}
				// probably can't simply assign??? let's use _strdup then 
				// MDH@17APR2020: _strdup() replaced by _getChars() as on so many other places today
				_argumentmapelement->_variable->_name=SUBOWNED(owned_chars(_getChars(functionParameterMapelement->_variable->_name->chars),owner_functionargumentmap),1); // MDH@09JUN2020: OOPS make the right owner
				if(NULL==_argumentmapelement->_variable->_name){FREE_MAPELEMENT(_argumentmapelement,true,owner_functionargumentmap);break;}
				// associate the argument list element value (if available)
				// MDH@02NOV2019: OK, using assignValue() here (after adjusting assignValue to copy maps and lists)
				//				we get a problem with functions like push() and shove() that try to adjust their argument
				//				therefore we replace the call to assignValue() to a simple assignment
				if(argumentListelement!=NULL&&argumentListelement->index==argumentindex){ // we have an argument list element to use
					_argumentmapelement->_variable->_value=argumentListelement->_value;
					// replacing: assignValue(&_argumentmapelement->_variable->_value,argumentListelement->_value);
					// MDH@05NOV2019: no need to do the following anymore, because we incrementing the argument list element at the start of the loop; removing: argumentListelement=argumentListelement->_next;
				}else // use the default!!!
					_argumentmapelement->_variable->_value=functionParameterMapelement->_variable->_value;
					// replacing: assignValue(&_argumentmapelement->_variable->_value,functionParameterMapelement->_variable->_value);
				// append to _argumentMap
				if(_functionArgumentMap->_last!=NULL)_functionArgumentMap->_last->_next=_argumentmapelement;else _functionArgumentMap->_first=_argumentmapelement;
				_functionArgumentMap->_last=_argumentmapelement;
				_functionArgumentMap->numberOfElements++;
				functionParameterMapelement=functionParameterMapelement->_next;
			}

			// MDH@22OCT2020: let force in a list value associated with map key with name ''
			// let's first determine such a value in the given argument list			
			Mlist* _additionalFunctionArgumentList=NULL; // the list to populate with additional function arguments
			/* MDH@22OCT2020: for now let's assume that if the '' attribute is already in there it is supposed to be the default
			Mvalue* additionalFunctionArgumentListValue=NULL;
			Mmapelement* additionalFunctionArgumentListMapelement=getMapelement(_functionArgumentMap,"");
			if(additionalFunctionArgumentListMapelement){
				additionalFunctionArgumentListValue=additionalFunctionArgumentListMapelement->_variable->_value;
				if(additionalFunctionArgumentListValue&&additionalFunctionArgumentListValue->type==VT_LIST)_additionalFunctionArgumentList=additionalFunctionArgumentListValue->value._list;
			}
			*/
			if(report)outputInfo("Additional function call argument list created");

			// MDH@22OCT2020: append the additional arguments to the additional function argument list
			//				argumentindex is now equal to the number of formal function parameters
			//				determine the argument list element that would be an additional function argument			  
			if(argumentListelement!=NULL&&argumentListelement->index==argumentindex)argumentListelement=argumentListelement->_next;
			if(argumentListelement!=NULL){ // there are additional function call arguments
				if(report)outputInfo("Additional (unnamed) function call arguments defined");
				// if this list does not yet exist, create it
				if(NULL==_additionalFunctionArgumentList)
					_additionalFunctionArgumentList=owned_list(__list("additional function arguments list"),owner);
				if(_additionalFunctionArgumentList!=NULL){
					// create and populate the list first to hold the additional arguments
					while(argumentListelement!=NULL&&
							appendedToList(_additionalFunctionArgumentList,owner,argumentListelement->_value,M_LL_INVALID)>0)
						argumentListelement=argumentListelement->_next;
					if(report)output("Additional function call argument list initialized with %zd elements.\n",_additionalFunctionArgumentList->numberOfElements);
					Mvalue* additionalFunctionArgumentListValue=_getValueOfList(disowned_list(_additionalFunctionArgumentList,owner));
					if(NULL==additionalFunctionArgumentListValue){ // list not wrapped
						FREE_LIST(_additionalFunctionArgumentList,owner); // it's up to me to get rid of the list
						outputError("Failed to wrap the additional function call arguments list");
					}else
					if(appendedToMap(_functionArgumentMap,owner_functionargumentmap,"",additionalFunctionArgumentListValue)<=0)
						outputError("Failed to register the additional function call arguments list");
					else
					if(report)outputInfo("Additional function call argument list appended to argument map");
				}else
					outputError("Failed to create the additional function call arguments list");
			}else
			if(report)outputInfo("No additional (unnamed) function call arguments.");

			// MDH@29OCT2020: user functions have local variables defined in the declaration
			if(_function->type==FT_USER){
				Muserfunction* userfunction=_function->functionunion._userfunction;
				Mmapelement* localMapelement=(userfunction!=NULL&&userfunction->_localMap?userfunction->_localMap->_first:NULL);
				if(localMapelement!=NULL){
					// ASSERT _argumentMap is not NULL which means we have to free it if we fail to add all specified local variables
					Mvariable* localVariable;
					do{
						localVariable=localMapelement->_variable;
						if(localVariable!=NULL&&localVariable->_name!=NULL&&M_TRUE!=appendedToMap(_functionArgumentMap,owner_functionargumentmap,localVariable->_name->chars,localVariable->_value))break;
						localMapelement=localMapelement->_next;
					}while(localMapelement!=NULL);
					// if _localMapelement is still defined, apparently appending the local variable to the argument map failed
					if(localMapelement!=NULL){
						FREE_MAP(_functionArgumentMap,owner_functionargumentmap);_functionArgumentMap=NULL;
						outputError("Failed to register all specified local variables of the function");
					}
				}
			}
		}else
			outputError("Failed to create the additional function call argument list");
		if(report)
			if(_functionArgumentMap!=NULL)
				outputInfo("Argument map created.");
	}
	return _functionArgumentMap; // MDH@09JUN2020 OOPS forgot to disown prev. TODO so why not returned disowned then???????
}/* VALIDATED */

// newFunction renamed to _getFunction(), not to be confused with getFunction()
// _getFunction() will create the function
// MDH@03FEB2020: now wrapping the environment in the parameter list in a value
//				a new function is always created on the currently executing environment
// MDH@25JAN2023 NOTE: _getFunction add the given function to the given environment, and returns it, this means that said function will already be owned by said environment
//                     and there's no need to change it's ownership?????
Mfunction* _getFunction(Menvironment * const _environment,Mallocationowner owner_environment,const char* const name){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==name||strlen(name)==0)return NULL;
	Mfunction* _function=NULL;
	if(_environment!=NULL){
		_function=getFunction(_environment,name); // check for a function with the given name in the given environment
		if(NULL==_function&&_environment->_functionMap!=NULL){ // does not exist yet, and is registrable
			if(amVerboseDebugging())
				output("Registering function '%s'",name);
			///////////_function->type=functionType;
			/* MDH@03FEB2020 CORRECTION: if we decide to make these methods environment stack unaware we can do the assignment outside this function possibly at the moment that the function is passed outside its scope
			// MDH@03FEB2020: by assigning the presented environment value to the definition environment value, the reference counter of _environmentValue will be incremented because it is now bound to an additional variable!!!
			assignValue(&_function->_definitionEnvironmentValue,_environmentValue?_environmentValue:_executionEnvironmentValue); // MDH@03FEB2020 replacing: _function->_definitionEnvironment=_environment; // TODO why would we need this?????
			*/
			Mstring* _functionName=owned_string(__string(),owner);
			if(_functionName!=NULL){
				if(amVerbose())outputChar('.'); // 1
				Mstring* p=_functionName;
				p=string_append(p,name);
				if(p!=NULL){
					if(amVerbose())outputChar('.'); // 2
					_function=(Mfunction*)CALLOC_1(sizeof(Mfunction),'=',owner); // create the function
					if(_function!=NULL){
						if(amVerbose())outputChar('.'); // 3
						Mfunctionmapelement* _functionmapelement=(Mfunctionmapelement*)CALLOC_1(sizeof(Mfunctionmapelement),'+',owner);
						if(_functionmapelement!=NULL){
							if(amVerbose())outputChar('.'); // 4
							// MDH@25JAN2023: the problem here probably is the ownership of the chars inside the _functionName!!!!
							_functionmapelement->_name=(Mstring*)SUBOWNED(owned_string(disowned_string(_functionName,owner),owner_environment),4); // MDH@10JUL2019: moved over to the function map element
							// replacing: _functionmapelement->_name=(Mstring*)SUBOWNED(OWNED(DISOWNED(_functionName,owner),owner_environment),4); // MDH@10JUL2019: moved over to the function map element
							//////////output("\tOwner of the function map element name: '%s'.\n",getOwnerText(_functionmapelement->_name));
							_functionmapelement->_function=(Mfunction*)SUBOWNED(OWNED(DISOWNED(_function,owner),owner_environment),3); // no worries here
							SUBOWNED(OWNED(DISOWNED(_functionmapelement,owner),owner_environment),2); // TODO
							if(amVerbose())outputChar('.'); // 5
							Mfunctionmap* _functionmap=_environment->_functionMap;
							Mfunctionmapelement* _lastFunctionmapelement=(_functionmap!=NULL?_functionmap->_last:NULL);
							if(_lastFunctionmapelement!=NULL)_lastFunctionmapelement->_next=_functionmapelement;else _functionmap->_first=_functionmapelement;
							_functionmap->_last=_functionmapelement;
							_functionmap->numberOfFunctions++;
							if(amVerbose())outputChar('.'); // 6
							///////_function->_name=_functionName; // success!!!!!
						}else{ // failure
							p=NULL;
							output("%sFailed to create the function map element of '%s'.",M_ERROR_PREFIX,name);
						}
					}else{
						p=NULL;
						output("%sFailed to create function '%s'.",M_ERROR_PREFIX,name);
					}
					if(NULL==p){
						if(amVerbose())outputChar('!');
						FREE_STRING(_functionName,owner);
						_functionName=NULL;
						if(amVerbose())outputChar('!');
					}
					// try to append it to the functionMap, if we succeed store _functioName in ->_name
				}else
					output("%sFailed to store function name '%s'.\n",M_ERROR_PREFIX,name);
					// if we fail to register the name and/or the function with the environment free the function!!
				if(NULL==_functionName){
					if(_function!=NULL){
						if(amVerbose())outputChar('!');
							FREE_FUNCTION(_function,owner);_function=NULL;
							if(amVerbose())outputChar('!');
					}
				}else
				if(amVerbose())
					output("%s"," done"); // 7
			}else
			if(NULL==_function)
				output("%sFailed to create the function name to store '%s' in.",M_ERROR_PREFIX,name);
			// if(!_function)output("%sFailed to create function '%s'.\n",M_ERROR_PREFIX,name);
		}else
			output("%sNo function map in the environment to store '%s' in.",M_ERROR_PREFIX,name);
	}else
		output("%sNo environment to search for function '%s'.",M_ERROR_PREFIX,name);
	if(_function!=NULL)
		if(amVerbose())
			output("%s",".\n");
	return _function; /////// if _function is added to the environment successfully do not disown!!!! disowned_function(_function,owner); // MDH@24JAN2023: important to disown the returned function!!!
}/* VALIDATED */

// END FUNCTION STUFF

// the internal functions
// helper functions for Mtype()
// NOTE these three functions should always be used to convert between a character and a value type
// TODO in time these essential methods might be moved to Mexecution.c/h
/**
 * @brief returns the character equivalent of value type \p valuetype
 * @param valuetype 
 * @param immutable 
 * @return char the character equivalent of value type \p valuetype
 */
char getValueTypeCharacter(Mvaluetype valuetype,bool immutable){
	return(immutable?IMMUTABLEVALUETYPECHARS[valuetype]:MUTABLEVALUETYPECHARS[valuetype]);
}
/**
 * @brief returns the mutable value type of the character equivalent \p valuetypechar
 * 
 * @param valuetypechar 
 * @return Mvaluetype the mutable value type of the character equivalent \p valuetypechar
 */
Mvaluetype getCharacterOfMutableValueType(char valuetypechar){
	// ASSERT valuetypechar should represent the character associated with the mutable value type
	char* p=strchr(MUTABLEVALUETYPECHARS,valuetypechar);
	return (Mvaluetype)(p-MUTABLEVALUETYPECHARS);
}
/**
 * @brief returns the mutable value type character equivalent of \p valuetypechar
 * 
 * @param valuetypechar 
 * @return char the mutable value type character equivalent of \p valuetypechar
 */
char getMutableValueTypeCharacter(char valuetypechar){
	char* p=strchr(MUTABLEVALUETYPECHARS,valuetypechar);
	if(p!=NULL)return valuetypechar;
	p=strchr(IMMUTABLEVALUETYPECHARS,valuetypechar);
	if(p!=NULL)return MUTABLEVALUETYPECHARS[p-IMMUTABLEVALUETYPECHARS]; // NOTE I can determine the index (position) in the array by subtracting the start pointer (representing 0)!
	return '\0';
}
/**
 * @brief returns true if the composite value stored in \p value is immutable, false otherwise
 * 
 * @param value 
 * @return true 
 * @return false 
 */
bool isValueImmutable(Mvalue* value){
	bool result=true; // by default any value is immutable
	if(value!=NULL){
		if(value->type==VT_MAP)result=value->value._map->immutable;else
		if(value->type==VT_LIST)result=value->value._list->immutable;else
		if(value->type==VT_ARRAY)result=value->value._array->immutable;
	}
	return result;
}
/**
 * @brief returns the (text representation of) type and immutable flag of \p value
 * @param value the value of which to return the text representing the type and immutable flag
 * @return the (text representation of) type and immutable flag of \p value
 */
Mvalue* Mtype(Mvalue* value){
	// every value should have a type text, even if NULL
	// MDH@03NOV2019: actually _value should be the name of a variable because it not we cannot determine whether or not
	//				the variable is mutable, that's why settype() requires the name of the variable (as text)
	char result[6]="'\0\0\0\0"; // this means that all characters (except the first) are '\0', so we won't have to append an end-of-text character!!! 
	if(value!=NULL){
		if(value->type==VT_REFERENCE){ // a variabler reference
			Mvariable* referencedVariable=value->value._reference->variable;
			// NOTE for any reference we return not 'r' but the type of the referenced variable (which we wouldn't have access to otherwise)
			if(referencedVariable!=NULL)result[1]=getValueTypeCharacter(referencedVariable->valuetype,referencedVariable->immutable);
		}else{ // a non-reference type
			// MDH@05NOV2019: composite values (maps and lists) also have an element (value) type, and also have an immutable flags
			//				i.e. noncomposite values are immutable by definition
			//				in essence the immutability pertains to the map or list, so yes 
			result[1]=getValueTypeCharacter(value->type,isValueImmutable(value));
			if(value->type==VT_LIST){
				result[2]=getValueTypeCharacter(value->value._list->valuetype,false);
			}else
			if(value->type==VT_MAP){
				result[2]=getValueTypeCharacter(value->value._map->valuetype,false);
			}else
			if(value->type==VT_ARRAY){
				result[2]=getValueTypeCharacter(value->value._array->valuetype,false);
				if(value->value._array->valuetype==VT_ARRAY){
					result[3]=getValueTypeCharacter(value->value._array->values[0]->value._array->valuetype,false);
				}
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
	return _getTextValue(result);
}

/**
 * @brief returns true if all elements of list \p list are of type \p valuetype, false otherwise
 * 
 * @param list 
 * @param valuetype 
 * @return true 
 * @return false 
 */
static bool allArrayElementsAreOfType(Marray* array,Mvaluetype valuetype){
	// ASSERT list must NOT be NULL
	if(array!=NULL)
	if(valuetype!=VT_UNDEFINED){ // a specific type requested
		size_t l=array->numberOfElements;
		while(l>0){
			--l;
			// allowing list element with value NULL or value type VT_UNDEFINED no matter what
			if(array->values[l]!=NULL)if(array->values[l]->type!=VT_UNDEFINED&&array->values[l]->type!=valuetype)return false;
		}
	}
	return true;
}
/**
 * @brief returns true if all elements of list \p list are of type \p valuetype, false otherwise
 * 
 * @param list 
 * @param valuetype 
 * @return true 
 * @return false 
 */
static bool allListElementsAreOfType(Mlist* list,Mvaluetype valuetype){
	// ASSERT list must NOT be NULL
	if(list!=NULL)
	if(valuetype!=VT_UNDEFINED){ // a specific type requested
		Mlistelement* listelement=list->_first;
		while(listelement!=NULL){
			// allowing list element with value NULL or value type VT_UNDEFINED no matter what
			if(listelement->_value!=NULL)if(listelement->_value->type!=VT_UNDEFINED&&listelement->_value->type!=valuetype)return false;
			listelement=listelement->_next;
		}
	}
	return true;
}
/**
 * @brief returns true if all map attribute values are of type \p valuetype, false otherwise
 * 
 * @param map 
 * @param valuetype 
 * @return true 
 * @return false 
 */
static bool allMapAttributesAreOfType(Mmap* map,Mvaluetype valuetype){
	// ASSERT map must NOT be NULL
	if(map!=NULL)
	if(valuetype!=VT_UNDEFINED){ // a specific type requested
		Mmapelement* mapelement=map->_first;
		Mvalue* attributeValue;
		Mvariable* attribute;
		while(mapelement!=NULL){
			// allowing map attribute with value NULL or value type VT_UNDEFINED no matter what
			attribute=mapelement->_variable;
			if(attribute!=NULL&&attribute->valuetype!=VT_UNDEFINED&&attribute->valuetype!=valuetype)return false; // TODO do we need this as well??? Can't remember why we are storing map values like this (through a variable!!!!)
			attributeValue=(attribute?attribute->_value:NULL);
			// TODO should we check the value type of the given variable as well?????
			if(attributeValue!=NULL)if(attributeValue->type!=VT_UNDEFINED&&attributeValue->type!=valuetype)return false;
			mapelement=mapelement->_next;
		}
	}
	return true;
}

/**
 * @brief to set the type and immutable flag of a variable or composite value to type \p valuetypeValue and immutability \p immutableValue
 * @param value
 * @param valuetypeValue
 * @param immutableValue
 */
Mvalue* Msettype(Mvalue* value,Mvalue* valuetypeValue,Mvalue* immutableValue){
	// check the types first, both should be strings
	Mvariable* variable=NULL;
	bool valuetypeSpecified=(valuetypeValue!=NULL&&valuetypeValue->type==VT_TEXT&&strlen(valuetypeValue->value._text->_c)>0); // the value type is specified (and there is at least one character), if not specified will NOT change the value type
	// TODO should we force value type to be text???? for now yes
	if(value!=NULL&&(immutableValue!=NULL||valuetypeSpecified)){
		switch(value->type){
			case VT_MAP:case VT_LIST:break;
			case VT_REFERENCE:
				{
					variable=value->value._reference->variable;
					// it's best NOT to create the variable if it does not yet exist although we could
					if(NULL==variable){output("%s",M_ERROR_PREFIX);outputValue("Cannot set the type of an non-existing variable through reference '",value,"'.\n");return NULL;}
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
			if(!mutablevaluetypechar){output("%s",M_ERROR_PREFIX);outputValue("Invalid value type specification '",valuetypeValue,"'.\n");return NULL;}
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
		if(immutableValue!=NULL)immutable=isValueOne(immutableValue); // accepting all values that represent 1 to be considered true
		// only change the type when a (valid) value type was specified
		if(valuetypeSpecified){
			if(variable!=NULL){
				if(valuetype!=variable->valuetype){ // a change of the value type intended (e.g. from undefined i.e. free to integer, or real or whatever)
					// you cannot change the type of a variable that its current value is not (unless the value is undefined or the type of the value is undefined)
					if(variable->_value!=NULL&&variable->_value->type!=valuetype&&variable->_value->type!=VT_UNDEFINED){
						output("%sUnable to change the value type of %s to '%c' when its value is of type '%c'.\n",M_ERROR_PREFIX,variable->_name,MUTABLEVALUETYPECHARS[valuetype],MUTABLEVALUETYPECHARS[variable->_value->type]);
						return NULL;
					}
					variable->valuetype=valuetype; // update the value type
				}
				// see below: if(immutable!=M_LL_INVALID)variable->immutable=(immutable==M_TRUE);
			}else{
				// MDH@05NOV2019: only allow the change of the valuetype of list/map elements if all the current values in the list are already of that type (or of a supertype)
				if(value->type==VT_ARRAY){
					// if changing to a more strict type (it is always possible to return to the VT_UNDEFINED type)
					if(value->value._array->valuetype!=valuetype){
						if(allArrayElementsAreOfType(value->value._array,valuetype))
							value->value._array->valuetype=valuetype;
						else
							outputError("Unable to change the array value type: not all current elements are of the new type");
					}else
					if(amVerbose())output("The list is already of the requested value type.");
				}else
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
			if(variable!=NULL){
				variable->immutable=immutableflag;
				if(amVerbose())output("Variable '%s' is now %smutable.\n",(variable->immutable?"im":""));
			}else
			if(value->type==VT_ARRAY){
				value->value._array->immutable=immutableflag;
				if(amVerbose())output("Array '%s' is now %smutable.\n",(value->value._array->immutable?"im":""));
			}else
			if(value->type==VT_LIST){
				value->value._list->immutable=immutableflag;
				if(amVerbose())output("List '%s' is now %smutable.\n",(value->value._list->immutable?"im":""));
			}else
			if(value->type==VT_MAP){
				value->value._map->immutable=immutableflag;
				if(amVerbose())output("List '%s' is now %smutable.\n",(value->value._list->immutable?"im":""));
			}else
			if(amVerbose())output("%sUnable to change the mutability of a value of type %s.\n",M_ERROR_PREFIX,VALUETYPENAMES[value->type]);
		}
	}
	return Mtype(value);
}/* INVALIDATED */

/**
 * @brief returns true, when successfully registering no argument function \p noArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param noArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,NoArgumentFunction noArgumentFunction){
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		_function->type=FT_INTERNAL_NO_ARGUMENTS;
		_function->functionunion.noArgumentFunction=noArgumentFunction;
		_function->_parameterMap=NULL;
		if(amVerbose())output("Registered no-argument function '%s' completed.\n",functionName);
		return true;
	}
	return false;
}/* VALIDATED */
// MDH@03JUN2020: if we assume that _function (in all following methods) is disowned, we can simply take over ownership
// MDH@24JAN2023: the parameter map is hosted by the function and should be a subowner of the function
/**
 * @brief returns true, when successfully registering one (value) argument function \p oneArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param oneArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){///////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		_function->_parameterMap=owned_map(_getMap("v"),Msubowner(owner_environment,4)); // MDH@24JAN2023: owner replaced by owner_function
		if(_function->_parameterMap){
			// no defaults here!!!
			if(amVerbose())output("Registered single value argument function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register single value argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering one (float) argument function \p oneArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param oneArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedFloatFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		_function->_parameterMap=owned_map(_getFloatMap("x",_getFloatValue(M_LD_NAN)),Msubowner(owner_environment,4)); // MDH@20JUN2019: now using the invalid real value as default (to indicate a missing value)
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered single real argument function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register single real argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering one (integer) argument function \p oneArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param oneArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedIntegerFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getIntegerMap("i",_getIntegerValue(M_LL_INVALID)),Msubowner(owner_environment,4)); // MDH@20JUN2019: now using the invalid value as default (to indicate a missing!!!!)
		if(_function->_parameterMap!=NULL){
		   if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;	
		}
		output("%sFailed to register single integer argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering one (list) argument function \p oneArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param oneArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getListMap("l",_getListValue(VT_UNDEFINED,false,"completedListFunction")),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered list function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register single list argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering one (map) argument function \p oneArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param oneArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedMapFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getMapMap("m",_getMapValue(VT_UNDEFINED,false,"completedMapFunction")),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered map function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register single map argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering two (list and text) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListTextFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){////////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getListTextMap("tosort:list|map","sortmethod:text"),Msubowner(owner_environment,4));
		if(_function->_parameterMap){
			if(amVerbose())output("Registered sort list|map function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register sort list|map function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (token and list) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedTokenListFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,OneArgumentFunction oneArgumentFunction){////////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_ONE_ARGUMENT;
		_function->functionunion.oneArgumentFunction=oneArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getListMap("l",_getListValue(VT_TOKEN,false,"completedTokenListFunction")),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())
				output("Registered token list function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register single token list argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (integer and boolean) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedIntegerBooleanFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getIntegerBooleanMap("number of decimals","compute sine table"),Msubowner(owner_environment,4));
		if(_function->_parameterMap){
			if(amVerbose())output("Registered integer boolean function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register integer boolean argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* NOT VALIDATED */
/**
 * @brief returns true, when successfully registering two (list and function) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListFunctionFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		// NOTE _getIntegerValue(0) will be bound to the variable "i" in the single integer map, and will be freed by free_variable() if this variable is not bound to the map!!
		_function->_parameterMap=owned_map(_getListFunctionMap("list","function"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered list function function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register integer boolean argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* NOT VALIDATED */

/**
 * @brief returns true, when successfully registering two (strings) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedStringStringFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getStringStringMap("variable","type"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register double string argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (floats) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedFloatFloatFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getFloatFloatMap("base","exponent"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register double real argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (map and token) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedMapTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){///////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getMapTokenMap("parameters","body"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register map token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (tokens) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedTokenTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenMap("while condition","while body"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register two token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (values) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){///////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenMap("value to text","format specifier"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register two value argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (integer and value) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedIntegerValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenMap("number of elements","fill value"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register integer value argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (value and integer) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueIntegerFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenMap("list, array or text","new length"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register value integer argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (list and index) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListIndexFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenMap("list","index"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register list index function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering two (list and value) argument function \p twoArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param twoArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,TwoArgumentFunction twoArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_TWO_ARGUMENTS;
		_function->functionunion.twoArgumentFunction=twoArgumentFunction;
		// MDH@12JUL2023: "value" replaced by "index" because that's what it most likely is
		_function->_parameterMap=owned_map(_getTokenTokenMap("list","value"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register list value function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering three (string, map and token) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedStringMapTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getStringMapTokenMap("name","parameters","body"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register string map token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (list, function and value) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListFunctionValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getListFunctionValueMap("list","function","initial value"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register list function value function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (map, map and list) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedMapMapListFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getMapMapListMap("parameters:map","local variables:map","commands:list"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register map map list function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (value, text and function) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueTextValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getStringMapTokenMap("variable name, list or map","value type","immutable"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register a value text value argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (list, function and value) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueValueValueFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getValueValueValueMap("text(s)","separator(s)","item wrapper(s)"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register a value text value argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}
/**
 * @brief returns true, when successfully registering three (list, value and integer) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListValueIntegerFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getListValueIntegerMap("list to search","value to find","maximum number of elements"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register list value integer function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (list, value and index) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedListValueIndexFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getListValueIntegerMap("list to insert into","value to insert","index of list element"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register list value index function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering three (integers) argument function \p threeArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param threeArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedThreeIntegersFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,ThreeArgumentFunction threeArgumentFunction){//////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function!=NULL){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_THREE_ARGUMENTS;
		_function->functionunion.threeArgumentFunction=threeArgumentFunction;
		_function->_parameterMap=owned_map(_getThreeIntegerMap("red","green","blue"),Msubowner(owner_environment,4));
		if(_function->_parameterMap!=NULL){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register three integer argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */


/**
 * @brief returns true, when successfully registering four (value and 3 tokens) argument function \p fourArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param fourArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedValueTokenTokenTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,FourArgumentFunction fourArgumentFunction){////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		//////output("1\n");
		_function->type=FT_INTERNAL_FOUR_ARGUMENTS;
		//////output("2\n");
		_function->functionunion.fourArgumentFunction=fourArgumentFunction;
		/////output("3\n");
		_function->_parameterMap=owned_map(_getValueTokenTokenTokenMap("if condition","then clause","else clause","undefined clause"),Msubowner(owner_environment,4));
		/////output("4\n");
		if(_function->_parameterMap){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register value three token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */
/**
 * @brief returns true, when successfully registering four (tokens) argument function \p fourArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param fourArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedTokenTokenTokenTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,FourArgumentFunction fourArgumentFunction){///////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_FOUR_ARGUMENTS;
		_function->functionunion.fourArgumentFunction=fourArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenTokenTokenMap("for initialization","for condition","for increment","for body"),Msubowner(owner_environment,4));
		if(_function->_parameterMap){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register four token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

/**
 * @brief returns true, when successfully registering five (tokens) argument function \p fiveArgumentFunction with name \p functionName in M enviroment \p _environment, false otherwise
 * 
 * @param _environment 
 * @param owner_environment 
 * @param functionName 
 * @param fiveArgumentFunction 
 * @return true 
 * @return false 
 */
bool completedTokenTokenTokenTokenTokenFunction(Menvironment* const _environment,Mallocationowner owner_environment,const char* const functionName,FiveArgumentFunction fiveArgumentFunction){/////Mallocationowner owner=getOwner(__LINE__);
	Mfunction* _function=_getFunction(_environment,owner_environment,functionName);
	if(_function){
		// OWNED(_function,owner);
		_function->type=FT_INTERNAL_FIVE_ARGUMENTS;
		_function->functionunion.fiveArgumentFunction=fiveArgumentFunction;
		_function->_parameterMap=owned_map(_getTokenTokenTokenTokenTokenMap("for initialization","for condition","for increment","for body","result"),Msubowner(owner_environment,4));
		if(_function->_parameterMap){
			if(amVerbose())output("Registered function '%s' completed.\n",functionName);
			return true;
		}
		output("%sFailed to register five token argument function '%s'.\n",M_ERROR_PREFIX,functionName);
	}
	return false;
}/* VALIDATED */

// MDH@09JUL2019: a function is defined as a parameter map (with defaults) and a body token
// MDH@10JUL2019: the caller will need to register the function in the function map of the definition environment
//				here we only need to return the wrapped user function
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
	Mvalue* _value=__value();
	if(_value){_value->type=VT_TOKEN;_value->value._userfunction=_userfunction->_bodyTokenValue->value._token;}
	if(!_value)free_userfunction(_userfunction);
	return _value;
}
*/

/**
 * @brief returns the number of function commands in the body of the function with name \p functionName
 * @details returns -1 if the function is not found or the name is undefined
 * @param functionName 
 * @return unsigned long long the number of commands in the body of the function with name \p functionName
 */
unsigned long long getNumberOfFunctionCommands(char const * const functionName){
	Mfunction* function=getFunction(getExecutionEnvironment(),functionName);
	if(NULL==function||function->type!=FT_USER){if(NULL==function)output("%sFunction '%s' not found.\n",M_ERROR_PREFIX,functionName);return -1;}
	return (function->functionunion._userfunction->_bodyCommandList!=NULL?function->functionunion._userfunction->_bodyCommandList->numberOfElements:0);
}
/*
bool registerFunctionCommand(const char* const functionName,Mtoken* _command,Mallocationowner owner_command){Mallocationowner owner=getOwner(__LINE__);
	if(!functionName||!_command)return false;
	Mfunction* function=getFunction(getExecutionEnvironment(),functionName);
	if(function&&function->type==FT_USER){
		Mvalue* _commandValue=_getValueOfToken(_command,owner_command); // DONE it's up to the caller TODO I don't think command should be released if we fail, that's why we indicate that it's a global
		if(_commandValue){
			if(!function->functionunion._userfunction->_bodyCommandList)
				function->functionunion._userfunction->_bodyCommandList=__list("function body command list");
			if(appendedToList(function->functionunion._userfunction->_bodyCommandList,Msubowner(getOwnerExecutionEnvironment(),4),_commandValue,M_LL_INVALID)>0)
				return true;
			output("%sFailed to add command to list of body of '%s'.\n",M_ERROR_PREFIX,functionName);
		}else
			output("%sFailed to wrap a command of function '%s'.\n",M_ERROR_PREFIX,functionName);
	}else
		output("%s'%s' does not represent a user function.\n",M_ERROR_PREFIX,functionName);
	return false;
}
*/
// MDH@10JAN2020: there's not much that can go wrong here
/**
 * @brief returns M_TRUE on setting $ to immutable successfully, M_FALSE or M_LL_INVALID otherwise
 * 
 * @return Mvalue* M_TRUE, M_FALSE or M_LL_INVALID
 */
Mvalue* Mbreak(){
	long long result=M_LL_INVALID;
	Mvariable* dollarVariable=getVariable(NULL,"$",false);
	if(dollarVariable!=NULL)result=setImmutable(dollarVariable,true);
	return _getIntegerValue(result);
}
/**
 * @brief returns M_TRUE when successfully setting the value of variable $ to \p _value and making $ immutable, M_FALSE or M_LL_INVALID otherwise
 * 
 * @param _value 
 * @return Mvalue* M_TRUE, M_FALSE or M_LL_INVALID
 */
Mvalue* Mreturn(Mvalue* _value){
	// sets the value of the function execution result variable to _value
	// if you call return with NO value, the current value of $ will be used (or the value of the last executed function body command)
	// do NOT replace the value of "$" if _value is NULL (which should indicate a return without argument), this means you cannot undo the result value
	// perhaps with $=NULL though
	// MDH@10JAN2021
	long long result=M_LL_INVALID;
	if(setValue(getExecutionEnvironment(),"$",_value)){
		result=setImmutable(getVariable(NULL,"$",false),true);
	}
	return _getIntegerValue(result);
	/* replacing:
	if(_value!=NULL&&!setValue(getExecutionEnvironment(),"$",_value)){
		outputError("Failed to set the function execution result variable");
		return NULL;
	}
	// setting the exit flag variable will get the function execution aborted
	if(!setValue(getExecutionEnvironment(),"!",_getIntegerValue(1))){
		outputError("Failed to set the function execution exit flag variable");
		return NULL;
	}
	return _value; // echo the input value
	*/
}/*VALIDATED */

// MDH@23OCT2020: it would be neat if we could even refer to an environment in the name of the variable, 
//				how about returning the previous value?????? but then we wouldn't know it set succeeded????
// MDH@25OCT2020: setting the '' automatic result variable is NOT allowed
// MDH@25OCT2020: I suppose it's a good idea to return a 'boolean' (i.e. M_LL_INVALID, M_FALSE or M_TRUE)
/**
 * @brief sets the value of variable with name \p _variableNameValue to \p _value in the current execution environment
 * 
 * @param _variableNameValue 
 * @param _value 
 * @return Mvalue* M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
Mvalue* Mset(Mvalue* _variableNameValue,Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	long long success=M_LL_INVALID;
	Mstring* _variableName=owned_string(_getValueText(_variableNameValue,true),owner);
	if(_variableName!=NULL&&string_length(_variableName)>0){
		char* variableName=string(_variableName);
		Menvironment* executionEnvironment=getExecutionEnvironment();
		// although we create the variable if it does not exist now, we force it's type to be the type of the value that is going to be assigned to it
		if(getVariable(executionEnvironment,variableName,false)!=NULL||addVariable(executionEnvironment,owner,variableName,(_value?_value->type:VT_UNDEFINED),false))
			success=(setValue(getExecutionEnvironment(),variableName,_value)?M_TRUE:M_FALSE);
		FREE_STRING(_variableName,owner);
	}
	return _getIntegerValue(success);
}
/**
 * @brief returns the value of the variable with name \p _variableNameValue in the current execution environment
 * 
 * @param _variableNameValue 
 * @return Mvalue* the value of the variable with name \p _variableNameValue in the current execution environment
 */
Mvalue* Mget(Mvalue* _variableNameValue){Mallocationowner owner=getOwner(__LINE__);
	if(_variableNameValue){
		Mstring* _variableName=owned_string(_getValueText(_variableNameValue,true),owner);
		if(_variableName){
			Mvalue* _get=getValue(getExecutionEnvironment(),string(_variableName));
			FREE_STRING(_variableName,owner);
			return _get;
		}
	}
	// by default return the value of the last command execution
	return getValue(getExecutionEnvironment(),"");
}

/**
 * @brief sets the current locale to the locale name in \p _localeValue
 * 
 * @param _localeValue 
 * @return Mvalue* M_TRUE on successfully changing the locale, and M_FALSE or M_LL_INVALID on failure
 */
Mvalue* Msetlocale(Mvalue* _localeValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(_localeValue!=NULL&&_localeValue->type==VT_TEXT){
		result=M_FALSE;
		char* locale=setlocale(LC_ALL,_localeValue->value._text->_c);
		if(locale!=NULL){
			// OOPS we can't NULL the immutable locale settings variable, we can only ask for updating the map
			// by NULLing the localesettingsVariable localeValue (as kept by Mlocale.c) will have a zero count, well hopefully)
			// NOTE if the user assigns it to another variable, the map will get copied and not referenced, so the count won't be incremented on the original!
			if(!updateLocalesettingsMap()) // replacing: !setVariable(NULL,M_LOCALE_SETTINGS_VARIABLE_NAME,NULL)||!setVariable(NULL,M_LOCALE_SETTINGS_VARIABLE_NAME,getLocaleValue()))
				outputError("Failed to update the locale settings");
			else
				result=M_TRUE; // success
			/* replacing:
			Mstring* _locale=owned_string(_getString("'"),owner); // free asap
			if(_locale){
				if(string_append(_locale,locale)){
					// NOTE convenient to use getlocale() instead of locale (although they should be the same)
					localeValue=_getTextValue(string(_locale));
					if(localeValue){
						if(!setVariable(NULL,M_LOCALE_SETTINGS_VARIABLE_NAME,localeValue)){
							output("%sFailed to save the new current locale '%s'.\n",M_ERROR_PREFIX,locale);
							localeValue=NULL; // TODO would this be OK?
						}
					}else
						output("%sFailed to create the value of new current locale '%s'.",M_ERROR_PREFIX,locale);
				}else
					outputError("Failed to store the locale text");
				FREE_STRING(_locale,owner);
			}else
				outputError("Failed to create the locale text");
			*/
		}else
			outputError("Proposed new locale not accepted");
	}else
		outputError("Proposed locale not of type text");
	return _getIntegerValue(result);
}
/**
 * @brief returns the current locale
 * 
 * @return Mvalue* the current locale
 */
Mvalue* Mlocalesettings(){Mallocationowner owner=getOwner(__LINE__);
	Mvariable* localeVariable=getVariable(getExecutionEnvironment(),M_LOCALE_SETTINGS_VARIABLE_NAME,false);
	if(localeVariable!=NULL)return localeVariable->_value;
	output("%sVariable %s not found",M_ERROR_PREFIX,M_LOCALE_SETTINGS_VARIABLE_NAME);
	return NULL;
}

// these internal functions do NOT have a body as M defined functions have...
/**
 * @brief registers all internal functions in M environment \p _environment
 * 
 * @param _environment 
 * @param owner_environment 
 * @return true on success
 * @return false on failure
 */
bool registerInternalFunctions(Menvironment* const _environment,Mallocationowner owner_environment){

	if(!completedValueFunction(_environment,owner_environment,"get",Mget))return false;
	if(!completedValueValueFunction(_environment,owner_environment,"set",Mset))return false;

	if(!completedValueFunction(_environment,owner_environment,"setlocale",Msetlocale))return false;
	if(!completedFunction(_environment,owner_environment,"localesettings",Mlocalesettings))return false;

	// variable functions

	// MDH@02NOV2020: random functions
	if(!completedFunction(_environment,owner_environment,"rand",Mrand))return false;
	if(!completedValueFunction(_environment,owner_environment,"rands",Mrands))return false;
	if(!completedValueFunction(_environment,owner_environment,"srand",Msrand))return false;
	// TODO can we have an completedInteger and completedIntegerInteger function here????
	if(!completedValueFunction(_environment,owner_environment,"irand",Mirand))return false;
	if(!completedValueValueFunction(_environment,owner_environment,"irands",Mirands))return false;

	// math functions
	if(!completedFloatFunction(_environment,owner_environment,"cos",Mcos))return false;
	if(!completedFloatFunction(_environment,owner_environment,"cordiccos",Mcordiccos))return false;
	if(!completedFloatFunction(_environment,owner_environment,"sin",Msin))return false;
	if(!completedFloatFunction(_environment,owner_environment,"cordicsin",Mcordicsin))return false;
	if(!completedFloatFunction(_environment,owner_environment,"tan",Mtan))return false;
	if(!completedFloatFunction(_environment,owner_environment,"cosh",Mcosh))return false;
	if(!completedFloatFunction(_environment,owner_environment,"sinh",Msinh))return false;
	if(!completedFloatFunction(_environment,owner_environment,"tanh",Mtanh))return false;
	if(!completedFloatFunction(_environment,owner_environment,"sqrt",Msqrt))return false;
	if(!completedFloatFunction(_environment,owner_environment,"log",Mlog))return false;
	if(!completedFloatFunction(_environment,owner_environment,"log10",Mlog10))return false;
	if(!completedFloatFunction(_environment,owner_environment,"floor",Mfloor))return false;
	if(!completedFloatFunction(_environment,owner_environment,"trunc",Mtrunc))return false;
	if(!completedFloatFunction(_environment,owner_environment,"round",Mround))return false;
	if(!completedFloatFunction(_environment,owner_environment,"ceil",Mceil))return false;
	if(!completedFloatFunction(_environment,owner_environment,"exp",Mexp))return false;
	if(!completedFloatFunction(_environment,owner_environment,"dexp",Mdexp))return false;
	// MDH@04NOV2019: settype now has 3 arguments the last one being the immutable flag
	if(!completedValueFunction(_environment,owner_environment,"type",Mtype))return false;
	if(!completedValueTextValueFunction(_environment,owner_environment,"settype",Msettype))return false;
	if(!completedFloatFloatFunction(_environment,owner_environment,"pow",Mpow))return false;

	if(!completedFunction(_environment,owner_environment,"break",Mbreak))return false;
	if(!completedValueFunction(_environment,owner_environment,"return",Mreturn))return false;

	if(!completedValueFunction(_environment,owner_environment,"out",Mout))return false;

	if(!completedThreeIntegersFunction(_environment,owner_environment,"brgb",Mbrgb))return false;
	if(!completedThreeIntegersFunction(_environment,owner_environment,"trgb",Mtrgb))return false;
	return true;
}/* VALIDATED */

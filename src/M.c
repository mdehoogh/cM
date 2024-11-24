// MDH@13APR2022: for use with CMake defining the major and minor version (and patch)
#include "MConfig.h"
 
// remove the line below when not in debug mode
/////#define __DEBUG__
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <locale.h>

// MDH@27FEB2020: on top of environment management we have the 'shell' for setting up the root M environment
#include "M.h"

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_MAIN,id};}

extern unsigned long long M_MODULE_DEBUGGING;

extern char const * const M_MODULE_DEBUG_CHARACTERS; // MDH@05DEC2020: as defined in Mmodule.h

// the constants are defined in Mshell.c
extern const char* const M_ERROR_PREFIX;
//extern const char* const INFO_PREFIX; // MDH@27FEB2020: as for now NO actual info prefix text to use
extern const char* const M_WARNING_PREFIX; // used in Mexecution.c as well (defined there as extern!!!)
extern const char* const M_BUG_PREFIX; // MDH@05NOV2019: for reporting bugs
extern const char* const M_RESULT_PREFIX; // MDH@28AUG2024: for reporting command results
extern const char* const M_MESSAGE_PREFIX; // MDH@10SEP2024

extern const char M_WHITESPACE_CHARACTER; // MDH@31OCT2019: let's use another character for storing whitespace in tokens (would normally be a blank)
extern const char M_NEWLINE_CHARACTER; // MDH@31OCT2019: the character to request a newline with!!!
extern const char* const M_VARIABLE_NAME; // MDH@14NOV2019: the variable to hold the list of remembered commands and the results they evaluated to
extern const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES];
extern const char INPUTCHARACTERTYPES[];
extern const char* const MFUNCTION_NAME; // the text to represent values that are undefined...
extern const char* const DOFUNCTION_NAME;
extern const char* const FORFUNCTION_NAME;
extern const char* const FORWITHFUNCTION_NAME; // MDH@23DEC2020: this used to be the original for function in that it creates a temporary environment to execute in
extern const char* const DEFINEUSERFUNCTION_NAME;
extern const char* const DEFINEANONYMOUSFUNCTION_NAME; // MDH@08FEB2020
extern const char* const M_NULL_VALUE_TEXT; // the text to represent values that are undefined...
extern const char* const M_NULL_VARIABLE_NAME;
extern const char* const M_UNDEFINED_VALUE_TEXT; // the text to represent values that are undefined...
extern const char* const M_UNDEFINED_VARIABLE_NAME;
extern long long M_DP; // the default decimal precision (initially 20) TODO should this be a constant after all?????????
extern const unsigned long long M_BITS_PER_ENV_LEVEL; // the minimum is 4 (to allow for a depth of 15 environments at the same time), the maximum is 60 of course in which case the maximum depth is 1, 8 gives a maximum depth of 7 and 256 at each level
extern const Mvalue* NULL_value;
extern const long long M_LL_INVALID;
extern const long long M_TRUE;
extern const long long M_FALSE;
extern const long long M_LIST_ELEMENTS_AT_START;
extern const long long M_LIST_ELEMENTS_AT_END;
extern const long long M_ARRAY_ELEMENTS_AT_START;
extern const long long M_ARRAY_ELEMENTS_AT_END;
extern const char M_DEREFERENCE_CHARACTER; // MDH@10MAR2020: defined in Mshell.c
extern const char M_PROPERTY_SEPARATOR_CHARACTER; // MDH@12MAR2020: defined in Mshell.c
extern const int8_t * const BLOCK_FLAGS; // MDH@18MAR2024

char const * const M_VERSION="v" M_VERSION_MAJOR "." M_VERSION_MINOR "." M_VERSION_PATCH;
char const * const M_BUILD=M_VERSION_BUILD;
char const * const M_DATE=M_VERSION_DATE;
char const * const M_TIMESTAMP=M_BUILD_TIMESTAMP;
/* replacing: 
char const * const M_VERSION="0.1.5"; // MDH@05DEC2020: this is were Mexpression is renamed to Mtoken
char const * const M_BUILD="10";char const * const M_DATE="20 October 2021"; // using getSubcommandValue() on the first do and for function arguments which should evaluate to a map containing the local variables to use in the remaining arguments
*/
//char const * const M_BUILD="9";char const * const M_DATE="25 February 2021"; // using getSubcommandValue() on the first do and for function arguments which should evaluate to a map containing the local variables to use in the remaining arguments
//char const * const M_BUILD="8";char const * const M_DATE="24 February 2021"; // 
//char const * const M_BUILD="7";char const * const M_DATE="28 December 2020"; // updating while(), for(), introducing with(), end(), fixing setValue (Menvironment module)
//char const * const M_BUILD="6";char const * const M_DATE="23 December 2020"; // updating while(), for(), introducing with(), end(), fixing setValue (Menvironment module)
//char const * const M_BUILD="5";char const * const M_DATE="15 December 2020"; // adding a VT_DATE for handling dates (see Mdate.c/h)
//char const * const M_BUILD="4";char const * const M_DATE="14 December 2020"; // adding a VT_DATE for handling dates (see Mdate.c/h)
//char const * const M_BUILD="3";char const * const M_DATE="8 December 2020"; // adding a VT_DATE for handling dates (see Mdate.c/h)
//char const * const M_BUILD="2";char const * const M_DATE="7 December 2020"; // with Mlocale.c/h to be able to get/set the locale
//char const * const M_BUILD="1";char const * const M_DATE="5 December 2020"; // MDH@24NOV2020: timsort and harmonica sort 'i' and 'b' variants

//char const * const M_VERSION="0.1.4"; // the new version with file access capabilities (as of 28 September 2020)
//char const * const M_BUILD="21";char const * const M_DATE="3 December 2020"; // MDH@24NOV2020: timsort and harmonica sort 'i' and 'b' variants
//char const * const M_BUILD="20";char const * const M_DATE="24 November 2020"; // MDH@24NOV2020: copy of array on assign (assignValue), binary operator on array/list combinations
//char const * const M_BUILD="19";char const * const M_DATE="23 November 2020"; // MDH@17NOV2020: array data type added
//char const * const M_BUILD="18";char const * const M_DATE="17 November 2020"; // MDH@17NOV2020: yes the bug (setting one list element to many in the stack and so writing outside the reserved dynamic memory) was fixed, by harmonica binary sort still way too slow
//char const * const M_BUILD="17";char const * const M_DATE="16 November 2020"; // MDH@16NOV2020: addressing a serious memory bug in sorting using lharmonicabinarysort
//char const * const M_BUILD="15";char const * const M_DATE="5 November 2020"; // MDH@05NOV2020: assignValue() changed to only copy maps and lists when currently bounded somehow
//char const * const M_BUILD="14";char const * const M_DATE="3 November 2020"; // MDH@03NOV2020: assignValue() changed to only copy maps and lists when currently bounded somehow
//char const * const M_BUILD="13";char const * const M_DATE="2 November 2020"; // MDH@02NOV2020: wasn't really there though
//char const * const M_BUILD="12";char const * const M_DATE="1 November 2020"; // MDH@23OCT2020: passing the body of function() as list of command texts when creating it!!
//char const * const M_BUILD="11";char const * const M_DATE="28 October 2020"; // MDH@23OCT2020: passing the body of function() as list of command texts when creating it!!
//char const * const M_BUILD="10";char const * const M_DATE="27 October 2020"; // MDH@23OCT2020: allowing the use of the '' automatic result variable in function calls as well
//char const * const M_BUILD="9";char const * const M_DATE="25 October 2020"; // MDH@23OCT2020: allowing the use of the '' automatic result variable in function calls as well
//char const * const M_BUILD="8";char const * const M_DATE="23 October 2020"; // MDH@23OCT2020: trying to find a way to be able to use the last command evalation result, in the process now allowing to use integers into maps (as they can be converted to text to use as attribute key)
//char const * const M_BUILD="7";char const * const M_DATE="22 October 2020"; // MDH@22OCT2020: user functions can now have undefined parameter maps, additional arguments are stored in _ variable, so can be used
//char const * const M_BUILD="6";char const * const M_DATE="21 October 2020"; // MDH@21OCT2020: user function creation debugged, as well as certain disowned stuff for function execution environments and returned rationals
//char const * const M_BUILD="5";char const * const M_DATE="19 October 2020"; // MDH@19OCT2020: fixed assigning to a new property like z.a=12, and replacing newline character to '\n' and removing it on left arrow
//char const * const M_BUILD="4";char const * const M_DATE="16 October 2020"; // MDH@Petra's 56th birthday: taking care of using the Enter key inside a command (differentiating between in string or outside string)
//char const * const M_BUILD="3";char const * const M_DATE="15 October 2020"; // MDH@Petra's 56th birthday: taking care of using the Enter key inside a command (differentiating between in string or outside string)
//char const * const M_BUILD="2a";char const * const M_DATE="13 October 2020"; // MDH@Petra's 56th birthday: taking care of using the Enter key inside a command (differentiating between in string or outside string)
//char const * const M_BUILD="1";char const * const M_DATE="28 September 2020"; // file capabilities

// char const * const M_VERSION="0.1.3"; // the new version with ownership imposed on all dynamic memory allocation (well, almost all)
// char const * const M_BUILD="5";char const * const M_DATE="21 September 2020";
// char const * const M_BUILD="4";char const * const M_DATE="30 June 2020";
// char const * const M_BUILD="3";char const * const M_DATE="17 June 2020";
//char const * const M_BUILD="2";char const * const M_DATE="25 May 2020";
//char const * const M_BUILD="1";char const * const M_DATE="22 May 2020";

// MDH@20APR2020: definitely not the first build but I think I forgot to switch to builds here (as opposed to git branching)
// char const * const M_VERSION="0.1.2";
// char const * const M_BUILD="1";char const * const M_DATE="21 April 2020, 17:00"; // make M to create lists and maps automatically when using indexing to properties and array elements that are not there yet
// char const * const M_BUILD="2";char const * const M_DATE="22 April 2020"; // make M to create lists and maps automatically when using indexing to properties and array elements that are not there yet
// char const * const M_BUILD="3";char const * const M_DATE="02 May 2020"; // dynamic allocation (M_MODULE_DEBUGGING&MM_MAIN) (M_MODULE_DEBUGGING&MM_MAIN)
// char const * const M_BUILD="4";char const * const M_DATE="03 May 2020"; // dynamic allocation (M_MODULE_DEBUGGING&MM_MAIN) (M_MODULE_DEBUGGING&MM_MAIN)
// char const * const M_BUILD="5";char const * const M_DATE="06 May 2020"; // dynamic allocation (M_MODULE_DEBUGGING&MM_MAIN) (M_MODULE_DEBUGGING&MM_MAIN)

//char const * const M_VERSION="0.1.1";
//char const * const M_BUILD="1";char const * const M_DATE="15 November 2019, 18:00";
//char const * const M_BUILD="2";char const * const M_DATE="17 November 2019, 19:00";
//char const * const M_BUILD="3";char const * const M_DATE="18 November 2019, 17:00";
//char const * const M_BUILD="4";char const * const M_DATE="20 November 2019, 14:00";
//char const * const M_BUILD="5";char const * const M_DATE="25 November 2019, 18:00";
//char const * const M_BUILD="6";char const * const M_DATE="27 November 2019, 18:00";
//char const * const M_BUILD="7";char const * const M_DATE="20 Februari 2020, 18:00";
//char const * const M_BUILD="8";char const * const M_DATE="27 Februari 2020, 18:00";
//char const * const M_BUILD="9";char const * const M_DATE="28 Februari 2020, 18:00";
//char const * const M_BUILD="10";char const * const M_DATE="2 March 2020, 12:00";
//char const * const M_BUILD="11";char const * const M_DATE="11 March 2020, 18:00";
//char const * const M_BUILD="12";char const * const M_DATE="12 March 2020, 18:00";
//char const * const M_BUILD="14";char const * const M_DATE="23 March 2020, 18:00"; // introducing PROPERTY token
//char const * const M_BUILD="15";char const * const M_DATE="26 March 2020, 18:00"; // make M to create lists and maps automatically when using indexing to properties and array elements that are not there yet
//char const * const M_BUILD="16";char const * const M_DATE="31 March 2020, 15:00"; // make M to create lists and maps automatically when using indexing to properties and array elements that are not there yet
//char const * const M_BUILD="17";char const * const M_DATE="7 April 2020, 12:00"; // make M to create lists and maps automatically when using indexing to properties and array elements that are not there yet

//char const * const M_VERSION="0.1.0";
//char const * const M_BUILD="1";char const * const M_DATE="21 October 2019, 17:00";
//char const * const M_BUILD="2";char const * const M_DATE="22 October 2019, 12:00";
//char const * const M_BUILD="3";char const * const M_DATE="23 October 2019, 18:00";
//char const * const M_BUILD="4";char const * const M_DATE="24 October 2019, 11:00";
//char const * const M_BUILD="5";char const * const M_DATE="25 October 2019, 16:00"; // managed to get rid of (mostly) all the warnings!!!
//char const * const M_BUILD="6";char const * const M_DATE="26 October 2019, 20:20";
//char const * const M_BUILD="7";char const * const M_DATE="27 October 2019, 12:00";
//char const * const M_BUILD="8";char const * const M_DATE="28 October 2019, 18:00";
//char const * const M_BUILD="9";char const * const M_DATE="30 October 2019, 18:00";
//char const * const M_BUILD="10";char const * const M_DATE="31 October 2019, 12:00";
//char const * const M_BUILD="11";char const * const M_DATE="4 November 2019, 22:00";
//char const * const M_BUILD="12";char const * const M_DATE="5 November 2019, 18:00";
//char const * const M_BUILD="14";char const * const M_DATE="6 November 2019, 16:00";
//char const * const M_BUILD="15";char const * const M_DATE="9 November 2019, 22:00";
//char const * const M_BUILD="16";char const * const M_DATE="11 November 2019, 14:00";
//char const * const M_BUILD="17";char const * const M_DATE="14 November 2019, 21:00"; // adding the M variable and M() and variables() function 
//char const * const M_BUILD="18";char const * const M_DATE="15 November 2019, 14:00"; // keeping track of the amount of memory used by the 'managed' (M) types

/**
 * @brief the text to use for inserting the previous command result
 * 
 */
static char const * const GET_CALL_CHARACTERS="get()";

// used externally
//Mvaluetype={VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_FLOAT,VT_TEXT,VT_LIST,VT_MAP}
// the list of token type ids in the corresponding order!!!
/**
 * @brief returns the foreground color string of token type \p tokenType
 * 
 * @param tokenType 
 * @return const char* the foreground color string of token type \p tokenType
 */
static const char* getTokenColor(enum TOKENTYPE_ENUM tokenType){
	//if(tokenType==TT_COMMENT)return getCommentColor();
	uint8_t tokentype_id=TOKENTYPE_IDS[tokenType];
	////////output("(%d)",tokentype_id);
	switch(tokentype_id>>6){
		case 0: // value token
			return getValueTokenColor(tokentype_id);
		case 1: // operator: unary, binary, ternary, assignment the operator category will be: (tokentype_id&0x30)>>4
			return getOperatorTokenColor((tokentype_id&0x30)>>4);
		case 2: // comment or end of comment
			return getCommentColor();
		case 3: // error token
			/////////outputChar('E');
			return getErrorColor();
	}
	outputError("No token color");
	return "";
}
/**
 * @brief outputs the background and foreground color associated with token type \p tokenTye
 * 
 * @param tokenType 
 */
static void outputTokenTypeColor(TokenType tokenType){
	setBackColor(getBackgroundColor());
	setColor(getTokenColor(tokenType));
}
/**
 * @brief outputs the background and foreground colors of token \p _token
 * 
 * @param _token 
 */
static void outputTokenColor(Mtoken* _token){
	if(_token)outputTokenTypeColor(_token->type);
	///////printf("[%d]",_userInputCommand->_lastToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
/*
// I guess we could allow the user to specify another eps value through the QEPS command line argument!!!
static void outputCommandInfo(Mcommand* command){
	if(!command||!command->_lastToken)return;
	// MDH@12AUG2019: identifiers first
	Mtoken* identifierToken=command->_lastToken->prevIdentifier;
	if(identifierToken){
		output("%s","Identifiers:");
		while(1){
			//if(identifierToken==TT_VARIABLE||identifierToken==TT_NEW_VARIABLE){
				// all identier tokens with argument equal to 1 should be considered new, if not it is a bug
				if(identifierToken->argument==1&&identifierToken->type!=TT_NEW_VARIABLE)setColor(getErrorColor());else outputTokenColor(identifierToken);
				output(" %s",string(identifierToken->text));
				resetOutputColor();
				output("(%u)",identifierToken->offset);
			//}
			identifierToken=identifierToken->prevIdentifier;
			if(!identifierToken)break;
		}
		outputChar('\n');
	}
	// tokens
	Mtoken* token=command->_firstToken;
	uint16_t tokenIndex=0;
	output("%s:\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t\t\t%s\n","Tokens","#","OFFSET","USED","LENGTH","ARG","ENV DEPTH/INDEX","TYPE","TEXT");
	while(token!=NULL){
		tokenIndex++;
		output("%u\t%u\t%u\t%u\t%" PRId32 "\t%x/%x\t\t%-24s`%s`",tokenIndex,token->offset,getTokenSignificantCharacterCount(token),string_length(token->text),token->argument,(token->envid&15),(token->envid>>4),TOKENTYPE_STRING[token->type],string(token->text));
		if(token->expr)
			output("\n%s\t%u\t%s\t%s\t%-24s\n"," part of",token->expr->offset,"","",TOKENTYPE_STRING[token->expr->type]);
		else
			output("\t%s\n","Not part of another expression!");
		if(token->prevIdentifier)
			output("%s\t%u\t%s\t%s\t%-24s\n"," points to",token->prevIdentifier->offset,"","",TOKENTYPE_STRING[token->prevIdentifier->type]);
		token=token->next;
	}
}
*/
/**
 * @brief writes the current time stamp to file \p _file
 * @param _file
 */
void writeTimestamp(FILE* _file){if(!_file)return;Mallocationowner owner=getOwner(__LINE__);
	Mstring* _timestamp=owned_string(_getTimestamp(NULL),owner);
	if(NULL==_timestamp)return;
	fprintf(_file,"%s\t",string(_timestamp));
	FREE_STRING(_timestamp,owner);
	/* replacing:
	time_t now=time(NULL);
	struct tm * nowlocal=localtime(&now);
	char buffer[50];
	strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",nowlocal);
	fprintf(_file,"%s\t",buffer);
	*/
}

/**
 * @brief the file to contain debuf information
 * 
 */
FILE* debugfile=NULL;
#include <stdarg.h>
#ifdef __GNUC__
	__attribute__((format(printf, 1, 2)))
#endif
/**
 * @brief outputs all arguments using format string \p fmt to the debug file
 * 
 * @param fmt 
 * @param ... 
 */
void debugWrite(const char* fmt,...){
	if(NULL==debugfile){
		debugfile=fopen("./Mdebug.txt","a+t"); // append (or create) in text mode
		fputc('\n',debugfile); // start with a single empty line (separating the sessions)
	}
	if(debugfile!=NULL){
		va_list args;
		va_start(args,fmt);
		writeTimestamp(debugfile);		
		vfprintf(debugfile,fmt,args);
		fputc('\n',debugfile);
		fflush(debugfile);
		va_end(args);
	}
}

/*
Mvalueunion* getMNumberValueunion(Mnumber* pMnumber){
	if(!pMnumber)return NULL;
	Mvalueunion* pMnumberValueunion=(Mvalueunion*)malloc(sizeof(Mvalueunion));
	if(pMnumberValueunion){pMnumberValueunion->n=pMnumber;} // store the Mnumber instance
	return pMnumberValueunion; // redirection operator
}
bool initVariable(Menvironment* _Menvironment,char* name,double d){
	Mvalueunion* pMValueunion=getMNumberValueUnion(getDoubleMnumber(d)); // dynamically allocated value union (i.e. on the heap)
	return setVariableValue(addVariable(_Menvironment,name,VT_NUMBER),*pMValueunion); // pass the union itself (can you actually assign a union???)
}
*/
// there will be a root (M) environment
/**
 * @brief the (root) M environment (as returned by the call to shellInitialized)
 * 
 */
Menvironment* _Menvironment=NULL; // this is the root (M) environment that we will be executing in
///// NOT HERE see Mexecution.c!!!! Menvironment* _executionEnvironment=NULL; // the current execution environment (in which functions are called!!!)

/* some prototypes we need in initEnvironment()
Mtoken* _getToken(Mtoken* prevToken,TokenType newTokenType);
Mvalue* Miffunction(Mvalue* _conditionTokenValue,Mvalue* _thenTokenValue,Mvalue* _elseTokenValue);
Mvalue* Mwhilefunction(Mvalue* _conditionTokenValue,Mvalue* _whilebodyTokenValue);
Mvalue* Mdofunction(Mvalue* _doTokenValue);
Mvalue* Mforfunction(Mvalue* _initializationTokenValue,Mvalue* _conditionTokenValue,Mvalue* _incrementTokenValue,Mvalue* _forbodyTokenValue);
Mvalue* Mevalfunction(Mvalue* value);
*/

// user interaction stuff
// #include "Msession.h"
/*
char* _getFormattedText(uint8_t maxlength,char* fmt,...){
	char* str=malloc(maxlength+1); // get enough room on the heap
	if(str){va_list args;va_start(args,fmt);sprintf(str,fmt,args);va_end(args);}
	return str;
}
*/
/* source: https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value
#ifdef DBL_DECIMAL_DIG
  #define OP_DBL_Digs (LDBL_DECIMAL_DIG)
#else  
  #ifdef DECIMAL_DIG
	#define OP_DBL_Digs (LDECIMAL_DIG)
  #else  
	#define OP_DBL_Digs (LDBL_DIG + 3)
  #endif
#endif
*/

/**
 * @brief returns a new M string containing the text representaion of M function map \p _functionMap
 * 
 * @param _functionmap 
 * @return Mstring* a new M string containing the text representaion of M function map \p _functionMap
 */
Mstring* _getFunctionMapText(Mfunctionmap* _functionmap){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _s=owned_string(__string(),owner);
	if(_s!=NULL){
		Mstring* p=string_append_char(_s,'[');
		if(_functionmap!=NULL){
			///printf("\n%s(%d)",string(p),_functionmap->numberOfFunctions);
			Mfunctionmapelement* _functionmapelement=_functionmap->_first;
			while(_functionmapelement!=NULL){
				///printf("\n%s","start");
				Mfunction* _function=_functionmapelement->_function;
				if(_function!=NULL){
					Mstring* _functionText=owned_string(_getFunctionText(_function,string(_functionmapelement->_name)),owner);
					if(_functionText!=NULL){
						p=string_append(p,string(_functionText));
						FREE_STRING(_functionText,owner);
					}
					/* replacing:
					///printf("\n%s","func");
					p=string_append(p,string(_functionmapelement->_name)); // _name moved from _function to _functionmapelement
					if(NULL==p)break;
					///printf("\n%s","name");
					// I guess we might show the parameter map (if any)
					string_append_char(p,'(');
					if(_function->_parameterMap!=NULL){
						Mstring* parameterMapText=owned_string(_getMapText(_function->_parameterMap,false,false,false),owner); // do NOT show curly braces, quotes or missing defaults
						if(parameterMapText!=NULL){
							string_append(p,string(parameterMapText));
							FREE_STRING(parameterMapText,owner);
						}
					}
					///printf("\n%s","params");
					string_append_char(p,')');
					*/
				}
				_functionmapelement=_functionmapelement->_next;
				if(_functionmapelement!=NULL)p=string_append(p,", "); // only when there's a next map element to process
				///printf("\n%s","next");
			}
		}
		//printf("\n%s(%d)",string(p),string_length(p));
		p=string_append_char(p,']');
		///printf("\n%s",string(p));
		// if we failed, we have to free s here!!!
		if(NULL==p){FREE_STRING(_s,owner);return NULL;}
	}
	return disowned_string(_s,owner);
}
/**
 * @brief outputs the function map of the current execution M environment
 * 
 */
void outputFunctions(){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _functionsText=owned_string(_getFunctionMapText(getExecutionEnvironment()->_functionMap),owner);
	if(!_functionsText){outputError("Failed to create the output. Possible cause: out of memory.");return;}
	output("\nFunctions: %s.\n",string(_functionsText));
	FREE_STRING(_functionsText,owner);
}/* VALIDATED */
/**
 * @brief outputs the variables of the current execution M environment
 * 
 */
void outputVariables(){Mallocationowner owner=getOwner(__LINE__);
	// much easier now that we get the text of any Mvalue (like the variable map of an environment!)
	// MDH@24OCT2019: now using _getVariableMapText() instead of _getMapText() because the former is environment aware and can show the symbols with the same value (if any)
	Mstring* _variablesText=owned_string(_getVariableMapText(getExecutionEnvironment(),false,false,true,false),owner); // do NOT show the hidden variables!!!
	if(!_variablesText){outputError("Failed to create the output. Possible cause: out of memory.");return;}
	output("\nVariables: %s.\n",string(_variablesText));
	FREE_STRING(_variablesText,owner);
}/* VALIDATED */
/**
 * @brief the enum of possible input modes: command, control or shell
 * 
 */
enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell
/**
 * @brief the actual (current) input mode
 * 
 */
enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode
/**
 * @brief the prompt info for each possible input mode
 * 
 */
char* promptinfo[]={"Command mode: clear the command with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Use history command|Verbose|Wrap - Options: Beep|Reset|eXit|Functions|History|Shell.","Shell mode: enter a system command to execute."};
/**
call prompt() when ready to receive a new command
 */
// MDH@16OCT2020 not used anymore: const char OPTION_CHAR='`'; // TODO should this character become part of options????

// USER INPUT STUFF

/* TODO are we using the storeCursor() and restoreCursor() sometime?
// VT100 codes...
void storeCursor(){printf("\0337");}
void restoreCursor(){printf("\0338");}
*/

/**
 * @brief outputs the display (settings) flags
 * 
 */
void displayFlags(){
	output("Edit flags: %c%c%c%c%c - Display flags: %c%c.\n",amAssisting()?'A':'a',amVerboseDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amAcceptinghistorycommand()?'U':'u',amWrapping()?'W':'w',48+getColorscheme());
}
/**
 * @brief outputs all setting flag characters
 * 
 */
void outputFlags(){
	output("%c%c%c%c%c%c%c",amAssisting()?'A':'a',(48+getColorscheme()),amVerboseDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amWrapping()?'W':'w',amAcceptinghistorycommand()?'U':'u');
}

// the user input (either the shell command or the M user input command)
/**
 * @brief the current shell command (in command mode)
 * 
 */
Mstring* _shellCommand=NULL;Mallocationowner owner_shellCommand=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief the current user input command (in command mode)
 * 
 */
Mcommand* _userInputCommand=NULL;Mallocationowner owner_userInputCommand=(Mallocationowner){MI_MAIN,__LINE__,1}; // the current input command

// keeping track of both the cursor position and the total command length
/**
 * @brief returns the length of the current (shell or user input) command
 * @details enters 0 when in control mode or when in command mode there is no user input command or token
 * @return size_t the length of the current (shell or user input) command
 */
size_t getUserInputLength(){
	if(inputMode==IM_COMMAND)
		return(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL?_userInputCommand->_lastToken->offset+string_length(_userInputCommand->_lastToken->text):0);
	if(inputMode==IM_SHELL)
		return string_length(_shellCommand);
	return 0;
}

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText
// MDH@30OCT2019: if we let toInfoInputLine() return the number of lines it moved back we can pass that into toUserInputCursorPosition() to go down that number of lines
// MDH@30OCT2019: for each user input line, keep track of the total number of characters written by the user
/**
 * @brief the record with information on a single user inout line (in command mode)
 * 
 */
typedef struct Muserinputline{
	size_t offset,index; // the number of characters on previous lines and the line index
	struct Muserinputline *_prev; // for accessing previous lines
}Muserinputline;
/**
 * @brief the current user input line pointer
 * 
 */
static Muserinputline* _userinputline=NULL;static Mallocationowner owner_userinputline=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief returns the number of lines showing the user input command
 * 
 * @return size_t the number of lines showing the user input command
 */
static size_t getNumberOfCommandLines(){return(_userinputline!=NULL?_userinputline->index:0)+1;}
// MDH@24SEP2020: unless we're at the end of a command getUserInputLength() would be the current offset BUT if a command is being output completely that would be an invalid assumption therefore we now pass in the offset to use
/**
 * @brief returns a new user input line starting at offset \p offset in the current user input command
 * 
 * @param offset 
 * @return Muserinputline* a new user input line starting at offset \p offset in the current user input command
 */
static Muserinputline* __userinputline(size_t offset){Mallocationowner owner=getOwner(__LINE__);
	Muserinputline* _newUserinputline=CALLOC_1(sizeof(Muserinputline),'6',owner);
	if(_newUserinputline!=NULL){
		_newUserinputline->_prev=_userinputline;
		_newUserinputline->offset=offset; // this should be the total number of characters in the command on previous input lines
		_newUserinputline->index=getNumberOfCommandLines(); // count the lines
		_userinputline=OWNED(DISOWNED(_newUserinputline,owner),owner_userinputline); // MDH@30JUN2020: pass ownership when assigning to the global _userinputline because then it it bound!!!!
	}
	// TODO what if this fails????
	return _newUserinputline;
}
// call free_userinputline() when starting a new user input command
/**
 * @brief frees the current (last) user input line
 * 
 * @return size_t the number of user input lines
 */
static size_t free_userinputline(){
	size_t numberOfUserInputLines=0;
	Muserinputline* prevUserinputline;
	while(_userinputline!=NULL){
		numberOfUserInputLines++;
		prevUserinputline=_userinputline->_prev;
		FREE_DISOWNED_1(_userinputline,'6',owner_userinputline);
		_userinputline=prevUserinputline;
	}
	return numberOfUserInputLines;
}
// MDH@24SEP2020: if a user input line is removed, we return the difference between the removed line offset and the new line offset, which essentially is the number of command characters on the new command line
/**
 * @brief removes the current user input line
 * 
 * @return size_t the length of the current user input line
 */
static size_t removeUserinputline(){
	if(NULL==_userinputline)return 0;
	size_t offset=_userinputline->offset;
	Muserinputline* prevUserinputline=_userinputline->_prev;
	FREE_DISOWNED_1(_userinputline,'6',owner_userinputline);
	_userinputline=prevUserinputline;
	return offset-(_userinputline!=NULL?_userinputline->offset:0);
}
// MDH@30OCT2019 END
/**
 * @brief moves the user input cursor to the info input line (the line in front of the command prompt)
 * 
 * @return size_t the number of lines moved up
 */
size_t toInfoInputLine(){
	size_t linesUp=0,lines=getNumberOfCommandLines(); // ASSERT lines should be at least 1
	while(linesUp<lines){oneLineUp();linesUp++;}clearLine();// (M_MODULE_DEBUGGING&MM_MAIN): output("[%zd]",lines);
	return linesUp;
} // MDH@30OCT2019: only after moving all the input lines up do we need to go to the start, also clearLine() will ascertain to end up at the start of the line
// output functions that require access to the current token
/**
 * @brief outputs the user input last command token colors
 * 
 */
void outputUserInputCommandTokenColor(){
	if(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL)outputTokenColor(_userInputCommand->_lastToken); // return to the current token color
}
/**
 * @brief the prompt length
 * 
 */
uint8_t promptLength=0;
/**
 * @brief the number of characters on the current user input command line
 * 
 */
size_t numberOfLineCommandCharacters=0; // the number of command characters on the current command line
/**
 * @brief returns the cursor to the current user command line input position
 * 
 * @return size_t the number of user input command line characters
 */
size_t returnToUserInputCommandCursorPosition(){
	// ASSERT we're on the last user input line i.e. the input line the user is currently entering command characters
	toStartOfLine();
	size_t numberOfInputCommandLineCharacters=numberOfLineCommandCharacters; // replacing: =getUserInputLength()-(_userinputline?_userinputline->offset:0);
	// size_t debugInfoLength=output(" %zd ",numberOfInputCommandLineCharacters);moveCursorRight(promptLength+numberOfInputCommandLineCharacters-debugInfoLength); // the offset of the current user input line (if any) determines how many characters the user typed on this input line
	moveCursorRight(promptLength+numberOfInputCommandLineCharacters);
	outputUserInputCommandTokenColor();
	// output("{%zd}",numberOfLineCommandCharacters);
	return numberOfInputCommandLineCharacters;
}
// MDH@15OCT2020: changed to also be able to move up (which we need after showing the suggested text)
/**
 * @brief returns the cursor to the position of user input after moving to the user input info line moving up \p lines
 * 
 * @param lines the number of lines to move down again
 * @return size_t the number of user input command line characters
 */
size_t toUserInputCursorPosition(long lines){
	while(lines!=0){if(lines>0){oneLineDown();lines--;}else{oneLineUp();lines++;}} // replacing: while(linesDown>0){oneLineDown();linesDown--;}
	return returnToUserInputCommandCursorPosition();
}
// MDH@28FEB2020: define inputInfo/inputError as static because Mshell.c also has functions with this name (as defaults to inputInfo/inputError)
/**
 * @brief writes \p ... using format \p fmt in the default input color on the user input info line
 * 
 * @param fmt 
 * @param ... the arguments to write
 */
static void inputInfo(const char* const fmt,...){
	// STUPID ME!!!!! return;
	if(fmt!=NULL&&strlen(fmt)>0){ // we have a format
		// outputChar('X');
		size_t linesMovedUp=toInfoInputLine();
		resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		// MDH@22JUL2019: as we're not calling output() here, we can make output() read a character to allow interuption????
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toUserInputCursorPosition(linesMovedUp); // back to where we came from
	}
}
/**
 * @brief writes \p ... using format \p fmt in the error color on the user input info line
 * 
 * @param fmt 
 * @param ... 
 */
static void inputError(const char* const fmt,...){
	if(fmt!=NULL&&strlen(fmt)>0){ // we have a format
		size_t linesMovedUp=toInfoInputLine();
		setColor(getErrorColor());setBackColor(getBackgroundColor());
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toUserInputCursorPosition(linesMovedUp);
	}
}

// FEED FORWARD STUFF
// manual feed forward characters stuff
// what the user consumed manually, and is supposed to remain continguous i.e. uninterrupted by other feed forward texts
// it's possible that manual feed forward text is empty so it will block the identifier continuation text when that is the case!!
/**
 * @brief the global manual feed forward M string
 * 
 */
Mstring* _manualFeedforwardText=NULL;Mallocationowner owner_manualFeedforwardText=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief deletes the manual feed forward text
 * 
 */
void deleteManualFeedforwardText(){
	// ASSERT _manualFeedforwardText not NULL
	FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;
}
/**
 * @brief the number of identifier continuation feed forward characters
 * 
 */
size_t numberOfIdentifierContinuationManualFeedforwardCharacters=0; // MDH@06OCT2019: determine the number of manual feed forward characterr matching the identifier continuation
// the following two functions are called only once!!!
/**
 * @brief prepends character \p c to the manual feedforward text
 * 
 * @param c 
 * @return true on success
 * @return false on failure
 */
bool manualFeedforwardCharacterPrepended(char c){
	if(!c)return false;
	if(NULL==_manualFeedforwardText){_manualFeedforwardText=owned_string(__string(),owner_manualFeedforwardText);
	if(NULL==_manualFeedforwardText)return false;}
	return(string_insert_char(_manualFeedforwardText,0,c)!=NULL);
}
/**
 * @brief prepends C string \p characters to the manual feed forward text
 * 
 * @param characters 
 * @return true on success
 * @return false on failure
 */
bool prependedToManualFeedforwardText(char const * const characters){
	if(NULL==characters)return false;
	if(NULL==_manualFeedforwardText){_manualFeedforwardText=owned_string(__string(),owner_manualFeedforwardText);
	if(NULL==_manualFeedforwardText)return false;}
	return(string_prepend(_manualFeedforwardText,characters)!=NULL); // MDH@05DEC2022 preferably compare to NULL
}
/**
 * @brief removes and returns the first manual feed forward text character
 * 
 * @return char the first manual feed forward text character
 */
char getFirstManualFeedforwardCharacterRemoved(){
	// ASSERT _manualFeedforwardText should not be NULL
	//if(_manualFeedforwardText){
	char c=string_removed_char(_manualFeedforwardText,0);
	if(c&&string_length(_manualFeedforwardText)==0)deleteManualFeedforwardText();
	//}
	return c;
	/* replacing: return(_manualFeedforwardText?string_removed_char(_manualFeedforwardText,0):'\0'); */
}

// identifier continuation stuff
// Mcommand stuff moved over to Mshell

// MDH@28OCT2019: not needed here anymore... Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the sequence of tokens starting with _userInputCommand->_firstToken
// MDH@02OCT2019: might need this in multiple places!!
// MDH@04NOV2019: added TT_REFERENCE tokens as well
// MDH@15SEP2023: inIndentifierToken is only called from updateLastTokenIdentifierContinuation and should include property completion as well
//                so adding TT_PROPERTY
/**
 * @brief returns true if \p lastCommandToken is an identier token, false otherwise
 * 
 * @param lastCommandToken 
 * @return true if \p lastCommandToken is an identifier token
 * @return false if \p lastCommandToken is not an identifier token
 */
bool inIdentifierToken(Mtoken* lastCommandToken){
	return(lastCommandToken!=NULL?lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_PROPERTY||lastCommandToken->type==TT_FUNCTION||lastCommandToken->type==TT_NEW_VARIABLE||lastCommandToken->type==TT_REFERENCE:false);
}

/* MDH@28OCT2019 replacing: 
Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the command input by the user
*/

/**
 * @brief whether or not the identifier has changed
 * 
 */
bool userInputCommandIdentifierContinuationNeedsUpdating=false; // whether or not the identifier has changed
/**
 * @brief the identifier continuation C string characters
 * 
 */
char* _identifierContinuationCharacters=NULL; // the single text that we can continue the current identifier token with
/**
 * @brief returns the first identifier continuation character
 * 
 * @return char 
 */
char getFirstIdentifierContinuationCharacter(){
	inputInfo("%s","Determining the first identifier continuation character!");
	return(_identifierContinuationCharacters!=NULL?_identifierContinuationCharacters[0]:'\0');
}

/**
 * @brief removes the first identifier continuation character
 * 
 * @return char 
 */
char firstIdentifierContinuationCharacterRemoved(){
	char identifierContinuationCharacterToRemove=(_identifierContinuationCharacters!=NULL?_identifierContinuationCharacters[0]:'\0');
	if(identifierContinuationCharacterToRemove){ // something to remove i.e. the length is at least 1
		if(strlen(_identifierContinuationCharacters)>1){ // something left over
			char* newIdentifierContinuationText=strdup(_identifierContinuationCharacters+1); // get dynamic copy of remainder, ignore if we fail!!!
			if(newIdentifierContinuationText!=NULL){ // the part to replace created!!
				free(_identifierContinuationCharacters);
				_identifierContinuationCharacters=newIdentifierContinuationText;
			}else // failure
				identifierContinuationCharacterToRemove='\0';
		}else
			_identifierContinuationCharacters[0]='\0';
	}
	return identifierContinuationCharacterToRemove;
}
/* replacing:
bool deleteFirstIdentifierContinuationCharacter(){
	if(!_identifierContinuationCharacters)return false;
	// not undefined or empty...
	char* newIdentifierContinuationText=(strlen(_identifierContinuationCharacters)>1?strdup(_identifierContinuationCharacters+1):NULL); // get dynamic copy of remainder, ignore if we fail!!!
	free(_identifierContinuationCharacters);
	_identifierContinuationCharacters=newIdentifierContinuationText; // NOTE if we failed to copy the remainder, we get NULL now
	return true;
}
*/

/**
 * @brief the set of characters expected next in continuing the current identifier token
 * 
 */
char* _identifierContinuationOptionalCharacters=NULL; // the set of characters expected next of existing identifiers
// MDH@26SEP2019: we create a separate method that will determine the last token identifier continuation text and continuation characters
/**
 * @brief deletes the identifier continuation C string
 * 
 */
void deleteIdentifierContinuation(){
	if(_identifierContinuationOptionalCharacters!=NULL){free(_identifierContinuationOptionalCharacters);_identifierContinuationOptionalCharacters=NULL;}
	if(_identifierContinuationCharacters!=NULL){free(_identifierContinuationCharacters);_identifierContinuationCharacters=NULL;}
}
// user input command specific

// MDH@20SEP2019: we used to keep track of the behind cursor text, in a single Mstring instance, but because we also want to be able to add variable completion we keep a sequence of char* 
//				each feed forward char* is associated with a single token, and if that token is removed so should the associated feed forward
// call deleteTokenautocompletiontexts whenever the behind cursor text changes
/**
 * @brief storing the auto completion text associated with a given token
 * 
 */
typedef struct Mtokenautocompletiontext{
	Mtoken* token;
	Mchars* _text; // MDH@23APR2020: replacing char*
	struct Mtokenautocompletiontext* _next;
	///////bool inactive; // keep track of whether or not active... (a feed forward text can become inactive when the associated token itself is still around but the text was moved to the command with a left or right arrow key)
}Mtokenautocompletiontext;
// MDH@30SEP2019: all token feed forward texts accepted (i.e. consumed) are pointed to by _lastConsumedAutoCompletiontext
//				consumption of feed forward texts is done by right arrow (one character at a time) or tab (all feed forward characters)
//				right arrow reads the first feed forward character, gets it accepted and then moves the feed forward character to the consumed feed forward texts
/**
 * @brief the pointers to the first and last consumed token auto completion text
 * 
 */
Mtokenautocompletiontext *_firstTokenautocompletiontext=NULL,*_lastConsumedAutoCompletiontext=NULL;Mallocationowner tokenautocompletiontextowner=(Mallocationowner){MI_MAIN,__LINE__,1};
// MDH@26SEP2019: it's prudent to store the identifier continuation text separated from the rest of the (autogenerated) feed forward text
/**
 * @brief returns the auto completion M string text separating the successive token auto completion C strings with character \p sep
 * 
 * @param sep 
 * @return Mstring* the auto completion M string text separating the successive token auto completion C strings with character \p sep
 */
Mstring* _getAutoCompletionText(char sep){Mallocationowner owner=getOwner(__LINE__);
	// constructs the total feed forward text
	Mstring* autoCompletionText=owned_string(__string(),owner);
	if(autoCompletionText!=NULL){
		Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext!=NULL){
			if(tokenautocompletiontext->_text&&!string_append(autoCompletionText,tokenautocompletiontext->_text->chars))break;
			///////if(autocompletiontext->token)if(!string_append_char(_suggestedText,'#'))break;
			if(sep)if(NULL==string_append_char(autoCompletionText,sep))break;
			tokenautocompletiontext=tokenautocompletiontext->_next;
		}
		/* MDH@04OCT2019: moved over to updateAutoCompletionText()
		// MDH@03OCT2019: remove the identifier continuation text from the start of the feed forward text (as we should)
		if(!sep){
			if(_identifierContinuationCharacters){
				size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
				if(numberOfIdentifierContinuationCharacters>0){
					size_t numberOfFeedforwardCharacters=string_length(_suggestedText);
					if(numberOfIdentifierContinuationCharacters<numberOfFeedforwardCharacters){ // there might be feed forward characters left
						char c=string_replacedchar(_suggestedText,'\0',numberOfIdentifierContinuationCharacters); // temporarily pretend the feed forward text to have the same length as the identifier continuation text
						bool matching=(strcmp(string(_suggestedText),_identifierContinuationCharacters)==0); // get the comparison result
						string_setchar(_suggestedText,c,numberOfIdentifierContinuationCharacters); // // put the removed character back BEFORE removing the identifier continuation when matching
						if(matching) // the same!!!
							if(string_removed(_suggestedText,0,numberOfIdentifierContinuationCharacters)!=numberOfIdentifierContinuationCharacters) // remove the identifier continuation from the feed forward text
								inputError("Failed to remove the identifier continuation (completely) from the start of the rest of the suggested text");
					}else
					if(numberOfIdentifierContinuationCharacters==numberOfFeedforwardCharacters) // 'remove' all feed forward characters
						string_setlength(_suggestedText,0);
				}
			}
		}
		*/
	}
	return disowned_string(autoCompletionText,owner);
}
/**
 * @brief the number of characters written behind the prompt
 * 
 */
size_t numberOfBehindPromptCharactersWritten=0; // MDH@25SEP2019: the total number of text characters written behind the prompt

// the globally constructed behind cursor text (without separator!!!)
/**
 * @brief the M string holding the auto completion text
 * 
 */
Mstring* _autoCompletionText=NULL; // MDH@27FEB2019: we keep track of the auto completion text
Mallocationowner owner_autoCompletionText=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief invalidates the auto completion text
 * 
 */
void invalidateAutoCompletionText(){
	if(_autoCompletionText!=NULL){
		if(amVerboseDebugging())
			inputInfo("Deleting autocompletion text.");
		FREE_STRING(_autoCompletionText,owner_autoCompletionText); // MDH@19MAY2020: TODO should we disown ->chars before calling free_string????
		_autoCompletionText=NULL;
	}else
	if(amVerboseDebugging())
		inputInfo("No auto completion text to delete.");
}
/**
 * @brief updates the auto completion text from its constituent feed forward parts
 * 
 */
void updateAutoCompletionText(){
	invalidateAutoCompletionText();
	_autoCompletionText=owned_string(_getAutoCompletionText('\0'),owner_autoCompletionText); // get the new characters
	// merge with identifier continuation text
	if(_autoCompletionText!=NULL){
		if(_identifierContinuationCharacters!=NULL){
			size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
			if(numberOfIdentifierContinuationCharacters>0){
				size_t numberOfFeedforwardCharacters=string_length(_autoCompletionText);
				if(numberOfIdentifierContinuationCharacters<numberOfFeedforwardCharacters){ // there might be feed forward characters left
					char c=string_replacedchar(_autoCompletionText,'\0',numberOfIdentifierContinuationCharacters); // temporarily pretend the feed forward text to have the same length as the identifier continuation text
					bool matching=(strcmp(string(_autoCompletionText),_identifierContinuationCharacters)==0); // get the comparison result
					string_setchar(_autoCompletionText,c,numberOfIdentifierContinuationCharacters); // // put the removed character back BEFORE removing the identifier continuation when matching
					if(matching) // the same!!!
						if(string_removed(_autoCompletionText,0,numberOfIdentifierContinuationCharacters)!=numberOfIdentifierContinuationCharacters) // remove the identifier continuation from the feed forward text
							inputError("Failed to remove the identifier continuation (completely) from the start of the rest of the suggested text");
				}else
				if(numberOfIdentifierContinuationCharacters==numberOfFeedforwardCharacters) // 'remove' all feed forward characters
					string_setlength(_autoCompletionText,0/*,owner_autoCompletionText*/);
			}
		}
		logToOutputFile("\t\tAuto completion text: '%s'.\n",string(_autoCompletionText));
	}
}

// MDH@04DEC2023: 
/* see _expectedCharacterStack MDH@04DEC2023: we're going to collect the enders in a single Mstring
Mstring* _enders=NULL;
Mallocationowner owner_enders=(Mallocationowner){MI_MAIN,__LINE__,1};
*/
/**
static char getTokenTypeFeedforwardCloser(TokenType tokenType){
	switch(tokenType){
		case TT_DQSTRING:return '"'; 
		// MDH@20DEC2022: take out the function and function call token feed forward out here, which is now being taken care of in determining the immediate feed forward text!!!
		case TT_FUNCTION_CALL:return ')';
		case TT_LIST:return ']';
		case TT_MAP:return '}';
		case TT_SQSTRING:return '\'';
		default:break;
	}
	return '\0';
}
*/
// MDH@25MAY2023: we can use it again
// MDH@20DEC2022: apparently not getting called anywhere anymore
static char getTokenTypeFeedforwardCharacter(TokenType tokenType){
	// some token types have an associated feed forward character!!!
	switch(tokenType){
		case TT_BINARY_aErU:return '=';
		case TT_DQSTRING:return '"'; 
		// MDH@20DEC2022: take out the function and function call token feed forward out here, which is now being taken care of in determining the immediate feed forward text!!!
		case TT_FUNCTION:return '(';
		case TT_FUNCTION_CALL:return ')';
		case TT_LIST:return ']';
		case TT_MAP:return '}';
		case TT_NEW_VARIABLE:return '=';
		case TT_SQSTRING:return '\'';
		default:break;
	}
	return '\0';
}
static char firstCommandClosingCharacter='\0';
static void updateTheFirstCommandClosingCharacter(){
	firstCommandClosingCharacter='\0';
	Mtoken* lastCommandToken=(_userInputCommand!=NULL?_userInputCommand->_lastToken:NULL);
	if(lastCommandToken!=NULL&&isTokenFinished(lastCommandToken)){ // the last command token is finished
		firstCommandClosingCharacter=getTokenTypeFeedforwardCharacter(lastCommandToken->type);
		// even if there's a first command closing character, it might not be allowed if the current token
		if(firstCommandClosingCharacter){
			inputInfo("First command closing character: '%c'.",firstCommandClosingCharacter);
		}else{
			inputInfo("No first command closing character!");
		}
	}
}

// MDH@24SEP2019: we do not always want to set the identifier continuation characters
// MDH@03OCT2019: now excluding the last token immediate feed forward text because that is being taken care of whenever the last token (type) change
//				because of this only the matching parentheses feed forward characters remain so TODO simplify this
/**
 * @brief returns the C string with the auto completion text associated with the last user input command token
 * 
 * @return char* the C string with the auto completion text associated with the last user input command token
 */
static char* _getLastTokenAutoCompletionText(){// Mallocationowner owner=getOwner(__LINE__);
	// MDH@20DEC2022: changed from "" to " ", as this is a bit dangerous since this way we get a 1-character string
	char *tokenAutoCompletionText=" "; // on the stack (zero-terminated string)
	if(amMatchingparentheses())
	if(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL)
	switch(_userInputCommand->_lastToken->type){
		case TT_ASSIGNMENT:break;
		case TT_BINARY_AeRu:case TT_BINARY_Aeru:case TT_BINARY_aERu:break;
		// MDH@03OCT2019: case TT_BINARY_aErU:tokenFeedforwardText="=";break;
		case TT_BINARY_aeru:break;
		case TT_COMMENT:break;
		// MDH@03OCT2019: case TT_DQSTRING:tokenFeedforwardText="\"";break; // TODO using " for the double quoted string might change in the future and we'd be in trouble then
		case TT_END_OF_DQSTRING:break; /////tokenAutoCompletionText="\"";break; // MDH@04DEC2023: moved here
		case TT_END_OF_SQSTRING:break; /////tokenAutoCompletionText="'";break; // MDH@04DEC2023: moved here
		case TT_END_OF_FUNCTION_CALL:case TT_END_OF_LIST:case TT_END_OF_MAP:break;
		case TT_ERROR:break;
		case TT_EXPRESSION:if(string_last_char(_userInputCommand->_lastToken->text)=='(')tokenAutoCompletionText=")";break; // TODO use other type e.g. TT_FUNCTION_CALL instead of TT_EXPRESSION on (
		// MDH@03OCT2019: case TT_FUNCTION:tokenFeedforwardText="(";break;
		case TT_FUNCTION_CALL:tokenAutoCompletionText=")";break;
		case TT_INTEGER:break;
		case TT_LIST:tokenAutoCompletionText="]";break;
		case TT_LISTELEMENT:break;
		case TT_MAP:tokenAutoCompletionText="}";break;
		case TT_MAP_VALUE:break;
		// MDH@27SEP2019: if there's an identifier continuation behind the name of a new variable, the user can't reach it, so appending = is not going to be a good idea
		//				however, this would require the identifier continuation text to be updated BEFORE calling this method
		// MDH@01OCT2019: it's probably better to leave the assignment operator character showing to indicate that the current identifier is new, the user can always delete the identifier continuation using Delete key
		// MDH@03OCT2019: case TT_NEW_VARIABLE:/*if(!_identifierContinuationCharacters)*/tokenFeedforwardText="=";break;
		case TT_REAL:break;
		// MDH@03OCT2019: case TT_SQSTRING:tokenFeedforwardText="'";break; // TODO using ' for the double quoted string might change in the future and we'd be in trouble then
		case TT_TERNARY_aeru:break;
		case TT_UNARY:break;
		case TT_VARIABLE:case TT_REFERENCE:
		case TT_PROPERTY: // MDH@23MAR2020
		default:break;
	}
	// MDH@25MAY2020: by returning a dynamic duplicate the caller needs to free it
	return(tokenAutoCompletionText[0]!=' '?strdup(tokenAutoCompletionText):NULL); // TODO I suppose we might decide to no longer create a copy on the heap here (due to separating identifier continuation text from other feed forward text)
}
/*
Mtokenautocompletiontext* disowned_tokenautocompletiontext(Mtokenautocompletiontext* _autocompletiontext,Mallocationowner owner_autocompletiontext){
	if(!_autocompletiontext)return NULL;
	disowned_tokenautocompletiontext(_autocompletiontext->_next,owner_autocompletiontext);
	if(_autocompletiontext->_text)disowned_chars(_autocompletiontext->_text,owner_autocompletiontext); // MDH@02MAY2020: switching to freeChars() given that _getChars() was used to create it
	return DISOWNED(_autocompletiontext,owner_autocompletiontext);
}
*/
Mallocationowner owner_tokenAutoCompletionTexts=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief frees auto completion text \p _autocompletiontext and all its successors
 * @param _autocompletiontext the auto completion text to free
 */
void free_tokenautocompletiontext(Mtokenautocompletiontext* _autocompletiontext/*,Mallocationowner owner_autocompletiontext*/){
	if(NULL==_autocompletiontext)return;
	if(_autocompletiontext->_next!=NULL)free_tokenautocompletiontext(_autocompletiontext->_next/*,owner_autocompletiontext*/);
	if(_autocompletiontext->_text!=NULL)FREECHARS(_autocompletiontext->_text,owner_tokenAutoCompletionTexts); // MDH@02MAY2020: switching to freeChars() given that _getChars() was used to create it
	FREE_DISOWNED_1(_autocompletiontext,'7',owner_tokenAutoCompletionTexts); // MDH@07APR2020: _autocompletiontext is an Mstring* so release as 'S'
}

// MDH@04OCT2019: if we remember the immediate feed forward token we can determine whether or not we need to remove the associated feed forward text
/**
 * @brief the immediate feed forward token
 * 
 */
Mtoken* immediateFeedforwardToken=NULL;
/**
 * @brief deletes the token auto completion texts
 * 
 */
void deleteTokenautocompletiontexts(){
	invalidateAutoCompletionText();
	////////if(amVerboseDebugging())inputInfo("AutoCompletion text deleted.");
	/////////////////numberOfBehindPromptCharactersWritten=getCommandLength(); // MDH@25SEP2019: TODO if you know a better place to do this then here let me know
	free_tokenautocompletiontext(_firstTokenautocompletiontext);
	_firstTokenautocompletiontext=NULL; // OOPS pretty essential!!!!
	immediateFeedforwardToken=NULL; // MDH@04OCT2019: also pretty essential as we won't have a feed forward text with this token anymore
	if(amVerboseDebugging())inputInfo("Token autocompletion texts deleted.");
}
/*
void deleteTokenautocompletionCharacters(size_t numberOfTokenAutoCompletionCharacters){

}
*/
// every time feed forward text is to be added, it is prepended to the list of feed forward texts setting the token pointer to _userInputCommand->_lastToken
/**
 * @brief returns the auto completion text associated with token \p token (if any)
 * @param token 
 * @return Mtokenautocompletiontext* the auto completion text associated with token \p token
 */
Mtokenautocompletiontext* getTokenAutoCompletionText(Mtoken* token){
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext!=NULL&&tokenautocompletiontext->token!=token)
		tokenautocompletiontext=tokenautocompletiontext->_next;
	return tokenautocompletiontext;
}
// _text is dynamically allocated and should be freed if not bound to some!!
// MDH@24SEP2019: whenever the user uses the left arrow to move characters out of the token into the feed forward text
//				it should be remembered that these characters were associated with this token
//				so that when the feed forward text for a given token is set
//				the removed token characters and the actual feed forward characters of the token can be combined
// MDH@25MAY2020: _text changed to _chars (assumed to be a const character array owned somewhere else that we completely leave alone, that's what we can do with completely constant pointers)
/**
 * @brief set the current user input command last token auto completion text to C string \p _chars
 * 
 * @param _chars 
 * @return char* \p _chars
 */
char* setLastTokenAutoCompletionText(char const * const _chars){Mallocationowner owner=getOwner(__LINE__);
	// do not prepend empty feed forward texts!!!
	if(_chars!=NULL){
		// MDH@01OCT2019: BUG FIX it would be wrong to NOT update the last token feed forward text when no text is specified as we were doing
		// MDH@02OCT2019: BUG FIX if _text is empty and there is no token yet do NOT add the token
		/////////// removing: if(strlen(_text)>0){ // there's text to store
		// if we already have an autogenerated feed forward text associated with the last command token
		Mtokenautocompletiontext* lastTokenAutoCompletionText=getTokenAutoCompletionText(_userInputCommand->_lastToken);
		if(lastTokenAutoCompletionText!=NULL){ // yes, so replace the text contents
			if(lastTokenAutoCompletionText->_text!=NULL)FREECHARS(lastTokenAutoCompletionText->_text,owner_tokenAutoCompletionTexts);
			invalidateAutoCompletionText();
			lastTokenAutoCompletionText->_text=owned_chars(_getChars(_chars),Msubowner(owner_tokenAutoCompletionTexts,1)); // replacing: _getChars(_text,foid); // _text now bound!! // MDH@23APR2020 NO not bound, as _text replaced by _getChars(_text)
		}else{ // not present yet, so add (i.e. prepend!!)
			if(strlen(_chars)>0){
				if(amVerboseDebugging())
					inputInfo("Prepending auto completion text '%s' of token '%s' with offset %zu.",_chars,string(_userInputCommand->_lastToken->text),_userInputCommand->_lastToken->offset);
				Mtokenautocompletiontext* _tokenautocompletiontext=CALLOC_1(sizeof(Mtokenautocompletiontext),'7',owner_tokenAutoCompletionTexts);
				if(_tokenautocompletiontext!=NULL){
					invalidateAutoCompletionText(); // I guess this is a bit confusing
					_tokenautocompletiontext->_text=owned_chars(_getChars(_chars),Msubowner(owner_tokenAutoCompletionTexts,1)); // MDH@23APR2020: not bound because _getChars() copies the text as well
					_tokenautocompletiontext->token=_userInputCommand->_lastToken;
					// how about skipping all anonymous feed forward texts????
					// if the first one is anonymous mark that one as last anonymous feed forward text
					Mtokenautocompletiontext *lastAnonymousTokenautocompletiontext=(_firstTokenautocompletiontext!=NULL&&NULL==_firstTokenautocompletiontext->token?_firstTokenautocompletiontext:NULL);
					// if the successor is also anonymous increment
					while(lastAnonymousTokenautocompletiontext!=NULL&&lastAnonymousTokenautocompletiontext->_next!=NULL&&NULL==lastAnonymousTokenautocompletiontext->_next->token)
						lastAnonymousTokenautocompletiontext=lastAnonymousTokenautocompletiontext->_next;
					if(lastAnonymousTokenautocompletiontext!=NULL){
						_tokenautocompletiontext->_next=lastAnonymousTokenautocompletiontext->_next;
						lastAnonymousTokenautocompletiontext->_next=_tokenautocompletiontext;
					}else{
						_tokenautocompletiontext->_next=_firstTokenautocompletiontext;
						_firstTokenautocompletiontext=_tokenautocompletiontext;
					}
					if(amVerboseDebugging())
						inputInfo("Feed forward text '%s' of token '%s' prepended!",_chars,string(_userInputCommand->_lastToken->text));
				}else
					inputInfo("Failed to prepend feed forward text '%s' of token '%s'.",_chars,string(_userInputCommand->_lastToken->text));
			}
		}
	}
	return _chars; // MDH@25MAY2020 convenient to return the input (see updateLastTokenAutoCompletionText())
	// MDH@25MAY2020 no need to do the following anymore: freeChars(_text,owner_tokenAutoCompletionTexts); // will only succeed if currently disowned
}

// MDH@01OCT2019: in general when a token is removed its associated (autogenerated) suggested (identifier continuation and feed forward text) should be removed as well
//				NOTE invalidateAutoCompletionTextOfToken delegates to invalidateAutoCompletionTextOfToken
/**
 * @brief deletes the auto completion text associated with token \p token
 * 
 * @param token 
 * @return true on success
 * @return false on failure
 */
bool deleteTokenAutoCompletionText(Mtoken* token){
	// locate the (autogenerated) feed forward text associated with this token (most likely the first one)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext!=NULL&&tokenautocompletiontext->token!=token){
		prevtokenautocompletiontext=tokenautocompletiontext;
		tokenautocompletiontext=prevtokenautocompletiontext->_next;
	}
	if(tokenautocompletiontext!=NULL){ // found
		if(prevtokenautocompletiontext!=NULL)
			prevtokenautocompletiontext->_next=tokenautocompletiontext->_next;
		else 
			_firstTokenautocompletiontext=tokenautocompletiontext->_next;
		invalidateAutoCompletionText(); // the feed forward text changed so needs to be reconstructed whenever it is to be shown
		tokenautocompletiontext->_next=NULL;
		free_tokenautocompletiontext(tokenautocompletiontext); // only free the token feed forward text we are to remove!!
	} // MDH@04OCT2019: it's NOT there, so we may consider it deleted
	// relink the rest of the feed forward text chain to skip this feed forward text
	// if we have a previous feed forward text make if point to the successor of the feed forward text we are now removing, otherwise we get a new first feed forward text
	return true;
}

/*
void removeFirstFeedforwardCharacterFromLastTokenWhenMatching(char inputChar){
	// find the last token's feed forward text (if any)
	Mtokenautocompletiontext* _autocompletiontext=_firstTokenautocompletiontext;while(_autocompletiontext&&_autocompletiontext->token!=_userInputCommand->_lastToken)_autocompletiontext=_autocompletiontext->_next;
	if(!_autocompletiontext)return;
	char* p=_autocompletiontext->_text;
	if(!p)return; // if the pointer to the text is undefined, we don't have it
	// NOTE no need to test the length because inputChar typically will not equal '\0'
	if(*p!=inputChar)return; // if the character pointed to by the pointer does not match the input character nothing to do
	// we'll be cutting off the first character!!
	deleteTokenautocompletiontexts();
	char* _leftover=strdup(p+1); // make a dynamic copy starting at the second character!
	free(p);
	_autocompletiontext->_text=_leftover;
}
*/
/* MDH@02NOV2021: for now we return to calling firstAutoCompletionCharacterRemoved instead of delete..., and taking the first character of _autoCompletionText instead of actually calling the following function
// MDH@30SEP2019: here the problem is that if we remove the first feed forward character we loose the token reference that generated it
//				so we might split up this functionality in two parts: 1. get the first feed forward character 2. remove it either by consuming it or delete if it disappears completely
//				alternatively we can simply remove the character but not remove the feed forward text when it becomes empty, that way we can still consume the feed forward text
//				however, suppose accepting this character as command characters fails, it would make sense to NOT actually remove the feed forward character until after accepting it?????????
//				I suppose the initial solution should be to extract the character and once used successfully remove it
//				so, replacing getFirstAutoCompletionCharacterRemoved() by getFirstAutoCompletionCharacter() and a function to actually delete that first feed forward character (and either consume or delete it)
char getFirstAutoCompletionCharacter(bool autogenerated){
	// if autogenerated is set any first character should be returned, if not, the first feed forward character should be in a feed forward text associated with a token (i.e. autogenerated)
	// most of time _firstTokenautocompletiontext will contain that first feed forward character
	Mtokenautocompletiontext *tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){
		if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text->chars[0]!='\0')return(!autogenerated||tokenautocompletiontext->token?tokenautocompletiontext->_text->chars[0]:'\0');
		tokenautocompletiontext=tokenautocompletiontext->_next; // get the next to check
	}
	return '\0';
}
bool deleteFirstAutoCompletionCharacter(char firstAutoCompletionCharacter,bool consumed){
	if(firstAutoCompletionCharacter!='\0'){
		// find it
		Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext){
			if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text->chars[0]==firstAutoCompletionCharacter)break;
			prevtokenautocompletiontext=tokenautocompletiontext; // remember the current feed forward text as the previous feed forward text
			tokenautocompletiontext=prevtokenautocompletiontext->_next; // get the next to check
		}
		if(tokenautocompletiontext){ // got it
			invalidateAutoCompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
			// NOTE currently the length of the feed forward text will ALWAYS be 1 but if that would not always be the case we would need to consider addressing that happening as well
			Mchars* p=tokenautocompletiontext->_text;
			size_t l=(p?strlen(p->chars):0);
			if(l==1){
				if(consumed){ // i.e. we should remember the token text, so we can unconsume it
					Mtokenautocompletiontext* nextFirstTokenautocompletiontext=NULL;
					while(_firstTokenautocompletiontext){
						nextFirstTokenautocompletiontext=_firstTokenautocompletiontext->_next; // remember what the next first token feed forward text will become
						// make the current first token feed forward text point to what the current last consumed token feed forward text is
						_firstTokenautocompletiontext->_next=_lastConsumedAutoCompletiontext;
						// set the last consumed token feed forward text to the current first token feed forward text
						_lastConsumedAutoCompletiontext=_firstTokenautocompletiontext;
						// update the first token feed forward text to what it was originally pointing to
						_firstTokenautocompletiontext=nextFirstTokenautocompletiontext;
						// if the token feed forward text with the first feed forward character was consumed, we're done
						if(_lastConsumedAutoCompletiontext==tokenautocompletiontext)break;
					}
				}else{ // a true delete
					// replace the current text by what's behind the first character (if any)
					tokenautocompletiontext->_text=(l>1?owned_chars(_getChars(p->chars+1),Msubowner(owner_tokenAutoCompletionTexts,1)):NULL);
					FREECHARS(p,owner_tokenAutoCompletionTexts); // free the currently used dynamic memory still pointed to by p
					if(!tokenautocompletiontext->_text){ // nothing left (or failing to copy the remainder over)
						Mtokenautocompletiontext* nexttokenautocompletiontext=tokenautocompletiontext->_next; // remember where to link the previous to
						tokenautocompletiontext->_next=NULL;free_tokenautocompletiontext(tokenautocompletiontext); // get rid of the feed forward text
						// link the predecessor to the successor
						if(prevtokenautocompletiontext)
							prevtokenautocompletiontext->_next=nexttokenautocompletiontext;
						else
							_firstTokenautocompletiontext=nexttokenautocompletiontext;
					}
				}
			}else{
				// TODO implement this when this might become the case in the future
			}
			return true;
		}
		inputError("First suggested character %c vanished.",firstAutoCompletionCharacter);
	}else
		inputError("No first suggested character to remove.");
	return false;
}
*/
// MDH@30SEP2019: TODO still using the following for true feed forward deletes but should in due course be replaced by using the above two methods
// MDH@02NOV2021: apparently NOT replacing firstAutoCompletionCharacterRemoved()!!!!
/**
 * @brief returns the first auto completion character removed
 * 
 * @return char the auto completion text associated with token \p token
 */
char firstAutoCompletionCharacterRemoved(){
	if(amVerboseDebugging())inputInfo("Determining the first auto completion character!");
	char autocompletionCharacterRemoved='\0';
	// find first feed forward text with text (so skipping all without text)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext!=NULL&&(NULL==tokenautocompletiontext->_text||tokenautocompletiontext->_text->chars[0]=='\0'))
	{prevtokenautocompletiontext=tokenautocompletiontext;tokenautocompletiontext=prevtokenautocompletiontext->_next;}
	if(tokenautocompletiontext!=NULL){
		Mchars* p=tokenautocompletiontext->_text;
		if(p!=NULL){
			size_t l=strlen(p->chars); // the number of autocompletion characters
			if(l>0){ // there is a first autocompletion character
				autocompletionCharacterRemoved=p->chars[0];
				invalidateAutoCompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
				if(amVerboseDebugging())inputInfo("First auto completion character '%c'.",autocompletionCharacterRemoved);
				// replace the current text by what's behind the first character (if any)
				tokenautocompletiontext->_text=(l>1?owned_chars(_getChars(p->chars+1),Msubowner(owner_tokenAutoCompletionTexts,1)):NULL);
				FREECHARS(p,owner_tokenAutoCompletionTexts); // free the currently used dynamic memory still pointed to by p // MDH@11JUN2020: i.e. freeChars called on the disowned Mchars*
				if(NULL==tokenautocompletiontext->_text){ // nothing left (or failing to copy the remainder over)
					Mtokenautocompletiontext* nexttokenautocompletiontext=tokenautocompletiontext->_next; // remember where to link the previous to
					tokenautocompletiontext->_next=NULL;
					free_tokenautocompletiontext(tokenautocompletiontext); // get rid of the feed forward text
					// link the predecessor to the successor
					if(prevtokenautocompletiontext!=NULL)
						prevtokenautocompletiontext->_next=nexttokenautocompletiontext;
					else 
						_firstTokenautocompletiontext=nexttokenautocompletiontext;
				}
			}else
				inputError("No auto completion characters!");
		}else
			inputError("No auto completion text!");
	}else
		inputError("No auto completion text found!");	
	return autocompletionCharacterRemoved;
}
/*
// a character can be 'anonymously' prepended to the feed forward text
// BUT if it matches the feed forward text of the current token and the current token does not have a feed forward text yet, it shouldn't be anonymous!!!
// MDH@24SEP2019: NO not anonymous because the character was removed from the current token and should be remembered as such
//				however this cause a problem if the originating token actually is the feed forward character of another token
// MDH@30SEP2019: now we should unconsume this character if it was last consumed
// MDH@04SEP2019: immediate feed forward characters should ALWAYS be prepended explicitly and so true is to be passed in for \p new
//				NOTE currently there are only two calls, one to prepend an immediate feed forward character (which should always be flagged as new), and left arrow prepending which is marked as false
Mtokenautocompletiontext*  getAutoCompletionTextOfCharacterPrepended(char c,bool new){Mallocationowner owner=getOwner(__LINE__);
	Mtokenautocompletiontext* result=NULL;
	if(c){
		// possible recently consumed...
		// TODO technically we should check the last character in last consumed feed forward text (instead of assuming every feed forward text consists of a single character!!)
		if(!new&&_lastConsumedAutoCompletiontext&&_lastConsumedAutoCompletiontext->_text&&_lastConsumedAutoCompletiontext->_text->chars[0]==c){
			Mtokenautocompletiontext* nextLastConsumedTokenautocompletiontext=_lastConsumedAutoCompletiontext->_next; // remember what the last consumed token feed forward text is pointing to
			_lastConsumedAutoCompletiontext->_next=_firstTokenautocompletiontext; // make the last consumed token feed forward text to the current first token feed forward text
			_firstTokenautocompletiontext=_lastConsumedAutoCompletiontext; // replace the first token feed forward text by the last consumed token feed forward text
			_lastConsumedAutoCompletiontext=nextLastConsumedTokenautocompletiontext; // replace the last consumed token feed forward text by what it was previously pointing to
			result=_lastConsumedAutoCompletiontext;
		}else{
			// TODO for now always use an anonymous (nontoken) prepend
			Mtokenautocompletiontext* autocompletiontext=CALLOC(sizeof(Mtokenautocompletiontext),'7',owner);
			if(autocompletiontext){
				Mstring* _string=OWNED(__string(),owner); // free asap
				if(_string){
					string_append_char(_string,c);
					// if(autocompletiontext->_text){
					// 	string_append(_string,autocompletiontext->_text);
					// 	free(autocompletiontext->_text); // get rid of the memory with the original text
					// 	autocompletiontext->_text=NULL; // just in case (of what?)
					// }
					autocompletiontext->_text=SUBOWNED(OWNED(_getChars(string(_string)),owner));
					// free the memory occupied by the vessel
					FREE_STRING(_string,owner);
					// if our feed forward text is not the current first fft make it so
					if(autocompletiontext!=_firstTokenautocompletiontext){
						autocompletiontext->_next=_firstTokenautocompletiontext;
						_firstTokenautocompletiontext=autocompletiontext;
					}
					result=autocompletiontext; // remember the 
					invalidateAutoCompletionText();
				}
				// if failed, ascertain to free what is not bound in the feedforward text list
				if(!result){autocompletiontext->_next=NULL;free_tokenautocompletiontext(autocompletiontext,owner);autocompletiontext=NULL;}
			}
		}
	}
	return DISOWNED(result,owner);
}
*/

// MDH@13JUL2023: keeping track of the closers
// MDH@13DEC2023: now renamed to _expectedCharacterStack indicating that these are the expected closing characters
Mstring* _expectedCharacterStack=NULL;Mallocationowner owner_expectedCharacterStack=(Mallocationowner){MI_MAIN,__LINE__,1};
///////void deleteFeedforwardClosers(){FREE_STRING(_expectedCharacterStack,owner_expectedCharacterStack);_expectedCharacterStack=NULL;}

// MDH@03OCT2019: it's essential to differentiate between current token dependent feed forward and other feed forward
//				deleteLastTokenImmediateFeedforwardText() is to be called when _userInputCommand->_lastToken stops being the current token or when the type of the current token changes
//				updateUserInputCommandImmediateFeedforwardText() is to be called when _userInputCommand->_lastToken just became the current token (or when its type changes)
// MDH@04OCT2019: deciding to keep the immediate feed forward text separate from the other feed forward texts, that way it is easier to merge the identifier continuation and feed forward texts
/**
 * @brief the M string containing the immediate feed forward text
 * 
 */
Mstring* _immediateFeedforwardText=NULL;Mallocationowner owner_immediateFeedforwardText=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief deletes the immediate feed forward text M string
 * 
 */
void deleteImmediateFeedforwardText(){
	FREE_STRING(_immediateFeedforwardText,owner_immediateFeedforwardText);
	_immediateFeedforwardText=NULL;
}
// MDH@05DEC2022 TODO not used anymore?: bool immediateFeedforwardToBeUpdated=false;
// MDH@05DEC2022 NOTE: called once when updating the immediate feed forward text
// MDH@20DEC2022 NOTE: currently determines the single character that is most expected to end the current token, although the assignment operator (=) actually is a feed forward character on a new variable which does
//					 not need to be new at all and could confuse the user
/**
 * @brief returns the user input command immediate feed forward character
 * 
 * @return char the user input command immediate feed forward character
 */
char getUserInputCommandImmediateFeedforwardCharacter(){
	// returns the character that might directly follow the current token (matching parentheses feed forward characters excluded)
	Mtoken* token=(_userInputCommand!=NULL?_userInputCommand->_lastToken:NULL);
	if(token!=NULL&&string_length(token->text)>0)
	switch(token->type){
		case TT_NEW_VARIABLE:case TT_BINARY_aErU:return '='; // start an assignment
		case TT_DQSTRING:return '"'; // end a double quoted string literal
		case TT_FUNCTION:return '('; // start the function call
		case TT_SQSTRING:return '\''; // end a single quoted string literal
		default:{
			/* MDH@16JUL2024: comma's are easy to insert, and get in the way of closing a function call prematurely
			// MDH@20DEC2022: can we deal with adding the end function call argument character? either , or ) here??????
			if(token->expr!=NULL&&token->expr->type==TT_FUNCTION_CALL&&token->type!=TT_END_OF_FUNCTION_CALL&&token->expr->argument<=-4){ 
				// we need to know more, if all the arguments are given we need to predict ), otherwise a , unless the current argument is not yet valid
				//if(!isTokenUnfinished(token)){ // the token may be considered finished
				// MDH@12JUL2023: I think we should not return ')' in any case because ')' is probably already appended
				if(token->expr->argument!=-4)return ',';
				/// replacing:return(token->expr->argument==-4?')':',');
			}
			*/
			break;
		}
	}
	return '\0';
}
// MDH@20DEC2022 NOTE:
// this does NOT take care of the closing ) of a function call, although perhaps it should, we can definitely say that we're missing the , and ) that ends the current argument in a function call
// so what we could do is use this type of feedforward text to take care of doing that, and remove it from the auto completion feedforward that inserts the closing parenthesis of a function call
// the point being that if the argument is now valid we can always start the next argument
/**
 * @brief updates the user input command immediate feed forward text
 * 
 * @return true 
 * @return false 
 */
bool updateUserInputCommandImmediateFeedforwardText(){
	if(NULL==_immediateFeedforwardText)return false; // should have one
	char lastTokenImmediateFeedforwardCharacter=getUserInputCommandImmediateFeedforwardCharacter();
	return(lastTokenImmediateFeedforwardCharacter=='\0'||
					string_append_char(_immediateFeedforwardText,lastTokenImmediateFeedforwardCharacter)!=NULL);
}
/* replacing:
// \brief prepends the immediate feed forward character of the current token (if any), returns true on success, false otherwise
bool updateUserInputCommandImmediateFeedforwardText(){
	// MDH@03OCT2019: getAutoCompletionTextOfCharacterPrepended was adjusted to return true when the character passed to it equals '\0'!!
	//				however TODO currently the prepending is anonymous, whereas this prepending should NOT be done anonymous, otherwise we can't delete it later on
	// get rid of any current immediate feed forward text
	if(immediateFeedforwardToken&&!invalidateAutoCompletionTextOfToken(immediateFeedforwardToken))return false;
	immediateFeedforwardToken=NULL;
	char lastTokenImmediateFeedforwardCharacter=getUserInputCommandImmediateFeedforwardCharacter(_userInputCommand->_lastToken);
	if(!lastTokenImmediateFeedforwardCharacter)return true;
	// try to prepend the character
	Mtokenautocompletiontext* lastTokenImmediateFeedforwardtext=getAutoCompletionTextOfCharacterPrepended(lastTokenImmediateFeedforwardCharacter,true); // second argument forces always prepending this character!!
	if(!lastTokenImmediateFeedforwardtext){inputError("Failed to prepend the immediate feed forward character!");return false;} // failed to prepend the immediate feed forward text anonymously
	immediateFeedforwardToken=_userInputCommand->_lastToken;
	lastTokenImmediateFeedforwardtext->token=immediateFeedforwardToken; // remember the immediate feed forward token!!!
	return true;
}
*/
/*
// the following functions are user input command specific
Mtoken* setLastUserInputCommandToken(Mtoken* lastUserInputCommandToken){
	if(!_userInputCommand){inputError("%sNo user input command.",M_BUG_PREFIX);return NULL;}
	_userInputCommand->_lastToken=lastUserInputCommandToken;
	// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(lastUserInputCommandToken); // MDH@02OCT2019: as we're setting the type of the token AFTER creating it, we wait until after doing so to update identifierContinuationIsDirty!!	
	return _userInputCommand->_lastToken;
}
*/
/*
Mtoken* setLastUserInputCommandToken(Mtoken* newLastCommandToken){
	// MDH@03OCT2019: every time the current command token changes (preferably done by calling this function), we should first remove the immediate feed forward of the current token, update the current token, and add the immediate feed forward text on the newly accepted current token
	//				however this should also happen when the type of the current token changes
	// MDH@04OCT2019: a little less efficient to move it to the input loop but more reliable!!!
	//// removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the last token immediate feed forward text.");
	if(_userInputCommand)_userInputCommand->_lastToken=newLastCommandToken;else inputError("BUG: No user input command");
	//// removing: if(!updateUserInputCommandImmediateFeedforwardText())inputError("Failed to add the last token immediate feed forward text.");
	userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand);
}
*/
/*
void invalidateAutoCompletionTextOfToken(Mtoken* token,bool deleteIdentifierContinuationText){
	if(!token)return;
	if(deleteIdentifierContinuationText)deleteIdentifierContinuation(); // this is the easiest way but makes sense
	invalidateAutoCompletionTextOfToken(token);
}
*/
/**
 * @brief the suggested text (consisting of all feed forward texts)
 * 
 */
Mstring* _suggestedText=NULL;Mallocationowner owner_suggestedText=(Mallocationowner){MI_MAIN,__LINE__,1}; // MDH@04SEP2019: where we'll be storing the entire feed forward text (i.e. identifier continuation, immediate feed forward and auto completion text)

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
/** 
 * @brief all commands entered so far
*/
long long commandCount=0; // the total number of command input
/**
 * @brief the current user input command index
 * 
 */
long long commandIndex=0; // the index of the current command from the end of the command list (stored in commands)


/// MDH@28OCT2019: replaced by _userInputCommand: Mtoken* _userInputCommand->_firstToken=NULL;

/**
 * @brief returns the total number of suggested characters
 * 
 * @return size_t the total number of suggested characters
 */
size_t getTotalNumberOfSuggestedCharacters(){
	// TODO should be computed using suggestedTextSources[] so showSuggestedText can be changed independent of this function
	if(_manualFeedforwardText!=NULL)return string_length(_manualFeedforwardText);
	size_t totalNumberOfSuggestedCharacters=(_identifierContinuationCharacters!=NULL?strlen(_identifierContinuationCharacters):0);
	if(_immediateFeedforwardText!=NULL)totalNumberOfSuggestedCharacters+=string_length(_immediateFeedforwardText);
	if(_expectedCharacterStack!=NULL)totalNumberOfSuggestedCharacters+=string_length(_expectedCharacterStack);
	return totalNumberOfSuggestedCharacters;
}

/**
 * @brief returns the number of suggested text characters in the suggested text source with index \p suggestedTextSourceIndex
 * 
 * @return * long long 
 */
long long getNumberOfSuggestedCharacters(size_t suggestedTextSourceIndex){
	switch(suggestedTextSourceIndex){
		case 1:return(_manualFeedforwardText!=NULL?string_length(_manualFeedforwardText):-1);
		case 2:return(_identifierContinuationCharacters!=NULL?strlen(_identifierContinuationCharacters):-1);
		case 3:return(_immediateFeedforwardText!=NULL?string_length(_immediateFeedforwardText):-1);
		case 4:return(_expectedCharacterStack!=NULL?string_length(_expectedCharacterStack):-1);
	}
	return -2;
	//return(_suggestedText!=NULL?string_length(_suggestedText):0);
}
/**
 * @brief returns the number of characters in the current user input command and the number of suggested characters
 * 
 * @return size_t the number of characters in the current user input command and the suggested text
 */
size_t getCommandLength(){return getUserInputLength()+getTotalNumberOfSuggestedCharacters();} // TODO not correct this way!!!!

// request body of function moved over to Mshell.h/c
/**
 * @brief outputs the current timestamp
 * 
 */
void outputTimestamp(){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _promptTimestamp=owned_string(_getTimestamp(NULL),owner); // should return a disowned timestamp, so we do not need to obtain ownership that we need to detach on calling free_string
	if(NULL==_promptTimestamp)return;
	outputToFile(NULL,string(_promptTimestamp),">\n"); // pass it along to echoToOutputFile to show in front of < that indicates the start of an output fragment
	FREE_STRING(_promptTimestamp,owner);
}

/**
 * @brief the subcommand block level
 * 
 */
size_t blockCommandLevel=0;
/**
 * @brief increments the command count as being shown in the prompt
 * 
 */
void incrementPromptCommandCount(){
	// if inside a function declaration no need to increment anything
	if(blockCommandLevel>0)
		getCurrentBlock()->insertedBlockCommands++;
	else
	if(getCurrentFunctionBodyInput()==NULL)
		getExecutionEnvironment()->commandCount++;
}

size_t getNewPromptCommandIndex(char* environmentName,size_t defaultPromptCommandCount){Mallocationowner owner=getOwner(__LINE__);
	// when inside a block of commands show the block name
	if(blockCommandLevel>0){
		Mstring* _blockName=owned_string(_getBlockName(),owner);
		if(_blockName!=NULL){
			promptLength+=output(string(_blockName));
			FREE_STRING(_blockName,owner);
		}
		if(getCurrentBlock()->subcommandBlockType=='1')return 0; // MDH@16APR2024: if only a single command expected no need to display the command index!!
		return(getCurrentBlock()->insertedBlockCommands+1);
	}
	// MDH@19JUL2019: when dealing with a function body being entered, we show a different prompt
	if(getCurrentFunctionBodyInput()!=NULL&&environmentName!=NULL)
		return getNumberOfFunctionCommands(environmentName)+1;	// replacing: printf("%lu",(commandCount+1));
	return defaultPromptCommandCount+1;
}

// MDH@11AUG2024: if we want to be able to set the message id to the actual prompt we need to collect
//                the prompt characters first
Mstring* prompt=NULL;Mallocationowner owner_prompt=(Mallocationowner){MI_MAIN,__LINE__,1}; // the gobal variable to store the prompt in!!
/**
 * @brief shows the prompt and sets the global prompt length \p promptLength accordingly
 * 
 */
void showPrompt(){Mallocationowner owner=getOwner(__LINE__);
	resetOutputColor();
	numberOfBehindPromptCharactersWritten=0; // MDH@27SEP2019: so far no characters were written behind the prompt
	numberOfLineCommandCharacters=0; // MDH@22JUN2020: here as well as in showContinuedPrompt()
	///////////printf("%d-",commandIndex);
	char str[11]; // with a maximum of 2,xxx,xxx,xxx 11 positions would suffice
	promptLength=0;
	switch(inputMode){
		case IM_COMMAND:
			{
				string_setlength(prompt,0);
				outputTimestamp(); // TODO now obsolete, so to be removed eventually
				echoToOutputFile();
				/* MDH@16APR2024: removed, because we're now using the environment to get at the name, see below
				Mstring* _environmentName=owned_string(_getExecutionEnvironmentName(),owner); // free asap
				if(_environmentName!=NULL){
					output(string(_environmentName));
					promptLength=string_length(_environmentName);
					FREE_STRING(_environmentName,owner);
				}
				*/
				/* replacing:
				output("M");
				promptLength=1;
				*/
				// MDH@09APR2024: we need the environment on multiple occasions
				Menvironment* environment=getExecutionEnvironment();
				Mstring* _environmentName=owned_string(_getExecutionEnvironmentName(),owner);
				/* replacing:
				char* environmentName=(environment!=NULL&&environment->_name!=NULL?environment->_name->chars:NULL);
				*/
				if(_environmentName!=NULL){
					// MDH@11AUG2024: ascertain to append the environment name to the prompt M string
					string_append(prompt,string(_environmentName));
					/*
					if(string_append(prompt,string(_environmentName))!=NULL)
						promptLength=output(string(_environmentName));
						*/
				}
				// MDH@18APR2024: now asking getNewPromptCommandIndex for the command index to use in the prompt
				size_t newPromptCommandIndex=getNewPromptCommandIndex(string(_environmentName),environment->commandCount);
				if(newPromptCommandIndex){
					string_append_char(prompt,'[');
					string_append_ull(prompt,newPromptCommandIndex);
					string_append_char(prompt,']');
					/*
					if(string_append_ull(prompt,newPromptCommandIndex)!=NULL)
						promptLength+=output("[%lu]",newPromptCommandIndex);
					*/
				}
				// MDH@11AUG2024: register the current prompt as the current message id
				promptLength=output("%s",setMessageId(string(prompt))); 
				/* replacing (so we won't need str anymore)
				// when inside a block of commands show the block name
				if(blockCommandLevel>0){
					Mstring* _blockName=owned_string(_getBlockName(),owner);
					if(_blockName!=NULL){
						promptLength+=output(string(_blockName));
						FREE_STRING(_blockName,owner);
					}
					if(getCurrentBlock()->subcommandBlockType!='1') // MDH@16APR2024: if only a single command expected no need to display the command index!!
						sprintf(str,"%lu",(getCurrentBlock()->insertedBlockCommands+1));	// replacing: printf("%lu",(commandCount+1));
					else
						str[0]=0;
				}else
				// MDH@19JUL2019: when dealing with a function body being entered, we show a different prompt
				if(getCurrentFunctionBodyInput()!=NULL&&_environmentName!=NULL)
					sprintf(str,"%lu",1+getNumberOfFunctionCommands(string(_environmentName)));	// replacing: printf("%lu",(commandCount+1));
				else
					sprintf(str,"%lu",environment->commandCount+1);
				if(str[0])promptLength+=output("[%s]",str);
				*/
				if(_environmentName!=NULL)FREE_STRING(_environmentName,owner);
				dontEchoToOutputFile(); // MDH@13MAR2020: not interested in the rest of the prompt just the command we're in
				promptLength+=output("%s"," = ");
				clearScreenFromCursor();
			}
			break;
		case IM_CONTROL:
			// how about showing the flags?????
			outputFlags();
			promptLength+=output(" > ");
			break;
		case IM_SHELL:
			promptLength+=output("$ ");
			break;
	}
	/////////storeCursor();
	///////////inputMode=true; // expecting a command (until the option character is received)
	/* MDH@26FEB2019: we do not need the following because that's taken care of in writeTokens(_userInputCommand->_firstToken) right after promptForUserInput()
	getUserInputLength()=0; // starting at position 0
	*/
}
// MDH@30OCT2019: we'd like to be able to continue a command on the next line
// MDH@21SEP2020: added the newline flag to indicate that a new line character is to be written
// MDH@23SEP2020: now returning the number of prompt characters output
// MDH@24SEP2020: bool notsuggested replaced by offset which is either the number of characters in the command so far (and therefore positive), or indicates that we're not currently writing active command text (e.g. suggested text)
/**
 * @brief shows the prompt continuation
 * 
 * @param offset the number of user input command characters on previous input lines
 * @param newline whether or not this is a new user input line
 * @return unsigned long long the number of prompt continuation characters written
 */
unsigned long long showContinuedPrompt(int64_t offset,bool newline){
	unsigned long long promptCharactersWritten=0;
	if(inputMode==IM_COMMAND){
		// ASSERT only to be called in command mode with _userInputCommand not NULL
		// MDH@20FEB2020: passing getUserInputLength() to set the offset of the new user input line (because I'm the only one who can tell)
		if(offset>=0){
			if(NULL==__userinputline(offset))return 0; // if we fail to create a new user input line (to keep track of the number of characters on previous user input lines)
			numberOfLineCommandCharacters=0; // MDH@26JUN2020: keeping track of the number of command characters on the current user input line
		}
		if(newline){
			clearScreenFromCursor(); // to get rid of any suggested text behind the cursor
			promptCharactersWritten+=outputChar('\n'); // move over to the next line
		}
		// promptCharactersWritten+=output("(%lld)",offset); // (M_MODULE_DEBUGGING&MM_MAIN)
		uint8_t blanks=promptLength;while(blanks>3){promptCharactersWritten+=outputChar(' ');blanks--;}
		resetOutputColor();
		promptCharactersWritten+=output(" %c ",(offset>=0?'=':' ')); // if not suggested write an equal sign, otherwise write a blank (as what's being written is not part of the command yet)
	}else
	if(inputMode==IM_SHELL){
		promptCharactersWritten=output("%c ",(offset>=0?':':' ')); // TODO continuation of $ not certain what to display here but a colon is used often (e.g. in Python)
	}
	return promptCharactersWritten;
	/// can't call this here!!!! outputTokenColor(_userInputCommand->_lastToken);
}

// MDH@24JUN2020: if we know how many characters that can fit on a single user input line we will be able to 
//				determine exactly where the soft-breaks will be
// MDH@26JUN2020: this is a bit hazardous when getCurrentNumberOfWindowTextColumns() returns a value that is below promptLength which is not to be allowed!!!
/**
 * @brief the number of characters available on each input line
 * 
 */
int numberOfLineCharacters=0;
// call initializeNumberOfLineCharacters every time the prompt is shown
/**
 * @brief initializes the total number of line characters
 * 
 */
void initializeNumberOfLineCharacters(){
	int minimumNumberOfLineCharacters=MIN(20,promptLength+10); // what we need at least
	int newNumberOfLineCharacters=getCurrentNumberOfWindowTextColumns();
	if(newNumberOfLineCharacters>0){
		numberOfLineCharacters=newNumberOfLineCharacters;
		// let's only accept values above 20 but at least 10 over the prompt length (which is at least 7)
		if(numberOfLineCharacters<minimumNumberOfLineCharacters){
			oneLineDown();toStartOfLine();
			if(numberOfLineCharacters<=0)outputWarning("Failed to obtain the number of columns of the input window");
			while(1){	
				numberOfLineCharacters=0;
				output("How many characters would fit on a single user input line? (minimally %i)? ",minimumNumberOfLineCharacters);
				char c;
				while(inputCharRead(&c)){ // should be Ok to use inputCharRead() here
					if(c==13||c==10)break;
					if(c<48||c>57){beep();continue;}
					outputChar(c);
					numberOfLineCharacters*=10;
					if(c!=48)numberOfLineCharacters+=(c-48);
				}
				outputChar('\n');
				if(numberOfLineCharacters>=minimumNumberOfLineCharacters)break;
				output("The number of line characters %i does not exceed %i. Please try again...\n",numberOfLineCharacters,minimumNumberOfLineCharacters);
			}
			// we should prompt again for the command
			showPrompt();
		}
	}else
	if(newNumberOfLineCharacters!=numberOfLineCharacters){ // a change
		if(numberOfLineCharacters>0)
			output("Undefined number of line characters: will assume %i characters.\n",numberOfLineCharacters);
		else
			output("Number of line characters still unknown.\n");
	}
}

// MDH@06MAY2020
/**
 * @brief outputs the total memory usage
 * 
 */
void outputTotalMemoryUsage(){//Mallocationowner owner=getOwner(__LINE__);
	resetOutputColor(); // MDH@30JUN2020: apparently sometimes required
	if(!amVerbose()){
		unsigned long long numberOfAllocationTypeSizes=0; // i.e. only interested in the overall types
		long long numberOfAllocationMarks=-1; // i.e. only interested in the last (=current) mark
		// MDH@22MAY2020 TODO unfortunately _getAllationTypeSizes is NOT under allocation managed control
		long long * _allocationTypeSizes=_getAllocationTypeSizes(NULL,&numberOfAllocationTypeSizes,&numberOfAllocationMarks);
		if(_allocationTypeSizes){
			if(numberOfAllocationTypeSizes>0&&numberOfAllocationMarks>0)
			{output("Dynamicaly allocated memory: ");outputLongLongLocale(_allocationTypeSizes[1]);output(".\n");}
			// replacing: output("Dynamically allocated memory: %s bytes.\n",LL_SEP(_allocationTypeSizes[1]));
			free(_allocationTypeSizes);
		}else
			outputError("No memory allocation information available!");
	}else{ // verbose information will also show all the current allocation marks
		output("Dynamic memory allocation:\n");
		outputAllocationTypeMarks("\t");
	}
}/* VALIDATED */
// MDH@11MAY2020: if we want to know what changed since the previous mark as for the incremental memory usage
//				it's easiest to ask for all and only show the changes
/**
 * @brief outputs the incremental memory usage
 * 
 * @param incrementalNumberOfAllocationMarks 
 * @return long long the number of allocation marks
 */
long long outputIncrementalMemoryUsage(long long incrementalNumberOfAllocationMarks){Mallocationowner owner=getOwner(__LINE__);
	unsigned long long numberOfAllocationTypeSizes=0; // i.e. only interested in the overall types
	long long numberOfAllocationMarks=incrementalNumberOfAllocationMarks+1; // request at least one more allocation mark than increments we need 
	// MDH@22MAY2020 TODO unfortunately _getAllationTypeSizes is NOT under allocation managed control
	long long * _allocationTypeSizes=_getAllocationTypeSizes("",&numberOfAllocationTypeSizes,&numberOfAllocationMarks);
	if(amVerboseDebugging())
		output("Number of allocation type sizes received: %lld. Number of allocation marks received: %lld.\n",numberOfAllocationTypeSizes,numberOfAllocationMarks);
	if(_allocationTypeSizes!=NULL){
		// I suppose we need at least two allocation marks returned so we can determine at least one increment
		if(numberOfAllocationMarks>1){
			// for(unsigned long long allocationTypeSizeIndex=0;allocationTypeSizeIndex<numberOfAllocationTypeSizes;allocationTypeSizeIndex++){
			// 	if(allocationTypeSizeIndex%(numberOfAllocationMarks+1)==0)outputChar('|');
			// 	output("\t%llu:%lld",allocationTypeSizeIndex,_allocationTypeSizes[allocationTypeSizeIndex]);
			// }
			// outputChar('\n');
			long long increment,decrement,allocationMark;
			char allocationType;
			unsigned long long allocationTypeInfo,allocationTypeSize,allocationSize;
			// there will be 3 values per type: the type itself, what is now occupied, and what was occupied before
			output("Dynamically allocated memory units:\n");
			output("Type\tB/unit\tNow\t  Then\t  Chronological changes\n");
			for(unsigned long long allocationTypeSizeIndex=0;allocationTypeSizeIndex<numberOfAllocationTypeSizes;){
				allocationTypeInfo=_allocationTypeSizes[allocationTypeSizeIndex++]; // by incrementing the loop index we end up on the first memory item
				allocationType=allocationTypeInfo&0x7F; // the type character is stored in the lower 7 bits!!!
				allocationTypeSize=allocationTypeInfo>>8; // the size is stored in the remainder bytes
				// at least one of the allocation mark must be different
				allocationMark=numberOfAllocationMarks;
				while(--allocationMark>0&&_allocationTypeSizes[allocationTypeSizeIndex+allocationMark]==_allocationTypeSizes[allocationTypeSizeIndex+allocationMark-1])
				;
				if(allocationMark>0){
					allocationSize=_allocationTypeSizes[allocationTypeSizeIndex];
					output("%c\t%llu\t%lld",allocationType,allocationTypeSize,(allocationTypeSize>0?(allocationSize/allocationTypeSize):allocationSize)); // showing the current allocation type size 
					allocationSize=_allocationTypeSizes[allocationTypeSizeIndex+numberOfAllocationMarks-1];
					output("\t= %lld",(allocationTypeSize>0?(allocationSize/allocationTypeSize):allocationSize)); // what it was originally
					// let's show the increments starting at the oldest (later) mark
					allocationMark=numberOfAllocationMarks-1; // count numberOfAllocationMarks
					increment=_allocationTypeSizes[allocationTypeSizeIndex];decrement=_allocationTypeSizes[allocationTypeSizeIndex+allocationMark];
					/*
					outputChar('\t');
					if(increment<0||decrement<0)outputChar('?');
					if(increment!=decrement){
						if(increment>decrement)outputChar('+');
						output("%lld",increment-decrement);
					}
					*/
					while(--allocationMark>=0){
						increment=_allocationTypeSizes[allocationTypeSizeIndex+allocationMark];
						outputChar('\t');
						if(increment<0||decrement<0)outputChar('?');
						if(increment!=decrement){
							if(increment>decrement)
								output("+ %lld",allocationTypeSize>0?(increment-decrement)/allocationTypeSize:increment-decrement); // replacing: output("\t%lld:%lld:%lld",increment-decrement,allocationTypeSizeIndex,allocationMark);
							else
								output("- %lld",allocationTypeSize>0?(decrement-increment)/allocationTypeSize:decrement-increment); // replacing: output("\t%lld:%lld:%lld",increment-decrement,allocationTypeSizeIndex,allocationMark);
							decrement=increment;
						}
					}
					outputChar('\n');
				}
				allocationTypeSizeIndex+=numberOfAllocationMarks;
			}
		}else
			outputError("Failed to obtain (and output) the incremental memory usage.");
		free(_allocationTypeSizes); // MDH@25MAY2020 TODO we should know the type though!!!
	}
	return numberOfAllocationMarks;
}/* VALIDATED */

// MDH@30OCT2019 END
/**
 * @brief prompts for user input on the start of a new command
 * 
 */
void promptForUserInput(){
	free_userinputline();
	if(_userinputline!=NULL)outputBug("Failed to release user input line info"); // MDH@30OCT2019: get rid of all previously stored user input line info
	enableRawmode(0);
	resetOutputColor();
	newline();
	outputLine(promptinfo[inputMode]); // show the appropriate input mode prompt info
	showPrompt();
	initializeNumberOfLineCharacters(); // MDH@24JUN2020: determine the number of line characters available!!!
	//////if(inputMode==IM_COMMAND)
}
/* The following ANSI escape sequences are currently supported.
 * If n and/or m are omitted, they default to 1.
 *   ESC [nA moves up n lines
 *   ESC [nB moves down n lines
 *   ESC [nC moves right n spaces
 *   ESC [nD moves left n spaces
 *   ESC [m;nH" moves cursor to (m,n)
 *   ESC [J clears screen from cursor
 *   ESC [K clears line from cursor
 *   ESC [nL inserts n lines ar cursor
 *   ESC [nM deletes n lines at cursor
 *   ESC [nP deletes n chars at cursor
 *   ESC [n@ inserts n chars at cursor
 *   ESC [nm enables rendition n (0=normal, 4=bold, 5=blinking, 7=reverse)
 *   ESC M scrolls the screen backwards if the cursor is on the top line
 */
/**
 * @brief outputs \p text using format \p fmt in the default output color
 * 
 * @param fmt 
 * @param text 
 */
void outputText(char* fmt,char* text){resetOutputColor();output(fmt,text);}

// MDH@23SEP2020: if characters are output from a certain start position we can return an Mcursormovement indicating the final position and the number of characters written
/**
 * @brief for storing information on a cursor movement
 * 
 */
typedef struct{
	uint16_t position;
	size_t written,lines;
}Mcursormovement;

// MDH@07JUL2020: if you want to output text that wraps to the number of command line characters call outputCommandText()
//				passing in the text and the current position on the line, passing out the final position on the line
// MDH@22SEP2020: it makes sense to return the number of characters written as well we can use a long long for that
//				assuming 32 bits for the position and number of characters to write suffice or we could use 16 bits
//				for the position and 48 bits for the maximum number of characters to write
// MDH@23SEP2020: uses and updates cursormovement
// MDH@24SEP2020: bool notsuggested replaced by int64_t offset (pass -1 for offset if not in the command)
//				but be careful here because 
// MDH@13OCT2020: I thought I already handled the explicit newline characters but don't see that here
/**
 * @brief outputs the command line text \p text in foreground color \p textcolor
 * @details wraps the text so it will be completely visible
 * @param text the command line text to output
 * @param _cursormovement is updated with the movement done by the cursor as a result of writing \p text
 * @param textcolor 
 * @param commandCharactersWrittenSoFar the number of command characters written so far
 */
static void outputCommandLineText(char* text,Mcursormovement* _cursormovement,char* textcolor,int64_t commandCharactersWrittenSoFar){
	if(NULL==_cursormovement)return;
	size_t numberOfCharactersToOutput=(text!=NULL?strlen(text):0);
	if(numberOfCharactersToOutput>0){
		char *textCharacter=text;
		setColor(textcolor);
		uint16_t maximumNumberOfLineCommandCharacters=(numberOfLineCharacters>0?numberOfLineCharacters-promptLength:0); // MDH@23SEP2020: I suppose we have one character more (if we allow a character on the last position of the line)
		// MDH@13OCT2020: a ha it is possible that all the given characters fit on the current line but contain newline characters
		// which would not be recognized if we do it this way!!!!!! which means we're forced to output the text one 
		// character at a time anyway
		/*
		if(maximumNumberOfLineCommandCharacters>0&&_cursormovement->position+numberOfCharactersToOutput>maximumNumberOfLineCommandCharacters){
		*/
			/*unsigned*/ long long leftOnLine=(maximumNumberOfLineCommandCharacters>0?maximumNumberOfLineCommandCharacters-_cursormovement->position:-1); // what we can fit on the line
			// careful: leftOnLine could now be zero, essentially we know that characters will be written on successive lines
			while(1){
				if(leftOnLine==0){
					size_t prompted=showContinuedPrompt(commandCharactersWrittenSoFar,false);setColor(textcolor); // normally we would use setTokenColor(token) which would also set the background color but we're assuming that the background color won't change
					if(prompted)_cursormovement->lines++; // MDH@15OCT2020 replacing:	_cursormovement->skipped+=prompted;
					leftOnLine=maximumNumberOfLineCommandCharacters; // NOTE: so leftOnLine is the total number of command characters that we can fit after the prompt
				}
				
				// MDH@16OCT2020: we're not supposed to explicitly display newline characters although we do count them??????? yes because they are in the tokens!!!
				//				TODO this might definitely give problems at some point
				size_t written=((*textCharacter)!=127&&(*textCharacter)>=32?outputChar(*textCharacter):0); // MDH@19OCT2020: not 'writing' any ASCII character that is not 'visible'
				if(commandCharactersWrittenSoFar>=0)commandCharactersWrittenSoFar++; // it's always a single character that we 'wrote'
				_cursormovement->written+=written;
				if((*textCharacter)==M_NEWLINE_CHARACTER){
					// outputChar('\n'); // TODO is this the way to force going one line down???????
					size_t prompted=showContinuedPrompt(commandCharactersWrittenSoFar,true);setColor(textcolor);
					if(prompted)_cursormovement->lines++; // MDH@15OCT2020 replacing: _cursormovement->skipped+=prompted;
					if(leftOnLine>=0)leftOnLine=maximumNumberOfLineCommandCharacters;else _cursormovement->position=0;
				}else{
					// outputChar('Y');
					if(leftOnLine>=0)leftOnLine-=written;else _cursormovement->position+=written;
				}
				if((--numberOfCharactersToOutput)==0)break; // if there are no characters to output left we're done
				textCharacter++; // increment the pointer that points to the next character
			}
			if(leftOnLine>=0)_cursormovement->position=maximumNumberOfLineCommandCharacters-leftOnLine;
		/*
		}else{
			outputChar('A');
			size_t charactersOutput=output("%s",text);
			_cursormovement->position+=charactersOutput;
			_cursormovement->written+=charactersOutput;
		}
		*/
	}
}

// Token is now defined in Mexpression.h which is included by Mexecution.h so struct Token is indirectly supplied by Mexpression.h!!!
// MDH@24JUN2020: need to reconsider how to output the token given that a single token can occupy multiple 
//				output lines based on its position and length (and numberOfLineCharacters and promptLength)
// MDH@24SEP2020: every function that outputs text on the command line should receive a Mcursormovement reference to be passed along to outputCommandLineText...
//				NOTE let's allow passing in NULL for _cursormovement which is valid when the result of outputToken is not used (as is often the case)
// MDH@15OCT2020: because output
/**
 * @brief outputs token \p _token returning the resulting cursor movement in \p _cursormovement
 * 
 * @param _token 
 * @param _cursormovement the resulting cursor movement
 */
static void outputToken(Mtoken const * const _token,Mcursormovement* _cursormovement){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_token)return;
	// MDH@24SEP2020: the following ASSERT still holds except that _cursormovement->position should actually hold the same value
	// ASSERT assuming _token->position actually contains the number of command characters on the current command line in front of this token
	// MDH@24SEP2020 with _cursormovement being the added parameter we do not need this anymore: size_t position=0;
	size_t tokenCharacterCount=string_length(_token->text);
	if(tokenCharacterCount>0){
		Mcursormovement cursormovement={_token->position};if(NULL==_cursormovement)_cursormovement=&cursormovement; // if no cursor movement instance was provided, we create one from the token's position, of course no result will be available to the outside
		// MDH@24SEP2020 not being used in this function apparently: uint16_t maximumNumberOfLineCommandCharacters=(numberOfLineCharacters>0?numberOfLineCharacters-promptLength-1:0);
		// MDH@26JUN2020: we can speed things up a bit by immediately using string()
		char* tokenText=string(_token->text);
		// MDH@31OCT2019: by introducing ` as new line request character (whitespace) we'll be having visible whitespace characters at the end of the token which we do not want to show in the same color
		// ascertain that the token text ends at the first whitespace character (if there is any whitespace) NOTE there's no need to put '\0' back, therefore we use '\0' if we didn't replace the character to start with
		char firstWhitespaceCharacter='\0';
		size_t significantTokenCharacterCount;
		// MDH@15OCT2020: if we force the whitespace characters to be written....
		//				a ha that would result the 'whitespace' to be written in the color of the token type which we do not want to, which means we can (and should) write all of whitespace with a single outputCommandLineText
		if(isTokenFinished(_token)){
			significantTokenCharacterCount=getTokenSignificantCharacterCount(_token);
			firstWhitespaceCharacter=tokenText[significantTokenCharacterCount];
			tokenText[significantTokenCharacterCount]='\0';
		}else
			significantTokenCharacterCount=tokenCharacterCount;
		// if we allow comments in tokens we're in trouble!!!
		// now done by outputCommandLineText(): outputTokenColor(_token);
		// MDH@07JUL2020: delegating writing the significant token characters to outputCommandLineText()
		// MDH@22SEP2020: outputCommandLineText() now also returning the number of characters written (and the position in the first 16 bits)
		// MDH@24SEP2020 with _cursormovement being the added parameter we do not need this anymore (passing &cursormovement to outputCommandLineText): Mcursormovement cursormovement={_token->position};
		
		// MDH@16OCT2020: most likely _token->offset will equal _cursormovement->written but this is not obligatory
		size_t charactersWrittenSoFar=_cursormovement->written;

		outputCommandLineText(tokenText,_cursormovement,getTokenColor(_token->type),_token->offset);
		
		int64_t commandCharactersWrittenSoFar=_token->offset+(_cursormovement->written-charactersWrittenSoFar); // MDH@16OCT2020: this did the trick in determining what to pass to the next outputCommandLineText() below!!!
		// MDH@24SEP2020 NOTE: _cursormovement->written now contains the number of token characters written
		/* replacing:
		// MDH@24JUN2020: if a token is on multiple lines (due to a limiting number of line characters)
		//				we have to 'insert' so-called soft-breaks
		size_t left;
		if(_token->position+significantTokenCharacterCount>=maximumNumberOfLineCommandCharacters){
			// I think it's easier to simply write one character at a time
			left=maximumNumberOfLineCommandCharacters-_token->position; // what we can fit on the line
			for(int tokenCharacterIndex=0;tokenCharacterIndex<significantTokenCharacterCount;tokenCharacterIndex++){
				outputChar(tokenText[tokenCharacterIndex]);
				if(--left==0){
					showContinuedPrompt(true);outputTokenColor(_token);
					left=maximumNumberOfLineCommandCharacters;
				}
			}
		}else{ // all characters fit on this line (no soft-breaks required)
			output("%s",tokenText); // although string() will write the '\0' at the end we've already written one in front of that position
			left=maximumNumberOfLineCommandCharacters-_token->position-significantTokenCharacterCount;
		}
		*/
		// if there's whitespace text to start with write it in the default output color
		if(firstWhitespaceCharacter){
			// resetOutputColor();
			tokenText[significantTokenCharacterCount]=firstWhitespaceCharacter; // put it back

			// MDH@15OCT2020: using outputCommandLineText() to display ALL of the 'whitespace' (incl. the explicit line breaks that might be in there) in the info color, no need to reset the output color (above)
			outputCommandLineText(tokenText+significantTokenCharacterCount,_cursormovement,getInfoColor(),commandCharactersWrittenSoFar);
			/* replacing all this lot which is no longer needed as outputCommandLineText now takes care of the explicit line breaks which it didn't do before:
			// MDH@04OCT2020: with any number of explicit newline characters following the token itself
			//				and knowing that outputCommandLineText does not take those into account
			//				we need to 'split' the text by these newline characters
			size_t endOfWhitespace,startOfWhitespace=significantTokenCharacterCount;
			bool whitespaceEndsWithNewlineCharacter;
			while(1){
				endOfWhitespace=startOfWhitespace;
				while(tokenText[endOfWhitespace]!='\0'&&tokenText[endOfWhitespace]!=M_NEWLINE_CHARACTER)endOfWhitespace++;
				// make firstWhitespaceCharacter equal to the first whitespace character behind the end of line character (if any)
				// if we detected an explicit end of line character we should write all text up until the newline character
				whitespaceEndsWithNewlineCharacter=(tokenText[endOfWhitespace]==M_NEWLINE_CHARACTER);
				if(whitespaceEndsWithNewlineCharacter){
					endOfWhitespace++; // pointing to the first character behind the newline character
					firstWhitespaceCharacter=tokenText[endOfWhitespace]; // yes, could as well be '\0'
					tokenText[endOfWhitespace]='\0';
				}
				// MDH@07JUL2020: delegating to outputCommandLineText() for writing the whitespace characters
				// MDH@22SEP2020: same here
				if(endOfWhitespace>startOfWhitespace){ // there's whitespace in between to write
					// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...
					outputCommandLineText(tokenText+startOfWhitespace,_cursormovement,getInfoColor(),commandCharactersWrittenSoFar+_cursormovement->written);
					commandCharactersWrittenSoFar+=_cursormovement->written; // update the number of command characters written so far (that is including the token characters we've now written)
				}
				// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: position=cursormovement.position;

				// MDH@04JUL2020: the token may end with the explicit newline character!
				if(!whitespaceEndsWithNewlineCharacter)break; // if not ending with a new line characters 
				startOfWhitespace=endOfWhitespace; // ready for the next time
				tokenText[startOfWhitespace]=firstWhitespaceCharacter; // put the first whitespace character back
				if(showContinuedPrompt(commandCharactersWrittenSoFar,true))_cursormovement->lines++; // MDH@15OCT2020 replacing: _cursormovement->skipped+=showContinuedPrompt(commandCharactersWrittenSoFar,true); // MDH@24SEP2020: the number of prompt characters written now appended to the skipped field of _cursormovement
				_cursormovement->position=0; // MDH@24SEP2020: I guess we're at the beginning of the command line again
				// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: position=0;
			}
			*/
		}
		// resetOutputColor();
	}
	// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: return position; // replacing: tokenCharacterCount;
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}

// MDH@30APR2019: when a function returns to a variable and the other way round
// MDH@26JUN2020: TODO has to be reviewed!!!
/**
 * @brief reoutputs token \p _token
 * 
 * @param _token 
 */
static void reoutputToken(Mtoken const * const _token){
	// DEBUG outputChar('X');
	size_t tokenCharacterCount=(_token&&_token->text?string_length(_token->text):0);
	if(tokenCharacterCount==0)return; // shouldn't happen though
	// MDH@31OCT2019: this is particularly hard if the token is written over several lines
	//				I solved this by making the newline character always end a token (by treating it as whitespace that effectively ends any current token)
	//				but now we have the situation that the given token might be at the previous line, this is the case when the token was ended with a newline and we're now at the start of the next line
	//				the situation is: we're at the end of the token and its type changed and we have to write it again and return to the current position
	//				we might have consumed an assignment operator behind it returning us to this token which might be variable that could also be a function (?????)
	//				the problem with writing the token is that it does not recognize the newline character at the end
	//				for safety reasons we look at _userinputline because if _userinputline is NULL there's no previous command input line
	bool tokenOnPreviousInputLine=(_userinputline?(string_last_char(_token->text)==M_NEWLINE_CHARACTER):false);
	if(tokenOnPreviousInputLine){oneLineUp();toStartOfLine();moveCursorRight(promptLength+(_userinputline->offset-(_userinputline->_prev?_userinputline->_prev->offset:0)));}
	moveCursorLeft(tokenCharacterCount);
	outputToken(_token,NULL); // back where we started (hopefully) // MDH@24SEP2020: not passing an Mcursormovement in, as the result of outputToken is not used!!!
	if(tokenOnPreviousInputLine){oneLineDown();toStartOfLine();moveCursorRight(promptLength+getUserInputLength()-_userinputline->offset);}
}
/**
 * @brief clears the user input info line
 * 
 */
void clearInfo(){
	toUserInputCursorPosition(toInfoInputLine());
} // MDH@30OCT2019: toInfoInputLine() automatically clears the info input line!!!
/**
 * @brief outputs command \p command on the user input info line
 * 
 * @param command 
 */
void inputInfoCommand(Mcommand* command){
	size_t linesMovedUp=toInfoInputLine();
	resetOutputColor();
	if(command!=NULL){Mtoken* token=command->_firstToken;while(token!=NULL){output("%s|",string(token->text));token=token->next;}}
	toUserInputCursorPosition(linesMovedUp);
}
/**
 * @brief outputs the status on the input character entered \p inputChar of type \p inputCharType
 * 
 * @param inputChar 
 * @param inputCharType 
 */
void outputStatus(char inputChar,char inputCharType){Mallocationowner owner=getOwner(__LINE__);
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=owned_string(_getAutoCompletionText('|'),owner);if(!_separatedBehindCursorText)return;
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %s | Cursor position: %zu | Command length: %zu | Manual feed forward: '%s' | Identifier continuation: '%s' | Feed forward: '%s'.",inputChar,inputChar,inputCharType,(_userInputCommand->_lastToken!=NULL?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:""),getUserInputLength(),getCommandLength(),(_manualFeedforwardText?string(_manualFeedforwardText):""),(_identifierContinuationCharacters?_identifierContinuationCharacters:""),string(_separatedBehindCursorText));
	FREE_STRING(_separatedBehindCursorText,owner);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}
/**
 * @brief outputs debug info on the current user input command and feed forward text
 * 
 */
void outputDebugInfo(){Mallocationowner owner=getOwner(__LINE__);
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=owned_string(_getAutoCompletionText('|'),owner);
	if(NULL==_separatedBehindCursorText)return;
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Cursor position: %zu | Command length: %zu | Token type: % s | Manual feed forward: '%s' | Identifier continuation: '%s' | Auto completion: '%s'."
						,getUserInputLength()
						,getCommandLength()
						,(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:"")
						,(_manualFeedforwardText!=NULL?string(_manualFeedforwardText):"")
						,(_identifierContinuationCharacters!=NULL?_identifierContinuationCharacters:"")
						,string(_separatedBehindCursorText));
	FREE_STRING(_separatedBehindCursorText,owner);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}

/*
// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType){
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//				but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=_getToken(lastCommandToken,tokenType);
	if(_newCommandToken)
		setTokenType(_newCommandToken,tokenType);
	else 
		inputError("Failed to create a command token");
	return _newCommandToken;
}
*/

Mallocationowner owner_currentFunctionBodyInput=(Mallocationowner){MI_MAIN,__LINE__,1};

// keep track of all commands so far
/**
 * @brief the number of commands in one command block
 * 
 */
#define COMMAND_BLOCKSIZE 8
 // array for storing the pointers to the first token of all commands entered

// MDH@18JUN2020: a registered command might be a command that is a duplicate of a previous command
/**
 * @brief a record to store a registered command
 * 
 */
typedef struct{
	Mcommand* _command;
	unsigned long long previousCommandIndex;
}Mregisteredcommand;

/**
 * @brief stores the registered commands
 * 
 */
// MDH@17APR2024 reinstated global registration of commands /* MDH@09APR2024: a list of registered commands is now kept by the current environment (much more convenient!!!)
Mregisteredcommand* _registeredcommands=NULL;
Mallocationowner owner_registeredcommands=(Mallocationowner){MI_MAIN,__LINE__,1};
size_t commandBlocks=0;

// MDH@24MAY2020 NOTE: registerCommand is ONLY called once with _userInputCommand as argument but 
// MDH@12JUN2020 TODO TODO TODO how to deal with the command being registered and whether or not the tokens are to be disowned when put in the value 
/**
 * @brief registers command \p command with owner \p owner_command
 * 
 * @param command 
 * @param owner_command 
 * @return true on success
 * @return false on failure
 */
bool registerCommand(Mcommand* command,Mallocationowner owner_command){if(NULL==command)return false;Mallocationowner owner=getOwner(__LINE__);
	if(NULL==getCurrentFunctionBodyInput()){ // a top-level (non function body) command
		Menvironment* environment=getExecutionEnvironment();
		if(environment==NULL){outputBug("Environment vanished registering a command");return false;}
		if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
			// I have to copy all first token pointers to a new array large enough
			Mregisteredcommand* newRegisteredCommands=
				(commandBlocks==0
					?MALLOC(sizeof(Mregisteredcommand),COMMAND_BLOCKSIZE,-'C',owner_registeredcommands)
					:REALLOC(_registeredcommands,commandBlocks*COMMAND_BLOCKSIZE,(commandBlocks+1)*COMMAND_BLOCKSIZE,sizeof(Mregisteredcommand),-'C')
				);
			if(newRegisteredCommands==NULL)return false;
			commandBlocks++;
			_registeredcommands=newRegisteredCommands;
		}
		// MDH@18JUN2020: NOTE commandIndex now stored in any command will equal 0 when it is a new command, so when it is being stored commandIndex will tell us whether it is a new command or not
		//				as soon as we store the current command and it is a new command we store the index of the command i.e. where it is located in the list of registered commands
		// if(commandIndex>0)command->sourceCommandIndex=(commandCount-commandIndex+1);
		///////outputLine("Registering command!");
		_registeredcommands[commandCount]=(Mregisteredcommand){owned_command(disowned_command(command,owner_command),owner_registeredcommands)};
		if(commandIndex>0)_registeredcommands[commandCount].previousCommandIndex=commandCount-commandIndex+1; // will be positive for any positive commandIndex, because commandIndex is in [1,commandCount-1)
		commandCount++;
		// should be done before executing a command!!!! environment->commandCount++;
		if(amVerboseDebugging())outputLine("Command registered!");
		return true;
	}
	// should be added to the function body
	// NOTE we can create the value and when it is not appended to the list it will not be bound, and be released by the 'garbage collector'
	// MDH@25MAY2020 TODO should we pass {1} to _getValueOfToken()?????????
	// MDH@08JUN2020 TODO still have to check the following... what would be the owner of the first command token??????? I suppose it will be subowned by the user input command
	// MDH@17JUN2020 if we store the first command token in a value it needs to be disowned so it can be owned by the value 
	//			   NOTE that _getValueOfToken will free the token (and all connected tokens) when failing to create the value
	//			   we need to determine what to do with the command itself
	Mvalue* _commandToEvaluateTokenValue=_getValueOfToken(disowned_token(command->_firstToken,owner_command)); // MDH@12JUN2020: TODO as we do NOT want to loose _firstToken we do NOT disown it before asking for the value of the token
	if(_commandToEvaluateTokenValue!=NULL){ // the first command token is now bound (or otherwise released)
		// MDH@22MAY2020: __list creates a list that is to be subowned by the function in the current function body input
		if(NULL==getCurrentFunctionBodyInput()->_function->_bodyCommandList)
			getCurrentFunctionBodyInput()->_function->_bodyCommandList=
				owned_list(__list("body command list"),Msubowner(owner_currentFunctionBodyInput,2));
		// MDH@08JUN2020: if we succeed in adding the command value to the function body we still return false as result which will result in command to be freed
		//				BUT by NULLing command->_firstToken we prevent the tokens from being freed in the command as we should
		if(appendedToList(getCurrentFunctionBodyInput()->_function->_bodyCommandList,owner_currentFunctionBodyInput,_commandToEvaluateTokenValue,M_LL_INVALID)>0){
			command->_firstToken=NULL;
			return true;
		}
		outputError("Failed to add the command to the body of the function");
	}else
		outputError("No token (value) to add to the body of the function");
	return false;
}
// tokenizer constants moved over to Mshell.h

/* MDH@11AUG2019: NOT doing the following anymore, instead we store the identifier information in the tokens themselves
// MDH@06AUG2019: I have to keep track of variable initializations in the current command so that I know when a variable is new or not
//				analysis: variables used in a command typically refer to variables created in previous commands
//				i.e. to variables kept in the current execution environment
//				but we want to allow for local variables like the counter in a for loop or local to a group of commands as now possible in a do() function call
//				and probably also in a user function call
//				so technically calls to the predefined functions called for, do and function can have arguments in which local variables are defined
//				these local variables should be considered to exist for the duration of the call i.e. in all following arguments of the call when being entered, so these variables do not need to be 'global'
//				in case of for the first argument contains the local variable initializations, we can do the same in the do function call, in function its the second argument (where the first argument represents the name of the function)
//				this means that we need to know in which argument of any for, do or function call when it is being entered, of course with function we could create a function by assigning it to a variable name instead of defining the function name as first argument
//				in which case the first argument would be come the argument with the local variables, which would be more convenient!!!!
typedef struct Minitialization{
	char* _variableName;
	int64_t argument; // keep track of the argument this value is initialized at the beginning and decremented/incremented whenever necessary this argument needs to be zero
	struct Minitialization *_prev;
}Minitialization;
Minitialization *_lastInitialization=NULL; // stack of initializations
void free_initialization(Minitialization* _initialization){if(_initialization){if(_initialization->_variableName)free(_initialization->_variableName);FREE(_initialization,'I');}}
bool pushInitialization(char* variableName){
	Minitialization* _initialization=(variableName&&strlen(variableName)?CALLOC(sizeof(Minitialization),'I'):NULL);
	if(_initialization){
		_initialization->_variableName=_strdup(variableName);
		if(_initialization->_variableName){
			_initialization->_prev=_lastInitialization;
			// typically we copy the argument count over from the previous initialization but if this initialization is an end of a function call, we have to copy the argument 
			Minitialization* _argumentCountInitialization=_lastInitialization;
			if(*variableName==')'){ // this 'initialization' ends the current function call
				bool startOfFunctionCallInitialization;
				while(_argumentCountInitialization){
					startOfFunctionCallInitialization=(*(_argumentCountInitialization->_variableName)=='(');
					_argumentCountInitialization=_argumentCountInitialization->_prev;
					if(startOfFunctionCallInitialization)break;
				}
			}
			_initialization->argument=(_argumentCountInitialization?_argumentCountInitialization->argument:-1);
			_lastInitialization=_initialization;
		}else{
			FREE(_initialization,'I'); // no need to call free_initialization as no variable to free
			_initialization=NULL;
		}
		/// do this in the caller!!!! inputError("Failed to remember initialization '%s'.",_variableName);
	}
	return(_initialization!=NULL);
}
bool popInitialization(){
	if(_lastInitialization){
		Minitialization* _initialization=_lastInitialization->_prev;
		free_initialization(_lastInitialization);
		_lastInitialization=_initialization;
		return true;
	}
	return false;
}
bool initializable(){return(!_lastInitialization||_lastInitialization->argument==0);} // we need to be in the right argument to be initializable, MUST be called BEFORE calling pushInitialization() when a variable name is pushed!!!
void showInitializations(){
	// we have to compose a text first, then call inputInfo()
	Mstring* _initializationsText=__string();
	if(_initializationsText){
		Mstring* p=_initializationsText;
		Minitialization* _initialization=_lastInitialization;
		while(p&&_initialization){
			char* _initializationText=_getFormattedText(80," %s:%lld",_initialization->_variableName,_initialization->argument);
			if(_initializationText){
				p=string_prepend(p,_initializationText);
				free(_initializationText);
			}
			_initialization=_initialization->_prev;
		}
		if(p)inputInfo("Initializations:%s.",string(_initializationsText));else inputError("Failed to show the initializations.");
		FREE_STRING(_initializationsText);
	}
}
void removeInitializations(){while(popInitialization());} // keep popping until failure
void determineCommandInitializations(){
	// TODO this is going to be quite hard...

}
bool initialized(char* variableName){
	// it's a little harder than just looking for a matching _variableName in the initializations as we can have subinitializations in do() and for() function calls which we will need to skip
	// as such we will need to count the level of initializations
	Minitialization* _initialization=_lastInitialization;
	unsigned long long level=0;
	char firstVariableNameCharacter;
	while(_initialization){
		firstVariableNameCharacter=*(_initialization->_variableName);
		switch(firstVariableNameCharacter){
			case '(':level--;break; // end of sublevel
			case ')':level++;break; // start of sublevel
			case ',':break; // a previous argument
			default:if(level==0&&!strcmp(_initialization->_variableName,variableName))return true;
		}
		_initialization=_initialization->_prev;
	}
	return(_initialization!=NULL);
}
// MDH@06AUG2019 END
*/
// suggesting NOT to be able to get out of an error condition but to allow viewing information on the error somehow!!! (how about tab as this will do feed forward!!!!!)
// if we put the error info in the error token

//// first operator characters (0=assignment character, 1-6: binary 1 and 2-character operators, 7-8: 1-character binary, 9-10: unary/binary, 11-12: 1-character unary)
//const char ASSIGNMENT_CHARACTER='=';
//const char FIRST_OPERATOR_CHARACTERS[]={ASSIGNMENT_CHARACTER,'<','>','|','&','*','/','^','%','+','-','~','!','\0'}; // i.e. "=<>|&*/^%+-~!";
// continuation of 2-character operators
////////const char* SECOND_OPERATOR_CHARACTERS[]={"=","=<>","=<>","|","&","*","/"};
/* if we have a token representing an operator we can check whether a new character is acceptable as continuation
bool continuesOperator(Mtoken* _userInputCommand->_lastToken,char inputChar){
	unsigned int l=string_get_length(_userInputCommand->_lastToken->text);
	if(inputChar==ASSIGNMENT_CHARACTER){ // appending the 'assignment' operator
		return(l==1||string_get_last_char(_userInputCommand->_lastToken->text)!=ASSIGNMENT_CHARACTER);
	}else{
		if(l>1)return false; // cannot continue a two-character operator
		// if subtype is assumed to represent the index into the first operator character set
		unsigned int operatorType=_userInputCommand->_lastToken->type.subtype;
		return(operatorType<=6&&strchr(SECOND_OPERATOR_CHARACTERS[operatorType],inputChar)!=NULL);
	}
}
*/
/**
 * @brief returns whether or not token type \p tokenType is a binary operator token type
 * 
 * @param tokenType 
 * @return true
 * @return false 
 */
bool isBinaryOperatorTokenType(uint8_t tokenType){
	return(TOKENTYPE_IDS[tokenType]>>4)==0b0110;
}

// removeLastToken() removes the last user input command token, delegating the actual removal to removeLastCommandToken now defined in Mshell.h/c
/**
 * @brief removes the last user input command token
 * 
 * @return true on success
 * @return false on failure
 */
bool removeLastUserInputCommandToken(){
	Mtoken* tokenToRemove=(_userInputCommand!=NULL?_userInputCommand->_lastToken:NULL);
	if(NULL==tokenToRemove)return false; // there's no last token to remove
	// MDH@20SEP2019: if a token is removed, we also need to remove any associated feed forward text associated with the token
	deleteTokenAutoCompletionText(tokenToRemove);
	// MDH@20DEC2022: if removing the comma in a function call arguments list, we need to increment the number of expected arguments again
	if(tokenToRemove->type==TT_LISTELEMENT&&tokenToRemove->expr&&tokenToRemove->expr->type==TT_FUNCTION_CALL){
		tokenToRemove->expr->argument-=1;
		////inputInfo("Number of expected arguments: %lld.",-tokenToRemove->expr->argument-3);
	}
	/////////////////////outputInfo("Removing last command token!");
	removedLastCommandToken(_userInputCommand,owner_userInputCommand); // NOT using the result (which would be the new last command token)
	return true;
}

// MDH@04NOV2019: moved from line 1800 or so over here as it calls removeLastUserInputCommandToken() and we do not like to have to use prototypes TODO remove all prototype() definitions
/**
 * @brief updates the user input command identifier continuation
 * 
 */
void updateUserInputCommandIdentifierContinuation(){Mallocationowner owner=getOwner(__LINE__);
	// MDH@30OCT2019: simplified updating the identifier continuation a bit so wee do not need to be afraid that it won't work AND we no longer need the userInputCommandIdentifierContinuationNeedsUpdating flag!!!!
	//				BUT right after a delete we should be allowed to set the flag so the continuation will be deleted and nothing more
	//				OK, to make delete work, I have to check the NeedsUpdating flag which should be set to true when the token is set
	deleteIdentifierContinuation();
	// if currently in an identifier, we technically need updating the identifier continuation unless the NeedsUpdating flag has been turned off
	bool canHaveAnIdentifierContinuation=inIdentifierToken(_userInputCommand!=NULL?_userInputCommand->_lastToken:NULL);
	if(canHaveAnIdentifierContinuation){ // theoretically we could have identifier continuation
		if(userInputCommandIdentifierContinuationNeedsUpdating){ // not blocked
			// MDH@04NOV2019: if inside a reference, skip the reference 'operator' at the start of the reference when requesting completion text
			// MDH@15SEP2023: we'd like to be able to complete a map property as well, which means we will have include everything up until the
			//                original variable name
			Mstring* _identifier=owned_string(_getString(_userInputCommand->_lastToken->type!=TT_REFERENCE?string(_userInputCommand->_lastToken->text):string_remainder(_userInputCommand->_lastToken->text,1)),owner);
			Mtoken* token=_userInputCommand->_lastToken;
			/////inputInfo("Token: '%s' of type '%i'.",string(token->text),token->type);
			if(token->type==TT_PROPERTY){
				// let's go back and prefix all property so far up until the variable token
				//inputInfo("Looking for the completion of property '%s'.",string(token->text));
				do{
					token=token->prev;
					string_prepend(_identifier,string(token->text));
				}while(token->type==TT_PROPERTY);
			}
			//////inputInfo("Looking for the completion of '%s'.",string(_identifier));
			// MDH@15SEP2023 TODO when should we look for functions as well, NOT when this is a reference or a property CHECK THIS!!!! 
			Mstring* _completionText=owned_string(_getCompletion(string(_identifier),_userInputCommand->_lastToken->type!=TT_REFERENCE&&_userInputCommand->_lastToken->type!=TT_PROPERTY),owner);
			FREE_STRING(_identifier,owner);
			// need at least two characters (the type and something text behind it)
			// OOPS if there's only one character in the text (just the type) which is quite possible we will need to free _completionText even then!!!
			if(_completionText!=NULL){
				if(string_length(_completionText)>1){
					switch(string_char(_completionText,0)){
						case 1:
							_identifierContinuationCharacters=strdup(string(_completionText)+1);
							if(NULL==_identifierContinuationCharacters)
								inputError("Failed to create the identifier continuation");
							else 
								if(amVerbose())inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
							break;
						case 2:
							_identifierContinuationOptionalCharacters=strdup(string(_completionText)+1);
							if(_identifierContinuationOptionalCharacters!=NULL)
								inputInfo("Identifier continuation characters: %s.",_identifierContinuationOptionalCharacters);
							else 
								inputError("No identifier continuation characters!");
							break;
					}
				}else{
					if(_userInputCommand->_lastToken->type==TT_REFERENCE){
						// MDH@04NOV2019: if we mark the entire reference token as error, we're going to have problems recovering it, so it would make sense to mark the character that caused not having a completion text anymore as erroneous
						//				so we should cut off that last character and put it in an error token, ready to be removed again
						Mstring* lastTokenText=_userInputCommand->_lastToken->text; // NOTE no need to release this, we just need the pointer multiple times
						Mallocationowner owner_lastTokenText=Msubowner(owner_userInputCommand,2);
						size_t lastTokenLength=string_length(lastTokenText);
						if(lastTokenLength>1){ // at least one character of the existing variable present in the reference
							char c=string_last_char(lastTokenText); // get the last character (to mark as erroneous)
							if(c){ // we've got the character that is responsible for not getting a completion text anymore
								int8_t variableExistsIndicator=containsVariable(NULL,string_remainder(lastTokenText,1),(amVerbose()?-1:0));
								// TODO should NOT return 0 because it only does that when the name is invalid
								if(variableExistsIndicator<=0){ // not already complete TODO perhaps there's a better way to compose the completion text in this case in _getCompletion()
									// let's do something like a backspace but without moving the cursor on the screen
									// replacing the last character with a blank is another option????
									if(string_setlength(lastTokenText,lastTokenLength-1/*,owner_lastTokenText*/)){ // managed to 'cut off' c (although it's still there, because when you set the length only ->length is adjusted nothing yet to the text itself)
										Mtoken* _errorToken=_getNewCommandToken(_userInputCommand->_lastToken,TT_ERROR,true); // by passing in NULL all the complicated stuff is not happening!!!
										if(_errorToken!=NULL){
											// MDH@23OCT2020 OOPS take over ownership!!!
											_userInputCommand->_lastToken=owned_token(_errorToken,Msubowner(owner_userInputCommand,1)); // update the last token assuming we will succeed in doing what needs doing
											if(NULL==string_append_char(_errorToken->text,c)){
												if(removeLastUserInputCommandToken())
													inputError("Failed to mark the last invalid reference character as erroneous because it cannot result in a reference to an existing variable.");
												else
													inputError("%s%sFailed to undo failing to mark the last character as erroneous.",M_BUG_PREFIX,M_MESSAGE_PREFIX);
											}else{
												reoutputToken(_errorToken);
												// DEBUG outputChar('W');
											}
										}else
											inputError("The supposed reference can never become an existing variable reference.");
										// if we failed to create the error token, we have to append the removed character again (should be no problem because Mstring does not reduce the memory when deleting characters from the end)
										if(_errorToken!=_userInputCommand->_lastToken){
											if(NULL==string_setlength(_userInputCommand->_lastToken->text,lastTokenLength/*,owner_lastTokenText*/))
											{inputError("%sCouldn't undo the adjustments made to an erroneous reference.",M_BUG_PREFIX);}
										}
									}else
										inputError("Failed to retrieve the last (erroneous) character in a variable reference.");
								}else
								if(amVerboseDebugging())
									inputInfo("Reference complete!");
							}else
								inputInfo("%s%sLast character in reference vanished.",M_BUG_PREFIX,M_MESSAGE_PREFIX);
						}else
							inputError("There is no existing variable that can be referenced anymore.");
					}
				}
				FREE_STRING(_completionText,owner);
			}else 
			if(amVerboseDebugging())
				inputInfo("No identifier continuation.");
		}
	}
	// by forcing the flag to be true AFTER each update, we ascertain that you can only block it once, AND there's no need actually to set it on every new token!!!!
	/**
	 * @brief a flag that keeps track of whether or not we are in an identifier token
	 * 
	 */
	userInputCommandIdentifierContinuationNeedsUpdating=true; // as long as we're in an identifier token, keep the 'dirty' flag true
	/* replacing:
	bool couldHaveAnIdentifierContinuation=inIdentifierToken(_userInputCommand?_userInputCommand->_lastToken:NULL);
	// get rid of the identifier continuation if we're can't have one or the identifier has supposedly changed
	if(!couldHaveAnIdentifierContinuation)userInputCommandIdentifierContinuationNeedsUpdating=false; // if we're not in an identifier token always consider the identifier to be unchanged
	if(!couldHaveAnIdentifierContinuation||userInputCommandIdentifierContinuationNeedsUpdating)deleteIdentifierContinuation();
	if(userInputCommandIdentifierContinuationNeedsUpdating){ // the identifier has supposedly changed, and we could have an identifier continuation update it
		Mstring* _completionText=_getCompletion(string(_userInputCommand->_lastToken->text));
		// need at least two characters (the type and something text behind it)
		// OOPS if there's only one character in the text (just the type) which is quite possible we will need to free _completionText even then!!!
		if(_completionText){
			if(string_length(_completionText)>1)
			switch(string_char(_completionText,0)){
				case 1:
					{
						_identifierContinuationCharacters=strdup(string(_completionText)+1);
						if(!_identifierContinuationCharacters)inputError("Failed to create the identifier continuation");else if(amVerbose())inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
					}
					break;
				case 2:
					{
						_identifierContinuationOptionalCharacters=strdup(string(_completionText)+1);
						if(_identifierContinuationOptionalCharacters)inputInfo("Identifier continuation characters: %s.",_identifierContinuationOptionalCharacters);else inputError("No identifier continuation characters!");
					}
					break;
			}
			FREE_STRING(_completionText);
		}else 
		if(amVerbose())inputInfo("No identifier continuation.");
	}
	*/
	if(amVerboseDebugging())inputInfo("User input command identifier continuation updated.");
}/* VALIDATED */
/**
 * @brief returns the text represention of the current user input command
 * 
 * @param color whether or not to show the command in color
 * @return Mstring* a new M string holding the current user input command text
 */
Mstring* _getCommandText(bool color){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _commandText=owned_string(__string(),owner);
	if(_commandText!=NULL){
		Mtoken* commandToken=_userInputCommand->_firstToken; // TODO can we get rid of using commandcount-1 here????
		while(commandToken!=NULL){
			// if we bump into a comment we're done!!!
			// MDH@08JUL2023: since we now allow comments in a command as well (that end with a newline character) we should simply skip it
			// replacing: if(commandToken->type==TT_COMMENT)break;
			if(commandToken->type!=TT_COMMENT){
				// TODO there must be a better way to do the coloring!!!
				if(color){
					string_append(_commandText,ES"38;5;");/////output("Command text: '%s'.\n",string(_commandText));
					string_append(_commandText,getTokenColor(commandToken->type));//////output("Command text: '%s'.\n",string(_commandText));
					string_append_char(_commandText,'m');/////output("Command text: '%s'.\n",string(_commandText));
				} // assuming the same back color is used on ALL tokens, so we won't have to pass that along
				// MDH@31OCT2019: for now decided NOT to show the whitespace inside the tokens (by replacing the first whitespace character with the end-of-string marker)
				char firstWhitespaceTokenCharacter=(isTokenFinished(commandToken)?string_replacedchar(commandToken->text,'\0',getTokenSignificantCharacterCount(commandToken)):'\0');
				string_append(_commandText,string(commandToken->text));
				if(firstWhitespaceTokenCharacter)string_setchar(commandToken->text,firstWhitespaceTokenCharacter,getTokenSignificantCharacterCount(commandToken)); // put first whitespace character (if any) back
				// MDH@03MAY2019: place an asterisk in front of the type to indicate that expr is NOT null!!
				if(amAssisting()){
					if(color){
						string_append(_commandText,ES"38;5;");/////output("Command text: '%s'.\n",string(_commandText));
						string_append(_commandText,getInfoColor());/////output("Command text: '%s'.\n",string(_commandText));
						string_append_char(_commandText,'m');/////output("Command text: '%s'.\n",string(_commandText));
					}
					string_append_char(_commandText,'(');
					if(commandToken->expr!=NULL)string_append_char(_commandText,'*');/////output("Command text: '%s'.\n",string(_commandText));}
					string_append(_commandText,TOKENTYPE_STRING[commandToken->type]);/////output("Command text: '%s'.\n",string(_commandText));
					string_append(_commandText,") ");/////output("Command text: '%s'.\n",string(_commandText));
				}
			}
			commandToken=commandToken->next;
		}
		if(color)if(!amAssisting()){string_append(_commandText,ES"38;5;");string_append(_commandText,getInfoColor());string_append_char(_commandText,'m');} // reset to info color
	}
	return disowned_string(_commandText,owner);
}
/**
 * @brief clear the current user input command
 * 
 */
void clearCommand(){
	///////output("Clearing the command!\n");
	FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL; // MDH@28OCT2019: using the command now...
	// numberOfLineCommandCharacters=0; // MDH@16OCT2020 not essential because a better place is when a (continued) prompt is displayed
	free_userinputline(); // MDH@24SEP2020: essential BRO'
	/* replacing:
	pLastCommandToEvaluate=NULL;
	// a small precaution here!!!
	if(_userInputCommand->_firstToken){freeToken(_userInputCommand->_firstToken);_userInputCommand->_firstToken=NULL;}
	*/
}
// TODO find a way to not have to replicate as we do now what getValueText() is also doing (but without coloring of course)
// MDH@13MAR2020: result type changed to size_t because now returning the number of characters written
// MDH@07DEC2020: all numbers are displayed taking the current locale into account (what _getValueText() itself never does!!!)
extern char** Mtimezonenames;
/**
 * @brief outputs \p _value color coded
 * 
 * @param _value the value to output
 * @return size_t the number of characters written
 */
size_t outputValueColored(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	// MDH@13OCT2023 (Petra's 59th birthday!): have to revision this speeding up displaying very large big integers for instance
	size_t written=0;
	if(_value!=NULL){
		switch(_value->type){
			case VT_UNDEFINED:
				outputTokenTypeColor(TT_DQSTRING);
				written=output("%s",M_UNDEFINED_VALUE_TEXT);
				break; // let's use the same color as for double quotes string (for now)
			case VT_TOKEN:
				outputTokenTypeColor(_value->value._token->type);
				written=output("%s",string(_value->value._token->text));
				break; // easy the token type determines the color to use!!!
			case VT_INTEGER:
				outputTokenTypeColor(TT_INTEGER);
				written=outputIntegerLocale(_value->value._integer);
				break;
			case VT_TIME:
				{
					outputTokenTypeColor(TT_INTEGER);
					written=outputLongLongLocale(_value->value._time->t-(_value->value._time->tzsec==INT16_MIN?0:_value->value._time->tzsec));
					if(_value->value._time->tznindex!=0){
						resetOutputColor();
						written+=outputChar('@');
						outputTokenTypeColor(TT_SQSTRING);
						written+=output("%s",Mtimezonenames[abs(_value->value._time->tznindex)-1]);
					}
				}
				break; // MDH@08DEC2020: simple, NO?
			case VT_BIGINTEGER:
				outputTokenTypeColor(TT_INTEGER);
				written+=outputBigintegerLocale(_value->value._biginteger);
				break;
			case VT_DECIMAL:
				outputTokenTypeColor(TT_REAL);
				written+=outputDecimalLocale(_value->value._decimal,false);
				break;
			case VT_RATIONAL:
				if(_value->value._rational!=NULL){
					written+=outputChar('(');
					outputTokenTypeColor(TT_INTEGER);
					written+=outputBigintegerLocale(_value->value._rational->num);resetOutputColor();
					written+=outputChar('/');
					// written+=outputChar('/'); // MDH@28OCT2020: inserting
					outputTokenTypeColor(TT_INTEGER);
					if(_value->value._rational->den!=NULL){
						written+=outputBigintegerLocale(_value->value._rational->den);
					}
					else written+=outputChar('1'); // a missing denominator means it's equal to 1
					resetOutputColor();
					written+=outputChar(')');
					if(_value->value._rational->delta!=NULL){
						outputTokenTypeColor(TT_REAL);
						if(_value->value._rational->delta->ld>=0)written+=outputChar('+');
						written+=outputFloatLocale(_value->value._rational->delta);
						/* MDH@07DEC2020: replacing:
						Mstring* _realValueText=owned_string(__string(),owner);
						if(_realValueText){
							appendld(_realValueText,_value->value._rational->delta->ld);
							written+=output("%s",string(_realValueText));
							FREE_STRING(_realValueText,owner);
						}
						*/
						// replacing:	output("%.*Lf",LDBL_DIG,_value->value._rational->delta->ld);
						resetOutputColor();
					}
				}
				break;
			case VT_FLOAT:
				outputTokenTypeColor(TT_REAL);
				written+=outputFloatLocale(_value->value._float);
				break;
			case VT_TEXT:
				outputTokenTypeColor(_value->value._text->presuffix=='"'?TT_DQSTRING:TT_SQSTRING);
				written+=outputValue(NULL,_value,NULL);
				break;
			case VT_BYTES:
				outputTokenTypeColor(TT_SQSTRING);
				written+=outputValue(NULL,_value,NULL);
				break;
			case VT_ARRAY:
				{
					// MDH@0.1.7.14+25JUN2023: too bad it doesn't use _getValueText() here, so we need to change ( and ) into [ and ] here as well
					written+=outputChar('[');
					Marray* _array=_value->value._array;
					if(_array!=NULL){
						unsigned long long arraylength=_array->numberOfElements;
						if(arraylength>0){
							Mvalue** values=_array->values;
							long long numberOfElementsNotWritten=arraylength;
							numberOfElementsNotWritten-=(M_ARRAY_ELEMENTS_AT_START+M_ARRAY_ELEMENTS_AT_END);
							unsigned long long arrayindex=0; // the expected array index
							if(numberOfElementsNotWritten<=0){ // all elements will be written
								arraylength--;
								// write all except the last element
								while(arrayindex<arraylength){
									written+=outputValueColored(values[arrayindex++]);
									written+=outputChar(','); // something will follow
								}
								// write the last element
								written+=outputValueColored(values[arraylength]);	
							}else{
								// show the first M_ARRAY_ELEMENTS_AT_START elements
								while(arrayindex<M_ARRAY_ELEMENTS_AT_START){
									// let's write the index on the first element, the last element
									written+=outputValueColored(values[arrayindex++]);
									written+=outputChar(','); // something will follow
								}
								// show how many elements are not displayed
								written+=output("(%lld element%s not displayed),",numberOfElementsNotWritten,(numberOfElementsNotWritten>1?"s":""));
								arraylength--;
								arrayindex=arraylength-M_ARRAY_ELEMENTS_AT_END;
								output("%llu:",arrayindex+1);
								// show all further elements
								while(arrayindex<arraylength){
									written+=outputValueColored(values[arrayindex++]);
									written+=outputChar(','); // something will follow
								}
								written+=outputValueColored(values[arraylength]); // write the last element
							}
						}
					}
					written+=outputChar(']');
				}
				break;
			case VT_LIST:
				// TODO not using _getListText() as defined in Mexecution
				/////////if(amVerbose())outputValue("List value '",_value,"'.");
				{
					// MDH@02NOV2020: given that a list can be very large, let's stick to displaying at most 100 values
					//				that's like 50 starting values and 50 ending values
					// MDH@03NOV2020: it makes sense when dealing with sparse arrays to not show all the comma's
					//				but show the index numbers instead (obviously it's hard to count)
					//				let's decide to only write the set elements, and if the index difference is not 1 with the previous element write the index in front of the value
					written+=outputChar('(');
					Mlist* _list=_value->value._list;
					if(_list!=NULL&&_list->numberOfElements){
						long long numberOfElementsNotWritten=_list->numberOfElements;
						numberOfElementsNotWritten-=(M_LIST_ELEMENTS_AT_START+M_LIST_ELEMENTS_AT_END);
						Mlistelement* _listelement=_list->_first;
						unsigned long long listitemindex=1; // the expected list item index
						if(numberOfElementsNotWritten<=0){ // all elements will be written
							while(_listelement!=NULL){
								if(listitemindex!=_listelement->index)written+=output("%llu:",_listelement->index);
								// replacing: if(_listelement->index)while(listitemindex<_listelement->index){listitemindex++;written+=outputChar(',');} // missing elements
								written+=outputValueColored(_listelement->_value);
								listitemindex=_listelement->index+1; // the index of the excepted next element
								_listelement=_listelement->_next;
								if(_listelement!=NULL)written+=outputChar(','); // something will follow
							}
						}else{
							// show the first 50 elements
							long long leftToWrite=M_LIST_ELEMENTS_AT_START;
							while(_listelement!=NULL&&--leftToWrite>=0){
								// let's write the index on the first element, the last element
								if(_listelement==_list->_first
									||leftToWrite==0
									||listitemindex!=_listelement->index)
									written+=output("%llu:",_listelement->index);
								// replacing: f(_listelement->index)while(listitemindex<MIN(50,_listelement->index)){listitemindex++;written+=outputChar(',');} // missing elements
								written+=outputValueColored(_listelement->_value);
								listitemindex=_listelement->index+1;
								_listelement=_listelement->_next;
								written+=outputChar(','); // something will follow
							}
							// show how many elements are not displayed
							written+=output("(%lld elements not displayed),",numberOfElementsNotWritten);
							// and skip them
							while(--numberOfElementsNotWritten>=0)_listelement=_listelement->_next;
							listitemindex=_listelement->index;
							written+=output("%llu:",_listelement->index); // write the index on the first element
							// show all further elements
							while(_listelement!=NULL){
								if(_listelement==_list->_last||listitemindex!=_listelement->index)
									written+=output("%llu:",_listelement->index);								
								// replacing: if(_listelement->index)while(listitemindex<_listelement->index){listitemindex++;written+=outputChar(',');} // missing elements
								written+=outputValueColored(_listelement->_value);
								listitemindex=_listelement->index+1;
								_listelement=_listelement->_next;
								if(_listelement!=NULL)written+=outputChar(','); // something will follow
							}
						}
					}
					written+=outputChar(')');
				}
				break;
			case VT_MAP:
				{
					written+=outputChar('{');
					Mmap* _map=_value->value._map;
					if(_map!=NULL&&_map->numberOfElements){
						Mvariable* _mapelementvariable;
						Mmapelement* _mapelement=_map->_first;
						while(_mapelement!=NULL){
							_mapelementvariable=_mapelement->_variable;
							// TODO are we coloring the name?????
							// quoting the name to indicate it is alphanumeric!!
							written+=output("%c%s%c%c",'\'',_mapelementvariable->_name,'\'',':');
							written+=outputValueColored(_mapelementvariable->_value);
							if(NULL==_mapelement->_next)break;
							written+=outputChar(',');
							_mapelement=_mapelement->_next;
						}
					}
					written+=outputChar('}');
				}
				break;
				/* MDH@11MAR2020: better to use _getValueText() for all cases where we're NOT color coding as _getValueText() is kept up to date in the first place
			case VT_REFERENCE:
				outputChar(M_DEREFERENCE_CHARACTER); // same as the reference character 
				if(_value->value._reference&&_value._reference->variable){ // MDH@11MAR2020 now corrected to account for NULL variable field
					output("%s",_value->value._reference->variable->_name);
					outputChar(':');
					output("%zu",_value->value._reference->referenceindex);
				}
				break;
				*/
			default: // for VT_REFERENCE, VT_FUNCTION, VT_ENVIRONMENT and the like
				{
					Mstring* _valueText=owned_string(_getValueText(_value,false,false),owner);
					if(_valueText!=NULL)
					{written+=output("%s",string(_valueText));FREE_STRING(_valueText,owner);}
				}
				break;
		}
	}else{
		outputTokenTypeColor(TT_DQSTRING);
		written+=output("%s",M_NULL_VALUE_TEXT);
	} // TODO what color should we use????
	resetOutputColor();
	return written;
}
/**
 * @brief unfinishes command token \p commandToken
 * 
 * @param commandToken 
 */
void unfinishCommandToken(Mtoken* commandToken){
	// MDH@03SEP2019: adjusted so that not only the unary operators are left finished but all one character token types (which are the only tokens that are immediately finished once a single character is entered!!)
	//				NOTE unfinishing of the current token is done so that the token can be continued, so technically we should unfinish all tokens that can be continued after a character is removed from them
	// for all non-unary token that we are in now that is finished, unfinish 
	// TODO there are other one-character tokens
	// TODO is the test string_length(commandToken->text)==getTokenSignificantCharacterCount(commandToken) always correct?
	//	  NOTE I think it is because if that is the case there is no whitespace at the end of the token and we
	//		   should unfinish the token so we can append to it again
	//		   also 'one character' token types are always finished immediately, so we should not unfinish those!!!
	if(commandToken!=NULL)
		if(!isOneCharacterTokenType(commandToken->type)) // not a unary operator (of length 1) we ended up in
			if(string_length(commandToken->text)==getTokenSignificantCharacterCount(commandToken)) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
				unfinishToken(commandToken);
}

// void outputCommandInfo(Mcommand* command);
/* MDH@28FEB2020: moved over to Mshell.c/h
// if a sequence of tokens needs to be evaluated to a value, call getCommandValue()
Mvalue* getCommandValue(Mcommand* command,char commandType){
	if(amVerbose())outputCommandInfo(command);
	if(!isAValidCommand(command,amVerbose()))return NULL;
	getExecutionEnvironment()->expressionToken=command->_firstToken->next; // prepare the current environment for executing the command
	if(amVerbose())outputLine("Evaluating...");
	return getValueOfExpression(getExecutionEnvironment()->_name,commandType,(TokenType[]){},0);
}
*/
/* MDH@14APR2020 removing:
// MDH@29NOV2019: we're going to keep track of the system and user allocation counts
size_t* getAllocationCountDifferences(size_t* from,size_t* to){
	return NULL;
}
long long *_systemallocationcounts,*_userallocationcounts;
long long *_lastcommandsystemallocationcounts,*_lastcommanduserallocationcounts;
void prepareForEvaluatingCommand(){
	// we need a new pre evaluation allocation counts
	if(_lastcommandsystemallocationcounts)free(_lastcommandsystemallocationcounts);
	_lastcommandsystemallocationcounts=_getAllocationCounts();
	// using that we can update the system allocation counts by subtracting _lastcommanduserallocationcounts

}
*/

/**
 * @brief the number of allocation marks addrd
 * 
 */
long long allocationMarksAdded=0;

/**
 * @brief removes all comment tokens
 * 
 */
void sanitizeCommand(){
	Mtoken *nextToken,*token=(_userInputCommand!=NULL?_userInputCommand->_firstToken:NULL);
	while(token!=NULL){
		nextToken=token->next;
		if(token->type==TT_COMMENT){
			if(token->prev!=NULL)token->prev->next=nextToken;else _userInputCommand->_firstToken=nextToken;
			if(token->next!=NULL)nextToken->prev=token->prev;else _userInputCommand->_lastToken=token->prev;
			token->next=NULL; // to prevent all subsequent tokens to be freed!!!
			//////////outputInfo("Freeing a command token!");
			FREE_TOKEN(token,owner_userInputCommand); // free this comment token alone
		}
		token=nextToken;
	}
}
/**
 * @brief uncomments the current user input command and returns the last comment token if any
 * @details removes all comment tokens from the forward token chain in current command
 * @return Mtoken* the comment token that ends the command
 */
Mtoken* uncommentCommand(){
	Mtoken* endCommentToken=NULL;
	Mtoken* firstToken=(_userInputCommand!=NULL?_userInputCommand->_firstToken:NULL);
	if(firstToken!=NULL){
		Mtoken *token=firstToken,*nextToken=firstToken;
		while(1){
			// ASSERT token should never be a comment token!!!
			nextToken=nextToken->next;
			if(nextToken==NULL)break;
			if(nextToken->type!=TT_COMMENT){
				endCommentToken=NULL;
				////////assert(token!=NULL&&token->type!=TT_COMMENT); // TODO remove once M is sufficiently debugged
				if(token->next!=nextToken)token->next=nextToken;
				token=nextToken;
			}else{
				endCommentToken=nextToken;
				// MDH@30MAR2024: because we can now have intermediate comments (because of embedded commands)
				//                we should 'remove' these intermediate comments as well
				//                removing means point the previous token over skip the comment token!!!!
				endCommentToken->prev->next=endCommentToken->next;
			}
		}
		/* MDH@30MAR2024: we do not need the following anymore if we remove all intermediate comments in the loop above!!
		if(endCommentToken!=NULL)endCommentToken->prev->next=NULL;
		*/
		///if(amVerboseDebugging()){
			size_t tokenCount=0;
			while(firstToken!=NULL){
				if(firstToken->type==TT_COMMENT)
					outputMessage(M_ERROR_PREFIX,"Failed to remove comment token #%zd.\n",tokenCount);
				firstToken=firstToken->next;
				tokenCount++;
			}
			///output("Number of non-comment tokens: %zd.\n",tokenCount);
		///}
	}
	return endCommentToken;
}
void recommentCommand(Mtoken* endCommentToken){
	Mtoken *firstToken=(_userInputCommand!=NULL?_userInputCommand->_firstToken:NULL);
	if(firstToken==NULL)return;
	Mtoken *prevToken,*nextToken,*token=firstToken;
	while(1){
		// ASSERT token!=NULL which is guaranteed because it starts non-NULL and breaking out will prevent it to become NULL
		nextToken=token->next;
		if(nextToken==NULL)break;
		if(nextToken->type!=TT_COMMENT){
			// if the predecessor of nextToken is not equal to token, we've skipped one or more comments
			prevToken=nextToken->prev;
			// MDH@30MAR2024: you know prevToken must be a 'removed' comment token, however 
			//                it's probably best to test this explicitly 
			if(prevToken!=token){
				// find the first token going back in the 'removed' comment token chain until we bump into token!! 
				while(prevToken!=NULL&&prevToken->prev!=token)
					prevToken=prevToken->prev;
				// NOTE prevToken represents the last prevToken that points backwards to token (as it should)
				if(prevToken!=NULL){
					token->next=prevToken;
					if(prevToken->type!=TT_COMMENT)
						outputMessage(M_BUG_PREFIX,"A disconnected non-comment token of type %s encountered.\n",TOKENTYPE_STRING[prevToken->type]);
				}else
					outputBug("Removed tokens cannot be reinserted");
			}
		}else
			outputBug("A comment encountered in the uncommented command");
		token=nextToken;
	}
	// endcommenttoken ends the command but cannot be reinserted by the above loop
	if(endCommentToken!=NULL)
		endCommentToken->prev->next=endCommentToken;
	if(amVerboseDebugging()){
		// check!!!
		size_t tokenCount=0;
		while(firstToken!=NULL){
			if(firstToken->type==TT_COMMENT)outputToken(firstToken,NULL);
			outputChar('\n');
			firstToken=firstToken->next;
			tokenCount++;
		}
		output("Number of original tokens: %zd.\n",tokenCount);
	}
}
// anything the user types is a sequence of tokens which we can store in a linked list
// MDH@14NOV2019: passing in the address for storing the Mvalue* of the evaluation result
//				instead of returning a bool we could return the command text (or NULL if failing to do so????)
/**
 * @brief evaluates the current user input command
 * 
 * @param resultValue stores the result value
 * @return true on success
 * @return false on failure
 */
bool evaluateCommand(Mvalue* *resultValue){Mallocationowner owner=getOwner(__LINE__);
	
	/// NOT HERE!! outputChar('\n'); // indicating that the command is being evaluated!!!
	int8_t aValidCommandIndicator=isAValidCommandIndicator(_userInputCommand/*,owner_userInputCommand*/,true);
	if(aValidCommandIndicator<=0){
		// MDH@20FEB2020: this is what we did in isAValidCommand() before, but removed from it: if the command ends with an error, we remove the error token, unfinish the (new) last token, so we can re-use it
		if(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL&&_userInputCommand->_lastToken->type==TT_ERROR){
			removeLastUserInputCommandToken();
			if(NULL==_userInputCommand->_lastToken)_userInputCommand=NULL;else unfinishCommandToken(_userInputCommand->_lastToken);
		}
		//outputMessage(M_ERROR_PREFIX,"Invalid command indicator: %d.",aValidCommandIndicator);
		return false;
	}

	allocationMarksAdded=0;
	if(amVerbose()){
		if(allocationMarkAdded())
			allocationMarksAdded++;
		else
			outputError("Failed to mark the allocation before evaluating the command"); // mark the allocations at the start of evaluating a command!!!
	}

	if(amVerboseDebugging())
		output("Number of allocated/freed formula elements before evaluating the command: (%zd,%zd).\n",getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));

	// evaluating means getting the value of the expression that _userInputCommand->_firstToken points to
	// NOTE that the first token is always a dummy token (which will at most contain the whitespace at the start of the command)
	///////////output("Requesting the command text!\n");
	Mstring* _commandText=owned_string(_getCommandText(true),owner); // MDH@13MAR2020 TODO determine later???????

	// MDH@12JUL2023: let's sanitize the command by removing all comments
	// MDH@17JUL2023: 'removing' the comments temporarily allows us to uncomment the command after evaluation!!
	Mtoken* endCommentToken=uncommentCommand(); 
	// replacing: sanitizeCommand();

	//////////output("Command text '%s'.\n",string(_commandText));
	// plug the token following the dummy starting token of the command into the current execution environment (typically _Menvironment I suppose)
	clock_t before_evaluating=clock();
	
	// MDH@17JUL2023: check if there are any command tokens at all
	Mtoken* firstCommandToken=_userInputCommand->_firstToken->next;
	if(firstCommandToken==NULL)output("No command tokens!");
	
	getExecutionEnvironment()->expressionToken=firstCommandToken;

	 // initialize the (current) expression token
	*resultValue=getValueOfExpression("command",'e',(TokenType[]){},0);
	
	//outputValue("Result value: '",*resultValue,"'.\n"); // DEBUG

	long long elapsed_evaluating=(clock()-before_evaluating)/M_CLOCKS_PER_MS;

	recommentCommand(endCommentToken);

	resetOutputColor(); // MDH@02OCT2019: given that the out() might've been used to write stuff to the console in weird colorings TODO doesn't seem to help	
	if(elapsed_evaluating>0)
		output("The evaluation took %lld ms.\n",elapsed_evaluating); // MDH@13MAR2020: because the output text can take long to show
	/*
	Mstring* _showResultTimestamp=owned_string(_getTimestamp(NULL),owner);
	if(_showResultTimestamp){
		outputToFile("@",string(_showResultTimestamp),":\n");
		FREE_STRING(_showResultTimestamp,owner);
	}else
		outputError("Failed to obtain a timestamp");
	echoToOutputFile();
	*/

	// output the commandText
	if(_commandText!=NULL){
		output("%s",string(_commandText));
		FREE_STRING(_commandText,owner);
	}else
		outputError("Failed to obtain the command text");
	output(" = ");

	// if the result is a null value, show the NULL_value
	clock_t before_writing=clock();
	size_t written=outputValueColored(isValueNull(*resultValue)?NULL_value:*resultValue);
	long long elapsed_writing=(clock()-before_writing)/M_CLOCKS_PER_MS;
	
	newline(); // outputValueColored() doesn't do that!!

	// MDH@19AUG2024: let's now collect the same thing but without color!
	Mstring* _commandResultText=owned_string(__string(),owner);
	Mstring* _uncoloredCommandText=owned_string(_getCommandText(false),owner); // MDH@13MAR2020 TODO determine later???????
	if(_uncoloredCommandText!=NULL){
		string_append(_commandResultText,string(_uncoloredCommandText));
		// replacing: q2collect("%s",string(_uncoloredCommandText));
		FREE_STRING(_uncoloredCommandText,owner);
	}else
		string_append_char(_commandResultText,'?'); // replacing: q2collect("%sFailed to obtain the command text",M_ERROR_PREFIX);
	string_append(_commandResultText," = "); // replacing: q2collect("%s"," = ");
	Mstring* _resultText=owned_string(_getValueText(isValueNull(*resultValue)?NULL_value:*resultValue,false,false),owner);
	if(_resultText!=NULL){
		string_append(_commandResultText,string(_resultText)); // replacing: q2collect("%s\n",string(_resultText));
		FREE_STRING(_resultText,owner);
	}else
		string_append_char(_commandResultText,'?'); ///q2collect("%s\n","Failed to obtain the result text");
	// we need to collect the result so it ends up as registered message, but not output it
	if(!collectMessage(M_RESULT_PREFIX,"%s",string(_commandResultText)))
		outputError("Failed to register the result message");
	FREE_STRING(_commandResultText,owner);
	/*
	Mstring* _doneShowingResultTimestamp=owned_string(_getTimestamp(NULL),owner);
	if(_doneShowingResultTimestamp){
		outputToFile(">",string(_doneShowingResultTimestamp),"\n");
		FREE_STRING(_doneShowingResultTimestamp,owner);}
	else
		outputError("Failed to obtain a timestamp");
	dontEchoToOutputFile();
	*/
	if(elapsed_writing>0)
		output("The writing took %lld ms.\n",elapsed_writing);

	///// outputValueColored() does do this (and should): resetOutputColor();
	if(written>1000)
		if(elapsed_evaluating>0)output("The evaluation took %lld ms.\n",elapsed_evaluating);/////////else output("less than 1 ms.");

	///////////////decrementReferenceCount(_commandExpressionValue); if(amVerbose())outputInfo("Result released!"); // TODO do we need to do this?????

	///////if(amVerbose())outputInfo("Command to release!");

	if(amVerboseDebugging())
		output("Number of allocated/freed formula elements after evaluating the command: (%llu,%llu).\n",getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));

	////////if(amVerbose())outputInfo("Command released!");
	return true;

}

// MDH@24APR2019: outputCommand() writes the command to evaluate, and sets _userInputCommand->_lastToken in the process
// MDH@31OCT2019: there's a complication when the last character in the token is the newline character
// MDH@24SEP2020: now returning the position on the last command line 
/**
 * @brief outputs command \p command
 * 
 * @param command 
 * @return size_t the position of the cursor movement (on the current line)
 */
size_t outputCommand(Mcommand * const command){
	Mtoken* token=(command!=NULL?command->_firstToken:NULL);
	Mcursormovement cursormovement={}; // MDH@24SEP2020: outputToken will update cursormovement accordingly so that cursormovement.position will always be the position on the command line to store in the position field of the next token
	while(token!=NULL){
		// MDH@16OCT2020: doing the following is debatable as we could argue that the command itself hasn't changed since it was entered but consider the fact that the number of available line characters could have changed!!!!!
		if(token->position!=cursormovement.position){
			// output("[%zd!=%zd]",token->position,cursormovement.position); // MDH@16OCT2020 (M_MODULE_DEBUGGING&MM_MAIN)
			token->position=cursormovement.position; // MDH@24SEP2020: adding this because the prompt length might've changed in which case token->position would not be correct anymore
		}
		// MDH@24SEP2020: passing the cursor movement in to outputToken in order to keep track of where to place the continued prompts
		outputToken(command->_lastToken=token,&cursormovement);
		// replacing: numberOfBehindPromptCharactersWritten+=outputToken(command->_lastToken=token);
		/* MDH@07JUL2020 CORRECTION outputToken() also takes care of showing continued prompts: 
		// are we supposed to generate a newline?
		// NOTE we're assuming there that a string can never end with this character which is also the text escape character (which requires another character following it)
		if(string_last_char(command->_lastToken->text)==M_NEWLINE_CHARACTER)showContinuedPrompt(true);
		*/
		token=token->next;
	}
	// the last line could be full
	if(numberOfLineCharacters>0&&cursormovement.position==numberOfLineCharacters-promptLength){
		showContinuedPrompt(cursormovement.written,false);
		cursormovement.position=0;
	}
	numberOfBehindPromptCharactersWritten=cursormovement.written; // MDH@24SEP2020: if we stick to the definition of numberOfBehindPromptCharactersWritten!!!!!!
	// MDH@24SEP2020: don't know whether the following is still used somewhere TODO check if numberOfBehindPromptCharactersWritten is now obsolete (as we have numberOfCommandLineCharacters now)
	return cursormovement.position;
}

// MDH@24SEP2020: not expecting the shell to output a token on a session command line, so outputTokenText replaces outputToken (which is now declared differently)
size_t outputTokenText(Mtoken const * const _token){
	if(NULL==_token)return 0;
	outputTokenColor(_token);
	return output("%s",string(_token->text));
}
/**
 * @brief the command page to show
 * 
 */
uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
/**
 * @brief the total number of command pages
 * 
 */
uint32_t commandPages=0; // the total number of command pages
/**
 * @brief sets the current command page to \p createUserInputCommandPage and shows the commands on that page
 * 
 * @param createUserInputCommandPage 
 */
void setCommandPage(uint32_t createUserInputCommandPage){
	commandPage=createUserInputCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	Menvironment* environment=getExecutionEnvironment();
	if(NULL==environment){outputBug("Environment vanished setting the command page");return;}
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Mtoken* token=_registeredcommands[lastCommandToShowIndex+commandToShowIndex]._command->_firstToken;
		// MDH@24SEP2020: this is definitely an issue because outputToken() assumes the token is written on the command line which in this case is not the case, so we shouldn't use outputToken()
		while(token!=NULL){
			outputTokenText(token); // write the token text in the color of it's type
			// replacing: outputToken(token);
			token=token->next;
		}
		outputChar('\n');
	}
	resetOutputColor();
	output("%s","Select the last digit of the command to use, or the up/down key to show the next/previous page.");
	output("%c%c%c%c",' ','>','>',' '); // TODO what kind of prompting do we want to do???
	/* MDH@09NOV2022: read the character NO the main input loop is doing that!!!
	char c;
	while(!inputCharRead(&c));
	outputChar(c);newline();
	switch(c){
		case '\n': 
			output("No command selected!");break;
		case '\e': 
			output("Escape character received!");
			if(inputCharRead(&c)){
			if(c==91){
				if(inputCharRead(&c)){
					if(c==51){
						if(inputCharRead(&c)){
							if(c==126){ // delete
								beep();
							}
						}
					}else
					if(c==65){ // up arrow 
						showNextCommandPage();
					}else
					if(c==66){ // down arrow
						showPreviousCommandPage();
					}else
					if(c==67){ // right arrow
							beep();
					}else
					if(c==68){ // left arrow
							beep();
					}
				}
			}
			break;
		case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
			// TODO what are we going to do????
			break;
		default:
			output("Invalid input character! Try again!");
			setCommandPage(commandPage);
			break;
		}
	}
	// going up and down is an issue
	*/
}
/**
 * @brief shows the next command page
 * 
 */
void showNextCommandPage(){
	if(commandPage<commandPages)
		setCommandPage(commandPage+1);
	else
		output("%s","No further commands to show.");
}
/**
 * @brief shows the previous command page
 * 
 */
void showPreviousCommandPage(){
	if(commandPage>1)
		setCommandPage(commandPage-1);
	else
		output("%s","No further commands to show.");
}
// MDH@25NOV2019: it's more convenient to output the values in control mode than asking for it in command mode
//				as values() command would affect what is returned (given that it's a command)
/**
 * @brief outputs the current values in a table
 * 
 */
void outputValues(){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _valuesTable=owned_list(_getValuesTable(NULL),owner);
	if(_valuesTable!=NULL){
		outputTable(_valuesTable);
		FREE_LIST(_valuesTable,owner);
	}else
		outputError("No values table to output.");
}
/*
// when the user tries to insert a character we need to cut off the rest of the command and append it afterwards
char* removedRestOfCommand(){
	if(getUserInputLength()<getCommandLength()){
		Mstring* restOfCommand=__string();
		if(restOfCommand!=NULL){
			uint16_t tokenPosition=getUserInputLength()-_userInputCommand->_lastToken->offset;
			if(tokenPosition)string_append(restOfCommand,string_remainder(_userInputCommand->_lastToken->text,tokenPosition));
			string_setlength(_userInputCommand->_lastToken->text,tokenPosition); // the new length of the token (cutting off what's behind it)
			// now to append the text in the rest of the tokens
			Mtoken* token=_userInputCommand->_lastToken->next;
			if(token!=NULL){
				while(token!=NULL){string_append(restOfCommand,string(_userInputCommand->_lastToken->text));token=token->next;}
				freeToken(token); // we'll free all the token starting at the successor of _userInputCommand->_lastToken
				_userInputCommand->_lastToken->next=NULL;
			}
			outputInfo("Rest of command: '%s'.",string(restOfCommand));
			return string(restOfCommand);
		}
	}
	return NULL;
} 
*/
/*
void writeRestOfCommand(){ // writes rest of command assuming _userInputCommand->_lastToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=getCommandLength()-getUserInputLength();
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(getUserInputLength()>_userInputCommand->_lastToken->offset){ // part of current token to write
			outputTokenColor(_userInputCommand->_lastToken);printf("%s",string_remainder(_userInputCommand->_lastToken->text,getUserInputLength()-_userInputCommand->_lastToken->offset));
		}
		// write the rest of the tokens
		writeTokens(_userInputCommand->_lastToken->next);
		moveCursorLeft(leftToWrite);
	}
}
*/
/**
 * @brief sets the input mode to \p newInputMode
*/
void setInputMode(enum INPUTMODE_ENUM newInputMode){
	inputMode=newInputMode;
	deleteTokenautocompletiontexts();
	// in command mode the behind cursor text is determined JIT, whereas it is actively used in the other modes!!!
	if(_suggestedText!=NULL)string_setlength(_suggestedText,0/*,owner_suggestedText*/);
}
/**
 * @brief switches to control mode showing \p message
 * 
 * @param message 
 * @return char the character that makes the user input loop end
 */
char switchToControlMode(char* message){
	if(inputMode!=IM_CONTROL){
		if(inputMode==IM_COMMAND)clearCommand();
		// MDH@03APR2024: if there's a message let's beep!!!
		if(message!=NULL){
			outputError(message);
			/*
			newline();
			setColor(getErrorColor());
			outputLine(message);
			*/
			beep();
		} // MDH@01OCT2019: message will typically be an error so
		resetOutputColor();
		setInputMode(IM_CONTROL);
		if(amVerbose())outputValues(); // MDH@25NOV2019: it's convenient to also output the values when verbose
		outputVariables(); // immediately show the list of available variables (so we can then use v for verbose flag, AND s is available for Shell again!!!!)
	}
	///////////outputFlags(); // show the user the current flags!!
	return 'o'; // to make the loop know to quit
	//output("%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}
/**
 * @brief returns the number of characters in the auto completion text M string
 * 
 * @return size_t the number of characters in the auto completion text M string
 */
size_t getNumberOfAutoCompletionCharacters(){return string_length(_autoCompletionText);} // TODO assuming _autoCompletionText has actually been constructed!!!!
/**
 * @brief returns the number of token auto completion texts
 * 
 * @return size_t the number of token auto completion texts
 */
size_t getNumberOfTokenAutoCompletionTexts(){
	size_t numberOfTokenAutoCompletionTexts=0;
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){numberOfTokenAutoCompletionTexts++;tokenautocompletiontext=tokenautocompletiontext->_next;}
	return numberOfTokenAutoCompletionTexts;
}

// MDH@16JAN2024: keep track of the source of the first character in the suggested text
//                this way there's no need to actually construct _suggestedText
unsigned char suggestedTextSources[]={0,0,0,0,0}; // there are 4 possible text sources but the actually sources

// MDH@23SEP2020: perhaps more convenient to keep track of any cursor displacement
/**
 * @brief outputs the manual feed forward characters
 * 
 * @param _cursormovement the updated cursor movement
 */
void outputManualFeedforwardCharacters(Mcursormovement* _cursormovement){
	if(NULL==_cursormovement)return;
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	// MDH@04OCT2019: append it to the suggested text
	if(string_length(_manualFeedforwardText)>0){
		if(numberOfIdentifierContinuationManualFeedforwardCharacters>0){
			char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationManualFeedforwardCharacters);
			// MDH@22SEP2020: update cursormovement by outputting the identifier continuation feed forward characters
			outputCommandLineText(string(_manualFeedforwardText),_cursormovement,getIdentifierContinuationTextColor(),-1);
			// replacing:setColor(getIdentifierContinuationTextColor());numberOfManualFeedforwardCharactersWritten=output("%s",string(_manualFeedforwardText));
			string_setchar(_manualFeedforwardText,c,numberOfIdentifierContinuationManualFeedforwardCharacters); // OOPS put it back before showing not after showing!!!
		}
		// now write the rest in the manual feed forward text color
		outputCommandLineText(string(_manualFeedforwardText)+_cursormovement->written,_cursormovement,getManualFeedforwardTextColor(),-1); // MDH@23SEP2020 NOTE: remember numberOfManualFeedforwardCharactersWritten is a (position/characters written) ULL
		// replacing:setColor(getManualFeedforwardTextColor());numberOfManualFeedforwardCharactersWritten+=output("%s",string(_manualFeedforwardText)+manualFeedforwardCharactersWrittenSoFar);
		string_setlength(_manualFeedforwardText,_cursormovement->written); // in case not all characters were actually written
		// MDH@16JAN2024: update the suggested text source to 1
		suggestedTextSources[++suggestedTextSources[0]]=1;
		/* replacing:
		if(NULL==string_append(_suggestedText,string(_manualFeedforwardText)))
			_cursormovement->written=0;
		//else string_append_char(_suggestedText,'#');
		*/
	}
}
/**
 * @brief outputs the identifier continuation text characters
 * 
 * @param _cursormovement the updated cursor movement
 */
void outputIdentifierContinuationTextCharacters(Mcursormovement* _cursormovement){
	if(_cursormovement!=NULL&&_identifierContinuationCharacters!=NULL&&strlen(_identifierContinuationCharacters)){
		outputCommandLineText(_identifierContinuationCharacters,_cursormovement,getIdentifierContinuationTextColor(),-1);
		suggestedTextSources[++suggestedTextSources[0]]=2;
	}
	/* replacing:
	if(_cursormovement&&_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)&&string_append(_suggestedText,_identifierContinuationCharacters))
		outputCommandLineText(_identifierContinuationCharacters,_cursormovement,getIdentifierContinuationTextColor(),-1);
	*/
	/* replacing:
	if(!_cursormovement)return;
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	size_t numberOfIdentifierContinuationCharactersWritten=(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0);
	// MDH@04OCT2019: append it to the suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0){
		if(!string_append(_suggestedText,_identifierContinuationCharacters))
			numberOfIdentifierContinuationCharactersWritten=0; // append to suggested text
		// else	string_append_char(_suggestedText,'#');
	}
	if(numberOfIdentifierContinuationCharactersWritten>0)
		outputCommandLineText(_identifierContinuationCharacters,_cursormovement,getIdentifierContinuationTextColor(),-1);
	*/
		// replacing: {setColor(getIdentifierContinuationTextColor());output("%s",_identifierContinuationCharacters);
		// replacing: {size_t numberOfIdentifierContinuationCharactersToWrite=numberOfIdentifierContinuationCharactersWritten;while(numberOfIdentifierContinuationCharactersToWrite){outputChar(' ');numberOfIdentifierContinuationCharactersToWrite--;}
	// return numberOfIdentifierContinuationCharactersWritten; // one less character written than the computed length!!!
}
/**
 * @brief outputs the immediate feed forward characters
 * 
 * @param _cursormovement the updated cursor movement
 */
void outputImmediateFeedforwardCharacters(Mcursormovement* _cursormovement){
	if(_cursormovement!=NULL&&string_length(_immediateFeedforwardText)){
		outputCommandLineText(string(_immediateFeedforwardText),_cursormovement,getFeedForwardTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));}
		suggestedTextSources[++suggestedTextSources[0]]=3;
	}
	/* replacing:
	if(_cursormovement!=NULL&&string_length(_immediateFeedforwardText)&&string_append(_suggestedText,string(_immediateFeedforwardText))!=NULL)
		outputCommandLineText(string(_immediateFeedforwardText),_cursormovement,getFeedForwardTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));}
	*/
	/* replacing:
	if(!_cursormovement)return;
	unsigned long long numberOfImmediateFeedforwardCharactersWritten=(_immediateFeedforwardText?string_length(_immediateFeedforwardText):0);
	if(numberOfImmediateFeedforwardCharactersWritten>0){
		if(!string_append(_suggestedText,string(_immediateFeedforwardText)))
			numberOfImmediateFeedforwardCharactersWritten=0; // append to suggested text
		// else	string_append_char(_suggestedText,'#');
	}
	if(numberOfImmediateFeedforwardCharactersWritten>0)
		outputCommandLineText(string(_immediateFeedforwardText),_cursormovement,getFeedForwardTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));}
	*/
}

/**
 * @brief outputs expected characters stored in _expectedCharacterStack as part of the feedforward text
 * @details appends the expected characters to _suggestedText
 * @param _cursormovement the cursor movement updated to keep track of the current line and cursor position
 */
void outputExpectedCharacters(Mcursormovement* _cursormovement){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_cursormovement)return;
	Mchars *_chars=owned_chars(_getReversedChars(string(_expectedCharacterStack)),owner);
	if(NULL==_chars)return;
	// MDH@16JAN2024: keep track of the source of the first character in the suggested text
	//                this way there's no need to actually construct _suggestedText
	outputCommandLineText(_chars->chars,_cursormovement,getExpectedCharacterStackTextColor(),-1);
	// MDH@16JAN2024: updating suggestedTextSources
	suggestedTextSources[++suggestedTextSources[0]]=4;
	/* replacing:
	if(string_append(_suggestedText,_chars->chars)!=NULL)
		outputCommandLineText(_chars->chars,_cursormovement,getExpectedCharacterStackTextColor(),-1);
	else
		_cursormovement->written=0;
	*/
	FREE_CHARS(_chars,1,strlen(_chars->chars)+1,'\'',owner); // MDH27DEC2023 TODO: we should adapt FREE_CHARS if possible to use the defaults!!!
}

/**
 * @brief outputs the auto completion characters
 * 
 * @param _cursormovement the updated cursor movement
 */
void outputAutoCompletionCharacters(Mcursormovement* _cursormovement){
	// MDH@05DEC2022: implemented a little bit better
	if(_cursormovement!=NULL&&string_length(_autoCompletionText)&&string_append(_suggestedText,string(_autoCompletionText)))
		outputCommandLineText(string(_autoCompletionText),_cursormovement,getAutoCompletionTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output("%s",string(_autoCompletionText));}
	/* replacing:
	if(!_cursormovement)return;
	unsigned long long numberOfAutoCompletionCharactersWritten=(_autoCompletionText?string_length(_autoCompletionText):0);
	if(numberOfAutoCompletionCharactersWritten>0){
		if(!string_append(_suggestedText,string(_autoCompletionText)))
			numberOfAutoCompletionCharactersWritten=0; // append to suggested text
		// else string_append_char(_suggestedText,'#');
	}
	if(numberOfAutoCompletionCharactersWritten>0)
		outputCommandLineText(string(_autoCompletionText),_cursormovement,getAutoCompletionTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output("%s",string(_autoCompletionText));}
	*/
}
/* replacing:
// MDH@22SEP2020: when suggested text is output on the command input line we need to embed prompt length blanks
//				there's already a outputCommandLineText() function we can call
//				added a position size_t argument that should tell the function where the cursor is currently located
unsigned long long getNumberOfManualFeedforwardCharactersWritten(uint16_t position){
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	unsigned long long numberOfManualFeedforwardCharactersWritten=position; // MDH@23SEP2020: by default returning the original position (and no characters output)
	// MDH@04OCT2019: append it to the suggested text
	if(string_length(_manualFeedforwardText)>0){
		if(numberOfIdentifierContinuationManualFeedforwardCharacters>0){
			char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationManualFeedforwardCharacters);
			// MDH@22SEP2020: the problem is that outputCommandLineText() does NOT return the number of characters
			//				written, but the new position but this we can change
			//				the first 16 bits contain the new position and the remaining 48 bits the number of characters written
			numberOfManualFeedforwardCharactersWritten=outputCommandLineText(string(_manualFeedforwardText),position,getIdentifierContinuationTextColor());
			// replacing:setColor(getIdentifierContinuationTextColor());numberOfManualFeedforwardCharactersWritten=output("%s",string(_manualFeedforwardText));
			string_setchar(_manualFeedforwardText,c,numberOfIdentifierContinuationManualFeedforwardCharacters); // OOPS put it back before showing not after showing!!!
		}
		unsigned long long manualFeedforwardCharactersWrittenSoFar=(numberOfManualFeedforwardCharactersWritten>>16); // already written
		numberOfManualFeedforwardCharactersWritten=outputCommandLineText(string(_manualFeedforwardText)+manualFeedforwardCharactersWrittenSoFar,numberOfManualFeedforwardCharactersWritten&0xFFFF,getManualFeedforwardTextColor()); // MDH@23SEP2020 NOTE: remember numberOfManualFeedforwardCharactersWritten is a (position/characters written) ULL
		numberOfManualFeedforwardCharactersWritten+=(manualFeedforwardCharactersWrittenSoFar<<16); // don't forget to also count the identifier continuation written before
		// replacing:setColor(getManualFeedforwardTextColor());numberOfManualFeedforwardCharactersWritten+=output("%s",string(_manualFeedforwardText)+manualFeedforwardCharactersWrittenSoFar);
		if(numberOfManualFeedforwardCharactersWritten>>16){
			string_setlength(_manualFeedforwardText,numberOfManualFeedforwardCharactersWritten>>16); // just in case not all characters were written!!!
			if(!string_append(_suggestedText,string(_manualFeedforwardText)))numberOfManualFeedforwardCharactersWritten=position;
		}
	}
	return numberOfManualFeedforwardCharactersWritten; // one less character written than the computed length!!!
}
unsigned long long getNumberOfIdentifierContinuationTextCharactersWritten(uint16_t position){
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	unsigned long long numberOfIdentifierContinuationCharactersWritten=(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0);
	// MDH@04OCT2019: append it to the suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0)if(!string_append(_suggestedText,_identifierContinuationCharacters))numberOfIdentifierContinuationCharactersWritten=0; // append to suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0){
		numberOfIdentifierContinuationCharactersWritten=outputCommandLineText(_identifierContinuationCharacters,position,getIdentifierContinuationTextColor());
		// replacing: setColor(getIdentifierContinuationTextColor());output("%s",_identifierContinuationCharacters);
		// replacing: size_t numberOfIdentifierContinuationCharactersToWrite=numberOfIdentifierContinuationCharactersWritten;while(numberOfIdentifierContinuationCharactersToWrite){outputChar(' ');numberOfIdentifierContinuationCharactersToWrite--;}
	}
	return numberOfIdentifierContinuationCharactersWritten; // one less character written than the computed length!!!
}
unsigned long long getNumberOfImmediateFeedforwardCharactersWritten(uint16_t position){
	unsigned long long numberOfImmediateFeedforwardCharactersWritten=(_immediateFeedforwardText?string_length(_immediateFeedforwardText):0);
	if(numberOfImmediateFeedforwardCharactersWritten>0)if(!string_append(_suggestedText,string(_immediateFeedforwardText)))numberOfImmediateFeedforwardCharactersWritten=0; // append to suggested text
	if(numberOfImmediateFeedforwardCharactersWritten>0){
		numberOfImmediateFeedforwardCharactersWritten=outputCommandLineText(string(_immediateFeedforwardText),position,getFeedForwardTextColor());
		// replacing: setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));
	}
	return numberOfImmediateFeedforwardCharactersWritten;
}
unsigned long long getNumberOfAutoCompletionCharactersWritten(uint16_t position){
	unsigned long long numberOfAutoCompletionCharactersWritten=(_autoCompletionText?string_length(_autoCompletionText):0);
	if(numberOfAutoCompletionCharactersWritten>0)if(!string_append(_suggestedText,string(_autoCompletionText)))numberOfAutoCompletionCharactersWritten=0; // append to suggested text
	if(numberOfAutoCompletionCharactersWritten>0){ // additional auto completion text to write
		/////debugWrite("Auto completion characters to write: '%s'.",string(_autoCompletionText));
		numberOfAutoCompletionCharactersWritten=outputCommandLineText(string(_autoCompletionText),position,getFeedForwardTextColor());
		// replacing: setColor(getFeedForwardTextColor());output("%s",string(_autoCompletionText));
	}
	return numberOfAutoCompletionCharactersWritten;
}
*/
/*
// MDH@27SEP2019: when the user just deleted the identifier continuation we would not want it to be generated immediately
void writeSuggestedText(bool updateUserInputCommandIdentifierContinuationText){
	// MDH@26SEP2019: behind cursor text now consists of two parts now: identifier continuation text and feed forward text
	// 0. preparation
	resetOutputColor();
	// we're going to compute the total number of characters written behind the prompt (), so we will know how many blanks we need to append
	// MDH@27SEP2019: the best idea today is to update the identifier continuation text just before showing it
	// 1. write the identifier continuation text and feed forward text
	size_t commandLength=getCommandLength();
	size_t newNumberOfBehindPromptCharactersWritten=commandLength;
	if(updateUserInputCommandIdentifierContinuationText)updateUserInputCommandIdentifierContinuation(); // force an update of the identifier continuation (could have been already done in updateLastTokenAutoCompletionText()!)
	newNumberOfBehindPromptCharactersWritten+=getNumberOfIdentifierContinuationTextCharactersWritten();
	newNumberOfBehindPromptCharactersWritten+=getNumberOfFeedforwardCharactersWritten();
	////////inputInfo("Number of written suggested characters: %zu.",newNumberOfBehindPromptCharactersWritten);
	// 2. write additional blanks overwriting what we had before
	while(newNumberOfBehindPromptCharactersWritten<numberOfBehindPromptCharactersWritten){newNumberOfBehindPromptCharactersWritten++;outputChar(' ');}
	numberOfBehindPromptCharactersWritten=newNumberOfBehindPromptCharactersWritten;
	outputChar(' ');  // one extra to be on the safe size TODO why?????????
	moveCursorLeft(newNumberOfBehindPromptCharactersWritten+1-commandLength); // return to where the command ends
	// 3. finalize: remember the actual number of characters written (and therefore will not be blanks)
	// update numberOfBehindPromptCharactersWritten (we do not need to remember blanks written!!!å)
	if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the color of the current token
}
*/
// MDH@22OCT2021: central point where a consumed suggested character (from _suggestedText) is removed from one of the constituent parts
//				NOTE that _suggestedText itself is not consumed and would therefore need to be updated afterwards to sync it again
//				NOTE that consumption of suggested characters only happens one at a time with right arrow and by token with tab
// MDH@16JAN2024: suggestedTextSources[0] contains the index of the source of the
/**
 * @brief returns the first suggested character
 * @details suggestedTextSources[0] contains the positive index (if any) in suggestedTextSources where the source index of the
 *          suggested text can be found
 * @return char the first suggested character
 */
char getFirstSuggestedCharacter(){
	char firstSuggestedCharacter='\0';
	switch(suggestedTextSources[suggestedTextSources[0]]){
		case 1:firstSuggestedCharacter=string_char(_manualFeedforwardText,0);break;
		case 2:firstSuggestedCharacter=getFirstIdentifierContinuationCharacter();break;
		case 3:firstSuggestedCharacter=string_char(_immediateFeedforwardText,0);break;
		case 4:firstSuggestedCharacter=string_last_char(_expectedCharacterStack);break; // OOPS take the last character not the first!!!
	}
	return firstSuggestedCharacter;
}

/**
 * @brief 'removes' the identifier continuation text
 * @details only places the null character at the first position of _identifierContinuationCharacters
 * @return true 
 * @return false 
 */
bool removeIdentifierContinuationCharacters(){
	if(_identifierContinuationCharacters!=NULL){
		userInputCommandIdentifierContinuationNeedsUpdating=false; // will this work?????
		_identifierContinuationCharacters[0]='\0';
		while(++suggestedTextSources[0]<5&&!suggestedTextSources[suggestedTextSources[0]]); // increment suggestedTextSources[0] does not equal 5 (it should be 2) to find the first new suggested text source
		return true;
	}
	return false;
}

/**
 * @brief removes the first (consumed) suggested character
 * 
 * @param inputChar the consumed character
 * @return true on success
 * @return false on failure
 */
bool removeFirstSuggestedCharacter(char inputChar){
	bool result=true;
	bool sourceRemoved=false;
	// MDH@16JA2024: since we now know the source of the first suggested character, there's no need to guess it anymore
	switch(suggestedTextSources[suggestedTextSources[0]]){
		case 1:
			{
				char firstManualFeedForwardCharacter=string_char(_manualFeedforwardText,0);
				if(inputChar!='\0'&&inputChar!=firstManualFeedForwardCharacter){
					result=false;
					outputMessage(M_ERROR_PREFIX,"First manual feed forward character '%c' does not match the consumed suggested character '%c'.",firstManualFeedForwardCharacter,inputChar);
				}else{
					char manualFeedForwardCharacterRemoved=string_removed_char(_manualFeedforwardText,0);
					if(manualFeedForwardCharacterRemoved=='\0'||(manualFeedForwardCharacterRemoved!=inputChar&&inputChar!='\0')){
						result=false;
						outputMessage(M_ERROR_PREFIX,"Failed to remove the first manual feed forward character '%c'.",firstManualFeedForwardCharacter);
					}else
						sourceRemoved=!string_length(_manualFeedforwardText);
				}
			}
			break;
		case 2:
			{
				char firstIdentifierContinuationCharacter=_identifierContinuationCharacters[0];
				if(inputChar!='\0'&&inputChar!=firstIdentifierContinuationCharacter){
					result=false;
					inputError("First identifier continuation character '%c' does not match the consumed suggested character '%c'.",firstIdentifierContinuationCharacter,inputChar);
				}else{
					// MDH@02NOV2021: here we replaced delete... by firstIdentifierContinuationCharacterRemoved() similar to what we do with the other feed forward constituent parts
					char identifierContinuationCharacterRemoved=firstIdentifierContinuationCharacterRemoved();
					if(identifierContinuationCharacterRemoved=='\0'||(firstIdentifierContinuationCharacter!=inputChar&&inputChar!='\0')){
						result=false;
						logToOutputFile("%sFailed to remove the first identifier continuation character '%c'!",firstIdentifierContinuationCharacter);
					}else{
						sourceRemoved=!strlen(_identifierContinuationCharacters);
						inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
					}
				}
			}
			break;
		case 3:
			{
				char firstImmediateFeedforwardCharacter=string_char(_immediateFeedforwardText,0);
				if(inputChar!='\0'&&inputChar!=firstImmediateFeedforwardCharacter){
					result=false;
					outputMessage(M_ERROR_PREFIX,"%sFirst immediate feed forward character '%c' does not match the input character '%c'.",M_MESSAGE_PREFIX,firstImmediateFeedforwardCharacter,inputChar);
				}else{
					char immediateFeedforwardCharacterRemoved=string_removed_char(_immediateFeedforwardText,0);
					if(immediateFeedforwardCharacterRemoved=='\0'||(inputChar!='\0'&&immediateFeedforwardCharacterRemoved!=inputChar)){
						result=false;
						outputMessage(M_ERROR_PREFIX,"%sFailed to remove the first immediate feed forward character '%c'.",M_MESSAGE_PREFIX,firstImmediateFeedforwardCharacter);
					}else
						sourceRemoved=!string_length(_immediateFeedforwardText);
				}
			}
			break;
		case 4:
			// expectedCharacterStack is a different ball park since it is kept updated based on the current input command
			// and does not need to be adapted by explicitly removing the consumed 
			break;
		default:
			result=false;
	}
	// increment the suggested text source index, if the current suggested text source is depleted
	// NOTE if the suggested text sources ends up becoming 5 it's out of range apparently
	if(sourceRemoved)
		while(++suggestedTextSources[0]<5&&!suggestedTextSources[suggestedTextSources[0]]);
	///inputInfo("Suggested text source: %d.",suggestedTextSources[0]);
	/* replacing:
	if(string_length(_manualFeedforwardText)>0){
		char firstManualFeedForwardCharacter=string_char(_manualFeedforwardText,0);
		if(inputChar!='\0'&&inputChar!=firstManualFeedForwardCharacter){
			result=false;
			outputMessage(M_ERROR_PREFIX,"First manual feed forward character '%c' does not match the consumed suggested character '%c'.",firstManualFeedForwardCharacter,inputChar);
		}else{
			char manualFeedForwardCharacterRemoved=string_removed_char(_manualFeedforwardText,0);
			if(manualFeedForwardCharacterRemoved=='\0'||(manualFeedForwardCharacterRemoved!=inputChar&&inputChar!='\0')){
				result=false;
				outputMessage(M_ERROR_PREFIX,"Failed to remove the first manual feed forward character '%c'.",firstManualFeedForwardCharacter);
			}
		}
	}else
	if(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0){
		char firstIdentifierContinuationCharacter=_identifierContinuationCharacters[0];
		if(inputChar!='\0'&&inputChar!=firstIdentifierContinuationCharacter){
			result=false;
			inputError("First identifier continuation character '%c' does not match the consumed suggested character '%c'.",firstIdentifierContinuationCharacter,inputChar);
		}else{
			// MDH@02NOV2021: here we replaced delete... by firstIdentifierContinuationCharacterRemoved() similar to what we do with the other feed forward constituent parts
			char identifierContinuationCharacterRemoved=firstIdentifierContinuationCharacterRemoved();
			if(identifierContinuationCharacterRemoved=='\0'||(firstIdentifierContinuationCharacter!=inputChar&&inputChar!='\0')){
				result=false;
				outputMessage(M_ERROR_PREFIX,"Failed to remove the first identifier continuation character '%c'!",firstIdentifierContinuationCharacter);
			}
		}
	}else
	if(string_length(_immediateFeedforwardText)>0){
		char firstImmediateFeedforwardCharacter=string_char(_immediateFeedforwardText,0);
		if(inputChar!='\0'&&inputChar!=firstImmediateFeedforwardCharacter){
			result=false;
			outputMessage(M_ERROR_PREFIX,"First immediate feed forward character '%c' does not match the input character '%c'.",firstImmediateFeedforwardCharacter,inputChar);
		}else{
			char immediateFeedforwardCharacterRemoved=string_removed_char(_immediateFeedforwardText,0);
			if(immediateFeedforwardCharacterRemoved=='\0'||(inputChar!='\0'&&immediateFeedforwardCharacterRemoved!=inputChar)){
				result=false;
				outputMessage(M_ERROR_PREFIX,"Failed to remove the first immediate feed forward character '%c'.",firstImmediateFeedforwardCharacter);
			}
		}
	}else
	if(string_length(_expectedCharacterStack)>0){
		char firstExpectedCharacter=string_last_char(_expectedCharacterStack);
		if(inputChar!='\0'&&inputChar!=firstExpectedCharacter){
			result=false;
			outputMessage(M_ERROR_PREFIX,"First expected character '%c' does not match the consumed suggested character '%c'.",firstExpectedCharacter,inputChar);
		}else{
			string_declength(_expectedCharacterStack); // this way nothing can go wrong
			///replacing:
			///char expectedCharacterRemoved=firstExpectedCharacterRemoved(); // MDH@02NOV2021: now reinstated to remove the first autocompletion character!!
			///if(expectedCharacterRemoved=='\0'||(inputChar!='\0'&&expectedCharacterRemoved!=inputChar)){ // replacing: if(!deleteFirstAutoCompletionCharacter(suggestedChar,true)){ // replacing: if(suggestedChar!=string_removed_char(_autoCompletionText,0))
			///	result=false;
			///	outputMessage(M_ERROR_PREFIX,"Failed to remove the first expected character '%c'.",firstExpectedCharacter);
			///}
		}
	}
	///replacing:
	///if(string_length(_autoCompletionText)>0){
	///	// BUG FIX: _autoCompletionText is like _suggestedText a composed text i.e. it is the concatenation of all token auto completion texts
	///	//		  therefore you need to remove the first of the characters in the token auto completion texts instead
	///	// NOTE this is actually the only place where deleteFirstAutoCompletionCharacter is called!!!!!
	///	char firstAutoCompletionCharacter=string_char(_autoCompletionText,0); // NOTE __not__ using getFirstAutoCompletionCharacter anymore!!!!
	///	if(inputChar!='\0'&&inputChar!=firstAutoCompletionCharacter){
	///		result=false;
	///		outputMessage(M_ERROR_PREFIX,"First auto completion character '%c' does not match the consumed suggested character '%c'.",firstAutoCompletionCharacter,inputChar);
	///	}else{
	///		char autocompletionCharacterRemoved=firstAutoCompletionCharacterRemoved(); // MDH@02NOV2021: now reinstated to remove the first autocompletion character!!
	///		if(autocompletionCharacterRemoved=='\0'||(inputChar!='\0'&&autocompletionCharacterRemoved!=inputChar)){ // replacing: if(!deleteFirstAutoCompletionCharacter(suggestedChar,true)){ // replacing: if(suggestedChar!=string_removed_char(_autoCompletionText,0))
	///			result=false;
	///			outputMessage(M_ERROR_PREFIX,"Failed to remove the first auto completion character '%c'.",firstAutoCompletionCharacter);
	///		}
	///	}
	///}
	else{
		result=false;
		if(inputChar!='\0')
			outputMessage(M_ERROR_PREFIX,"Suggested input character '%c' not consumed!",inputChar);
		else
			outputError("Failed to find (and consume) the first suggested character!");
	}
	*/
	return result;
}
/*
// MDH@02NOV2021: consuming a suggested character depends on the current input character and the suggested text
//				if we fail to remove the matching 
void consumeFirstSuggestedCharacter(char inputChar){
	// should only fail when the first suggested character matches inputChar, and failing to remove that character
	if(inputChar){
		if(inputChar==getFirstSuggestedCharacter()&&!removeFirstSuggestedCharacter(inputChar))
			inputError("%sFailed to consume suggested character '%c'. See the log file for details!",M_BUG_PREFIX,inputChar);
	}else{ // typically the result of arrow right
		if(!removeFirstSuggestedCharacter('\0'))
			logToOutputFile("%sFailed to consume the first suggested character. See the log file for details!");
	}
}
*/

// MDH@01OCT2019: it's better to show the suggested text JIT i.e. just before asking the user for input
//				this is also better because at that moment we know the user should be seeing it
/**
 * @brief the number of suggested characters written
 * 
 */
unsigned long long numberOfSuggestedCharactersWritten=0; // MDH@23SEP2020: basically storing both line position and number of characters written but corrected in showSuggestedText
// MDH@30JUN2020: the suggested text can also soft-break onto next lines!!!!
/**
 * @brief updates and shows the entire suggested text
 * 
 */
void showSuggestedText(){
	// MDH@16JAN2024: initialize suggestedTextSources[0] to 0 indicating there is NO suggested text
	memset(suggestedTextSources,0,sizeof suggestedTextSources); // reset the suggested text sources
	/* replacing:
	// ASSERT _suggestedText should not be NULL
	if(NULL==_suggestedText)return;
	// MDH@26SEP2019: behind cursor text now consists of two parts now: identifier continuation text and feed forward text
	// 0. preparation
	string_setlength(_suggestedText,0); // clear the suggested text!!!
	*/
	// 0. the current cursor position (in the command) is the number of line command characters
	Mcursormovement cursormovement={numberOfLineCommandCharacters};

	// MDH@20JAN2024: if there's manual feed forward text it's best NOT to show any other feed forward
	if(!string_length(_manualFeedforwardText)){ // user is not editing the command entered so far (manual feedforward text)
		
		outputIdentifierContinuationTextCharacters(&cursormovement);
	
		// MDH@17JUL2024: if there is no identifier continuation text we allow immediate feedforward characters
		if(cursormovement.written==0)outputImmediateFeedforwardCharacters(&cursormovement);

		// MDH@27DEC2023: if we do not have any suggested text yet, show the last feedforward closer character
		// MDH@17JAN2024: for now we show all
		if(cursormovement.written==0)outputExpectedCharacters(&cursormovement);

	}else
		outputManualFeedforwardCharacters(&cursormovement);
	
	// MDH@21JAN2024: suggestedTextSources[0] should point to the first suggested text to use
	if(suggestedTextSources[0])suggestedTextSources[0]=1;
	//// replacing: outputAutoCompletionCharacters(&cursormovement);

	// MDH@15OCT2020: returning to the position where command characters should be input depends on cursor position changes as remembered in cursormovement
	//				the commented code below did not work when dealing with explicit newlines, therefore we need to use cursormovement explicitly to determine what to do with the cursor
	//				in order to do so, I added lines field to Mcursormovement (replacing skipped) to contain the number of lines moved down

	// (M_MODULE_DEBUGGING&MM_MAIN): output("[%zd]",(_userinputline?_userinputline->offset:0));
	clearScreenFromCursor(); // MDH@05DEC2022: clear all earlier visible suggested text (if any)

	toUserInputCursorPosition(-cursormovement.lines);

	logToOutputFile("\t\t\tSuggested text: '%s'\n",string(_suggestedText));
	/* replacing:
	numberOfSuggestedCharactersWritten=cursormovement.written+cursormovement.skipped;

	if(amVerboseDebugging())
		numberOfSuggestedCharactersWritten+=output("[%zd-%zd=%zd,%zd]",getUserInputLength(),numberOfLineCommandCharacters,(_userinputline?_userinputline->offset:0),(_userinputline?_userinputline->index:0));

	moveCursorLeft(numberOfSuggestedCharactersWritten);
	
	// 3. finalize: remember the actual number of characters written (and therefore will not be blanks)
	// update numberOfBehindPromptCharactersWritten (we do not need to remember blanks written!!!å)
	outputUserInputCommandTokenColor(); // replacing: if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the color of the current token
	*/
}
// MDH@23SEP2020 TODO: hiding the suggested text involves removing the prompt characters embedded in the suggested text outputted
/**
 * @brief hides the suggested text by overwriting it with blanks
 * 
 */
void hideSuggestedText(){
	long long numberOfBlanksToWrite=numberOfSuggestedCharactersWritten;
	size_t numberOfBlanksOutput=0; // MDH@03APR2023: a little more precise to count the blanks written
	while(--numberOfBlanksToWrite>=0)numberOfBlanksOutput+=outputChar(' ');
	///////outputChar(' ');
	moveCursorLeft(numberOfBlanksOutput);
}
/**
 * @brief outputs the shell command
 * 
 */
void outputShellCommand(){
	// we're supposed to wrap write the shell command
	if(numberOfLineCharacters>0){
		uint16_t maximumNumberOfLineCommandCharacters=numberOfLineCharacters-promptLength-1;
		size_t l=string_length(_shellCommand),left=maximumNumberOfLineCommandCharacters;
		if(left<l){
			for(size_t shellCommandIndex=0;shellCommandIndex<l;shellCommandIndex++){
				outputChar(string_char(_shellCommand,shellCommandIndex));
				if(--left==0){showContinuedPrompt(shellCommandIndex,false);left=maximumNumberOfLineCommandCharacters;}
			}
			return;
		}
	}
	output("%s",string(_shellCommand));
}
/**
 * @brief returns to the prompt
 * 
 */
void backToPrompt(){
	// MDH@27SEP2019: assuming for now that whatever is written next will determine the number of characters written behind the prompt
	numberOfBehindPromptCharactersWritten=0; // assume no text characters written so far
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	// replacing: restoreCursor();clearScreenFromCursor();

	// MDH@31OCT2019: with a clearScreenFromCursor() following it suffices to first move to the initial prompt line
	size_t linesfreed=free_userinputline();
	while(linesfreed>0){linesfreed--;oneLineUp();}
	toStartOfLine();
	moveCursorRight(promptLength); // should now be at the right position for clearing
	/* replacing:
	uint16_t cp=getUserInputLength();
	while(cp--)backspace(); // MDH@24APR2019 replacing: while(getUserInputLength()>0){getUserInputLength()--;backspace();}
	*/
	/*
	if(getUserInputLength()>0){moveCursorLeft(getUserInputLength());getUserInputLength()=0;}
	clearScreenFromCursor();
	*/
	/* replacing:
	while(characterCount>0){
		characterCount--;
		moveCursorLeft(1);resetOutputColor();outputChar(' ');moveCursorLeft(1);
	}
	*/
}

// MDH@22JUN2020: updateNumberOfLineCharacters() is to be called AFTER a character is input by the user and BEFORE that character is processed
// MDH@29JUN2020: let's use two different strategies depending on whether or not we know the change to the number of lines the command now occupies
//				1. clear the screen before we rewrite the entire command (if we do not know the number of lines the command now occupies),  
/**
 * @brief updates the number of characters available on a line
 * @details rewrites the current (shell) command when the number of line characters changed
 * @return true on success
 * @return false on failure
 */
bool updateNumberOfLineCharacters(){
	int newNumberOfLineCharacters=getCurrentNumberOfWindowTextColumns();
	if(newNumberOfLineCharacters==numberOfLineCharacters)return true; // no change to the viewport width
	// taken care of by promptForUserInput()!!!! numberOfLineCharacters=newNumberOfLineCharacters;
	// printf("%c",'X'); // DEBUG
	// it is essential to realize that no changes need to be made iff the number of lines the command uses does not change
	// although newNumberOfLineCharacters is positive it could be smaller than promptLength
	// how about demanding newNumberOfLineCharacters to be at least promptLength+2!!!!
	if(newNumberOfLineCharacters>0&&newNumberOfLineCharacters<promptLength+2)return false; // if defined but too small to fit at least one command character behind the prompts
	/* MDH@02JUL2020: these exceptions can simply be combined with the input mode tests once (below)
	if(inputMode==IM_COMMAND){
		// inputInfo("New number of line characters: %i.",newNumberOfLineCharacters); // DEBUG
		// if we do not have at least one user input command characters, there's nothing to do NOTE a non-empty command has at least two tokens TODO check this assumption
		// it's a bit of a nuisance that the first token can have no characters in it but if it does first and last token will be the same so we still have to test the number of characters
		if(!_userInputCommand||!_userInputCommand->_firstToken||(_userInputCommand->_firstToken==_userInputCommand->_lastToken&&string_length(_userInputCommand->_lastToken->text)==0))return true;
			// removing:
			//// determine how many extra lines we got depending how many characters are on each line
			//// but wait: due to the change of line characters the window has been wrapping/unwrapping
			//// technically it's easiest to simply write the entire command again
			//size_t newNumberOfCommandLines=1; // the current number of command lines
			//// redetermine the position of where the token should be placed
			//size_t maximumNumberOfLineCommandCharacters=(newNumberOfLineCharacters>0?newNumberOfLineCharacters-promptLength-1:0);
			//size_t l,left=maximumNumberOfLineCommandCharacters; // what's left on the first line of the command for command characters
			//Mtoken* token=_userInputCommand->_firstToken;
			//while(token){
			//	l=string_length(token->text);
			//	if(l>0){ // there are characters in the token (e.g. most of the time the first (expression) token will be empty)
			//		// this token either fits on the current line or it does not but if newNumberOfLineCharacters equals zero it always does
			//		if(maximumNumberOfLineCommandCharacters>0){ // a limited amount of characters fit on the current line, so there could be any number of soft-breaks
			//			if(l>left){ // at least one soft-break
			//				// decrease l by left and by the total number of characters that fit on a single line until 
			//				do{
			//					l-=left; // all (remaining) characters fit the rest of the line
			//					newNumberOfCommandLines++;
			//					left=maximumNumberOfLineCommandCharacters; // what's left on the line for command characters
			//				}while(l>left);
			//			}
			//			// ASSERT l is smaller than or equal to left
			//			left-=l;
			//		}
			//		// if the last token character (which always exists as l>0) equals M_NEWLINE_CHARACTER we have a hard-break 
			//		if(string_last_char(token->text)==M_NEWLINE_CHARACTER){left=maximumNumberOfLineCommandCharacters;newNumberOfCommandLines++;}
			//	}
			//	// determine the new line and position based on the last token
			//	token=token->next;
			//}
			//if(newNumberOfCommandLines!=getNumberOfCommandLines()){
				//*/
				// output("%i",newNumberOfCommandLines); // DEBUG
				// ASSERT line now contains the number of lines the command will occupy when displayed again
				// go to the line where the prompt now is
				///*
				//clearScreenFromCursor(); // get rid of the suggested text
				//oneLineDown();toStartOfLine(); // move to the start of the next line
				//*/
				// replacing:
				///* replacing:
				//while(newNumberOfCommandLines>0){oneLineUp();newNumberOfCommandLines--;}toStartOfLine();moveCursorRight(promptLength); // TODO as this is similar to backToPrompt() can we use backToPrompt()????
				//if(newNumberOfLineCharacters==0||numberOfLineCharacters<newNumberOfLineCharacters)clearScreenFromCursor(); // in case we have less lines!!!
				//*/
			// }
			//else outputChar('Y'); // DEBUG
		//}
	//}else
	//if(inputMode==IM_SHELL){ // TODO implement
	//	if(!_shellCommand||string_length(_shellCommand)==0)return true;
	//}
	//*/
	/* no need to clear the screen anymore if we manage to compute the actual number of command lines accurately
	outputControlText("2J"); // clear screen
	outputControlText("H"); // put cursor in top-left corner
	
	// much the same as what happens at the start of requesting a command loop
	outputTotalMemoryUsage();
	promptForUserInput();
	*/

	if(inputMode==IM_COMMAND){
		if(!_userInputCommand||!_userInputCommand->_firstToken||(_userInputCommand->_firstToken==_userInputCommand->_lastToken&&string_length(_userInputCommand->_lastToken->text)==0))return true;
		// MDH@02JUL2020: by experimenting I found out that that the cursor can also move to the next line if it is at the last position
		//				in essence this means that we should actually accept that there's ONE additional position at the end of a line that is available for a character even though we are not using it
		//				e.g. imagine that the user would decrement the viewport width by 1 this would NOT result in extra command lines because if it changed from 20 to 19 we'd have at most 19 characters on the line
		//				which fit perfectly
		// only when the new number of line characters is smaller should we redetermine how many lines back the prompt is
		// if it is larger we assume that we the number of command lines did not change (visually)
		if(newNumberOfLineCharacters<numberOfLineCharacters){
			
			size_t numberOfExtraCommandLines=0;
			// MDH@03JUL2020: how about using the user input lines instead of iterating over all tokens instead????

			// redetermine the position of where the token should be placed
			// the problem is that for every current line we have to determine
			size_t maximumNumberOfLineCommandCharacters=(numberOfLineCharacters>0?numberOfLineCharacters-promptLength-1:0); // this would be the maximum number of command characters that would be on a single line right now in case of a soft-break continued on the next line
			size_t numberOfLineCommandCharactersThatWouldFit=(newNumberOfLineCharacters>0?newNumberOfLineCharacters-promptLength/*-1*/:0); // one more than what we would actually use...
			// as soon as the number of characters on a line exceeds newMaximumOfLineCommandCharacters we know an extra line is inserted
			
			size_t l,numberOfLineCommandCharactersSoFar=0; // the number of command characters so far
			// we should determine the number of command characters on each line
			// if this number of characters exceeds the new number of line command characters we have to increment numberOfExtraCommandLines
			Mtoken* token=_userInputCommand->_firstToken;
			while(token){
				l=string_length(token->text);
				if(l>0){ // there are characters in the token (e.g. most of the time the first (expression) token will be empty)
					// this token either fits on the current line or it does not but if newNumberOfLineCharacters equals zero it always does
					if(maximumNumberOfLineCommandCharacters>0){ // this token could occupy multiple (original) lines
						// how many characters of the token fit on this line????
						while(l+numberOfLineCommandCharactersSoFar>=numberOfLineCommandCharactersThatWouldFit){ // rest of the current original command line is completely occupied by characters of this token
							// if command characters on this line were wrapped onto the next terminal line increment the number of extra command lines
							if(maximumNumberOfLineCommandCharacters>numberOfLineCommandCharactersThatWouldFit)numberOfExtraCommandLines++;
							// determine the number of characters left in the token
							l-=(numberOfLineCommandCharactersThatWouldFit-numberOfLineCommandCharactersSoFar-1); // subtract the number of token characters that actually are present 
							numberOfLineCommandCharactersSoFar=0; // no characters on start of line
						}
						numberOfLineCommandCharactersSoFar+=l; // update the number of characters used on the line the token ends on
					}
					// if the last token character (which always exists as l>0) equals M_NEWLINE_CHARACTER we have a hard-break 
					if(string_last_char(token->text)==M_NEWLINE_CHARACTER){
						// if this hard-break newline characters wrapped onto the next line we have an additional command line
						if(l+numberOfLineCommandCharactersSoFar>numberOfLineCommandCharactersThatWouldFit)numberOfExtraCommandLines++;
						/* MDH@16OCT2020: you can see that below numberOfLineCommandCharacters is set to the result of outputCommand() (as it should) so there's no need to zero it hear, considering the value is not used in this loop at all
						// we know the next token is on the next line
						numberOfLineCommandCharacters=0;
						*/
					}
				}
				// determine the new line and position based on the last token
				token=token->next;
			}
			// for every extra command line we go one line up
			while(numberOfExtraCommandLines>0){oneLineUp();numberOfExtraCommandLines--;}
		}

		backToPrompt();

		// now to write the command
		// output("UPDATING");moveCursorLeft(10);//sleep(1);
		// we need to free the user input lines first because we're supposed to be right behind the prompt!!
		// taken care of by promptForUserInput(): free_userinputline();
		numberOfLineCharacters=newNumberOfLineCharacters; // we need this before actually showing the tokens
		
		// MDH@24SEP2020: giving that the following is essentially a rewrite of the user input command again we can simply do that
		numberOfLineCommandCharacters=outputCommand(_userInputCommand);
		/* replacing:
		Mtoken* token=_userInputCommand->_firstToken;
		// MDH@07JUL2020: outputToken() now outputs the position on the current command line, which we can use to initialize token->position as used by outputToken()
		// MDH@24SEP2020: position replaced by an Mcursormovement 
		Mcursormovement cursormovement={}; // replacing: size_t position=0;
		while(token){
			token->position=cursormovement.position; // replacing: =position;
			outputToken(token,&cursormovement); // replacing: position=outputToken(token);
			token=token->next;
		}
		numberOfLineCommandCharacters=cursormovement.position; // also essential to end up with the right value for numberOfLineCommandCharacters!!!!!
		*/
		
		// MDH@05DEC2022 now in showSuggestedText!!: clearScreenFromCursor(); // TODO can't harm but not certain about this
		/* MDH@07JUL2020 because we now allow the cursor on the last line position we do not need the following anymore:
		// the cursor could end up on the last available position on the command line (i.e. when the last command line is full) where it is never supposed to be at
		numberOfLineCommandCharacters=getUserInputLength()-(_userinputline?_userinputline->offset:0);
		if(numberOfLineCommandCharacters+promptLength+1>=numberOfLineCharacters)showContinuedPrompt();
		*/
		showSuggestedText();

	}else
	if(inputMode==IM_SHELL){
		if(!_shellCommand||string_length(_shellCommand)==0)return true;
		numberOfLineCharacters=newNumberOfLineCharacters; // we need this before actually showing the tokens
		// TODO output the shell command wrapped
		outputShellCommand();
	}
	
		// inputInfo("New number of line characters: %i.",numberOfLineCharacters);
		// outputChar('X');
	return true;
}

// MDH@13DEC2023: after an input character is accepted (and successfully appended to the current command)
//                it is used to update the expended characters accordingly
/**
 * @brief updates the expected characters after appending \p inputChar successfully to the input command
 * 
 * @param inputChar the accepted input character
 * @param currentToken the current input command token
 * @return int8_t positive on success, zero on input failure, negative on failing to update expected characters
 */
int8_t expectedCharacterStackUpdatedOnAddition(char inputChar,Mtoken const * const currentToken){
	if(NULL==_expectedCharacterStack)return 0;
	if(currentToken!=NULL&&currentToken->type==TT_ERROR)return 0; // MDH@18JUN2024: when in an error do not append!!
	// if the last expected character indicates we're in a string literal inputChar is considered part of that string literal
	char lastExpectedCharacter=string_last_char(_expectedCharacterStack);
	if(lastExpectedCharacter=='"'||lastExpectedCharacter=='\''){
		// only when inputChar actually ends the current token
		// (technically the end of string literal token could be escaped!!!)
		if(inputChar==lastExpectedCharacter){
			if(isTokenFinished(currentToken)){
				if(NULL==string_declength(_expectedCharacterStack)){
					inputError("Failed to register '%c' as expected character",inputChar);
					return -3;
				} // 'removes' the last expected token by decrementing the string length
			}else{
				inputInfo("Token is not finished!");
				return 0;
			}
		}
	}else
	if(inputChar=='"'||inputChar=='\''){ 
		// could start a string literal
		if(string_length(currentToken->text)==1){ // actually started the string literal (is NOT escaped)
			if(NULL==string_append_char(_expectedCharacterStack,inputChar)){
				inputError("Failed to register '%c' as expected character",inputChar);
				return -1;
			}
		}
	}else
	if(inputChar=='('){
		// if this starts a function call argument list we're going to need to append comma's for each
		// argument and a final closing parenthesis, but always the closing parenthesis is expected
		if(NULL==string_append_char(_expectedCharacterStack,')')){
			inputError("Failed to register ')' as expected character.");
			return -2;
		}
		// MDH@03JUL2024: appending the expected commas as well is a nice feature BUT
		//                BUT it is seriously interfering with updating the feed forward because
		//                when a user does NOT enter these arguments but closes the function call with )
		//                beforehand these expected characters will still remain on the expected character stack
		//                and we really do not want that, 
		//                now instead of keeping the commas in, for now, we just do NOT add them 
		//                it's not that hard to enter a , so as feed forward not that immportant!!!!
		/*
		Mtoken* prevToken=currentToken->prev;
		if(currentToken->type==TT_FUNCTION_CALL){
			char* _functionName=_getSignificantTokenCharacters(prevToken);
			long long numberOfFunctionParameters=getNumberOfFunctionParameters(functionName);
			free(_functionName);
			if(numberOfFunctionParameters>=0){
				///////inputInfo("Number of arguments in function '%s': %lld",function,numberOfFunctionParameters);
				while(--numberOfFunctionParameters>0)if(string_append_char(_expectedCharacterStack,',')==NULL){
					inputError("Failed to register ',' as expected character.");
					return -2;
				}
			}else{
				inputError("Unknown function '%s'.",_functionName);
				return -2;
			}
		}
		////else{
		///	if(prevToken!=NULL)
		///		inputInfo("Previous token of type %s.",TOKENTYPE_STRING[prevToken->type]);
		///	else
		///		inputInfo("Not a function call");
		///	return 1;
		///}
		*/
	}else
	if(inputChar=='['){
		if(string_append_char(_expectedCharacterStack,']')==NULL){
			inputError("Failed to register ']' as expected character.");
			return -2;
		}
	}else
	if(inputChar=='{'){
		if(NULL==string_append_char(_expectedCharacterStack,'}')){
			inputError("Failed to register '}' as expected character.");
			return -2;
		}
	}else
	if(inputChar==','){
		if(string_last_char(_expectedCharacterStack)==','){
			if(NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove expected character ','.");
				return -3;
			}
		}
	}else
	if(inputChar==')'){
		// MDH@01JUL2024: TODO the problem here is that we could be closing an call before having entered all arguments in which case we've got a couple of commas in the expected character stack
		//                     in front of the closing parenthesis, that we should get rid of first
		while(string_last_char(_expectedCharacterStack==',')){
			if(NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove expected character ','.");
				return -3;
			}
		}
		if(string_last_char(_expectedCharacterStack)==')'){
			if(NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove expected character ')'.");
				return -3;
			}
		}
	}else
	if(inputChar==']'){
		if(string_last_char(_expectedCharacterStack)==']'){
			if(NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove expected character ']'.");
				return -3;
			}
		}
	}else
	if(inputChar=='}'){
		if(string_last_char(_expectedCharacterStack)=='}'){
			if(NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove expected character '}'.");
				return -3;
			}
		}
	}
	if(amVerbose())
		inputInfo("'%c+%c': expected characters: '%s' (%zu).",lastExpectedCharacter,inputChar,string(_expectedCharacterStack),string_length(_expectedCharacterStack));
	return 1;
}

// MDH@14DEC2023: 
int8_t expectedCharacterStackUpdatedOnRemoval(char removedChar,Mtoken const * const token){
	if(!removedChar)return 0;
	if(NULL==token)return 0;
	// let's take care of the quote characters first
	if(removedChar=='"'){
		// 3 possibilities: removing the starting quote, the end quote and an escaped quote
		if(token->type==TT_DQSTRING&&string_length(token->text)>0){ // end or escape double quote removed
			// if not behind the \ escape character, we're removed the finishing double quote, and are moving
			// back into the string literal which means we have to expect the double quote again
			if(string_last_char(token->text)!='\\'&&NULL==string_append_char(_expectedCharacterStack,'"')){
				inputError("Failed to register '\"' as expected character.");
				return -1;
			}
		}else // starting quote removed, which means there must be a corresponding expected end quote to remove!!
		if(string_last_char(_expectedCharacterStack)!='"'||NULL==string_declength(_expectedCharacterStack)){
			inputError("Failed to remove the expected double quote.");
			return -1;
		}
	}else
	if(removedChar=='\''){
		// 3 possibilities: removing the starting quote, the end quote and an escaped quote
		if(token->type==TT_SQSTRING&&string_length(token->text)>0){ // end or escape single quote removed
			// if not behind the \ escape character, we're removed the finishing single quote, and are moving
			// back into the string literal which means we have to expect the single quote again
			if(string_last_char(token->text)!='\\'&&NULL==string_append_char(_expectedCharacterStack,'\'')){
				inputError("Failed to register '\'' as expected character.");
				return -1;
			}
		}else // starting quote removed, which means there must be a corresponding expected end quote to remove!!
		if(string_last_char(_expectedCharacterStack)!='\''||NULL==string_declength(_expectedCharacterStack)){
			inputError("Failed to remove the expected single quote.");
			return -1;
		}
	}else
	// a removed character could move it back onto the expected character stack
	// but let's deal with being in a string literal first, because if we are in a string literal
	// no character we delete actually should be placed on the expected character stack
	if(token->type!=TT_SQSTRING&&token->type!=TT_DQSTRING){
		if(removedChar==','||removedChar==')'||removedChar==']'||removedChar=='}'){
			if(NULL==string_append_char(_expectedCharacterStack,removedChar)){
				inputError("Failed to append '%c' to the expected character stack.",removedChar);
				return -1;
			}
		}else
		if(removedChar=='('){
			// if the start of something that is closed is removed, so should the associated expected character
			char lastExpectedCharacter=string_last_char(_expectedCharacterStack);
			if(lastExpectedCharacter!=')'||NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove ')' from the expected character stack.");
				return -1;
			}
		}else
		if(removedChar=='['){
			char lastExpectedCharacter=string_last_char(_expectedCharacterStack);
			if(lastExpectedCharacter!=']'||NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove ']' from the expected character stack.");
				return -1;
			}
		}else
		if(removedChar=='{'){
			char lastExpectedCharacter=string_last_char(_expectedCharacterStack);
			if(lastExpectedCharacter!='}'||NULL==string_declength(_expectedCharacterStack)){
				inputError("Failed to remove '}' from the expected characters.");
				return -1;
			}
		}
	}/*else{ // the token is a string literal
		// if the string literal is currently finished we haven't removed the final quote
		if(isTokenUnfinished(token)){
			if(string_length(token->text)){ // there are still characters in the string literal
				if(string_last_char(token->text)!='\\'){ // the removed character is not escaped
					if(token->type==TT_SQSTRING){
						// only when the closing quote was removed, should it be placed on the stack
						if(removedChar=='\''&&NULL==string_append_char(_expectedCharacterStack,removedChar)){
							inputError("Failed to register removed character '\'' on the expected character stack.");
							return -1;
						}
					}else{ // a double quoted string
						if(removedChar=='"'&&NULL==string_append_char(_expectedCharacterStack,removedChar)){
							inputError("Failed to register removed character '\"' on the expected character stack.");
							return -1;
						}
					}
				}
			}
		}
	}*/
	/////inputInfo("Expected character stack after removing '%c': '%s' (%zu).",removedChar,string(_expectedCharacterStack),string_length(_expectedCharacterStack));
	return 1;
}

// MDH@20SEP2019: whenever a token changes the associated feed forward text might change, so it makes sense to update the feed forward text accordingly
//				assuming that the given type is correct (e.g. an identifier of which has been determined whether it is a function or variable name)
// MDH@26SEP2019: due to the separation of the identifier continuation text and the other feed forward text we separate getting the identifier continution text from getting the other feed forward text
// MDH@21OCT2020: sometimes when the suggested text already contains the last token auto completion text we do not want to add it again
/**
 * @brief updates the last token auto completion text
 * 
 * @param onlyWhenNotAlreadyEndingTheSuggestedText 
 */
void updateLastTokenAutoCompletionText(bool onlyWhenNotAlreadyEndingTheSuggestedText){
	char* _lastTokenAutoCompletionText=_getLastTokenAutoCompletionText();
	if(NULL==_lastTokenAutoCompletionText)return;
	// the following is critical in that we empty _lastTokenAutoCompletionText when the suggested text ends with the auto completion text BUT we're still setting the last token auto completion text!!!
	if(onlyWhenNotAlreadyEndingTheSuggestedText)
		if(string_endswith(_suggestedText,_lastTokenAutoCompletionText))
			_lastTokenAutoCompletionText[0]='\0';
	// MDH@27SEP2019: updating the identifier continuation now moved to writeSuggestedText(true), so JIT update
	// MDH@25MAY2020: TODO _getLastTokenAutoCompletionText() returns a dynamically allocated char* (using strdup) which therefore is NOT under version control
	free(setLastTokenAutoCompletionText(_lastTokenAutoCompletionText));
	// inputInfo("Last token auto completion text updated.");
	updateTheFirstCommandClosingCharacter();
}
/**
 * @brief sets the current user input command to \p command and outputs it
 * @details update numberOfLineCommandCharacters to the result of calling outputCommand()
 * @param command 
 */
void setUserInputCommand(Mcommand* command){
	// ASSERT assuming we are currently right behind the prompt!!!
	// NOTE command is either an existing command (commandIndex>0) or a new command (commandIndex=0)
	//	  so if it is a registered command we should NOT obtain ownership
	_userInputCommand=command; // MDH@24MAY2020: take over ownership!!!!
	/// string_setlength(_expectedCharacterStack,0); // MDH@13DEC2023 TODO: is there a better place to do this??????
	// replacing: _userInputCommand->_lastToken=_userInputCommand->_firstToken=pCommand;
	numberOfLineCommandCharacters=outputCommand(_userInputCommand); // MDH@24SEP2020: essential to 'sync' numberOfLineCommandCharacters to the number of command characters on the last command line
	// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=(_userInputCommand?inIdentifierToken(_userInputCommand->_lastToken):false); // MDH@02OCT2019 because we're setting _userInputCommand->_lastToken but not calling setLastUserInputCommandToken()
	//////////writeSuggestedText(true);
	// MDH@06AUG2019 TODO: determine the initializations associated with a stored command!!!
	////////// removing: determineCommandInitializations();
}
/**
 * @brief deletes the current user input command
 * @details frees user input command when it is a new command (not one from history)
 */
void deleteUserInputCommand(){
	numberOfLineCommandCharacters=0; // TODO is this the right place to do this?????? the principle should be that whenever _userInputCommand changes, we should compute numberOfLineCommandCharacters a new
	if(NULL==_userInputCommand)return;
	// if _userInputCommand is a registered command, it should NOT be freed
	if(commandIndex==0){
		///////////////output("Freeing user input command!\n");
		FREE_COMMAND(_userInputCommand,owner_userInputCommand);
	}
	_userInputCommand=NULL;
}

/**
 * @brief sets the current command index to \p createUserInputCommandIndex
 * @details accepts \p createUserInputCommandIndex between 0 and commandCount at most
 *          but 0 is now also accepted, returning to show _userInputCommand->_firstToken (if any)
 * @param createUserInputCommandIndex 
 */
void setCommandIndex(uint32_t createUserInputCommandIndex){Mallocationowner owner=getOwner(__LINE__);
	// MDH@25MAY2020: we should 'delete' the user input command BEFORE updating commandIndex because the current command index determines whether we're dealing with a new command or an existing command
	//				we used to do that after clearing the screen below
	clearInfo();
	deleteUserInputCommand();
	commandIndex=createUserInputCommandIndex;
	////if(amVerbose())inputInfo("Command index %lld.",commandIndex);
	// it's easier to go to the beginning of the line although we could be on the line below!!!!
	// replacing: 
	backToPrompt();
	clearScreenFromCursor();
	// MDH@29OCT2019: we have to do the following because otherwise inputInfo() will jump back to the end of the command instead of right behind the prompt!!!!
	//				NOTE typically _userInputCommand will not be NULL when we're scrolling through the list of previous commands!!!!
	// MDH@24APR2019 obsolete: getCommandLength()=getUserInputLength()=0; // do we need this????
	// TODO do we need to do this: clear the behind cursor text (in any situation)
	invalidateAutoCompletionText(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	if(commandIndex>0){
		Menvironment* environment=getExecutionEnvironment();
		if(NULL==environment){outputBug("Environment vanished setting the command index");return;}
		// MDH@29OCT2019 should already have _userInputCommand equal to NULL: _userInputCommand->_lastToken=NULL; 
		// replacing: _userInputCommand->_lastToken=NULL; // MDH@03SEP2019: I have to do this otherwise inputInfo() won't work the way we want it to
		Mcommand* command=_registeredcommands[commandCount-commandIndex]._command; // MDH@18JUN2020: 
		// MDH@19APR2019: if not to accept the history command we use the previous command as behind cursor text
		// TODO this construction (with a return in the middle is a bit unclear)
		if(amAcceptinghistorycommand()){ // use the history command as autocompletion text instead of accepting it immediately as command!!!
			if(amVerboseDebugging())inputInfo("Showing registered command #%lld.",commandCount-commandIndex+1);
			setUserInputCommand(command);
		}else{
			if(amVerboseDebugging())inputInfo("Showing command #%lld as suggested text.",commandCount-commandIndex+1);
			// the previous command will be used as behind cursor text, and not immediately as command
			// MDH@20SEP2019
			// this is a bit of a nuisance as there will be no associated tokens for the entire behind cursor text
			// theoretically this means we could store the entire behind cursor text in a single feed forward text
			// we can abuse feedforwardText, by using it to compose it, and afterwards use it to create a single feed forward text instance
			Mstring* _commandFeedforward=owned_string(__string(),owner); // free asap
			if(_commandFeedforward){
				Mtoken* token=command->_firstToken;
				while(token){
					///////outputText("(%s)",tokenText);
					if(!string_append(_commandFeedforward,string(token->text)))break;
					token=token->next;
				}
				// because the last token is NULL, we can use setLastTokenAutoCompletionText to register _commandFeedforward used as the entire feed forward text
				setLastTokenAutoCompletionText(string(_commandFeedforward));
				FREE_STRING(_commandFeedforward,owner); // get rid of the feed forward text we constructed
			}
		}
	}
	/* MDH@29OCT2019 removed to the start:
	setUserInputCommand(NULL); // TODO probably shouldn't have to do this??????
	clearInfo(); // TODO do we need this when showing a previous command as behind cursor text??????
	*/
	/////////////printf("(%d)",getCommandLength());
}
/**
 * @brief selects (and shows) the next remembered command
 * 
 * @return true on success
 * @return false on failure
 */
bool commandDown(){
	if(commandCount==0)return false;
	/*Menvironment* environment=getExecutionEnvironment();
	if(NULL==environment){outputBug("Environment vanished.");return false;}*/
	setCommandIndex(commandIndex<commandCount?commandIndex+1:0);
	return true;
}
/**
 * @brief selects (and shows) the previous remembered command
 * 
 * @return true on success
 * @return false on failure
 */
bool commandUp(){
	// MDH@26FEB2019: instead of stopping at the start of the commands it's better to move back to the new command which is at commandCount
	if(commandCount==0)return false;
	/*Menvironment* environment=getExecutionEnvironment();
	if(NULL==environment){outputBug("Environment vanished.");return false;}*/
	setCommandIndex(commandIndex>0?commandIndex-1:commandCount);
	return true;
}
/*
// MDH@23SEP2019: whenever the type of the current token (_userInputCommand->_lastToken) changes (possibly with the start of a new token), so will the feed forward text associated with that token
//				therefore it is best to set the last token type using a separate function
// MDH@03OCT2019: every time the token type changes we need to sync the immediate feed forward text as well!!!!
void setTokenType(Mtoken* token,TokenType tokenType){
	if(token){
		if(tokenType!=token->type){
			// MDH@04OCT2019 moved to input loop removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the current token immediate feed forward text.");
			token->type=tokenType;
			// MDH@04OCT2019 moved to input loop removing: if(!updateUserInputCommandImmediateFeedforwardText())inputError("Failed to add the current token immediate feed forward text.");
		}
	}
	///////// MDH@29OCT2019 probably don't need this here anymore: if(endOfInput)updateLastTokenAutoCompletionText();
}
*/
/*
// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType){
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//				but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=_getToken(lastCommandToken,tokenType);
	if(_newCommandToken)setTokenType(_newCommandToken,tokenType);else inputError("Failed to create a command token");
	return _newCommandToken;
}
*/

/**
 * @brief creates the user input command
 * @return true on success
 * @return false on failure
 */
bool createUserInputCommand(){
	logToOutputFile("Creating the user input command.\n");
	// MDH@24APR2019 obsolete: getCommandLength()=string_length(feedforwardText); // MDH@21APR2019: oops was 0 before...
	resetOutputColor(); // TODO do we need this here?????
	// if(amVerboseDebugging())inputInfo("Creating the new user input command.");
	// MDH@23SEP2019: createUserInputCommandToken() added to take care of updating _userInputCommand->_lastToken (should be NULL as it is used to represent the previous last token)
	///////output("Creating user input command!\n");
	Mblock* currentBlock=getCurrentBlock();
	Mtoken* offsetToken=(currentBlock!=NULL?currentBlock->insertToken:NULL);
	_userInputCommand=owned_command(_getNewCommand(true,offsetToken),owner_userInputCommand);
	// MDH@29OCT2019: the following is absolutely silly although how about updating 
	if(_userInputCommand!=NULL){
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=false; // MDH@29OCT2019: instead of calling setLastUserInputCommandToken()
		updateLastTokenAutoCompletionText(false); // TODO perhaps we do not need this after all here????? NOTE used to do that in setTokenType() when endInput was true but not doing that anymore
		Mtoken* firstCommandToken=_userInputCommand->_firstToken;
		// MDH@09APR2024: if the current environment has an insert token we have to copy most of the insert token fields
		//                to the created first token in the new command, this is VERY essential because it there's an 
		//                insert token, the entered command has to know what it will be continuing otherwise the fields
		//                of the command will not be initialized correctly
		if(firstCommandToken!=NULL&&offsetToken!=NULL){
			// if(amVerboseDebugging())inputInfo("New user input command created.");
			////firstCommandToken->expr=environmentInsertToken->expr;
			firstCommandToken->prevIdentifier=offsetToken->prevIdentifier;
			///firstCommandToken->argument=environmentInsertToken->argument;
			///firstCommandToken->envid=environmentInsertToken->envid;
			// NOT: type, significantCharacterCount,text,offset,position,prev and next
		}
	}else
		inputError("Failed to create a new user input command.");
	return(_userInputCommand!=NULL);
	/* replacing: 
	_userInputCommand->_firstToken=_getNewCommandToken(NULL,TT_EXPRESSION,true,true);
	setLastUserInputCommandToken(_userInputCommand->_firstToken); // so updating identifierContinuationIsDirty is guaranteed!!!
	if(!_userInputCommand->_firstToken)outputError("Failed to create a new command");else _userInputCommand->_firstToken->expr=NULL;
	*/
	/* replacing:
	_userInputCommand->_lastToken=_userInputCommand->_firstToken=_getToken(NULL,TT_EXPRESSION);
	if(!_userInputCommand->_firstToken){outputError("Failed to create a new command");return;}
	setLastTokenType(TT_EXPRESSION,true); // MDH@23SEP2019: endOfInput set to true, although we know there will be no feed forward on an expression
	////////// removing: removeInitializations(); // MDH@06AUG2019: ready for new initializations at the start of a new command
	// MDH@27MAY2019: NO let's just keep expr NULL!!!
	_userInputCommand->_firstToken->expr=NULL; // TODO do I need this???? YES, because we used _getToken()! PERHAPS NOT as prevToken is NULL???????
	*/
}

// TODO copyUserInputCommand() should set ->expr correctly
// MDH@29OCT2019: TODO caller should check whether or not _userInputCommand is NULL if it is copying failed!!!!
// MDH@24SEP2020: with _userInputCommand being a remembered command, any editing needs to occur on an exact copy which includes copying the tokens exactly
/**
 * @brief copies the user input command
 * 
 * @return true on success
 * @return false on failure
 */
bool copyUserInputCommand(){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT _userInputCommand must NOT be NULL and we're assuming that _userInputCommand now points to one of the remembered commands (that needs to be duplicated in order to allow editing it)
	//		it's probably best to first create a new command, copy the tokens over from _userInputCommand and set the user input command to that new command
	Mcommand* _newUserInputCommand=owned_command(_getNewCommand(false,NULL),owner); // get a new command without tokens (should NEVER fail unless memory shortage)
	if(NULL==_newUserInputCommand)
	{outputError("Failed to duplicate the current user input command");return false;}
	
	// if fails to copy _userInputCommand->_firstToken _userInputCommand->_lastToken should end up as NULL
	if(amVerboseDebugging())inputInfo("Preparing the user input command for editing.");
	////////////else output("Copying the user input command!\n");

	///// NOT NEEDED using the false flag in _getNewCommand()!!!! _newUserInputCommand->_lastToken=NULL;_newUserInputCommand->_firstToken=NULL;
	Mtoken* _tokenToCopy=_userInputCommand->_firstToken;
	// the essence is that _userInputCommand->_lastToken points to the last token in _userInputCommand->_firstToken
	// NOTE theoretically _userInputCommand->_lastToken could be NULL due to _getToken() failing to create a new token
	while(_tokenToCopy){
		// MDH@28OCT2019: because we adapted _getNewCommandToken to receive the last command token as argument, and returning the new command token, we need to assign the result to _userInputCommand->_lastToken!!!
		_newUserInputCommand->_lastToken=owned_token(_getNewCommandToken(_newUserInputCommand->_lastToken,_tokenToCopy->type,false),Msubowner(owner,1));
		if(NULL==_newUserInputCommand->_lastToken)break;
		// replacing: if(!setLastUserInputCommandToken(_getNewCommandToken(_userInputCommand->_lastToken,_tokenToCopy->type)))break; // MDH@23SEP2019: TODO should we do something to _userInputCommand->_firstToken when this happens? or show some error???
		/* MDH@23SEP2019 replacing:
		_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken,_tokenToCopy->type);
		///////// MDH@23SEP2019: moved out of _getToken() using false for endOfInput to prevent adding/changing the associated feed forward text
		setLastTokenType(_tokenToCopy->type,false);
		*/
		/* TODO check whether the following is correct!!! guess not!!
		if(_userInputCommand->_lastToken->type==TT_END_OF_FUNCTION_CALL||_userInputCommand->_lastToken->type==TT_END_OF_LIST||_userInputCommand->_lastToken->type==TT_END_OF_MAP){
			if(_tokenToCopy->expr)
				_userInputCommand->_lastToken->expr=_userInputCommand->_lastToken->expr->expr;
			else
				outputInfo("BUG: End of argument list or map encountered, but not started.");
		}
		*/

		// MDH@29OCT2019: if we want to do it right we should check who's referencing back to _tokenToCopy
		Mtoken *referencedToken=_tokenToCopy->expr;
		if(_tokenToCopy->expr!=NULL){ // some token referenced
			Mtoken *referencedToken=_tokenToCopy,*newReferencedToken=_newUserInputCommand->_lastToken;
			// move back until we find the token referenced (and we should find it)
			while(referencedToken!=_tokenToCopy->expr)
			{referencedToken=referencedToken->prev;newReferencedToken=newReferencedToken->prev;}
			// ASSERT referencedToken now equals the token in the original command being referenced (which could be itself obviously), and newReferencedToken is a token in the new user input command that should be pointed to!!!
			if(newReferencedToken)
				_newUserInputCommand->_lastToken->expr=newReferencedToken;
			else 
				inputError("%s%sFailed to synchronize a token reference.",M_BUG_PREFIX,M_MESSAGE_PREFIX);
		}else // nothing pointed to, so just in case
			_newUserInputCommand->_lastToken->expr=NULL;
		// replacing: _newUserInputCommand->_lastToken->expr=_tokenToCopy->expr; // MDH@20MAY2019: just copy the expr over!!!!
		
		setTokenSignificantCharacterCount(_newUserInputCommand->_lastToken,getTokenSignificantCharacterCount(_tokenToCopy));
		// if failing to copy the text over get rid of the command constructed so far, and break
		_newUserInputCommand->_lastToken->text=owned_string(_stringCopy(_tokenToCopy->text,0),Msubowner(owner,2)); // copies the entire Mstring over
		if(NULL==_newUserInputCommand->_lastToken->text){
			_newUserInputCommand->_lastToken=NULL;
			break;
		} // TODO perhaps we'd have to do a little more than just this?????
		// MDH@24APR2019 obsolete: getCommandLength()+=string_length(_userInputCommand->_lastToken->text);
		// MDH@24SEP2020 DONE some additional fields to copy over (NOT the offset is that is set automatically)
		_newUserInputCommand->_lastToken->argument=_tokenToCopy->argument;
		_newUserInputCommand->_lastToken->offset=_tokenToCopy->offset;
		_newUserInputCommand->_lastToken->position=_tokenToCopy->position;
		_newUserInputCommand->_lastToken->envid=_tokenToCopy->envid;
		_newUserInputCommand->_lastToken->prevIdentifier=_tokenToCopy->prevIdentifier;
		// (M_MODULE_DEBUGGING&MM_MAIN): output("[%zd]",_newUserInputCommand->_lastToken->position);
#ifdef __DEBUG__
		printf("%d:%s",_userInputCommand->_lastToken->type,string(_userInputCommand->_lastToken->text));
#endif
		if(NULL==_newUserInputCommand->_firstToken)
			_newUserInputCommand->_firstToken=_newUserInputCommand->_lastToken; // TODO=DONE will never happen???? it does here
		// get the next token to copy...
		_tokenToCopy=_tokenToCopy->next;
	}
	if(NULL==_newUserInputCommand->_lastToken){
		FREE_COMMAND(_newUserInputCommand,owner);
		return NULL;
	}

	// OOPS do NOT call setUserInputCommand() here as it will write the command once more so it might suffice to assign
	////////output("Owning the new user input command!\n");
	_userInputCommand=owned_command(disowned_command(_newUserInputCommand,owner),owner_userInputCommand); // replacing: setUserInputCommand(_newUserInputCommand); // testing whether successful: inputInfoCommand(_userInputCommand);
	return true;
}

// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual createUserInputCommand
/**
 * setCommand() creates a new (empty) command (in _userInputCommand->_firstToken) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *			  which is supposedly showing behind the cursor!!!
 * ASSUMPTION should only be called when at the prompt (getUserInputLength()=0) ready for starting or changing a command
 * setCommand() won't show the command anymore as we assume that any registered command passed in is already showing!!!
 */
/*
Mtoken* getCommand(){
	return(commandIndex&&getUserInputLength()?commands[commandCount-commandIndex]:_userInputCommand->_firstToken);
}
void echoCommand(){
	Mtoken* token=_userInputCommand->_firstToken;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
void setCommand(Mtoken* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. getUserInputLength()==0 and _userInputCommand->_firstToken==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(_userInputCommand->_firstToken){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	createUserInputCommandToEvaluate(); // NOTE might fail, in which case _userInputCommand->_lastToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set _userInputCommand->_lastToken!!!
		Mtoken* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically _userInputCommand->_lastToken could be NULL due to _getToken() failing to create a new token
		while(_userInputCommand->_lastToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!_stringCopy(pNewToken->text,_userInputCommand->_lastToken->text)){clearCommand();break;}
			// MDH@24APR2019 obsolete: getCommandLength()+=string_length(_userInputCommand->_lastToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
			_userInputCommand->_lastToken->type=pNewToken->type;
#ifdef __DEBUG__
			printf("%d:%s",_userInputCommand->_lastToken->type,string(_userInputCommand->_lastToken->text));
#endif
			pNewToken=pNewToken->next;
			if(!pNewToken)break;
			// we're going to need another token!!!
			_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken);
		}
		// if the user decides to start typing ascertain to show it in the right color!!
		if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken);
#ifdef __DEBUG__
		echoCommand();
#endif
	}
}
*/
/* MDH@11AUG2019: no longer required now that all environments get a unique id in tokenization
// every non-function call token can be inside a call to a special function that can have local variables
Mtoken* getSpecialFunctionCallToken(const Mtoken* const token){
	Mtoken* container=(token?token->expr:NULL);
	// NOTE if the type is not a function call (like a list or map) or its argument is 0 or below it is not a special function
	while(container&&(container->type!=TT_FUNCTION_CALL||container->argument<=0))container=container->expr;
	return container;
}
*/

// MDH@30APR2019: if the current token is a variable/function check whether it still is
//				call whenever the current token changes (in removePreviousTokenCharacter() and commandCharacterAccepted())
// MDH@01OCT2019: aSuggestedCharacter is actually not used anymore, so no need to pass it in anymore
/**
 * @brief check whether \p lastCommandToken is a function
 * @param lastCommandToken
 * @param endOfInput
 */
bool tokenCheckedForBeingAFunction(Mtoken* lastCommandToken,bool endOfInput/*,bool aSuggestedCharacter*/){
	// only identifiers should be checked...
	// what about properties????? properties should NEVER be considered functions
	if(NULL==lastCommandToken)return false; // MDH@03APR2023: best to check
	if(lastCommandToken->type!=TT_VARIABLE&&
			lastCommandToken->type!=TT_NEW_VARIABLE&&
			lastCommandToken->type!=TT_FUNCTION)
		return false;
	// non-existing variables should be assigned to so it's a good idea to put the assignment operator behind it, although it might be hard to remove it though
	char* _identifierName=_getSignificantTokenCharacters(lastCommandToken); // replacing: (lastCommandToken->text,getTokenSignificantCharacterCount(lastCommandToken)); // free asap
	logToOutputFile("Checking '%s'.\n",_identifierName);
	if(lastCommandToken->type!=TT_FUNCTION){ // is it a function (now)?
		if(getFunction(getExecutionEnvironment(),_identifierName)!=NULL){ // yes, it is
			// if a new variable before (now a function), remove the (assignment) character in the behind cursor text
			// MDH@20SEP2019: I suppose we need to ascertain that an opening parenthesis is associated with the token now, and no longer anything else
			/* MDH@20SEP2019: removing:
			if(amMatchingparentheses())if(_userInputCommand->_lastToken->type==TT_NEW_VARIABLE)if(string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			*/
			// the minimum we can do is put an opening parenthesis in the behind cursor text
			setTokenType(lastCommandToken,TT_FUNCTION/*,endOfInput*/);
			if(endOfInput)updateLastTokenAutoCompletionText(false);
			reoutputToken(lastCommandToken);
			// MDH@20OCT2021 this is when an identifier is identified as a function!!!!
			// DEBUG: outputChar('V');
			// insert an opening parenthesis for the function call
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: if(endOfInput&&amMatchingparentheses())setLastTokenAutoCompletionText("(");else invalidateAutoCompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: either force the feedforward text to match an opening parenthesis or nothing TODO does endOfInput matter?????
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='(')string_insert_char(feedforwardText,0,'(');
			*/
		}
	}else{ // is it (still) a function?
		if(!getFunction(getExecutionEnvironment(),_identifierName)){ // no, it ain't
			// the minimum we can do is remove the opening parenthesis behind it (if it is still there!!!!!)
			// MDH@11MAR2020: ok, here we have an issue: 
			setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);
			if(endOfInput)updateLastTokenAutoCompletionText(false);
			reoutputToken(lastCommandToken);
			// outputChar('U');
			inputInfo("'%s' considered to be an existing variable.",_identifierName);
			//////////outputInfo("Variable redrawn!");
			// remove any opening parenthesis from the behind cursor text
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: invalidateAutoCompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: I suppose when TT_VARIABLE changes to TT_NEW_VARIABLE later on, an equal sign might be added!!!
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(getTotalNumberOfSuggestedCharacters())if(string_char(feedforwardText,0)=='(')string_removed_char(feedforwardText,0);
			*/
		}
	}
	logToOutputFile("Continuing to determine if '%s' is an existing variable!\n",_identifierName);
	// check whether the variable exists or not
	// MDH@07AUG2019: this variable could exist in this command, which we should check
	if(lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_NEW_VARIABLE){ // might not exist after all both in the command and in the current environment
		// MDH@08AUG2019 WARNING: all variables assigned to in the local variable declaration argument of the special functions should ALWAYS be considered new, but of course we cannot see that until they are assigned to
		//						unless we do not require them to be assigned to (and we can just use them by name itself without assigning a value to them) in which case they are local but uninitialized...
		int8_t variableExistsIndicator=0; // assuming invalid
		if(lastCommandToken->argument!=1){
			// MDH@25FEB2021: A HA I was looking for this!!!
			if(!existsAsLocalVariable(_identifierName,lastCommandToken->envid)&&
					!existsInCommand(_userInputCommand,_identifierName,lastCommandToken->envid)){
				// MDH@12MAR2020: if containsVariable() returns -2 this only happens with a property reference that is invalid in which case the token should be considered an error
				//				I suppose we should then change the token type to TT_ERROR in which case the type won't change from NEW_VARIABLE to VARIABLE or vice versa
				variableExistsIndicator=containsVariable(NULL,_identifierName,-1);
				logToOutputFile("Contains variable indicator: %d.\n",variableExistsIndicator);
				switch(variableExistsIndicator){
					case -5: // value of type MAP but no map defined (TODO is that a bug?????)
					case -3: // value does not exist
					case -2: // variable hosting the property does not exist
						break;
					case 0: // illegal input
					case -4: // there is a value but it is NOT a map, I suppose this would be the only ERRONEOUS situation
						{
							char* propertyName=strrchr(_identifierName,M_PROPERTY_SEPARATOR_CHARACTER);
							if(propertyName!=NULL){
								setTokenType(lastCommandToken,TT_ERROR);
								reoutputToken(lastCommandToken);
								// DEBUG outputChar('Z');
								inputInfo("Invalid property '%s'.",propertyName);
							}else
								inputInfo("Missing environment or variable name (%s)!",_identifierName);
						}
						break;
					case -1:
						inputInfo("'%s' does not exist!",_identifierName);
						break;
					case 1:
						inputInfo("'%s' exists, its value is a function.",_identifierName);
						break;
					case 2:
						inputInfo("'%s' exists.",_identifierName);
						break;
				}
			}else{
				variableExistsIndicator=3; // assumed to exist, but whether a function or not cannot be determined
				inputInfo("'%s' is initialized in the command.",_identifierName);
			}
		}else{
			variableExistsIndicator=-3; // assumed to NOT exist but whether a function or not cannot be determined
			inputInfo("'%s' is local, so it cannot be a existing variable.",_identifierName);
		}
		if(lastCommandToken->type==TT_VARIABLE){ // might not exist after all both in the command and in the current environment
			if(variableExistsIndicator<0){ // apparently does NOT exist
				// inputInfo("'%s' not an existing variable.",_identifierName);
				setTokenType(lastCommandToken,TT_NEW_VARIABLE/*,endOfInput*/);
				if(endOfInput)updateLastTokenAutoCompletionText(false);
				reoutputToken(lastCommandToken);
				// DEBUG outputChar('Y');
				// suggested characters should make = show (probably already present in the behind cursor text)
				// MDH@23SEP2019 take care of by setLastTokenType, so removed: setLastTokenAutoCompletionText("="); // MDH@20SEP2019 replacing: if(endOfInput&&!aSuggestedCharacter)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='=')string_insert_char(feedforwardText,0,'=');
			}
		}else
		if(lastCommandToken->type==TT_NEW_VARIABLE){ // a new variable
			logToOutputFile("'%s' is still a new variable!\n",_identifierName);
			if(variableExistsIndicator>0){ // now an existing variable
				setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);
				if(endOfInput)updateLastTokenAutoCompletionText(false);
				logToOutputFile("Re-outputting token!\n");
				reoutputToken(lastCommandToken);
				// DEBUG outputChar('X');
				///// MDH@23SEP2019 removed: invalidateAutoCompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019 replacing: if(endOfInput)if(amMatchingparentheses())if(string_length(feedforwardText)&&string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			}
		}
	}
	free(_identifierName); // freed
	return true;
}

// MDH@14AUG2019: cancelCommand() takes care of removing everything in the current command
/**
 * @brief cancels the current user input command
 * 
 */
void cancelCommand(){ // in response to Ctrl-C or backspace on the first character
	if(amVerboseDebugging())
		inputInfo("Cancelling the command.");
	// TODO perhaps every cancelCommand() needs this: 
	deleteUserInputCommand(); // MDH@20OCT2021: BUG FIX setCommandIndex(0) worked fine using this, but cancelCommand() needs it as well
	// MDH@31OCT2019: with a command now possibly covering multiple lines we have to do a little more than we did before but we can put that in backToPrompt()
	backToPrompt();
	clearScreenFromCursor(); // inserting doing this otherwise (in the case of backspace) we would apparently still see the behind cursor text
	clearCommand();
	// by removing the behind cursor text, we ascertain that when the Enter key is pressed, we will switch to control mode, as otherwise we wouldn't, on the other hand, if _userInputCommand->_firstToken is NULL we should always switch to control mode (even if)
	if(_manualFeedforwardText!=NULL)deleteManualFeedforwardText();
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	// it's a good idea to inform the user that the command was cleared
	if(amVerboseDebugging())
		inputInfo("Command cleared!");
}
/**
 * @brief updates the screen on removing last token character \p removedCharacter
 * 
 * @param removedCharacter 
 */
void updateOnTokenCharacterRemoved(char removedCharacter){
	// adapt screen
	clearScreenFromCursor(); // will clear what's behind the cursor
	// MDH@14AUG2019: shouldn't we ALWAYS write the behind cursor text, because I think we should, and if we want do not want to see it we should delete it beforehand!!!!! which is much preferred over not showing it when it is still there!!!!!
	// MDH@24APR2019 obsolete: getCommandLength()--; // decrement the total command length
	if(getUserInputLength()){ // still something left of the command (that we might check for being a function or not)
		// on screen as well please
		// before writing the behind cursor text we're going to check whether the current token still is a function or variable
		tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,true/*,false*/); // MDH@14AUG2019: no, not a suggested character (as called on the backspace user action)
		// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in feedforwardText that's what we should now write
		//// MDH@14AUG2019 moving to execute always: writeSuggestedText(true);
		// replacing: if(_userInputCommand->_firstToken)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
	}/* removedTokenCharacter() calls removeToken which will NULL the _userInputCommand->_firstToken and _userInputCommand->_lastToken when the first command character is removed, in which case we do not need:
		else clearCommand();*/
	
	/* MDH@20JAN2024 removing: doing the following was moved into removedTokenCharacter and therefore is no longer needed here
	// MDH@14DEC2023: TODO implement how to respond
	switch(expectedCharacterStackUpdatedOnRemoval(removedCharacter,_userInputCommand->_lastToken)){
		case -1:break; // something went wrong!!!
		case 0: break; // invalid input
		case 1: break; // success
	}
	*/
	
	// MDH@20SEP2019: the following is about removing the feed forward characters that were added when a certain token started but as you can see 
	//				it is all about feed forward associated with the start of a token, so removing the associated feed forward can also be done at the moment the token is actually removed
	//				so for now we remove the following block and simply write the behind cursor text
	////////////writeSuggestedText(true);
	/* replacing:
	// MDH@14AUG2019: how about removing any matching character?????
	if(string_length(getFeedforwardCharacters())){ // MDH@20SEP2019: will reconstruct feedforwardText if need be
		if(amMatchingparentheses()){
			// TODO are there any other characters that we might need to remove
			// NOTE assuming the character removed is ALWAYS present in INPUTCHARACTERTYPES so we can find its associated input type!!!!!
			//////////if(amVerbose()){inputInfo("Adapting the suggested text.");}
			char characterToRemove='\0';	
			switch(INPUTCHARACTERTYPES[removedCharacter]){
				case '[':characterToRemove=']';break;
				case '{':characterToRemove='}';break;
				case '(':characterToRemove=')';break;
				case '!':characterToRemove='=';break;
				case 'D':case 'S':characterToRemove=removedCharacter;break;
			}
			// if what can be removed matches the first character of the suggested text, remove it
			if(!characterToRemove&&string_char(feedforwardText,0)==characterToRemove){
				if(!string_removed_char(feedforwardText,0)){
					inputError("Failed to remove the first matching character of the suggested text");
				}
			}
		}
		if(string_length(feedforwardText)){
			//////////if(amVerbose())inputInfo("Writing suggested text.");
			writeSuggestedText(true);
		}
	}
	*/
}

// MDH@01OCT2019: updating the type of an identifier token due to the removal of the last token character is now done in removeTokenCharacter() just as commandCharacterAccepted() does!!!
//				also uint16_t behindCursor (as it is always called with constant value 1) removed as formal parameter, and endOfInput (typically true) added!!!
/**
 * @brief removes and returns the last token character
 * 
 * @param endOfInput 
 * @return char 
 */
char removedTokenCharacter(bool endOfInput){
	// MDH@30OCT2019: ASSERT there's something to 'remove'
	// MDH@03SEP2019: 
	char tokenCharacterRemoved='\0';
	if(_userInputCommand!=NULL){ // should ALWAYS be the case
		size_t tokenCharacterPosition=0;
		// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
		Mtoken* lastToken=_userInputCommand->_lastToken;
		if(lastToken==NULL)
			outputBug("Last command token vanished!");
		else
		while(1){
			tokenCharacterPosition=string_length(lastToken->text); // MDH@24APR2019 replacing (what is essentially the same): getUserInputLength()-_userInputCommand->_lastToken->offset;
#ifdef __DEBUG__
			printf("%d",tokenCharacterPosition);
#endif
			if(tokenCharacterPosition>0)break;
#ifdef __DEBUG__
			outputChar('.');
#endif		
			// ASSERT token lastToken is empty, so to be skipped!!!
			// MDH@12APR2024: since we should NOT allow removing the first token (with offset 0) which NOW can have a predecessor we have to demand that lastToken->offset is positive
			//                TODO alternatively we could check against _userInputCommand->_firstToken
			if(lastToken==_userInputCommand->_firstToken)break; // if lastToken is already _userInputCommand->_firstToken (because it's offset is zero) we should not allow lastToken to become 'less'
			lastToken=lastToken->prev;
			if(NULL==lastToken)break; // never allow _userInputCommand->_lastToken to become NULL!!!!
			_userInputCommand->_lastToken=lastToken;
		}
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand->_lastToken); // MDH@02OCT2019: should be called whenever _userInputCommand->_lastToken changes...
		// MDH@28JUN2023: testing tokenCharacterPosition for being positive is probably easier
		if(tokenCharacterPosition>0){ // replacing: _userInputCommand->_lastToken!=NULL)
			tokenCharacterRemoved=string_removed_char(_userInputCommand->_lastToken->text,tokenCharacterPosition-1);
			/////////output("'%c'",tokenCharacterRemoved?tokenCharacterRemoved:'`');
		}
		if(tokenCharacterRemoved){
			if(endOfInput){
				// MDH@30OCT2019: with multiline user input it sometimes is a little harder than calling moveCursorLeft(1)
				//				if the offset of the current user input line is beyond the total command length apparently we've 'removed' the last command character on the previous line
				// MDH@24SEP2020: now it's probably better to test numberOfLineCommandCharacters because it it is zero we're obviously at the first position on a command line
				if(numberOfLineCommandCharacters==0){
					//////outputChar('>');
					// MDH@16OCT2020: think man, there's an explicit or implicit end of line in front of it
					/* replacing:
					size_t userInputLength=getUserInputLength();
					if(_userinputline&&_userinputline->offset>userInputLength){
					*/
					toStartOfLine();clearScreenFromCursor(); // yes, we need this in particular when at the start of a new line and doing a left arrow in which case the prompt on the line needs to be removed
					// MDH@16OCT2020: ok, the following is crucial to set the number of line command characters to where we end up that is one line up
					//				this is the only place where removeUserinputline() is called, so we can adapt it any way we want
					//				aha, I think we have a problem when going to the first line as removeUserinputline() would return 0, so we prevented that from happening...
					numberOfLineCommandCharacters=removeUserinputline(); // MDH@24SEP2020: subtract 1 because we've actually consumed a single character just now!!
					if(numberOfLineCommandCharacters>0)numberOfLineCommandCharacters--;
					toUserInputCursorPosition(-1); // one line up and to the proper position (not sure what will happen to the suggested text though)
				}else{ // still command characters on the current command line
					//////outputChar('<');
					moveCursorLeft(1); // MDH@01OCT2019: this ought to be done BEFORE tokenCheckedForBeingAFunction() is called so we moved it over here!!!
					numberOfLineCommandCharacters--; // MDH@07JUL2020: essential to keep track of the number of line command characters
				}
			}
			// MDH@01OCT2019: whenever the last token does not change but the last token character is removed, we should check the type 
			//				HOWEVER we're assuming that we're dealing with an end of input situation
			// MDH@12APR2024: do NOT allow removing _userInputCommand->_firstToken
			bool tokenRemoved=(string_empty(_userInputCommand->_lastToken->text)&&_userInputCommand->_lastToken!=_userInputCommand->_firstToken?removeLastUserInputCommandToken():false);
			unfinishCommandToken(_userInputCommand->_lastToken); // we need to do this to allow appending characters to the token again
			if(endOfInput){
				if(!tokenRemoved)
					tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,true/*endOfInput*/);
				// MDH@20JAN2024: calling expectedCharacterStackUpdatedOnRemoval() called here because thie function
				//                is called both on left-arrow and backspace and those are exacly the two situations where
				//                we need to update expectedCharacterStack due to removing the last token character from the user
				//                input command
				// TODO should we do this whether or not tokenRemoved is true or not?????
				// MDH@15DEC2023: we need to update the expected character stack as if c was acutally
				//                removed as it is no longer part of the current user input command
				//                (essentially we always need to keep _expectedCharacterStack correct)
				switch(expectedCharacterStackUpdatedOnRemoval(tokenCharacterRemoved,_userInputCommand->_lastToken)){
					case -1:break;
					case 0 :break;
					case 1 :break;
				}
			}
		}
	}else
		inputInfo("%s%sNo command to remove characters from!",M_BUG_PREFIX,M_MESSAGE_PREFIX);
	return tokenCharacterRemoved;
}

// in response to backspace the previous token character is to be removed
/**
 * @brief removes the previous token character
 * 
 * @return true 
 * @return false 
 */
bool removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	if(commandIndex>0){
		commandIndex=0;
		if(!copyUserInputCommand())return false;
	} // MDH@03SEP2019 BUG FIX: I have to do this if scrolling through the list of previous commands!!!
	// ASSERT the current user input command needs at least one character to remove
	char removedCharacter=removedTokenCharacter(true); // MDH@01OCT2019: will now also perform moveCursorLeft(1) when the argument is true and success
	if(!removedCharacter){
		inputError("%s","Failed to remove the last entered character.");
		return false;
	}
	// MDH@01OCT2019: moveCursorLeft(1); // TODO check if this is necessary also when cancelling the command
	if(amVerboseDebugging())
		inputInfo("Character '%c' removed.",removedCharacter);
	// if no text is left in the command we cancel the command (as a service to the user who wouldn't understand that Enter wouldn't switch to Control mode on an otherwise empty command!!!)
	// MDH@18JUN2020: I've adapted copyUserInputCommand() in such a way that if it fails _userInputCommand will NOT be replaced so it will not become NULL (i.e. it will remain as it was), so the following test will always evaluate to TRUE
	if(_userInputCommand!=NULL){ // we still have a command being evaluated (NOTE that removedTokenCharacter() can actually set _userInputCommand->_firstToken to NULL)
		if(_userInputCommand->_lastToken==_userInputCommand->_firstToken&&
				string_length(_userInputCommand->_firstToken->text)==0){
			string_setlength(_expectedCharacterStack,0); // MDH@14DEC2023: unfortunate I have to do this here
			// MDH@20OCT2021 already in cancelCommand(): if(amVerboseDebugging())inputInfo("%s","Cancelling the command.");
			cancelCommand();
			// MDH@20OCT2021 already in cancelCommand(): if(amVerboseDebugging())inputInfo("%s","Command cancelled.");
			/* replacing:
			if(string_length(feedforwardText))inputInfo("Use Ctrl-C to clear the text suggestion as well.");else cancelCommand();
			*/
		}else{
			if(amVerboseDebugging())inputInfo("%s","Updating.");
			updateOnTokenCharacterRemoved(removedCharacter);
			if(amVerboseDebugging())inputInfo("%s","Updated.");
		}
		return true;
	}
	inputError("%s","The user input command vanished.");
	return false;
}
/**
 * @brief returns true if we're currently positioned behind an argument prompt, false otherwise
 * 
 * @return true 
 * @return false 
 */
bool atArgumentPrompt(){
	Mtoken* lastToken=(_userInputCommand!=NULL?_userInputCommand->_lastToken:NULL);
	if(lastToken!=NULL){
		if(lastToken->type==TT_FUNCTION_CALL||lastToken->type==TT_LISTELEMENT){
			if(lastToken->significantCharacterCount>0&&string_last_char(lastToken->text)==':')
				return true;
		}
	}
	return false;
}
bool argumentPromptRemoved(){
	Mtoken* lastToken=_userInputCommand->_lastToken;
	while(string_length(lastToken->text)>lastToken->significantCharacterCount){
		if(!removePreviousTokenCharacter())return false;
	}
	/* replacing:
	size_t numberOfCharactersToRemove=string_length(lastToken->text)-lastToken->significantCharacterCount;
	while(numberOfCharactersToRemove){
		if(!removePreviousTokenCharacter())return false;
		numberOfCharactersToRemove--;
	}
	*/
	unfinishToken(lastToken); // TODO is this really necessary???
	return true;
}
// MDH@09JUL2019: count the number of list elements in front of the current token
/**
 * @brief returns and counts the number of list elements in front of the last token
 * 
 * @return uint32_t 
 */
uint32_t getListElementCount(){
	uint32_t listElementCount=0;
	Mtoken* token=_userInputCommand->_lastToken;
	Mtoken* startToken=_userInputCommand->_lastToken->expr;
	while(token!=startToken){if(token->expr==startToken&&token->type==TT_LISTELEMENT)listElementCount++;token=token->prev;}
	return listElementCount;
}

// MDH@27FEB2020: commandCharacterAppended() moved over to Mshell.c

// MDH@24SEP2020: call newCommandLine() either to start a new command line either explicitly or implicitly (when no further characters fit on the current command line)
/**
 * @brief begins a user input line
 * @details simply writes the continued prompt and outputs the last token color
 * @param newline whether or not this is a new user input line
 */
static void newCommandLine(bool newline){
	numberOfLineCommandCharacters=0; // MDH@24SEP2020: TODO we should never forget to reset numberOfLineCommandCharacters after showing a continued prompt
	showContinuedPrompt(getUserInputLength(),newline);
	outputTokenColor(_userInputCommand->_lastToken);
}

/* MDH@13DEC2023:
		whenever inputChar gets accepted (and added to the current command we should update _expectedCharacterStack based
		on inputChar and possible the current or previous token type
*/
// MDH@12APR2019: in order to implement the Tab character we have to delegate entering a character (typed) to a separate function
//	   		  ASSERTION _userInputCommand->_firstToken and _userInputCommand->_lastToken are  NOT  NULL
//				the endOfInput flag is used to indicate whether this is the end of the input
//				the aSuggestedCharacter flag tells commandCharacterAccepted() that the input character came from feedforwardText (the feed forward), so it will in that case not alter feedforwardText (by removing the same character that was entered)
// MDH@26JUN2020: should now also keep track of the total number of command characters on the current user input line (numberOfLineCommandCharacters)
// MDH@19OCT2020: we are going to return not just true (1) or false (0) but a value that tells a little more (e.g. whether or not a new token was started)
const uint8_t NEW_TOKEN_CHARACTER=2;
const uint8_t FINISHING_TOKEN_CHARACTER=4;
// obsolete: const uint8_t REMOVE_SUGGESTED_CHARACTER_FAILURE=64;
const uint8_t NO_USER_INPUT_ERROR=128;
// upper bits indicate errors, 
/**
 * @brief processes input character \p inputChar of type \p inputCharacterType
 * 
 * @param inputChar the character input by the user
 * @param inputCharacterType the type of the character input by the user
 * @param endOfInput whether or not this is the end of the input
 * @param aSuggestedCharacter whether or not this is a consumed (suggested) character
 * @return uint8_t 
 */
uint8_t commandCharacterAccepted(char inputChar,char *inputCharacterType,bool endOfInput,bool aSuggestedCharacter){Mallocationowner owner=getOwner(__LINE__);
	logToOutputFile("Accepting input character '%c'.\n",inputChar);
	// MDH@11JUL2023: now I have to ascertain that M_NEWLINE_CHARACTER on a comment is accepted instead of rejected
	bool initializationsChanged=false;
	// MDH@21APR2019: there are two situation where we need to get a command
	/////outputChar('1');
	//				1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(NULL==_userInputCommand) // no current command
		createUserInputCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyUserInputCommand();
	/////outputChar('2');
	// if _userInputCommand->_lastToken is now NULL something went wrong (in copyUserInputCommand or createUserInputCommand most likely)
	if(NULL==_userInputCommand){
		inputError("%s%sNo user input command.",M_BUG_PREFIX,M_MESSAGE_PREFIX);
		return NO_USER_INPUT_ERROR;
	}
	
	uint8_t result=1;
	/* MDH@02NOV2021: aSuggestedCharacter already represents whether or not the first sugggested character was consumed in the process!!!
	if(aSuggestedCharacter){
		inputInfo("Consuming first suggested character '%c'.",inputChar);
		if(!removeFirstSuggestedCharacter(inputChar)){
			result|=REMOVE_SUGGESTED_CHARACTER_FAILURE;
			//inputError("Failed to consume suggested character '%c'.",inputChar);
		}else
			inputInfo("First suggested character '%c' removed!",inputChar);
	}
	*/
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	/////outputChar('3');
	// MDH@21SEP2020: clearInfo() is also responsible for the problem with right arrow because the result is that the character is output one line down with every right arrow
	// clearInfo(); // MDH@28FEB2020: is responsible for the experienced problem
	/////outputChar('4');
	/////////if(amVerboseDebugging())inputInfo("A");
	/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
	if(inputCharType=='C'){
		_userInputCommand->_lastToken->type^=0x70; // toggling bit 7
		// a comment character will NEVER change the (actual) token type but it should change the color to use
		if(_userInputCommand->_lastToken->type&0x70){commenting=true;outputTokenColor(_userInputCommand->_lastToken);}else notCommenting=true; // if a comment was started, switch to the comment token color
	}else // not a comment character
	if((_userInputCommand->_lastToken->type&0x70)==0){ // not in a comment
		if(notCommenting){notCommenting=false;outputTokenColor(_userInputCommand->_lastToken);} // if behind coming out of a comment, we have to reset the output token color
	*/
	/*
	// MDH@26FEB2019: when a user starts inserting characters instead of appending them we can cut off the rest of the characters in the command
	//				and put it in a single Mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/

	// MDH@28OCT2019: all the code that deals with updating the tokens 
	//				NOTE passing in the address of _userInputCommand->_lastToken, so it can be changed!!!!
	logToOutputFile("Appending input character '%c'.\n",inputChar);
	Mtoken* newLastCommandToEvaluateToken=commandCharacterAppended(_userInputCommand/*,owner_userInputCommand*/,inputChar,inputCharacterType,endOfInput);
	if(NULL==newLastCommandToEvaluateToken)return -1; // MDH@13DEC2023: can't actually return -1 though!!!!
	if(newLastCommandToEvaluateToken!=_userInputCommand->_lastToken){
		result|=NEW_TOKEN_CHARACTER;
		//////outputLine("Owning the last command token!");
		_userInputCommand->_lastToken=owned_token(newLastCommandToEvaluateToken,Msubowner(owner_userInputCommand,1)); // MDH@28MAY2020: take over ownership of the new last command token
		//////outputLine("Last command token owned!");
		outputUserInputCommandTokenColor();
	}else{
		// MDH@31OCT2019: show whitespace in the standard info color!!
		if(getTokenSignificantCharacterCount(_userInputCommand->_lastToken)>0){
			result|=FINISHING_TOKEN_CHARACTER;
			resetOutputColor();
			if(*inputCharacterType=='W'&&inputChar==M_NEWLINE_CHARACTER)*inputCharacterType=' '; // convert the newlinecharacter (which type should be W to the blank)
		}
		/*
		if(string_last_char(_userInputCommand->_lastToken->text)=='`'&&_userInputCommand->_lastToken->type!=TT_DQSTRING&&_userInputCommand->_lastToken->type!=TT_SQSTRING){
			resetOutputColor();
		}
		*/
	}

	/////if(amVerboseDebugging())inputInfo("J");
#ifdef __DEBUG__
	printf("[%s]",string(_userInputCommand->_lastToken->text));
#endif

	// MDH@07JUL2020: allowing the cursor to be on the last available line position BUT not to allow input characters to appear there!!!!!
	// MDH@21SEP2020: if(numberOfLineCommandCharacters+promptLength+1==numberOfLineCharacters){showContinuedPrompt(true);outputTokenColor(_userInputCommand->_lastToken);}
	// MDH@24APR2019 obsolete: getCommandLength()++; // increment total command length
	// outputChar('>');

	// MDH@16OCT2020: special characters that would typically result in a line break should NOT be written!!
	if(inputChar!='\n'&&inputChar!='\r'){
		outputChar(inputChar); ///////// replacing: outputLastTokenChar(_userInputCommand->_lastToken); // echo the last token character
		numberOfLineCommandCharacters++;
	}

	// MDH21SEP2020:65th birthday: if the line is now full move the cursor to the next line
	//							 not to write a newline character because we are supposedly already at the start of the next line
	if(numberOfLineCharacters>0&&numberOfLineCommandCharacters+promptLength>=numberOfLineCharacters)
		newCommandLine(false);

	//putchar('\b');

	if(endOfInput){
		//////if(amAssisting())output(":%c",*inputCharacterType);
		// debugWrite("Command length after inserting %c: %zu.",inputChar,getCommandLength());
	}
	/* MDH@01NOV2021: perhaps if we move this upwards the error will go away!!
	// MDH@22OCT2021 TODO check if we should always update the suggested text constituent parts even if endOfInput is false (as it would be with Tab)
	if(aSuggestedCharacter){
		inputInfo("Consuming first suggested character '%c'.",inputChar);
		if(!removeFirstSuggestedCharacter(inputChar)){
			result=-2;
			//inputError("Failed to consume suggested character '%c'.",inputChar);
		}else
			inputInfo("First suggested character '%c' removed!",inputChar);
	}
	*/
	// MDH@24APR2019 obsolete: getUserInputLength()++; // increment the current cursor position
	// MDH@07AUG2019: after a character is input by the user (or some other source) the identifier type will be checked...
	//				BUT 
	// MDH@01OCT2019: argument aSuggestedCharacter is no longer used in tokenCheckedForBeingAFunction and consequently by this function, so it is removed as argument and replaced by updateidentifiercontinuation (which we do need)
	bool notCheckedForBeingAFunction=!tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,endOfInput/*,aSuggestedCharacter*/); // MDH@28MAY2019: ALWAYS check for being a function!!!!
	/////if(amVerboseDebugging())inputInfo("K");
	if(endOfInput){
		/* MDH@20SEP2019: because I created updateLastTokenAutoCompletionText which should take care of adding the right token feed forward I do not need to do the following
		// MDH@29APR2019: I'd like to detect when a variable becomes a function or vice versa
		if(notCheckedForBeingAFunction){
			/////if(amVerboseDebugging())inputInfo("L");
			// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
			// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
			if(_userInputCommand->_lastToken->type!=TT_ERROR){ // MDH@29APR2019: don't add closing bracket to autocompletion text when in error!!!
				// MDH@20SEP2019: typically the token type should determine what the feed forward text of the token should be
				//				so it is less optimal to explicitly check for certain input characters instead of simply looking at the type of token that started!!!
				//				even so, knowing that each of the input characters that insert a feed forward character actually started a new token and there's at that point no token feed forward yet
				//				it's easiest to simply set the feed forward text of the last token
				if(!aSuggestedCharacter){ // MDH@14AUG2019: using this flag here means no changes to the suggested text are done, when this character was consumed from the suggested text
					if(_userInputCommand->_lastToken->type!=TT_BINARY_aErU){
						// MDH@14AUG2019: whether or not to insert a feed-forward matching parenthesis is open for debate
						//				this is a bit of an issue because string literal start and ends is the same, and how do we know if a string is started or ended????? solution: test the type
						if(amMatchingparentheses()){
							// MDH@14AUG2019: I think we should always insert the character we need to close the bracket no matter what
							//				NOTE we're using inputCharType here, not inputChar but as you may notice in removeCharacter there it's not using the input character type, I suppose we should
							//				with strings is important only to insert the same character when the inserted character started the string
							// removing: if(!string_length(feedforwardText)) // MDH@28MAY2019: if there's nothing behind the cursor yes we do append closing stuff
							switch(inputCharacterType){
								case '[':
									setLastTokenAutoCompletionText("]"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,']');
									break;
								case '{':
									setLastTokenAutoCompletionText("}"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'}');
									break;
								case '(':
									setLastTokenAutoCompletionText(")"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,')');
									break;
								case 'D':
									if(_userInputCommand->_lastToken->type==TT_DQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutoCompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								case 'S':
									if(_userInputCommand->_lastToken->type==TT_SQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutoCompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								// MDH@14AUG2019: if the user typed the same character as is currently behind the cursor let's remove that character
								//				but only when the character entered is NOT consumed because in that case it was already removed (and shouldn't be removed again if there's another such a character which can happen a lot with matching parentheses)
								default:
									// MDH@20SEP2019: it is well possible that the user will type the first character of the feed forward associated with the token in which case I suppose that character should be removed from the feed forward text
									removeFirstFeedforwardCharacterFromLastTokenWhenMatching(inputChar);
									// replacing: if(string_length(feedforwardText)&&string_char(feedforwardText,0)==inputChar)if(!string_removed_char(feedforwardText,0))inputError("Failed to remove the matching first feed forward character");
									break;
							}
						}
					}else
						setLastTokenAutoCompletionText("="); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'=');
				}
			}
			/////if(amVerboseDebugging())inputInfo("M");
		}
		*/
		// MDH@16NOV2021: NOT all feed forward characters actually have an associated ending, also when removing such a token, the associated end should be removed??????
		//				therefore deciding to ALWAYS update the last token autocompletion text
		// MDH@22OCT2021: with aSuggestedCharacter equal to true puts the responsibility of removing this suggested character from the appropriate constituent part of the suggested text
		//				NOTE that 
		///////if(aSuggestedCharacter){
			////// see above!!!! if(!removeFirstSuggestedCharacter(inputChar))result=false; // inputError("Failed to consume suggested character '%c'.");
			/* replacing:
			if(_identifierContinuationCharacters){
				if(!deleteFirstIdentifierContinuationCharacter())
					inputError("Failed to remove the accepted first identifier continuation character.")
					; // i.e. we're NOT switching to control mode or returning false
			}else{
				if(!deleteFirstAutoCompletionCharacter(inputChar,true))
					// MDH@22OCT2021 already reported on in call: inputError("Failed to remove the accepted first auto completion character.")
					; // i.e. we're NOT switching to control mode or returning false
			}
			*/
		///////}
		///////else // MDH@22OCT2021: I think if we're consuming a suggested character there's no need to append the last token auto completion text (as it would already be there)
		
		updateLastTokenAutoCompletionText(/*acceptedFirstSuggestedCharacterDeleted*/false); // it makes sense to update the current token feed forward text just before actually showing it AND to update the identifier continuation first
		
		// MDH@13DEC2023 TODO how to respond appropriately to some error???????
		switch(expectedCharacterStackUpdatedOnAddition(inputChar,newLastCommandToEvaluateToken)){
			case 0: break;
			case -1:break; // input error
			case -2:break; // appending inputChar failed
			case -3:break; // removing inputChar failed
		}
		// MDH@13DEC2023 END
		
		/////////writeSuggestedText(false); // just in case we removed some character (see TT_FUNCTION->TT_VARIABLE)
		/////if(amVerboseDebugging())inputInfo("N");
		// debugWrite("Command length after writing behind cursor text: %zu.",getCommandLength());
		//////////if(!initializationsChanged)outputStatus(inputChar,*inputCharacterType);
		/////if(amVerboseDebugging())inputInfo("O");

		// MDH@30JUL2024: a want to embed the name of the expected argument if this is a function call or list element token
		if(result&NEW_TOKEN_CHARACTER){
			size_t argumentIndex=0;
			Mtoken *functionToken=NULL,*sequenceToken=NULL;
			if(newLastCommandToEvaluateToken->type==TT_FUNCTION_CALL){
				functionToken=newLastCommandToEvaluateToken->prev;
				argumentIndex=1;
			}else
			if(newLastCommandToEvaluateToken->type==TT_LISTELEMENT){
				Mtoken* startListToken=newLastCommandToEvaluateToken->expr;
				if(startListToken!=NULL){
					if(startListToken->type==TT_FUNCTION_CALL){
						functionToken=startListToken->prev;
						// can we get the actual argument index?????
						if(functionToken!=NULL)argumentIndex=1+startListToken->argument-newLastCommandToEvaluateToken->argument;
					}else
					if(startListToken->type==TT_LIST||startListToken->type==TT_EXPRESSION){
						sequenceToken=startListToken;
						argumentIndex=newLastCommandToEvaluateToken->argument; /////1+startListToken->argument-newLastCommandToEvaluateToken->argument;
					}
				}
			}
			if(functionToken!=NULL||sequenceToken!=NULL){
				finishToken(newLastCommandToEvaluateToken);
				Mstring* _argumentPromptText=owned_string(__string(),owner);
				if(_argumentPromptText!=NULL){
					if(functionToken!=NULL){
						Mstring* _functionNameText=owned_string(_getSignificantTokenText(functionToken),owner);
						Mfunction* function=getFunction(NULL,string(_functionNameText));
						FREE_STRING(_functionNameText,owner);
						char* parameterName=(function!=NULL?getMapKey(function->_parameterMap,argumentIndex):NULL);
						if(NULL==parameterName||NULL==string_append(_argumentPromptText,parameterName)){
							FREE_STRING(_argumentPromptText,owner);_argumentPromptText=NULL;
						}
					}else{
						if(NULL==string_append_ll(_argumentPromptText,argumentIndex)){
							FREE_STRING(_argumentPromptText,owner);_argumentPromptText=NULL;
						}
					}
					if(_argumentPromptText!=NULL){
						if(string_append_char(_argumentPromptText,':')!=NULL){
							////////finishToken(newLastCommandToEvaluateToken);
							char* argumentPrompt=string(_argumentPromptText);
							if(string_append(newLastCommandToEvaluateToken->text,argumentPrompt)!=NULL){
								setColor(getCommentColor());
								while(*argumentPrompt){
									outputChar(*argumentPrompt);
									numberOfLineCommandCharacters++;
									if(numberOfLineCharacters>0&&numberOfLineCommandCharacters+promptLength>=numberOfLineCharacters){
										newCommandLine(false);
										setColor(getCommentColor());
									}
									++argumentPrompt;
								}
							}
						}
						FREE_STRING(_argumentPromptText,owner);
					}
				}
			}
		}

	}
	/////if(amVerboseDebugging())inputInfo("P");
	/////inputInfo("%i",result);
	return result;
}

bool COMMAND_PROCESSOR_AVAILABLE=0;

/**
 * @brief executes \p shellCommandText in the shell
 * 
 * @param shellCommandText 
 * @return int the return code of executing \p shellCommandText
 */
int execute_shellCommandText(char const * const shellCommandText){
	// ASSERTION string_length(_shellCommand) should be positive
	return(shellCommandText!=NULL&&strlen(shellCommandText)>0?system(shellCommandText):INT_MIN);
}

/**
 * @brief executes the text represented in \p pythonCommandValue in the shell calling python
 * 
 * @param pythonCommandValue 
 * @return Mvalue* the result of executing the given python command
 */
Mvalue* Mpython(Mvalue const * const pythonCommandValue,Mvalue const * const sysExitValue){Mallocationowner owner=getOwner(__LINE__);
 	Mvalue* resultValue=NULL;
	// if pythonCommandValue exists it needs to be a file, a text or a list or array
 	if(pythonCommandValue!=NULL&&(pythonCommandValue->type==VT_FILE||pythonCommandValue->type==VT_TEXT||pythonCommandValue->type==VT_LIST||pythonCommandValue->type==VT_ARRAY)){
 		// 1. CREATE AND FILL THE PYTHON SCRIPT FILE
		Mfile* pythonScriptFile=owned_file(_getFile("M.py"),owner);
		if(pythonScriptFile!=NULL){
			output("Opening Python script file.\n");
			/* now we need to open this script file
			Mvalue* pythonScriptFileValue=_getValueOfFile(disowned_file(pythonScriptFile,owner)); // will free the file when failing to wrap it
			if(pythonScriptFileValue!=NULL){
				if(Mfopen(pythonScriptFileValue,_getTextValue("'w"))!=NULL){
				*/
			// TODO we force delete M.py here but we need to check whether the Python command file does not match M.py!!!!
			// NOTE deleting python script file like this did result in it not to be written correctly, so we have to find a way to force open a script file even when it exists!!!
			// MDH@05JUN2024: determine whether or not we're actually supposed to execute the (default) Python script file
			bool pythonScriptFileToExecute=false; // whether or not we're executing the python script file
			Mfile* pythonCommandFile=(pythonCommandValue->type==VT_FILE?pythonCommandValue->value._file:NULL);
			if(pythonCommandFile!=NULL){
				fUpdateStats(pythonCommandFile,false);
				fUpdateStats(pythonScriptFile,false);
				if(pythonCommandFile->staterrno==0&&pythonScriptFile->staterrno==0){
					if(pythonCommandFile->stat.st_dev==pythonScriptFile->stat.st_dev&&pythonCommandFile->stat.st_ino==pythonScriptFile->stat.st_ino)
						pythonScriptFileToExecute=true;
				}else
					outputMessage(M_ERROR_PREFIX,"Failed to determine whether '%s' represents the default Python script file to execute (M.py).\n",string(pythonCommandFile->_name));
			}
			if(pythonScriptFileToExecute||fOpened(pythonScriptFile,owner,"w",true,false)==M_TRUE){
				bool headerLinesWritten=false;
				if(!pythonScriptFileToExecute){
					output("Python script file opened.\n");
					// 1a. WRITE THE HEADER LINES
					headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"import sys",true)==0);
					if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"import atexit",true)==0);
					if(headerLinesWritten){
						// compose the line defining the exit handler
						Mstring* _exitHandlerText=owned_string(_getString("def Matexit(): "),owner);
						if(_exitHandlerText!=NULL){
							Mstring* p=_exitHandlerText;
							if(sysExitValue!=NULL){
								p=string_append(p,"print(\"{0}\".format(");
								if(sysExitValue->type!=VT_TEXT){
									Mstring* _sysExitExpressionText=owned_string(_getValueText(sysExitValue,false,true),owner);
									if(_sysExitExpressionText!=NULL){
										///output("Writing result '%s' to the Python script file.\n",string(_sysExitExpressionText));
										p=string_append(p,string(_sysExitExpressionText));
										FREE_STRING(_sysExitExpressionText,owner);
									}else
										p=NULL;
								}else // the text to write is already wrapped, so we're writing it as is (we're not going to use _getStringText to dequote it??????)
									p=string_append(p,sysExitValue->value._text->_c);
								p=string_append(p,")); ");
							}
							p=string_append(p,"sys.stdout.flush(); return"); // finish exit handler
							headerLinesWritten=(p!=NULL?fWriteCharsToFile(pythonScriptFile,string(p),true)==0:false);
							FREE_STRING(_exitHandlerText,owner);
						}else
							headerLinesWritten=false;
					}
					/* replaces:
					if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"def Matexit(): ",false)==0);
					if(sysExitValue!=NULL){
						if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"print(\"{0}\".format(",false)==0);
						if(sysExitValue->type!=VT_TEXT){
							Mstring* _sysExitExpressionText=owned_string(_getValueText(sysExitValue,false,true),owner);
							if(_sysExitExpressionText!=NULL){
								output("Writing result '%s' to the Python script file.\n",string(_sysExitExpressionText));
								if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,_getTextValue(string(_sysExitExpressionText)),false)==0);
								FREE_STRING(_sysExitExpressionText,owner);
							}
						}else{ // the text to write is already wrapped, so we're writing it as is (we're not going to use _getStringText to dequote it??????)
							if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,sysExitValue->value._text->_c,false)==0);
						}
						if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,")); ",false)==0);
					}
					if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"sys.stdout.flush(); return",true)==0); // finish exit handler
					*/
					if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"atexit.register(Matexit)",true)==0); // register exit handler
					if(headerLinesWritten)headerLinesWritten=(fWriteCharsToFile(pythonScriptFile,"# end of header inserted by M, do ** NOT ** edit before this line!",true)==0); // M warning
				}
				bool pythonScriptLinesWritten=false;
				if(pythonScriptFileToExecute||headerLinesWritten){ // the header was written successfully
					output("Python header lines written.\n");
					// 1b. WRITE THE LINES represented by pythonCommandValue
					if(pythonCommandValue!=NULL){
						if(pythonCommandValue->type==VT_FILE){
							pythonScriptLinesWritten=true;
							if(!pythonScriptFileToExecute){ // another Python file to execute
								// how about opening the python command file if is currently closed
								bool opened=false;
								if(NULL==pythonCommandFile->_f)
									if(fOpened(pythonCommandFile,getValueDataOwner(),"r",false,false)==M_TRUE)
										opened=true;
									else
										outputMessage(M_ERROR_PREFIX,"Failed to open Python command file '%s' to execute.\n",string(pythonCommandFile));
								if(pythonCommandFile->_f!=NULL&&fIsReadable(pythonCommandFile,false)){ // the file is opened and can be read
									outputMessage(NULL,"Copying the Python script lines from '%s' to the Python script file.\n",string(pythonCommandValue->value._file->_name));
									// TODO should we close it before reading the lines from it????
									unsigned long long lineIndex=0;
									Mstring* _pythonSourceLine=fReadLine(pythonCommandFile);
									while(_pythonSourceLine!=NULL){
										lineIndex++;
										// do not write any empty lines at the end
										if(string_length(_pythonSourceLine)){
											if(fWriteCharsToFile(pythonScriptFile,string(_pythonSourceLine),false)==0){ // the source line written successfully
												// construct and write the comment telling what the source line was!
												Mstring* _sourceLineTextComment=owned_string(_getString("' # source: "),owner);
												Mstring* p=_sourceLineTextComment;
												if(p!=NULL){
													p=string_append(p,string(pythonCommandValue->value._file->_name)); // appending the name of the inserted file
													p=string_append_char(p,':');
													p=string_append_ull(p,lineIndex);
													if(p!=NULL&&getValueInteger(Mfwrite(pythonScriptFile,_getTextValue(string(p))))!=0)
														pythonScriptLinesWritten=false;
													if(_sourceLineTextComment!=NULL)FREE_STRING(_sourceLineTextComment,owner);
												}
											}else
												pythonScriptLinesWritten=false;
										}
										// always write EOLN
										if(pythonScriptLinesWritten&&fWriteCharsToFile(pythonScriptFile,"'",true)!=0)
											pythonScriptLinesWritten=false;
										if(!pythonScriptLinesWritten){
											outputMessage(M_ERROR_PREFIX,"Failed to write '%s' to the Python script file.\n",string(_pythonSourceLine));
											break;
										}
										FREE_STRING(_pythonSourceLine,owner);
										///////output("Reading the next line!\n");
										_pythonSourceLine=fReadLine(pythonCommandFile);
									}
									outputMessage(NULL,"Number of Python source lines written: %u.\n",lineIndex);
									if(opened)
										if(fClosed(pythonCommandFile,getValueDataOwner(),true)!=M_TRUE)
											outputError("Failed to close the Python source file");
								}else
									pythonScriptLinesWritten=false;
							}
						}else
						if(pythonCommandValue->type==VT_TEXT){
							if(fWriteCharsToFile(pythonScriptFile,pythonCommandValue->value._text->_c,true)==0)
								pythonScriptLinesWritten=true;
							else
								outputError("Failed to write the Python command text");
							/* replacing:
							Mstring* _pythonCommandText=owned_string(_getStringOfText(pythonCommandValue->value._text,true),owner);
							if(_pythonCommandText!=NULL){
								if(string_prepend(_pythonCommandText,"'")!=NULL&&
										fWriteTextToFile(pythonScriptFile,_getTextValue(string(_pythonCommandText)),true)==0)
									pythonScriptLinesWritten=true;
								else
									outputError("Failed to write the Python command text");
								FREE_STRING(_pythonCommandText,owner);
							}else
								outputError("No Python command text to write");
								*/
						}else
						if(pythonCommandValue->type==VT_ARRAY){
							// write every array element as a python script line
							Mvalue** arrayValues=pythonCommandValue->value._array->values;
							size_t arrayElementIndex=0;
							Mvalue* arrayElementValue;
							pythonScriptLinesWritten=true;
							while(arrayElementIndex<pythonCommandValue->value._array->numberOfElements){
								arrayElementValue=arrayValues[arrayElementIndex];
								if(arrayElementValue!=NULL&&arrayElementValue->type==VT_TEXT){
									Mstring* _pythonCommandText=owned_string(_getStringOfText(arrayElementValue->value._text,true),owner);
									if(_pythonCommandText!=NULL){
										if(NULL==string_prepend(_pythonCommandText,"'")||
												fWriteTextToFile(pythonScriptFile,_getTextValue(string(_pythonCommandText)),true)!=0)
											pythonScriptLinesWritten=false;
										FREE_STRING(_pythonCommandText,owner);
									}else
										outputError("Failed to write a Python source command to the Python script file");
									if(!pythonScriptLinesWritten)break;
								}
								arrayElementIndex++;
							}
						}else
						if(pythonCommandValue->type==VT_LIST){
							pythonScriptLinesWritten=true;
							Mvalue* listElementValue;
							Mlistelement* pythonCommandListelement=pythonCommandValue->value._list->_first;
							pythonScriptLinesWritten=true;
							while(pythonCommandListelement!=NULL){
								listElementValue=pythonCommandListelement->_value;
								if(listElementValue!=NULL&&listElementValue->type==VT_TEXT){
									Mstring* _pythonCommandText=owned_string(_getStringOfText(listElementValue->value._text,true),owner);
									if(_pythonCommandText!=NULL){
										// ascertain to add a FILE_EOLN to write to the Python script file as well
										if(NULL==string_prepend(_pythonCommandText,"'")||
												fWriteTextToFile(pythonScriptFile,_getTextValue(string(_pythonCommandText)),true)!=0)
											pythonScriptLinesWritten=false;
										FREE_STRING(_pythonCommandText,owner);
									}else
										outputError("Failed to write a Python source command to the Python script file");
									if(!pythonScriptLinesWritten)break;
								}
								pythonCommandListelement=pythonCommandListelement->_next;
							}
						}else
							outputError("Python command type invalid!");
					}
				}else
					outputError("Not all header lines written to the Python script file to run");
				// 1c. CLOSE THE PYTHON SCRIPT FILE
				FREE_FILE(pythonScriptFile,owner);
				/* replacing:
				if(fClosed(pythonScriptFile,owner)!=M_TRUE)
					outputError("Failed to close the Python script file");
					*/
				// only when we've succeeded in writing the Python script lines to the Python script file are we going to execute it!!
				if(pythonScriptLinesWritten){
					// 2. EXECUTE THE PYTHON SCRIPT
					int pythonCallErrorcode=execute_shellCommandText("python M.py > M.py.out");
					if(pythonCallErrorcode==0){
						outputMessage(NULL,"Retrieving the contents of the Python output file.\n");
						Mfile* outputFile=owned_file(_getFile("M.py.out"),owner);
						if(outputFile!=NULL){
							// open the output file for reading, which we have to succeed in to continue
							if(fOpened(outputFile,owner,"r",false,false)==M_TRUE){
								//////output("Output file '%s' created.\n",string(outputFile->_name));
								// I suggest reading one line at a time using mfreadline() so we won't read the line separators
								// which would f*ck up Mevalfunction 
								Mlist* evaluatedLinesList=owned_list(__list("python"),owner);
								if(evaluatedLinesList!=NULL){
									Mstring *prevOutputFileLine=NULL,*outputFileLine=owned_string(fReadLine(outputFile),owner);
									long long lineIndex=0;
									while(outputFileLine!=NULL){
										lineIndex++;
										outputMessage(NULL,"Processing Python output line '%s'.\n",string(outputFileLine));
										if(sysExitValue!=NULL){ // only interested in the last valid line, which we will copy
											if(string_length(outputFileLine)>1){ // we know the read line starts with a single quote
												// replace the previously remembered last line by the newly read output file line
												if(prevOutputFileLine!=NULL){FREE_STRING(prevOutputFileLine,owner);prevOutputFileLine=NULL;}
												prevOutputFileLine=outputFileLine;
											}
										}else // register any line in the output file!!!
										if(appendedToList(evaluatedLinesList,owner,_getTextValue(string(outputFileLine)),lineIndex)<=0)
											outputMessage(M_ERROR_PREFIX,"Failed to register line #%lld of the python output file.\n",lineIndex);
										// we should NOT evaluate the read line here, that's basically up to the caller
										if(prevOutputFileLine!=outputFileLine){FREE_STRING(outputFileLine,owner);outputFileLine=NULL;}
										outputFileLine=owned_string(fReadLine(outputFile),owner);
									}
									// if we have remember the last valid line that's the result line to return!!!
									if(prevOutputFileLine!=NULL){
										if(appendedToList(evaluatedLinesList,owner,_getTextValue(string(prevOutputFileLine)),lineIndex)<=0)
											outputMessage(M_ERROR_PREFIX,"Failed to register the result at line #%lld of the python output file.\n",lineIndex);
										FREE_STRING(prevOutputFileLine,owner);
									}
									/* no need to do that here!!! let's close the output file
									if(fClosed(outputFile)!=M_TRUE)
										outputWarning("Failed to close the python output file");
									*/
									outputMessage(NULL,"Number of output lines evaluated: %lld.\n",evaluatedLinesList->numberOfElements);
									if(evaluatedLinesList->numberOfElements){
										if(evaluatedLinesList->numberOfElements==1){ // a single output line
											assignValue(&resultValue,evaluatedLinesList->_first->_value);
											FREE_LIST(evaluatedLinesList,owner);
										}else // return the list of evaluated lines (the indexes represent the lines)
											resultValue=_getValueOfList(disowned_list(evaluatedLinesList,owner));
									}else
										FREE_LIST(evaluatedLinesList,owner);
								}else
									outputError("Failed to create the list with evaluated python output lines");
							} // disowned but not freed yet
							FREE_FILE(outputFile,owner); // will also close it!!!
						}else
							outputError("Failed to wrap the Python output file");
					}else
						outputError("Failed to execute the Python script file");
				}else
					outputError("Failed to add all Python source commands to the Python script file to run");
			}else
				outputError("Failed to open the Python script file for writing to it");
		}else
			outputError("Failed to create the Python scipt file");
	}
// replacing (what worked just fine but relied on os commands that may or may not correctly work on every OS)
// Mvalue* Mpython(Mvalue const * const pythonCommandValue,Mvalue const * const sysExitValue){Mallocationowner owner=getOwner(__LINE__);
// 	Mvalue* resultValue=NULL;
// 	// if pythonCommandValue exists it needs to be a file, a text or a list or array
// 	if(pythonCommandValue!=NULL&&(pythonCommandValue->type==VT_FILE||pythonCommandValue->type==VT_TEXT||pythonCommandValue->type==VT_LIST||pythonCommandValue->type==VT_ARRAY)){
// 		// 1. initialize the header of the python script file
// 		//    we need to put a dollar sign in front of the echo literal so it won't get rid of the double quotes around {0}
// 		Mstring* initializeHeaderPythonInputFileText=owned_string(_getString("echo $'import sys\nimport atexit\ndef Matexit(): "),owner);
// 		if(initializeHeaderPythonInputFileText!=NULL){
// 			Mstring* p=initializeHeaderPythonInputFileText;
// 			if(sysExitValue!=NULL){ // a result expression text defined
// 				p=string_append(p,"sys.stdout.write(\"{0}\".format(");
// 				if(sysExitValue->type==VT_TEXT)
// 					p=string_append(p,sysExitValue->value._text->_c);
// 				else
// 					p=string_append(p,_getValueText(sysExitValue,false,true));
// 				p=string_append(p,")); ");
// 			}
// 			// finish the Matexit() Python function by flushing the Python output buffer, and register it in the atexit module
// 			p=string_append(p,"sys.stdout.flush(); return\natexit.register(Matexit)\n' > M.py");
// 			if(p!=NULL){
// 				int initializeHeaderPythonInputFileErrorCode=execute_shellCommandText(string(initializeHeaderPythonInputFileText));
// 				if(initializeHeaderPythonInputFileErrorCode==0){
// 					// 2. append the python code to the Python input file
// 					bool inputFileCompleted=false;
// 					if(pythonCommandValue->type==VT_FILE){
// 						Mstring* completeInputFileCommandText=owned_string(_getString("cat "),owner);
// 						if(completeInputFileCommandText!=NULL){
// 							Mstring* p=string_append(completeInputFileCommandText,pythonCommandValue->value._file->_name);
// 							p=string_append(p,">> M.py");
// 							if(p!=NULL){
// 								int completeInputFileErrorCode=system(string(p));
// 								if(completeInputFileErrorCode==0)
// 									inputFileCompleted=true;
// 								else
// 									outputMessage(M_ERROR_PREFIX,"The shell command to complete the Python input file failed with error code %d.",completeInputFileErrorCode);
// 							}else
// 								outputError("Failed to create the complete input file shell command");
// 							FREE_STRING(completeInputFileCommandText,owner);
// 						}
// 					}else
// 					if(pythonCommandValue->type==VT_TEXT){
// 						// simply echoing the Python code to execute 
// 						Mstring* completeInputFileCommandText=owned_string(_getString("echo $'"),owner);
// 						if(completeInputFileCommandText!=NULL){
// 							Mstring* p=completeInputFileCommandText;
// 							// I'll have to write this text to a temporary file, let's use "Mpythoninput.txt by default!
// 							p=string_append(p,pythonCommandValue->value._text->_c);
// 							p=string_append(p,"' >> M.py");
// 							if(p!=NULL){
// 								if(execute_shellCommandText(string(p))==0){ // success storing the python input code in the text file
// 									inputFileCompleted=true;
// 									/* replacing:
// 									Mfile* pythonInputfile=owned_file(_getFile("M.py"),owner);
// 									if(pythonInputfile!=NULL){
// 										Mvalue* pythonInputfileValue=_getValueOfFile(disowned_file(pythonInputfile,owner));
// 										if(pythonInputfileValue!=NULL){
// 											Mvalue* pythonInputfileWriteValue=mfwrite(pythonInputfileValue,pythonCommandValue);
// 											long long notWrittenToPythonInputFile=getValueInteger(pythonInputfileWriteValue);
// 											if(notWrittenToPythonInputFile==0){ // success
// 												if(sysExitValue!=NULL){
// 													// the essential thing here is that we cannot call sys.exit() with a text message to use as result, because there's no way of retrieving that
// 													// but what we can do is simply print the sys exit text at the end, so we know the last line of the code actually is the system exit code
// 													// perhaps it's a good idea to somehow mark this line e.g. by appending a comment that indicates that line to be the result text something like ## M result ##
// 													Mstring* sysExitText=owned_string(_getString("\"\nimport sys\nprint('## M result')\nsys.stdout.write('{0}'.format("),owner); // prefixing f means that sysExitText may contain Python style formatting elements!!!
// 													if(sysExitText!=NULL){
// 														if(sysExitValue->type==VT_TEXT){
// 															if(NULL==string_append(sysExitText,sysExitValue->value._text->_c))
// 																sysExitTextAdded=false;
// 														}else{
// 															if(NULL==string_append(sysExitText,_getValueText(sysExitValue,true,true)))
// 																sysExitTextAdded=false;
// 														}
// 														if(sysExitTextAdded)if(NULL==string_append(sysExitText,"))\nsys.stdout.flush()"))sysExitTextAdded=false;
// 														if(!sysExitTextAdded||getValueInteger(Mfwrite(pythonInputfileValue,_getTextValue(string(sysExitText))))){
// 															sysExitTextAdded=false;
// 															outputError("Failed to insert the system exit text");
// 														}
// 														FREE_STRING(sysExitText,owner);
// 													}else
// 														outputError("Failed to create the system exit text");
// 												}else{
// 													// we'd write the flush output buffer python code anyway
// 													Mvalue* flushOutputBufferValue=_getTextValue("'\nimport sys\nsys.stdout.flush()");
// 													if(flushOutputBufferValue!=NULL){
// 														outputValue("Writing the flush output buffer python code '",flushOutputBufferValue,"'.\n");
// 														long long flushOutputBufferValueNotWritten=getValueInteger(Mfwrite(pythonInputfileValue,flushOutputBufferValue));
// 														if(flushOutputBufferValueNotWritten>0)
// 															outputMessage(M_ERROR_PREFIX,"Failed to write %lld flush output buffer characters.",flushOutputBufferValueNotWritten);
// 														else
// 														if(flushOutputBufferValueNotWritten==M_LL_INVALID)
// 															outputError("Failed to write the flush output buffer python code");
// 													}else
// 														outputWarning("Can't write the flush output buffer python code");
// 												}
// 												if(getValueInteger(Mfclose(pythonInputfileValue))!=M_TRUE)
// 													outputError("Failed to close the python input file");
// 												else
// 													output("Writing the python input text to the python input file succeeded!\n");
// 											}else{
// 												// no need to free the file because it's wrapped and will be garbage-collected
// 												pythonInputfile=NULL; // indicating failure
// 												if(notWrittenToPythonInputFile==M_LL_INVALID)
// 													outputError("Failed to write the python text to execute");
// 												else
// 													outputMessage(M_ERROR_PREFIX,"Failed to write %lld python input code characters to the python input file.",notWrittenToPythonInputFile);
// 											}
// 										}else{ // not wrapped input file 
// 											free_file(pythonInputfile);
// 											pythonInputfile=NULL;
// 										}
// 									}
// 									*/
// 								}else
// 									outputError("Failed to complete the Python input file with the Python code");
// 							}else
// 								outputError("Failed to construct the shell command to complete the Python input file");
// 							FREE_STRING(completeInputFileCommandText,owner);
// 						}else
// 							outputError("Failed to create the complete input file shell command");
// 					}else
// 					if(pythonCommandValue->type==VT_ARRAY){
// 						// every element in the array represents a line of text to append to the Python input file
// 					}else
// 					if(pythonCommandValue->type==VT_LIST){

// 					}
// 					if(inputFileCompleted){
// 						// 3. execute the code from the python input file
// 						// TODO we should replace 'python' with the full path returned by whereis python (or where python in Windows)
// 						int pythonCallErrorcode=execute_shellCommandText("python M.py > M.py.out");
// 						if(pythonCallErrorcode==0){
// 							output("Retrieving the contents of the Python output file.\n");
// 							Mfile* outputFile=owned_file(_getFile("M.py.out"),owner);
// 							if(outputFile!=NULL){
// 								//////output("Output file '%s' created.\n",string(outputFile->_name));
// 								Mvalue* outputFileValue=_getValueOfFile(disowned_file(outputFile,owner));
// 								if(outputFileValue!=NULL){
// 									q2outputValue("Processing Python output file '",outputFileValue,"'.\n");
// 									// I suggest reading one line at a time using mfreadline() so we won't read the line separators
// 									// which would f*ck up Mevalfunction 
// 									Mlist* evaluatedLinesList=owned_list(__list("python"),owner);
// 									if(evaluatedLinesList!=NULL){
// 										Mvalue * prevOutputFileLineValue=NULL,*outputFileLineValue=mfreadline(outputFileValue);
// 										long long lineIndex=0;
// 										while(outputFileLineValue!=NULL){
// 											lineIndex++;
// 											if(outputFileLineValue->type==VT_TEXT){
// 												outputValue("Processing python output line '",outputFileLineValue,"'.\n");
// 												if(sysExitValue!=NULL){ // only interested in the last valid line!!
// 													if(strlen(outputFileLineValue->value._text->_c))
// 														assignValue(&prevOutputFileLineValue,outputFileLineValue);
// 												}else // register any line in the output file!!!
// 												if(appendedToList(evaluatedLinesList,owner,/*Mevalfunction(*/outputFileLineValue/*)*/,lineIndex)<=0)
// 													outputMessage("Failed to register line #%lld of the python output file.",lineIndex);
// 												// we should NOT evaluate the read line here, that's basically up to the caller
// 											}
// 											outputFileLineValue=mfreadline(outputFileValue);
// 										}
// 										// if we have remember the last valid line that's the result line to return!!!
// 										if(prevOutputFileLineValue!=NULL)
// 											if(appendedToList(evaluatedLinesList,owner,/*Mevalfunction(*/prevOutputFileLineValue/*)*/,lineIndex)<=0)
// 												outputMessage("Failed to register the result at line #%lld of the python output file.",lineIndex);
// 										// let's close the output file
// 										if(getValueInteger(Mfclose(outputFileValue))!=M_TRUE)
// 											outputWarning("Failed to close the python output file");
// 										output("Number of output lines evaluated: %lld.\n",evaluatedLinesList->numberOfElements);
// 										if(evaluatedLinesList->numberOfElements){
// 											if(evaluatedLinesList->numberOfElements==1){ // a single output line
// 												assignValue(&resultValue,evaluatedLinesList->_first->_value);
// 												FREE_LIST(evaluatedLinesList,owner);
// 											}else // return the list of evaluated lines (the indexes represent the lines)
// 												resultValue=_getValueOfList(disowned_list(evaluatedLinesList,owner));
// 										}else
// 											FREE_LIST(evaluatedLinesList,owner);
// 									}else
// 										outputError("Failed to create the list with evaluated python output lines");
// 								}else // disowned but not freed yet
// 									free_file(outputFile);
// 							}else
// 								outputError("Failed to wrap the Python output file");
// 						}else
// 							outputError("Failed to execute the Python script file");
// 					}
// 				}else
// 					outputMessage("Failed to initialize the Python script file (error code %d).",initializeHeaderPythonInputFileErrorCode);
// 			}else
// 				outputError("Failed to construct the header of the Python script file");
// 			FREE_STRING(initializeHeaderPythonInputFileText,owner);
// 		}
// 	}
 	return resultValue;
}

/**
 * @brief clears the shell command
 * 
 */
void clear_shellCommand(){
	string_setlength(_suggestedText,0/*,owner_suggestedText*/);
	string_setlength(_shellCommand,0/*,owner_shellCommand*/);
	// MDH@24APR2019 obsolete: getUserInputLength()=0;
}

/**
 * @brief executes the shell command
 * 
 */
void executeShellCommand(){
	output(""); // get a new line before we see the result of executing this command!!
	int result=execute_shellCommandText(string(_shellCommand));
	if(result)output("Shell command return code: %d.\n",result);else output("\n"); // non-zero result
	clear_shellCommand(); // ready for the next execution
}

/**
 * @brief switches to shell mode
 * 
 * @param message 
 * @return char the exit code
 */
char switchToShellMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)output("%s",message);
	setInputMode(IM_SHELL);
	clear_shellCommand();
	return 's';
	//output("%s\n $ ","Enter your shell command, and press the Return button to execute.");
}
/**
 * @brief switches to (user input) command mode
 * 
 */
void switchToCommandMode(){
	if(inputMode==IM_COMMAND)return;
	if(inputMode==IM_CONTROL)newline(); // MDH@03MAR2020: TODO let's see if this is OK
	setInputMode(IM_COMMAND);
	// ascertain to not have autocompletion text
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
}
/* MDH@05DEC2022: only called once, so no need for it anymore
char getFirstSuggestedCharacter(){
	return string_char(_suggestedText,0); // replacing for obvious reasons: (_suggestedText?string_char(_suggestedText,0):'\0');
	// MDH@27OCT2021: somehow the following does not seem to work, and it makes perfect sense to use the first character of _suggestedText (if any) NOTE the first character could be the end of text character '\0' which is allowed
	// replacing: return(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0?getFirstIdentifierContinuationCharacter():getFirstAutoCompletionCharacter(autogenerated));
}
*/

/* only called once
// MDH@01OCT2019: I created consumeFirstSuggestedCharacter() today to consume the first suggested character but initially called commandCharacterAccepted with endOfInput flag equal to false
//				I suppose that was wrong because there's exactly one character being accepted and that's also the end of input character 
//				the result will be that updateLastTokenAutoCompletionText(true) and writeSuggestedText(true) are executed already by commandCharacterAccepted() so I won't have to do that here anymore
char getFirstSuggestedCharacterConsumed(char firstSuggestedCharacter,bool endOfInput){
	// if the first suggested character is provided use that, otherwise get any
	if(!firstSuggestedCharacter)firstSuggestedCharacter=getFirstSuggestedCharacter(); // any first suggested character will do
	if(firstSuggestedCharacter){
		char firstSuggestedCharacterInputType=INPUTCHARACTERTYPES[firstSuggestedCharacter];
		if(!commandCharacterAccepted(firstSuggestedCharacter,&firstSuggestedCharacterInputType,endOfInput,true))firstSuggestedCharacter='\0';
	}
	return firstSuggestedCharacter;
}
*/
/* MDH@22OCT2021: not sure whether we need this at all
void deleteFirstSuggestedCharacter(char firstSuggestedCharacter,bool consumed){
	if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){ // a feed forward text character was consumed
		deleteFirstAutoCompletionCharacter(firstSuggestedCharacter,consumed);
	}
}
*/
// MDH@14NOV2019: we want to keep a list of evaluated commands inside the main M environment
//				then the user can use variable M to get at the stored commands, and the M function to get results
Mvalue* M_value=NULL;Mallocationowner owner_M_value=(Mallocationowner){MI_MAIN,__LINE__,1}; // where the list of evaluated commands is to be stored 
// the M function (full name MM or perhaps MMfunction) allows one to use a previous result in a command
/**
 * @brief returns the wrapped (user input) command at index \p indexValue
 * 
 * @param indexValue 
 * @return Mvalue* the wrapped (user input) command at index \p indexValue
 */
Mvalue* MM(Mvalue* indexValue){
	Mvalue* resultValue=NULL;
	Mlist* M_list=(M_value!=NULL&&M_value->type==VT_LIST?M_value->value._list:NULL);
	if(M_list!=NULL){
		// because the command and result are prepended to the M_value list, the index identifies how far to go back
		long long index=(indexValue?getValueInteger(indexValue):-1); // if no index value is specified (e.g. when user called M() without argument), index 1 is used
		Mvalue* commandresultValue=getValueAtIndex(M_list,index); // get the commandresult value stored at the given index!!
		if(commandresultValue!=NULL&&
				commandresultValue->type==VT_LIST&&
				commandresultValue->value._list->_last)
			resultValue=commandresultValue->value._list->_last->_value;
	}
	return resultValue;
}
// MDH@14NOV2019 instead of storing the command itself (which would result in a memory security risk we store the command text instead, that way we can still grab it and evaluate it)
// MDH@25MAY2020: commandText is a constant text and therefore ownership if not to be possesed
/**
 * @brief registers the result M value \p evaluationresultValue of evaluating \p commandText
 * 
 * @param commandText 
 * @param evaluationresultValue 
 * @param commandIndex 
 * @return true on succes
 * @return false on failure
 */
bool registerCommandEvaluation(char const * const commandText,Mvalue* evaluationresultValue,long long commandIndex){Mallocationowner owner=getOwner(__LINE__);
	// prepend a list containing the command and the result to the list wrapped in M_value
	if(M_value!=NULL){
		Mlist* M_list=(M_value->type==VT_LIST?M_value->value._list:NULL);
		if(M_list!=NULL){
			// we know M_list is a list stored in an Mvalue which has owner getValueOwner()
			Mallocationowner owner_M_list=Msubowner(getValueOwner(),1); // MDH@25MAY2020: the owner of M_list
			Mvalue* commandresultValue=_getMapValue(VT_UNDEFINED,false,"registerCommandEvaluation"); // the map that is to contain the command text and its result value text
			if(commandresultValue!=NULL){
				// NOTE we're wrapping the first token of the command into a value which is dangerous because when the list is freed, the token shouldn't!!
				//	  so theoretically that value is weak, whereas the evaluation result is strong
				// TODO find a way to fix this!!!
				// Mtext* _commandText=OWNED(_getText(commandText),owner); // MDH@25MAY2020: when we are wrapping the command text we need to get a copy!!!! NOTE string() is used on an Mstring to pass in the characters in the command text, so we can NOT use that itself as it is owned by the Mstring
				Mvalue* _commandTextValue=_getTextValue(commandText); // TODO _getTextValue expects a char * so why do we need _commandText?????
				Mallocationowner owner_map=Msubowner(getValueOwner(),1); // MDH@09JUN2020: what we can safely assume
				long long commandTextValueMapIndex=appendedToMap(commandresultValue->value._map,owner_map,"c",_commandTextValue);
				if(commandTextValueMapIndex==M_TRUE){
					if(appendedToMap(commandresultValue->value._map,owner_map,"v",evaluationresultValue)>0){
						if(appendedToList(M_list,owner_M_list,commandresultValue,M_LL_INVALID)>0)
							return true;
						outputError("Failed to append the command result to the result list.");
						if(removedFromMap(commandresultValue->value._map,owner_map,"v")<=0)
							outputBug("Failed to remove the result value text from the failed command result registration.");
					}else
						outputError("Failed to append the command result text and result to the result list.");
					// DONE:TODO remove from the map again
					if(removedFromMap(commandresultValue->value._map,owner_map,"c")<=0)
						outputBug("Failed to remove the command text from the failed command result registration.");
				}
				// we may expect _commandTextValue to be repossed by the garbage collector as the text failed to be stored
				// NOTE with commandresultValue not appended to M_list we assume it will be garbage collected!!!!
			}
		}
		outputError("Failed to remember the command and its result value text.");
	}
	return false;
}
/**
 * @brief returns a wrapped map containing the names of all variables in the current execution environment
 * 
 * @return Mvalue* a wrapped map containing the names of all variables in the current execution environment
 */
Mvalue* Mvariables(Mvalue* environmentValue){//Mallocationowner owner=getOwner(__LINE__);
	if(amVerbose())
		outputInfo("Getting the variables!");
	Menvironment* environment=(environmentValue!=NULL&&environmentValue->type!=VT_ENVIRONMENT?environmentValue->value._environment:getExecutionEnvironment());
	// returning the names of the local variables (including the hidden ones)
	// NOTE _getVariableNamesMap() always requires a non NULL environment to start with
	return _getValueOfMap(_getVariableNamesMap(environment));
}
// MDH@15NOV2019: returning value counts (per value type), passing in a list of variable names
// MDH@25NOV2019: what about returning a table??? which is a list
/**
 * @brief returns the wrapped value table of all variables in \p variableNamesValue
 * 
 * @param variableNamesValue 
 * @return Mvalue* the wrapped value table of all accessible variables
 */
Mvalue* Mvalues(Mvalue* variableNamesValue){//Mallocationowner owner=getOwner(__LINE__);
	if(amVerbose())
		outputInfo("Getting the values!");
	// MDH@25NOV2019: requesting the table allows for better reproduction
	//				TODO instead of the table return the text representation of the table (which is easier to inspect!!!)
	return _getValueOfList(_getValuesTable(variableNamesValue));
	// replacing: return _getValueOfMap(_getValuesTable(variableNamesValue),true);
}
// method for reading a text from standard out which means reading characters until Enter-key is encountered!!
/**
 * @brief reads text from standard out as requested by calling this in() function
 * 
 * @param value the wrapped prompt
 * @return Mvalue* the wrapped text input by the user
 */
Mvalue* Min(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* result=NULL;
	// value represents the text to write in front of the prompt for text i.e. it's a prompt text
	Mout(value); // just get it out!!!!
	Mstring* _inText=owned_string(_getString("'"),owner); // initialize _inText to a a single quote character (as required by _getTextValue)
	char c;
	while(inputCharRead(&c)){ // should be Ok to use inputCharRead() here
		outputChar(c);/////output("[%u]",c);
		// how about allowing starting over (with Ctrl-C)
		if(c==13||c==10)break;
		if(c==3){
			string_setlength(_inText,1/*,owner*/);
			outputChar('\n');
			Mout(value);
			continue;
		}
		if(c==127){
			if(string_length(_inText)>1){
				// take the last character off
				string_setlength(_inText,string_length(_inText)-1/*,owner*/);
				backspace();
				outputChar(' ');
				backspace();
			}else
				beep();
			continue;
		}
		if(c==27){ // Escape sequence
			if(inputCharRead(&c)){
				if(c==91){
					if(inputCharRead(&c)){
						if(c==51){
							if(inputCharRead(&c)){
								if(c==126){ // delete
									beep();
								}
							}
						}else
						if(c==65){ // up arrow 
							beep();
						}else
						if(c==66){ // down arrow
							beep();
						}else
						if(c==67){ // right arrow
							beep();
						}else
						if(c==68){ // left arrow
							beep();
						}
					}
				}
			}
			continue;
		}
		string_append_char(_inText,c);
	}
	outputChar('\n'); // go to the next line...
	if(_inText){
		result=_getTextValue(string(_inText));
		FREE_STRING(_inText,owner);
	}
	return result;
}

// MDH@03MAR2020: useful to have a command to execute an OS command
/**
 * @brief returns the result of executing OS command wrapped in \p _commandValue
 * 
 * @param _commandValue the OS command to execute
 * @return Mvalue* 
 */
Mvalue* MexecuteOSCommand(Mvalue* _commandValue){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _commandOutputValue=NULL;
	Mstring* _commandText=owned_string(_getValueText(_commandValue,true,true),owner);
	if(/*_commandText&&*/string_length(_commandText)){
		FILE *fp=popen(string(_commandText),"r");
		if(fp!=NULL){
			Mlist* _commandOutputList=owned_list(_getListOfType(VT_TEXT),owner);	
	  	char path[1036]; // incremented by one to store the single quote
			path[0]='\''; // a single-quote to surround the output text lines
			/* Read the output a line at a time - output it. */
			while(fgets(path+1,sizeof(path)-1,fp)!=NULL){
				// put an end-of-line at the position where end-of-line is encountered
				uint16_t i=0;while(++i<1035)if(path[i]=='\n'||path[i]=='\r')break;path[i]='\0';
				Mvalue* _pathValue=_getTextValue(path);
				if(NULL==_pathValue)continue;
				if(appendedToList(_commandOutputList,owner,_pathValue,M_LL_INVALID)<=0){
					outputError("Not all output retrieved!");
					break;
				}
			}
			pclose(fp);
			_commandOutputValue=_getValueOfList(disowned_list(_commandOutputList,owner));
		}else
			outputMessage(M_ERROR_PREFIX,"Failed to execute OS command '%s'.\n",string(_commandText));
	}
	if(_commandText!=NULL)FREE_STRING(_commandText,owner);
	return _commandOutputValue;
}

// in an interactive session we'll have additional variables to set up
/**
 * @brief prepares the shell environment for an interactive session
 * 
 * @return uint16_t the error flags
 */
uint16_t prepareShellEnvironmentForInteractiveSession(){Mallocationowner owner=getOwner(__LINE__);
	
	uint16_t errorflags=0;

	// Menvironment* _Menvironment=getValueEnviroment(_MenvironmentValue); // MDH@03FEB2020: extracting the environment from its wrapper
	// if(!pushExecutionEnvironment(_Menvironment))return false;
	Mallocationowner owner_executionenvironment=getOwnerExecutionEnvironment();

	// we're gonna need a list to store lists of command and result pairs
	M_value=_getListValue(VT_MAP,false,"M command list"); // don't change the ownership of Mvalue instances

	// MDH@14NOV2019: typically M is created as an immutable variable BUT of course I can change the assigned M_value myself directly but the user can't!!
	//				NOTE if we would have used setValue to set M to M_value it would copy the list that M_value holds instead of using M_value itself, setVariable won't do that
	if(M_value!=NULL){
		if(amVerboseDebugging())output("Adding variable '%s'.\n",M_VARIABLE_NAME);
		if(!addVariable(_Menvironment,owner_executionenvironment,M_VARIABLE_NAME,VT_LIST,true)){
			outputMessage(M_WARNING_PREFIX,"Failed to add variable '%s'.\n",M_VARIABLE_NAME);
			errorflags|=1;
		}else
		if(!setVariable(_Menvironment,M_VARIABLE_NAME,M_value)){
			outputMessage(M_WARNING_PREFIX,"Failed to initialize variable '%s'.\n",M_VARIABLE_NAME);
			errorflags|=1;
		}
		// if M_value wasn't bound to the M variable, it's memory will be freed by the garbage collector, by setting M_value to NULL we know we do not need to update the wrapped list
		if(M_value->count==0){
			M_value=NULL;
			errorflags|=2;
			outputMessage(M_ERROR_PREFIX,"Failed to initialize variable '%s'.\n",M_VARIABLE_NAME);
		}else
		if(amVerboseDebugging())
			outputMessage(NULL,"Variable '%s' initialized.\n",M_VARIABLE_NAME); 
	}else{
		errorflags|=4;
		outputWarning("Failed to create the list in which commands and their values will be stored. You won't be able to use it in your commands!");
	}
	
	// MDH@14NOV2019: the M function allows access to the results of previously executed commands (before reset() clears them all!!!)
	if(M_value!=NULL){
		if(!registerFunction(_Menvironment,owner_executionenvironment,MFUNCTION_NAME,MM,1,(char*[]){"index(integer)"},NULL)){
			errorflags|=8;
			outputMessage(M_ERROR_PREFIX,"Failed to register function %s.\n",MFUNCTION_NAME);
		}else
		if(amVerbose())
			outputMessage(NULL,"Function '%s' registered.\n",MFUNCTION_NAME);
	}

	if(!registerFunction(_Menvironment,owner_executionenvironment,"variables",Mvariables,1,(char*[]){"(environment)"},NULL)){
		errorflags|=16;
		outputWarning("Failed to register the variables() function");
	}
	if(!registerFunction(_Menvironment,owner_executionenvironment,"values",Mvalues,1,(char*[]){"variables(list)"},NULL)){
		errorflags|=32;
		outputWarning("Failed to register the values() function");
	}

	// MDH@27FEB2020: Min is special as it used inputCharRead to read single characters, so it should only be available in sessions
	if(!registerFunction(_Menvironment,owner_executionenvironment,"in",Min,1,(char*[]){"prompt(text)"},NULL)){
		errorflags|=64;
		outputWarning("Failed to register the in function"); // moved out of registerInternalFunctions!!!!
	}
	if(!registerFunction(_Menvironment,owner_executionenvironment,"os",MexecuteOSCommand,1,(char*[]){"os command(text)"},NULL)){
		errorflags|=128;
		outputWarning("Failed to register the os function"); // moved out of registerInternalFunctions!!!!
	}

	// color functions
	if(!registerFunction(_Menvironment,owner_executionenvironment,"bc",Mbc,1,(char*[]){"background color(integer)"},NULL)){
		errorflags|=256;
		outputWarning("Failed to register the bc function"); // moved out of registerInternalFunctions!!!!
	}
	if(!registerFunction(_Menvironment,owner_executionenvironment,"tc",Mtc,1,(char*[]){"text color(integer)"},NULL)){
		errorflags|=512;
		outputWarning("Failed to register the tc function"); // moved out of registerInternalFunctions!!!!
	}
	if(!registerFunction(_Menvironment,owner_executionenvironment,"python",Mpython,2,(char*[]){"python code(file|text)","output variable(text)"},NULL)){
		errorflags|=1024;
		outputWarning("Failed to register the python function"); // moved out of registerInternalFunctions!!!!
	}

	return errorflags;
}

// MDH@21JUN2019: reset() takes care of removing all stored commands
/**
 * @brief removes all stored commands
 * 
 */
void reset(){Mallocationowner owner=getOwner(__LINE__);
	newline();
	/*Menvironment* environment=getExecutionEnvironment();
	if(NULL==environment){outputBug("Environment vanished.");return;}*/
	if(_registeredcommands!=NULL){ // MDH@18JUN2020: testing commandBlocks is better than testing commandCount, and testing _registeredcommands is perhaps even better
		output("Delete all remembered commands? ");
		char c;
		while(!inputCharRead(&c))
		;outputChar(c);newline();
		if(c=='Y'||c=='y'){
			if(commandCount>0){
				output("Deleting %lld command%s.\n",commandCount,(commandCount>1?"s":""));
				unsigned long long numberOfOriginalCommandsFreed=0;
				// TODO we might get a problem if a command is replicated in _registeredcommands // DONE MDH@18JUN2020: commandIndex added to Mcommand so if it is the same as where it is located in _registeredcommands we free it otherwise we do not
				while(commandCount){
					commandCount--;
					if(_registeredcommands[commandCount].previousCommandIndex==0){
						FREE_COMMAND(_registeredcommands[commandCount]._command,owner_registeredcommands); // freeing all new (i.e. not duplicated) commands
						numberOfOriginalCommandsFreed++;
					}
					////////if(commandCount==0)break;
				}
				if(numberOfOriginalCommandsFreed>0)output("Number of original commands freed: %llu.\n",numberOfOriginalCommandsFreed);else outputWarning("No original commands freed");
			}
			if(commandBlocks>0){
				FREE_DISOWNED(_registeredcommands,commandBlocks*COMMAND_BLOCKSIZE,'C',owner_registeredcommands);
				commandBlocks=0;
			} // free all (disowned!!!!!) allocated command blocks
			_registeredcommands=NULL; // MDH@18JUN2020: makes sense to do this as well
			outputInfo("All registered commands deleted!");
			// MDH@18JUN2020: also remove all the items in the command result 
			if(M_value!=NULL){
				Mvalue* clearValue=Mclear(M_value);
				if(isValueZero(clearValue))output("Command result history cleared...\n");else 
				if(isValueNegative(clearValue))outputWarning("Command result history not completely cleared...");
			}
		}else
			outputInfo("Deleting commands canceled by user!");
		if(c==27)while(inputCharRead(&c)); // clear the input buffer
	}else
		outputInfo("No commands to delete!");
#ifndef __PRODUCTION__
	/* MDH@18JUN2020: let's NOT reset the allocation management on reset()
	////syncallocations();
	// Mstring* _hms=owned_string(_getTimestamp("%H:%M:%S"),owner);
	if(resetAllocationManagement())
		outputInfo("Allocation management reset.");
	else
		outputWarning("Failed to reset allocation management.");
	// FREE_STRING(_hms,owner);
	*/
#endif
}

// additional functions are available in an interactive session to be added to the shell environment
// as well as specific functions for displaying input info and input error messages
/**
 * @brief initializes the user interactive session
 * 
 * @return true on success
 * @return false on failure
 */
bool interactiveSessionInitialized(){
	uint16_t errorflags=prepareShellEnvironmentForInteractiveSession();
	if(errorflags){
		output("Errors preparing for running an interactive session (with code %x). Do you want to continue? ",errorflags);
		char answer;
		while(!inputCharRead(&answer))
		;
		outputChar(answer);newline();
		if(answer!='Y'&&answer!='y')return false;
		/*
		if(!blocksInitialized()){outputError("Failed to initialize the subcommand block feature");return false;}
		outputInfo("Subcommand block feature initialized.");*/
	}
	return true;
}
// MDH@27FEB2020: called from within main() only, so can be placed directly in front of main (and separated into a separate M.c or better Minterpreter.c or Mcli.c)
/**
 * @brief prepares for user input
 * 
 * @return true on success
 * @return false on failure
 */
bool preparedForUserInput(){
	// MDH@11AUG2024: ascertain to have a prompt string that we can use as message id
	prompt=owned_string(__string("prompt"),owner_prompt);
	if(NULL==prompt||NULL==string_setlength(prompt,getNumberOfWindowTextColumns())){
		outputError("Failed to initialize the message id string");
		return false;
	}
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	// MDH@23OCT2021: initialize the session passing in the prefix and suffix of the output filename
	bool result=sessionInitialized("M",".log");
	if(result)
		output("Window dimensions: %dx%d.\n",getNumberOfWindowTextColumns(),getNumberOfWindowTextLines());
	else
		outputWarning("Failed to obtain the window dimensions.");
	setbuf(stdout,NULL);
	return true;
}

bool userInputCommandEvaluated(){Mallocationowner owner=getOwner(__LINE__);
	assert(_userInputCommand!=NULL);
	incrementPromptCommandCount(); // replacing: getExecutionEnvironment()->commandCount++; // increment command count BEFORE evaluating as evaluating might create a new environment!!!!!
	Mvalue* userInputCommandResultValue=NULL;
	bool commandEvaluated=evaluateCommand(&userInputCommandResultValue);
	// MDH@25OCT2020: immediately bind the result to the '' variable of the environment
	if(!setVariable(getExecutionEnvironment(),"",(commandEvaluated?userInputCommandResultValue:NULL)))
		outputError("Failed to store the result of the command execution");
	newline();
	// let's mark the allocation directly behind evaluating the command
	if(allocationMarksAdded>0){
		if(allocationMarkAdded())allocationMarksAdded++;
		else outputError("Failed to mark the allocations after evaluating the command.");
	}
	// TODO the next part should be improved, as it is getting a bit messy
	Mstring* _userInputCommandText=owned_string(_getCommandText(false),owner); // MDH@14NOV2019: used in the next part and in registerCommandEvaluation as well, free ASAP do NOT get out unless doing so
	if(!commandEvaluated){
		if(string_length(_userInputCommandText)==0){
			clearCommand();
			outputInfo("Nothing to evaluate!");
		}else // MDH@16MAY2019: no need to tell the user that evaluation failed, because an error message would have been shown to indicate what went wrong (see evaluateCommand())
			outputInfo("Please complete, correct or cancel the command.");
		FREE_STRING(_userInputCommandText,owner); // freed!
		_userInputCommandText=NULL;
		return false;
	}
	resetOutputColor();
	if(amVerboseDebugging())
		outputInfo("Command evaluated!");
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
	// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
	// MDH@18JUN2020: if the current command is not an original command 
	// debugging: Mstring* _commandText=owned_string(_getCommandText(true),owner_userInputCommand);output("Registering command '%s'.\n",string(_commandText));FREE_STRING(_commandText,owner_userInputCommand);
	if(amVerboseDebugging())
		outputInfo("Registering command!");
	/*
	Menvironment* environment=getExecutionEnvironment();
	if(NULL==environment)
		switchToControlMode("Environment vanished.");
	else
	*/
	if(!registerCommand(_userInputCommand,(commandIndex>0?owner_registeredcommands:owner_userInputCommand))){
		if(NULL==getCurrentFunctionBodyInput()){ // not inside a function body
			// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
			if(commandIndex==0){ // a new command being registered!!!
				if(amVerboseDebugging())
					outputInfo("Freeing command!");
				FREE_COMMAND(_userInputCommand,owner_userInputCommand); // MDH@29OCT2019 replacing: freeToken(_userInputCommand->_firstToken);
				outputError("Failed to register the command! Probable cause: out of memory");
			}else
				outputError("Failed to register the command again! Probable cause: out of memory");
		}
	}else{
		if(NULL==getCurrentFunctionBodyInput()){ // not inside a function body
			if(amVerboseDebugging())
				outputInfo("Command registered!");
			if(M_value!=NULL){
				// perhaps we should store the command text not the command itself?????
				// NOTE prepend a single quote is essential to get the text enquoted!!!
				if(NULL==string_insert_char(_userInputCommandText,0,'\'')||!registerCommandEvaluation(string(_userInputCommandText),userInputCommandResultValue,commandCount))
					outputWarning("Failed to store the command and the value it evaluates to for use in subsequent commands.");
				else
				if(amVerboseDebugging())
					output("User input command and result stored in %s.\n",M_VARIABLE_NAME);
			}
		}
	}
	// MDH@16NOV2020 NOTE: think we already freed the user input command text (see above)
	if(_userInputCommandText!=NULL){
		FREE_STRING(_userInputCommandText,owner);
		_userInputCommandText=NULL;
	} // MDH@14NOV2019: freed

	// start anew (without a current command to evaluate!!!!) NOTE the memory is either still pointed to in `commands` or freed because it failed to bind it in commands so we're free to NULL the pointer here!!!
	_userInputCommand=NULL; // MDH@29OCT2019 replacing non Mcommand style (before today): _userInputCommand->_lastToken=_userInputCommand->_firstToken=NULL; // remove reference to current command

	// MDH@18MAR2024: only garbage collect when not inside a block collecting block commands
	if(blockCommandLevel==0){
		// garbage collection: remove any values not used anymore...
		// if(amVerboseDebugging())
		if(amVerbose/*Debugging*/())
			outputInfo("Removing unreferenced values.");
		size_t removedValueCount=getNumberOfRemovedValues(amVerbose()); //amVerbose()&&amVerboseDebugging()); // MDH@12MAY2020: (M_MODULE_DEBUGGING&MM_MAIN) needs to be set to view information on the values released
		if(amVerbose()){
			if(removedValueCount)
				output("Number of garbage collected values: %lu.\n",removedValueCount);
			else 
				outputInfo("No values garbage collected.");
		}
		// MDH@17JAN2023
		size_t removedNulledAllocations=nulledAllocationsRemoved(amVerbose(),amVerboseDebugging());
		if(amVerbose())
			output("Number of nulled allocations removed: %lld.\n",removedNulledAllocations);
		if(amVerboseDebugging())
			reportAllocations("Allocations.\n","\t");

		// switch to function body input mode when this command contained at least one user function definition
		// (even when dealing with currently inputting function body commands)
		if(getFirstFunctionBodyRequest()!=NULL&&!startFunctionBodyInput())
			outputError("Failed to start requesting the body of a new function");

		// MDH@12MAY2020: output two incremental out
		if(allocationMarksAdded>0){
			outputTotalMemoryUsage();
			if(outputIncrementalMemoryUsage(allocationMarksAdded)<allocationMarksAdded)
				outputError("Not all command allocation marks output.");
			while(--allocationMarksAdded>=0)if(!oldestAllocationMarkDropped())break; // drop as many allocation marks as we have created
		}
	}/*else // a little trick to return to entering immediately executing commands
	if(blockCommandLevel<0)
		blockCommandLevel=0;*/
	return true;
}

/**
 * @brief ends the current subcommand block
 * 
 * @param executeCompletedCommand whether or not to execute the final completed command
 * @return true 
 * @return false 
 */
bool endSubcommandBlock(bool removeSubcommandSeparator){
	bool result=true;
	Mblock* endedBlock=endBlock();
	if(endedBlock!=NULL){
		output("Block ended!\n");
		blockCommandLevel--;
		// once a block ended successfully we're back in the environment containing the perhaps now
		// completed command
		// if the command is complete, it should be offered for execution or added
		// if not, we should again prompt the user
		// 1. find the next placeholder token (since we ended a block we're in the original parent
		//    that contains the continuationToken
		Mtoken* nextPlaceholderToken=getCurrentBlock()->continuationToken;
		// MDH@15APR2024: remove a subcommand separator
		if(removeSubcommandSeparator){
			Mtoken* separatorToken=endedBlock->insertToken;
			if(separatorToken->type==TT_LISTELEMENT){
				nextPlaceholderToken->prev=separatorToken->prev;
				separatorToken->prev->next=nextPlaceholderToken;
				separatorToken->next=NULL;FREE_TOKEN(separatorToken,owner_userInputCommand); // release the separator token
				/* replacing:
				separatorToken->prev->next=separatorToken->next;
				separatorToken->next->prev=separatorToken->prev;
				*/
			}
		}
		getCurrentBlock()->continuationToken=NULL; // in case we actually processed the last one
		// MDH@03APR2024: multiple placeholders allowed in the same function call
		//                therefore blockKeywordId should NOT be initialized unless
		//                we decide to reiterate the entire command again from the start
		//                which is probably a good idea because one placeholder is now resolved
		while(nextPlaceholderToken!=NULL&&nextPlaceholderToken->type!=TT_PLACEHOLDER)
			nextPlaceholderToken=nextPlaceholderToken->next;
		if(nextPlaceholderToken!=NULL){ // any placeholder starts a new environment in which commands are to be entered
			//////int8_t nextBlockKeywordId=getPlaceholderBlockKeywordId(nextPlaceholderToken);
			if(startBlock(NULL,nextPlaceholderToken,Msubowner(owner_userInputCommand,1))){
				output("Next subcommand block environment activated!\n"); ///DEBUGGING
				nextPlaceholderToken->next=NULL;FREE_TOKEN(nextPlaceholderToken,owner_userInputCommand);
				////////////_userInputCommand=NULL;
				blockCommandLevel++;
			}else{
				result=false;
				outputError("Failed to start requesting the next subcommand block");
			}
		}else
		if(blockCommandLevel>0){ // the top-level incomplete command is now complete and therefore executable
			// the incomplete command is now finished and completed but not at the top-level
			// and therefore is ready to be added as block command
			Mcommand* completedBlockCommand=getCurrentBlock()->incompleteCommand;
			if(addBlockCommand(completedBlockCommand)){
				getCurrentBlock()->incompleteCommand=NULL;
				completedBlockCommand->_firstToken->next=NULL;
				FREE_COMMAND(completedBlockCommand,owner_userInputCommand);
				output("Subcommand released.\n");
			}else{
				result=false;
				outputError("Failed to add the completed subcommand");
			}
		}else{
			// we have to set _userInputCommand to that completed command
			_userInputCommand=getCurrentBlock()->incompleteCommand;
			if(_userInputCommand!=NULL){
				getCurrentBlock()->incompleteCommand=NULL;
				/////output("Completed command to execute: '");outputCommand(_userInputCommand);output("'.\n");resetOutputColor();
				if(!userInputCommandEvaluated())
					outputError("Failed to evaluate the completed user input command");
			}
		}
	}else
		result=false;
	return result;
}
/**
 * @brief applies session setting character \p sessionSettingCharacter
 * @details the user typically writes these on the command line behind a hyphen
 * @param sessionSettingCharacter 
 * @return signed char a result character
 */
signed char getSessionSettingApplied(char sessionSettingCharacter){
	signed char result=0;
	if(sessionSettingCharacter=='m'||sessionSettingCharacter=='M')setMatchingparentheses(sessionSettingCharacter=='M');
	else
	if(sessionSettingCharacter>='0'&&sessionSettingCharacter<='9'){setColorscheme(sessionSettingCharacter-'0');result='n';}
	else
	if(sessionSettingCharacter=='w'||sessionSettingCharacter=='W')setWrapping(sessionSettingCharacter=='W');
	else
	////////////if(inputChar=='v'||inputChar=='V'){outputVariables();inputCharType='n';break;}
	if(sessionSettingCharacter=='u'||sessionSettingCharacter=='U'){setAcceptinghistorycommand(sessionSettingCharacter=='U');/*result='n';*/}
	else
	if(sessionSettingCharacter=='b'||sessionSettingCharacter=='B'){beep();result='n';}
	else // MDH@23SEP2019: so we can test whether beep() is working... TODO for toggling beeping????
	if(sessionSettingCharacter=='f'||sessionSettingCharacter=='F'){outputFunctions();result='n';}
	else
	if(sessionSettingCharacter=='r'||sessionSettingCharacter=='R'){reset();result='n';}
	else
	// options
	// MDH@31MAR2020: with lowercase 'x' let's ask for confirmation
	if(sessionSettingCharacter=='x'){
		if(!getCurrentFunctionBodyInput()){
			if(!blockCommandLevel){
				output("Do you really want to exit M? ");
				char c;
				while(!inputCharRead(&c))
				;
				outputChar(c);newline();
				if(c=='Y'||c=='y')result='x';
			}else{
				if(!endSubcommandBlock(true))
					outputError("Failed to end the subcommand block");
				result='n';
			}
		}else // in a function body
			result='x';
	}else
	if(sessionSettingCharacter=='X')result='x';
	else
	if(sessionSettingCharacter=='s'||sessionSettingCharacter=='S')result=switchToShellMode(NULL);
	else
	if(sessionSettingCharacter=='h'||sessionSettingCharacter=='H'){
		// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
		// we could call them a, b, c etc.
		if(commandCount){
			commandPages=1+(commandCount-1)/10;
			setCommandPage(commandPages); // as soon as commandPage>0 we are paging...
		}else
			output("%s\n","No commands to show.");
	}else // unprocessed
		result=-1;
	return result;
}

static Mstring* _separator=NULL;Mallocationowner owner_separator=(Mallocationowner){MI_MAIN,__LINE__,1};
/**
 * @brief shows a separator line
 * @details typically called before the prompting for the next command
 */
void showSeparatorLine(){
	int columns=getCurrentNumberOfWindowTextColumns();
	if(columns<=0){outputWarning("No number of columns");return;}
	if(!_separator){_separator=owned_string(__string(),owner_separator);
	if(!_separator){outputError("No separator!");return;}}
	// output("Number of columns: %d.\n",columns);
	size_t separatorlength=string_length(_separator);
	if(separatorlength<3*columns)if(!string_setlength(_separator,3*columns/*,owner_separator*/)){outputError("Failed to resize the separator.");return;}; // MDH@23APR2020: prudent to ascertain that the text is sufficient long enough to contain the separator characters
	// output("Expanding the separator!");
	while(separatorlength<3*columns){string_setchars(_separator,separatorlength,"\u2500");separatorlength+=3;} // ascertain that the separator contains the separator characters
	// output("Separator expanded!");
	output("%.*s",3*columns,string(_separator)); // ASCII 196 is the character that spans an entire column in the middle (better then the underscore)
}
/**
 * @brief returns the last token type
 * 
 * @return TokenType the last token type
 */
TokenType getLastTokenType(){
	return(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL?_userInputCommand->_lastToken->type:TT_ERROR);
}
/**
 * @brief returns the suggested character accepted as input
 * 
 * @param inputCharType 
 * @return char 
 */
char getAcceptedSuggestedCharacter(char suggestedCharacter,char * const inputCharType){
	char acceptedSuggestedCharacter='\0';
	// if no suggested character is provided, get the first one (if any)
	if(!suggestedCharacter)
		if(suggestedTextSources[0]%5)
			suggestedCharacter=getFirstSuggestedCharacter();
	if(suggestedCharacter){
		// MDH@31OCT2019: this might well be a newline character!!!
		char suggestedInputCharType=INPUTCHARACTERTYPES[suggestedCharacter];
		// MDH@21SEP2020: it is a suggested character isn't it????? didn't help changing false to true!!!!
		// MDH@22OCT2021: changed false to true NOW because theoretically it is true now, and so should be marked as true
		//				this is to prevent from adding the matching parenthesis again!!!!
		// MDH@02NOV2021: not being able to consume the first suggested character is a bug and reported as such, but should not prevent further command character acceptation
		if((commandCharacterAccepted(suggestedCharacter,&suggestedInputCharType,true,true)&NO_USER_INPUT_ERROR)==0){
			if(!removeFirstSuggestedCharacter('\0'))
				inputError("%s%sFailed to remove the first suggested character '%c'. See the log file for details!",M_BUG_PREFIX,M_MESSAGE_PREFIX,suggestedCharacter);
			//string_removed_char(_suggestedText,0); // TEST
			*inputCharType=suggestedInputCharType; // MDH@31OCT2019: because might have changed!!!
			if(*inputCharType==' ')newCommandLine(true); // MDH@24SEP2020 replacing and improving upon: showContinuedPrompt(true,true); // MDH@31OCT2019: we just consumed a newline (request) character

			// MDH@18JUL2024: the following helps in preventing adding characters to consume over and over again
			//                TODO this is not an optimal solution!!!
			TokenType lastTokenType=getLastTokenType();
			if(lastTokenType!=TT_ERROR){
				// MDH@18JUL2024: the following is an problem when it keeps adding the same character over and over again
				//                like a closing single quote, and when tabbing it should not again append it
				updateLastTokenAutoCompletionText(true); // MDH@16NOV2021: WILL THIS HELP????? yes, but sometimes we get too many		
				// MDH@18JUL2024: it helps blocking further processing of suggested characters (due to a Tab)
				//                preventing this when the token type is error
				acceptedSuggestedCharacter=suggestedCharacter;
			}///else clearAutocompletionText();
			// MDH@22OCT2021: the consumed suggested character has already been removed by commandCharacterAccepted(), so no need to do that here anymore
			//				BUT we do need to adapt _suggestedText unless it's redetermined from the adapted constituent parts
			/*
			// where to remove it from????
			// NOTE identifier continuation and immediate feed forward are redetermined automatically so do not need to be adjusted here
			if(string_length(_manualFeedforwardText)){
				if(getFirstManualFeedforwardCharacterRemoved()!=c)
					inputCharType=switchToControlMode("Wrong first suggested character accepted!");
			}else
			if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){
				if(string_length(_immediateFeedforwardText)==0){
					if(firstAutoCompletionCharacterRemoved()=='\0')
						inputCharType=switchToControlMode("Failed to remove the accepted first auto completion character.");
				}
			}
			*/
		}else
			*inputCharType=switchToControlMode("Suggested character not accepted.");
	}
	return acceptedSuggestedCharacter;
}

// MDH@18MAR2024: when all blocks have ended the block commands should be executed sequentially
/**
 * @brief executes the block commands in the current execution environment (which would be the global M environment)
 * @result true on success
 * @result false on failure
bool executeBlockCommands(){
	Mlist* blockCommandList=_Menvironment->blockCommandList;
	if(blockCommandList!=NULL){
		Mlistelement* nextBlockCommandListelement;
		while(blockCommandList->_first!=NULL){
			nextBlockCommandListelement=blockCommandList->_first->_next;
			// evaluate the command
			_Menvironment->expressionToken=blockCommandList->_first->_value->value._token->next; // skip the first TT_EXPRESSION token
			Mvalue* blockCommandResult=getValueOfExpression("block command execution",'e',(TokenType[]){},0);
			// register the executed command and the result, and display the result as always

			FREE_LISTELEMENT(blockCommandList->_first,blockCommandList->weak,getValueDataOwner());
			blockCommandList->_first=nextBlockCommandListelement;
		}
		FREE_LIST(blockCommandList,getValueDataOwner());
		_Menvironment->blockCommandList=NULL;
		return true;
	}else
		outputError("No block commands to execute!");
	return false;
}
*/

/**
 * @brief returns the keyword id associated with argument token \p placeholderToken
 * 
 * @param placeholderToken 
 * @return int8_t the keyword id associated with argument token \p placeholderToken
 */
/*
int8_t getPlaceholderBlockKeywordId(Mtoken const * const placeholderToken){
	int8_t blockKeywordId=-1;
	Mtoken* expressionToken=placeholderToken->expr;
	if(expressionToken!=NULL&&expressionToken->type==TT_FUNCTION_CALL){
		char* _functionName=_getSignificantTokenCharacters(expressionToken->prev);
		blockKeywordId=getBlockKeywordId(_functionName);
		free(_functionName);
	}
	return blockKeywordId;
}
*/

// in order to be able to use M from a cpp kernel app
void initializeM(){
	output("Initializes M for use from a Jupyter Notebook kernel!\n");
	output("Not implemented yet!");
	// should do whatever needs doing, before the REPL is started in main()
}

/**
 * @brief main entry point of M
 * 
 * @param argc 
 * @param argv 
 * @param envp 
 * @return int 
 */
int main(int argc, char **argv,char* envp[]){Mallocationowner owner=getOwner(__LINE__); // using 0 is kind of an exception to the rule that every function has a positive function id

	COMMAND_PROCESSOR_AVAILABLE=system(NULL); // check if there's a command processor available

	// setlocale(LC_ALL,"C"); // now passing the locale to shellInitialized()!!

#ifdef __DEBUG__
	printf("\n%s","Operator token types:");
	printf("\nUnary operator                            : %d.",TT_UNARY);
	printf("\nAssignment operator                       : %d.",TT_ASSIGNMENT);
	printf("\nBinary operator                           : %d.",TT_BINARY_aeru);
	printf("\nAssignable repeatable binary operator     : %d.",TT_BINARY_AeRu);
	printf("\nEqualizable repeatable binary operator    : %d.",TT_BINARY_aERu);
	printf("\nTEqualizable unfinished binary operator   : %d.",TT_BINARY_aErU);
	printf("\nAssignable binary operator                : %d.",TT_BINARY_Aeru);
	printf("\nTernary operator                          : %d.",TT_TERNARY_aeru);
	printf("\n%s","Unfinishable token types:");
	printf("\nComment (allowed when command is complete): %d.",TT_COMMENT);
	printf("\nError                                     : %d.",TT_ERROR);

#endif

	if(!outputCollectorInitialized())
		outputError("Failed to initialize output collector.");
	else
		outputInfo("Output collection initialized!");
		
	// MDH@07APR2020 NOTE until we do the following allocation types will NOT get registered (which resulted in bug reports when they got freed by the garbage collector at the end)
	// tell user whether allocation recording is active!!!
	outputInfo("Initializing dynamic memory allocation management.");
	if(!allocationRecordingInitialized()){
		outputError("Failed to initialize memory allocation management!");
		exit(1);
	}
	outputInfo("Dynamic memory allocation management initialized!");

	// MDH@23FEB2019: how about being able to continue with commands stored in a file, or perhaps allow for -log <logfile> or log=
	// whereas any filename without prefix is the file to execute at the start
	unsigned long long Debugging=0; // MDH@05DEC2020
	Mstring* _settingsCharacterText=owned_string(__string(),owner);
	if(_settingsCharacterText!=NULL){
		output("Settings characters text: '%s'.\n",string(_settingsCharacterText));
		if(argc>1){
			output("%s\n","Arguments");
			char debuggingCharacters[]={'\0','\0','\0','\0'}; // for each module we take two successive characters
			char settingCharacter,debuggingCharacter;
			for(int arg=1;arg<argc;arg++){
				output("%i. %s\n",arg,argv[arg]);
				if(argv[arg][0]=='-'){ // a setting flag (or flags)
					int i=0;
					while((settingCharacter=argv[arg][++i])){
						// if it's not a session setting we're going to pass it along to shellInitialized (see below) if we can
						if(getSessionSettingApplied(settingCharacter)<0){ // not a session setting
							_settingsCharacterText=string_append_char(_settingsCharacterText,settingCharacter); // register to pass along to shell initialized
							if(!_settingsCharacterText)settingApplied(settingCharacter); // if failing to add, do it from here
						}
					}
				}else
				if(argv[arg][0]=='+'){ // a (M_MODULE_DEBUGGING&MM_MAIN) flag (or flags)
					// for most modules we can stick to using the first two character of the module
					int i=0;char* pos;unsigned long long moduleMask;
					while((debuggingCharacter=argv[arg][++i])){
						if(debuggingCharacter>=97)debuggingCharacter-=32; // just in case
						debuggingCharacters[strlen(debuggingCharacters)]=debuggingCharacter;
						if(strlen(debuggingCharacters)==2){ // we've got two successive characters
							debuggingCharacters[2]=' '; // when we're looking
							pos=strstr(M_MODULE_DEBUG_CHARACTERS,debuggingCharacters);
							debuggingCharacters[2]='\0'; // done looking
							if(pos){ // TODO if we can find a better way to get the flag
								moduleMask=(1<<((pos-M_MODULE_DEBUG_CHARACTERS)/3));
								outputMessage(NULL,"Module (M_MODULE_DEBUGGING&MM_MAIN) mask: 0x%x.\n",moduleMask);
								Debugging|=moduleMask;
							}else
								outputMessage(M_ERROR_PREFIX,"'%s' does not denote a module.\n",debuggingCharacters);
							// and reinitialize
							debuggingCharacters[0]='\0';
							debuggingCharacters[1]='\0';
						}
					}
					debuggingCharacters[0]='\0'; // just in case a user forgot the second character!!! 
					outputMessage(NULL,"Module (M_MODULE_DEBUGGING&MM_MAIN) flags: 0x%x.\n",Debugging);
				}
			}
		}
	}

	// BEFORE using the command-line parameters (will effectuate wrap mode and color scheme) as it will clear the screen!	
	if(!preparedForUserInput()){
		outputError("Failed to initialize the user session");
		resetOutputColor();
		exit(2);
	}
	output("User session initialized.\n");

	// MDH@27FEB2020: initEnvironment() renamed to getShellEnvironment() and moved over to Mshell.h/c
	// MDH@04MAR2020: initialize the shell passing in the required callbacks (replacing the original set... methods in Mshell.h/c) which is better to NOT forget any callbacks
	// MDH@24SEP2020: replacing outputToken by outputTokenText as the shell is not session command line aware (knowing Mcursormovement)
	// MDH@07DEC2020: try to switch to the local locale (passing empty string as locale)
	if(!shellInitialized((_settingsCharacterText?string(_settingsCharacterText):NULL),"",Debugging,inputCharRead,inputInfo,inputError,outputTokenText,reoutputToken,updateLastTokenAutoCompletionText,NULL)){ // ascertain to have an shell environment!!!
		outputError("Failed to initialize the M shell!");
		resetOutputColor();
		exit(3);
	}
	output("Shell initialized.\n");
	if(_settingsCharacterText){FREE_STRING(_settingsCharacterText,owner);_settingsCharacterText=NULL;}

	_Menvironment=getExecutionEnvironment(); // the currently executing environment will be referenced in _Menvironment

	// prepare an interactive session
	if(!interactiveSessionInitialized()){
		outputError("Failed to initialize the interactive session");
		resetOutputColor();
		exit(2);
	}
	output("Ready for an interactive session.\n");

	if(!blocksInitialized()){
		outputError("Failed to activate the subcommand block feature");
		resetOutputColor();
		exit(3);
	}

	resetOutputColor(); // just in case
	///////q2outputandcollect("%s\n","Welcome to M.");
	output("Welcome to M.");
	newline();
	q2collect("Version: %s - Build: %s - Date: %s - Build at: %s.\n",M_VERSION,M_BUILD,M_DATE,M_TIMESTAMP);
	newline();
	displayFlags();
	newline();
	
	// MDH@11NOV2019: at this point getNumberOfValues() still represents the actual number of remembered values (before values are removed from it)
	////if(amVerbose())
		output("M shell initialized with %llu predefined values.\n",getNumberOfValues());

	// done by getShellEnvironment()!!!! pushExecutionEnvironment(_Menvironment);
	
	Mstring* predefinedVariableNames=owned_string(_getVariableNames(getExecutionEnvironment(),", "),owner);
	if(predefinedVariableNames!=NULL){
		output("Predefined variables: %s.\n",string(predefinedVariableNames));
		FREE_STRING(predefinedVariableNames,owner); // no get rid of it!!!
		predefinedVariableNames=NULL;
	}else
		outputWarning("No predefined variables!");
	//////////output("Number of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	_shellCommand=owned_string(__string(),owner_shellCommand); // MDH@12APR2019: allow executing shell commands (calling system())
	// MDH@20SEP2019 removing: feedforwardText=__string(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	_userInputCommand=NULL; // the current (user input) command

	///// writeCommand() will take care of this!!!! getCommandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

	// MDH@18MAR2024: we start a the immediate user input command execution mode i.e. not inside a block of commands

	// TODO shouldn't we do this in initEnvironment? (or its alternative initM() yet to be created)
	_immediateFeedforwardText=owned_string(__string(),owner_immediateFeedforwardText);
	if(NULL==_immediateFeedforwardText)
		outputError("Failed to allow immediate feed forward"); // TODO we can do better than this!!

	// MDH@13JUL2023: closers are characters that end a (function call argument) (list), a array (element) or a map (element)
	//                NOTE that with function calls the amount of argument list elements is limited, whereas in an array or map it is not
	//                     essentially this means that with function calls we could technically put the argument separators in the closers
	//                     and consume them one by one, whereas in arrays or maps the amount of elements is unlimited, and the element separator that we expect
	//                     will always be the comma, which is a nuisance because it would prevent showing the closing parentheses to the user, unless
	//                     we decide to show both the comma and the closing parenthesis, and the comma is deletable!!!! 
	_expectedCharacterStack=owned_string(__string(),owner_expectedCharacterStack);
	if(NULL==_expectedCharacterStack)
		outputError("Failed to feed forward closing parentheses!");

	_suggestedText=owned_string(__string(),owner_suggestedText);
	if(NULL==_suggestedText)
		outputError("Failed to allow suggested text");
	/* do not initialize _manualFeedforwardText because there's now a difference between manual feed forward being NULL or empty (not blocking vs blocking identifier continuation)
	_manualFeedforwardText=__string();if(!_manualFeedforwardText)outputError("Failed to allow manual feed forward");
	*/
	
	char inputChar,inputCharType;

	newline();
	output("Use Ctrl-Z to exit M immediately at any time.\n");
	output("In any mode press the Enter key on an empty line to switch modes.\n");

	// let's mark the allocations BEFORE we start looping

	// MDH@13MAR2020: echo all requested output to the log file as well, I suppose we should use a timestamp in the name, so we get a different log for each session
	// MDH@23OCT2021: always logging to M.log (for easy access by user simultaneously)
	Mstring* _outputFilename=owned_string(__string(),owner); // MDH@23OCT2021 replacing: _getTimestamp("%Y-%m-%d.%H:%M:%S"),owner);
	if(_outputFilename!=NULL){
		if(string_prepend(_outputFilename,"M")&&string_append(_outputFilename,".log")){
			if(setOutputFilename(string(_outputFilename))){
				dontEchoToOutputFile(); // turn off what setOutputFilename turned on
				Mstring* _sessionStartTimestamp=owned_string(_getTimestamp(NULL),owner);
				if(_sessionStartTimestamp){
					outputToFile("M session start at ",string(_sessionStartTimestamp),".\n");
					FREE_STRING(_sessionStartTimestamp,owner);
				}else
					outputBug("Failed to obtain a session start timestamp");
				outputMessage(NULL,"Session information will be written to %s.\n",string(_outputFilename));
			}else
				outputMessage(M_ERROR_PREFIX,"Failed to open %s for writing session information to.\n",string(_outputFilename));
		}else
			outputError("No session log will be written, due to failing to compose the output filename");
		FREE_STRING(_outputFilename,owner);
		_outputFilename=NULL; // MDH@16NOV2020: precaution
	}

	if(getNumberOfAllocationMarks()==0){
		if(!allocationMarkAdded()){
			outputError("Failed to create the first allocation mark!");
			exit(3);
		}
	}

	if(amVerbose())
		reportAllocations("Initial allocations:\n","\t");

	// the main user input loop
	while(1){ // command loop

		outputTotalMemoryUsage(); // have to think about this though

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//				if _userInputCommand->_firstToken is NOT null, we have to make it NULL
		commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
		if(_userInputCommand->_firstToken) // if we still have a command to free, free it entirely
			if(!clearCommand())
				switchToControlMode("Switching to control mode, due to failing to remove the command.");
		*/
		/* replacing (there shouldn't be a command to write right now, unless perhaps when someone entered an invalid command???? to be continued)
		   point: an evaluated command should be discarded??? in which case a user cannot correct it and has to type it in again
		   so it makes sense to be allowed to complete a command (that failed to evaluate)
		*/
		// TODO what if we're not in inputMode here??????
		if(inputMode==IM_COMMAND){
			commandIndex=0; // TODO should we do this always (even if we have an incomplete command?????)
			// MDH@24APR2019: _userInputCommand->_firstToken could be non-null if we failed to evaluate it (e.g. when being imcomplete), and we allow a retry
			//				NOTE registered commands should always be successfully evaluated, so do NOT get rid of any pending command!!!!
			// MDH@24SEP2020: the result of outputCommand will now be the number of command characters written on the last command line, and therefore should be assigned to numberOfLineCommandCharacters!!!!
			if(_userInputCommand!=NULL)
				numberOfLineCommandCharacters=outputCommand(_userInputCommand);
			else 
				deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0);
			/////////////// if(_userInputCommand->_firstToken)clearCommand(); // TODO do we need this????
			/* replacing:
			if(_userInputCommand->_firstToken==NULL)if(!string_setlength(feedforwardText,0))output("??"); // TODO should we be loosing feedforwardText here????
			getCommandLength()=getUserInputLength()=writeTokens(_userInputCommand->_firstToken);
			*/
			/////////outputStatus();
		}
		// which used to be: outputCommand(); // write the current command (if any)

		/* replacing:
		_userInputCommand->_lastToken=_userInputCommand->_firstToken;
		// an existing command to show
		while(_userInputCommand->_lastToken!=NULL){
			outputToken(_userInputCommand->_lastToken);
			// the cursor will move along with every printf()
			getUserInputLength()+=string_length(_userInputCommand->_lastToken->text);
			_userInputCommand->_lastToken=_userInputCommand->_lastToken->next;
		}
		*/

		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(1){ // command input loop
			
			if(inputMode==IM_COMMAND){
				// MDH@07OCT2019: the newly created feed forward category (manual) takes precedence over the identifier continuation and immediate feed forward text
				//				although it is shown in the same color
				// MDH@07OCT2019: now decided to ALWAYS update the identifier continuation BUT it will be merged with the manual feed forward text
				// MDH@23JAN2024: do NOT update the identifier continuation when it was deleted!!!! which is true when it's length equals 0
				//                this means that now we need to delete _identifierContinuationCharacters every time we want it to get updated!!!!!
				/////////if(NULL==_identifierContinuationCharacters)
					updateUserInputCommandIdentifierContinuation();
				// if we have manual feed forward starting with the given identifier continuation, the identifier continuation will remain
				// and the identifier continuation will be removed from the manual feed forward
				if(_manualFeedforwardText!=NULL){ // existing manual feed forward text that may block identifier continuation characters
					if(amVerboseDebugging())
						inputInfo("Determining manual feed forward text.");
					// if _manualFeedforwardText is empty ANY identifier continuation will be blocked (e.g. when a single identifier continuation character is removed)
					numberOfIdentifierContinuationManualFeedforwardCharacters=string_number_of_matching_chars(_manualFeedforwardText,_identifierContinuationCharacters);
					// MDH@08OCT2019: when the manual feed forward matches the start of the identifier continuation use the latter
					//				this poses a problem though, the alternative being to always show the identifier continuation behind the manual feed forward
					//				which means that we have to get rid of the identifier continuation if we are not showing it
					//				the point is we cannot simply get rid of the manual feed forward text being the result of a number of left arrow actions
					//				we can only augment it with identifier continuation although the identifier continuation would reappear automatically when we do
					/*
					if(numberOfIdentifierContinuationManualFeedforwardCharacters==string_length(_manualFeedforwardText)){ // the entire manual feed forward text starts the identifier continuation
						// we want to keep the manual feed forward but also show the remainder of the identifier continuation
						// there's no need to delete the identifier continuation because the manual feed forward takes precendence over showing the identifier continuation
						if(strlen(_identifierContinuationCharacters)>numberOfIdentifierContinuationManualFeedforwardCharacters){
							string_append(_manualFeedforwardText,_identifierContinuationCharacters+numberOfIdentifierContinuationManualFeedforwardCharacters);
						}
						// using the identifier continuation instead of the manual feed forward text replacing: FREE_STRING(_manualFeedforwardText);_manualFeedforwardText=NULL;
					}
					*/
					///// replacing: if(_identifierContinuationCharacters){
						/* replacing:
						// MDH@06OCT2019: if they are the same we should prefer the identifier continuation
						if(strcmp(string(_manualFeedforwardText),_identifierContinuationCharacters)) // manual feed forward text starts with the identifier continuation
							deleteIdentifierContinuation();
						else
							string_setlength(_manualFeedforwardText,0);
						*/
						/* replacing:
						size_t numberOfIdentifierContinuationCharacters=strlen(_identifierContinuationCharacters);
						if(numberOfIdentifierContinuationCharacters>0&&numberOfIdentifierContinuationCharacters<=string_length(_manualFeedforwardText)){
							char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationCharacters);
							if(strcmp(string(_manualFeedforwardText),_identifierContinuationCharacters)==0){ // manual feed forward text starts with the identifier continuation
								numberOfIdentifierContinuationCharacters-=string_removed(_manualFeedforwardText,0,numberOfIdentifierContinuationCharacters);
								// NOTE the \0 that we put in will end up at position 0 if successful removal of the identifier continuation from the manual feed forward text
								// if not all removed that should've been removed report!!
								if(numberOfIdentifierContinuationCharacters>0)inputError("Failed to merge manual and identifier continuation feed forward text");
							}else // no match
								deleteIdentifierContinuation();
							string_replacedchar(_manualFeedforwardText,c,numberOfIdentifierContinuationCharacters);
						}else // no match
							deleteIdentifierContinuation();
						*/
					//////////}
				}
				////////////if(amVerbose()||!amVerboseDebugging())inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
				// if we do NOT have manual feed forward text, 'update' the immediate feed forward text i.e. only show immediate feed forward text when there's no manual feed forward text!!!
				// get rid of the current immediate feed forward text and update it
				string_setlength(_immediateFeedforwardText,0/*,owner_immediateFeedforwardText*/);
				//// MDH@13DEC2023 NOT HERE: string_setlength(_expectedCharacterStack,0); // MDH@13JUL2023: get rid of the current list of feed forward closers

				////outputChar('A');
				if(/*!_manualFeedforwardText||*/string_length(_manualFeedforwardText)==0)
					if(getLastTokenType()!=TT_ERROR)
						updateUserInputCommandImmediateFeedforwardText();
				////outputChar('B');
				// MDH@03OCT2019: some feed forward texts are also current token specific, therefore we need to sync the feed forward texts
				//				TODO perhaps we should distinguish between feed forward and auto completion (as with the brackets)
				//				DONE solved this by taking care of immediate feed forward texts whenever the current token (type) changes
				///////////updateFeedforwardTexts();
				updateAutoCompletionText(); // to force it being reconstructed!!! TODO if we decide to always do that we do not need to do this here!!!
				////////updateFeedforwardClosers(); // MDH@03DEC2023: replacing the auto completion text

				////outputChar('C');
				showSuggestedText();
				////outputChar('D');
				if(amVerboseDebugging())outputDebugInfo();
				/////outputChar('E');

				// if the line is full now we put the character on the next line
				// if(numberOfLineCommandCharacters+promptLength==numberOfLineCharacters){oneLineDown();showContinuedPrompt();}

			}
			////////outputChar('X');

			if(amVerboseDebugging())
				inputInfo("%d %d(%d) %d(%d) %d(%d) %d(%d) M='%s' C='%s' I='%s' E='%s'",
										suggestedTextSources[0],
										suggestedTextSources[1],getNumberOfSuggestedCharacters(suggestedTextSources[1]),
										suggestedTextSources[2],getNumberOfSuggestedCharacters(suggestedTextSources[2]),
										suggestedTextSources[3],getNumberOfSuggestedCharacters(suggestedTextSources[3]),
										suggestedTextSources[4],getNumberOfSuggestedCharacters(suggestedTextSources[4]),
										string(_manualFeedforwardText),
										(_identifierContinuationCharacters!=NULL?_identifierContinuationCharacters:""),
										string(_immediateFeedforwardText),
										string(_expectedCharacterStack));

			// ask the user for input
			// MDH@30JUN2020: blocking call inputCharRead() replaced by a non-blocking call that allows executing updateNumberOfLineCharacters after each 1/10 second timeout
			if(!inputCharReadNonBlocking(&inputChar,&updateNumberOfLineCharacters))break; // let's see how the terminal window wraps... &updateNumberOfLineCharacters))break;

			// outputChar(inputChar);

			// hide the suggested text again before processing the character read
			if(inputMode==IM_COMMAND){
				hideSuggestedText();
				/*
				Mstring* _commandText=owned_string(_getCommandText(false),owner);
				logToOutputFile("\nCommand: '%s'\n",string(_commandText));
				FREE_STRING(_commandText,owner);
				*/
				logToOutputFile("\tInput character: '%c'(%d)\n",inputChar,inputChar);
			}

			////////inputChar=getInputChar();

			if(inputChar>127)continue; // undefined input character

			// MDH@29JUN2020: if we fail to update the number of line characters we switch to control mode
			if(!updateNumberOfLineCharacters()){
				// TODO consider storing the (unfinished) command (the same way we do when it ends with an error token)
				switchToControlMode("The number of available positions for command characters is too small. Please increase the viewport width.");
			}else
				inputCharType=INPUTCHARACTERTYPES[inputChar];
			
			///// WHY WAS IT DOING THIS!!!!!!! outputChar(inputCharType);
			/* something terribly going wrong when the following code is executed!!!
			if(inputMode==IM_COMMAND){
				if(!amVerboseDebugging())outputStatus(inputChar,inputCharType);
			}
			*/
			logToOutputFile("\t\tInput char type: %c(%d)\n",inputCharType,inputCharType);

			// if not in control mode, and the switch to control mode character is entered, switch to control mode if first character (NOTE getUserInputLength() is only defined in the other two modes)
			// MDH@16APR2019: I want to use the Enter key (ASCII 13) to switch to the next mode, because the associated input character type is n which will ALWAYS break
			//				in that case we do NOT need the o input character type!!!
			if(inputCharType=='o'){
				if(inputMode==IM_CONTROL){
					switchToCommandMode();
					break;
				}
				// not in control mode, go to control mode if first character on line
				if(!getUserInputLength()){
					inputCharType=switchToControlMode(NULL);
					break;
				}
				// accept (might be an acceptable character in string literal in commands or in shell commands)
			}

			/////if(inputChar!=ESCAPE_CHARACTER)printf("(%d)",inputChar);

			// special (control) input character types
			// first the ones that will break in any input mode!!!!
			if(inputCharType=='i')continue; // insignificant input character without specific purpose

			if(inputCharType=='n'){ // end-of-line (now only the LF character, the CR character has been mapped to r now!!!!)
				// MDH@27NOV2019: how about treating the Enter key as line break when the token is finished
				/////////outputChar('X');
				///////inputInfo("Checking for command continuation");
				if(inputMode==IM_COMMAND){
					if(_userInputCommand!=NULL&&_userInputCommand->_lastToken!=NULL){
						// MDH@09MAR2020: I forgot to check whether the last token supposedly is a function which actually is a variable
						if(_userInputCommand->_lastToken->type==TT_FUNCTION){
							changeFunctionTokenToAVariable(_userInputCommand,false);
							/* replacing: can't use tokenCheckedFor because that only applies to changed token text and not to a function name used at the end of an expression
							if(tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,true)){
								if(_userInputCommand->_lastToken->type==TT_VARIABLE)inputInfo("%s","Token recognized as variable!");
							}else
								inputInfo("%s%s",M_ERROR_PREFIX,"Checking the token for being a variable failed!");
							char c;inputCharRead(&c);
							*/
						}
						int8_t aValidCommandIndicator=isAValidCommandIndicator(_userInputCommand/*,owner_userInputCommand*/,false);
						/////////// do NOT do this twice... 
						inputInfo("Valid command indicator: %d.",aValidCommandIndicator);
						// not using \ for newline continuation forces me to actually check whether the command is valid!!
						if(aValidCommandIndicator<=0){ // MDH@10MAR2020: use false for the report parameter because isAValidCommand uses outputInfo/Error which we cannot use during user input!
							/////////inputInfo("User newline break");
							// MDH@08JUL2023: comments may be finished
							/* somehow can't finish an intermediate comment token somehow, otherwise the next line will given an error
							// MDH@17JUL2023: if not doing this we'll loose them in _getCommandText() 
							if(_userInputCommand->_lastToken->type==TT_COMMENT)finishToken(_userInputCommand->_lastToken); 
							*/
							// MDH@11JUL2023: prevent breaking out of the loop when the last token is a comment, because now comments are allowed at the end of each command line
							//                which means that the Enter key simply becomes part of a comment that ends a line
							// removing again: if(_userInputCommand->_lastToken->type!=TT_COMMENT){
							// MDH@11JUL2023: doing so helped to prevent breaking BUT did not end the comment so a new line was NOT produced, so the next step is to find out why not!!!!
								inputCharType='W';
								inputChar=M_NEWLINE_CHARACTER; // MDH@13OCT2020 replacing: '\\';
								switch(aValidCommandIndicator){
									case   0:inputInfo("Invalid or empty command.");break;
									case  -1:{inputInfo("Erroneous command.");inputChar='\0';}break;
									case  -2:inputInfo("Value behind operator at end of command missing.");break;
									case  -3:inputInfo("Missing end of list.");break;
									case  -4:inputInfo("Missing end of function call.");break;
									case  -5:inputInfo("Missing end of map.");break;
									case  -6:inputInfo("Unknown expression with first token left unfinished.");break;
									case  -7:inputInfo("Function call missing at end of command.");break;
									case  -8:inputInfo("Unfinished function call.");break;
									case  -9:inputInfo("Unfinished list.");break;
									case -10:inputInfo("Unfinished string literal.");break;
									case -11:inputInfo("Unfinished expression.");break;
									case -12:inputInfo("Unfinished map.");break;
									default:inputInfo("Invalid command indicator %d.",aValidCommandIndicator);break;
								}
							//}
						}
					}
				}
				if(inputCharType=='n')
				break; // MDH@31OCT2019: changed to using ` as newline character which is considered whitespace so it's appended to the end of a token and ends that token as well, so that a token cannot be split over two lines (which would be ackward)
				/* replacing using it as a newline character:
				// MDH@30OCT2019: if not in command mode or (in command mode) we do not have an input command or it is not valid
				if(inputMode!=IM_COMMAND||!_userInputCommand||isAValidCommand(_userInputCommand,false))break;
				// ASSERT in command mode with an invalid (i.e. unfinished) command
				// ignore if at the start position on the line!!!
				if(!_userinputline||getUserInputLength()>_userinputline->offset){
					showContinuedPrompt();
					showSuggestedText(); // we have to rewrite the suggested text though
					outputTokenColor(_userInputCommand->_lastToken); // and show the right color
				}else // at start of user input line, do not allow having empty user input lines!!!
					beep();
				continue;
				*/
			}

			if(inputCharType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character

			// from now on no continue's anymore, because at the end of the loop we want to check for inputCharType equaling o
			if(inputMode==IM_COMMAND){
#ifdef __DEBUG__
				outputChar(inputCharType);
#endif
				// MDH@03SEP2019: any input character that somehow changes the command needs to ascertain that no previous command is being used (i.e. when commandIndex is not zero)
				//////////outputStatus(inputChar,inputCharType);
				// MDH@27OCT2020: Ctrl-G maps to inserting 'get()' as we expect people to use it a lot
				if(inputCharType=='g'){
					// TODO should we consume suggested text characters??????
					//	  of course it is also possible to insert these as suggested text??????
					if(string_length(_suggestedText)==0){
						char* inputChars=GET_CALL_CHARACTERS;
						while(*inputChars){
							inputChar=*inputChars;
							inputCharType=INPUTCHARACTERTYPES[inputChar];
							// MDH@01NOV2021: removing a suggested character failure cannot occur!!!
							if(commandCharacterAccepted(inputChar,&inputCharType,true,false)&NO_USER_INPUT_ERROR)break;
							inputChars++;
						}
					}else
						beep();
				}else
				if(inputCharType=='d'){ // MDH@18APR2019: delete now always deletes the first character in the behind cursor text
					// MDH@23JAN204: is this code actually ever called now, if so it should be replaced!!!
					char firstSuggestedCharacter=getFirstSuggestedCharacter();
					if(firstSuggestedCharacter){
						if(!removeFirstSuggestedCharacter(firstSuggestedCharacter))
							inputCharType=switchToControlMode("Failed to remove the first suggested character!");
					}else
						beep();
					/* replacing:
					clearScreenFromCursor(); // MDH@16OCT2020: this might help
					/////debugWrite("DELETE");
					// MDH@20SEP2019: equivalent to removing ANY first character in the first feed forward text (if any)
					// MDH@27SEP2019: removing the identifier continuation text takes precedence!!!
					// MDH@07OCT2019: manual feed forward now comes first (in showing)
					if(string_length(_manualFeedforwardText)){
						// remove one character at a time (TODO might be confusing with the removal of the entire identifier continuation on Delete)
						if(!getFirstManualFeedforwardCharacterRemoved())
							inputCharType=switchToControlMode("Failed to delete the first suggested character.");
					}else
					if(_identifierContinuationCharacters!=NULL){
						// MDH@30OCT2019: by blocking the subsequent update (the next time), we loose the identifier continuation automatically
						userInputCommandIdentifierContinuationNeedsUpdating=false; // ascertain to block or not block accordingly (i.e. if we're in an identifier don't block, either wise block)
						/// replacing: deleteIdentifierContinuation();
						///////writeSuggestedText(false);
					}else
					if(_firstTokenautocompletiontext!=NULL){ // replacing: string_length(feedforwardText)){ // something to delete
						if(firstAutoCompletionCharacterRemoved()=='\0')
							inputCharType=switchToControlMode("Failed to remove the first character in the suggested text.");
					}else // no first feed forward text (and character)!
						beep();
						*/
				}else
				if(inputCharType=='b'){ // backspace
					///////debugWrite("BACKSPACE");
					// something to remove?
					if(getUserInputLength()){ // TODO _userInputCommand->_firstToken should be NULL at the same time getCommandLength() becomes 0!!!
						// MDH@31JUL2024: if we're right behind an argument prompt string, we remove that
						if(atArgumentPrompt()){
							if(!argumentPromptRemoved())
								switchToControlMode("Failed to remove the previous argument prompt. Possible cause: out of memory.");
						}else // TODO should we only remove the argument prompt, or the previous token character as well?
						if(!removePreviousTokenCharacter())
							switchToControlMode("Failed to remove the previous token character. Possible cause: out of memory.");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='h'){
					if(_userInputCommand!=NULL)
						inputInfo("Press Enter to execute the command, Ctrl-C to clear the command, Ctrl-Z to exit M immediately.");
					else
						inputInfo("Use Ctrl-Z to exit M immediately, press the Enter key to switch to Control mode.");
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					// replacing: if(_userInputCommand->_firstToken!=NULL){clearCommand();break;}beep(); 
					if(NULL==_userInputCommand){
						inputInfo("Nothing to clear. (Use Ctrl-H for some help.)");
						beep();
					}else
					if(_userInputCommand->_firstToken){
						/////////if(amWrapping()())break; // if in amWrapping()() can't guarantee backspace() to move into the previous line which means just prompt again...
						// MDH@03SEP2019: what to do when Ctrl-C is called on a previous command????? i.e. when _userInputCommand->_firstToken points to a previous command, I'd say that we should return to the current command
						if(commandIndex==0){
							// MDH@20OCT2021 moved over to cancelCommand() as we probably need to do that every cancelCommand(): deleteUserInputCommand();
							cancelCommand();
						}else
							setCommandIndex(0);
					} else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// MDH@03SEP2019: here commandCharacterAccepted() will take care of copying the command if commandIndex is not (yet) zero
					// if there's a preview (well, code completion by way of a feedforwardText)
					// MDH@20SEP2019: I suppose we can simply get the characters from the (constructed) feedforwardText
					//				but that would cause problems in case something goes wrong
					// MDH@25SEP2019: the following goes wrong in those case where the feed forward text is generated, so it should not generate feed forward text until after all feed forward characters are consumed!!!!
					//				we solve this by always passing false for the endOfInput argument!!!
					/* replacing:
					if(_firstTokenautocompletiontext){
						char newInputChar;
						while((newInputChar==firstAutoCompletionCharacterRemoved())){
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],false,true)){
								beep();
								inputCharType=switchToControlMode("Failed to accept a suggested character.");
								break;
							}
						}
						// MDH@25SEP2019: we now need to generate the completion text again
						deleteTokenautocompletiontexts(); // APPARENTLY I need to do this to force writeSuggestedText(true) to reconstruct it (although I thought updateLastTokenAutoCompletionText() would call deleteTokenautocompletiontexts())
						updateLastTokenAutoCompletionText();
						writeSuggestedText(true); // TODO can we not find a better way to do this??????
					}else
						beep();
					*/
					// the original way of doing this (by computing the behind cursor text and consuming it)
					// MDH@26SEP2019: how about only consuming the identifier continuation text if we have it as a service to the user????? I think that makes sense doesn't it
					//				updateBehindCursorText() adjusted to return (if available) a pointer to the characters in the feed forward text (TODO change behind cursor into feed forward when appropriate)
					// MDH@04OCT2019: are we going to consume in parts????? NOTE if there's no identifier continuation text, the immediate feed forward and auto completion texts to consume are present in _suggestedText (thank god)
					// MDH@07OCT2019: manual feed forward text (as a whole) takes precedence
					
					// MDH@20JAN2024: we can now replace the original code by a series of calls to rightArrowProcessed()
					//                until the token changes, that way a Tab simply performs a number of successive right arrows
					//                and will therefore be consistently the same
					// MDH@18JUL2024: BUG when in an error, tabbing a closing quote will keep on adding it indefinitely
					//                FIX ascertain that the suggested character is NOT accepted, so only a single character
					//                    is accepted
					char newInputChar;
					Mtoken* token=NULL;
					do{
						// we should not remove a suggested character
						char suggestedCharacter=getFirstSuggestedCharacter();
						if(!suggestedCharacter)break; // there no longer is a suggested character
						// check whether or not the suggested character is acceptable
						if(token!=NULL&&!characterContinuesToken(token,suggestedCharacter,INPUTCHARACTERTYPES[suggestedCharacter]))
							break;
						newInputChar=getAcceptedSuggestedCharacter(suggestedCharacter,&inputCharType);
						if(!newInputChar)break; // the suggested character was not accepted
						if(token==NULL)token=_userInputCommand->_lastToken; // after accepting the first token
					}while(inputMode==IM_COMMAND);
					if(inputMode!=IM_COMMAND)beep(); // some error condition
					/* replacing:
					// MDH@17JAN2024: _suggestedText is replaced by suggestedTextSources, and getting the first suggested character
					//                is now replaced by getFirstSuggestedCharacter()
					//                code adapted so that each character is removed immediately once accepted which is opposite
					//                to the original code that deletes the umber of suggested characters consumed in one go					
					size_t numberOfSuggestedCharactersConsumed=0;
					if(suggestedTextSources[0]%5){ // there still is a suggested character (although suggestedTextSources[0] could be 5)
						char newInputChar='\0',newInputCharType='\0';
						int8_t characterAccepted;
						do{
							newInputChar=getFirstSuggestedCharacter();
							if(!newInputChar){
								inputCharType=switchToControlMode("Suggested characters vanished somehow.");
								break;
							}
							newInputCharType=INPUTCHARACTERTYPES[newInputChar];
							// TODO=DONE I have to think about the following
							// this is because as soon as the token ends we stop consuming suggested characters!!!
							if(numberOfSuggestedCharactersConsumed>0&&
									!characterContinuesToken(_userInputCommand->_lastToken,newInputChar,newInputCharType))
								break;
							characterAccepted=(newInputChar!='#'?commandCharacterAccepted(newInputChar,&newInputCharType,false,true):0);
							if(characterAccepted&NO_USER_INPUT_ERROR){ // the character was not accepted
								inputCharType=switchToControlMode("No user input.");
								break;
							}
							if(characterAccepted==0&&numberOfSuggestedCharactersConsumed>0){
								inputCharType=switchToControlMode("Not all suggested characters accepted.");
								break;
							}
							// TODO the first time characterAccepted can also be '\0' shouldn't we get switch to control mode then as well??
							if(!removeFirstSuggestedCharacter(newInputChar)){
								inputCharType=switchToControlMode("Failed to remove the appended suggested character.");
								break;
							}
							numberOfSuggestedCharactersConsumed++; // prepare to process the next character to consume (if any)
							if(newInputCharType==' ')newCommandLine(true); // MDH@24SEP2020 replacing (and improving upon): showContinuedPrompt(true,true); // MDH@31OCT2019: whenever a newline (request) character is consumed, make a new line
							// if suggestedTextSources[0] is now 5 there are no further suggested characters to consume!!!
						}while(suggestedTextSources[0]<5);
						if(inputMode==IM_COMMAND){ // all went well
							// if identifier continuation characters were consumed nothing to do, otherwise either to clear the manual
							// MDH@08OCT2019: have to be careful here because identifier continuation characters might come out of the manual feed forward text
							if(numberOfSuggestedCharactersConsumed>0)
								updateLastTokenAutoCompletionText(true); // TODO do we need the following????
								// we might end up behind some character that produces auto completion stuff like [ or {
							else
								inputCharType=switchToControlMode("No suggested characters accepted.");
						}
					}else{ // no suggested text to consume
						beep();
						//inputCharType=switchToControlMode("No suggested characters to consume!");
					}
					*/
					/* replacing:
					size_t numberOfCharactersToConsume=string_length(_suggestedText);
					if(numberOfCharactersToConsume>0){
						// TODO needs to be reviewed in that we should continue adding suggested characters as long as we are able to actually consume them
						// if there are manual feed forward characters it can start with a number of identifier continuation characters to consume as a whole using Tab
						size_t numberOfManualFeedforwardCharacters=(_manualFeedforwardText!=NULL?string_length(_manualFeedforwardText):0);
						size_t numberOfIdentifierContinuationCharacters=(numberOfManualFeedforwardCharacters?numberOfIdentifierContinuationManualFeedforwardCharacters:(_identifierContinuationCharacters!=NULL?strlen(_identifierContinuationCharacters):0));
						// the feed forward text that is colored differently (identifier continuation and manual feed forward text is to be accepted as a whole when Tab is used)
						// it's easiest to do that by specifying the number of suggested characters to consume
						if(numberOfIdentifierContinuationCharacters>0)numberOfCharactersToConsume=numberOfIdentifierContinuationCharacters;
						else
						if(numberOfManualFeedforwardCharacters>0)numberOfCharactersToConsume=numberOfManualFeedforwardCharacters;
						char newInputChar='\0',newInputCharType='\0';
						// MDH@21OCT2020: decided to make numberOfCharactersToConsume equal to 0 to indicate that not all characters were actually consumed
						// MDH@19OCT2020: is there a way somehow to consume characters until the token is finished or a new token starts??????????
						//				in essence at least a single character needs to be accepted
						size_t numberOfSuggestedCharactersConsumed=0;
						int8_t characterAccepted=0;
						while(numberOfSuggestedCharactersConsumed<numberOfCharactersToConsume){
							newInputChar=string_char(_suggestedText,numberOfSuggestedCharactersConsumed);
							if(newInputChar=='\0'){
								inputCharType=switchToControlMode("Suggested characters vanished somehow.");
								break;
							}
							/////////////inputInfo("Consuming: '%c'.",newInputChar);
							// MDH@24APR2019 obsolete: getCommandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							// MDH@14AUG2019: suggestedCharacter is set to true now, this makes perfect sense as I'm consuming all characters here and we do not want to remove them, NOTE that characters may still be inserted but only when bc=0 obviously
							newInputCharType=INPUTCHARACTERTYPES[newInputChar];
							// MDH@21OCT2020: OOPS the first character SHOULD always be accepted even if it finishes or starts a new token
							//				it's easiest to only set numberOfCharactersToConsume to suggestedCharacterIndex below when numberOfSuggestedCharactersConsumed is positive
							//				NOTE also when the character does not continue the token we simply break leaving numberOfCharactersToConsume positive
							//				TODO improvement possible by switching to control mode if anything goes wrong and not taking care of it 
							// MDH@19OCT2020: stop as soon as the character does not continue the current token
							if(numberOfSuggestedCharactersConsumed>0&&
									!characterContinuesToken(_userInputCommand->_lastToken,newInputChar,newInputCharType))
								break;
							// TODO shouldn't endOfInput equal true instead of false????
							// NOTE with endOfInput defined as false, true doesn't have any effect unless I change that!!!
							characterAccepted=(newInputChar!='#'?commandCharacterAccepted(newInputChar,&newInputCharType,false,true):0);
							if((characterAccepted&NO_USER_INPUT_ERROR)!=0) // the character was not accepted
								inputCharType=switchToControlMode("No user input.");
							/// MDH@02NOV2021: no longer used
							///else
							///if((characterAccepted&REMOVE_SUGGESTED_CHARACTER_FAILURE)!=0) // the character was not accepted
							///	inputCharType=switchToControlMode("Failed to accept a suggested character.");
							
							else
							if(characterAccepted==0&&numberOfSuggestedCharactersConsumed>0)
								inputCharType=switchToControlMode("Not all suggested characters accepted.");
							// if some error occurred we're no longer in command mode anymore
							if(inputMode!=IM_COMMAND)break;
							numberOfSuggestedCharactersConsumed++; // prepare to process the next character to consume (if any)
							if(newInputCharType==' ')newCommandLine(true); // MDH@24SEP2020 replacing (and improving upon): showContinuedPrompt(true,true); // MDH@31OCT2019: whenever a newline (request) character is consumed, make a new line
						}
						// remove at most numberOfSuggestedCharactersAccepted from the suggested text
						if(inputMode==IM_COMMAND){ // all went well
							// if identifier continuation characters were consumed nothing to do, otherwise either to clear the manual
							// MDH@08OCT2019: have to be careful here because identifier continuation characters might come out of the manual feed forward text
							if(numberOfSuggestedCharactersConsumed>0){ // at least one character consumed
								if(numberOfManualFeedforwardCharacters){ // some of the manual feed forward characters were consumed
									// MDH@05DEC2022: let's turn this around
									if(numberOfSuggestedCharactersConsumed>=string_length(_manualFeedforwardText)) // all manual feed forward characters were consumed
										deleteManualFeedforwardText();
									else // not all manual feed forward characters were consumed 
										// remove numberOfSuggestedCharactersAccepted from the start of the manual feed forward text
									if(!string_removed(_manualFeedforwardText,0,numberOfSuggestedCharactersConsumed))
										inputCharType=switchToControlMode("Not all accepted suggested characters removed from the suggested text.");
									/// replacing:
									///if(numberOfSuggestedCharactersConsumed>=string_length(_manualFeedforwardText)){ // all manual feed forward characters were consumed
									///	FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;
									///}else{ // not all manual feed forward characters were consumed 
									///	// remove numberOfSuggestedCharactersAccepted from the start of the manual feed forward text
									///	if(!string_removed(_manualFeedforwardText,0,numberOfSuggestedCharactersConsumed))
									///		inputCharType=switchToControlMode("Not all accepted suggested characters removed from the suggested text.");
									///}
									///
								}else
									deleteTokenautocompletiontexts();
								// MDH@21OCT2020: this is a very good point because apparently when the last consumed character is ( it's appending ) which is NOT always required
								//				I suppose that's technically only the case when there's no ) in the remainder of the suggested text left
								//				essentially the last token auto completion text could be present in the suggested text to start with
								// MDH@16NOV2021: this will ascertain to append new auto completion characters e.g. when ( or [ or { is tabbed
								updateLastTokenAutoCompletionText(true); // TODO do we need the following????
								// we might end up behind some character that produces auto completion stuff like [ or {
							}else
								inputCharType=switchToControlMode("No suggested characters accepted.");
						}
					}else{ // no consumable text
						beep();
						//inputCharType=switchToControlMode("No suggested characters to consume!");
					}
					*/
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharReadNonBlocking(&inputChar,NULL)){
						///printf("(%d)",inputChar);
						if(inputChar==91){
							if(inputCharReadNonBlocking(&inputChar,NULL)){//inputChar=getInputChar();
								///printf("(%d)",inputChar);
								if(inputChar==51){
									if(inputCharReadNonBlocking(&inputChar,NULL)){//inputChar=getInputChar();
										if(inputChar==126){ // delete
											// MDH@20JAN2024: deleting suggested text is allowed one character at a time
											//                but deleting identifier continuation is probably best done entirely
											if(suggestedTextSources[0]%5){
												// MDH@23JAN2024: when dealing with deleting identifier continuation, we delete the entire identifier continuation
												if(suggestedTextSources[suggestedTextSources[0]]==2){
													if(!removeIdentifierContinuationCharacters())
														inputCharType=switchToControlMode("Failed to delete the identifier continuation.");
												}else
												if(!removeFirstSuggestedCharacter(getFirstSuggestedCharacter()))
													inputCharType=switchToControlMode("Failed to remove the first suggested character.");
											}
											/* replacing:
											/////////inputInfo("Delete");
											// we should have suggested (identifier continuation or feed forward (autocompletion)) text
											// MDH@07OCT2019: manual feed forward text goes first
											if(string_length(_suggestedText)){ // there is suggested text with parts to delete
												clearScreenFromCursor(); // MDH@17OCT2020 bug fix: ascertaining not to keep seeing the last suggested character
												// any identifier continuation characters precede manual feed forward text
												if(string_length(_manualFeedforwardText)){
													// MDH@05DEC2022: moving deleting manualFeedforwardText to getFirstMgetFirstManualFeedforwardCharacterRemoved
													if(!getFirstManualFeedforwardCharacterRemoved())
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
													/// replacing:
													///if(getFirstManualFeedforwardCharacterRemoved()){
													///	if(string_length(_manualFeedforwardText)==0)deleteManualFeedforwardText();
													///}else
													///	inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else
												if(_identifierContinuationCharacters!=NULL&&strlen(_identifierContinuationCharacters)){
													// if there's immediate feed forward token, there's no manual feed forward
													// PROBLEM can't remove the identifier continuation characters as a whole because if I do it will be recreated
													if(!prependedToManualFeedforwardText(_identifierContinuationCharacters+1))
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else{
													// MDH@06OCT2019: as long as the user is manually deleting identifier continuation characters
													//				we prevent the automatic update of the identifier continuation?????
													// MDH@06OCT2019: it makes sense to let Delete consume only a single character, otherwise the user might get confused
													//				because sometimes only one character is consumed, and sometimes more
													//				so to keep it simple we consume a single character at a time
													//				BUT if there are characters left we move these into the manual feed forward
													//				that way no identifier continuation will show until the user consumed the entire manual feed forward
													//				this makes sense because the identifier continuation itself won't change while using Delete only
													//				HOWEVER what should happen if the user types another character thus changing what's in front of the
													//				manual feed forward?????
													if(string_length(_immediateFeedforwardText)==0){ // MDH@20SEP2019 replacing: string_length(feedforwardText)){
														if(firstAutoCompletionCharacterRemoved()=='\0') // MDH@20SEP2019 replacing: string_removed_char(feedforwardText,0))
															inputCharType=switchToControlMode("Failed to delete the first character of the suggested text.");	
													}else{ // remove the immediate feed forward text
														string_setlength(_immediateFeedforwardText,0);
														/////////////immediateFeedforwardToBeUpdated=false; // do not update next time
													}
												}
											}else
												beep();
											*/
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(commandIndex==0&&_userInputCommand!=NULL)
											inputError("%s","Won't show previous commands when one is being entered.");
										else
										if(!commandDown())
											beep();
									}else
									if(!commandPage)
										showPreviousCommandPage();
									/*
									else
									if(!commandDown())
										outputInfo("%s","No previous command!");
									*/
								}else
								if(inputChar==66){ // down arrow
									if(inputMode==IM_COMMAND){
										if(commandIndex==0&&_userInputCommand!=NULL)
											inputError("%s","Won't show next commands when one is being entered!");
										else 
										if(!commandUp())
											beep();
									}else
									if(!commandPage)
										showNextCommandPage();
									/*
									else
									if(!commandUp())
										outputInfo("%s","No next command!");
									*/
								}else
								if(inputChar==67){ // right arrow
									// MDH@24JAN2024: if we delegate to a function we can implement Tab by calling right arrow
									//                a number of times
									//                NOTE getAcceptedSuggestedCharacter() is allowed to change inputCharType
									getAcceptedSuggestedCharacter('\0',&inputCharType); // not interested in the result
									if(inputMode!=IM_COMMAND)beep();
								}else
								if(inputChar==68){ // left arrow
									if(getUserInputLength()){
										if(atArgumentPrompt()){
											if(!argumentPromptRemoved())
												switchToControlMode("Failed to remove the argument prompt!");
										}else{ // regular token character removal
											// TODO apparently _userInputCommand->_firstToken will still be NULL when we're scrolling through the list of previous commands...
											// MDH@03SEP2019: BUG FIX forgot to make commandIndex 0 when copying the command (as copyUserInputCommand() itself does not seem to do that!!!)
											if(commandIndex){commandIndex=0;copyUserInputCommand();} // will also set getCommandLength()!!!
											// MDH@27FEB2019: we should remove the last character of the current token (and command) and move it into feedforwardText
											// MDH@20SEP2019: we do NOT want the character removed to disappear when the token it came from disappears, therefore the addition to the feed forward text should be anonymous
											// MDH@25SEP2019: because we're going to prepend c to the feed forward text, we have to determine the associated token i.e. the token that generated c
											//				i.e. if the character being moved would have been auto-generated we want to know the associated token
											//				the problem now is that removedTokenCharacter() will already remove the token when its a single-character token
											//				TODO something is not correct because if removedTokenCharacter() takes care of removing the token which check again below????????
											//				it's easiest to check whether the token will be removed: this will be the case if there's only one character in the token
											//				in which case that will be the originating token but only in the situation where c matches the feed forward character of that expression
											///////Mtoken* startOfExpressionToken=(string_length(_userInputCommand->_lastToken->text)==1?_userInputCommand->_lastToken->expr:NULL); // MDH@25SEP2019: remember what the start of expression token associated with the current last command token is
											char c=removedTokenCharacter(true); // passing true will force removedTokenCharacter() to actually check an identifier token type
											// MDH@19OCT2020: now, if we consume a nonvisible character (like what we currently use for newline character (.i.e. '\n' instead of '\\' we did before we actually consume it))
											if(c>=32&&c!=127){ // removing the character behind the cursor succeeded
												// debugWrite("Character '%c' removed.",c);
												// MDH@01OCT2019 removed as now done by removedTokenCharacter(): moveCursorLeft(1); // MDH@01OCT2019: TODO will this be sufficient when the identifier token type changed????? probably
												/* MDH@01OCT2019: no need to perform a tokenChecked... here anymore as we moved that functionality over to removedTokenCharacter(true)
												// TODO is the following necessary at all? given that removedTokenCharacter should/could have done so??????
												// TODO doing the following is better moved over to removedTokenCharacter() because if 
												if(string_length(_userInputCommand->_lastToken->text)>0)
													tokenCheckedForBeingAFunction(true);
												*/
												/*
												else // nothing left in current token // MDH@23SEP2019: it seems better to call removeLastUserInputCommandToken() here as removeLastUserInputCommandToken() will also remove the token's feed forward text
													removeLastUserInputCommandToken();
												*/
												// MDH@30SEP2019: this is the only call to getAutoCompletionTextOfCharacterPrepended() therefore we can simply adjust that function to check whether this character matches a consumed character!!!
												//				but I guess failing to do so is not that terrible that we should switch to control mode
												// MDH@01OCT2019: if this character also starts the new identifier continuation, it should not be anonymously prepended 
												//				let's construct the identifier continuation text that we would get 
												//				CORRECTION checking against the first identifier continuation text suffices (instead of comparing with the entire new identifier continuation)
												// MDH@04OCT2019: there was so much going wrong doing the following that I simply replaced it by prepending the consumed character to the feed forward texts and leaving it
												//				to the input loop part to deal with identifier continuations that match the start of the feed forward text(s)
												// MDH@07OCT2019: I've introduced a new feed forward element (manualFeedforwardText) to contain the part of the text
												//				that the user took out of the (tokenized) command to e.g. correct a command
												if(!manualFeedforwardCharacterPrepended(c)) // replacing: if(!getAutoCompletionTextOfCharacterPrepended(c,true))
													inputCharType=switchToControlMode("Failed to accept the removed command character as suggested text.");
												/* this would be else when we succeeded!!!!!!
												else
												if(amVerbose())
												inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
												*/
												/* replacing:
												updateUserInputCommandIdentifierContinuation();
												///outputChar('1');
												// prepend only anonymously when not matching the identifier continuation character!!
												if(!_identifierContinuationCharacters||_identifierContinuationCharacters[0]!=c)
												if(!getAutoCompletionTextOfCharacterPrepended(c,false))
												inputError("Failed to accept the removed command character as suggested text.");
												///outputChar('2');
												updateLastTokenAutoCompletionText(); // if we haven't updated the identifier continuation text do it again	
												///outputChar('3');
												*/
												////////writeSuggestedText(false);
												/* replacing:
												bool removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter=false;
												Mstring* identifierContinuationText=NULL;
												if(_userInputCommand->_lastToken->type==TT_FUNCTION||_userInputCommand->_lastToken->type==TT_VARIABLE||_userInputCommand->_lastToken->type==TT_NEW_VARIABLE){
													identifierContinuationText=__string();
													if(identifierContinuationText){
														string_append_char(identifierContinuationText,c);
														if(_identifierContinuationCharacters)string_append(identifierContinuationText,_identifierContinuationCharacters);
														updateUserInputCommandIdentifierContinuation(); // determine the new identifier continuation
														if(_identifierContinuationCharacters&&strcmp(_identifierContinuationCharacters,string(identifierContinuationText))==0)
															removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter=true;
													}
												}
												if(!removedTokenCharacterMatchesFirstNewIdentifierContinuationTextCharacter)
												if(!getAutoCompletionTextOfCharacterPrepended(c))
												inputError("Failed to accept the removed command character as suggested text.");
												updateLastTokenAutoCompletionText(identifierContinuationText==NULL); // if we haven't updated the identifier continuation text do it again	
												writeSuggestedText(false);
												if(identifierContinuationText)FREE_STRING(identifierContinuationText);
												*/
											}else
											if(!c)
												inputCharType=switchToControlMode("Failed to remove the last command character.");
										}
									}else
										beep();
								}else
									beep();
							}
						}
					}
				}else
				if(inputChar){
					logToOutputFile("Processing normal input character '%c'(%d).\n",inputChar,inputChar);
					// MDH@23JAN2024: _suggestedText is not created anymore, is to be replaced by getFirstSuggestedCharacter()
					// MDH@21APR2019: creating a command if need be is delegated to commandCharacterAccepted() which we know
					//				we always need a command (being edited)
					// MDH@01OCT2019: if the input character matches the first non-anonymous i.e. autogenerated feed forward character same functionality as right arrow (except now we know the character entered)
					char firstSuggestedCharacter=getFirstSuggestedCharacter(); // replacing: string_char(_suggestedText,0); // MDH@05DEC2022 replacing: getFirstSuggestedCharacter();
					if(inputChar==firstSuggestedCharacter){
						//if(amVerboseDebugging())
							logToOutputFile("\t\tInput character '%c' matches the first suggested character.\n",inputChar);
						
						//inputInfo("Consuming the first suggested character!");
						///// no need for this!!! char firstSuggestedCharacterInputType=INPUTCHARACTERTYPES[firstSuggestedCharacter];
						bool firstSuggestedCharacterConsumed=removeFirstSuggestedCharacter(inputChar);
						// TODO perhaps we should do more?????? DONE let's not accept this happening
						if(firstSuggestedCharacterConsumed){
							int8_t result=commandCharacterAccepted(inputChar,&inputCharType,true,firstSuggestedCharacterConsumed);
							if(result&NO_USER_INPUT_ERROR)
								inputError("No user input!");
						}else{
							////beep();
							inputError("%s%sFailed to consume the first suggested character '%c'.",M_BUG_PREFIX,M_MESSAGE_PREFIX,inputChar);
							switchToControlMode("See the log for details.");
						}
						/// MDH@02NOV2021: can't happen anymore	
						///if(result&REMOVE_SUGGESTED_CHARACTER_FAILURE){
						///	inputError("Failed to remove the first accepted character!");
						///// replacing:
						///// char firstSuggestedCharacterConsumed=getFirstSuggestedCharacterConsumed(inputChar,true);
						/////if(!firstSuggestedCharacterConsumed)
						///	//inputCharType=switchToControlMode("Failed to accept the matching first suggested character.");
						///}
						// MDH@22OCT2021: no need to do the following anymore because commandCharacterAccepted() takes care of removing the consumed suggested character: 
						// else deleteFirstSuggestedCharacter(firstSuggestedCharacterConsumed,false);
					}else{
						//if(amVerboseDebugging())
							logToOutputFile("\t\tInput character '%c' does not match the first suggested character '%c'.\n",inputChar,(firstSuggestedCharacter?firstSuggestedCharacter:'-'));
						// MDH@31OCT2019: when the accepted character is the backslash in whitespace we should continue with the command on the next line
						//				OOPS a backtick inside a string is also recognized as such which shouldn't happen
						//				ALSO because the backtick will be visible it's probably better to insert an empty token for it of type TT_NEWLINE or something like that
						//				it's probably best to check whether to accept a backtick here???? NOTE we could have backticks in commands read from files as well????
						if((commandCharacterAccepted(inputChar,&inputCharType,true,false)&NO_USER_INPUT_ERROR)==0){
							// outputChar('X');
							if(inputCharType==' '){ // a newline request (whenever M_NEWLINE_CHARACTER is input at a functional position)
								newCommandLine(true); // MDH@24SEP2020 replacing and improving upon: showContinuedPrompt(true,true);
								//////////showSuggestedText(); // we have to rewrite the suggested text though
								///////////outputTokenColor(_userInputCommand->_lastToken); // and show the right color
							}
						}else
						if(inputCharType!='`') // not a new line request character MDH@16APR2024: ` is now the same as an L although that makes it usable anywhere in a variable name
							inputCharType=switchToControlMode(_userInputCommand->_firstToken?"Failed to accept the character.":"Failed to create a new command.");
						else
							inputError("New line request character not allowed here!");
					}
					/*
					// we need to have a token (to append the input character to) which initializes to _userInputCommand->_firstToken
					if(_userInputCommand->_firstToken==NULL) // no first command token
						// if commandIndex we should one of the registered commands
						setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set getCommandLength()!!!
					// if still NULL (also when we fail to actually create a new first command token)
					if(_userInputCommand->_lastToken!=NULL){
						if(!commandCharacterAccepted(inputChar,inputCharType,true))
							switchToControlMode("Failed to accept the character.");
					}else
						switchToControlMode("Failed to create a new command!");
					*/
				}else	// MDH@0.1.7.14+28JUN2023: I've added this so that any command currently considered an error would simply report that we're in an error and ignore the input
					beep(); // MDH@08JUL2023: TODO I wanted to do something here!!!!
				///////////// MDH@06AUG2019 NOT AGAIN: outputStatus(inputChar,inputCharType);
			}else
			if(inputMode==IM_CONTROL){ // inputChar received in control mode
				outputChar(inputChar); // nice to see the character we typed...
				newline();
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					signed char sessionSettingApplied=getSessionSettingApplied(inputChar); // try to process myself
					// we can get 'n' or 'x' responses
					if(sessionSettingApplied<0){
						if(!settingApplied(inputChar))
							outputMessage(M_ERROR_PREFIX,"Setting character '%c' not recognized.\n",inputChar);
					}else
					if(sessionSettingApplied>0)inputCharType=sessionSettingApplied;
					break;
				}else
					// user might have selected one of the commands (letter a through j)
					commandPage=0; // stop paging
			}else{ // Shell command input mode
				// we still allow using certain 'special' characters for composing the command (much like we did with a command)
				if(inputCharType=='b'){ // backspace
					uint16_t cp=getUserInputLength();
					if(cp){
						if(string_removed_char(_shellCommand,cp-1)){
							moveCursorLeft(1);
							////////writeSuggestedText(true);
						}else
							inputCharType=switchToControlMode("Failed to remove the shell command character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='d'){
					/* MDH@21JAN2024: _suggestedText replaced by suggestedTextSources[]
					if(string_length(_suggestedText)){ // something behind the cursor that we can remove
						if(string_removed_char(_suggestedText,0))
							///////writeSuggestedText(true)
							;
						else
							inputCharType=switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
					*/
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					if(_userInputCommand->_firstToken!=NULL){
						backToPrompt();
						clear_shellCommand();
						//////////if(amWrapping()())break;
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a feedforwardText)
					// here we have a serious problem in that we now have identifier continuation text, immediate feed forward text and permanent feed forward texts
					/* MDH@21JAN2024: _suggestedTextReplaced by suggestedTextSources[]
					uint16_t bc=getTotalNumberOfSuggestedCharacters();
					if(bc){
						while(bc--){
							char newInputChar=string_removed_char(_suggestedText,0);
							if(newInputChar=='\0'){
								///////writeSuggestedText(true);
								inputCharType=switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							if(newInputChar!='#')string_append_char(_shellCommand,newInputChar);
						}
					}else
						beep();
					*/
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharReadNonBlocking(&inputChar,NULL)){////inputChar=getInputChar();
						if(inputChar==91){
							if(inputCharReadNonBlocking(&inputChar,NULL)){/////inputChar=getInputChar();
								if(inputChar==51){
									if(inputCharReadNonBlocking(&inputChar,NULL)){///////inputChar=getInputChar();
										if(inputChar==126){ // delete
											/* MDH@21JAN2024: _suggestedText replaced by suggestedTextSources[]
											// TODO FIX this does not seem to be right!!!!!
											if(getTotalNumberOfSuggestedCharacters()){
												// we could go one to the right and do a backspace!!
												moveCursorRight(1);
												// TODO what to do here??? removePreviousTokenCharacter();
											}else // nothing under the cursor to delete
												beep();
											*/
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									beep();
								}else
								if(inputChar==66){ // down arrow
									beep();
								}else
								if(inputChar==67){ // right arrow
									/* MDH@21JAN2024: _suggestedText replaced by suggestedTextSources[]
									if(getTotalNumberOfSuggestedCharacters()){
										char newInputChar=string_removed_char(_suggestedText,0);
										if(!newInputChar){
											////////writeSuggestedText(true);
											inputCharType=switchToControlMode("Failed to accept the suggested characters.");
										}else
											string_insert_char(_shellCommand,getUserInputLength(),newInputChar);
									}else
										beep();
									*/
								}else
								if(inputChar==68){ // left arrow
									uint16_t cp=getUserInputLength();
									if(cp){
										bool success=false;
										char c=string_removed_char(_shellCommand,cp-1);
										if(c){ // removing the character behind the cursor succeeded
											// prefix it to _suggestedText
											string_insert_char(_suggestedText,0,c);
											moveCursorLeft(1);
											///////writeSuggestedText(true);
										}else
											inputCharType=switchToControlMode("Failed to move the cursor left.");
									}else
										beep();
								}else
									beep();
							}
						}
					}
				}else{
					string_append_char(_shellCommand,inputChar);
					// outputChar('<');
					outputChar(inputChar);
					// outputChar('>');
					// MDH@24APR2019: getUserInputLength()++;
				}
			}
			// if switched to control mode, inputCharType will be equal to 'o' and we break out of this input loop!!!
			if(inputCharType=='o'||inputCharType=='s')break;
		} // end of character input 

		// MDH@07OCT2019: there are several places where we might get rid of any manual feed forward text still pending
		//				but this seems to be a very good place for it
		//				_manualFeedforwardText is only used in command and shell mode, not in control mode
		//				perhaps it's better done at the prompt
		//				TODO consider doing the same for feed forward texts?????
		if(_manualFeedforwardText!=NULL)deleteManualFeedforwardText(); //{FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;}

		// if eXit input character(s) received...
		if(inputCharType=='x'){
			// we should only exit M when not entering a function body
			if(getCurrentFunctionBodyInput()){
				// switch back to command mode
				if(endFunctionBodyInput())
					output("Function body ended!\n");
				else
					outputError("Failed to end the function body");
				switchToCommandMode(); // NOTE as apparently we're in control mode!!
			}else
			if(blockCommandLevel>0){ // user decides to end a block
				if(!endSubcommandBlock(true))
					outputError("Failed to end a subcommand block");
				switchToCommandMode();
			}else
				break; // break out of user input loop, effectively ending M
		}else
		// MDH@16APR2019: now if we use n to switch modes as well, we can do that if there's no command
		if(inputCharType=='n'){
			if(inputMode==IM_COMMAND){
				// MDH@21JUL2019: if the last token appears to be a function identifier change it to a variable
				//				so we won't end up with refusal of evaluation
				if(_userInputCommand!=NULL&&
						_userInputCommand->_lastToken!=NULL&&
						_userInputCommand->_lastToken->type==TT_FUNCTION){
					changeFunctionTokenToAVariable(_userInputCommand,false); // MDH@28FEB2020: command now passed in to the function call as well (see Mshell.c)
					inputInfo("%s","Assumed function apparently a variable!");
				}else
					inputInfo("%s",""); // so that line will be empty
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				clearScreenFromCursor(); // so we won't see the behind cursor text anymore
			}
			outputChar('\n');

			if(inputMode==IM_COMMAND){ // the newline character ends the command to be evaluated!!
				string_setlength(_expectedCharacterStack,0); // MDH@13DEC2023: is there a better place to do this?????
				// if _userInputCommand->_firstToken is set, we have a command to evaluate
				/*
				Mtoken* _userInputCommand->_firstTokenToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(_userInputCommand->_firstToken){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(getUserInputLength()>string_length(_userInputCommand->_firstToken->text)){
						// finish the last token???
						if(_userInputCommand->_lastToken->significantCharacterCount==0)_userInputCommand->_lastToken->significantCharacterCount=string_length(_userInputCommand->_lastToken->text);
						_userInputCommand->_firstTokenToEvaluate=_userInputCommand->_firstToken; // but only when not at start of command!!!
						if(amVerboseDebugging())outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&getUserInputLength()) // NOTE using getUserInputLength() is better than using amAcceptinghistorycommand() (causing it!!)
					_userInputCommand->_firstTokenToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(_userInputCommand!=NULL&&_userInputCommand->_firstToken!=NULL){ // technically something to process (not necessarily evaluate!)
					
					if(amVerboseDebugging())
						outputCommandInfo(_userInputCommand); // now defined in Mshell.c/h

					// MDH@18MAR2024: if this user input command is a block command we should collect it in the current environment, and NOT execute it
					//                we can determine if a user input command is a block command by keeping a global variable called blockCommandLevel that keeps track of the number of active blocks
					//                however, if a block command represents the declaration of local variables it should be executed in the block design environment
					//                NOTE that any user input command should be allowed to start/end a new environment whether or not already collectingBlockCommands
					//                for a user input command to start/end a block it's first token should be a block keyword but with the right number of arguments???

					// MDH@26MAR2024: we should find the successive placeholder tokens, if any, NOTE there could be multiple
					// MDH@06APR2024: determine if this is an incomplete (and therefore to be completed) command
					Mtoken* firstPlaceholderToken=_userInputCommand->_firstToken;
					while(firstPlaceholderToken!=NULL&&firstPlaceholderToken->type!=TT_PLACEHOLDER)
						firstPlaceholderToken=firstPlaceholderToken->next;
					/* replacing:
					// that there could be multiple placeholders might be problematic
					Mtoken* token=getExecutionEnvironment()->continuationToken; // the continuation token is the token to search for the next placeholder token from
					int8_t blockKeywordId=-1; // the keyword id of the last function call
					if(token==NULL)token=_userInputCommand->_firstToken->next;
					while(token!=NULL&&token->type!=TT_PLACEHOLDER){
						if(token->type==TT_FUNCTION_CALL){
							char* functionTokenCharacters=_getSignificantTokenCharacters(token->prev);
							if(functionTokenCharacters!=NULL&&strlen(functionTokenCharacters)>0)blockKeywordId=getBlockKeywordId(functionTokenCharacters);
						}else
						if(token->type==TT_END_OF_FUNCTION_CALL)
							blockKeywordId=-1;
						token=token->next;
					}
					*/
					if(firstPlaceholderToken!=NULL){ // any placeholder starts a new environment in which commands are to be entered
						// MDH@08APR2024 moved over to startBlock: int8_t firstBlockKeywordId=getPlaceholderBlockKeywordId(firstPlaceholderToken);
						/*
						getExecutionEnvironment()->placeholderToken=token;
						getExecutionEnvironment()->continuationToken=token->next; // NOTE will NOT be NULL as a command cannot end with a placeholder token!!!!
						getExecutionEnvironment()->insertToken=token->prev; // this is the token where to connect the block command to
						*/
						// can we find the block keyword id?????? don't think we actually need it, hmmm although we need to know if it's a function that will create an environment!!!!!
						if(startBlock(_userInputCommand,firstPlaceholderToken,Msubowner(owner_userInputCommand,1))){
							output("Subcommand block activated!\n"); ///DEBUGGING
							// get rid of the placeholder token
							firstPlaceholderToken->next=NULL;FREE_TOKEN(firstPlaceholderToken,owner_userInputCommand);
							/*
							// determine how many commands can be embedded: if the placeholder token is followed by a comma (,) only a single command is allowed (and no end() to end the block is required)
							getExecutionEnvironment()->multipleCommandsAllowed=(token->next!=NULL&&token->next->type!=TT_LISTELEMENT);
							// set the first incomplete command token of the parent of the current execution environment to the first token of the user input command
							getExecutionEnvironment()->_parent->value._environment->firstIncompleteCommandToken=_userInputCommand->_firstToken; // remember the start of the command we need to exeute
							*/
							_userInputCommand=NULL; // MDH@06APR2024: _userInputCommand was embedded in the parent environment as incompleteCommand
							blockCommandLevel++;
						}else // this is a serious problem so let's switch to control mode (TODO allow garbage collection in control mode or some memory allocation test to check wether out of memory)
							switchToControlMode("Failed to start requesting subcommands");
					}
					/* replacing:
					Mtoken* secondToken=_userInputCommand->_firstToken->next;
					
					if(secondToken!=NULL&&secondToken->type==TT_FUNCTION_CALL){
						char* secondTokenCharacters=_getSignificantTokenCharacters(secondToken);
						if(secondTokenCharacters!=NULL&&strlen(secondTokenCharacters)>0){
							output("Second token: '%s'.\n",secondTokenCharacters);
							int8_t blockKeywordId=getBlockKeywordId(secondTokenCharacters);
							Mtoken* completedBlockCommandToken=NULL; // the first token of the completed command
							if(blockKeywordId>=0){ // a block keyword either starts and/or ends a block
								Menvironment* blockEnvironment=NULL;
								// a block is always ended before a new block is started
								if(BLOCK_FLAGS[blockKeywordId]&4){ // end all blocks
									// end all blocks (the M environment does not have a parent so it's endBlock will return false)
									while(blockCommandLevel){
										blockEnvironment=endBlock();
										if(NULL==blockEnvironment)break;
										completedBlockCommandToken=blockEnvironment->blockCommandList->_first->_value->value._token;
										blockEnvironment->blockCommandList=NULL; 
										FREE_ENVIRONMENT(blockEnvironment,getValueDataOwner());
										blockCommandLevel--;
									}
								}else
								if(BLOCK_FLAGS[blockKeywordId]&2){ // ends a block
									blockEnvironment=endBlock();
									if(blockEnvironment!=NULL){
										completedBlockCommandToken=blockEnvironment->blockCommandList->_first->_value->value._token;
										blockEnvironment->blockCommandList=NULL; 
										FREE_ENVIRONMENT(blockEnvironment,getValueDataOwner());
										blockCommandLevel--;
										output("Block of commands ended!\n");
									}else
										outputError("Failed to end the block of commands");
								}
								if(BLOCK_FLAGS[blockKeywordId]&1){ // starts a block
									// MDH@25MAR2024: we should determine the insert token to be initialized in the block environment
									//                we should find the closing parenthesis of this function call
									//                but we also need to ascertain that function call is incomplete!!!
									Mtoken* insertToken=secondToken->next; // now on the opening parenthesis of the function call
									while(insertToken!=NULL&&insertToken->next!=NULL&&insertToken->next->type!=TT_END_OF_FUNCTION_CALL){
										insertToken=insertToken->next;
									}
									if(insertToken!=NULL){
										if(startBlock(blockKeywordId,_userInputCommand,insertToken,Msubowner(owner_userInputCommand,1))){
											blockCommandLevel++;
											output("Block of commands started.\n");
										}else
											outputError("Failed to start a new block of commands!");
									}
								}else{ 
									// as soon as we're done with all the blocks, we should execute all block commands
									if(blockCommandLevel==0){
										// NOTE: _userInputCommand should NOW be completed, and ready to be executed, so no need to extract it now!!!!
										//       unless we know that the completed block command field has actually been NULLed in the finished environment
										_userInputCommand->_firstToken->next=completedBlockCommandToken;
										///replacing:
										///if(!executeBlockCommands()){
									 	///		outputError("Failed to execute the block commands.");
										///	blockCommandLevel=-2;
										///}else
										///	blockCommandLevel=-1;
									}
								}
							}
						}
					}*/
					else // the user input command is NOT incomplete
					if(blockCommandLevel>0){ // (still) inside a block
						output("Embedding the subcommand.\n");
						// does this user input command designate an end of block?????
						// MDH@08APR2024: if we always also add the end() command to the current block we can still use it by executing end() [and know it was a placeholder command!!!!!!]
						if(addBlockCommand(_userInputCommand)){
							output("Subcommand embedded.\n");
							// MDH@08APR2024: get rid of the current user input command (_userInputCommand can be set though when we returned to block command level 0)
							// check if this command actually ends the current block!!!
							Mblock* block=getCurrentBlock();
							bool endOfBlock=(block->subcommandBlockType=='1');
							/* MDH@15APR2024: only a single command block can be ended immediately here!!
							if(numberOfBlocksToEnd==0){ // not an implicit end of block
								Mtoken* endFunctionToken=_userInputCommand->_firstToken;
								while(endFunctionToken!=NULL){
									// TODO any function call could be an embedded function call which we would not want so we might want to check whether any end() or endall() call would actually be executed as it could be conditional
									if(endFunctionToken->type==TT_FUNCTION){
										char* _functionName=_getSignificantTokenCharacters(endFunctionToken);
										if(!strcmp(_functionName,"endall")){
											numberOfBlocksToEnd=blockCommandLevel;
										}else
										if(!strcmp(_functionName,"end"))
											numberOfBlocksToEnd=1;
										free(_functionName);
										if(numberOfBlocksToEnd)break;
									}
									endFunctionToken=endFunctionToken->next;
								}
							}
							*/
							/*
							// we can register this command in the current execution environment in which case it is bound!!!
							if(!registerCommand(_userInputCommand,owner_userInputCommand)){
								outputError("Failed to register the embedded command");
								// can't do this because _userInputCommand as a whole is embedded NO not the first token
								_userInputCommand->_firstToken->next=NULL;
								FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL;
								output("Embedded command freed!\n");
							}
							*/
							_userInputCommand->_firstToken->next=NULL;
							FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL;
							output("Embedded command freed!\n");
							// end any block we're supposed to end
							if(!endOfBlock){ // no block to end, so we should
								// block not ended yet, so we're going to have another command to embed
								// so we need to insert a command separator
								// environment->insertToken points to the last inserted token
								output("Embedding a command separator.\n");
								Mtoken* nextInsertToken=block->insertToken->next;
								Mtoken* listelementToken=owned_token(_getNewCommandToken(block->insertToken,TT_LISTELEMENT,false),Msubowner(owner_userInputCommand,1));
								if(listelementToken!=NULL){
									string_append_char(listelementToken->text,',');
									listelementToken->significantCharacterCount=1;
									block->insertToken=listelementToken;
									// and link to the next insert token
									listelementToken->next=nextInsertToken;
									nextInsertToken->prev=listelementToken;
									// should I link it forward just in case the user decides to end the list of subcommands??????
								}else
									switchToControlMode("Failed to embed a command separator.");
							}else{
								if(!endSubcommandBlock(false))
									switchToControlMode("Failed to end a subcommand block!");
							}
						}
						/* replacing:
						// two ways to end a block: explicit by end() or implicit()
						////// replacing: bool endOfBlock=(getExecutionEnvironment()->blockKeywordId>=0&&BLOCK_FLAGS[getExecutionEnvironment()->blockKeywordId]&2);
						if(!endOfBlock){ // doesn't end a block
							// add the user input command to the list of environment block commands
							if(addBlockCommand(_userInputCommand)){
								//if(amVerboseDebugging())
									output("Block command added!\n");
								FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL;
								// if the execution environment does not allow multiple commands we're done and the block should be closed
								if(!)endOfBlock=true;
							}else // that's a rather serious error
								switchToControlMode("Failed to register block command!");
						}else{
							FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL;
						}
						*/
					}//else

					// TODO now checking for _userInputCommand being NULL but used to be blockCommandLevel==0
					if(inputMode==IM_COMMAND&&_userInputCommand!=NULL){ // a directly executable user input command
						// MDH@15APR2024: code to evaluate _userInputCommand moved over to evaluateUserInputCommand()
						////DEBUGGING output("Command to evaluate: ");outputToken(_userInputCommand->_firstToken,NULL);outputLine(".");
						// MDH@11MAY2020 obsolete: size_t mark=allocationmark();if(amVerbose())output("Mark: %zu.\n",mark);
						if(!userInputCommandEvaluated())
							outputError("Failed to evaluate the user input command");
					}
				}else
					// MDH@14AUG2019: if a user presses Enter when there's no command but still feedforwardText it looses feedforwardText but we do switch to the control mode as I think that is what the user wants (if only to look at the list of variables)
					//				NOTE that I might consider keeping feedforwardText, so it will be redisplayed when the user returns to the command mode
					switchToControlMode(NULL); // replacing: if(getTotalNumberOfSuggestedCharacters()==0)switchToControlMode(NULL);else outputError("Still suggested text");
			}else
			if(inputMode==IM_SHELL){
				if(string_length(_shellCommand))
					executeShellCommand();
				else // MDH@16APR2019: back to command mode
					switchToCommandMode();
			}else // Return key in control mode, always to return to command input!!
				switchToCommandMode();
		}
		showSeparatorLine();
		if(!allocationMarkAdded())
			outputError("Failed to add a new memory allocation mark");
	}
	// 'normal' exit
	exit(0);
}
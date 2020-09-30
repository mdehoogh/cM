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

// MDH@27FEB2020: on top of environment management we have the 'shell' for setting up the root M environment
#include "Msession.h"

static uint32_t const MODULE_ID=0; // the 'main' module always has module id 0
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}

// the constants are defined in Mshell.c
extern char const* const M_ERROR_PREFIX;
extern const char* const INFO_PREFIX; // MDH@27FEB2020: as for now NO actual info prefix text to use
extern const char* const M_WARNING_PREFIX; // used in Mexecution.c as well (defined there as extern!!!)
extern const char* const M_BUG_PREFIX; // MDH@05NOV2019: for reporting bugs
extern const char M_WHITESPACE_CHARACTER; // MDH@31OCT2019: let's use another character for storing whitespace in tokens (would normally be a blank)
extern const char M_NEWLINE_CHARACTER; // MDH@31OCT2019: the character to request a newline with!!!
extern const char* const M_VARIABLE_NAME; // MDH@14NOV2019: the variable to hold the list of remembered commands and the results they evaluated to
extern const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES];
extern const char INPUTCHARACTERTYPES[];
extern const char* const MFUNCTION_NAME; // the text to represent values that are undefined...
extern const char* const DOFUNCTION_NAME;
extern const char* const FORFUNCTION_NAME;
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
extern const char M_DEREFERENCE_CHARACTER; // MDH@10MAR2020: defined in Mshell.c
extern const char M_PROPERTY_SEPARATOR_CHARACTER; // MDH@12MAR2020: defined in Mshell.c

char const * const M_VERSION="0.1.4"; // the new version with file access capabilities (as of 28 September 2020)
char const * const M_BUILD="1";char const * const M_DATE="28 September 2020";

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
// char const * const M_BUILD="3";char const * const M_DATE="02 May 2020"; // dynamic allocation debugging debugging
// char const * const M_BUILD="4";char const * const M_DATE="03 May 2020"; // dynamic allocation debugging debugging
// char const * const M_BUILD="5";char const * const M_DATE="06 May 2020"; // dynamic allocation debugging debugging

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

// used externally
//Mvaluetype={VT_UNDEFINED,VT_TOKEN,VT_INTEGER,VT_BIGINTEGER,VT_DECIMAL,VT_RATIONAL,VT_FLOAT,VT_TEXT,VT_LIST,VT_MAP}
// the list of token type ids in the corresponding order!!!
static const char* getTokenColor(enum TOKENTYPE_ENUM tokenType){
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
	return "";
}
static void outputTokenTypeColor(TokenType tokenType){
	setBackColor(getBackgroundColor());
	setColor(getTokenColor(tokenType));
}
static void outputTokenColor(Mtoken* _token){
	if(_token)outputTokenTypeColor(_token->type);
	///////printf("[%d]",_userInputCommand->_lastToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
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
		/* removing:
		if(token->type==TT_VARIABLE||token->type==TT_NEW_VARIABLE){
			Mtoken* specialFunctionCallToken=getSpecialFunctionCallToken(token);
			if(specialFunctionCallToken){
				output("%s\t%u\n"," local to",specialFunctionCallToken->offset);
			}
		}
		*/
		token=token->next;
	}
}

Mstring* _getTimestamp(char const * const format){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _timestamp=owned_string(__string(),owner);
	if(_timestamp){
		Mstring* p=string_setlength(_timestamp,50/*,owner*/);
		if(p){
	    	time_t now=time(NULL);
			struct tm * nowlocal=localtime(&now);
			p=string_setlength(p,strftime(p->_chars->chars,50,(format?format:"%Y-%m-%d %H:%M:%S"),nowlocal)/*,owner*/);
		}
		if(!p){FREE_STRING(_timestamp,owner);_timestamp=NULL;}
	}
	return disowned_string(_timestamp,owner);
}

void writeTimestamp(FILE* _file){if(!_file)return;Mallocationowner owner=getOwner(__LINE__);
	Mstring* _timestamp=owned_string(_getTimestamp(NULL),owner);
	if(!_timestamp)return;
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

FILE* debugfile=NULL;
#include <stdarg.h>
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
void debugWrite(const char* fmt,...){
	if(!debugfile){
		debugfile=fopen("./Mdebug.txt","a+t"); // append (or create) in text mode
		fputc('\n',debugfile); // start with a single empty line (separating the sessions)
	}
	if(debugfile){
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

Mstring* _getFunctionMapText(Mfunctionmap* _functionmap){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _s=owned_string(__string(),owner);
	if(_s){
		Mstring* p=string_append_char(_s,'[');
		if(_functionmap){
			///printf("\n%s(%d)",string(p),_functionmap->numberOfFunctions);
			Mfunctionmapelement* _functionmapelement=_functionmap->_first;
			while(_functionmapelement){
				///printf("\n%s","start");
				Mfunction* _function=_functionmapelement->_function;
				if(!_function)continue;
				///printf("\n%s","func");
				p=string_append(p,string(_functionmapelement->_name)); // _name moved from _function to _functionmapelement
				if(!p)break;
				///printf("\n%s","name");
				// I guess we might show the parameter map (if any)
				string_append_char(p,'(');
				if(_function->_parameterMap){
					Mstring* parameterMapText=owned_string(_getMapText(_function->_parameterMap,false,false,false),owner); // do NOT show curly braces, quotes or missing defaults
					if(parameterMapText){
						string_append(p,string(parameterMapText));
						FREE_STRING(parameterMapText,owner);
					}
				}
				///printf("\n%s","params");
				string_append_char(p,')');
				_functionmapelement=_functionmapelement->_next;
				if(_functionmapelement)p=string_append(p,", "); // only when there's a next map element to process
				///printf("\n%s","next");
			}
		}
		//printf("\n%s(%d)",string(p),string_length(p));
		p=string_append_char(p,']');
		///printf("\n%s",string(p));
		// if we failed, we have to free s here!!!
		if(!p){FREE_STRING(_s,owner);_s=NULL;}
	}
	return disowned_string(_s,owner);
}
void outputFunctions(){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _functionsText=owned_string(_getFunctionMapText(getExecutionEnvironment()->_functionMap),owner);
	if(!_functionsText){outputError("Failed to create the output. Possible cause: out of memory.");return;}
	output("\nFunctions: %s.\n",string(_functionsText));
	FREE_STRING(_functionsText,owner);
}/* VALIDATED */
void outputVariables(){Mallocationowner owner=getOwner(__LINE__);
	// much easier now that we get the text of any Mvalue (like the variable map of an environment!)
	// MDH@24OCT2019: now using _getVariableMapText() instead of _getMapText() because the former is environment aware and can show the symbols with the same value (if any)
	Mstring* _variablesText=owned_string(_getVariableMapText(getExecutionEnvironment(),false,false,true,false),owner); // do NOT show the hidden variables!!!
	if(!_variablesText){outputError("Failed to create the output. Possible cause: out of memory.");return;}
	output("\nVariables: %s.\n",string(_variablesText));
	FREE_STRING(_variablesText,owner);
}/* VALIDATED */

enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell

enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode

char* promptinfo[]={"Command mode: clear the command with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Use history command|Verbose|Wrap - Options: Beep|Reset|eXit|Functions|History|Shell.","Shell mode: enter a system command to execute."};
/**
call prompt() when ready to receive a new command
 */
const char OPTION_CHAR='`'; // TODO should this character become part of options????

// USER INPUT STUFF

/* TODO are we using the storeCursor() and restoreCursor() sometime?
// VT100 codes...
void storeCursor(){printf("\0337");}
void restoreCursor(){printf("\0338");}
*/

void displayFlags(){
	output("Edit flags: %c%c%c%c%c - Display flags: %c%c.\n",amAssisting()?'A':'a',amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amAcceptinghistorycommand()?'U':'u',amWrapping()?'W':'w',48+getColorscheme());
}
void outputFlags(){
	output("%c%c%c%c%c%c%c",amAssisting()?'A':'a',(48+getColorscheme()),amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'V':'v',amWrapping()?'W':'w',amAcceptinghistorycommand()?'U':'u');
}

// the user input (either the shell command or the M user input command)
Mstring* _shellCommand=NULL;Mallocationowner owner_shellCommand=(Mallocationowner){MODULE_ID,__LINE__,1};

Mcommand* _userInputCommand=NULL;Mallocationowner owner_userInputCommand=(Mallocationowner){MODULE_ID,__LINE__,1}; // the current input command

// keeping track of both the cursor position and the total command length
size_t getUserInputLength(){
	if(inputMode==IM_COMMAND)return(_userInputCommand&&_userInputCommand->_lastToken?_userInputCommand->_lastToken->offset+string_length(_userInputCommand->_lastToken->text):0);
	if(inputMode==IM_SHELL)return string_length(_shellCommand);
	return 0;
}

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText
// MDH@30OCT2019: if we let toInfoInputLine() return the number of lines it moved back we can pass that into toUserInputCursorPosition() to go down that number of lines
// MDH@30OCT2019: for each user input line, keep track of the total number of characters written by the user
typedef struct Muserinputline{
	size_t offset,index; // the number of characters on previous lines and the line index
	struct Muserinputline *_prev; // for accessing previous lines
}Muserinputline;
static Muserinputline* _userinputline=NULL;static Mallocationowner owner_userinputline=(Mallocationowner){MODULE_ID,__LINE__,1};
static size_t getNumberOfCommandLines(){return(_userinputline?_userinputline->index:0)+1;}
// MDH@24SEP2020: unless we're at the end of a command getUserInputLength() would be the current offset BUT if a command is being output completely that would be an invalid assumption therefore we now pass in the offset to use
static Muserinputline* __userinputline(size_t offset){Mallocationowner owner=getOwner(__LINE__);
	Muserinputline* _newUserinputline=CALLOC_1(sizeof(Muserinputline),'6',owner);
	if(_newUserinputline){
		_newUserinputline->_prev=_userinputline;
		_newUserinputline->offset=offset; // this should be the total number of characters in the command on previous input lines
		_newUserinputline->index=getNumberOfCommandLines(); // count the lines
		_userinputline=OWNED(DISOWNED(_newUserinputline,owner),owner_userinputline); // MDH@30JUN2020: pass ownership when assigning to the global _userinputline because then it it bound!!!!
	}
	return _newUserinputline;
}
// call free_userinputline() when starting a new user input command
static size_t free_userinputline(){
	size_t numberOfUserInputLines=0;
	Muserinputline* prevUserinputline;
	while(_userinputline){
		numberOfUserInputLines++;
		prevUserinputline=_userinputline->_prev;
		FREE_DISOWNED_1(_userinputline,'6',owner_userinputline);
		_userinputline=prevUserinputline;
	}
	return numberOfUserInputLines;
}
// MDH@24SEP2020: if a user input line is removed, we return the difference between the removed line offset and the new line offset, which essentially is the number of command characters on the new command line
static size_t removeUserinputline(){
	if(!_userinputline)return 0;
	size_t offset=_userinputline->offset;
	Muserinputline* prevUserinputline=_userinputline->_prev;
	FREE_DISOWNED_1(_userinputline,'6',owner_userinputline);
	_userinputline=prevUserinputline;
	return offset-(_userinputline?_userinputline->offset:0);
}
// MDH@30OCT2019 END
size_t toInfoInputLine(){
	size_t linesUp=0,lines=getNumberOfCommandLines(); // ASSERT lines should be at least 1
	while(linesUp<lines){oneLineUp();linesUp++;}clearLine();// debugging: output("[%zd]",lines);
    return linesUp;
} // MDH@30OCT2019: only after moving all the input lines up do we need to go to the start, also clearLine() will ascertain to end up at the start of the line
// output functions that require access to the current token
void outputUserInputCommandTokenColor(){
	if(_userInputCommand&&_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the current token color
}
uint8_t promptLength=0;
size_t numberOfLineCommandCharacters=0; // the number of command characters on the current command line
size_t returnToUserInputCommandCursorPosition(){
	// ASSERT we're on the last user input line i.e. the input line the user is currently entering command characters
	toStartOfLine();
	size_t numberOfInputCommandLineCharacters=numberOfLineCommandCharacters; // replacing: =getUserInputLength()-(_userinputline?_userinputline->offset:0);
	// size_t debugInfoLength=output(" %zd ",numberOfInputCommandLineCharacters);moveCursorRight(promptLength+numberOfInputCommandLineCharacters-debugInfoLength); // the offset of the current user input line (if any) determines how many characters the user typed on this input line
	moveCursorRight(promptLength+numberOfInputCommandLineCharacters);
	outputUserInputCommandTokenColor();
	return numberOfInputCommandLineCharacters;
}
size_t toUserInputCursorPosition(size_t linesDown){
	while(linesDown>0){oneLineDown();linesDown--;}
	return returnToUserInputCommandCursorPosition();
}
// MDH@28FEB2020: define inputInfo/inputError as static because Mshell.c also has functions with this name (as defaults to inputInfo/inputError)
static void inputInfo(const char* const fmt,...){
	return;
	if(fmt&&strlen(fmt)){ // we have a format
		// outputChar('X');
		size_t linesMovedUp=toInfoInputLine();
		resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		// MDH@22JUL2019: as we're not calling output() here, we can make output() read a character to allow interuption????
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toUserInputCursorPosition(linesMovedUp);
	}
}
static void inputError(const char* const fmt,...){
	if(fmt&&strlen(fmt)){ // we have a format
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
Mstring* _manualFeedforwardText=NULL;Mallocationowner owner_manualFeedforwardText=(Mallocationowner){MODULE_ID,__LINE__,1};
size_t numberOfIdentifierContinuationManualFeedforwardCharacters=0; // MDH@06OCT2019: determine the number of manual feed forward characterr matching the identifier continuation
bool manualFeedforwardCharacterPrepended(char c){
	if(!c)return false;
	if(!_manualFeedforwardText)_manualFeedforwardText=owned_string(__string(),owner_manualFeedforwardText);
	return(_manualFeedforwardText&&string_insert_char(_manualFeedforwardText,0,c));
}
bool prependedToManualFeedforwardText(char const * const characters){
	if(!characters)return false;
	if(!_manualFeedforwardText)_manualFeedforwardText=owned_string(__string(),owner_manualFeedforwardText);
	return string_prepend(_manualFeedforwardText,characters);
}
char getFirstManualFeedforwardCharacterRemoved(){
	return(_manualFeedforwardText?string_removed_char(_manualFeedforwardText,0):'\0');
}

// identifier continuation stuff
// Mcommand stuff moved over to Mshell

// MDH@28OCT2019: not needed here anymore... Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the sequence of tokens starting with _userInputCommand->_firstToken
// MDH@02OCT2019: might need this in multiple places!!
// MDH@04NOV2019: added TT_REFERENCE tokens as well
bool inIdentifierToken(Mtoken* lastCommandToken){
	return(lastCommandToken?lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_FUNCTION||lastCommandToken->type==TT_NEW_VARIABLE||lastCommandToken->type==TT_REFERENCE:false);
}

/* MDH@28OCT2019 replacing: 
Mtoken* _userInputCommand->_lastToken=NULL; // the last token in the command input by the user
*/
bool userInputCommandIdentifierContinuationNeedsUpdating=false; // whether or not the identifier has changed
char* _identifierContinuationCharacters=NULL; // the single text that we can continue the current identifier token with
char getFirstIdentifierContinuationCharacter(){
	inputInfo("%s","Determining the first identifier continuation character!");
	return(_identifierContinuationCharacters?_identifierContinuationCharacters[0]:'\0');
}
bool deleteFirstIdentifierContinuationCharacter(){
	if(!_identifierContinuationCharacters)return false;
	// not undefined or empty...
	char* newIdentifierContinuationText=(strlen(_identifierContinuationCharacters)>1?strdup(_identifierContinuationCharacters+1):NULL); // get dynamic copy of remainder, ignore if we fail!!!
	free(_identifierContinuationCharacters);
	_identifierContinuationCharacters=newIdentifierContinuationText; // NOTE if we failed to copy the remainder, we get NULL now
	return true;
}
char* _identifierContinuationOptionalCharacters=NULL; // the set of characters expected next of existing identifiers
// MDH@26SEP2019: we create a separate method that will determine the last token identifier continuation text and continuation characters
void deleteIdentifierContinuation(){
	if(_identifierContinuationOptionalCharacters){free(_identifierContinuationOptionalCharacters);_identifierContinuationOptionalCharacters=NULL;}
	if(_identifierContinuationCharacters){free(_identifierContinuationCharacters);_identifierContinuationCharacters=NULL;}
}
// user input command specific

// MDH@20SEP2019: we used to keep track of the behind cursor text, in a single Mstring instance, but because we also want to be able to add variable completion we keep a sequence of char* 
//                each feed forward char* is associated with a single token, and if that token is removed so should the associated feed forward
// call deleteTokenautocompletiontexts whenever the behind cursor text changes
typedef struct Mtokenautocompletiontext{
	Mtoken* token;
	Mchars* _text; // MDH@23APR2020: replacing char*
	struct Mtokenautocompletiontext* _next;
	///////bool inactive; // keep track of whether or not active... (a feed forward text can become inactive when the associated token itself is still around but the text was moved to the command with a left or right arrow key)
}Mtokenautocompletiontext;
// MDH@30SEP2019: all token feed forward texts accepted (i.e. consumed) are pointed to by _lastConsumedAutocompletiontext
//                consumption of feed forward texts is done by right arrow (one character at a time) or tab (all feed forward characters)
//                right arrow reads the first feed forward character, gets it accepted and then moves the feed forward character to the consumed feed forward texts
Mtokenautocompletiontext *_firstTokenautocompletiontext=NULL,*_lastConsumedAutocompletiontext=NULL;Mallocationowner tokenautocompletiontextowner=(Mallocationowner){MODULE_ID,__LINE__,1};
// MDH@26SEP2019: it's prudent to store the identifier continuation text separated from the rest of the (autogenerated) feed forward text
Mstring* _getAutoCompletionText(char sep){Mallocationowner owner=getOwner(__LINE__);
	// constructs the total feed forward text
	Mstring* _autocompletionText=owned_string(__string(),owner);
	if(_autocompletionText){
		Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext){
			if(tokenautocompletiontext->_text&&!string_append(_autocompletionText,tokenautocompletiontext->_text->chars))break;
			///////if(autocompletiontext->token)if(!string_append_char(_suggestedText,'#'))break;
			if(sep&&!string_append_char(_autocompletionText,sep))break;
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
	return disowned_string(_autocompletionText,owner);
}

size_t numberOfBehindPromptCharactersWritten=0; // MDH@25SEP2019: the total number of text characters written behind the prompt

// the globally constructed behind cursor text (without separator!!!)
Mstring* _autoCompletionText=NULL; // MDH@27FEB2019: we keep track of the auto completion text
Mallocationowner owner_autoCompletionText=(Mallocationowner){MODULE_ID,__LINE__,1};
void deleteAutocompletionText(){
	if(_autoCompletionText){
		if(amVerboseDebugging())inputInfo("Deleting autocompletion text.");
		FREE_STRING(_autoCompletionText,owner_autoCompletionText); // MDH@19MAY2020: TODO should we disown ->chars before calling free_string????
		_autoCompletionText=NULL;
	}else
	if(amVerboseDebugging())inputInfo("No auto completion text to delete.");
}
void updateAutoCompletionText(){
	deleteAutocompletionText();
	_autoCompletionText=owned_string(_getAutoCompletionText('\0'),owner_autoCompletionText); // get the new characters
	// merge with identifier continuation text
	if(_autoCompletionText&&_identifierContinuationCharacters){
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
}

char getTokenTypeFeedforwardCharacter(TokenType tokenType){
	// some token types have an associated feed forward character!!!
	switch(tokenType){
		case TT_BINARY_aErU:return '=';
		case TT_DQSTRING:return '"'; 
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

// MDH@24SEP2019: we do not always want to set the identifier continuation characters
// MDH@03OCT2019: now excluding the last token immediate feed forward text because that is being taken care of whenever the last token (type) change
//                because of this only the matching parentheses feed forward characters remain so TODO simplify this
char* _getLastTokenAutoCompletionText(){// Mallocationowner owner=getOwner(__LINE__);
	char* tokenAutoCompletionText=""; // on the stack
	if(amMatchingparentheses())
	if(_userInputCommand&&_userInputCommand->_lastToken)
	switch(_userInputCommand->_lastToken->type){
		case TT_ASSIGNMENT:break;
		case TT_BINARY_AeRu:case TT_BINARY_Aeru:case TT_BINARY_aERu:break;
		// MDH@03OCT2019: case TT_BINARY_aErU:tokenFeedforwardText="=";break;
		case TT_BINARY_aeru:break;
		case TT_COMMENT:break;
		// MDH@03OCT2019: case TT_DQSTRING:tokenFeedforwardText="\"";break; // TODO using " for the double quoted string might change in the future and we'd be in trouble then
		case TT_END_OF_DQSTRING:case TT_END_OF_FUNCTION_CALL:case TT_END_OF_LIST:case TT_END_OF_MAP:case TT_END_OF_SQSTRING:break;
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
		//                however, this would require the identifier continuation text to be updated BEFORE calling this method
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
	return strdup(tokenAutoCompletionText); // TODO I suppose we might decide to no longer create a copy on the heap here (due to separating identifier continuation text from other feed forward text)
}
/*
Mtokenautocompletiontext* disowned_tokenautocompletiontext(Mtokenautocompletiontext* _autocompletiontext,Mallocationowner owner_autocompletiontext){
	if(!_autocompletiontext)return NULL;
	disowned_tokenautocompletiontext(_autocompletiontext->_next,owner_autocompletiontext);
	if(_autocompletiontext->_text)disowned_chars(_autocompletiontext->_text,owner_autocompletiontext); // MDH@02MAY2020: switching to freeChars() given that _getChars() was used to create it
	return DISOWNED(_autocompletiontext,owner_autocompletiontext);
}
*/
Mallocationowner owner_tokenAutoCompletionTexts=(Mallocationowner){MODULE_ID,__LINE__,1};
void free_tokenautocompletiontext(Mtokenautocompletiontext* _autocompletiontext/*,Mallocationowner owner_autocompletiontext*/){
	if(!_autocompletiontext)return;
	if(_autocompletiontext->_next)free_tokenautocompletiontext(_autocompletiontext->_next/*,owner_autocompletiontext*/);
	if(_autocompletiontext->_text)FREECHARS(_autocompletiontext->_text,owner_tokenAutoCompletionTexts); // MDH@02MAY2020: switching to freeChars() given that _getChars() was used to create it
	FREE_DISOWNED_1(_autocompletiontext,'7',owner_tokenAutoCompletionTexts); // MDH@07APR2020: _autocompletiontext is an Mstring* so release as 'S'
}

// MDH@04OCT2019: if we remember the immediate feed forward token we can determine whether or not we need to remove the associated feed forward text
Mtoken* immediateFeedforwardToken=NULL;
void deleteTokenautocompletiontexts(){
	deleteAutocompletionText();
	////////if(amDebugging())inputInfo("Autocompletion text deleted.");
	/////////////////numberOfBehindPromptCharactersWritten=getCommandLength(); // MDH@25SEP2019: TODO if you know a better place to do this then here let me know
	free_tokenautocompletiontext(_firstTokenautocompletiontext);
	_firstTokenautocompletiontext=NULL; // OOPS pretty essential!!!!
	immediateFeedforwardToken=NULL; // MDH@04OCT2019: also pretty essential as we won't have a feed forward text with this token anymore
	if(amVerboseDebugging())inputInfo("Token autocompletion texts deleted.");
}
/*
void deleteTokenautocompletionCharacters(size_t numberOfTokenAutocompletionCharacters){

}
*/
// every time feed forward text is to be added, it is prepended to the list of feed forward texts setting the token pointer to _userInputCommand->_lastToken
Mtokenautocompletiontext* getTokenAutocompletionText(Mtoken* token){
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&tokenautocompletiontext->token!=token)tokenautocompletiontext=tokenautocompletiontext->_next;
	return tokenautocompletiontext;
}
// _text is dynamically allocated and should be freed if not bound to some!!
// MDH@24SEP2019: whenever the user uses the left arrow to move characters out of the token into the feed forward text
//                it should be remembered that these characters were associated with this token
//                so that when the feed forward text for a given token is set
//                the removed token characters and the actual feed forward characters of the token can be combined
// MDH@25MAY2020: _text changed to _chars (assumed to be a const character array owned somewhere else that we completely leave alone, that's what we can do with completely constant pointers)
char* setLastTokenAutocompletionText(char const * const _chars){Mallocationowner owner=getOwner(__LINE__);
	// do not prepend empty feed forward texts!!!
	if(!_chars)return NULL;
	// MDH@01OCT2019: BUG FIX it would be wrong to NOT update the last token feed forward text when no text is specified as we were doing
	// MDH@02OCT2019: BUG FIX if _text is empty and there is no token yet do NOT add the token
	/////////// removing: if(strlen(_text)>0){ // there's text to store
	// if we already have an autogenerated feed forward text associated with the last command token
	Mtokenautocompletiontext* lastTokenAutocompletionText=getTokenAutocompletionText(_userInputCommand->_lastToken);
	if(lastTokenAutocompletionText){ // yes, so replace the text contents
		if(lastTokenAutocompletionText->_text)FREECHARS(lastTokenAutocompletionText->_text,owner_tokenAutoCompletionTexts);
		deleteAutocompletionText();
		lastTokenAutocompletionText->_text=owned_chars(_getChars(_chars),Msubowner(owner_tokenAutoCompletionTexts,1)); // replacing: _getChars(_text,foid); // _text now bound!! // MDH@23APR2020 NO not bound, as _text replaced by _getChars(_text)
	}else{ // not present yet, so add (i.e. prepend!!)
		if(strlen(_chars)>0){
			if(amVerboseDebugging())
				inputInfo("Prepending auto completion text '%s' of token '%s' with offset %zu.",_chars,string(_userInputCommand->_lastToken->text),_userInputCommand->_lastToken->offset);
			Mtokenautocompletiontext* _tokenautocompletiontext=CALLOC_1(sizeof(Mtokenautocompletiontext),'7',owner_tokenAutoCompletionTexts);
			if(_tokenautocompletiontext){
				deleteAutocompletionText(); // I guess this is a bit confusing
				_tokenautocompletiontext->_text=owned_chars(_getChars(_chars),Msubowner(owner_tokenAutoCompletionTexts,1)); // MDH@23APR2020: not bound because _getChars() copies the text as well
				_tokenautocompletiontext->token=_userInputCommand->_lastToken;
				// how about skipping all anonymous feed forward texts????
				// if the first one is anonymous mark that one as last anonymous feed forward text
				Mtokenautocompletiontext *lastAnonymousTokenautocompletiontext=(_firstTokenautocompletiontext&&!_firstTokenautocompletiontext->token?_firstTokenautocompletiontext:NULL);
				// if the successor is also anonymous increment
				while(lastAnonymousTokenautocompletiontext&&lastAnonymousTokenautocompletiontext->_next&&!lastAnonymousTokenautocompletiontext->_next->token)
					lastAnonymousTokenautocompletiontext=lastAnonymousTokenautocompletiontext->_next;
				if(lastAnonymousTokenautocompletiontext){
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
	return _chars; // MDH@25MAY2020 convenient to return the input (see updateLastTokenAutocompletionText())
	// MDH@25MAY2020 no need to do the following anymore: freeChars(_text,owner_tokenAutoCompletionTexts); // will only succeed if currently disowned
}

// MDH@01OCT2019: in general when a token is removed its associated (autogenerated) suggested (identifier continuation and feed forward text) should be removed as well
//                NOTE deleteAutocompletionTextOfToken delegates to deleteAutocompletionTextOfToken
bool deleteTokenAutocompletionText(Mtoken* token){
	// locate the (autogenerated) feed forward text associated with this token (most likely the first one)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&tokenautocompletiontext->token!=token){prevtokenautocompletiontext=tokenautocompletiontext;tokenautocompletiontext=prevtokenautocompletiontext->_next;}
	if(tokenautocompletiontext){
		if(prevtokenautocompletiontext)prevtokenautocompletiontext->_next=tokenautocompletiontext->_next;else _firstTokenautocompletiontext=tokenautocompletiontext->_next;
		deleteAutocompletionText(); // the feed forward text changed so needs to be reconstructed whenever it is to be shown
		tokenautocompletiontext->_next=NULL;free_tokenautocompletiontext(tokenautocompletiontext); // only free the token feed forward text we are to remove!!
	} // MDH@04OCT2019: it's NOT there, so we may consider it deleted
	// relink the rest of the feed forward text chain to skip this feed forward text
	// if we have a previous feed forward text make if point to the successor of the feed forward text we are now removing, otherwise we get a new first feed forward text
	return true;
}

char getImmediateFeedforwardCharacterOfUserInputCommand(){
	// returns the character that might directly follow the current token (matching parentheses feed forward characters excluded)
	Mtoken* token=(_userInputCommand?_userInputCommand->_lastToken:NULL);
	if(token)
	switch(token->type){
		case TT_NEW_VARIABLE:case TT_BINARY_aErU:return '=';
		case TT_DQSTRING:return '"';
		case TT_FUNCTION:return '(';
		case TT_SQSTRING:return '\'';
		default:break;
	}
	return '\0';
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
// MDH@30SEP2019: here the problem is that if we remove the first feed forward character we loose the token reference that generated it
//                so we might split up this functionality in two parts: 1. get the first feed forward character 2. remove it either by consuming it or delete if it disappears completely
//                alternatively we can simply remove the character but not remove the feed forward text when it becomes empty, that way we can still consume the feed forward text
//                however, suppose accepting this character as command characters fails, it would make sense to NOT actually remove the feed forward character until after accepting it?????????
//                I suppose the initial solution should be to extract the character and once used successfully remove it
//                so, replacing getFirstAutocompletionCharacterRemoved() by getFirstAutocompletionCharacter() and a function to actually delete that first feed forward character (and either consume or delete it)
char getFirstAutocompletionCharacter(bool autogenerated){
	// if autogenerated is set any first character should be returned, if not, the first feed forward character should be in a feed forward text associated with a token (i.e. autogenerated)
	// most of time _firstTokenautocompletiontext will contain that first feed forward character
	Mtokenautocompletiontext *tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){
		if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text->chars[0]!='\0')return(!autogenerated||tokenautocompletiontext->token?tokenautocompletiontext->_text->chars[0]:'\0');
		tokenautocompletiontext=tokenautocompletiontext->_next; // get the next to check
	}
	return '\0';
}
bool deleteFirstAutocompletionCharacter(char firstAutocompletionCharacter,bool consumed){
	if(firstAutocompletionCharacter!='\0'){
		// find it
		Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
		while(tokenautocompletiontext){
			if(tokenautocompletiontext->_text&&tokenautocompletiontext->_text->chars[0]==firstAutocompletionCharacter)break;
			prevtokenautocompletiontext=tokenautocompletiontext; // remember the current feed forward text as the previous feed forward text
			tokenautocompletiontext=prevtokenautocompletiontext->_next; // get the next to check
		}
		if(tokenautocompletiontext){ // got it
			deleteAutocompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
			// NOTE currently the length of the feed forward text will ALWAYS be 1 but if that would not always be the case we would need to consider addressing that happening as well
			Mchars* p=tokenautocompletiontext->_text;
			size_t l=(p?strlen(p->chars):0);
			if(l==1){
				if(consumed){ // i.e. we should remember the token text, so we can unconsume it
					Mtokenautocompletiontext* nextFirstTokenautocompletiontext=NULL;
					while(_firstTokenautocompletiontext){
						nextFirstTokenautocompletiontext=_firstTokenautocompletiontext->_next; // remember what the next first token feed forward text will become
						// make the current first token feed forward text point to what the current last consumed token feed forward text is
						_firstTokenautocompletiontext->_next=_lastConsumedAutocompletiontext;
						// set the last consumed token feed forward text to the current first token feed forward text
						_lastConsumedAutocompletiontext=_firstTokenautocompletiontext;
						// update the first token feed forward text to what it was originally pointing to
						_firstTokenautocompletiontext=nextFirstTokenautocompletiontext;
						// if the token feed forward text with the first feed forward character was consumed, we're done
						if(_lastConsumedAutocompletiontext==tokenautocompletiontext)break;
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
		inputError("First suggested character vanished.");
	}
	return false;
}
// MDH@30SEP2019: TODO still using the following for true feed forward deletes but should in due course be replaced by using the above two methods
char getFirstAutocompletionCharacterRemoved(){
	if(amVerbose())inputInfo("%s","Determining the first feed forward character!");
	char firstAutocompletionCharacterRemoved='\0';
	// find first feed forward text with text (so skipping all without text)
	Mtokenautocompletiontext *prevtokenautocompletiontext=NULL,*tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext&&(!tokenautocompletiontext->_text||tokenautocompletiontext->_text->chars[0]=='\0'))
	{prevtokenautocompletiontext=tokenautocompletiontext;tokenautocompletiontext=prevtokenautocompletiontext->_next;}
	if(tokenautocompletiontext){
		Mchars* p=tokenautocompletiontext->_text;
		if(p){
			size_t l=strlen(p->chars); // the number of autocompletion characters
			if(l>0){ // there is a first autocompletion character
				firstAutocompletionCharacterRemoved=p->chars[0];
				deleteAutocompletionText(); // ESSENTIAL otherwise it wouldn't update the feed forward text when it needs to (when writing the behind cursor text!!)
				if(amVerbose())inputInfo("First feed forward character '%c'.",firstAutocompletionCharacterRemoved);
				// replace the current text by what's behind the first character (if any)
				tokenautocompletiontext->_text=(l>1?owned_chars(_getChars(p->chars+1),Msubowner(owner_tokenAutoCompletionTexts,1)):NULL);
				FREECHARS(p,owner_tokenAutoCompletionTexts); // free the currently used dynamic memory still pointed to by p // MDH@11JUN2020: i.e. freeChars called on the disowned Mchars*
				if(!tokenautocompletiontext->_text){ // nothing left (or failing to copy the remainder over)
					Mtokenautocompletiontext* nexttokenautocompletiontext=tokenautocompletiontext->_next; // remember where to link the previous to
					tokenautocompletiontext->_next=NULL;
					free_tokenautocompletiontext(tokenautocompletiontext); // get rid of the feed forward text
					// link the predecessor to the successor
					if(prevtokenautocompletiontext)prevtokenautocompletiontext->_next=nexttokenautocompletiontext;else _firstTokenautocompletiontext=nexttokenautocompletiontext;
				}
			}else
				inputError("%s","No feed forward characters!");
		}else
			inputError("%s","No feed forward text!");
	}else
		inputError("%s","No feed forward found!");	
	return firstAutocompletionCharacterRemoved;
}
/*
// a character can be 'anonymously' prepended to the feed forward text
// BUT if it matches the feed forward text of the current token and the current token does not have a feed forward text yet, it shouldn't be anonymous!!!
// MDH@24SEP2019: NO not anonymous because the character was removed from the current token and should be remembered as such
//                however this cause a problem if the originating token actually is the feed forward character of another token
// MDH@30SEP2019: now we should unconsume this character if it was last consumed
// MDH@04SEP2019: immediate feed forward characters should ALWAYS be prepended explicitly and so true is to be passed in for \p new
//                NOTE currently there are only two calls, one to prepend an immediate feed forward character (which should always be flagged as new), and left arrow prepending which is marked as false
Mtokenautocompletiontext*  getAutocompletionTextOfCharacterPrepended(char c,bool new){Mallocationowner owner=getOwner(__LINE__);
	Mtokenautocompletiontext* result=NULL;
	if(c){
		// possible recently consumed...
		// TODO technically we should check the last character in last consumed feed forward text (instead of assuming every feed forward text consists of a single character!!)
		if(!new&&_lastConsumedAutocompletiontext&&_lastConsumedAutocompletiontext->_text&&_lastConsumedAutocompletiontext->_text->chars[0]==c){
			Mtokenautocompletiontext* nextLastConsumedTokenautocompletiontext=_lastConsumedAutocompletiontext->_next; // remember what the last consumed token feed forward text is pointing to
			_lastConsumedAutocompletiontext->_next=_firstTokenautocompletiontext; // make the last consumed token feed forward text to the current first token feed forward text
			_firstTokenautocompletiontext=_lastConsumedAutocompletiontext; // replace the first token feed forward text by the last consumed token feed forward text
			_lastConsumedAutocompletiontext=nextLastConsumedTokenautocompletiontext; // replace the last consumed token feed forward text by what it was previously pointing to
			result=_lastConsumedAutocompletiontext;
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
					deleteAutocompletionText();
				}
				// if failed, ascertain to free what is not bound in the feedforward text list
				if(!result){autocompletiontext->_next=NULL;free_tokenautocompletiontext(autocompletiontext,owner);autocompletiontext=NULL;}
			}
		}
	}
	return DISOWNED(result,owner);
}
*/
// MDH@03OCT2019: it's essential to differentiate between current token dependent feed forward and other feed forward
//                deleteLastTokenImmediateFeedforwardText() is to be called when _userInputCommand->_lastToken stops being the current token or when the type of the current token changes
//                updateImmediateFeedforwardTextOfUserInputCommand() is to be called when _userInputCommand->_lastToken just became the current token (or when its type changes)
// MDH@04OCT2019: deciding to keep the immediate feed forward text separate from the other feed forward texts, that way it is easier to merge the identifier continuation and feed forward texts
Mstring* _immediateFeedforwardText=NULL;Mallocationowner owner_immediateFeedforwardText=(Mallocationowner){MODULE_ID,__LINE__,1};
bool immediateFeedforwardToBeUpdated=false;
bool updateImmediateFeedforwardTextOfUserInputCommand(){
	if(!_immediateFeedforwardText)return false; // should have one
	char lastTokenImmediateFeedforwardCharacter=getImmediateFeedforwardCharacterOfUserInputCommand();
	return(!lastTokenImmediateFeedforwardCharacter||!string_append_char(_immediateFeedforwardText,lastTokenImmediateFeedforwardCharacter));
}
/* replacing:
// \brief prepends the immediate feed forward character of the current token (if any), returns true on success, false otherwise
bool updateImmediateFeedforwardTextOfUserInputCommand(){
	// MDH@03OCT2019: getAutocompletionTextOfCharacterPrepended was adjusted to return true when the character passed to it equals '\0'!!
	//                however TODO currently the prepending is anonymous, whereas this prepending should NOT be done anonymous, otherwise we can't delete it later on
	// get rid of any current immediate feed forward text
	if(immediateFeedforwardToken&&!deleteAutocompletionTextOfToken(immediateFeedforwardToken))return false;
	immediateFeedforwardToken=NULL;
	char lastTokenImmediateFeedforwardCharacter=getImmediateFeedforwardCharacterOfUserInputCommand(_userInputCommand->_lastToken);
	if(!lastTokenImmediateFeedforwardCharacter)return true;
	// try to prepend the character
	Mtokenautocompletiontext* lastTokenImmediateFeedforwardtext=getAutocompletionTextOfCharacterPrepended(lastTokenImmediateFeedforwardCharacter,true); // second argument forces always prepending this character!!
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
	//                however this should also happen when the type of the current token changes
	// MDH@04OCT2019: a little less efficient to move it to the input loop but more reliable!!!
	//// removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the last token immediate feed forward text.");
	if(_userInputCommand)_userInputCommand->_lastToken=newLastCommandToken;else inputError("BUG: No user input command");
	//// removing: if(!updateImmediateFeedforwardTextOfUserInputCommand())inputError("Failed to add the last token immediate feed forward text.");
	userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand);
}
*/
/*
void deleteAutocompletionTextOfToken(Mtoken* token,bool deleteIdentifierContinuationText){
	if(!token)return;
	if(deleteIdentifierContinuationText)deleteIdentifierContinuation(); // this is the easiest way but makes sense
	deleteAutocompletionTextOfToken(token);
}
*/
Mstring* _suggestedText=NULL;Mallocationowner owner_suggestedText=(Mallocationowner){MODULE_ID,__LINE__,1}; // MDH@04SEP2019: where we'll be storing the entire feed forward text (i.e. identifier continuation, immediate feed forward and auto completion text)

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
long long commandCount=0; // the total number of command input
long long commandIndex=0; // the index of the current command from the end of the command list (stored in commands)

/// MDH@28OCT2019: replaced by _userInputCommand: Mtoken* _userInputCommand->_firstToken=NULL;

size_t getNumberOfSuggestedCharacters(){return(_suggestedText?string_length(_suggestedText):0);}
size_t getCommandLength(){return getUserInputLength()+getNumberOfSuggestedCharacters();} // TODO not correct this way!!!!

// request body of function moved over to Mshell.h/c
void outputTimestamp(){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _promptTimestamp=owned_string(_getTimestamp(NULL),owner); // should return a disowned timestamp, so we do not need to obtain ownership that we need to detach on calling free_string
	if(!_promptTimestamp)return;
	outputToFile(NULL,string(_promptTimestamp),">\n"); // pass it along to echoToOutputFile to show in front of < that indicates the start of an output fragment
	FREE_STRING(_promptTimestamp,owner);
}

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
				outputTimestamp();
				echoToOutputFile();
				Mstring* _environmentName=owned_string(_getExecutionEnvironmentName(),owner); // free asap
				if(_environmentName){
					output(string(_environmentName));
					promptLength=string_length(_environmentName);
					FREE_STRING(_environmentName,owner);
				}
				/* replacing:
				output("M");
				promptLength=1;
				*/
				// MDH@19JUL2019: when dealing with a function body being entered, we show a different prompt
				if(getCurrentFunctionBodyInput())
					sprintf(str,"%lld",1+getNumberOfFunctionCommands(getExecutionEnvironment()->_name->chars));	// replacing: printf("%lu",(commandCount+1));
				else
					sprintf(str,"%lld",(commandCount+1));	// replacing: printf("%lu",(commandCount+1));
				promptLength+=output("[%s]",str);
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
unsigned long long showContinuedPrompt(int64_t offset,bool newline){
	unsigned long long promptCharactersWritten=0;
	if(inputMode==IM_COMMAND){
		// ASSERT only to be called in command mode with _userInputCommand not NULL
		// MDH@20FEB2020: passing getUserInputLength() to set the offset of the new user input line (because I'm the only one who can tell)
		if(offset>=0){
			if(!__userinputline(offset))return 0; // if we fail to create a new user input line (to keep track of the number of characters on previous user input lines)
			numberOfLineCommandCharacters=0; // MDH@26JUN2020: keeping track of the number of command characters on the current user input line
		}
		if(newline){
			clearScreenFromCursor(); // to get rid of any suggested text behind the cursor
			promptCharactersWritten+=outputChar('\n'); // move over to the next line
		}
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
//                determine exactly where the soft-breaks will be
// MDH@26JUN2020: this is a bit hazardous when getCurrentNumberOfWindowTextColumns() returns a value that is below promptLength which is not to be allowed!!!
int numberOfLineCharacters=0;
// call initializeNumberOfLineCharacters every time the prompt is shown
void initializeNumberOfLineCharacters(){
	int minimumNumberOfLineCharacters=MIN(20,promptLength+10);
	numberOfLineCharacters=getCurrentNumberOfWindowTextColumns();
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
}

// MDH@06MAY2020
void outputTotalMemoryUsage(){//Mallocationowner owner=getOwner(__LINE__);
	resetOutputColor(); // MDH@30JUN2020: apparently sometimes required
	if(!amVerbose()){
		unsigned long long numberOfAllocationTypeSizes=0; // i.e. only interested in the overall types
		long long numberOfAllocationMarks=-1; // i.e. only interested in the last (=current) mark
		// MDH@22MAY2020 TODO unfortunately _getAllationTypeSizes is NOT under allocation managed control
		long long * _allocationTypeSizes=_getAllocationTypeSizes(NULL,&numberOfAllocationTypeSizes,&numberOfAllocationMarks);
		if(_allocationTypeSizes){
			if(numberOfAllocationTypeSizes>0&&numberOfAllocationMarks>0)
				output("Dynamically allocated memory: %llu bytes.\n",_allocationTypeSizes[1]);
			free(_allocationTypeSizes);
		}else
			outputError("No memory allocation information available!");
	}else{ // verbose information will also show all the current allocation marks
		output("Dynamic memory allocation:\n");
		outputAllocationTypeMarks("\t");
	}
}/* VALIDATED */
// MDH@11MAY2020: if we want to know what changed since the previous mark as for the incremental memory usage
//                it's easiest to ask for all and only show the changes
long long outputIncrementalMemoryUsage(long long incrementalNumberOfAllocationMarks){Mallocationowner owner=getOwner(__LINE__);
	unsigned long long numberOfAllocationTypeSizes=0; // i.e. only interested in the overall types
	long long numberOfAllocationMarks=incrementalNumberOfAllocationMarks+1; // request at least one more allocation mark than increments we need 
	// MDH@22MAY2020 TODO unfortunately _getAllationTypeSizes is NOT under allocation managed control
	long long * _allocationTypeSizes=_getAllocationTypeSizes("",&numberOfAllocationTypeSizes,&numberOfAllocationMarks);
	if(amVerboseDebugging())output("Number of allocation type sizes received: %lld. Number of allocation marks received: %lld.\n",numberOfAllocationTypeSizes,numberOfAllocationMarks);
	if(_allocationTypeSizes){
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
void promptForUserInput(){
	free_userinputline();if(_userinputline)outputBug("Failed to release user input line info"); // MDH@30OCT2019: get rid of all previously stored user input line info
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
void outputText(char* fmt,char* text){resetOutputColor();output(fmt,text);}

// MDH@23SEP2020: if characters are output from a certain start position we can return an Mcursormovement indicating the final position and the number of characters written
typedef struct{
	uint16_t position;
	size_t written,skipped;
}Mcursormovement;

// MDH@07JUL2020: if you want to output text that wraps to the number of command line characters call outputCommandText()
//                passing in the text and the current position on the line, passing out the final position on the line
// MDH@22SEP2020: it makes sense to return the number of characters written as well we can use a long long for that
//                assuming 32 bits for the position and number of characters to write suffice or we could use 16 bits
//                for the position and 48 bits for the maximum number of characters to write
// MDH@23SEP2020: uses and updates cursormovement
// MDH@24SEP2020: bool notsuggested replaced by int64_t offset (pass -1 for offset if not in the command)
//                but be careful here because 
static void outputCommandLineText(char* text,Mcursormovement* _cursormovement,char* textcolor,int64_t commandCharactersWrittenSoFar){
	if(!_cursormovement)return;
	size_t numberOfCharactersToOutput=(text?strlen(text):0);
	if(numberOfCharactersToOutput>0){
		setColor(textcolor);
		uint16_t maximumNumberOfLineCommandCharacters=(numberOfLineCharacters>0?numberOfLineCharacters-promptLength:0); // MDH@23SEP2020: I suppose we have one character more (if we allow a character on the last position of the line)
		if(maximumNumberOfLineCommandCharacters>0&&_cursormovement->position+numberOfCharactersToOutput>maximumNumberOfLineCommandCharacters){
			unsigned long long leftOnLine=maximumNumberOfLineCommandCharacters-_cursormovement->position; // what we can fit on the line
			// careful: leftOnLine could now be zero, essentially we know that characters will be written on successive lines
			for(int characterIndex=0;characterIndex<numberOfCharactersToOutput;characterIndex++){
				if(leftOnLine==0){
					size_t prompted=showContinuedPrompt(commandCharactersWrittenSoFar,false);setColor(textcolor); // normally we would use setTokenColor(token) which would also set the background color but we're assuming that the background color won't change
					_cursormovement->skipped+=prompted;
					leftOnLine=maximumNumberOfLineCommandCharacters; // NOTE: so leftOnLine is the total number of command characters that we can fit after the prompt
				}
				size_t written=outputChar(text[characterIndex]);
				if(written){
					if(commandCharactersWrittenSoFar>=0)commandCharactersWrittenSoFar+=written; // increment offset indicating the number of command characters written so far
					leftOnLine-=written;
					_cursormovement->written+=written;
				}
			}
			_cursormovement->position=maximumNumberOfLineCommandCharacters-leftOnLine;
		}else{
			size_t charactersOutput=output("%s",text);
			_cursormovement->position+=charactersOutput;
			_cursormovement->written+=charactersOutput;
		}
	}
}

// Token is now defined in Mexpression.h which is included by Mexecution.h so struct Token is indirectly supplied by Mexpression.h!!!
// MDH@24JUN2020: need to reconsider how to output the token given that a single token can occupy multiple 
//                output lines based on its position and length (and numberOfLineCharacters and promptLength)
// MDH@24SEP2020: every function that outputs text on the command line should receive a Mcursormovement reference to be passed along to outputCommandLineText...
//                NOTE let's allow passing in NULL for _cursormovement which is valid when the result of outputToken is not used (as is often the case)
static void outputToken(Mtoken* _token,Mcursormovement* _cursormovement){Mallocationowner owner=getOwner(__LINE__);
	if(!_token)return;
	// MDH@24SEP2020: the following ASSERT still holds except that _cursormovement->position should actually hold the same value
	// ASSERT assuming _token->position actually contains the number of command characters on the current command line in front of this token
	// MDH@24SEP2020 with _cursormovement being the added parameter we do not need this anymore: size_t position=0;
	size_t tokenCharacterCount=string_length(_token->text);
	if(tokenCharacterCount>0){
		Mcursormovement cursormovement={_token->position};if(!_cursormovement)_cursormovement=&cursormovement; // if no cursor movement instance was provided, we create one from the token's position, of course no result will be available to the outside
		// MDH@24SEP2020 not being used in this function apparently: uint16_t maximumNumberOfLineCommandCharacters=(numberOfLineCharacters>0?numberOfLineCharacters-promptLength-1:0);
		// MDH@26JUN2020: we can speed things up a bit by immediately using string()
		char* tokenText=string(_token->text);
		// MDH@31OCT2019: by introducing ` as new line request character (whitespace) we'll be having visible whitespace characters at the end of the token which we do not want to show in the same color
		// ascertain that the token text ends at the first whitespace character (if there is any whitespace) NOTE there's no need to put '\0' back, therefore we use '\0' if we didn't replace the character to start with
		char firstWhitespaceCharacter;
		size_t significantTokenCharacterCount;
		if(isTokenFinished(_token)){
			significantTokenCharacterCount=getTokenSignificantCharacterCount(_token);
			firstWhitespaceCharacter=tokenText[significantTokenCharacterCount];
			tokenText[significantTokenCharacterCount]='\0';
		}else{
			significantTokenCharacterCount=tokenCharacterCount;
			firstWhitespaceCharacter='\0';
		}
		// if we allow comments in tokens we're in trouble!!!
		// now done by outputCommandLineText(): outputTokenColor(_token);
		// MDH@07JUL2020: delegating writing the significant token characters to outputCommandLineText()
		// MDH@22SEP2020: outputCommandLineText() now also returning the number of characters written (and the position in the first 16 bits)
		// MDH@24SEP2020 with _cursormovement being the added parameter we do not need this anymore (passing &cursormovement to outputCommandLineText): Mcursormovement cursormovement={_token->position};
		int64_t commandCharactersWrittenSoFar=_token->offset;
		outputCommandLineText(tokenText,_cursormovement,getTokenColor(_token->type),commandCharactersWrittenSoFar);
		// MDH@24SEP2020 NOTE: _cursormovement->written now contains the number of token characters written
		/* replacing:
		// MDH@24JUN2020: if a token is on multiple lines (due to a limiting number of line characters)
		//                we have to 'insert' so-called soft-breaks
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
		resetOutputColor();
		if(firstWhitespaceCharacter){
			tokenText[significantTokenCharacterCount]=firstWhitespaceCharacter; // put it back
			// MDH@07JUL2020: delegating to outputCommandLineText() for writing the whitespace characters
			// MDH@22SEP2020: same here
			// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...
			outputCommandLineText(tokenText+significantTokenCharacterCount,_cursormovement,getInfoColor(),commandCharactersWrittenSoFar+_cursormovement->written);
			commandCharactersWrittenSoFar+=_cursormovement->written; // update the number of command characters written so far (that is including the token characters we've now written)
			// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: position=cursormovement.position;
			/*
			// if spanning multiple lines write one character at a time
			if(tokenCharacterCount>left+significantTokenCharacterCount){
				for(size_t tokenCharacterIndex=significantTokenCharacterCount;tokenCharacterIndex<tokenCharacterCount;tokenCharacterIndex++){
					outputChar(tokenText[tokenCharacterIndex]);
					if(--left==0){
						showContinuedPrompt(true);
						left=maximumNumberOfLineCommandCharacters;
					}
				}
			}else
				output("%s",tokenText+significantTokenCharacterCount);
			*/
			// MDH@04JUL2020: the token may end with the explicit newline character!
			if(tokenText[tokenCharacterCount-1]==M_NEWLINE_CHARACTER){
				_cursormovement->skipped+=showContinuedPrompt(commandCharactersWrittenSoFar,true); // MDH@24SEP2020: the number of prompt characters written now appended to the skipped field of _cursormovement
				_cursormovement->position=0; // MDH@24SEP2020: I guess we're at the beginning of the command line again
				// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: position=0;
			}
		}
		// resetOutputColor();
	}
	// MDH@24SEP2020: passing on _cursormovement, so no need for a separate position local anymore...: return position; // replacing: tokenCharacterCount;
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}
// MDH@30APR2019: when a function returns to a variable and the other way round
// MDH@26JUN2020: TODO has to be reviewed!!!
static void reoutputToken(Mtoken* _token){
	// outputChar('X');
	size_t tokenCharacterCount=(_token&&_token->text?string_length(_token->text):0);
	if(tokenCharacterCount==0)return; // shouldn't happen though
	// MDH@31OCT2019: this is particularly hard if the token is written over several lines
	//                I solved this by making the newline character always end a token (by treating it as whitespace that effectively ends any current token)
	//                but now we have the situation that the given token might be at the previous line, this is the case when the token was ended with a newline and we're now at the start of the next line
	//                the situation is: we're at the end of the token and its type changed and we have to write it again and return to the current position
	//                we might have consumed an assignment operator behind it returning us to this token which might be variable that could also be a function (?????)
	//                the problem with writing the token is that it does not recognize the newline character at the end
	//                for safety reasons we look at _userinputline because if _userinputline is NULL there's no previous command input line
	bool tokenOnPreviousInputLine=(_userinputline?(string_last_char(_token->text)==M_NEWLINE_CHARACTER):false);
	if(tokenOnPreviousInputLine){oneLineUp();toStartOfLine();moveCursorRight(promptLength+(_userinputline->offset-(_userinputline->_prev?_userinputline->_prev->offset:0)));}
	moveCursorLeft(tokenCharacterCount);
	outputToken(_token,NULL); // back where we started (hopefully) // MDH@24SEP2020: not passing an Mcursormovement in, as the result of outputToken is not used!!!
	if(tokenOnPreviousInputLine){oneLineDown();toStartOfLine();moveCursorRight(promptLength+getUserInputLength()-_userinputline->offset);}
}

void clearInfo(){
	toUserInputCursorPosition(toInfoInputLine());
} // MDH@30OCT2019: toInfoInputLine() automatically clears the info input line!!!

void inputInfoCommand(Mcommand* command){
	size_t linesMovedUp=toInfoInputLine();
	resetOutputColor();
	if(command){Mtoken* token=command->_firstToken;while(token){output("%s|",string(token->text));token=token->next;}}
	toUserInputCursorPosition(linesMovedUp);
}

void outputStatus(char inputChar,char inputCharType){Mallocationowner owner=getOwner(__LINE__);
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=owned_string(_getAutoCompletionText('|'),owner);if(!_separatedBehindCursorText)return;
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %s | Cursor position: %zu | Command length: %zu | Manual feed forward: '%s' | Identifier continuation: '%s' | Feed forward: '%s'.",inputChar,inputChar,inputCharType,(_userInputCommand->_lastToken!=NULL?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:""),getUserInputLength(),getCommandLength(),(_manualFeedforwardText?string(_manualFeedforwardText):""),(_identifierContinuationCharacters?_identifierContinuationCharacters:""),string(_separatedBehindCursorText));
	FREE_STRING(_separatedBehindCursorText,owner);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}
void outputDebugInfo(){Mallocationowner owner=getOwner(__LINE__);
	////////printf("[%u,%u]",getUserInputLength(),getCommandLength());
	Mstring* _separatedBehindCursorText=owned_string(_getAutoCompletionText('|'),owner);if(!_separatedBehindCursorText)return;
	/////////debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
	inputInfo("Cursor position: %zu | Command length: %zu | Token type: % s | Manual feed forward: '%s' | Identifier continuation: '%s' | Auto completion: '%s'.",getUserInputLength(),getCommandLength(),(_userInputCommand&&_userInputCommand->_lastToken?TOKENTYPE_STRING[_userInputCommand->_lastToken->type]:""),(_manualFeedforwardText?string(_manualFeedforwardText):""),(_identifierContinuationCharacters?_identifierContinuationCharacters:""),string(_separatedBehindCursorText));
	FREE_STRING(_separatedBehindCursorText,owner);
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",getUserInputLength(),getCommandLength(),string(feedforwardText));
}

/*
// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType){
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//                but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=_getToken(lastCommandToken,tokenType);
	if(_newCommandToken)
		setTokenType(_newCommandToken,tokenType);
	else 
		inputError("Failed to create a command token");
	return _newCommandToken;
}
*/

Mallocationowner owner_currentFunctionBodyInput=(Mallocationowner){MODULE_ID,__LINE__,1};

// keep track of all commands so far
#define COMMAND_BLOCKSIZE 8
 // array for storing the pointers to the first token of all commands entered
// MDH@18JUN2020: a registered command might be a command that is a duplicate of a previous command
typedef struct{
	Mcommand* _command;
	unsigned long long previousCommandIndex;
}Mregisteredcommand;
Mregisteredcommand* _registeredcommands=NULL;Mallocationowner owner_registeredcommands=(Mallocationowner){MODULE_ID,__LINE__,1};
size_t commandBlocks=0;
// MDH@24MAY2020 NOTE: registerCommand is ONLY called once with _userInputCommand as argument but 
// MDH@12JUN2020 TODO TODO TODO how to deal with the command being registered and whether or not the tokens are to be disowned when put in the value 
bool registerCommand(Mcommand* command,Mallocationowner owner_command){if(!command)return false;Mallocationowner owner=getOwner(__LINE__);
	if(!getCurrentFunctionBodyInput()){ // a top-level (non function body) command
		if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
			// I have to copy all first token pointers to a new array large enough
			Mregisteredcommand* newRegisteredCommands=(commandBlocks==0?MALLOC(sizeof(Mregisteredcommand),COMMAND_BLOCKSIZE,'C',owner_registeredcommands):REALLOC(_registeredcommands,commandBlocks*COMMAND_BLOCKSIZE,(commandBlocks+1)*COMMAND_BLOCKSIZE,sizeof(Mregisteredcommand),'C'));
			if(newRegisteredCommands==NULL)return false;
			commandBlocks++;
			_registeredcommands=newRegisteredCommands;
		}
		// MDH@18JUN2020: NOTE commandIndex now stored in any command will equal 0 when it is a new command, so when it is being stored commandIndex will tell us whether it is a new command or not
		//                as soon as we store the current command and it is a new command we store the index of the command i.e. where it is located in the list of registered commands
		// if(commandIndex>0)command->sourceCommandIndex=(commandCount-commandIndex+1);
		_registeredcommands[commandCount]=(Mregisteredcommand){owned_command(disowned_command(command,owner_command),owner_registeredcommands)};
		if(commandIndex>0)_registeredcommands[commandCount].previousCommandIndex=commandCount-commandIndex+1; // will be positive for any positive commandIndex, because commandIndex is in [1,commandCount-1)
		commandCount++;
		if(amVerboseDebugging())outputLine("Command registered!");
		return true;
	}
	// should be added to the function body
	// NOTE we can create the value and when it is not appended to the list it will not be bound, and be released by the 'garbage collector'
	// MDH@25MAY2020 TODO should we pass {1} to _getValueOfToken()?????????
	// MDH@08JUN2020 TODO still have to check the following... what would be the owner of the first command token??????? I suppose it will be subowned by the user input command
	// MDH@17JUN2020 if we store the first command token in a value it needs to be disowned so it can be owned by the value 
	//               NOTE that _getValueOfToken will free the token (and all connected tokens) when failing to create the value
	//               we need to determine what to do with the command itself
	Mvalue* _commandToEvaluateTokenValue=_getValueOfToken(disowned_token(command->_firstToken,owner_command)); // MDH@12JUN2020: TODO as we do NOT want to loose _firstToken we do NOT disown it before asking for the value of the token
	if(_commandToEvaluateTokenValue){ // the first command token is now bound (or otherwise released)
		// MDH@22MAY2020: __list creates a list that is to be subowned by the function in the current function body input
		if(!getCurrentFunctionBodyInput()->_function->_bodyCommandList)
			getCurrentFunctionBodyInput()->_function->_bodyCommandList=
				owned_list(__list("body command list"),Msubowner(owner_currentFunctionBodyInput,2));
		// MDH@08JUN2020: if we succeed in adding the command value to the function body we still return false as result which will result in command to be freed
		//                BUT by NULLing command->_firstToken we prevent the tokens from being freed in the command as we should
		if(appendedToList(getCurrentFunctionBodyInput()->_function->_bodyCommandList,owner_currentFunctionBodyInput,_commandToEvaluateTokenValue,M_LL_INVALID)>0)
			command->_firstToken=NULL;
	}
	outputError("Failed to add the command to the body of the function");
	return false;
}
// tokenizer constants moved over to Mshell.h

/* MDH@11AUG2019: NOT doing the following anymore, instead we store the identifier information in the tokens themselves
// MDH@06AUG2019: I have to keep track of variable initializations in the current command so that I know when a variable is new or not
//                analysis: variables used in a command typically refer to variables created in previous commands
//                i.e. to variables kept in the current execution environment
//                but we want to allow for local variables like the counter in a for loop or local to a group of commands as now possible in a do() function call
//                and probably also in a user function call
//                so technically calls to the predefined functions called for, do and function can have arguments in which local variables are defined
//                these local variables should be considered to exist for the duration of the call i.e. in all following arguments of the call when being entered, so these variables do not need to be 'global'
//                in case of for the first argument contains the local variable initializations, we can do the same in the do function call, in function its the second argument (where the first argument represents the name of the function)
//                this means that we need to know in which argument of any for, do or function call when it is being entered, of course with function we could create a function by assigning it to a variable name instead of defining the function name as first argument
//                in which case the first argument would be come the argument with the local variables, which would be more convenient!!!!
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
bool isBinaryOperatorTokenType(uint8_t tokenType){
	return(TOKENTYPE_IDS[tokenType]>>4)==0b0110;
}

// removeLastToken() removes the last user input command token, delegating the actual removal to removeLastCommandToken now defined in Mshell.h/c
bool removeLastUserInputCommandToken(){
	if(!_userInputCommand||!_userInputCommand->_lastToken)return false;
	// MDH@20SEP2019: if a token is removed, we also need to remove any associated feed forward text associated with the token
	deleteTokenAutocompletionText(_userInputCommand->_lastToken);
	removedLastCommandToken(_userInputCommand,owner_userInputCommand); // NOT using the result (which would be the new last command token)
	return true;
}

// MDH@04NOV2019: moved from line 1800 or so over here as it calls removeLastUserInputCommandToken() and we do not like to have to use prototypes TODO remove all prototype() definitions
void updateUserInputCommandIdentifierContinuation(){Mallocationowner owner=getOwner(__LINE__);
	// MDH@30OCT2019: simplified updating the identifier continuation a bit so wee do not need to be afraid that it won't work AND we no longer need the userInputCommandIdentifierContinuationNeedsUpdating flag!!!!
	//                BUT right after a delete we should be allowed to set the flag so the continuation will be deleted and nothing more
	//                OK, to make delete work, I have to check the NeedsUpdating flag which should be set to true when the token is set
	deleteIdentifierContinuation();
	// if currently in an identifier, we technically need updating the identifier continuation unless the NeedsUpdating flag has been turned off
	bool canHaveAnIdentifierContinuation=inIdentifierToken(_userInputCommand?_userInputCommand->_lastToken:NULL);
	if(canHaveAnIdentifierContinuation){ // theoretically we could have identifier continuation
		if(userInputCommandIdentifierContinuationNeedsUpdating){ // not blocked
			// MDH@04NOV2019: if inside a reference, skip the reference 'operator' at the start of the reference when requesting completion text
			Mstring* _completionText=owned_string(_userInputCommand->_lastToken->type!=TT_REFERENCE?_getCompletion(string(_userInputCommand->_lastToken->text),true):_getCompletion(string_remainder(_userInputCommand->_lastToken->text,1),false),owner);
			// need at least two characters (the type and something text behind it)
			// OOPS if there's only one character in the text (just the type) which is quite possible we will need to free _completionText even then!!!
			if(_completionText){
				if(string_length(_completionText)>1){
					switch(string_char(_completionText,0)){
						case 1:
							_identifierContinuationCharacters=strdup(string(_completionText)+1);
							if(!_identifierContinuationCharacters)inputError("Failed to create the identifier continuation");else if(amVerbose())inputInfo("Identifier continuation: '%s'.",_identifierContinuationCharacters);
							break;
						case 2:
							_identifierContinuationOptionalCharacters=strdup(string(_completionText)+1);
							if(_identifierContinuationOptionalCharacters)inputInfo("Identifier continuation characters: %s.",_identifierContinuationOptionalCharacters);else inputError("No identifier continuation characters!");
							break;
					}
				}else{
					if(_userInputCommand->_lastToken->type==TT_REFERENCE){
						// MDH@04NOV2019: if we mark the entire reference token as error, we're going to have problems recovering it, so it would make sense to mark the character that caused not having a completion text anymore as erroneous
						//                so we should cut off that last character and put it in an error token, ready to be removed again
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
										Mtoken* _errorToken=_getNewCommandToken(_userInputCommand->_lastToken,TT_ERROR); // by passing in NULL all the complicated stuff is not happening!!!
										if(_errorToken){
											_userInputCommand->_lastToken=_errorToken; // update the last token assuming we will succeed in doing what needs doing
											if(!string_append_char(_errorToken->text,c)){
												if(removeLastUserInputCommandToken())
													inputError("Failed to mark the last invalid reference character as erroneous because it cannot result in a reference to an existing variable.");
												else
													inputError("%sFailed to undo failing to mark the last character as erroneous.",M_BUG_PREFIX);
											}else
												reoutputToken(_errorToken);
										}else
											inputError("The supposed reference can never become an existing variable reference.");
										// if we failed to create the error token, we have to append the removed character again (should be no problem because Mstring does not reduce the memory when deleting characters from the end)
										if(_errorToken!=_userInputCommand->_lastToken){
											if(!string_setlength(_userInputCommand->_lastToken->text,lastTokenLength/*,owner_lastTokenText*/))
											{inputError("%sCouldn't undo the adjustments made to an erroneous reference.",M_BUG_PREFIX);}
										}
									}else
										inputError("Failed to retrieve the last (erroneous) character in a variable reference.");
								}else
								if(amVerboseDebugging())
									inputInfo("Reference complete!");
							}else
								inputInfo("%sLast character in reference vanished.",M_BUG_PREFIX);
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
	if(amDebugging())inputInfo("User input command identifier continuation updated.");
}/* VALIDATED */

Mstring* _getCommandText(bool color){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _commandText=owned_string(__string(),owner);
	if(_commandText){
		Mtoken* commandToken=_userInputCommand->_firstToken; // TODO can we get rid of using commandcount-1 here????
		while(commandToken){
			// if we bump into a comment we're done!!!
			if(commandToken->type==TT_COMMENT)break;
			// TODO there must be a better way to do the coloring!!!
			if(color){string_append(_commandText,ES"38;5;");string_append(_commandText,getTokenColor(commandToken->type));string_append_char(_commandText,'m');} // assuming the same back color is used on ALL tokens, so we won't have to pass that along
			// MDH@31OCT2019: for now decided NOT to show the whitespace inside the tokens (by replacing the first whitespace character with the end-of-string marker)
			char firstWhitespaceTokenCharacter=(isTokenFinished(commandToken)?string_replacedchar(commandToken->text,'\0',getTokenSignificantCharacterCount(commandToken)):'\0');
			string_append(_commandText,string(commandToken->text));
			if(firstWhitespaceTokenCharacter)string_setchar(commandToken->text,firstWhitespaceTokenCharacter,getTokenSignificantCharacterCount(commandToken)); // put first whitespace character (if any) back
			// MDH@03MAY2019: place an asterisk in front of the type to indicate that expr is NOT null!!
			if(amAssisting()){
				if(color){string_append(_commandText,ES"38;5;");string_append(_commandText,getInfoColor());string_append_char(_commandText,'m');}
				string_append_char(_commandText,'(');if(commandToken->expr)string_append_char(_commandText,'*');string_append(_commandText,TOKENTYPE_STRING[commandToken->type]);string_append(_commandText,") ");
			}
			commandToken=commandToken->next;
		}
		if(color)if(!amAssisting()){string_append(_commandText,ES"38;5;");string_append(_commandText,getInfoColor());string_append_char(_commandText,'m');} // reset to info color
	}
	return disowned_string(_commandText,owner);
}

void clearCommand(){
	FREE_COMMAND(_userInputCommand,owner_userInputCommand);_userInputCommand=NULL; // MDH@28OCT2019: using the command now...
	numberOfLineCommandCharacters=0;free_userinputline(); // MDH@24SEP2020: both essential BRO'
	/* replacing:
	pLastCommandToEvaluate=NULL;
	// a small precaution here!!!
	if(_userInputCommand->_firstToken){freeToken(_userInputCommand->_firstToken);_userInputCommand->_firstToken=NULL;}
	*/
}
// TODO find a way to not have to replicate as we do now what getValueText() is also doing (but without coloring of course)
// MDH@13MAR2020: result type changed to size_t because now returning the number of characters written
size_t outputValueColored(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	if(_value){
		switch(_value->type){
			case VT_UNDEFINED:outputTokenTypeColor(TT_DQSTRING);written=output("%s",M_UNDEFINED_VALUE_TEXT);break; // let's use the same color as for double quotes string (for now)
			case VT_TOKEN:outputTokenTypeColor(_value->value._token->type);written=output("%s",string(_value->value._token->text));break; // easy the token type determines the color to use!!!
			case VT_INTEGER:outputTokenTypeColor(TT_INTEGER);written=outputValue(NULL,_value,NULL);break;
			case VT_BIGINTEGER:outputTokenTypeColor(TT_INTEGER);written=outputBiginteger(NULL,_value->value._biginteger,NULL);break;
			case VT_DECIMAL:outputTokenTypeColor(TT_REAL);outputDecimal(NULL,_value->value._decimal,NULL);break;
			case VT_RATIONAL:
				if(_value->value._rational){
					written+=outputChar('(');
					outputTokenTypeColor(TT_INTEGER);written+=outputBiginteger(NULL,_value->value._rational->num,NULL);resetOutputColor();
					written+=outputChar('/');outputTokenTypeColor(TT_INTEGER);
					if(_value->value._rational->den)written+=outputBiginteger(NULL,_value->value._rational->den,NULL);else written+=outputChar('1'); // a missing denominator means it's equal to 1
					resetOutputColor();
					written+=outputChar(')');
					if(_value->value._rational->delta){
						outputTokenTypeColor(TT_REAL);
						if(_value->value._rational->delta->ld>=0)written+=outputChar('+');
						Mstring* _realValueText=owned_string(__string(),owner);
						if(_realValueText){
							appendld(_realValueText,_value->value._rational->delta->ld);
							written+=output("%s",string(_realValueText));
							FREE_STRING(_realValueText,owner);
						}
						// replacing:	output("%.*Lf",LDBL_DIG,_value->value._rational->delta->ld);
						resetOutputColor();
					}
				}
				break;
			case VT_FLOAT:outputTokenTypeColor(TT_REAL);written+=outputValue(NULL,_value,NULL);break;
			case VT_TEXT:outputTokenTypeColor(_value->value._text->presuffix=='"'?TT_DQSTRING:TT_SQSTRING);written+=outputValue(NULL,_value,NULL);break;
			case VT_LIST:
				// TODO not using _getListText() as defined in Mexecution
				/////////if(amVerbose())outputValue("List value '",_value,"'.");
				{
					written+=outputChar('[');
					Mlist* _list=_value->value._list;
					if(_list&&_list->numberOfElements){
						Mlistelement* _listelement=_list->_first;
						unsigned long long listitemindex=1;
						while(_listelement){
							if(_listelement->index)while(listitemindex<_listelement->index){listitemindex++;written+=outputChar(',');} // missing elements
							written+=outputValueColored(_listelement->_value);
							_listelement=_listelement->_next;
						}
					}
					written+=outputChar(']');
				}
				break;
			case VT_MAP:
				{
					written+=outputChar('{');
					Mmap* _map=_value->value._map;
					if(_map&&_map->numberOfElements){
						Mvariable* _mapelementvariable;
						Mmapelement* _mapelement=_map->_first;
						while(_mapelement){
							_mapelementvariable=_mapelement->_variable;
							// TODO are we coloring the name?????
							// quoting the name to indicate it is alphanumeric!!
							written+=output("%c%s%c%c",'\'',_mapelementvariable->_name,'\'',':');
							written+=outputValueColored(_mapelementvariable->_value);
							if(!_mapelement->_next)break;
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
					Mstring* _valueText=owned_string(_getValueText(_value,false),owner);
					if(_valueText){written+=output("%s",string(_valueText));FREE_STRING(_valueText,owner);}
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

void unfinishCommandToken(Mtoken* commandToken){
	// MDH@03SEP2019: adjusted so that not only the unary operators are left finished but all one character token types (which are the only tokens that are immediately finished once a single character is entered!!)
	//                NOTE unfinishing of the current token is done so that the token can be continued, so technically we should unfinish all tokens that can be continued after a character is removed from them
	// for all non-unary token that we are in now that is finished, unfinish 
	// TODO there are other one-character tokens
	// TODO is the test string_length(commandToken->text)==getTokenSignificantCharacterCount(commandToken) always correct?
	//      NOTE I think it is because if that is the case there is no whitespace at the end of the token and we
	//           should unfinish the token so we can append to it again
	//           also 'one character' token types are always finished immediately, so we should not unfinish those!!!
	if(commandToken)
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
long long allocationMarksAdded=0;

// anything the user types is a sequence of tokens which we can store in a linked list
// MDH@14NOV2019: passing in the address for storing the Mvalue* of the evaluation result
//                instead of returning a bool we could return the command text (or NULL if failing to do so????)
bool evaluateCommand(Mvalue* *resultValue){Mallocationowner owner=getOwner(__LINE__);
	
	/// NOT HERE!! outputChar('\n'); // indicating that the command is being evaluated!!!
	int8_t aValidCommandIndicator=isAValidCommandIndicator(_userInputCommand,owner_userInputCommand,true);
	if(aValidCommandIndicator<=0){
		// MDH@20FEB2020: this is what we did in isAValidCommand() before, but removed from it: if the command ends with an error, we remove the error token, unfinish the (new) last token, so we can re-use it
		if(_userInputCommand&&_userInputCommand->_lastToken&&_userInputCommand->_lastToken->type==TT_ERROR){
			removeLastUserInputCommandToken();
			if(!_userInputCommand->_lastToken)_userInputCommand=NULL;else unfinishCommandToken(_userInputCommand->_lastToken);
		}
		//output("%sInvalid command indicator: %d.\n",M_ERROR_PREFIX,aValidCommandIndicator);
		return false;
	}

	allocationMarksAdded=0;
	if(amVerbose()){
		if(allocationMarkAdded())allocationMarksAdded++;else outputError("Failed to mark the allocation before evaluating the command."); // mark the allocations at the start of evaluating a command!!!
	}

	if(amDebugging())
		output("Number of allocated/freed formula elements before evaluating the command: (%zd,%zd).\n",getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));

	// evaluating means getting the value of the expression that _userInputCommand->_firstToken points to
	// NOTE that the first token is always a dummy token (which will at most contain the whitespace at the start of the command)
	Mstring* _commandText=owned_string(_getCommandText(true),owner); // MDH@13MAR2020 TODO determine later???????
	// plug the token following the dummy starting token of the command into the current execution environment (typically _Menvironment I suppose)
	clock_t before_evaluating=clock();
	getExecutionEnvironment()->expressionToken=_userInputCommand->_firstToken->next; // initialize the (current) expression token
	*resultValue=getValueOfExpression("command",'e',(TokenType[]){},0);
	long long elapsed_evaluating=(clock()-before_evaluating)/1000;

	resetOutputColor(); // MDH@02OCT2019: given that the out() might've been used to write stuff to the console in weird colorings TODO doesn't seem to help	
	if(elapsed_evaluating>0)output("The evaluation took %lld ms.\n",elapsed_evaluating); // MDH@13MAR2020: because the output text can take long to show
	
	Mstring* _showResultTimestamp=owned_string(_getTimestamp(NULL),owner);
	if(_showResultTimestamp){outputToFile("@",string(_showResultTimestamp),":\n");FREE_STRING(_showResultTimestamp,owner);}else outputError("Failed to obtain a timestamp");
	
	echoToOutputFile();

	// output the commandText
	if(_commandText){output("%s",string(_commandText));FREE_STRING(_commandText,owner);}else output("%sFailed to obtain the command result text",M_ERROR_PREFIX);
	output(" = ");

	// if the result is a null value, show the NULL_value
	clock_t before_writing=clock();size_t written=outputValueColored(isValueNull(*resultValue)?NULL_value:*resultValue);long long elapsed_writing=(clock()-before_writing)/1000;
	
	newline(); // outputValueColored() doesn't do that!!
	
	Mstring* _doneShowingResultTimestamp=owned_string(_getTimestamp(NULL),owner);
	if(_doneShowingResultTimestamp){outputToFile(">",string(_doneShowingResultTimestamp),"\n");FREE_STRING(_doneShowingResultTimestamp,owner);}else outputError("Failed to obtain a timestamp");
	
	dontEchoToOutputFile();

	if(elapsed_writing>0)output("The writing took %lld ms.\n",elapsed_writing);

	///// outputValueColored() does do this (and should): resetOutputColor();
	if(written>1000)if(elapsed_evaluating>0)output("The evaluation took %lld ms.\n",elapsed_evaluating);/////////else output("less than 1 ms.");

	///////////////decrementReferenceCount(_commandExpressionValue); if(amVerbose())outputInfo("Result released!"); // TODO do we need to do this?????

	///////if(amVerbose())outputInfo("Command to release!");

	if(amDebugging())
		output("Number of allocated/freed formula elements after evaluating the command: (%llu,%llu).\n",getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));

	////////if(amVerbose())outputInfo("Command released!");
	return true;

}

// MDH@24APR2019: outputCommand() writes the command to evaluate, and sets _userInputCommand->_lastToken in the process
// MDH@31OCT2019: there's a complication when the last character in the token is the newline character
// MDH@24SEP2020: now returning the position on the last command line 
size_t outputCommand(Mcommand * const command){
	Mtoken* token=(command?command->_firstToken:NULL);
	Mcursormovement cursormovement={}; // MDH@24SEP2020: outputToken will update cursormovement accordingly
	while(token){
		token->position=cursormovement.position; // MDH@24SEP2020: adding this because the prompt length might've changed in which case token->position would not be correct anymore
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
size_t outputTokenText(Mtoken* _token){if(_token){outputTokenColor(_token);return output("%s",string(_token->text));}return 0;}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t createUserInputCommandPage){
	commandPage=createUserInputCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Mtoken* token=_registeredcommands[lastCommandToShowIndex+commandToShowIndex]._command->_firstToken;
		// MDH@24SEP2020: this is definitely an issue because outputToken() assumes the token is written on the command line which in this case is not the case, so we shouldn't use outputToken()
		while(token){
			outputTokenText(token); // write the token text in the color of it's type
			// replacing: outputToken(token);
			token=token->next;
		}
		outputChar('\n');
	}
	resetOutputColor();
	output("%s","Select the last digit of the command to use, or the up/down key to show the next/previous page.");
	output("%c%c%c%c",' ','>','>',' '); // TODO what kind of prompting do we want to do???
}
void showNextCommandPage(){
	if(commandPage<commandPages)
		setCommandPage(commandPage+1);
	else
		output("%s","No further commands to show.");
}
void showPreviousCommandPage(){
	if(commandPage>1)
		setCommandPage(commandPage-1);
	else
		output("%s","No further commands to show.");
}
// MDH@25NOV2019: it's more convenient to output the values in control mode than asking for it in command mode
//                as values() command would affect what is returned (given that it's a command)
void outputValues(){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _valuesTable=owned_list(_getValuesTable(NULL),owner);
	if(_valuesTable){
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
void setInputMode(enum INPUTMODE_ENUM newInputMode){
	inputMode=newInputMode;
	deleteTokenautocompletiontexts();
	// in command mode the behind cursor text is determined JIT, whereas it is actively used in the other modes!!!
	if(_suggestedText)string_setlength(_suggestedText,0/*,owner_suggestedText*/);
}
char switchToControlMode(char* message){
	if(inputMode!=IM_CONTROL){
		if(inputMode==IM_COMMAND)clearCommand();
		if(message!=NULL){newline();setColor(getErrorColor());outputLine(message);} // MDH@01OCT2019: message will typically be an error so
		resetOutputColor();
		setInputMode(IM_CONTROL);
		if(amVerbose())outputValues(); // MDH@25NOV2019: it's convenient to also output the values when verbose
		outputVariables(); // immediately show the list of available variables (so we can then use v for verbose flag, AND s is available for Shell again!!!!)
	}
	///////////outputFlags(); // show the user the current flags!!
	return 'o'; // to make the loop know to quit
	//output("%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}

size_t getNumberOfAutocompletionCharacters(){return string_length(_autoCompletionText);} // TODO assuming _autoCompletionText has actually been constructed!!!!
size_t getNumberOfTokenAutocompletionTexts(){
	size_t numberOfTokenAutocompletionTexts=0;
	Mtokenautocompletiontext* tokenautocompletiontext=_firstTokenautocompletiontext;
	while(tokenautocompletiontext){numberOfTokenAutocompletionTexts++;tokenautocompletiontext=tokenautocompletiontext->_next;}
	return numberOfTokenAutocompletionTexts;
}

// MDH@23SEP2020: perhaps more convenient to keep track of any cursor displacement
void outputManualFeedforwardCharacters(Mcursormovement* _cursormovement){
	if(!_cursormovement)return;
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
		if(!string_append(_suggestedText,string(_manualFeedforwardText)))_cursormovement->written=0;
	}
}
void outputIdentifierContinuationTextCharacters(Mcursormovement* _cursormovement){
	if(!_cursormovement)return;
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	size_t numberOfIdentifierContinuationCharactersWritten=(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0);
	// MDH@04OCT2019: append it to the suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0&&!string_append(_suggestedText,_identifierContinuationCharacters))numberOfIdentifierContinuationCharactersWritten=0; // append to suggested text
	if(numberOfIdentifierContinuationCharactersWritten>0)outputCommandLineText(_identifierContinuationCharacters,_cursormovement,getIdentifierContinuationTextColor(),-1);
		// replacing: {setColor(getIdentifierContinuationTextColor());output("%s",_identifierContinuationCharacters);
		// replacing: {size_t numberOfIdentifierContinuationCharactersToWrite=numberOfIdentifierContinuationCharactersWritten;while(numberOfIdentifierContinuationCharactersToWrite){outputChar(' ');numberOfIdentifierContinuationCharactersToWrite--;}
	// return numberOfIdentifierContinuationCharactersWritten; // one less character written than the computed length!!!
}
void outputImmediateFeedforwardCharacters(Mcursormovement* _cursormovement){
	if(!_cursormovement)return;
	unsigned long long numberOfImmediateFeedforwardCharactersWritten=(_immediateFeedforwardText?string_length(_immediateFeedforwardText):0);
	if(numberOfImmediateFeedforwardCharactersWritten>0)if(!string_append(_suggestedText,string(_immediateFeedforwardText)))numberOfImmediateFeedforwardCharactersWritten=0; // append to suggested text
	if(numberOfImmediateFeedforwardCharactersWritten>0)outputCommandLineText(string(_immediateFeedforwardText),_cursormovement,getFeedForwardTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output(string(_immediateFeedforwardText));}
}
void outputAutocompletionCharacters(Mcursormovement* _cursormovement){
	if(!_cursormovement)return;
	unsigned long long numberOfAutocompletionCharactersWritten=(_autoCompletionText?string_length(_autoCompletionText):0);
	if(numberOfAutocompletionCharactersWritten>0)if(!string_append(_suggestedText,string(_autoCompletionText)))numberOfAutocompletionCharactersWritten=0; // append to suggested text
	if(numberOfAutocompletionCharactersWritten>0)outputCommandLineText(string(_autoCompletionText),_cursormovement,getFeedForwardTextColor(),-1);// replacing: {setColor(getFeedForwardTextColor());output("%s",string(_autoCompletionText));}
}
/* replacing:
// MDH@22SEP2020: when suggested text is output on the command input line we need to embed prompt length blanks
//                there's already a outputCommandLineText() function we can call
//                added a position size_t argument that should tell the function where the cursor is currently located
unsigned long long getNumberOfManualFeedforwardCharactersWritten(uint16_t position){
	// MDH@27SEP2019 doesn't update the identifier continuation anymore (as it might be optional and is moved over to writeBehindCursorText) removing: updateUserInputCommandIdentifierContinuation();
	unsigned long long numberOfManualFeedforwardCharactersWritten=position; // MDH@23SEP2020: by default returning the original position (and no characters output)
	// MDH@04OCT2019: append it to the suggested text
	if(string_length(_manualFeedforwardText)>0){
		if(numberOfIdentifierContinuationManualFeedforwardCharacters>0){
			char c=string_replacedchar(_manualFeedforwardText,'\0',numberOfIdentifierContinuationManualFeedforwardCharacters);
			// MDH@22SEP2020: the problem is that outputCommandLineText() does NOT return the number of characters
			//                written, but the new position but this we can change
			//                the first 16 bits contain the new position and the remaining 48 bits the number of characters written
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
unsigned long long getNumberOfAutocompletionCharactersWritten(uint16_t position){
	unsigned long long numberOfAutocompletionCharactersWritten=(_autoCompletionText?string_length(_autoCompletionText):0);
	if(numberOfAutocompletionCharactersWritten>0)if(!string_append(_suggestedText,string(_autoCompletionText)))numberOfAutocompletionCharactersWritten=0; // append to suggested text
	if(numberOfAutocompletionCharactersWritten>0){ // additional auto completion text to write
		/////debugWrite("Auto completion characters to write: '%s'.",string(_autoCompletionText));
		numberOfAutocompletionCharactersWritten=outputCommandLineText(string(_autoCompletionText),position,getFeedForwardTextColor());
		// replacing: setColor(getFeedForwardTextColor());output("%s",string(_autoCompletionText));
	}
	return numberOfAutocompletionCharactersWritten;
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
	if(updateUserInputCommandIdentifierContinuationText)updateUserInputCommandIdentifierContinuation(); // force an update of the identifier continuation (could have been already done in updateLastTokenAutocompletionText()!)
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

// MDH@01OCT2019: it's better to show the suggested text JIT i.e. just before asking the user for input
//                this is also better because at that moment we know the user should be seeing it
unsigned long long numberOfSuggestedCharactersWritten=0; // MDH@23SEP2020: basically storing both line position and number of characters written but corrected in showSuggestedText
// MDH@30JUN2020: the suggested text can also soft-break onto next lines!!!!
void showSuggestedText(){
	// ASSERT _suggestedText should not be NULL
	// MDH@26SEP2019: behind cursor text now consists of two parts now: identifier continuation text and feed forward text
	// 0. preparation
	string_setlength(_suggestedText,0/*,owner_suggestedText*/); // clear the suggested text!!!
	
	// 0. the current cursor position (in the command) is the number of line command characters
	Mcursormovement cursormovement={numberOfLineCommandCharacters};
	if(string_length(_manualFeedforwardText)>0)
		outputManualFeedforwardCharacters(&cursormovement);
	else
		outputIdentifierContinuationTextCharacters(&cursormovement);

	outputImmediateFeedforwardCharacters(&cursormovement);

	outputAutocompletionCharacters(&cursormovement);

	numberOfSuggestedCharactersWritten=cursormovement.written+cursormovement.skipped;

	if(amVerboseDebugging())
		numberOfSuggestedCharactersWritten+=output("[%zd-%zd=%zd,%zd]",getUserInputLength(),numberOfLineCommandCharacters,(_userinputline?_userinputline->offset:0),(_userinputline?_userinputline->index:0));

	moveCursorLeft(numberOfSuggestedCharactersWritten);

	/* replacing:
	unsigned long long cursorPosition=numberOfLineCommandCharacters+promptLength;
	// doesn't seem to work: outputControlText("s"); // i.e. store the current cursor position

	// 1. write the identifier continuation first, then the manual feed forward, the immediate feed forward characters and finally the auto completion text
	resetOutputColor();
	unsigned long long suggestedCharactersWritten=0; // for storing the result of writing suggested characters (position/number of characters written combination stored in 16/48 bits of a unsigned long long)
	if(string_length(_manualFeedforwardText)>0)
		suggestedCharactersWritten=getNumberOfManualFeedforwardCharactersWritten(cursorPosition);
	else
		suggestedCharactersWritten=getNumberOfIdentifierContinuationTextCharactersWritten(cursorPosition);
	cursorPosition=(suggestedCharactersWritten&0xFFFF); // first 16 bits
	numberOfSuggestedCharactersWritten=(suggestedCharactersWritten>>16); // remove the position part
	
	suggestedCharactersWritten=getNumberOfImmediateFeedforwardCharactersWritten(cursorPosition);
	cursorPosition=(suggestedCharactersWritten&0xFFFF); // first 16 bits
	numberOfSuggestedCharactersWritten+=(suggestedCharactersWritten>>16); // remove the position part
	
	suggestedCharactersWritten=getNumberOfAutocompletionCharactersWritten(cursorPosition);
	cursorPosition=(suggestedCharactersWritten&0xFFFF); // first 16 bits
	numberOfSuggestedCharactersWritten+=(suggestedCharactersWritten>>16); // remove the position part
	// 2. and back to where the cursor is supposed to be
	// TODO this is an issue
	// doesn't seem to work: outputControlText("u"); // restoring the cursor position
	moveCursorLeft(numberOfSuggestedCharactersWritten); // return to where the command ends
	*/
	
	// 3. finalize: remember the actual number of characters written (and therefore will not be blanks)
	// update numberOfBehindPromptCharactersWritten (we do not need to remember blanks written!!!å)
	outputUserInputCommandTokenColor(); // replacing: if(_userInputCommand->_lastToken)outputTokenColor(_userInputCommand->_lastToken); // return to the color of the current token
}
// MDH@23SEP2020 TODO: hiding the suggested text involves removing the prompt characters embedded in the suggested text outputted
void hideSuggestedText(){
	long long numberOfBlanksToWrite=numberOfSuggestedCharactersWritten;
	while(--numberOfBlanksToWrite>=0)outputChar(' ');
	///////outputChar(' ');
	moveCursorLeft(numberOfSuggestedCharactersWritten);
}

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

void backToPrompt(){
	// MDH@27SEP2019: assuming for now that whatever is written next will determine the number of characters written behind the prompt
	numberOfBehindPromptCharactersWritten=0; // assume no text characters written so far
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	// replacing: restoreCursor();clearScreenFromCursor();

	// MDH@31OCT2019: with a clearScreenFromCursor() following it suffices to first move to the initial prompt line
	size_t linesfreed=free_userinputline();
	while(linesfreed>0){linesfreed--;oneLineUp();}
	toStartOfLine();moveCursorRight(promptLength); // should now be at the right position for clearing
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
//                1. clear the screen before we rewrite the entire command (if we do not know the number of lines the command now occupies),  
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
		//                in essence this means that we should actually accept that there's ONE additional position at the end of a line that is available for a character even though we are not using it
		//                e.g. imagine that the user would decrement the viewport width by 1 this would NOT result in extra command lines because if it changed from 20 to 19 we'd have at most 19 characters on the line
		//                which fit perfectly
		// only when the new number of line characters is smaller should we redetermine how many lines back the prompt is
		// if it is larger we assume that we the number of command lines did not change (visually)
		size_t numberOfExtraCommandLines=0;
		if(newNumberOfLineCharacters<numberOfLineCharacters){
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
						// we know the next token is on the next line
						numberOfLineCommandCharacters=0;
					}
				}
				// determine the new line and position based on the last token
				token=token->next;
			}
		}
		// for every extra command line we go one line up
		while(numberOfExtraCommandLines>0){oneLineUp();numberOfExtraCommandLines--;}
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
		
		clearScreenFromCursor(); // TODO can't harm but not certain about this
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

// MDH@20SEP2019: whenever a token changes the associated feed forward text might change, so it makes sense to update the feed forward text accordingly
//                assuming that the given type is correct (e.g. an identifier of which has been determined whether it is a function or variable name)
//MDH@26SEP2019: due to the separation of the identifier continuation text and the other feed forward text we separate getting the identifier continution text from getting the other feed forward text
void updateLastTokenAutocompletionText(){
	// MDH@27SEP2019: updating the identifier continuation now moved to writeSuggestedText(true), so JIT update
	// MDH@25MAY2020: TODO _getLastTokenAutoCompletionText() returns a dynamically allocated char* (using strdup) which therefore is NOT under version control
	free(setLastTokenAutocompletionText(_getLastTokenAutoCompletionText())); // MDH@24SEP2019: the bool forces setting the identifier continuation characters when available, so we can show them to the user
	// inputInfo("Last token auto completion text updated.");
}

void setUserInputCommand(Mcommand* command){
	// NOTE command is either an existing command (commandIndex>0) or a new command (commandIndex=0)
	//      so if it is a registered command we should NOT obtain ownership
	_userInputCommand=command; // MDH@24MAY2020: take over ownership!!!!
	// replacing: _userInputCommand->_lastToken=_userInputCommand->_firstToken=pCommand;
	numberOfLineCommandCharacters=outputCommand(_userInputCommand); // MDH@24SEP2020: essential to 'sync' numberOfLineCommandCharacters to the number of command characters on the last command line
	// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=(_userInputCommand?inIdentifierToken(_userInputCommand->_lastToken):false); // MDH@02OCT2019 because we're setting _userInputCommand->_lastToken but not calling setLastUserInputCommandToken()
	//////////writeSuggestedText(true);
	// MDH@06AUG2019 TODO: determine the initializations associated with a stored command!!!
	////////// removing: determineCommandInitializations();
}
void deleteUserInputCommand(){
	numberOfLineCommandCharacters=0; // TODO is this the right place to do this?????? the principle should be that whenever _userInputCommand changes, we should compute numberOfLineCommandCharacters a new
	if(!_userInputCommand)return;
	// if _userInputCommand is a registered command, it should NOT be freed
	if(commandIndex==0)FREE_COMMAND(_userInputCommand,owner_userInputCommand);
	_userInputCommand=NULL;
}

/**
 * setCommandIndex() accepts \p createUserInputCommandIndex between 0 and commandCount at most
 * but 0 is now also accepted, returning to show _userInputCommand->_firstToken (if any)
 */
void setCommandIndex(uint32_t createUserInputCommandIndex){Mallocationowner owner=getOwner(__LINE__);
	// MDH@25MAY2020: we should 'delete' the user input command BEFORE updating commandIndex because the current command index determines whether we're dealing with a new command or an existing command
	//                we used to do that after clearing the screen below
	clearInfo();
	deleteUserInputCommand();
	commandIndex=createUserInputCommandIndex;
	////if(amVerbose())inputInfo("Command index %lld.",commandIndex);
	// it's easier to go to the beginning of the line although we could be on the line below!!!!
	// replacing: 
	backToPrompt();
	clearScreenFromCursor();
	// MDH@29OCT2019: we have to do the following because otherwise inputInfo() will jump back to the end of the command instead of right behind the prompt!!!!
	//                NOTE typically _userInputCommand will not be NULL when we're scrolling through the list of previous commands!!!!
	// MDH@24APR2019 obsolete: getCommandLength()=getUserInputLength()=0; // do we need this????
	// TODO do we need to do this: clear the behind cursor text (in any situation)
	deleteAutocompletionText(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	if(commandIndex>0){
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
				// because the last token is NULL, we can use setLastTokenAutocompletionText to register _commandFeedforward used as the entire feed forward text
				setLastTokenAutocompletionText(string(_commandFeedforward));
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
bool commandDown(){
	if(commandCount==0)return false;
	setCommandIndex(commandIndex<commandCount?commandIndex+1:0);
	return true;
}
bool commandUp(){
	// MDH@26FEB2019: instead of stopping at the start of the commands it's better to move back to the new command which is at commandCount
	if(commandCount==0)return false;
	setCommandIndex(commandIndex>0?commandIndex-1:commandCount);
	return true;
}
/*
// MDH@23SEP2019: whenever the type of the current token (_userInputCommand->_lastToken) changes (possibly with the start of a new token), so will the feed forward text associated with that token
//                therefore it is best to set the last token type using a separate function
// MDH@03OCT2019: every time the token type changes we need to sync the immediate feed forward text as well!!!!
void setTokenType(Mtoken* token,TokenType tokenType){
	if(token){
		if(tokenType!=token->type){
			// MDH@04OCT2019 moved to input loop removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the current token immediate feed forward text.");
			token->type=tokenType;
			// MDH@04OCT2019 moved to input loop removing: if(!updateImmediateFeedforwardTextOfUserInputCommand())inputError("Failed to add the current token immediate feed forward text.");
		}
	}
	///////// MDH@29OCT2019 probably don't need this here anymore: if(endOfInput)updateLastTokenAutocompletionText();
}
*/
/*
// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType){
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//                but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=_getToken(lastCommandToken,tokenType);
	if(_newCommandToken)setTokenType(_newCommandToken,tokenType);else inputError("Failed to create a command token");
	return _newCommandToken;
}
*/
void createUserInputCommand(){
	// MDH@24APR2019 obsolete: getCommandLength()=string_length(feedforwardText); // MDH@21APR2019: oops was 0 before...
	resetOutputColor(); // TODO do we need this here?????
	// if(amDebugging())inputInfo("Creating the new user input command.");
	// MDH@23SEP2019: createUserInputCommandToken() added to take care of updating _userInputCommand->_lastToken (should be NULL as it is used to represent the previous last token)
	_userInputCommand=owned_command(_getNewCommand(true),owner_userInputCommand);
	// MDH@29OCT2019: the following is absolutely silly although how about updating 
	if(_userInputCommand){
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=false; // MDH@29OCT2019: instead of calling setLastUserInputCommandToken()
		updateLastTokenAutocompletionText(); // TODO perhaps we do not need this after all here????? NOTE used to do that in setTokenType() when endInput was true but not doing that anymore
		// if(amDebugging())inputInfo("New user input command created.");
	}else
		inputError("Failed to create a new user input command.");
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
bool copyUserInputCommand(){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT _userInputCommand must NOT be NULL and we're assuming that _userInputCommand now points to one of the remembered commands (that needs to be duplicated in order to allow editing it)
	//        it's probably best to first create a new command, copy the tokens over from _userInputCommand and set the user input command to that new command
	Mcommand* _newUserInputCommand=owned_command(_getNewCommand(false),owner); // get a new command without tokens (should NEVER fail unless memory shortage)
	if(!_newUserInputCommand){outputError("Failed to duplicate the current user input command");return false;}
	
	// if fails to copy _userInputCommand->_firstToken _userInputCommand->_lastToken should end up as NULL
	if(amVerboseDebugging())inputInfo("Preparing the user input command for editing.");
	///// NOT NEEDED using the false flag in _getNewCommand()!!!! _newUserInputCommand->_lastToken=NULL;_newUserInputCommand->_firstToken=NULL;
	Mtoken* _tokenToCopy=_userInputCommand->_firstToken;
	// the essence is that _userInputCommand->_lastToken points to the last token in _userInputCommand->_firstToken
	// NOTE theoretically _userInputCommand->_lastToken could be NULL due to _getToken() failing to create a new token
	while(_tokenToCopy){
		// MDH@28OCT2019: because we adapted _getNewCommandToken to receive the last command token as argument, and returning the new command token, we need to assign the result to _userInputCommand->_lastToken!!!
		_newUserInputCommand->_lastToken=owned_token(_getNewCommandToken(_newUserInputCommand->_lastToken,_tokenToCopy->type),Msubowner(owner,1));
		if(!_newUserInputCommand->_lastToken)break;
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
		if(_tokenToCopy->expr){ // some token referenced
			Mtoken *referencedToken=_tokenToCopy,*newReferencedToken=_newUserInputCommand->_lastToken;
			// move back until we find the token referenced (and we should find it)
			while(referencedToken!=_tokenToCopy->expr){referencedToken=referencedToken->prev;newReferencedToken=newReferencedToken->prev;}
			// ASSERT referencedToken now equals the token in the original command being referenced (which could be itself obviously), and newReferencedToken is a token in the new user input command that should be pointed to!!!
			if(newReferencedToken)_newUserInputCommand->_lastToken->expr=newReferencedToken;else inputError("%sFailed to synchronize a token reference.",M_BUG_PREFIX);
		}else // nothing pointed to, so just in case
			_newUserInputCommand->_lastToken->expr=NULL;
		// replacing: _newUserInputCommand->_lastToken->expr=_tokenToCopy->expr; // MDH@20MAY2019: just copy the expr over!!!!
		
		setTokenSignificantCharacterCount(_newUserInputCommand->_lastToken,getTokenSignificantCharacterCount(_tokenToCopy));
		// if failing to copy the text over get rid of the command constructed so far, and break
		_newUserInputCommand->_lastToken->text=owned_string(_stringCopy(_tokenToCopy->text,0),Msubowner(owner,2)); // copies the entire Mstring over
		if(!_newUserInputCommand->_lastToken->text){_newUserInputCommand->_lastToken=NULL;break;} // TODO perhaps we'd have to do a little more than just this?????
		// MDH@24APR2019 obsolete: getCommandLength()+=string_length(_userInputCommand->_lastToken->text);
		// MDH@24SEP2020 DONE some additional fields to copy over (NOT the offset is that is set automatically)
		_newUserInputCommand->_lastToken->argument=_tokenToCopy->argument;
		_newUserInputCommand->_lastToken->offset=_tokenToCopy->offset;
		_newUserInputCommand->_lastToken->position=_tokenToCopy->position;
		_newUserInputCommand->_lastToken->envid=_tokenToCopy->envid;
		_newUserInputCommand->_lastToken->prevIdentifier=_tokenToCopy->prevIdentifier;
#ifdef __DEBUG__
		printf("%d:%s",_userInputCommand->_lastToken->type,string(_userInputCommand->_lastToken->text));
#endif
		if(!_newUserInputCommand->_firstToken)_newUserInputCommand->_firstToken=_newUserInputCommand->_lastToken; // TODO=DONE will never happen???? it does here
		// get the next token to copy...
		_tokenToCopy=_tokenToCopy->next;
	}
	if(!_newUserInputCommand->_lastToken){FREE_COMMAND(_newUserInputCommand,owner);return NULL;}

	// OOPS do NOT call setUserInputCommand() here as it will write the command once more so it might suffice to assign
	_userInputCommand=owned_command(disowned_command(_newUserInputCommand,owner),owner_userInputCommand); // replacing: setUserInputCommand(_newUserInputCommand); // testing whether successful: inputInfoCommand(_userInputCommand);
	return true;
}

// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual createUserInputCommand
/**
 * setCommand() creates a new (empty) command (in _userInputCommand->_firstToken) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *              which is supposedly showing behind the cursor!!!
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
//                call whenever the current token changes (in removePreviousTokenCharacter() and commandCharacterAccepted())
// MDH@01OCT2019: aSuggestedCharacter is actually not used anymore, so no need to pass it in anymore
bool tokenCheckedForBeingAFunction(Mtoken* lastCommandToken,bool endOfInput/*,bool aSuggestedCharacter*/){
	// only identifiers should be checked...
	// what about properties????? properties should NEVER be considered functions
	if(lastCommandToken->type!=TT_VARIABLE&&lastCommandToken->type!=TT_NEW_VARIABLE&&lastCommandToken->type!=TT_FUNCTION)return false;
	// non-existing variables should be assigned to so it's a good idea to put the assignment operator behind it, although it might be hard to remove it though
	char* _identifierName=_stringstart(lastCommandToken->text,getTokenSignificantCharacterCount(lastCommandToken)); // free asap
	if(lastCommandToken->type!=TT_FUNCTION){ // is it a function (now)?
		if(getFunction(getExecutionEnvironment(),_identifierName)){ // yes, it is
			// if a new variable before (now a function), remove the (assignment) character in the behind cursor text
			// MDH@20SEP2019: I suppose we need to ascertain that an opening parenthesis is associated with the token now, and no longer anything else
			/* MDH@20SEP2019: removing:
			if(amMatchingparentheses())if(_userInputCommand->_lastToken->type==TT_NEW_VARIABLE)if(string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			*/
			// the minimum we can do is put an opening parenthesis in the behind cursor text
			setTokenType(lastCommandToken,TT_FUNCTION/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
			reoutputToken(lastCommandToken);
			// insert an opening parenthesis for the function call
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: if(endOfInput&&amMatchingparentheses())setLastTokenAutocompletionText("(");else deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: either force the feedforward text to match an opening parenthesis or nothing TODO does endOfInput matter?????
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='(')string_insert_char(feedforwardText,0,'(');
			*/
		}
	}else{ // is it (still) a function?
		if(!getFunction(getExecutionEnvironment(),_identifierName)){ // no, it ain't
			// the minimum we can do is remove the opening parenthesis behind it (if it is still there!!!!!)
			// MDH@11MAR2020: ok, here we have an issue: 
			setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
			reoutputToken(lastCommandToken);
			inputInfo("'%s' considered to be an existing variable.",_identifierName);
			//////////outputInfo("Variable redrawn!");
			// remove any opening parenthesis from the behind cursor text
			// MDH@23SEP2019 take care of by setLastTokenType, so removed: deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019: I suppose when TT_VARIABLE changes to TT_NEW_VARIABLE later on, an equal sign might be added!!!
			/* MDH@20SEP2019 replacing:
			if(endOfInput)if(amMatchingparentheses())if(getNumberOfSuggestedCharacters())if(string_char(feedforwardText,0)=='(')string_removed_char(feedforwardText,0);
			*/
		}
	}
	// check whether the variable exists or not
	// MDH@07AUG2019: this variable could exist in this command, which we should check
	if(lastCommandToken->type==TT_VARIABLE||lastCommandToken->type==TT_NEW_VARIABLE){ // might not exist after all both in the command and in the current environment
		// MDH@08AUG2019 WARNING: all variables assigned to in the local variable declaration argument of the special functions should ALWAYS be considered new, but of course we cannot see that until they are assigned to
		//                        unless we do not require them to be assigned to (and we can just use them by name itself without assigning a value to them) in which case they are local but uninitialized...
		int8_t variableExistsIndicator=0; // assuming invalid
		if(lastCommandToken->argument!=1){
			if(!existsInCommand(_userInputCommand,_identifierName,lastCommandToken->envid)){
				// MDH@12MAR2020: if containsVariable() returns -2 this only happens with a property reference that is invalid in which case the token should be considered an error
				//                I suppose we should then change the token type to TT_ERROR in which case the type won't change from NEW_VARIABLE to VARIABLE or vice versa
				variableExistsIndicator=containsVariable(NULL,_identifierName,-1);
				switch(variableExistsIndicator){
					case -5: // value of type MAP but no map defined (TODO is that a bug?????)
					case -3: // value does not exist
					case -2: // variable hosting the property does not exist
						break;
					case 0: // illegal input
					case -4: // there is a value but it is NOT a map, I suppose this would be the only ERRONEOUS situation
						{
							char* propertyName=strrchr(_identifierName,M_PROPERTY_SEPARATOR_CHARACTER);
							if(propertyName){
								setTokenType(lastCommandToken,TT_ERROR);
								reoutputToken(lastCommandToken);
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
				setTokenType(lastCommandToken,TT_NEW_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
				reoutputToken(lastCommandToken);
				// suggested characters should make = show (probably already present in the behind cursor text)
				// MDH@23SEP2019 take care of by setLastTokenType, so removed: setLastTokenAutocompletionText("="); // MDH@20SEP2019 replacing: if(endOfInput&&!aSuggestedCharacter)if(amMatchingparentheses())if(string_char(feedforwardText,0)!='=')string_insert_char(feedforwardText,0,'=');
			}
		}else
		if(lastCommandToken->type==TT_NEW_VARIABLE){ // a new variable
			if(variableExistsIndicator>0){ // now an existing variable
				setTokenType(lastCommandToken,TT_VARIABLE/*,endOfInput*/);if(endOfInput)updateLastTokenAutocompletionText();
				reoutputToken(lastCommandToken);
				///// MDH@23SEP2019 removed: deleteAutocompletionTextOfToken(_userInputCommand->_lastToken); // MDH@20SEP2019 replacing: if(endOfInput)if(amMatchingparentheses())if(string_length(feedforwardText)&&string_char(feedforwardText,0)=='=')string_removed_char(feedforwardText,0);
			}
		}
	}
	free(_identifierName); // freed
	return true;
}

// MDH@14AUG2019: cancelCommand() takes care of removing everything in the current command
void cancelCommand(){ // in response to Ctrl-C or backspace on the first character
	if(amVerbose())inputInfo("Cancelling the command.");
	// MDH@31OCT2019: with a command now possibly covering multiple lines we have to do a little more than we did before but we can put that in backToPrompt()
	backToPrompt();
	clearScreenFromCursor(); // inserting doing this otherwise (in the case of backspace) we would apparently still see the behind cursor text
	clearCommand();
	// by removing the behind cursor text, we ascertain that when the Enter key is pressed, we will switch to control mode, as otherwise we wouldn't, on the other hand, if _userInputCommand->_firstToken is NULL we should always switch to control mode (even if)
	if(_manualFeedforwardText){FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;}
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
	// it's a good idea to inform the user that the command was cleared
	if(amVerbose())inputInfo("Command cleared!");
}
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
	
	// MDH@20SEP2019: the following is about removing the feed forward characters that were added when a certain token started but as you can see 
	//                it is all about feed forward associated with the start of a token, so removing the associated feed forward can also be done at the moment the token is actually removed
	//                so for now we remove the following block and simply write the behind cursor text
	////////writeSuggestedText(true);
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
//                also uint16_t behindCursor (as it is always called with constant value 1) removed as formal parameter, and endOfInput (typically true) added!!!
char removedTokenCharacter(bool endOfInput){
	// MDH@30OCT2019: ASSERT there's something to 'remove'
	// MDH@03SEP2019: 
	char tokenCharacterRemoved='\0';
	if(_userInputCommand){ // should ALWAYS be the case
		uint16_t tokenCharacterPosition;
		// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
		while(_userInputCommand->_lastToken){
			tokenCharacterPosition=string_length(_userInputCommand->_lastToken->text); // MDH@24APR2019 replacing (what is essentially the same): getUserInputLength()-_userInputCommand->_lastToken->offset;
#ifdef __DEBUG__
			printf("%d",tokenCharacterPosition);
#endif
			if(tokenCharacterPosition>=1)break;
#ifdef __DEBUG__
			outputChar('.');
#endif		
			_userInputCommand->_lastToken=_userInputCommand->_lastToken->prev;
		}
		// MDH@30OCT2019: userInputCommandIdentifierContinuationNeedsUpdating=inIdentifierToken(_userInputCommand->_lastToken); // MDH@02OCT2019: should be called whenever _userInputCommand->_lastToken changes...
		if(_userInputCommand->_lastToken)tokenCharacterRemoved=string_removed_char(_userInputCommand->_lastToken->text,tokenCharacterPosition-1);
#ifdef __DEBUG__
			outputChar(tokenCharacterRemoved);
#endif
		if(tokenCharacterRemoved){
			if(endOfInput){
				// MDH@30OCT2019: with multiline user input it sometimes is a little harder than calling moveCursorLeft(1)
				//                if the offset of the current user input line is beyond the total command length apparently we've 'removed' the last command character on the previous line
				// MDH@24SEP2020: now it's probably better to test numberOfLineCommandCharacters because it it is zero we're obviously at the first position on a command line
				if(numberOfLineCommandCharacters==0){
				/* replacing:
				size_t userInputLength=getUserInputLength();
				if(_userinputline&&_userinputline->offset>userInputLength){
					*/
					toStartOfLine();clearScreenFromCursor(); // yes, we need this in particular when at the start of a new line and doing a left arrow in which case the prompt on the line needs to be removed
					numberOfLineCommandCharacters=removeUserinputline()-1; // MDH@24SEP2020: subtract 1 because we've actually consumed a single character just now!!
					oneLineUp(); // I think oneLineUp() should not always be called?????????
					toUserInputCursorPosition(0); // one line up and to the proper position (not sure what will happen to the suggested text though)
				}else{ // still command characters on the current command line
					moveCursorLeft(1); // MDH@01OCT2019: this ought to be done BEFORE tokenCheckedForBeingAFunction() is called so we moved it over here!!!
					numberOfLineCommandCharacters--; // MDH@07JUL2020: essential to keep track of the number of line command characters
				}
			}
			// MDH@01OCT2019: whenever the last token does not change but the last token character is removed, we should check the type 
			//                HOWEVER we're assuming that we're dealing with an end of input situation
			bool tokenRemoved=(string_empty(_userInputCommand->_lastToken->text)?removeLastUserInputCommandToken():false);
			unfinishCommandToken(_userInputCommand->_lastToken); // we need to do this to allow appending characters to the token again
			if(endOfInput)if(!tokenRemoved)tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,endOfInput);
		}
	}else
		inputInfo("%sNo command to remove characters from!",M_BUG_PREFIX);
	return tokenCharacterRemoved;
}

// in response to backspace the previous token character is to be removed
bool removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	if(commandIndex>0){
		commandIndex=0;
		if(!copyUserInputCommand())return false;
	} // MDH@03SEP2019 BUG FIX: I have to do this if scrolling through the list of previous commands!!!
	char removedCharacter=removedTokenCharacter(true); // MDH@01OCT2019: will now also perform moveCursorLeft(1) when the argument is true and success
	if(!removedCharacter){inputError("%s","Failed to remove the last entered character.");return false;}
	// MDH@01OCT2019: moveCursorLeft(1); // TODO check if this is necessary also when cancelling the command
	if(amVerboseDebugging())inputInfo("Character '%c' removed.",removedCharacter);
	// if no text is left in the command we cancel the command (as a service to the user who wouldn't understand that Enter wouldn't switch to Control mode on an otherwise empty command!!!)
	// MDH@18JUN2020: I've adapted copyUserInputCommand() in such a way that if it fails _userInputCommand will NOT be replaced so it will not become NULL (i.e. it will remain as it was), so the following test will always evaluate to TRUE
	if(_userInputCommand){ // we still have a command being evaluated (NOTE that removedTokenCharacter() can actually set _userInputCommand->_firstToken to NULL)
		if(_userInputCommand->_lastToken==_userInputCommand->_firstToken&&string_length(_userInputCommand->_firstToken->text)==0){
			if(amVerboseDebugging())inputInfo("%s","Cancelling the command.");
			cancelCommand();
			if(amVerboseDebugging())inputInfo("%s","Command cancelled.");
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

// MDH@09JUL2019: count the number of list elements in front of the current token
uint32_t getListElementCount(){
	uint32_t listElementCount=0;
	Mtoken* token=_userInputCommand->_lastToken;
	Mtoken* startToken=_userInputCommand->_lastToken->expr;
	while(token!=startToken){if(token->expr==startToken&&token->type==TT_LISTELEMENT)listElementCount++;token=token->prev;}
	return listElementCount;
}

// MDH@27FEB2020: commandCharacterAppended() moved over to Mshell.c

// MDH@24SEP2020: call newCommandLine() either to start a new command line either explicitly or implicitly (when no further characters fit on the current command line)
static void newCommandLine(bool newline){
	numberOfLineCommandCharacters=0; // MDH@24SEP2020: TODO we should never forget to reset numberOfLineCommandCharacters after showing a continued prompt
	showContinuedPrompt(getUserInputLength(),newline);
	outputTokenColor(_userInputCommand->_lastToken);
}

// MDH@12APR2019: in order to implement the Tab character we have to delegate entering a character (typed) to a separate function
//       		  ASSERTION _userInputCommand->_firstToken and _userInputCommand->_lastToken are  NOT  NULL
//                the endOfInput flag is used to indicate whether this is the end of the input
//                the aSuggestedCharacter flag tells commandCharacterAccepted() that the input character came from feedforwardText (the feed forward), so it will in that case not alter feedforwardText (by removing the same character that was entered)
// MDH@26JUN2020: should now also keep track of the total number of command characters on the current user input line (numberOfLineCommandCharacters)
bool commandCharacterAccepted(char inputChar,char *inputCharacterType,bool endOfInput,bool aSuggestedCharacter){
	bool initializationsChanged=false;
	// MDH@21APR2019: there are two situation where we need to get a command
	/////outputChar('1');
	//                1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(!_userInputCommand) // no current command
		createUserInputCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyUserInputCommand();
	/////outputChar('2');
	// if _userInputCommand->_lastToken is now NULL something went wrong (in copyUserInputCommand or createUserInputCommand most likely)
	if(!_userInputCommand){inputError("%sNo user input command.",M_BUG_PREFIX);return false;}
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	/////outputChar('3');
	// MDH@21SEP2020: clearInfo() is also responsible for the problem with right arrow because the result is that the character is output one line down with every right arrow
	// clearInfo(); // MDH@28FEB2020: is responsible for the experienced problem
	/////outputChar('4');
	/////////if(amDebugging())inputInfo("A");
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
	//                and put it in a single Mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/

	// MDH@28OCT2019: all the code that deals with updating the tokens 
	//                NOTE passing in the address of _userInputCommand->_lastToken, so it can be changed!!!!
	Mtoken* newLastCommandToEvaluateToken=commandCharacterAppended(_userInputCommand,inputChar,inputCharacterType,endOfInput);
	if(!newLastCommandToEvaluateToken)return false;
	if(newLastCommandToEvaluateToken!=_userInputCommand->_lastToken){
		_userInputCommand->_lastToken=owned_token(newLastCommandToEvaluateToken,Msubowner(owner_userInputCommand,1)); // MDH@28MAY2020: take over ownership of the new last command token
		outputUserInputCommandTokenColor();
	}else{
		// MDH@31OCT2019: show whitespace in the standard info color!!
		if(getTokenSignificantCharacterCount(_userInputCommand->_lastToken)>0){
			resetOutputColor();
			if(*inputCharacterType=='W'&&inputChar==M_NEWLINE_CHARACTER)*inputCharacterType=' '; // convert the newlinecharacter (which type should be W to the blank)
		}
		/*
		if(string_last_char(_userInputCommand->_lastToken->text)=='`'&&_userInputCommand->_lastToken->type!=TT_DQSTRING&&_userInputCommand->_lastToken->type!=TT_SQSTRING){
			resetOutputColor();
		}
		*/
	}

	/////if(amDebugging())inputInfo("J");
#ifdef __DEBUG__
	printf("[%s]",string(_userInputCommand->_lastToken->text));
#endif

	// MDH@07JUL2020: allowing the cursor to be on the last available line position BUT not to allow input characters to appear there!!!!!
	// MDH@21SEP2020: if(numberOfLineCommandCharacters+promptLength+1==numberOfLineCharacters){showContinuedPrompt(true);outputTokenColor(_userInputCommand->_lastToken);}
	// MDH@24APR2019 obsolete: getCommandLength()++; // increment total command length
	// outputChar('>');
	outputChar(inputChar); ///////// replacing: outputLastTokenChar(_userInputCommand->_lastToken); // echo the last token character

	numberOfLineCommandCharacters++;

	// MDH21SEP2020:65th birthday: if the line is now full move the cursor to the next line
	//                             not to write a newline character because we are supposedly already at the start of the next line
	if(numberOfLineCharacters>0&&numberOfLineCommandCharacters+promptLength>=numberOfLineCharacters)
		newCommandLine(false);

	//putchar('\b');

	if(endOfInput){
		//////if(amAssisting())output(":%c",*inputCharacterType);
		// debugWrite("Command length after inserting %c: %zu.",inputChar,getCommandLength());
	}

	// MDH@24APR2019 obsolete: getUserInputLength()++; // increment the current cursor position
	// MDH@07AUG2019: after a character is input by the user (or some other source) the identifier type will be checked...
	//                BUT 
	// MDH@01OCT2019: argument aSuggestedCharacter is no longer used in tokenCheckedForBeingAFunction and consequently by this function, so it is removed as argument and replaced by updateidentifiercontinuation (which we do need)
	bool notCheckedForBeingAFunction=!tokenCheckedForBeingAFunction(_userInputCommand->_lastToken,endOfInput/*,aSuggestedCharacter*/); // MDH@28MAY2019: ALWAYS check for being a function!!!!
	/////if(amDebugging())inputInfo("K");
	if(endOfInput){
		/* MDH@20SEP2019: because I created updateLastTokenAutocompletionText which should take care of adding the right token feed forward I do not need to do the following
		// MDH@29APR2019: I'd like to detect when a variable becomes a function or vice versa
		if(notCheckedForBeingAFunction){
			/////if(amDebugging())inputInfo("L");
			// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
			// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
			if(_userInputCommand->_lastToken->type!=TT_ERROR){ // MDH@29APR2019: don't add closing bracket to autocompletion text when in error!!!
				// MDH@20SEP2019: typically the token type should determine what the feed forward text of the token should be
				//                so it is less optimal to explicitly check for certain input characters instead of simply looking at the type of token that started!!!
				//                even so, knowing that each of the input characters that insert a feed forward character actually started a new token and there's at that point no token feed forward yet
				//                it's easiest to simply set the feed forward text of the last token
				if(!aSuggestedCharacter){ // MDH@14AUG2019: using this flag here means no changes to the suggested text are done, when this character was consumed from the suggested text
					if(_userInputCommand->_lastToken->type!=TT_BINARY_aErU){
						// MDH@14AUG2019: whether or not to insert a feed-forward matching parenthesis is open for debate
						//                this is a bit of an issue because string literal start and ends is the same, and how do we know if a string is started or ended????? solution: test the type
						if(amMatchingparentheses()){
							// MDH@14AUG2019: I think we should always insert the character we need to close the bracket no matter what
							//                NOTE we're using inputCharType here, not inputChar but as you may notice in removeCharacter there it's not using the input character type, I suppose we should
							//                with strings is important only to insert the same character when the inserted character started the string
							// removing: if(!string_length(feedforwardText)) // MDH@28MAY2019: if there's nothing behind the cursor yes we do append closing stuff
							switch(inputCharacterType){
								case '[':
									setLastTokenAutocompletionText("]"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,']');
									break;
								case '{':
									setLastTokenAutocompletionText("}"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'}');
									break;
								case '(':
									setLastTokenAutocompletionText(")"); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,')');
									break;
								case 'D':
									if(_userInputCommand->_lastToken->type==TT_DQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutocompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								case 'S':
									if(_userInputCommand->_lastToken->type==TT_SQSTRING&&string_length(_userInputCommand->_lastToken->text)==1)
									setLastTokenAutocompletionText(string(_userInputCommand->_lastToken->text)); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,inputChar);
									break;
								// MDH@14AUG2019: if the user typed the same character as is currently behind the cursor let's remove that character
								//                but only when the character entered is NOT consumed because in that case it was already removed (and shouldn't be removed again if there's another such a character which can happen a lot with matching parentheses)
								default:
									// MDH@20SEP2019: it is well possible that the user will type the first character of the feed forward associated with the token in which case I suppose that character should be removed from the feed forward text
									removeFirstFeedforwardCharacterFromLastTokenWhenMatching(inputChar);
									// replacing: if(string_length(feedforwardText)&&string_char(feedforwardText,0)==inputChar)if(!string_removed_char(feedforwardText,0))inputError("Failed to remove the matching first feed forward character");
									break;
							}
						}
					}else
						setLastTokenAutocompletionText("="); // MDH@20SEP2019 replacing: string_insert_char(feedforwardText,0,'=');
				}
			}
			/////if(amDebugging())inputInfo("M");
		}
		*/
		bool acceptedFirstSuggestedCharacterDeleted=(!aSuggestedCharacter||(_identifierContinuationCharacters?deleteFirstIdentifierContinuationCharacter():deleteFirstAutocompletionCharacter(inputChar,true)));
		if(!acceptedFirstSuggestedCharacterDeleted)inputError("Failed to remove the accepted first suggested character."); // i.e. we're NOT switching to control mode or returning false
		updateLastTokenAutocompletionText(/*acceptedFirstSuggestedCharacterDeleted*/); // it makes sense to update the current token feed forward text just before actually showing it AND to update the identifier continuation first
		/////////writeSuggestedText(false); // just in case we removed some character (see TT_FUNCTION->TT_VARIABLE)
		/////if(amDebugging())inputInfo("N");
		// debugWrite("Command length after writing behind cursor text: %zu.",getCommandLength());
		//////////if(!initializationsChanged)outputStatus(inputChar,*inputCharacterType);
		/////if(amDebugging())inputInfo("O");
	}
	/////if(amDebugging())inputInfo("P");
	return true;
}

bool COMMAND_PROCESSOR_AVAILABLE=0;
void clear_shellCommand(){
	string_setlength(_suggestedText,0/*,owner_suggestedText*/);
	string_setlength(_shellCommand,0/*,owner_shellCommand*/);
	// MDH@24APR2019 obsolete: getUserInputLength()=0;
}
void execute_shellCommand(){
	// ASSERTION string_length(_shellCommand) should be positive
	output(""); // get a new line before we see the result of executing this command!!
	int result=system(string(_shellCommand));
	if(result)output("Result: %d.\n",result); // non-zero result
	clear_shellCommand(); // ready for the next execution
}
char switchToShellMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)output("%s",message);
	setInputMode(IM_SHELL);
	clear_shellCommand();
	return 's';
	//output("%s\n $ ","Enter your shell command, and press the Return button to execute.");
}
void switchToCommandMode(){
	if(inputMode==IM_COMMAND)return;
	if(inputMode==IM_CONTROL)newline(); // MDH@03MAR2020: TODO let's see if this is OK
	setInputMode(IM_COMMAND);
	// ascertain to not have autocompletion text
	deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); 
}

char getFirstSuggestedCharacter(bool autogenerated){return(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0?getFirstIdentifierContinuationCharacter():getFirstAutocompletionCharacter(autogenerated));}

// MDH@01OCT2019: I created consumeFirstSuggestedCharacter() today to consume the first suggested character but initially called commandCharacterAccepted with endOfInput flag equal to false
//                I suppose that was wrong because there's exactly one character being accepted and that's also the end of input character 
//                the result will be that updateLastTokenAutocompletionText(true) and writeSuggestedText(true) are executed already by commandCharacterAccepted() so I won't have to do that here anymore
char getFirstSuggestedCharacterConsumed(char firstSuggestedCharacter,bool endOfInput){
	// if the first suggested character is provided use that, otherwise get any
	if(!firstSuggestedCharacter)firstSuggestedCharacter=getFirstSuggestedCharacter(false); // any first suggested character will do
	if(firstSuggestedCharacter){
		char firstSuggestedCharacterInputType=INPUTCHARACTERTYPES[firstSuggestedCharacter];
		if(!commandCharacterAccepted(firstSuggestedCharacter,&firstSuggestedCharacterInputType,endOfInput,true))firstSuggestedCharacter='\0';
	}
	return firstSuggestedCharacter;
}

void removeFirstSuggestedCharacter(char firstSuggestedCharacter,bool consumed){
	if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){ // a feed forward text character was consumed
		deleteFirstAutocompletionCharacter(firstSuggestedCharacter,consumed);
	}
}

// MDH@14NOV2019: we want to keep a list of evaluated commands inside the main M environment
//                then the user can use variable M to get at the stored commands, and the M function to get results
Mvalue* M_value=NULL;Mallocationowner owner_M_value=(Mallocationowner){MODULE_ID,__LINE__,1}; // where the list of evaluated commands is to be stored 
// the M function (full name MM or perhaps MMfunction) allows one to use a previous result in a command
Mvalue* MM(Mvalue* indexValue){
	Mvalue* resultValue=NULL;
	Mlist* M_list=(M_value&&M_value->type==VT_LIST?M_value->value._list:NULL);
	if(M_list){
		// because the command and result are prepended to the M_value list, the index identifies how far to go back
		long long index=(indexValue?getValueInteger(indexValue):-1); // if no index value is specified (e.g. when user called M() without argument), index 1 is used
		Mvalue* commandresultValue=getValueAtIndex(M_list,index); // get the commandresult value stored at the given index!!
		if(commandresultValue&&commandresultValue->type==VT_LIST&&commandresultValue->value._list->_last)resultValue=commandresultValue->value._list->_last->_value;
	}
	return resultValue;
}
// MDH@14NOV2019 instead of storing the command itself (which would result in a memory security risk we store the command text instead, that way we can still grab it and evaluate it)
// MDH@25MAY2020: commandText is a constant text and therefore ownership if not to be possesed
bool registerCommandEvaluation(char const * const commandText,Mvalue* evaluationresultValue,long long commandIndex){Mallocationowner owner=getOwner(__LINE__);
	// prepend a list containing the command and the result to the list wrapped in M_value
	if(M_value){
		Mlist* M_list=(M_value->type==VT_LIST?M_value->value._list:NULL);
		if(M_list){
			// we know M_list is a list stored in an Mvalue which has owner getValueOwner()
			Mallocationowner owner_M_list=Msubowner(getValueOwner(),1); // MDH@25MAY2020: the owner of M_list
			Mvalue* commandresultValue=_getMapValue(VT_UNDEFINED,false); // the map that is to contain the command text and its result value text
			if(commandresultValue){
				// NOTE we're wrapping the first token of the command into a value which is dangerous because when the list is freed, the token shouldn't!!
				//      so theoretically that value is weak, whereas the evaluation result is strong
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

Mvalue* Mvariables(){//Mallocationowner owner=getOwner(__LINE__);
	if(amVerbose())
		outputInfo("Getting the variables!");
	// returning the names of the local variables (including the hidden ones)
	// NOTE _getVariableNamesMap() always requires a non NULL environment to start with
	return _getValueOfMap(_getVariableNamesMap(getExecutionEnvironment()));
}
// MDH@15NOV2019: returning value counts (per value type), passing in a list of variable names
// MDH@25NOV2019: what about returning a table??? which is a list
Mvalue* Mvalues(Mvalue* variableNamesValue){//Mallocationowner owner=getOwner(__LINE__);
	if(amVerbose())
		outputInfo("Getting the values!");
	// MDH@25NOV2019: requesting the table allows for better reproduction
	//                TODO instead of the table return the text representation of the table (which is easier to inspect!!!)
	return _getValueOfList(_getValuesTable(variableNamesValue));
	// replacing: return _getValueOfMap(_getValuesTable(variableNamesValue),true);
}
// method for reading a text from standard out which means reading characters until Enter-key is encountered!!
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
Mvalue* MexecuteOSCommand(Mvalue* _commandValue){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _commandOutputValue=NULL;
	Mstring* _commandText=owned_string(_getValueText(_commandValue,true),owner);
	if(_commandText&&string_length(_commandText)){
		FILE *fp=popen(string(_commandText),"r");
		if(fp){
			Mlist* _commandOutputList=owned_list(_getListOfType(VT_TEXT),owner);	
	  		char path[1036]; // incremented by one to store the single quote
			path[0]='\''; // a single-quote to surround the output text lines
  			/* Read the output a line at a time - output it. */
  			while(fgets(path+1,sizeof(path)-1,fp)!=NULL){
				  // put an end-of-line at the position where end-of-line is encountered
				  uint16_t i=0;while(++i<1035)if(path[i]=='\n'||path[i]=='\r')break;path[i]='\0';
				  Mvalue* _pathValue=_getTextValue(path);
				  if(!_pathValue)continue;
				  if(appendedToList(_commandOutputList,owner,_pathValue,M_LL_INVALID)<=0){
					  outputError("Not all output retrieved!");
					  break;
				  }
			}
			pclose(fp);
			_commandOutputValue=_getValueOfList(disowned_list(_commandOutputList,owner));
		}else
			output("%sFailed to execute OS command '%s'.\n",M_ERROR_PREFIX,string(_commandText));
	}
	if(_commandText)FREE_STRING(_commandText,owner);
	return _commandOutputValue;
}

// in an interactive session we'll have additional variables to set up
uint16_t prepareShellEnvironmentForInteractiveSession(){Mallocationowner owner=getOwner(__LINE__);
	
	uint16_t errorflags=0;

	// Menvironment* _Menvironment=getValueEnviroment(_MenvironmentValue); // MDH@03FEB2020: extracting the environment from its wrapper
	// if(!pushExecutionEnvironment(_Menvironment))return false;
	Mallocationowner owner_executionenvironment=getOwnerExecutionEnvironment();

	// we're gonna need a list to store lists of command and result pairs
	M_value=_getListValue(VT_MAP,false,"M command list"); // don't change the ownership of Mvalue instances

	// MDH@14NOV2019: typically M is created as an immutable variable BUT of course I can change the assigned M_value myself directly but the user can't!!
	//                NOTE if we would have used setValue to set M to M_value it would copy the list that M_value holds instead of using M_value itself, setVariable won't do that
	if(M_value){
		if(amVerboseDebugging())output("Adding variable '%s'.\n",M_VARIABLE_NAME);
		if(!addVariable(_Menvironment,owner_executionenvironment,M_VARIABLE_NAME,VT_LIST,true)){
			output("%sFailed to add variable '%s'.\n",M_WARNING_PREFIX,M_VARIABLE_NAME);
			errorflags|=1;
		}else
		if(!setVariable(_Menvironment,M_VARIABLE_NAME,M_value)){
			output("%sFailed to initialize variable '%s'.\n",M_WARNING_PREFIX,M_VARIABLE_NAME);
			errorflags|=1;
		}
		// if M_value wasn't bound to the M variable, it's memory will be freed by the garbage collector, by setting M_value to NULL we know we do not need to update the wrapped list
		if(M_value->count==0){
			M_value=NULL;errorflags|=2;output("%sFailed to initialize variable '%s'.\n",M_ERROR_PREFIX,M_VARIABLE_NAME);
		}else
		if(amVerboseDebugging())
			output("Variable '%s' initialized.\n",M_VARIABLE_NAME); 
	}else{
		errorflags|=4;
		outputWarning("Failed to create the list in which commands and their values will be stored. You won't be able to use it in your commands!");
	}
	
	// MDH@14NOV2019: the M function allows access to the results of previously executed commands (before reset() clears them all!!!)
	if(M_value){
		if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,MFUNCTION_NAME),MFUNCTION_NAME,MM)){
			errorflags|=8;
			output("%sFailed to register function %s.",M_ERROR_PREFIX,MFUNCTION_NAME);
		}else
		if(amVerbose())
			output("Function %s registered.\n",MFUNCTION_NAME);
	}

	if(!completedFunction(_getFunction(_Menvironment,owner_executionenvironment,"variables"),"variables",Mvariables)){
		errorflags|=16;
		outputWarning("Failed to register the variables() function");
	}
	if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,"values"),"values",Mvalues)){
		errorflags|=32;
		outputWarning("Failed to register the values() function");
	}

	// MDH@27FEB2020: Min is special as it used inputCharRead to read single characters, so it should only be available in sessions
	if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,"in"),"in",Min)){
		errorflags|=64;
		outputWarning("Failed to register the in function"); // moved out of registerInternalFunctions!!!!
	}
	if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,"os"),"os",MexecuteOSCommand)){
		errorflags|=128;
		outputWarning("Failed to register the os function"); // moved out of registerInternalFunctions!!!!
	}

	// color functions
	if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,"bc"),"bc",Mbc)){
		errorflags|=256;
		outputWarning("Failed to register the bc function"); // moved out of registerInternalFunctions!!!!
	}
    if(!completedValueFunction(_getFunction(_Menvironment,owner_executionenvironment,"tc"),"tc",Mtc)){
		errorflags|=512;
		outputWarning("Failed to register the tc function"); // moved out of registerInternalFunctions!!!!
	}

	return errorflags;
}

// MDH@21JUN2019: reset() takes care of removing all stored commands
void reset(){Mallocationowner owner=getOwner(__LINE__);
	newline();
	if(_registeredcommands){ // MDH@18JUN2020: testing commandBlocks is better than testing commandCount, and testing _registeredcommands is perhaps even better
		output("Delete all remembered commands? ");char c;inputCharRead(&c);outputChar(c);newline();
		if(c=='Y'||c=='y'){
			if(commandCount>0){
				output("Deleting %lld command%s.\n",commandCount,(commandCount>1?"s":""));
				unsigned long long numberOfOriginalCommandsFreed=0;
				// TODO we might get a problem if a command is replicated in _registeredcommands // DONE MDH@18JUN2020: commandIndex added to Mcommand so if it is the same as where it is located in _registeredcommands we free it otherwise we do not
				while(1){
					commandCount--;
					if(_registeredcommands[commandCount].previousCommandIndex==0){
						FREE_COMMAND(_registeredcommands[commandCount]._command,owner_registeredcommands); // freeing all new (i.e. not duplicated) commands
						numberOfOriginalCommandsFreed++;
					}
					if(commandCount==0)break;
				}
				if(numberOfOriginalCommandsFreed>0)output("Number of original commands freed: %llu.\n",numberOfOriginalCommandsFreed);else outputWarning("No original commands freed");
			}
			if(commandBlocks>0){FREE_DISOWNED(_registeredcommands,commandBlocks*COMMAND_BLOCKSIZE,'C',owner_registeredcommands);commandBlocks=0;} // free all (disowned!!!!!) allocated command blocks
			_registeredcommands=NULL; // MDH@18JUN2020: makes sense to do this as well
			outputInfo("All commands deleted!");
			// MDH@18JUN2020: also remove all the items in the command result 
			if(M_value){
				Mvalue* clearValue=Mclear(M_value);
				if(isValueZero(clearValue))output("Command result history cleared...\n");else if(isValueNegative(clearValue))outputWarning("Command result history not completely cleared...");
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
bool interactiveSessionInitialized(){
	uint16_t errorflags=prepareShellEnvironmentForInteractiveSession();
	if(errorflags){
		output("Errors preparing for running an interactive session (with code %x). Do you want to continue? ",errorflags);
		char answer;
		inputCharRead(&answer);
		if(answer!='Y'||answer!='y')return false;
	}
	outputInfo("Ready for an interactive session.");
	return true;
}
// MDH@27FEB2020: called from within main() only, so can be placed directly in front of main (and separated into a separate M.c or better Minterpreter.c or Mcli.c)

bool preparedForUserInput(){
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	bool result=sessionInitialized();
	if(result)
		output("Window dimensions: %dx%d.\n",getNumberOfWindowTextColumns(),getNumberOfWindowTextLines());
	else
		outputWarning("Failed to obtain the window dimensions.");
	setbuf(stdout,NULL);
	return true;
}

signed char getSessionSettingApplied(char sessionSettingCharacter){
	signed char result=0;
	if(sessionSettingCharacter=='m'||sessionSettingCharacter=='M')setMatchingparentheses(sessionSettingCharacter=='M');else
	if(sessionSettingCharacter>='0'&&sessionSettingCharacter<='9'){setColorscheme(sessionSettingCharacter-'0');result='n';}else
	if(sessionSettingCharacter=='w'||sessionSettingCharacter=='W')setWrapping(sessionSettingCharacter=='W');else
	////////////if(inputChar=='v'||inputChar=='V'){outputVariables();inputCharType='n';break;}
	if(sessionSettingCharacter=='u'||sessionSettingCharacter=='U'){setAcceptinghistorycommand(sessionSettingCharacter=='U');/*result='n';*/}else
	if(sessionSettingCharacter=='b'||sessionSettingCharacter=='B'){beep();result='n';}else // MDH@23SEP2019: so we can test whether beep() is working... TODO for toggling beeping????
	if(sessionSettingCharacter=='f'||sessionSettingCharacter=='F'){outputFunctions();result='n';}else
	if(sessionSettingCharacter=='r'||sessionSettingCharacter=='R'){reset();result='n';}else
	// options
	// MDH@31MAR2020: with lowercase 'x' let's ask for confirmation
	if(sessionSettingCharacter=='x')
	{if(!getCurrentFunctionBodyInput()){char c;output("Do you really want to exit M? ");inputCharRead(&c);outputChar(c);newline();if(c=='Y'||c=='y')result='x';}else result='x';}else
	if(sessionSettingCharacter=='X')result='x';else
	if(sessionSettingCharacter=='s'||sessionSettingCharacter=='S')result=switchToShellMode(NULL);else
	if(sessionSettingCharacter=='h'||sessionSettingCharacter=='H'){
		// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
		// we could call them a, b, c etc.
		if(commandCount){
			commandPages=1+(commandCount-1)/10;
			showNextCommandPage(); // as soon as commandPage>0 we are paging...
		}else
			output("%s\n","No previous commands to show.");
	}else // unprocessed
		result=-1;
	return result;
}

static Mstring* _separator=NULL;Mallocationowner owner_separator=(Mallocationowner){MODULE_ID,__LINE__,1};
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

int main(int argc, char **argv){Mallocationowner owner=getOwner(__LINE__); // using 0 is kind of an exception to the rule that every function has a positive function id

	COMMAND_PROCESSOR_AVAILABLE=system(NULL); // check if there's a command processor available

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

	// MDH@07APR2020 NOTE until we do the following allocation types will NOT get registered (which resulted in bug reports when they got freed by the garbage collector at the end)
	// tell user whether allocation recording is active!!!
	output("Initializing dynamic memory allocation management.\n");
	if(!allocationRecordingInitialized()){
		output("%sFailed to initialize memory allocation management!",M_ERROR_PREFIX);
		exit(1);
	}
	outputInfo("Dynamic memory allocation management initialized!");

	// MDH@23FEB2019: how about being able to continue with commands stored in a file, or perhaps allow for -log <logfile> or log=
	// whereas any filename without prefix is the file to execute at the start
	Mstring* _settingsCharacterText=owned_string(__string(),owner);
	if(_settingsCharacterText){
		output("Settings characters text: '%s'.\n",string(_settingsCharacterText));
		if(argc>1){
			printf("%s\n","Arguments");
			char settingCharacter;
			for(int arg=1;arg<argc;arg++){
				printf("%i. %s\n",arg,argv[arg]);
				if(argv[arg][0]=='-'){ // a flag (or flags)
					int i=0;
					while((settingCharacter=argv[arg][++i])){
						// if it's not a session setting we're going to pass it along to shellInitialized (see below) if we can
						if(getSessionSettingApplied(settingCharacter)<0){ // not a session setting
							_settingsCharacterText=string_append_char(_settingsCharacterText,settingCharacter); // register to pass along to shell initialized
							if(!_settingsCharacterText)settingApplied(settingCharacter); // if failing to add, do it from here
						}
					}
				}
			}
		}
	}

	// BEFORE using the command-line parameters (will effectuate wrap mode and color scheme) as it will clear the screen!	
	if(!preparedForUserInput()){
		outputError("Failed to initialize the user session.");
		resetOutputColor();
		exit(2);
	}
	outputInfo("User session initialized.");

	// MDH@27FEB2020: initEnvironment() renamed to getShellEnvironment() and moved over to Mshell.h/c
	// MDH@04MAR2020: initialize the shell passing in the required callbacks (replacing the original set... methods in Mshell.h/c) which is better to NOT forget any callbacks
	// MDH@24SEP2020: replacing outputToken by outputTokenText as the shell is not session command line aware (knowing Mcursormovement)
	if(!shellInitialized((_settingsCharacterText?string(_settingsCharacterText):NULL),inputCharRead,inputInfo,inputError,outputTokenText,reoutputToken,updateLastTokenAutocompletionText,outputCommandInfo)){ // ascertain to have an shell environment!!!
		outputError("Failed to initialize the M shell!");
		resetOutputColor();
		exit(3);
	}
	outputInfo("Shell initialized.");
	if(_settingsCharacterText)FREE_STRING(_settingsCharacterText,owner);

	_Menvironment=getExecutionEnvironment(); // the currently executing environment will be referenced in _Menvironment

	// prepare an interactive session
	if(!interactiveSessionInitialized()){
		outputError("Failed to initialize the interactive session.");
		resetOutputColor();
		exit(2);
	}

	resetOutputColor(); // just in case
	outputInfo("Welcome to M.");
	newline();
	output("Version: %s - Build: %s - Date: %s.\n",M_VERSION,M_BUILD,M_DATE);
	newline();
	displayFlags();
	newline();
	
	// MDH@11NOV2019: at this point getNumberOfValues() still represents the actual number of remembered values (before values are removed from it)
	if(amVerbose())output("M shell initialized with %llu predefined values.\n",getNumberOfValues());

	// done by getShellEnvironment()!!!! pushExecutionEnvironment(_Menvironment);
	
	Mstring* predefinedVariableNames=owned_string(_getVariableNames(getExecutionEnvironment(),", "),owner);
	if(predefinedVariableNames){
		output("Predefined variables: %s.\n",string(predefinedVariableNames));
		FREE_STRING(predefinedVariableNames,owner); // no get rid of it!!!
	}else
		outputInfo("No predefined variables!");
	//////////output("Number of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	_shellCommand=owned_string(__string(),owner_shellCommand); // MDH@12APR2019: allow executing shell commands (calling system())
	// MDH@20SEP2019 removing: feedforwardText=__string(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	_userInputCommand=NULL; // the current (user input) command

	///// writeCommand() will take care of this!!!! getCommandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

	// TODO shouldn't we do this in initEnvironment? (or its alternative initM() yet to be created)
	_immediateFeedforwardText=owned_string(__string(),owner_immediateFeedforwardText);if(!_immediateFeedforwardText)outputError("Failed to allow immediate feed forward"); // TODO we can do better than this!!
	_suggestedText=owned_string(__string(),owner_suggestedText);if(!_suggestedText)outputError("Failed to allow suggested text");
	/* do not initialize _manualFeedforwardText because there's now a difference between manual feed forward being NULL or empty (not blocking vs blocking identifier continuation)
	_manualFeedforwardText=__string();if(!_manualFeedforwardText)outputError("Failed to allow manual feed forward");
	*/
	
	char inputChar,inputCharType;

	outputInfo("");
	outputInfo("Use Ctrl-Z to exit M immediately at any time.");
	outputInfo("In any mode press the Enter key on an empty line to switch modes.");

	// let's mark the allocations BEFORE we start looping

	// MDH@13MAR2020: echo all requested output to the log file as well, I suppose we should use a timestamp in the name, so we get a different log for each session
	Mstring* _outputFilename=owned_string(_getTimestamp("%Y-%m-%d.%H:%M:%S"),owner);
	if(_outputFilename){
		if(string_prepend(_outputFilename,"M.")&&string_append(_outputFilename,".log")){
			if(setOutputFilename(string(_outputFilename))){
				dontEchoToOutputFile(); // turn off what setOutputFilename turned on
				Mstring* _sessionStartTimestamp=owned_string(_getTimestamp(NULL),owner);
				if(_sessionStartTimestamp){
					outputToFile("M session start at ",string(_sessionStartTimestamp),".\n");
					FREE_STRING(_sessionStartTimestamp,owner);
				}else
					outputBug("Failed to obtain a session start timestamp.");
				output("Session information will be written to %s.\n",string(_outputFilename));
			}else
				output("%sFailed to open %s for writing session information to.\n",M_ERROR_PREFIX,string(_outputFilename));
		}else
			outputError("No session log will be written, due to failing to compose the output filename.");
		FREE_STRING(_outputFilename,owner);
	}

	if(getNumberOfAllocationMarks()==0){
		if(!allocationMarkAdded()){
			outputError("Failed to create the first allocation mark!");
			exit(3);
		}
	}

	while(1){ // command loop

		outputTotalMemoryUsage(); // have to think about this though

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//                if _userInputCommand->_firstToken is NOT null, we have to make it NULL
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
			//                NOTE registered commands should always be successfully evaluated, so do NOT get rid of any pending command!!!!
			// MDH@24SEP2020: the result of outputCommand will now be the number of command characters written on the last command line, and therefore should be assigned to numberOfLineCommandCharacters!!!!
			if(_userInputCommand)numberOfLineCommandCharacters=outputCommand(_userInputCommand);else deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0);
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
				//                although it is shown in the same color
				// MDH@07OCT2019: now decided to ALWAYS update the identifier continuation BUT it will be merged with the manual feed forward text
				updateUserInputCommandIdentifierContinuation();
				// if we have manual feed forward starting with the given identifier continuation, the identifier continuation will remain
				// and the identifier continuation will be removed from the manual feed forward
				if(_manualFeedforwardText){ // existing manual feed forward text that may block identifier continuation characters
					if(amDebugging())inputInfo("Determining manual feed forward text.");
					// if _manualFeedforwardText is empty ANY identifier continuation will be blocked (e.g. when a single identifier continuation character is removed)
					numberOfIdentifierContinuationManualFeedforwardCharacters=string_number_of_matching_chars(_manualFeedforwardText,_identifierContinuationCharacters);
					// MDH@08OCT2019: when the manual feed forward matches the start of the identifier continuation use the latter
					//                this poses a problem though, the alternative being to always show the identifier continuation behind the manual feed forward
					//                which means that we have to get rid of the identifier continuation if we are not showing it
					//                the point is we cannot simply get rid of the manual feed forward text being the result of a number of left arrow actions
					//                we can only augment it with identifier continuation although the identifier continuation would reappear automatically when we do
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
				////////////if(amVerbose()||!amDebugging())inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
				// if we do NOT have manual feed forward text, 'update' the immediate feed forward text i.e. only show immediate feed forward text when there's no manual feed forward text!!!
				// get rid of the current immediate feed forward text and update it
				string_setlength(_immediateFeedforwardText,0/*,owner_immediateFeedforwardText*/);
				////outputChar('A');
				if(!_manualFeedforwardText||string_length(_manualFeedforwardText)==0)updateImmediateFeedforwardTextOfUserInputCommand();
				////outputChar('B');
				// MDH@03OCT2019: some feed forward texts are also current token specific, therefore we need to sync the feed forward texts
				//                TODO perhaps we should distinguish between feed forward and auto completion (as with the brackets)
				//                DONE solved this by taking care of immediate feed forward texts whenever the current token (type) changes
				///////////updateFeedforwardTexts();
				updateAutoCompletionText(); // to force it being reconstructed!!! TODO if we decide to always do that we do not need to do this here!!!
				////outputChar('C');
				showSuggestedText();
				////outputChar('D');
				if(amDebugging())outputDebugInfo();
				/////outputChar('E');

				// if the line is full now we put the character on the next line
				// if(numberOfLineCommandCharacters+promptLength==numberOfLineCharacters){oneLineDown();showContinuedPrompt();}

			}
			////////outputChar('X');

			// ask the user for input
			// MDH@30JUN2020: blocking call inputCharRead() replaced by a non-blocking call that allows executing updateNumberOfLineCharacters after each 1/10 second timeout
			if(!inputCharReadNonBlocking(&inputChar,&updateNumberOfLineCharacters))break; // let's see how the terminal window wraps... &updateNumberOfLineCharacters))break;

			// outputChar(inputChar);

			// hide the suggested text again before processing the character read
			if(inputMode==IM_COMMAND){
				hideSuggestedText();
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
				if(!amDebugging())outputStatus(inputChar,inputCharType);
			}
			*/
			////////printf("(%d)",inputCharType);

			// if not in control mode, and the switch to control mode character is entered, switch to control mode if first character (NOTE getUserInputLength() is only defined in the other two modes)
			// MDH@16APR2019: I want to use the Enter key (ASCII 13) to switch to the next mode, because the associated input character type is n which will ALWAYS break
			//                in that case we do NOT need the o input character type!!!
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

			if(inputCharType=='n'){ // end-of-line (CR of LF) character
				// MDH@27NOV2019: how about treating the Enter key as line break when the token is finished
				/////////outputChar('X');
				///////inputInfo("Checking for command continuation");
				if(inputMode==IM_COMMAND){
					if(_userInputCommand&&_userInputCommand->_lastToken){
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
						int8_t aValidCommandIndicator=isAValidCommandIndicator(_userInputCommand,owner_userInputCommand,false);
						inputInfo("Valid command indicator: %d.",aValidCommandIndicator);
						// not using \ for newline continuation forces me to actually check whether the command is valid!!
						if(aValidCommandIndicator<=0){ // MDH@10MAR2020: use false for the report parameter because isAValidCommand uses outputInfo/Error which we cannot use during user input!
							/////////inputInfo("User newline break");
							inputCharType='W';
							inputChar='\\'; // TODO should we do this more generic????
							switch(aValidCommandIndicator){
								case   0:inputInfo("Invalid or empty command.");break;
								case  -1:inputInfo("Erroneous command.");break;
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
				if(inputCharType=='d'){ // MDH@18APR2019: delete now always deletes the first character in the behind cursor text
					/////debugWrite("DELETE");
					// MDH@20SEP2019: equivalent to removing ANY first character in the first feed forward text (if any)
					// MDH@27SEP2019: removing the identifier continuation text takes precedence!!!
					// MDH@07OCT2019: manual feed forward now comes first (in showing)
					if(!_manualFeedforwardText&&string_length(_manualFeedforwardText)>0){
						// remove one character at a time (TODO might be confusing with the removal of the entire identifier continuation on Delete)
						if(!getFirstManualFeedforwardCharacterRemoved())inputCharType=switchToControlMode("Failed to delete the first suggested character.");
					}else
					if(_identifierContinuationCharacters){
						// MDH@30OCT2019: by blocking the subsequent update (the next time), we loose the identifier continuation automatically
						userInputCommandIdentifierContinuationNeedsUpdating=false; // ascertain to block or not block accordingly (i.e. if we're in an identifier don't block, either wise block)
						/// replacing: deleteIdentifierContinuation();
						///////writeSuggestedText(false);
					}else
					if(_firstTokenautocompletiontext){ // replacing: string_length(feedforwardText)){ // something to delete
						if(!getFirstAutocompletionCharacterRemoved())inputCharType=switchToControlMode("Failed to remove the first character in the suggested text.");
					}else // no first feed forward text (and character)!
						beep();
				}else
				if(inputCharType=='b'){ // backspace
					///////debugWrite("BACKSPACE");
					// something to remove?
					if(getUserInputLength()){ // TODO _userInputCommand->_firstToken should be NULL at the same time getCommandLength() becomes 0!!!
						if(!removePreviousTokenCharacter())
							switchToControlMode("Failed to remove the previous token character. Possible cause: out of memory.");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='h'){
					if(_userInputCommand)
						inputInfo("Press Enter to execute the command, Ctrl-C to clear the command, Ctrl-Z to exit M immediately.");
					else
						inputInfo("Use Ctrl-Z to exit M immediately, press the Enter key to switch to Control mode.");
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					// replacing: if(_userInputCommand->_firstToken!=NULL){clearCommand();break;}beep(); 
					if(!_userInputCommand){
						inputInfo("Nothing to clear. (Use Ctrl-H for some help.)");
						beep();
					}else
					if(_userInputCommand->_firstToken){
						/////////if(amWrapping()())break; // if in amWrapping()() can't guarantee backspace() to move into the previous line which means just prompt again...
						// MDH@03SEP2019: what to do when Ctrl-C is called on a previous command????? i.e. when _userInputCommand->_firstToken points to a previous command, I'd say that we should return to the current command
						if(commandIndex)
							setCommandIndex(0);
						else
							cancelCommand();
					} else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// MDH@03SEP2019: here commandCharacterAccepted() will take care of copying the command if commandIndex is not (yet) zero
					// if there's a preview (well, code completion by way of a feedforwardText)
					// MDH@20SEP2019: I suppose we can simply get the characters from the (constructed) feedforwardText
					//                but that would cause problems in case something goes wrong
					// MDH@25SEP2019: the following goes wrong in those case where the feed forward text is generated, so it should not generate feed forward text until after all feed forward characters are consumed!!!!
					//                we solve this by always passing false for the endOfInput argument!!!
					/* replacing:
					if(_firstTokenautocompletiontext){
						char newInputChar;
						while((newInputChar=getFirstAutocompletionCharacterRemoved())){
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],false,true)){
								beep();
								inputCharType=switchToControlMode("Failed to accept a suggested character.");
								break;
							}
						}
						// MDH@25SEP2019: we now need to generate the completion text again
						deleteTokenautocompletiontexts(); // APPARENTLY I need to do this to force writeSuggestedText(true) to reconstruct it (although I thought updateLastTokenAutocompletionText() would call deleteTokenautocompletiontexts())
						updateLastTokenAutocompletionText();
						writeSuggestedText(true); // TODO can we not find a better way to do this??????
					}else
						beep();
					*/
					// the original way of doing this (by computing the behind cursor text and consuming it)
					// MDH@26SEP2019: how about only consuming the identifier continuation text if we have it as a service to the user????? I think that makes sense doesn't it
					//                updateBehindCursorText() adjusted to return (if available) a pointer to the characters in the feed forward text (TODO change behind cursor into feed forward when appropriate)
					// MDH@04OCT2019: are we going to consume in parts????? NOTE if there's no identifier continuation text, the immediate feed forward and auto completion texts to consume are present in _suggestedText (thank god)
					// MDH@07OCT2019: manual feed forward text (as a whole) takes precedence
					size_t numberOfCharactersToConsume=string_length(_suggestedText);
					if(numberOfCharactersToConsume>0){
						// if there are manual feed forward characters it can start with a number of identifier continuation characters to consume as a whole using Tab
						size_t numberOfManualFeedforwardCharacters=(_manualFeedforwardText?string_length(_manualFeedforwardText):0);
						size_t numberOfIdentifierContinuationCharacters=(numberOfManualFeedforwardCharacters?numberOfIdentifierContinuationManualFeedforwardCharacters:(_identifierContinuationCharacters?strlen(_identifierContinuationCharacters):0));
						// the feed forward text that is colored differently (identifier continuation and manual feed forward text is to be accepted as a whole when Tab is used)
						// it's easiest to do that by specifying the number of suggested characters to consume
						if(numberOfIdentifierContinuationCharacters>0)numberOfCharactersToConsume=numberOfIdentifierContinuationCharacters;
						else
						if(numberOfManualFeedforwardCharacters>0)numberOfCharactersToConsume=numberOfManualFeedforwardCharacters;
						char newInputChar='\0',newInputCharType='\0';
						size_t numberOfSuggestedCharactersAccepted=0;
						while(numberOfSuggestedCharactersAccepted<numberOfCharactersToConsume){
							newInputChar=string_char(_suggestedText,numberOfSuggestedCharactersAccepted);
							if(!newInputChar){
								inputCharType=switchToControlMode("Suggested characters vanishing somehow.");
								break;
							}
							// MDH@24APR2019 obsolete: getCommandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							// MDH@14AUG2019: suggestedCharacter is set to true now, this makes perfect sense as I'm consuming all characters here and we do not want to remove them, NOTE that characters may still be inserted but only when bc=0 obviously
							newInputCharType=INPUTCHARACTERTYPES[newInputChar];
							if(newInputChar!='#'&&!commandCharacterAccepted(newInputChar,&newInputCharType,false,true)){
								inputCharType=switchToControlMode("Failed to consume a suggested character.");
								newInputChar='\0'; // to indicate some error occurred
								break;
							}
							if(newInputCharType==' ')newCommandLine(true); // MDH@24SEP2020 replacing (and improving upon): showContinuedPrompt(true,true); // MDH@31OCT2019: whenever a newline (request) character is consumed, make a new line
							numberOfSuggestedCharactersAccepted+=1;
						}
						// remove at most numberOfSuggestedCharactersAccepted from the suggested text
						if(inputMode==IM_COMMAND){
							// if identifier continuation characters were consumed nothing to do, otherwise either to clear the manual
							// MDH@08OCT2019: have to be careful here because identifier continuation characters might come out of the manual feed forward text
							if(numberOfSuggestedCharactersAccepted){ // at least one character consumed
								if(numberOfSuggestedCharactersAccepted==numberOfCharactersToConsume){
									if(numberOfManualFeedforwardCharacters){ // some of the manual feed forward characters were consumed
										if(numberOfSuggestedCharactersAccepted>=string_length(_manualFeedforwardText)){ // all manual feed forward characters were consumed
											FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;
										}else{ // not all manual feed forward characters were consumed 
											// remove numberOfSuggestedCharactersAccepted from the start of the manual feed forward text
											if(!string_removed(_manualFeedforwardText,0,numberOfSuggestedCharactersAccepted))
												inputCharType=switchToControlMode("Not all accepted suggested characters removed from the suggested text.");
										}
									}else
										deleteTokenautocompletiontexts();
									updateLastTokenAutocompletionText(); // TODO do we need the following????
								}else
									inputCharType=switchToControlMode("Not all suggested characters accepted.");
								// we might end up behind some character that produces auto completion stuff like [ or {
							}else
								inputCharType=switchToControlMode("No suggested characters accepted.");					
						}
					}else // no consumable text
						beep();
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
											/////////inputInfo("Delete");
											// we should have suggested (identifier continuation or feed forward (autocompletion)) text
											// MDH@07OCT2019: manual feed forward text goes first
											if(_suggestedText&&string_length(_suggestedText)){ // there is suggested text with parts to delete
												// any identifier continuation characters precede manual feed forward text
												if(string_length(_manualFeedforwardText)){
													if(getFirstManualFeedforwardCharacterRemoved()){
														if(string_length(_manualFeedforwardText)==0){FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;}
													}else
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else
												if(_identifierContinuationCharacters&&strlen(_identifierContinuationCharacters)>0){
													// if there's immediate feed forward token, there's no manual feed forward
													// PROBLEM can't remove the identifier continuation characters as a whole because if I do it will be recreated
													if(!prependedToManualFeedforwardText(_identifierContinuationCharacters+1))
														inputCharType=switchToControlMode("Failed to delete the first suggested character.");
												}else{
													// MDH@06OCT2019: as long as the user is manually deleting identifier continuation characters
													//                we prevent the automatic update of the identifier continuation?????
													// MDH@06OCT2019: it makes sense to let Delete consume only a single character, otherwise the user might get confused
													//                because sometimes only one character is consumed, and sometimes more
													//                so to keep it simple we consume a single character at a time
													//                BUT if there are characters left we move these into the manual feed forward
													//                that way no identifier continuation will show until the user consumed the entire manual feed forward
													//                this makes sense because the identifier continuation itself won't change while using Delete only
													//                HOWEVER what should happen if the user types another character thus changing what's in front of the
													//                manual feed forward?????
													if(string_length(_immediateFeedforwardText)==0){ // MDH@20SEP2019 replacing: string_length(feedforwardText)){
														if(!getFirstAutocompletionCharacterRemoved()) // MDH@20SEP2019 replacing: string_removed_char(feedforwardText,0))
															inputCharType=switchToControlMode("Failed to delete the first character of the suggested text.");	
													}else{ // remove the immediate feed forward text
														string_setlength(_immediateFeedforwardText,0/*,owner_immediateFeedforwardText*/);
														/////////////immediateFeedforwardToBeUpdated=false; // do not update next time
													}
												}
											}else
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(!commandIndex&&_userInputCommand)
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
										if(!commandIndex&&_userInputCommand)
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
									// MDH@27SEP2019: don't forget the continuation text as well!!!
									if(string_length(_suggestedText)){
										char c=string_char(_suggestedText,0);
										if(c){
											// MDH@31OCT2019: this might well be a newline character!!!
											char suggestedInputCharType=INPUTCHARACTERTYPES[c];
											// MDH@21SEP2020: it is a suggested character isn't it????? didn't help changing false to true!!!!
											if(commandCharacterAccepted(c,&suggestedInputCharType,true,false)){
												inputCharType=suggestedInputCharType; // MDH@31OCT2019: because might have changed!!!
												if(inputCharType==' ')newCommandLine(true); // MDH@24SEP2020 replacing and improving upon: showContinuedPrompt(true,true); // MDH@31OCT2019: we just consumed a newline (request) character
												// where to remove it from????
												// NOTE identifier continuation and immediate feed forward are redetermined automatically so do not need to be adjusted here
												if(string_length(_manualFeedforwardText)){
													if(getFirstManualFeedforwardCharacterRemoved()!=c)
														inputCharType=switchToControlMode("Wrong first suggested character accepted!");
												}else
												if(!_identifierContinuationCharacters||strlen(_identifierContinuationCharacters)==0){
													if(string_length(_immediateFeedforwardText)==0){
														if(!getFirstAutocompletionCharacterRemoved())
															inputCharType=switchToControlMode("Failed to remove the accepted first auto completion character.");
													}
												}												
											}else
												inputCharType=switchToControlMode("First suggested character not accepted.");
										}else
											inputCharType=switchToControlMode("First suggested character vanished.");
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									if(getUserInputLength()){
										// TODO apparently _userInputCommand->_firstToken will still be NULL when we're scrolling through the list of previous commands...
										// MDH@03SEP2019: BUG FIX forgot to make commandIndex 0 when copying the command (as copyUserInputCommand() itself does not seem to do that!!!)
										if(commandIndex){commandIndex=0;copyUserInputCommand();} // will also set getCommandLength()!!!
										// MDH@27FEB2019: we should remove the last character of the current token (and command) and move it into feedforwardText
										// MDH@20SEP2019: we do NOT want the character removed to disappear when the token it came from disappears, therefore the addition to the feed forward text should be anonymous
										// MDH@25SEP2019: because we're going to prepend c to the feed forward text, we have to determine the associated token i.e. the token that generated c
										//                i.e. if the character being moved would have been auto-generated we want to know the associated token
										//                the problem now is that removedTokenCharacter() will already remove the token when its a single-character token
										//                TODO something is not correct because if removedTokenCharacter() takes care of removing the token which check again below????????
										//                it's easiest to check whether the token will be removed: this will be the case if there's only one character in the token
										//                in which case that will be the originating token but only in the situation where c matches the feed forward character of that expression
										///////Mtoken* startOfExpressionToken=(string_length(_userInputCommand->_lastToken->text)==1?_userInputCommand->_lastToken->expr:NULL); // MDH@25SEP2019: remember what the start of expression token associated with the current last command token is
										char c=removedTokenCharacter(true); // passing true will force removedTokenCharacter() to actually check an identifier token type
										if(c){ // removing the character behind the cursor succeeded
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
											// MDH@30SEP2019: this is the only call to getAutocompletionTextOfCharacterPrepended() therefore we can simply adjust that function to check whether this character matches a consumed character!!!
											//                but I guess failing to do so is not that terrible that we should switch to control mode
											// MDH@01OCT2019: if this character also starts the new identifier continuation, it should not be anonymously prepended 
											//                let's construct the identifier continuation text that we would get 
											//                CORRECTION checking against the first identifier continuation text suffices (instead of comparing with the entire new identifier continuation)
											// MDH@04OCT2019: there was so much going wrong doing the following that I simply replaced it by prepending the consumed character to the feed forward texts and leaving it
											//                to the input loop part to deal with identifier continuations that match the start of the feed forward text(s)
											// MDH@07OCT2019: I've introduced a new feed forward element (manualFeedforwardText) to contain the part of the text
											//                that the user took out of the (tokenized) command to e.g. correct a command
											if(!manualFeedforwardCharacterPrepended(c)) // replacing: if(!getAutocompletionTextOfCharacterPrepended(c,true))
												inputCharType=switchToControlMode("Failed to accept the removed command character as suggested text.");
											/*
											else
											if(amVerbose())
											inputInfo("Manual feed forward: '%s'.",string(_manualFeedforwardText));
											*/
											/* replacing:
											updateUserInputCommandIdentifierContinuation();
											///outputChar('1');
											// prepend only anonymously when not matching the identifier continuation character!!
											if(!_identifierContinuationCharacters||_identifierContinuationCharacters[0]!=c)
											if(!getAutocompletionTextOfCharacterPrepended(c,false))
											inputError("Failed to accept the removed command character as suggested text.");
											///outputChar('2');
											updateLastTokenAutocompletionText(); // if we haven't updated the identifier continuation text do it again	
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
											if(!getAutocompletionTextOfCharacterPrepended(c))
											inputError("Failed to accept the removed command character as suggested text.");
											updateLastTokenAutocompletionText(identifierContinuationText==NULL); // if we haven't updated the identifier continuation text do it again	
											writeSuggestedText(false);
											if(identifierContinuationText)FREE_STRING(identifierContinuationText);
											*/
										}else
											inputCharType=switchToControlMode("Failed to remove the last command character.");
									}else
										beep();
								}else
									beep();
							}
						}
					}
				}else{
					// MDH@21APR2019: creating a command if need be is delegated to commandCharacterAccepted() which we know
					//                we always need a command (being edited)
					// MDH@01OCT2019: if the input character matches the first non-anonymous i.e. autogenerated feed forward character same functionality as right arrow (except now we know the character entered)
					if(inputChar==getFirstSuggestedCharacter(true)){
						char firstSuggestedCharacterConsumed=getFirstSuggestedCharacterConsumed(inputChar,true);
						if(firstSuggestedCharacterConsumed)removeFirstSuggestedCharacter(firstSuggestedCharacterConsumed,false);
						else inputCharType=switchToControlMode("Failed to accept the matching first suggested character.");
					}else{
						// MDH@31OCT2019: when the accepted character is the backslash in whitespace we should continue with the command on the next line
						//                OOPS a backtick inside a string is also recognized as such which shouldn't happen
						//                ALSO because the backtick will be visible it's probably better to insert an empty token for it of type TT_NEWLINE or something like that
						//                it's probably best to check whether to accept a backtick here???? NOTE we could have backticks in commands read from files as well????
						if(commandCharacterAccepted(inputChar,&inputCharType,true,false)){
							// outputChar('X');
							if(inputCharType==' '){ // a newline request (whenever M_NEWLINE_CHARACTER is input at a functional position)
								newCommandLine(true); // MDH@24SEP2020 replacing and improving upon: showContinuedPrompt(true,true);
								//////////showSuggestedText(); // we have to rewrite the suggested text though
								///////////outputTokenColor(_userInputCommand->_lastToken); // and show the right color
							}
						}else
						if(inputCharType!='`') // not a new line request character
							inputCharType=switchToControlMode(_userInputCommand->_firstToken?"Failed to accept the character.":"Failed to create a new command.");
						else
							inputError("New line request character not allowed here");
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
				}
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
						if(!settingApplied(inputChar))output("%sSetting character '%c' not recognized.\n",M_ERROR_PREFIX,inputChar);
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
					if(string_length(_suggestedText)){ // something behind the cursor that we can remove
						if(string_removed_char(_suggestedText,0))
							///////writeSuggestedText(true)
							;
						else
							inputCharType=switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
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
					uint16_t bc=getNumberOfSuggestedCharacters();
					if(bc){
						while(bc--){
							char newInputChar=string_removed_char(_suggestedText,0);
							if(!newInputChar){
								///////writeSuggestedText(true);
								inputCharType=switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							if(newInputChar!='#')string_append_char(_shellCommand,newInputChar);
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharReadNonBlocking(&inputChar,NULL)){////inputChar=getInputChar();
						if(inputChar==91){
							if(inputCharReadNonBlocking(&inputChar,NULL)){/////inputChar=getInputChar();
								if(inputChar==51){
									if(inputCharReadNonBlocking(&inputChar,NULL)){///////inputChar=getInputChar();
										if(inputChar==126){ // delete
											// TODO FIX this does not seem to be right!!!!!
											if(getNumberOfSuggestedCharacters()){
												// we could go one to the right and do a backspace!!
												moveCursorRight(1);
												// TODO what to do here??? removePreviousTokenCharacter();
											}else // nothing under the cursor to delete
												beep();
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
									if(getNumberOfSuggestedCharacters()){
										char newInputChar=string_removed_char(_suggestedText,0);
										if(!newInputChar){
											////////writeSuggestedText(true);
											inputCharType=switchToControlMode("Failed to accept the suggested characters.");
										}else
											string_insert_char(_shellCommand,getUserInputLength(),newInputChar);
									}else
										beep();
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
		//                but this seems to be a very good place for it
		//                _manualFeedforwardText is only used in command and shell mode, not in control mode
		//                perhaps it's better done at the prompt
		//                TODO consider doing the same for feed forward texts?????
		if(_manualFeedforwardText){FREE_STRING(_manualFeedforwardText,owner_manualFeedforwardText);_manualFeedforwardText=NULL;}

		// if eXit input character(s) received...
		if(inputCharType=='x'){
			// we should only exit M when not entering a function body
			if(!getCurrentFunctionBodyInput())break; // break out of user input loop
			// switch back to command mode
			endFunctionBodyInput();
			switchToCommandMode();
		}else
		// MDH@16APR2019: now if we use n to switch modes as well, we can do that if there's no command
		if(inputCharType=='n'){
			if(inputMode==IM_COMMAND){
				// MDH@21JUL2019: if the last token appears to be a function identifier change it to a variable
				//                so we won't end up with refusal of evaluation
				if(_userInputCommand&&_userInputCommand->_lastToken&&_userInputCommand->_lastToken->type==TT_FUNCTION){
					changeFunctionTokenToAVariable(_userInputCommand,false); // MDH@28FEB2020: command now passed in to the function call as well (see Mshell.c)
					inputInfo("%s","Assumed function apparently a variable!");
				}else
					inputInfo("%s",""); // so that line will be empty
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				clearScreenFromCursor(); // so we won't see the behind cursor text anymore
			}
			outputChar('\n');
			if(inputMode==IM_COMMAND){ // the newline character ends the command to be evaluated!!
				// if _userInputCommand->_firstToken is set, we have a command to evaluate
				/*
				Mtoken* _userInputCommand->_firstTokenToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(_userInputCommand->_firstToken){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(getUserInputLength()>string_length(_userInputCommand->_firstToken->text)){
						// finish the last token???
						if(_userInputCommand->_lastToken->significantCharacterCount==0)_userInputCommand->_lastToken->significantCharacterCount=string_length(_userInputCommand->_lastToken->text);
						_userInputCommand->_firstTokenToEvaluate=_userInputCommand->_firstToken; // but only when not at start of command!!!
						if(amDebugging())outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&getUserInputLength()) // NOTE using getUserInputLength() is better than using amAcceptinghistorycommand() (causing it!!)
					_userInputCommand->_firstTokenToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(_userInputCommand&&_userInputCommand->_firstToken){ // technically something to evaluate
					if(amVerbose())outputCommandInfo(_userInputCommand);
					// MDH@11MAY2020 obsolete: size_t mark=allocationmark();if(amVerbose())output("Mark: %zu.\n",mark);
					Mvalue* userInputCommandResultValue=NULL;
					bool commandEvaluated=evaluateCommand(&userInputCommandResultValue);
					newline();
					// let's mark the allocation directly behind evaluating the command
					if(allocationMarksAdded>0){
						if(allocationMarkAdded())allocationMarksAdded++;else outputError("Failed to mark the allocations after evaluating the command.");
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
						continue;
					}
					resetOutputColor();
					if(amVerbose())outputInfo("Command evaluated!");
					deleteTokenautocompletiontexts(); // MDH@20SEP2019 replacing: string_setlength(feedforwardText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
					// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
					// MDH@18JUN2020: if the current command is not an original command 
					if(!registerCommand(_userInputCommand,(commandIndex>0?owner_registeredcommands:owner_userInputCommand))){
						// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
						if(commandIndex==0){ // a new command being registered!!!
							FREE_COMMAND(_userInputCommand,owner_userInputCommand); // MDH@29OCT2019 replacing: freeToken(_userInputCommand->_firstToken);
							outputError("Failed to register the command! Probable cause: out of memory");
						}else
							outputError("Failed to register the command again! Probable cause: out of memory");
					}else{
						if(amVerboseDebugging())outputInfo("Command registered!");
						if(M_value){
							// perhaps we should store the command text not the command itself?????
							// NOTE prepend a single quote is essential to get the text enquoted!!!
							if(!string_insert_char(_userInputCommandText,0,'\'')||!registerCommandEvaluation(string(_userInputCommandText),userInputCommandResultValue,commandCount))
								outputWarning("Failed to store the command and the value it evaluates to for use in subsequent commands.");
							else
							if(amVerboseDebugging())output("User input command and result stored in %s.\n",M_VARIABLE_NAME);
						}
					}
					FREE_STRING(_userInputCommandText,owner); // MDH@14NOV2019: freed

					// start anew (without a current command to evaluate!!!!) NOTE the memory is either still pointed to in `commands` or freed because it failed to bind it in commands so we're free to NULL the pointer here!!!
					_userInputCommand=NULL; // MDH@29OCT2019 replacing non Mcommand style (before today): _userInputCommand->_lastToken=_userInputCommand->_firstToken=NULL; // remove reference to current command

					// garbage collection: remove any values not used anymore...
					// if(amDebugging())
					if(amVerbose())outputInfo("Removing unreferenced values.");
					size_t removedValueCount=getNumberOfRemovedValues(amVerbose()&&amDebugging()); // MDH@12MAY2020: debugging needs to be set to view information on the values released
					if(amVerbose())
					{if(removedValueCount)output("Number of garbage collected values: %lu.\n",removedValueCount);else outputInfo("No garbage collected values.");}

					// switch to function body input mode when this command contained at least one user function definition
					// (even when dealing with currently inputting function body commands)
					if(getFirstFunctionBodyRequest()&&!startFunctionBodyInput())outputError("Failed to start requesting the body of a new function");

					// MDH@12MAY2020: output two incremental out
					if(allocationMarksAdded>0){
						outputTotalMemoryUsage();
						if(outputIncrementalMemoryUsage(allocationMarksAdded)<allocationMarksAdded)outputError("Not all command allocation marks output.");
						while(--allocationMarksAdded>=0)if(!oldestAllocationMarkDropped())break; // drop as many allocation marks as we have created
					}

				}else{
					// MDH@14AUG2019: if a user presses Enter when there's no command but still feedforwardText it looses feedforwardText but we do switch to the control mode as I think that is what the user wants (if only to look at the list of variables)
					//                NOTE that I might consider keeping feedforwardText, so it will be redisplayed when the user returns to the command mode
					switchToControlMode(NULL); // replacing: if(getNumberOfSuggestedCharacters()==0)switchToControlMode(NULL);else outputError("Still suggested text");
				}
			}else
			if(inputMode==IM_SHELL){
				if(string_length(_shellCommand))
					execute_shellCommand();
				else // MDH@16APR2019: back to command mode
					switchToCommandMode();
			}else // Return key in control mode, always to return to command input!!
				switchToCommandMode();
		}
		showSeparatorLine();
		if(!allocationMarkAdded())outputError("Failed to add a new memory allocation mark.");
	}
	// 'normal' exit
	exit(0);
}
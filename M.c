// remove the line below when not in debug mode
//#define __DEBUG__

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "Msettings.h"
#include "Moutput.h"
#include "Msession.h"

void writeTimestamp(FILE* _file){
	if(_file){
    time_t now=time(NULL);
		struct tm * nowlocal=localtime(&now);
    char buffer[50];strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",nowlocal);
    fprintf(_file,"%s\t",buffer);
	}
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

// Mexecution includes mstring.h
#include "Mexecution.h"

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

const long double LD_PI=3.141592653589793238462643383279L; // 30 decimal digits of PI
const long double LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E

// how about storing all results here?????? instead of in the root environment????
Mvalue* _resultListValue=NULL; // were the results are being kept
// the function that is used to return a specific result value
Mvalue* getResult(Menvironment* _executionEnvironment,Mvalue* _index){
	if(amVerbose())output("\nResult requested!");
	if(!_index)return _resultListValue;
	return getValueAtIndex(_resultListValue->value._list,_index);
}

Menvironment* _Menvironment; // this is the root (M) environment
Menvironment* _executionEnvironment=NULL; // the current execution environment (in which functions are called!!!)

bool initEnvironment(){
	_resultListValue=_getListValue(VT_UNDEFINED); // ascertain to have a list value in which the results can be stored
	// ESSENTIAL not to loose this list immediately!!!
	if(_resultListValue)incrementReferenceCount(_resultListValue);else outputLine("WARNING: Failing to create the results list. The results will not be available through the M function!");
	_Menvironment=calloc(1,sizeof(Menvironment));
	if(_Menvironment){
		Mmap* environmentVariableMap=calloc(1,sizeof(Mmap));
		Mfunctionmap* environmentFunctionMap=calloc(1,sizeof(Mfunctionmap));
		if(environmentVariableMap&&environmentFunctionMap){
			_Menvironment->_variableMap=environmentVariableMap;
			// create and add PI and E constants!!!
			Mvalue* PI_value=_getRealValue(LD_PI);
			if(!PI_value){
				outputLine("ERROR: Failed to create PI.");
				return false;
			}
			if(!addVariable(_Menvironment,"PI",VT_REAL,true)){
				outputLine("ERROR: Failed to add PI.");
				///////free_value(PI_value);
				return false;
			}
			if(!setValue(_Menvironment,"PI",PI_value)){
				////////free_value(PI_value);
				outputLine("ERROR: Failed to initialize PI.");
				return false;
			}
			Mvalue* E_value=_getRealValue(LD_E);
			if(!E_value){
				///////free_value(E_value);
				outputLine("ERROR: Failed to create E.");
				return false;
			}
			if(!addVariable(_Menvironment,"E",VT_REAL,true)){
				outputLine("ERROR: Failed to add E.");
				return false;
			}
			if(!setValue(_Menvironment,"E",E_value)){
				//////free_value(E_value);
				outputLine("ERROR: Failed to initialize E.");
				return false;
			}
			/*
			// we're going to store all commands in a list called M
			Mvalue* Mvalue=_getListValue(VT_UNDEFINED);
			if(!M_value){
				outputLine("ERROR: Failed to create the M result list.");
				return false;
			}
			if(!addVariable(_Menvironment,"M",VT_LIST,true)){
				outputLine("ERROR: Failed to add the M result list.");
				return false;
			}
			if(!setValue(_Menvironment,"M",M_value)){
				outputLine("ERROR: Failed to initialize the M result list.");
				return false;
			}
			*/
			_Menvironment->_functionMap=environmentFunctionMap;
			if(!registerInternalFunctions(_Menvironment)){
				outputLine("ERROR: Failed to register all internal functions.");
				return false;
			}
			// additional functions some of which need to know the root environment, I suppose a function should have access to its environment?????
			if(_resultListValue&&!completedIntegerFunction(newFunction(_Menvironment,"M"),getResult)){
				outputLine("ERROR: Failed to register function M (for requesting previous results).");
				return false;
			}
		}
	}
	return true;
}

// user interaction stuff
#include "Msession.h"

/* sometimes we want to preformat text
char* getFormattedText(char* fmt,uint8_t maxlength,...){
	char str[maxlength+1];
	va_list args;va_start(args,fmt);sprintf(str,fmt,args);va_end(args);
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

mstring* _getFunctionMapText(Mfunctionmap* _functionmap){
	mstring* s=string_create();
	if(s){
		mstring* p=string_append_char(s,'[');
		if(_functionmap){
			///printf("\n%s(%d)",string(p),_functionmap->numberOfFunctions);
			Mfunctionmapelement* _functionmapelement=_functionmap->_first;
			while(_functionmapelement){
				///printf("\n%s","start");
				Mfunction* _function=_functionmapelement->_function;
				if(!_function)continue;
				///printf("\n%s","func");
				p=string_append(p,string(_function->_name));
				if(!p)break;
				///printf("\n%s","name");
				// I guess we might show the parameter map (if any)
				string_append_char(p,'(');
				if(_function->_parameterMap){
					mstring* parameterMapText=_getMapText(_function->_parameterMap);
					if(parameterMapText){
						string_append(p,string(parameterMapText));
						free_mstring(parameterMapText);
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
		if(!p){free_mstring(s);s=NULL;}
	}
	return s;
}
void outputFunctions(){
	mstring* functionsText=_getFunctionMapText(_Menvironment->_functionMap);
	output("\nFunctions: %s.",string(functionsText));
	free_mstring(functionsText);
}
void outputVariables(){
	// much easier now that we get the text of any Mvalue (like the variable map of an environment!)
	mstring* variablesText=_getMapText(_Menvironment->_variableMap);
	output("\nVariables: %s.",string(variablesText));
	free_mstring(variablesText);
}

enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell

enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode

char* promptinfo[]={"Command mode: cancel the input text with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Wrap|Use history command - Options: eXit|Functions|History|Shell|Variables.","Shell mode: enter a system command to execute."};
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
	output("\nEdit flags: %c%c%c%c%c - Display flags: %c%c.",amAssisting()?'A':'a',amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'s':'S',amAcceptinghistorycommand()?'U':'u',amWrapping()?'W':'w',48+getColorscheme());
}

void outputFlags(){
	output("%c%c%c%c%c%c%c",amAssisting()?'A':'a',(48+getColorscheme()),amDebugging()?'D':'d',amMatchingparentheses()?'M':'m',amVerbose()?'s':'S',amWrapping()?'W':'w',amAcceptinghistorycommand()?'U':'u');
}

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
long long commandCount=0; // the total number of command input
long long commandIndex=0;

mstring* behindCursorText=NULL; // MDH@27FEB2019: we keep track of the characters behind the cursor
mstring* shellCommand=NULL;
Mtoken* pCommandToEvaluate=NULL;
Mtoken* pLastCommandToEvaluateToken=NULL; // the last token in the sequence of tokens starting with pCommandToEvaluate

// keeping track of both the cursor position and the total command length
uint16_t cursorPosition(){return(inputMode==IM_COMMAND?(pLastCommandToEvaluateToken?pLastCommandToEvaluateToken->offset+string_length(pLastCommandToEvaluateToken->text):0):(inputMode==IM_SHELL?string_length(shellCommand):0));}
uint16_t behindCursor(){return string_length(behindCursorText);}
uint16_t commandLength(){return cursorPosition()+behindCursor();}

uint8_t promptLength=0;
void prompt(){
	resetOutputColor();
	///////////printf("%d-",commandIndex);
	char str[11]; // with a maximum of 2,xxx,xxx,xxx 11 positions would suffice
	switch(inputMode){
		case IM_COMMAND:
			sprintf(str,"%lld",(commandCount+1));	// replacing: printf("%lu",(commandCount+1));
			output("M[%s]%s",str," = ");
			clearScreenFromCursor();
			promptLength=strlen(str)+3+3;
			break;
		case IM_CONTROL:
			// how about showing the flags?????
			outputFlags();
			output(" > ");
			promptLength=5+3; // the flags and the control mode prompt
			break;
		case IM_SHELL:
			output("$ ");
			promptLength=2;
			break;
	}
	/////////storeCursor();
	///////////inputMode=true; // expecting a command (until the option character is received)
	/* MDH@26FEB2019: we do not need the following because that's taken care of in writeTokens(pCommandToEvaluate) right after promptForUserInput()
	cursorPosition()=0; // starting at position 0
	*/
}

void promptForUserInput(){
	enableRawmode();
	resetOutputColor();
	output("\n\n%s\n",promptinfo[inputMode]); // show the appropriate input mode prompt info
	prompt();
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

// Token is now defined in Mexpression.h which is included by Mexecution.h so struct Token is indirectly supplied by Mexpression.h!!!

// the list of token type ids in the corresponding order!!!
const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES]={0,0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,0b1000000,0b11111111};

const char* getTokenColor(enum TOKENTYPE_ENUM tokenType){
	uint8_t tokentype_id=TOKENTYPE_IDS[tokenType];
	///////printf("(%d)",tokentype_id);
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
void outputTokenTypeColor(TokenType tokenType){
	setBackColor(getBackgroundColor());
	setColor(getTokenColor(tokenType));
}
void outputTokenColor(Mtoken* _token){
	if(_token)outputTokenTypeColor(_token->type);
	///////printf("[%d]",pLastCommandToEvaluateToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
void outputToken(Mtoken* _token){
	if(!_token)return;
	outputTokenColor(_token);
	// if we allow comments in tokens we're in trouble!!!
	output("%s",string(_token->text));
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}
void outputLastTokenChar(Mtoken* _token){
	///////outputTokenColor(pLastCommandToEvaluateToken);
	outputChar(string_last_char(_token->text));
	//////////resetOutputColor();
}
// MDH@30APR2019: when a function returns to a variable and the other way round
void reoutputToken(Mtoken* _token){
	if(!_token)return;
	moveCursorLeft(string_length(_token->text));
	outputToken(_token); // back where we started (hopefully)
}
/**
 * freeToken() frees the memory @pLastCommandToEvaluateToken points to and returns true on successfully removing the entire chain of tokens it points to
 * @returns the previous token (as we need that )  
 */
Mtoken* freeToken(Mtoken* _token){
	// MDH@30APR2019: let's delegate to free_token()
	Mtoken* _prevToken=NULL;if(_token){_prevToken=_token->prev;free_token(_token);}return _prevToken;
}

// output functions that require access to the current token
void toStartOfPreviousLine(){oneLineUp();toStartOfLine();clearLine();toStartOfLine();}
void toStartOfNextLine(){oneLineDown();toStartOfLine();}
void toCursorPosition(){
	moveCursorRight(promptLength+cursorPosition());
	if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken); // return to the current token color
}

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText
void outputValue(const char* const prefix,const Mvalue* _value,const char* const postfix){if(prefix)output("%s",prefix);if(_value){mstring* _valueText=_getValueText(_value);output("%s",string(_valueText));free_mstring(_valueText);}if(postfix)output("%s",postfix);}

void outputError(const char* const error){if(error&&!strlen(error))output("\nERROR: %s",error);}

void inputInfo(const char* const fmt,...){
	if(fmt&&strlen(fmt)){ // we have a format
		toStartOfPreviousLine();resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toStartOfNextLine();toCursorPosition();
	}
}
void inputError(const char* const fmt,...){
	toStartOfPreviousLine();setColor(getErrorColor());setBackColor(getBackgroundColor());
	va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
	toStartOfNextLine();toCursorPosition();
}
void clearInfo(){toStartOfPreviousLine();resetOutputColor();clearLine();toStartOfNextLine();toCursorPosition();}

void outputStatus(char inputChar,char inputCharType){
	////////printf("[%u,%u]",cursorPosition(),commandLength());
	debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
	if(amDebugging())
		inputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %u | Cursor position: %" PRIu16 " | Command length: %" PRIu16 " | Behind cursor text: '%s'.",inputChar,inputChar,inputCharType,(pLastCommandToEvaluateToken!=NULL?pLastCommandToEvaluateToken->type:255),cursorPosition(),commandLength(),string(behindCursorText));
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
}

Mtoken* newToken(Mtoken* prevToken){
	Mtoken* pNewToken=(Mtoken*)calloc(1,sizeof(Mtoken));
	if(pNewToken){
		// MDH@03MAY2019: if the previous token starts an expression itself, use prevToken itself and not its expr field!!!!
		if(prevToken){
			// finish the previous token
			prevToken->next=pNewToken; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
			if(!prevToken->significantCharacterCount)prevToken->significantCharacterCount=string_length(prevToken->text); // MDH@22MAR2019: if the token character length is NOT set, set it now...
			// initialize the new token
			pNewToken->prev=prevToken; // set the predecessor
			// MDH@18MAY2019: if a , starts an expression we won't be pointing to the opening parenthesis!!!
			//                which would mean that on verification we'd have to jump back until we found a non-comma!!!
			//                so we can fix this by NOT including TT_EXPRESSION prev tokens to point to!!!
			//                BUT the first (dummy) expression token should be included though!!!
			// TODO having to test an expression for starting with ( is a bit of a nuisance (so we won't accidently do that on the initial expression token and any comma token!!!)
			if(prevToken->type==TT_LIST||prevToken->type==TT_FUNCTION_CALL||prevToken->type==TT_MAP||(prevToken->type==TT_EXPRESSION&&string_char(prevToken->text,0)=='(')){
				///////if(amVerbose())inputInfo("%s","Start of list, map or function call remembered!");
				pNewToken->expr=prevToken;
			}else
			if(prevToken->type==TT_END_OF_LIST||prevToken->type==TT_END_OF_FUNCTION_CALL||prevToken->type==TT_END_OF_MAP){
				pNewToken->expr=prevToken->expr->expr;
			}else{
				///////if(amVerbose()){if(prevToken->expr)inputInfo("%s","Copying the start of the list, map or function!");else inputInfo("%s","No list, map or function start to remember!");}
				pNewToken->expr=prevToken->expr;
			}
			///////if(amVerbose()){if(pNewToken->expr)inputInfo("Pointing to %s of type %s.",string(pNewToken->expr->text),TOKENTYPE_STRING[pNewToken->expr->type]);else inputInfo("Nothing to point to.");}
			//////// ending with NULL means all is Ok!! if(!pNewToken->expr)pNewToken->expr=pCommandToEvaluate; // TODO will this help???
			pNewToken->offset=prevToken->offset+string_length(prevToken->text); // set the offset
		}
		// MDH@03MAY2019: TT_EXPRESSION is the default (0) now (always ending at the next non-space character): pNewToken->type=TT_EXPRESSION; // makes more sense to start as expression (same as what we get after a ( or [
		pNewToken->text=string_create();
		/* not needed with calloc() allocation
		pNewToken->significantCharacterCount=0; // MDH@22MAR2019: remembers the amount of significant characters (to be set when the token ends)
		pNewToken->next=NULL;
		*/
	}
	return pNewToken;
}

// keep track of all commands so far
#define COMMAND_BLOCKSIZE 8
Mtoken** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(){
	if(!pCommandToEvaluate)return false;
	if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
		// I have to copy all first token pointers to a new array large enough
		commandBlocks++;
		Mtoken** newCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Mtoken*));
		if(newCommands==NULL)return false;
        commands=newCommands;
	}
	commands[commandCount++]=pCommandToEvaluate;
	return true;
}

// associated every possible input characters (0 through 127) with a character type where a period denotes a non-command input character
// t=tab(feedforward variable),n=newline(end of command),U=unary operator,D=double quoted string literal,C=comment,L=letter (in identifiers),l=letter (not at start of identifier)
// D=digit,d=digit (not at start of numeric value),e=the letter e which may be part of an 'extended' number (or represent the constant e)
// B=binary operator,b=binary operator that cannot be used as first binary operator character,A=assignment operator,
// E=starts an expression(a comma),e=ends and expression ( ) and ]), (NOTE: some characters are best represented by themselves
// all lowercase characters represent control characters, like t=tab, n=newline, x=escape control character,o=switch to control mode,d=delete,b=backspace
// O=operator that can be either unary or binary depending on its position (+ and - characters)
// use x for eXit (e.g. with Ctrl-C and Ctrl-Z), c for cancel command, and m for going into M (control) mode
// as for operators: there are 8 different groups of operators
// !     not unary operator or first character of binary operator !=
// ~     pure unary operator
// -+    sign unary operator or binary minus/plus operator
// %^    pure binary operator
// */    binary operator extensible to make ** power operator or // integer division operator
// <>    binary operator extensible to make << or >> operator but can also be followed by an = sign (is this not the same as */?)
// =     assignment operator that can follow most of the binary operators (except < and >)
// |&    binary or and operator extensible to make || logical or or && logical and operator but the latter cannot be followed by =
// MDH@16APR2019: removing the o input character type (for switching explicitly to or from control mode), replacing it by n, so we can use the backtick for certain purposes...
//                in certain languages it means evaluate this (or the result of a system command??????)
//                furthermore we're combining operators to a single input character type: \^~% become %, /* become * and |& become &
//                                -------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-.*NNNNNNNNNN:;>=>?@LLLLELLLLLLLLLLLLLLLLLLLLL[%]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{&}%b";
// replacing: const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-./NNNNNNNNNN:;<=>?@LLLLELLLLLLLLLLLLLLLLLLLLL[%]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{|}~d";

// now we define all the state transitions i.e. what input character types result in which new token type
// NOTE this can be organized in many ways perhaps it's easiest to tell per input character what the transformation is
//      only changes to the token type need to be registered, so if the change is NOT present, no need to put it in the transition table
//      EWW means that when starting an expression any whitespace starts a whitespace token, we use * to indicate ALL possible input character types
//      *WW means that any W character received in any state will result in a W state 
// we can make an array of transitions with each element corresponding to the character in TOKENTYPES, so the first entry contains all responses to E, the second entry the responses to W etc.
// it's easier to tell for any possible resulting token type which input character types will result in that type
// it's a hell of a job to create the token type transitions matrix
/* LEGEND:
   - signs are allowed in an EREAL but only directly behind the E, which means we have to somehow have an EREALEXPONENT element unless you treat this E as a binary operator which I think is a very good idea!!!
   - E stands for *10** so is this an assignable operator I suppose you could make it assignable as in 4e=3 to muliply by 1000, yes this look strange, as such . could also be considered an operator but Ok
     E is Assignable e r u, so we can get rid of the EREAL token type!!!
*/
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","","-+!","`D","`S","","","","","","","","LEN","",""}; // MDH@30APR2019: oops one extra needed...

/* MDH@18MAR2019: I have to add all token containing operator characters which is any of 8 different types of operators
   NOTE some operators are temporary in that they can be completed to become another (final) operator like ! or = when an = could be added, so it's actually a transition from an existing token to the same token
   Operator token types:
   UNARY 						! - + 				which consist of ! (not) and - and + first characters at a place where a unary operator is acceptable
   ASSIGNMENT 					=					any token that ends with = with an identifier in front of (possibly of a list element which will make it more complex)
   BIN_UNEXT_ASSIGNABLE			+ - ~ ^ \ %			a non-extendable binary operator but that is assignable behind a variable identifier
   BIN_EXT_ASSIGNABLE 			* /					a binary operator that is extendable (with the same character) but (both) with an assignment operator (behind an identifier token)
   BIN_EXT_OR_ASSIGNABLE		& |					a binary operator that can either be extended (with the same character) or assigned (because it's a binary operator by itself)
   BIN_EQ_OR_NEQ				! =					binary equal or unequal operator (to be postfixed with =) where a binary operator is expected (behind an identifier or some other value argument)
   BINARY  						? :					things that are immediately binary (and that do not allow additional characters in the token)
   COMPARISON					< >					comparison operator that is extendable with the same sign and it assignable after adding this second sign, but still = can be added to it to become binary
   You may notice that the interpretation of the first character may differ for ! - + (unary or binary) = (binary assignment behind identifier or equality operator elsewhere)
   Some of these token types are intermediate that is INCOMPLETE and I think these are the first token that is not inherently complete immediately as with identifiers and literals (wel double quoted string are also inccomplete)
   Technically we could finish up with UNARY and BINARY or even OPERATOR as the position determine if it's a unary or binary operator BUT there's nothing wrong with keeping ASSIGNMENT, COMPARISON, EQUAL_OR_UNEQUAL, COMPARISON
   We can code these characters with digits 1, 2, 3, 4, 5, 6, 7, 8 unary could be encoded with 1 
   Well characters with multiple meanings like ! - + and = could be represented by themselves but the first letter of the token type that would be U A B C which leaves us with four additional for which we can use % / & 
*/
/* MDH@23MAR2019: syntacticly we have less operators
	TOKENTYPE(TT_ONE_CHAR_UNARY=0b10000001) 						!(un) -(un) +(un)
	TOKENTYPE(TT_ONE_CHAR_BINARY_=0b10100001)      					?
	TOKENTYPE(TT_ONE_CHAR_ASSIGNABLE_BIANRY=0b10101010)  			= ~ ^ % \ -(bin) +(bin)
	TOKENTYPE(TT_TWO_CHAR_BINARY=0b10101110)      					! (followed by =)
	TOKENTYPE(TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY=0b10101011)		& | (interesting =+= and &+= and |+= and itself)
	TOKENTYPE(TT_TWO_CHAR_ASSIGNABLE_BINARY=0b10111011)				< > * /
	printf("\nError                                        : %d.",TT_ERROR);
	printf("\nOne character unary operator                 : %d.",TT_ONE_CHAR_UNARY);
	printf("\nAssignment operator                          : %d.",TT_ASSIGNMENT);
	printf("\nOne character binary operator                : %d.",TT_ONE_CHAR_BINARY);
	printf("\nOne character assignable binary operator     : %d.",TT_ONE_CHAR_ASSIGNABLE_BINARY);
	printf("\nTwo character binary operator                : %d.",TT_TWO_CHAR_BINARY);
	printf("\nTwo character once assignable binary operator: %d.",TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY);
	printf("\nTwo character assignable binary operator     : %d.",TT_TWO_CHAR_ASSIGNABLE_BINARY);
	printf("\nComparison or shift operator                 : %d.",TT_COMPARISON_OR_SHIFT_BINARY);
*/
/* MDH@10APR2019: 
- some transitions only change the type but do not start a new token, but this is true for all binary operators, so I guess we can force that programmatically
- if we put ERROR at the end we do not need to add an array for dealing with error transitions (as we cannot leave an error!!)
*/
// operator input type characters: ! ~ + - % * < = | (8 different operator groups)
// ! ~ and + start a unary operator when a value is expected
// MDH@15APR2019: still to determine what to do with @ and ` (the latter for system commands????)
//                inserting macro's should also be possible somehow...
/*
 "EXPR","UNA","A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","VAR" ,"NEWVAR","L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ] }="}, /* EXPRESSION */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}="}, /* ONE CHARACTER UNARY !-+ */ \
{"("   ,"!-+","" ,"="    ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ] }" }, /* ASSIGNMENT = */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}="}, /* Baeru finished bin.op. */ \
{""    ,""   ,"" ,"="    ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@;!CDS%()&*+-,.>?:LEN[]{}" }, /* BaErU unfinished bin.op. */ \
{"("   ,"!-+","=",""     ,""     ,""     ,""      ,"R"    ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}" }, /* BAeRu assignable repeatable */ \
{"("   ,"!-+","" ,"="    ,""     ,""     ,""      ,"R"    ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  ,  ?:    ]{}" }, /* BaERu comp. (<>) bin.op. */ \
{"("   ,"!-+","=",""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* BAeru assignable bin.op. */ \
{"("   ,"!-+","=",""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* Taeru ternary op. (? only now) */ \
{""    ,""   ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,"LEN.",""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@;  DS (               {"  }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""    ,""   ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,"LEN."  ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@;  DS (     ,         {"  }, /* NEW_VARIABLE (variable that does not exist yet) */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*   .>?:     {}="}, /* LIST ELEMENT (similar to expression) */ \
{";"   ,""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""      ,","   ,"N"  ,"."   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (          L  [ {"  }, /* INTEGER: (signless) list of digits */ \
{";"   ,""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""      ,","   ,""   ,"N"   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (      .   L  [ {"  }, /* REAL: part behind a decimal period */ \
{""    ,""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,"D"      ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* DQSTRING: double quoted string */ \
{""    ,""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,"S"      ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* SQSTRING: single quoted string */ \
{";"   ,""   ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,""    ,""      ,","   ,""   ,""    ,"D"       ,"S"       ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";"   ,""   ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*   .>?:      }="}, /* LIST: [ starts a list */ \
{";"   ,""   ,"=","?"    ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .  :LEN  {"  }, /* END_OF_LIST: behind ] that ends a list */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,"}"    ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  ,.>?:    ]{ ="}, /* MAP: { starts a map */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  , >?:    ] }="}, /* MAP_VALUE: : starts a map value */ \
{";"   ,""   ,"" ,"?"    ,"!="   ,"&*"   ,">"     ,"+"    ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS% (   - .  :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""    ,""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,"("     ,""      ,""  ,"`@;!CDS%& )*+-,.>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"("   ,"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"LE"  ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %&  *   .>?:    ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{";"   ,""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .   LEN  {"  }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

// suggesting NOT to be able to get out of an error condition but to allow viewing information on the error somehow!!! (how about tab as this will do feed forward!!!!!)
// if we put the error info in the error token

uint8_t nextTokenType(uint8_t inputTokenType,char inputCharacterType){
	if(inputTokenType<NUMBER_OF_FINISHABLE_TOKEN_TYPES){ // can only move to another token type if currently inside a valid token (i.e. you cannot get out of a TT_ERROR token type!!!)
		// finding the type will be more difficult actually if we end up with the token type character instead of the token type index!!!
		char* noTransition=NO_TRANSITIONS[inputTokenType];
#ifdef __DEBUG__
		printf("'%s'",noTransition);
#endif
		if(strlen(noTransition)==0||(noTransition[0]=='`'?strchr(noTransition,inputCharacterType)!=NULL:strchr(noTransition,inputCharacterType)==NULL)){
			int8_t tokenType=NUMBER_OF_TOKEN_TYPES; // MDH@10APR2019: BUG FIX uint8_t changed to int8_t otherwise would circle around
			while(--tokenType>=0)if(strchr(TRANSITIONS[inputTokenType][tokenType],inputCharacterType)!=NULL)return tokenType;
		}
#ifdef __DEBUG__
		else{
			outputChar('=');
		}
#endif
	}
	return inputTokenType; // if no match was found assume no change to the token type!!
}

//// first operator characters (0=assignment character, 1-6: binary 1 and 2-character operators, 7-8: 1-character binary, 9-10: unary/binary, 11-12: 1-character unary)
//const char ASSIGNMENT_CHARACTER='=';
//const char FIRST_OPERATOR_CHARACTERS[]={ASSIGNMENT_CHARACTER,'<','>','|','&','*','/','^','%','+','-','~','!','\0'}; // i.e. "=<>|&*/^%+-~!";
// continuation of 2-character operators
////////const char* SECOND_OPERATOR_CHARACTERS[]={"=","=<>","=<>","|","&","*","/"};
/* if we have a token representing an operator we can check whether a new character is acceptable as continuation
bool continuesOperator(Mtoken* pLastCommandToEvaluateToken,char inputChar){
	unsigned int l=string_get_length(pLastCommandToEvaluateToken->text);
	if(inputChar==ASSIGNMENT_CHARACTER){ // appending the 'assignment' operator
		return(l==1||string_get_last_char(pLastCommandToEvaluateToken->text)!=ASSIGNMENT_CHARACTER);
	}else{
		if(l>1)return false; // cannot continue a two-character operator
		// if subtype is assumed to represent the index into the first operator character set
		unsigned int operatorType=pLastCommandToEvaluateToken->type.subtype;
		return(operatorType<=6&&strchr(SECOND_OPERATOR_CHARACTERS[operatorType],inputChar)!=NULL);
	}
}
*/
// keep track of the state of entering a command
void removeToken(){		
	// ASSERT pLastCommandToEvaluateToken should NOT be NULL and empty (i.e. empty tokens should be removed!!!)
	// NOTE if we call freeToken() to free this token all forwardly connected tokens are also freed, so pPrevToken->next should become NULL
	pLastCommandToEvaluateToken=freeToken(pLastCommandToEvaluateToken); // pLastCommandToEvaluateToken now equals its own previous token!!
	if(pLastCommandToEvaluateToken)pLastCommandToEvaluateToken->next=NULL;else pCommandToEvaluate=NULL;
}
void unfinishToken(){
	// for all non-unary token that we are in now that is finished, unfinish it!!
	if(pLastCommandToEvaluateToken)
		if(pLastCommandToEvaluateToken->type!=TT_UNARY) // not a unary operator (of length 1) we ended up in
			if(string_length(pLastCommandToEvaluateToken->text)==pLastCommandToEvaluateToken->significantCharacterCount) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
				pLastCommandToEvaluateToken->significantCharacterCount=0;
}
char removedTokenCharacter(uint16_t behindCursor){
#ifdef __DEBUG__
		printf("%d",behindCursor);
#endif
	uint16_t tokenCharacterPosition;
	// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
	while(true){
		if(pLastCommandToEvaluateToken==NULL)return '\0';
		tokenCharacterPosition=string_length(pLastCommandToEvaluateToken->text); // MDH@24APR2019 replacing (what is essentially the same): cursorPosition()-pLastCommandToEvaluateToken->offset;
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition);
#endif
		if(tokenCharacterPosition>=behindCursor)break;
#ifdef __DEBUG__
		outputChar('.');
#endif		
		pLastCommandToEvaluateToken=pLastCommandToEvaluateToken->prev;
	}
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition-behindCursor);
#endif	
	// if failing to remove the character serious error
	char c=string_removed_char(pLastCommandToEvaluateToken->text,tokenCharacterPosition-behindCursor);
#ifdef __DEBUG__
		outputChar(c);
#endif		
	if(c){
		if(string_empty(pLastCommandToEvaluateToken->text))removeToken(); // text now empty, remove the token entirely...
		unfinishToken();
	}
	return c;
}

// HERE THE EVALUATION OF EXPRESSIONS TAKE PLACE
/* MDH@21MAY2019: every Mvalue* should be created on the value stack and never elsewhere, every assignment to an Mvalue should be done using assignValue() and never using =
Mvalue* getListValue(Mlist* _list){
	Mvalue* _listValue=(Mvalue*)calloc(1,sizeof(Mvalue)); // OOPS, not the sizeof the pointer but Mvalue itself!!!!
	if(_listValue)_listValue->value._list=_list;
	return _listValue;
}
*/
// the following is not required if we only allow the x[i/a,i/a,i/a] syntax or alternatively x[i/a][i/a] etc. and we only need to keep the last value and the 'index' or 'attribute' value reference
/* MDH@06MAY2019: expressions contain references to places where values are stored which is not a variable
typedef struct Mvaluepointeritem{
	Mvalue* _value; // index (list) or attribute 
	struct Mvaluepointeritem* _item; // the next item to access within this composite value
}Mvaluepointeritem;
// at the top we have a pointer that references a variable (in some environment)
typedef struct Mvaluepointer{
	Mvariable* _variable;
	Mvaluepointeritem* _item;
}Mvaluepointer;
// and at some point we'd need to get the value of where the value pointer points to
Mvaluepointeritem* getLastValuepointeritem(Mvaluepointer* _valuepointer){
	// the problem here is that to make assignments possible we have to remember the last value pointer item
	// this is because Mvalue instances themselves are immutable!!!
	Mvalue* _value=NULL;
	if(_valuepointer){
		_value=_valuepointer->_variable->_value;
		Mvaluepointeritem* _item=_valuepointer->_item;
		// if we have an item and a value
		while(_item&&_value){
			Mvalue* _itemvalue=_item->_value;
			// the value of the item could be of the wrong type i.e. 
			if(_itemvalue->type==VT_INTEGER&&_value->type==VT_LIST){
				_value=getListElement(_value->value._list,_itemvalue->value._integer);
			}else
			if(_itemvalue->type==VT_STRING&&_value->type==VT_MAP){
				_value=getMapElement(_value->value._list,_itemvalue->value._string);
			}else // invalid reference
				_value=NULL;
		}
	}
	return _value;
}
*/

// MDH@06MAY2019: Mvaluereference stands for a variable in combination with an item id, this will allow assignments as we know the variable involved!!!!
typedef struct Mvaluereference{
	char* _name; // the name of the host variable or NULL if we're in a substructure
	Mvalue* _value; // either the host value (if no variable name is defined), or the value of the host variable
	Mvalue* _itemid; // the item referenced!!!
}Mvaluereference;
/*
typedef struct Mexpressionvalue{
	Mvaluereference* _valuereference; // thre result of evaluating an expression is always a single value!!!
	Mtoken* token; // supposed to be the token the evaluation ended with (so the token in front of the first in the next evaluation)
}Mexpressionvalue;
// free_expressionvalue does NOT free the token as it will probably be passed on...
void free_expressionvalue(Mexpressionvalue* _expressionvalue){
	if(_expressionvalue){
		if(amVerbose())outputLine("Releasing the result!");
		// MDH@03MAY2019: append the result value at the proper index (as indicated by the command index)
		if(_resultListValue){if(appendedToList(_resultListValue->value._list,_expressionvalue->_valuereference->_variable->_value,commandCount+1))outputLine("ERROR: Failed to save the result.");else if(amVerbose())outputLine("Result saved.");}
		// replacing: if(!appendToListVariable(_Menvironment,"M",_expressionvalue->_value))outputLine("ERROR: Failed to append the result to the M list.");else if(amVerbose())outputLine("Result appended to the M list.");
		//// NEVER free what does not have an underscore at the start!!!! free_token(_expressionvalue->token); // probably NULLed already as this will not be new token, so I guess we could remove the _ to prevent freeing!!!
		////////output("\nFreeing expression!");
		decrementReferenceCount(_expressionvalue->_valuereference->_variable->_value); // as where freeing _expressionvalue!!!!
		free(_expressionvalue);
	}else
	if(amVerbose())outputLine("No result to free!");
	
}
*/
/* MDH@21MAY2019: replaced by placing the list and map (which is what it was used for) on the main value list, so it can be removed when no longer referenced!!!!
// helper function to get an expression value of hold a value of a specific type
// OOPS THIS value is NOT stored on the central value list!!!
Mvalue* getValueOfExpressionOfType(enum Mvaluetype valuetype){
	Mvalue* _value=(Mvalue*)calloc(1,sizeof(Mvalue));
	// allocate the value to hold, if a composite type (map or list), initialize the map and list to an empty map or list (integer, real and string are not set in advance)
	if(valuetype!=VT_UNDEFINED){
		_value->type=valuetype;
		switch(_value->type){
			case VT_INTEGER:_value->value._integer=(Minteger*)calloc(1,sizeof(Minteger));break; // initialized to 0 I presume
			case VT_REAL:_value->value._real=(Mreal*)calloc(1,sizeof(Mreal));break; // initialized to 0.0 I presume
			case VT_STRING:_value->value._string=(Mstring*)calloc(1,sizeof(Mstring));break;
			case VT_LIST:_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
			case VT_MAP:_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
			default:break;
		}
	}
	return _value;
}
*/
/*
an expression represents a value, and therefore:
<expression>::=<value>{<binary operator><value>}
NOTE that binary operator is atomic
this is not a recursive definition but it could be <expression>::=<value>[<binary operator><expression>] but that would result in right-to-left evaluation
But I forgot to include assignment 


NOTE that a formula differs from an expression in that it does not contain assignments!!!!

This poses the question what x=3+4 evaluates to; we do not want to write x=(3+4) to get it properly evaluated, therefore x=3+4 means x= 3+4 i.e. everything behind x= is evaluated before being assigned
i.e. assignment is NOT a binary operator
<expression>::=[<variable>[<shortcut binary operator>]<assignment operator>]<formula>
<formula>::=<value>{<binary operator><value>} this way a binary operator never ends a expression and is not recursively processed
<value>::={<unary operator>} [function]<(>{<expression><,>}<expression><)> | <value literal> | <variable>)
<value literal>::= <integer> | <real> | <string> | <[><expression>{,<expression>}<]> | <{><string literal>:<expression>{,<string value>:<expression><}>

<variable> ::= <variable identifier> [<[>{<integer expression><,>}<integer expresssion><]>]

Note that certain elements have repeating elements (optional) like argument list, binary operator lists, and map element lists, which have different separators
I guess we can use that in the evaluation because these define the separators!!!! so with any list we can define the token types that separate the successive list elements!!!
but <value><operator><value> here operator is a set of token types that separate the values but the operators should end up in the produced list as they are significant/meaningful
*/
/**
 * getValueOfExpression() evaluates an expression, obviously this means that we need to have some sort of understanding of where expression occur in the syntax of the M language
 * @info: some information text on the expression type (used in messages)
 * @resulttype: one character to indicate the type of expression result value (e.g. 'i' stands for index, i.e. an index into a list variable)
 * @firstToken: the first token in the expression to process
 * @endTokenTypes[]: the tokens that end the expression
 * @endTokenTypeCount: the number of end tokens
 * returns: the last token processed (which should be one of the end tokens) or NULL if all tokens were processed, and the Mvalue the expression evaluates to
 */
Mtoken* expressionToken=NULL; // the current evaluation token
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount); // prototype definition of getValueOfExpression() so we can call it from getValueOfList() and getValueOfMap()

// NOTE by adding endTokenType and maximumNumberOfElements to getListExpressionValue we can use it as well for getting an arguments list...
Mvalue* getValueOfList(TokenType endTokenType,uint32_t maximumNumberOfElements){
	if(amVerbose())output("\nComposing a list starting with '%s'.",string(expressionToken->text));
	// MDH@21MAY2019: _getListValue() as opposed to getValueOfExpressionOfType() creates a Mvalue on the value list which will be removed when the reference count of the Mvalue list ends up being 0
	//                then, the list element values will be dereferenced and if their reference count becomes zero freed as well successfully!!!!
	Mvalue* _listValue=_getListValue(VT_UNDEFINED); // replacing: getValueOfExpressionOfType(VT_LIST);
	Mlist* _list=_listValue->value._list; // grab the (empty) list to fill
	if(!_list){output("\nFailed to create a list to return.");return NULL;}
	if(_list->_first||_list->_last){output("\nSupposedly empty list not initialized correctly.");return NULL;}
	if(amVerbose())output("\nComposing a list starting with token '%s' of type '%s'.",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
	///////enum TOKENTYPE_ENUM listElementEndTokenTypes[]={TT_END_OF_LIST,TT_LISTELEMENT};
	// we iterate over the list elements, so at the start we assume expressionToken represents the start token of the list (literal)
	unsigned long long listElementIndex=0;
	while(true){
		expressionToken=expressionToken->next; // now on the first element
		if(!expressionToken)break;
		if(expressionToken->type==endTokenType)break; // missing elements should be skipped but counted
		listElementIndex++;
		if(amVerbose())output("\nProcessing list element #%llu starting with token '%s' of type '%s'.",listElementIndex,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		// theoretically it is possible that this list element is empty in which case we should append NULL to the list
		Mvalue* _listElementValue=(expressionToken->type!=TT_LISTELEMENT?getValueOfExpression("list element",'l',(TokenType[]){endTokenType,TT_LISTELEMENT},2):NULL);
		if(!_listElementValue){if(amVerbose())output("\nList element missing!");continue;} // undefined list elements should NEVER be added to the list
		if(amVerbose())output("\nList element ending token: %s.",TOKENTYPE_STRING[expressionToken->type]);
		// get the next list element value, here's a problem as we're supposed to return the offset not the first token
		// if we already have the maximum number of elements, we do not append this list element!!!
		// we're NOT using the number of elements in the list to check agains anymore but the list element index
		if(!maximumNumberOfElements||listElementIndex<=maximumNumberOfElements){
			// MDH@21MAY2019 IMPORTANT: because NULL list elements are NOT stored explicitly in the list (because a list is stored sparse), the list index should be passed in
			if(appendedToList(_list,_listElementValue,listElementIndex)!=listElementIndex){outputValue("\nERROR: Failed to append list element '",_listElementValue,"'.");break;}
			if(amVerbose())output("\nList element #%lld appended to list!",listElementIndex);
		}else
		if(amVerbose())output("\nMaximum number of elements reached.");
		if(expressionToken->type==endTokenType)break; // the list element could have ended with the end token type, in which case we're done!!!
	}
	if(amVerbose())output("\nList extracted!");
	return _listValue;
}

Mvalue* getValueOfMap(){
	Mvalue* _mapValue=_getMapValue(VT_UNDEFINED); // MDH@21MAY2019 for the same reason as above: replacing: getValueOfExpressionOfType(VT_MAP);
	Mmap* _map=_mapValue->value._map; // grab the map to fill
	//enum TOKENTYPE_ENUM mapAttributeNameEndTokenTypes[]={TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT};
	//enum TOKENTYPE_ENUM mapAttributeValueEndTokenTypes[]={TT_END_OF_MAP,TT_LISTELEMENT};
	// NOTE a map can be empty in which case _firstToken will immediately be of type TT_END_OF_MAP
	while(true){
		expressionToken=expressionToken->next;
		if(!expressionToken)break;
		if(expressionToken->type==TT_END_OF_MAP)break;
		if(expressionToken->type==TT_LISTELEMENT)continue; // missing attribute name-value pair
		// get the next attribute name, value pair
		// obviously the name should be something that evaluates to a string
		Mvalue* _attributeNameValue=getValueOfExpression("map attribute name",'s',(TokenType[]){TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT},3);
		// NOTE _attributeNameValue will be released after evaluation because it is not assigned to something else...
		if(expressionToken->type==TT_END_OF_MAP)break;
		// for now let's decide to simply not store the attribute if the name is not of type string
		if(expressionToken->type!=TT_MAP_VALUE)continue; // if no value part defined (behind :), skip
		Mvalue* _attributeValueValue=getValueOfExpression("map attribute value",'v',(TokenType[]){TT_END_OF_MAP,TT_LISTELEMENT},2);
		mstring* attributeName=_getValueText(_attributeNameValue); // parse the attribute name value 
		if(!attributeName)continue; // unable to parse the attribute name expression value into a string
		if(!appendedToMap(_map,string(attributeName),_attributeValueValue))output("\nERROR: Failed to append the map element."); // NOTE can't break until we actually bump into the TT_END_OF_MAP!!!
		free_mstring(attributeName); // ALWAYS free the value text 
		if(expressionToken->type==TT_END_OF_MAP)break;
	}
	return _mapValue;
}

// a function call needs a function and a map of arguments (defining the values to use for the formal parameters of the function)
Mvalue* getValueOfFunctionCall(Mfunction* _function,Mmap* _argumentMap){
	////////Mvalue* _resultValue=NULL;
	switch(_function->type){
		case FT_M:{
			// TODO create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values

				break;
			}
		case FT_INTERNAL_NO_ARGUMENTS:
			if(amVerbose())output("\nCalling no-argument function %s.",string(_function->_name));
			return (*_function->functionunion.noArgumentFunction)(_Menvironment);
		case FT_INTERNAL_ONE_ARGUMENT:
			if(amVerbose())output("\nCalling one-argument function %s.",string(_function->_name));
			return (*_function->functionunion.oneArgumentFunction)(_Menvironment,_argumentMap->_first->_variable->_value);
		case FT_INTERNAL_TWO_ARGUMENTS:{
			if(amVerbose())output("\nCalling two-argument function %s.",string(_function->_name));
			Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
			Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement?_firstArgumentmapelement->_next:NULL);
			return (*_function->functionunion.twoArgumentFunction)(_Menvironment,(_firstArgumentmapelement?_firstArgumentmapelement->_variable->_value:NULL)
																														,(_secondArgumentmapelement?_secondArgumentmapelement->_variable->_value:NULL));
		}
	}
	return NULL;
}

/**
 * MDH@Jacky=65yrs:
 * getValueOfExpression() returns the value of the tokens behind _offsetToken together with the token that ends the expression in an Mexpressionvalue*
 * the general idea is to make it recursive so it delegates getting specific subvalues from getValueOfExpression()
 * let's analyze evaluating an expression:
 * an expression 'evaluates' to a value means that we have to apply functions (or unary operators) to arguments, and binary operators to arguments as well
 * this value can also be a composite value like a list or a map, nevertheless a list or a map is a single value
 * it makes sense to delegate getting a list or a map literal to another function
 * NOTE getValueOfExpression() knows nothing about the type of expression it is processing, so it has to check whether to delegate or not
 *      however the general structure would be: <value><binary operator><value> or perhaps <value><ternary operator><value> but the idea is the same
 *      we could store these parts in elements of a list, where operator is stored as string and value as Mvalue*, so technically simply a list of Mvalue's so an Mlist*
 *      we can call these operands and operators or perhaps expressionelements??????
 * 			at the end the operators would need to be removed from the expressionelements array and we'd end up with a single value as result...
 * 		  if the resulting value is an variable, we should return the value of the variable, technically this means that an expressionelement cannot be a variable that makes sense
 *      so I guess we should only accept assignments at the start of an expression (which makes perfect sense)
 */

/**
 * get_valuereference() wraps a single Mvalue instance
 */
Mvaluereference* get_valuereference(Mvalue* _value){
	if(amVerbose())outputValue("\nWrapping value '",_value,"'.");
	Mvaluereference* _valueReference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference));
	assignValue(&_valueReference->_value,_value);
	////////////////////incrementReferenceCount(_valueReference->_value); // TODO is this correct???
	if(amVerbose())outputValue("\nValue '",_value,"' wrapped in value reference.");
	return _valueReference;
}
void free_valuereference(Mvaluereference* _valuereference){
	if(_valuereference){
		if(_valuereference->_name)free(_valuereference->_name);
		// values themselves are never freed!!!
		if(_valuereference->_value)decrementReferenceCount(_valuereference->_value);
		if(_valuereference->_itemid)decrementReferenceCount(_valuereference->_itemid);
		free(_valuereference);
	}
}
Mvalue* getReferencedValue(Mvaluereference* _valuereference){
	// TODO what if the value is not a list and it is indexed??????
	return(_valuereference?_valuereference->_itemid?getValueAtIndex(_valuereference->_value->value._list,_valuereference->_itemid):_valuereference->_value:NULL);
}

Mvalue* applyUnaryOperator(char operator,Mvalue* _value){
	if(amVerbose()){
		output("\nApplying unary operator '%c'",operator);
		if(_value){outputValue(" to value '",_value,"'");output(" of type %u.",_value->type);}
	}
	switch(operator){
		case '~':if(_value->type==VT_INTEGER)return _getIntegerValue(~(_value->value._integer->ll));break;
		case '!':if(_value->type==VT_INTEGER)return _getIntegerValue((_value->value._integer->ll?0:1));break;
		case '-':if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);if(_value->type==VT_REAL)return _getRealValue(-_value->value._real->ld);break;
	}
	return NULL;
}

/**
 * getValueReference() retrieves a single value reference that either ends when a binary operator token is encountered or one of the end token types
 * a value reference syntax: optionally a number of unary operators, optionally followed by function call with arguments, and variable or value literal
 * we need to store the value in a value reference just in case the value is the destination of an assignment, so yes, reference is an apt name
*/
Mvaluereference* getValueReference(TokenType endTokenTypes[],uint8_t endTokenTypeCount){

	if(amVerbose())output("\nThe first value token: '%s' of type '%s'.",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);

	Mvaluereference* _valueReference=NULL;

	mstring* unaryOperators=NULL; // a value starts with a number (zero or more) of unary operators
		
	while(expressionToken&&expressionToken->type==TT_UNARY){
		char unaryOperatorChar=string_char(expressionToken->text,0);
		if(unaryOperatorChar!='+'){
			if(!unaryOperators)unaryOperators=string_create();
			string_append_char(unaryOperators,unaryOperatorChar);
		}
		expressionToken=expressionToken->next;
	}
	// ASSERT unary operators extracted

	if(expressionToken){
		if(amVerbose())output("\ngetValueOfReference() interpreting first value token '%s' of type %s.",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		_valueReference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference));
		// expecting either a function (call), (new) variable or (integer, real, string, list or map) literal
		/* NO we can NOT change the tokens themselves (to keep them editable!!!)
		if(expressionToken->type==VT_INTEGER){
			if(expressionToken->next&&expressionToken->next->type==VT_REAL){
				expressionToken=expressionToken->next;
				// let's prepend the integer token text to the real (fraction) token text
				string_prepend(string(expressionToken->prev->text),expressionToken->text);
			}
		}
		*/
		switch(expressionToken->type){
			case TT_FUNCTION:
				{
					if(amVerbose())output("\nCall of function '%s'.",string(expressionToken->text));
					Mfunction* function=getFunction(_Menvironment,string(expressionToken->text)); // get the function associated with the name of the function
					if(function){					
						// 1. get the list of function arguments, which depends on the function!!
						expressionToken=expressionToken->next;
						Mvalue* _functionArgumentsValue=getValueOfList(TT_END_OF_FUNCTION_CALL,(function?function->_parameterMap->numberOfElements:1));
						if(_functionArgumentsValue){
							if(amVerbose())output("\nConstructing the function call argument map!");
							// 2. get the arguments map
							Mmap* _functionArgumentMap=_getFunctionArgumentMap(function,_functionArgumentsValue->value._list); // assuming to have a list returned by getListExpressionValue()
							/// we do not need to release the function arguments list value because it it never assigned by itself, it is simply a container for the argument list elements (which do have a reference count incremented when added to the list)
							/*
							if(amVerbose())output("\nDecrementing the reference count of the function arguments value!");
							decrementReferenceCount(_functionArgumentsValue); // TODO is this correct?????
							if(amVerbose())output("\nReference count of the function arguments value decremented!");
							*/
							// 3. the result of applying the function to the arguments is the end result
							assignValue(&_valueReference->_value,getValueOfFunctionCall(function,_functionArgumentMap));
							// we have to free the map ourselves (this is what the _ in front of getFunctionArgumentMap means)
							if(amVerbose())output("\nFreeing the function argument map!");
							free_map(_functionArgumentMap); // MDH@21MAY2019: no need for the function argument map anymore!!!
							if(amVerbose())output("\nFunction argument map freed!");
						}else
							output("\nERROR: No function arguments!");
					}else
						output("\nERROR: Function '%s' unknown!",string(expressionToken->text));
				}
				break;
			case TT_NEW_VARIABLE: // a non-existing value reference
				// we have to create the variable first (TODO should we wait until actually assigning???)
				if(!addVariable(_Menvironment,string(expressionToken->text),VT_UNDEFINED,false))break; // NO retrieves the undefined value subsequently!!
			case TT_VARIABLE: // a value reference
				_valueReference->_name=_strdup(string(expressionToken->text)); // store a copy of the name of the variable being referenced
				assignValue(&_valueReference->_value,getValue(_Menvironment,_valueReference->_name)); // store a reference to the value
				/////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				// a variable can be followed by an index that we should store in the value reference's itemid field
				if(expressionToken->next&&expressionToken->next->type==TT_LIST){
					expressionToken=expressionToken->next;
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0); // typically allow for any number of indices (although perhaps we should check!!)
					// using the indexValue we should now update the value represented up until the last index (in case we have an assignment)
					// which means that only the last index value has to be stored and the container of that last index (map or list)
					if(indexListValue&&indexListValue->type==VT_LIST&&indexListValue->value._list->numberOfElements>0){ // a list with at least one index
						Mlist* indexList=indexListValue->value._list;
						Mlistelement* indexListelement=indexList->_first; // must be there!!!
						// as long as there are successors we haven't reach the last index yet!!!!
						// TODO what if someone does not specify ALL indices??????
						while(indexListelement->_next){
							// replace the current value with the value in the list (TODO map) at the current index
							assignValue(&_valueReference->_value,getValueAtIndex(_valueReference->_value->value._list,indexListelement->_value));
							indexListelement=indexListelement->_next;
						}
						if(amVerbose())outputValue("Last index: ",indexListelement->_value,"'.");
						// TODO what is going to happen to indexListValue?????? it should be discarded as its reference count will remain zero but all elements that are used elsewhere (like the last index stored in _valueReference will persist a little longer!!)
						assignValue(&_valueReference->_itemid,indexListelement->_value); // store the last index value in the _itemid field
					}
				}
				break;
			case TT_INTEGER: // an integer possibly followed by a real (fractional) part
				{
					long long ll=atoll(string(expressionToken->text));
					if(expressionToken->next&&expressionToken->next->type==TT_REAL){ // the integer part of a real
						expressionToken=expressionToken->next; // now pointing to the real fraction part text following the given integer!!!!
						assignValue(&_valueReference->_value,_getRealValue(_strtold(string(expressionToken->text))+ll));
					}else // just an integer
						assignValue(&_valueReference->_value,_getIntegerValue(ll));
					/////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				}
				break;
			case TT_REAL: // unlikely without integer part in front of it though
				assignValue(&_valueReference->_value,_getRealValue(_strtold(string(expressionToken->text))));
				///////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_DQSTRING:
			case TT_SQSTRING: // a string literal
				assignValue(&_valueReference->_value,_getStringValue(string(expressionToken->text)));
				////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_LIST: // a list literal
				_valueReference=get_valuereference(getValueOfList(TT_END_OF_LIST,0));
				break;
			case TT_MAP: // a map literal
				_valueReference=get_valuereference(getValueOfMap());
				break;
			case TT_EXPRESSION: // an expression wrapped in parentheses which ends with a TT_END_OF_FUNCTION_CALL (although theoretically it's not an end of function call of course)
			{
				Mvalue* _expressionListValue=getValueOfList(TT_END_OF_FUNCTION_CALL,1);
				if(amVerbose())output("\nGoing to wrap the list extracted!");
				// well, actually, we need the first element of the list that is returned!!!
				// use only the first element if the list only has one element, otherwise use the list itself
				if(_expressionListValue->value._list->numberOfElements==1){
					_valueReference=get_valuereference(_expressionListValue->value._list->_first->_value);
				}else
					_valueReference=get_valuereference(_expressionListValue);
				if(amVerbose())output("\nExtracted list wrapped!");
				break;
			}
			default:
				break;
		}

		if(amVerbose()){if(_valueReference&&_valueReference->_value)outputValue("\nValue: '",_valueReference->_value,"'.");else output("\nNo result!");}
		// apply the unary operators (backwards)
		if(unaryOperators){
			if(amVerbose())output("\nApplying unary operators: '%s'.",string(unaryOperators));
			uint16_t l=string_length(unaryOperators);
			while(l>0&&_valueReference->_value){
				/////////////decrementReferenceCount(_valueReference->_value);
				assignValue(&_valueReference->_value,applyUnaryOperator(string_char(unaryOperators,--l),_valueReference->_value));
				///////////////////////if(_valueReference->_value)incrementReferenceCount(_valueReference->_value);
			}
			if(amVerbose())output("\nUnary operator applied!");
		}else
			if(amVerbose())output("\nNo unary operators to apply!");
		
		// move over to the next expression token (following the end token)
		if(expressionToken)expressionToken=expressionToken->next;

	}

	return _valueReference;

	/*
		// it could be an assignment in which case we remove the assignee and assigned value
		if(firstToken->type==TT_VARIABLE||firstToken->type==TT_NEW_VARIABLE){ // something that can be assigned to
			// an index might be defined on a variable that is a list
			if(secondToken->type==TT_LIST){
				// TODO locate the end of list token at the same level????
			}
			// TODO there might be a binary operator behind (in front of the assignment operator)
			char* shortcutBinaryOperator=NULL;
			if(secondToken->type==TT_BINARY_AeRu||secondToken->type==TT_BINARY_Aeru){
				shortcutBinaryOperator=string(secondToken->text);
				secondToken=secondToken->next;
			}
			if(secondToken&&secondToken->type==TT_ASSIGNMENT){
				if(firstToken->type==TT_NEW_VARIABLE)addVariable(_Menvironment,firstTokenText,VT_UNDEFINED,false);
				Mvalue* _expressionValue=getValueOfExpression("assignment",'a',endTokenTypes,endTokenTypeCount);
				if(_expressionValue){
					if(shortcutBinaryOperator){ 
						// TODO apply the shortcut binary operator to the current value of the assignee before assigning
					}
					// if we failed to create the variable (see above), the following obviously will fail!!! (or of course when the type of the value is wrong)
					//if(amVerbose())output("\nStoring value '%s' in variable '%s'.",string(_getValueText(_expressionvalue->_value)),firstTokenText);
					if(!setValue(_Menvironment,firstTokenText,_valuereference->_variable->_value)){
						//output("\nERROR: Value '%s' not stored.",string(_getValueText(_expressionvalue->_value)));
						// no need to ever free a value ourselves, the 'garbage collection' takes care of that (see removedValues())
						///free_value(_expressionvalue->_value);
						///_expressionvalue->_value=NULL;
					}else
					if(amVerbose())
						output("\nVariable '%s' set to '%s'.",firstTokenText,string(_getValueText(getValue(_Menvironment,firstTokenText))));
				}
			}
		}
	}
	return _valuereference;
	*/
}

// two-argument arithmetic
Mvalue* add(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)+(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}
Mvalue* subtract(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)-(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}
Mvalue* multiply(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll*_value2->value._integer->ll);
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)*(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}
Mvalue* power(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(pow(_value1->value._integer->ll,_value2->value._integer->ll));
		return _getRealValue(pow(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld,_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld));
	}
	return NULL;
}
Mvalue* epower(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll*pow(10.,_value2->value._integer->ll));
		return _getRealValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld*pow(10.,(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld))));
	}
	return NULL;
}
Mvalue* divide(Mvalue* _value1,Mvalue* _value2){
	// always real divide
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld);
		return _getRealValue(ld1/ld2);
	}
	return NULL;
}
Mvalue* integerdivide(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld);
		return _getIntegerValue(truncl(ld1/ld2));
	}
	return NULL;
}
Mvalue* divideremainder(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).rem);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld);
		return _getRealValue(ld1-ld2*truncl(ld1/ld2)); // what's left after subtracting the truncated value
	}
	return NULL;
}
// integer arithmetic 
Mvalue* xor(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll^_value2->value._integer->ll);
	return NULL;
}
Mvalue* bitwiseand(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&_value2->value._integer->ll);
	return NULL;
}
Mvalue* logicaland(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&&_value2->value._integer->ll);
	return NULL;
}
Mvalue* bitwiseor(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll|_value2->value._integer->ll);
	return NULL;
}
Mvalue* logicalor(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll||_value2->value._integer->ll);
	return NULL;
}
Mvalue* shiftleft(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll<<_value2->value._integer->ll);
	return NULL;
}
Mvalue* shiftright(Mvalue* _value1,Mvalue* _value2){
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll>>_value2->value._integer->ll);
	return NULL;
}
// comparison operators
Mvalue* smallerthan(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)<(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* smallerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)<=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* largerthan(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)>(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* largerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)>=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* unequalto(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)!=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}
Mvalue* equalto(Mvalue* _value1,Mvalue* _value2){
	if((_value1->type==VT_INTEGER||_value1->type==VT_REAL)&&(_value2->type==VT_INTEGER||_value2->type==VT_REAL))
		return _getIntegerValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._real->ld)==(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._real->ld)?1:0);
	return NULL;
}

Mvalue* applyBinaryOperator(char* operator,Mvalue* _value1,Mvalue* _value2){
	if(_value1&&_value2){
		if(amVerbose()){outputValue("\nComputing '",_value1,NULL);output("' %s '",operator);outputValue(NULL,_value2,"'.");}
		switch(operator[0]){
			// real arithmetic
			case '+' :return add(_value1,_value2);
			case '-' :return subtract(_value1,_value2);
			case '*' :return (strlen(operator)-1?power(_value1,_value2):multiply(_value1,_value2));
			case 'e' :return epower(_value1,_value2);
			case '/' :return (strlen(operator)-1?integerdivide(_value1,_value2):divide(_value1,_value2));
			case '\\':return integerdivide(_value1,_value2);
			case '%' :return divideremainder(_value1,_value2);
			// integer arithmetic
			case '^' :return xor(_value1,_value2);
			case '&' :return (strlen(operator)-1?logicaland(_value1,_value2):bitwiseand(_value1,_value2));
			case '|' :return (strlen(operator)-1?logicalor(_value1,_value2):bitwiseor(_value1,_value2));
			// comparison operators
			case '<' :return (strlen(operator)-1?(operator[1]=='<'?shiftleft(_value1,_value2):smallerthanorequalto(_value1,_value2)):smallerthan(_value1,_value2));
			case '>' :return (strlen(operator)-1?(operator[1]=='>'?shiftright(_value1,_value2):largerthanorequalto(_value1,_value2)):largerthan(_value1,_value2));
			case '!' :return unequalto(_value1,_value2);
			case '=' :return equalto(_value1,_value2);
		}
	}
	return NULL;
}

typedef struct Mformulaelement{
	Mvaluereference* _operand; // an operand to apply the binary operator to
	mstring* _operator; // a (shortcut) binary operator 
	struct Mformulaelement* _next;
	struct Mformulaelement* _prev; // MDH@21MAY2019: unfortunately needed for moving back!!
}Mformulaelement;
/* any formula starts with
typedef struct Mformula{
	Mvaluereference* _operand;
	Mformulaelement* _next;
}Mformula;
*/
// once we have constructed a formula it needs to be computed

/**
 * an expression is the top-level element of the M language hierarchy
 * which optionally starts with an assignment to a single variable, BUT it makes sense to allow for multiple assignments in a row?????
 * typically this assignee can be composite referencing indices or attributes in maps, obviously we can put this variable in some sort of structure
 * it composes a list of value references to which operators are to be applied
 */
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount){
	// typically the offset token determines what the expression ends with!!
	// e.g. ( ends with , or )    [ ends with ]     { ends with }    etc.   
	Mvalue* _expressionValue=NULL;
	/////////_expressionvalue->_valuereference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference)); // create a value reference that is to hold a single value reference as result

	if(expressionToken){

		if(amVerbose()){
			output("\ngetValueOfExpression() interpreting %s expression starting with token '%s' of type '%s'",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			if(endTokenTypeCount){
				output(" that ends");
				uint8_t endTokenTypeIndex=0;
				while(endTokenTypeIndex<endTokenTypeCount){output(endTokenTypeIndex?" or ":" with ");output(TOKENTYPE_STRING[endTokenTypes[endTokenTypeIndex++]]);}
			}
			outputChar('.');
		}

		// construct a formula
		Mformulaelement* formula=calloc(1,sizeof(Mformulaelement));
		Mformulaelement* _formulaelement=formula; // the current formula element!!!
		Mvaluereference* _valuereference;

		int8_t endTokenTypeIndex; // max. 127 token types should suffice!!!

		while(expressionToken){

			if(amVerbose())output("\nProcessing %s expression token '%s' of type %s.",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			// does this token end the expression????
			endTokenTypeIndex=endTokenTypeCount;
			while(endTokenTypeIndex&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1]&&expressionToken->type>=8)endTokenTypeIndex--;
			if(endTokenTypeIndex){if(amVerbose())output("\nEnd of %s expression.",info);break;}
			
			_formulaelement->_operand=getValueReference(endTokenTypes,endTokenTypeCount);

			// MDH@16MAY2019: can't end an expression with an operator BRO'
			endTokenTypeIndex=endTokenTypeCount;
			while(endTokenTypeIndex&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1]/*&&expressionToken->type>=8*/)endTokenTypeIndex--;
			if(endTokenTypeIndex){if(amVerbose())output("\nToken '%s' of type %s ends the %s expression.",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],info);break;}
			
			// the next token(s) should be a binary operator
			// NOTE some binary operators are stored in a couple of tokens!!!
			if(expressionToken)if(expressionToken->type==TT_END_OF_DQSTRING||expressionToken->type==TT_END_OF_SQSTRING)expressionToken=expressionToken->next;
			if(expressionToken){
				if(amVerbose())output("\nInterpreting operator token '%s' of type '%s'.",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
				_formulaelement->_operator=string_create();
				if(!string_copy(expressionToken->text,_formulaelement->_operator)){output("\nERROR: Failed to copy the operator!");break;}
				string_setlength(_formulaelement->_operator,expressionToken->significantCharacterCount); // cut off the nonsignificant stuff
				// append any other binary operator behind it (like a continuation or assignment operator)
				if(expressionToken->next->type>0&&expressionToken->next->type<=8){
					expressionToken=expressionToken->next;
					string_append_char(_formulaelement->_operator,string_char(expressionToken->text,0)); // CHECK works for assignment operator but not per se for any operator!!!
				}
				if(amVerbose())output("\nFormula element operator: '%s'.",string(_formulaelement->_operator));
				_formulaelement->_next=(Mformulaelement*)calloc(1,sizeof(Mformulaelement));
				_formulaelement=_formulaelement->_next;
				expressionToken=expressionToken->next;
			}else
			{
				if(amVerbose())outputLine("No further formula elements!");
			}
			
		}

		// evaluate the formula
		if(formula){

			// skip all assignments
			uint16_t numberOfAssignments=0;
			Mformulaelement* _lastAssignmentFormulaelement=NULL;
			_formulaelement=formula;
			while(_formulaelement){
				if(string_last_char(_formulaelement->_operator)!='=')break; // not ending with assignment operator character to start with
				if(string_char(_formulaelement->_operator,0)=='<'||string_char(_formulaelement->_operator,0)=='>'||string_char(_formulaelement->_operator,0)=='!')break; // break on <=, >= and !=
				if(string_length(_formulaelement->_operator)>1&&string_char(_formulaelement->_operator,0)=='=')break; // break on ==
				if(_lastAssignmentFormulaelement)_formulaelement->_prev=_lastAssignmentFormulaelement; // MDH@21MAY2019: in order to be able to traverse back!!!
				_lastAssignmentFormulaelement=_formulaelement;
				numberOfAssignments++;
				_formulaelement=_formulaelement->_next;
			}
			if(amVerbose())output("\nNumber of assignments: %u.",numberOfAssignments);

			Mvalue* _result=getReferencedValue(_formulaelement->_operand); // the first result computed
			// 'applying' the binary operators left-to-right remembering the intermediate result in _result
			// NOTE because all formula-elements are freed afterwards (see below) there's no need to so while applying the binary operators
			while(_formulaelement->_next){ // a binary operator to apply
				_result=applyBinaryOperator(string(_formulaelement->_operator),_result,getReferencedValue(_formulaelement->_next->_operand));
				_formulaelement=_formulaelement->_next;
			}
			if(amVerbose())outputValue("\nResult: '",_result,"'.");
			
			// perform assignments right-to-left (which is a little problematic though)
			if(numberOfAssignments){
				if(amVerbose())output("\nPerforming %u assignments.",numberOfAssignments);
				_formulaelement=_lastAssignmentFormulaelement;
				while(_formulaelement){
					_valuereference=_formulaelement->_operand;
					if(amVerbose())output("\nAssignment to %s using operator %s!",_valuereference->_name,string(_formulaelement->_operator));
					string_shorten(_formulaelement->_operator,1); // cutting off the assignment operator is fine, as we do not need it anymore!!!
					if(string_length(_formulaelement->_operator)){ // _result will change due to applying the shortcut binary operator
						// we have to be a bit careful here if the value reference uses an index id
						// does the _value field already contain the current value of the variable, if so we may immediately use that here instead of getValue()
						assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result));
						// replacing:	assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getValue(_Menvironment,_valuereference->_name),_result));
					}
					if(_valuereference->_itemid){
						// TODO check whether all the items are of the right type!!!
						appendedToList(_valuereference->_value->value._list,_result,_valuereference->_itemid->value._integer->ll);
						// TODO typically the list will be mutable, but the point here is that we need the full index list to get the right value (which we didn't store!!!!)
					}else{
						setValue(_Menvironment,_valuereference->_name,_result);
						// use the current value of the variable assigned to as new result!!
						assignValue(&_result,getValue(_Menvironment,_valuereference->_name)); // CHECK assign??
					}
					///////////if(!(--numberOfAssignments))break; // no more assignments???
					_formulaelement=_formulaelement->_prev;
				}
			}

			// the expression value is the value of the first operand!!!
			assignValue(&_expressionValue,_result); // MDH@21MAY2019: this will increment the reference count of _result so it makes sense to actually decrement its reference count after being used

			// free the formula
			Mformulaelement* _nextformulaelement;
			_formulaelement=formula;
			while(_formulaelement){
				free_mstring(_formulaelement->_operator);
				free_valuereference(_formulaelement->_operand);
				_nextformulaelement=_formulaelement->_next;
				free(_formulaelement);
				_formulaelement=_nextformulaelement;
			}
		}else
		if(amVerbose())output("\nNo result to store.");
	}
	return _expressionValue;
}
/**
 * getValueOfExpression() is the work horse for evaluating individual (simple i.e. non composite expressions) expressions 
 * the first token is being passed in which of course should represent a value somehow, evaluateExpression needs 
 */
/*
Mexpressionvalue* getFunctionValue(Mtoken* _offsetToken,char* functionName){
	if(_offsetToken&&functionName){
		Mexpressionvalue* _functionExpressionvalue=(Mexpressionvalue*)calloc(1,sizeof(Mexpressionvalue*));
		_functionExpressionvalue->_token=_offsetToken->next;
		// pFirstToken is the first token in the argument list, arguments are separated by commas
		// 1. compose the list of arguments
		Mlist* arguments=(Mlist*)calloc(1,sizeof(Mlist*));
		char* variableName=NULL; // keep track of the current variable name (that we might need when we run into an assignment)
		char* variableOperator=NULL; // the variable operator applicable to the variable name (right behind the variable and possibly in front of the assignment operator)
		while(pToken!=NULL){
			if(pToken->type==TT_VARIABLE){
				variableName=
			}
			pToken=pToken->next;
		}
		// 2. apply the function to the arguments and return it's result
		if(functionName){
		
		}
		// ASSERT if we get here arguments is the result
		if(arguments->numberOfElements){ 
			// we have arguments left
			if(arguments->numberOfElements==1)return arguments->_first->_value; // the first argument's value is the result
			// wrap the list in an Mvalue!!
			return getListValue(arguments);
	}
	// no result!!!
	return NULL;
}
*/

mstring* _getCommandText(bool color){
	mstring* commandText=string_create();
	Mtoken* pCommandToken=pCommandToEvaluate; // TODO can we get rid of using commandcount-1 here????
	while(pCommandToken){
		// if we bump into a comment we're done!!!
		if(pCommandToken->type==TT_COMMENT)break;
		// TODO there must be a better way to do the coloring!!!
		if(color){string_append(commandText,ES"38;5;");string_append(commandText,getTokenColor(pCommandToken->type));string_append_char(commandText,'m');} // assuming the same back color is used on ALL tokens, so we won't have to pass that along
		string_append(commandText,string(pCommandToken->text));
		// MDH@03MAY2019: place an asterisk in front of the type to indicate that expr is NOT null!!
		if(amAssisting()){
			if(color){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');}
			string_append_char(commandText,'(');if(pCommandToken->expr)string_append_char(commandText,'*');string_append(commandText,TOKENTYPE_STRING[pCommandToken->type]);string_append(commandText,") ");
		}
		pCommandToken=pCommandToken->next;
	}
	if(color)if(!amAssisting()){string_append(commandText,ES"38;5;");string_append(commandText,getInfoColor());string_append_char(commandText,'m');} // reset to info color
	return commandText;
}

void clearCommand(){
	pLastCommandToEvaluateToken=NULL;
	// a small precaution here!!!
	if(pCommandToEvaluate){freeToken(pCommandToEvaluate);pCommandToEvaluate=NULL;}
}
void outputValueColored(Mvalue* _value){
	switch(_value->type){
		case VT_INTEGER:outputTokenTypeColor(TT_INTEGER);outputValue(NULL,_value,NULL);break;
		case VT_REAL:outputTokenTypeColor(TT_REAL);outputValue(NULL,_value,NULL);break;
		case VT_STRING:outputTokenTypeColor(_value->value._string->presuffix=='"'?TT_DQSTRING:TT_SQSTRING);outputValue(NULL,_value,NULL);break;
		case VT_LIST:
			// TODO not using _getListText() as defined in Mexecution
			outputChar('[');
			Mlist* _list=_value->value._list;
			if(_list&&_list->numberOfElements){
				Mlistelement* _listelement=_list->_first;
				unsigned long long listitemindex=1;
				while(_listelement){
					if(_listelement->index)while(listitemindex<_listelement->index){listitemindex++;output(",");} // missing elements
					outputValueColored(_listelement->_value);
					_listelement=_listelement->_next;
				}
			}
			outputChar(']');
			break;
		case VT_MAP:
			outputChar('{');
			Mmap* _map=_value->value._map;
			if(_map&&_map->numberOfElements){
				Mvariable* _mapelementvariable;
				Mmapelement* _mapelement=_map->_first;
				while(_mapelement){
					_mapelementvariable=_mapelement->_variable;
					// TODO are we coloring the name?????
					output("%s%c",_mapelementvariable->_name,':');
					outputValueColored(_mapelementvariable->_value);
					if(!_mapelement->_next)break;
					outputChar(',');
					_mapelement=_mapelement->_next;
				}
			}
			outputChar('}');
			break;
		default:
			break;
	}
	resetOutputColor();
}
// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(){
	
	// 1. if the last token is a comment, remove it before further evaluation TODO should we unfinish the token??????
	//    as a result pLastCommandToEvaluateToken and pCommandToEvaluate could now both be NULL, that's why we test this first
	if(pLastCommandToEvaluateToken->type==TT_COMMENT)removeToken();

	// 2. if no command nothing evaluated TODO don't call when this is the case though
	if(!pCommandToEvaluate){outputError("Nothing to evaluate!");return false;}
	
	// 3. any command always has two significant tokens TODO could compare pCommandToEvaluate with pLastCommandToEvaluateToken which should be different!!!
	//    in this case we clear the command, so that the command won't be repeated, and the user can switch to control mode immediately with the Enter key!!
	if(pCommandToEvaluate==pLastCommandToEvaluateToken){outputError("Empty command.");clearCommand();return false;}

	// 2. if the last token is an error, can't evaluate (well, better not)
	// TODO it makes sense to remove the error token
	if(pLastCommandToEvaluateToken->type==TT_ERROR){outputError("Can't evaluate erroneous command.");removeToken();unfinishToken();return false;}

	// 3. if the last token is an operator of sorts the command is incomplete
	if(pLastCommandToEvaluateToken->type<=8){outputError("Value behind operator at end of command missing.");return false;}

	// MDH@03MAY2019: this is new, if expr is not NULL apparently we have missing parentheses!!!!
	//                BUT given that the first token always is of type TT_EXPRESSION and the last token will be pointing to it when complete we'd have to check for that too
	//                    this actually means that if expr is NULL there's one parentheses too many!!!
	/*
	if(!pLastCommandToEvaluateToken->expr){outputError("Too many parentheses!");return false;}
	if(pLastCommandToEvaluateToken->expr!=pCommandToEvaluate){outputError("Not enough parentheses!");return false;}
	*/
	// MDH@22MAY2019: the following is complex because we might be right behind the closing of a list, map or function call, in which case the command is still complete!!!
	if(pLastCommandToEvaluateToken->expr&&pLastCommandToEvaluateToken->expr->expr){
		switch(pLastCommandToEvaluateToken->expr->type){
			case TT_LIST:outputError("Missing end of list.");break;
			case TT_FUNCTION_CALL:outputError("Missing end of function call!");break;
			case TT_MAP:outputError("Missing end of map!");break;
			default:outputError("Not enough parentheses.");break;
		}
		return false;
	}

	// 4. can't end with function of function call
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION){outputError("Function call missing at end of command.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION_CALL){outputError("Unfinished function call.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_LIST||pLastCommandToEvaluateToken->type==TT_LISTELEMENT){outputError("Unfinished list.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_DQSTRING||pLastCommandToEvaluateToken->type==TT_SQSTRING){outputError("Unfinished string literal.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_EXPRESSION){outputError("Unfinished expression.");return false;}
	if(pLastCommandToEvaluateToken->type==TT_MAP||pLastCommandToEvaluateToken->type==TT_MAP_VALUE){outputError("Unfinished map.");return false;}

	// evaluating means getting the value of the expression that pCommandToEvaluate points to
	// NOTE that the first token is always a dummy token (which will at most contain the whitespace at the start of the command)
	mstring* commandText=_getCommandText(true);
	expressionToken=pCommandToEvaluate->next; // initialize the (current) expression token
	Mvalue* _commandExpressionValue=getValueOfExpression("command",'e',(TokenType[]){},0);
	if(_commandExpressionValue){
		output("\n%s = ",string(commandText));
		outputValueColored(_commandExpressionValue);
		decrementReferenceCount(_commandExpressionValue); // TODO do we need to do this?????
		if(amVerbose())outputLine("Result released!");
	}else
	if(amVerbose())output("\n'%s' is undefined!",string(commandText));
	if(amVerbose())outputLine("Command to release!");
	free_mstring(commandText);
	if(amVerbose())outputLine("Command released!");
	return true;
}

void prepareForUserInput(){
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	setbuf(stdout,NULL);
	initSession();
}

// MDH@24APR2019: writeCommand() writes the command to evaluate, and sets pLastCommandToEvaluateToken in the process
void writeCommand(){Mtoken* token=pCommandToEvaluate;while(token){outputToken(pLastCommandToEvaluateToken=token);token=token->next;}}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t newCommandPage){
	commandPage=newCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("\n%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Mtoken* token=commands[lastCommandToShowIndex+commandToShowIndex];
		while(token){outputToken(token);token=token->next;}
	}
	resetOutputColor();
	output("\n%s","Select the last digit of the command to use, or the up/down key to show the next/previous page.");
	output("\n%s",">> "); // TODO what kind of prompting do we want to do???
}
void showNextCommandPage(){
	if(commandPage<commandPages)
		setCommandPage(commandPage+1);
	else
		output("\n%s","No further commands to show.");
}
void showPreviousCommandPage(){
	if(commandPage>1)
		setCommandPage(commandPage-1);
	else
		output("\n%s","No further commands to show.");
}
/*
// when the user tries to insert a character we need to cut off the rest of the command and append it afterwards
char* removedRestOfCommand(){
	if(cursorPosition()<commandLength()){
		mstring* restOfCommand=string_create();
		if(restOfCommand!=NULL){
			uint16_t tokenPosition=cursorPosition()-pLastCommandToEvaluateToken->offset;
			if(tokenPosition)string_append(restOfCommand,string_remainder(pLastCommandToEvaluateToken->text,tokenPosition));
			string_setlength(pLastCommandToEvaluateToken->text,tokenPosition); // the new length of the token (cutting off what's behind it)
			// now to append the text in the rest of the tokens
			Mtoken* token=pLastCommandToEvaluateToken->next;
			if(token!=NULL){
				while(token!=NULL){string_append(restOfCommand,string(pLastCommandToEvaluateToken->text));token=token->next;}
				freeToken(token); // we'll free all the token starting at the successor of pLastCommandToEvaluateToken
				pLastCommandToEvaluateToken->next=NULL;
			}
			outputInfo("Rest of command: '%s'.",string(restOfCommand));
			return string(restOfCommand);
		}
	}
	return NULL;
} 
*/
/*
void writeRestOfCommand(){ // writes rest of command assuming pLastCommandToEvaluateToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=commandLength()-cursorPosition();
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(cursorPosition()>pLastCommandToEvaluateToken->offset){ // part of current token to write
			outputTokenColor(pLastCommandToEvaluateToken);printf("%s",string_remainder(pLastCommandToEvaluateToken->text,cursorPosition()-pLastCommandToEvaluateToken->offset));
		}
		// write the rest of the tokens
		writeTokens(pLastCommandToEvaluateToken->next);
		moveCursorLeft(leftToWrite);
	}
}
*/

char switchToControlMode(char* message){
	if(inputMode!=IM_CONTROL){
		if(inputMode==IM_COMMAND)clearCommand();
		resetOutputColor();
		if(message!=NULL)outputLine(message);
		inputMode=IM_CONTROL;
	}
	///////////outputFlags(); // show the user the current flags!!
	return 'o'; // to make the loop know to quit
	//output("\n%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}

void writeBehindCursorText(bool clearAfterBehindCursorText){
	uint16_t l=string_length(behindCursorText);
	if(l||clearAfterBehindCursorText){
		debugWrite("Behind cursor text to write: '%s'.",string(behindCursorText));
		resetOutputColor();
		setColor(getBehindCursorTextColor());
		if(l)output("%s",string(behindCursorText));
		if(clearAfterBehindCursorText){
			outputChar(' ');
			moveCursorLeft(l+1);
		}else
		if(l)
			moveCursorLeft(l); // back to where we started to write the behind cursor text
		if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken); // return to the color of the current token
	}
}

void backToPrompt(){
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	// replacing: restoreCursor();clearScreenFromCursor();
	uint16_t cp=cursorPosition();
	while(cp--)backspace(); // MDH@24APR2019 replacing: while(cursorPosition()>0){cursorPosition()--;backspace();}
	/*
	if(cursorPosition()>0){moveCursorLeft(cursorPosition());cursorPosition()=0;}
	clearScreenFromCursor();
	*/
	/* replacing:
	while(characterCount>0){
		characterCount--;
		moveCursorLeft(1);resetOutputColor();outputChar(' ');moveCursorLeft(1);
	}
	*/
}

void setCommandToEvaluate(Mtoken* pCommand){
	pLastCommandToEvaluateToken=pCommandToEvaluate=pCommand;
	writeCommand();
	writeBehindCursorText(false);
}
/**
 * setCommandIndex() accepts @newCommandIndex between 0 and commandCount at most
 * but 0 is now also accepted, returning to show pCommandToEvaluate (if any)
 */
void setCommandIndex(uint32_t newCommandIndex){
	commandIndex=newCommandIndex;
	// it's easier to go to the beginning of the line although we could be on the line below!!!!
	// replacing: 
	backToPrompt();
	clearScreenFromCursor();
	// MDH@24APR2019 obsolete: commandLength()=cursorPosition()=0; // do we need this????
	string_setlength(behindCursorText,0); // clear the behind cursor text (in any situation)
	if(commandIndex){
		Mtoken* token=commands[commandCount-commandIndex];
		// MDH@19APR2019: if not to accept the history command we use the previous command as behind cursor text
		if(amAcceptinghistorycommand()){ // use the history command as autocompletion text instead of accepting it immediately as command!!!
			setCommandToEvaluate(token);
			inputInfo("Showing registered command #%u.",(commandCount-commandIndex+1));
			return;
		}
		// the previous command will be used as behind cursor text!!
		while(token){
			///////outputText("(%s)",tokenText);
			string_append(behindCursorText,string(token->text));
			token=token->next;
		}
	}
	setCommandToEvaluate(NULL);
	clearInfo();
	/////////////printf("(%d)",commandLength());
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

void newCommand(){
	// MDH@24APR2019 obsolete: commandLength()=string_length(behindCursorText); // MDH@21APR2019: oops was 0 before...
	resetOutputColor(); // TODO do we need this here?????
	pLastCommandToEvaluateToken=pCommandToEvaluate=newToken(NULL);
}

// TODO copyCommand() should set ->expr correctly
void copyCommand(){
	// if fails to copy pCommandToEvaluate pLastCommandToEvaluateToken should end up as NULL
	pLastCommandToEvaluateToken=NULL;
	Mtoken* _tokenToCopy=pCommandToEvaluate;
	pCommandToEvaluate=NULL;
	// the essence is that pLastCommandToEvaluateToken points to the last token in pCommandToEvaluate
	// NOTE theoretically pLastCommandToEvaluateToken could be NULL due to newToken() failing to create a new token
	while(_tokenToCopy){
		pLastCommandToEvaluateToken=newToken(pLastCommandToEvaluateToken);
		pLastCommandToEvaluateToken->type=_tokenToCopy->type;
		/* TODO check whether the following is correct!!! guess not!!
		if(pLastCommandToEvaluateToken->type==TT_END_OF_FUNCTION_CALL||pLastCommandToEvaluateToken->type==TT_END_OF_LIST||pLastCommandToEvaluateToken->type==TT_END_OF_MAP){
			if(_tokenToCopy->expr)
				pLastCommandToEvaluateToken->expr=pLastCommandToEvaluateToken->expr->expr;
			else
				outputLine("BUG: End of argument list or map encountered, but not started.");
		}
		*/
		pLastCommandToEvaluateToken->expr=_tokenToCopy->expr; // MDH@20MAY2019: just copy the expr over!!!!
		pLastCommandToEvaluateToken->significantCharacterCount=_tokenToCopy->significantCharacterCount;
		// if failing to copy the text over get rid of the command constructed so far, and break
		if(!string_copy(_tokenToCopy->text,pLastCommandToEvaluateToken->text)){pLastCommandToEvaluateToken=NULL;break;}
		// MDH@24APR2019 obsolete: commandLength()+=string_length(pLastCommandToEvaluateToken->text);
		// some additional fields to copy over (NOT the offset is that is set automatically)
#ifdef __DEBUG__
        printf("%d:%s",pLastCommandToEvaluateToken->type,string(pLastCommandToEvaluateToken->text));
#endif
		if(!pCommandToEvaluate)pCommandToEvaluate=pLastCommandToEvaluateToken;
		// get the next token to copy...
		_tokenToCopy=_tokenToCopy->next;
	}
}

// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual newCommand
/**
 * setCommand() creates a new (empty) command (in pCommandToEvaluate) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *              which is supposedly showing behind the cursor!!!
 * ASSUMPTION should only be called when at the prompt (cursorPosition()=0) ready for starting or changing a command
 * setCommand() won't show the command anymore as we assume that any registered command passed in is already showing!!!
 */
/*
Mtoken* getCommand(){
	return(commandIndex&&cursorPosition()?commands[commandCount-commandIndex]:pCommandToEvaluate);
}
void echoCommand(){
	Mtoken* token=pCommandToEvaluate;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
void setCommand(Mtoken* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. cursorPosition()==0 and pCommandToEvaluate==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(pCommandToEvaluate){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	newCommandToEvaluate(); // NOTE might fail, in which case pLastCommandToEvaluateToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set pLastCommandToEvaluateToken!!!
		Mtoken* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically pLastCommandToEvaluateToken could be NULL due to newToken() failing to create a new token
		while(pLastCommandToEvaluateToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!string_copy(pNewToken->text,pLastCommandToEvaluateToken->text)){clearCommand();break;}
			// MDH@24APR2019 obsolete: commandLength()+=string_length(pLastCommandToEvaluateToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
			pLastCommandToEvaluateToken->type=pNewToken->type;
#ifdef __DEBUG__
            printf("%d:%s",pLastCommandToEvaluateToken->type,string(pLastCommandToEvaluateToken->text));
#endif
			pNewToken=pNewToken->next;
			if(!pNewToken)break;
			// we're going to need another token!!!
			pLastCommandToEvaluateToken=newToken(pLastCommandToEvaluateToken);
		}
		// if the user decides to start typing ascertain to show it in the right color!!
		if(pLastCommandToEvaluateToken)outputTokenColor(pLastCommandToEvaluateToken);
#ifdef __DEBUG__
		echoCommand();
#endif
	}
}
*/

// MDH@30APR2019: if the current token is a variable/function check whether it still is
//                call whenever the current token changes (in removePreviousTokenCharacter() and commandCharacterAccepted())
bool tokenCheckedForBeingAFunction(){
	bool result=false;
	if(pLastCommandToEvaluateToken->type==TT_VARIABLE||pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE){
		result=true;
		// is it a function (now)?
		if(getFunction(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // yes, it is
			// if a new variable before (now a function), remove the (assignment) character in the behind cursor text
			if(amMatchingparentheses())if(pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE)if(string_char(behindCursorText,0)=='=')string_removed_char(behindCursorText,0);
			// the minimum we can do is put an opening parenthesis in the behind cursor text
			pLastCommandToEvaluateToken->type=TT_FUNCTION;
			reoutputToken(pLastCommandToEvaluateToken);
			// insert an opening parenthesis for the function call
			if(amMatchingparentheses())if(string_char(behindCursorText,0)!='(')string_insert_char(behindCursorText,0,'(');
		}
	}else
	if(pLastCommandToEvaluateToken->type==TT_FUNCTION){
		result=true;
		// is it (still) a function?
		if(!getFunction(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // no, it ain't
			// the minimum we can do is remove the opening parenthesis behind it (if it is still there!!!!!)
			pLastCommandToEvaluateToken->type=TT_VARIABLE;
			reoutputToken(pLastCommandToEvaluateToken);
			//////////outputInfo("Variable redrawn!");
			// remove any opening parenthesis from the behind cursor text
			if(amMatchingparentheses())if(behindCursor())if(string_char(behindCursorText,0)=='(')string_removed_char(behindCursorText,0);
		}
	}
	// non-existing variables should be assigned to so it's a good idea to put the assignment operator behind it, although it might be hard to remove it though
	if(result){ // a variable or function
		// check whether the variable exists or not
		if(pLastCommandToEvaluateToken->type==TT_VARIABLE){ // a (new) variable
			if(!containsVariable(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // apparently does NOT exist
				pLastCommandToEvaluateToken->type=TT_NEW_VARIABLE;
				reoutputToken(pLastCommandToEvaluateToken);
				if(amMatchingparentheses())if(string_char(behindCursorText,0)!='=')string_insert_char(behindCursorText,0,'=');
			}
		}else
		if(pLastCommandToEvaluateToken->type==TT_NEW_VARIABLE){ // a new variable
			if(containsVariable(_Menvironment,string(pLastCommandToEvaluateToken->text))){ // now an existing variable
				pLastCommandToEvaluateToken->type=TT_VARIABLE;
				reoutputToken(pLastCommandToEvaluateToken);
				if(amMatchingparentheses())if(string_char(behindCursorText,0)=='=')string_removed_char(behindCursorText,0);
			}
		}
	}
	return result;
}

// in response to backspace the previous token character is to be removed
void removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	char removedCharacter=removedTokenCharacter(1);
	if(removedCharacter){
		// adapt screen
		moveCursorLeft(1); // will decrement cursorPosition() // MDH@24APR2019: NOT anymore...
		clearScreenFromCursor(); // will clear what's behind the cursor
		// MDH@24APR2019 obsolete: commandLength()--; // decrement the total command length
		if(cursorPosition()){ // still something left of the command (that we might check for being a function or not)
			// on screen as well please
			// before writing the behind cursor text we're going to check whether the current token still is a function or variable
			tokenCheckedForBeingAFunction();
			// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in behindCursorText that's what we should now write
			writeBehindCursorText(false);
			// replacing: if(pCommandToEvaluate)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
		}/* removedTokenCharacter() calls removeToken which will NULL the pCommandToEvaluate and pLastCommandToEvaluateToken when the first command character is removed, in which case we do not need:
			else clearCommand();*/
	}else // MDH@03MAY2019: can't switch to control mode here (so we just report the error!!!)
		inputError("%s","Failed to remove the last entered character.");
}

void outputTokenInfo(){
	Mtoken* token=pCommandToEvaluate;
	uint16_t tokenIndex=0;
	output("\n%s:","Tokens");
	output("\n%s\t%s\t%s\t%s\t%s\t\t\t%s","#","OFFSET","USED","LENGTH","TYPE","TEXT");
	while(token!=NULL){
		tokenIndex++;
		output("\n%u\t%u\t%u\t%u\t%-24s`%s`",tokenIndex,token->offset,token->significantCharacterCount,string_length(token->text),TOKENTYPE_STRING[token->type],string(token->text));
		token=token->next;
	}
}

bool isBinaryOperatorTokenType(uint8_t tokenType){return(TOKENTYPE_IDS[tokenType]>>4)==0b0110;}

// MDH@12APR2019: in order to implement the Tab character we have to delegate entering a character (typed) to a separate function
//       		  ASSERTION pCommandToEvaluate and pLastCommandToEvaluateToken are  NOT  NULL
//                the endofinput flag is used to indicate whether this is the end of the input
bool commandCharacterAccepted(char inputChar,char inputCharacterType,bool endOfInput){
	// MDH@21APR2019: there are two situation where we need to get a command
	//                1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(!pCommandToEvaluate) // no current command
		newCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyCommand();
	// if pLastCommandToEvaluateToken is now NULL something went wrong (in copyCommand or newCommand most likely)
	if(pLastCommandToEvaluateToken==NULL)return false;
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	clearInfo(); // TODO make a separate function to do this???

	/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
	if(inputCharType=='C'){
		pLastCommandToEvaluateToken->type^=0x70; // toggling bit 7
		// a comment character will NEVER change the (actual) token type but it should change the color to use
		if(pLastCommandToEvaluateToken->type&0x70){commenting=true;outputTokenColor(pLastCommandToEvaluateToken);}else notCommenting=true; // if a comment was started, switch to the comment token color
	}else // not a comment character
	if((pLastCommandToEvaluateToken->type&0x70)==0){ // not in a comment
		if(notCommenting){notCommenting=false;outputTokenColor(pLastCommandToEvaluateToken);} // if behind coming out of a comment, we have to reset the output token color
	*/
	/*
	// MDH@26FEB2019: when a user starts inserting characters instead of appending them we can cut off the rest of the characters in the command
	//                and put it in a single mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/
	// determine the token type associated with the newly inputted character
	// MDH@28MAR2019: if we're in a binary token type with the repeatable flag set AND the user has repeated the previous first token character the inputCharacterType should become R to get the right transition
	if((TOKENTYPE_IDS[pLastCommandToEvaluateToken->type]&0x62)==0x62)if(inputChar==string_char(pLastCommandToEvaluateToken->text,0))inputCharacterType='R';
	// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
	if(inputCharacterType=='W')if(pLastCommandToEvaluateToken->type==TT_ERROR||pLastCommandToEvaluateToken->type==TT_COMMENT||pLastCommandToEvaluateToken->type==TT_DQSTRING||pLastCommandToEvaluateToken->type==TT_SQSTRING)inputCharacterType='w';
	if(inputCharacterType!='W'){ // only characters that are not whitespace can start a new token
		int16_t newTokenType=nextTokenType(pLastCommandToEvaluateToken->type,inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
#ifdef __DEBUG__
	resetOutputColor();
	printf("[%d+%c->%d]",pLastCommandToEvaluateToken->type,inputCharacterType,newTokenType);
	outputTokenColor(pLastCommandToEvaluateToken);
#endif
		// TODO just like unary operators expressions, maps and list end immediately
		// some combinations are (still) not allowed...
		if(newTokenType==pLastCommandToEvaluateToken->type){
			// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators and list element tokens (although undefined list element cells do not need to be inserted!!)
			if(pLastCommandToEvaluateToken->type!=TT_UNARY&&pLastCommandToEvaluateToken->type!=TT_TERNARY_aeru&&pLastCommandToEvaluateToken->type!=TT_LISTELEMENT&&pLastCommandToEvaluateToken->significantCharacterCount>0){
				newTokenType=TT_ERROR;
				if(amVerbose())inputError("Token already finished!");
			}
		}else{ // different token types
			// a shortcut assignment can NOT be turned into a equality comparison
			if(inputCharacterType=='='&&pLastCommandToEvaluateToken->type==TT_ASSIGNMENT&&(pLastCommandToEvaluateToken->prev->type==TT_BINARY_AeRu||pLastCommandToEvaluateToken->prev->type==TT_BINARY_Aeru)){
				newTokenType=TT_ERROR;
				if(amVerbose())inputError("A shortcut operator assignment cannot change into an equality.");
			}
		}
		// MDH@03MAY2019: no matter what the new token type is, any token of type TT_EXPRESSION always ends immediately...
		//                this is because the first (offset) token in a command is always of type TT_EXPRESSION which should end immediately on any next token although significantCharacterCount will still be zero
		//                this way it will always be there!!
		if(newTokenType!=pLastCommandToEvaluateToken->type||pLastCommandToEvaluateToken->type==TT_EXPRESSION||pLastCommandToEvaluateToken->significantCharacterCount>0){
			// MDH@10APR2019: NOT every new token type starts a new token:
			//                if we're in a binary operator and move to another binary operator type it's an extension
			//                NO we decide NOT to do this when the command is evaluated we should compose the values and apply the operators
			///////if(!isBinaryOperatorTokenType(pLastCommandToEvaluateToken->type)||!isBinaryOperatorTokenType(newTokenType))

			// MDH@16APR2019: a character that is assumed to indicate the assignment operator has to be checked because it could well be the = that starts the binary equality operator
			//                which means we have to switch from assignment token to BearU token (which is unfinished)
			if(newTokenType==TT_ASSIGNMENT){
				// checking for validity of accepting as assignment is not that easy
				// we can allow a binary operator in front of the assignment of course in that case it definitely is an assignment if it is not the = is an error!!
				bool behindBinaryOperator=(pLastCommandToEvaluateToken->type==TT_BINARY_AeRu||pLastCommandToEvaluateToken->type==TT_BINARY_Aeru);
				// NOTE if behind binary operator there must always be a token in front of it, so pLastCommandToEvaluateTokenToCheck cannot be NULL!!
				// MDH@21MAY2019: possibly we have multiple tokens representing a binary operator (like ** << and >> which are allowed!!!) so we need to skip all binary operators in front of the assignment character
				Mtoken* pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateToken;
				if(behindBinaryOperator)while(pLastCommandToEvaluateTokenToCheck->type>=3&&pLastCommandToEvaluateTokenToCheck->type<=7)pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->prev;
				if(amVerbose())inputInfo("Type of token to check: %s.",TOKENTYPE_STRING[pLastCommandToEvaluateTokenToCheck->type]);
				// ASSERT pLastCommandToEvaluateTokenToCheck should either represent a variable or the end of a list element to allow for operator
				if(pLastCommandToEvaluateTokenToCheck->type==TT_END_OF_LIST){ // end of a list
					// we have to find the associated start of the list, and the token in front of that (which should be a variable!!!)
					// which is easy because the expr tells us the start of the list BUT 
					pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->expr;
					///////////if(amVerbose())inputInfo("Presumed list start token");
					if(pLastCommandToEvaluateTokenToCheck)pLastCommandToEvaluateTokenToCheck=pLastCommandToEvaluateTokenToCheck->prev;else inputError("%s","Start of index list not found!");
				}
				// two options: = behind a binary operator without variable (or list) in front of it is not allowed, i.e. an error, otherwise we assume that = represents the first = of == the equality operator...
				if(pLastCommandToEvaluateTokenToCheck==NULL||(pLastCommandToEvaluateTokenToCheck->type!=TT_VARIABLE&&pLastCommandToEvaluateTokenToCheck->type!=TT_NEW_VARIABLE)){
					if(behindBinaryOperator){
						newTokenType=TT_ERROR;
						//if(amVerbose())inputError("No variable to assign to.");
					}else
						newTokenType=TT_BINARY_aErU;
				}
			}

			pLastCommandToEvaluateToken=newToken(pLastCommandToEvaluateToken);
/*
#ifdef __DEBUG__
			printf("@%p=%p?:%s",pCommandToEvaluate,pLastCommandToEvaluateToken,string(pCommandToEvaluate->text));
#endif
*/
			// ending a function call, list or map is only allowed with expr defined
			if(newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_MAP){
				if(pLastCommandToEvaluateToken->expr){
					// check whether the match is correct
					switch(newTokenType){
						case TT_END_OF_FUNCTION_CALL:
							if(pLastCommandToEvaluateToken->expr->type!=TT_FUNCTION_CALL&&pLastCommandToEvaluateToken->expr->type!=TT_EXPRESSION){
								/////inputError("%s","No function call or expression to end here!");
								inputError("End of function call/expression character does not match '%s' of type '%s'!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								newTokenType=TT_ERROR;
							}
							break;
						case TT_END_OF_LIST:
							if(pLastCommandToEvaluateToken->expr->type!=TT_LIST&&pLastCommandToEvaluateToken->expr->type!=TT_LISTELEMENT){
								inputError("End of list character does not match '%s' of type '%s'!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								newTokenType=TT_ERROR;
							}
							break;
						case TT_END_OF_MAP:
							if(pLastCommandToEvaluateToken->expr->type!=TT_MAP){
								inputError("End of map character does not match '%s' of type '%s'!",string(pLastCommandToEvaluateToken->expr->text),TOKENTYPE_STRING[pLastCommandToEvaluateToken->expr->type]);
								//inputError("No map to end here!");
								newTokenType=TT_ERROR;
							}
							break;
					}
					// if we do the following we need to move 
					// TODO we can do the following even on error but what if we didn't???
					///////////////////pLastCommandToEvaluateToken->expr=pLastCommandToEvaluateToken->expr->expr;
				}else{
					inputError("%s","Can't end a (function argument) list or map here!");
					newTokenType=TT_ERROR;
				}
			}

			pLastCommandToEvaluateToken->type=newTokenType;
			
			if(pLastCommandToEvaluateToken->type==TT_UNARY)pLastCommandToEvaluateToken->significantCharacterCount=1;
			// MDH@15APR2019: there are some other characters as well, that immediately end the token like parentheses, comma's and semicolons and ? and : TODO are there more??????
			if(pLastCommandToEvaluateToken->significantCharacterCount==0)
				if(pLastCommandToEvaluateToken->type!=TT_ERROR&&pLastCommandToEvaluateToken->type!=TT_COMMENT&&pLastCommandToEvaluateToken->type!=TT_DQSTRING&&pLastCommandToEvaluateToken->type!=TT_SQSTRING)
					if(inputCharacterType=='('||inputCharacterType=='['||inputCharacterType=='{'||inputCharacterType==','||inputCharacterType==';'||inputCharacterType==':'||inputCharacterType=='?')
						pLastCommandToEvaluateToken->significantCharacterCount=1;
			// TODO should we write the associated colors here?????
			outputTokenColor(pLastCommandToEvaluateToken);
		}
	}else // a functional whitespace character, ends a current token!!
	if(pLastCommandToEvaluateToken->significantCharacterCount==0&&pLastCommandToEvaluateToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
		pLastCommandToEvaluateToken->significantCharacterCount=string_length(pLastCommandToEvaluateToken->text);

	// append the typed character at cursorPosition() minus current token offset in pLastCommandToEvaluateToken->text
	string_append_char(pLastCommandToEvaluateToken->text,inputChar);
#ifdef __DEBUG__
	printf("[%s]",string(pLastCommandToEvaluateToken->text));
#endif
	// MDH@24APR2019 obsolete: commandLength()++; // increment total command length
	outputChar(inputChar); ///////// replacing: outputLastTokenChar(pLastCommandToEvaluateToken); // echo the last token character
	
	if(endOfInput){
		//////if(amAssisting())output(":%c",inputCharacterType);
		debugWrite("Command length after inserting %c: %" PRIu16 ".",inputChar,commandLength());
	}

	// MDH@24APR2019 obsolete: cursorPosition()++; // increment the current cursor position

	if(endOfInput){
		// MDH@29APR2019: I'd like to detect when a variable becomes a function or vice versa
		if(!tokenCheckedForBeingAFunction()){
			// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
			if(pLastCommandToEvaluateToken->type==TT_BINARY_aErU){string_insert_char(behindCursorText,0,'=');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
			// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
			if(amMatchingparentheses()){
				if(pLastCommandToEvaluateToken->type!=TT_ERROR){ // MDH@29APR2019: don't add closing bracket to autocompletion text when in error!!!
					if(inputCharacterType=='['){string_insert_char(behindCursorText,0,']');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
					if(inputCharacterType=='{'){string_insert_char(behindCursorText,0,'}');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
					if(inputCharacterType=='('){string_insert_char(behindCursorText,0,')');/* MDH@24APR2019 obsolete: commandLength()++;*/}
				}
			}
		}
		writeBehindCursorText(true); // just in case we removed some character (see TT_FUNCTION->TT_VARIABLE)
		debugWrite("Command length after writing behind cursor text: %" PRIu16 ".",commandLength());
		outputStatus(inputChar,inputCharacterType);
	}

	return true;
}

bool COMMAND_PROCESSOR_AVAILABLE=0;
void clearShellCommand(){
	string_setlength(behindCursorText,0);
	string_setlength(shellCommand,0);
	// MDH@24APR2019 obsolete: cursorPosition()=0;
}
void executeShellCommand(){
	// ASSERTION string_length(shellCommand) should be positive
	output("\n"); // get a new line before we see the result of executing this command!!
	int result=system(string(shellCommand));
	if(result)output("Result: %d.\n",result); // non-zero result
	clearShellCommand(); // ready for the next execution
}
char switchToShellMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)output("\n%s",message);
	inputMode=IM_SHELL;
	clearShellCommand();
	return 's';
	//output("\n%s\n $ ","Enter your shell command, and press the Return button to execute.");
}
void switchToCommandMode(){
	if(inputMode==IM_COMMAND)return;
	inputMode=IM_COMMAND;
	string_setlength(behindCursorText,0); // ascertain to not have autocompletion text
}

int main(int argc, char **argv){

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

	// MDH@23FEB2019: how about being able to continue with commands stored in a file, or perhaps allow for -log <logfile> or log=
	// whereas any filename without prefix is the file to execute at the start
	if(argc>1){
		printf("%s\n","Arguments");
		for(int arg=1;arg<argc;arg++){
			printf("%i. %s\n",arg,argv[arg]);
			if(argv[arg][0]=='-'){ // a flag (or flags)
				int i=0;
				while(argv[arg][++i]){
					if(argv[arg][i]=='s')setVerbose(true);else
					if(argv[arg][i]=='S')setVerbose(false);else
					if(argv[arg][i]=='d')setDebugging(false);else
					if(argv[arg][i]=='D')setDebugging(true);else
					if(argv[arg][i]=='a')setAssisting(false);else
					if(argv[arg][i]=='A')setAssisting(true);else
					if(argv[arg][i]=='m')setMatchingparentheses(false);else
					if(argv[arg][i]=='M')setMatchingparentheses(true);else
					if(argv[arg][i]=='C')setColorscheme(0);else // light color scheme
					if(argv[arg][i]=='c')setColorscheme(1);else // dark color scheme
					if(argv[arg][i]=='W')setWrapping(true);else
					if(argv[arg][i]=='w')setWrapping(false);else
					if(argv[arg][i]>='0'&&argv[arg][i]<='9')setColorscheme(argv[arg][i]-'0'); // a digit indicating the color scheme to use
				}
			}
		}
	}

	prepareForUserInput(); // AFTER using the command-line parameters (will effectuate wrap mode and color scheme)

	resetOutputColor(); // just in case
	outputLine("Welcome to M, the fancy interpreter.");
	outputLine("");
	displayFlags();
	outputLine("");
	
	if(!initEnvironment()){ // ascertain to have an execution environment!!!
		setColor(getErrorColor());setBackColor(getBackgroundColor());
		outputLine("Exiting: due to failing to initialize the M execution environment!");
		resetOutputColor();
		exit(1);
	}
	if(amVerbose())output("\nM environment initialized with %llu predefined values.",getNumberOfValues());

	mstring* predefinedVariableNames=_getVariableNames(_Menvironment,", ");
	if(predefinedVariableNames){
		output("\nPredefined variables: %s.",string(predefinedVariableNames));
		free_mstring(predefinedVariableNames); // no get rid of it!!!
	}else
		outputLine("No predefined variables!");
	//////////output("\nNumber of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	shellCommand=string_create(); // MDH@12APR2019: allow executing shell commands (calling system())
	behindCursorText=string_create(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	pCommandToEvaluate=NULL; // the current command (token)

	///// writeCommand() will take care of this!!!! commandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

	char inputChar,inputCharType;

	outputLine("");
	outputLine("Use Ctrl-Z to exit M immediately at any time.");
	outputLine("In any mode press the Enter key on an empty line to switch modes.");

	while(1){

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//                if pCommandToEvaluate is NOT null, we have to make it NULL
		commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
		if(pCommandToEvaluate) // if we still have a command to free, free it entirely
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
			// MDH@24APR2019: pCommandToEvaluate could be non-null if we failed to evaluate it (e.g. when being imcomplete), and we allow a retry
			//                NOTE registered commands should always be successfully evaluated, so do NOT get rid of any pending command!!!!
			if(pCommandToEvaluate){writeCommand();writeBehindCursorText(false);}else string_setlength(behindCursorText,0);
			/////////////// if(pCommandToEvaluate)clearCommand(); // TODO do we need this????
			/* replacing:
			if(pCommandToEvaluate==NULL)if(!string_setlength(behindCursorText,0))output("??"); // TODO should we be loosing behindCursorText here????
			commandLength()=cursorPosition()=writeTokens(pCommandToEvaluate);
			*/
			/////////outputStatus();
		}
		// which used to be: writeCommand(); // write the current command (if any)

		/* replacing:
		pLastCommandToEvaluateToken=pCommandToEvaluate;
		// an existing command to show
		while(pLastCommandToEvaluateToken!=NULL){
			outputToken(pLastCommandToEvaluateToken);
			// the cursor will move along with every printf()
			cursorPosition()+=string_length(pLastCommandToEvaluateToken->text);
			pLastCommandToEvaluateToken=pLastCommandToEvaluateToken->next;
		}
		*/
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(inputCharRead(&inputChar)){
			
			////////inputChar=getInputChar();

			if(inputChar>127)continue; // undefined input character

			inputCharType=INPUTCHARACTERTYPES[inputChar];
			
			////////printf("(%d)",inputCharType);

			// if not in control mode, and the switch to control mode character is entered, switch to control mode if first character (NOTE cursorPosition() is only defined in the other two modes)
			// MDH@16APR2019: I want to use the Enter key (ASCII 13) to switch to the next mode, because the associated input character type is n which will ALWAYS break
			//                in that case we do NOT need the o input character type!!!
			if(inputCharType=='o'){
				if(inputMode==IM_CONTROL){
					switchToCommandMode();
					break;
				}
				// not in control mode, go to control mode if first character on line
				if(!cursorPosition()){
					inputCharType=switchToControlMode(NULL);
					break;
				}
				// accept (might be an acceptable character in string literal in commands or in shell commands)
			}

			/////if(inputChar!=ESCAPE_CHARACTER)printf("(%d)",inputChar);

			// special (control) input character types
			// first the ones that will break in any input mode!!!!
			if(inputCharType=='i')continue; // insignificant input character without specific purpose

			if(inputCharType=='n')break; // end-of-line (CR of LF) character
			if(inputCharType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character

			// from now on no continue's anymore, because at the end of the loop we want to check for inputCharType equaling o
			if(inputMode==IM_COMMAND){
#ifdef __DEBUG__
				outputChar(inputCharType);
#endif
				//////////outputStatus(inputChar,inputCharType);
				if(inputCharType=='d'){ // MDH@18APR2019: delete now always deletes the first character in the behind cursor text
					/////debugWrite("DELETE");
					if(string_length(behindCursorText)){
						if(string_removed_char(behindCursorText,0)){ // success!!!
							writeBehindCursorText(true);
						}else
							inputCharType=switchToControlMode("Failed to remove the first character in the auto-complete text.");	
					}else
						beep();
				}else
				if(inputCharType=='b'){ // backspace
					///////debugWrite("BACKSPACE");
					// something to remove?
					if(cursorPosition()) // TODO pCommandToEvaluate should be NULL at the same time commandLength() becomes 0!!!
						removePreviousTokenCharacter();
					else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-D)
					// replacing: if(pCommandToEvaluate!=NULL){clearCommand();break;}beep(); 
					if(pCommandToEvaluate!=NULL){
						backToPrompt();
						clearCommand();
						/////////if(amWrapping()())break; // if in amWrapping()() can't guarantee backspace() to move into the previous line which means just prompt again...
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a behindCursorText)
					uint16_t bc=behindCursor();
					if(bc){
						// normally all will be Ok, and we can (post)decrement bc until it's zero
						while(bc--){
							char newInputChar=string_removed_char(behindCursorText,0);
							if(!newInputChar){
								writeBehindCursorText(false); // there will be characters behind the cursor left to show
								inputCharType=switchToControlMode("Failed to remove the suggested character.");
								break;
							}
							// MDH@24APR2019 obsolete: commandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],bc==0)){
								inputCharType=switchToControlMode("Failed to accept the suggested character.");
								break;
							}
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead(&inputChar)){
						///printf("(%d)",inputChar);
						if(inputChar==91){
							if(inputCharRead(&inputChar)){//inputChar=getInputChar();
								///printf("(%d)",inputChar);
								if(inputChar==51){
									if(inputCharRead(&inputChar)){//inputChar=getInputChar();
										if(inputChar==126){ // delete
											if(string_length(behindCursorText)){
												if(string_removed_char(behindCursorText,0))
													writeBehindCursorText(true);
												else
													inputCharType=switchToControlMode("Failed to remove the first character of the auto-completion text.");	
											}else // nothing under the cursor to delete
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(!commandIndex&&pCommandToEvaluate)
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
										if(!commandIndex&&pCommandToEvaluate)
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
									if(behindCursor()){
										// MDH@22MAR2019: instead of doing everything here (duplicating all code that is down below), we can find a way to use the 'normal' code
										////////////bool success=false;
										char newInputChar=string_removed_char(behindCursorText,0);
										if(newInputChar){
											// MDH@24APR2019: commandLength()--;
											if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],true))
												inputCharType=switchToControlMode("Suggested character extracted, but not accepted.");
										}else
											inputCharType=switchToControlMode("Suggested character not extracted!");
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									if(cursorPosition()){
										// TODO apparently pCommandToEvaluate will still be NULL when we're scrolling through the list of previous commands...
										if(commandIndex)copyCommand(); // will also set commandLength()!!!
										// MDH@27FEB2019: we should remove the last character of the current token (and command) and move it into behindCursorText
										bool success=false;
										char c=removedTokenCharacter(1);
										if(c){ // removing the character behind the cursor succeeded
											debugWrite("Character '%c' removed.",c);
											// prefix it to behindCursorText
											string_insert_char(behindCursorText,0,c);
											debugWrite("Behind cursor text: '%s'.",string(behindCursorText));
											success=true;
										}
										if(success){
											moveCursorLeft(1);
											/* NO going back and forth with the cursor does NOT change commandLength()!!!!
											commandLength()--; // MDH@28FEB2019: essential bro' otherwise when we go back to the end, cursorPosition() will stay below commandLength()
											*/
											// if the cursor position now matches the offset of the current token
											// i.e. the current token is now empty!!!!
											if(!string_length(pLastCommandToEvaluateToken->text)){
												// we can check the offset to see if this is the first token, but pLastCommandToEvaluateToken->prev is a little more secure
												pLastCommandToEvaluateToken=freeToken(pLastCommandToEvaluateToken);
												if(!pLastCommandToEvaluateToken)pCommandToEvaluate=NULL; // if no last command token anymore, we apparently released the first command token
											}
											writeBehindCursorText(false);
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
					// MDH@21APR2019: creating a command if need be is delegated to commandCharacterAccepted() which we know
					//                we always need a command (being edited)
					if(!commandCharacterAccepted(inputChar,inputCharType,true))
						inputCharType=switchToControlMode(pCommandToEvaluate?"Failed to accept the character.":"Failed to create a new command");
					/*
					// we need to have a token (to append the input character to) which initializes to pCommandToEvaluate
					if(pCommandToEvaluate==NULL) // no first command token
						// if commandIndex we should one of the registered commands
						setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set commandLength()!!!
					// if still NULL (also when we fail to actually create a new first command token)
					if(pLastCommandToEvaluateToken!=NULL){
						if(!commandCharacterAccepted(inputChar,inputCharType,true))
							switchToControlMode("Failed to accept the character.");
					}else
						switchToControlMode("Failed to create a new command!");
					*/
				}
				outputStatus(inputChar,inputCharType);
			}else
			if(inputMode==IM_CONTROL){ // inputChar received in control mode
				outputChar(inputChar); // nice to see the character we typed...
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					// single character responses (and out again)
					if(inputChar=='a'||inputChar=='A'){setAssisting(inputChar=='A');inputCharType='n';break;}
					if(inputChar=='d'||inputChar=='D'){setDebugging(inputChar=='D');inputCharType='n';break;}
					if(inputChar=='m'||inputChar=='M'){setMatchingparentheses(inputChar='M');inputCharType='n';break;}
					if(inputChar=='s'||inputChar=='S'){setVerbose(inputChar=='s');inputCharType='n';break;} // 'amVerbose()' is implemented using 'silent' flag!!!
					if(inputChar=='u'||inputChar=='U'){setAcceptinghistorycommand(inputChar='U');inputCharType='n';break;}
					if(inputChar>='0'&&inputChar<='9'){setColorscheme(inputChar-'0');inputCharType='n';break;}
					if(inputChar=='w'||inputChar=='W'){setWrapping(inputChar=='W');inputCharType='n';break;}
					if(inputChar=='v'||inputChar=='V'){outputVariables();inputCharType='n';break;}
					if(inputChar=='f'||inputChar=='F'){outputFunctions();inputCharType='n';break;}
					// options
					if(inputChar=='x'||inputChar=='X'){inputCharType='x';break;}
					if(inputChar=='s'||inputChar=='S')inputCharType=switchToShellMode(NULL);
					if(inputChar=='h'||inputChar=='H'){
						// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
						// we could call them a, b, c etc.
						if(commandCount){
							commandPages=1+(commandCount-1)/10;
							showNextCommandPage(); // as soon as commandPage>0 we are paging...
						}else
							output("\n%s\n","No previous commands to show.");
					}
				}else
					// user might have selected one of the commands (letter a through j)
					commandPage=0; // stop paging
			}else{ // Shell command input mode
				// we still allow using certain 'special' characters for composing the command (much like we did with a command)
				if(inputCharType=='b'){ // backspace
					uint16_t cp=cursorPosition();
					if(cp){
						if(string_removed_char(shellCommand,cp-1)){
							moveCursorLeft(1);
							writeBehindCursorText(false);
						}else
							inputCharType=switchToControlMode("Failed to remove the shell command character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='d'){
					if(string_length(behindCursorText)){ // something behind the cursor that we can remove
						if(string_removed_char(behindCursorText,0))
							writeBehindCursorText(true);
						else
							inputCharType=switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					if(pCommandToEvaluate!=NULL){
						backToPrompt();
						clearShellCommand();
						//////////if(amWrapping()())break;
					}else
						beep();
				}else
				if(inputCharType=='t'){ // Tab character
					// if there's a preview (well, code completion by way of a behindCursorText)
					uint16_t bc=behindCursor();
					if(bc){
						while(bc--){
							char newInputChar=string_removed_char(behindCursorText,0);
							if(!newInputChar){
								writeBehindCursorText(false);
								inputCharType=switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							string_append_char(shellCommand,newInputChar);
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead(&inputChar)){////inputChar=getInputChar();
						if(inputChar==91){
							if(inputCharRead(&inputChar)){/////inputChar=getInputChar();
								if(inputChar==51){
									if(inputCharRead(&inputChar)){///////inputChar=getInputChar();
										if(inputChar==126){ // delete
											// TODO FIX this does not seem to be right!!!!!
											if(behindCursor()){
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
									if(behindCursor()){
										char newInputChar=string_removed_char(behindCursorText,0);
										if(!newInputChar){
											writeBehindCursorText(false);
											inputCharType=switchToControlMode("Failed to accept the suggested characters.");
										}else
											string_insert_char(shellCommand,cursorPosition(),newInputChar);
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									uint16_t cp=cursorPosition();
									if(cp){
										bool success=false;
										char c=string_removed_char(shellCommand,cp-1);
										if(c){ // removing the character behind the cursor succeeded
											// prefix it to behindCursorText
											string_insert_char(behindCursorText,0,c);
											moveCursorLeft(1);
											writeBehindCursorText(false);
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
					string_append_char(shellCommand,inputChar);
					outputChar(inputChar);
					// MDH@24APR2019: cursorPosition()++;
				}
			}
			// if switched to control mode, inputCharType will be equal to 'o' and we break out of this input loop!!!
			if(inputCharType=='o'||inputCharType=='s')break;
		} // end of character input 

		// if eXit input character(s) received...
		if(inputCharType=='x')break;

		// MDH@16APR2019: now if we use n to switch modes as well, we can do that if there's no command
		if(inputCharType=='n'){
			if(inputMode==IM_COMMAND){ // the newline character ends the command to be evaluated!!
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				// if pCommandToEvaluate is set, we have a command to evaluate
				/*
				Mtoken* pCommandToEvaluateToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(pCommandToEvaluate){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(cursorPosition()>string_length(pCommandToEvaluate->text)){
						// finish the last token???
						if(pLastCommandToEvaluateToken->significantCharacterCount==0)pLastCommandToEvaluateToken->significantCharacterCount=string_length(pLastCommandToEvaluateToken->text);
						pCommandToEvaluateToEvaluate=pCommandToEvaluate; // but only when not at start of command!!!
						if(amDebugging())outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&cursorPosition()) // NOTE using cursorPosition() is better than using amAcceptinghistorycommand() (causing it!!)
					pCommandToEvaluateToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(pCommandToEvaluate){ // technically something to evaluate
					if(!evaluateCommand()){
						mstring* commandText=_getCommandText(false);
						if(!string_length(commandText)){
							clearCommand();
							output("Nothing to evaluate!");
						}else // MDH@16MAY2019: no need to tell the user that evaluation failed, because an error message would have been shown to indicate what went wrong (see evaluateCommand())
							output("\nPlease complete, correct or cancel the command.",string(commandText));
						free_mstring(commandText);
						continue;
					}
					if(amVerbose())outputLine("Command evaluated!");
					string_setlength(behindCursorText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
					// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
					if(!registerCommand()){
						if(amVerbose())outputLine("Command registered!");
						// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
						if(!commandIndex){ // a new command being registered!!!
							freeToken(pCommandToEvaluate);
							outputLine("ERROR: Failed to register the command! Out of memory?");
						}else
							outputLine("ERROR: Failed to register the command again! Out of memory?");
					}
					// start anew (without a current command to evaluate!!!!)
					pLastCommandToEvaluateToken=pCommandToEvaluate=NULL; // remove reference to current command

					// remove any values not used anymore...
					size_t removedValueCount=getNumberOfRemovedValues();
					if(amVerbose())output("\nNumber of removed values: %lu.",removedValueCount);

				}else
				if(behindCursor()==0)
					switchToControlMode(NULL);
			}else
			if(inputMode==IM_SHELL){
				if(string_length(shellCommand))
					executeShellCommand();
				else // MDH@16APR2019: back to command mode
					switchToCommandMode();
			}else // Return key in control mode, always to return to command input!!
				switchToCommandMode();
		}
	}
	// 'normal' exit
	exit(0);
}
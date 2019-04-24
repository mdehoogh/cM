// remove the line below when not in debug mode
//#define __DEBUG__

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>

//#include <cstdlib>

FILE* debugfile=NULL;
#include <stdarg.h>
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
void debugWrite(const char* fmt,...){
	if(debugfile==NULL)debugfile=fopen("./Mdebug.txt","w");
	if(debugfile==NULL)return;
    va_list args;
    va_start(args,fmt);
	/*
    time_t now;
    char buffer[20];
    time(&now);
    strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",gmtime(&now));
    fprintf(debugfile,"[%s] ",buffer);
    */
    vfprintf(debugfile,fmt,args);
    fputc('\n',debugfile);
    fflush(debugfile);
    va_end(args);
}

// Mexecution includes mstring.h
#include "Mexecution.h"

// helper functions
Mreal* getMreal(long double ld){
	Mreal* pMreal=(Mreal*)malloc(sizeof(Mreal));
	if(pMreal)pMreal->ld=ld;
	return pMreal;
}
Minteger* getMinteger(long long ll){
	Minteger* pMinteger=(Minteger*)malloc(sizeof(Minteger));
	if(pMinteger)pMinteger->ll=ll;
	return pMinteger;
}
/*
Mvalueunion* getMNumberValueunion(Mnumber* pMnumber){
	if(!pMnumber)return NULL;
	Mvalueunion* pMnumberValueunion=(Mvalueunion*)malloc(sizeof(Mvalueunion));
	if(pMnumberValueunion){pMnumberValueunion->n=pMnumber;} // store the Mnumber instance
	return pMnumberValueunion; // redirection operator
}
bool initVariable(Menvironment* pMenvironment,char* name,double d){
	Mvalueunion* pMValueunion=getMNumberValueUnion(getDoubleMnumber(d)); // dynamically allocated value union (i.e. on the heap)
	return setVariableValue(addVariable(pMenvironment,name,VT_NUMBER),*pMValueunion); // pass the union itself (can you actually assign a union???)
}
*/
// there will be a root (M) environment

const long double LD_PI=3.141592653589793238462643383279L; // 30 decimal digits of PI
const long double LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E

struct Menvironment* pMenvironment;
bool initEnvironment(){
	pMenvironment=calloc(1,sizeof(Menvironment));
	if(pMenvironment){
		Mmap* environmentVariableMap=calloc(1,sizeof(Mmap));
		if(environmentVariableMap){
			pMenvironment->variableMap=environmentVariableMap;
			// create and add PI and E constants!!!
			if(!setValueOfRealVariable(addVariable(pMenvironment,"PI",VT_REAL),getMreal(LD_PI))){printf("\nERROR: Failed to add PI.");return false;}
			if(!setValueOfRealVariable(addVariable(pMenvironment,"E",VT_REAL),getMreal(LD_E))){printf("\nERROR: Failed to add E.");return false;} 
			///// which is: 2.71828182845904523536)); // MDH@24APR2019: this is an approximation but the next decimal digits is a 0 as in 0287471352662497757247 (before the next 0)
			// we're going to store all commands in a list called M
			if(!addVariable(pMenvironment,"M",VT_LIST))return false;
		}
		return true;
	}
	return false;
}

// user interaction stuff
// terminal input stuff
#include <termios.h>
struct termios orig_termios;
bool rawMode=false;
void disableRawMode(){
	rawMode=false;
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}
void endOfUserInput(); // prototype
void enableRawMode(){
	rawMode=true;
	tcgetattr(STDIN_FILENO,&orig_termios);
	atexit(endOfUserInput); // or std::atexit() in C++
	struct termios raw=orig_termios;

  	// ISIG turns off Ctrl-C and Ctrl-Z
	raw.c_lflag&=~(ECHO|ICANON|ISIG); // we kill echoing so we can first look at what we received!!
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
void output(const char *fmt,...){va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);} // NOTE use vprintf here, NOT printf!!!!
// convenience methods delegating to output() so all output (to stdout by default) goes through function output()
void outputChar(char c){output("%c",c);} // MDH@18APR2019: individual characters can use outputChar (which might have used putchar)

// all output to the display has to go through output!!
#define ES "\033["
void outputControlText(char* s){output(ES"%s",s);}
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

void outputVariables(){
	// we're going to write all the variables and their values in the M environment
	// this means iterating over the variables in the environment
	Mmap* variableMap=pMenvironment->variableMap;
	Mmapelement* variable=variableMap->first;
	Mvalue* pMvalue;
	Minteger* pMinteger;
	Mreal* pMreal;
	Mstring* pMstring;
	Mlist* pMlist;
	Mmap* pMmap;
	long long ll;
	long double ld;
	output("\n%s:","Variables");
	while(variable){
		output(" %s",variable->name);
		pMvalue=variable->mValue;
		if(pMvalue){ // we're got a value to write
			////outputChar(':');output("%i",pMvalue->type);outputChar('=');
			switch(pMvalue->type){
				case VT_INTEGER:
					ll=pMvalue->value.i->ll;
					output(":%lu=%ll",sizeof(ll),ll);
					break;
				case VT_REAL:
					ld=pMvalue->value.r->ld;
					output("=%.*Lf",LDBL_DIG,ld); // TODO how to determine the number of significant decimal digits (of my long double??)
					break;
				case VT_STRING:
					pMstring=pMvalue->value.s;
					output("=%c%s%c",pMstring->presuffix,string(pMstring->m),pMstring->presuffix); // wrap in single quotes (unless we put the quotes around the text as well, or we store the prefix/postfix char separately in Mstring)
					break;
				case VT_MAP:
					pMmap=pMvalue->value.m;
					outputChar('=');
					outputChar('{');
					// TODO write map elements
					outputChar('}');
					break;
				case VT_LIST:
					pMlist=pMvalue->value.l;
					outputChar('=');
					outputChar('[');
					// TODO write list elements
					outputChar(']');
					break;
			}
		}
		variable=variable->next;
	}
}

// Edit flags
bool accepthistorycommand=true; // whether to immediately accept a history command
bool matchparentheses=true;  // by default will 'match' parentheses

#ifdef __DEBUG__
bool assisting=true; // assist flag can be turned on to guide the user
bool debugging=true; // program debugging flag so it will show the token information before evaluation of a command
#else
bool assisting=false; // assist flag can be turned on to guide the user
bool debugging=false; // program debugging flag so it will show the token information before evaluation of a command
#endif

enum INPUTMODE_ENUM {IM_COMMAND,IM_CONTROL,IM_SHELL}; // the possible input modes: command, control, and shell

enum INPUTMODE_ENUM inputMode=IM_COMMAND; // whether or not in command mode

char* promptinfo[]={"Command mode: cancel the input text with Ctrl-C.","Control mode: Flags: Assist|color scheme (0 or 1)|Debug|Match parentheses|Wrap|Use history command - Options: eXit|History|Shell.","Shell mode: enter a system command to execute."};
/**
call prompt() when ready to receive a new command
 */
const char OPTION_CHAR='`'; // TODO should this character become part of options????

// USER INPUT STUFF
char inputChar,inputCharType; // the last read input character and its associated type (which we can set to o to escape to control mode!!)
int inputCharRead(){
	if(!rawMode)enableRawMode();
	if(read(STDIN_FILENO,&inputChar,1)==1){
		//printf("{%i}",inputChar);
		return 1;
	}
	return 0;
}

// MDH@16APR2019: let's define the standard colors and high-itensity colors which are dark and light versions
const char BLACK[]="0";
const char DARK_RED[]="1";
const char DARK_GREEN[]="2";
const char DARK_YELLOW[]="3";
const char DARK_BLUE[]="4";
const char DARK_PURPLE[]="5";
const char DARK_CYAN[]="6";
const char DARK_GREY[]="7";
const char LIGHT_GREY[]="8";
const char LIGHT_RED[]="9";
const char LIGHT_GREEN[]="10";
const char LIGHT_YELLOW[]="11";
const char LIGHT_BLUE[]="45"; // "12" is really TOO dark!!
const char LIGHT_PURPLE[]="13";
const char LIGHT_CYAN[]="14";
const char WHITE[]="15";
const char ORANGE[]="202"; // instead of DARK_YELLOW use (a dark version of) ORANGE
// the background colors (which are not used behind 38;5 or 48;5 but directly )
const char BACKGROUND_BLACK[]="40";
const char BACKGROUND_WHITE[]="47";

#define NUMBER_OF_COLOR_SCHEMES 2

// colors
const char* BACKGROUND_COLORS[NUMBER_OF_COLOR_SCHEMES]={BACKGROUND_BLACK,BACKGROUND_WHITE}; // assuming either a black or white background

const char* DEBUG_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY}; // light gray
const char* INFO_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK}; // black

const char* COMMENT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY}; // light gray
const char* ERROR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_RED,LIGHT_RED}; // red

const char* ASSIGNMENT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* UNARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* BINARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};
const char* TERNARY_OPERATOR_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_PURPLE,DARK_PURPLE};

const char* EXPRESSION_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK}; // black
const char* VARIABLE_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_BLUE,DARK_BLUE}; ////"13"; // magenta
const char* FUNCTION_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_CYAN,DARK_CYAN}; /////"93"; // something more blueish
const char* LIST_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
const char* MAP_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
const char* NUMBER_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREEN,DARK_GREEN}; //"22"; // green
const char* STRING_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_YELLOW,ORANGE}; //////12"; // blue

const char* RESULT_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK};
///////const char* OPTION_COLORS[]={BLACK,WHITE};
const char* PROMPT_COLORS[NUMBER_OF_COLOR_SCHEMES]={WHITE,BLACK}; // same as the info color

const char* BEHIND_CURSOR_TEXT_COLORS[NUMBER_OF_COLOR_SCHEMES]={LIGHT_GREY,DARK_GREY};

// operator token colors (all the same)
const char** OPERATOR_TOKEN_COLORS[]={ASSIGNMENT_COLORS,UNARY_OPERATOR_COLORS,BINARY_OPERATOR_COLORS,TERNARY_OPERATOR_COLORS};

// value token colors
const char** VALUE_TOKEN_COLORS[]={EXPRESSION_COLORS,VARIABLE_COLORS,LIST_COLORS,NUMBER_COLORS,NUMBER_COLORS,STRING_COLORS,STRING_COLORS,STRING_COLORS,STRING_COLORS,LIST_COLORS,LIST_COLORS,MAP_COLORS,MAP_COLORS,MAP_COLORS,FUNCTION_COLORS,FUNCTION_COLORS,FUNCTION_COLORS};

#define ESCAPE_CHARACTER 27

void setColor(const char* colortext){output("\033[38;5;%sm",colortext);}
void setBackColor(const char* colortext){output("\033[48;5%sm",colortext);}

void oneLineUp(){outputControlText("1A");} // ascertain that the previous line is visible
void oneLineDown(){outputControlText("1B");} // one line down
void toStartOfLine(){outputChar('\r');}
void clearLine(){outputControlText("K");}
void clearDisplay(){outputControlText("2J");}
void moveCursorLeft(uint16_t pos){if(pos)output(ES"%huD",pos);} // TODO can't use outputControlText here!!!
void moveCursorRight(uint16_t pos){if(pos)output(ES"%huC",pos);} // TODO can't use outputControlText here!!!
void clearScreenFromCursor(){outputControlText("J");}
void beep(){outputChar('\a');}
void removeLastCharacter(){outputChar('\b');}
void hidecursor(){outputControlText("?25l");}
void showcursor(){outputControlText("?25h");}
void emptyline(){outputControlText("2K\r");}
void backspace(){outputControlText("D"); /* go left one character */ outputControlText("K"); /* clear the rest of the line */}

/* TODO are we using the storeCursor() and restoreCursor() sometime?
// VT100 codes...
void storeCursor(){printf("\0337");}
void restoreCursor(){printf("\0338");}
*/

// display flags
uint8_t colorScheme=0; // the active color scheme (either 0 for white, or 1 for black background), toggle with C in control mode
void setColorScheme(uint8_t newColorScheme){
	colorScheme=(newColorScheme%NUMBER_OF_COLOR_SCHEMES);
	output(ES"%sm",BACKGROUND_COLORS[colorScheme]); // TODO can't use outputControlText here!!!
	clearDisplay();
	/////// user will see!!!! output("\n%s\n>> ",(colorScheme?"Will assume dark background!":"Will assume white background!"));
}

void resetOutputColor(){setColor(INFO_COLORS[colorScheme]);setBackColor(BACKGROUND_COLORS[colorScheme]);}
void outputLine(char* s){resetOutputColor();output("\n%s",s);} // for writing a single line of output text in the info color

// M settings
bool wrapMode=true; // by default use 'wrap' mode, not 'Origin' mode
void setWrapMode(bool newWrapMode){
	wrapMode=newWrapMode;
	outputControlText(wrapMode?"?6l":"?7l"); // 'Origin' mode (not 'wrap' mode) in 132 columns (if possible)
	if(wrapMode)output("\nWrap mode enabled!\n");else output("\nWrap mode disabled!\n");
}
void displayFlags(){output("\nEdit flags: %c%c%c%c - Display flags: %c%c.",assisting?'A':'a',debugging?'D':'d',matchparentheses?'M':'m',accepthistorycommand?'U':'u',wrapMode?'W':'w',48+colorScheme);}

void outputFlags(){output("%c%c%c%c%c%c",assisting?'A':'a',(48+colorScheme),debugging?'D':'d',matchparentheses?'M':'m',wrapMode?'W':'w',accepthistorycommand?'U':'u');}

void initDisplay(){
	outputControlText("=3h"); // 80x25 color mode
	outputControlText("?3l"); // switch to 132 column mode (if possible)
	outputControlText("0m");
	setColorScheme(colorScheme); // will also clear the display screen
	setWrapMode(wrapMode);
}

// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
uint32_t commandCount=0; // the total number of command input
uint32_t commandIndex=0;

mstring* behindCursorText=NULL; // MDH@27FEB2019: we keep track of the characters behind the cursor
mstring* shellCommand=NULL;
Token* pCommandToEvaluate=NULL;
Token* pToken=NULL; // the last token in the sequence of tokens starting with pCommandToEvaluate

// keeping track of both the cursor position and the total command length
uint16_t cursorPosition(){return(inputMode==IM_COMMAND?(pToken?pToken->offset+string_length(pToken->text):0):(inputMode==IM_SHELL?string_length(shellCommand):0));}
uint16_t behindCursor(){return string_length(behindCursorText);}
uint16_t commandLength(){return cursorPosition()+behindCursor();}

uint8_t promptLength=0;
void prompt(){
	resetOutputColor();
	///////////printf("%d-",commandIndex);
	char str[11]; // with a maximum of 2,xxx,xxx,xxx 11 positions would suffice
	switch(inputMode){
		case IM_COMMAND:
			sprintf(str,"%u",(commandCount+1));	// replacing: printf("%lu",(commandCount+1));
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
	if(!rawMode)enableRawMode();
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
const uint8_t TOKENTYPE_IDS[]={0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0b1000000,0b11111111};

void outputTokenColor(Token* pToken){
	///////printf("[%d]",pToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
	setBackColor(BACKGROUND_COLORS[colorScheme]);
	uint8_t tokentype_id=TOKENTYPE_IDS[pToken->type];
	///////printf("(%d)",tokentype_id);
	switch(tokentype_id>>6){
		case 0: // value token
			setColor(VALUE_TOKEN_COLORS[tokentype_id][colorScheme]);
			break;
		case 1: // operator: unary, binary, ternary, assignment
			{
			uint8_t opCategory=(tokentype_id&0x30)>>4;
			/////////printf("(%d,%d)",tokentype_id,opCategory);
			setColor(OPERATOR_TOKEN_COLORS[opCategory][colorScheme]);
			break;
			}
		case 2: // comment or end of comment
			setColor(COMMENT_COLORS[colorScheme]);
			break;
		case 3: // error token
			/////////outputChar('E');
			setColor(ERROR_COLORS[colorScheme]);
			break;
	}
}
void outputToken(Token* pToken){
	outputTokenColor(pToken);
	// if we allow comments in tokens we're in trouble!!!
	output("%s",string(pToken->text));
	/////////if(assisting){resetOutputColor();outputChar('|');}
}
void outputLastTokenChar(Token* pToken){
	///////outputTokenColor(pToken);
	outputChar(string_last_char(pToken->text));
	//////////resetOutputColor();
}
/**
 * freeToken() frees the memory @pToken points to and returns true on successfully removing the entire chain of tokens it points to
 * @returns the previous token (as we need that )  
 */
Token* freeToken(Token* pToken){
	if(!pToken)return NULL;
	free(pToken->text); // free the (mstring) text (every new token should have a text it points to)
	freeToken(pToken->next); // free all this token points to
	Token* pPrevToken=pToken->prev; // remember what to return
	free(pToken);
	return pPrevToken;
}

// output functions that require access to the current token
void toStartOfPreviousLine(){oneLineUp();toStartOfLine();clearLine();toStartOfLine();}
void toStartOfNextLine(){oneLineDown();toStartOfLine();}
void toCursorPosition(){
	moveCursorRight(promptLength+cursorPosition());
	if(pToken)outputTokenColor(pToken); // return to the current token color
}

void outputInfo(const char* fmt,...){
	if(strlen(fmt)){ // we have a format
		toStartOfPreviousLine();resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toStartOfNextLine();toCursorPosition();
	}
}
void outputError(char* error){
	if(!strlen(error))return;
	toStartOfPreviousLine();setColor(ERROR_COLORS[colorScheme]);setBackColor(BACKGROUND_COLORS[colorScheme]);output("%s",error);toStartOfNextLine();toCursorPosition();
}
void clearInfo(){toStartOfPreviousLine();resetOutputColor();clearLine();toStartOfNextLine();toCursorPosition();}

void outputStatus(char inputChar,char inputCharType){
	////////printf("[%u,%u]",cursorPosition(),commandLength());
	debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
	if(debugging)
		outputInfo("Input character: %c(=0x%x) | Input character type: %c | Token type: %u | Cursor position: %" PRIu16 " | Command length: %" PRIu16 " | Behind cursor text: '%s'.",inputChar,inputChar,inputCharType,(pToken!=NULL?pToken->type:255),cursorPosition(),commandLength(),string(behindCursorText));
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition(),commandLength(),string(behindCursorText));
}

Token* newToken(Token* prevToken){
	Token* pNewToken=malloc(sizeof(Token));
	if(prevToken!=NULL){
		prevToken->next=pNewToken; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
		if(prevToken->significantCharacterCount==0)prevToken->significantCharacterCount=string_length(prevToken->text); // MDH@22MAR2019: if the token character length is NOT set, set it now...
	}
	if(pNewToken!=NULL){
		pNewToken->expr=(prevToken!=NULL?prevToken->expr:NULL); // copy the pointer to the expression this token is part of
		pNewToken->offset=(prevToken!=NULL?prevToken->offset+string_length(prevToken->text):0);
		pNewToken->prev=prevToken;
		pNewToken->type=TT_EXPRESSION; // makes more sense to start as expression (same as what we get after a ( or [
		pNewToken->significantCharacterCount=0; // MDH@22MAR2019: remembers the amount of significant characters (to be set when the token ends)
		pNewToken->text=string_create();
		pNewToken->next=NULL;
	}
	return pNewToken;
}

// keep track of all commands so far
#define COMMAND_BLOCKSIZE 8
Token** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(){
	if(!pCommandToEvaluate)return false;
	if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
		// I have to copy all first token pointers to a new array large enough
		commandBlocks++;
		Token** newCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Token*));
		if(newCommands==NULL)return false;
        commands=newCommands;
	}
	commands[commandCount++]=pCommandToEvaluate;
	return true;
}

static const char* TOKENTYPE_STRING[]={
	FOREACH_TOKENTYPE(GENERATE_STRING)
};

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
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","-+!","`D","`S","","","","","","","","",""};

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
 "UNA","A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","EXPR","VAR" ,"L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}="}, /* ONE CHARACTER UNARY !-+ */ \
{"!-+","" ,"="    ,""     ,""     ,""      ,"%"    ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C    )&*  , >?:    ] }" }, /* ASSIGNMENT = */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  %()&*  , >?:    ]{}="}, /* Baeru finished bin.op. */ \
{""   ,"" ,"="    ,""     ,""     ,""      ,""     ,""     ,""    ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@;!CDS%()&*+-,.>?:LEN[]{}" }, /* BaErU unfinished bin.op. */ \
{"!-+","=",""     ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"  ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; CDS% )&*  , >?:    ]{}" }, /* BAeRu assignable repeatable */ \
{"!-+","" ,"="    ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* BaERu comp. (<>) bin.op. */ \
{"!-+","=",""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* BAeru assignable bin.op. */ \
{"!-+","=",""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  , >?:    ]{}" }, /* Taeru ternary op. (? only now) */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,",("  ,"LE"  ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*   .>?:    ] }="}, /* EXPRESSION */ \
{""   ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,","   ,"LEN.","["   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@;  DS (               {"  }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`@; C  % )&*  ,.>?:     {}="}, /* LIST ELEMENT (similar to expression) */ \
{""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,",;"  ,""    ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (          L  [ {"  }, /* INTEGER: (signless) list of digits */ \
{""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,",;"  ,""    ,""    ,""   ,"N"   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS (      .   L  [ {"  }, /* REAL: part behind a decimal period */ \
{""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""    ,""    ,""   ,""    ,""        ,""        ,"D"      ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* DQSTRING: double quoted string */ \
{""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,"S"      ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* SQSTRING: single quoted string */ \
{""   ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,",;"  ,""    ,""    ,""   ,""    ,"D"       ,"S"       ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{""   ,"" ,"+"    ,""     ,"&"    ,">"     ,""     ,"?"    ,",;"  ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@ ! DS%&( * - .   LEN[ {"  }, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,",("  ,"LE"  ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*   .>?:      }="}, /* LIST: [ starts a list */ \
{""   ,"=","?"    ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,",;"  ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .  :LEN  {"  }, /* END_OF_LIST: behind ] that ends a list */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,"}"    ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  ,.>?:    ]{ ="}, /* MAP: { starts a map */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %& )*  , >?:    ] }="}, /* MAP_VALUE: : starts a map value */ \
{""   ,"" ,"?"    ,"!="   ,"&*"   ,">"     ,"+"    ,"?"    ,",;"  ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS% (   - .  :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""   ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"  ,""    ,"N"  ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,"("     ,""      ,""  ,"`@;!CDS%& )*+-,.>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"!-+","" ,""     ,""     ,""     ,""      ,""     ,""     ,"("   ,"LE"  ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"`@; C  %&  *  ,.>?:    ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{""   ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%"  ,"?"    ,",;"  ,""    ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`@   DS  (     .   LEN  {"  }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
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
bool continuesOperator(Token* pToken,char inputChar){
	unsigned int l=string_get_length(pToken->text);
	if(inputChar==ASSIGNMENT_CHARACTER){ // appending the 'assignment' operator
		return(l==1||string_get_last_char(pToken->text)!=ASSIGNMENT_CHARACTER);
	}else{
		if(l>1)return false; // cannot continue a two-character operator
		// if subtype is assumed to represent the index into the first operator character set
		unsigned int operatorType=pToken->type.subtype;
		return(operatorType<=6&&strchr(SECOND_OPERATOR_CHARACTERS[operatorType],inputChar)!=NULL);
	}
}
*/
// keep track of the state of entering a command
void removeToken(){		
	// ASSERT pToken should NOT be NULL and empty (i.e. empty tokens should be removed!!!)
	// NOTE if we call freeToken() to free this token all forwardly connected tokens are also freed, so pPrevToken->next should become NULL
	pToken=freeToken(pToken); // pToken now equals its own previous token!!
	if(pToken)pToken->next=NULL;else pCommandToEvaluate=NULL;
}
void unfinishToken(){
	// for all non-unary token that we are in now that is finished, unfinish it!!
	if(pToken->type!=TT_UNARY) // not a unary operator (of length 1) we ended up in
		if(string_length(pToken->text)==pToken->significantCharacterCount) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
			pToken->significantCharacterCount=0;
}
char removedTokenCharacter(uint16_t behindCursor){
#ifdef __DEBUG__
		printf("%d",behindCursor);
#endif
	uint16_t tokenCharacterPosition;
	// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
	while(true){
		if(pToken==NULL)return '\0';
		tokenCharacterPosition=string_length(pToken->text); // MDH@24APR2019 replacing (what is essentially the same): cursorPosition()-pToken->offset;
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition);
#endif
		if(tokenCharacterPosition>=behindCursor)break;
#ifdef __DEBUG__
		outputChar('.');
#endif		
		pToken=pToken->prev;
	}
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition-behindCursor);
#endif	
	// if failing to remove the character serious error
	char c=string_removed_char(pToken->text,tokenCharacterPosition-behindCursor);
#ifdef __DEBUG__
		outputChar(c);
#endif		
	if(c){
		if(string_empty(pToken->text))removeToken(); // text now empty, remove the token entirely...
		unfinishToken();
	}
	return c;
}

// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(){
	// 1. if no command nothing evaluated TODO don't call when this is the case though
	if(pCommandToEvaluate==NULL){outputError("Nothing to evaluate!");return false;}
	
	// 2. if the last token is an error, can't evaluate (well, better not)
	// TODO it makes sense to remove the error token
	if(pToken->type==TT_ERROR){outputError("Can't evaluate erroneous command.");removeToken();unfinishToken();return false;}

	// if the last token is a comment, remove it before further evaluation TODO should we unfinish the token??????
	if(pToken->type==TT_COMMENT)removeToken();

	// 3. if the last token is an operator of sorts the command is incomplete
	if(pToken->type<=8){outputError("Value behind operator at end of command missing.");return false;}

	// 4. can't end with function of function call
	if(pToken->type==TT_FUNCTION){outputError("Function call missing at end of command.");return false;}
	if(pToken->type==TT_FUNCTION_CALL){outputError("Unfinished function call.");return false;}
	if(pToken->type==TT_LIST||pToken->type==TT_LISTELEMENT){outputError("Unfinished list.");return false;}
	if(pToken->type==TT_DQSTRING||pToken->type==TT_SQSTRING){outputError("Unfinished string literal.");return false;}
	if(pToken->type==TT_EXPRESSION){outputError("Unfinished expression.");return false;}
	if(pToken->type==TT_MAP||pToken->type==TT_MAP_VALUE){outputError("Unfinished map.");return false;}

	printf("\nEvaluating '");
	Token* pCommandToken=pCommandToEvaluate; // TODO can we get rid of using commandcount-1 here????
	while(pCommandToken){
		// if we bump into a comment we're done!!!
		if(pCommandToken->type==TT_COMMENT)break;
/*
#ifdef __DEBUG__
		printf("{%p}",pCommandToEvaluateToken);
#endif
*/
		output("%s",string(pCommandToken->text));
		if(assisting)output("(%s) ",TOKENTYPE_STRING[pCommandToken->type]);
		pCommandToken=pCommandToken->next;
	}
	printf("'");
	return true;
}

void prepareForUserInput(){
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	setbuf(stdout,NULL);
	initDisplay();
}

void endOfUserInput(){
#ifdef __DEBUG__
	printf("\nEnd of user input.");
#endif
	// return to the 'right' colors
	resetOutputColor();
	output("\n\n%s\n\n","Thanks for using M.");
	if(rawMode)disableRawMode();
}

// MDH@24APR2019: writeCommand() writes the command to evaluate, and sets pToken in the process
void writeCommand(){Token* token=pCommandToEvaluate;while(token){outputToken(pToken=token);token=token->next;}}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t newCommandPage){
	commandPage=newCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();
		output("\n%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		Token* token=commands[lastCommandToShowIndex+commandToShowIndex];
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
			uint16_t tokenPosition=cursorPosition()-pToken->offset;
			if(tokenPosition)string_append(restOfCommand,string_remainder(pToken->text,tokenPosition));
			string_setlength(pToken->text,tokenPosition); // the new length of the token (cutting off what's behind it)
			// now to append the text in the rest of the tokens
			Token* token=pToken->next;
			if(token!=NULL){
				while(token!=NULL){string_append(restOfCommand,string(pToken->text));token=token->next;}
				freeToken(token); // we'll free all the token starting at the successor of pToken
				pToken->next=NULL;
			}
			outputInfo("Rest of command: '%s'.",string(restOfCommand));
			return string(restOfCommand);
		}
	}
	return NULL;
} 
*/
/*
void writeRestOfCommand(){ // writes rest of command assuming pToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=commandLength()-cursorPosition();
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(cursorPosition()>pToken->offset){ // part of current token to write
			outputTokenColor(pToken);printf("%s",string_remainder(pToken->text,cursorPosition()-pToken->offset));
		}
		// write the rest of the tokens
		writeTokens(pToken->next);
		moveCursorLeft(leftToWrite);
	}
}
*/
bool clearCommand(){
	bool result=freeToken(pCommandToEvaluate);
	pToken=pCommandToEvaluate=NULL;
	return result;
}

void switchToControlMode(char* message){
	if(inputMode==IM_CONTROL)return; // already in control mode thank you
	/////////outputChar('X');
	if(inputMode==IM_COMMAND)clearCommand();
	resetOutputColor();
	if(message!=NULL)printf("\n%s",message);
	inputMode=IM_CONTROL;
	///////////outputFlags(); // show the user the current flags!!
	inputCharType='o'; // to make the loop know to quit
	//output("\n%s\n >> ","Control mode: Flags: Assist Debug - Options: eXit History Shell");
}

void writeBehindCursorText(bool clearAfterBehindCursorText){
	uint16_t l=string_length(behindCursorText);
	if(l||clearAfterBehindCursorText){
		debugWrite("Behind cursor text to write: '%s'.",string(behindCursorText));
		resetOutputColor();
		setColor(BEHIND_CURSOR_TEXT_COLORS[colorScheme]);
		if(l)output("%s",string(behindCursorText));
		if(clearAfterBehindCursorText){
			outputChar(' ');
			moveCursorLeft(l+1);
		}else
		if(l)
			moveCursorLeft(l); // back to where we started to write the behind cursor text
		if(pToken)outputTokenColor(pToken); // return to the color of the current token
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

void setCommandToEvaluate(Token* pCommand){
	pToken=pCommandToEvaluate=pCommand;
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
		Token* token=commands[commandCount-commandIndex];
		// MDH@19APR2019: if not to accept the history command we use the previous command as behind cursor text
		if(accepthistorycommand){ // use the history command as autocompletion text instead of accepting it immediately as command!!!
			setCommandToEvaluate(token);
			outputInfo("Showing registered command #%u.",(commandCount-commandIndex+1));
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
	pToken=pCommandToEvaluate=newToken(NULL);
}

void copyCommand(){
	// if fails to copy pCommandToEvaluate pToken should end up as NULL
	pToken=NULL;
	Token* pTokenToCopy=pCommandToEvaluate;
	pCommandToEvaluate=NULL;
	// the essence is that pToken points to the last token in pCommandToEvaluate
	Token* pCommandCopy=NULL;
	// NOTE theoretically pToken could be NULL due to newToken() failing to create a new token
	while(pTokenToCopy){
		pToken=newToken(pToken);
		pToken->type=pTokenToCopy->type;
		pToken->significantCharacterCount=pTokenToCopy->significantCharacterCount;
		// if failing to copy the text over get rid of the command constructed so far, and break
		if(!string_copy(pTokenToCopy->text,pToken->text)){pToken=NULL;break;}
		// MDH@24APR2019 obsolete: commandLength()+=string_length(pToken->text);
		// some additional fields to copy over (NOT the offset is that is set automatically)
#ifdef __DEBUG__
        printf("%d:%s",pToken->type,string(pToken->text));
#endif
		if(!pCommandToEvaluate)pCommandToEvaluate=pToken;
		// get the next token to copy...
		pTokenToCopy=pTokenToCopy->next;
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
Token* getCommand(){
	return(commandIndex&&cursorPosition()?commands[commandCount-commandIndex]:pCommandToEvaluate);
}
void echoCommand(){
	Token* token=pCommandToEvaluate;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
void setCommand(Token* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. cursorPosition()==0 and pCommandToEvaluate==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(pCommandToEvaluate){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	newCommandToEvaluate(); // NOTE might fail, in which case pToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set pToken!!!
		Token* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically pToken could be NULL due to newToken() failing to create a new token
		while(pToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!string_copy(pNewToken->text,pToken->text)){clearCommand();break;}
			// MDH@24APR2019 obsolete: commandLength()+=string_length(pToken->text);
			// some additional fields to copy over (NOT the offset is that is set automatically)
			pToken->type=pNewToken->type;
#ifdef __DEBUG__
            printf("%d:%s",pToken->type,string(pToken->text));
#endif
			pNewToken=pNewToken->next;
			if(!pNewToken)break;
			// we're going to need another token!!!
			pToken=newToken(pToken);
		}
		// if the user decides to start typing ascertain to show it in the right color!!
		if(pToken)outputTokenColor(pToken);
#ifdef __DEBUG__
		echoCommand();
#endif
	}
}
*/

// in response to backspace the previous token character is to be removed
void removePreviousTokenCharacter(){ // NOTE always due to a backspace!
	char removedCharacter=removedTokenCharacter(1);
	if(removedCharacter){
		// MDH@24APR2019 obsolete: commandLength()--; // decrement the total command length
		if(cursorPosition()==0)pCommandToEvaluate=NULL;
		// on screen as well please
		moveCursorLeft(1); // will decrement cursorPosition() // MDH@24APR2019: NOT anymore...
		clearScreenFromCursor(); // will clear what's behind the cursor
		// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in behindCursorText that's what we should now write
		writeBehindCursorText(false);
		// replacing: if(pCommandToEvaluate)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
	}else
		switchToControlMode("Failed to remove the character in response to pressing the backspace key.");
}

void outputTokenInfo(){
	Token* token=pCommandToEvaluate;
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
//       		  ASSERTION pCommandToEvaluate and pToken are  NOT  NULL
//                the endofinput flag is used to indicate whether this is the end of the input
bool commandCharacterAccepted(char inputChar,char inputCharacterType,bool endOfInput){
	// MDH@21APR2019: there are two situation where we need to get a command
	//                1. we haven't got one 2. we have got a registered command which hasn't changed yet (in which case commandIndex will still be positive)
	if(!pCommandToEvaluate) // no current command
		newCommand(); // we need to make a new token (to start the command to evaluate)
	else // we have a current command BUT 
	if(commandIndex)
		copyCommand();
	// if pToken is now NULL something went wrong (in copyCommand or newCommand most likely)
	if(pToken==NULL)return false;
	commandIndex=0; // to indicate we are now working with a NEW command (even if we fail to accept the character!!!)
	clearInfo(); // TODO make a separate function to do this???

	/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
	if(inputCharType=='C'){
		pToken->type^=0x70; // toggling bit 7
		// a comment character will NEVER change the (actual) token type but it should change the color to use
		if(pToken->type&0x70){commenting=true;outputTokenColor(pToken);}else notCommenting=true; // if a comment was started, switch to the comment token color
	}else // not a comment character
	if((pToken->type&0x70)==0){ // not in a comment
		if(notCommenting){notCommenting=false;outputTokenColor(pToken);} // if behind coming out of a comment, we have to reset the output token color
	*/
	/*
	// MDH@26FEB2019: when a user starts inserting characters instead of appending them we can cut off the rest of the characters in the command
	//                and put it in a single mstring instance and append these one at a time 
	char* removed=removedRestOfCommand();
	*/
	// determine the token type associated with the newly inputted character
	// MDH@28MAR2019: if we're in a binary token type with the repeatable flag set AND the user has repeated the previous first token character the inputCharacterType should become R to get the right transition
	if((TOKENTYPE_IDS[pToken->type]&0x62)==0x62)if(inputChar==string_char(pToken->text,0))inputCharacterType='R';
	// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
	if(inputCharacterType=='W')if(pToken->type==TT_ERROR||pToken->type==TT_COMMENT||pToken->type==TT_DQSTRING||pToken->type==TT_SQSTRING)inputCharacterType='w';
	if(inputCharacterType!='W'){ // only characters that are not whitespace can start a new token
		int16_t newTokenType=nextTokenType(pToken->type,inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
#ifdef __DEBUG__
	resetOutputColor();
	printf("[%d+%c->%d]",pToken->type,inputCharacterType,newTokenType);
	outputTokenColor(pToken);
#endif
		// some combinations are (still) not allowed...
		if(newTokenType==pToken->type){
			// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators
			if(pToken->type!=TT_UNARY&&pToken->type!=TT_TERNARY_aeru&&pToken->significantCharacterCount>0)newTokenType=TT_ERROR;
		}else{ // different token types
			// a shortcut assignment can NOT be turned into a equality comparison
			if(inputCharacterType=='='&&pToken->type==TT_ASSIGNMENT&&(pToken->prev->type==TT_BINARY_AeRu||pToken->prev->type==TT_BINARY_Aeru))newTokenType=TT_ERROR;
		}
		if(newTokenType!=pToken->type||pToken->significantCharacterCount>0){
			// MDH@10APR2019: NOT every new token type starts a new token:
			//                if we're in a binary operator and move to another binary operator type it's an extension
			//                NO we decide NOT to do this when the command is evaluated we should compose the values and apply the operators
			///////if(!isBinaryOperatorTokenType(pToken->type)||!isBinaryOperatorTokenType(newTokenType))

			// MDH@16APR2019: a character that is assumed to indicate the assignment operator has to be checked because it could well be the = that starts the binary equality operator
			//                which means we have to switch from assignment token to BearU token (which is unfinished)
			if(newTokenType==TT_ASSIGNMENT){
				// checking for validity of accepting as assignment is not that easy
				// we can allow a binary operator in front of the assignment of course in that case it definitely is an assignment if it is not the = is an error!!
				bool behindBinaryOperator=(pToken->type==TT_BINARY_AeRu||pToken->type==TT_BINARY_Aeru);
				// NOTE if behind binary operator there must always be a token in front of it, so pTokenToCheck cannot be NULL!!
				Token* pTokenToCheck=(behindBinaryOperator?pToken->prev:pToken);
				// ASSERT pTokenToCheck should either represent a variable or the end of a list element to allow for operator
				if(pTokenToCheck->type==TT_END_OF_LIST){ // end of a list
					// we have to find the associated start of the list, and the token in front of that (which should be a variable!!!)
					// unfortunately we might come across other list and we need to skip them
					int listCounter=1;
					while(listCounter>0){pTokenToCheck=pTokenToCheck->prev;if(pTokenToCheck==NULL)break;if(pTokenToCheck->type==TT_END_OF_LIST)listCounter++;else if(pTokenToCheck->type==TT_LIST||pTokenToCheck->type==TT_LISTELEMENT)listCounter--;}
				}
				// two options: = behind a binary operator without variable (or list element) in front of it is not allowed, i.e. an error, otherwise we assume that = represents the first = of == the equality operator...
				if(pTokenToCheck==NULL||(pTokenToCheck->type!=TT_VARIABLE&&pTokenToCheck->type!=TT_LISTELEMENT))newTokenType=(behindBinaryOperator?TT_ERROR:TT_BINARY_aErU);
			}

			pToken=newToken(pToken);
/*
#ifdef __DEBUG__
			printf("@%p=%p?:%s",pCommandToEvaluate,pToken,string(pCommandToEvaluate->text));
#endif
*/
			pToken->type=newTokenType;
			if(pToken->type==TT_UNARY)pToken->significantCharacterCount=1;
			// MDH@15APR2019: there are some other characters as well, that immediately end the token like parentheses, comma's and semicolons and ? and : TODO are there more??????
			if(pToken->significantCharacterCount==0)
				if(pToken->type!=TT_ERROR&&pToken->type!=TT_COMMENT&&pToken->type!=TT_DQSTRING&&pToken->type!=TT_SQSTRING)
					if(inputCharacterType=='('||inputCharacterType=='['||inputCharacterType=='{'||inputCharacterType==','||inputCharacterType==';'||inputCharacterType==':'||inputCharacterType=='?')
						pToken->significantCharacterCount=1;
			// TODO should we write the associated colors here?????
			outputTokenColor(pToken);
		}
	}else // a functional whitespace character, ends a current token!!
	if(pToken->significantCharacterCount==0&&pToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
		pToken->significantCharacterCount=string_length(pToken->text);

	// append the typed character at cursorPosition() minus current token offset in pToken->text
	string_append_char(pToken->text,inputChar);
#ifdef __DEBUG__
	printf("[%s]",string(pToken->text));
#endif
	// MDH@24APR2019 obsolete: commandLength()++; // increment total command length
	outputChar(inputChar); ///////// replacing: outputLastTokenChar(pToken); // echo the last token character
	
	if(endOfInput){
		//////if(assisting)output(":%c",inputCharacterType);
		debugWrite("Command length after inserting %c: %" PRIu16 ".",inputChar,commandLength());
	}

	// MDH@24APR2019 obsolete: cursorPosition()++; // increment the current cursor position

	if(endOfInput){
		// MDH@16APR2019: we can check for an unfinished binary operator in which case we should show = behind 
		if(pToken->type==TT_BINARY_aErU){string_insert_char(behindCursorText,0,'=');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
		// MDH@15APR2019: it seems like a good idea to adapt the behind cursor text if we entered the start character of a list (element), map or expression opening parenthesis
		if(matchparentheses){
			if(inputCharacterType=='['){string_insert_char(behindCursorText,0,']');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
			if(inputCharacterType=='{'){string_insert_char(behindCursorText,0,'}');/* MDH@24APR2019 obsolete: commandLength()++;*/}else
			if(inputCharacterType=='('){string_insert_char(behindCursorText,0,')');/* MDH@24APR2019 obsolete: commandLength()++;*/}
		}
		writeBehindCursorText(false);
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
void switchToShellMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)output("\n%s",message);
	inputMode=IM_SHELL;
	clearShellCommand();
	inputCharType='s';
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
					if(argv[arg][i]=='d')debugging=false;else
					if(argv[arg][i]=='D')debugging=true;else
					if(argv[arg][i]=='a')assisting=false;else
					if(argv[arg][i]=='A')assisting=true;else
					if(argv[arg][i]=='m')matchparentheses=false;else
					if(argv[arg][i]=='M')matchparentheses=true;else
					if(argv[arg][i]=='C')colorScheme=0;else // light color scheme
					if(argv[arg][i]=='c')colorScheme=1;else // dark color scheme
					if(argv[arg][i]=='W')wrapMode=true;else
					if(argv[arg][i]=='w')wrapMode=false;
				}
			}
		}
	}

	prepareForUserInput(); // AFTER using the command-line parameters (will effectuate wrap mode and color scheme)

	resetOutputColor(); // just in case
	output("\n%s\n","Welcome to M, the fancy interpreter.");
	output("\n%s","Use Ctrl-Z to exit M immediately at any time.");
	output("\n%s","In any mode press the Enter key on an empty line to switch modes.");
	displayFlags();

	if(!initEnvironment()){ // ascertain to have an execution environment!!!
		setColor(ERROR_COLORS[colorScheme]);setBackColor(BACKGROUND_COLORS[colorScheme]);
		output("\n%s","Exiting, due to failing to initialize the M execution environment!");
		exit(1);
	}

	mstring* predefinedVariableNames=getVariableNames(pMenvironment,", ");
	if(predefinedVariableNames){
		output("\nPredefined variables: %s.",string(predefinedVariableNames));
		free(predefinedVariableNames); // TODO or keep it around?????
	}else
		outputLine("No predefined variables!");
	//////////output("\nNumber of predefined variables: %d.",getNumberOfVariables(mEnvironment));
	
	// initialize commands and input mode
	shellCommand=string_create(); // MDH@12APR2019: allow executing shell commands (calling system())
	behindCursorText=string_create(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	pCommandToEvaluate=NULL; // the current command (token)

	///// writeCommand() will take care of this!!!! commandLength()=0; // keep track of the total command length...
	inputMode=IM_COMMAND; // TODO should this go into promptForUserInput()?

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
		pToken=pCommandToEvaluate;
		// an existing command to show
		while(pToken!=NULL){
			outputToken(pToken);
			// the cursor will move along with every printf()
			cursorPosition()+=string_length(pToken->text);
			pToken=pToken->next;
		}
		*/
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(inputCharRead()){

			if(inputChar>127)continue; // undefined input character

			inputCharType=INPUTCHARACTERTYPES[inputChar];

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
					switchToControlMode(NULL);
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
							switchToControlMode("Failed to remove the first character in the auto-complete text.");	
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
						/////////if(wrapMode)break; // if in wrapmode can't guarantee backspace() to move into the previous line which means just prompt again...
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
								switchToControlMode("Failed to remove the suggested character.");
								break;
							}
							// MDH@24APR2019 obsolete: commandLength()--; // until we manage to insert the character removed, we have one less character in the total command length
							if(!commandCharacterAccepted(newInputChar,INPUTCHARACTERTYPES[newInputChar],bc==0)){
								switchToControlMode("Failed to accept the suggested character.");
								break;
							}
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead()){
						///printf("(%d)",inputChar);
						if(inputChar==91){
							if(inputCharRead()){
								///printf("(%d)",inputChar);
								if(inputChar==51){
									if(inputCharRead()){
										if(inputChar==126){ // delete
											if(string_length(behindCursorText)){
												if(string_removed_char(behindCursorText,0))
													writeBehindCursorText(true);
												else
													switchToControlMode("Failed to remove the first character of the auto-completion text.");	
											}else // nothing under the cursor to delete
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(inputMode==IM_COMMAND){ // i.e. show previous command if any
										if(!commandIndex&&pCommandToEvaluate)
											outputError("Won't show previous commands when one is being entered.");
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
											outputError("Won't show next commands when one is being entered!");
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
												switchToControlMode("Suggested character extracted, but not accepted.");
										}else
											switchToControlMode("Suggested character not extracted!");
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
											if(!string_length(pToken->text)){
												// we can check the offset to see if this is the first token, but pToken->prev is a little more secure
												
												if(pToken->prev){ // NOT the first token

												}else{ // the first token
													clearCommand();
												}
											}
											writeBehindCursorText(false);
										}else
											switchToControlMode("Failed to move the cursor left.");
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
						switchToControlMode(pCommandToEvaluate?"Failed to accept the character.":"Failed to create a new command");
					/*
					// we need to have a token (to append the input character to) which initializes to pCommandToEvaluate
					if(pCommandToEvaluate==NULL) // no first command token
						// if commandIndex we should one of the registered commands
						setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set commandLength()!!!
					// if still NULL (also when we fail to actually create a new first command token)
					if(pToken!=NULL){
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
					// flags
					if(inputChar=='a'||inputChar=='A'){assisting=(inputChar=='A');output("\n%s",(assisting?"Will assist!":"Will not assist!"));inputCharType='n';break;}
					if(inputChar=='d'||inputChar=='D'){debugging=(inputChar=='D');output("\n%s",(debugging?"Will debug!":"Will not debug!"));inputCharType='n';break;}
					if(inputChar=='m'||inputChar=='M'){matchparentheses=(inputChar=='M');output("\n%s",(matchparentheses?"Will match parentheses!":"Will not match parentheses!"));inputCharType='n';break;}
					if(inputChar=='u'||inputChar=='U'){accepthistorycommand=(inputChar=='U');output("\n%s",(accepthistorycommand?"Will use history command immediately!":"Will use history command in auto-completion!"));inputCharType='n';break;}
					if(inputChar>='0'&&inputChar<='9'){setColorScheme(inputChar-'0');inputCharType='n';break;}
					if(inputChar=='w'||inputChar=='W'){setWrapMode(inputChar=='W');inputCharType='n';break;}
					// options
					if(inputChar=='x'||inputChar=='X'){inputCharType='x';break;}
					if(inputChar=='s'||inputChar=='S')switchToShellMode(NULL);
					if(inputChar=='h'||inputChar=='H'){
						// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
						// we could call them a, b, c etc.
						if(commandCount){
							commandPages=1+(commandCount-1)/10;
							showNextCommandPage(); // as soon as commandPage>0 we are paging...
						}else
							output("%s\n","No previous commands to show.");
					}
					if(inputChar=='v'||inputChar=='V'){
						outputVariables();
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
							switchToControlMode("Failed to remove the shell command character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='d'){
					if(string_length(behindCursorText)){ // something behind the cursor that we can remove
						if(string_removed_char(behindCursorText,0))
							writeBehindCursorText(true);
						else
							switchToControlMode("Failed to remove the first autocompletion character!");
					}else // nothing to remove
						beep();
				}else
				if(inputCharType=='c'){ // cancel command (Ctrl-C)
					if(pCommandToEvaluate!=NULL){
						backToPrompt();
						clearShellCommand();
						//////////if(wrapMode)break;
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
								switchToControlMode("Failed to accept all suggested characters.");
								break;
							}
							string_append_char(shellCommand,newInputChar);
						}
					}else
						beep();
				}else
				if(inputCharType=='m'){ // Esc character...
					if(inputCharRead()){
						if(inputChar==91){
							if(inputCharRead()){
								if(inputChar==51){
									if(inputCharRead()){
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
											switchToControlMode("Failed to accept the suggested characters.");
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
											switchToControlMode("Failed to move the cursor left.");
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
				Token* pCommandToEvaluateToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(pCommandToEvaluate){ // a current command being edited
					// MDH@22MAR2019: currently the first token is an EXPRESSION token
					if(cursorPosition()>string_length(pCommandToEvaluate->text)){
						// finish the last token???
						if(pToken->significantCharacterCount==0)pToken->significantCharacterCount=string_length(pToken->text);
						pCommandToEvaluateToEvaluate=pCommandToEvaluate; // but only when not at start of command!!!
						if(debugging)outputTokenInfo();
					}
				}else // no command yet, although we might be looking at a previous command
				if(commandIndex&&cursorPosition()) // NOTE using cursorPosition() is better than using accepthistorycommand (causing it!!)
					pCommandToEvaluateToEvaluate=commands[commandCount-commandIndex];
				*/

				// if we succeeded in evaluating a command we should register it
				if(pCommandToEvaluate!=NULL){
					if(!evaluateCommand()){
						outputLine("Failed to evaluate the command! Please complete, correct or cancel the command.");
						continue;
					}
					string_setlength(behindCursorText,0); // clear the autocompletion text NOTE if we fail to evaluate the command it will not be cleared!!!!
					// if we succeed in registering the command the command tokens should NOT be freed, BUT if we fail to register the command we should free ALL command tokens
					if(!registerCommand()){
						// if commandIndex (>0) we have evaluated a previous command which should also NEVER be freed
						if(commandIndex)
							outputLine("ERROR: Failed to register the command again! Out of memory?");
						else
						if(freeToken(pCommandToEvaluate))
							outputLine("ERROR: Failed to register the command! Out of memory?");
						else
							outputLine("ERROR: Failed to register and remove the command! Out of memory?");
					}
					// start anew (without a current command to evaluate!!!!)
					pToken=pCommandToEvaluate=NULL; // remove reference to current command
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
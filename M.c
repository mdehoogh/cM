// remove the line below when not in debug mode
//#define __DEBUG__

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
//#include <cstdlib>

// to hold mutable strings (in tokens and expressions)
#include "mstring.h"

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

bool assisting=false; // assist flag can be turned on to guide the user

char* promptinfo="\nType ` to enter the menu; cancel the input command with Ctrl-D.\n";
/**
call prompt() when ready to receive a new command
 */
const char OPTION_CHAR='`'; // TODO should this character become part of options????

void prompt(); // prototype of prompt!!
void promptForUserInput(){
	if(!rawMode)enableRawMode();
	printf("%s",promptinfo);
	prompt();
}
/*
// we can take the original implementation (as used in pyM) of Token and Expression
class Token{
public:
	Token(){

	}
	~Token(){

	}
};
class Expression{
public:
	Expression(){

	}
	~Expression(){

	}
};
*/
/*
mstring * expressionUserInputString=NULL;
void evaluateExpression(){
	if(expressionUserInputString==NULL)return;
	// TODO do I need to expose of the result of get_all myself?????
	char * expressionUserInputText=string_get_all(expressionUserInputString);
	if(expressionUserInputText!=NULL){		
		printf("\nEvaluating expression '%s'.",expressionUserInputText);
		// as soon as we're dont with the expression user input text, we dispose it...
		free(expressionUserInputText); // get rid of the user input text...
	}
	string_dispose(expressionUserInputString);
	expressionUserInputString=NULL;
}
*/

// USER INPUT STUFF
bool commandInput; // whether or not in command mode
char inputChar; // the last read input character
int inputCharRead(){
	if(!rawMode)enableRawMode();
	return(read(STDIN_FILENO,&inputChar,1)==1);
}

const char DEBUG_COLOR[]="8"; // light gray
const char INFO_COLOR[]="0"; // black
const char COMMENT_COLOR[]="8"; // light gray
const char ERROR_COLOR[]="9"; // red
const char IDENTIFIER_COLOR[]="202"; // orange for unknown identifiers (although these would be used for assignments)
const char VARIABLE_COLOR[]="13"; // magenta
// FUNCTION_COLOR=93 # something more blueish
const char LITERAL_COLOR[]="22"; // green
const char OPERATOR_COLOR[]="12"; // blue
const char RESULT_COLOR[]="15"; // quite dark
const char OPTION_COLOR[]="15"; // RESULT_COLOR
const char* PROMPT_COLOR=INFO_COLOR; // same as the info color

// the back colors
const char DEBUG_BACKCOLOR[]="255"; // light-gray
const char INFO_BACKCOLOR[]="231"; // white
const char ERROR_BACKCOLOR[]="231";
const char IDENTIFIER_BACKCOLOR[]="231";
const char LITERAL_BACKCOLOR[]="231";
const char OPERATOR_BACKCOLOR[]="231";
const char RESULT_BACKCOLOR[]="69";
const char* OPTION_BACKCOLOR=RESULT_BACKCOLOR;

const char* TOKEN_COLORS[]={ERROR_COLOR,COMMENT_COLOR,COMMENT_COLOR,INFO_COLOR,INFO_COLOR,VARIABLE_COLOR
						  ,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR
						  ,INFO_COLOR,INFO_COLOR,INFO_COLOR
						  ,OPERATOR_COLOR,OPERATOR_COLOR,OPERATOR_COLOR
						  ,OPERATOR_COLOR,INFO_COLOR,INFO_COLOR,INFO_COLOR,INFO_COLOR
						  };
const char* TOKEN_BACKCOLORS[]={ERROR_COLOR,COMMENT_COLOR,COMMENT_COLOR,INFO_COLOR,INFO_COLOR,VARIABLE_COLOR
						  ,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR,LITERAL_COLOR
						  ,INFO_COLOR,INFO_COLOR,INFO_COLOR
						  ,OPERATOR_COLOR,OPERATOR_COLOR,OPERATOR_COLOR
						  ,OPERATOR_COLOR,INFO_COLOR,INFO_COLOR,INFO_COLOR,INFO_COLOR
						  };

#define ESCAPE_CHARACTER 27
						  
void setColor(const char* colortext){printf("\033[38;5;%sm",colortext);}
void setBackColor(const char* colortext){printf("\033[48;5%sm",colortext);}
void oneLineUp(){printf("\033[1A");} // ascertain that the previous line is visible
void oneLineDown(){printf("\033[1B");} // one line down
void toStartOfLine(){putchar('\r');}
void clearLine(){printf("\033[K");}
void moveCursorLeft(uint16_t pos){if(pos)printf("\033[%huD",pos);}
void moveCursorRight(uint16_t pos){if(pos)printf("\033[%huC",pos);}
void clearScreenFromCursor(){printf("\033[J");}
/*
void saveCursor(){printf("\033[s");} // TODO might not work
void restoreCursor(){printf("\033[u");} // TODO might not work
*/
void resetOutputColor(){printf("\033[0m");}
void beep(){putchar('\a');}
void removeLastCharacter(){putchar('\b');}
void hidecursor(){printf("\033[?25l");}
void showcursor(){printf("\033[?25h");}
void emptyline(){printf("\033[2K\r");}
void backspace(){ // means go one position to the left on the current line, and clear the rest of the line
	printf("\033[D"); // go left one character
	printf("\033[K"); // clear the rest of the line
}
// keeping track of the command count, the cursor position and the prompt length (so we can write information messages on the line above where the prompt is)
uint32_t commandCount=0; // the total number of command input
uint32_t commandIndex=0;
// keeping track of both the cursor position and the total command length
uint16_t cursorPosition=0,commandLength=0;

uint8_t promptLength=0;
void prompt(){
	resetOutputColor();
	///////////printf("%d-",commandIndex);
	char str[11]; // with a maximum of 2,xxx,xxx,xxx 11 positions would suffice
	sprintf(str,"%u",(commandCount+1));	// replacing: printf("%lu",(commandCount+1));
	printf("%s%s",str," >> ");
	///////saveCursor();
	clearScreenFromCursor();
	promptLength=strlen(str)+4;
	commandInput=true; // expecting a command (until the option character is received)
	cursorPosition=0; // starting at position 0
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
void outputText(char* text){	
	resetOutputColor(); 
	printf("%s",text);
}
void outputInfo(char* info){
	oneLineUp();
	toStartOfLine();
	clearLine();
	toStartOfLine();
	if(strlen(info))outputText(info);
	oneLineDown();
	toStartOfLine();
	moveCursorRight(promptLength+cursorPosition);
}

/*
typedef struct{
	unsigned int ended:1; // one flag to indicate whether or not the Token has ended
	unsigned int complete:1; // one flag to indicate whether or not the token is complete
	unsigned int type:2; // 00=value, 01=unary operator, 02=binary operator, 03=ternary operator
	unsigned int subtype:4; // what subtype it is, i.e. the type of operator
}TokenType;
*/
typedef struct Token{
	uint8_t /*TokenType*/ type; // actually the index into the TOKENTYPES array!!!
	uint8_t offset; // number of character in front of this token in the command
	mstring* text;
	struct Token* prev; // we need this during user input
	struct Token* next;
}Token;

void outputTokenColor(Token* pToken){	
	setColor(TOKEN_COLORS[pToken->type]);
	setBackColor(TOKEN_BACKCOLORS[pToken->type]);
}
void outputToken(Token* pToken){
	outputTokenColor(pToken);
	printf("%s",string(pToken->text));
	if(assisting){resetOutputColor();putchar('|');}
}
void outputLastTokenChar(Token* pToken){
	///////outputTokenColor(pToken);
	putchar(string_last_char(pToken->text));
	//////////resetOutputColor();
}
/**
 * freeToken() frees the memory @pToken points to and returns true on successfully removing the entire chain of tokens it points to
 * will only return false if failing to actually free the token pointed to!!!
 */
bool freeToken(Token* pToken){
	if(pToken!=NULL){
		// free text and next fields FIRST // NOTE apparently in C there's no need to test for the pointer being NULL as free() will do that for us
		// next() first, because when that feels we still want the text to be around!!
		// if pToken->next is NULL will return true so should be OK in that situation (we don't want to check twice)
		// NOTE that we do not NULL the pointer anywhere, but the structure with the pointer is freed so the next field will not be around anymore!!
		if(!freeToken(pToken->next))return false;
		free(pToken->text);
		free(pToken);
	}
	return true;
}

// MDH@19DEC2018: I want to represent the state transition from the current token type to the next token type
// the list of possible token types
// E=expression,W=whitespace,C=comment
// operators: U=unary operator (always one character),B=binary,b=binary ended,A=assignment,
// symbolic values: V=variable,F=function call,f=end of function call,
// numeric values: I=integer,R=real,E=extended integer/real,F=function(call),f=end of function call,L=list start,l=list end
//	text values: D=double quoted string, d=end of double quoted string,S=single quoted string,s=end of single quoted string
//  list: L=list,l=end of list
// I suppose an expression starts with an E token and ends with an e token; this way we can tell when an expression starts
// we can store [ and , as an E token 

#define NUMBER_OF_TOKEN_TYPES 23
#define FOREACH_TOKENTYPE(TOKENTYPE) \
		TOKENTYPE(TT_ERROR) \
		TOKENTYPE(TT_COMMENT) \
		TOKENTYPE(TT_ENDOFCOMMENT) \
		TOKENTYPE(TT_EXPRESSION) \
		TOKENTYPE(TT_WHITESPACE) \
		TOKENTYPE(TT_VARIABLE) \
		TOKENTYPE(TT_INTEGER) \
		TOKENTYPE(TT_REAL) \
		TOKENTYPE(TT_EREAL) \
		TOKENTYPE(TT_DQSTRING) \
		TOKENTYPE(TT_SQSTRING) \
		TOKENTYPE(TT_END_OF_DQSTRING) \
		TOKENTYPE(TT_END_OF_SQSTRING) \
		TOKENTYPE(TT_LIST) \
		TOKENTYPE(TT_LIST_ELEMENT) \
		TOKENTYPE(TT_END_OF_LIST) \
		TOKENTYPE(TT_UNARY_OPERATOR) \
		TOKENTYPE(TT_OPERATOR) \
		TOKENTYPE(TT_BINARY_OPERATOR) \
		TOKENTYPE(TT_FUNCTION) \
		TOKENTYPE(TT_FUNCTION_CALL) \
		TOKENTYPE(TT_FUNCTION_ARGUMENT) \
		TOKENTYPE(TT_END_OF_FUNCTION_CALL)
#define GENERATE_TOKENTYPE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
enum TOKENTYPE_ENUM {
	FOREACH_TOKENTYPE(GENERATE_TOKENTYPE_ENUM)
};

Token* newToken(Token* prevToken){
	Token* pNewToken=malloc(sizeof(Token));
	if(prevToken!=NULL)prevToken->next=pNewToken; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
	if(pNewToken!=NULL){
		pNewToken->offset=(prevToken!=NULL?prevToken->offset+string_length(prevToken->text):0);
		pNewToken->prev=prevToken;
		pNewToken->type=TT_WHITESPACE; // start with a whitespace token (currently empty!!)
		pNewToken->text=string_create();
		pNewToken->next=NULL;
	}
	return pNewToken;
}

// keep track of all commands so far

#define COMMAND_BLOCKSIZE 8

// keep track of the user input count
Token* pCommand=NULL; // the current command

Token** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(){
	if(!pCommand)return false;
	if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
		// I have to copy all first token pointers to a new array large enough
		commandBlocks++;
		Token** newCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Token*));
		if(newCommands==NULL)return false;
        commands=newCommands;
	}
	commands[commandCount++]=pCommand;
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

//                                -------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
const char INPUTCHARACTERTYPES[]="---c----btn--n------------xm----WUDCLBBS()BO,O.BNNNNNNNNNN:;BAB?@LLLLLLLLLLLLLLLLLLLLLLLLLL[B]BLoLLLLELLLLLLLLLLLLLLLLLLLLL{B}Ud";

// now we define all the state transitions i.e. what input character types result in which new token type
// NOTE this can be organized in many ways perhaps it's easiest to tell per input character what the transformation is
//      only changes to the token type need to be registered, so if the change is NOT present, no need to put it in the transition table
//      EWW means that when starting an expression any whitespace starts a whitespace token, we use * to indicate ALL possible input character types
//      *WW means that any W character received in any state will result in a W state 
// we can make an array of transitions with each element corresponding to the character in TOKENTYPES, so the first entry contains all responses to E, the second entry the responses to W etc.
// it's easier to tell for any possible resulting token type which input character types will result in that type
// it's a hell of a job to create the token type transitions matrix
char* const NO_TRANSITIONS[NUMBER_OF_TOKEN_TYPES]={
	"*","!#","","","W",
	"LEN","N","N","N","!D",
	"!S","","","","",
	"","","","","",
	"","",""
};
const char * const TRANSITIONS[NUMBER_OF_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* ERROR */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* COMMENT */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* ENDOFCOMMENT */ \
	{"BOU  ","C"," "," "," "," ","N"," "," ","","","","","",""," "," "," "," ","","","",""}, /* EXPRESSION */ \
	{"BOU  ","C"," "," "," ","L","N"," "," ","","","","","",""," "," "," "," ","","","",""}, /* WHITESPACE */ \
	{"     ","C"," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* VARIABLE: some identifier not yet recognized as function name */ \
	{"LDS  ","C"," "," "," "," "," ",".","E","","","","","",""," ","U","O","B","","","",""}, /* INTEGER: (signless) list of digits */ \
	{"LDS. ","C"," "," "," "," "," "," ","E","","","","","",""," "," "," "," ","","","",""}, /* REAL: part behind a decimal period */ \
	{"LDS.E","C"," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* EREAL part behind character 'e' in integer or real (only digits allowed) */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* DQSTRING: double quoted string */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* SQSTRING: single quoted string */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* END_DQSTRING: double quoted string at end of double quoted string */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* END_SQSTRING single quoted string at end of single quoted string */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* LIST: [ opens a list */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* LIST_ELEMEMT: , in list */ \
	{"     ","C"," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* END_OF_LIST: behind ] that ends a list */ \
	{"     ","C"," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* UNARY_OPERATOR: a single-character uniquely defining a unary operator: ! and ~ */ \
	{"     ","C"," "," "," "," ","N"," "," ","","","","","",""," "," "," "," ","","","",""}, /* OPERATOR: a single character defining either a unary or binary operator: + or - */ \
	{"     ","C"," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* BINARY_OPERATOR: characters defining a binary operator */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* FUNCTION: some identifier recognized as function name */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* FUNCTION_CALL ( following the name of a function */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* FUNCTION_CALL_ARGUMENT , in front of a new argument in a function call */ \
	{"     "," "," "," "," "," "," "," "," ","","","","","",""," "," "," "," ","","","",""}, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

// suggesting NOT to be able to get out of an error condition but to allow viewing information on the error somehow!!! (how about tab as this will do feed forward!!!!!)
// if we put the error info in the error token

uint8_t nextTokenType(uint8_t inputTokenType,char inputCharacterType){
	// finding the type will be more difficult actually if we end up with the token type character instead of the token type index!!!
	char* noTransition=NO_TRANSITIONS[inputTokenType];
#ifdef __DEBUG__
	printf("'%s'",noTransition);
#endif
	if(strlen(noTransition)==0||(noTransition[0]=='!'?strchr(noTransition,inputCharacterType)!=NULL:strchr(noTransition,inputCharacterType)==NULL)){
		uint8_t tokenType=NUMBER_OF_TOKEN_TYPES;
		while(tokenType>0){
			tokenType--;
/*
#ifdef __DEBUG__
			printf("(%d)",tokenType);
#endif
*/
			if(strchr(TRANSITIONS[inputTokenType][tokenType],inputCharacterType)!=NULL)return tokenType;
		}
	}
#ifdef __DEBUG__
	else{
		putchar('=');
	}
#endif
	return inputTokenType; // if no match was found assume no change to the token type!!
}

// first operator characters (0=assignment character, 1-6: binary 1 and 2-character operators, 7-8: 1-character binary, 9-10: unary/binary, 11-12: 1-character unary)
const char ASSIGNMENT_CHARACTER='=';
const char FIRST_OPERATOR_CHARACTERS[]={ASSIGNMENT_CHARACTER,'<','>','|','&','*','/','^','%','+','-','~','!','\0'}; // i.e. "=<>|&*/^%+-~!";
// continuation of 2-character operators
const char* SECOND_OPERATOR_CHARACTERS[]={"=","=<>","=<>","|","&","*","/"};
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

Token* pToken=NULL; // the current token
void removeToken(){
	// ASSERT pToken should NOT be NULL and empty (i.e. empty tokens should be removed!!!)
	Token* pPrevToken=pToken->prev;
	Token* pNextToken=pToken->next; // remember the previous and next token (we have to link to each other)
	// fix links
	if(pPrevToken!=NULL)pPrevToken->next=pNextToken;
	if(pNextToken!=NULL)pNextToken->prev=pPrevToken;
	freeToken(pToken); // get rid of the token
	pToken=pPrevToken; // replace pToken by the previous token
	// TODO the following does not seem to work!!!!
	if(pToken==NULL){
		pCommand=NULL;
#ifdef __DEBUG__
		putchar('Q');
#endif
	}
}
char removedTokenCharacter(uint16_t behindCursor){
#ifdef __DEBUG__
		printf("%d",behindCursor);
#endif
	uint16_t tokenCharacterPosition;
	// find the token that we should remove a character from (either the current token or the one in front of it (if all tokens are non-empty!))
	while(true){
		if(pToken==NULL)return '\0';
		tokenCharacterPosition=cursorPosition-pToken->offset;
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition);
#endif
		if(tokenCharacterPosition>=behindCursor)break;
#ifdef __DEBUG__
		putchar('.');
#endif		
		pToken=pToken->prev;
	}
#ifdef __DEBUG__
		printf("%d",tokenCharacterPosition-behindCursor);
#endif	
	// if failing to remove the character serious error
	char c=string_removed_char(pToken->text,tokenCharacterPosition-behindCursor);
#ifdef __DEBUG__
		putchar(c);
#endif		
	if(c)if(string_empty(pToken->text))removeToken(); // the token could now be empty, in which case we should remove it from the command
	return c;
}

// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(){
	if(pCommand==NULL)return false;
	printf("\nEvaluating '");
	Token* pCommandToken=pCommand; // TODO can we get rid of using commandcount-1 here????
	while(pCommandToken!=NULL){
/*
#ifdef __DEBUG__
		printf("{%p}",pCommandToken);
#endif
*/
		printf("%s",string(pCommandToken->text));
		if(assisting)putchar('|');
		pCommandToken=pCommandToken->next;
	}
	printf("'");
	return true;
}

void prepareForUserInput(){
	//enableRawMode();
	// disable output buffering on printf (as in raw input mode it would not write at all)
	setbuf(stdout,NULL);
}
void endOfUserInput(){
#ifdef __DEBUG__
	printf("\nEnd of user input.");
#endif
	// return to the 'right' colors
	resetOutputColor();
	if(rawMode)disableRawMode();
	printf("\nThanks for using M.\n\n");
}
// cursorLeft() will return the token under the cursor
void cursorLeft(){
	cursorPosition--;
	moveCursorLeft(1);
}
void cursorRight(){
	cursorPosition++;
	moveCursorRight(1);
}
////////void moveCursorLeft(uint8_t positions){while(--positions>=0)cursorLeft();}
// write rest of command will return the number of characters written
uint16_t writeTokens(Token* pFirstToken){
	uint16_t tokenCharactersWritten=0;
	Token* token=pFirstToken;
	while(token!=NULL){outputToken(token);tokenCharactersWritten+=string_length(token->text);token=token->next;}
	return tokenCharactersWritten;
}

uint32_t commandPage=0; // the command page to show (when 0 not paging through the commands)
uint32_t commandPages=0; // the total number of command pages
void setCommandPage(uint32_t newCommandPage){
	commandPage=newCommandPage;
	int32_t commandToShowIndex=10,lastCommandToShowIndex=commandCount-(commandPage*10);
	while(--commandToShowIndex>=0&&lastCommandToShowIndex+commandToShowIndex>=0){
		resetOutputColor();printf("\n%d. ",lastCommandToShowIndex+commandToShowIndex+1);
		writeTokens(commands[lastCommandToShowIndex+commandToShowIndex]);
	}
	resetOutputColor();
	printf("\nSelect the last digit of the command to use, or the up/down key to show the next/previous page.");
	printf("\n>> "); // TODO what kind of prompting do we want to do???
}
void showNextCommandPage(){
	if(commandPage<commandPages)
		setCommandPage(commandPage+1);
	else
		printf("\nNo further commands to show.");
}
void showPreviousCommandPage(){
	if(commandPage>1)
		setCommandPage(commandPage-1);
	else
		printf("\nNo further commands to show.");
}

void writeRestOfCommand(){
	uint16_t leftToWrite=commandLength-cursorPosition;
	if(leftToWrite){		
		outputTokenColor(pToken);printf("%s",string_remainder(pToken->text,cursorPosition-pToken->offset));
		// write the rest of the tokens
		writeTokens(pToken->next);
		moveCursorLeft(leftToWrite);
	}
}
void clearCommand(){
	if(!freeToken(pCommand))return;
	pCommand=NULL;
	pToken=NULL; // we shouldn't have a current token if we do not have a command anymore
}
void switchToControlMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)printf("\n%s",message);
	if(!commandInput)return;
	commandInput=false;
	printf("\nAvailable control options: eXit Assist History.\n>> ");
}
void backToPrompt(){
	// this will be more complicated if the command occupies multiple lines
	// therefore we need to move the cursor left, write a single blank and move the cursor one left again and so on
	if(cursorPosition>0){moveCursorLeft(cursorPosition);cursorPosition=0;}
	clearScreenFromCursor();
	/* replacing:
	while(characterCount>0){
		characterCount--;
		cursorLeft();resetOutputColor();putchar(' ');cursorLeft();
	}
	*/
}
/**
 * setCommandIndex() accepts @newCommandIndex between 0 and commandCount at most
 * but 0 is now also accepted, returning to show pCommand (if any)
 */
void setCommandIndex(uint32_t newCommandIndex){
	commandIndex=newCommandIndex;
	if(commandIndex){
		char infoText[80];snprintf(infoText,80,"Showing registered command #%u.",(commandCount-commandIndex+1));
		outputInfo(infoText);
	}else
		outputInfo("");
	// we're supposed to show one of the remembered commands
	backToPrompt();
	commandLength=cursorPosition=writeTokens(commandIndex?commands[commandCount-commandIndex]:pCommand);
	/////////////printf("(%d)",commandLength);
}
bool commandDown(){
	if(commandIndex>=commandCount)return false;
	setCommandIndex(commandIndex+1);
	return true;
}
bool commandUp(){
	if(commandIndex==0)return false;
	setCommandIndex(commandIndex-1);
	return true;
}
void newCommand(){
	commandLength=0;
	pCommand=newToken(NULL);
	pToken=pCommand;
	resetOutputColor(); // TODO do we need this here?????
}
void echoCommand(){
	Token* token=pCommand;
	resetOutputColor();
	while(token){printf("%s",string(token->text));token=token->next;}
}
Token* getCommand(){
	return(commandIndex?commands[commandCount-commandIndex]:pCommand);
}
// NEWYEAR'S DAY 2019: It's a nuisance to show a command without copying it into an actual newCommand
/**
 * setCommand() creates a new (empty) command (in pCommand) and initializes it to the token in pNewCommand (the command pointed to by commandIndex)
 *              which is supposedly showing behind the cursor!!!
 * ASSUMPTION should only be called when at the prompt (cursorPosition=0) ready for starting or changing a command
 * setCommand() won't show the command anymore as we assume that any registered command passed in is already showing!!!
 */
void setCommand(Token* pNewCommand){
	// ASSERT let's assume we're at the prompt (i.e. cursorPosition==0 and pCommand==NULL)
	// NO we cannot assume that because there might be a command currently showing at the prompt
	if(pCommand){clearCommand();backToPrompt();} // if we have a command get rid of it and ascertain to be at the prompt!!
	// the problem is that we do NOT want to actually change the new command, so we have to copy it somehow
	newCommand(); // NOTE might fail, in which case pToken will be NULL!!
	if(pNewCommand){ // something to copy
		// at least once we need to set pToken!!!
		Token* pNewToken=pNewCommand; // first token to copy!!
		// NOTE theoretically pToken could be NULL due to newToken() failing to create a new token
		while(pToken){
			// if failing to copy the text over get rid of the command constructed so far, and break
			if(!string_copy(pNewToken->text,pToken->text)){clearCommand();break;}
			commandLength+=string_length(pToken->text);
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
#ifdef __DEBUG__
		echoCommand();
#endif
	}/*else commandIndex=0; // don't think we need this anymore, as pNewCommand will only be NULL when commandIndex==0 */
	/* won't echo what is supposedly already there!!!)
	// if we end up with a command, show it...
	cursorPosition=commandLength;
	if(cursorPosition>0)writeTokens(pCommand);
	*/
}
int main(){

	// TODO allow non-interactive mode i.e. execute commands from an M source file
	prepareForUserInput();

	resetOutputColor(); // just in case
	printf("\nWelcome to M.\n");
	printf("\nUse Ctrl-Z to exit M immediately at any time.\n");

	pCommand=NULL; // the current command (token)
	commandLength=0; // keep track of the total command length...
	commandInput=true; // TODO should this go into promptForUserInput()?
	char inputCharacterType;
	while(1){

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();
		//////////outputInfo("Let's see what happens!",promptLength+cursorPosition);
		pToken=pCommand;
		// an existing command to show
		while(pToken!=NULL){
			outputToken(pToken);
			// the cursor will move along with every printf()
			cursorPosition+=string_length(pToken->text);
			pToken=pToken->next;
		}
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(inputCharRead()){
			////////putchar('@');
			if(inputChar>127)continue; // undefined input character
			inputCharacterType=INPUTCHARACTERTYPES[inputChar];
#ifdef __DEBUG__
			printf("(%c)",inputCharacterType);
#endif
			// special (control) input character types
			// first the ones that will break
			if(inputCharacterType=='n')break; // end-of-line (CR of LF) character
			if(inputCharacterType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character
			if(inputCharacterType=='-')continue; // input character without specific purpose
			if(inputCharacterType=='b'||inputCharacterType=='d'){ // backspace or delete
				// something to remove?
				if(commandLength){ // TODO pCommand should be NULL at the same time commandLength becomes 0!!!
					char removedCharacter=removedTokenCharacter(1);
					if(removedCharacter){
						commandLength--; // decrement the total command length
						if(commandLength==0)pCommand=NULL;
						// on screen as well please
						cursorLeft(); // will decrement cursorPosition
						clearScreenFromCursor(); // will clear what's behind the cursor
						if(pCommand)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
					}else
						switchToControlMode("Switching to control mode, due to failing to remove the intended character!");
				}else // nothing to remove
					beep();
				continue;
			}
			if(inputCharacterType=='c'){ // cancel command (Ctrl-D)
				if(pCommand!=NULL){
					clearCommand();
					backToPrompt();
				}else
					beep();
				continue;
			}
			if(inputCharacterType=='m'){
				if(inputCharRead()){
					if(inputChar==91){
						if(inputCharRead()){
							if(inputChar==65){ // up arrow 
								if(commandInput){ // i.e. show previous command if any
									if(pCommand)
										outputInfo("Won't show previous commands when one is being entered.");
									else
									if(!commandDown())
										beep();
								}else
								if(!commandPage)
									showPreviousCommandPage();
								/*
								else
								if(!commandDown())
									outputInfo("No previous command!");
								*/
							}else
							if(inputChar==66){ // down arrow
								if(commandInput){									
									if(pCommand)
										outputInfo("No next command!");
									else 
									if(!commandUp())
										beep();
								}else
								if(!commandPage)
									showNextCommandPage();
								/*
								else
								if(!commandUp())
									outputInfo("No next command!");
								*/
							}else
							if(inputChar==67){ // right arrow
								if(cursorPosition<commandLength){
									cursorRight();
								}else
									beep();
							}else
							if(inputChar==68){ // left arrow
								if(cursorPosition>0){
									cursorLeft();
									// if the cursor position now matches the offset of the current token
									// we're at the end of the previous token
									if(cursorPosition==pToken->offset){
										pToken=pToken->prev;
									}
								}else
									beep();
							}
						}
					}
				}
				continue;
			}
			if(inputCharacterType=='o'){
				switchToControlMode(NULL);
				continue;
			}
			// in command input we allow to do things with the command
			if(commandInput){
				////////printf(" (%d)",inputChar);
				// we need to have a token (to append the input character to) which initializes to pCommand
				if(pCommand==NULL){ // no first command token
					// if commandIndex we should one of the registered commands
					setCommand(commandIndex?commands[commandCount-commandIndex]:NULL); // will also set commandLength!!!
/*
#ifdef __DEBUG__
					printf("@%p",pCommand);
#endif
*/
					//////////if(pToken==NULL)printf("?");
				}
				// if still NULL (also when we fail to actually create a new first command token)
				if(pToken!=NULL){
					// determine the token type associated with the newly inputted character
					int16_t newTokenType=nextTokenType(pToken->type,inputCharacterType);
#ifdef __DEBUG__
					resetOutputColor();
					printf("[%d+%c->%d]",pToken->type,inputCharacterType,newTokenType);
					outputTokenColor(pToken);
#endif
					// if this is not the same token type we have to start a new token
					// TODO will be different if we're inserting characters
					if(newTokenType>=0&&newTokenType!=pToken->type){ // character ends the current token
						pToken=newToken(pToken);
/*
#ifdef __DEBUG__
						printf("@%p=%p?:%s",pCommand,pToken,string(pCommand->text));
#endif
*/
						pToken->type=newTokenType;
						// TODO should we write the associated colors here?????
						outputTokenColor(pToken);
					}
					// insert the typed character at cursorPosition minus current token offset in pToken->text
					string_insert_char(pToken->text,cursorPosition-pToken->offset,inputChar);
#ifdef __DEBUG__
					printf("[%s]",string(pToken->text));
#endif
					commandLength++; // increment total command length
					putchar(inputChar); ///////// replacing: outputLastTokenChar(pToken); // echo the last token character
					cursorPosition++; // increment the current cursor position
					// write all remaining characters in the command after which we should return to the current position
					writeRestOfCommand();
				}else
					switchToControlMode("Switching to control mode, due to failing to create a new command!");
			}else{
				putchar(inputChar); // nice to see the character we typed...
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					// an option character!!!
					if(inputChar=='x'||inputChar=='X')exit(0);else
					if(inputChar=='a'||inputChar=='A'){assisting=!assisting;printf("\n%s\n>> ",(assisting?"Will assist!":"Will not assist!"));}
					if(inputChar=='h'||inputChar=='H'){
						// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
						// we could call them a, b, c etc.
						if(commandCount){
							commandPages=1+(commandCount-1)/10;
							showNextCommandPage(); // as soon as commandPage>0 we are paging...
						}else
							printf("\nNo previous commands to show.");
					}
				}else{
					// user might have selected one of the commands (letter a through j)
					commandPage=0; // stop paging
				}
			}
		}
		// if eXit input character(s) received...
		if(inputCharacterType=='x')break;
		if(inputCharacterType=='n'){
			if(commandInput){ // the newline character ends the command to be evaluated!!
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				// if the command ended with a normal end-of-line character, evaluate and register the command
				// NOTE if pCommand is not set yet, but the user retrieved a previously executed command, that one should be reexecuted
				//      and registered (of course it will be pointing to the same chain of tokens but it might evaluated differently now)
				if(!pCommand)if(commandIndex)pCommand=commands[commandCount-commandIndex];
				if(pCommand){
					// typically the command will not evaluate if it is not complete
					if(!evaluateCommand()){
						printf("\nFailed to evaluate the command!");
						clearCommand();
					}else
					if(!registerCommand()){
						switchToControlMode("Switching to control mode, due to failing to register the command.");
						clearCommand();
					}else{ // evaluated AND registered, go back to the top
						commandIndex=0;
						pCommand=NULL; // prepare for a new command BUT do not clear the command because it was registered!!!
					}
				}else{
					printf("\nNo command to evaluate.");
				}
			}else // always to return to command input!!
				commandInput=true;
		}
	}
	// 'normal' exit
	exit(0);
}
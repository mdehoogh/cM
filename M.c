// remove the line below when not in debug mode
//#define __DEBUG__

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>

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

// MDH@28FEB2019: most conveniently to be able to output to the console through a single method that will allow a format string, and any number of arguments
//                TODO delegate all functions that output to the output device to this function
void output(const char *fmt,...){va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);} // NOTE use vprintf here, NOT printf!!!!

bool assisting=false; // assist flag can be turned on to guide the user
bool debugging=true; // program debugging flag so it will show the token information before evaluation of a command

char* promptinfo="\nCommand mode: type ` to enter control mode; cancel the current command with Ctrl-C.\n";
/**
call prompt() when ready to receive a new command
 */
const char OPTION_CHAR='`'; // TODO should this character become part of options????

void prompt(); // prototype of prompt!!
void promptForUserInput(){
	if(!rawMode)enableRawMode();
	output("\n%s",promptinfo);
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
	if(read(STDIN_FILENO,&inputChar,1)==1){
		//printf("{%i}",inputChar);
		return 1;
	}
	return 0;
}

// text colors
const char DEBUG_COLOR[]="8"; // light gray
const char INFO_COLOR[]="0"; // black
// for tokens
const char COMMENT_COLOR[]="8"; // light gray
const char ERROR_COLOR[]="9"; // red
const char ASSIGNMENT_COLOR[]="202"; // orange for assignment operator
const char VARIABLE_COLOR[]="13"; // magenta
const char FUNCTION_COLOR[]="93"; // something more blueish
const char FUNCTIONARG_COLOR[]="12";
const char LIST_COLOR[]="12";
const char NUMBER_COLOR[]="22"; // green
const char STRING_COLOR[]="12"; // blue
const char UNARY_COLOR[]="93"; // like a function I suppose
const char BINARY_COLOR[]="93"; // like a function I suppose
const char TERNARY_COLOR[]="93"; // like a function I suppose

const char RESULT_COLOR[]="15"; // quite dark
const char OPTION_COLOR[]="15"; // RESULT_COLOR
const char* PROMPT_COLOR=INFO_COLOR; // same as the info color

// the back colors
const char DEBUG_BACKCOLOR[]="255"; // light-gray
const char INFO_BACKCOLOR[]="231"; // white
// for tokens
const char ERROR_BACKCOLOR[]="231";
const char ASSIGNMENT_BACKCOLOR[]="0";
const char COMMENT_BACKCOLOR[]="0";
const char VARIABLE_BACKCOLOR[]="0";
const char FUNCTION_BACKCOLOR[]="0";
const char FUNCTIONARG_BACKCOLOR[]="0";
const char LIST_BACKCOLOR[]="0";
const char NUMBER_BACKCOLOR[]="0";
const char STRING_BACKCOLOR[]="0";
const char UNARY_BACKCOLOR[]="0";
const char BINARY_BACKCOLOR[]="0";
const char TERNARY_BACKCOLOR[]="0";

const char IDENTIFIER_BACKCOLOR[]="231";
/*
const char LITERAL_BACKCOLOR[]="231";
const char OPERATOR_BACKCOLOR[]="231";
*/
const char RESULT_BACKCOLOR[]="69";
const char* OPTION_BACKCOLOR=RESULT_BACKCOLOR;
const char BEHIND_CURSOR_TEXT_COLOR[]="250"; // MDH@27FEB2019: same as DEBUG_BACKCOLOR (which we're NOT using?)

// operator token colors
const char* OPERATOR_TOKEN_COLORS[]={ASSIGNMENT_COLOR,UNARY_COLOR,BINARY_COLOR,TERNARY_COLOR};
const char* OPERATOR_TOKEN_BACKCOLORS[]={INFO_BACKCOLOR,INFO_BACKCOLOR,INFO_BACKCOLOR,INFO_BACKCOLOR};

// value token colors
const char* VALUE_TOKEN_COLORS[]={COMMENT_COLOR,INFO_COLOR,VARIABLE_COLOR,NUMBER_COLOR,STRING_COLOR,STRING_COLOR,STRING_COLOR,STRING_COLOR,LIST_COLOR,LIST_COLOR,LIST_COLOR,FUNCTION_COLOR,FUNCTIONARG_COLOR,FUNCTIONARG_COLOR,FUNCTIONARG_COLOR};
const char* VALUE_TOKEN_BACKCOLORS[]={COMMENT_BACKCOLOR,INFO_BACKCOLOR,VARIABLE_BACKCOLOR,NUMBER_BACKCOLOR,STRING_BACKCOLOR,STRING_BACKCOLOR,STRING_BACKCOLOR,LIST_BACKCOLOR,LIST_BACKCOLOR,LIST_BACKCOLOR,FUNCTION_BACKCOLOR,FUNCTIONARG_BACKCOLOR,FUNCTIONARG_BACKCOLOR,FUNCTIONARG_BACKCOLOR};

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
mstring* behindCursorText=NULL; // MDH@27FEB2019: we keep track of the characters behind the cursor

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
	/* MDH@26FEB2019: we do not need the following because that's taken care of in writeTokens(pCommand) right after promptForUserInput()
	cursorPosition=0; // starting at position 0
	*/
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
void outputText(char* fmt,char* text){	
	resetOutputColor(); 
	output(fmt,text);
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

/* MDH@25MAR2019: 
   - technically an assignment is a binary operator that requires a variable identifier to the left of it
     for now we define the assignment token to be the only operator with length, although we could've categorized it in TT_ONE_CHAR_BINARY as well
     but I separate the assignment operator from any other binary operator in front of it (for shortcutting certain binary operations)
     bit 0-1: maximum length
     bit 2  : minimum length 0=1 1=2
     bit 3  : 1-assignable
     bit 4  : 2-assignable
     bit 5  : unary (0) or binary (1) 
*/
/* MDH@28MAR2019: 
   - after creating a state diagram of the possible operators I have made a new categorization of binary operators which typically are sequences of characters of a restricted length
     most of which are 1 character operators that are extensible with another character (often the same), but are operators already, some are not extensible, some need two characters in which case the one character operator is not complete yet, which means it is not finished yet
     in the state diagram we end up with a total of 11 possible states 
     the general idea is that the assignment is NOT part of the operator in front of it, i.e. an assignment token is a separate token which may follow certain binary operators in particular arithmetic operators
     some of the operators may be combined at some point, which means we might not need all of them at the end
     let's first write down all tokens that we would end up in when the first token character is entered:
     FIRST TOKEN CHARACTER TOKENS:
     < >         SMALLER_OR_LARGER_THEN = COMPARISON (EQUALIZABLE,REPEATABLE) is an operator already by itself (so a complete operator) which can be followed by either = to become a (finished) comparison operator (non assignable) or by the same character (< or >) to become a shift operator (which is assignable)
     & |         BITWISE = ARITHMETIC (REPEATABLE)                            is an operator already by itself (a bitwise operator) which can be followed by the same character (extensible, assignable) to become the && or || logical operator (AND_OR_OR or LOGICAL) wich is not assignable
     ! =         UNEQUAL_OR_EQUAL = COMPARISON (UNFINISHED,EQUALIZABLE)       is not yet an operator by itself has to be followed by = to become a finished non assignable comparison operator (IDEA is this the same operator we end up with coming from < and >??????????)
     =           ASSIGNMENT                                                   a single character binary operator that should be preceded by optionally a binary operator that is assignable (ARITHMETIC probably) and something that can be assigned to (variable or list element), in all other situations it should be assumed to be UNEQUAL_OR_EQUAL (to be completed with another =)
     ? :         TERNARY                                                      a single character binary operator which is not extensible and not assignable (so its a completed binary operator that cannot be extended) IDEA: is TERNARY a binary operator we have or should we split it up in TERNARY FIRST and TERNARY LAST????? I guess we can check for another ternary in front and determine if ? or : is acceptable!!!
     - + ~ ^ %   ARITHMETIC (FINISHED)                                        by itself already a binary operator which is assignable but not extensible (with the same character), so a finished binary operator which yet we could call ARITHMETIC perhaps when + is used behind something that represents a string we should can it CONCATENATION unless we decide to use another character for concatenation like .
     +           CONCATENATION                                                when preceded by something that represents a string value + should be considered a CONCATENATION operator (which may be difficult to determine when a function is called in front of the + sign (we could force functions that result in string results to start with STR_ or end with $ or something like that)
     * /         MULT_OR_DIVIDE = ARITHMETIC REPEATABLE                       by itself a binary operator which is assignable AND extensible with the same character, this could be an intermediate binary operator that we turn into ARITHMETIC after the next character which means that MULT_OR_DIVIDE is NOT a token that we have when we evaluate the expression
     SECOND TOKEN EXTENSION CHARACTERS: after the first token a second token can be entered as part of an operator that allows for continuation or transformation of the operator to another operator, in this case always a finished operator (that does not allow further continuation)
	 < >         SHIFT = ARITHMETHIC                                          when < or > is repeated we end up with a (completed) SHIFT ARITHMETIC operator which is assignable, so we could have ARITHMETIC EXTENSIBLE and ARITHMETIC (UNEXTENSIBLE) the latter when it's finished
	 & |         LOGICAL = ARITHMETIC                                         when & or | is repeated we end up with a (completed) LOGICAL operator that is NOT assignable although we could allow it to be assignable in which case it is ARITHMETIC (UNEXTENSIBLE)
	 SECOND TOKEN CHARACTERS:
	 =           COMPARISON (NON ASSIGNABLE,NON REPEATABLE,NOT EQUALIZABLE)   when ! or = is followed by = we end up with a completed COMPARISON operator that is not assignable and not extensible, but note that = is either an UNEQUAL_OR_EQUAL or ASSIGNMENT which means that unless = is assignment, it should be treated as UNEQUAL_OR_EQUAL we should start with assignment when another = is NOT required per se
	 * /         ARITHMETIC                                                   follows the same character to remain an assignable complete arithmetic operator

     So, we have one unfinished comparison operator (! or =), and a COMPARISON operator could also be called arithmetic theoretically, so ARITHMETIC is one end type of binary operator but a comparison operator is not assignable (that's because you can have <= and >= and != and ==), so COMPARISON=ARITHMETIC NOT ASSIGNABLE, ARITHMETIC ASSIGNABLE
     So, COMPARISON is < or > which becomes ARITHMETIC NOT ASSIGNABLE when followed by = although the = is not obligatory, as compared with ! and = which is COMPARISON UNFINISHED or ARITHMETIC UNFINISHED 
     So, if we rebuild the token results
     FIRST TOKEN CHARACTER TOKENS:
     < >         COMPARISON = ARITHMETIC NONASSIGNABLE EQUALIZABLE (something you can put an equal sign behind to become an finished not assignable arithmetic operator or the same character to become an assignable arithmetic operator)
     & |         BITWISE = ARITHMETIC REPEATABLE (something you can assign to immediately or repeat and still be assignable)
     ! =         NOT_OR_EQUAL = ARITMETIC NONASSIGNABLE EQUALIZABLE UNFINISHED = COMPARISON (UNFINISHED,UNREPEATABLE) (something you MUST put an equal sign behind to become a finished not assignable arithmetic operator)
     =           ASSIGNMENT (a finished binary operator) that could become a ARITHMETIC UNASSIGNABLE operator when equal sign is appended to it
     ? :         TERNARY (a finished one character binary operator, not assignable, non repeatable, not equalizable) operator
     - + ~ ^ %   ARITHMETIC ASSIGNABLE operator
     * /         ARITHMETIC ASSIGNABLE REPEATABLE operator
     Two character operators:
     << >>		 SHIFT = ARITHMETIC ASSIGNABLE
     <= >=       ARITHMETIC NOTASSIGNABLE 
     && ||       LOGICAL = ARITHMETHIC ASSIGNABLE
     != ==       ARITHMETIC NOTASSIGNABLE
     ** //       ARITHMETIC ASSIGNABLE
     So main categories: ASSIGNMENT (1xxx xx01) ARITHMETIC (1xxx xx10) TERNARY FIRST (1xxx xx11), in addition to UNARY (1xxx xx00), unless we use the uppermost flags to denote the three categories non-operator, unary operator, binary operator, ternary operator, so 00xx xxxx (non-operators), 01xx xxxx (unary operators), 10xx xxxx (binary operators), 11xx xxxx (ternary operators)
     And the flags on operators are in the uppermost bits from right to left alphabetic
     Flags: ASSIGNABLE, REPEATABLE, FINISHED, EQUALIZABLE but those are only applicable to binary operators??????
     Are we putting an error flag in the type?????? forcing to display the token in red (overriding any basic color we would have...), because then we would still recognized the type of token even though there's an error somewhere in front of the token sequence... which ends when the subexpression ends, which would be nice to have
     At the moment all zeroes (0b00000000) means error, should we start with an ERROR token????? that would make sense because an empty token is erroneous or it could represent the undefined null token like (undefined)==(undefined), in which case we should have an UNDEFINED token type (an assignment like 'x=' could define x but keep it undefined')
     I think that would be an elegant expression, in which case the result of an expression would also represent the undefined value, which could be represented textual as (undefined) or something similar, or just the text undefined because a string value should be enclosed in quotes.
     So the top bit could be the error bit, so error is not a separate token but a token property although I'd prefer starting an error token when an error occurs!! so TT_WHITESPACE is actually TT_UNDEFINED, any TT_UNDEFINED should change to something else after entering a non whitespace character!!!
     Ok, we could use the upmost 4 token type bits for the operator flags UNFINISHED REPEATABLE EQUALIZABLE ASSIGNABLE 
     The next two bits are for the operator type, 00 for assignment, 01 for unary, 10 for arithmetic, 11 for ternary
     Which means that we end up with:
		ASSIGNMENT                       1111 1111 =                                     because this is a special operator we use the special value 0xFF to indicate the assignment operator (bit 7 is the operator bit)
		UNARY                            1001 0000 ! - + when a value is expected next behind operator or at start of expression
		BINARY aeru                      1010 0000 <= >=
		BINARY aErU                      1010 0101 ! =
		BINARY AeRu                      1010 1010 & | * /
		BINARY aERu                      1010 0110 < >
		BINARY Aeru                      1010 1000 - + ^ ~ % \    TWO CHARACTER: << >> ** // && ||           
        TERNARY aeru                     1100 0000 ? :

*/
#define NUMBER_OF_TOKEN_TYPES 27
#define FOREACH_TOKENTYPE(TOKENTYPE) \
		TOKENTYPE(TT_ERROR) \
		TOKENTYPE(TT_COMMENT) \
		TOKENTYPE(TT_END_OF_COMMENT) \
		TOKENTYPE(TT_UNARY) \
		TOKENTYPE(TT_ASSIGNMENT) \
		TOKENTYPE(TT_BINARY_aeru) \
		TOKENTYPE(TT_BINARY_aErU) \
		TOKENTYPE(TT_BINARY_AeRu) \
		TOKENTYPE(TT_BINARY_aERu) \
		TOKENTYPE(TT_BINARY_Aeru) \
		TOKENTYPE(TT_TERNARY_aeru) \
		TOKENTYPE(TT_EXPRESSION) \
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
		TOKENTYPE(TT_FUNCTION) \
		TOKENTYPE(TT_FUNCTION_CALL) \
		TOKENTYPE(TT_FUNCTION_ARGUMENT) \
		TOKENTYPE(TT_END_OF_FUNCTION_CALL)
#define GENERATE_TOKENTYPE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
enum TOKENTYPE_ENUM {
	FOREACH_TOKENTYPE(GENERATE_TOKENTYPE_ENUM)
};
// the list of token type ids in the corresponding order!!!
const uint8_t TOKENTYPE_IDS[]={0b11111111,0b1000000,0b10111111,0b01010000,0b010000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
/*
typedef struct{
	unsigned int ended:1; // one flag to indicate whether or not the Token has ended
	unsigned int complete:1; // one flag to indicate whether or not the token is complete
	unsigned int type:2; // 00=value, 01=unary operator, 02=binary operator, 03=ternary operator
	unsigned int subtype:4; // what subtype it is, i.e. the type of operator
}TokenType;
*/
typedef struct Token{
	enum TOKENTYPE_ENUM type; // actually the index into the TOKENTYPES array!!!
	uint8_t significantCharacterCount; // MDH@22MAR2019: the number of significant characters in the token (in front of any whitespace that the users add, should be set to the length of the text when that happens)
	uint16_t offset; // number of characters in front of this token in the command
	mstring* text;
	struct Token* expr; // the expression this token is part of
	struct Token* prev; // we need this during user input
	struct Token* next;
}Token;

void outputTokenColor(Token* pToken){
	///////printf("[%d]",pToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
	uint8_t tokentype_id=TOKENTYPE_IDS[pToken->type];
	switch(tokentype_id>>6){
		case 0: // value token
			setColor(VALUE_TOKEN_COLORS[tokentype_id]);
			setBackColor(VALUE_TOKEN_BACKCOLORS[tokentype_id]);
			break;
		case 1: // operator: unary, binary, ternary, assignment
			setColor(OPERATOR_TOKEN_COLORS[(pToken->type&0x30)>>4]);
			setBackColor(OPERATOR_TOKEN_BACKCOLORS[(pToken->type&0x30)>>4]);
			break;
		case 2: // comment or end of comment
			setColor(COMMENT_COLOR);
			setBackColor(COMMENT_BACKCOLOR);
			break;
		case 3: // error token
			//putchar('E');
			setColor(ERROR_COLOR);
			setBackColor(ERROR_BACKCOLOR);
			break;
	}
}
void outputToken(Token* pToken){
	outputTokenColor(pToken);
	// if we allow comments in tokens we're in trouble!!!
	printf("%s",string(pToken->text));
	/////////if(assisting){resetOutputColor();putchar('|');}
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

Token* pToken=NULL; // the current token

// output functions that require access to the current token
void toStartOfPreviousLine(){
	oneLineUp();
	toStartOfLine();
	clearLine();
	toStartOfLine();
}
void toStartOfNextLine(){
	oneLineDown();
	toStartOfLine();
}
void toCursorPosition(){
	moveCursorRight(promptLength+cursorPosition);
	if(pToken)outputTokenColor(pToken); // return to the current token color
}
void outputInfo(const char* fmt,...){
	if(strlen(fmt)){ // we have a format
		toStartOfPreviousLine();
		resetOutputColor(); // get the default output color!!
		// NOTE we have to call vprintf here NOT printf!!!
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		toStartOfNextLine();
		toCursorPosition();
	}
}

void outputStatus(){
	////////printf("[%u,%u]",cursorPosition,commandLength);
	debugWrite("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition,commandLength,string(behindCursorText));
	if(assisting)
		outputInfo("Status: Cursor position=%" PRIu16 " - command length=%" PRIu16 " - behind cursor text='%s'.",cursorPosition,commandLength,string(behindCursorText));
	//////outputInfo("Status: Cursor position=%u - command length=%u - behind cursor text='%s'.",cursorPosition,commandLength,string(behindCursorText));
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

// keep track of the user input count
Token* pCommand=NULL; // the current command

Token** commands=NULL; // array for storing the pointers to the first token of all commands entered
uint32_t commandBlocks=0;
bool registerCommand(Token* pCommandToRegister){
	if(!pCommandToRegister)return false;
	if(commandCount==commandBlocks*COMMAND_BLOCKSIZE){
		// I have to copy all first token pointers to a new array large enough
		commandBlocks++;
		Token** newCommands=realloc(commands,COMMAND_BLOCKSIZE*commandBlocks*sizeof(Token*));
		if(newCommands==NULL)return false;
        commands=newCommands;
	}
	commands[commandCount++]=pCommandToRegister;
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
const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-./NNNNNNNNNN:;<=>?@LLLLELLLLLLLLLLLLLLLLLLLLL[%]%LoLLLLELLLLLLLLLLLLLLLLLLLLL{&}%d";

// now we define all the state transitions i.e. what input character types result in which new token type
// NOTE this can be organized in many ways perhaps it's easiest to tell per input character what the transformation is
//      only changes to the token type need to be registered, so if the change is NOT present, no need to put it in the transition table
//      EWW means that when starting an expression any whitespace starts a whitespace token, we use * to indicate ALL possible input character types
//      *WW means that any W character received in any state will result in a W state 
// we can make an array of transitions with each element corresponding to the character in TOKENTYPES, so the first entry contains all responses to E, the second entry the responses to W etc.
// it's easier to tell for any possible resulting token type which input character types will result in that type
// it's a hell of a job to create the token type transitions matrix
char* const NO_TRANSITIONS[NUMBER_OF_TOKEN_TYPES]={"","","","","","","","","","","","","","","","","","","","","","","","","","",""};

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
	TOKENTYPE(TT_ONE_CHAR_BINARY_=0b10100001)      					? :
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
// operator input type characters: ! ~ + - % * < = | (8 different operator groups)
// ! ~ and + start a unary operator when a value is expected
const char * const TRANSITIONS[NUMBER_OF_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
	{"ERROR"                   ,"COMMENT","ENDOFCOMMENT","UNA","A","Baeru","BaErU","BAeRu","BaERu","BAeru","Taeru","EXPRESSION","VARIABLE","INTEGER","REAL","EREAL","DQSTRING","SQSTRING","ENDOFDQSTRING","ENDOFSQSTRING","LIST","LIST_ELEMENT","ENDOFLIST","FUNCTION","FUNCTIONCALL","FUNCTIONCALLARGUMENT","ENDOFFUNCTIONCALL"}, /* ERROR */ \
	{""                        ,"C"      ,""            ,""   ,"" ,""     ,""     ,""     ,""      ,""    ,""     ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* COMMENT */ \
	{""                        ,""       ,"C"           ,""   ,"" ,""     ,""     ,""     ,""      ,""    ,""     ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* END OF COMMENT */ \
	{"D%&S)*/,.:;<>?@E%]{}="   ,""       ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* ONE CHARACTER UNARY !-+ */ \
	{"D%&S)*/,.:;<>?@E%]{}"    ,"C"      ,""            ,"!-+","" ,""     ,""     ,"="    ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* ASSIGNMENT = */ \
	{"%&()*/,.:;<>=?@E%]"      ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* Baeru */ \
	{"D%&S)*/,.:;<>?@E%]{}C"   ,""       ,""            ,"!-+","" ,"="    ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* BaErU */ \
	{"D%&S),.:;<>?@E%]{}"      ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,"R"   ,""     ,""          ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* BAeRu */ \
	{"D%S)*/,.:;<>?@E%]{}"     ,"C"      ,""            ,"!-+","" ,"="    ,""     ,"&|*/" ,""      ,"R"   ,""     ,""          ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* BaERu */ \
	{"D%&S)*/,.:;<>?@E%]{}"    ,"C"      ,""            ,"!-+","=",""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* BAeru */ \
	{"D%&S)*/,.:;<>?@E%]{}"    ,"C"      ,""            ,"!-+","=",""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,""        ,""        ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* Taeru */ \
	{"%&)*/,.:;<>=?@E%]}"      ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* EXPRESSION */ \
	{"!DS({"                   ,"C"      ,""            ,""   ,"=",""     ,"!"    ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,"LEN"     ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
	{"!DS(@{="                 ,"C"      ,""            ,""   ,"" ,"?:"   ,"!="   ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,"N"      ,"."   ,"E"    ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* INTEGER: (signless) list of digits */ \
	{"!DS(.@{="                ,"C"      ,""            ,""   ,"" ,"?:"   ,"!="   ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,"E"    ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* REAL: part behind a decimal period */ \
	{"!DS(.@E{="               ,"C"      ,""            ,""   ,"" ,"?:"   ,"!="   ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* EREAL part behind character 'e' in integer or real (only digits allowed) */ \
	{""                        ,"C"      ,""            ,""   ,"" ,""     ,""     ,""     ,""      ,""    ,""     ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* DQSTRING: double quoted string */ \
	{""                        ,"C"      ,""            ,""   ,"" ,""     ,""     ,""     ,""      ,""    ,""     ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* SQSTRING: single quoted string */ \
	{"!DL%&S(*/,.@E[{%-"       ,"C"      ,""            ,""   ,"" ,"?:+"  ,""     ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
	{"!DL%&S(*/,.@E[{%-"       ,"C"      ,""            ,""   ,"" ,"?:+"  ,""     ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* END_SQSTRING single quoted string at end of single quoted string */ \
	{"%&)*/,.:;<>=?@E%}"       ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,"]"        ,""        ,""            ,""                    ,")"                }, /* LIST: [ opens a list */ \
	{"%&)*/,.:;<>=?@E%]}"      ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* LIST_ELEMEMT: , in list */ \
	{"!DL(.N@E{"               ,"C"      ,""            ,""   ,"" ,"?:"   ,"%-+"  ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* END_OF_LIST: behind ] that ends a list */ \
	{"!D%&S)*/+-,.:;<>=?@[%]{}","C"      ,""            ,""   ,"" ,""     ,""     ,""     ,""      ,""    ,""     ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,""            ,""         ,""        ,"("           ,""                    ,""                 }, /* FUNCTION: some identifier recognized as function name */ \
	{"%&*/,.:;<>=?@E%]}"       ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* FUNCTION_CALL ( following the name of a function */ \
	{"%&)*/,.:;<>=?@E%]}"      ,"C"      ,""            ,"!-+","" ,""     ,""     ,""     ,""      ,""    ,""     ,"("         ,"LE"      ,"N"      ,""    ,""     ,"D"       ,"S"       ,""             ,""             ,"["   ,""            ,""         ,""        ,""            ,""                    ,""                 }, /* FUNCTION_CALL_ARGUMENT , in front of a new argument in a function call */ \
	{"!DLS(.N@E{"              ,"C"      ,""            ,""   ,"" ,"?:"   ,"%-+"  ,"&|*/" ,"<>"    ,"-+%" ,"?:"   ,""          ,""        ,""       ,""    ,""     ,""        ,""        ,""             ,""             ,""    ,","           ,"]"        ,""        ,""            ,""                    ,")"                }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

// suggesting NOT to be able to get out of an error condition but to allow viewing information on the error somehow!!! (how about tab as this will do feed forward!!!!!)
// if we put the error info in the error token

uint8_t nextTokenType(uint8_t inputTokenType,char inputCharacterType){
	// finding the type will be more difficult actually if we end up with the token type character instead of the token type index!!!
	char* noTransition=NO_TRANSITIONS[inputTokenType];
#ifdef __DEBUG__
	printf("'%s'",noTransition);
#endif
	if(strlen(noTransition)==0||(noTransition[0]=='`'?strchr(noTransition,inputCharacterType)!=NULL:strchr(noTransition,inputCharacterType)==NULL)){
		uint8_t tokenType=NUMBER_OF_TOKEN_TYPES-1;
		while(--tokenType>=0){
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
	if(c){
		if(string_empty(pToken->text))
			removeToken(); // the token could now be empty, in which case we should remove it from the command
		else
		if(pToken->type!=TT_UNARY&&INPUTCHARACTERTYPES[c]=='W') // a whitespace is removed
			if(string_length(pToken->text)==pToken->significantCharacterCount) // the current length equals the number of significant characters (i.e. we remove the first whitespace in the token)
				pToken->significantCharacterCount=0;
	}
	return c;
}

// anything the user types is a sequence of tokens which we can store in a linked list
bool evaluateCommand(Token* pCommandToEvaluate){
	if(pCommandToEvaluate==NULL)return false;
	printf("\nEvaluating '");
	Token* pCommandToken=pCommandToEvaluate; // TODO can we get rid of using commandcount-1 here????
	while(pCommandToken!=NULL){
/*
#ifdef __DEBUG__
		printf("{%p}",pCommandToken);
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

// cursorLeft() will return the token under the cursor
void cursorLeft(){
	// ASSERT cursorPosition is assumed to be positive
	// MDH@27FEB2019: if we decide to move the text under the cursor into behindCursorText, this means removing the current token character
	//                as before BUT removePreviousTokenCharacter() already does that, so
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

/* MDH@26FEB2019: convenience method that takes care of writing the current command (and NOT returning to the start of the command!)
void writeCommand(){
	// MDH@27FEB2019: before actually writing the current command (if any) we clear the behindCursorText as we are assumed to be at the end of the command
	string_setlength(behindCursorText,0);
	commandLength=cursorPosition=writeTokens(pCommand);
	outputStatus();
}
*/
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

// when the user tries to insert a character we need to cut off the rest of the command and append it afterwards
char* removedRestOfCommand(){
	if(cursorPosition<commandLength){
		mstring* restOfCommand=string_create();
		if(restOfCommand!=NULL){
			uint16_t tokenPosition=cursorPosition-pToken->offset;
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

void writeRestOfCommand(){ // writes rest of command assuming pToken is not NULL and we are to return to the current cursor position adterwards!!
	uint16_t leftToWrite=commandLength-cursorPosition;
	if(leftToWrite>0){ // something left to write
		// something of the current token to write?
		if(cursorPosition>pToken->offset){ // part of current token to write
			outputTokenColor(pToken);printf("%s",string_remainder(pToken->text,cursorPosition-pToken->offset));
		}
		// write the rest of the tokens
		writeTokens(pToken->next);
		moveCursorLeft(leftToWrite);
	}
}
bool clearCommand(){
	// MDH@
	bool result=freeToken(pCommand);
	pCommand=NULL;
	pToken=NULL; // we shouldn't have a current token if we do not have a command anymore
	return result;
}
void switchToControlMode(char* message){
	clearCommand();
	resetOutputColor();
	if(message!=NULL)printf("\n%s",message);
	if(!commandInput)return;
	commandInput=false;
	output("\n%s\n >> ","Control flags: Assist Debug - Options: eXit History");
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
		outputInfo("%s",infoText);
	}else
		outputInfo("%s","");
	// we're supposed to show one of the remembered commands
	backToPrompt();
	// the problem here is that we cannot write pCommand (as that is supposedly containing the current command being edited)
	// now, we may decide to not demand that pCommand is NULL at the moment that a user is using the up and down arrows
	// however, up and down arrow are executed in the inner loop so it's OK to have pCommand not equal to NULL
	commandLength=cursorPosition=writeTokens(commandIndex?commands[commandCount-commandIndex]:pCommand);
	/////////////printf("(%d)",commandLength);
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
		// if the user decides to start typing ascertain to show it in the right color!!
		if(pToken)outputTokenColor(pToken);
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

void writeBehindCursorText(){
	uint16_t l=string_length(behindCursorText);
	if(l){
		debugWrite("Behind cursor text to write: '%s'.",string(behindCursorText));
		resetOutputColor();
		setColor(BEHIND_CURSOR_TEXT_COLOR);
		printf("%s",string(behindCursorText));
		moveCursorLeft(l); // back to where we started to write the behind cursor text
		if(pToken)outputTokenColor(pToken); // return to the color of the current token
	}
}

// in response to backspace the previous token character is to be removed
void removePreviousTokenCharacter(){
	char removedCharacter=removedTokenCharacter(1);
	if(removedCharacter){
		commandLength--; // decrement the total command length
		if(commandLength==0)pCommand=NULL;
		// on screen as well please
		cursorLeft(); // will decrement cursorPosition
		clearScreenFromCursor(); // will clear what's behind the cursor
		// MDH@27FEB2019: if what's behind the cursor is NOT in the command but in behindCursorText that's what we should now write
		writeBehindCursorText();
		// replacing: if(pCommand)writeRestOfCommand(); // write all characters at and after the cursor (will reset the cursor!!)
	}else
		switchToControlMode("Switching to control mode, due to failing to remove the intended character!");
}

void outputTokenInfo(){
	Token* token=pCommand;
	uint16_t tokenIndex=0;
	output("\n%s:","Tokens");
	output("\n%s\t%s\t%s\t%s\t%s\t\t\t%s","#","OFFSET","USED","LENGTH","TYPE","TEXT");
	while(token!=NULL){
		tokenIndex++;
		output("\n%u\t%u\t%u\t%u\t%-24s`%s`",tokenIndex,token->offset,token->significantCharacterCount,string_length(token->text),TOKENTYPE_STRING[token->type],string(token->text));
		token=token->next;
	}
}

int main(int argc, char **argv){

#ifdef __DEBUG__
	printf("\n%s","Token types:");
	printf("\nError                                   : %d.",TT_ERROR);
	printf("\nUnary operator                          : %d.",TT_UNARY);
	printf("\nAssignment operator                     : %d.",TT_ASSIGNMENT);
	printf("\nOBinary operator                        : %d.",TT_BINARY_aeru);
	printf("\nAssignable repeatable binary operator   : %d.",TT_BINARY_AeRu);
	printf("\nEqualizable repeatable binary operator  : %d.",TT_BINARY_aERu);
	printf("\nTEqualizable unfinished binary operator : %d.",TT_BINARY_aErU);
	printf("\nAssignable binary operator              : %d.",TT_BINARY_Aeru);
	printf("\nComment                                 : %d.",TT_COMMENT);
	printf("\nEnd of comment                          : %d.",TT_END_OF_COMMENT);
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
					if(argv[arg][i]=='A')assisting=true;
				}
			}
		}
	}

	// TODO allow non-interactive mode i.e. execute commands from an M source file
	prepareForUserInput();

	resetOutputColor(); // just in case
	output("\n%s\n","Welcome to M.");
	output("\n%s","Use Ctrl-Z to exit M immediately at any time.");

	behindCursorText=string_create(); // MDH@27FEB2019: create the behind cursor text (to be cleared whenever we start a new command)
	pCommand=NULL; // the current command (token)
	///// writeCommand() will take care of this!!!! commandLength=0; // keep track of the total command length...
	commandInput=true; // TODO should this go into promptForUserInput()?

	char inputCharacterType;
	while(1){

		// if we're supposed to start a new command (i.e. it's not a command continuation)
		promptForUserInput();

		/* MDH@16MAR2019: we're behind the prompt now and should start out without a current command (in pCommnad)
		//                if pCommand is NOT null, we have to make it NULL
		commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
		if(pCommand) // if we still have a command to free, free it entirely
			if(!clearCommand())
				switchToControlMode("Switching to control mode, due to failing to remove the command.");
		*/
		/* replacing (there shouldn't be a command to write right now, unless perhaps when someone entered an invalid command???? to be continued)
		   point: an evaluated command should be discarded??? in which case a user cannot correct it and has to type it in again
		   so it makes sense to be allowed to complete a command (that failed to evaluate)
		*/
		// TODO what if we're not in commandInput here??????
		if(commandInput){
			commandIndex=0; // TODO should we do this always (even if we have an incomplete command?????)
			if(pCommand==NULL)if(!string_setlength(behindCursorText,0))output("??"); // TODO should we be loosing behindCursorText here????
			commandLength=cursorPosition=writeTokens(pCommand);
			outputStatus();
		}
		// which used to be: writeCommand(); // write the current command (if any)

		/* replacing:
		pToken=pCommand;
		// an existing command to show
		while(pToken!=NULL){
			outputToken(pToken);
			// the cursor will move along with every printf()
			cursorPosition+=string_length(pToken->text);
			pToken=pToken->next;
		}
		*/
		// we do NOT need a command until after the first character which makes sense because we allow ` and arrow up and down to switch to option mode or select another command
		// now we need to read characters one at a time and echo them from the command line
		// Ctrl-D to exit M
		while(inputCharRead()){
			////////putchar('@');
			////printf("[%i]",inputChar);
			if(inputChar>127)continue; // undefined input character

			inputCharacterType=INPUTCHARACTERTYPES[inputChar];

			// special (control) input character types
			// first the ones that will break in any input mode!!!!
			if(inputCharacterType=='n')break; // end-of-line (CR of LF) character
			if(inputCharacterType=='x')break; // eXit (Ctrl-C or Ctrl-Z) character

			if(commandInput){

#ifdef __DEBUG__
				printf("(%c)",inputCharacterType);
#endif
				if(inputCharacterType=='i')continue; // insignificant input character without specific purpose
				
				if(inputCharacterType=='b'||inputCharacterType=='d'){ // backspace or delete
					// something to remove?
					if(commandLength) // TODO pCommand should be NULL at the same time commandLength becomes 0!!!
						removePreviousTokenCharacter();
					else // nothing to remove
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
				
				if(inputCharacterType=='m'){ // Esc character...
					char newInputChar='\0'; // if c ends up being something else it should be processed as a normal character!!!
					if(inputCharRead()){
						if(inputChar==91){
							if(inputCharRead()){
								if(inputChar==51){
									if(inputCharRead()){
										if(inputChar==126){ // delete
											if(cursorPosition<commandLength){
												// we could go one to the right and do a backspace!!
												cursorRight();
												removePreviousTokenCharacter();
											}else // nothing under the cursor to delete
												beep();
										}
									}
								}else
								if(inputChar==65){ // up arrow 
									if(commandInput){ // i.e. show previous command if any
										if(pCommand)
											outputInfo("%s","Won't show previous commands when one is being entered.");
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
									if(commandInput){									
										if(pCommand)
											outputInfo("%s","Won't show next commands when one is being entered!");
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
									if(cursorPosition<commandLength){
										// MDH@22MAR2019: instead of doing everything here (duplicating all code that is down below), we can find a way to use the 'normal' code
										////////////bool success=false;
										newInputChar=string_removed_char(behindCursorText,0);
										if(!newInputChar)
											debugWrite("%s","Failed to remove the first behind cursor text character.");
										/* MDH@22MAR2019: we can stop doing the following...										
											debugWrite("Character %c to be appended!",c);
											// this is complex in that it's not just about appending c
											// but also determining what the next token will be and if need be
											// start a new token
											if(!pToken)pToken=newToken(NULL); // we need a token!!!
											inputCharacterType=INPUTCHARACTERTYPES[c];
											int16_t newTokenType=(inputCharacterType=='W'?-1:nextTokenType(pToken->type,inputCharacterType));
											// MDH@22MAR2019: the next token type might be the same BUT if the current token already ended (due to whitespace) we should always start a new token
											if(newTokenType>=0&&(newTokenType!=pToken->type||pToken->significantCharacterCount>0)){ // character ends current token
												pToken=newToken(pToken);
												pToken->type=newTokenType;
												if(pToken->type==TT_SINGLE_CHARACTER_UNARY_OPERATOR)pToken->significantCharacterCount=1; // MDH@22MAR2019: every unary token has at most one significant character
												// TODO should we write the associated colors here?????
											}else // no change to the token type (so character did not start a new token)
											if(inputCharacterType=='W'&&pToken->significantCharacterCount==0&&pToken->type!=TT_WHITESPACE) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
												pToken->significantCharacterCount=string_length(pToken->text);
											debugWrite("%s","Inserting character!");
											string_insert_char(pToken->text,cursorPosition-pToken->offset,c);
											
											// MDH@28FEB2019: does NOT change commandLength, so NOT doing: commandLength++;
											outputTokenColor(pToken);
											putchar(c);
											success=true;
										}else
											debugWrite("%s","Failed to remove the first behind cursor text character.");
										if(success){
											cursorPosition++; /////////cursorRight();
											debugWrite("%s","Cursor position incremented!");
											///////writeBehindCursorText();
										}else
											switchToControlMode("Failed to move the cursor right.");
										*/
									}else
										beep();
								}else
								if(inputChar==68){ // left arrow
									if(cursorPosition>0){
										// TODO apparently pCommand will still be NULL when we're scrolling through the list of previous commands...
										if(pCommand==NULL)if(commandIndex)setCommand(commands[commandCount-commandIndex]); // will also set commandLength!!!
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
											cursorLeft();
											/* NO going back and forth with the cursor does NOT change commandLength!!!!
											commandLength--; // MDH@28FEB2019: essential bro' otherwise when we go back to the end, cursorPosition will stay below commandLength
											*/
											// if the cursor position now matches the offset of the current token
											// we're at the end of the previous token
											if(cursorPosition==pToken->offset){
												// we have to be careful here, because if this is the first token (cursorPosition==0), we should NOT NULL the token!!!
												if(cursorPosition)pToken=pToken->prev;else pToken->type=TT_EXPRESSION;
											}
											writeBehindCursorText();
										}else
											switchToControlMode("Failed to move the cursor left.");
									}else
										beep();
								}else
									beep();
							}
						}
					}
					if(!newInputChar)continue;
					// update inputChar and inputCharacterType for further processing...
					inputCharacterType=INPUTCHARACTERTYPES[inputChar=newInputChar];
				}
				
				if(inputCharacterType=='o'){
					switchToControlMode(NULL);
					continue;
				} // MDH@22MAR2019 in certain situation we should NOT skip the remainder: else
			}

			// ASSERTION if we get here inputChar and inputCharacterType are available!!!!
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
					/* MDH@28MAR2019: if the user enters the comment character we should toggle the token type's highest bit (bit 7)
					if(inputCharacterType=='C'){
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
					int16_t newTokenType=(inputCharacterType!='W'?nextTokenType(pToken->type,inputCharacterType):-1); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
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
						if(pToken->type==TT_UNARY)pToken->significantCharacterCount=1;
						// TODO should we write the associated colors here?????
						outputTokenColor(pToken);
					}else
					if(inputCharacterType=='W'&&pToken->significantCharacterCount==0&&pToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
						pToken->significantCharacterCount=string_length(pToken->text);

					// insert the typed character at cursorPosition minus current token offset in pToken->text
					string_insert_char(pToken->text,cursorPosition-pToken->offset,inputChar);
#ifdef __DEBUG__
					printf("[%s]",string(pToken->text));
#endif
					commandLength++; // increment total command length
					putchar(inputChar); ///////// replacing: outputLastTokenChar(pToken); // echo the last token character
					
					if(assisting)output(":%c",inputCharacterType);

					debugWrite("Command length after inserting %c: %" PRIu16 ".",inputChar,commandLength);
					cursorPosition++; // increment the current cursor position

					writeBehindCursorText();
					debugWrite("Command length after writing behind cursor text: %" PRIu16 ".",commandLength);
					/* replacing:
					// write all remaining characters in the command after which we should return to the current position
					writeRestOfCommand();
					*/
				}else
					switchToControlMode("Switching to control mode, due to failing to create a new command!");
			}else{
				putchar(inputChar); // nice to see the character we typed...
				// might be paging through the commands
				if(!commandPage){ // not currently paging through the commands
					// an option character!!!
					if(inputChar=='x'||inputChar=='X')exit(0);else
					if(inputChar=='a'||inputChar=='A'){assisting=!assisting;output("\n%s\n>> ",(assisting?"Will assist!":"Will not assist!"));}
					if(inputChar=='d'||inputChar=='D'){debugging=!debugging;output("\n%s\n>> ",(debugging?"Will debug!":"Will not debug!"));}
					if(inputChar=='h'||inputChar=='H'){
						// are we showing the history 5 commands at a time, or 9 at a time? we want the user to be able to select a command quickly
						// we could call them a, b, c etc.
						if(commandCount){
							commandPages=1+(commandCount-1)/10;
							showNextCommandPage(); // as soon as commandPage>0 we are paging...
						}else
							output("%s\n","No previous commands to show.");
					}
				}else{
					// user might have selected one of the commands (letter a through j)
					commandPage=0; // stop paging
				}
			}
			if(commandInput)outputStatus();
		}
		// if eXit input character(s) received...
		if(inputCharacterType=='x')break;
		if(inputCharacterType=='n'){
			if(commandInput){ // the newline character ends the command to be evaluated!!
				resetOutputColor(); // prevent showing subsequent output in the wrong colors
				// if the command ended with a normal end-of-line character, evaluate and register the command
				// NOTE if pCommand is not set yet, but the user retrieved a previously executed command, that one should be reexecuted
				//      and registered (of course it will be pointing to the same chain of tokens but it might evaluated differently now)
				// NOTE we need to think what to do with pCommand in different situations
				//      only discard it when the command was evaluated and not registered
				Token* pCommandToEvaluate=NULL; // this would be the command to register if we succeed in evaluating it!!!
				if(pCommand){ // a current command being edited
					// MDH@22MAR2019: currently the first token is a WHITESPACE token
					if(cursorPosition>string_length(pCommand->text)){
						// finish the last token???
						if(pToken->significantCharacterCount==0)pToken->significantCharacterCount=string_length(pToken->text);
						pCommandToEvaluate=pCommand; // but only when not at start of command!!!
						if(debugging)outputTokenInfo();
					}
				}else{
					if(commandIndex)pCommandToEvaluate=commands[commandCount-commandIndex];
				}
				// if we succeeded in evaluating a command we should register it
				if(pCommandToEvaluate!=NULL){
					// typically the command will not evaluate if it is not complete
					// in which case we should allow the user to correct it or make it complete
					if(!evaluateCommand(pCommandToEvaluate))
						outputInfo("Failed to evaluate the command! Complete, correct or discard the command please.");
					else
					// ASSERTION command to evaluate 
					// if we fail to register the command we should attempt to get rid of the command (and the memory it occupies)
					if(!registerCommand(pCommandToEvaluate)){
						if(pCommand&&!clearCommand())
							switchToControlMode("Switching to control mode, due to failing to register and clear the command.");
						else
							output("\n%s","WARNING: Failed to register the evaluated command. Out of memory?");
					}else{ // command registered successfully, which means we have to keep its tokens (and not free them)
						pCommand=NULL; // pointer is stored in memory, so we can get rid of the current command pointer!!
						debugWrite("\n%s","Command pointer cleared.");
					}
				}else{
					output("\n%s","No command to evaluate.");
					if(pCommand&&!clearCommand())output("\nWARNING: %s","Failed to clear the command pointer.");
					pCommand=NULL;
				}
				/* MDH@16MAR2019: preparation for the next command is not required here, it's better to do that at the beginning
				                  of this outer loop
				// prepare for accepting the next command
				commandIndex=0; // MDH@16MAR2019: pretty essential otherwise it would keep evaluating previous commands
				if(pCommand) // if we still have a command to free, free it entirely
					if(!clearCommand())
						switchToControlMode("Switching to control mode, due to failing to remove the command.");
				*/
			}else // always to return to command input!!
				commandInput=true;
		}
	}
	// 'normal' exit
	exit(0);
}
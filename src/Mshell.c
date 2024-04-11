#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
// MDH@30JAN2024: for including math constants
#define _USE_MATH_DEFINES
#include <math.h>

#include "Mshell.h"

// we can define the module ids in an enumeration

// MDH@18MAY2020: every 'module' i.e. file should get a unique module id to be used for generating pointer ownership ids
static Mallocationowner getOwner(uint16_t id){return(Mallocationowner){MI_SHELL,id};}

Mvalue* NULL_value=NULL;
// prototype definition of getValueOfExpression() so we can call it from getValueOfList() and getValueOfMap()

// all the available constants go here...
const char M_PATH_SEPARATOR =
#ifdef _WIN32
							  '\\';
#else
							  '/';
#endif
const char* VALUETYPENAMES[]={"unknown","token","integer","big integer","decimal","rational","float","text","array","list","map","reference","function","environment","file","time"};
const char* const M_VARIABLE_NAME="M"; // MDH@14NOV2019: the variable to hold the list of remembered commands and the results they evaluated to
const char* const MFUNCTION_NAME="M"; // MDH@14NOV2019: the name of the function for getting previous results
const char* const IFFUNCTION_NAME="if";
const char* const WHILEFUNCTION_NAME="while";
const char* const FORFUNCTION_NAME="for";
const char* const FORWITHFUNCTION_NAME="forw";
const char* const DOFUNCTION_NAME="do"; // MDH@05AUG2019: the do function allowing the creation of variables local to the do execution
const char* const EVALFUNCTION_NAME="eval"; // MDH@28OCT2019: evaluating a text is nice
const char* const DEFINEUSERFUNCTION_NAME="defun"; // MDH@04MAR2020: the 'classic' approach is by defining a function with a fixed name which cannot be passed along
const char* const DEFINEANONYMOUSFUNCTION_NAME="function"; // MDH@04MAR2020: an anonymous function that is to be assigned to a variable/argument
const char* const MUTABLEVALUETYPECHARS="uoibdqrtalmrfe#$"; // the characters associated with each of the value types
const char* const IMMUTABLEVALUETYPECHARS="UOIBDQRTALMRFE#$"; // the characters associated with each of the value types
const char* const INFO_PREFIX=""; // MDH@27FEB2020: as for now NO actual info prefix text to use
const char* const M_ERROR_PREFIX="ERROR: "; // used in Mexecution.c as well (defined there as extern!!!)
const char* const M_WARNING_PREFIX="WARNING: "; // used in Mexecution.c as well (defined there as extern!!!)
const char* const M_BUG_PREFIX="BUG: "; // MDH@05NOV2019: for reporting bugs

const char* M_HIDDEN_VARIABLE_NAMES[]={"M","?","_"}; // MDH@14NOV2019: the variable names not to show when the variables are shown (with their current value)
const unsigned long long M_NUMBER_OF_HIDDEN_VARIABLES=3;// MDH@14NOV2019: yes, three of them

// MDH@31OCT2019: if the value of something equals the NULL value, this is the text to use to represent it, this is also the name of the NULL variable!!!
//				alternatively we could use capital letters to denote the variable, and lowercase to denote the value (which makes sense I suppose)
//				to prevent confusion it's best to use the same text for the value, otherwise they see 'null' as value and think they can use that to embed a NULL value!!!
//				OK the NULL value is displayed in the normal foreground color whereas the variable is displayed in another color (see showValueColored() for the coloring)
const char* const M_NULL_VALUE_TEXT="NULL"; // the text to represent values that are undefined...
const char* const M_NULL_VARIABLE_NAME="NULL";
const char* const M_UNDEFINED_VALUE_TEXT="UNDEFINED"; // the text to represent values that are undefined...
const char* const M_UNDEFINED_VARIABLE_NAME="UNDEFINED";

// MDH@02NOV2020: because lists can be very long, we only show a limited amount of elements at the start and end
const long long M_ARRAY_ELEMENTS_AT_START=50;
const long long M_ARRAY_ELEMENTS_AT_END=50;
const long long M_LIST_ELEMENTS_AT_START=50;
const long long M_LIST_ELEMENTS_AT_END=50;
const long long M_LL_INVALID=LLONG_MIN; // the invalid long long defaults to LLONG_MIN
// it's preferable if the allowed range of integer (long long) values, does not include LLONG_MIN
const long long M_LL_MIN=LLONG_MIN+1;
const long long M_LL_MAX=LLONG_MAX;
const long long M_FALSE=0;
const long long M_TRUE=1;
const long long M_ZERO=0;
const long long M_POSITIVE=1;
const long long M_NEGATIVE=-1;
//const enum BOOLEAN_ENUM {M_FALSE,M_TRUE};
//const enum SIGN_ENUM {M_NEGATIVE,M_ZERO,M_POSITIVE};
const long double M_LD_NAN=0.0/0.0; // or strtold("nan",NULL) would work as well
const long double M_LD_INF=1.0/0.0; // positive infinity
const long double M_LD_NEGINF=-1.0/0.0; // negative infinity
const long double M_LD_Q_EPS=1e-18; // this is the exact boundary to use for approximating 13/11 (which seems to be an notorious long double to approximate with rational (13/11)!!!)

// can't get the proper number of decimals using M_PI!!!
#ifdef M_PI
const long double M_LD_PI=(long double)M_PI;
////printf("PI is predefined.\n");
#else
const long double M_LD_PI=3.1415926535897932384626433832795028841971L; // 40 non-zero decimal digits of PI (before the first 0)
                      ///?3,14159265358979323851280895940618620443
                      ///?3,141592653589793238512808959406186
#endif

#ifdef M_E
const long double M_LD_E=(long double)M_E;
///////printf("E is predefined.\n");
#else
const long double M_LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E
#endif

long long M_DP=20; // the default decimal precision (initially 20) TODO should this be a constant after all?????????

const unsigned long long M_BITS_PER_ENV_LEVEL=8; // the minimum is 4 (to allow for a depth of 15 environments at the same time), the maximum is 60 of course in which case the maximum depth is 1, 8 gives a maximum depth of 7 and 256 at each level

const char M_WHITESPACE_CHARACTER=' '; // MDH@31OCT2019: let's use another character for storing whitespace in tokens (would normally be a blank)
const char M_ESCAPE_CHARACTER='\\'; // MDH@13OCT2020: the character to use to enter certain characters in text
// MDH@26OCT2020: if we map the Enter-key (which is essentially ASCII 10 (LF)) to the return key which is also invisible we can still use it
const char M_NEWLINE_CHARACTER='\r'; // MDH@31OCT2019: the character to request a newline with!!! # MDH@19OCT2020: I suppose using a character that will not be displayed is probably best!!!!
const char M_DEREFERENCE_CHARACTER='@'; // MDH@10MAR2020: better to define a constant to that purpose
const char M_PROPERTY_SEPARATOR_CHARACTER='.'; // MDH@12MAR2020: the separator between map and property
const char M_COMMAND_CONTINUATION_CHARACTER='`'; // MDH@28OCT2020: the only character unused left to continue a command because I couldn't use \ because that's the escape character in text

const char* const M_ADDITIONAL_FUNCTION_ARGUMENTS_VARIABLE_NAME="_";

const char * const M_LOCALE_SETTINGS_VARIABLE_NAME="LOCALE_SETTINGS";

const char* const M_ISO8601_FORMAT="%Y-%m-%dT%H:%M:%S";
const char* const M_ISO8601_UTC_FORMAT="%Y-%m-%dT%H:%M:%SZ";

// you can set the modules to debug here using the module masks as defined in Mmodule.h
unsigned long long M_MODULE_DEBUGGING=0; // MDH@05DEC2020: will be initialized in shellInitialized()

// as needed by the tokenizer (as part of evaluating a command)
// associated every possible input characters (0 through 127) with a character type where a period denotes a non-command input character
// t=tab(feedforward variable),n=newline(end of command),U=unary operator,D=double quoted string literal,C=comment,L=letter (in identifiers),l=letter (not at start of identifier)
// D=digit,d=digit (not at start of numeric value),e=the letter e which may be part of an 'extended' number (or represent the constant e)
// B=binary operator,b=binary operator that cannot be used as first binary operator character,A=assignment operator,
// E=starts an expression(a comma),e=ends and expression ( ) and ]), (NOTE: some characters are best represented by themselves
// all lowercase characters represent control characters, like t=tab, n=newline, x=escape control character,o=switch to control mode,d=delete,b=backspace
// O=operator that can be either unary or binary depending on its position (+ and - characters)
// use x for eXit (e.g. with Ctrl-C and Ctrl-Z), c for cancel command, and m for going into M (control) mode
// as for operators: there are 8 different groups of operators
// !	 not unary operator or first character of binary operator !=
// ~	 pure unary operator
// -+	sign unary operator or binary minus/plus operator
// %^	pure binary operator
// */	binary operator extensible to make ** power operator or // integer division operator
// <>	binary operator extensible to make << or >> operator but can also be followed by an = sign (is this not the same as */?)
// =	 assignment operator that can follow most of the binary operators (except < and >)
// |&	binary or and operator extensible to make || logical or or && logical and operator but the latter cannot be followed by =
// MDH@16APR2019: removing the o input character type (for switching explicitly to or from control mode), replacing it by n, so we can use the backtick for certain purposes...
//				in certain languages it means evaluate this (or the result of a system command??????)
//				furthermore we're combining operators to a single input character type: \^~% become %, /* become * and |& become &
// MDH@31OCT2019: let's use the backtick (`) as special whitespace character to use when one wants to insert a line break (i.e. continue the command on the next line)
//				although this would mean that it would show up when writing the tokens
//				we tried inserting a TT_WHITESPACE token with a backtick character (i.e. using ` as associated input character type) but ran into all kinds of problems so now we treat ` as W input character type
//				so it is appended to the current token, we only need to get it displayed in another color
//				ok, we're going to use \ for newline request character, so \ used to be % now becomes for type \ indicating a newline request (or escape character in a string!!!!)
//				switched to using the blank to indicate a newline request (using \ is a bit clumsy, backtick goes back to being the backtick, although no idea what we can use it for)
//				no we let \ be whitespace but we can turn it into a blank when it's a functional newline request
// MDH@04NOV2019: in order to be able to pass value references (i.e. variables) to a function we define @ as the redirection operator so that not the value but the value reference is returned (unresolved)
//				by defining @ as of type R we indicate that it refers to an identifier that has to be an existing variable!!!
//								-------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
// MDH@26OCT2020: all the i input characters can be associated with a macro, e.g. Ctrl-G (7) will insert get() into the command
// MDH@28OCT2020: type of \ changed from W to e (i.e. the escape character), see what I can do with that elsewhere
/**
 * @brief for each possible input character the associated type
 * 
 */
const char INPUTCHARACTERTYPES[]="iiiciiigdtniiriiiiiiiiiiiixmiiiiW!DCL%&S()*+,-.*NNNNNNNNNN:;>=>?RLLLLLLLLLLLLLLLLLLLLLLLLLL[e]%L LLLLELLLLLLLLLLLLLLLLLLLLL{&}~b";
//const char INPUTCHARACTERTYPES[]="iiiciiigdtniiriiiiiiiiiiiixmiiiiW!DCL%&S()*+,-./NNNNNNNNNN:;<=>?@LLLLLLLLLLLLLLLLLLLLLLLLLL[e]%L LLLLELLLLLLLLLLLLLLLLLLLLL{&}~b";
// replacing: const char INPUTCHARACTERTYPES[]="iiiciiiibtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-./NNNNNNNNNN:;<=>?@LLLLELLLLLLLLLLLLLLLLLLLLL[%]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{|}~b";

// MDH@24MAR2020 BUG FIX: needed to insert an additional "" for TT_PROPERTY which I forgot previously
// now we define all the state transitions i.e. what input character types result in which new token type
// NOTE this can be organized in many ways perhaps it's easiest to tell per input character what the transformation is
//	  only changes to the token type need to be registered, so if the change is NOT present, no need to put it in the transition table
//	  EWW means that when starting an expression any whitespace starts a whitespace token, we use * to indicate ALL possible input character types
//	  *WW means that any W character received in any state will result in a W state 
// we can make an array of transitions with each element corresponding to the character in TOKENTYPES, so the first entry contains all responses to E, the second entry the responses to W etc.
// it's easier to tell for any possible resulting token type which input character types will result in that type
// it's a hell of a job to create the token type transitions matrix
/* LEGEND:
   - signs are allowed in an EREAL but only directly behind the E, which means we have to somehow have an EREALEXPONENT element unless you treat this E as a binary operator which I think is a very good idea!!!
   - E stands for *10** so is this an assignable operator I suppose you could make it assignable as in 4e=3 to muliply by 1000, yes this look strange, as such . could also be considered an operator but Ok
	 E is Assignable e r u, so we can get rid of the EREAL token type!!!
*/
// MDH@16OCT2020: as we can use any character we like to represent NOT better to use ! instead of what we did before (the backtick `)
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","","","q","q","!D","!S","","","","","","","","LEN","","",""}; // MDH@30APR2019: oops one extra needed...

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
	TOKENTYPE(TT_ONE_CHAR_BINARY_=0b10100001)	  					?
	TOKENTYPE(TT_ONE_CHAR_ASSIGNABLE_BIANRY=0b10101010)  			= ~ ^ % \ -(bin) +(bin)
	TOKENTYPE(TT_TWO_CHAR_BINARY=0b10101110)	  					! (followed by =)
	TOKENTYPE(TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY=0b10101011)		& | (interesting =+= and &+= and |+= and itself)
	TOKENTYPE(TT_TWO_CHAR_ASSIGNABLE_BINARY=0b10111011)				< > * /
	printf("\nError										: %d.",TT_ERROR);
	printf("\nOne character unary operator				 : %d.",TT_ONE_CHAR_UNARY);
	printf("\nAssignment operator						  : %d.",TT_ASSIGNMENT);
	printf("\nOne character binary operator				: %d.",TT_ONE_CHAR_BINARY);
	printf("\nOne character assignable binary operator	 : %d.",TT_ONE_CHAR_ASSIGNABLE_BINARY);
	printf("\nTwo character binary operator				: %d.",TT_TWO_CHAR_BINARY);
	printf("\nTwo character once assignable binary operator: %d.",TT_TWO_CHAR_ONCE_ASSIGNABLE_BINARY);
	printf("\nTwo character assignable binary operator	 : %d.",TT_TWO_CHAR_ASSIGNABLE_BINARY);
	printf("\nComparison or shift operator				 : %d.",TT_COMPARISON_OR_SHIFT_BINARY);
*/
/* MDH@10APR2019: 
- some transitions only change the type but do not start a new token, but this is true for all binary operators, so I guess we can force that programmatically
- if we put ERROR at the end we do not need to add an array for dealing with error transitions (as we cannot leave an error!!)
*/
// operator input type characters: ! ~ + - % * < = | (8 different operator groups)
// ! ~ and + start a unary operator when a value is expected
// MDH@15APR2019: still to determine what to do with @ and ` (the latter for system commands????)
//				inserting macro's should also be possible somehow...
// MDH@05AUG2019: it's a pity that I need to allow a , behind a new variable in order to allow that when a do function call executes code after initializing these variables that are not yet recognized as created
//				we can solve this by remembering ALL variables when they are created in every expression that is tokenized, this would be possible by creating a tokenizing environment where we remember all created variables in in the tokenizing process
// MDH@04NOV2019: the reference token type added, so we can pass references to functions wrapped inside a value
// MDH@08JUL2023: if we allow comments everywhere we have to remove C from the last element in each TRANSITIONS element, but changing it into c is also possible, to make it acceptable
// MDH@18JUL2023: a comma behind an opening parenthesis should now also be allowed, therefore , was removed from the last element of EXPRESSION and added to the L_EL string
/* MDH@18FEB2024: replacing binary operators by 3 types:
   ONE_CHARACTER_BINARY: + - & | ^ E
	 ONE_AND_TWO_CHARACTER_BINARY: / * < > extendible to // ** << <> and >>
	 TWO_CHARACTER_BINARY: != !< !> == but also !! instead of == so that a user can always not use = in binary operators
	 we need to prevent binary operators to be over two characters programmatically!!!
	 we now have = for == / ! for != !< and !> / < / and * for << // and ** / < for << and <> / and all the one character bin operators
*/
// MDH@26MAR2024: adding token type TT_PLACEHOLDER that starts with ? 
/*
 "EXPR","UNA" ,"A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","REF" ,"VAR"  ,"NEWVAR","PROP" ,"L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
/**
 * @brief token type + input character type -> new token type
 * 
 */
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"?","C","` ; c  % )&*    > :	   ] }=  "}, /* EXPRESSION */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"" ,"" ,"" ,"" ,"[","" ,"" ,"" ,"" ,"","" ,"" ,"" ,"" ,"`R; CDS% )&*  , >?:	   ]{}=  "}, /* ONE CHARACTER UNARY !-+~ */ \
{"(","!-+~","" ,"=" ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"" ,"C","` ; c  % )&*  , >?:	   ] }   "}, /* ASSIGNMENT = */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ," ","" ,"C","`R; c  % )&*  , >?:	   ] }=e "}, /* Baeru finished bin.op. */ \
{""	,""    ,"" ,"=" ,""  ,""   ,"" ,""     ,"" ,""    ,""	   ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"","" ,"" ,"" ,"C","`R;!cDS%()&*+-,.>?:LEN[]{} e~"}, /* BaErU unfinished bin.op. */ \
{"(","!-+~","=",""  ,""  ,""   ,"" ,"R"    ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"" ,"" ,"" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"" ,"C","`R; cDS% )&*  , >?:	   ]   e "}, /* BAeRu assignable repeatable */ \
{"(","!-+~","" ,"=" ,""  ,""   ,"" ,"R"    ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"" ,"C","`R; c  % )&*  ,  ?:	   ]   e "}, /* BaERu comp. (<>) bin.op. */ \
{"(","!-+~","=",""  ,""  ,""   ,"" ,""     ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"" ,"C","`R; c  % )&*  , >?:	   ]   e "}, /* BAeru assignable bin.op. */ \
{"(","!-+~","=",""  ,""  ,""   ,"" ,""     ,"" ,""    ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"" ,"C","`R; c  % )&*  , >?:	   ]{} e "}, /* Taeru ternary op. (? only now) */ \
{""	,""    ,"" ,""  ,""  ,""   ,"" ,""     ,"" ,"LEN" ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"" ,"]","" ,"" ,"}","","" ,")","" ,"C","`R;! DS%( &*+-  >?:   [ { =e~"}, /* REFERENCE to an existing variable */ \
{""	,""    ,"=",""  ,"!" ,"&*" ,">","-+%e" ,"" ,""    ,"RLEN",""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","" ,"C","` ;  DS (			   ?      {   ~"}, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""	,""    ,"=",""  ,""  ,""   ,"" ,""     ,"" ,""    ,""    ,"LEN","."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,""  ,"}","","" ,"" ,"" ,"C","`R;! DS%()&*+-  >?:	    {  e~"}, /* NEW_VARIABLE (variable that does not exist yet) */ \
{""	,""	   ,"=",""  ,"!" ,"&*" ,">","-+%e" ,"" ,""    ,""    ,""	  ,"RLEN.",",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","" ,"C","` ;  DS (		  	 ?      {   ~"}, /* PROPERTY (identifier starting with the property separator) */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","]","{","" ,"" ,"","" ,"" ,"?","C","` ; c  % )&*	  > :	     }=e "}, /* LIST ELEMENT (similar to expression) */ \
{";",""    ,"" ,":" ,"!=","&*" ,">","-+%eE","" ,""    ,""    ,""	  ,""     ,",","N"  ,"."   ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","" ,"C","`R   DS (		     ? L  [ {   ~"}, /* INTEGER: (signless) list of digits */ \
{";",""    ,"" ,":" ,"!=","&*" ,">","-+%eE","" ,""    ,""    ,""	  ,""     ,",",""   ,"N"   ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","" ,"C","`R   DS (	     . ? L  [ {   ~"}, /* REAL: part behind a decimal period */ \
{""	,""    ,"" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,""    ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"D","" ,"" ,"" ,"" ,"" ,"" ,"","" ,"" ,"" ,"" ,""						   }, /* DQSTRING: double quoted string */ \
{""	,""    ,"" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,""    ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"" ,"S","" ,"" ,"" ,"" ,"" ,"","" ,"" ,"" ,"" ,""						   }, /* SQSTRING: single quoted string */ \
{";",""    ,"" ,"+" ,"!=","&"  ,">",""     ,"" ,""    ,""    ,""	  ,""     ,",",""   ,""    ,"D","S","" ,"" ,"" ,"]","" ,":","}","","" ,")","" ,"C","`R   DS%&( * - . ? LEN[ {  e~"}, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";",""    ,"" ,"+" ,"!=","&"  ,">",""     ,"" ,""    ,""    ,""	  ,""     ,",",""   ,""    ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","" ,"C","`R   DS%&( * - . ? LEN[ {  e~"}, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,",","N"  ,""    ,"D","S","" ,"" ,"[","]","{","" ,"" ,"","" ,")","?","C","` ; c  %& )*   .> :   	 }=e "}, /* LIST: [ starts a list */ \
{";",""    ,"=",""  ,"!" ,"&*" ,">","-+%e" ,"" ,""    ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","" ,"C","`R   DS  (		   ? LEN  {   ~"}, /* END_OF_LIST: behind ] that ends a list */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,"" ,"N"  ,""    ,"D","S","" ,"" ,"[","" ,"" ,"" ,"}","","" ,")","" ,"C","` ; c  %& )*  ,.>?:    ]{ =e "}, /* MAP: { starts a map */ \
{"(","!-+~","" ,""  ,""  ,""   ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,")","" ,"C","` ; c  %& )*  , >?:	   ] }=e "}, /* MAP_VALUE: : starts a map value */ \
{";",""    ,"" ,""  ,"!=","&*" ,">","+"    ,"" ,""    ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,"" ,"}","","" ,")","" ,"C","`R   DS% (   -	 ?:LEN  {  e~"}, /* END_OF_MAP: behind } that ends a map */ \
{""	,""    ,"" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,""    ,""	  ,"."    ,"" ,""   ,""    ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"","(","" ,"" ,"C","`R;!cDS%& )*+-, >?:   []{}=e~"}, /* FUNCTION: some identifier recognized as function name */ \
{"(","!-+~","" ,""  ,""  ,"" 	 ,"" ,""     ,"" ,"R"   ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,")","?","C","` ; c  %&  *	  > :	   ] }=e "}, /* FUNCTION_CALL ( following the name of a function */ \
{";",""    ,"" ,":" ,"!=","&*" ,">","-+%eE","" ,""    ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","" ,"C","`R   DS  (		   ? L N  {   ~"}, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
{"" ,""    ,"" ,""  ,""  ,""   ,"" ,""     ,"" ,""    ,""    ,""	  ,""     ,",",""   ,""    ,"" ,"" ,"" ,"" ,"" ,"]","" ,"" ,"" ,"","" ,")","" ,"C","`R;!cDS%( &*+- .>?:LEN[ {}=e~"}, /* TT_PLACEHOLDER ? */ \
};
// MDH@23FEB2024: the following is an experimental transitions that was supposed to make all binary operators assignable
///const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","` ; c  % )&*    <>?:	  ] }="}, /* EXPRESSION */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"" ,"" ,"" ,"" ,"[","" ,"" ,"" ,"" ,"","" ,"" ,"" ,"`@; CDS% )&*  , <>?:	  ]{}="}, /* ONE CHARACTER UNARY !-+~ */ \
{"(","!-+~","" ,"=","" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","` ; c  % )&*  , <>?:	  ] }" }, /* ASSIGNMENT = */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ," ","C","`@; c  % )&*  , <>?:	  ] }="}, /* Baeru finished bin.op. */ \
{""	,""    ,"" ,"=","" ,""   ,""  ,""      ,"" ,""   ,""	  ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"","" ,"" ,"C","`@;!cDS%()&*+-,.<>?:LEN[]{}"}, /* BaErU unfinished bin.op. */ \
{"(","!-+~","=","" ,"" ,""   ,""  ,"@"     ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"" ,"" ,"" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","`@; cDS% )&*  , <>?:	  ]"   }, /* BAeRu assignable repeatable */ \
{"(","!-+~","" ,"=","" ,""   ,""  ,"@"     ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","`@; c  % )&*  ,   ?:	  ]"   }, /* BaERu comp. (<>) bin.op. */ \
{"(","!-+~","=","" ,"" ,""   ,""  ,""      ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","`@; c  % )&*  , <>?:	  ]"   }, /* BAeru assignable bin.op. */ \
{"(","!-+~","=","" ,"" ,""   ,""  ,""      ,"" ,""   ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,"" ,"C","`@; c  % )&*  , <>?:	  ]{}" }, /* Taeru ternary op. (? only now) */ \
{""	,""    ,"" ,"" ,"" ,""   ,""  ,""      ,"" ,"LEN",""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"" ,"]","" ,"" ,"}","","" ,")","C","`@;! DS%( &*+-  <>?:   [ { ="}, /* REFERENCE to an existing variable */ \
{""	,""    ,"=","" ,"!","*/>","<","-+&:%^" ,"?",""   ,"@LEN",""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","C","` ;  DS (			           {  "}, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""	,""    ,"=","" ,"" ,""   ,""  ,""      ,"" ,""   ,""    ,"LEN","."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,"" ,"}","","" ,"" ,"C","`@;! DS%()&*+-  <>?:	   {  "}, /* NEW_VARIABLE (variable that does not exist yet) */ \
{""	,""	   ,"=","" ,"!","*/>","<","-+&:%^" ,"?",""   ,""    ,""	  ,"@LEN.",",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","C","` ;  DS (			           {  "}, /* PROPERTY (identifier starting with the property separator) */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","]","{","" ,"" ,"","" ,"" ,"C","` ; c  % )&*	  <>?:	    }="}, /* LIST ELEMENT (similar to expression) */ \
{";",""    ,"" ,"=","!","*/>","<","-+&:%^E","?",""   ,""    ,""	  ,""     ,",","N"  ,"."   ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","C","`@   DS (		        L  [ {  "}, /* INTEGER: (signless) list of digits */ \
{";",""    ,"" ,"=","!","*/>","<","-+&:%^E","?",""   ,""    ,""	  ,""     ,",",""   ,"N"   ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","C","`@   DS (	     .    L  [ {  "}, /* REAL: part behind a decimal period */ \
{""	,""    ,"" ,"" ,"" ,""   ,""  ,""      ,"" ,""   ,""    ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"D","" ,"" ,"" ,"" ,"" ,"" ,"","" ,"" ,"" ,""						   }, /* DQSTRING: double quoted string */ \
{""	,""    ,"" ,"" ,"" ,""   ,""  ,""      ,"" ,""   ,""    ,""	  ,""     ,"" ,""   ,""    ,"" ,"" ,"" ,"S","" ,"" ,"" ,"" ,"" ,"","" ,"" ,"" ,""						   }, /* SQSTRING: single quoted string */ \
{";",""    ,"" ,"=","!","*/" ,"<","+&"     ,"?",""   ,""    ,""	  ,""     ,",",""   ,""    ,"D","S","" ,"" ,"" ,"]","" ,":","}","","" ,")","C","`@   DS%&( * - .    LEN[ {  "}, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";",""    ,"" ,"=","!","*/" ,"<","+&"     ,"?",""   ,""    ,""	  ,""     ,",",""   ,""    ,"" ,"" ,"" ,"" ,"" ,"]","" ,":","}","","" ,")","C","`@   DS%&( * - .    LEN[ {  "}, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,",","N"  ,""    ,"D","S","" ,"" ,"[","]","{","" ,"" ,"","" ,")","C","` ; c  %& )*   .<>?:	    }="}, /* LIST: [ starts a list */ \
{";",""    ,"=","=","!","*>" ,"<","-+&:%^E","?",""   ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","C","`@   DS  (		      LEN  {  "}, /* END_OF_LIST: behind ] that ends a list */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,"" ,"N"  ,""    ,"D","S","" ,"" ,"[","" ,"" ,"" ,"}","","" ,")","C","` ; c  %& )*  ,.<>?:	  ]{ ="}, /* MAP: { starts a map */ \
{"(","!-+~","" ,"" ,"" ,""   ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,"" ,"N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,")","C","` ; c  %& )*  , <>?:	  ] }="}, /* MAP_VALUE: : starts a map value */ \
{";",""    ,"" ,"=","!","*>" ,"<","+"      ,"?",""   ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,"" ,"}","","" ,")","C","`@   DS% (   -	   :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""	,""    ,"" ,"" ,"" ,""   ,""  ,""      ,"" ,""   ,""    ,""	  ,"."    ,"" ,""   ,""    ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"","(","" ,"C","`@;!cDS%& )*+-, <>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"(","!-+~","" ,"" ,"" ,"" 	 ,""  ,""      ,"" ,"@"  ,"LE"  ,""	  ,""     ,",","N"  ,"."   ,"D","S","" ,"" ,"[","" ,"{","" ,"" ,"","" ,")","C","` ; c  %&  *	  <>?:	  ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{";",""    ,"" ,"=","!","*/>","<","-+&:%^E","?",""   ,""    ,""	  ,"."    ,",",""   ,""    ,"" ,"" ,"" ,"" ,"[","]","" ,":","}","","" ,")","C","`@   DS  (		      L N  {  "}, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

/**
 * @brief the token type ids
 * 
 */
const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES]={0,0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0b10000000,0b11111111};

// MDH@25FEB2021: in certain cases we need to register local variables
/**
 * @brief register the variables in @p variableMap in M environment \p environment owned by \p owner_environment
 * 
 * @param environment 
 * @param owner_environment 
 * @param variableMap 
 * @param defaultVariableName 
 * @return true on success
 * @return false on failure
 */
bool registerVariables(Menvironment * environment,Mallocationowner owner_environment,Mmap const * const variableMap,char const * const defaultVariableName){
	if(NULL==environment)return false;
	Mmapelement* variableMapelement=(variableMap!=NULL?variableMap->_first:NULL);
	Mvariable* variableMapelementVariable;
	while(variableMapelement!=NULL){
		variableMapelementVariable=variableMapelement->_variable;
		if(variableMapelementVariable!=NULL&&variableMapelementVariable->_name!=NULL){
			char *variableName=variableMapelementVariable->_name->chars;
			if(variableName!=NULL){
				if(strlen(variableName)==0)if(defaultVariableName!=NULL)variableName=defaultVariableName; // use the default variable name if the name of the variable is empty
				// if(strlen(variableName)>0){
					// NOTE the map element variable name seems to be enclosed in quotes, and should be dequoted unless we do that when the argument map is created
					if(!addVariable(environment,owner_environment,variableName,variableMapelementVariable->valuetype,false)){
						output("%sFailed to add variable '%s' as local variable.\n",M_ERROR_PREFIX,variableName);
						return false;
					}
					if(!setValue(environment,variableName,variableMapelementVariable->_value)){
						output("%sFailed to initialize local variable '%s'.\n",M_ERROR_PREFIX,variableName);
						return false;
					}
					if(amVerboseDebugging())
						output("'%s' registered!\n",variableName);
				// }
			}
		}
		variableMapelement=variableMapelement->_next;
	}
	return true;
}

// MDH@22OCT2020: in order to be able to use any number of function arguments we now allow moving the list of variables that does not have a name to be placed in the variable that starts with _
//				it's up to the argument map creator to put all arguments that are not expected in the function and put them in the '' argument
/**
 * @brief initializes execution environment \p _executionEnvironment owned by \p owner_executionEnvironment with the variables from \p _variableMap
 * @details delegates to registerVariables()
 * @param _executionEnvironment 
 * @param owner_executionEnvironment 
 * @param _variableMap 
 * @param defaultVariableName 
 * @return true 
 * @return false 
 */
bool isExecutionEnvironmentInitialized(Menvironment* _executionEnvironment,Mallocationowner owner_executionEnvironment,Mmap* _variableMap,char const * const defaultVariableName){
	if(amVerboseDebugging())
		outputMap("Execution environment variable map: ",_variableMap,".\n");
	return registerVariables(_executionEnvironment,owner_executionEnvironment,_variableMap,defaultVariableName);
}
/*
\brief returns the environment for executing the the function called \p functionName
\p functionName the name of the function to execute
obviously when defining the function body there will be no commands to execute
 */
/**
 * @brief returns the new M environment for executing M function \p _function called \p functionName
 * 
 * @param _function 
 * @param functionName 
 * @param _argumentMap 
 * @return Menvironment* 
 */
Menvironment* _getFunctionExecutionEnvironment(Mfunction* _function,char* functionName,Mmap* _argumentMap){Mallocationowner owner=getOwner(__LINE__);
	// 1. create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values
	if(amVerbose())
		outputMap("Function execution argument map: ",_argumentMap,".\n");
	Menvironment* _functionExecutionEnvironment=owned_environment(_getNewEnvironment(),owner); // free asap
	if(_functionExecutionEnvironment!=NULL){
		if(amVerboseDebugging())
			outputInfo("Registering the name of the function execution environment");
		_functionExecutionEnvironment->_name=owned_chars(_getChars(functionName),Msubowner(owner,1)); // store the name of the function as environment name!!!
		/* NO, instead, just before popping the function body execution environment, we copy the function map reference
		// MDH@20JUL2019: this is fun, we're referencing the internal functions defined in the user function, and as we never free the functions
		//				we do not need to distinguish between the originals and the references (so we never loose the referenced functions
		//				when an execution environment is freed)
		_functionExecutionEnvironment->_functionMap=_function->functionunion._userfunction->_functionMap;
		*/
		// 2. make the definition environment the parent of the function execution environment
		assignValue(&_functionExecutionEnvironment->_parent,_function->_definitionEnvironmentValue); // MDH@03FEB2020 replacing: _functionExecutionEnvironment->_parent=_function->_definitionEnvironment;
		if(amVerboseDebugging())
			outputInfo("Parent of function execution environment set to the function definition environment");
		// 3. create the argument map fields as variables in the function execution environment
		bool functionExecutionEnvironmentInitialized=isExecutionEnvironmentInitialized(_functionExecutionEnvironment,owner,_argumentMap,M_ADDITIONAL_FUNCTION_ARGUMENTS_VARIABLE_NAME);
		if(functionExecutionEnvironmentInitialized){
			if(amVerboseDebugging())
				outputInfo("Function execution environment initialized");
			// add the function itself variable $_ so a function can call itself without knowing the name is is stored under or passed elsewhere
			if(!addVariable(_functionExecutionEnvironment,owner,"$_",VT_FUNCTION,true)||!setValue(_functionExecutionEnvironment,"$_",_getValueOfFunction(_function))){
				outputError("Failed to add the this variable to the function execution environment");
				functionExecutionEnvironmentInitialized=false;
			}else
			// add the result variable ($ or perhaps later a variable with empty name????) TODO make a predefined constant char* out of it
			if(!addVariable(_functionExecutionEnvironment,owner,"$",VT_UNDEFINED,false)){
				outputError("Failed to add the result variable to the function execution environment");
				functionExecutionEnvironmentInitialized=false;
			}else // also add the function exit flag variable (with name ! which cannot be set in the code because it is an invalid name)
			/* MDH@10JAN2021: no need to use the ! exit flag variable anymore (now replaced by the immutable flag of the result variable)
			if(!addVariable(_functionExecutionEnvironment,owner,"!",VT_UNDEFINED,false)){
				outputError("Failed to add the exit flag variable to the function execution environment");
				functionExecutionEnvironmentInitialized=false;
			}else // MDH@22OCT2020: fail-through code that will add a list that would normally contain the additional arguments in a function call which we force to be present always this way
			*/
			if(!addVariable(_functionExecutionEnvironment,owner,M_ADDITIONAL_FUNCTION_ARGUMENTS_VARIABLE_NAME,VT_LIST,false))
				outputInfo("Failed to add the additional arguments list variable to the function execution environment");
		}else
			outputError("Failed to initialize the function execution environment.");
		if(functionExecutionEnvironmentInitialized)return disowned_environment(_functionExecutionEnvironment,owner);
		// ASSERT function execution environment NOT initialized
		FREE_ENVIRONMENT(_functionExecutionEnvironment,owner);
	}
	return NULL;
}

/* moved back to M.c
void outputTokenTypeColor(TokenType tokenType){
	setBackColor(getBackgroundColor());
	setColor(getTokenColor(tokenType));
}
void outputTokenColor(Mtoken* _token){
	if(_token)outputTokenTypeColor(_token->type);
	///////printf("[%d]",_userInputCommand->_lastToken->type);
	// ah, the token colors will be a problem with the new type definitions, I suppose we need to distinguish between the operator and non-operator tokens	
}
*/
// MDH@04MAR2020: delegating displaying the output command to a callback that can be changed
/*
void setOutputCommandInfoFunction(OutputCommandInfoFunction* _outputCommandInfoFunction){
	outputCommandInfoFunction=_outputCommandInfoFunction;
}
*/

/**
 * @brief the function to read a single input character
 * 
 */
static InputCharReadFunction* inputCharReadFunction=NULL;

// MDH@04MAR2020: the default version outputs the command the same way as within a session except without the colors
/**
 * @brief outputs M command \p command
 * 
 * @param command 
 */
void outputCommandInfo(Mcommand const * const command){
	if(NULL==command||NULL==command->_lastToken)return;
	// MDH@12AUG2019: identifiers first
	Mtoken* identifierToken=command->_lastToken->prevIdentifier;
	if(identifierToken!=NULL){
		output("%s","Identifiers:");
		while(1){
			//if(identifierToken==TT_VARIABLE||identifierToken==TT_NEW_VARIABLE){
				// all identier tokens with argument equal to 1 should be considered new, if not it is a bug
				output(" %s",string(identifierToken->text));
				output("(%u)",identifierToken->offset);
			//}
			identifierToken=identifierToken->prevIdentifier;
			if(NULL==identifierToken)break;
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
		if(token->expr!=NULL)
			output("\n%s\t%u\t%s\t%s\t%-24s\n"," part of",token->expr->offset,"","",TOKENTYPE_STRING[token->expr->type]);
		else
			output("\t%s\n","Not part of another expression!");
		if(token->prevIdentifier!=NULL)
			output("%s\t%u\t%s\t%s\t%-24s\n"," points to",token->prevIdentifier->offset,"","",TOKENTYPE_STRING[token->prevIdentifier->type]);
		token=token->next;
	}
}
/**
 * @brief the function to output command info
 * 
 */
static OutputCommandInfoFunction* outputCommandInfoFunction=outputCommandInfo;

/**
 * @brief frees the M token \p _token owned by \p owner_token
 * @return the predecessor token of \p _token 
 */
static Mtoken* freeToken(Mtoken* _token,Mallocationowner owner_token){
	// MDH@30APR2019: let's delegate to FREE_TOKEN()
	Mtoken* _prevToken=NULL;
	if(_token!=NULL){_prevToken=_token->prev;FREE_TOKEN(_token,owner_token);}
	return _prevToken;
}

// MDH@25FEB2021: a helper function that can be called both for testing the validity of a command or a part of a command given the first and last token
//				taken as is from the original isValidCommandIndicator() (see below)
/**
 * @brief determines and returns the indicator of validity of M token \p lastCommandToken 
 * @details returns 0 when \p lastCommandToken is NULL
 *          returns -1 when \p lastCommandToken is of type TT_ERROR
 *          returns -2 when \p lastCommandToken is some sort of operator (with token type <= 8)
 *          returns -3 when a list is not ended
 *          returns -4 when a function call is not ended
 *          returns -5 when a map is not ended
 *          returns -6 when an unknown expression is not ended
 *          returns -7
 *          returns -8
 *          returns -9
 *          returns -10
 *          returns -11
 *          returns -12
 * @param lastCommandToken 
 * @param expressionTokenTypeToIgnore 
 * @param report 
 * @return int8_t 1 on success, a non positive integer on failure
 */
static int8_t isAValidLastCommandTokenIndicator(Mtoken const * const lastCommandToken,TokenType expressionTokenTypeToIgnore,bool report){

	if(NULL==lastCommandToken){if(report)outputError("Empty command");return 0;}
	
	// 3. any command always has two significant tokens TODO could compare _userInputCommand->_firstToken with _userInputCommand->_lastToken which should be different!!!
	//	in this case we clear the command, so that the command won't be repeated, and the user can switch to control mode immediately with the Enter key!!
	/// TODO fix: if(firstCommandToken==lastCommandToken->expr){if(report)outputError("Empty command");/*clearCommand(firstCommandToken);*/return false;} // TODO do we need clearCommand() here at all???????

	// MDH@28FEB2020: if the current last command token is an error do NOT remove, but let the caller handle it!!!!
	if(lastCommandToken->type==TT_ERROR){if(report)outputError("Command is erroneous.");return -1;}
	/* replacing:
	// 2. if the last token is an error, can't evaluate (well, better not)
	// TODO it makes sense to remove the error token
	if(lastCommandToken&&lastCommandToken->type==TT_ERROR){unfinishToken(lastCommandToken);lastCommandToken=removedLastCommandToken(command);return false;}

	{if(report)outputError("Can't evaluate erroneous command");if(!removedLastCommandToken(command)){if(report)outputError("Failed to remove the error");}unfinishToken(lastCommandToken);return false;}

	lastCommandToken=command->_lastToken;
	*/

	// 3. if the last token is an operator of sorts the command is incomplete
	if(lastCommandToken->type<=8){if(report)outputError("Value behind operator at end of command missing");return -2;}

	// MDH@03MAY2019: this is new, if expr is not NULL apparently we have missing parentheses!!!!
	//				BUT given that the first token always is of type TT_EXPRESSION and the last token will be pointing to it when complete we'd have to check for that too
	//					this actually means that if expr is NULL there's one parentheses too many!!!
	/*
	if(!_userInputCommand->_lastToken->expr){outputError("Too many parentheses!");return false;}
	if(_userInputCommand->_lastToken->expr!=_userInputCommand->_firstToken){outputError("Not enough parentheses!");return false;}
	*/
	// MDH@22MAY2019: the following is complex because we might be right behind the closing of a list, map or function call, in which case the command is still complete!!!
	// MDH@27MAY2019: the last token should now either point to the first token in the command, or to something that does point to the first token in the command
	//////////// already noticed while entering the expression!!!!: if(!_userInputCommand->_lastToken->expr){outputError("Too many parentheses!");return false;}
	Mtoken* expressionToken=lastCommandToken->expr; // the token pointed to by the last command token
	if(expressionToken!=NULL)
		if(lastCommandToken->type==TT_END_OF_LIST||lastCommandToken->type==TT_END_OF_FUNCTION_CALL||lastCommandToken->type==TT_END_OF_MAP)
			expressionToken=expressionToken->expr;
	if(expressionToken!=NULL){ // could be a problem
		// MDH@16OCT2019: I made ] ) and } again point to the associated [ ( and {, which of course should be pointing to NULL if it does not the command is incomplete
		/*
		if(amVerbose())
			if(report)
				output("First token in last expression pointed to: '%s' of type '%s' at offset '%" PRIu16 "'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],expressionToken->offset);
		*/
		/*
		TokenType expressionTypeToIgnore=TT_EXPRESSION;
		if(expressionTypesToIgnore){
			int expressionTypeToIgnoreIndex=0;
			while(expressionTypesToIgnore[expressionTypeToIgnoreIndex]!=expressionToken->type&&expressionTypesToIgnore[expressionTypeToIgnoreIndex]!=TT_EXPRESSION)expressionTypeToIgnoreIndex++;
			expressionTypeToIgnore=expressionTypesToIgnore[expressionTypeToIgnoreIndex];
		}
		*/
		// if we're not supposed to ignore this expression type, check it
		if(expressionToken->type!=expressionTokenTypeToIgnore)
		switch(expressionToken->type){
			case TT_LIST:{outputError("Missing end of list");return -3;}
			case TT_FUNCTION_CALL:{if(report)outputError("Missing end of function call");return -4;}
			case TT_MAP:{if(report)outputError("Missing end of map");return -5;}
			default:
				{
					if(report)output("%sUnknown expression with first token of type %s left unfinished.\n",M_ERROR_PREFIX,TOKENTYPE_STRING[expressionToken->expr->type]);
					return -6;
				}
		}
		/* replacing:
		// MDH@23JUL2019: we can now be very strict
		//				the last token should point to the first expression which only contains whitespace, whereas all other expression tokens start with ()
		if(_userInputCommand->_lastToken->expr->type!=TT_EXPRESSION||(string_length(_userInputCommand->_lastToken->expr->text)&&string_char(_userInputCommand->_lastToken->expr->text,0)!=' ')){
			outputError("Incomplete command");
			return false;
		}
		*/
		/* replacing:
		// this is allowed if this token ends something that points to NULL
		if((_userInputCommand->_lastToken->type!=TT_END_OF_LIST&&_userInputCommand->_lastToken->type!=TT_END_OF_FUNCTION_CALL&&_userInputCommand->_lastToken->type!=TT_END_OF_MAP)||_userInputCommand->_lastToken->expr->expr){
			switch(_userInputCommand->_lastToken->expr->expr->type){
				case TT_LIST:outputError("Missing end of list.");break;
				case TT_FUNCTION_CALL:outputError("Missing end of function call!");break;
				case TT_MAP:outputError("Missing end of map!");break;
				default:outputError("Not enough parentheses.");break;
			}
			return false;
		}
		*/
	}

	// 4. can't end with function of function call
	// MDH@20JUL2019: BUT we can treat the function as (new) variable, although new variables should not occur at the end of a command???
	if(lastCommandToken->type==TT_FUNCTION){if(report)outputError("Function call missing at end of command");return -7;}
	if(lastCommandToken->type==TT_FUNCTION_CALL){if(report)outputError("Unfinished function call");return -8;}
	if(lastCommandToken->type==TT_LIST||lastCommandToken->type==TT_LISTELEMENT){if(report)outputError("Unfinished list");return -9;}
	if(lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING){if(report)outputError("Unfinished string literal");return -10;}
	if(lastCommandToken->type==TT_EXPRESSION){if(report)outputError("Unfinished expression");return -11;}
	if(lastCommandToken->type==TT_MAP||lastCommandToken->type==TT_MAP_VALUE){if(report)outputError("Unfinished map");return -12;}
	
	return 1;
}

// keep track of the state of entering a command
// MDH@01OCT2019: result booled, but TODO can removeToken() fail??????
// MDH@28FEB2020: we NO longer NULL Mcommand* (we can't because that would require Mcommand**) BUT that would only be required 
//				I suppose this also means that we do not need to return true or false anymore, any caller can check for a last token itself (i.e. an empty command!!!!)
//				now returning the new last command token
/**
 * @brief removes and returns the last M token in M command \p command owned by \p owner_command
 * 
 * @param command 
 * @param owner_command 
 * @return Mtoken* 
 */
Mtoken* removedLastCommandToken(Mcommand* command,Mallocationowner owner_command){
	// NOTE we can still remove the pointer although you cannot use it anymore (except for testing) because free_token would have released the associated memory!!!
	if(command&&command->_lastToken){
		command->_lastToken=freeToken(command->_lastToken,Msubowner(owner_command,1)); // MDH@28FEB2020: used to be removeLastUserInputCommandToken
		if(command->_lastToken)command->_lastToken->next=NULL;
		else command->_firstToken=NULL; // MDH@20FEB2020 ADDITION: it makes sense to NULL _firstToken if _lastToken is NULL
	}
	return(command?command->_lastToken:NULL);
}

/**
 * @brief returns the validity indicator (positive on success) of M command \p command owned by \p owner_command
 * @details cuts off any last command token that is a comment before returning the validity of the last command token
 * @param command 
 * @paramXX owner_command 
 * @param report 
 * @return int8_t 
 */
int8_t isAValidCommandIndicator(Mcommand const * const command/*,Mallocationowner owner_command*/,bool report){
	// MDH@28JUN2023: SHOULD NOT CHANGE command, for now we can solve this by only allowing removal when report is true
	// 1. if no command nothing evaluated TODO don't call when this is the case though
	if(NULL==command||NULL==command->_firstToken)return 0; // replacing: {if(report)outputError("Undefined or empty command");return 0;}
	Mtoken* lastCommandToken=command->_lastToken;
	// MDH@11JUL2023: changed if into while to 'remove' all trailing comments
	while(lastCommandToken!=NULL&&lastCommandToken->type==TT_COMMENT){
		/* MDH@11JUL2023: probably best to never remove a comment last token
		if(report)
			lastCommandToken=removedLastCommandToken(command,owner_command);
		else*/
			lastCommandToken=lastCommandToken->prev;
	}
	// MDH@25FEB2021: inspecting the last command token now delegated to isAValidLastCommandTokenIndicator()!
	return isAValidLastCommandTokenIndicator(lastCommandToken,TT_EXPRESSION,report);
}
// if a sequence of tokens needs to be evaluated to a value, call getCommandValue()
/**
 * @brief evaluates \p command owned by \p owner_command returning the result
 * 
 * @param command 
 * @paramXX owner_command 
 * @param commandType 
 * @return Mvalue* the result of the evaluation of \p command
 */
static Mvalue* getCommandValue(Mcommand const * const command/*,Mallocationowner owner_command*/,char commandType){
	if(NULL==command)return NULL;
	// if(amVerboseDebugging())
		if(outputCommandInfoFunction)(*outputCommandInfoFunction)(command); // MDH@04MAR2020: using the given output command info function
	int8_t aValidCommandIndicator=isAValidCommandIndicator(command/*,owner_command*/,amVerboseDebugging());
	if(aValidCommandIndicator<=0)return NULL;
	getExecutionEnvironment()->expressionToken=command->_firstToken->next; // prepare the current environment for executing the command
	if(amVerboseDebugging())
		outputInfo("Evaluating...");
	return getValueOfExpression(getExecutionEnvironment()->_name->chars,commandType,(TokenType[]){},0);
}

// decimal stuff
/* MDH@20JUN2019: by not using DP_value anymore, we solved the problem of DP_value holding a reference to the decimal precision value which apparently was released at some point
				  so as soon as the value pointer is released by the value list, a reference is still kept by DP_value but the memory will be reused and _integer might point into uncharted territory at some point in the future
				  if we were to keep using DP_value we should have called assignValue() to assign the value and not DP_value=_getIntegerValue() (see initEnvironment())
Mvalue* DP_value=NULL; 
*/
/**
 * @brief the global default decimal context
*/
Mdecimalcontext* M_DECIMALCONTEXT=NULL; // the application-wide decimal context
/**
 * @brief returns the precision of the default decimal context
 * 
 * @return long long the precision of the default decimal context
 */
long long getDP(){
	if(!M_DECIMALCONTEXT)M_DECIMALCONTEXT=getDecimalcontext(M_DP); // _decimalContext won't be created until it's actually needed (so other decimal contexts might be created before!!!!!)
	// better to get it directly out of the _decimalContext (as that holds the actual decimal context being used)
	long long dp=(M_DECIMALCONTEXT?M_DECIMALCONTEXT->mpd_context->prec:M_LL_INVALID); // replacing: long long dp=(DP_value?DP_value->value._integer->ll:M_LL_INVALID);
	if(dp==M_LL_INVALID)outputBug("No default decimal context active!");
	return dp;
}
// MDH@18OCT2019: if someone wants to know about the decimal context
/**
 * @brief returns the decimal context of the decimal wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the decimal context information of the decimal wrapped in \p value
 */
Mvalue* getdc(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(value&&value->type==VT_DECIMAL){
		Mdecimalcontext* decimalcontext=getDecimalcontext(value->value._decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext!=NULL?decimalcontext->mpd_context:NULL);
		if(mpd_context!=NULL){
			Mmap* _contextMap=owned_map(_getMapOfType(VT_INTEGER),owner);
			if(_contextMap!=NULL){
				appendedToMap(_contextMap,owner,"status",_getIntegerValue(mpd_context->status));
				appendedToMap(_contextMap,owner,"precision",_getIntegerValue(mpd_context->prec));
				appendedToMap(_contextMap,owner,"round",_getIntegerValue(mpd_context->round));
				appendedToMap(_contextMap,owner,"exponentminimum",_getIntegerValue(mpd_context->emin));
				appendedToMap(_contextMap,owner,"exponentmaximum",_getIntegerValue(mpd_context->emax));
				appendedToMap(_contextMap,owner,"allcr",_getIntegerValue(mpd_context->allcr));
				appendedToMap(_contextMap,owner,"clamp",_getIntegerValue(mpd_context->clamp));
				appendedToMap(_contextMap,owner,"newtrap",_getIntegerValue(mpd_context->newtrap));
				appendedToMap(_contextMap,owner,"traps",_getIntegerValue(mpd_context->traps));
				return _getValueOfMap(disowned_map(_contextMap,owner));
			}
		}
	}
	return NULL;
}
/**
 * @brief returns the decimal precision of the M decimal wrapped in \p value
 * @details returns NULL if \p value does not wrap a M decimal
 * @param value 
 * @return Mvalue* 
 */
Mvalue* getdp(Mvalue* value){
	return(value&&value->type==VT_DECIMAL?_getIntegerValue(value->value._decimal->prec):NULL);
}
/**
 * @brief sets the current decimal precision to the integer wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* 
 */
Mvalue* setdp(Mvalue* value){
	// how about returning the current value, no matter what the argument is????
	long long olddecimalprecision=getDP();
	// ignore if NO value specified...
	if(value!=NULL&&value->type==VT_INTEGER){
		long long decimalprecision=value->value._integer->ll;
		if(decimalprecision!=M_LL_INVALID){ // if not the default!!!
			if(decimalprecision>=6){
				// if I fail to create the associated decimal context, no go
				Mdecimalcontext* _newDecimalContext=getDecimalcontext(decimalprecision);
				if(_newDecimalContext!=NULL){
					M_DECIMALCONTEXT=_newDecimalContext;
					M_DP=decimalprecision; // OOPS forgot this earlier TODO should we do this or not????
					////DP_value->value._integer->ll=_decimalContext->prec;
				}else
					output("%sActive decimal context not replaced: failed to create a decimal context with precision %llu.\n",M_ERROR_PREFIX,decimalprecision);
			}else
				output("%sRequested decimal precision (%llu) not activated: it should at least be 6.\n",M_ERROR_PREFIX,decimalprecision);
		}
	}
	return _getIntegerValue(olddecimalprecision);
}

// end Decimal support

// very special M functions
// MDH@20DEC2020: added the _invalidTokenValue to be evaluated when the condition is negative
//				and changed the evaluation of the condition to a sign
/**
 * @brief returns the result of evaluation of the if function with condition value \p _conditionValue and then clause \p _thenTokenValue and else clause \p _elseTokenValue and undefined clause _undefinedTokenValue
 * @details the sign of _conditionValue is used to determine which clause to evaluate
 *          when _conditionValue equals M_LL_INVALID, the result of the evaluation of \p _undefinedTokenValue is returned
 *          when _conditionValue is positive, the result of the evaluation of \p _thenTokenValue is returned
 *          otherwise (not positive), the result of the evaluation of \p _elseTokenValue is returned
  * @param _conditionValue 
 * @param _thenTokenValue 
 * @param _elseTokenValue 
 * @param _undefinedTokenValue 
 * @return Mvalue* the result of the evaluation of _thenTokenValue, _elseTokenValue or _undefinedTokenValue (see details)
 */
Mvalue* Miffunction(Mvalue* _conditionValue,Mvalue* _thenTokenValue,Mvalue* _elseTokenValue,Mvalue* _undefinedTokenValue){
	Mvalue* _result=NULL;
	// MDH@23DEC2020:
	// the sign will be -1 (negative), 0 (zero) or 1 (positive) or M_LL_INVALID (a non numeric thing)
	// and an if has three possible outcomes instead of four, because we distinguish true / false or anything else
	// but this really depends on how we define true and false
	// typically in M M_TRUE equals 1 and M_FALSE equals 0 and undefined (can't compute) is M_LL_INVALID
	// we can broaden that a bit by considering all positive values TRUE and all non-positive values (except M_LL_INVALID) as FALSE
	// currently exactly one out of three possible arguments is evaluated
	long long conditionSign=getValueSign(_conditionValue);
	if(conditionSign==M_LL_INVALID){
		if(_undefinedTokenValue!=NULL&&_undefinedTokenValue->type==VT_TOKEN){
			getExecutionEnvironment()->expressionToken=_undefinedTokenValue->value._token;
			_result=getValueOfExpression("undefined clause",'e',NULL,0);
		}
	}else
	if(conditionSign>0){ 
		if(_thenTokenValue!=NULL&&_thenTokenValue->type==VT_TOKEN){
			getExecutionEnvironment()->expressionToken=_thenTokenValue->value._token;
			_result=getValueOfExpression("then clause",'t',NULL,0);
		}
	}else{ // all non-positive values (except M_LL_INVALID)
		if(_elseTokenValue!=NULL&&_elseTokenValue->type==VT_TOKEN){
			getExecutionEnvironment()->expressionToken=_elseTokenValue->value._token;
			_result=getValueOfExpression("else clause",'e',NULL,0);
		}
	}
	return _result;
}

// MDH@21DEC2020: a new way to do a while is by receiving a single token list (just like do does!!)
/**
 * @brief executes the while loop stored in the M list wrapped in \p _whileTokenlistValue
 * 
 * @param _whileTokenlistValue 
 * @return Mvalue* the result of the execution of the last list command
 */
Mvalue* Mwhilefunction(Mvalue* _whileTokenlistValue){
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	Mvalue* _result=NULL;
	if(_whileTokenlistValue!=NULL&&_whileTokenlistValue->type==VT_LIST&&_whileTokenlistValue->value._list!=NULL){
		Mlist* whileTokenlist=_whileTokenlistValue->value._list;
		if(whileTokenlist->numberOfElements>1){
			Mlistelement* conditiontokenlistelement=whileTokenlist->_first;
			// we need the condition token and its successor
			if(conditiontokenlistelement!=NULL&&conditiontokenlistelement->_next!=NULL){
				Mtoken* conditiontoken=(conditiontokenlistelement!=NULL&&conditiontokenlistelement->_value!=NULL&&conditiontokenlistelement->_value->type==VT_TOKEN?conditiontokenlistelement->_value->value._token:NULL);
				if(conditiontoken!=NULL){
					Mlistelement* looptokenlistelement;
					Mtoken* looptoken;
					while(1){
						getExecutionEnvironment()->expressionToken=conditiontoken;
						Mvalue* _conditionValue=getValueOfExpression("while condition",'w',NULL,0);
						// MDH@23DEC2020: similar to in Miffunction we use the sign to determine whether
						//				or not the condition is 'true' (positive values only)
						if(report)
							outputValue("Condition value: '",_conditionValue,"'.\n");
						long long conditionSign=getValueSign(_conditionValue);
						if(conditionSign<=0)break;
						// evaluate the body
						looptokenlistelement=conditiontokenlistelement->_next;
						while(looptokenlistelement!=NULL){
							if(looptokenlistelement->_value!=NULL&&looptokenlistelement->_value->type==VT_TOKEN){
								looptoken=looptokenlistelement->_value->value._token;
								if(looptoken!=NULL){
									getExecutionEnvironment()->expressionToken=looptoken;
									_result=getValueOfExpression("while loop",'l',NULL,0);
									if(report)
										outputValue("Result so far: '",_result,"'.\n");
								}
							}
							looptokenlistelement=looptokenlistelement->_next;
						}
					}
				}else
					outputBug("First while token list element not a token");
			}else
				outputBug("Missing while token list condition");
		}else
			outputError("A while loop takes a condition and at least one loop expression");
	}else
		outputBug("While loop arguments not a list");
	return _result;
}
/* replacing the previous implementation
Mvalue* Mwhilefunction(Mvalue* _conditionTokenValue,Mvalue* _whilebodyTokenValue){
	Mvalue* _result=NULL;
	if(_conditionTokenValue&&_conditionTokenValue->type==VT_TOKEN&&_whilebodyTokenValue&&_whilebodyTokenValue->type==VT_TOKEN){
		while(true){
			// evaluate the condition
			getExecutionEnvironment()->expressionToken=_conditionTokenValue->value._token;
			Mvalue* _conditionValue=getValueOfExpression("while condition",'w',NULL,0);
			// MDH@22OCT2020: we know that a condition evaluates to M_TRUE, M_FALSE or M_LL_INVALID (undecisive), and the last one (M_LL_INVALID) is not equal zero so testing for being positive is preferred
			if(isValueZero(_conditionValue)==M_TRUE||isValueUndefined(_conditionValue)==M_TRUE)break; // condition evaluates to zero
			// evaluate the body
			getExecutionEnvironment()->expressionToken=_whilebodyTokenValue->value._token;
			_result=getValueOfExpression("while loop",'l',NULL,0);
		}
	}
	return _result;
}
*/
// MDH@05AUG2019: the do function allows for executing a single command in its own environment, so all variables created are local
//				the problem is that we want to allow the user to enter a list of token things i.e. an infinite list of arguments instead of having to wrap the single argument in a list itself
//				this is solvable if we convert the list of arguments to a single Mvalue wrapping the entire list of arguments before calling Mdofunction
// MDH@23OCT2021: I had a marvelous idea i.e. to simply return the environment do creates
//				as we can wrap it in a value, this way it can be retained by assigning it
//				and therefore become an 'object' that can be accessed (and have 'methods')
/**
 * @brief returns the do function call on the list of do commands in the M list wrapped in \p _doTokenValue
 * 
 * @param _doTokenValue 
 * @return Mvalue* 
 */
Mvalue* Mdofunction(Mvalue* _doTokenValue){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _result=NULL;
	if(_doTokenValue!=NULL&&_doTokenValue->type==VT_LIST){
		Mlist* doList=_doTokenValue->value._list;
		if(doList!=NULL&&doList->_first!=NULL){ // something to do
			// essentially all arguments are not evaluated
			// CAREFUL if we fail to create the environment so _doVariableMap was not bound to it, we have to free it with free_map explicitly
			Menvironment* _doEnvironment=owned_environment(__environment(),owner);
			if(_doEnvironment!=NULL){
				_doEnvironment->_name=owned_chars(_getChars("do"),Msubowner(owner,1));
				// let's add variable $ as result variable and ! as exit flag variable
				// MDH@10JAN2020: ! is replaced by making "$" immutable to indicate being done
				bool doEnvironmentInitialized=addVariable(_doEnvironment,owner,"$",VT_UNDEFINED,false)
				/* removing
												&&addVariable(_doEnvironment,owner,"!",VT_INTEGER,false)
												&&setValue(_doEnvironment,"!",_getIntegerValue(0))*/
												;
				if(doEnvironmentInitialized){
					// MDH@21DEC2020 BUG FIX: pushing an environment will wrap it inside a value
					//						which should be able to take over membership
					//						which means you have to disown the environment!!!!
					if(pushExecutionEnvironment(disowned_environment(_doEnvironment,owner))){ // _doEnvironment bound!!!
						// MDH@25OCT2021: _doEnvironment is now wrapped in a value but for now we do not have access to it
						// NOTE luckily we know who is owning the environment now (as it is now wrapped inside a value), so we can still register the variables (see below)
						// evaluate the first argument inside the do environment (we have to because we're executing the command in the current environment as well)
						Mlistelement* tokenValueListelement=doList->_first;
						Mvalue* expressionValue;
						if(amVerboseDebugging())
							outputValue("First do function call argument: '",tokenValueListelement->_value,"'.\n");
						Mvalue *tokenExpressionValue=tokenValueListelement->_value;
						if(tokenExpressionValue!=NULL){
							expressionValue=NULL;
							if(tokenExpressionValue->type==VT_TOKEN){
								// execute it in the do environment
								_doEnvironment->expressionToken=tokenExpressionValue->value._token;
								expressionValue=getValueOfExpression("do",'d',(TokenType[]){},0); // evaluate the expression
								if(expressionValue!=NULL){
									if(expressionValue->type==VT_MAP){
										if(!registerVariables(_doEnvironment,getValueDataOwner(),expressionValue->value._map,NULL))
											outputError("Failed to initialize the do function call environment!");
									}else
										outputWarning("Local variables of do function call not defined in a map!\n");
								}
							}else
							if(tokenExpressionValue->type==VT_MAP)
								if(!registerVariables(_doEnvironment,getValueDataOwner(),tokenExpressionValue->value._map,NULL))
									outputError("Failed to initialize the do function call environment!");
						}
						// now ready to process the 'body'
						tokenValueListelement=tokenValueListelement->_next; // skip the local variable map
						expressionValue=NULL; // to store the last evaluated argument value to be used as result when $ was not set
						while(tokenValueListelement!=NULL){
							tokenExpressionValue=tokenValueListelement->_value;
							if(tokenExpressionValue&&tokenExpressionValue->type==VT_TOKEN){ // some token to interpret
								_doEnvironment->expressionToken=tokenExpressionValue->value._token;
								if(_doEnvironment->expressionToken!=NULL){
									expressionValue=getValueOfExpression("do",'d',(TokenType[]){},0); // evaluate the expression
									// if the exit flag was set, exit
									if(isImmutable(getVariable(NULL,"$",false))==M_TRUE)break;
									/* replacing:
									if(isValueZero(getValue(_doEnvironment,"!"))!=M_TRUE)break; 
									*/
								}
							}
							// move over to the next expression to evaluate...
							tokenValueListelement=tokenValueListelement->_next;
						}
						_result=popExecutionEnvironment();
						/* MDH@25OCT2021: replacing
						Mvalue* doResultValue=getValue(_doEnvironment,"$");
						_result=(doResultValue?doResultValue:expressionValue);
						popExecutionEnvironment(); // pop the do environment we successfully pushed
						*/
					}else // _doEnvironment disowned, but not bound
						free_environment(_doEnvironment);
				}else{ // _doEnvironment bound to this function, so both disowned and free
					FREE_ENVIRONMENT(_doEnvironment,owner); // MDH@17JUN2020: check if this should be here
					outputError("Failed to create the do environment");
				}
			}
		}
	}
	return _result;
}
// MDH@11MAR2020: the value of the result token is assigned to $ so that will become the result of the application of the Mforfunction
// MDH@23DEC2020: Mforfunction renamed to Mforwithfunction because that's what it actually is, this will save the user from wrapping the for call in a with statement
/**
 * @brief returns the result of executing a for with loop
 * 
 * @param _initializationTokenValue the initialization clause 
 * @param _conditionTokenValue the condition clause
 * @param _incrementTokenValue the increment clause
 * @param _bodyTokenValue the body clause
 * @param _resultTokenValue the result clause
 * @return Mvalue* the result of executing a for with loop
 */
Mvalue* Mforwithfunction(Mvalue* _initializationTokenValue,Mvalue* _conditionTokenValue,Mvalue* _incrementTokenValue,Mvalue* _bodyTokenValue,Mvalue* _resultTokenValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	Mvalue* _result=NULL;
	if( (NULL==_initializationTokenValue||_initializationTokenValue->type==VT_TOKEN)&&
		(_conditionTokenValue!=NULL&&_conditionTokenValue->type==VT_TOKEN)&&
		(NULL==_incrementTokenValue||_incrementTokenValue->type==VT_TOKEN)&&
		(_bodyTokenValue!=NULL&&_bodyTokenValue->type==VT_TOKEN)&&
		(NULL==_resultTokenValue||_resultTokenValue->type==VT_TOKEN)){
		if(report){
			output("For loop:");
			outputValue(" Initialization=",_initializationTokenValue,NULL);
			outputValue(" Condition=",_conditionTokenValue,NULL);
			outputValue(" Increment=",_incrementTokenValue,NULL);
			outputValue(" Body=",_bodyTokenValue,NULL);
			outputValue(" Result=",_resultTokenValue,NULL);
			newline();
		}
		// create a new M environment to run the for with loop in
		Menvironment* _forEnvironment=owned_environment(__environment(),owner);
		if(_forEnvironment!=NULL){
			_forEnvironment->_name=owned_chars(_getChars("for loop"),Msubowner(owner,1));
			// better wait with pushing until _forEnvironment is initialized appropriately
			// MDH@11MAR2020: $ is NOT needed when there's an explicit result token value!!
			bool forEnvironmentInitialized=(_resultTokenValue!=NULL?true:false);
			if(forEnvironmentInitialized&&!addVariable(_forEnvironment,owner,"$",VT_UNDEFINED,false))forEnvironmentInitialized=false;
			if(forEnvironmentInitialized&&!addVariable(_forEnvironment,owner,"_",VT_INTEGER,false))forEnvironmentInitialized=false;
			// MDH@21DEC2020: TODO we will need to change "_" to something else because _ is used in functions
			//					 for storing the additional arguments
			if(forEnvironmentInitialized&&!setValue(_forEnvironment,"_",_getIntegerValue(0)))forEnvironmentInitialized=false;
			if(forEnvironmentInitialized){
				if(pushExecutionEnvironment(_forEnvironment)){
					// evaluate the initialization inside the for environment once
					if(_initializationTokenValue!=NULL){
						_forEnvironment->expressionToken=_initializationTokenValue->value._token;
						// MDH@11MAR2020: to force the creation of all identifiers that are assigned in the initialization token value, we have to ascertain that they are considered TT_NEW_VARIABLE
						//				essentially this means you cannot set an outside variable in the first for loop expression
						//				alternatively, we could simple add all these variables beforehand and mark all as TT_VARIABLE which is another way of doing that
						//				assignment is crucial? yes, if not assigned 
						Mvalue* initializationValue=getValueOfExpression("for initialization",'i',(TokenType[]){},0); // return value NOT imported
						// any map is used to initialize as local variables (just like we did in defining functions)
						// interestingly any text can be used to variables (outside the identifiers allowed by the interpreter)
						// although perhaps we should exclude using $ and _ well especially _
						// MDH@08NOV2019: a list is also allowed actually anything
						if(initializationValue!=NULL&&initializationValue->type==VT_MAP&&!isExecutionEnvironmentInitialized(_forEnvironment,getOwnerExecutionEnvironment(),initializationValue->value._map,NULL)){
							outputError("Failed to initialize the for loop local variables");
							forEnvironmentInitialized=false;
						}
					}
					if(forEnvironmentInitialized){
						// _forEnvironment->_variableMap->immutable=true; // MDH@10NOV2019: lock the variable map
						Mvalue *_forBodyValue=NULL,*_forIncrementValue=NULL;
						while(true){
							/*
								outputVariables(); // let's see what we got (in the currently executing (for) environment)
								char inputChar;
								if(!inputCharRead(&inputChar))break;
							*/
							// evaluate the condition
							_forEnvironment->expressionToken=_conditionTokenValue->value._token;
							/*
							output("Condition: ");
							Mtoken* token=_forEnvironment->expressionToken;while(token){outputToken(token);token=token->next;}
							outputChar('\n');resetOutputColor();
							*/
							Mvalue* _conditionValue=getValueOfExpression("for condition",'f',(TokenType[]){},0);
							if(report)
								outputValue("For loop condition value: '",_conditionValue,"'.\n");
							long long conditionSign=getValueSign(_conditionValue);
							if(conditionSign<=0)break; // condition evaluates to zero or is undefined
							// increment the implicit loop counter variable BEFORE executing the loop AFTER evaluating the condition
							setValue(_forEnvironment,"_",_getIntegerValue(getValue(_forEnvironment,"_")->value._integer->ll+1));
							if(report){
								outputValue("For loop condition in iteration #",getValue(_forEnvironment,"_"),NULL);
								outputValue(" evaluates to '",_conditionValue,"'.\n");
							}
							if(_bodyTokenValue!=NULL){
								// evaluate the for body
								_forEnvironment->expressionToken=_bodyTokenValue->value._token;
								/*
								output("Body: ");
								Mtoken* token=_forEnvironment->expressionToken;while(token){outputToken(token);token=token->next;}
								newline();resetOutputColor();
								*/
								_forBodyValue=getValueOfExpression("for loop",'l',(TokenType[]){},0);
								if(report){
									outputValue("For loop body in iteration #",getValue(_forEnvironment,"_"),NULL);
									outputValue(" evaluates to '",_forBodyValue,"'.\n");
								}
							}
							// MDH@10JAN2021: break if "$" is now immutable
							if(isImmutable(getVariable(NULL,"$",false))==M_TRUE)break;
							if(_incrementTokenValue!=NULL){
								// evaluate the increment
								_forEnvironment->expressionToken=_incrementTokenValue->value._token;
								/*
								output("Increment: ");
								Mtoken* token=_forEnvironment->expressionToken;
								while(token){outputToken(token);token=token->next;}
								newline();resetOutputColor();
								*/
								_forIncrementValue=getValueOfExpression("for increment",'i',(TokenType[]){},0);
								if(report){
									outputValue("For loop increment in iteration #",getValue(_forEnvironment,"_"),NULL);
									outputValue(" evaluates to '",_forIncrementValue,"'.\n");
								}
								/*
									char inputChar;
									if(!inputCharReadFunction(&inputChar))break;
								*/
							}
						}
						// MDH@11MAR2020: if there's a result token value, we use that value as the result of the for loop (in which case we would not need $ at all)
						//				of course we could let $ take precedence over the result token BUT the general idea is that any result token replaces the implicit result (which would be the number of times the loop is executed)
						if(_resultTokenValue!=NULL){
							_forEnvironment->expressionToken=_resultTokenValue->value._token;
							_result=getValueOfExpression("for loop",'l',(TokenType[]){},0);
						}else{ // no explicit result token which value denotes the result
							_result=getValue(_forEnvironment,"$"); // get the result
							if(NULL==_result){
								_result=getValue(_forEnvironment,"_"); // just return the value of the counter if $ was not set!!
								if(report)
									outputValue("For loop implicit result value (of increment counter local variable _): '",_result,"'.\n");
							}else
							if(report)
								outputValue("For loop explicit result value (of the $ local variable): '",_result,"'.\n");
						}
					}
					popExecutionEnvironment(); // pop the for execution environment (freeing it in the process)
					if(report)
						outputInfo("For loop environment popped.");
				}else{
					outputError("Failed to activate the for loop execution environment");
					forEnvironmentInitialized=false;
				}
			}else
				output("%sFailed to add or initialize the for loop result and counter local variables $ and _.\n",M_ERROR_PREFIX);
			if(!forEnvironmentInitialized){
				// FREE_ENVIRONMENT(_forEnvironment,owner); // have to free the environment myself
				// if(amVerbose())
				outputInfo("Uninitialized for loop environment discarded!");
			}
			FREE_ENVIRONMENT(_forEnvironment,owner); // MDH@17JUN2020: TODO should this be here???
		}else
			outputError("Failed to create the for loop execution environment");
	}
	return _result;
}
// MDH@23DEC2020: Mforfunction now implements the first choice Mforwithfunction for executing a for loop from a token list (of indefinite number of tokens just like while and do)
/**
 * @brief returns the result of executing a for token list wrapped in \p _forTokenlistValue
 * 
 * @param _forTokenlistValue 
 * @return Mvalue* 
 */
Mvalue* Mforfunction(Mvalue* _forTokenlistValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	Mvalue* _result=NULL;
	if(_forTokenlistValue!=NULL&&_forTokenlistValue->type==VT_LIST&&_forTokenlistValue->value._list!=NULL){
		Mlist* forTokenlist=_forTokenlistValue->value._list;
		if(forTokenlist->numberOfElements>3){ // at least four elements required
			// extracting valid initialization, condition and increment list element (of type VT_TOKEN)
			Mlistelement* initializationTokenlistelement=forTokenlist->_first;
			// NOTE ascertaining that the condition is NULL if the initialization token is not of the right type (or undefined)
			Mlistelement* conditionTokenlistelement=(initializationTokenlistelement!=NULL
														&&(!initializationTokenlistelement->_value||initializationTokenlistelement->_value->type==VT_TOKEN)
													?initializationTokenlistelement->_next
													:NULL);
			// NOTE let's allow the increment token list element to be NULL as well although this is not recommended
			Mlistelement* incrementTokenlistelement=(conditionTokenlistelement!=NULL&&conditionTokenlistelement->_value!=NULL&&conditionTokenlistelement->_value->type==VT_TOKEN
													?conditionTokenlistelement->_next
													:NULL);
			// we can suffice with testing the increment token
			if(incrementTokenlistelement!=NULL&&incrementTokenlistelement->_next!=NULL){
				Mtoken* conditionToken=conditionTokenlistelement->_value->value._token;
				if(conditionToken!=NULL){
					// allowing the increment to be NULL although this is not recommended, as otherwise the user could forget to make the condition change!!
					Mtoken* incrementToken=(incrementTokenlistelement->_value&&incrementTokenlistelement->_value->type==VT_TOKEN?incrementTokenlistelement->_value->value._token:NULL);
					// initialize
					Mtoken* initializationToken=(initializationTokenlistelement&&initializationTokenlistelement->_value?initializationTokenlistelement->_value->value._token:NULL);
					if(initializationToken!=NULL){
						getExecutionEnvironment()->expressionToken=initializationToken;
						Mvalue* _initializationValue=getValueOfExpression("for loop initialization",'v',NULL,0);
						if(report)
							outputValue("For loop initialization value: '",_initializationValue,"'.\n");
					}
					Mlistelement* loopTokenlistelement;
					Mtoken* loopToken;
					while(1){
						getExecutionEnvironment()->expressionToken=conditionToken;
						Mvalue* _conditionValue=getValueOfExpression("for loop condition",'i',NULL,0);
						if(report)
							outputValue("For loop condition value: '",_conditionValue,"'.\n");
						long long conditionSign=getValueSign(_conditionValue);
						// the condition is not met when the condition value is undefined or not positive
						if(conditionSign<=0)break; // condition is not met
						// evaluate the body elements
						loopTokenlistelement=incrementTokenlistelement->_next;
						while(loopTokenlistelement!=NULL){
							if(loopTokenlistelement->_value!=NULL&&loopTokenlistelement->_value->type==VT_TOKEN){
								loopToken=loopTokenlistelement->_value->value._token;
								if(loopToken!=NULL){
									getExecutionEnvironment()->expressionToken=loopToken;
									_result=getValueOfExpression("for loop body",'l',NULL,0);
									if(report)
										outputValue("Result so far: '",_result,"'.\n");
									/* OOPS this (new) for function does NOT run in it's own environment
									   one would need to wrap the for function call in a with (or do)
									// NOTE only the evaluation of the loop token can change the immutability of the result variable
									// MDH@10JAN2021: break if "$" is now immutable
									if(isImmutable(getVariable(NULL,"$",false))==M_TRUE)break;
									*/
								}
							}
							loopTokenlistelement=loopTokenlistelement->_next;
						}
						// finish with evaluating the increment token (if defined)
						if(initializationToken!=NULL){
							getExecutionEnvironment()->expressionToken=incrementToken;
							Mvalue* _initializationValue=getValueOfExpression("for loop increment",'i',NULL,0);
							if(report)
								outputValue("For loop increment value: '",_initializationValue,"'.\n");
						}
					}
				}else
					outputBug("For loop condition list element missing or not a token");
			}else
				outputBug("For loop increment list element missing or not a token");
		}else
			outputError("A for loop takes an initialization (possibly undefined), a condition (obligatory), an increment (possibly undefined) and at least one loop expression");
	}else
		outputBug("For loop arguments not a list");
	return _result;
}

// MDH@11MAR2024: we want to be able to use block ifs, whiles, and fors

// MDH@11MAR2024: by delegating registering a map of locals into an environment we can reuse this code
//                for all the block environments (in ifs, whiles and fors)
//                TODO move over to Menvironment.h/c eventually
/**
 * @brief pushes \p _withEnvironment as execution environment after successfully initializing it with \p localMap
 * @details any . value in \p localMap is used to name the environment
 * @param _withEnvironment the environment to push
 * @param owner the owner of the environment
 * @param localMap the map to initialize the environment with
 * @return long long M_TRUE on success, M_FALSE on failure
 */
static long long pushInitializedEnvironment(Menvironment * const _withEnvironment,Mallocationowner const owner,Mmap const * const localMap){
	bool result=M_LL_INVALID;
	if(_withEnvironment!=NULL){
		result=M_FALSE;
		Mmapelement* withNameMapelement=(localMap!=NULL?getMapelement(localMap,"."):NULL);
		Mvariable* withNameVariable=(withNameMapelement!=NULL?withNameMapelement->_variable:NULL);
		Mstring* _withNameText=(withNameVariable!=NULL?owned_string(_getValueText(withNameVariable->_value,true),owner):NULL);
		// if a property "." is defined, it's text value will be the name of the with environment
		if(_withNameText!=NULL){
			if(_withEnvironment->_name!=NULL){freeChars(_withEnvironment->_name);_withEnvironment->_name=NULL;}
			_withEnvironment->_name=owned_chars(_getChars(string(_withNameText)),Msubowner(owner,1));
		}
		// copy local map
		// TODO what if we fail to copy the map????????
		if(localMap!=NULL)_withEnvironment->_variableMap=owned_map(_getMapCopy(localMap),Msubowner(owner,1));
		if(NULL==localMap||_withEnvironment->_variableMap!=NULL){
			if(NULL==withNameMapelement||removedFromMap(_withEnvironment->_variableMap,owner,".")==M_TRUE){
				// almost there
				if(!pushExecutionEnvironment(disowned_environment(_withEnvironment,owner))){ // _withEnvironment not bound!!!
					free_environment(_withEnvironment);//////////_withEnvironment=NULL;
					output("%sFailed to initialize environment '%s'.\n",M_ERROR_PREFIX,_withEnvironment->_name->chars);
				}else{
					result=M_TRUE;
					output("Environment '%s' initialized.",_withEnvironment->_name->chars);
				}
			}else
			if(withNameMapelement!=NULL)
				outputError("Failed to remove the environment name from the local variables map");	
		}
		if(result==M_FALSE&&_withEnvironment!=NULL){
			/////////if(report)output("Freeing the with environment!");
			FREE_ENVIRONMENT(_withEnvironment,owner);
		}
		/* replacing:
		Mmapelement* withVariableMapelement=localMap->_first;
		while(withVariableMapelement){
			withNameVariable=withVariableMapelement->_variable;
			if(withNameVariable){
				Mchars* withlocalVariableName=withNameVariable->_name;
				if(withlocalVariableName->chars){
					if(!addVariable(_withEnvironment,owner,withlocalVariableName,));
					withVariableMapelement=withVariableMapelement->_next;
				}
			}
		}
		if(!withVariableMapelement){ // with environment successfully initialized
		}else{
			output("%sFailed to initialize %senvironment",M_ERROR_PREFIX,(_withNameText?"":"the with "));
			if(_withNameText)output(" '%s'",string(_withNameText));
			output(".\n");
		}
		*/
		if(_withNameText!=NULL)FREE_STRING(_withNameText,owner);
	}
	return result;
}

/**
 * @brief initiates a block if, returns M_TRUE on success, M_FALSE on failure, of M_LL_INVALID on invalid input
 * 
 * @param testValue 
 * @param localsValue 
 * @return Mvalue* 
 */
Mvalue* Mif(Mvalue* testValue,Mvalue* localsValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(testValue!=NULL){
		result=M_FALSE;
		// we'd be successful if we succesfully initialized the if environment
		// (copied over and adapted from Mdofunction!!!)
		Menvironment* ifEnvironment=owned_environment(__environment(),owner);
		if(ifEnvironment!=NULL){
			ifEnvironment->_name=owned_chars(_getChars("if"),Msubowner(owner,1));
			// let's add variable $ as result variable and ! as exit flag variable
			// MDH@10JAN2020: ! is replaced by making "$" immutable to indicate being done
			bool ifEnvironmentInitialized=addVariable(ifEnvironment,owner,"$",VT_UNDEFINED,false)
				/* removing
												&&addVariable(_doEnvironment,owner,"!",VT_INTEGER,false)
												&&setValue(_doEnvironment,"!",_getIntegerValue(0))*/
												;
			if(ifEnvironmentInitialized){
				Mmap* localMap=(localsValue!=NULL&&localsValue->type==VT_MAP?localsValue->value._map:NULL);
				// MDH@21DEC2020 BUG FIX: pushing an environment will wrap it inside a value
				//						which should be able to take over membership
				//						which means you have to disown the environment!!!!
				if(pushInitializedEnvironment(ifEnvironment,owner,localMap)==M_TRUE)
					result=M_TRUE;
			}else{ // _doEnvironment bound to this function, so both disowned and free
				FREE_ENVIRONMENT(ifEnvironment,owner); // MDH@17JUN2020: check if this should be here
				outputError("Failed to create the if environment");
			}
		}
	}
	return _getIntegerValue(result);
}
// MDH@11MAR2024 END

// the input info and error function default to shellInputInfo and shellInputError that write the text to the console  (and are replaced in M.c by functions that output above the user input lines and use colors)
/**
 * @brief displays input info 
 * 
 * @param fmt the format string for displaying all arguments using the format string \p fmt
 * @param ... 
 */
static void inputInfo(const char* const fmt,...){
	if(fmt!=NULL&&strlen(fmt)){ // we have a format
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		newline();
	}
}
/**
 * @brief displays input error
 * 
 * @param fmt the format string for displaying all arguments using the format string \p fmt
 * @param ... 
 */
static void inputError(const char* const fmt,...){
	if(fmt!=NULL&&strlen(fmt)){ // we have a format
		va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args); // NOTE would be a mistake to call output() here, resulting
		newline();
	}
}
// MDH@10MAR2020: initialized in shellInitialized() so shellInitialized() must be called prior to any input processing
/**
 * @brief the plugged in input info function (see shellInitialized)
 * 
 */
static InputResponseFunction* inputInfoFunction=NULL;
/**
 * @brief the plugged in input error function (see shellInitialized)
 * 
 */
static InputResponseFunction* inputErrorFunction=NULL;
// void setInputInfoFunction(InputResponseFunction* _inputResponseFunction){inputInfoFunction=_inputResponseFunction;}
// void setInputErrorFunction(InputResponseFunction* _inputResponseFunction){inputErrorFunction=_inputResponseFunction;}

// and the most special one
// requiring some other stuff for being able to interpret the text and create tokens!!!

// MDH@05JUN2019: it's prudent to return the negative value of the input token type if the given input character type ends the token 
//				i.e. when NO_TRANSITIONS is a match, so that the caller can set the significantCharacterCount
/**
 * @brief returns the next token type having receive an input character of type \p inputCharacterType when in a token of type \p inputTokenType
 * 
 * @param inputTokenType 
 * @param inputCharacterType 
 * @return int8_t 
 */
static int8_t nextTokenType(uint8_t inputTokenType,char inputCharacterType){
	logToOutputFile("Token type: %s + % c",TOKENTYPE_STRING[inputTokenType],inputCharacterType);
	if(inputTokenType<NUMBER_OF_FINISHABLE_TOKEN_TYPES){ // can only move to another token type if currently inside a valid token (i.e. you cannot get out of a TT_ERROR token type!!!)
		// finding the type will be more difficult actually if we end up with the token type character instead of the token type index!!!
		char* noTransition=NO_TRANSITIONS[inputTokenType];
#ifdef __DEBUG__
		printf("'%s'",noTransition);
#endif
		// TODO we can improve on the following
		///////////if(noTransition[0]!='`'&&!strchr(noTransition,inputCharacterType))return -inputTokenType;
		if(strlen(noTransition)==0||(noTransition[0]=='!'?strchr(noTransition,inputCharacterType)!=NULL:strchr(noTransition,inputCharacterType)==NULL)){
			int8_t tokenType=NUMBER_OF_TOKEN_TYPES; // MDH@10APR2019: BUG FIX uint8_t changed to int8_t otherwise would circle around
			// find the new token type
			while(--tokenType>=0)
				if(strchr(TRANSITIONS[inputTokenType][tokenType],inputCharacterType)!=NULL){
					logToOutputFile(" -> %s.\n",TOKENTYPE_STRING[tokenType]);
					return tokenType;
				}
		}
#ifdef __DEBUG__
		else{
			outputChar('=');
		}
#endif
	}
	///logToOutputFile(" -> NO TRANSITION.\n");
	return inputTokenType; // if no match was found assume no change to the token type!!
}
// TODO we could call the following function from tokenCheckedForBeingAFunction
// an identifier with a certain name in a certain special function call (to which it might be local)
// instead of requiring a specialFunctionCallToken it suffices to know the environment id
/**
 * @brief returns true if identifier \p identifierName exists in M command \p command
 * 
 * @param command 
 * @param identifierName 
 * @param identifierEnvironmentId 
 * @return true when the identifier with name \p identifierName exists in M command \p command
 * @return false otherwise
 */
bool existsInCommand(Mcommand* command,char* identifierName,uint64_t identifierEnvironmentId){ // replacing: const Mtoken* const specialFunctionCallToken){
	// every token contains a reference to its previous identifier (or name of the function being called), basically this means we can find all identifiers present in the current command
	// but we have to be careful because variables declared locally should be skipped unless they are in the same function call i.e. expr
	bool found=false;
	size_t l=strlen(identifierName);
	Mtoken* commandIdentifier=command->_lastToken->prevIdentifier;
	uint64_t commandIdentifierEnvironmentId,commandIdentifierEnvironmentLevels,ander=(1<<M_BITS_PER_ENV_LEVEL)-1;
	while(!found&&commandIdentifier){
		// if a function call or end of function call identifier, no need to check!!
		if(commandIdentifier->type!=TT_FUNCTION&&commandIdentifier->type!=TT_END_OF_FUNCTION_CALL){ // a (new) variable
			// MDH@11JAN2021: the commandIdentifier can now also be a map property, but only in calls to do() and forw() and I guess function()
			// MDH@11JAN2021: by using string_replacedchar() the length of the command identifier won't change, but string(commandIdentifier) will still bump into this '\0' if it is in front of the original '\0'
			//				the length of commandIdentifier (which is stored in Mstring so does not need to be computed!!!!) needs to be at least equal to that of the length of identifierName!!
			if(commandIdentifier->significantCharacterCount==l){ // the command identifier has the same number of significant characters as identifierName!!
				char* commandIdentifierName=string(commandIdentifier->text);
				if(strncmp(commandIdentifierName,identifierName,l)==0){ // the non-whitespace matches
					if(commandIdentifier->argument==1){ // the identifier is local to one of the special function calls (which is present in `do`, `for` and `function` function calls)
						// we can't tell for sure that this local identifier is in the same special function call unless `expr` field matches imagine the situation where multiple do's are in the same command following each other
						// the local variables in the first are not local to the second do call it's all about scope meaning we have to mark the end of a scope as well so we know which identifiers to skip i.e. those identifiers local to another special function call
						// so if we stored `( f g , h ) ( x, g` the second g is not in the first call and therefore does not exist in the command, so in going back you have to keep track of the level which should be the same as level of the caller
						// the special function call associated with the two identifiers must match!!
						// BUT a local variable of a special function call could be used in which the special function call of the identifier is nested within (like a do inside a do) in which case we should keep going up
						// so: identifier is local to its own special function call but the presented identifier might not i.e. it might be defined in a outer special function call
						if(identifierEnvironmentId){ // defined inside a subenvironment
							commandIdentifierEnvironmentId=commandIdentifier->envid;
							commandIdentifierEnvironmentLevels=(commandIdentifierEnvironmentId&15);
							// it's all about environmentid subclassing the environment id of identifier
							// i.e. environment id level should be at least the identifier's environment id
							if(commandIdentifierEnvironmentLevels<=(identifierEnvironmentId&15)){ // the registered identifier is defined at a level equal to or above that of the identifier
								commandIdentifierEnvironmentId>>=4;identifierEnvironmentId>>=4; // shift out the number of levels
								// all environment ids of the local identifier (commandIdentifier) should match those in identifierEnvironmentId
								while(commandIdentifierEnvironmentLevels>0&&((commandIdentifierEnvironmentId&ander)==(identifierEnvironmentId&ander))){
									commandIdentifierEnvironmentLevels--;
									commandIdentifierEnvironmentId>>=M_BITS_PER_ENV_LEVEL;
									identifierEnvironmentId>>=M_BITS_PER_ENV_LEVEL;
								}
								if(commandIdentifierEnvironmentId==0)found=true;
							}
						}
						/* replacing:
						if(specialFunctionCallToken){ // the given identifier exists inside a special function call therefore it might be the local identifier with the same name!!
							Mtoken *localIdentifierSpecialFunctionCallToken=getSpecialFunctionCallToken(identifier),*needleSpecialFunctionCallToken=specialFunctionCallToken; // which MUST exist i.e. will NOT be NULL
							while(needleSpecialFunctionCallToken&&needleSpecialFunctionCallToken!=localIdentifierSpecialFunctionCallToken)needleSpecialFunctionCallToken=getSpecialFunctionCallToken(needleSpecialFunctionCallToken);
							if(needleSpecialFunctionCallToken)found=true;
						}
						*/
					}else
						found=true;
				}
			} // TODO will blank always be the only possible whitespace character????? 
		}
		// get the next identifier
		commandIdentifier=commandIdentifier->prevIdentifier;
	}
	//////////if(found)inputInfo("%s",identifierName);else inputInfo("NOT %s",identifierName);
	return found;
}
/**
 * @brief the plugged in update last token auto completion text function
 * 
 */
static UpdateLastTokenAutocompletionTextFunction* updateLastTokenAutocompletionTextFunction=NULL;
// void setUpdateLastTokenAutocompletionTextFunction(UpdateLastTokenAutocompletionTextFunction* _updateLastTokenAutocompletionTextFunction){updateLastTokenAutocompletionTextFunction=_updateLastTokenAutocompletionTextFunction;}
/**
 * @brief the plugged in reoutput token function (argument to shellInitialized)
 * 
 */
static ReoutputTokenFunction* reoutputTokenFunction=NULL;
// void setReoutputTokenFunction(ReoutputTokenFunction* _reoutputTokenFunction){reoutputTokenFunction=_reoutputTokenFunction;}
/**
 * @brief returns true if M variable \p variable represents a function, false otherwise
 * 
 * @param variable 
 * @return true 
 * @return false 
 */
static bool representsAFunction(Mvariable* variable){
	// ASSERT variable should NOT be NULL
	if(variable->valuetype==VT_FUNCTION)return true; // TODO this is questionable BUT ok
	return(variable->_value?variable->_value->type==VT_FUNCTION:false);
}

// MDH@12MAR2020: because containsVariable() is not called in Menvironment.h/c itself, and it uses inputInfoFunction I moved it over here today just before it is getting used
/**
 * @brief 
 * @details returns 0 if \p name is NULL or an empty string
 *          returns -2, -3 or -4 if \p name is not a valid full property name 
 *          returns -1 if \p name does not exist in M environment \p _environment
 *          returns 1 if M environment \p _environment contains a function with name \p name
 *          returns 2 if M environment \p _environment contains a function with name \p name 
 * @param _environment
 * @param name
 * @param report
 * 
 */
int8_t containsVariable(Menvironment const * const _environment,char /*const*/ * const name, int8_t report){
	// MDH@09MAR2020: because we can now also have variables that are functions a true variable requires the variable to NOT be a function
	if(NULL==name)return 0; // invalid input
	// MDH@12MAR2020: when using dot notation to access properties in maps it is essential that the part in front of the period references an existing map if it does not the 'dot' is basically NOT allowed
	//				because getVariable() would also return NULL if the map does not yet contain the '' property (to indicate the '' property to be set) it cannot distinguish that situation in getVariable so we do it here
	//				and we can return -2 as well to indicate invalid input in which case a TT_ERROR token should be started
	size_t l=strlen(name);
	if(l==0)return 0;
	Mvariable* variable=NULL;
	// MDH@12MAR2020: with dot notation it starts with also determining whether or not the dot notation is valid 
	//				ok the essential thing here is that the thing holding the last property must be a variable that has a map value
	char* lastPropertySeparator=strrchr(name,M_PROPERTY_SEPARATOR_CHARACTER);
	if(lastPropertySeparator!=NULL){
		name[lastPropertySeparator-name]='\0'; // pretend the name to end at the last property separator
		if(report>0)
			output("Looking for map variable '%s'.\n",name);
		else 
		if(report<0)
			(*inputInfoFunction)("Looking for map variable '%s'.\n",name);
		variable=getVariable(_environment,name,report>0); // getVariable() uses output() and we can only use that when report>0
		name[lastPropertySeparator-name]=M_PROPERTY_SEPARATOR_CHARACTER; // put the last property separator back
		if(NULL==variable)return -2; // if this happens the part in front of the period does not denote an existing variable (and it should)
		if(NULL==variable->_value)return -3; // the part in front of it does not contain a value
		if(variable->_value->type!=VT_MAP)return -4; // the part in front of it is not a map
		// if the map contains property '' it's an existing property otherwise it's a non-existing property
		Mmap* map=variable->_value->value._map;
		if(NULL==map)return -5;
		Mmapelement* mapelement=map->_first;
		while(mapelement!=NULL&&(NULL==mapelement->_variable||strcmp(mapelement->_variable->_name->chars,lastPropertySeparator+1)))
			mapelement=mapelement->_next;
		// point variable to the _variable in the map element
		variable=(mapelement!=NULL?mapelement->_variable:NULL);
	}else // ASSERT not a property reference!!!!!
		variable=getVariable(_environment,name,false);
	// if variable is undefined, return -1
	if(NULL==variable){
		if(report>0)output("'%s' does not exist.",name);else if(report<0)(*inputInfoFunction)("'%s' does not exist.",name);
		return -1;
	}
	// ASSERT variable!=NULL
	// if(report<0)inputInfo("'%s' %s recognized as an existing variable.",name,(variable?"":" NOT "));else 
	if(representsAFunction(variable)){
		if(report>0)output("'%s' holds a function, not a value.\n",name);else if(report>0)(*inputInfoFunction)("'%s' holds a function, not a value.\n",name);
		return 1;
	}
	if(report>0)output("'%s' exists.\n",name);else if(report<0)(*inputInfoFunction)("'%s' exists.\n",name);
	return 2;
}/* VALIDATED */

// MDH@11JAN2021: if we want to recognize local variables in do, for with, and function commands, we will need to be able to remember the map properties of the first argument to these function calls
//				given that such a variable should be active as soon as the value has been entered (i.e. on the comma following it), there will be one active at any time depending on the envid
// MDH@14JAN2021 NOTE: essentially the property names of a local variable map (first argument in do's and forwith's)
// MDH@25FEB2021: because we now evaluate any special function first argument, we may decide to register this map during the processing of the rest of the command to check whether a variable used exists
//				which means that there will be exactly one local variable map per (active) special function call so essentially we keep a stack of these local variables
//				NOTE we don't have to bother about the stored localvariablesMapValue because it will be garbage collected after the command is executed and therefore this reference is to be considered a weak reference
/**
 * @brief Mlocalvariables stores local variables defined in the first argument of do's and forwith's
 * 
 */
typedef struct Mlocalvariables{
	Mvalue* mapValue;
	uint64_t envid;
	struct Mlocalvariables* _prev;
}Mlocalvariables;
Mlocalvariables *_lastLocalvariables=NULL;Mallocationowner owner_localvariables=(Mallocationowner){MI_SHELL,__LINE__,1};
/**
 * @brief pushes the local variables defined in the M map wrapped in \p localvariablesMapValue on the stack of local variables
 * 
 * @param localvariablesMapValue 
 * @param envid 
 * @return true 
 * @return false 
 */
static bool pushLocalvariables(Mvalue* localvariablesMapValue,uint64_t envid){
	Mlocalvariables* _localvariables=CALLOC_1(sizeof(Mlocalvariables),'L',owner_localvariables);
	if(NULL==_localvariables){if(inputErrorFunction)(*inputErrorFunction)("Failed to store the local variables.\n");return false;}
	_localvariables->mapValue=localvariablesMapValue;
	_localvariables->envid=envid;
	_localvariables->_prev=_lastLocalvariables;
	_lastLocalvariables=_localvariables;
	return true;
}
/**
 * @brief frees local variables \p _localvariables
 * 
 * @param _localvariables 
 */
static void freeLocalvariables(Mlocalvariables* _localvariables){
	if(!_localvariables)return;
	if(_localvariables->_prev)freeLocalvariables(_localvariables->_prev);
	FREE_DISOWNED_1(_localvariables,'L',owner_localvariables);
}
/**
 * @brief (re)initializes the global linked list of local variables
 * 
 */
static void initializeLocalvariables(){
	if(_lastLocalvariables){freeLocalvariables(_lastLocalvariables);_lastLocalvariables=NULL;}
}
/**
 * @brief pop the local variable with environment id \p envid
 * 
 * @param envid 
 * @return size_t the number of local variables popped
 */
static size_t popLocalvariables(uint64_t envid){
	size_t popped=0;
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	Mlocalvariables *prevlocalvariables,*localvariables=_lastLocalvariables;
	while(localvariables!=NULL){
		if(localvariables->envid!=envid)break;
		prevlocalvariables=localvariables->_prev; // remember the predecessor (to check next)
		FREE_DISOWNED_1(localvariables,'L',owner_localvariables);
		popped++;
		localvariables=prevlocalvariables;
	}
	_lastLocalvariables=localvariables;
	return popped;
}
// TODO might become local again
/**
 * @brief returns true if a local variable with name \p identifierName exists in environment with id \p envid, false otherwise
 * 
 * @param identifierName 
 * @param envid 
 * @return true 
 * @return false 
 */
bool existsAsLocalVariable(char* identifierName,uint64_t envid){
	// if(inputInfoFunction)(*inputInfoFunction)("Checking the existence of '%s' in environment '%llu'.\n",identifierName,envid);
	if(!_lastLocalvariables||_lastLocalvariables->envid!=envid)return false;
	Mlocalvariables* localvariables=_lastLocalvariables;
	while(localvariables&&!isMapProperty(localvariables->mapValue->value._map,identifierName))localvariables=localvariables->_prev;
	return(localvariables!=NULL);
}

// MDH@11MAR2020: Ok, need to be careful here
/**
 * @brief changes the last (function) token in \p command into a variable or new variable token
 * 
 * @param command 
 * @param endOfInput 
 */
void changeFunctionTokenToAVariable(Mcommand* command,bool endOfInput){
	Mtoken* functionToken=command->_lastToken;
	char* _identifierName=_getSignificantTokenCharacters(functionToken); // same as: =_stringstart(functionToken->text,getTokenSignificantCharacterCount(functionToken)); // free asap
	// MDH@07AUG2019: here we also need to exclude explicit local variables (with argument equal to 1) as possibly existing i.e. those variables are always non-existing so they will get created in the function call execution environment!!!
	// MDH@11JAN2020 TODO: this isn't true per se, because we now allow using local variables immediately after initializing them 
	/* MDH@25FEB2021 TODO not needed anymore??????
	if(functionToken->argument==1)
		functionToken->type=TT_NEW_VARIABLE;
	else
	*/
	if(existsAsLocalVariable(_identifierName,functionToken->envid)||existsInCommand(command,_identifierName,functionToken->envid))
		functionToken->type=TT_VARIABLE;
	else{
		int8_t variableExistsIndicator=containsVariable(NULL,_identifierName,-1);
		if(variableExistsIndicator>0) // MDH@11MAR2020: containsVariable() now returns -2 (no name or environment), 0 means it is a function variable, 1 means a value variable but existing nevertheless
			functionToken->type=TT_VARIABLE;
		else
		if(variableExistsIndicator<0){
			functionToken->type=TT_NEW_VARIABLE;
			// MDH@20OCT2021 doing this fucks up the reoutputToken(): outputChar('*');
		}
		else // TODO what more can we do????
			output("%sInvalid identifier name '%s'.",M_BUG_PREFIX,_identifierName);
	}
	// replacing: functionToken->type=(command->_lastToken->argument!=1&&(existsInCommand(command,_identifierName,command->_lastToken->envid/* replacing:getSpecialFunctionCallToken(_userInputCommand->_lastToken)*/)||containsVariable(getExecutionEnvironment(),_identifierName,-1))?TT_VARIABLE:TT_NEW_VARIABLE); // MDH@07AUG2019: the function might have been created (and used) in the current command
	free(_identifierName);
	// if the reoutput token function is defined, execute it
	if(reoutputTokenFunction)(*reoutputTokenFunction)(functionToken);else outputChar('*');
	/* MDH@01OCT2019 because the token isn't actually removed the feed forward text associated with the token does not need to be deleted actually
	// MDH@20SEP2019: this function is called when a function name changes into a variable name (because the user did not enter ( behind a function name)
	//				and it makes sense to simply remove the associated feed forward of the token
	deleteAutocompletionTextOfToken(_userInputCommand->_lastToken);
	*/
	/* replacing:
	// I think we should remove ( from the behind cursor text if it was inserted
	if(endOfInput)if(amMatchingparentheses())
	if(string_length(feedforwardText)&&string_char(feedforwardText,0)=='(')
	if(!string_removed_char(feedforwardText,0))inputError("Failed to remove the function argument list opening parenthesis from the feed forward text."); // TODO is there a better way???
	*/
	// ready to redetermine the new token type!!!!
}

// MDH@19OCT2020: sometimes we want to know whether or not a certain character will finish the current token or start a new token (like when tabbing through the suggested text)
//				for that we would need a way to ask for that information
//				NOTE we're keeping commandCharacterAppended() as it is now although we're replicating code here
// MDH@11JUL2023 NOTE: currently only called once (in characterContinuesToken)
/**
 * @brief corrects the type of input character \p inputChar pointed to in \p inputCharacterType
 * 
 * @param token 
 * @param inputChar 
 * @param inputCharacterType 
 */
static void correctInputCharacterType(Mtoken const * const token,char inputChar,char* inputCharacterType){

	///* MDH@29FEB2024 removed: repeating a character prevents transitioning to another token type
	if((TOKENTYPE_IDS[token->type]&0x62)==0x62)if(inputChar==string_char(token->text,0))*inputCharacterType='r'; // MDH@04NOV2019: changed into lowercase r as we're now using R for token of type reference!!!
	//*/

	// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
	// MDH@31OCT2019: until now only a blank was identified as a whitespace character, but now I've adapted the backtick as newline character which is also treated as whitespace
	//				there's no need to act differently here, we can simply check whether the last character in the returned token is a backtick
	if(*inputCharacterType=='W'){ // whitespace isn't always 'functional' whitespace (i.e. they can be part of the actual command)
		// MDH@11JUL2023: true whitespace ends a token, and a blank should NOT end a comment token, only a newline character should (although I doubt if a new line character gets here ever)
		if(token->type==TT_COMMENT){
			if(inputChar!=M_NEWLINE_CHARACTER)*inputCharacterType='w'; // a newline character should not be considered whitespace for sure
		}else
		if(token->type==TT_ERROR||token->type==TT_DQSTRING||token->type==TT_SQSTRING)*inputCharacterType='w';
	}else
	if(*inputCharacterType==' '){ // indicating a new line request (but not in a string)
		if(token->type==TT_DQSTRING||token->type==TT_SQSTRING)*inputCharacterType='w';
	}
}

/**
 * @brief returns false if \p token does not denote a binary operator, true otherwise
 * 
 * @param token 
 * @return true \p token represents a binary operator
 * @return false \p token does not represent a binary operator
 */
static bool isNotABinaryOperator(Mtoken const * const token){
	Mstring* tokenText=token->text;
	size_t tokenLength=string_length(tokenText);
	if(tokenLength){
		char firstTokenCharacter=string_char(tokenText,0);
		if(tokenLength==2){
			char secondTokenCharacter=string_char(tokenText,1);
			// if the second token character is = the first character needs to be either ! or =
			if(secondTokenCharacter=='='){
				if(firstTokenCharacter=='='||firstTokenCharacter=='!')
					return false;
			}else{ // the second character is not an equal sign
				// arithmetic 1-character binary operator + and - (but * and / do!) have no continuation
				// & and | 1-character bitwise operator also have no valid continuation
				// ! allows < and > but nothing else (but = see above)
				switch(firstTokenCharacter){
					case '*':if(secondTokenCharacter=='*')return false;break;
					case '/':if(secondTokenCharacter=='/')return false;break;
					case '!':
					case '<':if(secondTokenCharacter=='<'||secondTokenCharacter=='>')return false;break;
					case '>':if(secondTokenCharacter=='>')return false;break;
				}
			}
		}else
		if(tokenLength==1){
			// MDH@24FEB2024: adding the \ operator!!!
			switch(firstTokenCharacter){
				case '+':case '-':case '*':case '/':
				case '<':case '>':case '&':case '|':
				case '!':case '=':case '^':
				case '\\':
					return false;
			}
		}
	}
	return true;
}

/** TODO
 * @brief returns the new token type of token \p token on appending \p inputChar of type \p inputCharacterType
 * 
 * @param token 
 * @param inputChar 
 * @param inputCharacterType 
 * @param tokenType 
 * @return int8_t 
 */
static int8_t getNewTokenType(Mtoken const * const token,char inputChar,char inputCharacterType,int8_t *tokenType){
	*tokenType=token->type;
	int8_t newTokenType=nextTokenType(*tokenType,inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
	switch(newTokenType){
		case TT_ERROR:
			// MDH@09MAR2020: interestingly this is also the situation where a variable might have to become a function
			//				this happens e.g. when an identifier at the end changed from function to variable
			if(*tokenType==TT_FUNCTION){
				*tokenType=TT_VARIABLE;
				newTokenType=nextTokenType(*tokenType,inputCharacterType);
			}else
			if(*tokenType==TT_VARIABLE){
				if(inputCharacterType=='('&&getFunction(getExecutionEnvironment(),string(token->text))!=NULL){
					*tokenType=TT_FUNCTION;
					newTokenType=nextTokenType(*tokenType,inputCharacterType);
				}
			}
			break;
		case TT_END_OF_DQSTRING:
			// MDH@13OCT2020: if the last character is a real escape character (and not simply the escape character behind the escape character, and therefore a true \)
			//				the only way to find out whether this is true is when the number of escape characters at the end is replicated is odd
			if(string_last_char_count(token->text,M_ESCAPE_CHARACTER)%2)newTokenType=TT_DQSTRING;
			break;
		case TT_END_OF_SQSTRING:
			// MDH@13OCT2020: if the last character is a real escape character (and not simply the escape character behind the escape character, and therefore a true \)
			if(string_last_char_count(token->text,M_ESCAPE_CHARACTER)%2)newTokenType=TT_SQSTRING;
			break;
	}
	/////if(amDebugging())(*inputInfoFunction)("C");
	// TODO just like unary operators expressions, maps and list end immediately
	// some combinations are (still) not allowed...
	// MDH@05FEB2024: the test newTokenType<0 was ||'ed in the following condition but nothing would be done when newTokenType<0
	//                so I moved that boolean expression negated into the else part, to follow through with doing if the token types are not the same
	if(newTokenType==(*tokenType)){
		// MDH@06FEB2024: we should exclude all invalid binary operators which weren't yet discovered
		switch(newTokenType){
			case TT_BINARY_AeRu:
			case TT_BINARY_Aeru:
			case TT_BINARY_aERu:
			case TT_BINARY_aErU:
			case TT_BINARY_aeru:
				if(isNotABinaryOperator(token)){
					newTokenType=TT_ERROR;
					if(*inputErrorFunction!=NULL)
						(*inputErrorFunction)("'%s' is not a binary operator!",string(token->text));
				}
				break;
		}
		/* 
			MDH@27MAY2019: most of the time we do allow the same one-character token behind another!!!
			MDH@12JUL2019: BUT NOT ALWAYS (values and binary operator e.g.) I have to think this through again 
			MDH@14AUG2019: start of list i.e. [ is allowed behind another [ always, also ( behind ( is also allowed, 
		*/
		if(isTokenFinished(token)){
			// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators and list element tokens (although undefined list element cells do not need to be inserted!!)
			// MDH@23JUL2019: and TT_END_OF_FUNCTION_CALL and all the other end of something tokens!!
			if((*tokenType)!=TT_LIST&&(*tokenType)!=TT_FUNCTION_CALL&&(*tokenType)!=TT_UNARY&&(*tokenType)!=TT_TERNARY_aeru&&(*tokenType)!=TT_LISTELEMENT&&(*tokenType)!=TT_END_OF_FUNCTION_CALL&&(*tokenType)!=TT_END_OF_MAP&&(*tokenType)!=TT_END_OF_LIST){
				newTokenType=TT_ERROR;
			}
		}else{ // MDH@25MAR2020: a property cannot contain a 'dot' (period) other than at the first position
			// 'finishing' a token forces creating a new one below
			if(newTokenType==TT_PROPERTY&&inputChar==M_PROPERTY_SEPARATOR_CHARACTER)
				if(isTokenUnfinished(token))
					//finishToken(lastCommandToken)
					;
		}
	}else
	if(newTokenType>=0){ // different token types
		// a shortcut assignment can NOT be turned into a equality comparison
		if(inputCharacterType=='='&&(*tokenType)==TT_ASSIGNMENT&&(token->prev->type==TT_BINARY_AeRu||token->prev->type==TT_BINARY_Aeru))
			newTokenType=TT_ERROR;
		else{
			// MDH@26MAR2020: TODO check whether this should be done elsewhere???
			if(newTokenType==TT_PROPERTY&&(*tokenType)==TT_FUNCTION){
				/*
				if(isTokenUnfinished(lastCommandToken))finishToken(lastCommandToken);
				changeFunctionTokenToAVariable(command,true);
				*/
			}
		}
	}
	return newTokenType;
}

// MDH@19OCT2020: this is a first approximation
// MDH@11JUL2023: NOTE only called once when consuming feed forward characters
/**
 * @brief returns true if input character \p inputChar of type \p inputCharacterType continues token \p token, false otherwise
 * 
 * @param token 
 * @param inputChar 
 * @param inputCharacterType 
 * @return true 
 * @return false 
 */
bool characterContinuesToken(Mtoken const * const token,char inputChar,char inputCharacterType){
	if(NULL==token)return false;
	// MDH@11JUL2023: a comment no longer is automatically continued (since a newline character finishes it now)
	if(token->type==TT_ERROR/*||token->type==TT_COMMENT*/)return true;
	// ASSERT token is not NULL and not an error // replacing: neither a error or a comment
	correctInputCharacterType(token,inputChar,&inputCharacterType);
	///////output("(%i)",inputCharacterType);
	if(inputCharacterType=='W'){/*outputChar('W');*/return true;} // whitespace always continues the current token
	/////////outputChar('Q');
	int8_t tokenType;
	int8_t newTokenType=getNewTokenType(token,inputChar,inputCharacterType,&tokenType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
	if(tokenType==TT_EXPRESSION)return false; // any non-whitespace characters ends an expression
	if(newTokenType<0)return true;
	if(isTokenFinished(token))return false;
	return(tokenType==newTokenType);
}

// MDH@28OCT2019: in order to implement the eval function the part in commandCharacterAccepted() that can work with any command is moved over to commandCharacterAppended()
//				and is called from commandCharacterAccepted() passing _userInputCommand->_lastToken in as first argument!!
//				NOTE that commandCharacterAccepted() keeps the part of the code that has to do with the endOfInput and aSuggestedCharacter flag
//				NOTE we have to use the pointer to the last command token because if we used the last command token itself, we wouldn't be able to change the last command token!!!!
//				NOTE instead we're returning the last command token (which will change if starting a new token!!!!)
/**
 * @brief appends input character \p inputChar of type \p inputCharacterType to M command \p command
 * 
 * @param command 
 * @param owner_command
 * @param inputChar 
 * @param inputCharacterType 
 * @param endOfInput whether or not the character is entered at the end of input
 * @return Mtoken* 
 */
Mtoken* commandCharacterAppended(Mcommand* command/*,Mallocationowner owner_command*/,char inputChar,char *inputCharacterType,bool endOfInput){Mallocationowner owner=getOwner(__LINE__);
	// determine the token type associated with the newly inputted character
	// MDH@28MAR2019: if we're in a binary token type with the repeatable flag set AND the user has repeated the previous first token character the inputCharacterType should become R to get the right transition
	Mtoken* lastCommandToken=(command!=NULL?command->_lastToken:NULL);
	// TODO shouldn't be outputting to the console if the command is not the user input command
	if(NULL==lastCommandToken){(*inputErrorFunction)("%sNo last command token.",M_BUG_PREFIX);return NULL;}
	Mtoken* tokenToReturn=lastCommandToken; // MDH@18JUL2023: the token to return is either a new last command token (as stored in lastCommandToken) or command->_lastToken!!!
	///* MDH@17JUL2023: most likely comment tokens are at this point in time unfinished so the following is not required!!!
	// MDH@11JUL2023: ignore comments!!! NOTE this is apparently required since otherwise characters after an embedded comment are seen as erroneous!!!
	if(*inputCharacterType!='W')
	while(lastCommandToken!=NULL&&lastCommandToken->type==TT_COMMENT&&isTokenFinished(lastCommandToken))lastCommandToken=lastCommandToken->prev;
	//*/
	// if(amDebugging())(*inputInfoFunction)("Appending '%c'.",inputChar);
	/* MDH@31OCT2019: for now not allowing special TT_WHITESPACE tokens BUT returning to the original idea of appending whitespace to the current token
	// MDH@31OCT2019: by allowing dummy i.e. TT_WHITESPACE tokens in the command the type of the token to consider isn't that of lastCommandToken per se
	//				so it's actually best if we create a new token that points to the last non-whitespace command token
	//				and we should not allow an R input character type to continue an operator like that on the previous line, or checking whether someone entered a whitespace where it's not an whitespace ending a token
	Mtoken* lastNonwhitespaceCommandToken=lastCommandToken;while(lastNonwhitespaceCommandToken->type==TT_WHITESPACE)lastNonwhitespaceCommandToken=lastNonwhitespaceCommandToken->prev;
	if(lastNonwhitespaceCommandToken==lastCommandToken){ // not behind a whitespace (newline) token
	*/
		// MDH@11JUL2023 TODO DONE note that the following code matches the code in correctInputCharacterType, so 
		correctInputCharacterType(lastCommandToken,inputChar,inputCharacterType);
		/*
		if((TOKENTYPE_IDS[lastCommandToken->type]&0x62)==0x62)if(inputChar==string_char(lastCommandToken->text,0))*inputCharacterType='r'; // MDH@04NOV2019: changed into lowercase r as we're now using R for token of type reference!!!
		// MDH@16APR2019: W indicates a whitespace character BUT it is NOT a functional whitespace character in a comment, an error, or a string literal
		// MDH@31OCT2019: until now only a blank was identified as a whitespace character, but now I've adapted the backtick as newline character which is also treated as whitespace
		//				there's no need to act differently here, we can simply check whether the last character in the returned token is a backtick
		if(*inputCharacterType=='W'){ // whitespace isn't always 'functional' whitespace (i.e. they can be part of the actual command)
			// MDH@11JUL2023: since a comment is now allowed to end any input line, we remove turning W into w when processing a comment!!!!!
			//                CORRECTION this would be a mistake since only blanks end up here, and should NOT end the comment as well!!!
			if(lastCommandToken->type==TT_ERROR||lastCommandToken->type==TT_COMMENT||lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING)*inputCharacterType='w';
		}else
		if(*inputCharacterType==' '){ // indicating a new line request (but not in a string)
			if(lastCommandToken->type==TT_DQSTRING||lastCommandToken->type==TT_SQSTRING)*inputCharacterType='w';
		}
		*/
	/*
	}
	*/
	int16_t newTokenType=0; // MDH@05JUN2019: we need newTokenType AFTER appending the last character allowed in a token (like q behind a integer or real)
	if(*inputCharacterType!='W'){ // only characters that are not whitespace can start a new token
		// MDH@31OCT2019: if we decide to always insert an empty TT_NEWLINE token on a backtick (`) newline character
		//				we have to be careful here though because if we're in a TT_WHITESPACE (dummy) token, we should look at the one before that (so essentially any TT_WHITESPACE should end immediately)
		if(*inputCharacterType=='`'){ // a (functional) new line request character
			// not acceptable when not end of input or behind another new line token
			if(!endOfInput||lastCommandToken->type==TT_WHITESPACE)return NULL;
			newTokenType=TT_WHITESPACE;
		}else{ // not the newline character (currently also `)
			uint8_t lastSignificantCommandTokenType=lastCommandToken->type;
			/* MDH@11JUL2023: skip all comments
			if(lastSignificantCommandTokenType==TT_COMMENT){
				Mtoken* lastSignificantCommandToken=lastCommandToken->prev;
				while(lastSignificantCommandToken!=NULL&&(lastSignificantCommandTokenType=lastSignificantCommandToken->type)==TT_COMMENT)
					lastSignificantCommandToken=lastSignificantCommandToken->prev;
			}
			*/
			////////// while(lastCommandToken!=NULL&&lastCommandToken->type==TT_COMMENT)lastCommandToken=lastCommandToken->prev;
			// MDH@11JUL2023: with the last command type possibly a comment, we have to look at the command token in front of the comment
			newTokenType=nextTokenType(lastSignificantCommandTokenType,*inputCharacterType); // MDH@22MAR2019: this is a bit of a quick fix, so whitespace never ends up in nextTokenType() as whitespace never ends the current token, or changes its type
		}
#ifdef __DEBUG__
	resetOutputColor();
	printf("[%d+%c->%d]",_userInputCommand->_lastToken->type,inputCharacterType,newTokenType);
	outputTokenColor(_userInputCommand->_lastToken);
#endif
		/////if(amDebugging())(*inputInfoFunction)("B");
		//MDH@17JUL2019: typically we'd get an error immediately when NOT entering a function call character ( behind a function identifier
		// MDH@02OCT2019: we need some additional corrections in certain situations i.e. do NOT end a single/double quoted string if ' or " was entered behind the escape character
		switch(newTokenType){
			case TT_ERROR:
				// MDH@09MAR2020: interestingly this is also the situation where a variable might have to become a function
				//				this happens e.g. when an identifier at the end changed from function to variable
				if(lastCommandToken->type==TT_FUNCTION){
					// we should assume that the identifier represents a (new) variable (identifier)
					changeFunctionTokenToAVariable(command,endOfInput);
					newTokenType=nextTokenType(lastCommandToken->type,*inputCharacterType);
				}else
				if(lastCommandToken->type==TT_VARIABLE){
					if(*inputCharacterType=='('&&getFunction(getExecutionEnvironment(),string(lastCommandToken->text))!=NULL){
						lastCommandToken->type=TT_FUNCTION;
						if(reoutputTokenFunction)(*reoutputTokenFunction)(lastCommandToken);else outputChar('*');
						newTokenType=nextTokenType(lastCommandToken->type,*inputCharacterType);
					}
				}
				break;
			case TT_END_OF_DQSTRING:
				// MDH@13OCT2020: if the last character is a real escape character (and not simply the escape character behind the escape character, and therefore a true \)
				//				the only way to find out whether this is true is when the number of escape characters at the end is replicated is odd
				if(string_last_char_count(lastCommandToken->text,M_ESCAPE_CHARACTER)%2)newTokenType=TT_DQSTRING;
				break;
			case TT_END_OF_SQSTRING:
				// MDH@13OCT2020: if the last character is a real escape character (and not simply the escape character behind the escape character, and therefore a true \)
				if(string_last_char_count(lastCommandToken->text,M_ESCAPE_CHARACTER)%2)newTokenType=TT_SQSTRING;
				break;
		}

		// MDH@29FEB2024: a binary operator token is never 3 characters
		if(newTokenType>=TT_BINARY_aeru&&newTokenType<=TT_BINARY_Aeru&&newTokenType==lastCommandToken->type){
			(*inputInfoFunction)("Binary operator continuation.");
			if(lastCommandToken->significantCharacterCount>=2||string_length(lastCommandToken->text)>=2){
				finishToken(lastCommandToken);
				newTokenType=TT_ERROR;
				/////if(*inputErrorFunction)
				(*inputErrorFunction)("A binary operator may contain at most 2 characters.");
				// TODO should be finish the binary operator token?????
			}
		}

		/////if(amDebugging())(*inputInfoFunction)("C");
		// TODO just like unary operators expressions, maps and list end immediately
		// some combinations are (still) not allowed...
		// MDH@05FEB2024: here we can make the same change as we did in characterContinuesToken()
		//                i.e. move newTokenType<0 to the else part (and comment out the inner if test)
		if(/*newTokenType<0||*/newTokenType==lastCommandToken->type){
			/* 
			   MDH@27MAY2019: most of the time we do allow the same one-character token behind another!!!
			   MDH@12JUL2019: BUT NOT ALWAYS (values and binary operator e.g.) I have to think this through again 
			   MDH@14AUG2019: start of list i.e. [ is allowed behind another [ always, also ( behind ( is also allowed, 
			*/
			///if(newTokenType==lastCommandToken->type){
				if(isTokenFinished(lastCommandToken)){
					// MDH@16APR2019: most tokens cannot follow each other directly except for unary and TODO ternary operators and list element tokens (although undefined list element cells do not need to be inserted!!)
					// MDH@23JUL2019: and TT_END_OF_FUNCTION_CALL and all the other end of something tokens!!
					if(lastCommandToken->type!=TT_LIST&&lastCommandToken->type!=TT_FUNCTION_CALL&&lastCommandToken->type!=TT_UNARY&&lastCommandToken->type!=TT_TERNARY_aeru&&lastCommandToken->type!=TT_LISTELEMENT&&lastCommandToken->type!=TT_END_OF_FUNCTION_CALL&&lastCommandToken->type!=TT_END_OF_MAP&&lastCommandToken->type!=TT_END_OF_LIST){
						newTokenType=TT_ERROR;
						if(amVerbose())(*inputErrorFunction)("Token already finished!");
					}
				}else{ // MDH@25MAR2020: a property cannot contain a 'dot' (period) other than at the first position
					// 'finishing' a token forces creating a new one below
					if(newTokenType==TT_PROPERTY&&inputChar==M_PROPERTY_SEPARATOR_CHARACTER)
						if(isTokenUnfinished(lastCommandToken))
							finishToken(lastCommandToken);
				}
			///}
		}else
		if(newTokenType>=0){ // different (regular) token types
			// a shortcut assignment can NOT be turned into a equality comparison
			if(*inputCharacterType=='='&&lastCommandToken->type==TT_ASSIGNMENT&&(lastCommandToken->prev->type==TT_BINARY_AeRu||lastCommandToken->prev->type==TT_BINARY_Aeru)){
				newTokenType=TT_ERROR;
				//if(amVerbose())
				/////if(*inputErrorFunction)
				(*inputErrorFunction)("A shortcut operator assignment cannot change into an equality.");
			}else{
				// MDH@26MAR2020: TODO check whether this should be done elsewhere???
				if(newTokenType==TT_PROPERTY&&lastCommandToken->type==TT_FUNCTION){
					if(isTokenUnfinished(lastCommandToken))
						finishToken(lastCommandToken);
					changeFunctionTokenToAVariable(command,true);
				}
			}
		}
		/////if(amDebugging())(*inputInfoFunction)("D");
		// MDH@03MAY2019: no matter what the new token type is, any token of type TT_EXPRESSION always ends immediately...
		//				this is because the first (offset) token in a command is always of type TT_EXPRESSION which should end immediately on any next token although significantCharacterCount will still be zero
		//				this way it will always be there!!
		/* MDH@06FEB2024: now added negated to the if in the else part
		if(newTokenType<0){

		}else
		*/
		if(newTokenType>=0&&(newTokenType!=lastCommandToken->type||lastCommandToken->type==TT_EXPRESSION||isTokenFinished(lastCommandToken))){
			///////////if(amVerbose())outputInfo("!");/////(*inputInfoFunction)("New token!");
			// MDH@10APR2019: NOT every new token type starts a new token:
			//				if we're in a binary operator and move to another binary operator type it's an extension
			//				NO we decide NOT to do this when the command is evaluated we should compose the values and apply the operators
			///////if(!isBinaryOperatorTokenType(_userInputCommand->_lastToken->type)||!isBinaryOperatorTokenType(newTokenType))

			// MDH@16APR2019: a character that is assumed to indicate the assignment operator has to be checked because it could well be the = that starts the binary equality operator
			//				which means we have to switch from assignment token to BearU token (which is unfinished)
			if(newTokenType==TT_ASSIGNMENT){
				// checking for validity of accepting as assignment is not that easy
				// we can allow a binary operator in front of the assignment of course in that case it definitely is an assignment if it is not the = is an error!!
				bool behindBinaryOperator=(lastCommandToken->type==TT_BINARY_AeRu||lastCommandToken->type==TT_BINARY_Aeru);
				// NOTE if behind binary operator there must always be a token in front of it, so lastTokenToCheck cannot be NULL!!
				// MDH@21MAY2019: possibly we have multiple tokens representing a binary operator (like ** << and >> which are allowed!!!) so we need to skip all binary operators in front of the assignment character
				Mtoken* lastTokenToCheck=lastCommandToken;
				if(behindBinaryOperator)while(lastTokenToCheck->type>=3&&lastTokenToCheck->type<=7)lastTokenToCheck=lastTokenToCheck->prev;
				if(amVerbose())(*inputInfoFunction)("Type of token to check: %s.",TOKENTYPE_STRING[lastTokenToCheck->type]);
				// ASSERT lastTokenToCheck should either represent a variable or the end of a list element to allow for operator
				// MDH@17NOV2019: we now allow multiple index elements after one another not just one
				//				but we do need a variable in front of those
				while(lastTokenToCheck&&lastTokenToCheck->type==TT_END_OF_LIST){ // end of a list
					// we have to find the associated start of the list, and the token in front of that (which should be a variable!!!)
					// which is easy because the expr tells us the start of the list BUT 
					lastTokenToCheck=lastTokenToCheck->expr;
					///////////if(amVerbose())(*inputInfoFunction)("Presumed list start token");
					if(lastTokenToCheck)lastTokenToCheck=lastTokenToCheck->prev;else (*inputErrorFunction)("%s","Start of index list not found!");
				}
				// two options: = behind a binary operator without variable (or list) in front of it is not allowed, i.e. an error, otherwise we assume that = represents the first = of == the equality operator...
				if(lastTokenToCheck==NULL||(lastTokenToCheck->type!=TT_VARIABLE&&lastTokenToCheck->type!=TT_NEW_VARIABLE&&lastTokenToCheck->type!=TT_PROPERTY)){
					if(behindBinaryOperator){
						newTokenType=TT_ERROR;
						//if(amVerbose())(*inputErrorFunction)("No variable to assign to.");
					}else
						newTokenType=TT_BINARY_aErU;
				}
			}
			/////if(amDebugging())(*inputInfoFunction)("E");
			// MDH@23JUL2019: _getToken() will now also use newTokenType to set the (initial) type of the new token
			// MDH@23SEP2019: replacing _getToken() call by createUserInputCommandToken (and generating an error when this goes wrong somehow)
			// MDH@26OCT2020 TODO: shouldn't I be owning the new command token?????
			// MDH@11JUL2023: since lastCommandToken is the last SIGNIFICANT command token (in front of any comments), we need now to pass command->_lastToken instead of lastCommandToken
			// MDH@18JUL2023: with owner_command now added to the parameters, we can immediately take ownership of a new last command token
			//                TODO come up with a better idea to do this because theoretically lastCommandToken should be reowned at the moment it is assigned
			//                     the problem here is that when we skipped comments, we would be returning a lastCommandToken which is NOT the registered last token
			//                     and that's what's causing the problems!!!!
			tokenToReturn=disowned_token(owned_token(_getNewCommandToken(command->_lastToken/* replacing: lastCommandToken */,newTokenType/*,endOfInput*/),owner),owner);
			lastCommandToken=tokenToReturn; // MDH@18JUL2023: continue with tokenToReturn

			if(endOfInput)if(updateLastTokenAutocompletionTextFunction)(*updateLastTokenAutocompletionTextFunction)(false); // MDH@28FEB2020: a bit of a nuisance...

			if(NULL==lastCommandToken)return NULL;

			/* replacing:
			_userInputCommand->_lastToken=_getToken(_userInputCommand->_lastToken,newTokenType);
			// MDH@23SEP2019: moved out of _getToken (because not always will we need to update the feed forward text when new tokens are created, e.g. in copyUserInputCommand()!)
			setLastTokenType(newTokenType,endOfInput);
			*/
			/////if(amDebugging())(*inputInfoFunction)("F");
			/*
			if(amVerboseDebugging())
				printf("@%p=%p?:%s",_userInputCommand->_firstToken,_userInputCommand->_lastToken,string(_userInputCommand->_firstToken->text));
			*/
			/* MDH@23JUL2019 TODO check what we still need of the following!!!!: replacing:
			// ending a function call, list or map is only allowed with expr defined
			if(_userInputCommand->_lastToken->type==TT_END_OF_FUNCTION_CALL||_userInputCommand->_lastToken->type==TT_LISTELEMENT||_userInputCommand->_lastToken->type==TT_END_OF_LIST||_userInputCommand->_lastToken->type==TT_END_OF_MAP){
				if(_userInputCommand->_lastToken->expr){ // i.e. pointing to some token that should be of the right type!!!
					// check whether the match is correct
					switch(newTokenType){
						case TT_END_OF_FUNCTION_CALL:
							if(_userInputCommand->_lastToken->expr->type!=TT_FUNCTION_CALL&&_userInputCommand->_lastToken->expr->type!=TT_EXPRESSION){
								/////(*inputErrorFunction)("%s","No function call or expression to end here!");
								(*inputErrorFunction)("End of function call/expression character does not match '%s' of type '%s'!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
						case TT_LISTELEMENT:
							// MDH@09JUL2019: a comma (starting a list element) is allowed in a function call accepting multiple parameters)
							if(_userInputCommand->_lastToken->expr->type!=TT_LIST&&_userInputCommand->_lastToken->expr->type!=TT_MAP){
								// so if it's a function call it might be allowed
								if(_userInputCommand->_lastToken->expr->type!=TT_FUNCTION_CALL){
									(*inputErrorFunction)("First expression token '%s' of type '%s' does not start a list or map!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
									_userInputCommand->_lastToken->type=TT_ERROR;
								}else{
									// the token in front of the function call token should denote a function
									char* functionName=string(_userInputCommand->_lastToken->expr->prev->text);
									Mfunction* function=getFunction(getExecutionEnvironment(),functionName);
									/////////////if(amVerbose())output("Function '%s'.\n",functionName);
									// TODO this works for two-argument functions but can we tell which argument this is?????
									// we have to count the parameters by counting the list elements
									uint32_t listElementCount=getListElementCount()+1;
									if(!function||listElementCount>=function->_parameterMap->numberOfElements){
										if(function)
											(*inputErrorFunction)("Function '%s' does not allow for more than %u argument(s).",functionName,listElementCount);
										else
											(*inputErrorFunction)("Cannot tell whether function '%s' allows for more than %u argument(s).",functionName,listElementCount);
										_userInputCommand->_lastToken->type=TT_ERROR;
									}
								}
							}
							break;
						case TT_END_OF_LIST:
							if(_userInputCommand->_lastToken->expr->type!=TT_LIST){
								(*inputErrorFunction)("First token '%s' of type '%s' does not start a list!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
						case TT_END_OF_MAP:
							if(_userInputCommand->_lastToken->expr->type!=TT_MAP){
								(*inputErrorFunction)("First expression token '%s' of type '%s' does not start a map!",string(_userInputCommand->_lastToken->expr->text),TOKENTYPE_STRING[_userInputCommand->_lastToken->expr->type]);
								//(*inputErrorFunction)("No map to end here!");
								_userInputCommand->_lastToken->type=TT_ERROR;
							}
							break;
					}
					// if we do the following we need to move 
					// TODO we can do the following even on error but what if we didn't???
					///////////////////_userInputCommand->_lastToken->expr=_userInputCommand->_lastToken->expr->expr;
				}else{
					(*inputErrorFunction)("%s","Can't end a (function argument) list or map here!");
					_userInputCommand->_lastToken->type=TT_ERROR;
				}
			}
			// taken over in _getToken()
			_userInputCommand->_lastToken->type=newTokenType;
			*/

			// MDH@27MAY2019: a lot of tokens are one-character tokens
	
			// MDH@15APR2019: there are some other characters as well, that immediately end the token like parentheses, comma's and semicolons and ? and : TODO are there more??????
			if(isTokenUnfinished(lastCommandToken)){
				if(isOneCharacterTokenType(lastCommandToken->type)){
					setTokenSignificantCharacterCount(lastCommandToken,1);
					////////bool initializationsChanged=false;
					// MDH@06AUG2019: these are also the tokens we need to recognize for keeping track of the initialized variables (and the level)
					switch(lastCommandToken->type){
						case TT_ASSIGNMENT:
							if(lastCommandToken->argument==1)lastCommandToken->argument=-1; // indicating that whatever comes next, should not be considered local variables, i.e. should NOT be marked 'automatically' as new variables because they should exist!!!
							/* replacing:
							if(_userInputCommand->_lastToken->prev->type==TT_NEW_VARIABLE)if(initializable()){
								char* newVariableName=string(_userInputCommand->_lastToken->prev->text);
								if(pushInitialization(newVariableName)){
									initializationsChanged=true;
									if(amVerbose())(*inputInfoFunction)("New variable '%s' initialization registered.",newVariableName);
								}else
									(*inputErrorFunction)("Failed to register the initialization of new variable '%s'.",newVariableName);
							}
							*/
							break;
						case TT_FUNCTION_CALL:
							if(lastCommandToken->prev->type==TT_FUNCTION){
								/* replacing:
								char* functionName=string(_userInputCommand->_lastToken->prev->text);
								// we do not need to store the function name itself, just the argument that will contain the local variable initializations
								if(pushInitialization("(")){
									_lastInitialization->argument=((!strcmp(functionName,DOFUNCTION_NAME)||!strcmp(functionName,FORFUNCTION_NAME)?0:(!strcmp(functionName,DEFINEUSERFUNCTION_NAME)?1:-1)));
									initializationsChanged=true;
									if(amVerbose())(*inputInfoFunction)("Function '%s' registered.",functionName);
								}else
									(*inputErrorFunction)("Failed to register function call '%s'.",functionName);
								*/
							}else
								(*inputErrorFunction)("No function in front of function call.");
							break;
						case TT_END_OF_FUNCTION_CALL:
							if(lastCommandToken->argument==-1)lastCommandToken->argument=1;
							/* replacing:
							if(pushInitialization(")")){ // will set the argument count appropriately...
								if(amVerbose())(*inputInfoFunction)("End of function call registered.");
								initializationsChanged=true;
							}else
								(*inputErrorFunction)("Failed to register the end of a function call.");
							*/
							break;
						case TT_LISTELEMENT:
							if(lastCommandToken->expr&&lastCommandToken->expr->type==TT_FUNCTION_CALL){ // TODO is this correct?
								// MDH@01MAR2021 removal: if we do this the argument following would be considered containing local variables again, which we do not want to happen
								//			   TODO what should we do here then???????
								// if(lastCommandToken->argument==-1)lastCommandToken->argument=1;
								/* replacing:
								// not any comma is a function call argument separator!!!
								if(pushInitialization(",")){
									_lastInitialization->argument--; // decrement the argument count (once it is zero any initialization is local to the function call)
									if(amVerbose())(*inputInfoFunction)("End of function argument with count set to %lld.",_lastInitialization->argument);
									initializationsChanged=true;
								}else
									(*inputErrorFunction)("Failed to register a next function call argument!");
								*/
							}
						default:
							break;
					}
					/* removing:
					if(amDebugging()){
						if(!initializationsChanged)
							(*inputInfoFunction)("Token with text '%c' of type %s considered to be a one character token.",inputChar,TOKENTYPE_STRING[_userInputCommand->_lastToken->type]);
						else 
						if(!amVerbose())showInitializations();
					}
					*/
				}
				/* replacing:
				if(_userInputCommand->_lastToken->type!=TT_ERROR&&_userInputCommand->_lastToken->type!=TT_COMMENT&&_userInputCommand->_lastToken->type!=TT_DQSTRING&&_userInputCommand->_lastToken->type!=TT_SQSTRING)
					if(inputCharacterType=='('||inputCharacterType=='['||inputCharacterType=='{'||inputCharacterType==','||inputCharacterType==';'||inputCharacterType==':'||inputCharacterType=='?')
						_userInputCommand->_lastToken->significantCharacterCount=1;
				*/
			}
			// TODO should we write the associated colors here?????
			/////// moved over to commandCharacterAccepted because it's definitely not part of an inline command (evaluated by Meval!!!) outputTokenColor(lastCommandToken);
			/////if(amDebugging())(*inputInfoFunction)("H");
		}
	}else{ // a functional whitespace character, ends a current token!!
		if(isTokenUnfinished(lastCommandToken)&&lastCommandToken->type!=TT_EXPRESSION) // MDH@22MAR2019: first whitespace character in a non-whitespace token ends the current token (but should never change its type (see NO_TRANSITIONS))
			finishToken(lastCommandToken);
		if(inputChar==' ')inputChar=M_WHITESPACE_CHARACTER; // MDH@31OCT2019: so we can make the blanks visible!!
	}
	/////if(amDebugging())(*inputInfoFunction)("I");
	// append the typed character at getUserInputLength() minus current token offset in _userInputCommand->_lastToken->text
	string_append_char(lastCommandToken->text,inputChar);
	/////if(amDebugging())(*inputInfoFunction)("J");

	if(newTokenType<0)finishToken(lastCommandToken);

	return tokenToReturn; // will either be the newly created lastCommandToken, or command->_lastToken!!!

}

// MDH@25FEB2021: how about allowing the evaluation of a part of a subcommand (in its own evaluation environment), called by Mevalfunction() as well as the tokenizer for evaluating special function call arguments (that initialize local variables)
/**
 * @brief returns the text representation of the subcommand that starts with token \p firstSubcommandToken and ends with token \p lastSubcommandToken
 * 
 * @param firstSubcommandToken 
 * @param lastSubcommandToken 
 * @param defaultSubcommandText 
 * @return Mstring* 
 */
static Mstring* getSubcommandText(Mtoken const * const firstSubcommandToken,Mtoken const * const lastSubcommandToken,Mstring const * const defaultSubcommandText){
	if(defaultSubcommandText!=NULL)return defaultSubcommandText; // TODO is this correct??????
	Mallocationowner owner=getOwner(__LINE__);
	Mstring* _subcommandText=owned_string(_getString(string(firstSubcommandToken->text)),owner);
	if(!_subcommandText)return NULL;
	Mtoken* subcommandToken=firstSubcommandToken->next;
	while(subcommandToken){
		if(!string_append(_subcommandText,string(subcommandToken->text)))break; // failure
		if(subcommandToken==lastSubcommandToken)break;
		subcommandToken=subcommandToken->next;
	}
	return disowned_string(_subcommandText,owner);
}
/**
 * @brief returns the evaluated value of the sub command that starts with token \p firstSubcommandToken and ends with token \p lastSubcommandToken
 * 
 * @param firstSubcommandToken 
 * @param lastSubcommandToken 
 * @param expressionTypeToIgnore 
 * @param endTokenTypes 
 * @param endTokenTypeCount 
 * @param commandText 
 * @param source 
 * @return Mvalue* 
 */
Mvalue* getSubcommandValue(Mtoken const * const firstSubcommandToken,Mtoken const * const lastSubcommandToken,TokenType expressionTypeToIgnore,TokenType endTokenTypes[],uint8_t endTokenTypeCount,Mstring const * const commandText,char* source){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _subcommandValue=NULL;
	if(firstSubcommandToken&&lastSubcommandToken){
		Mstring* _commandText=NULL;
		if(amVerboseDebugging()){
			_commandText=getSubcommandText(firstSubcommandToken,lastSubcommandToken,commandText);
			output("Evaluating subcommand '%s'.\n",string(_commandText));
		}
		Menvironment* _evalEnvironment=owned_environment(__environment(),owner);
		if(_evalEnvironment!=NULL){
			_evalEnvironment->_name=owned_chars(_getChars(source),Msubowner(owner,1));
			if(pushExecutionEnvironment(disowned_environment(_evalEnvironment,owner))){
				// similar to getCommandValue() except starting at the given token instead (and without the verbose output)
				int8_t aValidSubcommandIndicator=isAValidLastCommandTokenIndicator(lastSubcommandToken,expressionTypeToIgnore,false); // TODO we might need to make command immutable because I suppose we do not want it to be changed
				if(aValidSubcommandIndicator>0){ // a valid command
					_evalEnvironment->expressionToken=firstSubcommandToken; // prepare the current environment for executing the command
					_subcommandValue=getValueOfExpression(source,'e',endTokenTypes,endTokenTypeCount); // NOTE could've used getExecutionEnvironment()->_name->chars but we know it would be eval!!
				}else
					output("%sSubcommand '%s' invalid (error indicator code %i)!\n",M_ERROR_PREFIX,string(_commandText=getSubcommandText(firstSubcommandToken,lastSubcommandToken,_commandText)),aValidSubcommandIndicator);
				// replacing: _subcommandValue=getCommandValue(_evalCommand,owner,'e');
				popExecutionEnvironment(); // pop the eval environment we successfully pushed
			}else{
				free_environment(_evalEnvironment); // MDH@17JUN2020: TODO check if it is correct to do that here
				output("%sUnable to setup the evaluation of '%s'.\n",M_ERROR_PREFIX,string(_commandText=getSubcommandText(firstSubcommandToken,lastSubcommandToken,_commandText)));
			}
		}else
			output("%sFailed to evaluate '%s'.\n",M_ERROR_PREFIX,string(_commandText=getSubcommandText(firstSubcommandToken,lastSubcommandToken,commandText)));
		if(!commandText&&_commandText)free_string(_commandText);
	}
	return _subcommandValue;
}

// MDH@25OCT2020: tokenization consist of converting a text to a command so an immutable commandText is provided to be converted into a command
//				NOTE that the text is tokenized within the current execution environment whatever that may be at this moment
/**
 * @brief returns the command represented by text \p commandText
 * 
 * @param commandText 
 * @return Mcommand* 
 */
Mcommand* _getTextCommand(char const * commandText){Mallocationowner owner=getOwner(__LINE__);
	Mcommand* _command=NULL;
	char commandCharacter=(commandText!=NULL?*commandText:'\0');
	if(commandCharacter){ // commandText should not be NULL and the first character in it should not be '\0'
		if(amVerboseDebugging())
			output("%s","Parsing '");
		_command=owned_command(_getNewCommand(true),owner);
		if(_command!=NULL){
			Mtoken* _commandToken=_command->_firstToken;
			/* already set: 
			_evalCommandToken->expr=NULL; // MDH@28OCT2019: essential bto'
			Mtoken* _lastEvalCommandToken=_evalCommandToken;
			*/
			char commandCharacterType;
			Mtoken* newCommandToken=NULL;
			while(commandCharacter){
				if(amVerboseDebugging())
					outputChar(commandCharacter);
				commandCharacterType=INPUTCHARACTERTYPES[commandCharacter];
				newCommandToken=commandCharacterAppended(_command,commandCharacter,&commandCharacterType,false); // MDH@29OCT2019: we have to pass false all the time TODO not this way please
				if(newCommandToken==NULL){output("%sFailed to append command character '%c' parsing '%s'!\n",M_ERROR_PREFIX,commandCharacter,commandText);break;}
				// MDH@28MAY2020: take over ownership of the new token returned
				if(newCommandToken!=_command->_lastToken)
					_command->_lastToken=owned_token(newCommandToken,Msubowner(owner,1)); // update our eval command's last token TODO do we need to test here????
				if(NULL==_command->_lastToken)break;
				commandCharacter=*(++commandText); // increment the char pointer to point to the next character to consume
			}
			if(commandCharacter){FREE_COMMAND(_command,owner);_command=NULL;} // some error occurred
		}
		if(amVerboseDebugging())
			output("'\n.");
	}
	return disowned_command(_command,owner);
}

// this is a fun method, allowing us to parse and evaluate any command (which we're gonna need when running M starting with commands to execute from a file)
/**
 * @brief returns the result of evaluating the text wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* 
 */
Mvalue* Mevalfunction(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _evalValue=NULL;
	Mstring* _evalValueText=owned_string(_getValueText(value,true),owner);
	if(_evalValueText!=NULL){
		if(amVerbose())output("To evaluate: '%s'.\n",string(_evalValueText));
		///*
		Mcommand* _evalCommand=owned_command(_getTextCommand(string(_evalValueText)),owner);
		if(_evalCommand!=NULL){
			// MDH@25FEB2021: delegate to getSubcommandValue, which accepts a first and last command token, and the command text (TODO which we could make it construct itself)
			_evalValue=getSubcommandValue(_evalCommand->_firstToken->next,_evalCommand->_lastToken,TT_EXPRESSION,NULL,0,_evalValueText,"eval");
			/* replacing (and embedded (somewhat adapted) now in getSubcommandValue):
			if(_evalCommand->_lastToken){
				Menvironment* _evalEnvironment=owned_environment(__environment(),owner);
				if(_evalEnvironment){
					_evalEnvironment->_name=owned_chars(_getChars("eval"),Msubowner(owner,1));
					if(pushExecutionEnvironment(disowned_environment(_evalEnvironment,owner))){
						_evalValue=getCommandValue(_evalCommand,owner,'e');
						popExecutionEnvironment(); // pop the eval environment we successfully pushed
					}else{
						free_environment(_evalEnvironment); // MDH@17JUN2020: TODO check if it is correct to do that here
						output("%sUnable to setup the evaluation of '%s'.\n",M_ERROR_PREFIX,string(_evalValueText));
					}
				}else
					output("%sFailed to evaluate '%s'.\n",M_ERROR_PREFIX,string(_evalValueText));
			}
			*/
			FREE_COMMAND(_evalCommand,owner);
		}
		//*/
		/* replacing:
		Mcommand* _evalCommand=owned_command(_getNewCommand(true),owner);
		if(_evalCommand){
			Mtoken* _evalCommandToken=_evalCommand->_firstToken;
			uint32_t pos=0;
			char evalInputChar,evalInputCharType;
			if(amVerbose())output("%s","Parsing '");
			Mtoken* newLastEvalCommandToken=NULL;
			while(pos<string_length(_evalValueText)){
				evalInputChar=string_char(_evalValueText,pos++);
				if(amVerbose())outputChar(evalInputChar);
				evalInputCharType=INPUTCHARACTERTYPES[evalInputChar];
				// MDH@26OCT2020: A HA commandCharacterAppended is the one doing the damage
				//				because if a new command token is create it is not currently owned
				newLastEvalCommandToken=commandCharacterAppended(_evalCommand,evalInputChar,&evalInputCharType,false); // MDH@29OCT2019: we have to pass false all the time TODO not this way please
				// MDH@28MAY2020: take over ownership of the new token returned
				// MDH@26OCT2020: owned_token replacing SUBOWNED(OWNED()) not completely certain why owned_token is to be used I guess that's because there's text in there as well to be owned!!!!
				if(newLastEvalCommandToken!=_evalCommand->_lastToken)
					_evalCommand->_lastToken=owned_token(newLastEvalCommandToken,Msubowner(owner,1)); // update our eval command's last token TODO do we need to test here????
				if(!_evalCommand->_lastToken)break;
			}
			if(amVerbose())outputInfo("'.");
			if(_evalCommand->_lastToken){
				// just like with do() we have to evaluate the command in a subenvironment
				Menvironment* _evalEnvironment=owned_environment(__environment(),owner);
				if(_evalEnvironment){
					_evalEnvironment->_name=owned_chars(_getChars("eval"),Msubowner(owner,1));
					// MDH@27OCT2020: when pushing an environment, it is being wrapped in a value, therefore we need to disown it before passing it
					if(pushExecutionEnvironment(disowned_environment(_evalEnvironment,owner))){
						// MDH@28FEB2020: only eval now uses getCommandValue() but getCommandValue() shares using isAValidCommand() with M.c, isAValidCommand() is therefore adjusted to NOT remove any error token at the end, because that was only done to be able to re-use the command (which we do not need to here)
						//				TODO we might decide to NOT allow comments in evaluated commands but at the moment we do OR we could move the comment out before!!!
						_evalValue=getCommandValue(_evalCommand,owner,'e'); // NOTE only place where getCommandValue() is called in Mshell.c
						popExecutionEnvironment(); // pop the eval environment we successfully pushed
					}else{
						// MDH@27OCT2020: since pushing the (disowned) environment failed we should free it here (otherwise the gc will take care of freeing it)
						free_environment(_evalEnvironment); // MDH@17JUN2020: TODO check if it is correct to do that here
						output("%sUnable to setup the evaluation of '%s'.\n",M_ERROR_PREFIX,string(_evalValueText));
					}
				}else
					output("%sUnable to evaluate '%s'.\n",M_ERROR_PREFIX,string(_evalValueText));
			}else
				output("%sUnable to evaluate the invalid command '%s'.\n",M_ERROR_PREFIX,string(_evalValueText));
			FREE_COMMAND(_evalCommand,owner); // clean up the command
		}
		*/
		FREE_STRING(_evalValueText,owner);
	}
	// output("Done evaluating...\n"); // DEBUG
	return _evalValue;
}

// MDH@27OCT2020: Manonymousfunction and Mdefinefunction moved over here, so we can parse 
//				the body provided into a list of commands (=tokens) to execute 

// end very special M functions

// MCommand stuff
/**
 * @brief returns \p _command owned by \p owner_command
 * 
 * @param _command 
 * @param owner_command 
 * @return Mcommand* \p _command owned by \p owner_command
 */
Mcommand* owned_command(Mcommand* _command,Mallocationowner owner_command){
	if(!_command)return NULL;
	if(_command->_firstToken)owned_token(_command->_firstToken,Msubowner(owner_command,1));
	return OWNED(_command,owner_command);
}
/**
 * @brief returns \p command disowned by \p owner_command
 * 
 * @param _command 
 * @param owner_command 
 * @return Mcommand* \p command disowned by \p owner_command
 */
Mcommand* disowned_command(Mcommand* _command,Mallocationowner owner_command){
	if(!_command)return NULL;
	if(_command->_firstToken)disowned_token(_command->_firstToken,Msubowner(owner_command,1));
	return DISOWNED(_command,owner_command);
}
/**
 * @brief frees M command \p _command
 * 
 * @param _command 
 */
void free_command(Mcommand* _command){
	if(!_command)return;
	if(_command->_firstToken)free_token(_command->_firstToken); //Msubowner(owner_command,1)); // will free ALL connected tokens!!!
	FREE_1(_command,'K');
}

// MDH@23SEP2019: whenever the type of the current token (_userInputCommand->_lastToken) changes (possibly with the start of a new token), so will the feed forward text associated with that token
//				therefore it is best to set the last token type using a separate function
// MDH@03OCT2019: every time the token type changes we need to sync the immediate feed forward text as well!!!!
/**
 * @brief sets the type of token \p token to \p tokenType
 * @param token
 * @param tokenType
 */
void setTokenType(Mtoken* token,TokenType tokenType/*,bool endOfInput*/){
	if(token){
		if(tokenType!=token->type){
			// MDH@04OCT2019 moved to input loop removing: if(!deleteLastTokenImmediateFeedforwardText())inputError("Failed to remove the current token immediate feed forward text.");
			token->type=tokenType;
			// MDH@04OCT2019 moved to input loop removing: if(!updateImmediateFeedforwardTextOfUserInputCommand())inputError("Failed to add the current token immediate feed forward text.");
		}
	}
	///////// MDH@29OCT2019 probably don't need this here anymore: if(endOfInput)updateLastTokenAutocompletionText();
}

/**
 * @brief propagates the properties of \p prevToken assuming newTokenType to be the successors 
 * 
 * @param prevToken 
 * @param newTokenType 
 * @return true 
 * @return false 
 */
static bool tokenPropertiesPropagated(Mtoken const * const prevToken){
	TokenType newTokenType=TT_ERROR; // assume failure
	if(prevToken!=NULL&&prevToken->next!=NULL){
		// initialize _token and newTokenType
		Mtoken* _token=prevToken->next;
		newTokenType=_token->type;
		
		// now we have the block of code copied from _getToken
		/////if(amDebugging())inputInfo("E2");
		// MDH@27MAY2019: let's by default copy prevToken-expr over

		// MDH@18MAY2019: if a , starts an expression we won't be pointing to the opening parenthesis!!!
		//				which would mean that on verification we'd have to jump back until we found a non-comma!!!
		//				so we can fix this by NOT including TT_EXPRESSION prev tokens to point to!!!
		//				BUT the first (dummy) expression token should be included though!!!
		// TODO having to test an expression for starting with ( is a bit of a nuisance (so we won't accidently do that on the initial expression token and any comma token!!!)
		// MDH@27MAY2019: set expr NOTE the first token behind the (start of) expression token, should keep pointing to NULL
		// MDH@23JUL2019: we're going to change this a little bit because we want } ) ] to point to what the expr of prevToken points to
		//				and NOT wait for the next token
		//				typically a new token points to the same expr that the predecessor points to
		//				but we want 
		// take special care when the new token ends a list, map or function call
		// MDH@29OCT2019: no need for \p first anymore (that we used previously) because testing for the first TT_EXPRESSION can also be done by looking at the text in the expression
		//				TODO in time we should change the first token into a WHITESPACE token
		if(prevToken->type==TT_LIST||prevToken->type==TT_FUNCTION_CALL||prevToken->type==TT_MAP||(prevToken->type==TT_EXPRESSION&&string_length(prevToken->text)>0&&string_char(prevToken->text,0)!=' '))
			_token->expr=prevToken;
		else
			_token->expr=prevToken->expr; // DEFAULT: take over the expr of the previous token
		
		// MDH@09AUG2019: before we actually kill the expr in the end of function call we update the envid
		// if ending a special function call, we should zero the last set octet, but determining whether that is the case is not as easy as it seems
		// I suppose the argument of the expr field of the new token will tell us if it is a special function call (because the argument field would then be positive)
		if(newTokenType==TT_END_OF_FUNCTION_CALL&&_token->expr!=NULL&&_token->expr->type==TT_FUNCTION_CALL&&_token->expr->argument>0){
			//////////inputInfo("*** End of special function call! ***");
			// we have to decrement the octet that should be incremented
			// it would be nicer to make the octet we loose 0 in the process because in that case we do not need to do that when we nest again
			// the number of bits per level determines value to increment ander with and shift (at this moment the maximum depth is at most 15 i.e. 4 bits are always used to keep track of the current level)
			uint64_t ander=0,incrementoctet=0;while(incrementoctet!=(prevToken->envid&15)){ander=(ander<<M_BITS_PER_ENV_LEVEL)+((1<<M_BITS_PER_ENV_LEVEL)-1);incrementoctet++;}
			_token->envid=(((prevToken->envid>>4)<<4)+incrementoctet-1)&((ander<<4)+15); // shifting ander by 4 additional bits and adding 15 to maintain the level value (increment octet)
		}else
			_token->envid=prevToken->envid; // MDH@09AUG2019: take over the environment id!!

		// MDH@16OCT2019: if the previous token was an end of list/function call/map it was accepted and itself would be pointing to the start of the list/function call/map
		//				therefore we do not need to set 
		if(prevToken->type==TT_END_OF_LIST||prevToken->type==TT_END_OF_FUNCTION_CALL||prevToken->type==TT_END_OF_MAP){
			// MDH@23JUL2019: this new token is actually only allowed when there's a matching token, but if there isn't _token->expr will most likely be NULL
			//				TODO this is checked afterwards, so perhaps we should do that here?????
			if(_token->expr!=NULL)_token->expr=_token->expr->expr;else newTokenType=TT_ERROR;
		}
		// we still have to recognize an error
		if(newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_END_OF_MAP)if(NULL==_token->expr)newTokenType=TT_ERROR;

		/* replacing:
		if(newTokenType==TT_END_OF_LIST||newTokenType==TT_END_OF_FUNCTION_CALL||newTokenType==TT_END_OF_MAP){
			// MDH@23JUL2019: this new token is actually only allowed when there's a matching token, but if there isn't _token->expr will most likely be NULL
			//				TODO this is checked afterwards, so perhaps we should do that here?????
			if(_token->expr)_token->expr=_token->expr->expr;else newTokenType=TT_ERROR;
		}
		*/
		/*
		if(amVerbose()){
			if(_token->expr)inputInfo("Matching: %s",string(_token->expr->text));else inputInfo("%s","-");
		}
		*/
		///////if(amVerbose()){if(_token->expr)inputInfo("Pointing to %s of type %s.",string(_token->expr->text),TOKENTYPE_STRING[_token->expr->type]);else inputInfo("Nothing to point to.");}
		//////// ending with NULL means all is Ok!! if(!_token->expr)_token->expr=_userInputCommand->_firstToken; // TODO will this help???
		_token->offset=prevToken->offset+string_length(prevToken->text); // set the offset
		
		// MDH@07AUG2019: a token 'inherits' the prevIdentifier and argument of its previous token, to be adapted if necessary depending on what it is
		//				of course if prevToken is an identifier itself, the new token should point to that token and not to the identifier prevToken is pointing to
		//				how about function identifiers? they are special in that they change the argument value
		/////if(amDebugging())inputInfo("E3");
		if(prevToken->type==TT_FUNCTION){ // a function identifier that we can point to (although perhaps we should not do that?) TODO shouldn't we test whether the new token type is TT_FUNCTION_CALL instead??????
			_token->prevIdentifier=prevToken;
			// what should now be the argument value? this depends on the name of the function
			char* _functionName=_getSignificantTokenCharacters(prevToken); // free asap
			// all new tokens have argument equal to zero (and counting down on each comma encountered, so all variables created are considered global, because only the tokens with argument equal to 1 should be considered local)

			// MDH@28OCT2020: defining user functions is no longer 'special' in that the body should simply be a list of command texts and tokenized by Mdefinefunction and Manonymousfunction itself
			// MDH@20DEC2022: we can change this to encode the number of arguments left to enter somehow in _token->argument (for common non-special functions)
			if(strcmp(_functionName,DOFUNCTION_NAME)&&strcmp(_functionName,FORWITHFUNCTION_NAME)){ // not a special function (like do and forw)
				//  MDH@20DEC2022: used to assign -2 but now -3 minus the number of function arguments (so -2 would then be considered an unknown function)
				long long numberOfExpectedArguments=getNumberOfFunctionParameters(_functionName);//outputChar('X');
				_token->argument=(numberOfExpectedArguments<0?-2:-numberOfExpectedArguments-3);
				// informing the user
				if(inputInfoFunction){
					if(numberOfExpectedArguments>=0)
						(*inputInfoFunction)("Number of expected arguments: %lld.",numberOfExpectedArguments);
					else
						(*inputInfoFunction)("Number of expected arguments unknown!");
				}
			}else
				_token->argument=1;
			free(_functionName); // MDH@10APR2024: moved from after the next block here!!
			/* replacing:
			// MDH@11AUG2019: the default now no longer should be zero, because 1 will be toggled to -1 and back, therefore we should not encounter -1s in an ordinary function call
			if(!strcmp(_functionName,DOFUNCTION_NAME)||!strcmp(_functionName,FORFUNCTION_NAME)||!strcmp(_functionName,DEFINEANONYMOUSFUNCTION_NAME))_token->argument=1;
			else 
			if(!strcmp(_functionName,DEFINEUSERFUNCTION_NAME))_token->argument=2;
			*/

			// MDH@09AUG2019: special function calls have arguments that declare local variables explicitly, execution of these function calls will run in their own execution environment in which these local variables are created, 
			if(_token->argument>0){ // a special function call // MDH@09MAR2020: added >0 TODO is that correct?
				uint64_t incrementoctet=(prevToken->envid&15),environmentid=prevToken->envid,addendum=16; // addendum: what we need to add to the envid to get a new unique environment id, ander: what we need to and the envid with to make the octet to the left 0 again (ready for having nested special function calls)
				// the maximum value of incrementoctet (the environment depth) is 60/M_BITS_PER_ENV_LEVEL
				if((incrementoctet*M_BITS_PER_ENV_LEVEL)<60&&(prevToken->envid)>>((incrementoctet+1)*M_BITS_PER_ENV_LEVEL)<(2<<M_BITS_PER_ENV_LEVEL)-1){ // checking the octet to increment as well because it should not be 15 (or we would get overflow!!)
					while(incrementoctet>0){addendum<<=M_BITS_PER_ENV_LEVEL;incrementoctet--;}
					// we have to increment the addendum by 1 because we also need to increment the octet that should be incremented when a nested special function call is encountered!!
					_token->envid=(prevToken->envid+addendum+1); // ander will take care of removing what's too the left
				}else{ // can't increment
					newTokenType=TT_ERROR; // MDH@10APR2024 bug fix at the end of this block token->type is still set to newTokenType and directly setting _token->type would have no effect: replacing: _token->type=TT_ERROR;
					if(inputErrorFunction)(*inputErrorFunction)("Cannot exceed the maximum number of 15 (nested) special function calls");
				}
			}
			// every , that ends a function call argument should decrement the argument value
		}else{ // not a function identifier	
			/////if(amDebugging())inputInfo("E4");		
			if(prevToken->type!=TT_NEW_VARIABLE&&prevToken->type!=TT_VARIABLE&&prevToken->type!=TT_END_OF_FUNCTION_CALL){ // not behind a variable identifier or end of function call
				/////inputInfo("Checking new token of type %s behind token of type %s!",TOKENTYPE_STRING[newTokenType],TOKENTYPE_STRING[prevToken->type]);
				_token->prevIdentifier=prevToken->prevIdentifier;
			}else{ // behind a variable identifier or end of function call
				// MDH@01MAR2021: it's essential to skip local variables (because otherwise they would be treated as existing outside the special function call)
				//				TODO should we not also not use TT_NEW_VARIABLEs????????
				char* _identifierName=_getSignificantTokenCharacters(prevToken); // free asap
				if(prevToken->type!=TT_VARIABLE||!existsAsLocalVariable(_identifierName,prevToken->envid))
					_token->prevIdentifier=prevToken;
				else
					_token->prevIdentifier=prevToken->prevIdentifier;
				free(_identifierName);
			}
			/////if(amDebugging())inputInfo("E5");
			// what to do with the argument if a function call ends???????
			// the function name of the function call should contain the right argument value TODO check this!!!!!!!!
			// BUG FIX aha end of function call does not always end a function call, but an expression (a single opening parenthesis without a function name in front of it), so explicitly checking for that!!!
			if(prevToken->type==TT_END_OF_FUNCTION_CALL&&prevToken->expr!=NULL&&prevToken->expr->type==TT_FUNCTION_CALL)
				_token->argument=prevToken->expr->prev->argument;
			else
				_token->argument=prevToken->argument;
			/////if(amDebugging())inputInfo("E6");
			// should we change the argument??????
			if(newTokenType==TT_LISTELEMENT){ // ha ha, can't use _token->type here as not assigned yet!!!
				///////inputInfo("List element!");	
				// careful now, is this a comma that ends a function call argument??????
				// let's inspect the expr field which should point to start parenthesis
				// BUT we should only subtract from argument when this is a `do`, `for` or `function` call
				// MDH@09MAR2020: `function` renamed to `defun` and `function` now represents anynomous function
				//				which has to be assigned to a variable in order to be remembered (and used)
				//				both with `function` and `defun` the user can define the body inside the definition itself
				if(_token->expr!=NULL){
					if(_token->expr->type==TT_FUNCTION_CALL){ // a function call argument
						// MDH@09MAR2020: with function calls that have a 'body' i.e. for, do, function and defun
						//				I think we can use envid to determine whether this is the case
						//				there's different behaviour for the different arguments
						if(_token->expr->argument>0){
							_token->argument=_token->argument-1;
							if(amVerboseDebugging())
								if(inputInfoFunction)(*inputInfoFunction)("Local variables argument!");
							// MDH@09MAR2020: we need to do something on every argument with 0 argument attribute
							//				what we would do on ) 
							if(_token->argument==0){
								// outputChar('A');
								// MDH@25FEB2021:  we can now evaluate the command from the first token in the first argument to this special function representing the local variable map of this special function
								Mvalue* localVariablesMapValue=getSubcommandValue(_token->expr->next,prevToken,TT_FUNCTION_CALL,(TokenType[]){TT_EXPRESSION},1,NULL,"local variables");
								// outputChar('B');
								if(!localVariablesMapValue||localVariablesMapValue->type==VT_MAP){
									// outputChar('C');
									if(amVerboseDebugging())
										if(inputInfoFunction)(*inputInfoFunction)("Local variables map identified!");
										//outputValue("Local variables map:",localVariablesMapValue,"'.\n");
									if(!pushLocalvariables(localVariablesMapValue,_token->envid)){
										// outputChar('D');
										newTokenType=TT_ERROR; // TODO I suppose we could have a separate TT_BUG token type perhaps?????
										if(inputErrorFunction)(*inputErrorFunction)("Failed to register local variables!");
									}
									// outputChar('E');
								}else{ // it's not a map which it should be
									// outputChar('F');
									newTokenType=TT_ERROR; 
									if(inputInfoFunction)(*inputInfoFunction)("%sLocal variables argument does not evaluate to a map!",M_WARNING_PREFIX);
								}
								// outputChar('G');
							}
						}else
						if(_token->expr->argument<-2){ // MDH@20DEC2022: a known number of expected arguments
							if(_token->expr->argument==-3){ // already reached the total number of expected arguments
								newTokenType=TT_ERROR;
								if(inputErrorFunction)(*inputErrorFunction)("Another argument not allowed.");
							}else{
								_token->expr->argument=_token->expr->argument+1;
								//if(amVerboseDebugging())
								if(inputInfoFunction)(*inputInfoFunction)("Number of expected arguments: %lld.",-_token->expr->argument-3);
							}
						}else{
								if(inputInfoFunction)(*inputInfoFunction)("Number of expected arguments unknown!");
						}
					}else
					if(_token->expr->type!=TT_LIST&&_token->expr->type!=TT_MAP&&_token->expr->type!=TT_EXPRESSION){
						// MDH@10APR2023: now allowed in TT_EXPR, so that we can have array results
						newTokenType=TT_ERROR;
						if(inputErrorFunction)(*inputErrorFunction)("Comma not allowed in expression of type %s.",TOKENTYPE_STRING[_token->expr->type]);
					}
				}else{ // a comma should always match either a map or list or expression start
					newTokenType=TT_ERROR;
					if(inputErrorFunction)(*inputErrorFunction)("Comma not allowed outside map, list or function call!");
				}
			}
			/////if(amDebugging())inputInfo("E7");
		}
		// MDH@11JAN2021: at every colon that defines the value of a local variable (to a do(), forw() or function() call, we remember the name of the property and register it when we encounter a comma)
		if(newTokenType==TT_MAP_VALUE){
			// outputChar('A');
			/* MDH@25FEB2021: no need to do the following anymore because we managed to evaluate this argument as a whole at the comma (TT_LIST_ELEMENT) following it
			if(_token->argument==1){
				outputChar('A');
				if(inputInfoFunction)(*inputInfoFunction)("Property value token in first special function argument!\n");
				// the property name can be a literal or the name of a variable, of which we know the current value (essentially not something that currently exists in the command, because then we could not evaluate its value!!)
				if(prevToken->type==TT_END_OF_SQSTRING||prevToken->type==TT_END_OF_DQSTRING){
					outputChar('C');
					if(prevToken->prev){
						outputChar('E');
						// register the actual name (without the quote that prefixes the property name)
						bool localvariablepushed=push_localvariable(string(prevToken->prev->text)+1,_token->envid);
						if(localvariablepushed){
							outputChar('G');
							if(inputInfoFunction)(*inputInfoFunction)("Local variable '%s' registered.\n",_lastlocalvariable->_name);
						}else
						if(inputInfoFunction)(*inputInfoFunction)("%sFailed to register property name '%s'.\n",M_ERROR_PREFIX,string(prevToken->prev->text)+1);
					}
				}else
				if(prevToken->type==TT_VARIABLE){
					outputChar('B');
					char* _variableName=_getSignificantTokenCharacters(prevToken);
					if(_variableName){
						outputChar('D');
						// if we're able to resolve this variable to a text we can push it as a text
						Mvariable* variable=getVariable(getExecutionEnvironment(),_variableName,false);
						if(variable){
							outputChar('F');
							Mstring* _valueText=owned_string(_getValueText(variable->_value,true),owner);
							if(_valueText){
								outputChar('H');
								bool localvariablepushed=pushLocalvariable(string(_valueText),_token->envid);
								if(localvariablepushed){
									outputChar('J');
									if(inputInfoFunction)(*inputInfoFunction)("Local variable '%s' registered.\n",_lastlocalvariable->_name);
								}else
								if(inputInfoFunction)(*inputInfoFunction)("%sFailed to register property name '%s'.\n",M_ERROR_PREFIX,string(_valueText));
								FREE_STRING(_valueText,owner);
							}else
							if(inputInfoFunction)(*inputInfoFunction)("%sUnable to determine the text variable '%s' represents.\n",M_ERROR_PREFIX,_variableName);
						}else
						if(inputInfoFunction)(*inputInfoFunction)("%sUnknown variable name '%s'.\n",M_ERROR_PREFIX,_variableName);
						free(_variableName); // unfortunately not managed so only usable for local stuff
					}else
					if(inputInfoFunction)(*inputInfoFunction)("%sNo variable name.\n",M_ERROR_PREFIX);
				}
			}else
			if(inputInfoFunction)(*inputInfoFunction)("Property value token!\n");
			*/
		}


	}
	// only when newTokenType does not equal TT_ERROR there's success
	return(newTokenType!=TT_ERROR);
}

// MDH@23SEP2019: setting the type of the new token is moved outside because setLastTokenType() replaces setting the type of a token directly
//				this means that _getToken can use newTokenType but should NOT set ->type of the given token unless we decide to remove newTokenType from _getToken of cours in the future...
/**
 * @brief returns a new token following \p prevToken of type \p newTokenType
 * @details only called from _getNewCommandToken
 * @param prevToken 
 * @param newTokenType 
 * @return Mtoken* a new token following \p prevToken of type \p newTokenType
 */
static Mtoken* _getToken(Mtoken* prevToken,TokenType newTokenType){Mallocationowner owner=getOwner(__LINE__);
	Mtoken* _token=owned_token(__token(),owner);
	if(_token!=NULL){
		_token->text=owned_string(__string(),Msubowner(owner,1));
		// MDH@23JUL2019: we can do this for now TODO this is a serious memory error which a better way to deal with that is crucial
		if(NULL==_token->text){
			FREE_TOKEN(_token,owner); // MDH@10APR2024: probably best to return NULL so that we'd switch to control mode and report a serious (probably memory) error as we can't continue
			////if(inputErrorFunction)(*inputErrorFunction)("Failed to initialize the new token text.");
			return NULL;
			/* replacing: _token->type=TT_ERROR;
			if(inputErrorFunction)(*inputErrorFunction)("Failed to initialize the new token.");
			*/
		}
		/////if(amDebugging())inputInfo("E1");
		// MDH@03MAY2019: if the previous token starts an expression itself, use prevToken itself and not its expr field!!!!
		if(prevToken!=NULL){
			// finish the previous token
			prevToken->next=_token; // how could I forget about doing this (and checking whether prevToken is not NULL!)!!
			if(isTokenUnfinished(prevToken))
				finishToken(prevToken); // MDH@22MAR2019: if the token character length is NOT set, set it now...
			// initialize the new token
			_token->prev=prevToken; // set the predecessor

			// MDH@10APR2024: we can put the following code in a separate function possibly adapting newTokenType
			_token->type=newTokenType; // we need to do this because tokenPropertiesPropagated initializes its local newTokenType to the type of the successor of prevToken 
			if(tokenPropertiesPropagated(prevToken))newTokenType=TT_ERROR;
		}
		/////if(amDebugging())inputInfo("E8");
		// MDH@03MAY2019: TT_EXPRESSION is the default (0) now (always ending at the next non-space character): _token->type=TT_EXPRESSION; // makes more sense to start as expression (same as what we get after a ( or [
		/* moving the following to the beginning, and returning NULL when we fail to initialize text as we need it to be able to add text to it!!!!
		_token->text=owned_string(__string(),Msubowner(owner,1));
		// MDH@23JUL2019: we can do this for now TODO this is a serious memory error which a better way to deal with that is crucial
		if(NULL==_token->text){
			_token->type=TT_ERROR; 
			if(inputErrorFunction)(*inputErrorFunction)("Failed to initialize the new token.");
		}else
		if(amVerboseDebugging())
			if(inputInfoFunction)(*inputInfoFunction)("New token text initialized."); // TODOhow about 
		*/
		/////if(amDebugging())inputInfo("E9");
		/* not needed with calloc() allocation
		_token->significantCharacterCount=0; // MDH@22MAR2019: remembers the amount of significant characters (to be set when the token ends)
		_token->next=NULL;
		*/
	}
	if(NULL==_token){
		if(inputErrorFunction)(*inputErrorFunction)("Failed to create a new token.");
		return NULL;
	}
	_token->type=newTokenType;
	return disowned_token(_token,owner);
}

// MDH@23SEP2019: prudent to replace all calls to _getToken that simply append a new token to the command, by a method that will always call setLastTokenType() 
// command generic (i.e. it does not need to be the user input command, it could be some command that is being parsed)
/**
 * @brief returns a new command token following \p lastCommandToken of type \p tokenType
 * @param lastCommandToken
 * @param tokenType
 * @return a new command token following \p lastCommandToken of type \p tokenType
 */
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType/*,bool endOfInput*/){Mallocationowner owner=getOwner(__LINE__);
	// MDH@01OCT2019: because the current token is NOT removed from the command, we should NOT delete its associated feed forward text
	//				but we should remove any identifier continuation
	// MDH@02OCT2019 no need for this anymore here: if(endOfInput)deleteIdentifierContinuation(); // remove whatever feed forward text that was associated with the now finished last command token as it will no longer be applicabld
	Mtoken* _newCommandToken=owned_token(_getToken(lastCommandToken,tokenType),owner);
	// MDH@10APR2024 TODO: the following is weird because _getToken could change the type of _newCommandToken to TT_ERROR which would be overwritten again by the following 
	if(_newCommandToken!=NULL){
		if(inputErrorFunction)if(tokenType!=TT_ERROR&&_newCommandToken->type==TT_ERROR)(*inputErrorFunction)("Assumed new error token type corrected!"); // MDH@10APR2024
		setTokenType(_newCommandToken,tokenType);
	}else
	if(amVerboseDebugging())
		if(inputErrorFunction)(*inputErrorFunction)("Failed to create a command token");
	return disowned_token(_newCommandToken,owner);
}
/**
 * @brief returns a new M command with or without first token as determined by \p withFirstToken
 * 
 * @param withFirstToken 
 * @return Mcommand* returns a new M command with or without first token as determined by \p withFirstToken
 */
Mcommand* _getNewCommand(bool withFirstToken){Mallocationowner owner=getOwner(__LINE__);
	Mcommand* _command=(Mcommand*)CALLOC_1(sizeof(Mcommand),'K',owner);
	if(_command!=NULL){
		// if(amDebugging())(*inputInfoFunction)("New command created.");
		if(withFirstToken){
			_command->_firstToken=owned_token(_getNewCommandToken(NULL,TT_EXPRESSION/*,endInput*/),Msubowner(owner,1)); // MDH@24MAY2020: obtain ownership immediately
			if(_command->_firstToken!=NULL){ // we've got a first token allocated
				// if(amVerboseDebugging())if(inputInfoFunction)(*inputInfoFunction)("New command token created.");
				_command->_lastToken=_command->_firstToken;
				_command->_firstToken->expr=NULL;
			}else{ // too bad, out of memory!
				FREE_DISOWNED_1(_command,'K',owner);_command=NULL;
				// if(amVerboseDebugging())
				if(inputErrorFunction)(*inputErrorFunction)("Failed to create the first command token.");
			}
		}
	}else
	// if(amVerboseDebugging())
	if(inputErrorFunction)(*inputErrorFunction)("Failed to create the command.");
	return disowned_command(_command,owner);
}

// and finally
// functions registered in getShellEnvironment()
// PI all little more accurate (we could make a PI100 from these numbers)
// source: https://blog.wolfram.com/2011/06/30/all-rational-approximations-of-pi-are-useless/
// wolfram has a Rationalize function to compute rational approximations to a certain accuracy (see https://reference.wolfram.com/language/ref/Rationalize.html)
/////const char* M_QNUM_PI100="394372834342725903069943709807632345074473102456264";
/////const char* M_QDEN_PI100="125532772013612015195543173729505082616186012726141";

// MDH@24MAY2020: because _Iadd is currently only called with freeonfailure equal to false we removed that argument otherwise we would have needed to provide the two owners!!!

/**
 * @brief returns the result of adding big integers \p a and \p b
 * @param a
 * @param b
 * @return the big integer sum of big integers \p a and \p b
 */
Mbiginteger* _Iadd(Mbiginteger* a,Mbiginteger* b/*,bool freeonfailure*/){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT do NOT call with either a or b NULL
	Mbiginteger* _sum=NULL;
	if(a!=NULL&&b!=NULL){
		if(!isBigintegerZero(a)&&!isBigintegerZero(b)){
			_sum=owned_biginteger(__biginteger(),owner);
			if(_sum!=NULL&&mp_add(MP_INT_POINTER(a),MP_INT_POINTER(b),MP_INT_POINTER(_sum))!=MP_OKAY){FREE_BIGINTEGER(_sum,owner);_sum=NULL;} // if the addition fails return 0
		}else
			_sum=owned_biginteger(_getBigintegerCopy(isBigintegerZero(a)?b:a),owner);
	}
	///////outputBiginteger("\nBig integer sum of ",a,NULL);outputBiginteger(" and ",b,NULL);outputBiginteger(" equals ",sum,".");
	// if(!sum)if(freeonfailure){FREE_BIGINTEGER(a);FREE_BIGINTEGER(b);}
	return disowned_biginteger(_sum,owner);
} // adding two big integers, if either is NULL return NULL

/**
 * @brief returns the result of multiplying big integers \p a and \p b
 * @param a
 * @param b
 * @return the big integer product of big integers \p a and \p b
 */
Mbiginteger* _Imultiply(Mbiginteger* a,Mbiginteger* b/*,bool freeonfailure*/){Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _product=NULL;
	if(a!=NULL&&b!=NULL){
		if(!isBigintegerOne(a)&&!isBigintegerOne(b)){
			_product=owned_biginteger(__biginteger(),owner); // defaults to zero, which would be the result as well if either big integer is zero!!!
			if(_product!=NULL&&mp_mul(MP_INT_POINTER(a),MP_INT_POINTER(b),MP_INT_POINTER(_product))!=MP_OKAY){FREE_BIGINTEGER(_product,owner);_product=NULL;}
		}else
			_product=owned_biginteger(_getBigintegerCopy(isBigintegerOne(a)?b:a),owner);
	}
	//////////outputBiginteger("\nProduct of big integers ",a,NULL);outputBiginteger(" and ",b,NULL);outputBiginteger(" equals ",product,".");
	// if(!product)if(freeonfailure){FREE_BIGINTEGER(a);FREE_BIGINTEGER(b);}
	return disowned_biginteger(_product,owner);
} // multiplying two big integers, if either is NULL return NULL

// _Imul is special big integer multiplier that assumes a NULL big integer equals 1
/**
 * @brief returns the big integer product of big integers \p a and \p b
 * @details an input value equal to NULL is considered to equal 1 (not both)
 * @param a 
 * @param b 
 * @return Mbiginteger* the big integer product of big integers \p a and \p b
 */
Mbiginteger* _Imul(Mbiginteger* a,Mbiginteger* b){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==a&&NULL==b)return NULL;
	if(NULL==a)return _getBigintegerCopy(b);
	if(NULL==b)return _getBigintegerCopy(a);
	Mbiginteger* _product=owned_biginteger(__biginteger(),owner);
	if(_product!=NULL&&mp_mul(MP_INT_POINTER(a),MP_INT_POINTER(b),MP_INT_POINTER(_product))!=MP_OKAY){FREE_BIGINTEGER(_product,owner);_product=NULL;}
	return disowned_biginteger(_product,owner);
}

// RATIONAL STUFF
// long double helper functions for use with the delta of rationals
/**
 * @brief returns the negated value of M float \p _float
 * 
 * @param _float
 * @return long double 
 */
long double realneg(Mfloat* _float){return (floatIsUndefined(_float)?M_LD_NAN:-_float->ld);}

/* MDH@19SEP2019: replaced by appropriate versions in Mrational.h/c
// operators applied to rationals
Mrational* _qmultiply(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	bool delta1undefined=floatIsUndefined(_rational1->delta),delta2undefined=floatIsUndefined(_rational2->delta);
	Mbiginteger *_num=_Imul(_rational1->num,_rational2->num),*_den=_Imul(_rational1->den,_rational2->den);
	// pure rationals are easy
	if(delta1undefined&&delta2undefined)return _getRational(_num,_den,M_LD_NAN,true,true);
	// TODO take the delta's into account!!!
	return NULL;
}
Mrational* _qdivide(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	// even if we have delta's we will always need these products
	Mbiginteger *_num=_Imul(_rational1->num,_rational2->den),*_den=_Imul(_rational2->num,_rational1->den);
	bool delta1undefined=floatIsUndefined(_rational1->delta),delta2undefined=floatIsUndefined(_rational2->delta);
	// pure rationals are easy
	if(delta1undefined&&delta2undefined){
		return _getRational(_num,_den,M_LD_NAN,true,true);
	}
	// if we assume the delta's to be very small (as they will be), the parts containing squares to be too small to care about 
	if(delta1undefined){
		// second denominator term is -(b*d*delta2)**2 which for reasonably small b and d (both denominators) will be negligable

	}
	if(delta2undefined){

	}
	// neither undefined
	return NULL;
}
*/
/* replacing:
Mrational* _qdivide(Mrational* rational1,Mrational* rational2){
	// if either of them has a delta (i.e. is not pure), convert to double reals first
	Mrational* _rational=NULL;
	if(rational1&&rational2){
		long double delta1=getRealLongDouble(rational1->delta),delta2=getRealLongDouble(rational2->delta);
		if((ldIsNaN(delta1)||ldIsZero(delta1))&&(ldIsNaN(delta2)||ldIsZero(delta2))){ // both are 'pure' rationals
			if(amVerbose())output("Pure rational division.");
			// compute the nsew numerator and denominator, both should not be NULL as the numerators are not NULL, so should be freed if we can't drop them
			Mbiginteger* _numerator=(rational2->den?_Imultiply(rational1->num,rational2->den,false):_getBigintegerCopy(rational1->num));
			Mbiginteger* _denominator=(rational1->den?_Imultiply(rational2->num,rational1->den,false):_getBigintegerCopy(rational2->num));
			if(!_numerator||!_denominator){ // failed to compute either, so somewhere it went wrong
				FREE_BIGINTEGER(_numerator);FREE_BIGINTEGER(_denominator);
			}else{
				if(isBigintegerOne(_denominator)){FREE_BIGINTEGER(_denominator);_denominator=NULL;} // prevent storing 1 explicitly...
				_rational=_getRational(_numerator,_denominator,M_LD_NAN,true,true); // free num/den when failing to bind them
			}
		}else{ // either one or both are unpure i.e. 'reals' approximated by rationals 
			if(amVerbose())output("Approximate rational division.");
		}
	}
	return _rational;
}

// MDH@17SEP2019: _qsum used to be _qadd but we now have a _qadd in Mrational.c which will check for errors in performing the big integer multiplications, so is better
Mrational* _qsum(Mrational* _rational1,Mrational* _rational2){
	Mrational* _rational=__rational(); // we need a rational to hold the sum
	if(_rational){
		outputInfo("Adding two rationals.");
		mp_err status=_qadd(_rational,_rational1,_rational2);
		output("Two rationals added (status=%i).\n",status);
		if(status!=MP_OKAY){FREE_RATIONAL(_rational);_rational=NULL;outputError("Failed to add two rationals");}else outputInfo("Two rationals added successfully."); // addition failed somehow...
	}
	return _rational;
}
*/
/* replacing:
Mrational* _qadd(Mrational* _rational1,Mrational* _rational2){
	Mrational* _rational=NULL;
	if(_rational1&&_rational2){
		// we can speed things up by using a special multiplication method
		Mbiginteger *_num1=_Imul(_rational1->num,_rational2->den),*_num2=_Imul(_rational2->num,_rational1->den);
		if(_num1&&_num2) // we got (and need) both
			_rational=_getRational(_Iadd(_num1,_num2,false),_Imul(_rational1->den,_rational2->den),realsum(_rational1->delta,_rational2->delta),true,true); // free the numerator and denominator
		if(_num1)FREE_BIGINTEGER(_num1);if(_num2)FREE_BIGINTEGER(_num2);
	}
	return _rational;
}
*/

/* MDH@19SEP2019: replaced by _getRationalDifference in Mrational.h/c
Mrational* _qneg(Mrational* _rational){
	// normalizes the negated rational if not currently normalized, otherwise it will not normalize it
	Mrational* _rationalNeg=(_rational?_getRational(_getNegatedBiginteger(_rational->num),_getBigintegerCopy(_rational->den),realneg(_rational->delta),!_rational->normalized,true):NULL);
	if(_rationalNeg)if(_rational->normalized)_rationalNeg->normalized=true; // if original assumed normalized, so is the negated value
	return _rationalNeg;
}
Mrational* _qsubtract(Mrational* _rational1,Mrational* _rational2){
	if(!_rational1||!_rational2)return NULL;
	Mrational* _rational2Neg=_qneg(_rational2); // get the negated rational2
	Mrational* _rational=_qsum(_rational1,_rational2Neg);
	FREE_RATIONAL(_rational2Neg); // free the negated rational2
	return _rational;
}
*/
// end rational stuff

/* replaced by methods in Mdecimal.h/c
Mdecimal* _dadd(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputInfo("Adding two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qadd(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){FREE_DECIMAL(_result);_result=NULL;outputError("Failed to compute the sum of two decimals.");}
	}else
		outputError("Failed to create the sum decimal");
	return _result;
}
Mdecimal* _ddiv(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputInfo("Dividing two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qdiv(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			FREE_DECIMAL(_result);_result=NULL;
			outputError("Failed to compute the quotient of two decimals");
		}
	}else 
		outputError("Failed to create the quotient decimal");
	return _result;
}
Mdecimal* _dmul(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputInfo("Multiplying two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qmul(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			FREE_DECIMAL(_result);_result=NULL;
			outputError("Failed to compute the product of two decimals");
		}
	}else
		outputError("Failed to create the product decimal");
	return _result;
}
Mdecimal* _dsub(Mdecimal* _decimal1,Mdecimal* _decimal2){
	if(!_decimal1||!_decimal2)return NULL;
	Mdecimalcontext* decimalcontext=getDecimalcontext(MAX(_decimal1->prec,_decimal2->prec));
	mpd_context_t* mpd_context=(decimalcontext?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
	if(!mpd_context){outputError("No decimal context!");return NULL;}
	Mdecimal* _result=__decimal(mpd_context,0,0);
	///////outputInfo("Multiplying two decimals.");
	if(_result){
		uint32_t status=0;
		mpd_qsub(_result->mpd,_decimal1->mpd,_decimal2->mpd,mpd_context,&status);
		if(status&0xEFBF){
			FREE_DECIMAL(_result);_result=NULL;
			outputError("Failed to compute the difference of two decimals");
		}
	}else
		outputError("Failed to create the difference decimal");
	return _result;
}
*/
/**
 * @brief returns the value of PI in the decimal precision from \p value
 * 
 * @param value 
 * @param computesinetableValue 
 * @return Mvalue* 
 */
Mvalue* Mpi(Mvalue* value,Mvalue* computesinetableValue){Mallocationowner owner=getOwner(__LINE__);
	// _value should be a positive integer defining the required precision
	if(amVerboseDebugging())output("Computing pi using decimals.\n");
	// MDH@18JUN2020: if a list of values
	if(value!=NULL&&value->type==VT_LIST&&isValueUndefined(computesinetableValue)){
		Mlist* _piList=owned_list(__list("pi"),owner);
		Mlistelement* listelement=value->value._list->_first;
		while(listelement!=NULL){
			if(appendedToList(_piList,owner,Mpi(listelement->_value,NULL),M_LL_INVALID)<0)break;
			listelement=listelement->_next;
		}
		return _getValueOfList(disowned_list(_piList,owner));
	}
	// MDH@17AUG2019: delegate to pi_decimal defined in Mdecimal.h/c
	long long numberOfRequestedDecimals=getDP();
	if(value!=NULL)
		numberOfRequestedDecimals=getValueInteger(value);
	else 
	if(!amVerbose())
		output("No decimal precision specified! Will use the current default decimal precision (%lld)!\n",numberOfRequestedDecimals);
	if(numberOfRequestedDecimals<6){
		output("%s",M_ERROR_PREFIX);
		outputValue("Argument '",value,"' to the pi() function should ");
		if(numberOfRequestedDecimals==M_LL_INVALID)
			output("denote a valid small integer");
		else
		if(numberOfRequestedDecimals<=0)
			output("denote a positive integer");		
		else
			output("at least equal 6");	
		output(", which it does not!\n");
		return NULL;
	}
	// NOTE if the second argument (computesinetableValue is NOT specified and isValueZero() returns M_LL_INVALID, compute as well)
	// CORRECTION by default should NOT compute the sine table (to speed up computing pi)
	return _getValueOfDecimal(pi_decimal(getDecimalcontext(numberOfRequestedDecimals),isValueZero(computesinetableValue)!=M_TRUE));
}

// wolfram reports 13 different approximations to pi at http://functions.wolfram.com/Constants/Pi/10/

/*
the following very fast approximation (which computes decimal digits), wich I guess we should use to compute pi with a sufficient number of decimal digits
source: https://en.wikipedia.org/wiki/Chudnovsky_algorithm
from decimal import Decimal as Dec, getcontext as gc

def PI(maxK=70, prec=1008, disp=1007): # parameter defaults chosen to gain 1000+ digits within a few seconds
	gc().prec = prec
	K, M, L, X, S = 6, 1, 13591409, 1, 13591409
	for k in range(1, maxK+1):
		M = (K**3 - 16*K) * M // k**3 
		L += 545140134
		X *= -262537412640768000
		S += Dec(M * L) / X
		K += 12
	pi = 426880 * Dec(10005).sqrt() / S
	pi = Dec(str(pi)[:disp]) # drop few digits of precision for accuracy
	print("PI(maxK={} iterations, gc().prec={}, disp={} digits) =\n{}".format(maxK, prec, disp, pi))
	return pi

Pi = PI()
print("\nFor greater precision and more digits (takes a few extra seconds) - Try")
print("Pi = PI(317,4501,4500)") 
print("Pi = PI(353,5022,5020)")
 */
// but the primary formula is pretty simple: 4*sum((-1)k/(2k+1)): this is the very slow Gregory-Leibniz series approximation
// this is a very slow algorithm
/**
 * @brief returns a rational approximation of PI in maximally \p value iterations
 * 
 * @param value 
 * @return Mvalue* a rational approximation of PI in maximally \p value iterations
 */
Mvalue* pi_ql(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mrational* _rational=NULL;
	if(value!=NULL&&value->type==VT_INTEGER){
		long long maxiter=value->value._integer->ll;
		if(maxiter>=0){
			if(amVerbose())output("Approximating pi/4 by a sum of %llu rational fractions.\n",maxiter);
			// the first approximation (when iter=0) equals 4
			Mbiginteger* _bi1=owned_biginteger(_getBiginteger(1),owner);
			Mrational* _rational=(_bi1!=NULL?owned_rational(_getRational(_bi1,NULL,M_LD_NAN,false),owner):NULL);
			FREE_BIGINTEGER(_bi1,owner);
			if(_rational!=NULL){
				// obviously we can add 2 to the big integer storing the numerator
				if(maxiter>0){
					Mbiginteger* _addendumDenominator=owned_biginteger(_getBiginteger(3),owner);
					Mbiginteger* _denominatorIncrement=owned_biginteger(_getBiginteger(2),owner);
					if(_addendumDenominator!=NULL&&_denominatorIncrement!=NULL){
						for(long long iter=1;iter<=maxiter;iter++){
							// compute the numerator and (new) denominator of the addendum rational
							Mbiginteger* _addendumNumerator=owned_biginteger(_getBiginteger(iter%2?-1:1),owner); // the numerator is either 1 or -1
							if(NULL==_addendumNumerator){
								FREE_BIGINTEGER(_addendumDenominator,owner);
								output("%sFailed to set the addendum numerator at iteration %u.\n",M_ERROR_PREFIX,iter);
								break;
							}
							// both _addendumNumerator and _addendumDenominator are now available to be bound in the rational
							Mrational* _addendumRational=owned_rational(_getRational(_addendumNumerator,_addendumDenominator,M_LD_NAN,false),owner);
							FREE_BIGINTEGER(_addendumNumerator,owner);
							if(NULL==_addendumRational){ // failed to bind in the rational
								output("%sFailed to compute the rational to add to the approximation of pi in step %u.",M_ERROR_PREFIX,iter);
								break;
							}
							// add the addendum to the current rational
							if(amVerbose()){
								outputRational("Sum so far: ",_rational,NULL);
								outputRational(", addendum: ",_addendumRational,".\n");
							}
							Mrational* _newRational=owned_rational(_getRationalSum(_rational,_addendumRational),owner); // _qsum replaced by _getRationalSum in Mrational.h/c
							if(NULL==_newRational){
								// we have to free the addendum numerator and denominator
								output("%sFailed to add this addendum at step %u in approximating pi.\n",M_ERROR_PREFIX,iter);
								FREE_RATIONAL(_addendumRational,owner); // to free the addendum numerator and denominator bound to _addendumRational
								break;
							}
							// increment the denominator BEFORE we loose the addendum denominator we have now (as part of _rational)
							if(amVerbose())
								outputBiginteger("Incrementing the addendum denominator by ",_denominatorIncrement,".\n");		
							Mbiginteger* _newAddendumDenominator=owned_biginteger(_Iadd(_addendumDenominator,_denominatorIncrement),owner);
							if(NULL==_newAddendumDenominator){
								// MDH@27MAY2020: FREE_BIGINTEGER(_addendumDenominator,owner); // won't be using this in the addendum rational
								outputError("Failed to increment the addendum denominator");
								break;
							}
							if(amVerbose())
								outputBiginteger("New addendum denominator: ",_newAddendumDenominator,".\n");
							if(amVerbose())
								outputRational("New approximation to pi/4: ",_newRational,".\n");
							FREE_RATIONAL(_addendumRational,owner); // to free the addendum numerator and denominator bound to _addendumRational
							// replace _rational by _newRational
							FREE_RATIONAL(_rational,owner);
							_rational=_newRational;
							//if(amVerbose())
							if(amVerbose())
								outputRational("Sum approximation of pi/4 so far: ",_rational,".\n");
							// no need to normalize as the addendum is always normalized by itself
							// replace the addendum denominator with the new one)
							FREE_BIGINTEGER(_addendumDenominator,owner); // MDH@27MAY2020: release the current _addendumDenominator (as we're only freeing the final value below!!!)
							_addendumDenominator=_newAddendumDenominator;
							if(amVerbose())
								outputBiginteger("New addendum denominator: ",_addendumDenominator,".\n");
						}
					}else{
						outputError("Failed to initialize the addendum numerator and its increment value (2)");
					}
					// MDH@27MAY2020: free helper big integers
					FREE_BIGINTEGER(_addendumDenominator,owner);
					FREE_BIGINTEGER(_denominatorIncrement,owner);
				}
				// MDH@09APR2020: ok, this might be problematic if _rational_num is NULL so -> FIXED
				if(NULL==_rational->num||(mp_mul_2d(MP_INT_POINTER(_rational->num),2,MP_INT_POINTER(_rational->num))!=MP_OKAY)){
					if(amVerbose()){output("%s",M_ERROR_PREFIX);outputRational("Failed to multiply the approximation of pi/4 (",_rational," by 4.\n");}
					FREE_RATIONAL(_rational,owner);
					return NULL;
				} // multiply the numerator by 4 i.e. 2**2
				if(!normalizeRational(_rational,owner)){
					output("%s",M_ERROR_PREFIX);
					outputRational("Failed to normalize the rational approximation of pi ",_rational,".\n");
				}else
				if(amVerbose())outputRational("Normalized approximation of pi: ",_rational,".\n");
				// if we get here _rational is the result to return
				return _getValueOfRational(disowned_rational(_rational,owner));
			}
		}
	}
	return NULL;
}

// and the following is an implementation that can approximate pi using this formula
/**
 * @brief returns a rational approximation of PI in at most \p value approximations
 * 
 * @param value 
 * @return Mvalue* 
 */
Mvalue* pi_q(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL&&value->type==VT_INTEGER){
		long long iter=value->value._integer->ll;
		if(iter>=0){
			if(amVerbose())
				output("Computing %llu continued fractions of pi.\n",iter);
			Mbiginteger* _bi3=owned_biginteger(_getBiginteger(3),owner);
			if(NULL==_bi3){outputError("Failed to create big integer 3.");return NULL;}
			Mrational* _rational=owned_rational(_getRational(_bi3,NULL,M_LD_NAN,false),owner);
			FREE_BIGINTEGER(_bi3,owner);
			if(_rational!=NULL){
				if(iter>0){
					// working backwards starting with the last denominator quotient seems to be best
					// in every step you have to compute i**2/6 the second term of the denominator
					Mbiginteger* _bi6=owned_biginteger(_getBiginteger(6),owner);
					if(NULL==_bi6){outputError("Failed to create big integer of 6.");return NULL;}
					Mrational* _denominatorRational=(_bi6!=NULL?owned_rational(_getRational(_bi6,NULL,M_LD_NAN,false),owner):NULL); // the final denominator equals 6
					if(_denominatorRational!=NULL){
						if(amVerbose())
							output("First denominator rational computed.\n");
						long long square;
						while(--iter>0){
							square=4*(iter+1)*iter+1;
							////////////if(amVerbose())output("Square numerator: %llu.",square);
							Mbiginteger* _bigintegerSquare=owned_biginteger(_getBiginteger(square),owner);
							if(NULL==_bigintegerSquare){
								output("%sFailed to compute the big integer of square %llu.\n",M_ERROR_PREFIX,square);
								break;
							}
							if(amVerbose())
								output("%llu fractions yet to compute using numerator square '%llu'.\n",iter,square);
							// the new denominator becomes 6+square/prev denominator=
							Mbiginteger* _mult=owned_biginteger(_Imultiply(_denominatorRational->num,_bi6),owner);
							Mbiginteger* _add=(_denominatorRational->den!=NULL?owned_biginteger(_Imultiply(_denominatorRational->den,_bigintegerSquare),owner):_bigintegerSquare);
							Mbiginteger* _denominatorNumerator=(_mult!=NULL?owned_biginteger(_Iadd(_mult,_add),owner):NULL);
							// free all intermediate big integers
							FREE_BIGINTEGER(_mult,owner);
							FREE_BIGINTEGER(_bigintegerSquare,owner);
							if(_denominatorRational->den!=NULL)FREE_BIGINTEGER(_add,owner);
							// update the denominator rational, free the numerator if we fail to bind it to _denominatorRational
							// what's dangerous in the following is that _denominatorRational->num is not freed!!!!
							Mbiginteger* _previousDenominatorNumerator=owned_biginteger(_getBigintegerCopy(_denominatorRational->num),owner);
							FREE_RATIONAL(_denominatorRational,owner); // get the 'previous' numerator and denominator released!!!!!!
							_denominatorRational=owned_rational(_getRational(_denominatorNumerator,_previousDenominatorNumerator,M_LD_NAN,false),owner);
							FREE_BIGINTEGER(_previousDenominatorNumerator,owner);FREE_BIGINTEGER(_denominatorNumerator,owner); // MDH@27MAY2020 getRational() does not bind the passed in big integers anymore, so need to be always freed
							if(NULL==_denominatorRational)break; // let's keep it normalized???? TODO is that necessary
							if(amVerbose())
								outputRational("Denominator (unnormalized): ",_rational,".\n");
						}
					}
					FREE_BIGINTEGER(_bi6,owner);
					// MDH@27MAY2020: if(!_denominatorRational){outputError("Final denominator could not be computed");return NULL;}
					// NOTE: do NOT use the originals in inverting the denominator because those will be freed below so we need to pass in copies
					Mrational* _inverseDenominatorRational=owned_rational(_getInverseRational(_denominatorRational),owner);
					Mrational* _result=NULL;
					if(NULL==_inverseDenominatorRational){
						output("%s",M_ERROR_PREFIX);outputRational("Failed to compute the fractional part of pi (by inverting denominator rational ",_denominatorRational,").\n");
						FREE_RATIONAL(_rational,owner);_rational=NULL;
					}else
						_result=owned_rational(_getRationalSum(_rational,_inverseDenominatorRational),owner); // _qsum() replaced by _getRationalSum in Mrational.h/c
					FREE_RATIONAL(_denominatorRational,owner);
					FREE_RATIONAL(_inverseDenominatorRational,owner); // MDH@27MAY2020
					if(_result!=NULL){
						_rational=_result;
						if(amVerbose())
							outputRational("Approximation of pi: ",_rational,".\n");
					}else{
						FREE_RATIONAL(_rational,owner);
						_rational=NULL;
					}
				}
				return _getValueOfRational(disowned_rational(_rational,owner));
			}
		}
	}
	return NULL;
}

// we can also use decimals to approximate pi to a certain precision (=decimal digits)
/* Python test program for approximating pi!!!!
import cdecimal
import decimal

def pi(module, prec):
	"""From the decimal.py documentation"""
	module.getcontext().prec = prec + 2
	D = module.Decimal
	lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
	while s != lasts:
		lasts = s
		n, na = n+na, na+8
		d, da = d+da, da+32
		t = (t * n) / d
		s += t
	module.getcontext().prec -= 2
	return +s

for i in range(10000):
	x = pi(cdecimal, 28)

for i in range(10000):
	y = pi(decimal, 28)
 */

/* MDH@14NOV2019: replaced by the M variable and M function
// how about storing all results here?????? instead of in the root environment????
Mvalue* _resultListValue=NULL; // were the results are being kept
// the function that is used to return a specific result value
Mvalue* getResult(Mvalue* indexValue){
	if(amVerbose())outputInfo("Result requested!");
	if(!indexValue)return _resultListValue;
	long long indexValueInteger=getValueInteger(indexValue); // NOTE all index values should be positive!!!
	return (indexValueInteger>0?getValueAtIndex(_resultListValue->value._list,indexValueInteger):NULL); // TODO are we calling getResult anywhere????
}
*/
// LIST CONVERSIONS
////////Mvalue* ml(Menvironment* _executionEnvironment){return _getListValue(VT_LIST);} // a list that may only contain list elements is acceptable as map list!!

// list to map
/**
 * @brief returns the map representation of an M list wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the map representation of an M list wrapped in \p value
 */
Mvalue* l2m(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _mapValue=NULL;
	if(value!=NULL&&value->type==VT_LIST){
		Mmap* _map=owned_map(_getMapOfType(value->type),owner); // a strong map
		if(NULL==_map)return NULL;
		if(!listAppendedToMap(_map,owner,value->value._list))
			outputError("Failed to append a list to a map")
		; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
		_mapValue=_getValueOfMap(disowned_map(_map,owner)); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
	}
	return _mapValue;
}
// list to map list
/**
 * @brief returns the map list representation of the M list wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the map list representation of the M list wrapped in \p value
 */
Mvalue* l2ml(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _maplistValue=NULL;
	if(value!=NULL&&value->type==VT_LIST){
		Mlist* _maplist=owned_list(_getListOfType(VT_LIST),owner); // this will give me a strong list
		if(NULL==_maplist)return NULL;
		// TODO check if we're passing the right 
		if(!listAppendedToMaplist(_maplist,owner,value->value._list))
			outputError("Failed to append a list to a map list")
		; // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
		_maplistValue=_getValueOfList(disowned_list(_maplist,owner));
	}
	return _maplistValue;
}
// map list to list conversion
/**
 * @brief returns the list representation of the M map list wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the list representation of the M map list wrapped in \p value
 */
Mvalue* ml2l(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=(value!=NULL&&value->type==VT_LIST?(Mlist*)CALLOC_1(sizeof(Mlist),'L',owner):NULL);
	if(NULL==_list)return NULL;
	_list->valuetype=value->type;
		// replacing: _maplistValue=OWNED(_getListValue(value->type,false,"ml2l"),owner); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
	maplistAppendedToList(_list,owner,value->value._list); // TODO should we 'release' the map that was created somehow???? I guess the map not getting assigned will be released somehow automatically...
	return _getValueOfList(disowned_list(_list,owner));
}
/**
 * @brief returns the map representation of the M map list wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the map representation of the M map list wrapped in \p value
 */
Mvalue* ml2m(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _map=(value!=NULL&&value->type==VT_LIST?(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner):NULL);
	if(NULL==_map)return NULL;
	_map->valuetype=value->type;
	// replacing: _mapValue=OWNED(_getMapValue(value->type,false),owner); // create a map that is of the same type as the list is (typically VT_UNDEFINED)
	maplistAppendedToMap(_map,owner,value->value._list);
	return _getValueOfMap(disowned_map(_map,owner));
}
// map to map list conversion i.e. each list element is a attribute name - value pair
/**
 * @brief returns the map list representation of the M map wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the map list representation of the M map wrapped in \p value
 */
Mvalue* m2ml(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=(value!=NULL&&value->type==VT_MAP?(Mlist*)CALLOC_1(sizeof(Mlist),'L',owner):NULL);
	if(NULL==_list)return NULL;
	_list->valuetype=value->type; // TODO is this right?
	// replacing: _maplistValue=OWNED(_getListValue(VT_LIST,false,"m2ml"),owner); // a map list should always have element of type VT_LIST (this is the only additional requirement for a list to be accepted as map lists)
	mapAppendedToMaplist(_list,owner,value->value._map); // TODO should we release the list that was created somehow????
	return _getValueOfList(disowned_list(_list,owner));
}
/**
 * @brief returns the list representation of the M map wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the list representation of the M map wrapped in \p value
 */
Mvalue* m2l(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=(value!=NULL&&value->type==VT_MAP?CALLOC_1(sizeof(Mlist),'L',owner):NULL);
	if(NULL==_list)return NULL;
	_list->valuetype=value->type;
	mapAppendedToList(_list,owner,value->value._map); // TODO should we release the list that was created somehow????
	return _getValueOfList(disowned_list(_list,owner));
}
// conversion functions
 // the value wrapper for not a real and not an integer...
 /**
  * @brief the global Not-A-Float M value stored in variable NAF
  * 
  */
Mvalue* NAF_value=NULL;
/**
 * @brief the global Not-An-Integer M value stored in variable NAI
 * 
 */
Mvalue* NAI_value=NULL;
// Mvalue* NULL_value=NULL; // the value containing the text to show when a value equals NULL
/**
 * @brief the global UNDEFINED value stored in M value UNDEFINED
 * 
 */
Mvalue* UNDEFINED_value=NULL; // the value containing the text to show when a value equals UNDEFINED
/**
 * @brief the global constant with the abbreviation characters for every M value type stored in M value TYPES
 * 
 */
Mvalue* TYPES_value=NULL; // MDH@12SEP2023: the map containing all the type abbreviation characters
/**
 * @brief returns the long double representing a Not-A-Real stored in NAF_value
 * 
 * @return long double the long double representing a Not-A-Real stored in NAF_value
 */
long double getNAR(){return NAF_value->value._float->ld;}
/**
 * @brief returns the long long (M integer) representing a Not-An-Integer stored in NAI_value
 * 
 * @return long long 
 */
long long getNAI(){return NAI_value->value._integer->ll;}
// conversion to decimal,  hex and binary
// the 'real' number of octets used by a long double
#define M_LONG_DOUBLE_OCTETS 10
/**
 * @brief the union to access the octets separately constituting a long long
 * 
 */
typedef union {
	long long ll;
	uint8_t octets[sizeof(long long)];
} longlongunion;
// a long double itself is 10 octets but sizeof(long double) might be 12 or 16
/**
 * @brief the union to access the octets separately constituting a long double
 * 
 */
typedef union {
	long double ld;
	uint8_t octets[sizeof(long double)];
} longdoubleunion;
// return the value decimals in little endian order
/**
 * @brief returns the wrapper containing the M list of octets representing the long long \p ll in either little or big endian order
 * 
 * @param ll 
 * @param littleEndianOrder when true the octets are returned in little endian order, big endian order otherwise
 * @return Mvalue* the octets representing the long long \p ll wrapped in a M list
 */
Mvalue* getIntegerDecimalListValue(long long ll,bool littleEndianOrder){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _dlist=owned_list(_getListOfType(VT_INTEGER),owner);
	if(NULL==_dlist)return NULL;
	longlongunion llu;
	llu.ll=ll;
	int l=sizeof(long long);
	while(--l>=0&&appendedToList(_dlist,owner,_getIntegerValue(llu.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:M_LL_INVALID))>0);
	return _getValueOfList(disowned_list(_dlist,owner));
}
const char* const REAL_OCTET_INDEX_IDS[]={"1","2","3","4","5","6","7","8","9","10"};
/**
 * @brief returns the octets, mantisse and exponent representing long double \p ld in either little or big endian order in a wrapped M map
 * @details the octet map keys will be 1 through 10
 *          mantisse and exponent are returned as well, with key "m" and "e" respectively
 * @param ld 
 * @param littleEndianOrder when true returns the octets in little endian order, when false in big endian order
 * @return Mvalue* the M map wrapper containing the octets by (1-based) index of \p ld
 */
Mvalue* getLongDoubleDecimalMapValue(long double ld,bool littleEndianOrder){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _dmap=owned_map(_getMapOfType(VT_UNDEFINED),owner); // not just for storing integers!!!
	if(NULL==_dmap)return NULL;
	longdoubleunion lld;
	lld.ld=ld;
	int l=sizeof(long double);if(l>10)l=10; // assume 10-byte extended precision if sizeof(long double) exceeds 10 (like 12 or 16)
	// if we make a map with m0 through m7 for the mantisse, and e0 and e1 for the exponent
	if(littleEndianOrder^isLittleEndian()){ // user wants to see them in little endian order i.e. m0 first
		while(--l>=0)if(!appendedToMap(_dmap,owner,REAL_OCTET_INDEX_IDS[l],DISOWNED(OWNED(_getIntegerValue(lld.octets[l]),owner),owner)))break;
	}else{
		for(int i=0;i<l;i++)if(!appendedToMap(_dmap,owner,REAL_OCTET_INDEX_IDS[i],DISOWNED(OWNED(_getIntegerValue(lld.octets[i]),owner),owner)))break;
	}
	// how about extracting the mantisse and the exponent as well
	// TODO should we return the sign as well??????
	uint64_t mantisse;uint16_t exponent;extractMantisseAndExponent(ld,&mantisse,&exponent);
	// let's return the binary representation of exponent and mantisse with single quotes around it!!
	Mstring* _mantisseText=owned_string(_getUint64BinaryText(mantisse,'\''),owner);
	if(_mantisseText){appendedToMap(_dmap,owner,"m",_getTextValue(string(_mantisseText)));FREE_STRING(_mantisseText,owner);}
	Mstring* _exponentText=owned_string(_getUint16BinaryText(exponent,'\''),owner);
	if(_exponentText){appendedToMap(_dmap,owner,"e",_getTextValue(string(_exponentText)));FREE_STRING(_exponentText,owner);}
	/* replacing:
	Mbiginteger* _mantisse=new_Mbiginteger();mp_set_u64(_mantisse,mantisse); // we need a big integer here because uint64_t might not fit into a long long!!
	appendedToMap(_dmap,"m",getValueOfBiginteger(disowned_biginteger(_mantisse));appendedToMap(_dmap,"e",_getIntegerValue(exponent));
	*/
	return _getValueOfMap(disowned_map(_dmap,owner));
}
/**
 * @brief returns the text represention of long long \p ll
 * 
 * @param ll 
 * @return char* the text represention of long long \p ll
 */
char* _getIntegerCharacters(long long ll){//Mallocationowner owner=getOwner(__LINE__);
	char str[41];sprintf(str,"%lld",ll);return _strdup(str); // MDH@25NOV2020: for 128-bits 40 characters should do (39 if not signed)
}
/** TODO who uses this function?
 * @brief returns a M map wrapper containing the octets of the characters in the text stored in \p text
 * 
 * @param text 
 * @param ascendingindex the order in which to return the octet of the characters in \p text
 * @return Mvalue* a map with the integer representation of each character in the text wrapped in \p text
 */
Mvalue* getTextDecimalMapValue(Mtext* text,bool ascendingindex){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==text)return NULL;
	Mmap* _dmap=owned_map(_getMapOfType(VT_INTEGER),owner);
	if(NULL==_dmap)return NULL;
	char* characters=text->_c;
	long long index=0;
	char* _indexCharacters;
	if(ascendingindex){
		appendedToMap(_dmap,owner,"0",_getIntegerValue(text->presuffix)); // the quote character
		while(*characters){
			_indexCharacters=OWNED(_getIntegerCharacters(++index),owner);
			if(NULL==_indexCharacters)break; // TODO or else?
			appendedToMap(_dmap,owner,_indexCharacters,_getIntegerValue(*characters));
			FREE_DISOWNED(_indexCharacters,strlen(_indexCharacters)+1,-'"',owner);
			characters++; // OOPS pretty essential
		}
	}else{
		// go to the end
		while(*characters){index++;characters++;}
		while(index){
			_indexCharacters=OWNED(_getIntegerCharacters(index--),owner); // TODO not owned??
			if(NULL==_indexCharacters)break; // TODO or else?
			characters--;
			appendedToMap(_dmap,owner,_indexCharacters,_getIntegerValue(*characters));
			FREE_DISOWNED(_indexCharacters,strlen(_indexCharacters)+1,-'"',owner);
		}
		appendedToMap(_dmap,owner,"0",_getIntegerValue(text->presuffix)); // the quote character
	}
	return _getValueOfMap(disowned_map(_dmap,owner));
}
/**
 * @brief returns the M list wrapper containing at most 10 octets represented in long double \p ld in little or big endian order
 * 
 * @param ld 
 * @param littleEndianOrder when true, the octets are returned in little endian order, when false in big endian order
 * @return Mvalue* the M list wrapper containing at most 10 octets represented in long double \p ld in little or big endian order
 */
Mvalue* getLongDoubleDecimalListValue(long double ld,bool littleEndianOrder){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _dlist=owned_list(_getListOfType(VT_INTEGER),owner);
	if(NULL==_dlist)return NULL;
	longdoubleunion lld;
	lld.ld=ld;
	int l=sizeof(long double);if(l>10)l=10; // assume 10-byte extended precision if sizeof(long double) exceeds 10 (like 12 or 16)
	// how about adding a two-element list with the first equal to the field name?????
	while(--l>=0&&appendedToList(_dlist,owner,_getIntegerValue(lld.octets[l]),(isLittleEndian()&&littleEndianOrder?l+1:M_LL_INVALID))>0)
	;
	return _getValueOfList(disowned_list(_dlist,owner));
}

// MDH25NOV2020: converting to list and array might be useful
/**
 * @brief returns a M list wrapper representating whatever is wrapped in \p value
 * @details returns \p value when it already wraps a M list
 *          retains the value type of \p value when converting a M array or M map
 * @param value 
 * @return Mvalue* a M list wrapper representating whatever is wrapped in \p value
 */
Mvalue* Ml(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL){
		if(value->type==VT_LIST)return value;
		// we'll be needing a return list
		Mlist* _list=owned_list(__list("Ml"),owner);
		if(_list!=NULL){
			if(value->type==VT_ARRAY){
				Marray* array=value->value._array;
				if(array!=NULL){
					_list->valuetype=array->valuetype;
					unsigned long long arraylength=array->numberOfElements;
					if(arraylength>0){
						Mvalue** valueholder=array->values;
						do{
							if(appendedToList(_list,owner,*valueholder,M_LL_INVALID)<=0)
							{output(M_ERROR_PREFIX);outputValue("Failed to append '",*valueholder,"' to the list.\n");}
							valueholder++;
						}while(--arraylength);
					}
				}
			}else
			if(value->type==VT_MAP){
				Mmap* map=value->value._map;
				if(map!=NULL){
					unsigned long long maplength=map->numberOfElements;
					if(maplength>0){ // something to copy over
						Mmapelement* mapelement=map->_first;
						Mvariable* mapvariable;
						while(mapelement!=NULL){
							mapvariable=mapelement->_variable;
							if(mapvariable!=NULL){
								Mstring* _mapvariablename=owned_string(_getString("'"),owner);
								if(_mapvariablename!=NULL){
									if(string_append(_mapvariablename,mapvariable->_name->chars)){
										if(appendedToList(_list,owner,_getTextValue(string(_mapvariablename)),M_LL_INVALID)<=0
											||appendedToList(_list,owner,mapvariable->_value,M_LL_INVALID)<=0)
											outputError("Failed to either store a map key or value in the list");
									}else
										outputError("Failed to create the text to store the key in");
									FREE_STRING(_mapvariablename,owner);
								}
							}
							if(--maplength==0)break; // precaution to prevent writing beyond the end of the array (when the number of elements registered with the map would be incorrect)
							mapelement=mapelement->_next;
						}
					}
				}else
					outputBug("Value map missing!");
			}else{ // a single value, to be wrapped in a list
				if(appendedToList(_list,owner,value,M_LL_INVALID)<=0)
				{output(M_ERROR_PREFIX);outputValue("Failed to wrap '",value,"' in a list.\n");}
			}
			return _getValueOfList(disowned_list(_list,owner));
		}else
			outputError("Failed to create the list");
	}
	return NULL;
}
/**
 * @brief returns a M map wrapper representing whatever is wrapped in \p value
 * @details if \p value wraps a map, \p value is returned as is
 *          the value type of elements of a M list or M array is maintained in the returned M map TODO does not seem to be true though
 *          a simple value (not a list or map or array) is returned as the value of an empty string ("") map key
 * @param value 
 * @return Mvalue* a M map wrapper representing whatever is wrapped in \p value
 */
Mvalue* Mm(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL){
		if(value->type==VT_MAP)return value;
		Mmap* _map=owned_map(__map("Mm"),owner);
		if(_map!=NULL){
			if(value->type==VT_LIST){
				Mlist* list=value->value._list;
				if(list!=NULL){
					_map->valuetype=list->valuetype; // MDH@28MAR2023: seems to be a good idea
					Mlistelement* listelement=list->_first;
					while(listelement!=NULL){
						char* _key=OWNED(_getIntegerCharacters(listelement->index),owner);
						if(_key!=NULL){
							if(appendedToMap(_map,owner,_key,listelement->_value)!=M_TRUE)
							{output(M_ERROR_PREFIX);output("Failed to append list element #%llu to the map.\n",listelement->index);}
							FREE_DISOWNED(_key,strlen(_key)+1,-'"',owner);
						}else
							outputError("Failed to create the list index element map key");
						listelement=listelement->_next;
					}
				}else
					outputBug("Value list missing!");
			}else
			if(value->type==VT_ARRAY){
				Marray* array=value->value._array;
				if(array!=NULL){
					_map->valuetype=array->valuetype; // MDH@28MAR2023
					unsigned long long arraylength=array->numberOfElements;
					if(arraylength>0){
						Mvalue** valueholder=array->values;
						unsigned long long arrayindex=0;
						while(arrayindex<arraylength){
							char* _key=OWNED(_getIntegerCharacters(++arrayindex),owner);
							if(_key!=NULL){
								if(appendedToMap(_map,owner,_key,*valueholder)!=M_TRUE)
								{output(M_ERROR_PREFIX);output("Failed to append array element #%llu to the map.\n",arrayindex);}
								FREE_DISOWNED(_key,strlen(_key)+1,-'"',owner);
							}else
								outputError("Failed to create the array index map key");
							valueholder++;
						}
					}
				}else
					outputBug("Value array missing!");
			}else{
				if(appendedToMap(_map,owner,"",value)!=M_TRUE)
					outputError("Failed to wrap the value in a map");
			}
			return _getValueOfMap(disowned_map(_map,owner));
		}
		outputError("Failed to create the map");
	}
	return NULL;
}
/**
 * @brief returns a wrapped M array representing whatever is wrapped in \p value
 * @details if \p value wraps a M array, it is returned as is
 * @param value 
 * @return Mvalue* 
 */
Mvalue* Ma(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL){
		//outputValue("Casting '",value,"' to an array.\n");
		if(value->type==VT_ARRAY)return value;
		if(value->type==VT_LIST){
			Mlist* list=value->value._list;
			if(list!=NULL){
				// MDH@10APR2023: we actually want to have the same range so we should use the index of the last
				//                element of the list as listlength NOT the number of elements!!!!!!
				unsigned long long listlength=(list->_last!=NULL?list->_last->index:0); // replacing: list->numberOfElements;
				Marray* _array=owned_array(_getArray("Ma",listlength,NULL),owner);
				if(_array!=NULL){
					_array->valuetype=list->valuetype;
					if(listlength>0){
						Mvalue** valueholder=_array->values;
						Mlistelement* listelement=value->value._list->_first;
						unsigned long long arrayindex=0;
						while(listelement!=NULL){
							assignValue(&_array->values[listelement->index-1],listelement->_value);
							// MDH@10APR2023: if(++arrayindex==listlength)break; // done if we reached the end of the array
							listelement=listelement->_next;
							// MDH@10APR2023: valueholder++;
						}
					}
					return _getValueOfArray(disowned_array(_array,owner));
				}
				outputError("Failed to create the array to store the list elements in");			
			}else
				outputBug("No (value) list to convert to an array");
		}else
		if(value->type==VT_MAP){
			Mmap* map=value->value._map;
			if(map){
				unsigned long long maplength=map->numberOfElements;
				Marray* _array=owned_array(_getArray("Ma",maplength<<1,NULL),owner);
				if(_array){
					_array->valuetype=map->valuetype; // MDH@28MAR2023
					if(maplength>0){ // something to copy over
						Mmapelement* mapelement=map->_first;
						Mvariable* mapvariable;
						Mvalue** valueholder=_array->values;
						while(mapelement!=NULL){
							mapvariable=mapelement->_variable;
							if(mapvariable!=NULL){
								Mstring* _mapvariablename=owned_string(_getString("'"),owner);
								if(_mapvariablename){
									if(string_append(_mapvariablename,mapvariable->_name->chars)){
										assignValue(valueholder,_getTextValue(string(_mapvariablename)));
										valueholder++;
										assignValue(valueholder,mapvariable->_value);
										valueholder++;
									}
									FREE_STRING(_mapvariablename,owner);
								}
							}
							if(--maplength==0)break; // precaution to prevent writing beyond the end of the array (when the number of elements registered with the map would be incorrect)
							mapelement=mapelement->_next;
						}
					}
					return _getValueOfArray(disowned_array(_array,owner));
				}
				output("%sFailed to create the array to store %llu map elements in.",M_ERROR_PREFIX,maplength);
			}else
				outputBug("No (value) map to convert to an array!");
		}else{ // a single value, to be wrapped in an array
			Marray* _array=owned_array(_getArray("Ma",1,NULL),owner);
			if(_array!=NULL){
				assignValue(_array->values,value); // pretty simple!
				return _getValueOfArray(disowned_array(_array,owner));
			}
			outputError("Failed to create the array to wrap the value in");
		}
	}
	return NULL;
}
// we need d to compute the decimal from a given value instead of digitizing, so I suppose we'll rename d to b (for getting the bytes)
// TODO we should delegate to (_)getValueDecimal
// MDH@13AUG2023: added precision so we can get the decimal in any precision we want
/**
 * @brief returns the wrapped M decimal represented by \p value with \p precision number of decimal digits
 * 
 * @param value 
 * @param precision
 * @return Mvalue* the wrapped M decimal represented by \p value with \p precision number of decimal digits
 */
Mvalue* Md(Mvalue* value,Mvalue* precisionValue){Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL&&value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,Md,VT_UNDEFINED));
	if(value!=NULL&&value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,Md,VT_UNDEFINED));
  if(value!=NULL&&value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,Md,VT_UNDEFINED)); // MDH@28MAR2023
	Mvalue* dValue=value;
	if(value!=NULL&&value->type!=VT_DECIMAL){
		Mdecimal* _decimal=NULL;
		mpd_context_t* mpd_context=(precisionValue!=NULL?get_mpd_context(getValueInteger(precisionValue)):NULL);
		switch(value->type){
			// TODO all other types_
			case VT_INTEGER:_decimal=owned_decimal(__decimal(mpd_context,value->value._integer->ll,0),owner);break;
			case VT_BIGINTEGER:_decimal=owned_decimal(_getBigintegerDecimal(value->value._biginteger,mpd_context),owner);break;
			case VT_RATIONAL:_decimal=owned_decimal(_getRationalDecimal(value->value._rational,mpd_context),owner);break;
			case VT_FLOAT: // TODO check whether somewhere I am converting a long double without using text
			default:_decimal=owned_decimal(_getValueTextDecimal(value,mpd_context),owner);break;
		}
		// TODO if _decimal is NULL perhaps we should return NULL?????
		if(_decimal!=NULL)dValue=_getValueOfDecimal(disowned_decimal(_decimal,owner));
	}
	return dValue;
}

// MDH@18NOV2019: b/B renamed to o/O (for octets), and we're gonna create a b function for transforming to big integer
/**
 * @brief returns the wrapped M list octets of \p value
 * 
 * @param value 
 * @return Mvalue* the wrapped M list octets of \p value
 */
Mvalue* Mo(Mvalue* value){ // little-endian representation list to return
	if(value!=NULL){
		// TODO deal with all types possible
		switch(value->type){
			case VT_ARRAY:return _getValueOfArray(appliedToArray(value->value._array,Mo,VT_UNDEFINED));
			case VT_LIST:return _getValueOfList(appliedToList(value->value._list,Mo,VT_UNDEFINED));
			case VT_MAP:return _getValueOfMap(appliedToMap(value->value._map,Mo,VT_UNDEFINED)); // MDH@28MAR2023
			case VT_INTEGER:return getIntegerDecimalListValue(value->value._integer->ll,true);
			case VT_FLOAT:return getLongDoubleDecimalMapValue(value->value._float->ld,true);
			case VT_TEXT:return getTextDecimalMapValue(value->value._text,true);
			default:break;
		}
	}
	return NULL;
} 
/**
 * @brief returns the wrapped M list with big endian order octets of \p value
 * 
 * @param value 
 * @return Mvalue* the wrapped M list with big endian order octets of \p value
 */
Mvalue* MO(Mvalue* value){Mallocationowner owner=getOwner(__LINE__); // big endian decimal representation list to return
	if(value!=NULL){
		switch(value->type){
			case VT_ARRAY:return _getValueOfArray(appliedToArray(value->value._array,MO,VT_UNDEFINED));
			case VT_LIST:return _getValueOfList(appliedToList(value->value._list,MO,VT_UNDEFINED));
			case VT_MAP:return _getValueOfMap(appliedToMap(value->value._map,MO,VT_UNDEFINED)); // MDH@28MAR2023
			case VT_INTEGER:return getIntegerDecimalListValue(value->value._integer->ll,false);
			case VT_FLOAT:return getLongDoubleDecimalMapValue(value->value._float->ld,false);
			case VT_TEXT:return getTextDecimalMapValue(value->value._text,false);
			default:break;
		}
	}
	return NULL;
}
// TODO to add h/H and b/B functions

/**
 * @brief returns the wrapped M integer representation of \p value
 * @details returns NULL if \p value cannot be represented as an integer
 * @param value 
 * @return Mvalue* the wrapped M integer representation of \p value
 */
Mvalue* Mi(Mvalue* value){//Mallocationowner owner=getOwner(__LINE__);
	if(value==NULL)return NULL;
	if(value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,Mi,VT_UNDEFINED));
	if(value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,Mi,VT_UNDEFINED));
	if(value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,Mi,VT_UNDEFINED)); // MDH@28MAR2023
	if(amVerboseDebugging())outputValue("Converting '",value,"' to an integer.\n");
	long long ll=getValueInteger(value);
	return(ll!=M_LL_INVALID?_getIntegerValue(ll):NULL);
}

// convert to a big integer
/**
 * @brief returns the wrapped M big integer representation of \p value
 * 
 * @param value 
 * @return Mvalue* the wrapped M big integer representation of \p value
 */
Mvalue* Mb(Mvalue* value){//Mallocationowner owner=getOwner(__LINE__);
	if(value==NULL)return NULL;
	if(value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,Mb,VT_UNDEFINED));
	if(value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,Mb,VT_UNDEFINED));
	if(value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,Mb,VT_UNDEFINED));
	if(amVerboseDebugging())outputValue("Converting '",value,"' to a big integer.\n");
	Mvalue* bValue=value;
	if(value->type!=VT_BIGINTEGER)bValue=_getValueOfBiginteger(_getValueBiginteger(value));
	return bValue;
}

// TODO complete the q function

// double to rational conversion (called rat_approx which computes int64_t* num and denom parameters)
// now returning an Mrational*, the larger md is choosen so we might stick to using LLONG_MAX as largest possible denominator
// source: https://rosettacode.org/wiki/Convert_decimal_number_to_rational#C
/* f : number to convert.
 * num, denom: returned parts of the rational.
 * md: max denominator value.  Note that machine floating point number
 *	 has a finite resolution (10e-16 ish for 64 bit double), so specifying
 *	 a "best match with minimal error" is often wrong, because one can
 *	 always just retrieve the significand and return that divided by 
 *	 2**52, which is in a sense accurate, but generally not very useful:
 *	 1.0/7.0 would be "2573485501354569/18014398509481984", for example.
 */
/*
Mrational* _getLongDoubleRational(long double f){ // taking out: int64_t md, int64_t *num, int64_t *denom){
	//  a: continued fraction coefficients.
	long long a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	long long x, d, n = 1;
	int i, neg = 0;
 
	long long md=1000000; //////LLONG_MAX; // the largest possible long long

	// MDH@03JUN2019: with md equal to LLONG_MAX no need for: if (md <= 1) { *denom = 1; *num = (int64_t) f; return; }
 
	if (f < 0) { neg = 1; f = -f; }
 
	while (f != floor(f)) { n <<= 1; f *= 2; }
	d = f;
  
	output("%llu.",d);

	// continued fraction and check denominator each step
	for (i = 0; i < 64; i++) {
		a = n ? d / n : 0;
		if (i && !a) break;
 
		x = d; d = n; n = x % n;
 
		x = a;
		if (k[1] * a + k[0] >= md) {
			x = (md - k[0]) / k[1];
			if (x * 2 >= a || k[1] >= md)
				i = 65;
			else
				break;
		}
 
		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
	}
	return _getRational(_getBiginteger(neg?-h[1]:h[1]),_getBiginteger(k[1]),true);
	// replacing:*denom = k[1];*num = neg ? -h[1] : h[1];
}
*/
/* a Java version
public Rational limitDenominator(long maximumDenominator) {
	if (maximumDenominator < 1) {
		throw new IllegalArgumentException("Denominator cannot be less than 1.");
	}
	if(this.den <= maximumDenominator)
		// we can't get closer than the current value
		return this;
	long p0 = 0;
	long q0 = 1;
	long p1 = 1;
	long q1 = 0;
	long n = this.num;
	long d = this.den;
	while(true) {
		long a = n / d;
		long q2 = q0 + a * q1;
		if(q2 > maximumDenominator)
			break;
		long oldP0 = p0;
		p0 = p1;
		q0 = q1;
		p1 = oldP0 + a * p1;
		q1 = q2;
		long oldN = n;
		n = d;
		d = oldN - a * d;
	}
	long k = (maximumDenominator - q0) / q1;
	Rational bound1 = new Rational(p0 + k * p1, q0 + k * q1);
	Rational bound2 = new Rational(p1, q1);
	if(bound2.minus(this).abs().compareTo(bound1.minus(this).abs()) <= 0){
		return bound2;
	} else {
		return bound1;
	}
}
*/

// MDH@09OCT2019: unpure rationals can be purified using _getPurifiedRational
/**
 * @brief returns the long double wrapped in M float \p real
 * 
 * @param real 
 * @return long double 
 */
long double getReal(Mfloat* real){return(real!=NULL?real->ld:M_LD_NAN);}
/**
 * @brief returns the purified M rational of pure M rational \p pureRational with assumed inpurity \p delta
 * 
 * @param pureRational a pure M rational
 * @param delta the assumed inpurity added
 * @return Mrational* the purified M rational of \p pureRational with assumed delta \p delta
 */
Mrational* _getPurifiedRational(Mrational* pureRational,long double delta){Mallocationowner owner=getOwner(__LINE__);
	Mrational* _purifiedRational=NULL;
	if(pureRational!=NULL){
		// convert delta into a rational
		Mrational* _deltaRational=owned_rational(_getLongDoubleRational(delta,0),owner);
		if(_deltaRational!=NULL){
			_purifiedRational=owned_rational(_getPureRationalSum(pureRational,_deltaRational),owner);
			FREE_RATIONAL(_deltaRational,owner);
			if(NULL==_purifiedRational)outputError("Failed to sum two pure rationals");
		}else
			outputError("Failed to rationalize a real");
	}else
		outputError("No base pure rational to use in purification");
	return disowned_rational(_purifiedRational,owner);
}

// TODO how many iterations would we accept at most?????
/**
 * @brief returns the wrapped rational equivalent of \p value
 * 
 * @param value 
 * @return Mvalue* the wrapped rational equivalent of \p value
 */
Mvalue* MQ(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==value)return NULL;
	if(value->type==VT_RATIONAL)return value; // if the value holds a rational itself, return just that
	if(value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,MQ,VT_UNDEFINED));
	if(value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,MQ,VT_UNDEFINED));
	if(value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,MQ,VT_UNDEFINED));
	Mvalue* _rationalValue=NULL;
	if(value->type==VT_FLOAT)
		_rationalValue=_getValueOfList(_getLongDoubleRationalList(value->value._float->ld,250));
	else
		_rationalValue=_getValueOfRational(_getValueRational(value));
	if(amVerboseDebugging())
		if(_rationalValue!=NULL)
			outputValue("Converted to rational '",_rationalValue,"'.");
	return _rationalValue;
}
// MDH@09OCT2019: TODO=DONE how about turning a unpure rational into a pure rational???? yes, that's a good idea
/**
 * @brief returns the wrapped (purified) rational equivalent of \p value
 * @details if \p value wraps a rational, the purified rational is returned
 * @param value 
 * @return Mvalue* the wrapped (purified) rational equivalent of \p value
 */
Mvalue* Mq(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==value)return NULL;
	if(value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,Mq,VT_UNDEFINED));
	if(value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,Mq,VT_UNDEFINED));
	if(value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,Mq,VT_UNDEFINED));
	if(value->type==VT_RATIONAL){
		Mrational* rational=value->value._rational;
		if(rational==NULL||floatIsUndefinedOrZero(rational->delta))return value;
		long double rationaldelta=getReal(rational->delta);
		// MDH@29MAR2023: TODO since _getRational copies num and den input there's no need to create copies here!!!!!
		//                DONE no need to create _num and _den anymore!!!!!
		Mrational* _pureRational=owned_rational(_getRational(rational->num,rational->den,M_LD_NAN,true),owner); // MDH@29MAR2023: _num can be NULL as well
		/* replacing:
		// create a copy of the numerator and denominator of the provided rational
		Mbiginteger *_num=owned_biginteger(_getBigintegerCopy(rational->num),owner);
		if(rational->num!=NULL&&_num==NULL){outputError("Failed to copy the rational numerator");return NULL;}
		Mbiginteger *_den=owned_biginteger(_getBigintegerCopy(rational->den),owner);
		if(rational->den!=NULL&&_den==NULL){outputError("Failed to copy the rational denominator");FREE_BIGINTEGER(_num,owner);return NULL;}
		Mrational* _pureRational=owned_rational(_getRational(_num,_den,M_LD_NAN,true),owner); // MDH@29MAR2023: _num can be NULL as well
		*/
		Mrational* _purifiedRational=NULL;
		if(_pureRational!=NULL){
			_purifiedRational=owned_rational(_getPurifiedRational(_pureRational,rationaldelta),owner);
			FREE_RATIONAL(_pureRational,owner);
		}else
			outputError("Failed to create a pure rational");
		if(NULL==_purifiedRational){ // failed to wrap the numerator and denominator (copy), so considered unbound, and so to be freed!!!!
			// MDH@29MAR2023 removing (see above): FREE_BIGINTEGER(_num,owner);FREE_BIGINTEGER(_den,owner);
			outputError("Failed to purify a rational");
			return value; // MDH@29MAR2023: better to return the value if we fail to purify it!!!
		}
		return _getValueOfRational(disowned_rational(_purifiedRational,owner));
	}
	if(value->type==VT_FLOAT)
		return _getValueOfRational(_getLongDoubleRational(value->value._float->ld,250));
	// all remaining value types
	return _getValueOfRational(_getValueRational(value));
}
/**
 * @brief returns the wrapped M float equivalent of \p value
 * 
 * @param value 
 * @return Mvalue* the wrapped M float equivalent of \p value
 */
Mvalue* Mf(Mvalue* value){
	if(NULL==value||value->type==VT_FLOAT)return value;
	if(value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(value->value._array,Mf,VT_UNDEFINED));
	if(value->type==VT_LIST)return _getValueOfList(appliedToList(value->value._list,Mf,VT_UNDEFINED));
	if(value->type==VT_MAP)return _getValueOfMap(appliedToMap(value->value._map,Mf,VT_UNDEFINED));
	bool report=amVerboseDebugging(); //||(M_MODULE_DEBUGGING&MM_SHELL);
	long double ld=M_LD_NAN;
	if(report){outputValue("Converting '",value,"'");output(" of type %s to a floating point value.\n",VALUETYPENAMES[value->type]);}
	switch(value->type){
		case VT_INTEGER:ld=(long double)value->value._integer->ll;break;
		case VT_BIGINTEGER:if(value->value._biginteger)ld=mp_get_long_double(value->value._biginteger);break;
		case VT_DECIMAL:ld=getDecimalLongDouble(value->value._decimal);break;
		case VT_RATIONAL:ld=getRationalLongDouble(value->value._rational);break;
		case VT_TEXT:ld=_strtold(value->value._text->_c,getNAR());break;
		default:return NAF_value; // if NAF_value is returned, we do NOT disown it as we would with _floatValue being created here!!!
	}
	return(isLongDoubleUndefined(ld)==M_FALSE?_getFloatValue(ld):NULL);
}
// MDH@build 2: text representation of a value with a given format (either an integer denoting the number of positions to place the text in)
/**
 * @brief returns the wrapped M text equivalent of \p value
 * @details if \p format is defined and not an integer wrapper, NULL is returned
 * @param value 
 * @param format a wrapped integer determining whether to left-align or right-align the text representation
 * @return Mvalue* the wrapped M text equivalent of \p value
 */
Mvalue* Mt(Mvalue* value,Mvalue* format){if(format!=NULL&&format->type!=VT_INTEGER)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _result=NULL;
	// MDH@10DEC2020: this is a bit of an issue with time values in that _getValueText technically returns the epoch time text representation
	//				and not the timestamp (calendar time)
	Mstring* _valueText=owned_string(_getValueText(value,true),owner); // typically dequoted
	if(_valueText!=NULL){
		if(amVerbose())
		{outputValue("Text representation of '",value,"' before formatting: ");output("'%s'.\n",string(_valueText));}
		if(format!=NULL){
			if(format->type==VT_INTEGER){
				long long ll=format->value._integer->ll;
				if(ll>0){ // left-aligned in ll positions
					ll-=string_length(_valueText); // number of blanks to append
					while(--ll>=0)if(!string_append_char(_valueText,' '))break;
				}else
				if(ll<0){ // right-aligned in -ll positions
					ll+=string_length(_valueText); // - number of blanks to prepend
					while(++ll<=0)if(!string_insert_char(_valueText,0,' '))break;
				}
			}
		}
		if(string_insert_char(_valueText,0,(value->type==VT_TEXT?value->value._text->presuffix:'\''))){ // prepend a quote character otherwise we're in trouble in _getTextValue
			if(amVerbose())
			{outputValue("Text representation of '",value,"': ");output("'%s'.\n",string(_valueText));}
			_result=_getTextValue(string(_valueText));
		}else
			outputError("Failed to prepend a quote character to a text representation");
		FREE_STRING(_valueText,owner);
	}
	return _result;
}
Mvalue* add(Mvalue* _value1,Mvalue* _value2);
/**
 * @brief returns the sum of the elements of the list or array wrapped in \p value
 * 
 * @param value 
 * @return Mvalue* the sum of the elements of the list or array wrapped in \p value
 */
Mvalue* Msum(Mvalue* value){
	Mvalue* _sumValue=NULL;
	if(value!=NULL){
		if(amVerbose())outputValue("Computing the sum of '",value,"'.\n");
		if(value->type==VT_LIST){
			// all the values in the list could be integer
			Mlist* list=value->value._list;
			if(list){
				Mlistelement* listelement=list->_first;
				if(listelement!=NULL){
					// how about adding as decimals????
					assignValue(&_sumValue,listelement->_value);
					while(listelement->_next!=NULL){
						listelement=listelement->_next;
						assignValue(&_sumValue,add(_sumValue,listelement->_value));
					}
				}
			}
		}else
		if(value->type==VT_ARRAY){
			_sumValue=NULL;
			Marray* array=value->value._array;
			if(array!=NULL&&array->numberOfElements>0){
				register unsigned long long arrayindex=1;
				// TODO how about skipping all NULL values??????
				assignValue(&_sumValue,array->values[0]);
				while(arrayindex<array->numberOfElements)
					assignValue(&_sumValue,add(_sumValue,array->values[arrayindex++]));
				// outputValue("Sum: ",_sumValue,".\n");
			}
		}else // if not something that can be summed, returning the original value
			_sumValue=value;
	}
	return _sumValue;
}

// MDH@10OCT2019: applying unary operator (=function) to all elements in a list
/**
 * @brief returns the wrapped M list with M function \p function applied to the elements of \p _list
 * 
 * @param _list 
 * @param function 
 * @param maintainsValuetype whether or not to maintain the list value type
 * @return Mvalue* the wrapped M list with M function \p function applied to the elements of \p _list
 */
Mvalue* _functionAppliedToList(Mlist* _list,OneArgumentFunction function,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mlist* _result=NULL;
	if(function!=NULL&&_list!=NULL){ // we need both a function and a list
		_result=owned_list(_getListOfType(maintainsValuetype?_list->valuetype:VT_UNDEFINED),owner); // this could pose a problem as the function may not return the same value type as the elements in the list (i.e. if it doesn't we're in trouble!!!!)
		Mlistelement* _listelement=_list->_first;
		while(_listelement!=NULL&&appendedToList(_result,owner,function(_listelement->_value),_listelement->index))
			_listelement=_listelement->_next;
	}
	return(_result!=NULL?_getValueOfList(disowned_list(_result,owner)):NULL);
}
/**
 * @brief returns the wrapped M array with M function \p function applied to the elements of \p _array
 * 
 * @param _array 
 * @param function 
 * @param maintainsValuetype 
 * @return Mvalue* the wrapped M array with M function \p function applied to the elements of \p _array
 */
Mvalue* _functionAppliedToArray(Marray* _array,OneArgumentFunction function,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Marray* _result=NULL;
	if(function!=NULL&&_array!=NULL){ // we need both a function and a list
		_result=owned_array(_getArray("_functionAppliedToArray",_array->numberOfElements,NULL),owner); // this could pose a problem as the function may not return the same value type as the elements in the list (i.e. if it doesn't we're in trouble!!!!)
		if(_result!=NULL){
			if(maintainsValuetype)_result->valuetype=_array->valuetype;
			// using pointer arithmetic is the way to go
			long long arrayindex=_array->numberOfElements;
			if(arrayindex>0){
				Mvalue** _resultelementValueholder=_result->values+arrayindex;
				Mvalue** _arrayelementValueholder=_array->values+arrayindex; // the value to which the function is to be applied
				do{
					_resultelementValueholder--;
					_arrayelementValueholder--;
					assignValue(_resultelementValueholder,function(*_arrayelementValueholder));
				}while(--arrayindex>0);
			}
		}
	}
	return(_result!=NULL?_getValueOfArray(disowned_array(_result,owner)):NULL);
}
// MDH@29MAR2023: implementation of function applied to map
/**
 * @brief returns the wrapped M map with function \p function applied to all values in \p _map
 * 
 * @param _map 
 * @param function 
 * @param maintainsValuetype whether or not to maintain the value type
 * @return Mvalue* the wrapped M map with function \p function applied to all values in \p _map
 */
Mvalue* _functionAppliedToMap(Mmap* _map,OneArgumentFunction function,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _result=NULL;
	if(_map!=NULL&&function!=NULL){
		_result=owned_map(__map("_functionAppliedToMap"),owner); // this could pose a problem as the function may not return the same value type as the elements in the list (i.e. if it doesn't we're in trouble!!!!)
		if(_result!=NULL){
			if(maintainsValuetype)_result->valuetype=_map->valuetype;
			unsigned long long numberOfMapElements=_map->numberOfElements;
			Mmapelement* mapelement=_map->_first;
			while(numberOfMapElements-->0&&mapelement!=NULL){
				Mvariable* variable=mapelement->_variable;
				if(variable!=NULL&&!appendedToMap(_map,owner,variable->_name->chars,function(variable->_value))){
					outputError("Failed to append a map element");break;
				}
				mapelement=mapelement->_next;
			}
		}
	}
	return(_result!=NULL?_getValueOfMap(disowned_map(_result,owner)):NULL);
}

// MDH@10OCT2019: a special function to compute a reciprocal value
/**
 * @brief returns the reciprocal of \p value
 * 
 * @param value 
 * @return Mvalue* the reciprocal of \p value
 */
Mvalue* Mreciprocal(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _reciprocalValue=NULL;
	if(value!=NULL)
	switch(value->type){
		// composite types
		case VT_ARRAY:_reciprocalValue=_functionAppliedToArray(value->value._array,Mreciprocal,false);break;
		case VT_LIST:_reciprocalValue=_functionAppliedToList(value->value._list,Mreciprocal,false);break;
		case VT_MAP:_reciprocalValue=_functionAppliedToMap(value->value._map,Mreciprocal,false);break;
		// scalar types
		case VT_FLOAT:_reciprocalValue=_getFloatValue(1/value->value._float->ld);break; // TODO check what happens when the real equals 0
		case VT_RATIONAL:_reciprocalValue=_getValueOfRational(_getInverseRational(value->value._rational));break;
		case VT_INTEGER:
			{
				Mbiginteger* _denominator=owned_biginteger(_getBiginteger(value->value._integer->ll),owner); // create the big integer denominator
				if(_denominator!=NULL){
					_reciprocalValue=_getValueOfRational(_getRational(NULL,_denominator,M_LD_NAN,true));
					FREE_BIGINTEGER(_denominator,owner); // free the created big integer used to create the rational
				}else
					outputMemoryError("Failed to create a big integer");
			}
			break;
		case VT_BIGINTEGER:_reciprocalValue=_getValueOfRational(_getRational(NULL,value->value._biginteger,M_LD_NAN,true));break; // same as with VT_INTEGER but without freeing the to remain bound big integer
		case VT_DECIMAL:_reciprocalValue=_getValueOfDecimal(_getInverseDecimal(value->value._decimal));break;
		default:break;
	}
	return _reciprocalValue;
}
// MDH@29OCT2019: concatenate textual, typically used for lists
/**
 * @brief returns the M string wrapping the text representation of all elements of \p list separated by \p separator
 * 
 * @param list 
 * @param separator 
 * @return Mstring* the M string wrapping the text representation of all elements of \p list separated by \p separator
 */
static Mstring* _getConcatenated(Mlist* list,char* separator){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _concatenated=owned_string(__string(),owner);
	if(_concatenated!=NULL){
		Mlistelement* listelement=list->_first;
		Mvalue* listelementValue=NULL;
		while(listelement!=NULL){
			listelementValue=listelement->_value;
			Mstring* _listelementText=NULL;
			if(listelementValue!=NULL){
				if(listelementValue->type==VT_LIST)
					_listelementText=owned_string(_getConcatenated(listelementValue->value._list,separator),owner);
				else
					_listelementText=owned_string(_getValueText(listelementValue,true),owner);
			}
			if(_listelementText!=NULL){
				if(separator!=NULL)if(string_length(_concatenated)>0)string_append(_concatenated,separator);
				string_append(_concatenated,string(_listelementText));
				FREE_STRING(_listelementText,owner);
			}
			listelement=listelement->_next;
		}
	}else 
		outputError("Failed to initialize the concatenation result text");
	return disowned_string(_concatenated,owner);
}
/**
 * @brief returns the wrapped M text representation of wrapped M list \p value1 using \p value2 as separator
 * 
 * @param value1 
 * @param value2 
 * @return Mvalue* the wrapped M text representation of wrapped M list \p value1 using \p value2 as separator
 */
Mvalue* Mconcat(Mvalue* value1,Mvalue* value2){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _concatValue=NULL;
	// the first value would be the list of things to concatenate, the second value the separator text (if any)
	if(value1!=NULL){
		Mstring* _separator=(value2!=NULL?owned_string(_getValueText(value2,true),owner):NULL); // _getValueText() would return ? when receiving NULL, so for now we have to prevent that!!
		Mstring* _concat;
		if(value1->type==VT_LIST)
			_concat=owned_string(_getConcatenated(value1->value._list,(_separator!=NULL?string(_separator):NULL)),owner);
		else
			_concat=owned_string(_getValueText(value1,true),owner);
		if(_concat!=NULL){
			// _result itself won't contain quotes, so in order to make it usable we need to prepend either a single quote or a double quote
			if(string_insert_char(_concat,0,'\''))
				_concatValue=_getTextValue(string(_concat));
			else 
				outputError("Failed to construct the concatenation text");
			FREE_STRING(_concat,owner);
		}
		if(_separator!=NULL)FREE_STRING(_separator,owner);
	}
	return _concatValue;
}
/**
 * @brief returns the Fibonacci number at integer index \p value
 * 
 * @param value 
 * @return Mvalue* the Fibonacci number at integer index \p value
 */
Mvalue* Mfibonacci(Mvalue* value){Mallocationowner owner=getOwner(__LINE__);
	// are we allowing big integers?
	if(NULL==value)return NULL;
	if(value->type==VT_ARRAY)return _functionAppliedToArray(value->value._array,Mfibonacci,false);
	if(value->type==VT_LIST)return _functionAppliedToList(value->value._list,Mfibonacci,false);
	if(value->type==VT_MAP)return _functionAppliedToMap(value->value._map,Mfibonacci,false);
	Mvalue* _fibonnacciValue=NULL;
	// ASSERT assuming scalars
	Mbiginteger* _biginteger=owned_biginteger(_getValueBiginteger(value),owner);
	if(_biginteger!=NULL){
		mp_err status=MP_OKAY; // keep track of the result status
		Mbiginteger* _fibonacciBiginteger=NULL;
		if(isBigintegerUndefined(_biginteger)==M_FALSE){ // not an undefined big integer
			if(isBigintegerNegative(_biginteger)!=M_TRUE){ // not a negative big integer
				Mbiginteger *_counterBiginteger=owned_biginteger(_getBigintegerCopy(_biginteger),owner); // the number of times we will have to do an addition
				_fibonacciBiginteger=owned_biginteger(__biginteger(),owner); // where the result should be stored
				if(_fibonacciBiginteger!=NULL&&_counterBiginteger!=NULL)status=mp_decr(MP_INT_POINTER(_counterBiginteger));
				else status=MP_ERR;
				if(status==MP_OKAY){
					if(isBigintegerPositive(_counterBiginteger)==M_TRUE){ // at least one addition to do
						Mbiginteger *_firstBiginteger=owned_biginteger(_getBiginteger(0),owner),*_secondBiginteger=owned_biginteger(_getBiginteger(1),owner);
						if(_firstBiginteger!=NULL&&_secondBiginteger!=NULL){
							// NOTE _counterBiginteger defines the number of times we need to add the first and second big integer
							while(isBigintegerZero(_counterBiginteger)!=M_TRUE){ // the counter is not zero yet
								if((status=mp_decr(MP_INT_POINTER(_counterBiginteger)))!=MP_OKAY)break;
								if((status=mp_add(MP_INT_POINTER(_firstBiginteger),MP_INT_POINTER(_secondBiginteger),MP_INT_POINTER(_fibonacciBiginteger)))!=MP_OKAY)break;
								// if we're smart we only need to exchange one big integer
								if((status=mp_copy(MP_INT_POINTER(_secondBiginteger),MP_INT_POINTER(_firstBiginteger)))!=MP_OKAY)break;
								if((status=mp_copy(MP_INT_POINTER(_fibonacciBiginteger),MP_INT_POINTER(_secondBiginteger)))!=MP_OKAY)break;
							}
						}else 
							outputError("Failed to initialize the Fibonacci sequence");
						FREE_BIGINTEGER(_firstBiginteger,owner);FREE_BIGINTEGER(_secondBiginteger,owner);
					}else // no additions
						status=mp_copy(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_fibonacciBiginteger));
				}else
					outputError("Failed to initialize the Fibonacci sum");
				FREE_BIGINTEGER(_counterBiginteger,owner);
			}
		}
		if(value->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger,owner);
		if(status==MP_OKAY)_fibonnacciValue=_getValueOfBiginteger(disowned_biginteger(_fibonacciBiginteger,owner));
	}
	return _fibonnacciValue;
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
			if(_itemvalue->type==VT_TEXT&&_value->type==VT_MAP){
				_value=getMapElement(_value->value._list,_itemvalue->value._text);
			}else // invalid reference
				_value=NULL;
		}
	}
	return _value;
}
*/

/* MDH@26OCT2019: moved over to Mvalue.h/c so we can use it in Mvalue's as well
// MDH@06MAY2019: Mvaluereference stands for a variable in combination with an item id, this will allow assignments as we know the variable involved!!!!
typedef struct Mvaluereference{
	char* _name; // the name of the host variable or NULL if we're in a substructure
	Mvalue* _value; // either the host value (if no variable name is defined), or the value of the host variable
	Mvalue* _itemid; // the item referenced!!!
}Mvaluereference;
*/
/*
typedef struct Mexpressionvalue{
	Mvaluereference* _valuereference; // thre result of evaluating an expression is always a single value!!!
	Mtoken* token; // supposed to be the token the evaluation ended with (so the token in front of the first in the next evaluation)
}Mexpressionvalue;
// free_expressionvalue does NOT free the token as it will probably be passed on...
void free_expressionvalue(Mexpressionvalue* _expressionvalue){
	if(_expressionvalue){
		if(amVerbose())outputInfo("Releasing the result!");
		// MDH@03MAY2019: append the result value at the proper index (as indicated by the command index)
		if(_resultListValue){if(appendedToList(_resultListValue->value._list,_expressionvalue->_valuereference->_variable->_value,commandCount+1))outputInfo("ERROR: Failed to save the result.");else if(amVerbose())outputInfo("Result saved.");}
		// replacing: if(!appendToListVariable(_Menvironment,"M",_expressionvalue->_value))outputInfo("ERROR: Failed to append the result to the M list.");else if(amVerbose())outputInfo("Result appended to the M list.");
		//// NEVER free what does not have an underscore at the start!!!! FREE_TOKEN(_expressionvalue->token); // probably NULLed already as this will not be new token, so I guess we could remove the _ to prevent freeing!!!
		////////output("Freeing expression!");
		decrementReferenceCount(_expressionvalue->_valuereference->_variable->_value); // as where freeing _expressionvalue!!!!
		free(_expressionvalue);
	}else
	if(amVerbose())outputInfo("No result to free!");
	
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
			case VT_FLOAT:_value->value._float=(Mfloat*)calloc(1,sizeof(Mfloat));break; // initialized to 0.0 I presume
			case VT_TEXT:_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
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

/* MDH@07JUL2019: we need to put the current token of the expression being evaluated in the execution environment, so we can execute a user function call
				  without loosing the original expression we're evaluating
Mtoken* expressionToken=NULL; // the current evaluation token
*/

/**
 * @brief returns a copy of \p _token ready for evaluation
 * 
 * @param _token 
 * @return Mtoken* a copy of \p _token ready for evaluation
 */
Mtoken* _getEvaluatableTokenCopy(Mtoken* _token){Mallocationowner owner=getOwner(__LINE__);
	Mtoken* _tokenCopy=(_token!=NULL?owned_token(__token(),owner):NULL);
	if(_tokenCopy!=NULL){
		if(amVerbose()){
			output("Copying token '%s' of type '%s'.\n",string(_token->text),TOKENTYPE_STRING[_token->type]);
			if(_token->expr)output("\tpointing to token '%s' of type '%s'.\n",string(_token->expr->text),TOKENTYPE_STRING[_token->expr->type]);
		}
		_tokenCopy->type=_token->type;
		setTokenSignificantCharacterCount(_tokenCopy,getTokenSignificantCharacterCount(_token));
		if(_token->text!=NULL)_tokenCopy->text=owned_string(_getTokenText(_token),Msubowner(owner,1)); // copy the entire token text
		_tokenCopy->expr=_token->expr; // TODO do I need to do this??? this is also an issue because if we start comparing expr (on evaluation)
		_tokenCopy->argument=_token->argument; // MDH@11AUG2019: we need the argument as well bro' TODO how about the envid?????
		// we're NOT copying _next, _prev, _offset
		//////_tokenCopy->prev=NULL;_tokenCopy->next=NULL;_tokenCopy->offset=0;
	}
	return disowned_token(_tokenCopy,owner);
}

// NOTE by adding endTokenType and maximumNumberOfElements to getListExpressionValue we can use it as well for getting an arguments list...
/**
 * @brief evaluates a list in the current expression being evaluated
 * 
 * @param endTokenType ends the list being evaluated
 * @param maximumNumberOfElements the maximum number of elements to evaluate
 * @param numberOfElementsToNotEvaluate the number of elements not to evaluate at the start of the list
 * @param weak whether or not the result list should be flagged as weak
 * @return Mvalue* the evaluated list from the current expression being evaluated
 */
static Mvalue* getValueOfList(TokenType endTokenType,uint32_t maximumNumberOfElements,uint32_t numberOfElementsToNotEvaluate,bool weak){Mallocationowner owner=getOwner(__LINE__);
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // does NOT need to be freed, so no _ in front of it!
	if(amVerboseDebugging())
		output("Composing a list of %u elements with %u unevaluatable elements starting with '%s'.\n",maximumNumberOfElements,numberOfElementsToNotEvaluate,string(expressionToken->text));
	// MDH@21MAY2019: _getListValue() as opposed to getValueOfExpressionOfType() creates a Mvalue on the value list which will be removed when the reference count of the Mvalue list ends up being 0
	//				then, the list element values will be dereferenced and if their reference count becomes zero freed as well successfully!!!!
	Mlist* _list=owned_list(__list("getValueOfList"),owner);
	if(NULL==_list){
		outputError("Failed to create a list to return");
		return NULL;
	}
	_list->weak=weak;
	/* MDH@27MAY2020 replacing:
	Mvalue* _listValue=_getListValue(VT_UNDEFINED,weak,"getValueOfList"); // replacing: getValueOfExpressionOfType(VT_LIST);
	Mlist* _list=_listValue->value._list; // grab the (empty) list to fill
	*/
	if(_list->_first!=NULL||_list->_last!=NULL){
		outputError("Supposedly empty list not initialized correctly");
		return NULL;
	}
	if(amVerboseDebugging())
		output("Composing a list starting with token '%s' of type '%s'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
	///////enum TOKENTYPE_ENUM listElementEndTokenTypes[]={TT_END_OF_LIST,TT_LISTELEMENT};
	// we iterate over the list elements, so at the start we assume expressionToken represents the start token of the list (literal)
	unsigned long long listElementIndex=0;
	uint32_t firstElementToNotEvaluate=(maximumNumberOfElements==0||numberOfElementsToNotEvaluate>maximumNumberOfElements?0:maximumNumberOfElements-numberOfElementsToNotEvaluate+1);
	if(amVerboseDebugging())
		output("First element not to evaluate: %u.\n",firstElementToNotEvaluate);
	Mtoken* expr=expressionToken; // we need this when we are not to evaluate a list element, this will match the expr of all comma's and the list end token
	// keep advancing the expression token until we're out of them (MDH@17JUL2019: now getting them from the current execution environment)
	while((expressionToken=nextEnvironmentExpressionToken())!=NULL){
		if(expressionToken->type==endTokenType)break; // missing elements should be skipped but counted
		listElementIndex++;
		if(amVerboseDebugging())
			output("Processing list element #%llu starting with token '%s' of type '%s'.\n",listElementIndex,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		Mvalue* _listElementValue=NULL;
		if(firstElementToNotEvaluate>0&&listElementIndex>=firstElementToNotEvaluate){ // copy the tokens in the argument
			// it's easier to tell getValueOfExpression not to evaluate the tokens and make it copy them by passing in a boolean flag
			// however this would require passing the bool argument along to every function getValueOfExpression calls
			// so it's easier to find where this list element ends by checking expr on a list element or end of list we encounter in forward direction
			Mtoken* _firstUnevaluatedToken=owned_token(_getEvaluatableTokenCopy(expressionToken),owner);
			if(_firstUnevaluatedToken!=NULL){
				if(amVerboseDebugging())
					output("Evaluating special function call argument tokens:");
				Mtoken* unevaluatedToken=_firstUnevaluatedToken;
				while(unevaluatedToken!=NULL){
					if(amVerboseDebugging())
						output(" %s(%" PRId32 ")",string(unevaluatedToken->text),unevaluatedToken->argument);
					expressionToken=nextEnvironmentExpressionToken();
					if(NULL==expressionToken)break; // NOTE shouldn't happen though
					if(NULL==expressionToken->expr||expressionToken->expr==expr)if(expressionToken->type==endTokenType||expressionToken->type==TT_LISTELEMENT)break;
					unevaluatedToken->next=owned_token(_getEvaluatableTokenCopy(expressionToken),owner); // set next to the copy of the expression token
					unevaluatedToken=unevaluatedToken->next;
				}
				if(amVerboseDebugging())
					outputChar('\n');
				_listElementValue=_getValueOfToken(disowned_token(_firstUnevaluatedToken,owner));
			}
		}else{ // evaluate
			// theoretically it is possible that this list element is empty in which case we should append NULL to the list
			_listElementValue=(expressionToken->type!=TT_LISTELEMENT?getValueOfExpression("list element",'l',(TokenType[]){endTokenType,TT_LISTELEMENT},2):NULL);
			expressionToken=getEnvironmentExpressionToken(); // essential after calling any function that might advance the current token pointer
			if(amVerboseDebugging())
				outputValue("List element value: '",_listElementValue,"'.\n");
		}
		if(NULL==_listElementValue){
			if(amVerboseDebugging())
				outputInfo("List element missing!");
			continue;
		} // undefined list elements should NEVER be added to the list
		if(amVerboseDebugging())
			if(expressionToken!=NULL)
				output("List element ending token: %s.\n",TOKENTYPE_STRING[expressionToken->type]);
		// get the next list element value, here's a problem as we're supposed to return the offset not the first token
		// if we already have the maximum number of elements, we do not append this list element!!!
		// we're NOT using the number of elements in the list to check agains anymore but the list element index
		if(maximumNumberOfElements==0||listElementIndex<=maximumNumberOfElements){
			if(amVerboseDebugging())
				output("Appending list element #%llu.\n",listElementIndex);
			long long newListElementIndex=appendedToList(_list,owner,_listElementValue,listElementIndex); // TODO
			// MDH@21MAY2019 IMPORTANT: because NULL list elements are NOT stored explicitly in the list (because a list is stored sparse), the list index should be passed in
			if(newListElementIndex<=0){
				output("%s",M_ERROR_PREFIX);
				outputValue("Failed to append list element '",_listElementValue,"'.\n");
				break;
			}
			if(amVerboseDebugging())
				output("List element #%lld appended to list with index %lld!\n",listElementIndex,newListElementIndex);
		}else
		if(amVerboseDebugging())
			output("Maximum number of elements reached.\n");
		if(NULL==expressionToken)break; // MDH@15OCT2019: might be useful!! TODO how can we prevent this from happening????????
		if(expressionToken->type==endTokenType)break; // the list element could have ended with the end token type, in which case we're done!!!
	}
	Mvalue* _listValue=_getValueOfList(disowned_list(_list,owner));
	if(amVerboseDebugging())
		outputValue("List '",_listValue,"' extracted!\n");
	return _listValue;
}

// MDH@25JUN2023: now anything between [ and ] should be returned as an array not as a list
/**
 * @brief returns the array represented by the array literal (marked using the list tokens [ and ])
 * 
 * @return Mvalue* the array represented by the array literal
 */
static Mvalue* getValueOfArray(){
	return Ma(getValueOfList(TT_END_OF_LIST,0,0,false));
	/* equivalent to
	Mvalue* listValue=getValueOfList(TT_END_OF_LIST,0,0,false);
	if(listValue!=NULL&&listValue->type==TT_LIST&&listValue->value._list){

	}
	return NULL;
	*/
}
/**
 * @brief returns the evaluated value of a map in the expression being evaluated
 * 
 * @return Mvalue* the evaluated value of a map in the expression being evaluated
 */
static Mvalue* getValueOfMap(){Mallocationowner owner=getOwner(__LINE__);
	Mtoken* expressionToken=getEnvironmentExpressionToken(); // MDH@17JUL2019: one of five functions that use and advance the current expression token
	Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
	/* MDH@27MAY2020 replacing:
	Mvalue* _mapValue=_getMapValue(VT_UNDEFINED,false); // MDH@21MAY2019 for the same reason as above: replacing: getValueOfExpressionOfType(VT_MAP);
	Mmap* _map=_mapValue->value._map; // grab the map to fill
	*/
	//enum TOKENTYPE_ENUM mapAttributeNameEndTokenTypes[]={TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT};
	//enum TOKENTYPE_ENUM mapAttributeValueEndTokenTypes[]={TT_END_OF_MAP,TT_LISTELEMENT};
	// NOTE a map can be empty in which case _firstToken will immediately be of type TT_END_OF_MAP
	while((expressionToken=nextEnvironmentExpressionToken())!=NULL){
		if(expressionToken->type==TT_END_OF_MAP)break;
		if(expressionToken->type==TT_LISTELEMENT)continue; // missing attribute name-value pair
		// get the next attribute name, value pair
		// obviously the name should be something that evaluates to a string
		Mvalue* _attributeNameValue=getValueOfExpression("map attribute name",'s',(TokenType[]){TT_MAP_VALUE,TT_END_OF_MAP,TT_LISTELEMENT},3);
		expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
		// MDH@22JUL2019: it's better to dequote the name here because otherwise the name of the attribute would be in quotes (and it is clear to be text)
		Mstring* _attributeName=owned_string(_getValueText(_attributeNameValue,true),owner); // parse the attribute name value (could be undefined though)
		// NOTE _attributeNameValue will be released after evaluation because it is not assigned to something else...
		/////////////////////if(expressionToken->type==TT_END_OF_MAP)break;
		// for now let's decide to simply not store the attribute if the name is not of type string
		Mvalue* _attributeValueValue=NULL;
		if(expressionToken->type==TT_MAP_VALUE){
			expressionToken=nextEnvironmentExpressionToken(); // move to first element after the colon
			_attributeValueValue=getValueOfExpression("map attribute value",'v',(TokenType[]){TT_END_OF_MAP,TT_LISTELEMENT},2);
			expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
		}
		if(NULL==_attributeName)continue; // unable to parse the attribute name expression value into a string
		// MDH@22JUL2019: let's allow empty attribute name as well (why not!)
		////////if(string_length(_attributeName)>0)
		if(_map==NULL||!appendedToMap(_map,owner,string(_attributeName),_attributeValueValue)){
			output("%s",M_ERROR_PREFIX);outputValue("Failed to append the value of attribute '",_attributeNameValue,"'.\n");
		} // NOTE can't break until we actually bump into the TT_END_OF_MAP!!!
		FREE_STRING(_attributeName,owner); // ALWAYS free the name text
		if(NULL==expressionToken)break;
		if(expressionToken->type==TT_END_OF_MAP)break;
		if(amVerboseDebugging())output("Continued map parsing with token of type '%s'.\n",TOKENTYPE_STRING[expressionToken->type]);
	}
	Mvalue* _mapValue=_getValueOfMap(disowned_map(_map,owner));
	if(amVerboseDebugging())outputValue("Map '",_mapValue,"' extracted!\n");
	return _mapValue;
}

// a function call needs a function and a map of arguments (defining the values to use for the formal parameters of the function)
/**
 * @brief returns the evaluated value of a function call in the expression being evaluated
 * 
 * @param _function 
 * @param functionName 
 * @param _argumentMap 
 * @return Mvalue* the evaluated value of a function call in the expression being evaluated
 */
static Mvalue* getValueOfFunctionCall(Mfunction* _function,char* functionName,Mmap* _argumentMap){Mallocationowner owner=getOwner(__LINE__);
	////////Mvalue* _resultValue=NULL;
	switch(_function->type){
		case FT_USER:
			{
				// TODO replace following by calling getFunctionExecutionEnvironment
				// 1. create an environment in which to execute the expression list of the given function initialized with the argument map provided with the current argument variable values
				Menvironment* _functionExecutionEnvironment=owned_environment(_getFunctionExecutionEnvironment(_function,functionName,_argumentMap),owner);
				if(_functionExecutionEnvironment!=NULL){
					// ASSERT now it exists I ALWAYS need to free it 
					// MDH@21OCT2020: it makes sense to pass a disowned version of environment to pushExecutionEnvironment() so it can take over ownership
					//				NO because I want to free this given environment push should not take over ownership
					if(pushExecutionEnvironment(disowned_environment(_functionExecutionEnvironment,owner))){
						// ASSERT once pushed successfully I am responsible of ALWAYS popping
						// execute ALL the commands in _bodyCommandList
						Mlist* functionBodyCommandList=_function->functionunion._userfunction->_bodyCommandList;
						Mvalue *functionEvaluationValue=NULL,*functionBodyCommandValue=NULL;
						if(functionBodyCommandList!=NULL){
							Mlistelement* functionBodyCommandListelement=functionBodyCommandList->_first;
							while(functionBodyCommandListelement!=NULL){
								// MDH@22JUL2019: ALWAYS skip the initial dummy TT_EXPRESSION token of any command!!
								_functionExecutionEnvironment->expressionToken=functionBodyCommandListelement->_value->value._token->next;
								// evaluate the body command and remember the result
								functionBodyCommandValue=getValueOfExpression("function body command evaluation",'f',(TokenType[]){},0);
								// MDH@24JUL2019: check the function exit flag variable if it is set we're done
								// MDH@10JAN2021
								if(isImmutable(getVariable(NULL,"$",false))==M_TRUE)break; 
								/* replacing:
								if(getValue(_functionExecutionEnvironment,"!"))break; // the exit variable is set (by the return statement!!!!)
								*/
								functionEvaluationValue=functionBodyCommandValue; // store command evaluation result as function result
								if(!setVariable(_functionExecutionEnvironment,"",functionEvaluationValue))
									outputError("Failed to store the function command execution value");
								if(amVerbose())outputValue("Function evaluation value so far: '",functionEvaluationValue,"'.\n");
								functionBodyCommandListelement=functionBodyCommandListelement->_next;
							}
						}else
							output("No commands in body of user function '%s' to execute!\n",functionName);
						// before popping the function execution environment, see if the result was set
						if(amVerboseDebugging())output("Extracting the result of the execution of function '%s'.\n",functionName);
						Mvalue* functionResultValue=getValue(_functionExecutionEnvironment,"$");
						if(amVerboseDebugging())output("Exiting the environment of executing function '%s'.\n",functionName);
						popExecutionEnvironment();
						// the function result value (if set) takes precedence over the function evaluation value
						return (functionResultValue!=NULL?functionResultValue:functionEvaluationValue);
					}
					// ASSERT failed to push the created function execution environment which also means it failed to be bound in a value, and thus I need to free it as it will not be garbage-collected like any value would
					output("%sFailed to create the function execution environment of function '%s'.\n",M_ERROR_PREFIX,functionName);
					FREE_ENVIRONMENT(_functionExecutionEnvironment,owner);
				}else
					output("%sFailed to create the environment to execute function '%s'.\n",M_ERROR_PREFIX,functionName);
				return NULL;
			}
			break;
		case FT_INTERNAL_NO_ARGUMENTS:
			if(amVerboseDebugging())output("Calling no-argument function '%s'.\n",functionName);
			return (*_function->functionunion.noArgumentFunction)();
		case FT_INTERNAL_ONE_ARGUMENT:
			if(amVerboseDebugging())
			{output("Applying one-argument function '%s'",functionName);outputValue(" to '",_argumentMap->_first->_variable->_value,"'.\n");}
			return (*_function->functionunion.oneArgumentFunction)(_argumentMap->_first->_variable->_value);
		case FT_INTERNAL_TWO_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_next:NULL);
				if(amVerboseDebugging()){
					output("Applying two-argument function '%s'",functionName);
					if(_firstArgumentmapelement!=NULL)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement!=NULL)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.twoArgumentFunction)((_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_variable->_value:NULL)
																	  ,(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_variable->_value:NULL));
			}
		case FT_INTERNAL_THREE_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_next:NULL);
				Mmapelement* _thirdArgumentmapelement=(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_next:NULL);
				if(amVerboseDebugging()){
					output("Applying three-argument function '%s'",functionName);
					if(_firstArgumentmapelement!=NULL)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement!=NULL)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement!=NULL)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.threeArgumentFunction)((_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement!=NULL?_thirdArgumentmapelement->_variable->_value:NULL));
			}
		case FT_INTERNAL_FOUR_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_next:NULL);
				Mmapelement* _thirdArgumentmapelement=(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_next:NULL);
				Mmapelement* _fourthArgumentmapelement=(_thirdArgumentmapelement!=NULL?_thirdArgumentmapelement->_next:NULL);
				if(amVerboseDebugging()){
					output("Applying four-argument function '%s'",functionName);
					if(_firstArgumentmapelement!=NULL)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement!=NULL)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement!=NULL)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
					if(_fourthArgumentmapelement!=NULL)outputValue(" and '",_fourthArgumentmapelement->_variable->_value,"'");
					outputChar('.');outputChar('\n');
				}
				return (*_function->functionunion.fourArgumentFunction)((_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement!=NULL?_thirdArgumentmapelement->_variable->_value:NULL)
																		,(_fourthArgumentmapelement!=NULL?_fourthArgumentmapelement->_variable->_value:NULL));
			}
			break;
		case FT_INTERNAL_FIVE_ARGUMENTS:
			{
				Mmapelement* _firstArgumentmapelement=_argumentMap->_first;
				Mmapelement* _secondArgumentmapelement=(_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_next:NULL);
				Mmapelement* _thirdArgumentmapelement=(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_next:NULL);
				Mmapelement* _fourthArgumentmapelement=(_thirdArgumentmapelement!=NULL?_thirdArgumentmapelement->_next:NULL);
				Mmapelement* _fifthArgumentmapelement=(_fourthArgumentmapelement!=NULL?_fourthArgumentmapelement->_next:NULL);
				if(amVerboseDebugging()){
					output("Applying five-argument function '%s'",functionName);
					if(_firstArgumentmapelement!=NULL)outputValue(" to '",_firstArgumentmapelement->_variable->_value,"'");
					if(_secondArgumentmapelement!=NULL)outputValue(" and '",_secondArgumentmapelement->_variable->_value,"'");
					if(_thirdArgumentmapelement!=NULL)outputValue(" and '",_thirdArgumentmapelement->_variable->_value,"'");
					if(_fourthArgumentmapelement!=NULL)outputValue(" and '",_fourthArgumentmapelement->_variable->_value,"'");
					if(_fifthArgumentmapelement!=NULL)outputValue(" and '",_fifthArgumentmapelement->_variable->_value,"'");
					outputChar('.');newline();
				}
				return (*_function->functionunion.fiveArgumentFunction)((_firstArgumentmapelement!=NULL?_firstArgumentmapelement->_variable->_value:NULL)
																		,(_secondArgumentmapelement!=NULL?_secondArgumentmapelement->_variable->_value:NULL)
																		,(_thirdArgumentmapelement!=NULL?_thirdArgumentmapelement->_variable->_value:NULL)
																		,(_fourthArgumentmapelement!=NULL?_fourthArgumentmapelement->_variable->_value:NULL)
																		,(_fifthArgumentmapelement!=NULL?_fifthArgumentmapelement->_variable->_value:NULL));
			}
	}
	return NULL;
}

// MDH@19JUL2019: in order to be able to obtain the body code of functions we're keeping a stack of function names of which the body is requested
// requests can come out of a single command containing multiple function definitions
// _firstFunctionBodyRequest represents the first one to execute
/**
 * @brief returns a new function body request of the function with name \p functionName
 * 
 * @param functionName 
 * @return FunctionBodyRequest* a new function body request of the function with name \p functionName
 */
FunctionBodyRequest* __functionbodyrequest(char const * const functionName){Mallocationowner owner=getOwner(__LINE__);
	FunctionBodyRequest* _functionBodyRequest=NULL;
	if(functionName!=NULL&&strlen(functionName)>0){
		_functionBodyRequest=CALLOC_1(sizeof(FunctionBodyRequest),'9',owner);
		if(_functionBodyRequest!=NULL){
			_functionBodyRequest->_functionName=owned_chars(_getChars(functionName),Msubowner(owner,1));
			if(NULL==_functionBodyRequest->_functionName){
				FREE_DISOWNED_1(_functionBodyRequest,'9',owner);_functionBodyRequest=NULL;
			}
		}
	}
	_functionBodyRequest=DISOWNED(_functionBodyRequest,owner);
	if(!Misdisowned(_functionBodyRequest))output("ERROR: Function body request not disowned!");
	if(Misowned(_functionBodyRequest))output("\nERROR: Function body request still owned!");
	return _functionBodyRequest;
}
/**
 * @brief frees function body request \p _functionBodyRequest
 * 
 * @param _functionBodyRequest 
 * @param owner_functionBodyRequest 
 */
void free_functionbodyrequest(FunctionBodyRequest* _functionBodyRequest,Mallocationowner owner_functionBodyRequest){
	if(!_functionBodyRequest)return;
	FREECHARS(_functionBodyRequest->_functionName,owner_functionBodyRequest);
	FREE_DISOWNED_1(_functionBodyRequest,'9',owner_functionBodyRequest);
}
// active 'list' of function body requests
/**
 * @brief the global pointer to the first and last function body request respectively
 * 
 */
static FunctionBodyRequest *_firstFunctionBodyRequest=NULL,*_lastFunctionBodyRequest=NULL;
Mallocationowner owner_functionBodyRequest=(Mallocationowner){MI_SHELL,__LINE__,1};
/**
 * @brief returns the registered function body request with name \p functionName
 * 
 * @param functionName 
 * @return FunctionBodyRequest* the registered function body request with name \p functionName
 */
static FunctionBodyRequest* getFunctionBodyRequest(char const * const functionName){
	FunctionBodyRequest* functionBodyRequest=_firstFunctionBodyRequest;
	while(functionBodyRequest!=NULL&&strcmp(functionName,functionBodyRequest->_functionName->chars))functionBodyRequest=functionBodyRequest->_next;
	return functionBodyRequest;
}
/**
 * @brief returns the global first function body request
 * 
 * @return FunctionBodyRequest* the global first function body request
 */
FunctionBodyRequest* getFirstFunctionBodyRequest(){return _firstFunctionBodyRequest;}
// MDH@02MAR2020 NOTE: there's no need to return the new function body request instance as it is not used
/**
 * @brief registers a new function body request with name \p functionName
 * 
 * @param functionName 
 * @return FunctionBodyRequest* a new function body request with name \p functionName
 */
static FunctionBodyRequest* registerFunctionBodyRequest(char* functionName){
	if(NULL==functionName||!strlen(functionName)){outputError("Invalid or missing function name.");return NULL;} // invalid input
	// ASSERT a 'valid' function name
	if(getFunctionBodyRequest(functionName)!=NULL){output("%sDuplicate function name '%s'.",M_ERROR_PREFIX,functionName);return NULL;} // already have it
	// technically it should not have been requested already (or exist)
	FunctionBodyRequest* _functionBodyRequest=__functionbodyrequest(functionName); // guarantees that functionName is defined
	if(_functionBodyRequest!=NULL){ 
		// check for being disowned (originally we OWNED it immediately in creating it, but we got a bug saying it was not disowned!!!)
		if(!Misdisowned(_functionBodyRequest))outputBug("Function body request not currently disowned!");
		if(_lastFunctionBodyRequest!=NULL)_lastFunctionBodyRequest->_next=_functionBodyRequest;
		_lastFunctionBodyRequest=OWNED(_functionBodyRequest,owner_functionBodyRequest); // replace _lastFunctionBodyRequest taking over the ownership
		if(NULL==_firstFunctionBodyRequest)_firstFunctionBodyRequest=_lastFunctionBodyRequest;
		if(amVerbose())
			output("The request for the body of function '%s' was created.\n",functionName);
	}else
		output("%sFailed to create the request for the body of function '%s'.\n",M_ERROR_PREFIX,functionName);
	return _functionBodyRequest;
}
/**
 * @brief the global pointers to the first and ladt function body input
 * 
 */
static FunctionBodyInput *_functionBodyInputStack=NULL,*_currentFunctionBodyInput=NULL;static Mallocationowner owner_currentFunctionBodyInput=(Mallocationowner){MI_SHELL,__LINE__,1}; // the stack of function bodies being constructed
/**
 * @brief returns the current function body input
 * 
 * @return FunctionBodyInput* the current function body input
 */
FunctionBodyInput* getCurrentFunctionBodyInput(){return _currentFunctionBodyInput;}
/**
 * @brief returns the global function body input owner
 * 
 * @return Mallocationowner the global function body input owner
 */
Mallocationowner getCurrentFunctionBodyInputOwner(){return owner_currentFunctionBodyInput;}
// MDH@02MAR2020: as we're passing in the function body request I renamed argument _firstFunctionBodyRequest to _functionBodyRequest which makes more sense
/**
 * @brief creates a new function body input of the function body request \p _functionBodyRequest
 * 
 * @param _functionBodyRequest 
 * @return true on success
 * @return false on failure
 */
bool createFunctionBodyInput(FunctionBodyRequest const * const _functionBodyRequest){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT don't call with _firstFunctionBodyRequest equal to NULL
	///////////if(!_firstFunctionBodyRequest)return false;
	// MDH@21OCT2020 BUG FIX: somehow OWNED did't work is that because CALLOC_1 doesn't return a disowned thingie (yes I guess so)????? because it's a module variable it's better to immediately own it correctly
	_currentFunctionBodyInput=CALLOC_1(sizeof(FunctionBodyInput),'8',owner_currentFunctionBodyInput); // MDH@04JUN2020: given that _currentFunctionBodyInput is global it needs to be owned by a module global owner
	// replacing: _currentFunctionBodyInput=OWNED(CALLOC_1(sizeof(FunctionBodyInput),'8',owner),owner_currentFunctionBodyInput); // MDH@04JUN2020: given that _currentFunctionBodyInput is global it needs to be owned by a module global owner
	
	if(NULL==_currentFunctionBodyInput){outputError("Failed to create function body input");return false;} // TODO improve feedback
	Mfunction* function=getFunction(getExecutionEnvironment(),_functionBodyRequest->_functionName->chars);
	if(function!=NULL&&function->type==FT_USER){
		// it's better to put the next request in, so after finishing with this request we can do the following if any
		_currentFunctionBodyInput->_request=_functionBodyRequest->_next; // remember the request that initiated this body input
		_currentFunctionBodyInput->_function=function->functionunion._userfunction;
		if(NULL==_functionBodyInputStack){_functionBodyInputStack=_currentFunctionBodyInput;if(amVerbose())output("%s\n.","Function body input stack created.");}
		if(amVerbose()){output("Parameter map of new function '%s'",_functionBodyRequest->_functionName);outputMap(": ",function->_parameterMap,".\n");}
		// if we succeed in activating the execution environment of the new function we're good to go
		// we can use the functions parameterMap as argumentMap (providing the defaults to use for executing the newly entered body commands)
		// MDH@02MAR2020: _getFunctionExecutionEnvironment() will ALSO duplicate _functionName, so that we can safely release _firstFunctionBodyRequest!!!
		// MDH@03MAR2020 TODO can we pass function->_parameterMap like this or should we pass _getFunctionArgumentMap(function,NULL)????????
		Menvironment* _functionExecutionEnvironment=owned_environment(_getFunctionExecutionEnvironment(function,_functionBodyRequest->_functionName->chars,function->_parameterMap),owner);
		if(_functionExecutionEnvironment!=NULL){
			if(amVerbose())output("Execution environment of function '%s' created.\n",_functionBodyRequest->_functionName);
			if(pushExecutionEnvironment(disowned_environment(_functionExecutionEnvironment,owner)))return true;
			outputError("Failed to register the function execution environment.");
			FREE_ENVIRONMENT(_functionExecutionEnvironment,owner);
		}else
			outputError("Failed to create function execution environment for accepting its body commands"); // TODO improve feedback
	}else
		output("%sCan't find function '%s' for accepting its body commands.\n",M_ERROR_PREFIX,_functionBodyRequest->_functionName);
	FREE_DISOWNED_1(_currentFunctionBodyInput,'8',owner_currentFunctionBodyInput);
	_currentFunctionBodyInput=NULL; // TODO is this a good idea then?????
	return false;
}
/**
 * @brief starts function body input
 * 
 * @return true on success
 * @return false on failure
 */
bool startFunctionBodyInput(){
	// ASSERT only call with _firstFunctionBodyRequest not NULL
	// move out of the queue into the stack
	// push on top of the functionBodyInputStack
	/////////if(!_firstFunctionBodyRequest)return true; // NO function body request to 'execute'
	bool result=true;
	FunctionBodyRequest* nextFunctionBodyRequest=_firstFunctionBodyRequest->_next; // remember the function body request to do next
	// MDH@02MAR2020 ADJUSTMENT: because _functionName is now a heap copy of the original function name (from the function argument list to 'function') we need to free it BEFORE returning the result
	//						   now if we remember the pointer to it, we can release the request and STILL be able to release functionName afterwards!!!
	bool functionBodyInputCreated=createFunctionBodyInput(_firstFunctionBodyRequest);
	if(!functionBodyInputCreated)output("%sFailed to honour the request to input the body of function '%s'.\n",M_ERROR_PREFIX,_firstFunctionBodyRequest->_functionName);
	free_functionbodyrequest(_firstFunctionBodyRequest,owner_functionBodyRequest);_firstFunctionBodyRequest=NULL; // always free the function body request (if we succeed to request the function body input or not)
	if(!functionBodyInputCreated){ // i.e. failed to start requesting for the body of the given function, so we should continue with the next one
		// failed, so do the next one
		 // TODO why is this here???????
		// if we haven't got a next function body request, or we failed to start one, the result will be false
		_firstFunctionBodyRequest=nextFunctionBodyRequest; // simply skip this request!!
		if(!_firstFunctionBodyRequest||!startFunctionBodyInput())result=false;
	}
	// MDH@02MAR2020: forgot to do the following so here we go
	return result;
}
/**
 * @brief ends function body input
 * 
 * @return true on success
 * @return false on failure
 */
bool endFunctionBodyInput(){
	// ASSERT do NOT call with _currentFunctionBodyInput equal to NULL
	// pop the function body request execution environment we just ended
	// MDH@20JUL2019: I need to get a reference to the execution environments function map (before the execution environment get's freed and we loose the reference!!)
	_currentFunctionBodyInput->_function->_functionMap=getExecutionEnvironment()->_functionMap;
	if(amVerboseDebugging())outputExecutionEnvironmentName("End of the body of '","'.\n");
	popExecutionEnvironment();
	// the new first function body request is the successor of the previous one
	// TODO shouldn't we free it?
	_firstFunctionBodyRequest=_currentFunctionBodyInput->_request; // the next function body request as stored in the _request field
	output("Freeing the current function body input...");
	FREE_DISOWNED_1(_currentFunctionBodyInput,'8',owner_currentFunctionBodyInput); // replacing: free(_currentFunctionBodyInput);
	_currentFunctionBodyInput=NULL; // I suppose I should get rid of the current function body input in case we're done anyway
	output(" done!\n");
	if(NULL==_firstFunctionBodyRequest){_lastFunctionBodyRequest=NULL;return true;} // done with all the requests
	return startFunctionBodyInput(); // will NULL _firstFunctionBodyInput to ascertain not to get called in the main user input loop
}
// MDH@19JUL2019 END

/**
 * MDH@Jacky=65yrs:
 * getValueOfExpression() returns the value of the tokens behind _offsetToken together with the token that ends the expression in an Mexpressionvalue*
 * the general idea is to make it recursive so it delegates getting specific subvalues from getValueOfExpression()
 * let's analyze evaluating an expression:
 * an expression 'evaluates' to a value means that we have to apply functions (or unary operators) to arguments, and binary operators to arguments as well
 * this value can also be a composite value like a list or a map, nevertheless a list or a map is a single value
 * it makes sense to delegate getting a list or a map literal to another function
 * NOTE getValueOfExpression() knows nothing about the type of expression it is processing, so it has to check whether to delegate or not
 *	  however the general structure would be: <value><binary operator><value> or perhaps <value><ternary operator><value> but the idea is the same
 *	  we could store these parts in elements of a list, where operator is stored as string and value as Mvalue*, so technically simply a list of Mvalue's so an Mlist*
 *	  we can call these operands and operators or perhaps expressionelements??????
 * 			at the end the operators would need to be removed from the expressionelements array and we'd end up with a single value as result...
 * 		  if the resulting value is an variable, we should return the value of the variable, technically this means that an expressionelement cannot be a variable that makes sense
 *	  so I guess we should only accept assignments at the start of an expression (which makes perfect sense)
 */

/**
 * @brief returns the new M value reference of M value \p _value
 * @param _value the Mvalue to wrap
 * @return the created M value reference
 */
Mvaluereference* _getValuereference(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(amVerboseDebugging())outputValue("Wrapping value '",_value,"'.\n");
	Mvaluereference* _valuereference=(Mvaluereference*)CALLOC_1(sizeof(Mvaluereference),'5',owner);
	if(_valuereference!=NULL){
		_valuereference->_value=_value; // MDH@02NOV2019 replacing: assignValue(&_valuereference->_value,_value);
		if(amVerboseDebugging())outputValue("Value '",_value,"' wrapped in value reference.\n");
	}
	// MDH@18MAY2020: whatever you return should be disowned before passing along (and BOUND by the receiver)
	return disowned_valuereference(_valuereference,owner);
}
/* MDH@26OCT2019: moved over to Mvalue.h/c as we need it there so we can have value references as well!!!!!
void free_valuereference(Mvaluereference* _valuereference){
	if(_valuereference){
		if(_valuereference->_name)free(_valuereference->_name);
		// values themselves are never freed!!!
		if(_valuereference->_value)decrementReferenceCount(_valuereference->_value);
		if(_valuereference->_itemid)decrementReferenceCount(_valuereference->_itemid);
		free(_valuereference);
	}
}
*/

/**
 * @brief outputs M value reference \p _valuereference with prefix \p prefix and suffix \p suffix
 * 
 * @param prefix 
 * @param _valuereference 
 * @param suffix 
 */
void outputValuereference(char* prefix,Mvaluereference* _valuereference,char* suffix){
	if(prefix)output("%s",prefix);
	if(_valuereference){
		if(_valuereference->_name)output("%s",_valuereference->_name);
		if(_valuereference->_itemid)outputValue(NULL,_valuereference->_itemid,NULL);
		if(_valuereference->_value)outputValue("='",_valuereference->_value,"'");
	}
	if(suffix)output("%s",suffix);
}
// two essential methods for getting and setting referenced values
// MDH@14NOV2019: itemid can be a multiple index/attribute name list, and I have to make it work
// MDH@19OCT2020: I thought I had it in here somewhere that if the item id list contains a single element that the result would also be a single value instead of a list
/**
 * @brief returns the M value referenced by M value reference \p _valuereference
 * 
 * @param _valuereference 
 * @return Mvalue* the M value referenced by M value reference \p _valuereference
 */
Mvalue* getReferencedValue(Mvaluereference* _valuereference){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	// _itemid now represents the entire list of index/attribute name combinations
	Mvalue* referencedValue=NULL; // starting out with the actual value in the reference
	// replacing:	outputValuereference("ZZZZZZZZZZ Requesting the value of value reference '",_valuereference,"'.\n");
	if(_valuereference!=NULL){
		if(report)
		{
				output("Getting the value reference of '%s",_valuereference->_name);
				if(_valuereference->_itemid)outputValue(NULL,_valuereference->_itemid,NULL);
				output("'.\n");
		}
		referencedValue=_valuereference->_value; // if we do not have a name and and item id that's what we will return
		// MDH@02NOV2019 replacing: assignValue(&referencedValue,_valuereference->_value); // TODO must we use assignValue here??????????
		// MDH@14NOV2019: if no referened value is available we should use the name to obtain the value of the top level referenced value
		if(NULL==referencedValue){ // no actual referenced value stored (BUT that could actually be the value to return)
			// if we do NOT have a name it's a literal
			if(_valuereference->_name!=NULL&&strlen(_valuereference->_name->chars)>0){
				// MDH@04NOV2019: now that we've added the TT_REFERENCE token, the name may start with @ to indicate a variable reference
				if(_valuereference->_name->chars[0]==M_DEREFERENCE_CHARACTER){ // a reference to a variable which we need to leave as is i.e. wrap it inside a value
					// I suppose we need to wrap a copy unless we make a separate reference thing where we store the name of the variable which could just be an Mstring?????
					// MDH@11MAR2020: let's distinguish between an unnamed ref (with no variable name defined), and a named ref (where the variable SHOULD exist)
					Mvariable* variable=getVariable(getExecutionEnvironment(),&_valuereference->_name->chars[1],false);
					if(variable!=NULL||strlen(_valuereference->_name->chars)==1){
						referencedValue=_getValueOfReference(_getReference(variable));
						// MDH@11MAR2020: if such a variable could not be found we got a segmentation fault which should be prevented obviously, in which case we should still set the reference pointing to a NULL as variable
						//				so the variable is still recognized as reference variable ALTHOUGH it will not be assignable that way which is a nuisance
					}else
						output("%sReferenced variable '%s' does not exist.\n",M_ERROR_PREFIX,_valuereference->_name->chars+1);
				}else // a non-referenced variable which means we are supposed to return the value of the variable
					// if there is no itemid we simply return the 'entire' value of the given variable
					referencedValue=getValue(getExecutionEnvironment(),_valuereference->_name->chars); // the value at the top level
			}
		}
		// only composite values can be indexed... // MDH@23NOV2020: now including VT_ARRAY things as well
		if(referencedValue!=NULL&&(referencedValue->type==VT_ARRAY||referencedValue->type==VT_LIST||referencedValue->type==VT_MAP)){
			if(report)
				outputValue("Top level value reference: '",referencedValue,"'.\n");
			// MDH@14NOV2019: ANY value that evaluates to a list or map can be further indexed
			// if we have index/attribute names we have to get the final subvalue
			// MDH@07APR2020: TODO the following is copied over from setReferencedValue, so obviously it's possible to combine the two in a single function in the future
			Mlist* itemidList=(_valuereference->_itemid&&_valuereference->_itemid->type==VT_LIST?_valuereference->_itemid->value._list:NULL); // let's assume that it is always a list
			if(itemidList!=NULL&&NULL==itemidList->_first)itemidList=NULL;
			// empty lists should also return the full element, so only something to do when we actually have list elements!!!
			// MDH@07APR2020: should be similar to what setReferencedValue does except for the part of setting the value!!!
			if(itemidList!=NULL){
				if(report)
					outputList("Item id list: ",itemidList,"'.\n"); // DEBUG
				Mvalue* *valueholder=&referencedValue; // MDH@07APR2020 replacing what we used in setReferencedValue(): getValueHolder(getExecutionEnvironment(),_valuereference->_name);
				// MDH@26MAR2020 replacing: Mvalue* _value=getValue(getExecutionEnvironment(),_valuereference->_name); // we'll be needing the value at the top level to start with!!!!
				if(valueholder!=NULL&&(isValueUndefined(*valueholder)!=M_FALSE||((*valueholder)->type==VT_ARRAY||(*valueholder)->type==VT_LIST||(*valueholder)->type==VT_MAP))){
					// if(amVerboseDebugging())outputInfo("************ Element(s) to set.");
					// let's get the first index/attribute name
					Mlistelement* indexorattributenameListelement=itemidList->_first; // MDH@19JUN2020 not anymore: MDH@31MAR2020: we know there is a _first (see the creation of _itemidList above)
					// MDH@18OCT2019: we now allow a list that is empty (indicative of appending to the list), in that case indexorattributenameListelement would be NULL
					//				this works for lists not for maps
					// if(/*MDH@31MAR2020 not needed anymore: indexorattributenameListelement||*/isValueUndefined(*valueholder)!=M_FALSE||(*valueholder)->type==VT_LIST){ // we've got one, so not an empty index/attribute name list!!
					Mvalue*** _valueholders=MALLOC_1(sizeof(void*),-'_',owner); // set immediately so MALLOC suffices
					if(_valueholders!=NULL){
						// output("Value holder: %p.",_valueholders); // DEBUG
						size_t numberOfValueholders=1,numberOfNewValueholders=0; // if allocating memory for a single Mvalue** succeeds we have a go
						bool result=true;
						_valueholders[0]=valueholder; // put the root value holder in the first element of the valueholders array
						// we need to find the last index or attribute name
						Mvalue* indexorattributenameListelementValue=NULL;
						if(indexorattributenameListelement!=NULL){ // MDH@18OCT2019: might NOT happen now (on lists that is), so we need to test for that!!!
							// NOTE the last one needs to be assigned to
							while(indexorattributenameListelement!=NULL){
								indexorattributenameListelementValue=indexorattributenameListelement->_value;
								// if no value is defined, it is ignored TODO should we????
								if(indexorattributenameListelementValue!=NULL){
									if(report)
									{outputValue("Type of index value '",indexorattributenameListelementValue,"': ");output("%s.\n",VALUETYPENAMES[indexorattributenameListelementValue->type]);}
									// if no value is currently associated with the referenced variable, we need to create one (either a list or a map depending on the type of the index)
									// NOTE we need to check ALL valueholders
									// for each list element value we're going to need numberOfValueholders elements in newValueholders BUT with nested lists we can't tell in advance how many so we might need to use REALLOC to do so
									// we can start with initializing newValueholders to have at least numberOfValueholders elements
									// MDH@30MAR2020: we need to create newValueholder here because when we have a list as index we get copies of the value holder, so instead of assigning to value holder we assign to new value holder instead
									//				of course this makes it a bit more complicated, of course the alternative is to only duplicate the value holders when we come across a list, which makes perfect sense as well
									//				obviously we can check for a list BEFORE the loop instead of in the loop
									//				we can solve it by flattening the list, which means that we create a queue where we append elements to, so if we come across a list we 
									// MDH@06APR2020: because I want to allow for sublist representing indices to the current values we should NOT flatten the list anymore...
									//				so I have added a flattenLevel int argument, representing the flatten depth, when passing 0 the list values remain intact!!!
									Mlist* _flattenedIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,0,true),owner); // pass in a non-NULL value will only return NULL when an error occurs
									numberOfNewValueholders=(_flattenedIndexList?numberOfValueholders*_flattenedIndexList->numberOfElements:0);
									if(numberOfNewValueholders>0){ // _flattenedList contains all values in the list that are not lists anymore (MDH@06APR2020: now they can), so each of them will result in a single element to append
										// we can reuse valueholders iff we go backwards to the list but that's going to be hard unless we also filled the flattened list in reverse order
										if(report)
											outputList("Flattened (reversed) index list: ",_flattenedIndexList,".\n");
										// which we now did
										Mvalue*** _newValueholders=_valueholders;
										if(numberOfNewValueholders>numberOfValueholders)_newValueholders=REALLOC(_valueholders,numberOfValueholders,numberOfNewValueholders,sizeof(void*),-'_');
										if(_newValueholders!=NULL){ // REALLOC succeeded (or a single element to assign)
											// output("Number of new getReferencedValue() value holders: %zd.\n",numberOfNewValueholders); // DEBUG
											_valueholders=_newValueholders;
											// we can now consume numberOfNewValueholders by decrementing them by numberOfValueholders each time we iterate over the current value holders
											// MDH@06APR2020: it is very hard to determine what the end result should now be because an 'flattened' list element could now be a list itself, so it is hard to determine what is to be indexed...
											//				BUT it is still possible by looking at the first element in sublists...
											// let's check if all index list elements are integer, if not, convert integers to their text equivalent
											Mlistelement* flattenedIndexListelement=_flattenedIndexList->_first;
											Mvalue* flattenedIndexListelementValue;
											while(flattenedIndexListelement!=NULL){
												flattenedIndexListelementValue=getFirstScalarValue(flattenedIndexListelement->_value);
												if(flattenedIndexListelementValue->type!=VT_INTEGER&&flattenedIndexListelementValue->type!=VT_BIGINTEGER)break;
												flattenedIndexListelement=flattenedIndexListelement->_next;
											}
											if(!flattenedIndexListelement)_flattenedIndexList->valuetype=VT_INTEGER; // mark the index list as integer
											// knowing the index list (element) type already means that we know what the indexed value should be (a list or a map)
											// now we know whether what we are indexing should be lists or maps we can ascertain that it does
											long valueholderIndex=numberOfValueholders;
											while(--valueholderIndex>=0){
												valueholder=_valueholders[valueholderIndex];
												if(isValueUndefined(*valueholder)!=M_FALSE){
													// if the index is of type integer we should make a list out of it
													// NOTE no need to use assignValue here BECAUSE that would only result in copying the empty list or map again
													// if the index is a list we will be duplicating
													if(_flattenedIndexList->valuetype==VT_INTEGER){ // all integers in the index list
														assignValue(valueholder,_getListValue(VT_UNDEFINED,false,"value holder list creator"));
														if(amVerboseDebugging())output("Element #%zd of value of '%s' initialized to a list.\n",valueholderIndex,_valuereference->_name);
													}else{ // not all integers in the index list
														assignValue(valueholder,_getMapValue(VT_UNDEFINED,false,"getReferencedValue"));
														if(amVerboseDebugging())output("Element #%zd of value of '%s' initialized to a map.\n",valueholderIndex,_valuereference->_name);
													}
													if(isValueUndefined(*valueholder)!=M_FALSE){_valueholders[valueholderIndex]=NULL;outputError("Failed to create a list or map.");}
												}
												// as soon as the list or map valueholder is created we can use it to get the new value reference IFF the value is of the right type, we have to ascertain that all copies are zero
											}
											// now we can create the elements
											flattenedIndexListelement=_flattenedIndexList->_first;
											while(flattenedIndexListelement!=NULL){
												numberOfNewValueholders-=numberOfValueholders; // now the offset to where to put the new pointer
												indexorattributenameListelementValue=flattenedIndexListelement->_value; // the index value is the flattened list element, reusing indexorattributenameListelementvalue!!!!!!!
												if(indexorattributenameListelementValue!=NULL){
													if(report)
														outputValue("Inspecting whether or not to initialize element with index/property '",indexorattributenameListelementValue,"'.\n");
													int valueholderIndex=numberOfValueholders;
													while(--valueholderIndex>=0){
														valueholder=_valueholders[valueholderIndex];
														if(*valueholder!=NULL){
															// if we are accessing a map we have to ascertain that the attribute name in a string
															if((*valueholder)->type==VT_MAP){
																// MDH@22OCT2020: because the attributes are supposed to be text, we simply convert all indices to text using _getValueText(indexvalue,true)
																//				which means that even integer keys can be used
																// removing: if(_flattenedIndexList->valuetype!=VT_INTEGER){
																	// MDH@06APR2020: if indexorattributenameListelementValue can now also be a list of indices we need to iterate over the list elements and apply each list element as an index
																	//				so newValueholder should be the end result of applying several list elements BUT the idea would be that ALL index elements are map attribute names
																	//				which means that we can only retrieve successive elements from maps
																	//				we could flatten the value here????? so if it is a list we get the list of indices here
																	Mmap* valueholderMap=(*valueholder)->value._map;
																	Mlist* _valueIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,INT_MAX,false),owner); // MDH@06APR2020: if the index is a list we flatten it completely, so each element is a scalar
																	if(report)
																		outputList("Value index list: ",_valueIndexList,".\n");
																	// 'iterating' over all list elements
																	Mlistelement* valueIndexListelement=(_valueIndexList?_valueIndexList->_first:NULL);
																	if(valueIndexListelement!=NULL){
																		Mvalue** newValueholder;
																		while(valueholderMap!=NULL){
																			if(report)
																				outputMap("Value holder map: ",valueholderMap,".");
																			indexorattributenameListelementValue=valueIndexListelement->_value; // if we have a list element use it's value as index
																			if(indexorattributenameListelementValue!=NULL){
																				// MDH@07NOV2022 BUG FIX: _getValueText returns a disowned text, of which I should take ownership immediately
																				Mstring* _attributenameText=owned_string(_getValueText(indexorattributenameListelementValue,true),owner);
																				if(_attributenameText!=NULL){
																					newValueholder=getValueHolderOfAttribute(valueholderMap,string(_attributenameText));		
																					if(NULL==newValueholder){
																						if(appendedToMap(valueholderMap,Msubowner(getValueOwner(),1),string(_attributenameText),NULL)!=1)
																							output("%sFailed to add property '%s'.\n",M_ERROR_PREFIX,string(_attributenameText));
																						else
																							newValueholder=getValueHolderOfAttribute(valueholderMap,string(_attributenameText));
																					}/*else{
																						//FREE_DISOWNED_1(valueholders,'_');
																						_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;
																					}*/
																					FREE_STRING(_attributenameText,owner);
																				}else
																					newValueholder=NULL;
																			}
																			valueIndexListelement=valueIndexListelement->_next;
																			if(NULL==valueIndexListelement)break;
																			// we have another property 'index', so we should have a value holder map
																			valueholderMap=(newValueholder&&*newValueholder&&(*newValueholder)->type==VT_MAP?(*newValueholder)->value._map:NULL);
																		}
																		_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;
																	}
																	if(_valueIndexList!=NULL)FREE_LIST(_valueIndexList,owner);
																/*
																}else
																	_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;*/
																if(NULL==_valueholders[valueholderIndex+numberOfNewValueholders])output("%sFailed to set map element #%zd.\n",M_ERROR_PREFIX,valueholderIndex+numberOfNewValueholders);
															}else
															if((*valueholder)->type==VT_LIST||(*valueholder)->type==VT_ARRAY){
																if(_flattenedIndexList->valuetype==VT_INTEGER){
																	Mlist* valueholderList=((*valueholder)->type==VT_LIST?(*valueholder)->value._list:NULL);
																	Marray* valueholderArray=((*valueholder)->type==VT_ARRAY?(*valueholder)->value._array:NULL);
																	Mlist* _valueIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,INT_MAX,false),owner);
																	// outputList("Value index list: ",_valueIndexList,".\n");
																	// 'iterating' over all list elements
																	Mlistelement* valueIndexListelement=(_valueIndexList?_valueIndexList->_first:NULL);
																	if(valueIndexListelement!=NULL){
																		Mvalue** newValueholder;
																		while(valueholderList!=NULL||valueholderArray!=NULL){
																			// outputList("Value holder list: ",valueholderList,".");
																			indexorattributenameListelementValue=valueIndexListelement->_value; // if we have a list element use it's value as index
																			if(indexorattributenameListelementValue!=NULL){
																				long long listIndex=M_LL_INVALID;
																				if(indexorattributenameListelementValue->type==VT_INTEGER)listIndex=indexorattributenameListelementValue->value._integer->ll;else
																				if(indexorattributenameListelementValue->type==VT_BIGINTEGER)listIndex=biginteger2long(indexorattributenameListelementValue->value._biginteger);
																				// MDH@19JUN2020: I suppose we should also allow appending to the list if listIndex equals M_LL_INVALID
																				if(valueholderList!=NULL){ // indexing a list
																					if(listIndex!=M_LL_INVALID){
																						newValueholder=getValueHolderAtIndex(valueholderList,listIndex);
																						if(NULL==newValueholder){
																							listIndex=appendedToList(valueholderList,owner,NULL,listIndex);
																							if(listIndex>0){
																								newValueholder=getValueHolderAtIndex(valueholderList,listIndex);
																								if(amDebugging())
																									output("List element at index #%zd retrieved.\n",listIndex);	
																							}else
																								output("%sFailed to add list element at index '%lld'.\n",M_ERROR_PREFIX,listIndex);
																						}
																					}else
																						newValueholder=NULL;
																				}else{ // indexing an array
																					newValueholder=(listIndex>0&&listIndex<=valueholderArray->numberOfElements?&(valueholderArray->values[listIndex-1]):NULL);
																					if(report)
																						if(newValueholder!=NULL)
																							output("New value holder reference value #%llu in array.\n",listIndex);
																				}
																			}
																			valueIndexListelement=valueIndexListelement->_next;
																			if(NULL==valueIndexListelement)break;
																			if(NULL==newValueholder||NULL==*newValueholder)break;
																			// we have another property 'index', so we should have a value holder map
																			valueholderList=((*newValueholder)->type==VT_LIST?(*newValueholder)->value._list:NULL);
																			valueholderArray=((*newValueholder)->type==VT_ARRAY?(*newValueholder)->value._array:NULL);
																		}
																		_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;									
																	}
																	if(_valueIndexList!=NULL)FREE_LIST(_valueIndexList,owner);
																}else
																	_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
																// if(!_valueholders[valueholderIndex+numberOfNewValueholders]){output("%s",M_ERROR_PREFIX);outputValue("Assumed index '",indexorattributenameListelementValue,"' not an integer.\n");}
															}else
																_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
															if(NULL==_valueholders[valueholderIndex+numberOfNewValueholders])
																output("%sFailed to set list element #%zd.\n",M_ERROR_PREFIX,valueholderIndex+numberOfNewValueholders);
														}
													}
												}
												flattenedIndexListelement=flattenedIndexListelement->_next;
											}
											if(_flattenedIndexList->numberOfElements>1)numberOfValueholders*=_flattenedIndexList->numberOfElements; // because numberOfNewValueholders was consumed, we have to do it this way
										}else{ // REALLOC failed
											result=false;
											outputError("Failed to reallocate the indexed value references");
										}
										numberOfNewValueholders=0; // MDH@19OCT2020: in any case zero numberOfNewValueholders (to ascertain that FREE below will use numberOfValueholders!!!!)
									}
									if(_flattenedIndexList!=NULL)FREE_LIST(_flattenedIndexList,owner);
								}
								// if all the valueholders are NULL we break????
								int valueholderIndex=numberOfValueholders;while(--valueholderIndex>=0&&_valueholders[valueholderIndex]==NULL)asm("nop");if(valueholderIndex<0){result=false;break;}
								indexorattributenameListelement=indexorattributenameListelement->_next; // immediately increment
							}
						}
						// MDH@31MAR2020: supposedly we have ALL value holders to which _newValue needs to be assigned!!!
						if(result){
							if(report)
								output("Storing the values of %d elements.\n",numberOfValueholders);
							// MDH@19OCT2020: if the result contains a single element we return the first element (which is an Mvalue* and therefore does not need to be owned here)
							if(numberOfValueholders>1){
								// convert the values to a list
								Mlist* _resultList=owned_list(_getListOfType(VT_UNDEFINED),owner);
								int valueholderIndex=numberOfValueholders;
								while(--valueholderIndex>=0){
									if(report)
									{output("Storing value #%d: ",(valueholderIndex+1));outputValue(": ",*_valueholders[valueholderIndex],".\n");}
									if(_resultList!=NULL&&appendedToList(_resultList,owner,*_valueholders[valueholderIndex],0)<=0){
										FREE_LIST(_resultList,owner);
										_resultList=NULL;
										output("%sFailed to store value #%d.",M_ERROR_PREFIX,(valueholderIndex+1));
										break;
									}
								}
								referencedValue=_getValueOfList(disowned_list(_resultList,owner)); // the result
							}else
								referencedValue=*_valueholders[0];
						}else
							outputError("Failed to obtain the list of referenced values");
						// MDH@31MAR2020: essential to free _valueholders (because it was dynamically allocated)
						if(_valueholders!=NULL){
							// output("Freeing %zd value holders.\n",(numberOfNewValueholders>0?numberOfNewValueholders:numberOfValueholders)); // DEBUG
							FREE_DISOWNED(_valueholders,(numberOfNewValueholders>0?numberOfNewValueholders:numberOfValueholders),-'_',owner);
							// output("Value holders getReferencedValues() freed!\n"); // DEBUG
						}
					}
				}
				/* replacing:
				// let's get the first index/attribute name
				Mlistelement* indexorattributenameListelement=itemidList->_first;
				Mvalue* indexorattributenameListelementValue;
				unsigned long long index=0;
				// MDH@14NOV2019: we need a _value as well of type list or map as well, otherwise there's definitely nothing left to index!!!
				while(referencedValue&&indexorattributenameListelement){
					index++;
					indexorattributenameListelementValue=indexorattributenameListelement->_value;
					if(amVerbose()){output("Determining the value at index element #%llu ",index);outputValue(" with value '",indexorattributenameListelementValue,"'.\n");}
					// after extracting the value increment indexorattributenameListelement, so we can use continue
					indexorattributenameListelement=indexorattributenameListelement->_next;
					// if no value is defined, it is ignored TODO should we????
					if(indexorattributenameListelementValue){
						if(amVerbose())outputValue("Index or attribute list element value: '",indexorattributenameListelementValue,"'.\n");
						// if we are accessing a map we have to ascertain that the attribute name in a string
						if(referencedValue->type==VT_MAP){
							Mstring* attributenameText=_getValueText(indexorattributenameListelementValue,true); // TODO should we dequote??
							if(attributenameText){
								referencedValue=getValueOfAttribute(referencedValue->value._map,string(attributenameText));		
								FREE_STRING(attributenameText);
								continue;	
							}
							output("%s",M_ERROR_PREFIX);
							outputValue("Failed to convert assumed attribute name '",indexorattributenameListelementValue,"' to text.\n");		
						}else
						if(referencedValue->type==VT_LIST){
							// MDH@17OCT2019: how about allowing an index to be a list of indices????
							long long index;
							if(indexorattributenameListelementValue->type==VT_LIST){
								// we'll be returning a list value
								Mlist* _referencedValueList=_getListOfType(referencedValue->value._list->valuetype);
								Mlist* indexelementList=indexorattributenameListelementValue->value._list;
								Mlistelement* indexelementListelement=indexelementList->_first;
								Mvalue* valueAtIndex;
								while(indexelementListelement){
									index=getValueInteger(indexelementListelement->_value);
									valueAtIndex=(index!=0&&index!=M_LL_INVALID?getValueAtIndex(referencedValue->value._list,index):NULL);
									// OOPS can't append with 0 anymore, because 0 will do prepending
									if(appendedToList(_referencedValueList,valueAtIndex,M_LL_INVALID)==0)break;
									indexelementListelement=indexelementListelement->_next;
								}
								referencedValue=_getValueOfList(_referencedValueList,true);
							}else{
								// try to convert the index value into a positive integer
								long long index=getValueInteger(indexorattributenameListelementValue);
								if(index!=0&&index!=M_LL_INVALID){
									referencedValue=getValueAtIndex(referencedValue->value._list,index);
									continue;
								}
								if(index){
									output("%s",M_ERROR_PREFIX);
									outputValue("Assumed index '",indexorattributenameListelementValue,"' does not represent an integer.\n");
								}else
									outputError("A zero index is not allowed");
							}
						}else{
							output("%s",M_ERROR_PREFIX);
							outputValue("'",referencedValue,"' cannot be indexed.\n");
							referencedValue=NULL; // prevent further use TODO does this make sense?
							break;
						}
						
						// neither a list nor a map, so nothing to return!!!
						////////////////////////return NULL;

					}
				}
				*/
				/////// see below: if(amVerbose())outputValue("Value of indexed variable: '",_value,"'.\n");
			}
		}
		///////if(amVerbose()){outputValuereference("ZZZZZZZ Value of value reference '",_valuereference,"'");outputValue(": '",referencedValue,"'.\n");}
		if(report)
			outputValue("Returning referenced value: '",referencedValue,"'.\n");
	}
	return referencedValue;
}
// when assigning, we're supposed to assign to something with a variable name (and optional index/attribute name list) associated with it
// MDH@22AUG2023: when a variable is locked one cannot assign to it, or if a composite value is locked an element cannot be set!!!!
/**
 * @brief sets the referenced value object to \p _newValue
 * 
 * @param _valuereference 
 * @param owner_valuereference 
 * @param _newValue 
 * @return true on success
 * @return false on failure
 */
bool setReferencedValue(Mvaluereference * const _valuereference,Mallocationowner owner_valuereference,Mvalue* _newValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	bool result=false;
	if(_valuereference!=NULL&&_valuereference->_name!=NULL){
		if(report)
		{
			output("Setting the value reference of '%s",_valuereference->_name);
			if(_valuereference->_itemid)outputValue(NULL,_valuereference->_itemid,NULL);
			outputValue("' to '",_newValue,"'.\n");
		}
		// MDH@18OCT2019: without an _itemid the variable is allowed to NOT yet exist
		Mlist* itemidList=(_valuereference->_itemid&&_valuereference->_itemid->type==VT_LIST?_valuereference->_itemid->value._list:NULL); // let's assume that is it always a list
		// MDH@19JUN2020 allowing empty index lists again: if(itemidList&&!itemidList->_first)itemidList=NULL; // MDH@31MAR2020: empty lists are ignored (although that's an error theoretically)
		if(itemidList!=NULL){ // the hard part: index/attribute name list assignment!!
			// _valuereference->_value=_newValue; // MDH@07APR2020: TODO do we need this?????
			// MDH@25MAR2020: we can cut the user some slack by allowing automatic initialization to a list or map depending on the whether a property is added or an index
			//				so value needs to be a list or a map or NULL to be indexable unless we allow values to become maps, or making a list
			//				but that's dangerous, so _value&& changed to !_value||
			// MDH@26MAR2020: BUT in order to be able to put a value into the variable we need the address of the value pointer, i.e. the value holder so to speak
			//				i.e. we need a pointer to where the value pointer is stored, could we be using & on the value pointer being returned to get at the holder?????????
			// MDH@28MAR2020: if we allow item index elements to be lists we need an array of value holders
			Mvalue* *valueholder=getValueHolder(getExecutionEnvironment(),_valuereference->_name->chars);
			// MDH@26MAR2020 replacing: Mvalue* _value=getValue(getExecutionEnvironment(),_valuereference->_name); // we'll be needing the value at the top level to start with!!!!
			if(valueholder!=NULL&&(isValueUndefined(*valueholder)!=M_FALSE||((*valueholder)->type==VT_ARRAY||(*valueholder)->type==VT_LIST||(*valueholder)->type==VT_MAP))){
				// MDH@19JUN2020: if itemidList is empty, we use the default index or attribute name
				if(NULL==itemidList->_first)
					if(appendedToList(itemidList,owner_valuereference,(*valueholder)->type==VT_LIST||(*valueholder)->type==VT_ARRAY?_getIntegerValue(M_LL_INVALID):_getTextValue("'"),M_LL_INVALID)<0)
						return false;
				result=true;
				if(report)
					outputInfo("************ Element(s) to set.");
				// let's get the first index/attribute name
				Mlistelement* indexorattributenameListelement=itemidList->_first; // MDH@31MAR2020: we know there is a _first (see the creation of _itemidList above)
				// MDH@18OCT2019: we now allow a list that is empty (indicative of appending to the list), in that case indexorattributenameListelement would be NULL
				//				this works for lists not for maps
				// if(/*MDH@31MAR2020 not needed anymore: indexorattributenameListelement||*/isValueUndefined(*valueholder)!=M_FALSE||(*valueholder)->type==VT_LIST){ // we've got one, so not an empty index/attribute name list!!
				Mvalue*** _valueholders=MALLOC_1(sizeof(void*),-'_',owner); // set immediately so MALLOC suffices // MDH@19JUN2020: a single value holders array
				if(_valueholders!=NULL){
					size_t numberOfValueholders=1,numberOfNewValueholders=0; // if allocating memory for a single Mvalue** succeeds we have a go
					_valueholders[0]=valueholder; // put the root value holder in the first element of the valueholders array
					// we need to find the last index or attribute name
					Mvalue* indexorattributenameListelementValue=NULL;
					if(indexorattributenameListelement!=NULL){ // MDH@18OCT2019: might NOT happen now (on lists that is), so we need to test for that!!!
						// NOTE the last one needs to be assigned to
						while(indexorattributenameListelement!=NULL){
							// if(_valuereference->_itemid)outputValue("Item id: '",_valuereference->_itemid,"'.\n"); // DEBUG
							indexorattributenameListelementValue=indexorattributenameListelement->_value;
							// if no value is defined, it is ignored TODO should we????
							if(indexorattributenameListelementValue!=NULL){
								if(report)
									{outputValue("Type of index value '",indexorattributenameListelementValue,"': ");output("%s.\n",VALUETYPENAMES[indexorattributenameListelementValue->type]);}
								// if no value is currently associated with the referenced variable, we need to create one (either a list or a map depending on the type of the index)
								// NOTE we need to check ALL valueholders
								// for each list element value we're going to need numberOfValueholders elements in newValueholders BUT with nested lists we can't tell in advance how many so we might need to use REALLOC to do so
								// we can start with initializing newValueholders to have at least numberOfValueholders elements
								// MDH@30MAR2020: we need to create newValueholder here because when we have a list as index we get copies of the value holder, so instead of assigning to value holder we assign to new value holder instead
								//				of course this makes it a bit more complicated, of course the alternative is to only duplicate the value holders when we come across a list, which makes perfect sense as well
								//				obviously we can check for a list BEFORE the loop instead of in the loop
								//				we can solve it by flattening the list, which means that we create a queue where we append elements to, so if we come across a list we 
								// MDH@06APR2020: because I want to allow for sublist representing indices to the current values we should NOT flatten the list anymore...
								//				so I have added a flattenLevel int argument, representing the flatten depth, when passing 0 the list values remain intact!!!
								// if(_valuereference->_itemid)outputValue("Item id before flattening the index list: '",_valuereference->_itemid,"'.\n"); // DEBUG
								Mlist* _flattenedIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,0,true),owner); // pass in a non-NULL value will only return NULL when an error occurs
								// if(_valuereference->_itemid)outputValue("Item id after  flattening the index list: '",_valuereference->_itemid,"'.\n"); // DEBUG
								numberOfNewValueholders=(_flattenedIndexList?numberOfValueholders*_flattenedIndexList->numberOfElements:0);
								if(numberOfNewValueholders>0){ // _flattenedList contains all values in the list that are not lists anymore (MDH@06APR2020: now they can), so each of them will result in a single element to append
									// we can reuse valueholders iff we go backwards to the list but that's going to be hard unless we also filled the flattened list in reverse order
									// if(amVerboseDebugging())
									// outputList("Flattened (reversed) index list: ",_flattenedIndexList,".\n"); // DEBUG
									// which we now did
									Mvalue*** _newValueholders=_valueholders;
									if(numberOfNewValueholders>numberOfValueholders)_newValueholders=REALLOC(_valueholders,numberOfValueholders,numberOfNewValueholders,sizeof(void*),-'_');
									if(_newValueholders!=NULL){ // REALLOC succeeded (or a single element to assign)
										// output("Number of new setReferencedValue() value holders: %zd.\n",numberOfNewValueholders); // DEBUG
										_valueholders=_newValueholders;
										// we can now consume numberOfNewValueholders by decrementing them by numberOfValueholders each time we iterate over the current value holders
										// MDH@06APR2020: it is very hard to determine what the end result should now be because an 'flattened' list element could now be a list itself, so it is hard to determine what is to be indexed...
										//				BUT it is still possible by looking at the first element in sublists...
										// let's check if all index list elements are integer, if not, convert integers to their text equivalent
										Mlistelement* flattenedIndexListelement=_flattenedIndexList->_first;
										Mvalue* flattenedIndexListelementValue;
										while(flattenedIndexListelement!=NULL){
											flattenedIndexListelementValue=getFirstScalarValue(flattenedIndexListelement->_value);
											if(flattenedIndexListelementValue->type!=VT_INTEGER&&flattenedIndexListelementValue->type!=VT_BIGINTEGER)break;
											flattenedIndexListelement=flattenedIndexListelement->_next;
										}
										if(NULL==flattenedIndexListelement)_flattenedIndexList->valuetype=VT_INTEGER; // mark the index list as integer
										// knowing the index list (element) type already means that we know what the indexed value should be (a list or a map)
										// now we know whether what we are indexing should be lists or maps we can ascertain that it does
										int valueholderIndex=numberOfValueholders;
										while(--valueholderIndex>=0){
											valueholder=_valueholders[valueholderIndex];
											if(isValueUndefined(*valueholder)!=M_FALSE){
												// if the index is of type integer we should make a list out of it
												// NOTE no need to use assignValue here BECAUSE that would only result in copying the empty list or map again
												// if the index is a list we will be duplicating
												if(_flattenedIndexList->valuetype==VT_INTEGER){ // all integers in the index list
													assignValue(valueholder,_getListValue(VT_UNDEFINED,false,"value holder list creator"));
													if(report)
														output("Element #%zd of value of '%s' initialized to a list.\n",valueholderIndex,_valuereference->_name);
												}else{ // not all integers in the index list
													assignValue(valueholder,_getMapValue(VT_UNDEFINED,false,"value holder list creator"));
													if(report)
														output("Element #%zd of value of '%s' initialized to a map.\n",valueholderIndex,_valuereference->_name);
												}
												if(isValueUndefined(*valueholder)!=M_FALSE){_valueholders[valueholderIndex]=NULL;outputError("Failed to create a list or map.");}
											}
											// as soon as the list or map valueholder is created we can use it to get the new value reference IFF the value is of the right type, we have to ascertain that all copies are zero
										}
										// now we can create the elements
										if(report)
											outputList("****** Flattened index list: '",_flattenedIndexList,"'.\n");
										flattenedIndexListelement=_flattenedIndexList->_first;
										while(flattenedIndexListelement!=NULL){
											// output("%c\n",'A'); // DEBUG
											Mlist* _assignedIndexList=owned_list(__list("assigned indices"),owner); // where we'll be collecting all indices assigned based on this flattened index list element
											numberOfNewValueholders-=numberOfValueholders; // now the offset to where to put the new pointer
											indexorattributenameListelementValue=flattenedIndexListelement->_value; // the index value is the flattened list element, reusing indexorattributenameListelementvalue!!!!!!!
											if(indexorattributenameListelementValue!=NULL){
												if(amVerboseDebugging())
													outputValue("Inspecting whether or not to initialize element with index/property '",indexorattributenameListelementValue,"'.\n");
												int valueholderIndex=numberOfValueholders;
												while(--valueholderIndex>=0){
													valueholder=_valueholders[valueholderIndex];
													/* already handled in the loop in front of this code
													if(isValueUndefined(*_valueholders[valueholderIndex+numberOfNewValueholders])!=M_FALSE){
														// if(amVerbose())
															output("Will initialize '%s' to a composite value.\n",_valuereference->_name);
														// if the index is of type integer we should make a list out of it
														// NOTE no need to use assignValue here BECAUSE that would only result in copying the empty list or map again
														// if the index is a list we will be duplicating 
														if(_flattenedIndexList->valuetype==VT_INTEGER){
															assignValue(_valueholders[valueholderIndex+numberOfNewValueholders],_getListValue(VT_UNDEFINED,false));
															if(amVerbose())output("Element #%zd of value of '%s' initialized to a list.\n",valueholderIndex+numberOfNewValueholders,_valuereference->_name);
														}else{
															assignValue(_valueholders[valueholderIndex+numberOfNewValueholders],_getMapValue(VT_UNDEFINED,false));
															if(amVerbose())output("Element #%zd of value of '%s' initialized to a map.\n",valueholderIndex+numberOfNewValueholders,_valuereference->_name);
														}
														if(isValueUndefined(*_valueholders[valueholderIndex+numberOfNewValueholders])!=M_FALSE){_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;outputError("Failed to create a list or map.");}
													}
													*/
													if((*valueholder)!=NULL){
														// if we are accessing a map we have to ascertain that the attribute name in a string
														if((*valueholder)->type==VT_MAP){
															// MDH@23OCT2020: same here
															// removing: if(_flattenedIndexList->valuetype!=VT_INTEGER){
															// MDH@22AUG2023: the map can be locked????
															Mmap* valueholderMap=(*valueholder)->value._map;
															if(valueholderMap->unlockCode==0){
																// MDH@06APR2020: if indexorattributenameListelementValue can now also be a list of indices we need to iterate over the list elements and apply each list element as an index
																//				so newValueholder should be the end result of applying several list elements BUT the idea would be that ALL index elements are map attribute names
																//				which means that we can only retrieve successive elements from maps
																//				we could flatten the value here????? so if it is a list we get the list of indices here
																Mlist* _valueIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,INT_MAX,false),owner); // MDH@06APR2020: if the index is a list we flatten it completely, so each element is a scalar
																if(report)
																	outputList("Value index list: ",_valueIndexList,".\n"); // DEBUG
																// 'iterating' over all list elements
																Mlistelement* valueIndexListelement=(_valueIndexList!=NULL?_valueIndexList->_first:NULL);
																if(valueIndexListelement!=NULL){
																	Mvalue** newValueholder;
																	while(valueholderMap!=NULL){
																		// output("%c\n",'B'); // DEBUG
																		// outputMap("Value holder map: ",valueholderMap,".\n"); // DEBUG
																		indexorattributenameListelementValue=valueIndexListelement->_value; // if we have a list element use it's value as index
																		if(indexorattributenameListelementValue!=NULL){
																			// MDH@19OCT2020: if we do not unquote the value we can safely remove the final quote????? by decrementing the length...
																			// MDH@22OCT2020: however this will get us into trouble when dealing with values that are not text (e.g. integers), so we switch back to getting the text dequoted
																			Mstring* _attributenameText=owned_string(_getValueText(indexorattributenameListelementValue,true),owner); // MDH@19OCT2020 bug fix: take ownership
																			if(_attributenameText!=NULL){ // we need to free _attributenameText when we're done with it
																				if(string_insert_char(_attributenameText,0,'\'')!=NULL){ // ascertain that _attributenameText starts with a quote character, so we can use _getTextValue on it
																					char* _attributename=string(_attributenameText)+1; // skipping the initial quote
																					if(amVerboseDebugging())
																						output("Attribute name text: '%s'.\n",_attributename); // DEBUG
																					newValueholder=getValueHolderOfAttribute(valueholderMap,_attributename);		
																					if(NULL==newValueholder){
																						if(appendedToMap(valueholderMap,Msubowner(getValueOwner(),1),_attributename,NULL)==M_TRUE){
																							newValueholder=getValueHolderOfAttribute(valueholderMap,_attributename);
																							// register in the assigned index list
																							// MDH@19OCT2020 bug fix: _getTextValue assumes that _attributenameText starts with the text quote character!!
																							if(_assignedIndexList!=NULL&&appendedToList(_assignedIndexList,owner,_getTextValue(string(_attributenameText)),M_LL_INVALID)<=0)
																							{FREE_LIST(_assignedIndexList,owner);_assignedIndexList=NULL;}
																						}else
																							output("%sFailed to add property '%s'.\n",M_ERROR_PREFIX,_attributename);
																					}/*else{
																						//FREE_DISOWNED_1(valueholders,'_');
																						_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;
																					}*/
																				}
																				FREE_STRING(_attributenameText,owner);
																			}else
																				newValueholder=NULL;
																		}
																		valueIndexListelement=valueIndexListelement->_next;
																		if(NULL==valueIndexListelement)break;
																		// we have another property 'index', so we should have a value holder map
																		valueholderMap=(newValueholder&&*newValueholder&&(*newValueholder)->type==VT_MAP?(*newValueholder)->value._map:NULL);
																		// output("%c\n",'C'); // DEBUG
																	}
																	_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;
																}
																if(_valueIndexList!=NULL)FREE_LIST(_valueIndexList,owner);
																if(NULL==_valueholders[valueholderIndex+numberOfNewValueholders])
																	output("%sFailed to set map element #%zd.\n",M_ERROR_PREFIX,valueholderIndex+numberOfNewValueholders);
															}else{ // map is locked, and therefore immutable
																_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
																outputError("Map is locked, and therefore immutable!");
															}
														}else
														if((*valueholder)->type==VT_LIST||(*valueholder)->type==VT_ARRAY){ // MDH@23NOV2020: either a list or an array being indexed
															if(_flattenedIndexList->valuetype==VT_INTEGER){
																// we either have a list or an array (bit of a nuisance to have to do it this way?????)
																Mlist* valueholderList=((*valueholder)->type==VT_LIST?(*valueholder)->value._list:NULL);
																Marray* valueholderArray=((*valueholder)->type==VT_ARRAY?(*valueholder)->value._array:NULL);
																if((valueholderList!=NULL&&valueholderList->unlockCode==0)||(valueholderArray!=NULL&&valueholderArray->unlockCode==0)){
																	Mlist* _valueIndexList=owned_list(_getFlattenedList(indexorattributenameListelementValue,INT_MAX,false),owner);
																	if(report)
																		outputList("Value index list: ",_valueIndexList,".\n"); // DEBUG
																	// 'iterating' over all index list elements
																	Mlistelement* valueIndexListelement=(_valueIndexList?_valueIndexList->_first:NULL);
																	if(valueIndexListelement!=NULL){
																		Mvalue** newValueholder;
																		while(valueholderList!=NULL||valueholderArray!=NULL){
																			// output("%c\n",'D'); // DEBUG
																			// outputList("Value holder list: ",valueholderList,".");
																			indexorattributenameListelementValue=valueIndexListelement->_value; // if we have a list element use it's value as index
																			if(indexorattributenameListelementValue!=NULL){
																				long long listIndex=M_LL_INVALID;
																				if(indexorattributenameListelementValue->type==VT_INTEGER)listIndex=indexorattributenameListelementValue->value._integer->ll;else
																				if(indexorattributenameListelementValue->type==VT_BIGINTEGER)listIndex=biginteger2long(indexorattributenameListelementValue->value._biginteger);
																				// MDH@19JUN2020 M_LL_INVALID allowed as index indicating appending: if(listIndex!=M_LL_INVALID){
																				if(valueholderList!=NULL){ // MDH@23NOV2020: a list
																					newValueholder=(listIndex!=M_LL_INVALID?getValueHolderAtIndex(valueholderList,listIndex):NULL);
																					if(NULL==newValueholder){
																						listIndex=appendedToList(valueholderList,owner,NULL,listIndex);
																						if(listIndex>0){
																							if(amVerboseDebugging())
																							{output("List after appending NULL at index %lld",listIndex);outputList(": '",valueholderList,"'.\n");}
																							newValueholder=getValueHolderAtIndex(valueholderList,listIndex);
																							// if(amVerboseDebugging())output("List element at index #%zd retrieved.\n",listIndex);
																							// register in the assigned index list
																							if(_assignedIndexList!=NULL&&appendedToList(_assignedIndexList,owner,_getIntegerValue(listIndex),M_LL_INVALID)<=0)
																							{FREE_LIST(_assignedIndexList,owner);_assignedIndexList=NULL;}
																						}else
																							output("%sFailed to add list element at index '%lld'.\n",M_ERROR_PREFIX,listIndex);
																					}
																				}else{ // MDH@23NOV2020: an array (and we're NOT going to create an element that's not there like we do with a list!!!!)
																					newValueholder=(listIndex>0&&listIndex<=valueholderArray->numberOfElements?&(valueholderArray->values[listIndex-1]):NULL);
																					if(report)
																						if(newValueholder!=NULL)
																							output("New value holder reference value #%llu in array.\n",listIndex);
																				}
																				//}else	newValueholder=NULL;
																			}
																			valueIndexListelement=valueIndexListelement->_next;
																			if(NULL==valueIndexListelement)break;
																			// we have another property 'index', so we should have a value holder map
																			if(NULL==newValueholder||NULL==(*newValueholder))break;
																			valueholderList=((*newValueholder)->type==VT_LIST?(*newValueholder)->value._list:NULL);
																			valueholderArray=((*newValueholder)->type==VT_ARRAY?(*newValueholder)->value._array:NULL);
																			// output("%c\n",'E'); // DEBUG
																		}
																		// output("Storing value holder #%lld: %p.\n",valueholderIndex+numberOfNewValueholders,newValueholder); // DEBUG
																		_valueholders[valueholderIndex+numberOfNewValueholders]=newValueholder;									
																	}
																	if(_valueIndexList!=NULL)FREE_LIST(_valueIndexList,owner);
																}else{
																	_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
																	if(valueholderList!=NULL)outputError("List is locked, and therefore immutable!");else
																	if(valueholderArray!=NULL)outputError("Array is locked, and therefore immutable!");
																}
															}else
																_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
															// if(!_valueholders[valueholderIndex+numberOfNewValueholders]){output("%s",M_ERROR_PREFIX);outputValue("Assumed index '",indexorattributenameListelementValue,"' not an integer.\n");}
														}else
															_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
														if(NULL==_valueholders[valueholderIndex+numberOfNewValueholders])
															output("%sFailed to set list element #%zd.\n",M_ERROR_PREFIX,valueholderIndex+numberOfNewValueholders);
													}
												}
											}
											// if we managed to collect all assigned indices we use these to replace the current value in the flattened index list element
											if(_assignedIndexList!=NULL){
												if(_assignedIndexList->_first!=NULL){
													if(report)
														outputList("Assigned index list: '",_assignedIndexList,"'.\n"); // DEBUG
													if(_assignedIndexList->_first==_assignedIndexList->_last){
														assignValue(&flattenedIndexListelement->_value,_assignedIndexList->_first->_value);
														FREE_LIST(_assignedIndexList,owner);
													}else
														assignValue(&flattenedIndexListelement->_value,_getValueOfList(disowned_list(_assignedIndexList,owner))); // now bound (or released)
												}else
													FREE_LIST(_assignedIndexList,owner);
											}else
												outputBug("Failed to populate the assigned index list");
											flattenedIndexListelement=flattenedIndexListelement->_next;
											// output("%c\n",'F'); // DEBUG
										}
										if(_flattenedIndexList->numberOfElements>1)numberOfValueholders*=_flattenedIndexList->numberOfElements; // because numberOfNewValueholders was consumed, we have to do it this way
									}else{ // REALLOC failed
										result=false;
										outputError("Failed to reallocate the indexed value references");
									}
									numberOfNewValueholders=0; // MDH@19OCT2020: in both cases (whether realloc failed or not we can zero numberOfNewValueholders)
								}
								// MDH@19JUN2020 TODO not too happy about using _flattenedIndexList and assignedIndexList to collect the actual ids of the properties or array elements
								if(_flattenedIndexList!=NULL){
									if(_flattenedIndexList->_first!=NULL){ // at least one element
										if(report)
											outputList("Flattened index list: '",_flattenedIndexList,"'.\n");
										if(_flattenedIndexList->_first!=_flattenedIndexList->_last){ // more than one element: replace the value by the reversed list (which is disowned to start with!!!!)
											Mlist* _rereversedIndexList=owned_list(_getReversedList(_flattenedIndexList),owner);
											if(_rereversedIndexList!=NULL)
												assignValue(&indexorattributenameListelement->_value,_getValueOfList(disowned_list(_rereversedIndexList,owner)));
											else 
												outputBug("Failed to reverse an index list");
										}else // replace the value by the first value in the flattened index list
											assignValue(&indexorattributenameListelement->_value,_flattenedIndexList->_first->_value);
									}
									FREE_LIST(_flattenedIndexList,owner);
								}
							}
							// if all the valueholders are NULL we break????
							long long valueholderIndex=numberOfValueholders;
							while(--valueholderIndex>=0&&_valueholders[valueholderIndex]==NULL)asm("nop");
							if(valueholderIndex<0){result=false;break;}
							// how about putting the flattenedIndexList back????

							indexorattributenameListelement=indexorattributenameListelement->_next; // immediately increment
						}
					}
					// MDH@31MAR2020: supposedly we have ALL value holders to which _newValue needs to be assigned!!!
					if(result){
						long long valueholderIndex=numberOfValueholders;
						if(report)
						{output("Setting %llu values",valueholderIndex);outputValue(" to '",_newValue,"'.\n");}
						while(--valueholderIndex>=0)if(_valueholders[valueholderIndex]!=NULL)assignValue(_valueholders[valueholderIndex],_newValue);
						// output("Values set!\n"); // DEBUG
					}
					/* MDH@31MAR2020 we've dealt with the last index element as well in the block above, so replacing:
					if(result){
						if(indexorattributenameListelementValue){ // this is the last 'index' which can be a property name or index or list of property names and indices!!!!
							// if(amVerbose())
							{outputValue("Type of the last index value '",indexorattributenameListelementValue,"': ");output("%s.\n",VALUETYPENAMES[indexorattributenameListelementValue->type]);}
							Mlist* _flattenedIndexList=_getFlattenedList(indexorattributenameListelementValue,true);
							size_t numberOfNewValueholders=(_flattenedIndexList?numberOfValueholders*_flattenedIndexList->numberOfElements:0);
							if(numberOfNewValueholders>0){
								Mvalue*** _newValueholders=(numberOfNewValueholders>numberOfValueholders?REALLOC_1(_valueholders,numberOfValueholders,numberOfNewValueholders,sizeof(void*),'_'):_valueholders);
								if(_newValueholders){
									_valueholders=_newValueholders;
									Mlistelement* flattenedIndexListelement=_flattenedIndexList->_first;
									while(flattenedIndexListelement){
										numberOfNewValueholders-=numberOfValueholders; // now the offset to where to put the new pointer
										indexorattributenameListelementValue=flattenedIndexListelement->_value; // the index value is the flattened list element, reusing indexorattributenameListelementvalue!!!!!!!
										if(indexorattributenameListelementValue){
											// if(amVerbose())
												outputValue("Setting element at index/property ",indexorattributenameListelementValue,".\n");
											// iterating over the original value holders to ascertain that they are pointing to either a list or a map
											int valueholderIndex=numberOfValueholders;
											while(--valueholderIndex>=0){
												if(isValueUndefined(*_valueholders[valueholderIndex+numberOfNewValueholders])!=M_FALSE){
													// if(amVerbose())
														output("Will initialize element #%zd of '%s' to a list or a map.\n",valueholderIndex+numberOfNewValueholders,_valuereference->_name);
													// if the index is of type integer we should make a list out of it
													// NOTE no need to use assignValue here BECAUSE that would only result in copying the empty list or map again
													// if the index is a list we will be duplicating 
													if(indexorattributenameListelementValue->type==VT_INTEGER||indexorattributenameListelementValue->type==VT_BIGINTEGER){
														assignValue(_valueholders[valueholderIndex+numberOfNewValueholders],_getListValue(VT_UNDEFINED,false));
														// if(amVerbose())
															output("Value #%zd of '%s' initialized to a list.\n",valueholderIndex+numberOfNewValueholders,_valuereference->_name);
													}else{
														assignValue(_valueholders[valueholderIndex+numberOfNewValueholders],_getMapValue(VT_UNDEFINED,false));
														// if(amVerbose())
															output("Value #%zd of '%s' initialized to a map.\n",valueholderIndex+numberOfNewValueholders,_valuereference->_name);
													}
													if(isValueUndefined(*_valueholders[valueholderIndex+numberOfNewValueholders])!=M_FALSE){
														_valueholders[valueholderIndex+numberOfNewValueholders]=NULL;
														output("%sFailed to create a list or map at index %zd.",M_ERROR_PREFIX,valueholderIndex+numberOfNewValueholders);
														break;
													}
												}
											}
											// MDH@30MAR2020: if a value holder was not set, we do not continue TODO or should we only do this when all are undefined??????
											if(valueholderIndex>=0){result=false;outputError("Failed to create a list or map.");}
										}
										flattenedIndexListelement=flattenedIndexListelement->_next;
									}
									if(_flattenedIndexList->numberOfElements>1)numberOfValueholders*=_flattenedIndexList->numberOfElements;
								}else
									result=false;
							}
							if(_flattenedIndexList)FREE_LIST(_flattenedIndexList);
						}else
							outputError("No index value.");
					}
					if(result){
						// if(amDebugging())
						{outputValue("Assigning ",indexorattributenameListelement->_value," to ");output("%zd elements.\n",numberOfValueholders);}
						// now indexorattributenameListelement should point to the last index/attribute name and _value at the list/map to change
						int valueholderIndex=numberOfValueholders;
						while(--valueholderIndex>=0){
							if((*_valueholders[valueholderIndex])->type==VT_MAP){
								Mstring* _attributeName=_getValueText(indexorattributenameListelement->_value,true);
								if(appendedToMap((*_valueholders[valueholderIndex])->value._map,string(_attributeName),_newValue)!=1)result=false;
								FREE_STRING(_attributeName);
								// if(!result)return false;
							}else
							if((*_valueholders[valueholderIndex])->type==VT_LIST){
								// NOTE allow appending using 0 or inserting with negative values
								// MDH@18OCT2019: we now have four situations: 0=prepend, NULL=append, negative integers=set from the back (-1=last element)
								//				so if no list element is defined, we just append to the list!!!!
								//				the only invalid situations is when the _value is NULL although it still could NOT denote an integer
								long long index=(indexorattributenameListelement?getValueInteger(indexorattributenameListelement->_value):M_LL_INVALID);
								// replace the index to the actual index with the index of the element in the list (so getReferencedValue() will not complain!!!)
								if(index!=M_LL_INVALID){
									index=appendedToList((*_valueholders[valueholderIndex])->value._list,_newValue,index);
									// MDH@18OCT2019: why are we doing this????? i.e. is the value in the list still pointing somewhere??????
									if(index>0){
										if(indexorattributenameListelement)assignValue(&indexorattributenameListelement->_value,_getIntegerValue(index));
									}else
										break; // replacing: result=false
								}else
									break; // replacing: result=false
							}
						}
						if(valueholderIndex>=0)result=false;
					}
					*/
					// MDH@31MAR2020: essential to free _valueholders (because it was dynamically allocated)
					if(_valueholders!=NULL){
						// if(_valuereference->_itemid)outputValue("Item id before freeing the index list: '",_valuereference->_itemid,"'.\n"); // DEBUG
						// output("Freeing %zd value holders.\n",(numberOfNewValueholders>0?numberOfNewValueholders:numberOfValueholders)); // DEBUG
						FREE_DISOWNED(_valueholders,(numberOfNewValueholders>0?numberOfNewValueholders:numberOfValueholders),-'_',owner);
						// output("Value holders freed!\n"); // DEBUG
						// if(_valuereference->_itemid)outputValue("Item id after  freeing the index list: '",_valuereference->_itemid,"'.\n"); // DEBUG
					}
				}
				/*
				}else
				if(isValueUndefined(*valueholder)!=M_FALSE||(*valueholder)->type!=VT_LIST){
					result=false;
					outputError("No index/attribute name specified");
				}
				*/
			}else
				output("%sVariable '%s' cannot be indexed: its value is not a list or a map.\n",M_ERROR_PREFIX,_valuereference->_name);
		}else{
			// MDH@04MAR2020: here we can determine whether the value assigned is a function without a body, in which case we should also ask for the body of this function next
			//				the same way as happens when you use the defun internal function
			// MDH@23DEC2020: BUG FIX we have an issue here in that the given variable should be created if it does not currently exist, which means we should NOT call setValue()
			//						because setValue() does not create the variable!!!!
			//				SOLUTION so similar to what Mset() does we ascertain to add the variable if it does not yet exist, except we always use type VT_UNDEFINED (which is safer!!)
			char* variableName=_valuereference->_name->chars;
			Menvironment* executionEnvironment=getExecutionEnvironment();
			Mvariable* variable=getVariable(executionEnvironment,variableName,false); // if the variable exists, variable will be non-NULL
			//MDH@23AUG2023 NOTE: setValue() checks whether or not the variable is locked, and won't allow changing its value unless NULL TODO should we also lock when NULL??
			if((variable!=NULL||addVariable(executionEnvironment,owner,variableName,VT_UNDEFINED,false))
				&&setValue(executionEnvironment,variableName,_newValue)){
				// NOTE even if the value itself is NULL, its address is never NULL
				_valuereference->_value=_newValue; // MDH@02NOV2019 replacing: assignValue(&_valuereference->_value,_newValue);
				result=true;
				// MDH@04MAR2020: as soon as result is set, we can determine if a function without a body is assigned!!
				if(_newValue!=NULL&&_newValue->type==VT_FUNCTION){
					Mfunction* function=_newValue->value._function;
					if(function->type==FT_USER){
						Muserfunction* userfunction=function->functionunion._userfunction;
						if(userfunction!=NULL&&NULL==userfunction->_bodyCommandList){
							registerFunctionBodyRequest(_valuereference->_name->chars);
						}
					}
				}
				if(report)
					outputInfo("Value set!");
			}else{
				output("%sFailed to %s variable '%s'",M_ERROR_PREFIX,(variable?"set":"initialize"),variableName);outputValue(" to '",_newValue,"'.\n");
			}
		}
		// MDH@20JUL2019: here when we succeed in performing the assigment, we should update the value reference as well!!!!
	}
	// if(_valuereference->_itemid)outputValue("Item id: '",_valuereference->_itemid,"'.\n"); // DEBUG
	return result;
}

/**
 * @brief returns the result of applying unary operator \p operator to \p _value
 * 
 * @param _value 
 * @return Mvalue* the result of applying unary operator \p operator to \p _value
 */
Mvalue* applyUnaryOperator(char operator,Mvalue* _value){
	if(amVerboseDebugging())
	{
		output("Applying unary operator '%c'",operator);
		if(_value!=NULL){outputValue(" to value '",_value,"'");output(" of type %u.\n",_value->type);}else output(".\n");
	}
	// delegating to the one argument functions that we have is best!!!
	Mvalue* result=NULL;
	switch(operator){
		case '~':result=Mbnot(_value);break;
		case '!':result=Mnot(_value);break;
		case '-':result=Mneg(_value);break;
		case '+':result=_value;break;
		default:outputError("Unknown unary operator!");
	}
	/////if(!result)return NULL;
	if(amVerboseDebugging())
	{if(result!=NULL){outputValue(" Result of applying the unary operator (",result,")");output(" of type %u.\n",result->type);}else output(".\n");}
	return result;
}
/**
 * @brief returns whether or not token type \p tokenType is a one character token type or not
 * 
 * @param tokenType 
 * @return true when \p tokenType is a one character token type
 * @return false when \p tokenType is not a one character token type
 */
bool isOneCharacterTokenType(uint8_t tokenType){
	// TODO how about TT_EXPRESSION -> NO because a TT_EXPRESSION token is always considered ended, i.e. significantCharacterCount is not an issue in determining whether a new token starts there
	return(tokenType==TT_ASSIGNMENT||tokenType==TT_UNARY||tokenType==TT_TERNARY_aeru||tokenType==TT_LIST||tokenType==TT_LISTELEMENT||tokenType==TT_END_OF_LIST||tokenType==TT_MAP||tokenType==TT_END_OF_MAP||tokenType==TT_FUNCTION_CALL||tokenType==TT_END_OF_FUNCTION_CALL||tokenType==TT_END_OF_DQSTRING||tokenType==TT_END_OF_SQSTRING);
}

// MDH@01MAR2021: TODO check if this one should be here or in M.c
/**
 * @brief outputs token \p _token returning the number of characters to output
 * 
 * @param _token 
 * @return size_t the number of token characters to output
 */
static size_t outputToken(Mtoken const * const _token){
	size_t numberOfCharactersToOutput=(_token!=NULL&&_token->text!=NULL?string_length(_token->text):0);
	if(numberOfCharactersToOutput>0){
		// MDH@31OCT2019: by introducing ` as new line request character (whitespace) we'll be having visible whitespace characters at the end of the token which we do not want to show in the same color
		// ascertain that the token text ends at the first whitespace character (if there is any whitespace) NOTE there's no need to put '\0' back, therefore we use '\0' if we didn't replace the character to start with
		char firstWhitespaceCharacter=(isTokenFinished(_token)?string_replacedchar(_token->text,'\0',getTokenSignificantCharacterCount(_token)):'\0');
		// if we allow comments in tokens we're in trouble!!!
		output("%s",string(_token->text)); // although string() will write the '\0' at the end we've already written one in front of that position
		// if there's whitespace text to start with write it in the default output color
		if(firstWhitespaceCharacter){ // some whitespace left to write
			string_setchar(_token->text,firstWhitespaceCharacter,getTokenSignificantCharacterCount(_token));
			output("%s",string_remainder(_token->text,getTokenSignificantCharacterCount(_token)));
		}
	}
	return numberOfCharactersToOutput;
	/////////if(amAssisting()){resetOutputColor();outputChar('|');}
}
/**
 * @brief the global function to output a token
 * 
 */
static OutputTokenFunction* outputTokenFunction=NULL;
/**
 * @brief outputs the last character of token \p _token
 * 
 * @param _token 
 */
void outputLastTokenChar(Mtoken* _token){
	///////outputTokenColor(_userInputCommand->_lastToken);
	outputChar(string_last_char(_token->text));
	//////////resetOutputColor();
}

/**
 * getValueReference() retrieves a single value reference that either ends when a binary operator token is encountered or one of the end token types
 * a value reference syntax: optionally a number of unary operators, optionally followed by function call with arguments, and variable or value literal
 * we need to store the value in a value reference just in case the value is the destination of an assignment, so yes, reference is an apt name
*/
// MDH@17NOV2019: applying unary operator on an indexed value not working as it should, so has to be fixed
/**
 * @brief returns the next value reference from the current expression being evaluated
 * 
 * @param info 
 * @param endTokenTypes 
 * @param endTokenTypeCount 
 * @return Mvaluereference* the next value reference from the current expression being evaluated
 */
Mvaluereference* _getValueReference(char* info,TokenType endTokenTypes[],uint8_t endTokenTypeCount){Mallocationowner owner=getOwner(__LINE__);

	Mtoken* expressionToken=getEnvironmentExpressionToken();

	Mvaluereference* _valueReference=NULL;

	if(amVerboseDebugging())
		output("_getValueReference() extracting a(n) '%s' value that starts with token '%s' of type '%s'.\n",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);

	Mstring* unaryOperators=NULL; // a value starts with a number (zero or more) of unary operators
		
	while(expressionToken!=NULL&&expressionToken->type==TT_UNARY){
		char unaryOperatorChar=string_char(expressionToken->text,0);
		if(unaryOperatorChar!='+'){
			if(!unaryOperators)unaryOperators=__string();
			string_append_char(unaryOperators,unaryOperatorChar);
		}
		expressionToken=nextEnvironmentExpressionToken();
	}
	if(amVerboseDebugging())
		{if(unaryOperators)output("Unary operators: '%s'.\n",string(unaryOperators));else output("No unary operators!\n");}
	// ASSERT unary operators extracted

	if(expressionToken!=NULL){
		if(amVerboseDebugging())
			output("_getValueReference() interpreting first value token '%s' of type %s.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
		_valueReference=(Mvaluereference*)CALLOC_1(sizeof(Mvaluereference),'5',owner);
		// expecting either a function (call), (new) variable or (integer, real, string, list or map) literal
		/* NO we can NOT change the tokens themselves (to keep them editable!!!)
		if(expressionToken->type==VT_INTEGER){
			if(expressionToken->next&&expressionToken->next->type==VT_FLOAT){
				expressionToken=expressionToken->next;
				// let's prepend the integer token text to the real (fraction) token text
				string_prepend(string(expressionToken->prev->text),expressionToken->text);
			}
		}
		*/
		// MDH@17NOV2019: some value references can be indexed BEFORE applying the unary operators
		//				the index can be determined following determining what is being indexex
		//				alternatively: we can determine the initial value reference and check afterwards
		//				we can start with making the theoretic indexing possibility
		bool canbeindexedtheoretically=false; // this should have the same result as the tokenizer does
		char* _significantTokenText=_getSignificantTokenCharacters(expressionToken);
		switch(expressionToken->type){
			case TT_FUNCTION:
				{
					Mfunction* function=getFunction(getExecutionEnvironment(),_significantTokenText); // get the function associated with the name of the function
					if(function!=NULL){
						canbeindexedtheoretically=true; // MDH@17NOV2019: stick to what the tokenizer allow TODO exclude special functions
						// MDH@17JUL2019: we know the function and when the name is one of the special functions
						//				like 'function' to define a function we know not to evaluate the third argument!!
						//				it's easiest to define first element not to evaluate (i.e. to store the tokens in the list)
						// MDH@25JUL2019: adding if, while and for functions
						unsigned long long numberOfFunctionParameters=(function->_parameterMap!=NULL?function->_parameterMap->numberOfElements:0),numberOfElementsToNotEvaluate=0;
						/* MDH@28OCT2020: defining user functions no longer 'special' function (calls)
						if(!strcmp(_significantTokenText,DEFINEANONYMOUSFUNCTION_NAME)){
							if(amVerboseDebugging())
								outputInfo("Definition of an anonymous function encountered!");
							numberOfElementsToNotEvaluate=numberOfFunctionParameters-1; // i.e. evaluate the first argument only in the current context
						}else
						if(!strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){
							if(amVerboseDebugging())
								outputInfo("Definition of a user function encountered!");
							numberOfElementsToNotEvaluate=numberOfFunctionParameters-2;	// i.e. evaluate the first two arguments only in the current context
						}else
						*/
						// MDH@23DEC2020: the 'new' for function works the same way as while in that all arguments should not be evaluated beforehand (i.e. will be passed as tokens to Mforfunnction)
						if(!strcmp(_significantTokenText,FORFUNCTION_NAME)){
							// MDH@21DEC2020: if we decide that all arguments are not to be evaluated
							//				we can have as many as we want!!
							numberOfElementsToNotEvaluate=LLONG_MAX; // all elements should NOT be evaluated
							// the do function is special in that it allows an infinite number of arguments although the function itself expects them wrapped in a single Mvalue
							numberOfFunctionParameters=LLONG_MAX; // replacing 2 with the actual number of parameters we allow for the function
							// replacing: numberOfElementsToNotEvaluate=2;
						}else
						if(!strcmp(_significantTokenText,WHILEFUNCTION_NAME)){
							// MDH@21DEC2020: if we decide that all arguments are not to be evaluated
							//				we can have as many as we want!!
							numberOfElementsToNotEvaluate=LLONG_MAX; // all elements should NOT be evaluated
							// the do function is special in that it allows an infinite number of arguments although the function itself expects them wrapped in a single Mvalue
							numberOfFunctionParameters=LLONG_MAX; // replacing 2 with the actual number of parameters we allow for the function
							// replacing: numberOfElementsToNotEvaluate=2;
						}else
						if(!strcmp(_significantTokenText,IFFUNCTION_NAME)){
							numberOfElementsToNotEvaluate=3; // MDH@20DEC2020: not certain about this
						}else
						if(!strcmp(_significantTokenText,FORWITHFUNCTION_NAME)){ // the initialization argument should always be evaluated (once)
							numberOfElementsToNotEvaluate=5;
						}else
						if(!strcmp(_significantTokenText,DOFUNCTION_NAME)){ // all arguments to the do function should not be evaluated beforehand
							numberOfElementsToNotEvaluate=LLONG_MAX; // all elements should NOT be evaluated
							// the do function is special in that it allows an infinite number of arguments although the function itself expects them wrapped in a single Mvalue
							numberOfFunctionParameters=LLONG_MAX; // replacing 1 with the actual number of parameters we allow for the function
						}
						if(amVerboseDebugging())
							output("Call of function '%s' that takes %llu arguments.\n",_significantTokenText,numberOfFunctionParameters);
						// 1. get the list of function arguments, which depends on the function!!
						expressionToken=nextEnvironmentExpressionToken();
						// MDH@02NOB2019: force the arguments value list to be weak
						Mvalue* _functionArgumentsValue=getValueOfList(TT_END_OF_FUNCTION_CALL,numberOfFunctionParameters,numberOfElementsToNotEvaluate,true);
						expressionToken=getEnvironmentExpressionToken(); // OOPS always update expressionToken after calling a function that might advance it
						if(_functionArgumentsValue!=NULL){
							if(amVerboseDebugging())
								outputValue("Function argument list: '",_functionArgumentsValue,"'.\n");
							if(amVerboseDebugging())
								if(inputCharReadFunction){char c;output("Press any key to continue...");(*inputCharReadFunction)(&c);}
							// MDH@05AUG2019: if we're dealing with the do function I have to map all the arguments to a single list value
							Mlist* functionCallArgumentList=NULL;
							if(!strcmp(_significantTokenText,DOFUNCTION_NAME)
								||!strcmp(_significantTokenText,WHILEFUNCTION_NAME) // MDH@21DEC2020: While as well
								||!strcmp(_significantTokenText,FORFUNCTION_NAME) // MDH@23DEC2020: for as well
							){
								// MDH@02NOV2019: making the list weak
								functionCallArgumentList=owned_list(listMadeWeak(_getListOfType(VT_UNDEFINED)),owner); // creating a list
								if(functionCallArgumentList!=NULL&&appendedToList(functionCallArgumentList,owner,_functionArgumentsValue,M_LL_INVALID)<=0){
									outputError("Failed to create the to do expression list");
									FREE_LIST(functionCallArgumentList,owner);
									functionCallArgumentList=NULL; // so nothing will get done!!
								}
								// TODO what should we do with functionCallArgumentList (which we created) once we're done with it??????
								// DONE see below where it's freed as soon as we created the function call argument map, in the process decrement the reference count of all the argument list elements (Mvalues)
							}else
								functionCallArgumentList=_functionArgumentsValue->value._list; // use the wrapped list
							// 2. get the arguments map
							// MDH@02NOV2019 NOTE: this map will be weak as returned by _getFunctionArgumentMap!!
							Mmap* _functionCallArgumentMap=_getFunctionArgumentMap(function,functionCallArgumentList,owner); // assuming to have a list returned by getListExpressionValue()
							/* MDH@11NOV2019 OOPS: can't wrap the function call argument map here, free it, and have it freed later on again by the garbage collector!!!!
							outputValue("Function argument map: ",_getValueOfMap(_functionCallArgumentMap,false),".\n");
							*/
							// if this is a do() function call, we need to get rid of the single element list we created to wrap all arguments
							if(!strcmp(_significantTokenText,DOFUNCTION_NAME)
								||!strcmp(_significantTokenText,WHILEFUNCTION_NAME) // MDH@21DEC2020
								||!strcmp(_significantTokenText,FORFUNCTION_NAME) // MDH@23DEC2020
							)
								FREE_LIST(functionCallArgumentList,owner);
							/// we do not need to release the function arguments list value because it it never assigned by itself, it is simply a container for the argument list elements (which do have a reference count incremented when added to the list)
							/*
							if(amVerbose())output("Decrementing the reference count of the function arguments value!");
							decrementReferenceCount(_functionArgumentsValue); // TODO is this correct?????
							if(amVerbose())output("Reference count of the function arguments value decremented!");
							*/
							// 3. the result of applying the function to the arguments is the end result
							// MDH@19JUL2019: we need to know when a function is being created, so we can ask for the body commands in command mode
							// MDH@02MAR2020 BUG FIX: extract the function name BEFORE the function call is evaluated!!!!
							char* definedFunctionName=(strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)?NULL:_functionCallArgumentMap->_first->_variable->_value->value._text->_c);
							// if(amVerbose())
								if(definedFunctionName!=NULL){output("Parameter map of function '%s'",definedFunctionName);outputMap(": ",_functionCallArgumentMap,".\n");}
							Mvalue* functionCallValue=getValueOfFunctionCall(function,_significantTokenText,_functionCallArgumentMap);
							if(amVerboseDebugging())
								{output("Result of calling '%s'",_significantTokenText);outputValue(": '",functionCallValue,"'.\n");}
							// if this was a call to the 'define user function' function
							if(definedFunctionName!=NULL){ // MDH@02MAR2020: replacing: !strcmp(_significantTokenText,DEFINEUSERFUNCTION_NAME)){ // a function being defined
								// is the result 1???
								if(functionCallValue!=NULL&&functionCallValue->type==VT_INTEGER&&functionCallValue->value._integer->ll){ // function successfully created
									// let's push the function name on the stack of functions to create
									// we know the first argument contains the function name
									// MDH@02MAR2020: is this a bug???? because we cannot simply assign unless we strdup() the defined function name!!
									// MDH@02MAR2020 replacing (see above): char* definedFunctionName=_functionCallArgumentMap->_first->_variable->_value->value._text->_c;
									Mfunction* definedFunction=getFunction(getExecutionEnvironment(),definedFunctionName);
									// if the function now exists but does not yet have a body, queue the function name on the list of bodies to be set
									if(definedFunction!=NULL&&definedFunction->type==FT_USER&&NULL==definedFunction->functionunion._userfunction->_bodyCommandList)
										registerFunctionBodyRequest(definedFunctionName);
									else
									if(amVerbose())output("Function '%s' completely specified with single body command!\n",definedFunctionName);
								}else
								if(strlen(definedFunctionName))
									output("%sFailed to create function '%s'.\n",M_ERROR_PREFIX,definedFunctionName);
								else
									outputError("Name of function to create not defined.");
							}
							//////outputValue("Function call value '",functionCallValue,"'.\n");
							_valueReference->_value=functionCallValue; // MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,functionCallValue);
							// except getValueOfFunctionCall() doesn't CORRECTION can't harm can it????
							expressionToken=getEnvironmentExpressionToken(); // essential to update after calling a function that updates the expression token
							// we have to free the map ourselves (this is what the _ in front of getFunctionArgumentMap means)
							if(amVerboseDebugging())
								{outputValue("Function call result value: '",_valueReference->_value,"'.\n");outputInfo("Freeing the function argument map!");}
							// MDH@02NOV2019: release the function call argument map to be treated as weak map (i.e. the values do not need to be dereferenced)
							FREE_MAP(_functionCallArgumentMap,owner); // MDH@21MAY2019: no need for the function argument map anymore!!!
							if(amVerboseDebugging())
								outputInfo("Function argument map freed!");
						}else
							outputError("No function arguments");
					}else
						output("%sFunction '%s' unknown!\n",M_ERROR_PREFIX,_significantTokenText);
				}
				break;
			case TT_NEW_VARIABLE: // a non-existing value reference
				// we have to create the variable first (TODO should we wait until actually assigning???)
				// NOTE in certain situations tokenizing occurs outside the evaluation environment so it could be marked as new where it will not be when evaluated
				//	  therefore I've adapted addVariable() so it won't return false when the variable already exists
				// MDH@08AUG2019: any variable that's marked as new should be added to the top-level environment if it does not exist there
				//				we can make that happen by passing in NULL for getExecutionEnvironment() in which case it should check getExecutionEnvironment() only (and not all the parents as well)
				// MDH@09AUG2019: I suppose only explicit local variables (in special function calls) should not be checked to exist in parent environments, but otherwise they should
				//				we could give a warning if this variable is defined inside a special function call and is not a local variable
				////////if(amVerbose())
				if(amVerboseDebugging())
					output("Will add%s variable '%s'.\n",(expressionToken->argument==1?" local":""),_significantTokenText);
				if(!addVariable(expressionToken->argument==1?NULL:getExecutionEnvironment(),getOwnerExecutionEnvironment(),_significantTokenText,VT_UNDEFINED,false)){
					Mstring* _environmentName=owned_string(_getExecutionEnvironmentName(),owner);
					output("%sFailed to add%s variable '%s' to environment '%s'.\n",M_ERROR_PREFIX,(expressionToken->argument!=1&&expressionToken->envid?" implicitly declared local":""),_significantTokenText,string(_environmentName));
					FREE_STRING(_environmentName,owner);
					break; // NO retrieves the undefined value subsequently!!
				}
				if(amVerboseDebugging())
					if(expressionToken->argument!=1&&expressionToken->envid)
						output("WARNING: Not explicitly declared local variable '%s' encountered.\n",_significantTokenText);
			case TT_VARIABLE: // a value reference
				// MDH@25MAR2020 allow indexing of new variables as well!!!! removing: if(expressionToken->type==TT_VARIABLE)
				canbeindexedtheoretically=true;
				// MDH@17APR2020: replacing char* by Mchars* so no need to NULL _significantTokenText anymore (so it will be freed below) (_getChars() will copy the characters)
				_valueReference->_name=owned_chars(_getChars(_significantTokenText),Msubowner(owner,1));
				// replacing: _valueReference->_name=_significantTokenText;_significantTokenText=NULL; // store a copy of the name of the variable being referenced
				if(amVerboseDebugging())
					output("Value reference variable name: '%s'.\n",_valueReference->_name->chars);
				// NOTE do NOT assign the value of an indexed expression because it we did (as we done) the value would be returned as result and not the value at the given index
				///////////////////assignValue(&_valueReference->_value,getValue(_Menvironment,_valueReference->_name)); // store a reference to the value
				/////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				/* MDH@17NOV2019: as we're dealing with the indices below, we should not do it here!!!!
				// a variable can be followed by an index that we should store in the value reference's itemid field
				if(expressionToken->next&&expressionToken->next->type==TT_LIST){
					expressionToken=nextEnvironmentExpressionToken(); // MDH@16OCT2019: why was this commented out???????
					if(amVerbose()){output("Extracting the indices.\n");}
					// MDH@02NOV2019 TODO should this be a weak or strong list????
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0,0,false); // typically allow for any number of indices (although perhaps we should check!!)
					if(amVerbose()){output("XXXXXXX Index value of list '%s'",_valueReference->_name);outputValue("'",indexListValue,"'.\n");}
					expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
					if(amVerbose()){if(expressionToken){output("End of list index token: ");outputToken(expressionToken);}else output("No end of list index token!");outputChar('\n');}
					// using the indexValue we should now update the value represented up until the last index (in case we have an assignment)
					// which means that only the last index value has to be stored and the container of that last index (map or list)
					// MDH@18OCT2019: let's allow a NULL value to allow for appending to a list (as with Python)
					//				how should we treat an empty list???????? differently I guess
					//				the problem with NULL is that _itemid is NULL by itself, so this poses a problem it can't be NULL
					//				I think we'd get an empty list in return not a NULL value (which is a problem if we do!!!!)
					//				for now allow an empty list
					if(indexListValue&&indexListValue->type==VT_LIST){ // a non-empty list
						if(amVerbose())outputValue("Index id: '",indexListValue,"'.\n");
						// MDH@15OCT2019: apparently there is enlisting too many: we can take the first element to unlist what we received BUT this must mean there's a mistake somewhere
						// _valueReference->_itemid=indexListValue; // MDH@02NOV2019 replacing: 
						assignValue(&_valueReference->_itemid,indexListValue); //////////// NOT SURE... indexListValue->value._list->_first->_value); // now storing the entire index/attribute name list
						//// replacing (storing only the last index/attribute name):
						// Mlist* indexList=indexListValue->value._list;
						// Mlistelement* indexListelement=indexList->_first; // must be there!!!
						// // as long as there are successors we haven't reach the last index yet!!!!
						// // TODO what if someone does not specify ALL indices??????
						// while(indexListelement->_next){
						// 	// replace the current value with the value in the list (TODO map) at the current index
						// 	assignValue(&_valueReference->_value,getValueAtIndex(_valueReference->_value->value._list,indexListelement->_value));
						// 	indexListelement=indexListelement->_next;
						// }
						// if(amVerbose())outputValue("Last index: ",indexListelement->_value,"'.");
						// // TODO what is going to happen to indexListValue?????? it should be discarded as its reference count will remain zero but all elements that are used elsewhere (like the last index stored in _valueReference will persist a little longer!!)
						// assignValue(&_valueReference->_itemid,indexListelement->_value); // store the last index value in the _itemid field
						////
					}else
					if(indexListValue)
						output("%sIndex of variable '%s' not a list!\n",M_ERROR_PREFIX,_valueReference->_name);
					else
						output("%sIndex of variable '%s' undefined!\n",M_ERROR_PREFIX,_valueReference->_name);	
				}else
				if(amVerbose())output("Unindexed variable '%s'!\n",_valueReference->_name);
				// MDH@29MAY2019: if we do NOT have an indexed value, retrieve the value...
				// TODO as a side-effect getReferencedValue() will bind the added value to the value reference (as result) BUT I don't think that is how it should be!!! no the assignment takes care of that
				if(!_valueReference->_itemid){
					if(amVerbose())output("Retrieving the value of '%s' when no item id was specified.\n",_valueReference->_name);
					// MDH@18OCT2019: TODO this is dangerous?! MDH@14NOV2019: we could wait until the value is actually requested
					_valueReference->_value=getValue(getExecutionEnvironment(),_valueReference->_name);
					// MDH@02NOV2019: replacing: assignValue(&_valueReference->_value,getValue(getExecutionEnvironment(),_valueReference->_name));
				}
				*/
				if(amVerboseDebugging())outputValuereference("Completed variable value reference: '",_valueReference,"'.\n");
				break;
			case TT_REFERENCE:
				// TODO might allow indexing in the future???
				// MDH@04NOV2019: a reference is an interesting little bugger which we unfortunately need for certain function calls like settype()
				//				for now we only allow referencing FULL variables i.e. not parts of variables like array or map elements although that seems to be a straightforward extension
				//				so it's much similar to an unindexed variable at the moment
				//				for now the only thing we're going to do is store the name of the reference (i.e. starting with @) (without value) so that whoever uses it will know how to resolve it!!!
				// MDH@17APR2020: here we go again
				_valueReference->_name=owned_chars(_getChars(_significantTokenText),Msubowner(owner,1)); // MDH@09JUN2020: OOPS ownership again
				// replacing: _valueReference->_name=_significantTokenText;_significantTokenText=NULL; // store a copy of the name of the variable being referenced
				if(amVerboseDebugging())output("Value reference referenced variable name: '%s'.\n",_valueReference->_name->chars);
				break;			
			case TT_INTEGER: // an integer possibly followed by a real (fractional) part
				// MDH@20JUN2019: some error in the following part because every now and then we get a segmentation fault!!!!
				if(expressionToken->next&&expressionToken->next->type==TT_REAL){ // the integer part of a real
					// TODO fix this
					// first compose the full real text (with the integer text prepended to it)
					Mstring* _realText=owned_string(_getString(_significantTokenText),owner); // the integer part
					expressionToken=nextEnvironmentExpressionToken(); // now pointing to the real fraction part text following the given integer!!!!
					// OOPS do NOT add a '0' character to the token itself (as this would go wrong showing the tokens) TODO check why this goes wrong!!!
					Mstring* pRealText=_realText;
					if(pRealText!=NULL){
						char* _realSignificantTokenText=_getSignificantTokenCharacters(expressionToken); // free asap
						if(amVerboseDebugging())output("Integer part of decimal text: '%s'.\n",string(pRealText));
						pRealText=string_append(pRealText,_realSignificantTokenText);
						if(amVerboseDebugging())
							outputInfo("Fractional part appended!");
						if(strlen(_realSignificantTokenText)==1)pRealText=string_append_char(pRealText,'0'); // a single period is NOT considered equal to zero apparently!!!!
						if(amVerboseDebugging())output("Parsing '%s' to a decimal.\n",string(pRealText));
						// MDH@13JUN2019: instead of using a rational we can now use a decimal
						//				the problem is that we need a context, and therefore a decimal precision 
						//				to this purpose I've added an integer variable in which the actual decimal precision can be set
						uint32_t l=strlen(_realSignificantTokenText); // replacing: string_length(expressionToken->text);
						free(_realSignificantTokenText); // freed!!!
						if(amVerboseDebugging())output("Real part string length: %u.\n",l);
						if(getDP()<l)outputWarning("More decimals present in literal than expected. Rounding may occur.");
						if(amVerboseDebugging())outputInfo("Decimal precision checked!");
						Mdecimal* _decimal=owned_decimal(__decimal(get_default_mpd_context(),0,0),owner);
						if(amVerboseDebugging())outputInfo("Decimal created!");
						if(_decimal!=NULL){
							mpd_set_string(_decimal->mpd,string(pRealText),get_default_mpd_context());
							if(amVerboseDebugging())outputInfo("Decimal initialized.");
							if(!mpd_isnan(_decimal->mpd))
								assignValue(&_valueReference->_value,_getValueOfDecimal(disowned_decimal(_decimal,owner)));
							else
								outputErrorAndText("The decimal value of %s is undefined",string(pRealText));
						}else
							outputError("Failed to create a decimal");
						/* replacing:
						// MDH@07JUN2019: instead of converting the text representation to a long double 'real' we convert the decimal text representation to a rational
						Mrational* _rational=_getDecimalTextRational(string(pRealText));assignValue(&_valueReference->_value,_getValueOfRational(_rational));
						*/
						// replacing: assignValue(&_valueReference->_value,_getFloatValue(_strtold(string(pRealText),getNAR())));
						if(amVerboseDebugging())outputInfo("Releasing decimal text.");
						FREE_STRING(_realText,owner);
						if(amVerboseDebugging())outputInfo("Decimal text released.");
					}else
						outputError("Failed to initialize the text representation of a decimal");
				}else{ // just an integer
					// first we make a big integer, and if it fits into a VT_INTEGER that's where we put it
					Mbiginteger* _biginteger=owned_biginteger(__biginteger(),owner);
					// output("Converting '%s' to a big integer.\n",_significantTokenText); // DEBUG
					if(_biginteger!=NULL&&mp_read_radix(MP_INT_POINTER(_biginteger),_significantTokenText,10)==MP_OKAY){
						// outputBiginteger("Big integer: '",_biginteger,"'.\n"); // DEBUG
						if(mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMin()))!=MP_LT&&mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT){
							// outputBiginteger("Storing the small integer of '",_biginteger,"' as referenced value.\n"); // DEBUG
							_valueReference->_value=_getIntegerValue(mp_get_i64(MP_INT_POINTER(_biginteger)));
			  				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getIntegerValue(mp_get_i64(_biginteger)));
							FREE_BIGINTEGER(_biginteger,owner);
						}else{
							outputBiginteger("Storing big integer '",_biginteger,"' as referenced value.\n"); // DEBUG
							_valueReference->_value=_getValueOfBiginteger(disowned_biginteger(_biginteger,owner));
							// MDH@02NOV2019 replacing:	assignValue(&_valueReference->_value,getValueOfBiginteger(disowned_biginteger(_biginteger,true));
						}
					}else{
						if(_biginteger!=NULL)FREE_BIGINTEGER(_biginteger,owner);
						outputErrorAndText("Failed to create the big integer to store integer ",_significantTokenText);
					}
					// outputValue("Value referenced: '",_valueReference->_value,"'.\n"); // DEBUG
				}
				break;
			case TT_REAL: // unlikely without integer part in front of it though
				_valueReference->_value=_getFloatValue(_strtold(_significantTokenText,getNAR()));
				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getFloatValue(_strtold(_significantTokenText,getNAR())));
				///////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_DQSTRING:
			case TT_SQSTRING: // a string literal
				_valueReference->_value=_getTextValue(_significantTokenText);
				// MDH@02NOV2019 replacing: assignValue(&_valueReference->_value,_getTextValue(_significantTokenText,false));
				////////////////incrementReferenceCount(_valueReference->_value); // TODO combine this with getValue to something called storeValue
				break;
			case TT_LIST: // a list literal
				// MDH@25JUN2023: what used to be considered a list should now be stored in an array, so whatever getValueOfList() returns should become an array!!!
				//                now the question is whether or not we should make getValueOfList() returns an array or we should translate the returned value only here
				//                but essentially it's possible better to checkout every call to getValueOfList() and determine if it should be converted to an array
				canbeindexedtheoretically=true;
				_valueReference=owned_valuereference(_getValuereference(getValueOfArray()),owner); // replacing: getValueOfList(TT_END_OF_LIST,0,0,false)),owner);
				break;
			case TT_MAP: // a map literal
				{
					canbeindexedtheoretically=true;
					Mvalue* _mapValue=getValueOfMap();
					expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
					if(amVerboseDebugging())outputValue("Map extracted: '",_mapValue,"'.\n");
					_valueReference=owned_valuereference(_getValuereference(_mapValue),owner);
				}
				break;
			case TT_EXPRESSION: // an expression wrapped in parentheses which ends with a TT_END_OF_FUNCTION_CALL (although theoretically it's not an end of function call of course)
				{
					canbeindexedtheoretically=true;
					// MDH@10APR2023: CORRECTION 1
					//                cutting off all succeeding list elements prevents us from creating an array
					//                from an expression with multiple elements
					//                therefore we decide here to allow for not limiting the maximum number of list elements
					//                EFFECT: ok, this apparently helps creating a list result, but perhaps we want an array result
					//                CORRECTION 2
					//                if the resulting list has more than one element, we turn it into an array by applying Ma to it
					//                CORRECTION 3
					//                how about always mapping to an array??????
					Mvalue* _expressionListValue=getValueOfList(TT_END_OF_FUNCTION_CALL,0/* replacing: 1*/,0,false);
					expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
					if(amVerboseDebugging())outputInfo("Going to wrap the list extracted!");
					// well, actually, we need the first element of the list that is returned!!!
					// use only the first element if the list only has one element, otherwise use the list itself
					// MDH@0.1.7.14+25JUN2023: we have to undo removing the reduction to a single value since (1+2) should return 3 not the array [3]
					///* MDH@10APR2023: essentially what an expression should return is an array not a list, as well as mapping the result to an array, it should remain a list
					// MDH@18JUL2023: instead of testing the number of elements we should check whether the last assigned index equals 1
					if(_expressionListValue->value._list->_first==NULL)
						_valueReference=NULL;
					else
					if(_expressionListValue->value._list->_last->index==1){ // replacing: if(_expressionListValue->value._list->numberOfElements==1){
						_valueReference=owned_valuereference(_getValuereference(_expressionListValue->value._list->_first->_value),owner);
					}else
					//*/
						_valueReference=owned_valuereference(_getValuereference(_expressionListValue),owner); // applying Ma to _expressionListValue removed!!!!
					// the reference count of _expressionListValue should be 0 now!!!
					if(_expressionListValue!=NULL&&_expressionListValue->count)
						outputBug("Expression list value reference count not 0");
					////////if(amVerboseDebugging())outputInfo("Extracted list wrapped!");
				}
				break;
			default:
				break;
		}
		if(_significantTokenText!=NULL)free(_significantTokenText); // free the (duplicated significant) token text
		if(amVerboseDebugging()){
			if(_valueReference){
				outputInfo("Extracted reference:");
				if(_valueReference->_name!=NULL)output("\tName: '%s'.\n",_valueReference->_name);else outputInfo("\tNo name!");
				if(_valueReference->_value!=NULL)outputValue("\tValue: '",_valueReference->_value,"'.\n");else outputInfo("\tNo value referenced!");
				if(_valueReference->_itemid!=NULL)outputValue("\tIndex ids: ",_valueReference->_itemid,"'.\n");else outputInfo("\tNo item ids.");
			}else
				outputInfo("No value reference!");
		}

		// MDH@17NOV2019: if indexing is theoretically possible, we should further check for indexes
		//				now, even if _valueReference is NULL we have to consume the indexes if present
		if(canbeindexedtheoretically){
			// MDH@24MAR2020: 'indexing' can either take the form of something inside square brackets but now also combined with property names using dot notation
			//				BECAUSE all 'indexing' can be done using square bracket notation every property should become an element in the itemIdsList
			//				so apart from testing for TT_LIST (which start an square bracket index list), we should also test for TT_PROPERTY which also results in adding something to the index list
			expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
			// MDH@17NOV2019: moved over from getValueOfExpression() to where it should below i.e. before unary operators are applied!!!
			// MDH@24MAR2020: it's probably easier to create a list of item ids here to be filled with indices (some of which can be property names)
			Mlist* itemIdsList=NULL;
			while(expressionToken!=NULL&&expressionToken->next!=NULL&&(expressionToken->next->type==TT_LIST||expressionToken->next->type==TT_PROPERTY)){
				if(NULL==itemIdsList){
					itemIdsList=owned_list(_getListOfType(VT_UNDEFINED),owner); // we know we're going to need to list
					if(NULL==itemIdsList){output("%sFailed to create a list to store the indices of '%s'.\n",M_ERROR_PREFIX,_valueReference->_name);break;}
				}
				expressionToken=nextEnvironmentExpressionToken();
				// if(amDebugging())
				if(amVerboseDebugging())
					if(*outputTokenFunction){output("Augmented item id(s) token: ");(*outputTokenFunction)(expressionToken);outputChar('\n');}
				if(expressionToken->type==TT_LIST){
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0,0,false);
					if(indexListValue!=NULL&&indexListValue->type==VT_LIST&&indexListValue->value._list){
						Mlist* newItemIdsList=indexListValue->value._list;
						Mlistelement* newItemIdListElement=(newItemIdsList!=NULL?newItemIdsList->_first:NULL);
						while(newItemIdListElement!=NULL){
							if(appendedToList(itemIdsList,owner,newItemIdListElement->_value,M_LL_INVALID)<=0){
								outputError("Failed to append augmented item id.");
								// TODO can't break here?????
							}
							newItemIdListElement=newItemIdListElement->_next;
						}
					}else
						outputBug("Item ids not a list.");
					// indexListValue will be removed by the garbage collector
				}else{ // a property name (starting with M_PROPERTY_SEPARATOR_CHARACTER)
					// we have to wrap the property name inside a value as text
					Mstring* _propertyName=owned_string(_getSignificantTokenText(expressionToken),owner);
					if(_propertyName!=NULL){
						// MDH@25OCT2020: how about allowing property names to be integers as well, as a shortcut for using square bracket notation
						long long index=_strtoll(string(_propertyName)+1,getNAI()); // NOT including the period of course!!
						if(index!=getNAI()){
							Mvalue* indexValue=_getIntegerValue(index);
							if(NULL==indexValue||appendedToList(itemIdsList,owner,indexValue,M_LL_INVALID)<=0){
								output("%sFailed to add index '%s' to the index list of '%s'.\n",M_ERROR_PREFIX,string(_propertyName),_valueReference->_name);
								// TODO can't break here
							}
						}else
						if(string_setchar(_propertyName,'\'',0)){ // replace the period by a single quote (that we need in the VT_TEXT characters)
							Mvalue* propertyNameValue=_getTextValue(string(_propertyName)); // NOTE _getTextValue() strdup's the text passed in, so we can safely free _propertyName below
							if(NULL==propertyNameValue||appendedToList(itemIdsList,owner,propertyNameValue,M_LL_INVALID)<=0){
								output("%sFailed to add property name '%s' to the index list of '%s'.\n",M_ERROR_PREFIX,string(_propertyName),_valueReference->_name);
								// TODO can't break here
							}
						}
						// NOTE have to release _propertyName here
						FREE_STRING(_propertyName,owner);
					}
				}
				expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
				// if(amDebugging())
				if(amVerboseDebugging()){output("End of augmented item id(s) token: ");(*outputTokenFunction)(expressionToken);outputChar('\n');}
			}
			// MDH@24MAR2020: assuming itemIdsList contains all the index ids (indices and property names) we assign the value wrapped list to the _itemid of the current value reference
			if(itemIdsList!=NULL){
				assignValue(&_valueReference->_itemid,_getValueOfList(disowned_list(itemIdsList,owner)));
				if(amVerboseDebugging())outputValue("Augmented item ids: ",_valueReference->_itemid,".\n");
			}
		}

		// MDH@24MAR2020: with dot property notation now syntacticly accepted, after an index 

		// apply the unary operators (backwards)
		// MDH@17NOV2019: why is the unary operator applied to the _value instead of what _valueReference references?????
		size_t l=(unaryOperators?string_length(unaryOperators):0);
		if(l>0){
			Mvalue* referencedValue;
			char unaryOperator;
			while(l>0&&_valueReference!=NULL){
				unaryOperator=string_char(unaryOperators,--l);
				referencedValue=getReferencedValue(_valueReference);
				if(amVerboseDebugging())
				{output("Applying unary operators: '%c'",unaryOperator);outputValue(" to '",referencedValue,"'.\n");}
				/////////////decrementReferenceCount(_valueReference->_value);
				_valueReference->_value=applyUnaryOperator(unaryOperator,referencedValue); // MDH@17NOV2019 replacing: _valueReference->_value);
				// MDH@02NOV2019 replacing:	assignValue(&_valueReference->_value,applyUnaryOperator(string_char(unaryOperators,--l),_valueReference->_value));
				///////////////////////if(_valueReference->_value)incrementReferenceCount(_valueReference->_value);
				// MDH@17NOV2019: applying a unary operator is dangerous because we may set the value BUT that's NOT enough
				//				because if the name and/or item id remains it will be used again later on
				///output("A\n");
				if(_valueReference->_name!=NULL){
					///output("B\n");
					// MDH@09NOV2022: apparently the following (now commented out at the end) caused a BUG I guess because _valueReference->_name is NOT owned by the value owner
					FREECHARS(_valueReference->_name,Msubowner(owner,1)); // replacing: FREECHARS(_valueReference->_name,Msubowner(getValueOwner(),1));
					_valueReference->_name=NULL;
				}
				///output("C\n");
				if(_valueReference->_itemid!=NULL){ // this is is a value wrapping a list of indices
					///output("D\n");
					// conform what would happen in free_valuereference!!! 
					// TODO consider alternative creating a new value reference
					//	  which is probably better!!!!
					assignValue(&_valueReference->_itemid,NULL);
					///output("E\n");
					/* which is identical to:
					decrementReferenceCount(_valueReference->_itemid);
					_valueReference->_itemid=NULL; // TODO should we do more here? I think not because it's a weak list????
					*/
				}
				///output("F\n");
			}
			if(amVerboseDebugging())
				outputValue("Result after applying unary operators: '",_valueReference->_value,"'.\n");
		}else
		if(amVerboseDebugging())
			outputInfo("No unary operators to apply!");
		
		// move over to the next expression token (following the end token)
		if(expressionToken)expressionToken=nextEnvironmentExpressionToken();

	}

	if(NULL==_valueReference){outputError("No value reference!");return NULL;} // MDH@09NOV2022: just-in-case

	if(amVerboseDebugging())
	{
		if(_valueReference->_value!=NULL){
			outputValue("Value result: '",_valueReference->_value,"'");
			output(" of type '%s'.\n",VALUETYPENAMES[_valueReference->_value->type]);
		}else
			outputInfo("No value result!");
	}
	
	return disowned_valuereference(_valueReference,owner);

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
					//if(amVerbose())output("Storing value '%s' in variable '%s'.",string(_getValueText(_expressionvalue->_value)),firstTokenText);
					if(!setValue(_Menvironment,firstTokenText,_valuereference->_variable->_value)){
						//output("ERROR: Value '%s' not stored.",string(_getValueText(_expressionvalue->_value)));
						// no need to ever free a value ourselves, the 'garbage collection' takes care of that (see removedValues())
						///free_value(_expressionvalue->_value);
						///_expressionvalue->_value=NULL;
					}else
					if(amVerbose())
						output("Variable '%s' set to '%s'.",firstTokenText,string(_getValueText(getValue(_Menvironment,firstTokenText))));
				}
			}
		}
	}
	return _valuereference;
	*/
}

/**
 * @brief returns the 1 value of the given \p valuetype
 * 
 * @param valuetype 
 * @return Mvalue* the 1 value in the given value type \p valuetype
 */
/*
Mvalue* _getValueOneOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
	switch(valuetype){
		case VT_TIME:
		case VT_INTEGER: return _getIntegerValue(1);
		case VT_BIGINTEGER: return _getValueOfBiginteger(_getBiginteger(1));
		case VT_FLOAT: return _getFloatValue(1.0);
		case VT_RATIONAL: return _getValueOfRational(_getRational(_getBiginteger(1),NULL,M_LD_NAN,false));
		case VT_DECIMAL: return _getValueOfDecimal(_getDecimal(__mpd(get_default_mpd_context(),1),M_DP,0,true));
		default:break;
	}
	return NULL;
}
*/

/**
 * @brief returns the long double power of long double base \p base and wrapped exponent \p _powerValue
 * 
 * @param base 
 * @param _powerValue 
 * @return long double the power of long double base \p base and wrapped exponent \p _powerValue
 */
long double getRealPowerValue(long double base,Mvalue* _powerValue){
	// ASSERT assuming power does not equal 0
	if(isLongDoubleUndefined(base)==M_FALSE){ // TODO might still be infinite though
		if(_powerValue!=NULL)
		switch(_powerValue->type){
			case VT_INTEGER:return powl(base,_powerValue->value._integer->ll); // very easy, as we can expext to be able to convert the integer to a long double
			case VT_BIGINTEGER:return powl(base,mp_get_long_double(_powerValue->value._biginteger));
			case VT_DECIMAL:return powl(base,getDecimalLongDouble(_powerValue->value._decimal));
			case VT_RATIONAL:
				{
					long double power=powl(mp_get_long_double(_powerValue->value._rational->num),power);
					if(_powerValue->value._rational->den!=NULL)
						power/=powl(mp_get_long_double(_powerValue->value._rational->den),power);
					return powl(base,power);
				}
			case VT_FLOAT:return powl(base,_powerValue->value._float->ld);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}
/**
 * @brief returns the power of wrapped base \p _baseValue and long double exponent \p power
 * 
 * @param _baseValue 
 * @param power 
 * @return long double the power of wrapped base \p _baseValue and long double exponent \p power
 */
long double getFloatValuePower(Mvalue* _baseValue,long double power){
	// ASSERT assuming power does not equal 0
	if(isLongDoubleUndefined(power)==M_FALSE){ // TODO might still be infinite though
		if(_baseValue!=NULL)
		switch(_baseValue->type){
			case VT_INTEGER:return powl(_baseValue->value._integer->ll,power); // very easy, as we can expext to be able to convert the integer to a long double
			case VT_BIGINTEGER:return powl(mp_get_long_double(_baseValue->value._biginteger),power);
			case VT_DECIMAL:return powl(getDecimalLongDouble(_baseValue->value._decimal),power);
			case VT_RATIONAL:
				{ // transform the base value rational to a long double
					long double base=powl(mp_get_long_double(_baseValue->value._rational->num),power);
					if(_baseValue->value._rational->den!=NULL)
						base/=powl(mp_get_long_double(_baseValue->value._rational->den),power);
					return powl(base,power);
				}
			case VT_FLOAT:return powl(_baseValue->value._float->ld,power);
			default:break;
		}
	}
	return M_LD_NAN; // uncomputable
}
/**
 * @brief returns the decimal power of decimal base \p base and decimal exponent \p exponent
 * 
 * @param base 
 * @param exponent 
 * @param mpd_context 
 * @return Mdecimal* the decimal power of decimal base \p base and decimal exponent \p exponent
 */
Mdecimal* _getDecimalPower(mpd_t* base,mpd_t* exponent,mpd_context_t* mpd_context){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimalPower=NULL;
	if(base!=NULL&&exponent!=NULL&&mpd_context!=NULL){
		_decimalPower=owned_decimal(__decimal(mpd_context,0,0),owner);
		if(_decimalPower!=NULL){
			uint32_t status=0;
			mpd_qpow(_decimalPower->mpd,base,exponent,mpd_context,&status);
			if((status&0xEFBF)!=0)
			{outputError("Failed to apply the decimal power function");FREE_DECIMAL(_decimalPower,owner);_decimalPower=NULL;}
		}else
			outputError("Failed to create decimal power function result");
	}else
		outputError("Insufficient input for computing a decimal power");
	return disowned_decimal(_decimalPower,owner);
}

// computing the integer power of some value, can be performed more exact than when the exponent is not an integer
/**
 * @brief returns the big integer power of big integer base \p baseBiginteger and big integer exponent \p exponentBiginteger
 * 
 * @param baseBiginteger 
 * @param exponentBiginteger 
 * @return Mbiginteger* the big integer power of big integer base \p baseBiginteger and big integer exponent \p exponentBiginteger
 */
Mbiginteger* _getBigintegerPowerWithPositiveBigintegerExponent(Mbiginteger* baseBiginteger,Mbiginteger* exponentBiginteger){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT assuming exponentBiginteger is positive (so never zero!!!)
	Mbiginteger* _resultBiginteger=NULL;
	if(baseBiginteger!=NULL&&exponentBiginteger!=NULL){
		////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(isBigintegerZero(exponentBiginteger))
			_resultBiginteger=owned_biginteger(_getBiginteger(1),owner);
		else
		if(isBigintegerOne(exponentBiginteger))
			_resultBiginteger=owned_biginteger(_getBigintegerCopy(baseBiginteger),owner);
		else{
			// determine half the exponent
			Mbiginteger* _halfexponentBiginteger=owned_biginteger(__biginteger(),owner);
			if(_halfexponentBiginteger!=NULL){
				if(mp_div_2(MP_INT_POINTER(exponentBiginteger),MP_INT_POINTER(_halfexponentBiginteger))==MP_OKAY){
					Mbiginteger* _halfresultBiginteger=owned_biginteger(_getBigintegerPowerWithPositiveBigintegerExponent(baseBiginteger,_halfexponentBiginteger),owner);
					if(_halfresultBiginteger!=NULL){
						Mbiginteger* _doublehalfresultBiginteger=owned_biginteger(__biginteger(),owner);
						if(_doublehalfresultBiginteger!=NULL){
							if((mp_sqr(MP_INT_POINTER(_halfresultBiginteger),MP_INT_POINTER(_doublehalfresultBiginteger))==MP_OKAY)&&
								 (!mp_isodd(MP_INT_POINTER(exponentBiginteger))||mp_mul(MP_INT_POINTER(_doublehalfresultBiginteger),MP_INT_POINTER(baseBiginteger),MP_INT_POINTER(_doublehalfresultBiginteger))==MP_OKAY))
								_resultBiginteger=_doublehalfresultBiginteger;
							else
								FREE_BIGINTEGER(_doublehalfresultBiginteger,owner);
						}
						FREE_BIGINTEGER(_halfresultBiginteger,owner);
					}
				}
				FREE_BIGINTEGER(_halfexponentBiginteger,owner);
			}
		}
	}
	return disowned_biginteger(_resultBiginteger,owner);
}
/**
 * @brief returns the wrapped power of big integer base \p baseBiginteger and big integer exponent \p exponentBiginteger 
 * @details takes the sign of \p exponentBiginteger into account
 *          the result will be a rational if \p exponentBiginteger is negative
 * @param baseBiginteger 
 * @param exponentBiginteger 
 * @return Mvalue* the wrapped big integer power of big integer base \p baseBiginteger and big integer exponent \p exponentBiginteger 
 */
Mvalue* _getBigintegerBigintegerPowerValue(Mbiginteger* baseBiginteger,Mbiginteger* exponentBiginteger){Mallocationowner owner=getOwner(__LINE__);
	Mbiginteger* _bigintegerPower=NULL;
	bool neg=false;
	if(baseBiginteger!=NULL&&exponentBiginteger!=NULL){
		//////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(mp_iszero(MP_INT_POINTER(baseBiginteger))==MP_NO){ // non-zero base
			neg=(mp_isneg(MP_INT_POINTER(exponentBiginteger))==MP_YES);
			if(mp_iszero(MP_INT_POINTER(exponentBiginteger))!=MP_YES){ // not zero
				if(neg)MP_INT_POINTER(exponentBiginteger)->sign=MP_ZPOS; // sneaky, sneaky!! ascertaining to use a positive exponent!
				_bigintegerPower=owned_biginteger(_getBigintegerPowerWithPositiveBigintegerExponent(baseBiginteger,exponentBiginteger),owner);
				// MDH@30MAR2023: OOPS shouldn't we reset the sign?
				if(neg)MP_INT_POINTER(exponentBiginteger)->sign=MP_NEG;
			}else
				_bigintegerPower=owned_biginteger(_getBiginteger(1),owner);
		}else // base is zero, so power is zero as well
			_bigintegerPower=owned_biginteger(_getBiginteger(0),owner);
	}
	return(_bigintegerPower!=NULL?(neg?_getValueOfRational(_getRational(NULL,_bigintegerPower,0,false)):_getValueOfBiginteger(disowned_biginteger(_bigintegerPower,owner))):NULL);
}
/**
 * @brief returns the rational power of rational base \p baseRational and big integer exponent \p exponentBiginteger
 * 
 * @param baseRational 
 * @param exponentBiginteger 
 * @return Mrational* the rational power of rational base \p baseRational and big integer exponent \p exponentBiginteger
 */
Mrational* _getRationalBigintegerPower(Mrational* baseRational,Mbiginteger* exponentBiginteger){Mallocationowner owner=getOwner(__LINE__);
	// the result is the rational of the power of the numerator and the power of the denominator
	// if the exponent is negative we simply exchange the numerator and the denominator!!	
	Mrational* _rationalPower=NULL;
	if(baseRational!=NULL&&exponentBiginteger!=NULL){
		if(!isBigintegerZero(exponentBiginteger)){
			bool neg=false;
			if(mp_isneg(MP_INT_POINTER(exponentBiginteger))==MP_YES){
				MP_INT_POINTER(exponentBiginteger)->sign=MP_ZPOS;
				neg=true;
			}
			Mbiginteger *baseNumerator=(neg?baseRational->den:baseRational->num),*baseDenominator=(neg?baseRational->num:baseRational->den);
			Mbiginteger *_numerator=owned_biginteger(_getBigintegerPowerWithPositiveBigintegerExponent(baseNumerator,exponentBiginteger),owner);
			Mbiginteger *_denominator=owned_biginteger(_getBigintegerPowerWithPositiveBigintegerExponent(baseDenominator,exponentBiginteger),owner);
			if(neg)MP_INT_POINTER(exponentBiginteger)->sign=MP_NEG;
			_rationalPower=owned_rational(_getRational(_numerator,_denominator,M_LD_NAN,true),owner);
			if(NULL==_rationalPower||NULL==_rationalPower->num)FREE_BIGINTEGER(_numerator,owner);
			if(NULL==_rationalPower||NULL==_rationalPower->den)FREE_BIGINTEGER(_denominator,owner);
		}else // the exponent equals zero, so return rational 1
			_rationalPower=owned_rational(__rational(),owner);
	}
	return disowned_rational(_rationalPower,owner);
}
/**
 * @brief returns the maximum of the decimal context of \p d1 and \p d2
 * 
 * @param d1 
 * @param d2 
 * @return mpd_context_t* the maximum of the decimal context of \p d1 and \p d2
 */
mpd_context_t* getContextOfDecimals(Mdecimal* d1,Mdecimal* d2){// TODO check ownership 
	Mdecimalcontext* decimalcontext=getDecimalcontext(MAX((d1?d1->prec:0),(d2?d2->prec:0)));
	return(decimalcontext?decimalcontext->mpd_context:get_default_mpd_context());
}
/**
 * @brief returns the wrapped power of base \p baseValue and exponent \p exponentBiginteger
 * 
 * @param baseValue 
 * @param exponentBiginteger 
 * @return Mvalue* the wrapped power of base \p baseValue and exponent \p exponentBiginteger
 */
Mvalue* _getBigintegerPowerValue(Mvalue* baseValue,Mbiginteger* exponentBiginteger){Mallocationowner owner=getOwner(__LINE__);
	// the general idea is to recursively half the exponent until we end up with having to compute the square which is easy to do
	// but perhaps we should delegate further to functions that deal with specific base value types
	if(baseValue!=NULL)
	switch(baseValue->type){
		case VT_INTEGER:
			{
				Mvalue* _resultValue=NULL;
				Mbiginteger* _baseBiginteger=owned_biginteger(_getBiginteger(baseValue->value._integer->ll),owner);
				if(_baseBiginteger!=NULL){
					_resultValue=_getBigintegerBigintegerPowerValue(_baseBiginteger,exponentBiginteger);
					FREE_BIGINTEGER(_baseBiginteger,owner);
				}
				return _resultValue;
			}
		case VT_BIGINTEGER:
			return _getBigintegerBigintegerPowerValue(baseValue->value._biginteger,exponentBiginteger);
		case VT_RATIONAL:
			return _getValueOfRational(_getRationalBigintegerPower(baseValue->value._rational,exponentBiginteger));
		case VT_DECIMAL:
			if(baseValue->value._decimal->repeating>0){
				Mrational* _decimalRational=owned_rational(_getDecimalRational(baseValue->value._decimal),owner);
				if(_decimalRational!=NULL){
					Mrational* _decimalRationalPower=owned_rational(_getRationalBigintegerPower(_decimalRational,exponentBiginteger),owner);
					FREE_RATIONAL(_decimalRational,owner);
					return _getValueOfRational(disowned_rational(_decimalRationalPower,owner));
				}
			}else{ // base is a 'true' decimal
				Mdecimal* _exponentDecimal=owned_decimal(_getBigintegerDecimal(exponentBiginteger,NULL),owner);
				Mdecimal* _decimalPower=owned_decimal(_getDecimalPower(baseValue->value._decimal->mpd,_exponentDecimal->mpd,getContextOfDecimals(baseValue->value._decimal,_exponentDecimal)),owner);
				FREE_DECIMAL(_exponentDecimal,owner);
				return _getValueOfDecimal(disowned_decimal(_decimalPower,owner));
			}
		default:
			break;
	}
	return NULL;
}

// for finding the decimal root with an integer root degree we need to be able to compute any power of a decimal
// TODO more convenient to work with raw mpd_t instances directly
/**
 * @brief returns the decimal power of base decimal \p baseDecimal and big integer exponent \p exponentBiginteger
 * 
 * @param baseDecimal 
 * @param exponentBiginteger 
 * @return Mdecimal* the decimal power of base decimal \p baseDecimal and big integer exponent \p exponentBiginteger
 */
Mdecimal* _getDecimalPowerWithPositiveBigintegerExponent(Mdecimal* baseDecimal,Mbiginteger* exponentBiginteger){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT assuming exponentBiginteger is positive (so never zero!!!)
	Mdecimal* _resultDecimal=NULL;
	if(baseDecimal!=NULL&&exponentBiginteger!=NULL){
		////////////outputBiginteger("Computing big integer ",baseBiginteger,NULL);outputBiginteger(" ** ",exponentBiginteger,".\n");
		if(isBigintegerZero(exponentBiginteger))
			_resultDecimal=owned_decimal(__decimal(NULL,1,0),owner);
		else
		if(!isBigintegerOne(exponentBiginteger)){
			// get a decimal context
			Mdecimalcontext* decimalcontext=getDecimalcontext(baseDecimal->prec);
			mpd_context_t* mpd_context=(decimalcontext!=NULL?decimalcontext->mpd_context:get_default_mpd_context());
			// determine half the exponent
			Mbiginteger* _halfexponentBiginteger=owned_biginteger(__biginteger(),owner);
			if(_halfexponentBiginteger!=NULL){
				if(mp_div_2(MP_INT_POINTER(exponentBiginteger),MP_INT_POINTER(_halfexponentBiginteger))==MP_OKAY){
					Mdecimal* _halfresultDecimal=owned_decimal(_getDecimalPowerWithPositiveBigintegerExponent(baseDecimal,_halfexponentBiginteger),owner);
					if(_halfresultDecimal){
						Mdecimal* _doublehalfresultDecimal=owned_decimal(__decimal(NULL,1,0),owner);
						if(_doublehalfresultDecimal!=NULL){
							uint32_t status=0;
							mpd_qmul(_doublehalfresultDecimal->mpd,_halfresultDecimal->mpd,_halfresultDecimal->mpd,mpd_context,&status);
							if((status&0xEFBF)==0){
								if(mp_isodd(MP_INT_POINTER(exponentBiginteger))){
									_resultDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
									if(_resultDecimal!=NULL){
										mpd_qmul(_resultDecimal->mpd,_doublehalfresultDecimal->mpd,baseDecimal->mpd,mpd_context,&status);
										if((status&0xEFBF)!=0){FREE_DECIMAL(_resultDecimal,owner);_resultDecimal=NULL;}
									}else
										outputError("Failed to create a decimal in computing the integer power of a decimal");
								}else
									_resultDecimal=owned_decimal(_getDecimalCopy(_doublehalfresultDecimal),owner);
							}else
								outputError("Failed to compute the square of a decimal in computing the integer power of a decimal");
							FREE_DECIMAL(_doublehalfresultDecimal,owner);
						}
						FREE_DECIMAL(_halfresultDecimal,owner);
					}
				}
				FREE_BIGINTEGER(_halfexponentBiginteger,owner);
			}
		}else
			_resultDecimal=owned_decimal(_getDecimalCopy(baseDecimal),owner);
	}
	return disowned_decimal(_resultDecimal,owner);
}

// MDH@11OCT2019: until we know a better way I stick to using squared exponentation
/**
 * @brief returns the power in \p powerBiginteger of base big integer \p baseBiginteger and exponent big integer \p exponentBiginteger
 * 
 * @param baseBiginteger 
 * @param exponentBiginteger 
 * @param powerBiginteger the power of base big integer \p baseBiginteger and exponent big integer \p exponentBiginteger
 * @return mp_err the result error code
 */
mp_err computeBigintegerPower(Mbiginteger const * const baseBiginteger,Mbiginteger const * const exponentBiginteger,Mbiginteger * const powerBiginteger){Mallocationowner owner=getOwner(__LINE__);
	mp_err result=(baseBiginteger!=NULL&&exponentBiginteger!=NULL&&powerBiginteger!=NULL?MP_OKAY:MP_ERR);
	if(result==MP_OKAY){
		if(!isBigintegerOne(baseBiginteger)&&!isBigintegerZero(exponentBiginteger)){
			Mbiginteger *_multiplierBiginteger=owned_biginteger(_getBigintegerCopy(baseBiginteger),owner),
					   			*_exponentBiginteger=owned_biginteger(_getBigintegerCopy(exponentBiginteger),owner);
			if(_multiplierBiginteger!=NULL&&_exponentBiginteger!=NULL){
				if(mp_isodd(MP_INT_POINTER(_exponentBiginteger))!=MP_YES)
					mp_set_i32(MP_INT_POINTER(powerBiginteger),1);
				else 
					result=mp_copy(MP_INT_POINTER(baseBiginteger),MP_INT_POINTER(powerBiginteger)); // initialize powerBiginteger to 1
				// can we do this iteratively???
				while(result==MP_OKAY){
					if(mp_iszero(MP_INT_POINTER(_exponentBiginteger))==MP_YES)break;
					// half the exponent
					if((result=mp_div_2(MP_INT_POINTER(_exponentBiginteger),MP_INT_POINTER(_exponentBiginteger)))!=MP_OKAY)break;
					// square the multiplier
					if((result=mp_sqr(MP_INT_POINTER(_multiplierBiginteger),MP_INT_POINTER(_multiplierBiginteger)))!=MP_OKAY)break;
					if(mp_isodd(MP_INT_POINTER(_exponentBiginteger))==MP_YES)
					if((result=mp_mul(MP_INT_POINTER(powerBiginteger),MP_INT_POINTER(_multiplierBiginteger),MP_INT_POINTER(powerBiginteger)))!=MP_OKAY)break;
				}
			}
			FREE_BIGINTEGER(_multiplierBiginteger,owner);FREE_BIGINTEGER(_exponentBiginteger,owner);
		}else
			result=mp_copy(MP_INT_POINTER(baseBiginteger),MP_INT_POINTER(powerBiginteger));
	}
	return result;
}

// MDH@10OCT2019: if both the argument and the degree is rational we can use rational approximations of the (Newtonian) (decimal) algorithm used in _getBigintegerRootValue()
// MDH@15OCT2019: how about checking whether the root approximation is near the actual root????
/**
 * @brief returns the \p rootDegreeBiginteger root of rational \p rootArgumentRational
 * 
 * @param rootArgumentRational 
 * @param rootDegreeBiginteger 
 * @return Mrational* the \p rootDegreeBiginteger root of rational \p rootArgumentRational
 */
Mrational* _getRationalBigintegerRootRational(Mrational* rootArgumentRational,Mbiginteger* rootDegreeBiginteger){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT root degree big integer must NOT be negative, and use a single mp_digit (otherwise computing the function value computation is too hard)
	Mrational* _rationalBigintegerRootRational=NULL;
	if(rootArgumentRational!=NULL&&rootDegreeBiginteger!=NULL){
		// if the degree is zero, return 1
		if(!isBigintegerZero(rootDegreeBiginteger)){
			// if either rational is one, return a copy of the root argument rational
			if(!isBigintegerOne(rootDegreeBiginteger)&&!isRationalOne(rootArgumentRational)){ // neither equals 1
				if(MP_INT_POINTER(rootDegreeBiginteger)->used==1){ // should ALWAYS be the case!!!!
					outputBiginteger("Computing the rational approximation to the ",rootDegreeBiginteger,"th root");
					outputRational(" of ",rootArgumentRational,".\n");
					Mbiginteger *p_a=rootArgumentRational->num,
											*q_a=(rootArgumentRational->den!=NULL?rootArgumentRational->den:owned_biginteger(_getBiginteger(1),owner)); // helpers that will contain the numerator and denominator of A (the root argument)
					Mbiginteger *_pk=owned_biginteger(__biginteger(),owner),*_qk=owned_biginteger(_getBiginteger(1),owner); // initialize the solution to the root argument allowing that q_k equals NULL to indicate it is equal to 1
					if(_pk!=NULL&&_qk!=NULL){
						// TODO how to check whether rootDegreeBiginteger is nottoo large????
						mp_err result=MP_OKAY;
						// MDH@13OCT2019: TODO if there's exactly one mp_digit being used in the root degree we can improve on the initial approximation
						//				NOTE assuming that 
						int64_t rootDegreeDigit=mp_get_i64(MP_INT_POINTER(rootDegreeBiginteger));
						// let's change the initial approximation of the root using the n root method on the big integer numerator and denominator
						if((result=mp_n_root(MP_INT_POINTER(p_a),(mp_digit)rootDegreeDigit,MP_INT_POINTER(_pk)))==MP_OKAY&&
							(NULL==q_a||(result=mp_n_root(MP_INT_POINTER(q_a),(mp_digit)rootDegreeDigit,MP_INT_POINTER(_qk)))==MP_OKAY)){
							// TODO we could have a match already (currently discovered in the first step of the iterations below)
							// _pk now not above n root of p_a
							// _qk now not above n root of q_a
							// TODO knowing that the root argument rational is now between p_a/(q_a+1) and (p_a+1)/q_a
							/* let's not do this and allow the first initial solution to be on the wrong side
							// if we have a denominator that is smaller than p_a (i.e. the argument rational is larger than 1) go over 
							if(q_a&&mp_cmp(p_a,q_a)>0)result=mp_incr(_pk);
							*/
						}
						if(result==MP_OKAY){
							// try to initialize root degree times the denominator of the root argument (which could be NULL when it equals 1)
							Mbiginteger* _np_a=owned_biginteger(_getBigintegerCopy(rootDegreeBiginteger),owner);
							if(_np_a!=NULL&&q_a!=NULL&&mp_mul(MP_INT_POINTER(_np_a),MP_INT_POINTER(q_a),MP_INT_POINTER(_np_a))!=MP_OKAY){
								FREE_BIGINTEGER(_np_a,owner);_np_a=NULL;
							}
							if(_np_a!=NULL){
								// MDH@30MAY2020: TOTO own all the bigintegers
								// we need some additional helper big integers
								Mbiginteger *_pktothepowern=owned_biginteger(__biginteger(),owner)
										   ,*_qktothepowern=owned_biginteger(_getBiginteger(1),owner)
										   ,*_delta1=owned_biginteger(__biginteger(),owner)
										   ,*_delta2=owned_biginteger(__biginteger(),owner)
										   ,*_distancenumerator=owned_biginteger(__biginteger(),owner)
										   ,*_pktothepowernminus1=owned_biginteger(__biginteger(),owner)
										   ,*_divremainder=owned_biginteger(__biginteger(),owner)
										   ,*_gcd=owned_biginteger(__biginteger(),owner);
								Mbiginteger *_num1=owned_biginteger(__biginteger(),owner)
										   ,*_num=owned_biginteger(__biginteger(),owner)
										   ,*_den=owned_biginteger(__biginteger(),owner)
										   ,*_nextpk=owned_biginteger(__biginteger(),owner)
										   ,*_nextqk=owned_biginteger(__biginteger(),owner); // initially the same as _pk and _qk
								Mbiginteger *_distancedenominator=owned_biginteger(__biginteger(),owner); // the distance to the root
								Mbiginteger *_pkctothepowern=owned_biginteger(__biginteger(),owner)
										   ,*_pkonthisside=owned_biginteger(__biginteger(),owner)
										   ,*_pkontheotherside=owned_biginteger(__biginteger(),owner)
										   ,*_deltapk=owned_biginteger(__biginteger(),owner)
										   ,*_distanceonthisside=owned_biginteger(__biginteger(),owner)
										   ,*_distanceontheotherside=owned_biginteger(__biginteger(),owner)
										   ,*_pkdifference=owned_biginteger(__biginteger(),owner)
										   ,*_pkhalfway=owned_biginteger(__biginteger(),owner)
										   ,*_distancehalfway=owned_biginteger(__biginteger(),owner)
										   ,*_one=owned_biginteger(_getBiginteger(1),owner); // what we'll use for determining a value below the root
								if (_pktothepowern!=NULL&&_qktothepowern!=NULL&&_delta1!=NULL&&_delta2!=NULL&&_distancenumerator!=NULL&&
										_pktothepowernminus1!=NULL&&_divremainder!=NULL&&_gcd!=NULL&&_nextpk!=NULL&&_nextqk!=NULL&&
										_num1!=NULL&&_num!=NULL&&_den!=NULL&&_distancedenominator!=NULL&&_pkctothepowern!=NULL&&
										_pkonthisside!=NULL&&_pkontheotherside!=NULL&&_deltapk!=NULL&&_distanceonthisside!=NULL&&
										_distanceontheotherside!=NULL&&_pkdifference!=NULL&&_pkhalfway!=NULL&&_distancehalfway!=NULL&&_one!=NULL){
									char c;
									unsigned long long iter=0;
									Mrational* _rational;
									Mdecimal* _decimal;
									while(++iter){
										output("\nRational root approximation #%lld: ",iter);outputBiginteger("(",_pk,NULL);outputBiginteger("/",_qk,")");
										// let's show the decimal representation of this value
										_rational=owned_rational(_getRational(/*_getBigintegerCopy*/(_pk),/*_getBigintegerCopy*/(_qk),M_LD_NAN,false),owner);
										if(_rational!=NULL){
											_decimal=owned_decimal(_getRationalDecimal(_rational,NULL),owner);FREE_RATIONAL(_rational,owner);
											if(_decimal!=NULL){outputDecimal("=",_decimal,NULL);FREE_DECIMAL(_decimal,owner);}
										}
										output(".\n");
										// update the delta
										outputBiginteger("\tNumerator ",_pk," to power");outputBiginteger(" ",rootDegreeBiginteger,":");
										if(computeBigintegerPower(_pk,rootDegreeBiginteger,_pktothepowern)!=MP_OKAY)
										{outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										outputBiginteger(" ",_pktothepowern,".\n");
										outputBiginteger("\tDenominator ",_qk," to power");outputBiginteger(" ",rootDegreeBiginteger,":");
										if(computeBigintegerPower(_qk,rootDegreeBiginteger,_qktothepowern)!=MP_OKAY)
										{outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										outputBiginteger(" ",_qktothepowern,".\n");

										/* replacing:
										if(mp_exptmod(_pk,rootDegreeBiginteger,NULL,_pktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the numerator of the rational approximation");break;}
										if(mp_exptmod(_qk,rootDegreeBiginteger,NULL,_qktothepowern)!=MP_OKAY){outputError("Failed to compute the power of the denominator of the rational approximation");break;}
										*/
										if(NULL==q_a||NULL==_delta1||mp_mul(MP_INT_POINTER(_pktothepowern),MP_INT_POINTER(q_a),MP_INT_POINTER(_delta1))!=MP_OKAY)
										{outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										outputBiginteger("\tDelta 1: ",_delta1,".\n");
										if(NULL==p_a||NULL==_delta2||mp_mul(MP_INT_POINTER(_qktothepowern),MP_INT_POINTER(p_a),MP_INT_POINTER(_delta2))!=MP_OKAY)
										{outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										outputBiginteger("\tDelta 2: ",_delta2,".\n");
										if(NULL==_delta2||NULL==_delta1||NULL==_distancenumerator||mp_sub(MP_INT_POINTER(_delta2),MP_INT_POINTER(_delta1),MP_INT_POINTER(_distancenumerator))!=MP_OKAY)
										{outputError("Failed to compute the delta in the rational approximation of the root of a rational");break;}
										// we can compute the denominator of the distance as well which is q_a times _qktothepowern
										if(NULL==_qktothepowern||NULL==q_a||NULL==_distancedenominator||mp_mul(MP_INT_POINTER(_qktothepowern),MP_INT_POINTER(q_a),MP_INT_POINTER(_distancedenominator))!=MP_OKAY)
										{outputError("Failed to compute the denominator of the distance to the rational root argument");break;}

										outputBiginteger("\tDistance from (",_pk,"/");outputBiginteger(NULL,_qk,")");outputBiginteger("**",rootDegreeBiginteger," to ");
										outputBiginteger("root argument (",p_a,"/");outputBiginteger(NULL,q_a,"): ");
										outputBiginteger("(",_distancenumerator,"/");outputBiginteger(NULL,_distancedenominator,")");
										_rational=owned_rational(_getRational(/*_getBigintegerCopy*/(_distancenumerator),/*_getBigintegerCopy*/(_distancedenominator),M_LD_NAN,false),owner);
										if(_rational!=NULL){
											_decimal=owned_decimal(_getRationalDecimal(_rational,NULL),owner);FREE_RATIONAL(_rational,owner);
											if(_decimal!=NULL){outputDecimal("=",_decimal,NULL);FREE_DECIMAL(_decimal,owner);}
										}
										outputChar('\n');

										if(mp_iszero(MP_INT_POINTER(_distancenumerator)))break; // if delta is zero, exact hit (which I think can only happen when)
										
										if(mp_div(MP_INT_POINTER(_pktothepowern),MP_INT_POINTER(_pk),MP_INT_POINTER(_pktothepowernminus1),MP_INT_POINTER(_divremainder))!=MP_OKAY)
										{outputError("Failed to compute a helper big integer in the rational approximation of the root of a rational");break;}
										// update _pk (next) and _qk (next)
										if(mp_mul(MP_INT_POINTER(_pktothepowern),MP_INT_POINTER(_np_a),MP_INT_POINTER(_nextpk))!=MP_OKAY)
										{outputError("Failed to update the numerator of the rational approximation to the root of a rational");break;}
										if(mp_add(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_distancenumerator),MP_INT_POINTER(_nextpk))!=MP_OKAY)
										{outputError("Failed to update the numerator of the rational approximation to the root of a rational");break;}
										if(mp_mul(MP_INT_POINTER(_qk),MP_INT_POINTER(_pktothepowernminus1),MP_INT_POINTER(_nextqk))!=MP_OKAY)
										{outputError("Failed to update the denominator of the rational approximation to the root of a rational");break;}
										if(mp_mul(MP_INT_POINTER(_nextqk),MP_INT_POINTER(_np_a),MP_INT_POINTER(_nextqk))!=MP_OKAY)
										{outputError("Failed to update the denominator of the rational approximation to the root of a rational");break;}
										// that's neat isn't it?
										// how about normalizing _pk and _qk here, which might help
										if(mp_gcd(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_nextqk),MP_INT_POINTER(_gcd))!=MP_OKAY)
										{outputError("Failed to compute the greatest common denominator of the numerator and denominator approximation to the root of a rational");break;}
										if(!isBigintegerOne(_gcd)&&(mp_div(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_gcd),MP_INT_POINTER(_nextpk),MP_INT_POINTER(_divremainder))!=MP_OKAY
																	||mp_div(MP_INT_POINTER(_nextqk),MP_INT_POINTER(_gcd),MP_INT_POINTER(_nextqk),MP_INT_POINTER(_divremainder))!=MP_OKAY))
										{outputError("Failed to normalize the numerator and denominator approximation to the root of a rational");break;}

										// do the bracketing here (on the next pk and qk) 
										// ASSERT we have to ascertain that the denominator remains the same!!!!!
										// the sign of distance numerator tells us on which side of the root we are
										// how about using two big integers???? starting out with 
										if(computeBigintegerPower(_nextpk,rootDegreeBiginteger,_pktothepowern)!=MP_OKAY)
										{outputError("Failed to initialize the distance numerator for bracketing.");break;}
										if(computeBigintegerPower(_nextqk,rootDegreeBiginteger,_qktothepowern)!=MP_OKAY)
										{outputError("Failed to initialize the distance denominator for bracketing.");break;}
										if(mp_mul(MP_INT_POINTER(_pktothepowern),MP_INT_POINTER(q_a),MP_INT_POINTER(_delta1))!=MP_OKAY)
										{outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										if(mp_mul(MP_INT_POINTER(_qktothepowern),MP_INT_POINTER(p_a),MP_INT_POINTER(_delta2))!=MP_OKAY)
										{outputError("Failed to compute delta1 in the rational approximation to the root of a rational");break;}
										if(mp_sub(MP_INT_POINTER(_delta2),MP_INT_POINTER(_delta1),MP_INT_POINTER(_distancenumerator))!=MP_OKAY)
										{outputError("Failed to compute the new distance numerator in the rational approximation of the root of a rational");break;}
										if(mp_mul(MP_INT_POINTER(_qktothepowern),MP_INT_POINTER(q_a),MP_INT_POINTER(_distancedenominator))!=MP_OKAY)
										{outputError("Failed to compute new distance denominator of the rational approximation of the root of a rational");break;}
										outputBiginteger("\n\tDistance of the next Newtonian approximation (",_nextpk,"/");
										outputBiginteger(NULL,_nextqk,"):");
										outputBiginteger("(",_distancenumerator,"/");
										outputBiginteger(NULL,_distancedenominator,").\n");

										if(mp_copy(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_pkonthisside))==MP_OKAY
													&&mp_copy(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_pkontheotherside))==MP_OKAY
													&&mp_copy(MP_INT_POINTER(_distancenumerator),MP_INT_POINTER(_distanceonthisside))==MP_OKAY){
											output("\tWill use the Newtonian approximation to bracket the rational root with two successive rationals");
											outputBiginteger(" with denominator ",_nextqk,".\n");
											/* show the starting point of bracketing!!!
											outputBiginteger("\tBracketing initialized starting at (",_nextpk,"/");outputBiginteger(NULL,_nextqk,")");
											outputBiginteger(" with distance (",_distancenumerator,"/");outputBiginteger(NULL,_distancedenominator,").\n");
											*/
											mp_set_i64(MP_INT_POINTER(_deltapk),(mp_isneg(MP_INT_POINTER(_distancenumerator))==MP_YES?-1:1));
											unsigned long long halvingiterations=0,bracketingiterations=0;
											while(1){
												if(mp_iszero(MP_INT_POINTER(_deltapk))){ // we have two solutions, one on this side and one on the other side
													// the difference could be one between pkonthisside and pkontheotherside in which case we're done
													if(mp_sub(MP_INT_POINTER(_pkonthisside),MP_INT_POINTER(_pkontheotherside),MP_INT_POINTER(_pkdifference))!=MP_OKAY)break;
													if(mp_cmp_mag(MP_INT_POINTER(_pkdifference),MP_INT_POINTER(_one))<=0)break;
													if(mp_add(MP_INT_POINTER(_pkonthisside),MP_INT_POINTER(_pkontheotherside),MP_INT_POINTER(_pkhalfway))!=MP_OKAY)break;
													if(mp_div_2(MP_INT_POINTER(_pkhalfway),MP_INT_POINTER(_pkhalfway))!=MP_OKAY)break;
													if(computeBigintegerPower(_pkhalfway,rootDegreeBiginteger,_distancehalfway)!=MP_OKAY)break;
													if(mp_mul(MP_INT_POINTER(_distancehalfway),MP_INT_POINTER(q_a),MP_INT_POINTER(_distancehalfway))!=MP_OKAY)break;
													if(mp_sub(MP_INT_POINTER(_delta2),MP_INT_POINTER(_distancehalfway),MP_INT_POINTER(_distancehalfway))!=MP_OKAY)break;
													//////outputBiginteger("\tDistance of half way numerator (",_pkhalfway,"/");outputBiginteger(NULL,_qk,"):");outputBiginteger(" ",_distancehalfway,".\n");
													// replace the pk on the same side with the half way one, so soon the bracketing will end
													if(mp_copy(MP_INT_POINTER(_pkhalfway),(mp_isneg(MP_INT_POINTER(_distancehalfway))==mp_isneg(MP_INT_POINTER(_distanceontheotherside))?MP_INT_POINTER(_pkontheotherside):MP_INT_POINTER(_pkonthisside)))!=MP_OKAY)break;
													halvingiterations++;
												}else{
													if(mp_add(MP_INT_POINTER(_pkontheotherside),MP_INT_POINTER(_deltapk),MP_INT_POINTER(_pkontheotherside))!=MP_OKAY)break; // keep going 
													// as we are computing _distanceontheotherside we can use it to store intermediate results
													if(computeBigintegerPower(_pkontheotherside,rootDegreeBiginteger,_distanceontheotherside)!=MP_OKAY)break;
													// what is the distance now???? NOTE _delta2 remains the same because _qk won't change!!!!
													if(mp_mul(MP_INT_POINTER(_distanceontheotherside),MP_INT_POINTER(q_a),MP_INT_POINTER(_distanceontheotherside))!=MP_OKAY)break;
													if(mp_sub(MP_INT_POINTER(_delta2),MP_INT_POINTER(_distanceontheotherside),MP_INT_POINTER(_distanceontheotherside))!=MP_OKAY)break;
													///////outputBiginteger("\tDistance of corrected numerator (",_pkontheotherside,"/");outputBiginteger(NULL,_qk,"):");outputBiginteger(" ",_distanceontheotherside,".\n");
													if(mp_isneg(MP_INT_POINTER(_distanceonthisside))==mp_isneg(MP_INT_POINTER(_distanceontheotherside))){ // still on this side
														if(mp_mul_2(MP_INT_POINTER(_deltapk),MP_INT_POINTER(_deltapk))!=MP_OKAY)break; // double _deltapk otherwise we're going to slow!!!
													}else // yes we're on the other side now, so make _deltapk 0
														mp_set_i64(MP_INT_POINTER(_deltapk),0);
													bracketingiterations++;
												}
												outputChar('.');
											}
											// how about showing the brackets
											output("\n\tNumber of bracketing iterations=%llu - number of halving iterations=%llu.\n",bracketingiterations,halvingiterations);
											outputBiginteger("\tNumerator of approximation on this side of the root: ",_pkonthisside,NULL);outputBiginteger(" with distance ",_distanceonthisside,".\n");
											outputBiginteger("\tNumerator of approximation on the other side of the root: ",_pkontheotherside,NULL);outputBiginteger(" with distance ",_distanceontheotherside,".\n");
											// we need the one with a negative distance
											if(mp_copy((mp_isneg(MP_INT_POINTER(_distanceontheotherside))?MP_INT_POINTER(_pkontheotherside):MP_INT_POINTER(_pkonthisside)),MP_INT_POINTER(_nextpk))!=MP_OKAY)break;
											outputBiginteger("\tAccepted approximation numerator from bracketing: ",_nextpk,".\n");
										}else
											output("\t%sFailed to perform rational root bracketing.\n",M_ERROR_PREFIX);

										// what's the change in approximation?
										if(mp_mul(MP_INT_POINTER(_pk),MP_INT_POINTER(_nextqk),MP_INT_POINTER(_num))!=MP_OKAY)
										{outputError("Failed to initialize the numerator of the change to the rational root approximation");break;}
										if(mp_mul(MP_INT_POINTER(_qk),MP_INT_POINTER(_nextpk),MP_INT_POINTER(_num1))!=MP_OKAY)
										{outputError("Failed to initialize the change to the rational root approximation");break;}
										if(mp_sub(MP_INT_POINTER(_num),MP_INT_POINTER(_num1),MP_INT_POINTER(_num))!=MP_OKAY)
										{outputError("Failed to compute the numerator of the change to the rational root approximation");break;}
										if(mp_mul(MP_INT_POINTER(_nextqk),MP_INT_POINTER(_qk),MP_INT_POINTER(_den))!=MP_OKAY)
										{outputError("Failed to compute the denominator of the change to the rational root approximation");break;}
										if(mp_gcd(MP_INT_POINTER(_num),MP_INT_POINTER(_den),MP_INT_POINTER(_gcd))!=MP_OKAY)
										{outputError("Failed to compute the greatest common denominator of the change in rational approximation to the root of a rational");break;}
										if(!isBigintegerOne(_gcd)&&(mp_div(MP_INT_POINTER(_num),MP_INT_POINTER(_gcd),MP_INT_POINTER(_num),MP_INT_POINTER(_divremainder))!=MP_OKAY
											||mp_div(MP_INT_POINTER(_den),MP_INT_POINTER(_gcd),MP_INT_POINTER(_den),MP_INT_POINTER(_divremainder))!=MP_OKAY))
										{outputError("Failed to normalize the change in the rational approximation to the root of a rational");break;}
										output("\tChange in rational approximation: ",iter);outputBiginteger("(",_num,NULL);outputBiginteger("/",_den,")");
										bool decimalprecisionreached=false;
										_rational=owned_rational(_getRational(/*_getBigintegerCopy*/(_num),/*_getBigintegerCopy*/(_den),M_LD_NAN,false),owner);
										if(_rational!=NULL){
											_decimal=owned_decimal(_getRationalDecimal(_rational,NULL),owner);FREE_RATIONAL(_rational,owner);
											if(_decimal!=NULL){
												if(mpd_iszero(_decimal->mpd)==MP_YES)decimalprecisionreached=true;
												outputDecimal("=",_decimal,NULL);
												FREE_DECIMAL(_decimal,owner);
											}
										}
										output(".\n");
										if(decimalprecisionreached)break; // decimal precision reached
										if(inputCharReadFunction!=NULL){
											output("\t%s...","Press Ctrl-C to stop, or any other key to continue...");(*inputCharReadFunction)(&c);outputChar('\n'); // wait for any key
											if(c==3)break;
										}
										if(mp_copy(MP_INT_POINTER(_nextpk),MP_INT_POINTER(_pk))!=MP_OKAY)
										{outputError("Failed to update the numerator of the rational root approximation");break;}
										if(mp_copy(MP_INT_POINTER(_nextqk),MP_INT_POINTER(_qk))!=MP_OKAY)
										{outputError("Failed to update the denominator of the rational root approximation");break;}

									}
									FREE_BIGINTEGER(_pktothepowern,owner);
									FREE_BIGINTEGER(_qktothepowern,owner);
									FREE_BIGINTEGER(_delta1,owner);
									FREE_BIGINTEGER(_delta2,owner);
									FREE_BIGINTEGER(_distancenumerator,owner);
									FREE_BIGINTEGER(_pktothepowernminus1,owner);
									FREE_BIGINTEGER(_divremainder,owner);
									FREE_BIGINTEGER(_gcd,owner);
									FREE_BIGINTEGER(_num1,owner);
									FREE_BIGINTEGER(_num,owner);
									FREE_BIGINTEGER(_den,owner);
									FREE_BIGINTEGER(_nextpk,owner);
									FREE_BIGINTEGER(_nextqk,owner);
									FREE_BIGINTEGER(_distancedenominator,owner);
									FREE_BIGINTEGER(_pkctothepowern,owner);
									FREE_BIGINTEGER(_pkonthisside,owner);
									FREE_BIGINTEGER(_pkontheotherside,owner);
									FREE_BIGINTEGER(_deltapk,owner);
									FREE_BIGINTEGER(_distanceonthisside,owner);
									FREE_BIGINTEGER(_distanceontheotherside,owner);
									FREE_BIGINTEGER(_pkhalfway,owner);
									FREE_BIGINTEGER(_distancehalfway,owner);
									FREE_BIGINTEGER(_one,owner);
								}
								FREE_BIGINTEGER(_np_a,owner);
								_rationalBigintegerRootRational=owned_rational(_getRational(_pk,_qk,M_LD_NAN,true),owner);
							}
						}else
							outputError("Failed to initialize the rational root approximation");
					}else
						outputError("Failed to initialize the rational rational root approximation");
					if(NULL==rootArgumentRational->den)FREE_BIGINTEGER(q_a,owner); // MDH@30OCT2019: if the root argument denominator equals 1 i.e. the rational is actually a (big) integer...
					// take care of freeing the result numerator and denominator when we do not have a rational root rational
					if(NULL==_rationalBigintegerRootRational){FREE_BIGINTEGER(_pk,owner);FREE_BIGINTEGER(_qk,owner);}
				}else
					outputError("The root degree is too large (which should never happen though, as it should have been prevented)");
			}else
				_rationalBigintegerRootRational=owned_rational(_getRationalCopy(rootArgumentRational),owner);
		}else // the root degree equals 0
			_rationalBigintegerRootRational=owned_rational(_getRational(NULL,NULL,M_LD_NAN,false),owner);
	}
	return disowned_rational(_rationalBigintegerRootRational,owner);
}
// MDH@10OCT2019: better to return a decimal instead of already wrapping the result in a value (so we can do postprocessing!!!!)
//				wait we're wrapping it because the result could be different from a decimal!!!!
/**
 * @brief returns the \p rootDegreeBiginteger root of \p rootArgumentValue
 * 
 * @param rootArgumentValue 
 * @param rootDegreeBiginteger 
 * @return Mvalue* the \p rootDegreeBiginteger root of \p rootArgumentValue
 */
Mvalue* _getBigintegerRootValue(Mvalue* rootArgumentValue,Mbiginteger* rootDegreeBiginteger){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _bigintegerRootValue=NULL;
	if(rootArgumentValue!=NULL&&rootDegreeBiginteger!=NULL){
		if(amVerbose())
		{outputValue("Determining the root of ",rootArgumentValue,NULL);outputBiginteger(" with degree ",rootDegreeBiginteger,".\n");}
		// TODO check for special values like 0 or 1 or negatives...
		// computing with true decimals is fine, but with a decimal that is a rational approximation (i.e. with repeating) we're in trouble
		// a rational with a delta should be purified
		// we can do the decimal approximation first
		Mdecimal* _rootArgumentDecimal=owned_decimal(_getValueDecimal(rootArgumentValue,NULL),owner);
		if(_rootArgumentDecimal!=NULL){
			uint32_t status=0;
			if(amVerbose())
				outputDecimal("Root argument decimal: '",_rootArgumentDecimal,"'.\n");
			// we need an mpd_context for use in the decimal computations!!
			Mdecimalcontext* _decimalcontext=getDecimalcontext(_rootArgumentDecimal->prec);
			mpd_context_t* mpd_context=(_decimalcontext!=NULL?_decimalcontext->mpd_context:get_default_mpd_context());
			if(mpd_context!=NULL){
				Mdecimal* _rootDegreeDecimal=owned_decimal(_getBigintegerDecimal(rootDegreeBiginteger,mpd_context),owner);
				if(_rootDegreeDecimal!=NULL){
					if(amVerbose())
						outputDecimal("Root degree decimal: '",_rootDegreeDecimal,"'.\n");
					// MDH@10OCT2019: to anticipate on root arguments smaller than 1 of which the root will be larger instead of smaller we use the square root as first approximation
					// MDH@10OCT2019: because we are approaching the root from above, as soon as the next approximation is equal to or larger than the previous approximation we're done
					//				this means not using the distance anymore because e.g. 2**(7/9) with decimal precision 20 failed to converge (resulted in toggling between two decimals that different by the final digit)
					Mdecimal *_bigintegerRootDecimal=owned_decimal(__decimal(mpd_context,1,0),owner)
							,*_nextBigintegerRootDecimal=owned_decimal(__decimal(mpd_context,0,0),owner); // let's use 1 as first approximation for any decimal that is below 1
					if(_bigintegerRootDecimal!=NULL&&_nextBigintegerRootDecimal!=NULL){
						if(amVerbose())
						outputInfo("Root computation result decimals created...");
						uint32_t status=0;
						// let's determine on which side of one the root argument is located!!!!
						int rootArgumentComparison=mpd_qcmp(_rootArgumentDecimal->mpd,_bigintegerRootDecimal->mpd,&status);
						// if the root argument is equal to 1, the solution is 1 of course, and no need to continue
						if(rootArgumentComparison>0)mpd_qsqrt(_bigintegerRootDecimal->mpd,_rootArgumentDecimal->mpd,mpd_context,&status);
						// if the root argument does not equal one and we managed to initialize the root argument (to either 1 or the square root), we may continue
						if(rootArgumentComparison&&!(status&0xEFBF)){
							if(amVerbose())
							outputDecimal("Root computation result decimals initialized to ",_bigintegerRootDecimal,".\n");
							// TODO only when the root degree is larger than 2 do we do the iterative process
							// 0. preparations: we need (root degree - 1 ) regularly
							Mdecimal* _rootDegreeMinus1Decimal=owned_decimal(__decimal(mpd_context,0,0),owner); /////_getDecimalCopy(_rootDegreeDecimal);
							if(_rootDegreeMinus1Decimal!=NULL){
								if(amVerbose())
								outputInfo("Root computation helper decimal created...");
								// can't I use getDecimalOne() here?????? apparently not!!
								mpd_t* _decimalOne=__mpd(mpd_context,1);
								mpd_qsub(_rootDegreeMinus1Decimal->mpd,_rootDegreeDecimal->mpd,_decimalOne,mpd_context,&status);
								free_mpd(_decimalOne);
								if((status&0xEFBF)==0){
									if(amVerbose())
									outputInfo("Root computation helper decimal initialized...");
									// we need the root degree minus 1 as big integer as well
									Mbiginteger* _rootDegreeMinus1Biginteger=owned_biginteger(_getBigintegerCopy(rootDegreeBiginteger),owner);
									if(_rootDegreeMinus1Biginteger!=NULL){
										if(amVerbose())
										outputInfo("Root computation helper big integer created...");
										if(mp_decr(MP_INT_POINTER(_rootDegreeMinus1Biginteger))==MP_OKAY){
											if(amVerbose())
											outputInfo("Root computation helper big integer initialized...");
											// we need a product, a quotient and an addition help decimal
											/*
											Mdecimal *_distance=__decimal(mpd_context,0,0),*_prevdistance=__decimal(mpd_context,0,0);
											*/
											Mdecimal *_product=owned_decimal(__decimal(mpd_context,0,0),owner)
													,*_quotient=owned_decimal(__decimal(mpd_context,0,0),owner)
													,*_productplusquotient=owned_decimal(__decimal(mpd_context,0,0),owner)
													,*_power=owned_decimal(__decimal(mpd_context,0,0),owner);
											// initial value of the quotient denominator that we need for checking whether we're done and in the computation
											if(/*_distance&&_prevdistance&&*/_product!=NULL&&_quotient!=NULL&&_productplusquotient!=NULL&&_power!=NULL){
												if(amVerbose())
												outputInfo("Root computation helper decimals created...");
												// ready to rock 'n' roll, eh iterate
												// NOTE iterating until the next value is the same wasn't working, it might be better to compute the power value itself and to compare with the root argument value, if match stop!!
												unsigned long long iter=0;
												Mdecimal *_quotientdenominator=NULL;
												char c;
												while((status&0xEFBF)==0){
													// 'update' the quotient denominator, so we can use it in checking whether we are already there yet, and if not in the computation
													// TODO might it be a good idea to compute the quotient and compare the quotient with the current solution??????
													_quotientdenominator=owned_decimal(_getDecimalPowerWithPositiveBigintegerExponent(_bigintegerRootDecimal,_rootDegreeMinus1Biginteger),owner);
													if(NULL==_quotientdenominator){status=0xFFFFFFFF;break;}
													/* MDH@10OCT2019: not using the distance anymore!!!
													// are we there yet?????
													// compute the current power value
													mpd_qmul(_power->mpd,_quotientdenominator->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													if((status&0xEFBF)!=0)break;
													// if we like to know the distance to the goal we have to compute the difference
													mpd_qsub(_distance->mpd,_power->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													if((status&0xEFBF)!=0)break;
													// if the distance hasn't changed we're done (as we noticed the distance won't be zero in general)
													if(mpd_qcmp(_distance->mpd,_prevdistance->mpd,&status)==0)break; // a match, so done
													free_mpd(_prevdistance->mpd);_prevdistance->mpd=mpd_qncopy(_distance->mpd);
													if(!_prevdistance->mpd){outputError("Failed to copy the distance!");break;}
													*/
													// replacing: if(mpd_iszero(_distance->mpd))break;
													// if the product of the quotient denominator and the root decimal equals the root argument
													// replacing: if(mpd_qcmp(_power->mpd,_bigintegerRootDecimal->mpd,&status)==0)break; // a match, so done
													// next iteration!!!!
													iter++;
													if(amVerbose()){
														output("Root approximation at iteration #%" PRIu32 ":",iter);
														outputDecimal(" ",_bigintegerRootDecimal,".");
														////////outputDecimal(" Distance: ",_distance,".");
														if(inputCharReadFunction!=NULL){
															output(" %s...","Press any key to continue");
															(*inputCharReadFunction)(&c);
														}
														outputChar('\n');
													}
													mpd_qdiv(_quotient->mpd,_rootArgumentDecimal->mpd,_quotientdenominator->mpd,mpd_context,&status);
													FREE_DECIMAL(_quotientdenominator,owner); // don't need it anymore
													mpd_qmul(_product->mpd,_rootDegreeMinus1Decimal->mpd,_bigintegerRootDecimal->mpd,mpd_context,&status);
													mpd_qadd(_productplusquotient->mpd,_product->mpd,_quotient->mpd,mpd_context,&status);
													// update the solution
													mpd_qdiv(_nextBigintegerRootDecimal->mpd,_productplusquotient->mpd,_rootDegreeDecimal->mpd,mpd_context,&status);
													// check whether done or not which is when the next approximation is not smaller than the previous approximation
													if(mpd_qcmp(_nextBigintegerRootDecimal->mpd,_bigintegerRootDecimal->mpd,&status)>=0)break;
													// update _bigintegerRootDecimal to _nextBigintegerRootDecimal
													free_mpd(_bigintegerRootDecimal->mpd);_bigintegerRootDecimal->mpd=mpd_qncopy(_nextBigintegerRootDecimal->mpd);
												}
												if((status&0xEFBF)!=0){
													output("%sRoot computation ended with error code " PRIu32 ".\n",M_ERROR_PREFIX,status);
													FREE_DECIMAL(_bigintegerRootDecimal,owner);
												}else{
													_bigintegerRootValue=_getValueOfDecimal(disowned_decimal(_bigintegerRootDecimal,owner));
													if(amVerbose())
														outputDecimal("Root computation result decimal: '",_bigintegerRootDecimal,"'.\n");
												}
											}else
												outputError("Failed to create helper decimals in computing a root decimal");
											/*
											FREE_DECIMAL(_distance);FREE_DECIMAL(_prevdistance);
											*/
											FREE_DECIMAL(_product,owner);
											FREE_DECIMAL(_quotient,owner);
											FREE_DECIMAL(_productplusquotient,owner);
											FREE_DECIMAL(_power,owner);
										}else
											outputError("Failed to compute a helper big integer in computing a root decimal");
										FREE_BIGINTEGER(_rootDegreeMinus1Biginteger,owner);
										if(amVerbose())
										outputInfo("Root computation helper big integer released...");
									}else
										outputError("Failed to copy the root degree in computing a root decimal");
								}else
									outputError("Failed to compute a helper decimal in computing a root decimal");
							}else
								outputError("Failed to create a helper decimal in computing a root decimal");
							FREE_DECIMAL(_rootDegreeMinus1Decimal,owner);
							if(amVerbose())
							outputInfo("Root computation helper decimal released...");
						}else
						if(rootArgumentComparison)
							outputError("Failed to initialize the result of the root computation to the square root");
						else // wrap the result (which is 1)
							_bigintegerRootValue=_getValueOfDecimal(disowned_decimal(_bigintegerRootDecimal,owner));
					}
					FREE_DECIMAL(_nextBigintegerRootDecimal,owner);
					FREE_DECIMAL(_rootDegreeDecimal,owner);
					if(amVerbose())
					outputInfo("Root computation degree decimal released...");
				}else{
					output("%s",M_ERROR_PREFIX);
					outputBiginteger("Failed to convert root degree '",rootDegreeBiginteger,"' to a decimal.\n");
				}
			}else
				outputError("Failed to create a decimal context for computing a decimal root");
			if(rootArgumentValue->type!=VT_DECIMAL)FREE_DECIMAL(_rootArgumentDecimal,owner);
		}else{
			output("%s",M_ERROR_PREFIX);outputValue("Failed to convert root argument '",rootArgumentValue,"' to a decimal.\n");
		}
	}
	return _bigintegerRootValue;
}
/**
 * @brief returns the power of base \p _value1 and exponent \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the power of base \p _value1 and exponent \p _value2
 */
Mvalue* power(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,power,true);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,power,true);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,power,false); // MDH@02NOV2020 TODO: the input type is not always maintained for certain type combinations but sometimes it is
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,power,false);
	if(isValueZero(_value1)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return getValueOneOfType(_value1->type); // if the power is zero, we return the value 1 with the same type as 
	// MDH@26OCT2019: TODO same approach with any integer as in the other binary operators??????
	// MDH@27OCT2019: let's deal with if either is a real first
	// I suppose if the base or exponent is real, the result should also be real (because it will be approximate)
	if(_value2->type==VT_FLOAT)return _getFloatValue(getFloatValuePower(_value1,_value2->value._float->ld));
	// ASSERT exponent is NOT a real
	if(_value1->type==VT_FLOAT)return _getFloatValue(getRealPowerValue(_value1->value._float->ld,_value2));
	// ASSERT neither is real
	// MDH@27OCT2019: typically for integers with an expoonent that is positive the result should also be integer
	//				and we deal with that separatately
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&
		((_value2->type==VT_INTEGER&&isIntegerPositive(_value2->value._integer)==M_TRUE)||
		 (_value2->type==VT_BIGINTEGER&&isBigintegerPositive(_value2->value._biginteger)==M_TRUE))){
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		Mbiginteger* _powerBiginteger=NULL;
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerbose())
				{outputBiginteger("Exponentiating big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			_powerBiginteger=owned_biginteger(_getBigintegerPowerWithPositiveBigintegerExponent(_biginteger1,_biginteger2),owner);
			if(amVerbose())
				{outputBiginteger("Power: '",_powerBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		// MDH@24OCT2019: if the base is integer, we're going to try to return a small integer
		if(smallinteger1){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			if(amVerbose())outputInfo("Will try to convert the big integer result back to a small integer.");
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			long long llpower=getBigintegerInteger(_powerBiginteger);
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llpower!=M_LL_INVALID){
				if(amVerbose())outputInfo("Will remove the big integer exponentiation result!");
				FREE_BIGINTEGER(_powerBiginteger,owner);
				if(amVerbose())outputInfo("Returning the small integer equivalent of the big integer exponentation result.");
				return _getIntegerValue(llpower);
			}
			outputWarning("Small integer exponentation result out of range, will continue using the big integer exponentiation result.");
		}
		///////outputBiginteger("Exponentation result: '",_powerBiginteger,"'.\n");
		return _getValueOfBiginteger(disowned_biginteger(_powerBiginteger,owner));
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// computing the power is not so easy for certain value type combinations
		// ASSERT base and exponent are not reals
		// given that the way to compute the power might be different depending on the type of the exponent if differentiate between that
		Mvalue* _returnValue=NULL;
		// 1. when the exponent is integer
		if(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER||(_value2->type==VT_RATIONAL&&(NULL==_value2->value._rational->den||isBigintegerOne(_value2->value._rational->den)))){
			Mbiginteger* _exponentBiginteger=owned_biginteger(_getValueBiginteger(_value2),owner);
			_returnValue=_getBigintegerPowerValue(_value1,_exponentBiginteger);
			if(_value2->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_exponentBiginteger,owner);
		}else{ // non-integer exponent, only decimals and rationals remaining
			// if the exponent is inherently rational we should use 
			Mrational* _exponentRational=NULL;
			if(_value2->type==VT_RATIONAL)
				_exponentRational=_value2->value._rational;
			else 
			if(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0)
				_exponentRational=owned_rational(_getDecimalRational(_value2->value._decimal),owner);
			// MDH@14OCT2019: let's only do a rational approximation if the exponent is rational but the denominator is not too large i.e. using at most a single mp_digit (which might be large enough as it is though)
			if(_exponentRational!=NULL&&(NULL==_exponentRational->den||MP_INT_POINTER(_exponentRational->den)->used==1)){ // the exponent is rational and the exponent denominator (which results in root finding is not too large)
				Mvalue* _rootValue=NULL; // the result of the computation of taking the power of a decimal to a rational exponent
				//if(amVerbose())
				outputRational("Computing a power with rational exponent ",_exponentRational,".\n");
				bool neg=(MP_INT_POINTER(_exponentRational->num)->sign==MP_NEG);
				Mbiginteger* _positiveExponentNumerator=(neg?owned_biginteger(_getBigintegerNeg(_exponentRational->num),owner):_exponentRational->num);
				Mbiginteger* exponentDenominator=_exponentRational->den;
				if(_positiveExponentNumerator!=NULL){ // we 
					// TODO now we are testing whether the denominator does not equal one, but in the future all rationals with denominator 1 should have a NULL denominator!!!
					if(exponentDenominator!=NULL&&!isBigintegerOne(exponentDenominator)){ // a 'real' rational (i.e. not simply pretending to be one)
						// MDH@10OCT2019: we can improve on the computation of the power by computing the integer quotient of the rational and the remainder
						// MDH@10OCT2019: we can even improve even more by choosing the smallest of the numerator and denominator to be used in the power computation
						//				NO we can't because 2**(x/y) is NOT equal to 1/2**(y/x) as I conjectured, so we have to stick to the original approximation for now
						// MDH@13OCT2019: if the numerator is negative we will have to invert the solution
						
						mp_ord numdencomp=mp_cmp(MP_INT_POINTER(_positiveExponentNumerator),MP_INT_POINTER(exponentDenominator));
						if(numdencomp!=MP_EQ){ // numerator and denominator are not equal
							Mbiginteger *_integerdividend=owned_biginteger(__biginteger(),owner),
													*_remainder=owned_biginteger(__biginteger(),owner); // the defaults when the denominator equals NULL
							// we divide the maximum of the numerator and the denominator by the minimum of the numerator and the denominator (which typically means that _integerdividend will always be nonzero essentially)
							if(_integerdividend!=NULL
									&&_remainder!=NULL
									&&mp_div(MP_INT_POINTER(_positiveExponentNumerator),MP_INT_POINTER(exponentDenominator),MP_INT_POINTER(_integerdividend),MP_INT_POINTER(_remainder))==MP_OKAY)
							{
								// if _integerdividend is not zero we may compute the multiplier
								Mvalue* _multiplierValue=(mp_iszero(MP_INT_POINTER(_integerdividend))!=MP_YES?_getBigintegerPowerValue(_value1,_integerdividend):NULL);
								// MDH@10OCT2019: we have a special situation when the root argument (_value1) is rational itself in which case we are computing the 
								// instead of computing the power of the numerator we use the _remainder instead
								Mvalue* rootArgumentValue=_getBigintegerPowerValue(_value1,_remainder); // NOTE will be released by the value garbage collector
								// if the root argument is rational, we should use pure big integer computations and have all rational approximations to the root
								// MDH@30OCT2019: wait a minute, if the root argument value is a big integer we would still like to do a rational root instead of decimal root finding
								//				and also when the it's a rational in disguise (stored as a decimal with repeating digits) CAREFUL use a copy of the big integer calling _getRational!!!
								Mrational* _rootArgumentRational=NULL;
								if(rootArgumentValue->type==VT_BIGINTEGER)
									_rootArgumentRational=owned_rational(_getRational(_getBigintegerCopy(rootArgumentValue->value._biginteger),NULL,M_LD_NAN,false),owner);
								else
								if(rootArgumentValue->type==VT_RATIONAL&&floatIsUndefinedOrZero(rootArgumentValue->value._rational->delta))
									_rootArgumentRational=rootArgumentValue->value._rational;
								else
								if(rootArgumentValue->type==VT_DECIMAL&&rootArgumentValue->value._decimal->repeating>0)
									_rootArgumentRational=owned_rational(_getDecimalRational(rootArgumentValue->value._decimal),owner);
								if(_rootArgumentRational){ // the root argument is supposedly a rational
									// TODO if the base is not a pure rational, we could of course purify it
									_rootValue=_getValueOfRational(_getRationalBigintegerRootRational(_rootArgumentRational,exponentDenominator));
									if(rootArgumentValue->type!=VT_RATIONAL)FREE_RATIONAL(_rootArgumentRational,owner);
								}else // base NOT a pure rational, so we're goint go stick with using decimal root approximation i.e. the decimal approximation to the base will be used 
									_rootValue=_getBigintegerRootValue(rootArgumentValue,exponentDenominator);
								// now apply the multiplier if need be
								//if(amVerbose())outputValue("The value to take the root of: '",_rootArgumentValue,"'.\n");
								if(_multiplierValue!=NULL){ // have to multiply
									_rootValue=multiply(_multiplierValue,_rootValue);
									outputValue("Rational exponent root equals the product of multiplier ",_multiplierValue," and ");
									outputBiginteger("the ",exponentDenominator,"th ");
									outputValue("root of ",rootArgumentValue," ");
									outputValue("which is ",_rootValue,".\n");
								}else{ // no need to multiply
									outputBiginteger("The ",exponentDenominator,"th ");
									outputValue("root of ",rootArgumentValue," "); // TODO what happened to rootArgumentValue????
									outputValue("equals ",_rootValue,".\n");
								}
								///// wrong: if(numdencomp==MP_LT)_rootValue=Mreciprocal(_rootValue); // the numerator is smaller than the denominator, so we need to invert the value
								/* replacing NOT splitting up the rational exponent in an integer and remainder part (under 1)
								// base to the power of a rational is the denominatorth root of the numerators power of the base
								Mvalue* _rootArgumentValue=_getBigintegerPowerValue(_value1,_exponentRational->num);// NOTE will be released by the value garbage collector
								//if(amVerbose())outputValue("The value to take the root of: '",_rootArgumentValue,"'.\n");
								Mvalue* _rootValue=_getBigintegerRootValue(_rootArgumentValue,_exponentRational->den);
								outputValue("Rational exponent root of ",_rootArgumentValue,NULL);outputValue(": ",_rootValue,".\n");
								*/
							}else
								outputError("Failed to determine the integer and fractional part of a rational exponent");
							FREE_BIGINTEGER(_integerdividend,owner);FREE_BIGINTEGER(_remainder,owner);
						}else // the numerator equals the denominator meaning that _value1 is the value to return
							_rootValue=_value1;
					}else // an integer rational, so no need to take the root at all!!!
						_rootValue=_getBigintegerPowerValue(_value1,_positiveExponentNumerator); // that's all folks
					// don't forget the delta (if any)
					if(!floatIsUndefinedOrZero(_exponentRational->delta)) // a defined delta
						// multiply the result with base to the power of delta
						// TODO the base should determine what the type of the power computation should be???????
						_returnValue=multiply(_rootValue,_getFloatValue(getFloatValuePower(_value1,getReal(_exponentRational->delta))));
					else
						_returnValue=_rootValue;
					if(neg){
						FREE_BIGINTEGER(_positiveExponentNumerator,owner);
						if(_returnValue!=NULL)_returnValue=Mreciprocal(_returnValue);
					}
				}else
					outputError("Failed to reverse the sign of the rational exponent");
				if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_exponentRational,owner);
			}else{ // not integer based exponent (so if the exponent is a decimal is does not have a repeating part), so use decimals
				// there's a mpd_pow() methods that we technically use on anything that convertable to a decimal
				// converting a rational to a decimal is difficult unless the rational represents a decimal (i.e. the denominator is a power of 10 or we can make it a power of 10 somehow)
				Mdecimal *_baseDecimal=getValueDecimal(_value1,NULL),*_exponentDecimal=getValueDecimal(_value2,NULL);
				if(_value1->type!=VT_DECIMAL)owned_decimal(_baseDecimal,owner);
				if(_value2->type!=VT_DECIMAL)owned_decimal(_exponentDecimal,owner);
				if(_baseDecimal!=NULL&&_exponentDecimal!=NULL){
					Mdecimal* _powerDecimal=NULL;
					mpd_context_t* mpd_context=getContextOfDecimals(_baseDecimal,_exponentDecimal);
					if(NULL==mpd_context)outputError("No decimal context for use in the power function");
					// the decimal library has a function to compute the power of two decimals and we can use that for most of the value pairs
					// if either has a repeating part we have a problem
					if(_baseDecimal->repeating>0){
						if(amVerbose())outputInfo("Computing the power of a rational.");
						// the result is the quotient of the power of the numerator divided by the power of the denominator of the associated rational
						Mrational* _baseRational=getValueRational(_value1);if(_value1->type!=VT_RATIONAL)owned_rational(_baseRational,owner);
						Mdecimal* _baseNumDecimal=owned_decimal(_getBigintegerDecimal(_baseRational->num,mpd_context),owner);
						Mdecimal* _numPowerDecimal=owned_decimal(_getDecimalPower(_baseNumDecimal->mpd,_exponentDecimal->mpd,mpd_context),owner);
						Mdecimal* _baseDenDecimal=owned_decimal(_getBigintegerDecimal(_baseRational->den,mpd_context),owner);
						Mdecimal* _denPowerDecimal=owned_decimal(_getDecimalPower(_baseDenDecimal->mpd,_exponentDecimal->mpd,mpd_context),owner);
						_powerDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
						// the quotient of the numerator and denominator power is the end result
						if(_powerDecimal!=NULL){
							uint32_t status=0;
							mpd_qdiv(_powerDecimal->mpd,_numPowerDecimal->mpd,_denPowerDecimal->mpd,mpd_context,&status);
							if((status&0xEFBF)!=0)
							{outputError("Failed to divide the numerator and denominator powers");FREE_DECIMAL(_powerDecimal,owner);_powerDecimal=NULL;}
						}else
							outputError("Failed to create the decimal result of applying the power function to a rational");
						FREE_DECIMAL(_baseNumDecimal,owner);
						FREE_DECIMAL(_baseDenDecimal,owner);
						FREE_DECIMAL(_numPowerDecimal,owner);
						FREE_DECIMAL(_denPowerDecimal,owner);
						if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_baseRational,owner);
					}else{ // base and exponent decimals is true
						_powerDecimal=owned_decimal(_getDecimalPower(_baseDecimal->mpd,_exponentDecimal->mpd,mpd_context),owner);
						outputInfo("Power decimal computed!");
						// if the exponent is integer typed, the base type determines what to return
					}
					if(_powerDecimal)_returnValue=_getValueOfDecimal(disowned_decimal(_powerDecimal,owner));else outputError("Failed to create the power function result decimal");
				}
				// if the originals weren't decimals, free the created decimals!!!!
				if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_baseDecimal,owner);
				if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_exponentDecimal,owner);
			}
		}
		return _returnValue;
	}
	return NULL;
}

// MDH@15AUG2019: given that epower means base times 10 to the power of exponent, it makes sense to actually compute epower as mul(base,power(10,exponent)) where for 10 we use a single integer
/**
 * @brief returns \p _value times 10 to the power of \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* \p _value times 10 to the power of \p _value2
 */
Mvalue* epower(Mvalue* _value1,Mvalue* _value2){
	// we can still use the shortcuts
	if(!_value1||!_value2)return NULL;
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // NOTE if the power is zero, the multiplication factor will be 1
	return multiply(_value1,power(_getIntegerValue(10),_value2)); // TODO check whether _getIntegerValue(10) actually gets freed by the 'gc'
	/* replacing:
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,epower);if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,epower);

	// if both values are numeric (somehow) we can do the computation
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT||_value1->type==VT_BIGINTEGER||_value1->type==VT_DECIMAL||_value1->type==VT_RATIONAL)&&
		(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT||_value2->type==VT_BIGINTEGER||_value2->type==VT_DECIMAL||_value2->type==VT_RATIONAL)){
		// if the epower exponent is zero _value1 is the result
		// see above: if(isValueZero(_value2))return _value1;
		if(_value2->type==VT_INTEGER){ // an integer exponent
			long long exponentOf10=_value2->value._integer->ll;
			// if the power value is 0, _value1 is the result
			// multiplication or division by an integer power of 10 which is an integer therefore
			if(_value1->type==VT_DECIMAL){
				Mdecimal* _decimal=_getDecimalCopy(_value1->value._decimal);
				_decimal->mpd->exp+=exponentOf10; // TODO theoretically we can get overflow here!! the exponent is an int64_t (alternative is using mpd_scaleb)
				return _getValueOfDecimal(_decimal,true);
			}
			if(_value1->type==VT_RATIONAL){
				// either to multiply the numerator or the denominator with the exponent
				// TODO deal appropriately with any delta!!!
				Mbiginteger* _biginteger10=_getBiginteger(10);
				Mrational* _rational=NULL;
				if(_biginteger10){
					if(exponentOf10>0){
						Mbiginteger* _numerator=_getBigintegerCopy(_value1->value._rational->num);
						while(exponentOf10>0)if(mp_mul(_numerator,_biginteger10,_numerator)==MP_OKAY)exponentOf10--;else break;
						if(exponentOf10==0)
							_rational=_getRational(_numerator,_getBigintegerCopy(_value1->value._rational->den),getReal(_value1->value._rational->delta),true,true);
						else
							outputError("Failed to multiply the numerator of the rational by an integer power of 10.");
					}else{
						Mbiginteger* _denominator=(!_value1->value._rational->den?_getBiginteger(1):_getBigintegerCopy(_value1->value._rational->den));
						while(exponentOf10<0)if(mp_mul(_denominator,_biginteger10,_denominator)==MP_OKAY)exponentOf10++;else break;
						if(exponentOf10==0)
							_rational=_getRational(_getBigintegerCopy(_value1->value._rational->num),_denominator,getReal(_value1->value._rational->delta),true,true);
						else
							outputError("Failed to multiply the denominator of the rational by an integer power of 10.");
					}
					FREE_BIGINTEGER(_biginteger10);
				}
				return _getValueOfRational(_rational,true);
			}
		}
		return multiply(_value1,power(_getIntegerValue(10),_value2)); // temp. value like the power result and _getIntegerValue(10) will be garbage collected if not bound somewhere!!!
		// replacing: return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld*pow(10.,(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld))));
	}
	*/
	return NULL;
}

/**
 * @brief returns the integer quotient of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the integer quotient of \p _value1 and \p _value2
 */
Mvalue* integerdivide(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,integerdivide,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,integerdivide,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,integerdivide,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,integerdivide,false);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return NULL; // TODO or should we return some form of infinity?????
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerquotientBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//			 
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerbose()){outputBiginteger("Integer dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_integerquotientBiginteger=owned_biginteger(__biginteger(),owner); // OOPS have to own it!!!
			if(_integerquotientBiginteger!=NULL&&mp_div(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_integerquotientBiginteger),NULL)!=MP_OKAY)
			{FREE_BIGINTEGER(_integerquotientBiginteger,owner);_integerquotientBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Integer quotient: '",_integerquotientBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1){
			/////outputBiginteger("Freeing '",_biginteger1,"'.\n");
			FREE_BIGINTEGER(_biginteger1,owner);
			_biginteger1=NULL;
		}
		if(smallinteger2){
			//////outputBiginteger("Freeing '",_biginteger2,"'.\n");
			FREE_BIGINTEGER(_biginteger2,owner);
			_biginteger2=NULL;
		}
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//				but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(NULL==_integerquotientBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llintegerquotient=getBigintegerInteger(_integerquotientBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llintegerquotient!=M_LL_INVALID){
				FREE_BIGINTEGER(_integerquotientBiginteger,owner);
				return _getIntegerValue(llintegerquotient);
			}
			outputWarning("Small integer integer quotient out of range, will continue using big integer integer quotient.");
		}
		return _getValueOfBiginteger(disowned_biginteger(_integerquotientBiginteger,owner));
	}
	/* replacing:
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Integer dividing integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return(_value2->value._integer->ll!=0?_getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot):NULL);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerdivideBiginteger=NULL;
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Integer dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			if(mp_iszero(_biginteger2)==MP_NO){
				_integerdivideBiginteger=__biginteger();
				if(_integerdivideBiginteger){
					Mbiginteger* _integerremainderBiginteger=__biginteger();
					if(_integerremainderBiginteger){
						if(mp_div(_biginteger1,_biginteger2,_integerdivideBiginteger,_integerremainderBiginteger)!=MP_OKAY){FREE_BIGINTEGER(_integerdivideBiginteger);_integerdivideBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
						FREE_BIGINTEGER(_integerremainderBiginteger);
					}
				}else 
					outputError("Failed to create the integer divide result big integer");
			}
		}else 
			outputError("Failed to create two helper big integers");
		if(_value1->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getValueOfBiginteger(disowned_biginteger(_integerdivideBiginteger,true);
	}
	*/
	// MDH@28OCT2019: copied over from divide() and adjusted to return an integer
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		if(_value1->type!=VT_RATIONAL)OWNED(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)OWNED(_rational2,owner);
		if(amVerbose())
		{outputRational("Determining the integer part of dividing rational '",_rational1,"'");outputRational(" by '",_rational2,"'.\n");}
		Mrational* _quotientRational=_getRationalQuotient(_rational1,_rational2); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(amVerbose())outputRational("Quotient: '",_quotientRational,"'.\n");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner); // after dividing the two rationals we do not need the newly created rationals anymore
		// we're supposed to return the big integer by dividing the numerator by the denominator and forgetting the remainder
		// this means that we can reuse _getRationalInteger passing in _divisionRational and telling it to return the truncated integer
		if(NULL==_quotientRational)return NULL;
		Mbiginteger* _rationalInteger=owned_biginteger(_getRationalInteger(_quotientRational,true,true),owner);
		FREE_RATIONAL(_quotientRational,owner); // only used for temporary storage of the division rational
		return _getValueOfBiginteger(disowned_biginteger(_rationalInteger,owner));
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1,NULL),*_decimal2=getValueDecimal(_value2,NULL); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)OWNED(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)OWNED(_decimal2,owner); 
		Mdecimal* _divideDecimal=_getDecimalQuotient(_decimal1,_decimal2); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(NULL==_divideDecimal)return NULL;
		Mdecimal* _decimalInteger=owned_decimal(_getDecimalInteger(_divideDecimal,true,true),owner);
		FREE_DECIMAL(_divideDecimal,owner); // only used for temporary storage of the division result
		return _getValueOfDecimal(disowned_decimal(_decimalInteger,owner));
	}
	// MDH@28OCT2019: if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?truncl(ld1/ld2):M_LD_NAN); // same as divide, but applying truncl to the result (cutting off the fraction)
	}
	/* MDH@28OCT2019: see above
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		// if both integer, use lldiv to perform the integer division
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).quot);
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld);
		return _getIntegerValue(truncl(ld1/ld2));
	}
	*/
	return NULL;
}
/**
 * @brief returns \p _value1 modulo \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* \p _value1 modulo \p _value2
 */
Mvalue* divideremainder(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,divideremainder,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divideremainder,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,divideremainder,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divideremainder,false);
	if(isValueZero(_value1)==M_TRUE)return _value1;
	if(isValueOne(_value2)==M_TRUE)return(_value2->type==VT_INTEGER?_getIntegerValue(0):_getFloatValue(0));
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _moduloBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//			 
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerbose())
				{outputBiginteger("Moduloing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_moduloBiginteger=owned_biginteger(__biginteger(),owner);
			if(_moduloBiginteger!=NULL&&mp_div(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),NULL,MP_INT_POINTER(_moduloBiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_moduloBiginteger,owner);_moduloBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerbose()){outputBiginteger(" - Integer division remainder: '",_moduloBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//				but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(NULL==_moduloBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llmodulo=getBigintegerInteger(_moduloBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llmodulo!=M_LL_INVALID){FREE_BIGINTEGER(_moduloBiginteger,owner);return _getIntegerValue(llmodulo);}
			outputWarning("Small integer quotient remainder out of range, will continue using big integer quotient remainder.");
		}
		return _getValueOfBiginteger(disowned_biginteger(_moduloBiginteger,owner));
	}
	/* replacing:
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
		if(amVerbose())output("Integer remainder of dividing integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
		return(_value2->value._integer->ll!=0?_getIntegerValue(lldiv(_value1->value._integer->ll,_value2->value._integer->ll).rem):NULL);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _integerremainderBiginteger=NULL; // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2);
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Remainder of dividing big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			if(mp_iszero(_biginteger2)!=MP_YES){
				_integerremainderBiginteger=__biginteger();
				if(_integerremainderBiginteger){
					Mbiginteger *_integerdivideBiginteger=__biginteger();
					if(_integerdivideBiginteger){
						if(mp_div(_biginteger1,_biginteger2,_integerdivideBiginteger,_integerremainderBiginteger)!=MP_OKAY){FREE_BIGINTEGER(_integerremainderBiginteger);_integerremainderBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
						FREE_BIGINTEGER(_integerdivideBiginteger);
					}
				}
			}
		}
		if(_value1->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getValueOfBiginteger(disowned_biginteger(_integerremainderBiginteger,true);
	}
	*/
	// MDH@28OCT2019: copied over from integerdivide() and adjusted to return the remainder
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		if(_value1->type!=VT_RATIONAL)owned_rational(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)owned_rational(_rational2,owner);
		if(amVerbose()){outputRational("Determining the remainder of dividing rational '",_rational1,"'");outputRational(" by '",_rational2,"'.\n");}
		Mrational* _quotientRational=owned_rational(_getRationalQuotient(_rational1,_rational2),owner); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(amVerbose())outputRational("Quotient: '",_quotientRational,"'.\n");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		// after dividing the two rationals we do not need the newly created rationals anymore
		// we're supposed to return the big integer by dividing the numerator by the denominator and forgetting the remainder
		// this means that we can reuse _getRationalInteger passing in _divisionRational and telling it to return the truncated integer
		if(NULL==_quotientRational)return NULL;
		Mbiginteger* _rationalInteger=owned_biginteger(_getRationalInteger(_quotientRational,true,true),owner);
		FREE_RATIONAL(_quotientRational,owner); // only used for temporary storage of the division rational
		if(NULL==_rationalInteger)return NULL;
		return subtract(_value1,multiply(_value2,_getValueOfBiginteger(disowned_biginteger(_rationalInteger,owner)))); // it's easiest to simply subtract the result from the first value NOTE the intermediate _getBigintegerValue itself will never be bound, so _rationalInteger will be released when the value wrapper is by the GC
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1,NULL),*_decimal2=getValueDecimal(_value2,NULL); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)OWNED(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)OWNED(_decimal2,owner); 
		Mdecimal* _divideDecimal=_getDecimalQuotient(_decimal1,_decimal2); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(NULL==_divideDecimal)return NULL;
		Mdecimal* _decimalInteger=owned_decimal(_getDecimalInteger(_divideDecimal,true,true),owner);
		FREE_DECIMAL(_divideDecimal,owner); // only used for temporary storage of the division result
		if(NULL==_decimalInteger)return NULL;
		return subtract(_value1,multiply(_value2,_getValueOfDecimal(disowned_decimal(_decimalInteger,owner))));
	}
	// MDH@28OCT2019: if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose())
		{outputValue("Determining what's left after dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1-ld2*truncl(ld1/ld2):M_LD_NAN);
	}
	/* replacing:
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		// at least one is real, perform floating point division, then trunc!!!
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld);
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld);
		return _getFloatValue(ld1-ld2*truncl(ld1/ld2)); // what's left after subtracting the truncated value
	}
	*/
	return NULL;
}

// integer arithmetic 
// TODO yet to complete for big integers, rationals, decimals etc.
/**
 * @brief returns the not of long long \p boolean
 * 
 * @param boolean 
 * @return long long \p _value1 modulo \p _value2
 */
static long long not(long long boolean){return(boolean==M_LL_INVALID?M_LL_INVALID:(boolean==M_TRUE?M_FALSE:M_TRUE));} // if invalid, stays invalid, otherwise return M_FALSE when M_TRUE and vice versa
// bitwise operators (and, or, xor)
/**
 * @brief returns the bitwise xor of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the bitwise xor of \p _value1 and \p _value2
 */
Mvalue* bitwisexor(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,bitwisexor,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwisexor,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,bitwisexor,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwisexor,false);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)
		return _getIntegerValue(_value1->value._integer->ll^_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _xorbiginteger=owned_biginteger(__biginteger(),owner);
		if(_xorbiginteger!=NULL){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
			Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
			if(_biginteger1!=NULL&&_biginteger2!=NULL&&mp_xor(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_xorbiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_xorbiginteger,owner);_xorbiginteger=NULL;}
			FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
			return _getValueOfBiginteger(disowned_biginteger(_xorbiginteger,owner));
		}else
			outputError("Failed to create the xor result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
/**
 * @brief returns the bitwise and of \p _value1 or \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the bitwise and of \p _value1 or \p _value2
 */
Mvalue* bitwiseand(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,bitwiseand,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseand,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,bitwiseand,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseand,false);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll&_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _bitwiseandbiginteger=owned_biginteger(__biginteger(),owner);
		if(_bitwiseandbiginteger!=NULL){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
			Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
			if(_biginteger1!=NULL&&_biginteger2!=NULL&&mp_and(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_bitwiseandbiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_bitwiseandbiginteger,owner);_bitwiseandbiginteger=NULL;}
			FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
			return _getValueOfBiginteger(disowned_biginteger(_bitwiseandbiginteger,owner));
		}else
			outputError("Failed to create the bitwise and result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
/**
 * @brief returns the bitwise or of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the bitwise or of \p _value1 and \p _value2
 */
Mvalue* bitwiseor(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,bitwiseor,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,bitwiseor,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,bitwiseor,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,bitwiseor,false);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)
	return _getIntegerValue(_value1->value._integer->ll|_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _bitwiseorbiginteger=owned_biginteger(__biginteger(),owner);
		if(_bitwiseorbiginteger!=NULL){
			// creating two intermediate big integers that need to be freed asap
			Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
			Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
			if(_biginteger1!=NULL&&_biginteger2!=NULL&&mp_or(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_bitwiseorbiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_bitwiseorbiginteger,owner);_bitwiseorbiginteger=NULL;}
			FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
			return _getValueOfBiginteger(disowned_biginteger(_bitwiseorbiginteger,owner));
		}else
			outputError("Failed to create the bitwise or result big integer");
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}

// logical binary operators
/**
 * @brief returns the logical or of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the logical or of \p _value1 and \p _value2
 */
Mvalue* logicaland(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,logicaland,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicaland,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,logicaland,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicaland,false);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)
		return _getIntegerValue(_value1->value._integer->ll&&_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _logicalandbiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		if(_biginteger1!=NULL&&_biginteger2!=NULL)_logicalandbiginteger=_getBiginteger(mp_iszero(MP_INT_POINTER(_biginteger1))==MP_YES||mp_iszero(MP_INT_POINTER(_biginteger2))==MP_YES?0:1); // if either is zero, the result is zero otherwise 1
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return _getValueOfBiginteger(disowned_biginteger(_logicalandbiginteger,owner));
	}	
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}
Mvalue* logicalor(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,logicalor,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,logicalor,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,logicalor,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,logicalor,false);
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)
		return _getIntegerValue(_value1->value._integer->ll||_value2->value._integer->ll);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _logicalorbiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		if(_biginteger1!=NULL&&_biginteger2!=NULL)_logicalorbiginteger=_getBiginteger(mp_iszero(MP_INT_POINTER(_biginteger1))==MP_NO||mp_iszero(MP_INT_POINTER(_biginteger2))==MP_NO?1:0); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return _getValueOfBiginteger(disowned_biginteger(_logicalorbiginteger,owner));
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
	}else
	if(_value1->type==VT_RATIONAL||_value2->type==VT_RATIONAL){
	}
	return NULL;
}

// binary shift operators
// helper function taking care of shifting integers, taking the invalid integer values (for integer and shift) into account
/** TODO shifting might return a big integer
 * @brief returns \p integer shifted by \p shift positions
 * 
 * @param integer 
 * @param shift 
 * @return long long \p integer shifted by \p shift positions
 */
long long integerShift(long long integer,long long shift){
	long long result=(shift==M_LL_INVALID?M_LL_INVALID:integer);
	if(shift!=0&&result!=M_LL_INVALID){if(shift>0)result<<=shift;else result>>=(-shift);} // always shift left or right by a positive value!!
	return result;
}
/**
 * @brief returns \p _value1 shifted left by \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* p _value1 shifted left by \p _value2
 */
Mvalue* Mshiftleft(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Mshiftleft,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Mshiftleft,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Mshiftleft,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Mshiftleft,false);
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // MDH@25OCT2019: if either value is zero the result is the first value
	// ASSERT neither value zero
	// TODO deal with integers separately
	// do NOT allow shifting by anything that cannot be converted to an integer
	// MDH@12OCT2023: shifting left integers is dangerous as the result might become too large in which case we'd best
	//                decide to shift big integers 
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&
		 (_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mvalue* result=NULL;
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),
		     invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			// perhaps we can shift in steps in _biginteger2 is too large?????
			if(mp_iszero(MP_INT_POINTER(_biginteger2))==MP_YES){ // can't happen (see above)
				result=_value1;
			}else{
				Mbiginteger* _biginteger=owned_biginteger(__biginteger(),owner);
				Mbiginteger* _intmaxbiginteger=owned_biginteger(_getBiginteger(INT_MAX),owner);
				if(mp_isneg(MP_INT_POINTER(_biginteger2))==MP_YES){
					// negate _biginteger2
					if(mp_abs(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_biginteger2))==MP_OKAY){ /////MP_INT_POINTER(_biginteger2)->sign=MP_ZPOS;
						// how about shifting in groups of INT_MAX
						do{
							int integer2=INT_MAX;
							if(mp_cmp(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_intmaxbiginteger))!=MP_GT){
								integer2=mp_get_i64(MP_INT_POINTER(_biginteger2));
								mp_zero(MP_INT_POINTER(_biginteger2));
							}else
							if(mp_sub(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_intmaxbiginteger),MP_INT_POINTER(_biginteger))!=MP_OKAY){
								outputError("Failed to subtract shift left maximum");
								FREE_BIGINTEGER(_biginteger,owner);
								_biginteger=NULL;
							}
							if(_biginteger!=NULL&&mp_div_2d(MP_INT_POINTER(_biginteger1),integer2,MP_INT_POINTER(_biginteger),NULL)!=MP_OKAY){
								outputError("Failed to shift right");
								FREE_BIGINTEGER(_biginteger,owner);
								_biginteger=NULL;
							}
						}while(_biginteger!=NULL&&mp_iszero(MP_INT_POINTER(_biginteger2))==MP_NO);
					}else{
						FREE_BIGINTEGER(_biginteger,owner);
						_biginteger=NULL;
					}
				}else{
					do{
						int integer2=INT_MAX;
						if(mp_cmp(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_intmaxbiginteger))!=MP_GT){
							integer2=mp_get_i64(MP_INT_POINTER(_biginteger2));
							mp_zero(MP_INT_POINTER(_biginteger2));
						}else
						if(mp_sub(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_intmaxbiginteger),MP_INT_POINTER(_biginteger))!=MP_OKAY){
							outputError("Failed to subtract shift left maximum");
							FREE_BIGINTEGER(_biginteger,owner);
							_biginteger=NULL;
						}
						if(_biginteger!=NULL&&mp_mul_2d(MP_INT_POINTER(_biginteger1),integer2,MP_INT_POINTER(_biginteger))!=MP_OKAY){
							outputError("Failed to shift left");
							FREE_BIGINTEGER(_biginteger,owner);
							_biginteger=NULL;
						}
					}while(_biginteger!=NULL&&mp_iszero(MP_INT_POINTER(_biginteger2))==MP_NO);
				}
				if(_biginteger!=NULL){
					// convert back to small integer if possible
					if(mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT){
						result=_getIntegerValue(mp_get_i64(MP_INT_POINTER(_biginteger)));
						FREE_BIGINTEGER(_biginteger,owner);
					}else
						result=_getValueOfBiginteger(disowned_biginteger(_biginteger,owner));
				}
				FREE_BIGINTEGER(_intmaxbiginteger,owner);
			}
		}	
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		return result;
	}
	// MDH@12OCT2023 END

	long long shiftleftinteger=getValueInteger(_value2);
	if(shiftleftinteger==M_LL_INVALID)return NULL;
	///////////if(shiftleftinteger==0)return _value1; // return _value1 if no need to shift!!
	// only need to check the value1 type now
	if(_value1->type==VT_INTEGER)return _getIntegerValue(integerShift(_value1->value._integer->ll,shiftleftinteger));
	if(_value1->type==VT_FLOAT)return _getFloatValue(ldShift(_value1->value._float->ld,shiftleftinteger));
	if(_value1->type==VT_BIGINTEGER){
		Mbiginteger* _shiftleftBiginteger=owned_biginteger(_getBigintegerCopy(_value1->value._biginteger),owner); // make a copy of the big integer to shift left
		if(_shiftleftBiginteger!=NULL){
			if((shiftleftinteger<0
			    ?mp_div_2d(MP_INT_POINTER(_value1->value._biginteger),-shiftleftinteger,MP_INT_POINTER(_shiftleftBiginteger),NULL)
					:mp_mul_2d(MP_INT_POINTER(_value1->value._biginteger),shiftleftinteger,MP_INT_POINTER(_shiftleftBiginteger)))!=MP_OKAY){
				FREE_BIGINTEGER(_shiftleftBiginteger,owner);_shiftleftBiginteger=NULL;
				output("%s",M_ERROR_PREFIX);outputBiginteger("Failed to shift '",_value1->value._biginteger,"' to the left.\n");			
			}else 
				outputError("Failed to shift left a big integer");
		}else
			outputError("Failed to create the shift left result big integer");
		return _getValueOfBiginteger(disowned_biginteger(_shiftleftBiginteger,owner));
	}
	if(_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0)){
		Mrational* _shiftleftRational=NULL;
		Mrational* _rational1=owned_rational(_getValueRational(_value1),owner);
		if(_rational1!=NULL){
			// shifting to the right means dividing the rational by 2 the given number of times but this means doubling the denominator
			// i.e. we should never divide because we could end up with zero (and loose the precision of exact computations)
			_shiftleftRational=owned_rational(_getRationalCopy(_rational1),owner);
			if(_shiftleftRational!=NULL){
				///if(amVerbose())
				outputRational("Rational shift left copy: '",_shiftleftRational,"'.\n");
				if(shiftleftinteger<0){ // naughty boy (or girl for that matter)... // actually a shift right
					// multiply the denominator by 2 shiftleftinteger times
					if(NULL==_shiftleftRational->den)_shiftleftRational->den=_getBiginteger(1); // force having a non NULL denominator before trying to shift it
					if(_shiftleftRational->den==NULL||mp_mul_2d(MP_INT_POINTER(_shiftleftRational->den),-shiftleftinteger,MP_INT_POINTER(_shiftleftRational->den))!=MP_OKAY){
						FREE_RATIONAL(_shiftleftRational,owner);_shiftleftRational=NULL;
						outputError("Failed to half a rational");
					}
					// force normalization
					if(_shiftleftRational!=NULL){_shiftleftRational->normalized=false;normalizeRational(_shiftleftRational,owner);}
				}else{ 
					if(mp_mul_2d(MP_INT_POINTER(_shiftleftRational->num),shiftleftinteger,MP_INT_POINTER(_shiftleftRational->num))!=MP_OKAY){
						FREE_RATIONAL(_shiftleftRational,owner);_shiftleftRational=NULL;
						outputError("Failed to double a rational");
					}
				}
				if(_shiftleftRational!=NULL){
					_shiftleftRational->normalized=false;
					if(!normalizeRational(_shiftleftRational,owner))
					{output("%s",M_ERROR_PREFIX);outputRational("Failed to normalize shift left rational ",_shiftleftRational,".\n");}
				}
			}else 
				outputError("Failed to copy a rational");
			if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		}else 
			outputError("Failed to convert a decimal to a rational");
		return _getValueOfRational(disowned_rational(_shiftleftRational,owner));
	}
	if(_value1->type==VT_DECIMAL){ // a pure decimal 
		// similar approach as with a rational, except decimals have a shiftl and shiftr method
		Mdecimal* _shiftleftDecimal=owned_decimal(_getDecimalCopy(_value1->value._decimal),owner);
		if(_shiftleftDecimal!=NULL){
			uint32_t status=0;
			if(_shiftleftDecimal>0)mpd_qshiftr(_shiftleftDecimal->mpd,_shiftleftDecimal->mpd,shiftleftinteger,&status);else mpd_qshiftl(_shiftleftDecimal->mpd,_shiftleftDecimal->mpd,-shiftleftinteger,&status);
			if((status&0xEFBF)!=0){FREE_DECIMAL(_shiftleftDecimal,owner);_shiftleftDecimal=NULL;outputError("Failed to shift a decimal to the left");}
		}else 
			outputError("Failed to copy a decimal");
		return _getValueOfDecimal(disowned_decimal(_shiftleftDecimal,owner));
	}
	return NULL;
}
/**
 * @brief returns \p _value1 shifted right \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* \p _value1 shifted right \p _value2
 */
Mvalue* shiftright(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,shiftright,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,shiftright,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,shiftright,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,shiftright,false);
	if(isValueZero(_value1)==M_TRUE||isValueZero(_value2)==M_TRUE)return _value1; // MDH@26OCT2019: if either value is zero return _value1
	// ASSERT neither value is zero
	// do NOT allow shifting by anything that cannot be converted to an integer

	// MDH@12OCT2023: shifting left integers is dangerous as the result might become too large in which case we'd best
	//                decide to shift big integers 
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&
		 (_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mvalue* result=NULL;
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),
		     invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			// perhaps we can shift in steps in _biginteger2 is too large?????
			if(mp_iszero(MP_INT_POINTER(_biginteger2))==MP_YES){ // can't happen (see above)
				result=_value1;
			}else{
				Mbiginteger* _biginteger=owned_biginteger(__biginteger(),owner);
				Mbiginteger* _intmaxbiginteger=owned_biginteger(_getBiginteger(INT_MAX),owner);
				if(mp_isneg(MP_INT_POINTER(_biginteger2))==MP_YES){
					// negate _biginteger2
					if(mp_abs(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_biginteger2))==MP_OKAY){ /////MP_INT_POINTER(_biginteger2)->sign=MP_ZPOS;
						// how about shifting in groups of INT_MAX
						do{
							int integer2=INT_MAX;
							if(mp_cmp(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_intmaxbiginteger))!=MP_GT){
								integer2=mp_get_i64(MP_INT_POINTER(_biginteger2));
								mp_zero(MP_INT_POINTER(_biginteger2));
							}else
							if(mp_sub(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_intmaxbiginteger),MP_INT_POINTER(_biginteger))!=MP_OKAY){
								outputError("Failed to subtract shift left maximum");
								FREE_BIGINTEGER(_biginteger,owner);
								_biginteger=NULL;
							}
							if(_biginteger!=NULL&&mp_mul_2d(MP_INT_POINTER(_biginteger1),integer2,MP_INT_POINTER(_biginteger))!=MP_OKAY){
								outputError("Failed to shift left");
								FREE_BIGINTEGER(_biginteger,owner);
								_biginteger=NULL;
							}
						}while(_biginteger!=NULL&&mp_iszero(MP_INT_POINTER(_biginteger2))==MP_NO);
					}else{
						FREE_BIGINTEGER(_biginteger,owner);
						_biginteger=NULL;
					}
				}else{
					do{
						int integer2=INT_MAX;
						if(mp_cmp(MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_intmaxbiginteger))!=MP_GT){
							integer2=mp_get_i64(MP_INT_POINTER(_biginteger2));
							mp_zero(MP_INT_POINTER(_biginteger2));
						}else
						if(mp_sub(MP_INT_POINTER(_biginteger),MP_INT_POINTER(_intmaxbiginteger),MP_INT_POINTER(_biginteger))!=MP_OKAY){
							outputError("Failed to subtract shift right maximum");
							FREE_BIGINTEGER(_biginteger,owner);
							_biginteger=NULL;
						}
						if(_biginteger!=NULL&&mp_div_2d(MP_INT_POINTER(_biginteger1),integer2,MP_INT_POINTER(_biginteger),NULL)!=MP_OKAY){
							outputError("Failed to shift right");
							FREE_BIGINTEGER(_biginteger,owner);
							_biginteger=NULL;
						}
					}while(_biginteger!=NULL&&mp_iszero(MP_INT_POINTER(_biginteger2))==MP_NO);
				}
				if(_biginteger!=NULL){
					// convert back to small integer if possible
					if(mp_cmp(MP_INT_POINTER(_biginteger),MP_INT_POINTER(getBigintegerLLMax()))!=MP_GT){
						result=_getIntegerValue(mp_get_i64(MP_INT_POINTER(_biginteger)));
						FREE_BIGINTEGER(_biginteger,owner);
					}else
						result=_getValueOfBiginteger(disowned_biginteger(_biginteger,owner));
				}
				FREE_BIGINTEGER(_intmaxbiginteger,owner);
			}
		}	
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		return result;
	}
	// MDH@12OCT2023 END

	long long shiftrightinteger=getValueInteger(_value2);if(shiftrightinteger==M_LL_INVALID)return NULL;
	/////////////if(shiftrightinteger==0)return _value1; // return _value1 if no need to shift!!
	// only need to check the value1 type now
	if(_value1->type==VT_INTEGER)return _getIntegerValue(integerShift(_value1->value._integer->ll,-shiftrightinteger));
	if(_value1->type==VT_FLOAT)return _getFloatValue(ldShift(_value1->value._float->ld,-shiftrightinteger));
	if(_value1->type==VT_BIGINTEGER){
		Mbiginteger* _shiftrightBiginteger=owned_biginteger(_getBigintegerCopy(_value1->value._biginteger),owner); // make a copy of the big integer to shift right
		if(_shiftrightBiginteger!=NULL){
			if((shiftrightinteger>0?mp_div_2d(MP_INT_POINTER(_value1->value._biginteger),shiftrightinteger,MP_INT_POINTER(_shiftrightBiginteger),NULL):mp_mul_2d(MP_INT_POINTER(_value1->value._biginteger),-shiftrightinteger,MP_INT_POINTER(_shiftrightBiginteger)))!=MP_OKAY){
				FREE_BIGINTEGER(_shiftrightBiginteger,owner);_shiftrightBiginteger=NULL;
				output("%s",M_ERROR_PREFIX);outputBiginteger("Failed to shift '",_value1->value._biginteger,"' to the right.\n");			
			}else 
				outputError("Failed to shift right a big integer");
		}else
			outputError("Failed to create the shift right result big integer");
		return _getValueOfBiginteger(disowned_biginteger(_shiftrightBiginteger,owner));
		/* replacing:
		// what we shift by should fit in int64_t!!!!
		if(_value2->type==VT_INTEGER||_value2->value._biginteger->used<=1){
			int64_t shr=(_value2->type==VT_INTEGER?_value2->value._integer->ll:mp_get_i64(_value2->value._biginteger));
			if(shr!=M_LL_INVALID){
				Mbiginteger* _biginteger1=(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger));
				if(_biginteger1){
					if(amVerbose()){outputBiginteger("Shifting big integer '",_biginteger1,"' left");output(" by %" PRIi64 ".\n",shr);}
					if((shr>0?mp_div_2d(_biginteger1,shr,_biginteger1,NULL):mp_mul_2d(_biginteger1,-shr,_biginteger1))==MP_OKAY)return _getValueOfBiginteger(disowned_biginteger(_biginteger1,true);
					output("%s",M_ERROR_PREFIX);outputBiginteger("Failed to shift '",_biginteger1,"' to the right.\n");			
					FREE_BIGINTEGER(_biginteger1);
				}
			}
		}else
			outputError("Number of shift positions too large");
		*/
	}
	if(_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0)){
		Mrational* _shiftrightRational=NULL;
		Mrational* _rational1=owned_rational(_getValueRational(_value1),owner);
		if(_rational1!=NULL){
			// shifting to the right means dividing the rational by 2 the given number of times but this means doubling the denominator
			// i.e. we should never divide because we could end up with zero (and loose the precision of exact computations)
			_shiftrightRational=owned_rational(_getRationalCopy(_rational1),owner);
			if(_shiftrightRational!=NULL){
				///if(amVerbose())
				outputRational("Rational shift right copy: '",_shiftrightRational,"'.\n");
				if(shiftrightinteger>0){
					// multiply the denominator by 2 shiftrightinteger times
					if(NULL==_shiftrightRational->den)_shiftrightRational->den=OWNED(_getBiginteger(1),owner); // force having a non NULL denominator before trying to shift it
					if(_shiftrightRational->den==NULL||mp_mul_2d(MP_INT_POINTER(_shiftrightRational->den),shiftrightinteger,MP_INT_POINTER(_shiftrightRational->den))!=MP_OKAY){
						FREE_RATIONAL(_shiftrightRational,owner);_shiftrightRational=NULL;
						outputError("Failed to half a rational");
					}
					// force normalization
					if(_shiftrightRational!=NULL){_shiftrightRational->normalized=false;normalizeRational(_shiftrightRational,owner);}
				}else{ // naughty boy (or girl for that matter)...
					if(mp_mul_2d(MP_INT_POINTER(_shiftrightRational->num),-shiftrightinteger,MP_INT_POINTER(_shiftrightRational->num))!=MP_OKAY){
						FREE_RATIONAL(_shiftrightRational,owner);_shiftrightRational=NULL;
						outputError("Failed to double a rational");
					}
				}
				if(_shiftrightRational!=NULL){
					_shiftrightRational->normalized=false;
					if(!normalizeRational(_shiftrightRational,owner))
					{output("%s",M_ERROR_PREFIX);outputRational("Failed to normalize shift right rational ",_shiftrightRational,".\n");}
				}
			}else 
				outputError("Failed to copy a rational");
			if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		}else 
			outputError("Failed to convert a decimal to a rational");
		return _getValueOfRational(disowned_rational(_shiftrightRational,owner));
	}
	if(_value1->type==VT_DECIMAL){ // a pure decimal 
		// similar approach as with a rational, except decimals have a shiftl and shiftr method
		Mdecimal* _shiftrightDecimal=owned_decimal(_getDecimalCopy(_value1->value._decimal),owner);
		if(_shiftrightDecimal!=NULL){
			uint32_t status=0;
			if(_shiftrightDecimal>0)mpd_qshiftr(_shiftrightDecimal->mpd,_shiftrightDecimal->mpd,shiftrightinteger,&status);
			else mpd_qshiftl(_shiftrightDecimal->mpd,_shiftrightDecimal->mpd,-shiftrightinteger,&status);
			if((status&0xEFBF)!=0)
			{FREE_DECIMAL(_shiftrightDecimal,owner);_shiftrightDecimal=NULL;outputError("Failed to shift a decimal to the right");}
		}else 
			outputError("Failed to copy a decimal");
		return _getValueOfDecimal(disowned_decimal(_shiftrightDecimal,owner));
	}
	return NULL;
}

// binary comparison operators
// MDH@03NOV2020: helper functions that do the heavy lifting
// let's start with the one used by Msort and Msorted

// TODO these should return either TRUE, FALSE or UNDEFINED independent of the input type
/**
 * @brief returns M_TRUE if \p _value1 is smaller than \p _value2 , M_FALSE otherwise
 * @details deals with array and list arguments as well
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUE if \p _value1 is smaller than \p _value2 , M_FALSE otherwise
 */
Mvalue* Msmallerthan(Mvalue* _value1,Mvalue* _value2){
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Msmallerthan,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Msmallerthan,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Msmallerthan,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Msmallerthan,false);
	return _getIntegerValue(smallerthan(_value1,_value2));
}
/** TODO could also return M_LL_INVALID if the values are not comparable!!
 * @brief returns M_TRUE if \p _value1 is larger than \p _value2 , M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long M_TRUE if \p _value1 is larger than \p _value2 , M_FALSE otherwise
 */
static long long largerthan(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1&&NULL==_value2)return M_FALSE; // both NULL, so not larger than
	if(NULL==_value1||NULL==_value2)return(_value1!=NULL?M_TRUE:M_FALSE); // if at least one of them is NULL the result is M_TRUE if _value1 is __not__ NULL (and _value2 is NULL therefore), otherwise both are NULL and they are equal
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL?strcmp(string(_value1text),string(_value2text)):(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result>0?M_TRUE:M_FALSE);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)>(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long lllargerthan=(_biginteger1!=NULL&&_biginteger2!=NULL?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_GT?M_TRUE:M_FALSE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return(lllargerthan);
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is larger than another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalNegative(_rationalDifference));
				FREE_RATIONAL(_rationalDifference,owner);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=owned_decimal(_getValueDecimal(_value1,NULL),owner)
						,*_decimal2=owned_decimal(_getValueDecimal(_value2,NULL),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is larger than another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalNegative(_decimalDifference));
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUE if \p _value1 is larger than \p _value2 , M_FALSE otherwise
 * @details takes list and array arguments into account
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUE if \p _value1 is larger than \p _value2 , M_FALSE otherwise
 */
Mvalue* Mlargerthan(Mvalue* _value1,Mvalue* _value2){
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Mlargerthan,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Mlargerthan,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Mlargerthan,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Mlargerthan,false);
	return _getIntegerValue(largerthan(_value1,_value2));
}

/** TODO should return M_LL_INVALID if the arguments are not comparable
 * @brief returns M_TRUE if \p _value1 is larger than or equal to \p _value2 , M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long M_TRUE if \p _value1 is larger than or equal to \p _value2 , M_FALSE otherwise
 */
long long largerthanorequalto(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1&&NULL==_value2)return M_TRUE; // both NULL
	if(NULL==_value1||NULL==_value2)return(_value1!=NULL?M_FALSE:M_TRUE); // if _value1 is NULL, it is smaller, so false, otherwise _value2 is NULL and yes _value1 is larger
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL?strcmp(string(_value1text),string(_value2text)):(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result>=0?M_TRUE:M_FALSE);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)>=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long lllargerthanorequalto=(_biginteger1!=NULL&&_biginteger2!=NULL?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_LT?M_FALSE:M_TRUE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return lllargerthanorequalto;
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is larger than or equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalNegative(_rationalDifference));
				FREE_RATIONAL(_rationalDifference,owner);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=owned_decimal(_getValueDecimal(_value1,NULL),owner)
						,*_decimal2=owned_decimal(_getValueDecimal(_value2,NULL),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is larger than or equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalNegative(_decimalDifference));
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUEs when \p _value1 is larger than or equal to \p _value2 , M_FALSEs otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUEs when \p _value1 is larger than or equal to \p _value2 , M_FALSEs otherwise
 */
Mvalue* Mlargerthanorequalto(Mvalue* _value1,Mvalue* _value2){
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Mlargerthanorequalto,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Mlargerthanorequalto,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Mlargerthanorequalto,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Mlargerthanorequalto,false);
	return _getIntegerValue(largerthanorequalto(_value1,_value2));
}
/**
 * @brief return M_TRUE if \p _value1 is not equal to \p _value2 , M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long M_TRUE if \p _value1 is not equal to \p _value2 , M_FALSE otherwise
 */
static long long unequalto(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1&&NULL==_value2)return M_FALSE; // both NULL, so not unequal
	if(NULL==_value1||NULL==_value2)return M_TRUE; // not both NULL, so unequal
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL
								?strcmp(string(_value1text),string(_value2text))
								:(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result!=0?M_TRUE:M_FALSE);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)!=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long llunequalto=(_biginteger1!=NULL&&_biginteger2!=NULL
													?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_EQ?M_FALSE:M_TRUE)
													:M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return llunequalto;
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is not equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalZero(_rationalDifference)); 
				FREE_RATIONAL(_rationalDifference,owner);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=owned_decimal(_getValueDecimal(_value1,NULL),owner)
						,*_decimal2=owned_decimal(_getValueDecimal(_value2,NULL),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is not equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalZero(_decimalDifference));
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUEs if \p _value1 is not equal to \p _value2 , M_FALSEs otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUEs if \p _value1 is not equal to \p _value2 , M_FALSEs otherwise
 */
Mvalue* Munequalto(Mvalue* _value1,Mvalue* _value2){
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Munequalto,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Munequalto,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Munequalto,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Munequalto,false);
	return _getIntegerValue(unequalto(_value1,_value2));
}
/**
 * @brief returns M_TRUE if \p _value1 is equal to \p _value2 , M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long M_TRUE if \p _value1 is equal to \p _value2 , M_FALSE otherwise
 */
static long long equalto(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1&&NULL==_value2)return M_TRUE; // both NULL, so equal
	if(NULL==_value1||NULL==_value2)return M_FALSE; // either NULL but not both, definitely not equal
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL
								?strcmp(string(_value1text),string(_value2text))
								:(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result==0?M_TRUE:M_FALSE);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)==(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){		Mbiginteger* _equaltobiginteger=NULL;
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long llequalto=(_biginteger1!=NULL&&_biginteger2!=NULL
												?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_EQ?M_TRUE:M_FALSE)
												:M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return llequalto;
	}
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is equal to another rational: '",_rationalDifference,"'.\n");
				result=isRationalZero(_rationalDifference);
				FREE_RATIONAL(_rationalDifference,owner);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=owned_decimal(_getValueDecimal(_value1,NULL),owner)
						,*_decimal2=owned_decimal(_getValueDecimal(_value2,NULL),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is equal to another decimal: '",_decimalDifference,"'.\n");
				result=isDecimalZero(_decimalDifference);
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	/* see above
	// MDH@29OCT2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_text1=owned_string(_getValueText(_value1,true),owner),*_text2=owned_string(_getValueText(_value2,true),owner);
		// TODO should we care about the prefix?????
		long long result=(string_equal(_text1,_text2)?M_TRUE:M_FALSE);
		FREE_STRING(_text1,owner);FREE_STRING(_text2,owner);
		return _getIntegerValue(result);
	}
	*/
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUEs if \p _value1 is equal to \p _value2 , M_FALSEs otherwise
 * @details takes care of array and list arguments
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUEs if \p _value1 is equal to \p _value2 , M_FALSEs otherwise
 */
Mvalue* Mequalto(Mvalue* _value1,Mvalue* _value2){
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Mequalto,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Mequalto,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Mequalto,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Mequalto,false);
	return _getIntegerValue(equalto(_value1,_value2));
}

// MDH@21OCT2019: first comparison method dealing with decimals and rationals from which the rest was produced
/**
 * @brief returns M_TRUE if \p _value1 is smaller than or equal to \p _value2 , M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long M_TRUE if \p _value1 is smaller than or equal to \p _value2 , M_FALSE otherwise
 */
static long long smallerthanorequalto(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1&&NULL==_value2)return M_FALSE; // MDH@30MAR2023: OOPS forgotten earlier
	if(NULL==_value1||NULL==_value2)return(_value1!=NULL?M_FALSE:M_TRUE); // if at least one of them is NULL, if _value1 is, the result should be false, true otherwise
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),
						*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL
								?strcmp(string(_value1text),string(_value2text)):
								(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result<=0?M_TRUE:M_FALSE);
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)<=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long llsmallerthanorequalto=(_biginteger1!=NULL&&_biginteger2!=NULL
																		?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_GT?M_FALSE:M_TRUE)
																		:M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return llsmallerthanorequalto;
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is smaller than or equal to another rational: '",_rationalDifference,"'.\n");
				result=not(isRationalPositive(_rationalDifference)); // i.e. if difference is NOT positive, we should return M_TRUE
				FREE_RATIONAL(_rationalDifference,owner);
			}else 
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal *_decimal1=owned_decimal(_getValueDecimal(_value1,NULL),owner)
						,*_decimal2=owned_decimal(_getValueDecimal(_value2,NULL),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())outputDecimal("Difference in determining whether a decimal is smaller than or equal to another decimal: '",_decimalDifference,"'.\n");
				result=not(isDecimalPositive(_decimalDifference));
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUEs when \p _value1 is smaller than or equal to \p _value2 , M_FALSEs otherwise
 * @details takes care of list and array arguments
 * @param _value1 
 * @param _value2 
 * @return Mvalue* M_TRUEs when \p _value1 is smaller than or equal to \p _value2 , M_FALSEs otherwise
 */
Mvalue* Msmallerthanorequalto(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(_value1!=NULL&&_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Msmallerthanorequalto,false);
	if(_value1!=NULL&&_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Msmallerthanorequalto,false);
	if(_value2!=NULL&&_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,Msmallerthanorequalto,false);
	if(_value2!=NULL&&_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Msmallerthanorequalto,false);
	return _getIntegerValue(smallerthanorequalto(_value1,_value2));
}
// end comparison operator implementation

// MDH@01APR2020: we can make a list with intermediate values for multi-dimensional ranging by passing in the start list, the delta list and the end list each of which should have equal length
//				I guess we can pass in a count that tells us how many multidimensional points to return instead of the end 
/**
 * @brief returns the range of integer values starting with \p firstRangeValue and ending with \p lastRangeValue
 * 
 * @param firstRangeValue 
 * @param lastRangeValue 
 * @param up the direction set to true when \p firstRangeValue <= \p lastRangeValue , false otherwise
 * @return Mlist* he range of values starting with \p firstRangeValue and ending with \p lastRangeValue
 */
static Mlist* _getScalarRangeList(Mvalue* firstRangeValue,Mvalue* lastRangeValue, bool *up){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _scalarRangeList=(firstRangeValue!=NULL&&lastRangeValue!=NULL?owned_list(_getListOfType(VT_INTEGER),owner):NULL);
	if(_scalarRangeList!=NULL){
		long long direction=smallerthanorequalto(firstRangeValue,lastRangeValue);
		if(direction!=M_LL_INVALID){
			*up=(direction==M_TRUE);
			// if going up the first value is the ceil of _value1, otherwise it's the floor of _value1
			// I suppose there's no need to determine the last integer because we can use _value2 itself in the comparisons!!!
			Mvalue* firstIntegerRangeValue=(*up?Mceil(firstRangeValue):Mfloor(firstRangeValue)); // OOPS *up NOT up
			if(firstIntegerRangeValue!=NULL){
				long long rangeInteger=getValueInteger(firstIntegerRangeValue);
				if(rangeInteger!=M_LL_INVALID){
					Mvalue* integerrangeValue=_getIntegerValue(rangeInteger);
					if(integerrangeValue!=NULL){
						if(amVerbose()){
							Mvalue* lastIntegerRangeValue=(*up?Mfloor(lastRangeValue):Mceil(lastRangeValue));
							if(amDebugging()){
								outputValue("Determining the integers in [",integerrangeValue,",");outputValue(NULL,lastIntegerRangeValue,"].\n");
								if(inputCharReadFunction!=NULL){
									char c;output("%s...","Press Ctrl-C to stop or any other key to continue");(*inputCharReadFunction)(&c);if(c==3)return NULL;
								}
							}
						}
						Mvalue* inrangeValue;
						while(integerrangeValue!=NULL){
							// determine whether this value does not exceed the last value
							long long inrange=(*up?smallerthanorequalto(integerrangeValue,lastRangeValue):largerthanorequalto(integerrangeValue,lastRangeValue));
							if(inrange==M_LL_INVALID){outputError("Unable to determine whether the integer is inside the integer range");break;}
							if(inrange==M_FALSE)break; // not in range
							if(appendedToList(_scalarRangeList,owner,integerrangeValue,M_LL_INVALID)<=0)
							{FREE_LIST(_scalarRangeList,owner);_scalarRangeList=NULL;outputError("Failed to add an integer to an integer range");break;}
							// determine the next value to insert into the integer range
							if(*up)rangeInteger++;else rangeInteger--;
							integerrangeValue=_getIntegerValue(rangeInteger);
						}
					}else
						outputError("Failed to initialize the first candidate range integer");
				}else
					outputError("Failed to extract the lower bound of the integer range");
			}else
				outputError("Failed to determine the first integer range value");
		}else
			outputError("Unable to determine whether to go up or down in the integer range");
	}else
		outputError("Failed to create a list to store the integer range");
	return disowned_list(_scalarRangeList,owner);
}
// MDH@25NOV2020: preferable to store the range elements in an array
/**
 * @brief returns a M array containing the range of integer values starting at \p firstRangeValue and ending with \p lastRangeValue
 * 
 * @param firstRangeValue 
 * @param lastRangeValue 
 * @param up the returned direction, true when \p firstRangeValue <= \p lastRangeValue , false otherwise
 * @return Marray* the range of integer values starting at \p firstRangeValue and ending with \p lastRangeValue
 */
static Marray* _getScalarRangeArray(Mvalue* firstRangeValue,Mvalue* lastRangeValue, bool *up){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Marray* _scalarRangeArray=NULL; // can't create the array until we know how many values will be in it
	if(firstRangeValue!=NULL&&lastRangeValue!=NULL){
		long long direction=smallerthanorequalto(firstRangeValue,lastRangeValue);
		if(direction!=M_LL_INVALID){
			*up=(direction==M_TRUE);
			if(report)
			{outputValue("'",firstRangeValue,"' is ");output("%s",(*up?"smaller than or equal to":"larger than"));outputValue(" '",lastRangeValue,"'.\n");}
			// if going up the first value is the ceil of _value1, otherwise it's the floor of _value1
			// I suppose there's no need to determine the last integer because we can use _value2 itself in the comparisons!!!
			Mvalue* firstIntegerRangeValue=(*up?Mceil(firstRangeValue):Mfloor(firstRangeValue));
			if(firstIntegerRangeValue!=NULL){
				long long rangeInteger=getValueInteger(firstIntegerRangeValue);
				if(rangeInteger!=M_LL_INVALID){
					Mvalue* integerrangeValue=_getIntegerValue(rangeInteger);
					if(integerrangeValue!=NULL){
						Mvalue* lastIntegerRangeValue=(*up?Mfloor(lastRangeValue):Mceil(lastRangeValue));
						if(report){
							outputValue("Determining the integers in [",integerrangeValue,",");outputValue(NULL,lastIntegerRangeValue,"].\n");
							if(inputCharReadFunction){
								char c;output("%s...","Press Ctrl-C to stop or any other key to continue");(*inputCharReadFunction)(&c);if(c==3)return NULL;
							}
						}
						long long lastRangeInteger=getValueInteger(lastIntegerRangeValue);
						if(lastRangeInteger==M_LL_INVALID)return NULL;
						if(report)
							output("Determining all %s integers in [%lld,%lld].\n",(*up?"decreasing":"increasing"),rangeInteger,lastRangeInteger);
						unsigned long long arraylength=(*up
							?(lastRangeInteger>=rangeInteger?1+(lastRangeInteger-rangeInteger):0)
							:(rangeInteger>=lastRangeInteger?1+(rangeInteger-lastRangeInteger):0));
						_scalarRangeArray=owned_array(_getArray("_getScalarRangeArray",arraylength,NULL),owner);
						if(_scalarRangeArray!=NULL){
							if(arraylength>0){
								Mvalue** valueholder=_scalarRangeArray->values;
								while(integerrangeValue){
									assignValue(valueholder,integerrangeValue);
									if(--arraylength==0)break; // fail-safe to ascertain NOT to right beyond the end of the array
									valueholder++;
									// determine the next value to insert into the integer range
									if(*up)rangeInteger++;else rangeInteger--;
									integerrangeValue=_getIntegerValue(rangeInteger);
								}
							}
						}else 
							outputError("Failed to create the range array");
					}else
						outputError("Failed to initialize the first candidate range integer");
				}else
					outputError("Failed to extract the lower bound of the integer range");
			}else
				outputError("Failed to determine the first integer range value");
		}else
			outputError("Unable to determine whether to go up or down in the integer range");
	}else
		outputError("Start or end of range not defined");
	return(_scalarRangeArray?disowned_array(_scalarRangeArray,owner):NULL);
}

// MDH@18OCT2019: we can get the range of integers between two values
/**
 * @brief returns the range of integer values from \p _value1 to \p _value2
 * @details both arguments must be not NULL
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the range of integer values from \p _value1 to \p _value2
 */
Mvalue* Mrange(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_MAP||_value2->type==VT_MAP)return NULL; // neither operand can be a map for sure
	if(_value1->type==VT_REFERENCE||_value2->type==VT_REFERENCE)return NULL; // neither operand can be a reference for sure
	if(_value1->type==VT_FUNCTION||_value2->type==VT_FUNCTION)return NULL; // neither operand can be a function for sure
	if(_value1->type==VT_ENVIRONMENT||_value2->type==VT_ENVIRONMENT)return NULL; // neither operand can be a environment for sure
	// MDH@01APR2020: in the past we could use a list as first argument and as second argument and get the same result i.e. 1:[10,10] ===[1,1]:10 -> [[1,...,10],[1,...,10]]
	//				but now we allow multi-dimensional ranges for all calls that have a list as first argument, and getRangeList is used to get the multi-dimensional points
	//				I suppose we can stick to the original approach if there are less than 2 elements in the list
	bool up;
	if(_value1->type==VT_LIST){
		if(NULL==_value1->value._list||_value1->value._list->numberOfElements<2)return _appliedToList(_value1->value._list,_value2,Mrange,false);

		// with at least two elements in the list we could use the second argument as the count if it is not a list, this would give us additional functionality
		// because normally we would expect value2 to be an end point somehow and therefore a list
		Mlistelement* endIntegerRangeListelement=(_value2->type==VT_LIST?_value2->value._list->_first:NULL);
		Mvalue* endIntegerRangeValue=(_value2->type==VT_LIST?(endIntegerRangeListelement?endIntegerRangeListelement->_value:NULL):_value2);
		if(NULL==endIntegerRangeValue)return NULL; // we need a end value (whether from a scalar or from a list)
		
		Mlistelement* startIntegerRangeListelement=_value1->value._list->_first;
		Mvalue* startIntegerRangeValue=startIntegerRangeListelement->_value;
		if(NULL==startIntegerRangeValue)return NULL;

		Mlist* _integerRangeList=owned_list(_getScalarRangeList(startIntegerRangeValue,endIntegerRangeValue,&up),owner);
		if(NULL==_integerRangeList||NULL==_integerRangeList->_first)return NULL; // if undefined or empty apparently no integers between the start and end of the first dimensions

		if(amVerboseDebugging())
			outputList("First scalar range: ",_integerRangeList,".\n");

		Mvalue* rangeValue=subtract(endIntegerRangeValue,startIntegerRangeValue); // the total range in the first dimension
		// the first integer range list tells us how many elements we need to create for successive elements
		Mvalue *firstIntegerRangeValue=_integerRangeList->_first->_value
					,*lastIntegerRangeValue=_integerRangeList->_last->_value;
		Mvalue *startDeltaValue=subtract(firstIntegerRangeValue,startIntegerRangeValue)
					,*endDeltaValue=subtract(endIntegerRangeValue,lastIntegerRangeValue);

		// so we either have rangeValue=startDeltaValue+1+...+1+endDelta when up is true or rangeValue=endDelta+-1+...+-1+startDelta when up is false

		// the multiplication factor (deltato use in each successive dimension equals the difference between end and start value divided by rangeValue

		Mlist* _multFactorList=owned_list(_getListOfType(VT_UNDEFINED),owner);		
		if(NULL==_multFactorList){FREE_LIST(_integerRangeList,owner);return NULL;} // MDH@17JUN2020: free the integer range list please...

		// iterate over all successive elements in the _value1 list
		while(1){
			startIntegerRangeListelement=startIntegerRangeListelement->_next;
			if(NULL==startIntegerRangeListelement)break;
			Mvalue* startIntegerRangeValue=startIntegerRangeListelement->_value;
			if(NULL==startIntegerRangeValue)continue; // skip whatever is not present
			// in the _value2 'list' (if any) get the next end value
			if(endIntegerRangeListelement!=NULL){
				endIntegerRangeListelement=endIntegerRangeListelement->_next;
				if(endIntegerRangeListelement!=NULL)endIntegerRangeValue=endIntegerRangeListelement->_value;
			}
			// we need to compute the delta (step) 
			Mvalue* deltaRangeValue=divide(subtract(endIntegerRangeValue,startIntegerRangeValue),rangeValue);
			if(NULL==deltaRangeValue)continue;
			if(appendedToList(_multFactorList,owner,deltaRangeValue,M_LL_INVALID)<=0){
				FREE_LIST(_multFactorList,owner);
				_multFactorList=NULL;
				break;
			}
		}

		if(amVerboseDebugging())
			outputList("Multiplicators: ",_multFactorList,".\n");

		Mlist* _resultList=NULL;
		if(_multFactorList!=NULL){
			// now we have multiplication factors we can determine the values in the subsequent dimensions
			if(_multFactorList->numberOfElements>0){
				// initialize the start integer range start and end list element
				// endIntegerRangeListelement=(_value2->type==VT_LIST?_value2->value._list->_first:NULL);
				_resultList=owned_list(_getListOfType(VT_UNDEFINED),owner);
				if(_resultList!=NULL){
					// iterating over all elements in _integerRangeList
					Mvalue *firstRangeValue=startDeltaValue,*incrementValue=_getIntegerValue(1); // MDH@03MAR2020: no need to use assign here because firstRangeValue is temporary
					Mlistelement* _integerRangeListelement=_integerRangeList->_first;
					while(_integerRangeListelement!=NULL){
						Mlist* _pointList=owned_list(_getListOfType(VT_UNDEFINED),owner);
						if(NULL==_pointList){FREE_LIST(_resultList,owner);_resultList=NULL;break;}
						if(appendedToList(_pointList,owner,_integerRangeListelement->_value,M_LL_INVALID)<=0)
						{FREE_LIST(_resultList,owner);_resultList=NULL;break;}
						// now to compute the points in all other dimensions which means we have to increment startIntegerRangeListelement and endIntegerRangeListelement
						Mlistelement* multFactorListelement=_multFactorList->_first;
						Mvalue* rangeValue;
						// outputValue("Increment: ",incrementValue,".\n");
						startIntegerRangeListelement=_value1->value._list->_first;
						while(startIntegerRangeListelement->_next!=NULL){
							startIntegerRangeListelement=startIntegerRangeListelement->_next;
							startIntegerRangeValue=startIntegerRangeListelement->_value; // should always be there
							/* no need to know the end of the range because we know the multiplication factor (i.e. slope)
							if(endIntegerRangeListelement)endIntegerRangeListelement=endIntegerRangeListelement->_next;
							endIntegerRangeValue=(endIntegerRangeListelement?endIntegerRangeListelement->_value:_value2);
							*/
							// with startIntegerRangeValue and endIntegerRangeValue we should be able to compute the value to add (which also depends on the index count)
							// outputValue("First range value ",firstRangeValue,".\n");
							rangeValue=add(startIntegerRangeValue,multiply(multFactorListelement->_value,firstRangeValue));
							// outputValue("Range value: ",rangeValue,".\n");
							if(appendedToList(_pointList,owner,rangeValue,M_LL_INVALID)<=0)
							{FREE_LIST(_resultList,owner);_resultList=NULL;break;}
							multFactorListelement=multFactorListelement->_next;
						}
						if(NULL==_resultList)break;
						// append _pointList to the result list
						if(appendedToList(_resultList,owner,_getValueOfList(disowned_list(_pointList,owner)),M_LL_INVALID)<=0)
						{FREE_LIST(_resultList,owner);_resultList=NULL;break;}
						_integerRangeListelement=_integerRangeListelement->_next;
						firstRangeValue=add(firstRangeValue,incrementValue); // increment the first range value (which is the X offset so to speak from the first dimension)
					}
				}
				FREE_LIST(_integerRangeList,owner);
			}else
				_resultList=_integerRangeList;
			FREE_LIST(_multFactorList,owner);
		}
		return _getValueOfList(disowned_list(_resultList,owner));
		// replacing: return _appliedToList(_value1->value._list,_value2,Mrange);
	}
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Mrange,false);
	// now we're dealing with scalars
	return _getValueOfArray(_getScalarRangeArray(_value1,_value2,&up));
	// replacing: return _getValueOfList(_getScalarRangeList(_value1,_value2,&up));
	// it depends on whether _value1 is smaller than _value2 whether we'll be going up or down
}
/**
 * @brief returns the result of applying binary operator \p operator to \p _value1 and \p _value2
 * @param operator
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the result of applying binary operator \p operator to \p _value1 and \p _value2
 */
Mvalue* applyBinaryOperator(char* operator,Mvalue* _value1,Mvalue* _value2){
	Mvalue* result=NULL;
	if(_value1!=NULL&&_value2!=NULL){
		if(amVerbose()&&amDebugging())
		{outputValue("Computing '",_value1,NULL);output("' %s '",operator);outputValue(NULL,_value2,"'.\n");}
		switch(operator[0]){
			// real arithmetic
			case '+' :result=add(_value1,_value2);break;
			case '-' :result=subtract(_value1,_value2);break;
			case '*' :result=(strlen(operator)-1?power(_value1,_value2):multiply(_value1,_value2));break;
			case 'e' :result=epower(_value1,_value2);break;
			case '/' :result=(strlen(operator)-1?integerdivide(_value1,_value2):divide(_value1,_value2));break;
			case '\\':result=integerdivide(_value1,_value2);break;
			case '%' :result=divideremainder(_value1,_value2);break;
			// integer arithmetic
			case '^' :result=bitwisexor(_value1,_value2);break;
			case '&' :result=(strlen(operator)-1?logicaland(_value1,_value2):bitwiseand(_value1,_value2));break;
			case '|' :result=(strlen(operator)-1?logicalor(_value1,_value2):bitwiseor(_value1,_value2));break;
			// comparison operators
			case '<' :result=(strlen(operator)-1?(operator[1]=='<'?Mshiftleft(_value1,_value2):Msmallerthanorequalto(_value1,_value2)):Msmallerthan(_value1,_value2));break;
			case '>' :result=(strlen(operator)-1?(operator[1]=='>'?shiftright(_value1,_value2):Mlargerthanorequalto(_value1,_value2)):Mlargerthan(_value1,_value2));break;
			case '!' :result=Munequalto(_value1,_value2);break;
			case '=' :result=Mequalto(_value1,_value2);break;
			case ':' :result=Mrange(_value1,_value2);break; // MDH@18OCT2019: added the 'range' binary operator to generate a list with all integers between _value1 and _value2
			default:output("%sUnknown binary operator '%s'.\n",M_ERROR_PREFIX,operator);
		}
		if(amVerboseDebugging())
			{if(result)outputValue("Result of applying binary operator: '",result,"'.\n");else outputInfo("No result!");}
	}
	return result;
}
// MDH@14OCT2019: using (almost the) same precedence as used in C (except I have power operators as well ** and e)
/**
 * @brief returns the precedence value of operator \p operator
 * 
 * @param operator 
 * @return char the precedence value of operator \p operator
 */
char getOperatorPrecedence(Mstring* operator){
	if(operator){
		char c;
		switch(c=string_char(operator,0)){
			// real arithmetic
			case 'e' :return 11;
			case '*' :return 9+string_length(operator); // ** has precedence 11, * 10
			case '\\':
			case '%' :
			case '/' :return 10;
			case '+':
			case '-' :return 9;
			case '>' :
			case '<' :return (c==string_char(operator,1)?8:7); // shift operators have precedence 8, comparison operators (<, <=, > and >=) 7
			// bitwise (or logical) operators
			case '^' :return 4;
			case '&' :return (string_char(operator,1)?2:5);
			case '|' :return (string_char(operator,1)?1:3);
			case ':' : // MDH@18OCT2019: lowest priority right now but have to check on this!!!!
			// not-equal/equal operator
			case '!' :
			case '=' :return 6;
		}
	}
	return 0;
}
/**
 * @brief for storing formula elements
 * 
 */
typedef struct Mformulaelement{
	Mvaluereference* _operand; // an operand to apply the binary operator to
	Mstring* _operator; // a (shortcut) binary operator 
	struct Mformulaelement* _next;
	struct Mformulaelement* _prev; // MDH@21MAY2019: unfortunately needed for moving back!!
}Mformulaelement;
/**
 * @brief returns a new formula element
 * 
 * @param source 
 * @return Mformulaelement* a new formula element
 */
Mformulaelement* __formulaelement(char* source){Mallocationowner owner=getOwner(__LINE__);
	// output("BEFORE FORMULA ELEMENT ALLOCATION MARKS:\n");outputAllocationTypeMarks();
	Mformulaelement* _formulaelement=CALLOC_1(sizeof(Mformulaelement),'4',owner);
	/*
	if(_formulaelement){
		output("FORMULA ELEMENT %s ALLOCATED: %zd:%zd.\n",source,getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));
	}else
		output("%sFailed to create formula element '%s'.\n",M_ERROR_PREFIX,source);
	*/
	return DISOWNED(_formulaelement,owner);
}
/**
 * @brief frees formula element \p _formulaelement
 * 
 * @param _formulaelement 
 * @param owner_formulaelement 
 * @return size_t the number of formula elements freed
 */
size_t free_formulaelement(Mformulaelement* _formulaelement,Mallocationowner owner_formulaelement){
	// return the total number of formula elements freed
	size_t result=0;
	if(_formulaelement!=NULL){
		if(_formulaelement->_next!=NULL)result+=free_formulaelement(_formulaelement->_next,owner_formulaelement);
		if(_formulaelement->_operator!=NULL)FREE_STRING(_formulaelement->_operator,owner_formulaelement);
		if(_formulaelement->_operand!=NULL)FREE_VALUEREFERENCE(_formulaelement->_operand,owner_formulaelement);
		FREE_DISOWNED_1(_formulaelement,'4',owner_formulaelement);
		result+=1; // another one
		// output("FORMULA ELEMENT FREED: %zd:%zd.\n",getAllocationTypeOccupied('4',0),getAllocationTypeFreed('4',0));
		// outputChar('.');
	}
	return result;
}
/* any formula starts with
typedef struct Mformula{
	Mvaluereference* _operand;
	Mformulaelement* _next;
}Mformula;
*/
// once we have constructed a formula it needs to be computed

/* MDH@12JUL2019: we need the significant part of the token text
char* getSignificantTokenText(Mtoken* token){
	Mstring* tokenText=token->text; // get a reference to the token's text
	uint32_t tokenTextLength;char firstInsignificantTokenCharacter;
	if(token->significantCharacterCount){
		tokenTextLength=string_length(tokenText);
		firstInsignificantTokenCharacter=string_char(tokenText,token->significantCharacterCount);
		if(!string_shorten(tokenText,expressionToken->significantCharacterCount)){
			outputError("Failed to shorten token text");
			return NULL;
		}
	}
	// we can't call string() as that will write the '\0' overwriting the character we need back
	char* significantTokenText=string(tokenText);
	if(token->significantCharacterCount){
		if(!string_setlength(tokenText,tokenTextLength)||!string_setchar(tokenText,firstInsignificantTokenCharacter,token->significantCharacterCount)){
			// this would be very serious but also very unlikely because we're resetting the length, and writing a character in front of that length
			output("BUG: Failed to restore the token text.\n"); 
			significantTokenText=NULL;
		}
	}
	return significantTokenText;
}
*/
/**
 * an expression is the top-level element of the M language hierarchy
 * which optionally starts with an assignment to a single variable, BUT it makes sense to allow for multiple assignments in a row?????
 * typically this assignee can be composite referencing indices or attributes in maps, obviously we can put this variable in some sort of structure
 * it composes a list of value references to which operators are to be applied
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
/**
 * @brief returns the result of the evaluation of the expression being evaluated
 * 
 * @param info 
 * @param resulttype 
 * @param endTokenTypes 
 * @param endTokenTypeCount 
 * @return Mvalue* the result of the evaluation of the expression being evaluated
 */
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount){Mallocationowner owner=getOwner(__LINE__);

	Mvalue* _expressionValue=NULL;

	Mtoken* expressionToken=getEnvironmentExpressionToken();
	// typically the offset token determines what the expression ends with!!
	// e.g. ( ends with , or )	[ ends with ]	 { ends with }	etc.   
	/////////_expressionvalue->_valuereference=(Mvaluereference*)calloc(1,sizeof(Mvaluereference)); // create a value reference that is to hold a single value reference as result
	
	if(expressionToken!=NULL){
		if(amVerboseDebugging()){
			output("getValueOfExpression() interpreting %s expression starting with token '%s' of type '%s'",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			if(endTokenTypeCount){
				output(" that ends");
				uint8_t endTokenTypeIndex=0;
				while(endTokenTypeIndex<endTokenTypeCount){output(endTokenTypeIndex?" or ":" with ");output(TOKENTYPE_STRING[endTokenTypes[endTokenTypeIndex++]]);}
			}
			output(".\n");
		}
		
		///////////output("Number of allocated formula elements before: %zd.\n",getAllocationTypeCount('4'));

		Mvaluereference* _valuereference;
		Mformulaelement* formula=(Mformulaelement*)OWNED(__formulaelement("root"),owner); // replacing: CALLOC_1(sizeof(Mformulaelement),'4');
		Mformulaelement* _formulaelement=formula;
		size_t formulaElementCount=(_formulaelement!=NULL?1:0);

		int8_t endTokenTypeIndex; // max. 127 token types should suffice!!!

		while(expressionToken!=NULL){
			
			if(amVerboseDebugging())
				output("getValueOfExpression() processing %s expression token '%s' of type %s.\n",info,string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
			// does this token end the expression????
			endTokenTypeIndex=endTokenTypeCount;
			// OOPS operators shouldn't break here (and end the expression)
			while(endTokenTypeIndex>0&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1])endTokenTypeIndex--; // replacing: &&expressionToken->type>=8)endTokenTypeIndex--;
			if(endTokenTypeIndex>0){
				if(amVerboseDebugging())
					output("End of %s expression.\n",info);
				break;
			}
			
			if(_formulaelement!=NULL){
				// MDH@09NOV2022 NOTE: this is actually the only place where _getValueReference is getting called and may be confused with _getValuereference TODO
				_formulaelement->_operand=owned_valuereference(_getValueReference("operand",endTokenTypes,endTokenTypeCount),Msubowner(owner,1)); // MDH@08JUN2020: whatever we bind in the formula element needs to be subowned by it
				expressionToken=getEnvironmentExpressionToken(); // essential after calling a function that might advance the current expression token
				if(amVerboseDebugging())
					outputValue("Operand: ",getReferencedValue(_formulaelement->_operand),"'.\n");
			}

			// the next token(s) should be a binary operator
			// MDH@14NOV2019: we now also allow continued indexing i.e. an operand (value) that evaluates somehow to a list or map
			//				which means that what follows would be another list that should be appended to the item id of the value reference
			//				this can be done any number of times

			// NOTE some binary operators are stored in a couple of tokens!!!
			if(expressionToken!=NULL){
				if(expressionToken->type==TT_END_OF_DQSTRING||expressionToken->type==TT_END_OF_SQSTRING)
					expressionToken=nextEnvironmentExpressionToken();
				// MDH@14NOV2019: this is the first possible place where we should be aware of further indexing
				//				TODO alternatively we could move this functionality to getValueReference()!!
				//				TODO this also means that we can have an index on a value (not per se a variable)
				//				TODO are we allowing indexing strings as well??????
				/* MDH@17NOV2019: moved over to _getValueReference() where it actually belongs
				if(amVerbose()&&amDebugging())
				{output("Possible augmented list item ids expression token");outputToken(expressionToken);outputChar('\n');}
				while(expressionToken&&expressionToken->type==TT_LIST){
					if(amVerbose()&&amDebugging())
					{output("First augmented item id(s) token");outputToken(expressionToken);outputChar('\n');}
					Mvalue* indexListValue=getValueOfList(TT_END_OF_LIST,0,0,false);
					if(indexListValue&&indexListValue->type==VT_LIST&&indexListValue->value._list){
						// we should append the indices to the index_id
						Mvaluereference* operandValueReference=_formulaelement->_operand;
						if(operandValueReference->_itemid){ // there are already indices defined, so we should append the additional list items
							Mlist* itemIdsList=(operandValueReference->_itemid->type==VT_LIST?operandValueReference->_itemid->value._list:NULL);
							if(itemIdsList){
								Mlist* newItemIdsList=indexListValue->value._list;
								Mlistelement* newItemIdListElement=newItemIdsList->_first;
								while(newItemIdListElement){
									if(!appendedToList(itemIdsList,newItemIdListElement->_value,M_LL_INVALID))
										outputError("Failed to append augmented item id.");
									newItemIdListElement=newItemIdListElement->_next;
								}
							}else 
								outputBug("Item ids not a list.");
							// indexListValue will be removed by the garbage collector
						}else // no item id yet, so the same way as is done before set _itemid to the index list value
							assignValue(&operandValueReference->_itemid,indexListValue);
						if(amVerbose()&&amDebugging())
							outputValue("Augmented item ids: ",operandValueReference->_itemid,".\n");
					}
					expressionToken=nextEnvironmentExpressionToken();
				}
				*/
			}

			// MDH@16MAY2019: can't end an expression with an operator BRO'
			if(expressionToken!=NULL){
				if(amVerboseDebugging())
					output("Does '%s' of type '%s' end the expression? ",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
				endTokenTypeIndex=endTokenTypeCount;
				while(endTokenTypeIndex&&expressionToken->type!=endTokenTypes[endTokenTypeIndex-1]/*&&expressionToken->type>=8*/)endTokenTypeIndex--;
				if(endTokenTypeIndex){
					if(amVerboseDebugging())
						outputInfo("YES"); // replacing: output("Token '%s' of type %s ends the %s expression.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type],info);
					break;
				}
				if(amVerboseDebugging())
					outputInfo(" NO");

				if(amVerboseDebugging())
					output("Interpreting operator token '%s' of type '%s'.\n",string(expressionToken->text),TOKENTYPE_STRING[expressionToken->type]);
				// MDH@12JUL2019: 'remove' non-significant characters
				_formulaelement->_operator=owned_string(_getSignificantTokenText(expressionToken),Msubowner(owner,1)); // MDH@08JUN2020: take over ownership so we are allowed to free it // replacing: _stringCopy(expressionToken->text);
				if(NULL==_formulaelement->_operator){output("%sFailed to copy operator '%s'.\n",M_ERROR_PREFIX,string(expressionToken->text));break;}
				// MDH@12JUL2019 no need for this anymore: string_setlength(_formulaelement->_operator,expressionToken->significantCharacterCount); // cut off the nonsignificant stuff
				// append any other binary operator behind it (like a continuation or assignment operator)
				while(expressionToken->next!=NULL&&expressionToken->next->type>2&&expressionToken->next->type<=8){ // OOPS exclude unary operators AND allow for an assignment operator as well
					expressionToken=nextEnvironmentExpressionToken();
					string_append_char(_formulaelement->_operator,string_char(expressionToken->text,0)); // CHECK works for assignment operator but not per se for any operator!!!
				}
				if(expressionToken->next!=NULL&&expressionToken->next->type==TT_ASSIGNMENT){
					expressionToken=nextEnvironmentExpressionToken();
					string_append_char(_formulaelement->_operator,string_char(expressionToken->text,0)); // CHECK works for assignment operator but not per se for any operator!!!
				}
				if(amVerboseDebugging())
					output("Formula element operator: '%s'.\n",string(_formulaelement->_operator));
				_formulaelement->_next=OWNED(__formulaelement("successor"),owner); // MDH@08JUN2020: similar to all other formula elements this one needs to be owned by me as well otherwise I won't be able to free it myself
				_formulaelement=_formulaelement->_next;
				if(NULL==_formulaelement){
					outputError("Failed to create a new formula element.");
					break;
				}
				formulaElementCount++;
				expressionToken=nextEnvironmentExpressionToken();
			}else
			if(amVerboseDebugging())
				outputInfo("No further formula elements!");
		}

		// evaluate the formula
		if(formula!=NULL){

			if(amVerboseDebugging()){
				outputValue("First formula value: '",formula->_operand->_value,"'.\n");
				output("Number of formula elements: %zd.\n",formulaElementCount);
			}

			// skip all assignments
			// MDH@11AUG2019: how about creating ALL new variables IMMEDIATELY BEFORE evaluating the right-hand-side therefore allowing the use of these new variables in the right-hand-side in formulas as we have accepted??????
			//				the main advantage being that you can use it directly even in the same expression, so as such it won't harm and it has benefits e.g. you can use a local variable immediately
			uint16_t numberOfAssignments=0;
			Mformulaelement* _lastAssignmentFormulaelement=NULL;
			_formulaelement=formula;
			while(_formulaelement!=NULL){
				/////if(_formulaelement->_operator!=NULL){ // MDH@28JAN2024: no need to check null
					/////output("Checking formula operator '%s'.\n",string(_formulaelement->_operator));
					if(string_last_char(_formulaelement->_operator)!='=')break; // not ending with assignment operator character to start with
					// MDH@28JAN2024: oops <<= and >>= should be considered shortcut operators, so added the check for the length to equal 2
					//                TODO there might be shortcut operators that are getting through this way
					if(string_length(_formulaelement->_operator)==2&&(string_char(_formulaelement->_operator,0)=='<'||string_char(_formulaelement->_operator,0)=='>'||string_char(_formulaelement->_operator,0)=='!'))break; // break on <=, >= and !=
					if(string_length(_formulaelement->_operator)>1&&string_char(_formulaelement->_operator,0)=='=')break; // break on ==
					if(_lastAssignmentFormulaelement)_formulaelement->_prev=_lastAssignmentFormulaelement; // MDH@21MAY2019: in order to be able to traverse back!!!
					// MDH@11AUG2019: should we force existence?????? I don't think so because creation is done when the value reference is actually created, but I need to make certain that this is the case!!!!
					/*
					if(!addVariable(expressionToken->argument==1?NULL:getExecutionEnvironment(),_significantTokenText,VT_UNDEFINED,false)){
						Mstring* _environmentName=_getExecutionEnvironmentName();
						output("%sFailed to add%s variable '%s' to environment '%s'.\n",M_ERROR_PREFIX,(expressionToken->argument!=1&&expressionToken->envid?" implicitly declared local":""),_significantTokenText,string(_environmentName));
						FREE_STRING(_environmentName);
						break; // NO retrieves the undefined value subsequently!!
					}
					if(amVerbose())if(expressionToken->argument!=1&&expressionToken->envid)output("WARNING: Not explicitly declared local variable '%s' encountered.\n",_significantTokenText);
					*/
					_lastAssignmentFormulaelement=_formulaelement;
					numberOfAssignments++;
				///}
				_formulaelement=_formulaelement->_next;
			}
			if(amVerboseDebugging())
				output("Number of assignments: %u.\n",numberOfAssignments);
			
			unsigned long long allocated=getAllocationTypeOccupied('4',0),freed=getAllocationTypeFreed('4',0);
			if(amVerboseDebugging())
				output("Type '4' BEFORE: allocated: %llu - freed: %llu.\n",allocated,freed);

			// MDH@14OCT2019: applying binary operators typically is done taking operator precedence into account which means we cannot apply lower precedence binary operators until higher precedence binary operators are applied first
			//				which again means that you can apply an operator as soon as the next one does not have a higher priority which means that after applying the highest order operators we have apply the next highest order operator
			//				we always need to compare two successive operators if the precedence of the first is not below the precedence of the second you may apply the first operator, otherwise you skip applying the operator
			//				perhaps it's best to immediately consume formula elements we no longer need!!!!!
			Mvalue* _result=(_formulaelement->_next!=NULL?NULL:getReferencedValue(_formulaelement->_operand)); // bit of a nuisance though!!!
			Mformulaelement* nextformulaelement;
			char operatorprecedence,nextoperatorprecedence;
			while(_formulaelement->_next!=NULL){
				operatorprecedence=getOperatorPrecedence(_formulaelement->_operator);
				nextoperatorprecedence=getOperatorPrecedence(_formulaelement->_next->_operator);
				if(operatorprecedence>=nextoperatorprecedence){ // current operator has higher or the same precedence which means we can apply it
					_result=applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_formulaelement->_operand),getReferencedValue(_formulaelement->_next->_operand));
					// if we replace any value stored in the value reference of the first operand, we can reuse that formula element
					_formulaelement->_operand->_value=_result;
					// MDH@02NOV2019 replacing: assignValue(&(_formulaelement->_operand->_value),_result);
					// but because _result could be NULL we have to force _name to be NULL just in case 
					if(_formulaelement->_operand->_name)
					{FREECHARS(_formulaelement->_operand->_name,owner);_formulaelement->_operand->_name=NULL;}
					// we need to point the formula operand to the next of the consumed formula element, so the consumed formula element won't be used again in computations
					nextformulaelement=_formulaelement->_next;
					// point the formula element now storing the result to the next of the consumed formula element
					_formulaelement->_next=nextformulaelement->_next;
					// release the applied operator, and replace it by the successor operator
					// MDH@14MAY2020: if we make a copy of the next operator we can free the disconnected formula element entirely
					FREE_STRING(_formulaelement->_operator,owner); // TODO is owner correct?
					_formulaelement->_operator=owned_string(_getString(string(nextformulaelement->_operator)),owner); // MDH@09JUN2020: do NOT forget to obtain ownership of the next operator
					nextformulaelement->_next=NULL;free_formulaelement(nextformulaelement,owner); // NULL next of the nextformulaelement so it won't free all successive formula elements left to be applied
					formulaElementCount--;
					/* replacing:
					_formulaelement->operator=nextformulaelement->_operator;
					// can't reach the consumed formula element anymore, so release whatever it contains (except for the operator which we have retained)
					free_valuereference(nextformulaelement->_operand);// free the consumed operand
					nextformulaelement->_operand=NULL; // MDH@14MAY2020: it's prudent to NULL the pointer, so no-one will try to free the value reference again
					if(nextformulaelement){
						FREE_DISOWNED_1(nextformulaelement,'4'); // NOTE although it's operator is still pointing to something, it is still pointed to that Mstring (as we took that over), so it should NOT be released!!!!!!
						formulaElementCount--; // one less to free!!!
					}else
						outputInfo("No formula element to free!");
					*/
					// if we have a formula element behind us of which the operator has not yet been applied we go back there (because my operator has changed!!!!!)
					if(_formulaelement->_prev!=NULL)_formulaelement=_formulaelement->_prev;
					// is there a formula element in front of it that has not yet been applied?????
					if(amVerboseDebugging())
						outputValue("Value of expression: '",_result,"'.\n");
				}else{ // we have to apply the next operator BEFORE applying this operator
					_formulaelement->_next->_prev=_formulaelement; // point the next formula element to me, so it's knows that the operator behind it has not yet been applied
					_formulaelement=_formulaelement->_next; // skip applying the current operator for now
				}
			}
			/* replacing:
			Mvalue* _result=getReferencedValue(_formulaelement->_operand); // the first result computed
			if(amVerbose())outputValue("First result: '",_result,"'.\n");
			// 'applying' the binary operators left-to-right remembering the intermediate result in _result
			// NOTE because all formula-elements are freed afterwards (see below) there's no need to so while applying the binary operators
			while(_formulaelement->_next){ // a binary operator to apply
				if(amVerbose())output("Binary operator to apply: '%s'.\n",string(_formulaelement->_operator));
				_result=applyBinaryOperator(string(_formulaelement->_operator),_result,getReferencedValue(_formulaelement->_next->_operand));
				if(amVerbose())outputValue("Next result: '",_result,"'.\n");
				_formulaelement=_formulaelement->_next;
			}
			*/

			if(amVerboseDebugging())
				{
					outputValue("Result: '",_result,"'.\n");
					allocated=getAllocationTypeOccupied('4',0);
					freed=getAllocationTypeFreed('4',0);
					output("Type '4' AFTER: allocated: %zd - freed: %zd - left to free: %zd\n",allocated,freed,formulaElementCount);
				}

			// perform assignments right-to-left (which is a little problematic though)
			if(numberOfAssignments){
				if(amVerboseDebugging())
					output("Performing %u assignments.\n",numberOfAssignments);
				_formulaelement=_lastAssignmentFormulaelement;
				while(_formulaelement!=NULL){
					_valuereference=_formulaelement->_operand;
					if(amVerboseDebugging()){
						Mstring* _indexidText=owned_string(_getValueText(_valuereference->_itemid,false),owner);
						if(_indexidText!=NULL){
							output("Assignment to %s%s using operator %s!\n",_valuereference->_name,(_indexidText?string(_indexidText):""),string(_formulaelement->_operator));
							FREE_STRING(_indexidText,owner);
						}
					}
					string_shorten(_formulaelement->_operator,1); // cutting off the assignment operator is fine, as we do not need it anymore!!!
					if(string_length(_formulaelement->_operator)){ // _result will change due to applying the shortcut binary operator
						// we have to be a bit careful here if the value reference uses an index id
						if(amVerboseDebugging())
							output("Applying binary operator '%s'.\n",string(_formulaelement->_operator));
						// does the _value field already contain the current value of the variable, if so we may immediately use that here instead of getValue()
						_result=applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result);
						// MDH@02NOV2019 replacing: assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getReferencedValue(_valuereference),_result));
						// replacing:	assignValue(&_result,applyBinaryOperator(string(_formulaelement->_operator),getValue(_Menvironment,_valuereference->_name),_result));
					}
					if(amVerboseDebugging())outputValue("Result to store in the value reference: '",_result,"'.\n");
					
					setReferencedValue(_valuereference,Msubowner(owner,1),_result); // MDH@19JUN2020: should check whether or not we should pass the owner of the value reference
					
					// if(amVerboseDebugging())outputValue("###### Stored in the value reference: '",_valuereference->_value,"'.\n");
					
					Mvalue* referencedValue=getReferencedValue(_valuereference);
					
					// if(amVerboseDebugging())outputValue("###### Referenced value to use as result: '",referencedValue,"'.\n");
					
					// MDH@02NOV2019: we still didn't get a change to a list argument so here also we need to prevent copying the list/map
					_result=referencedValue; // MDH@02NOV2019: replacing: assignValue(&_result,referencedValue); // should we do this???? well, in case the assignment failed!!!
					/* replacing:
					if(_valuereference->_itemid){
						// TODO check whether all the items are of the right type!!!
						appendedToList(_valuereference->_value->value._list,_result,_valuereference->_itemid->value._integer->ll);
						// TODO typically the list will be mutable, but the point here is that we need the full index list to get the right value (which we didn't store!!!!)
					}else{
						setValue(_Menvironment,_valuereference->_name,_result);
						// use the current value of the variable assigned to as new result!!
						assignValue(&_result,getValue(_Menvironment,_valuereference->_name)); // CHECK assign??
					}
					*/
					///////////if(!(--numberOfAssignments))break; // no more assignments???
					_formulaelement=_formulaelement->_prev;
				}
			}

			// the expression value is the value of the first operand!!!
			if(amVerboseDebugging())
			{outputValue("Storing '",_result,"'");output(" as value of expression '%s'.\n",info);}
			
			_expressionValue=_result; // MDH@02NOV2019 replacing: assignValue(&_expressionValue,_result); // MDH@21MAY2019: this will increment the reference count of _result so it makes sense to actually decrement its reference count after being used

			// free the formula
			if(amVerboseDebugging())
				output("Freeing %zd formula elements.\n",formulaElementCount);
			
			// if(amVerbose())outputAllocationTypeMarks();

			// MDH@14MAY2020 think we shouldn't free formula actually as its pointer is passed to a formula element which is freed eventually:
			size_t numberOfFormulaElementsFreed=free_formulaelement(formula,owner);
			
			if(amVerboseDebugging())
				output("Number of formula elements freed: %zd.\n",numberOfFormulaElementsFreed);

			//*/
			/* replacing:
			Mformulaelement* _nextformulaelement;
			_formulaelement=formula;
			while(_formulaelement){
				FREE_STRING(_formulaelement->_operator);
				free_valuereference(_formulaelement->_operand);
				_nextformulaelement=_formulaelement->_next;
				FREE_DISOWNED_1(_formulaelement,'4'); // OOPS have to call FREE here not free()
				outputChar('.');
				_formulaelement=_nextformulaelement;
			}
			newline();
			*/
			if(amVerboseDebugging())
				outputInfo("Formula elements freed.");
		}else
		if(amVerboseDebugging())
			output("No result of expression '%s' to store.",info);
	}
	if(amVerboseDebugging())
	{output("'%s' expression evaluates to",info);outputValue(": '",_expressionValue,"'.\n");}
	return _expressionValue;
}

// MDH@22DEC2020: I can simplify the for loop by wrapping it inside an environment by being able to create one
//				on the fly with a local variable map similar to what the anonymous function does
//				thus effectively separating commands inside the block from the declaration of local variables
/**
 * @brief creates and returns a new (wrapped) environment initialized with variables defined in \p _localMapValue
 * 
 * @param _localMapValue 
 * @return Mvalue* a new (wrapped) environment initialized with variables defined in \p _localMapValue
 */
Mvalue* Mwith(Mvalue* _localMapValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	long long result=M_LL_INVALID;
	if(NULL==_localMapValue||_localMapValue->type==VT_MAP){
		result=M_FALSE;
		Mmap* localMap=(_localMapValue!=NULL?_localMapValue->value._map:NULL);
		if(NULL==localMap)
			outputWarning("No with (local) variables defined!");
		else
		if(localMap->numberOfElements==0)
			outputWarning("With map empty!");
		Menvironment* _withEnvironment=owned_environment(__environment(),owner);
		if(_withEnvironment!=NULL){
			if(report)
				output("With environment created.\n");
			result=pushInitializedEnvironment(_withEnvironment,owner,localMap);
			/* replacing: 
			Mmapelement* withNameMapelement=(localMap?getMapelement(localMap,"."):NULL);
			Mvariable* withNameVariable=(withNameMapelement!=NULL?withNameMapelement->_variable:NULL);
			Mstring* _withNameText=(withNameVariable!=NULL?owned_string(_getValueText(withNameVariable->_value,true),owner):NULL);
			// if a property "." is defined, it's text value will be the name of the with environment
			_withEnvironment->_name=owned_chars(_getChars((_withNameText?string(_withNameText):"")),Msubowner(owner,1));
			// copy local map
			if(localMap!=NULL)_withEnvironment->_variableMap=owned_map(_getMapCopy(localMap),Msubowner(owner,1));
			if(NULL==localMap||_withEnvironment->_variableMap){
				if(report)
					output("Local variables map registered.\n");
				if(NULL==withNameMapelement||removedFromMap(_withEnvironment->_variableMap,owner,".")==M_TRUE){
					// almost there
					if(!pushExecutionEnvironment(disowned_environment(_withEnvironment,owner))){ // _withEnvironment not bound!!!
						free_environment(_withEnvironment);_withEnvironment=NULL;
						output("%sFailed to register %senvironment",M_ERROR_PREFIX,(_withNameText?"":"the with "));
						if(_withNameText!=NULL)output(" '%s'",string(_withNameText));
						output(".\n");
					}else{
						result=M_TRUE;
						if(report)
							output("With environment activated.\n");
					}
				}else
				if(withNameMapelement!=NULL)
					outputError("Failed to remove the with environment name from the local variables map");	
			}
			if(result==M_FALSE&&_withEnvironment!=NULL){
				if(report)
					output("Freeing the with environment!");
				FREE_ENVIRONMENT(_withEnvironment,owner);
			}
			/// replacing:
			///Mmapelement* withVariableMapelement=localMap->_first;
			///while(withVariableMapelement){
			///	withNameVariable=withVariableMapelement->_variable;
			///	if(withNameVariable){
			///		Mchars* withlocalVariableName=withNameVariable->_name;
			///		if(withlocalVariableName->chars){
			///			if(!addVariable(_withEnvironment,owner,withlocalVariableName,));
			///			withVariableMapelement=withVariableMapelement->_next;
			///		}
			///	}
			///}
			///if(!withVariableMapelement){ // with environment successfully initialized
			///}else{
			///	output("%sFailed to initialize %senvironment",M_ERROR_PREFIX,(_withNameText?"":"the with "));
			///	if(_withNameText)output(" '%s'",string(_withNameText));
			///	output(".\n");
			///}
			if(_withNameText!=NULL)FREE_STRING(_withNameText,owner);
			*/
		}
	}else
		outputError("With argument not a (local variable) map!");
	return _getIntegerValue(result);
}
// MDH@23DEC2020: you can decide now to return whatever you want (from the current environment)
//				but if the value is NULL the default i.e. the entire variable map is returned
/**
 * @brief ends a with environment returning either \p _returnValue or, by default, the with environment variable map
 * @details returns \p _returnValue when \p _returnValue is not NULL
 * @param _returnValue the value to return instead of the with environment variable map
 * @return Mvalue* the return value
 */
Mvalue* Mendwith(Mvalue* _returnValue){Mallocationowner owner=getOwner(__LINE__);
	// let's make endwith return a copy of the variable map of the environment we're going to pop!!!
	// TODO we should check whether there's a with environment active!!!!!!!!
	if(NULL==getEnvironmentParent(getExecutionEnvironment())){
		outputError("No (with) environment to end.");
		return NULL;
	}
	Mmap* _resultMap=NULL;
	if(_returnValue==NULL){ // no return value specified
		_resultMap=owned_map(_getMapCopy(getExecutionEnvironment()->_variableMap),owner);
		if(NULL==_resultMap)
			outputError("Failed to return the with environment variable map");
	}
	popExecutionEnvironment();
	return(_resultMap!=NULL?_getValueOfMap(disowned_map(_resultMap,owner)):_returnValue);
}

// MDH@02APR2024: Mend is called when entering a block of commands is to be ended
/**
 * @brief ends a block of commands by popping the current execution environment, similar to what Mendwith does
 * 
 * @param _returnValue 
 * @return Mvalue* 
 */
Mvalue* Mend(Mvalue* _returnValue){
	return Mendwith(_returnValue);
}

// the functions to create functions are moved here from Menvironment.h/c because they require parsing the command texts
// MDH@04MAR2020: user functions now no longer need a internal name (but are typically assigned to a variable, so they can be)
//				so these are actually anonymous functions
// MDH@25OCT2020: it's easier to let _bodyTokenValue not be an actual command but a list of commands (untokenized) i.e. texts
//				this makes sense when reading commands from a text file, and yes when defining a function we're NOT evaluating the commands yet
//				which would mean tokenize the commands and NOT execute them
// MDH@28OCT2020: the body token value should now be a list of texts where the escape character should be used as last character to indicate that the command continues on the next line
//				TODO it's better to first create all the required elements first, before parsing the body
// MDH@29OCT2020: any user function might not know all the names of the external variables, but it will always know the variables that it wants to use locally
//				ok it might also know which external variables it uses and the body won't parse if trying to access an external variable that does not exist
//				I'm wondering if the local map value should define initial values at all????
//				what if a person forget the local map value???? I guess we will just assume no local variables
/**
 * @brief returns a wrapped anonymous function with parameter map \p _parameterMapValue , local variables \p _localMapValue and body \p _bodyValue
 * 
 * @param _parameterMapValue 
 * @param _localMapValue 
 * @param _bodyValue 
 * @return Mvalue* the new anonymous function wrapper
 */
Mvalue* Manonymousfunction(Mvalue* _parameterMapValue,Mvalue* _localMapValue,Mvalue* _bodyValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging();
	Mvalue* _functionValue=NULL;
	// MDH@29OCT2020: to meet the user a little more we simply allow skipping maps so that the first argument that is a list is assumed to be the body
	//				alternatively we could change the order i.e. body first, then parameters, then local variables in which case it is easier to skip the body
	//				however, if the body is missing the parameters and/or local variables should still be there
	//				OK, we need two maps and one list
	Mmap *parameterMap=NULL,*localMap=NULL;
	Mlist* bodyCommandList=NULL;
	if(_parameterMapValue!=NULL&&_parameterMapValue->type==VT_LIST){
		bodyCommandList=_parameterMapValue->value._list;
	}else{ // first argument not a list, so should be a map (if defined)
		if(_parameterMapValue!=NULL){
			if(_parameterMapValue->type!=VT_MAP){outputError("Parameters of function not defined in a map");return NULL;}
			parameterMap=_parameterMapValue->value._map;
		}
		if(_localMapValue!=NULL&&_localMapValue->type==VT_LIST){
			bodyCommandList=_bodyValue->value._list;		
		}else{ // second argument not a list, so should be a map
			if(_localMapValue!=NULL){
				if(_localMapValue->type!=VT_MAP){outputError("Local variables of function not defined in a map");return NULL;}
				localMap=_localMapValue->value._map;
			}
			if(_bodyValue!=NULL){
				if(_bodyValue->type!=VT_LIST){outputError("Body of function not defined in a list");return NULL;}
				bodyCommandList=_bodyValue->value._list;
			}
		}
	}
	// ASSERT at this point all arguments are processed and accepted
	// if(amVerbose())outputValue("Anonymous function parameter map: ",_parameterMapValue,".\n");
	Muserfunction* _userfunction=(Muserfunction*)CALLOC_1(sizeof(Muserfunction),'U',Msubowner(owner,1)); // TODO check why I need to use U here
	if(_userfunction!=NULL){
		if(report)
		{outputMap("Processing the declaration of a function with parameters ",parameterMap," and ");outputMap("local variables ",localMap,".\n");}
		Mfunction* _function=(Mfunction*)CALLOC_1(sizeof(Mfunction),'F',owner); // TODO check why I need to use F here
		if(_function!=NULL){
			if(report)outputInfo("Function created.");

			// MDH@02MAR2020: the following is dangerous, because the value might be freed in which case the map would be freed as well!!!!
			//				so we have to make a copy of the parameter map
			if(parameterMap!=NULL)
				_function->_parameterMap=owned_map(_getMapCopy(parameterMap),Msubowner(owner,1)); // MDH@03MAR2020: making a copy of the map wrapped in the value passed in
			else
			if(report)outputInfo("No parameters registered.");

			// MDH@29OCT2020: the local map is stored with the user function (and not the function because system functions do not need explicitly defined local variables)
			if(localMap!=NULL)
				_userfunction->_localMap=owned_map(_getMapCopy(localMap),Msubowner(owner,2)); // MDH@03MAR2020: making a copy of the map wrapped in the value passed in
			else
			if(report)outputInfo("No local variables registered.");

			_function->functionunion._userfunction=_userfunction; // NOTE already owned at the right level

			// user function expects a list of commands, so we have to wrap the single token (if any)
			if(bodyCommandList!=NULL){
				if(report)output("Will process the body commands.\n");
				Mlistelement* bodyCommandListelement=(bodyCommandList?bodyCommandList->_first:NULL);
				if(bodyCommandListelement!=NULL){
					_userfunction->_bodyCommandList=owned_list(_getListOfType(VT_TOKEN),Msubowner(owner,2));
					if(_userfunction->_bodyCommandList!=NULL){
						Mcommand* _command=NULL; // we'll be using _command to determine afterwards whether or not we succeeded in parsing the body commands
						// the only way to successively parse the body commands is by creating a temporary environment that will expose the parameters as existing variables
						// so the code here was taken from getValueOfFunctionCall() but because this is an anonymous function we do not have a name yet
						// which obviously prevents recursive calls by name (we should find a way to make recursive calls in anonymous functions though)
						Mmap* _argumentMap=_getFunctionArgumentMap(_function,NULL,owner); // to obtain the defaults (although we don't need them) we simply pass NULL as argument list
						if(_argumentMap!=NULL){
							Menvironment* _functionExecutionEnvironment=owned_environment(_getFunctionExecutionEnvironment(_function,"",_argumentMap),owner);
							if(_functionExecutionEnvironment!=NULL){
								if(pushExecutionEnvironment(disowned_environment(_functionExecutionEnvironment,owner))){
									if(report)output("Ready to parse %zd body command lines.\n",bodyCommandList->numberOfElements);
									// which is similar to what _getValueOfFunctionCall does
									bool commandContinued;
									char inputChar,inputCharType;
									while(bodyCommandListelement!=NULL){
										// convert the body command to a text (to be tokenized)
										Mstring* _bodyCommandText=owned_string(_getValueText(bodyCommandListelement->_value,true),owner);
										// TODO should we simply skip the command?????
										if(_bodyCommandText!=NULL){
											char *bodyCommandCharacter=string(_bodyCommandText);
											// immediately determine whether this command is continued on the next command text
											// if it does cut off the continuation character as we do not consider it to be part of the actual command text
											// TODO how about if the escape character does not indicate a continuation?????? e.g. when used in a string literal
											//	  this actually means that we cannot enter a string literal over multiple lines
											commandContinued=(string_last_char(_bodyCommandText)==M_COMMAND_CONTINUATION_CHARACTER);
											if(commandContinued)string_setlength(_bodyCommandText,string_length(_bodyCommandText)-1);
											if(report)output("Characters of command text%s '%s' parsed: '",(_command?" continuation":""),string(_bodyCommandText));
											// ascertain to have a command (we will have one if this command text is considered a continuation of the command so far)
											if(NULL==_command)_command=owned_command(_getNewCommand(true),owner);
											// can't break here if the command is NULL because we haven't freed _bodyCommandText yet
											if(_command!=NULL){ // a command to parse into in which inputChar will always be set
												// ignore whitespace at the beginning of the command
												// MDH@28OCT2020 NOTE: following the same approach as used in Mevalfunction()
												Mtoken* lastCommandToken=_command->_firstToken;
												bool whitespace=true;
												while((inputChar=*bodyCommandCharacter)){
													inputCharType=INPUTCHARACTERTYPES[inputChar];
													// MDH@28OCT2020: we're not expecting any non-printable characters can also be present
													whitespace&=(inputCharType=='W'||inputChar<=32);
													if(!whitespace){
														lastCommandToken=commandCharacterAppended(_command/*,owner*/,inputChar,&inputCharType,false);
														if(NULL==lastCommandToken)break; // some error
														if(lastCommandToken!=_command->_lastToken){
															_command->_lastToken=owned_token(lastCommandToken,Msubowner(owner,1)); // do NOT forget to take over ownership
															if(report)outputChar('|');
														}
														if(report)outputChar(inputChar);
													}
													bodyCommandCharacter++; // advance the body command character pointer
												}
											}
											FREE_STRING(_bodyCommandText,owner);
											// if we either do not have a command, or inputChar is still nonzero
											if(NULL==_command){outputError("Failed to create a body command");break;}
											if(inputChar){outputError("Failed to parse a body command");break;}
											if(!commandContinued){ // command not continued on the next line, therefore we should register the command
												// if the command is somehow invalid we should abort, and discard the result, this is done by ascertaining tokenValue to be NULL
												bool aValidCommandIndicator=isAValidCommandIndicator(_command/*,owner*/,false);
												// 0 means an empty command (e.g. a comment)
												if(aValidCommandIndicator!=0){
													Mvalue* tokenValue=(aValidCommandIndicator>0?_getValueOfToken(_command->_firstToken):NULL);
													// if the command is bound i.e. tokenValue is not NULL ascertain that the tokens will not be freed when freeing the command (below)
													if(tokenValue!=NULL)_command->_firstToken=NULL;
													if(tokenValue!=NULL&&appendedToList(_userfunction->_bodyCommandList,owner,tokenValue,M_LL_INVALID)<=0)tokenValue=NULL; // by doing this, after freeing the command below, we'll break and _command will be NULL and recognized as error below
													// we need to free the command anyway, to ascertain that the next command text will start with a new command altogether
													if(NULL==tokenValue){outputError("Failed to store the body command!");break;} // storing the command somehow failed, therefore _command will not be NULL and therefore indicate erroneous body command parsing
												}
												// prepare for parsing the next command text
												FREE_COMMAND(_command,owner);_command=NULL;
											}
											if(report)output("'.\n");
										}
										bodyCommandListelement=bodyCommandListelement->_next;
									}
									popExecutionEnvironment();
									_functionExecutionEnvironment=NULL;
									// if parsing somehow failed, get rid of the body
								}
							}
							// free the execution environment, if either failing to push or pop the execution environment
							if(_functionExecutionEnvironment!=NULL)FREE_ENVIRONMENT(_functionExecutionEnvironment,owner);
							if(_argumentMap!=NULL)FREE_MAP(_argumentMap,owner);
							// if _command is not currently defined parsing and storing the body commands failed somehow!!
							// OOPS that's not true because after registration of a command, the command is NULLed, so I guess that if there's a pending command something went wrong
							if(_command!=NULL){
								FREE_COMMAND(_command,owner);
								FREE_LIST(_userfunction->_bodyCommandList,owner);
								_userfunction->_bodyCommandList=NULL;
								outputWarning("Function body discarded because of parsing errors");
							}else
							if(report)output("Number of commands in the body: %zd.\n",_userfunction->_bodyCommandList->numberOfElements);
						}else
							outputError("Failed to initialize the argument and local variables map");
					}else
						outputError("Failed to store the inline command as body of an anonymous function");
				}else
					outputWarning("No commands in body");
			// replacing: assignValue(&_userfunction->_bodyTokenValue,_bodyTokenValue);
			}
			// return the result of applying the function to the default parameter map NO NO NO the function wrapped in a value
			_functionValue=_getValueOfFunction(disowned_function(_function,owner));
			if(report)outputInfo("Anonymous function value wrapped");
		}else{
			outputError("Failed to create an anonymous function");
			FREE_USERFUNCTION(_userfunction,owner);
		}
	}else
		outputError("Failed to create the anonymous user function");

	if(NULL==_functionValue)
		outputError("Failed to create an anonymous function");
	else
	if(report)outputInfo("Anonymous function created!");
	return _functionValue;
}
// might make the following obsolete (defun)
/**
 * @brief returns a new function with name \p _nameValue , parameter map \p _parameterMapValue and body \p _bodyTokenValue
 * 
 * @param _nameValue 
 * @param _parameterMapValue 
 * @param _bodyTokenValue 
 * @return Mvalue* a new function with name \p _nameValue , parameter map \p _parameterMapValue and body \p _bodyTokenValue
 */
Mvalue* Mdefinefunction(Mvalue* _nameValue,Mvalue* _parameterMapValue,Mvalue* _bodyTokenValue){Mallocationowner owner=getOwner(__LINE__);
	// the user specifies the body as a text (to prevent evaluation during defining the function)
	// but perhaps it could also be a list of tokens????? i.e. already tokenized (that is not evaluated)
	// of course, tokenizing is a problem later on, but this means that we need to prevent evaluation of the second argument before calling this function on it
	if(_nameValue!=NULL&&_parameterMapValue!=NULL){
		if(_nameValue->type==VT_TEXT&&_parameterMapValue->type==VT_MAP&&(!_bodyTokenValue||_bodyTokenValue->type==VT_TOKEN)){
			Muserfunction* _userfunction=(Muserfunction*)CALLOC_1(sizeof(Muserfunction),'-',owner);
			if(_userfunction!=NULL){
				if(amVerbose()){outputValue("Defining function '",_nameValue,"' with ");outputValue(" parameters ",_parameterMapValue,".\n");}
				Mtext* functionName=_nameValue->value._text;
				// user function expects a list of commands, so we have to wrap the single token (if any)
				if(_bodyTokenValue!=NULL){
					_userfunction->_bodyCommandList=SUBOWNED(owned_list(_getListOfType(VT_TOKEN),owner),1);
					if(NULL==_userfunction->_bodyCommandList||appendedToList(_userfunction->_bodyCommandList,Msubowner(owner,1),_bodyTokenValue,M_LL_INVALID)<=0)
						output("%sFailed to store the inline command as body of function definition of '%s'.\n",M_ERROR_PREFIX,functionName->_c);
					// replacing: assignValue(&_userfunction->_bodyTokenValue,_bodyTokenValue);
				}
				//////////Mvalue* _userfunctionValue=_getUserfunctionValue(_userfunction,true); // free asap or bound
				///////if(_userfunctionValue){
					// MDH@17JUL2019: the map needs to be stored with the Mfunction
				Mallocationowner owner_environment=getOwnerExecutionEnvironment();
				Mfunction* _function=owned_function(_getFunction(getExecutionEnvironment(),owner_environment,functionName->_c),owner);
				if(_function!=NULL){
					// MDH@02MAR2020: the following is dangerous, because the value might be freed in which case the map would be freed as well!!!!
					//				so we have to make a copy of the parameter map
					_function->_parameterMap=owned_map(_getMapCopy(_parameterMapValue->value._map),Msubowner(owner_environment,4)); // MDH@03MAR2020: making a copy of the map wrapped in the value passed in
					_function->functionunion._userfunction=owned_userfunction(disowned_userfunction(_userfunction,owner),Msubowner(owner_environment,4));
					// return the result of applying the function to the default parameter map

					return _getIntegerValue(1);
				}
				///////////free_value(_userfunctionValue); // freed
				output("%sFailed to create function '%s'.\n",M_ERROR_PREFIX,functionName);
				///////}
			}
		}else
			outputError("Invalid user function name, parameter map or body");
	}else{
		if(NULL==_nameValue)outputError("No name defined of function");
		if(NULL==_parameterMapValue)outputError("No (formal) parameter map defined for function");
		////////if(!_bodyTokenValue)outputError("No body (expression) defined of function");
	}
	return _getIntegerValue(0); // indicating failure...
}/*VALIDATED */

// MDH@04DEC2020: computing sample statistics on any sequence inserted here as it uses Misnumeric (see Mfunctions.c/h), multiply and add M functions
// MDH@04DEC2020: generic sample statistics map producer, which only processes the numeric values
//				as it uses M functions it has to be here and not in Mlist.c/h
/**
 * @brief returns the sample statistics map using values provided by M iterator \p iterator
 * 
 * @param iterator 
 * @return Mmap* the sample statistics map using values provided by M iterator \p iterator
 */
static Mmap* _getSampleStatisticsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _statisticsMap=(iterator!=NULL?owned_map(_getMapOfType(VT_UNDEFINED),owner):NULL);
	if(_statisticsMap!=NULL){
		if(amVerbose())output("Computing float sample statistics.\n");
		long long missings=0,errors=0,count=0,minimumindex=0,maximumindex=0;
		// initialize sum, sumofsquares, minimum and maximum to NULL
		Mvalue *value=NULL,*squarevalue=NULL,*sum=NULL,*sumofsquares=NULL,*minimum=NULL,*maximum=NULL;
		unsigned long long index=0;
		while((index=iter_nextindex(iterator))){
			// output("Index: %llu",index); // DEBUG
			value=iter_next(iterator);
			// output(" - value='",value,"'\n"); // DEBUG
			if(value!=NULL){
				if(getValueInteger(Misnumeric(value))==M_TRUE){
					squarevalue=multiply(value,value);
					if(count>0){
						sum=add(sum,value);
						sumofsquares=add(sumofsquares,squarevalue);
						if(smallerthan(value,minimum)==M_TRUE){minimum=value;minimumindex=index;}
						if(largerthan(value,maximum)==M_TRUE){maximum=value;maximumindex=index;}
					}else{ // no sum yet
						sum=value;
						minimum=value;minimumindex=index;
						maximum=value;maximumindex=index;
						sumofsquares=squarevalue;
					}
					count++;
				}else // not numeric, so skip and assume being an error
					errors++;
			}else
				missings++;
		}
		// ready to compose the map elements
		appendedToMap(_statisticsMap,owner,"size",_getIntegerValue(count));
		appendedToMap(_statisticsMap,owner,"sum",sum);
		appendedToMap(_statisticsMap,owner,"sumofsquares",sumofsquares);
		appendedToMap(_statisticsMap,owner,"minimum",minimum);
		appendedToMap(_statisticsMap,owner,"maximum",maximum);
		appendedToMap(_statisticsMap,owner,"minimumindex",_getIntegerValue(minimumindex));
		appendedToMap(_statisticsMap,owner,"maximumindex",_getIntegerValue(maximumindex));
		appendedToMap(_statisticsMap,owner,"missings",_getIntegerValue(missings));
		appendedToMap(_statisticsMap,owner,"errors",_getIntegerValue(errors));
		return disowned_map(_statisticsMap,owner);
	}
	if(iterator!=NULL)outputMemoryError("Failed to create a map to store statistics in.");
	return NULL;
}
/**
 * @brief returns the correleation of sequences \p _sequence1Value and \p _sequence2Value
 * 
 * @param _sequence1Value 
 * @param _sequence2Value 
 * @return Mvalue* he correleation of sequences \p _sequence1Value and \p _sequence2Value
 */
Mvalue* Mcorr(Mvalue* _sequence1Value,Mvalue* _sequence2Value){
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL));
	if(_sequence1Value!=NULL&&_sequence2Value!=NULL){
		// the sequences need to have the same number of elements
		if((_sequence1Value->type==VT_LIST||_sequence1Value->type==VT_ARRAY)
			&&(_sequence2Value->type==VT_LIST||_sequence2Value->type==VT_ARRAY)){
			unsigned long long numberOfElements1=(_sequence1Value->type==VT_LIST?_sequence1Value->value._list->numberOfElements:_sequence1Value->value._array->numberOfElements);
			unsigned long long numberOfElements2=(_sequence2Value->type==VT_LIST?_sequence2Value->value._list->numberOfElements:_sequence2Value->value._array->numberOfElements);
			if(numberOfElements1==numberOfElements2){
				Miterator iterator1=(_sequence1Value->type==VT_LIST?getListiterator(_sequence1Value->value._list):getArrayiterator(_sequence1Value->value._array));
				Miterator iterator2=(_sequence2Value->type==VT_LIST?getListiterator(_sequence2Value->value._list):getArrayiterator(_sequence2Value->value._array));
				// we should only use the elements when the index is the same
				// this means that the correlation could still be undefined
				unsigned long long index1=0,index2=0,count=0;
				Mvalue *value1,*value2,*prod12,*prod1,*prod2;
				Mvalue *sum12,*sum1,*sum2,*ssq1,*ssq2;
				while((index1=iter_nextindex(&iterator1))){
					if(index2<index1)
					while((index2=iter_nextindex(&iterator2))<index1);
					if(index1==index2){
						value1=iter_next(&iterator1);
						value2=iter_next(&iterator2);
						prod12=multiply(value1,value2);
						prod1=multiply(value1,value1);
						prod2=multiply(value2,value2);
						if(isNumeric(prod1)==M_TRUE&&isNumeric(prod2)==M_TRUE&&isNumeric(prod12)==M_TRUE){
							if(count){
								ssq1=add(ssq1,prod1);
								ssq2=add(ssq2,prod2);
								sum1=add(sum1,value1);
								sum2=add(sum2,value2);
								sum12=add(sum12,prod12);
							}else{ // initialize
								ssq1=prod1;
								ssq2=prod2;
								sum1=value1;
								sum2=value2;
								sum12=prod12;
							}
							count++;
						}
					}
				}
				if(count){
					Mvalue* countValue=_getIntegerValue(count);
					if(countValue!=NULL){
						// if(report)
						{
							output("Correlation constituent parts:");
							outputValue(" Count=",countValue,NULL);
							output(" | X:");outputValue(" sum=",sum1,NULL);outputValue(" - ssq=",ssq1,NULL);
							output(" | Y:");outputValue(" sum=",sum2,NULL);outputValue(" - ssq=",ssq2,NULL);
							outputValue(" | X*Y: sum=",sum12,".\n");
						}
						Mvalue* numerator=subtract(sum12,divide(multiply(sum1,sum2),countValue));
						Mvalue* denominator=Msqrt(multiply(subtract(ssq1,divide(multiply(sum1,sum1),countValue)),subtract(ssq2,divide(multiply(sum2,sum2),countValue))));
						return divide(numerator,denominator);
					}else
						outputError("Failed to wrap the sample count in computing a correlation coefficient");
				}
			}else{
				output("%sCannot compute the correlation between ",M_WARNING_PREFIX);
				outputValue("'",_sequence1Value,"' and ");
				outputValue("'",_sequence2Value,"' as they do not have the same number of elements.\n");
			}
		}
	}
	return NULL;
}
// MDH@04JAN2021: if the first element of the iterator is a list itself, we should be returning an array of sample statistics for each list element
//				and a correlation matrix for the combined samples but only for pairs with the same number of elements
/**
 * @brief returns the statistics map using the sample values from \p iterator
 * 
 * @param iterator 
 * @return Mmap* the statistics map using the sample values from \p iterator
 */
static Mmap* _getStatsMap(Miterator* iterator){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _statsMap=NULL;
	if(iterator!=NULL){
		// MDH@04JAN2021: the valuetype of the iterator is the valuetype of the array or the list
		//				I suppose that we should now accomodate MAP and LIST value iterators as well
		//				if the value type of the array is not set i.e. it can store values of any type, we use the type of the first element
		Mvalue* firstvalue=NULL;
		Mvaluetype valuetype=iterator->valuetype;
		if(valuetype==VT_UNDEFINED){
			// assume valueholder is a pointer to the first element of 
			firstvalue=(iterator->valueholder!=NULL?*(iterator->valueholder):NULL);
			if(firstvalue!=NULL)valuetype=firstvalue->type; // if firstvalue is NULL (i.e. on an empty list), valuetype will remain VT_UNDEFINED
		}
		if(valuetype==VT_LIST||valuetype==VT_ARRAY){ // both support iterators
			// does it really matter whether we return a list or array???????? I guess we can return a list because a lot of elements could be NULL in the array as well
			_statsMap=owned_map(__map("_getStatsMap"),owner);
			if(_statsMap!=NULL){
				// should we be storing the correlations in a list or in an array????
				// we might get a lot of NULL values in an array if the input is a sparse list
				Mlist *_statsList=owned_list(__list("_getStatsMap"),owner),
							*_corrsList=owned_list(__list("_getStatsMap"),owner);
				if(_statsList!=NULL&&_corrsList!=NULL){
					// wrap the lists and append them to the map
					// NOTE by disowning the lists they will be freed if failing to wrap them
					//	  which means we do not need to free them anymore
					Mvalue *statsListValue=_getValueOfList(disowned_list(_statsList,owner))
								,*corrsListValue=_getValueOfList(disowned_list(_corrsList,owner));
					if(statsListValue!=NULL&&corrsListValue!=NULL){
						// with the lists bound to the values, they will be freed if the values are gc'ed
						if(appendedToMap(_statsMap,owner,"statistics",statsListValue)==M_TRUE
							&&appendedToMap(_statsMap,owner,"correlations",corrsListValue)==M_TRUE){
							Mvalue *value,*nextvalue;
							Miterator nextiterator;
							unsigned long long index=0,nextindex;
							while((index=iter_nextindex(iterator))){
								// output("Index: %llu",index); // DEBUG
								value=iter_next(iterator);
								if(value!=NULL){
									if(value->type==VT_LIST||value->type==VT_ARRAY){
										// as we'll be storing all correlations with successive lists
										// we need to ascertain to have such a list
										if(index!=iterator->lastindex){ // assumedly not the last list/array
											Mlist* _nextcorrsList=owned_list(__list("_getStatsMap"),owner);
											if(_nextcorrsList!=NULL){
												if(appendedToList(_corrsList,owner,_getValueOfList(_nextcorrsList),index)>0){
													nextiterator=*iterator; // get a copy
													while((nextindex=iter_nextindex(&nextiterator))){
														nextvalue=iter_next(&nextiterator);
														if(NULL==nextvalue)continue;
														if(nextvalue->type==VT_LIST||nextvalue->type==VT_ARRAY){
															Mvalue* corr=Mcorr(value,nextvalue);
															if(corr!=NULL){
																if(appendedToList(_nextcorrsList,owner,corr,nextindex)<=0){
																	output("%sFailed to store correlation coefficient ",M_ERROR_PREFIX);
																	outputValue("'",corr,"'.\n");
																}
															}else 
																outputError("Failed to compute a correlation coefficient");
														}
													}
												}else{
													outputError("Failed to create a list to store correlations");
													free_list(_nextcorrsList);
												}
											}else
												outputError("Failed to create a list to store computed correlation coefficients");
										}
										// most convenient to use Mstats 
										Mvalue* valuestatsMapValue=Mstats(value);
										if(valuestatsMapValue!=NULL){
											if(appendedToList(_statsList,owner,valuestatsMapValue,index)<=0){
												valuestatsMapValue=NULL;
												output("%s",M_ERROR_PREFIX);
												outputValue("Failed to compute the statistics of '",value,"'.\n");
											}
										}
									}
								}
							}
						}
					}
				}else{ // failed to create both lists
					if(_statsList)FREE_LIST(_statsList,owner);
					if(_corrsList)FREE_LIST(_corrsList,owner);
					FREE_MAP(_statsMap,owner);_statsMap=NULL;
				}
			}
		}else
		if(valuetype==VT_INTEGER)
			_statsMap=owned_map(_getIntegerSampleStatisticsMap(iterator),owner);
		else
		if(valuetype==VT_BIGINTEGER)
			_statsMap=owned_map(_getBigintegerSampleStatisticsMap(iterator),owner);
		else
		if(valuetype==VT_RATIONAL)
			_statsMap=owned_map(_getRationalSampleStatisticsMap(iterator),owner);
		else
		if(valuetype==VT_DECIMAL)
			_statsMap=owned_map(_getDecimalSampleStatisticsMap(iterator),owner);
		else
		if(valuetype==VT_FLOAT)
			_statsMap=owned_map(_getFloatSampleStatisticsMap(iterator),owner);
		else
		if(valuetype==VT_UNDEFINED)
			_statsMap=owned_map(_getSampleStatisticsMap(iterator),owner);
		else
			output("%sInvalid iterator value type '%s' in computing statistics.\n",M_ERROR_PREFIX,VALUETYPENAMES[valuetype]);
	}
	return disowned_map(_statsMap,owner);
}
/**
 * @brief returns the wrapped statistics map of the sample values in sequence \p sequenceValue
 * 
 * @param sequenceValue 
 * @return Mvalue* the wrapped statistics map of the sample values in sequence \p sequenceValue
 */
Mvalue* Mstats(Mvalue* sequenceValue){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _statsMap=NULL;
	if(sequenceValue!=NULL){
		Miterator iterator={};
		if(sequenceValue->type==VT_LIST){
			Mlist* list=sequenceValue->value._list;
			if(list!=NULL){
				iterator=getListiterator(list);
				/*
				if(list->valuetype!=VT_MAP&&list->valuetype!=VT_REFERENCE&&list->valuetype!=VT_LIST&&list->valuetype!=VT_UNDEFINED){
					// the values in the list need to be scalars of the same type
					if(list->valuetype==VT_INTEGER)_statsMap=owned_map(_getIntegerSampleStatisticsMap(list),owner);
					if(list->valuetype==VT_BIGINTEGER)_statsMap=owned_map(_getBigintegerSampleStatisticsMap(list),owner);
					if(list->valuetype==VT_RATIONAL)_statsMap=owned_map(_getRationalSampleStatisticsMap(list),owner);
					if(list->valuetype==VT_DECIMAL)_statsMap=owned_map(_getDecimalSampleStatisticsMap(list),owner);
					if(list->valuetype==VT_FLOAT)_statsMap=owned_map(_getFloatSampleStatisticsMap(list),owner);
				}else
					output("All values in the list should be of the same numeric type (integer, big integer, float, rational or decimal).\n");
				*/
			}else 
				outputBug("Missing value list!");
		}else
		if(sequenceValue->type==VT_ARRAY){
			Marray* array=sequenceValue->value._array;
			if(array!=NULL)
				iterator=getArrayiterator(array);
			else
				outputBug("Missing value array!");
		}
		if(iterator.next!=NULL)
			_statsMap=owned_map(_getStatsMap(&iterator),owner);
	}else
		outputError("No sample list to compute statistics of");
	return(_statsMap?_getValueOfMap(disowned_map(_statsMap,owner)):NULL);
}

// MDH@29OCT2020: the famous array functions of JS: foreach, map, reduce, filter
/**
 * @brief reduces the wrapped M list \p _listValue using the wrapped M function \p _functionValue
 * 
 * @param _listValue 
 * @param _functionValue 
 * @param _initialAccumulatedValue 
 * @return Mvalue* the result of reducing M list \p _listValue
 */
Mvalue* Mlreduce(Mvalue* _listValue,Mvalue* _functionValue,Mvalue* _initialAccumulatedValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mvalue* _accumulatedValue=_initialAccumulatedValue;
	// it's up to the user to supply an initial accumulated value like a default
	Mlist* list=(_listValue!=NULL&&_listValue->type==VT_LIST?_listValue->value._list:NULL);
	if(list!=NULL){
		Mfunction* function=(_functionValue!=NULL&&_functionValue->type==VT_FUNCTION?_functionValue->value._function:NULL);
		if(function!=NULL){
			Mlist* _reduceFunctionArgumentList=owned_list(__list("reduce"),owner);
			if(_reduceFunctionArgumentList!=NULL){
				// we are to append a total of 
				Mlistelement* listelement=list->_first;
				if(listelement!=NULL){
					long long listelementIndex=0;
					long long accumulatedValueIndex=appendedToList(_reduceFunctionArgumentList,owner,_accumulatedValue,M_LL_INVALID);
					long long valueIndex=appendedToList(_reduceFunctionArgumentList,owner,listelement->_value,M_LL_INVALID);
					long long indexIndex=appendedToList(_reduceFunctionArgumentList,owner,_getIntegerValue(listelementIndex),M_LL_INVALID);
					if(accumulatedValueIndex>0&&valueIndex>0&&indexIndex>0){
						// function argument list initialized, ready to execute the function on each element of the given list
						do{
							// compute the accumulated value
							Mmap* _reduceFunctionArgumentMap=_getFunctionArgumentMap(function,_reduceFunctionArgumentList,owner);
							_accumulatedValue=getValueOfFunctionCall(function,"",_reduceFunctionArgumentMap);
							if(report){outputMap("Result of applying the reduce function to '",_reduceFunctionArgumentMap,"'");outputValue(": '",_accumulatedValue,"'.\n");}
							FREE_MAP(_reduceFunctionArgumentMap,owner); // TODO is this right??????
							if(appendedToList(_reduceFunctionArgumentList,owner,_accumulatedValue,accumulatedValueIndex)<=0)break;
							listelement=listelement->_next;
							if(!listelement)break;
							if(appendedToList(_reduceFunctionArgumentList,owner,listelement->_value,valueIndex)<=0)break;
							listelementIndex++;
							if(appendedToList(_reduceFunctionArgumentList,owner,_getIntegerValue(listelementIndex),indexIndex)<=0)break;					
						}while(listelement);
					}
				}
				FREE_LIST(_reduceFunctionArgumentList,owner);
			}else
				outputError("Failed to create the reduce function argument list");
		}else
			outputError("No reduce function specified");
	}else
		outputError("No list to reduce specified.");
	return _accumulatedValue;
}
/**
 * @brief returns the wrapped list with mapped elements of \p _listValue applying \p _functionValue to each element
 * 
 * @param _listValue 
 * @param _functionValue 
 * @return Mvalue* the wrapped list with mapped elements of \p _listValue applying \p _functionValue to each element
 */
Mvalue* Mlmap(Mvalue* _listValue,Mvalue* _functionValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mvalue* _mapValue=NULL;
	// it's up to the user to supply an initial accumulated value like a default
	Mlist* list=(_listValue!=NULL&&_listValue->type==VT_LIST?_listValue->value._list:NULL);
	if(list!=NULL){
		Mfunction* function=(_functionValue!=NULL&&_functionValue->type==VT_FUNCTION?_functionValue->value._function:NULL);
		if(function!=NULL){
			Mlist* _mapFunctionArgumentList=owned_list(__list("lmap"),owner);
			if(_mapFunctionArgumentList!=NULL){
				Mlist* _mapList=owned_list(__list("lmap"),owner);
				if(_mapList!=NULL){
					// we are to append a total of 
					Mlistelement* listelement=list->_first;
					if(listelement!=NULL){
						long long listelementIndex=listelement->index;
						long long valueIndex=appendedToList(_mapFunctionArgumentList,owner,listelement->_value,M_LL_INVALID);
						long long indexIndex=appendedToList(_mapFunctionArgumentList,owner,_getIntegerValue(listelementIndex),M_LL_INVALID);
						if(valueIndex>0&&indexIndex>0){
							// function argument list initialized, ready to execute the function on each element of the given list
							do{
								// compute the accumulated value
								Mmap* _mapFunctionArgumentMap=_getFunctionArgumentMap(function,_mapFunctionArgumentList,owner);
								Mvalue* _mapFunctionValue=getValueOfFunctionCall(function,"",_mapFunctionArgumentMap);
								if(report){outputMap("Result of applying the map function to '",_mapFunctionArgumentMap,"'");outputValue(": '",_mapFunctionValue,"'.\n");}
								FREE_MAP(_mapFunctionArgumentMap,owner);
								// if we fail to add the result of applying the map function, we report that but we do not break, the length of the result list should be the same as that of the input list
								if(appendedToList(_mapList,owner,_mapFunctionValue,listelementIndex)<=0)outputError("Failed to add the result of applying the map function");
								listelement=listelement->_next;
								if(NULL==listelement)break;
								if(appendedToList(_mapFunctionArgumentList,owner,listelement->_value,valueIndex)<=0)break;
								listelementIndex=listelement->index;
								if(appendedToList(_mapFunctionArgumentList,owner,_getIntegerValue(listelementIndex),indexIndex)<=0)break;					
							}while(listelement);
						}
					}
					_mapValue=_getValueOfList(disowned_list(_mapList,owner));
					if(NULL==_mapValue)free_list(_mapList); // if not bound (but already disowned) free the list myself
				}else
					outputError("Failed to create the map result list");
				FREE_LIST(_mapFunctionArgumentList,owner);
			}else
				outputError("Failed to create the map function argument list");
		}else
			outputError("No map function specified");
	}else
		outputError("No list to map specified.");
	return _mapValue;
}
/**
 * @brief returns a wrapped list of filtered elements of \p _listValue using boolean function wrapped in \p _functionValue
 * 
 * @param _listValue 
 * @param _functionValue 
 * @return Mvalue* a wrapped list of filtered elements of \p _listValue using boolean function wrapped in \p _functionValue
 */
Mvalue* Mlfilter(Mvalue* _listValue,Mvalue* _functionValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mvalue* _filterValue=NULL;
	// it's up to the user to supply an initial accumulated value like a default
	Mlist* list=(_listValue!=NULL&&_listValue->type==VT_LIST?_listValue->value._list:NULL);
	if(list!=NULL){
		Mlist* _filterList=owned_list(__list("lfilter"),owner);
		if(_filterList!=NULL){
			Mlistelement* listelement=list->_first;
			if(listelement!=NULL){
				Mfunction* function=(_functionValue!=NULL&&_functionValue->type==VT_FUNCTION?_functionValue->value._function:NULL);
				if(function!=NULL){
					Mlist* _filterFunctionArgumentList=owned_list(__list("lfilter"),owner);
					if(_filterFunctionArgumentList!=NULL){
						// we are to append a total of 
						long long listelementIndex=listelement->index;
						long long valueIndex=appendedToList(_filterFunctionArgumentList,owner,listelement->_value,M_LL_INVALID);
						long long indexIndex=appendedToList(_filterFunctionArgumentList,owner,_getIntegerValue(listelementIndex),M_LL_INVALID);
						if(valueIndex>0&&indexIndex>0){
							// function argument list initialized, ready to execute the function on each element of the given list
							do{
								// compute the accumulated value
								Mmap* _filterFunctionArgumentMap=_getFunctionArgumentMap(function,_filterFunctionArgumentList,owner);
								Mvalue* _filterFunctionValue=getValueOfFunctionCall(function,"",_filterFunctionArgumentMap);
								if(report){outputMap("Result of applying the filter function to '",_filterFunctionArgumentMap,"'");outputValue(": '",_filterFunctionValue,"'.\n");}
								FREE_MAP(_filterFunctionArgumentMap,owner);
								// if _filterFunctionValue is 'true' the current value should be appended to the result list
								// now the question is whether or not the filter function returned something that can be tested for being M_TRUE
								// I guess when the filter function value is positive let consider the list value valid
								if(isValuePositive(_filterFunctionValue)==M_TRUE&&appendedToList(_filterList,owner,listelement->_value,M_LL_INVALID)<=0){
									outputError("Failed to add the result of applying the filter function");
									break;
								}
								listelement=listelement->_next;
								if(NULL==listelement)break;
								if(appendedToList(_filterFunctionArgumentList,owner,listelement->_value,valueIndex)<=0)break;
								listelementIndex=listelement->index;
								if(appendedToList(_filterFunctionArgumentList,owner,_getIntegerValue(listelementIndex),indexIndex)<=0)break;					
							}while(listelement);
						}
						FREE_LIST(_filterFunctionArgumentList,owner);
					}else
						outputError("Failed to create the filter function argument list");
				}else{ // no filter function specified 
					do{
						if(appendedToList(_filterList,owner,listelement->_value,M_LL_INVALID)<=0)
							output("%sFailed to append a filter list element #%llu.\n",M_ERROR_PREFIX,listelement->index);
						listelement=listelement->_next;
					}while(listelement);
				}
			}
			_filterValue=_getValueOfList(disowned_list(_filterList,owner));
			if(NULL==_filterValue)free_list(_filterList); // if not bound (but already disowned) free the list myself
		}else
			outputError("Failed to create the filter result list");
	}else
		outputError("No list to filter specified");
	return _filterValue;
}
// NOTE how does foreach compare to map???? as it seems that foreach does not return a value as opposed to map
/**
 * @brief returns the wrapped M list applying \p _functionValue to each element of the list wrapped in \p _listValue
 * 
 * @param _listValue 
 * @param _functionValue 
 * @return Mvalue* the wrapped M list applying \p _functionValue to each element of the list wrapped in \p _listValue
 */
Mvalue* Mlforeach(Mvalue* _listValue,Mvalue* _functionValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long foreachCount=M_LL_INVALID; // counting the number of times the function was applied
	// similar to map but returning the number of elements the function was applied to
	// it's up to the user to supply an initial accumulated value like a default
	Mlist* list=(_listValue!=NULL&&_listValue->type==VT_LIST?_listValue->value._list:NULL);
	if(list!=NULL){ // there is a list to iterate
		Mlistelement* listelement=list->_first;
		// let's allow the function to be NULL
		Mfunction* function=(_functionValue!=NULL&&_functionValue->type==VT_FUNCTION?_functionValue->value._function:NULL);
		if(function!=NULL){
			Mlist* _foreachFunctionArgumentList=owned_list(__list("lforeach"),owner);
			if(_foreachFunctionArgumentList!=NULL){
				// we are to append a total of 
				if(listelement!=NULL){
					long long listelementIndex=listelement->index;
					// passing the list as first argument, and the index into the list as second argument on every call to the given function
					long long listIndex=appendedToList(_foreachFunctionArgumentList,owner,_listValue,M_LL_INVALID);
					long long indexIndex=appendedToList(_foreachFunctionArgumentList,owner,_getIntegerValue(listelement->index),M_LL_INVALID);
					if(listIndex>0&&indexIndex>0){
						// function argument list initialized, ready to execute the function on each element of the given list
						foreachCount=0;
						do{
							Mmap* _foreachFunctionArgumentMap=_getFunctionArgumentMap(function,_foreachFunctionArgumentList,owner);
							Mvalue* _foreachFunctionValue=getValueOfFunctionCall(function,"",_foreachFunctionArgumentMap);
							FREE_MAP(_foreachFunctionArgumentMap,owner);
							foreachCount++; // successfully applied the function to this element
							if(report){outputMap("Result of applying the foreach function to '",_foreachFunctionArgumentMap,"'");outputValue(": '",_foreachFunctionValue,"'.\n");}
							listelement=listelement->_next;
							if(NULL==listelement)break;
							if(appendedToList(_foreachFunctionArgumentList,owner,_getIntegerValue(listelement->index),indexIndex)<=0)break;
						}while(listelement);
						if(listelement!=NULL)foreachCount=-foreachCount;
					}
				}
				FREE_LIST(_foreachFunctionArgumentList,owner);
			}else
				outputError("Failed to create the foreach function argument list");

		}else{ // no function, simply return the number of elements in the list
			foreachCount=0;while(listelement){foreachCount++;listelement=listelement->_next;}
		}
	}else
		outputError("No list to foreach specified.");
	return _getIntegerValue(foreachCount);
}

// SORTING STUFF
/**
 * @brief sort statistics
 * 
 */
struct Msortstatistics{
	unsigned long long memoryallocation;
	unsigned long long comparisons;
	unsigned long long pointerassignments;
	unsigned long long fieldassignments; // any reference to a field pointed to by a pointer whether in a comparison or assigment (left or right-hand side)
	unsigned long long pointerreferences; // including field pointer assignments
	unsigned long long fieldreferences; // any reference to a field pointed to by a pointer whether in a comparison or assigment (left or right-hand side)
	unsigned long long pointertests;
	unsigned long long fieldtests;
};
/**
 * @brief the single sortstatistics instance
 * 
 */
static struct Msortstatistics sortstatistics;
/**
 * @brief outputs the current sort statistics
 * 
 */
static void outputSortStatistics(){
	output("Sort statistics: value comparisons=%llu | memory allocation=%llu | pointer: assignments=%llu - references=%llu - tests=%llu | field: assignments=%llu - references=%llu - tests=%llu.\n"
			,sortstatistics.comparisons,sortstatistics.memoryallocation
			,sortstatistics.pointerassignments,sortstatistics.pointerreferences,sortstatistics.pointertests
			,sortstatistics.fieldassignments,sortstatistics.fieldreferences,sortstatistics.fieldtests);
}
// delegate asmallerthan to smallerthan() which returns a long long instead of an int
// asmallerthan will receive pointers to an Mvalue*
static int alargerthan(const void* aValue,const void* bValue){
	long long result=largerthan(*((Mvalue**)aValue),*((Mvalue**)bValue));
	return(result<=0?-1:1);
}
/**
 * @brief returns M_TRUE when \p _array was quick sorted successfully, M_FALSE otherwise
 * @details will return M_LL_INVALID if \p _array is NULL
 * @param _array 
 * @return long long M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
static long long acsort(Marray* _array){
	if(NULL==_array)return M_LL_INVALID;
	qsort(_array->values,_array->numberOfElements,sizeof(Mvalue*),alargerthan);
	return M_TRUE;
}
/**
 * @brief exchanges elements at index \p index1 and \p index2 of the elements in \p values
 * 
 * @param values 
 * @param index1 
 * @param index2 
 */
static void aswap(Mvalue** const values,long long index1,long long index2){
	// normally we would not be allowed to do it this way, but the reference count
	// of both values remains the same when we exchange their position in the list
	// output("Swapping element #%llu and #%llu.\n",listelement1->index,listelement2->index);
	Mvalue* value=values[index1];
	values[index1]=values[index2];
	values[index2]=value;
	sortstatistics.pointerassignments++;sortstatistics.fieldreferences+=4;
	sortstatistics.pointerreferences+=2;sortstatistics.fieldassignments+=2;
}
/**
 * @brief partitions \p values from elements at index \p l through \p h
 * 
 * @param values 
 * @param l 
 * @param h 
 * @return unsigned long long 
 */
static unsigned long long apartition(Mvalue** const values,unsigned long long l,unsigned long long h){
	// output("Partitioning elements #%llu through #%llu.\n",l->index,h->index);
	sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;sortstatistics.pointerassignments++;
	Mvalue* x=values[h]; //* x=list[h] // x is set once, as the value at index h
	unsigned long long i=l; // MDH@25NOV2020: actually one above the first value to use (which means we increment i AFTER swapping, instead of before as was done in the original algorithm)
	// MDH@02NOV2020: in order to be able to call helper function smallerthanorequalto() x should not be a list, essentially list bubble up to the top I suppose
	sortstatistics.fieldtests++;
	if(x->type!=VT_LIST){
		for(register unsigned long long j=l;j<h;j++){
			long long notlarger=M_LL_INVALID;
			sortstatistics.pointerreferences++;sortstatistics.fieldtests++;
			if(values[j]->type!=VT_LIST){
				notlarger=smallerthanorequalto(values[j],x);
				sortstatistics.comparisons++;
			}
			if(notlarger==M_TRUE){
				sortstatistics.pointerreferences++;
				aswap(values,i++,j); // incrementing i AFTER using it in the call
			}
		}
	}
	aswap(values,i,h); //* swap(list[i+1],list[h])
	return i; //* i+1 but actually we are returning i itself because that's the first value used
}
// MDH@25NOV2020: because we want to work with unsigned long long values, adapting apartition accordingly
/**
 * @brief quick sorts \p _array, returning M_TRUE on success, or M_FALSE or M_LL_INVALID on failure
 * 
 * @param _array 
 * @return long long returning M_TRUE on success, or M_FALSE or M_LL_INVALID on failure
 */
static long long aquicksort(Marray* _array){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=M_LL_INVALID;
	sortstatistics.pointertests++;
	if(_array!=NULL){
		result=M_TRUE;
		clock_t sortstart=clock();
		sortstatistics.fieldreferences++;
		unsigned long long arraylength=_array->numberOfElements;
		if(arraylength>1){ // things to compare
			if(report)
				output("Sorting an array of %zd elements with quicksort.\n",arraylength);
				unsigned long long maxtop,initialmaxtop=2*((unsigned long long)ceil(log10(arraylength)));
				long long* stack=MALLOC(sizeof(long long),maxtop=initialmaxtop,-'u',owner);
				if(stack!=NULL){
					sortstatistics.memoryallocation+=(sizeof(long long)*maxtop);
					if(report)output("Initial quicksort stack size: %llu.\n",maxtop);
					result=2; // the current amount of stack elements used
					stack[0]=0; // i.e. the first index, which is 0 (l=lmin1+1)
					stack[1]=arraylength-1; // i.e. the last index, which is arraylength-1
					// it's better to store the number of elements in the stack instead of the top index
					// so that the smallest value of top will be 0
					unsigned long long top=2;
					long long lmin1,l,h,pmin1,p,pplus1;
					Mvalue** values=_array->values;
					sortstatistics.fieldreferences++;
					// as long as there are two elements on the stack
					while(top>1){ // two or more elements on the stack
						h=stack[--top];
						l=stack[--top];
						p=apartition(values,l,h);
						sortstatistics.pointerassignments+=3;sortstatistics.fieldreferences+=2;
						//if(!p){result=M_FALSE;outputError("Failed to partition");break;}
						sortstatistics.pointertests++;
						if(p>l+1){
							if(top>=maxtop){
								if(report)
									output("Expanding the stack.\n");
								stack=REALLOC(stack,maxtop,maxtop+initialmaxtop,sizeof(long long),-'u');
								if(NULL==stack){
									result=M_FALSE;
									output("%sNot enough memory for a stack of %llu list elements in quicksort.\n",M_ERROR_PREFIX,maxtop+initialmaxtop);
									break;
								}
								sortstatistics.memoryallocation+=(sizeof(long long)*initialmaxtop);
								maxtop+=initialmaxtop;
								if(report)
									output("Stack of quicksort expanded to contain %llu elements.\n",maxtop);
							}
							stack[top++]=l;
							stack[top++]=p-1;
							sortstatistics.fieldassignments+=2;sortstatistics.pointerreferences+=2;
							if(top>result)result=top;
						}
						if(p+1<h){
							if(top>=maxtop){
								if(report)
									output("Expanding the stack.\n");
								stack=REALLOC(stack,maxtop,maxtop+initialmaxtop,sizeof(long long),-'u');
								if(NULL==stack){
									result=M_FALSE;
									output("%sNot enough memory for a stack of %llu list elements in quicksort.\n",M_ERROR_PREFIX,maxtop+initialmaxtop);
									break;
								}
								sortstatistics.memoryallocation+=(sizeof(long long)*initialmaxtop);
								maxtop+=initialmaxtop;
								if(report)
									output("Stack of quicksort expanded to contain %llu elements.\n",maxtop);
							}
							stack[top++]=p+1;
							stack[top++]=h;
							sortstatistics.fieldassignments+=2;sortstatistics.pointerreferences+=2;
							if(top>result)result=top;
						}
					}
					if(report)
						output("Freeing the quicksort stack.\n");
					FREE_DISOWNED(stack,maxtop,-'u',owner);
					if(report)
						output("Quicksort stack freed.\n");
				}else{
					result=M_FALSE;
					outputError("Not enough memory for the stack to sort the array with quicksort");
				}
		}
		if(result>0)
			output("Duration of array sorting by quicksort: %.3f ms.\n",((double)(clock()-sortstart))/M_CLOCKS_PER_MS);
	}
	return result;
}

// MDH@02NOV2020: we can speed up sorting if we can somehow reverse parts of a list
/**
 * @brief swaps the value wrapped in \p listelement1 and \p listelement2
 * 
 * @param listelement1 
 * @param listelement2 
 */
static void lswap(Mlistelement* const listelement1,Mlistelement* const listelement2){
	// normally we would not be allowed to do it this way, but the reference count
	// of both values remains the same when we exchange their position in the list
	// output("Swapping element #%llu and #%llu.\n",listelement1->index,listelement2->index);
	Mvalue* value=listelement1->_value;
	listelement1->_value=listelement2->_value;
	listelement2->_value=value;
	sortstatistics.pointerassignments++;sortstatistics.fieldreferences+=2;sortstatistics.pointerreferences++;sortstatistics.fieldassignments+=2;
}
// it's best to pass l-1 instead of l, then we will always be able to compute l
/**
 * @brief helper function to partition a list
 * 
 * @param list 
 * @param lmin1 
 * @param h 
 * @return Mlistelement* the pointer to the first element of the partitioned list
 */
static Mlistelement* lpartition(Mlist* const list,Mlistelement* const lmin1,Mlistelement* const h){
	Mlistelement* l=(lmin1!=NULL?lmin1->_next:list->_first);
	// output("Partitioning elements #%llu through #%llu.\n",l->index,h->index);
	Mvalue* x=h->_value; //* x=list[h] // x is set once, as the value at index h
	Mlistelement* i=lmin1; //* i=l-1
	sortstatistics.pointerassignments+=3;sortstatistics.pointertests++;sortstatistics.fieldreferences+=2;sortstatistics.pointerreferences++;
	// MDH@02NOV2020: in order to be able to call helper function smallerthanorequalto() x should not be a list, essentially list bubble up to the top I suppose
	sortstatistics.fieldtests++;
	if(x->type!=VT_LIST){
		Mlistelement* j=l;
		sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
		while(1){
			sortstatistics.fieldtests+=2;
			if(j->index>=h->index)break;
			long long notlarger=M_LL_INVALID;
			sortstatistics.fieldtests+=2;
			if(j->_value->type!=VT_LIST){
				notlarger=smallerthanorequalto(j->_value,x);
				sortstatistics.comparisons++;
			}
			if(notlarger==M_TRUE){
				i=(i!=NULL?i->_next:list->_first); //* i++; // make i start at index l otherwise increment
				sortstatistics.pointerassignments++;sortstatistics.pointertests++;sortstatistics.fieldreferences++;
				lswap(i,j);
			}
			j=j->_next;
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
		}
		/* replacing:
		for(Mlistelement* j=l;j->index<h->index;j=j->_next){ // int j=1;j<h;j++
			// output("Comparing element #%zd with element #%zd.\n",j->index,h->index);
			// MDH@02NOV2020: every comparison this way creates a new value which will need to be discarded by the gc which is very wasteful
			//				as such it would make sense to actually not do it this way but prevent the wrapping of the integer result by letting these functions delegate to a function that does not do the wrapping
			//				unless we can find a way to NOT gc this value immediately as it's only a wrapper for the comparison??????
			//				SOLUTION all comparison functions now have a helper function that does the actual comparison but does not wrap the returned result (M_TRUE, M_FALSE or M_LL_INVALID)
			//				careful we have to ascertain that neither is a list
			long long notlarger=(j->_value->type!=VT_LIST?smallerthanorequalto(j->_value,x):M_LL_INVALID);
			if(notlarger==M_TRUE){
				i=(i?i->_next:list->_first); // i++; // make i start at index l otherwise increment
				lswap(i,j);
			}
		}
		*/
	}
	Mlistelement* iplus1=(i!=NULL?i->_next:list->_first);
	sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.fieldtests++;
	lswap(iplus1,h); //* swap(list[i+1],list[h])
	return i; //* i+1 but actually we are returning i itself because that's the first value used
}
// Mlsort performs an inline sort i.e. the input list is rearranged
// helper function to sort a list
// MDH@02NOV2020: how about returning the number of stack values we actually needed to give some additional information
/**
 * @brief quick sorts \p _list in place
 * 
 * @param _list 
 * @return long long M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
static long long lquicksort(Mlist* _list){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=M_LL_INVALID;
	sortstatistics.pointertests++;
	if(_list!=NULL){
		Mlistelement* listelement=_list->_first;
		sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
		sortstatistics.pointertests++;
		if(listelement!=NULL){
			sortstatistics.pointertests++;sortstatistics.fieldtests++;
			if(listelement!=_list->_last){ // at least two items
				// we need a stack of integers with the same size as the list length
				if(report)
					output("Sorting a list of %zd elements with quicksort.\n",_list->numberOfElements);
				// MDH@02NOV2020: best to use CALLOC not calloc
				// MDH@03NOV2020: if there are many many list elements allocating all at once is an issue
				//				how about re-allocating in
				unsigned long long maxtop,initialmaxtop=2*((unsigned long long)ceil(log10(_list->numberOfElements)));
				Mlistelement** stack=MALLOC(sizeof(Mlistelement*),maxtop=initialmaxtop,-'l',owner);
				if(stack!=NULL){
					sortstatistics.memoryallocation+=(sizeof(Mlistelement*)*maxtop);
					if(report)output("Initial quicksort stack size: %llu.\n",maxtop);
					result=2; // the current amount of stack elements used
					stack[0]=NULL; // i.e. the first lmin1
					stack[1]=_list->_last;
					sortstatistics.pointerassignments++;sortstatistics.fieldassignments++;sortstatistics.fieldreferences++;
					// it's better to store the number of elements in the stack instead of the top index
					// so that the smallest value of top will be 0
					unsigned long long top=2;
					Mlistelement *lmin1,*l,*h,*pmin1,*p,*pplus1; // two list elements
					sortstatistics.pointerassignments+=6;
					// as long as there are two elements on the stack
					while(top>1){ // two or more elements on the stack
						h=stack[--top];
						lmin1=stack[--top];
						pmin1=lpartition(_list,lmin1,h); // NOTE p is actually p-1
						sortstatistics.pointerassignments+=3;sortstatistics.fieldreferences+=2;
						//if(!p){result=M_FALSE;outputError("Failed to partition");break;}
						sortstatistics.pointertests++;
						l=(lmin1!=NULL?lmin1->_next:_list->_first);
						sortstatistics.fieldreferences++;sortstatistics.pointerassignments++;
						if(pmin1!=NULL){
							sortstatistics.fieldtests+=2;
							if(pmin1->index>l->index){
								if(top>=maxtop){
									if(report)output("Expanding the stack.\n");
									stack=REALLOC(stack,maxtop,maxtop+initialmaxtop,sizeof(Mlistelement*),-'l');
									if(NULL==stack){output("%sNot enough memory for a stack of %llu list elements in quicksort.\n",M_ERROR_PREFIX,maxtop+initialmaxtop);break;}
									sortstatistics.memoryallocation+=(sizeof(Mlistelement*)*initialmaxtop);
									maxtop+=initialmaxtop;
									if(report)output("Stack of quicksort expanded to contain %llu elements.\n",maxtop);
								}
								stack[top++]=lmin1;
								stack[top++]=pmin1;
								sortstatistics.fieldassignments+=2;sortstatistics.pointerreferences+=2;
								if(top>result)result=top;
							}
						}
						// move p two elements up
						p=(pmin1!=NULL?pmin1->_next:_list->_first);
						pplus1=(p!=NULL?p->_next:_list->_first);
						sortstatistics.fieldreferences+=2;sortstatistics.pointerassignments+=2;sortstatistics.pointertests+=3; // including the one below (of pplus1)
						if(pplus1!=NULL){
							sortstatistics.fieldtests+=2;
							if(pplus1->index<h->index){
								if(top>=maxtop){
									if(report)output("Expanding the stack.\n");
									stack=REALLOC(stack,maxtop,maxtop+initialmaxtop,sizeof(Mlistelement*),-'l');
									if(NULL==stack){output("%sNot enough memory for a stack of %llu list elements in quicksort.\n",M_ERROR_PREFIX,maxtop+initialmaxtop);break;}
									sortstatistics.memoryallocation+=(sizeof(Mlistelement*)*initialmaxtop);									
									maxtop+=initialmaxtop;
									if(report)output("Stack of quicksort expanded to contain %llu elements.\n",maxtop);
								}
								stack[top++]=p; // which is actually lmin1
								stack[top++]=h;
								sortstatistics.fieldassignments+=2;sortstatistics.pointerreferences+=2;
								if(top>result)result=top;
							}
						}
					}
					if(report)output("Freeing the quicksort stack.\n");
					FREE_DISOWNED(stack,maxtop,-'l',owner);
					if(report)output("Quicksort stack freed.\n");
				}else
					outputError("Not enough memory to sort the list with quicksort");
			}else
				result=M_TRUE;
		}else // no need to sort so success
			result=M_TRUE; // will always be different from top (which is always even)
	}
	return result;
}
/*
static long long amergesort(Marray* _array){
	long long result=M_LL_INVALID;
	if(_array){
		
	}
	return result;	
}
static long long lmergesort(Mlist* _list){
	long long result=M_LL_INVALID;
	if(_list){
		
	}
	return result;
}
*/
// Mindexrange and lmerge() used in both harmonicasort and timsort
typedef struct Mindexrange{
	unsigned long long first,last;
	struct Mindexrange* _next;
}Mindexrange;
// helper functions
/*
// abinarymerge is called from aharmonicabinarysort trying to speed up merging by using binary search to find the insert position
static bool abinarymerge(Mvalue** const values,unsigned long long l,unsigned long long m,unsigned long long r,bool report){Mallocationowner owner=getOwner(__LINE__);
	// bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	bool result=false;
	return result;
}
*/
// NOTE in amerge() l, m and r are zero-based
// NOTE amerge is used both in atimsort as in aharmonicasort
static void outputValues(char const * const prefix,char const * const info,Mvalue const * const * const values,unsigned long long length,unsigned long long bugindex){
	// if(prefix||info)output("%s%s",prefix,info);
	output("Sequence of length %llu: ",length);
	outputValue("'",*values,"'");
	for(unsigned long long index=1;index<length;index++){
		output("%c",',');
		if(index>bugindex)output("%c",'*');
		outputValue("'",*(values+index),"'");
	}
	output(".\n");
}
// the default amerge copies the presumable original values, then uses the copy to overwrite the original with the new values
/**
 * @brief merges elements l through m in \p values with elements m+1 through r in \p values
 * 
 * @param values 
 * @param l 
 * @param m 
 * @param r 
 * @param report 
 * @return true on success
 * @return false on failure
 */
static bool amerge(Mvalue** const values,unsigned long long l,unsigned long long m,unsigned long long r,bool report){Mallocationowner owner=getOwner(__LINE__);
	// bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	if(report)
		output("Merging ordered arrays of indices [%llu,%llu] and [%llu,%llu].\n",l,m,m+1,r);
	if(l>m||m>=r)return true; // if either list is empty return true
	bool result=false;
	unsigned long long len1=m-l+1,len2=r-m; // OOPS adding +1; (as I did previously) would definitely be wrong!!!!
	// allocate memory to store all value pointers so we can overwrite the originals
	// TODO can we do this without actually having to do this??????
	sortstatistics.pointerassignments+=2;
	Mvalue **left=MALLOC(sizeof(Mvalue*),len1,-'v',owner),**right=MALLOC(sizeof(Mvalue*),len2,-'v',owner);
	if(left!=NULL)sortstatistics.memoryallocation+=(sizeof(Mvalue*)*len1);
	if(right!=NULL)sortstatistics.memoryallocation+=(sizeof(Mvalue*)*len2);
	if(left!=NULL&&right!=NULL){
		// ASSERT both left and right (if needed) defined
		result=true;
		// copy the pointers over
		sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;memcpy(left,values+l,sizeof(Mvalue*)*len1);
		sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;memcpy(right,values+m+1,sizeof(Mvalue*)*len2);
		if(report)
		{
			output("\tTwo sequences of length %llu and %llu, respectively.\n",len1,len2);
			outputValue("\ti.e. [",*(values+l),",");outputValue(NULL,*(values+m),"] and ");
			outputValue("[",*(values+m+1),",");outputValue(NULL,*(values+r),"].\n");
		}
		unsigned long long i=0,j=0;
		// check
		if(report)
			outputValues("","Checking first sequence: ",left,len1,len1);
		while(++i<len1)if(smallerthan(left[i],left[i-1])==M_TRUE){outputValues(M_BUG_PREFIX,"First sequence is not ordered: ",left,len1,i-1);result=false;break;}
		if(report)
			outputValues("","Checking second sequence: ",right,len2,len2);
		while(++j<len2)if(smallerthan(right[j],right[j-1])==M_TRUE){outputValues(M_BUG_PREFIX,"Second sequence is not ordered: ",right,len2,j-1);result=false;break;}
		if(result){
			if(report)output("Sequences are in correct ascending order!\n");
		}else 
			outputBug("Sequences are not ordered!");
		
		i=j=0;
		sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
		Mvalue **previousvalueholder=NULL,**valueholder=(values+l);
		while(i<len1&&j<len2){
			sortstatistics.comparisons++;sortstatistics.fieldassignments++;sortstatistics.fieldreferences+=3;
			if(smallerthanorequalto(left[i],right[j])){
				*valueholder=left[i++];
				if(report)
					outputValue("->",*valueholder," ");
				// check
				if(previousvalueholder!=NULL&&smallerthan(*valueholder,*previousvalueholder)==M_TRUE)
				{result=false;
				output("%sMerged value #%llu",M_BUG_PREFIX,i);outputValue("'",*valueholder,"' from the first sequence is smaller than");outputValue(" the previously added value '",*previousvalueholder,"'.\n");break;}
			}else{
				*valueholder=right[j++];
				if(report)
					outputValue("<-",*valueholder," ");
				// check
				if(previousvalueholder!=NULL&&smallerthan(*valueholder,*previousvalueholder)==M_TRUE)
				{result=false;
				output("%sMerged value #%llu",M_BUG_PREFIX,j);outputValue("'",*valueholder,"' from the second sequence is smaller than");outputValue(" the previously added value '",*previousvalueholder,"'.\n");break;}
			}
			sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
			previousvalueholder=valueholder++;
		}
		while(i<len1){
			sortstatistics.fieldassignments++;sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;sortstatistics.pointerassignments++;
			*valueholder=left[i++];
			if(report)
				outputValue("->",*valueholder," ");
			// check
			if(previousvalueholder!=NULL&&smallerthan(*valueholder,*previousvalueholder)==M_TRUE)
			{result=false;output("%sAppended value #%llu",M_BUG_PREFIX,i);outputValue("'",*valueholder,"' from the first sequence is smaller than");outputValue(" the previously added value '",*previousvalueholder,"'.\n");break;}
			previousvalueholder=valueholder++;
		}
		while(j<len2){
			sortstatistics.fieldassignments++;sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;sortstatistics.pointerassignments++;
			*valueholder=right[j++];
			if(report)
				outputValue("->",*valueholder," ");
			// check
			if(previousvalueholder!=NULL&&smallerthan(*valueholder,*previousvalueholder)==M_TRUE)
			{result=false;output("%sAppended value #%llu",M_BUG_PREFIX,j);outputValue("'",*valueholder,"' from the second sequence is smaller than");outputValue(" the previously added value '",*previousvalueholder,"'.\n");break;}
			previousvalueholder=valueholder++;
		}
		// outputChar(M_NEWLINE_CHARACTER);
	}
	sortstatistics.pointertests+=2;
	if(left){sortstatistics.pointerreferences++;FREE_DISOWNED(left,len1,-'v',owner);}
	if(right){sortstatistics.pointerreferences++;FREE_DISOWNED(right,len2,-'v',owner);}
	return result;
}
// an possible improvement on amerge() is to only create a copy of the second sequence, so that these positions become available in the merging process
// by go backwards through the second sequence elements we can accomplish to move every value in the first sequence at most once (as intended)
/**
 * @brief alternative to amerge
 * 
 * @param values 
 * @param l 
 * @param m 
 * @param r 
 * @param report 
 * @return true on success
 * @return false on failure
 */
static bool ainsertmerge(Mvalue** const values,unsigned long long l,unsigned long long m,unsigned long long r,bool report){Mallocationowner owner=getOwner(__LINE__);
	// bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	if(report)
		output("Insert merging ordered arrays of indices [%llu,%llu] and [%llu,%llu].\n",l,m,m+1,r);
	if(l>m||m>=r)return true; // if either list is empty return true
	bool result=false;
	unsigned long long len1=m-l+1,len2=r-m; // OOPS adding +1; (as I did previously) would definitely be wrong!!!!
	// allocate memory to store all value pointers so we can overwrite the originals
	// TODO can we do this without actually having to do this??????
	sortstatistics.pointerassignments++;
	Mvalue **right=MALLOC(sizeof(Mvalue*),len2,-'v',owner);
	if(right!=NULL){
		sortstatistics.memoryallocation+=sizeof(Mvalue*)*len2;
		result=true;
		// copy the pointers over
		sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;memcpy(right,values+m+1,sizeof(Mvalue*)*len2);
		if(report)
		{
			output("\tTwo sequences of length %llu and %llu, respectively.\n",len1,len2);
			outputValue("\ti.e. [",*(values+l),",");outputValue(NULL,*(values+m),"] and ");
			outputValue("[",*(values+m+1),",");outputValue(NULL,*(values+r),"].\n");
		}
		// check
		if(report)
			outputValues("","Checking first sequence: ",values+l,len1,len1);
		// the first sequence starts at values+l
		unsigned long long i=0,j=0;
		while(++i<len1)if(smallerthan(values[l+i],values[l+i-1])==M_TRUE){outputValues(M_BUG_PREFIX,"First sequence is not ordered: ",values+l,len1,i-1);result=false;break;}
		if(report)
			outputValues("","Checking second sequence: ",right,len2,len2);
		while(++j<len2)if(smallerthan(right[j],right[j-1])==M_TRUE){outputValues(M_BUG_PREFIX,"Second sequence is not ordered: ",right,len2,j-1);result=false;break;}
		if(result){
			if(report)output("Sequences are in correct ascending order!\n");
		}else
			outputBug("Sequences are not ordered!");

		// NOTE programmatically a much more elegant merge than amerge()
		long long smaller; // the last position (index) where the previous element was inserted
		unsigned long long tomergeinto=len1,toinsert=len2; // keep track of amount to insert
		Mvalue **smallerValueholder,**toinsertValueholder=right+toinsert;
		// what we need to do is insert len2 elements from the second sequence from end to beginning into the first sequence
		do{
			toinsertValueholder--; // go to the next value to insert
			if(report)
			{output("Value #%llu to insert: ",toinsert);outputValue("'",*toinsertValueholder,"'");}
			// the smallest value of the insert position will be zero!!!
			smaller=tomergeinto;
			smallerValueholder=values+l+smaller; // one above the first element to compare against
			// keep going back until we find a smaller predecessor or we've investigated the first value
			// NOTE smaller may become -1 when all remaining (len1) elements in the first sequence are larger
			while(--smaller>=0){
				sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
				smallerValueholder--;
				sortstatistics.comparisons++;sortstatistics.fieldreferences+=2;
				if(smallerthanorequalto(*smallerValueholder,*toinsertValueholder)==M_TRUE)break; // equal to to maintain the original order if possible
			}
			if(report)
			{
				if(smaller>=0){output("Smaller value #%llu:",smaller+1);outputValue("'",*smallerValueholder,"'.\n");}else output("No smaller value found!\n");
			}
			// move all elements one larger than smallerValueholder
			sortstatistics.pointerreferences++;sortstatistics.pointerassignments++;
			if(smaller>=0){smallerValueholder++;smaller++;}else smaller=0; // now pointing to the first element to move
			// NOTE the first time toinsert will equal len2 and that is exactly the number of positions we will need to move up the elements
			sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;
			memmove(smallerValueholder+toinsert,smallerValueholder,sizeof(Mvalue*)*(tomergeinto-smaller));
			toinsert--; // one less to insert
			// prepend the toinsert value to what we've just moved up
			sortstatistics.fieldassignments++;sortstatistics.fieldreferences+=3; // TODO is this correct???
			*(smallerValueholder+toinsert)=*toinsertValueholder;
			tomergeinto=smaller; // what we have left in the first sequence, yes could be zero!
			if(report)
			{
				outputValues("Left to insert",": ",right,toinsert,toinsert);
				outputValues("into",": ",values,tomergeinto,tomergeinto);
			}
			if(report)
				outputValues("Merge result so far",": ",values,len1+len2,len1);
			// the last len2 to process would be zero, so when len2 is zero we stop
		}while(toinsert>0);
		// and we can get rid of right
		sortstatistics.pointerreferences++;
		FREE_DISOWNED(right,len2,-'v',owner);
	}
	return result;
}
// MDH@02DEC2020: we may further limit the number of comparisons by using a binary search when looking for the first smaller value
/**
 * @brief alternative to amerge and ainsertmerge
 * 
 * @param values 
 * @param l 
 * @param m 
 * @param r 
 * @param report 
 * @return true on success
 * @return false on failure
 */
static bool abinaryinsertmerge(Mvalue** const values,unsigned long long l,unsigned long long m,unsigned long long r,bool report){Mallocationowner owner=getOwner(__LINE__);
	// bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	// report=true;
	if(report)
		output("Binary insert merging ordered arrays of indices [%llu,%llu] and [%llu,%llu].\n",l,m,m+1,r);
	if(l>m||m>=r)return true; // if either list is empty return true
	bool result=false;
	unsigned long long len1=m-l+1,len2=r-m; // OOPS adding +1; (as I did previously) would definitely be wrong!!!!
	// allocate memory to store all value pointers so we can overwrite the originals
	// TODO can we do this without actually having to do this??????
	sortstatistics.pointerassignments++;
	Mvalue **right=MALLOC(sizeof(Mvalue*),len2,-'v',owner);
	if(right!=NULL){
		sortstatistics.memoryallocation+=(sizeof(Mvalue*)*len2);
		result=true;
		// copy the pointers over
		sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;memcpy(right,values+m+1,sizeof(Mvalue*)*len2);
		if(report)
		{
			output("\tTwo sequences of length %llu and %llu, respectively.\n",len1,len2);
			outputValue("\ti.e. [",*(values+l),",");outputValue(NULL,*(values+m),"] and ");
			outputValue("[",*(values+m+1),",");outputValue(NULL,*(values+r),"].\n");
		}
		// check
		if(report)
			outputValues("","Checking first sequence: ",values+l,len1,len1);
		// the first sequence starts at values+l
		unsigned long long i=0,j=0;
		while(++i<len1)if(smallerthan(values[l+i],values[l+i-1])==M_TRUE){outputValues(M_BUG_PREFIX,"First sequence is not ordered: ",values+l,len1,i-1);result=false;break;}
		if(report)
			outputValues("","Checking second sequence: ",right,len2,len2);
		while(++j<len2)if(smallerthan(right[j],right[j-1])==M_TRUE){outputValues(M_BUG_PREFIX,"Second sequence is not ordered: ",right,len2,j-1);result=false;break;}
		if(result){
			if(report)output("Sequences are in correct ascending order!\n");
		}else
			outputBug("Sequences are not ordered!");
		
		// NOTE programmatically a much more elegant merge than amerge()
		long long smaller; // the last position (index) where the previous element was inserted
		unsigned long long tomergeinto=len1,toinsert=len2; // keep track of amount to insert
		unsigned long long middle,upper;
		Mvalue **smallerValueholder,**toinsertValueholder=right+toinsert;
		// what we need to do is insert len2 elements from the second sequence from end to beginning into the first sequence
		do{
			toinsertValueholder--; // go to the next value to insert
			if(report)
			{output("Value #%llu to insert: ",toinsert);outputValue("'",*toinsertValueholder,"'");}
			
			// here's the part where we use a binary search instead of a linear search (backwards) in a quest to determine `smaller` which can range from -1 through tomergeinto-1
			// NOTE immediately determining the value of smaller+1 so it will range from [0,tomergeinto] afterwards
			if(tomergeinto>0){ // values to compare to
				// is the last value smaller than or equal to? if so it is the smaller value
				sortstatistics.comparisons++;
				if(largerthanorequalto(*toinsertValueholder,*(values+l+tomergeinto-1))!=M_TRUE){ // the value to insert is NOT larger than or equal to the last sequence value
					sortstatistics.comparisons++;
					if(smallerthan(*toinsertValueholder,*(values+l))!=M_TRUE){ // the value to insert is NOT smaller than the first sequence value
						smaller=0;
						upper=tomergeinto-1;
						while(smaller+1!=upper){ // there's still something in between (i.e. at least two values)
							middle=(smaller+upper)>>1;
							sortstatistics.comparisons++;
							if(smallerthan(*toinsertValueholder,*(values+l+middle))==M_TRUE)
								upper=middle;
							else
								smaller=middle;
						}
						smaller++;
					}else
						smaller=0;
				}else
					smaller=tomergeinto;
			}else
				smaller=0;
			smallerValueholder=values+l+smaller;

			if(report)
			{
				output("First value not smaller #%llu:",smaller+1);outputValue("'",*smallerValueholder,"'.\n");
			}

			// move all elements one larger than smallerValueholder
			// NOTE the first time toinsert will equal len2 and that is exactly the number of positions we will need to move up the elements
			sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;
			memmove(smallerValueholder+toinsert,smallerValueholder,sizeof(Mvalue*)*(tomergeinto-smaller));
			toinsert--; // one less to insert
			// prepend the toinsert value to what we've just moved up
			sortstatistics.fieldassignments++;sortstatistics.fieldreferences+=3; // TODO is this correct???
			*(smallerValueholder+toinsert)=*toinsertValueholder;
			tomergeinto=smaller; // what we have left in the first sequence, yes could be zero!
			if(report)
			{
				outputValues("Left to insert",": ",right,toinsert,toinsert);
				outputValues("into",": ",values,tomergeinto,tomergeinto);
			}
			if(report)
				outputValues("Merge result so far",": ",values,len1+len2,len1);
			// the last len2 to process would be zero, so when len2 is zero we stop
		}while(toinsert>0);
		// and we can get rid of right
		sortstatistics.pointerreferences++;
		FREE_DISOWNED(right,len2,-'v',owner);
	}
	return result;
}

// MDH@04NOV2020: due to the problem with the index values (which we need to keep in ascending order)
//				it's easier to simply merge in all second sequence values into the first sequence values
// MDH@05NOV2020: lmerge does not need to keep the index values ascending so it can safely
//				exchange the position of list elements in the list
/**
 * @brief merges \p _list elements 1 + \p beforefirstone through \p lastone with elements 1 + \p lastone through \p lastanother
 * 
 * @param _list 
 * @param beforefirstone 
 * @param lastone 
 * @param lastanother 
 * @return Mlistelement* the pointer to the first merged element
 */
static Mlistelement* lmerge(Mlist * const _list,Mlistelement * const beforefirstone,Mlistelement * const lastone,Mlistelement * const lastanother){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);

	Mlistelement* nextlastanother=lastanother->_next; // remember the successor of the current last another
	// as compared to lmerge (which simply is sort of an insertion algorithm at the moment (although it could be improved on though))
	Mlistelement* mergedlistelement=beforefirstone; // the current head of the merged list elements

	// initialize one and another to the first two elements we will need to compare
	Mlistelement *one=(beforefirstone?beforefirstone->_next:_list->_first),*another=lastone->_next;

	if(report){
		outputValue("First value to first sequence to merge: '",one->_value,"'.\n");
		outputValue("Last value of first sequence to merge: '",lastone->_value,"'.\n");
		outputValue("First value of second sequence to merge: '",another->_value,"'.\n");
		outputValue("Last value of second sequence to merge: '",lastanother->_value,"'.\n");
	}

	// initialize the two values to compare 
	Mvalue *oneValue=one->_value,*anotherValue=another->_value;

	Mlistelement *smallest=one; // keep track of the smallest list element as we will need to 

	sortstatistics.pointerassignments+=7;sortstatistics.pointerreferences+=2;
	sortstatistics.fieldreferences+=5;
	sortstatistics.pointertests++;

	while(one!=NULL&&another!=NULL){
		sortstatistics.pointertests+=2; // one&&another
		sortstatistics.comparisons++; // next
		// determine the first one that is larger than another
		if(smallerthanorequalto(oneValue,anotherValue)==M_TRUE){ // one<=another
			if(mergedlistelement)mergedlistelement->_next=one;else _list->_first=one;
			mergedlistelement=one;
			if(one!=lastone){
				one=one->_next;oneValue=one->_value;
				sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences+=2;
			}else{
				one=NULL;
				sortstatistics.pointerassignments++;
			}
		}else{
			if(mergedlistelement)mergedlistelement->_next=another;else _list->_first=another;
			mergedlistelement=another;
			if(another!=lastanother){
				another=another->_next;anotherValue=another->_value;
				sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences+=2;
			}else{
				another=NULL;
				sortstatistics.pointerassignments++;
			}
		}
		sortstatistics.pointerassignments++;sortstatistics.pointerreferences+=2;
		sortstatistics.fieldassignments++;
		sortstatistics.pointertests+=3;
	}
	// consume the rest of the ones or anothers (either one is left), which is easy because all the ones are already linked correctly
	if(one!=NULL){ // rest of the ones to consume
		if(mergedlistelement!=NULL)mergedlistelement->_next=one;else _list->_first=one;
		mergedlistelement=lastone;
	}else{ // rest of the anothers to consume
		if(mergedlistelement!=NULL)mergedlistelement->_next=another;else _list->_first=another;
		mergedlistelement=lastanother;
	}
	// link the head of the merged list elements to the successor of the original last another
	mergedlistelement->_next=nextlastanother;
	if(NULL==nextlastanother){_list->_last=mergedlistelement;sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;}
	sortstatistics.pointerassignments++;
	sortstatistics.pointerreferences+=3;
	sortstatistics.fieldassignments+=2;
	sortstatistics.pointertests+=3;
	return mergedlistelement;
}
/* lmerge replaces linsertingmerge by NOT receiving the beforefirstone but instead the firstone, and returning 
// we solve the problem of requiring both the new start and end by 
static Mlistelement* lmerge(Mlist * const _list,Mlistelement * const beforefirstone,Mlistelement * const lastone,Mlistelement * const lastanother){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);

	Mlistelement* nextlastanother=lastanother->_next; // remember the successor of the current last another

	// as compared to lmerge (which simply is sort of an insertion algorithm at the moment (although it could be improved on though))
	Mlistelement* mergedlistelement=beforefirstone; // the current head of the merged list elements

	// initialize one and another to the first two elements we will need to compare
	Mlistelement *one=(beforefirstone?beforefirstone->_next:_list->_first),*another=lastone->_next;
	if(report){
		outputValue("First value to first sequence to merge: '",one->_value,"'.\n");
		outputValue("Last value of first sequence to merge: '",lastone->_value,"'.\n");
		outputValue("First value of second sequence to merge: '",another->_value,"'.\n");
		outputValue("Last value of second sequence to merge: '",lastanother->_value,"'.\n");
	}

	// initialize the two values to compare 
	Mvalue *oneValue=one->_value,*anotherValue=another->_value;

	Mlistelement *smallest=one; // keep track of the smallest list element as we will need to 
	while(one&&another){
		// determine the first one that is larger than another
		if(smallerthanorequalto(oneValue,anotherValue)==M_TRUE){ // one<=another
			sortstatistics.comparisons++;
			if(mergedlistelement)mergedlistelement->_next=one;else _list->_first=one;
			mergedlistelement=one;
			if(one!=lastone){one=one->_next;oneValue=one->_value;}else one=NULL;
		}else{
			if(mergedlistelement)mergedlistelement->_next=another;else _list->_first=another;
			mergedlistelement=another;
			if(another!=lastanother){another=another->_next;anotherValue=another->_value;}else another=NULL;
		}
	}
	// consume the rest of the ones or anothers (either one is left), which is easy because all the ones are already linked correctly
	if(one){ // rest of the ones to consume
		if(mergedlistelement)mergedlistelement->_next=one;else _list->_first=one;
		mergedlistelement=lastone;
	}else{ // rest of the anothers to consume
		if(mergedlistelement)mergedlistelement->_next=another;else _list->_first=another;
		mergedlistelement=lastanother;
	}
	// link the head of the merged list elements to the successor of the original last another
	mergedlistelement->_next=nextlastanother;
	if(!nextlastanother)_list->_last=mergedlistelement;
	return mergedlistelement;
}
*/
// harmonica sort helper function
/**
 * @brief reverses elements \p firstindex through \p lastindex in \p values
 * 
 * @param values 
 * @param firstindex 
 * @param lastindex 
 * @param report 
 */
static void areverse(Mvalue** values,unsigned long long firstindex,unsigned long long lastindex,bool report){
	if(report)
	{outputValue("Reversing: '",*(values+firstindex),"'");outputValue(" through '",*(values+lastindex),"'.\n");}
	// the indices will become the same if there's an odd number of values to reverse
	Mvalue* value;
	while(lastindex>firstindex){
		// outputValue("'",*(values+firstindex),"' and ");outputValue("'",*(values+lastindex),"' exchanged to ");
		value=*(values+firstindex);*(values+firstindex)=*(values+lastindex);*(values+lastindex)=value; // exchange the values
		// outputValue("'",*(values+firstindex),"' and ");outputValue("'",*(values+lastindex),"'.\n");
		firstindex++;lastindex--; // go to the next indices
	}
}
// reversing the order of list elements is expected to be much faster than having to insert into a large list, where merging would be possible!!!
// CHANGING first -> ... -> last -> afterlast TO last -> ... -> first -> afterlast
//		  essentially all the successors change but beforefirst should now point to last I suppose
/**
 * @brief reverses elements 1 + \p beforefirst through \p last in \p _list
 * 
 * @param _list 
 * @param beforefirst 
 * @param last 
 * @param report 
 */
static void lreverse(Mlist* _list,Mlistelement * const beforefirst,Mlistelement * const last,bool report){
	// ASSERT beforelist can be NULL, but last should NOT
	Mlistelement* first=(beforefirst!=NULL?beforefirst->_next:_list->_first);
	if(report)
	{outputValue("Reversing: '",first->_value,"'");outputValue(" through '",last->_value,"'.\n");}
	if(beforefirst!=NULL){ // not at the start of the list
		beforefirst->_next=last;
		if(report)
		{outputValue("Successor of '",beforefirst->_value,"'");outputValue(" set to '",beforefirst->_next->_value,"'.\n");}
	}else{ // at the start of the list
		_list->_first=last;
		if(report)
			outputValue("First list element changed to '",_list->_first->_value,"'.\n");
	}
	Mlistelement* afterlast=last->_next; // salvage the current successor of last
	if(NULL==afterlast){
		_list->_last=first; // if last is the last element in the list, first will be the new last element of the list
		sortstatistics.pointerreferences++;sortstatistics.fieldassignments++;
		if(report)
			outputValue("Last list element changed to '",_list->_last->_value,"'.\n");
	}

	// ASSERT beforefirst and afterlast should be connected meaning that if you next up from beforefirst upwards, you'd end up at afterlast
	Mlistelement *nextnextlistelement,*listelement=first,*nextlistelement=first->_next;
	sortstatistics.pointerassignments+=6;sortstatistics.pointerreferences+=2;
	sortstatistics.fieldassignments++;sortstatistics.fieldreferences+=3;
	sortstatistics.pointertests+=3;
	while(listelement!=NULL){
		sortstatistics.pointertests++;
		if(report)
		{outputValue("Next list element '",nextlistelement->_value,"'");outputValue(" to point to '",listelement->_value,"'.\n");}
		// ASSERT originalnextlistelement should be the original next of listelement
		// i.e. we want to make originalnextlistelement->_next equal to listelement
		// NOTE that listelement itself is no longer pointing to originalnextlistelement, so the situation is <-listelement | nextlistelement->
		// step 1: remember the current successor of the originalnextlistelement
		nextnextlistelement=nextlistelement->_next;
		// step 2: CHANGING <- listelement | nextlistelement -> INTO <- listelement | <- nextlistelement
		nextlistelement->_next=listelement;
		sortstatistics.pointerassignments++;sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;
		sortstatistics.pointertests+=2; // see the comparison below
		if(nextlistelement==last)break;
		// step 3: make list element equal to its original successor (which is nextlistelement) and nextlistelement to it's original successor which is nextnextlistelement
		listelement=nextlistelement;
		nextlistelement=nextnextlistelement;
		sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences+=2;
	}
	// make first->_next point to the original successor of last
	first->_next=afterlast;
	sortstatistics.pointerreferences++;sortstatistics.fieldassignments++;
	if(report){
		outputValue("'",first->_value,"' now pointing to");
		if(afterlast)outputValue("': '",afterlast->_value,"'.\n");else output(" nothing!");
	}
}
// we'd like to pass the merge function to use depending
/**
 * @brief array merge function definition
 * 
 */
typedef bool (*ArrayMergeFunction)(Mvalue** const values,unsigned long long l,unsigned long long m,unsigned long long r,bool report);

// lharmonicasort is the original harmonicasort which is quite slow
/**
 * @brief harmonica sorts \p _array using \p arrayMergeFunction as array merge function
 * 
 * @param _array 
 * @param arrayMergeFunction 
 * @return long long 
 */
static long long aharmonicasort(Marray* _array,ArrayMergeFunction arrayMergeFunction){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	bool result=M_LL_INVALID;
	if(_array!=NULL){
		result=M_TRUE;
		clock_t sortstart=clock();
		unsigned long long arraylength=_array->numberOfElements;
		if(arraylength>1){ // at least two elements
			Mvalue** values=_array->values; // address of the first Mvalue pointer
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
			Mvalue** current=values; // initialize current to the address of first Mvalue pointer
			// skip all equal values so we can set the rundirection to either -1 or 1
			// obviously all elements could be equal
			unsigned long long arrayindex=0; // effectively counting the number of equal elements
			do{
				sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
				arrayindex++;
				current++; // increment current
				sortstatistics.comparisons++;sortstatistics.pointerreferences+=2;
				if(equalto(*values,*current)!=M_TRUE)break; // if not equal break
			}while(arrayindex<arraylength);
			// ASSERT current is the first unequal element (which is values+arrayindex)
			
			if(arrayindex<arraylength){ // not all array elements are equal!!!
				
				// we're either going up or down
				sortstatistics.pointerreferences+=2;sortstatistics.comparisons++; // the comparison below
				int rundirection=(largerthan(*current,*values)==M_TRUE?1:-1);
				
				// we'll be detecting the natural 'runs' and whenever the direction changes we will get the stuff behind it sorted
				// original code from Mrunpoints() below
				long long largest=-1;
				Mvalue** previous;
				sortstatistics.pointerassignments+=2;
				// MDH@01DEC2020: deciding to always also merge the end of the down/up run, therefore there's never an empty run!!! removing: bool emptyrun;
				// if we lower arraylength by 1, we do not need to increment arrayindex at the start
				arraylength--;
				while(arrayindex<arraylength){
					previous=current++; // we can update previous and current in one go, after which arrayindex points at previous!!!!
					sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;
					if(report)
					{outputValue("Comparing '",*current,"'");outputValue(" with '",*previous,"'.\n");}

					if(rundirection>0){ // in an up run
						sortstatistics.pointerreferences+=2;sortstatistics.comparisons++;
						if(smallerthan(*current,*previous)==M_TRUE){ // end of up run reached
							// we know that we need to merge the ended up run with the main up run (if any)
							if(report)
								outputValue("Up run maximum: '",*previous,"'.\n");
							sortstatistics.pointertests++;
							if(largest>=0&&!arrayMergeFunction(values,0,largest,arrayindex,report)){
								result=M_FALSE;
								outputError("Failed to merge an up run!");
								break;
							}
							largest=arrayindex;
							rundirection=-1;
							if(report)
							{outputValue("Extremes after merging up run: minimum='",*values,"'");outputValue(" - maximum='",*(values+largest),"'.\n");}
							/*
							if(report)
							{_array->numberOfElements=arrayindex+1;outputArray("The part of the array after merging an up run: '",_array,"'.\n");}
							*/
						}
					}else{ // in a down run
						if(largerthan(*current,*previous)==M_TRUE){ // // end of down run reached
							if(report)
								outputValue("Down run minimum: '",*previous,"'.\n");
							// we always need to reverse the ended down run (even if there's no main up run!!!)
							sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
							areverse(values,largest+1,arrayindex,report);
							if(report)
								output("Down run reversed!\n");
							// we only need to merge if largest>=0
							if(largest>=0&&!arrayMergeFunction(values,0,largest,arrayindex,report)){
								result=M_FALSE;
								outputError("Failed to merge an up run!");
								break;
							}
							largest=arrayindex;
							rundirection=1;
							if(report)
							{outputValue("Extremes after merging down run: minimum='",*values,"'");outputValue(" - maximum='",*(values+largest),"'.\n");}
							/*
							if(report)
							{_array->numberOfElements=arrayindex+1;outputArray("The part of the array after reversing the down run: '",_array,"'.\n");}
							*/
						}
					}
					arrayindex++;
				}
				// ASSERT arrayindex is equal to the index of the last element (currently arraylength)
				// take care of the last run
				if(result==M_TRUE){
					if(rundirection>0){ // end of an up run
						// obviously, if largest does not have a value yet, the list was already in ascending order to start with in which case we have nothing left to do!!
						if(largest>=0&&!arrayMergeFunction(values,0,largest,arrayindex,report)){
							result=M_FALSE;
							outputError("Failed to merge the final up run!");
						}
						/*
						if(report)
						{_array->numberOfElements=arrayindex+1;outputArray("The array after merging the final up run: '",_array,"'.\n");}
						*/
					}else
					if(rundirection<0){ // end of the down run
						sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
						// if we reverse from largest+1 instead of largest we can merge [0,largest] with [largest+1,arrayindex]
						areverse(values,largest+1,arrayindex,report);
						if(report)
							output("Final down run reversed!\n");
						if(largest>=0&&!arrayMergeFunction(values,0,largest,arrayindex,report)){
							result=M_FALSE;
							outputError("Failed to merge the final down run!");
						}
						/*
						if(report)
						{_array->numberOfElements=arrayindex+1;outputArray("The array after merging the final down run: '",_array,"'.\n");}
						*/
					}
					_array->numberOfElements=++arraylength; // in case we changed it (for display purposes)
				}
			}
		}
		if(result==M_TRUE){
			double duration=clock()-sortstart;
			if(report)
			{
				outputArray("The sorted array: '",_array,"'.\n");
				outputValue("First: '",*_array->values,"'.\n");
				outputValue("Last: '",*(_array->values+_array->numberOfElements-1),"'.\n");
			}
			output("Duration of array sorting by harmonicasort: %.3f ms.\n",duration/M_CLOCKS_PER_MS);
		}
	}
	return result;
}
/**
 * @brief lharmonica sorts \p _list 
 * @details returns M_LL_INVALID when _list is NULL
 * @param _list 
 * @return long long returns M_TRUE on success, or M_FALSE or M_LL_INVALID on failure
 */
static long long lharmonicasort(Mlist* _list){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=(_list!=NULL?M_TRUE:M_LL_INVALID);
	if(result==M_TRUE){
		Mlistelement* previous=_list->_first; // where we'll be keeping the first element in the list
		sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
		if(previous!=NULL){ // at least two elements in the list
			Mlistelement* current=previous->_next; // the first element to compare
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
			if(current!=NULL){
				while(1){
					sortstatistics.comparisons++;
					if(!equalto(previous->_value,current->_value))break;
					previous=current;
					current=current->_next;
					sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
					if(NULL==current)break;
				}
			}
			// skip all equal values so we can set the rundirection to either -1 or 1
			// obviously all elements could be equal
			sortstatistics.pointertests++; // the test below
			if(current!=NULL){ // at least 2 unequal elements in the list
				// we're either going up or down
				sortstatistics.comparisons++; // the comparison below
				int rundirection=(largerthan(current->_value,previous->_value)==M_TRUE?1:-1);
				// advance current (NOTE if the list has only two elements, current would then be NULL)
				previous=current;current=current->_next; 
				sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;
				
				// determine the index ranges
				Mindexrange indexrange={_list->_first->index,_list->_first->index};
				Mindexrange* nextindexrange=&indexrange;
				if(_list->_last->index>_list->numberOfElements){ // possibly multiple range
					Mlistelement* listelement=_list->_first;
					while(1){
						listelement=listelement->_next;
						if(NULL==listelement)break;
						if(listelement->index!=nextindexrange->last+1){ // there's a gap
							nextindexrange->_next=CALLOC_1(sizeof(Mindexrange),'~',owner);
							nextindexrange=nextindexrange->_next;
							if(NULL==nextindexrange)break; // TODO serious problem
							nextindexrange->first=listelement->index;
						}
						// update last
						nextindexrange->last=listelement->index;
					}
				}else
					nextindexrange->last=_list->_last->index;

				// we'll be detecting the natural 'runs' and whenever the direction changes we will get the stuff behind it sorted
				// original code from Mrunpoints() below
				Mlistelement *toinsert,*nexttoinsert,*compare,*lastcompare,*runsmallest,*runlargest;
				Mlistelement *smallest=NULL,*largest=NULL;
				Mvalue *currentValue=NULL,*previousValue=previous->_value;
				sortstatistics.pointerassignments+=10;sortstatistics.fieldreferences++; // NOTE counting the declarations as well but if that will really make a difference
				bool emptyrun;
				while(1){
					sortstatistics.pointertests++;
					if(NULL==current)break;
					currentValue=current->_value; // the value to compare
					sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
					if(report){outputValue("Comparing '",currentValue,"'");outputValue(" with '",previousValue,"'.\n");}
					sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;sortstatistics.comparisons++; // this we know will always happen
					if(rundirection>0){ // in an up run
						if(smallerthan(currentValue,previousValue)==M_TRUE){
							// we have to merge the down and the up run to an single up run i.e. \/ to / where the first \ is from largest to smallest
							// ASSERT all the elements in \ (largest to smallest) are larger than smallest so we know the following loop always ends
							runlargest=previous;
							if(report)outputValue("Up run maximum: '",previousValue,"'.\n");
							sortstatistics.pointertests++;
							if(smallest!=NULL){
								emptyrun=(smallest->_next==previous);
								sortstatistics.fieldtests++;sortstatistics.pointertests++;
							}else
								emptyrun=false;
							if(!emptyrun){
								// both the main run is up, as well as the run we just finished
								// if we have a main run we have a largest, if we do not have a largest this up run is to become the main run
								// MDH@08NOV2020: if I'm right you can use runsmallest as initial insertion point
								//				apparently NOT as it loops indefinitely somewhere
								// MDH@09NOV2020: OOPS we're supposed to pass in the beforefirst NOT the first, so how do we find the predecessor of runsmallest?????
								sortstatistics.pointertests++;
								if(largest!=NULL)lmerge(_list,NULL/*runsmallest*/,largest,previous);
								smallest=_list->_first;
								sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.fieldtests++;sortstatistics.pointertests++;
								if(previous->_next==current){largest=previous;sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;}

								if(report)outputValue("Maximum so far: '",largest->_value,"'.\n");
								if(report)outputList("List so far: '",_list,"'.\n");
							}else
							if(report)output("The up run is empty!\n");
							rundirection=-1;
						}
					}else{ // in a down run
						if(largerthan(currentValue,previousValue)==M_TRUE){ // switching to an up run
							runsmallest=previous;
							if(report)outputValue("Down run minimum: '",previousValue,"'.\n");
							// if there's nothing in between the down run is empty
							sortstatistics.pointertests++;
							if(largest!=NULL){
								emptyrun=(largest->_next==previous);
								sortstatistics.fieldtests++;sortstatistics.pointertests++;
							}else
								emptyrun=false;
							if(!emptyrun){
								// we have to reverse the down sequence i.e. the successor of largest through runsmallest (previous)
								// NOTE previous->_next will change and no longer point to current, but largest will subsequently point to current (as it should)
								sortstatistics.pointertests++;
								if(largest!=NULL){
									previous=largest->_next; // is the element containing the maximum of the down run (we can do this because previous is not used anymore until it is reset at the end of the loop)
									sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.fieldtests++;
									lreverse(_list,largest,runsmallest,report); // NOTE if this is the first run, largest will be NULL, so we have to pass the list to lreverse so it can determine the successor of beforefirst
									if(report)
										outputList("List after reversing the down list: '",_list,"'.\n");
									// now that the down run is transformed into an up run we can merge the sorted part so far with the upped run
									// oops, due to the reverse previous is no longer the largest value in the down run, you should use the successor of largest
									lmerge(_list,NULL,largest,previous);
								}else{ // there's no main up run, so we only need to reverse this down run at the beginning of the list
									largest=_list->_first; // obviously
									sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
									lreverse(_list,NULL,runsmallest,report); // NOTE if this is the first run, largest will be NULL, so we have to pass the list to lreverse so it can determine the successor of beforefirst
									if(report)
										output("Initial down run reversed!\n");
								}						
								smallest=_list->_first; // TODO we might not need to do this actually
								sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
								// ASSERT we've successfully merged the down run into the up run that we're going to end up with
								if(report)
								{outputValue("Minimum so far: '",smallest->_value,"'.\n");outputList("List so far: '",_list,"'.\n");}
							}else
							if(report)
								output("The down run is empty!\n");
							rundirection=1;
						}
					}
					// and the next one!!
					previous=current;previousValue=currentValue; // update previous
					current=current->_next;
					sortstatistics.pointerassignments+=3;sortstatistics.pointerreferences+=2;sortstatistics.fieldreferences++;
				}
				// take care of the last run
				sortstatistics.pointertests++; // test on largest below
				if(rundirection>0){ // end of an up run
					// obviously, if largest does not have a value yet, the list was already in ascending order to start with in which case we have nothing left to do!!
					if(largest!=NULL){
						if(report)output("Processing the final up run!\n");
						lmerge(_list,NULL,largest,previous);
					}else
					if(report)
						output("The list was already in ascending order.\n");
				}else
				if(rundirection<0){ // end of the down run
					if(report)output("Processing the final down run!\n");
					runsmallest=previous;
					sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
					if(largest!=NULL){ // something in front that we need to merge the down run into
						// nothing to reverse if there's only a single element in the down run
						sortstatistics.fieldtests++;sortstatistics.pointertests++;
						if(largest->_next!=previous){
							previous=largest->_next;
							sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
							lreverse(_list,largest,runsmallest,report);
						}
						lmerge(_list,NULL,largest,previous);
					}else
						lreverse(_list,NULL,runsmallest,report);
					// because it's a down run the maximum will be the last element in the list after reversal
				}
				
				// _list->_last=largest; // TODO this seems to be a valid assumption

				if(report)
				{
					outputList("The sorted list: '",_list,"'.\n");
					if(_list->_first)outputValue("First: '",_list->_first->_value,"'.\n");else outputBug("No first!");
					if(_list->_last)outputValue("Last: '",_list->_last->_value,"'.\n");else outputBug("No last!");
				}

				// reapply the collected indices from the index ranges
				nextindexrange=&indexrange;
				unsigned long long index=nextindexrange->first; // the fist index to assign
				Mlistelement* listelement=_list->_first;
				while(1){
					listelement->index=index;
					listelement=listelement->_next;
					if(NULL==listelement)break;
					// update the index to assign
					if(index==nextindexrange->last){
						nextindexrange=nextindexrange->_next;
						index=nextindexrange->first;
					}else
						index++;
				}
				// free all dynamically allocated index ranges
				Mindexrange* indexrangetofree=indexrange._next;
				while(indexrangetofree!=NULL){
					nextindexrange=indexrangetofree->_next;
					FREE_DISOWNED_1(indexrangetofree,'~',owner);
					indexrangetofree=nextindexrange;
				}
			}
		}
	}
	return result;
}
const unsigned long long M_RUN_LENGTH=32;
// calling the improved version harmonica binary sort which keeps waypoints on the part already sorted, to speed up merging
// the general idea is to keep every M_RUN_LENGTH list element of the already sorted list, so we can use binary sort to find the lower boundary of what we're inserting
// if we run out of memory we simply double the space between the remembered list elements
/**
 * @brief returns the integer square root of \p n
 * 
 * @param n 
 * @return unsigned long long the integer square root of \p n
 */
static unsigned long long llsqrt(unsigned long long n){
	unsigned long long q=1;
	while(q<=n)q<<=2;
	long long t,r=0;
	while(q>1){
		q>>=2;
		t=-r;
		t+=n;
		t-=q;
		r>>=1;
		if(t>=0){n=t;r+=q;}
	}
	return r;
}
/**
 * @brief returns the list element with value smaller than or equal to \p value
 * @details \p numberoflistelements list elements starting with \p listelements are to be searched
 * @param listelements 
 * @param numberoflistelements 
 * @param value 
 * @return Mlistelement* the list element with value smaller than or equal to \p value
 */
static Mlistelement* binarysearch(Mlistelement** listelements,unsigned long long numberoflistelements,Mvalue* value){
	if(NULL==listelements||numberoflistelements==0||smallerthan(value,(*listelements)->_value)==M_TRUE)return NULL;
	unsigned long long lastindex=--numberoflistelements;
	Mlistelement* last=listelements[lastindex];
	if(NULL==last||smallerthan(last->_value,value)==M_TRUE)return last;
	unsigned long long middleindex,firstindex=0;
	while(lastindex>firstindex+1){
		middleindex=(firstindex+lastindex)>>1;
		if(largerthan(listelements[middleindex]->_value,value)==M_TRUE)
			lastindex=middleindex;
		else
			firstindex=middleindex;
	}
	return listelements[firstindex];
}
// MDH@11NOV2020: the stack multiplier tells us how many times the stack size is to be multiplied with
/**
 * @brief lharmonica binary sorts \p _list
 * @details returns M_LL_INVALID only when \p _list is NULL
 * @param _list 
 * @param stackmultiplier 
 * @return long long M_TRUE on success, or M_FALSE or M_LL_INVALID on failure
 */
static long long lharmonicabinarysort(Mlist* const _list,long long stackmultiplier){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=(_list!=NULL?M_TRUE:M_LL_INVALID);
	if(result==M_TRUE){
		Mlistelement* previous=_list->_first; // where we'll be keeping the first element in the list
		if(report)
			outputValue("First value: '",previous->_value,"'.\n");
		sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
		if(previous!=NULL){ // at least two elements in the list
			Mlistelement* current=previous->_next; // the first element to compare
			if(report)
				outputValue("Second value: '",current->_value,"'.\n");
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
			
			// skip all equal values so we can set the rundirection to either -1 or 1
			// obviously all elements could be equal
			sortstatistics.pointertests++; // the test below
			if(current!=NULL){ // at least 2 unequal elements in the list

				// advance current (NOTE if the list has only two elements, current would then be NULL)
				/* NO
				previous=current;current=current->_next; 
				sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;
				*/
				// for use in binary search we'll be using a stack of list elements
				// but instead of presorting (as we intended originally) we simply place them at the start, and populate the stack if stacked elements were actually sorted
				Mlistelement** stack=NULL;
				unsigned long long betweenstackelements=llsqrt(_list->numberOfElements);
				/*
				if(stackmultiplier!=0)betweenstackelements/=stackmultiplier; // MDH@11NOV2020: divide by the stack multiplier if need be
				if(betweenstackelements<2)betweenstackelements=2; // MDH@11NOV2020: the minimum should be two elements
				*/
				/*
				unsigned long long leftbeforestack;
				Mlistelement* stacktop=NULL;
				*/
				// MDH@17NOV2020: the number of elements in the stack should be at most half the number of elements
				unsigned long long processedsofar=0,stacksize=(stackmultiplier!=0?_list->numberOfElements/MAX(2,betweenstackelements/llabs(stackmultiplier)):0); // the maximum number of stack elements we're going to need
				long long stackminimumindex=0,stackmaximumindex=-1;
				if(report)
					output("Stack size: %llu.\n",stacksize);
				if(stacksize>=3){
					stack=CALLOC(sizeof(Mlistelement*),stacksize,-'l',owner);
					if(stack!=NULL){
						if(report)
							output("Stack address: '%p'.\n",stack);
						if(stackmultiplier<0){ // the stack is to be filled gradually
							*stack=_list->_first;
							stackmaximumindex=0; // we have a single value on the stack so it's both the minimum and the maximum
						}else
							*stack=NULL; // essential because I'm using that to indicate that the stack is not initialized yet
						/*
						leftbeforestack=betweenstackelements;
						stacktop=previous;
						*/
					}
				}
				if(report)
					output("Stack size: %llu.\n",stacksize);
				// determine the index ranges
				Mindexrange indexrange={_list->_first->index,_list->_first->index};
				Mindexrange* nextindexrange=&indexrange;
				// MDH@10NOV2020: if we are supposed to create and use a stack the elements to stack will be placed between previous and current
				//				then we sort that sublist with quick sort (which doesn't pay attention to the indices which of course will now be incorrect)
				if(/*stack||*/_list->_last->index>_list->numberOfElements){ // possibly multiple range
					Mlistelement *prevlistelement=_list->_first;
					Mlistelement *listelement=prevlistelement->_next; // initialized to the second element
					while(listelement!=NULL){
						/* fill the sublist with the value associated with listelement 
						if(stack){
							leftbeforestack--;
							if(leftbeforestack==0){
								leftbeforestack=betweenstackelements;
								// unlink listelement i.e. link it's predecessor to the successor of listelement
								prevlistelement->_next=listelement->_next;
								if(report)
									outputValue("Stacking '",listelement->_value,"'.\n");
								// add listelement to the 'stack' without changing it's _next because if we didn't we couldn't keep iterating over all list elements and collect the index values
								stacktop->_next=listelement;
								stacktop=listelement; // the new stack top
								// don't do this!!!!! stacktop->_next=NULL; // as long as stacktop is the top of the stack it's successor will be NULL
							}else
							if(leftbeforestack==1)
								prevlistelement=listelement;
						}
						*/
						if(listelement->index!=nextindexrange->last+1){ // there's a gap
							nextindexrange->_next=CALLOC_1(sizeof(Mindexrange),'~',owner);
							nextindexrange=nextindexrange->_next;
							if(!nextindexrange)break; // TODO serious problem
							nextindexrange->first=listelement->index;
						}
						// update last
						nextindexrange->last=listelement->index;
						// if there is no next element, we're done
						listelement=listelement->_next;
					}
				}else
					nextindexrange->last=_list->_last->index;

				// we'll be detecting the natural 'runs' and whenever the direction changes we will get the stuff behind it sorted
				// original code from Mrunpoints() below

				/* sorting the stacked list elements is going to be fun
				if(stack){
					stacktop->_next=current; // the stack starts with element previous and the successor of previous used to be current
					current=previous->_next; // previous remained the same BUT current would've changed
					if(report)
						outputList("List after stacking: ",_list,"'.\n");
				}
				*/
				if(report)
				{outputValue("After stacking: first: '",previous->_value,"' - ");outputValue("second: '",current->_value,"'.\n");}
				// if elements were stacked it is quite unlikely that there are equal elements at the start	
				// nevertheless we use the same approach as originally: ascertaining that previous and current are different, so that we know the initial run direction to be either 1 or -1	
				do{
					sortstatistics.comparisons++;
					if(!equalto(previous->_value,current->_value))break;
					previous=current;
					current=current->_next;
					sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;sortstatistics.pointertests++; // the test below
				}while(current!=NULL);
				if(report)
				{
					outputValue("After skipping equal values: first: '",previous->_value,"' - ");
					outputValue("second: '",(current?current->_value:NULL),"'.\n");
				}

				// when all values are the same rundirection current will be NULL and rundirection will end up zero!!! 
				int rundirection=(current!=NULL?(largerthan(current->_value,previous->_value)==M_TRUE?1:-1):0);

				if(rundirection!=0){

					previous=current;current=current->_next; // move on to compare the third and second element next

					// if(stack)stackelementdistance=M_RUN_LENGTH; // start out with stacking every M_RUN_LENGTH element of the already sorted list
					unsigned long long runindex,stackelementindex,lowerstackindex,upperstackindex,middlestackindex; // start out with stacking every M_RUN_LENGTH element of the already sorted list
					
					Mlistelement *toinsert,*nexttoinsert,*compare,*lastcompare,*runsmallest,*runlargest,*smaller,*beforefirstone,*firstone,*stacklistelement;
					Mlistelement *smallest=NULL,*largest=NULL;
					Mvalue *runsmallestValue,*currentValue=NULL,*previousValue=previous->_value;
					sortstatistics.pointerassignments+=15;sortstatistics.fieldreferences++; // NOTE counting the declarations as well but if that will really make a difference
					bool emptyrun;
					while(1){
						sortstatistics.pointertests++;
						if(NULL==current)break;
						processedsofar++;
						currentValue=current->_value; // the value to compare
						sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
						if(report)
						{output("Run direction: %s",(rundirection>0?"up":"down"));outputValue(" - comparing '",previousValue,"'");outputValue(" with successor '",currentValue,"'.\n");}
						sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;sortstatistics.comparisons++; // this we know will always happen
						if(rundirection>0){ // in an up run
							if(smallerthan(currentValue,previousValue)==M_TRUE){
								// we have to merge the down and the up run to an single up run i.e. \/ to / where the first \ is from largest to smallest
								// ASSERT all the elements in \ (largest to smallest) are larger than smallest so we know the following loop always ends
								runlargest=previous;
								if(report)
									outputValue("Up run maximum: '",previousValue,"'.\n");
								sortstatistics.pointertests++;
								if(smallest!=NULL){
									emptyrun=(smallest->_next==previous);
									sortstatistics.fieldtests++;sortstatistics.pointertests++;
								}else
									emptyrun=false;
								if(!emptyrun){
									// both the main run is up, as well as the run we just finished
									// if we have a main run we have a largest, if we do not have a largest this up run is to become the main run
									// MDH@08NOV2020: if I'm right you can use runsmallest as initial insertion point
									//				apparently NOT as it loops indefinitely somewhere
									// MDH@09NOV2020: OOPS we're supposed to pass in the beforefirst NOT the first, so how do we find the predecessor of runsmallest?????
									sortstatistics.pointertests++;
									if(largest!=NULL){
										runsmallest=largest->_next; // the successor of the largest is essentially the first list element to merge from `another`
										smaller=NULL; // this is going to be the 'offset' to the main list that we're going to speed determine
										sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;sortstatistics.pointertests++;
										if(runsmallest){ // TODO should always be there I suppose!!!
											runsmallestValue=runsmallest->_value;
											sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.pointertests+=2; // test below
											if(stack!=NULL&&(*stack)!=NULL){ // if the first element in the stack is set
												sortstatistics.comparisons++;
												if(smallerthan(runsmallestValue,stack[stackminimumindex]->_value)!=M_TRUE){
													upperstackindex=stackmaximumindex; // replacing: stacksize-1
													sortstatistics.comparisons++;
													if(smallerthan(stack[upperstackindex]->_value,runsmallestValue)!=M_TRUE){
														lowerstackindex=stackminimumindex;
														while(upperstackindex!=lowerstackindex+1){ // not adjacent yet
															middlestackindex=(lowerstackindex+upperstackindex)>>1; // half
															sortstatistics.comparisons++;
															if(largerthan(runsmallestValue,stack[middlestackindex]->_value)==M_TRUE)
																lowerstackindex=middlestackindex;
															else
																upperstackindex=middlestackindex;
														}
														smaller=stack[lowerstackindex];
													}else{
														// if(report)
														{outputValue("New stack maximum in up run: '",runsmallestValue,"'");outputValue(" replacing: '",stack[upperstackindex]->_value,"'");output(" at index %llu.\n",upperstackindex);}
														smaller=stack[upperstackindex];
														if(upperstackindex!=stacksize-1){upperstackindex++;stackmaximumindex=upperstackindex;} // if the stack is not full yet, we get an additional element
														stack[upperstackindex]=runsmallest; // MDH@17NOV2020: need this obviously
													}
												}else{ // we have a new minimum
													// if(report)
													{outputValue("New stack minimum from up run: '",runsmallestValue,"'");outputValue(" replacing: '",stack[stackminimumindex]->_value,"'.\n");}
													// if we're filling the stack dynamically (instead of in one go)
													if(stackmaximumindex!=stacksize-1){ // the stack is not full yet (which is only possible with stackmultiplier<0)
														stackelementindex=(++stackmaximumindex);
														do{stack[stackelementindex]=stack[stackelementindex-1];}while(--stackelementindex!=stackminimumindex);
														// output("\n");
													}
													stack[stackminimumindex]=runsmallest;
												}
											}else{
												// MDH@10NOV2020: how about speeding up by doing an initial search of the list so far???
												// iterate over the part already sorted
												beforefirstone=NULL;
												firstone=_list->_first;
												sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;
												while(1){
													runindex=M_RUN_LENGTH*4;
													while(--runindex>=0){
														beforefirstone=firstone;
														firstone=beforefirstone->_next;
														sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;sortstatistics.pointertests+=2;
														if(firstone==largest){beforefirstone=NULL;sortstatistics.pointerassignments++;break;}
													}
													sortstatistics.pointertests++;
													if(NULL==beforefirstone)break;
													sortstatistics.comparisons++;
													if(largerthan(firstone->_value,runsmallestValue))break; // found one that is larger, which means we're done
													// ASSERT firstone is smaller than or equal to runsmallest
													smaller=beforefirstone;
													sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
												}
											}
										}
										if(report)
										{
											if(smaller)outputValue("'",smaller->_value,"' is smaller");else output("Nothing is smaller");
											if(runsmallest!=NULL)outputValue(" then '",runsmallestValue,"'");output(".\n");
										}
										largest=lmerge(_list,smaller,largest,previous);
										largest->index=0; // mark the largest with index 0 (so we can see where it currently is)
										if(report)
											outputList("List after merging up run: ",_list,"'.\n");
										if(stack!=NULL&&(*stack)==NULL){ // TODO should this be *stack!=NULL???????
											if(betweenstackelements>0){
												if(processedsofar>=stacksize){
													stackminimumindex=0;stackmaximumindex=stacksize-1;
													*stack=_list->_first;
													if(report)
														output("Registering %llu of %llu sorted list elements in the stack.\n",stacksize,processedsofar);
													Mlistelement** stackelement=stack;
													if(report)
														output("Stack elements after processing %llu elements:",processedsofar);
													stackelementindex=1; // MDH@16NOV2020: replacing 0 by 1 because we already have one stack element set
													// MDH@16NOV2020: it might be writing one value too many here?????
													while(++stackelementindex<=stacksize){
														if(report)
														{output(" %llu",stackelementindex);outputValue("=",(*stackelement)->_value,NULL);}
														*(stackelement+1)=(*stackelement)->_next;
														stackelement++;
													}
													if(report)
														output(".\n");
												}
											}
										}
									}
									smallest=_list->_first;
									sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.fieldtests++;sortstatistics.pointertests++;
									if(previous->_next==current){largest=previous;sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;}
									if(report)
									{outputValue("Maximum so far: '",largest->_value,"'.\n");outputList("List so far: '",_list,"'.\n");}
								}else
								if(report)
									output("The up run is empty!\n");
								rundirection=-1;
							}
						}else{ // in a down run
							if(largerthan(currentValue,previousValue)==M_TRUE){ // switching to an up run
								runsmallest=previous;
								if(report)
									outputValue("Down run minimum: '",previousValue,"'.\n");
								// if there's nothing in between the down run is empty
								sortstatistics.pointertests++;
								if(largest!=NULL){
									emptyrun=(largest->_next==previous);
									sortstatistics.fieldtests++;sortstatistics.pointertests++;
								}else
									emptyrun=false;
								if(!emptyrun){
									// we have to reverse the down sequence i.e. the successor of largest through runsmallest (previous)
									// NOTE previous->_next will change and no longer point to current, but largest will subsequently point to current (as it should)
									sortstatistics.pointertests++;
									if(largest!=NULL){
										previous=largest->_next; // is the element containing the maximum of the down run (we can do this because previous is not used anymore until it is reset at the end of the loop)
										sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;sortstatistics.fieldtests++;
										lreverse(_list,largest,runsmallest,report); // NOTE if this is the first run, largest will be NULL, so we have to pass the list to lreverse so it can determine the successor of beforefirst
										if(report)
											outputList("List after reversing the down list: '",_list,"'.\n");
										
										// now that the down run is transformed into an up run we can merge the sorted part so far with the upped run
										// oops, due to the reverse previous is no longer the largest value in the down run, you should use the successor of largest
										smaller=NULL; // this is going to be the 'offset' to the main list that we're going to speed determine
										// already set (see above): runsmallest=largest->_next; // the successor of the largest is essentially the first list element to merge from `another`
										sortstatistics.pointerassignments++;
										// MDH@10NOV2020: how about speeding up by doing an initial search of the list so far???
										runsmallestValue=runsmallest->_value;
										if(stack!=NULL&&(*stack)!=NULL){
											// when merging a down run we can have a new minimum
											sortstatistics.comparisons++;
											if(smallerthan(runsmallestValue,stack[stackminimumindex]->_value)!=M_TRUE){
												upperstackindex=stackmaximumindex; // replacing:stacksize-1;
												sortstatistics.comparisons++;
												if(smallerthan(stack[upperstackindex]->_value,runsmallestValue)!=M_TRUE){
													lowerstackindex=stackminimumindex;
													while(upperstackindex!=lowerstackindex+1){ // not adjacent yet
														middlestackindex=(lowerstackindex+upperstackindex)>>1; // half
														sortstatistics.comparisons++;
														if(largerthan(runsmallestValue,stack[middlestackindex]->_value)==M_TRUE)
															lowerstackindex=middlestackindex;
														else
															upperstackindex=middlestackindex;
													}
													if(report)
													{
													outputValue("'",runsmallestValue,"'");
													outputValue(" lies between '",stack[lowerstackindex]->_value,"'");
													outputValue("' and '",stack[upperstackindex]->_value,"'.\n");
													}
													smaller=stack[lowerstackindex];
												}else{
													// if(report)
													{outputValue("New stack maximum from down run: '",runsmallestValue,"'");outputValue(" replacing: '",stack[upperstackindex]->_value,"'");output(" at index %llu.\n",upperstackindex);}
													smaller=stack[upperstackindex];
													if(upperstackindex!=stacksize-1){ // if the stack isn't full yet, we can append the new maximum instead of replacing it
														upperstackindex++;stackmaximumindex=upperstackindex;
													}
													stack[upperstackindex]=runsmallest;
												}
											}else{ // a new minimum
												// if(report)
												{outputValue("New stack minimum from down run: '",runsmallestValue,"'");outputValue(" replacing: '",stack[stackminimumindex]->_value,"'.\n");}
												// if the stack is not full yet, we can prepend the new minimum
												if(stackmaximumindex!=stacksize-1){ // the stack is not full yet (which is only possible with stackmultiplier<0)
													stackelementindex=(++stackmaximumindex);
													do{stack[stackelementindex]=stack[stackelementindex-1];}while(--stackelementindex!=stackminimumindex);
													// outputChar('\n');
												}
												*(stack+stackminimumindex)=runsmallest; // as long as stackminimumindex equals zero, this would be the same as *stack=runsmallest
											}
										}else{
											// iterate over the part already sorted
											beforefirstone=NULL;
											firstone=_list->_first;
											sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;
											while(1){
												runindex=M_RUN_LENGTH;
												while(--runindex>=0){
													beforefirstone=firstone;
													firstone=beforefirstone->_next;
													sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;sortstatistics.pointertests+=2;
													if(firstone==largest){beforefirstone=NULL;sortstatistics.pointerassignments++;break;}
												}
												sortstatistics.pointertests++;
												if(NULL==beforefirstone)break;
												sortstatistics.pointerreferences++;sortstatistics.fieldreferences++;sortstatistics.comparisons++; // the test below
												if(largerthan(firstone->_value,runsmallestValue))break; // found one that is larger, which means we're done
												// ASSERT firstone is smaller than or equal to runsmallest
												smaller=beforefirstone;
												sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
											}
										}
										if(report)
										{
											if(smaller)outputValue("'",smaller->_value,"' is smaller");else output("Nothing is smaller");
											if(runsmallest)outputValue(" then '",runsmallestValue,"'");output(".\n");
										}
										largest=lmerge(_list,smaller,largest,previous);
										largest->index=0;
										if(report)
											outputList("List after merging down run: ",_list,"'.\n");
										if(stack!=NULL&&NULL==(*stack)){
											if(betweenstackelements>0){
												// filling the stack once if we have enough elements to fill it in one go
												if(processedsofar>=stacksize){
													stackminimumindex=0;stackmaximumindex=stacksize-1;
													*stack=_list->_first;
													Mlistelement** stackelement=stack;
													if(report)
														output("Stack elements after processing %llu elements:",processedsofar);
													stackelementindex=1; // MDH@16NOV2020 OOPS: need to do that here as well!!!!! replacing 0 by 1
													while(++stackelementindex<=stacksize){
														if(report)
														{output(" %llu",stackelementindex);outputValue("=",(*stackelement)->_value,NULL);}
														*(stackelement+1)=(*stackelement)->_next;
														stackelement++;
													}
													if(report)
														output(".\n");
												}
											}
										}
									}else{ // there's no main up run, so we only need to reverse this down run at the beginning of the list
										largest=_list->_first; // obviously
										sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
										lreverse(_list,NULL,runsmallest,report); // NOTE if this is the first run, largest will be NULL, so we have to pass the list to lreverse so it can determine the successor of beforefirst
										if(report)
											output("Initial down run reversed!\n");
									}						
									smallest=_list->_first; // TODO we might not need to do this actually
									sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
									// ASSERT we've successfully merged the down run into the up run that we're going to end up with
									if(report)
										outputValue("Minimum so far: '",smallest->_value,"'.\n");
									if(report)
										outputList("List so far: '",_list,"'.\n");
								}else
								if(report)
									output("The down run is empty!\n");
								rundirection=1;
							}
						}
						// and the next one!!
						previous=current;previousValue=currentValue; // update previous
						current=current->_next;
						sortstatistics.pointerassignments+=3;sortstatistics.pointerreferences+=2;sortstatistics.fieldreferences++;
						sortstatistics.pointertests++; // the test below
					}
					// take care of the last run
					sortstatistics.pointertests++; // test on largest below
					if(rundirection>0){ // end of an up run
						// obviously, if largest does not have a value yet, the list was already in ascending order to start with in which case we have nothing left to do!!
						if(largest!=NULL){
							if(report)
								output("Processing the final up run!\n");
							lmerge(_list,NULL,largest,previous);
						}else
						if(report)
							output("The list was already in ascending order.\n");
					}else
					if(rundirection<0){ // end of the down run
						if(report)output("Processing the final down run!\n");
						runsmallest=previous;
						sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
						if(largest!=NULL){ // something in front that we need to merge the down run into
							// nothing to reverse if there's only a single element in the down run
							sortstatistics.fieldtests++;sortstatistics.pointertests++;
							if(largest->_next!=previous){
								previous=largest->_next;
								sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
								lreverse(_list,largest,runsmallest,report);
							}
							lmerge(_list,NULL,largest,previous);
						}else
							lreverse(_list,NULL,runsmallest,report);
						// because it's a down run the maximum will be the last element in the list after reversal
					}

					// _list->_last=largest; // TODO this seems to be a valid assumption

					if(report)
					{outputList("The sorted list: '",_list,"'.\n");outputValue("First: '",_list->_first->_value,"'");outputValue(" - last: '",_list->_last->_value,"'.\n");}

					// reapply the collected indices from the index ranges
					nextindexrange=&indexrange;
					unsigned long long index=nextindexrange->first; // the fist index to assign
					Mlistelement* listelement=_list->_first;
					while(1){
						listelement->index=index;
						listelement=listelement->_next;
						if(NULL==listelement)break;
						// update the index to assign
						if(index==nextindexrange->last){
							nextindexrange=nextindexrange->_next;
							index=nextindexrange->first;
						}else
							index++;
					}
					unsigned long long indexrangesfreed=0;
					// free all dynamically allocated index ranges
					Mindexrange* indexrangetofree=indexrange._next;
					while(indexrangetofree!=NULL){
						indexrangesfreed++;
						nextindexrange=indexrangetofree->_next;
						FREE_DISOWNED_1(indexrangetofree,'~',owner);
						indexrangetofree=nextindexrange;
					}
					if(report)
						output("%lld captured index ranges freed!\n",indexrangesfreed);
				} // rundirection!=0

				if(stack!=NULL){
					if(NULL==(*stack))output("%sNo stack elements used (processed: %lld)!",M_WARNING_PREFIX,processedsofar);
					if(report)
						output("Freeing %llu elements of stack '%p'.\n",stacksize,stack);
					FREE_DISOWNED(stack,stacksize,-'l',owner); // replacing debug version: _FREE_DISOWNED(stack,stacksize,-'l',owner);
					if(report)
						output("Stack freed!\n");
					stack=NULL;
				}
			}
		}
	}
	return result;
}

// timsort implementation (based on geeksforgeeks.org/timsort)
/* replacing:
static void lmerge(Mlist* _list,Mlistelement* firstone,Mlistelement* lastone,Mlistelement* lastanother){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mlistelement *one=firstone,*another=lastone->_next,*nextone;
	if(report){
		outputValue("First value to first sequence to merge: '",firstone->_value,"'.\n");
		outputValue("Last value of first sequence to merge: '",lastone->_value,"'.\n");
		outputValue("First value of second sequence to merge: '",another->_value,"'.\n");
		outputValue("Last value of second sequence to merge: '",lastanother->_value,"'.\n");
	}
	Mvalue *oneValue=one->_value,*anotherValue,*tomoveupValue,*nexttomoveupValue;
	while(another){
		anotherValue=another->_value;
		// determine the first one that is larger than another
		while(smallerthanorequalto(oneValue,anotherValue)==M_TRUE){
			if(one==lastone){one=NULL;break;}
			one=one->_next;
			oneValue=one->_value;
		}
		if(!one)break; // all ones consumed
		// ASSERT oneValue>anotherValue
		if(report){outputValue("Inserting '",anotherValue,"'");outputValue(" in front of '",oneValue,"'.\n");}
		one->_value=anotherValue; // replace one->_value with anotherValue
		tomoveupValue=oneValue; // the first one to move on position up
		nextone=one;
		while(1){
			nextone=nextone->_next;
			nexttomoveupValue=nextone->_value; // remeber the one to move up next
			nextone->_value=tomoveupValue; // store the one to move up
			if(nextone==another)break; // when replaced the value associated with another done
			tomoveupValue=nexttomoveupValue; // update the value to move up
		}
		if(another==lastanother)break; // all another's inserted, so done
		one=one->_next;oneValue=one->_value;
		another=another->_next;
	}
}
*/
/* replacing:
// we have to implement the insertion sort a little different because we know first and can go up from there
// whereas the original algorithm determines the insertion point going back
// due to the merge the last element (containing the maximum could have changed), so we return it
static Mlistelement* lmerge(Mlist* _list,Mlistelement* beforeone,Mlistelement* beforeanother,Mlistelement* lastanother){
	
	bool report=amVerboseDebugging(); //||(M_MODULE_DEBUGGING&MM_SHELL);

	// 1. merging may result in a new smallest and largest element therefore it makes sense to actually take care of that first
	// if the minimum of the second sequence is the actual minimum we're going to remember this element as beforeone has to point to that element afterwards
	Mlistelement* mergedlistelement=beforeone; // any list element merged should set the _next on mergedlistelement to it

	Mlistelement* lastnext=(lastanother?lastanother->_next:NULL); // remember the successor of the last to merge

	// we're going to fix the successive index values when we're done as we have to keep the same sequence
	// (which yes is a nuisance)
	// currently the index field values are ascending AND we have to keep them ascending in the end result
	// this means that whenever a value is merged and therefore consumed the index of the consumed element should ALWAYS be less than whatever remains to be merged
	// I think we can't guarantee that the index of what is to be consumed is less than what is still to be consumed, so we really need to compare them with the successor
	// but if the index of one is above a successor we know that the index of one is from an another that was consumed and therefore it must be below the another's that are not yet consumed!!!
	Mlistelement *nextone;
	unsigned long long index;

	// initialize the first two elements to compare (one and another)
	Mlistelement *one=(beforeone?beforeone->_next:_list->_first),*another=beforeanother->_next;
	// and the values to compare
	Mvalue *oneValue=one->_value,*anotherValue=another->_value;
	if(report){
		outputValue("First value to first sequence to merge: '",oneValue,"'.\n");
		outputValue("Last value of first sequence to merge: '",beforeanother->_value,"'.\n");
		outputValue("First value of second sequence to merge: '",another->_value,"'.\n");
		outputValue("Last value of second sequence to merge: '",lastanother->_value,"'.\n");
	}
	
	while(one&&another){
		if(report){outputValue("Comparing '",oneValue,"'");outputValue(" with '",anotherValue,"'.\n");}
		// it makes sense to give precedence to the values from one because they are in front of another in the 
		// original list, so that when they are equal the earlier elements 
		// are we merging one value at a time? I suppose we could do two BUT it's better not to 
		// because in theory multiple values could have the same value and we want to maintain the 
		// original order as much as possible
		if(smallerthanorequalto(oneValue,anotherValue)==M_TRUE){ // one<=another
			// consume one
			if(mergedlistelement)mergedlistelement->_next=one;else _list->_first=another;
			mergedlistelement=one;
			if(report)outputValue("Merged value: '",mergedlistelement->_value,"'.\n");
			// we have kept the index values in the right ascending order, therefore we do not need to change the index of the consumed one!!!!
			if(one!=beforeanother){
				one=one->_next;
				oneValue=one->_value;
			}else // first sequence consumed
				one=NULL;
		}else{ // another<one
			if(mergedlistelement)mergedlistelement->_next=another;else _list->_first=another;
			mergedlistelement=another;
			// we do not need to compare another->index with its successor because another->index will always be smaller (it cannot get the index from a successor as the sucessor is not yet consumed)
			if(one->index<another->index){ // we have consume the index of one->index, one->index needs to become the larger index from another->index
				// we cannot simply set one->index to another->index because another->index is larger than any of the one's indices, so unfortunately we have to shift all indices up
				index=one->index; // remember the index to set on the consumed another
				// shift all the index values of the ones down except for the last one
				nextone=one;while(nextone!=beforeanother){nextone->index=nextone->_next->index;nextone=nextone->_next;}
				nextone->index=another->index; // ASSERT nextone should now equal beforeanother
				another->index=index; // the lower index is consumed!!!
			}
			if(report)outputValue("Merged value: '",mergedlistelement->_value,"'.\n");
			if(another!=lastanother){
				another=another->_next;
				anotherValue=another->_value;
			}else // second sequence consumed
				another=NULL;
		}
	}
	if(one){ // we've got one elements left
		while(1){
			mergedlistelement->_next=one; // for sure
			mergedlistelement=one;
			if(report)outputValue("Merged value: '",mergedlistelement->_value,"'.\n");
			// if we added the last one, we're done
			if(one==beforeanother)break;
			if(one->_next->index<one->index){
				index=one->index;one->index=one->_next->index;one->_next->index=index;
			}
			one=one->_next;
		}
	}else{ // we've got another elements left
		while(1){
			mergedlistelement->_next=another; // for sure
			mergedlistelement=another;			
			if(report)outputValue("Merged value: '",mergedlistelement->_value,"'.\n");
			// if we added the last one, we're done
			if(another==lastanother)break;
			if(another->_next->index<another->index){
				index=another->index;another->index=another->_next->index;another->_next->index=index;
			}
			another=another->_next;
		}
	}
	// ascertain that the last merged elements points to the successor of the last element
	mergedlistelement->_next=lastnext;
	if(!lastnext)_list->_last=mergedlistelement; // if there was no successor of the second sequence, we've merged up until the end of the list and we need to set the last of the list to mergedlistelement!!!!
	return mergedlistelement; // returning the last merged element (therefore the maximum)
}
*/

/**
 * @brief insertion sorts \p values from element index \p first through \p last
 * 
 * @param values 
 * @param first 
 * @param last 
 */
static void ainsertionsort(Mvalue** const values,unsigned long long first,unsigned long long last){
	// ASSERT first and last are assumed to be array positions (one-based) not zero-based
	//		which means that we need to insert [first,last-1] instead of (originally)
	//		this is done because we're using unsigneds so we cannot go below 0
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mvalue* toinsertValue=NULL;
	if(report)
		output("Insertion sorting array elements [%llu,%llu].\n",first,last);
	sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
	Mvalue** toinsertValueholder=(values+first); // the address of values[first] which is the first element to insert
	if(report)
		outputValue("\tFirst value: '",*toinsertValueholder,"'.\n");
	long long insertionarrayindex;
	// by using 1-based indices, we get rid of the test for zero
	for(register unsigned long long arrayindex=first+1;arrayindex<=last;arrayindex++){
		sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences+=2;
		toinsertValue=*toinsertValueholder; // the first time values[first+1]
		toinsertValueholder++;
		if(report)
			outputValue("\tInserting '",toinsertValue,"'.\n");
		// NOTE source (insertionSort) from 'https://geeksforgeeks.org/timsort/' adapted a bit (copied to timsort_geeksforgeeks.c)
		//	  the first element to compare with is the element in front of position arrayindex, and the element at position first would be the last
		insertionarrayindex=arrayindex-1; // still 1-based
		// NOTE the first time the condition insertionarrayindex>=first is always met
		do{
			// insertionarrayindex--; // converting from 1-based to 0-based (as we need in the comparison)
			sortstatistics.fieldreferences++;sortstatistics.pointerreferences+=2;
			sortstatistics.comparisons++;
			// switch to using a 0-based index into values
			if(largerthan(values[--insertionarrayindex],toinsertValue)!=M_TRUE)
			{insertionarrayindex++;break;} // increment again because we need to insert toinsertValue above the not larger value
			sortstatistics.fieldassignments++;sortstatistics.fieldreferences++;
			if(report)
			{outputValue("\t\tMoving value '",values[insertionarrayindex],"'");output(" at index %llu one position up",insertionarrayindex);outputValue(" replacing '",values[insertionarrayindex+1],"'.\n");}
			// because we decremented insertionarrayindex BEFORE instead of AFTER the following assignment
			// we're using the proper (i.e. zero-based) indices
			values[insertionarrayindex+1]=values[insertionarrayindex];
		}while(insertionarrayindex>=first);
		// ASSERT values[anotherarrayindex]<=temp
		sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;
		if(insertionarrayindex!=arrayindex){
			if(report)
			{outputValue("\t\tInserting value '",toinsertValue,"'");output(" at index %llu",insertionarrayindex);outputValue(" replacing '",values[insertionarrayindex],"'.\n");}
			values[insertionarrayindex]=toinsertValue;
		}
	}
}
/**
 * @brief atime sorts \p _array using array merge function \p arrayMergeFunction 
 * @details M_LL_INVALID is returned when \p _array is NULL
 * @param _array 
 * @param arrayMergeFunction 
 * @return long long M_TRUE on success, or M_FALSE or M_LL_INVALID on failure
 */
static long long atimsort(Marray* const _array,ArrayMergeFunction arrayMergeFunction){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	bool result=M_LL_INVALID;
	sortstatistics.pointertests++;
	if(_array!=NULL){
		result=M_TRUE;
		clock_t sortstart=clock(); // force to double
		sortstatistics.fieldreferences++;
		unsigned long long arraylength=_array->numberOfElements;
		if(arraylength>1){
			double sortstart=clock();
			sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
			Mvalue** values=_array->values;
			// sort the (fixed-size) runs with insertion sort
			// NOTE ainsertionsort expects indices one up the actual index
			for(unsigned long long runfirst=0;runfirst<arraylength;runfirst+=M_RUN_LENGTH)
				ainsertionsort(values,runfirst+1,MIN(runfirst+M_RUN_LENGTH,arraylength));
			// merge all pairs of successive runs
			for(unsigned long long size=M_RUN_LENGTH;size<arraylength;size<<=1)
				for(unsigned long long left=0;left<arraylength;left+=(size<<1))
					if(!arrayMergeFunction(values,left,left+size-1,MIN(left+(size<<1),arraylength)-1,report)){
						outputError("Failed to merge two array parts in timsort");
						return M_FALSE;
					}
		}
		output("Duration of array sorting by timsort: %.3f ms.\n",((double)(clock()-sortstart))/M_CLOCKS_PER_MS);
	}
	return result;
}
// linsertinginsertionSort works the same way linsertionSort does, except that it rearranges the list elements instead of moving the values
// and it returns the new last (if any)
/**
 * @brief insertion sorts \p _list from the element following \p beforefirst through \p last
 * @details rearranges the list elements not the values they contain as linsertionsort does
 * @param _list 
 * @param beforefirst 
 * @param last 
 * @return Mlistelement* the pointer to the smallest list element
 */
static Mlistelement* linsertinginsertionSort(Mlist * const _list,Mlistelement * const beforefirst,Mlistelement * const last){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	// ASSERT last should NOT be NULL
	Mlistelement* smallest=(beforefirst!=NULL?beforefirst->_next:_list->_first); // called first in linsertionSort
	if(report){outputValue("Insertion sorting '",smallest->_value,"'");outputValue(" through '",last->_value,"'.\n");}

	Mlistelement *afterlast=last->_next; // remember the successor of the last element

	// keep track of the largest list element found so far (that we need to link afterwards to the elements in front and behind)
	Mlistelement* largest=smallest;
	
	// to speed up inserting we keep track of the last inserted value, to be compared with the value to insert before comparing with the rest
	Mvalue* toinsertValue;
	// the first element to insert is the successor of the first element
	Mlistelement *larger,*notlarger,*nexttoinsert,*toinsert=smallest->_next;
	sortstatistics.pointerassignments+=8;sortstatistics.pointerreferences++;sortstatistics.fieldreferences+=3;sortstatistics.pointertests++;
	while(1){ // safety check
		sortstatistics.pointertests++;
		if(NULL==toinsert)break;
		nexttoinsert=toinsert->_next; // remember the next to insert
		toinsertValue=toinsert->_value; // the value to insert into what's in front of it
		// MDH@04NOV2020: we can speed things up a little bit by comparing with the largest value so far
		//				if toinsertValue is smaller than lastinsertedValue, we have to insert it
		sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences+=2;
		sortstatistics.comparisons++;
		if(smallerthan(toinsertValue,largest->_value)==M_TRUE){ // toinsertValue<largest value
			// we compare toinsertValue with all values ordered so far to find the first value that is larger
			notlarger=NULL;
			larger=smallest;
			sortstatistics.pointerassignments+=2;sortstatistics.pointerreferences++;
			// determine the first element with value larger than the value to insert
			while(1){
				sortstatistics.comparisons++;
				if(smallerthanorequalto(larger->_value,toinsertValue)!=M_TRUE)break;
				sortstatistics.pointertests+=2;
				if(larger==largest){outputBug("");outputValue("'",toinsertValue,"' seems to be larger than the largest so far: ");outputValue("'",largest->_value,"'.\n");break;}
				if(report){outputValue("'",larger->_value,"'<");outputValue("='",toinsertValue,"'.\n");}
				notlarger=larger;
				larger=larger->_next;
				sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;sortstatistics.pointerreferences++;
			}
			// larger>toinsert
			// larger should become the successor of toinsert (we haven't remembered toinsert->_next for nothing)
			toinsert->_next=larger;
			sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;sortstatistics.pointertests++;
			// toinsert has to become the successor of notlarger
			if(NULL==notlarger){ // we have a new minimum
				smallest=toinsert; // update what we consider to contain the smallest value
				if(report)outputValue("New smallest value: '",smallest->_value,"'.\n");
				if(beforefirst!=NULL)beforefirst->_next=smallest;else _list->_first=smallest; // we have to ascertain that beforefirst points to this new smallest value
				sortstatistics.fieldassignments++;sortstatistics.pointerreferences+=2;sortstatistics.pointerassignments++;
			}else{
				notlarger->_next=toinsert;
				sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;
			}
		}else{ // listelementValue>=largest value, so replaces largestValue, and there's no need to insert toinsert anywhere
			largest->_next=toinsert; // TODO do we need this?????
			largest=toinsert;
			if(report)outputValue("New largest value: '",largest->_value,"'.\n");
			sortstatistics.fieldassignments++;sortstatistics.pointerassignments++;sortstatistics.pointerreferences+=2;
		}
		sortstatistics.pointertests+=2;
		if(toinsert==last)break; // once we've inserted the last one, quit
		toinsert=nexttoinsert; // the list element to insert next, is the one we remembered at the beginning of the loop
		sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;
	}
	largest->_next=afterlast; // ascertain that the successor of largest is the successor of the original last
	sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;sortstatistics.pointertests++;
	if(NULL==afterlast){
		_list->_last=largest; // if there is no afterlast we should update the last element of the list
		sortstatistics.fieldassignments++;sortstatistics.pointerreferences++;
	}
	return largest;
}
/*
// NOTE linsertionSort moves the values NOT the list elements, therefore there's no need to change the index
static void linsertionSort(Mlist* _list,Mlistelement* first,Mlistelement* last){
	// ASSERT last should NOT be NULL
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mlistelement *afterlast=last->_next; // remember the successor of the last element
	// keep track of the smallest and largest list element found so far (that we need to link afterwards to the elements in front and behind)
	Mvalue* largestValue=first->_value;
	if(report){outputValue("Insertion sorting '",first->_value,"'");outputValue(" through '",last->_value,"'.\n");}
	
	// to speed up inserting we keep track of the last inserted value, to be compared with the value to insert before comparing with the rest
	Mvalue* listelementValue;
	// the first element to insert is the successor of the current smallest list element
	Mlistelement *checklistelement,*listelement=first->_next; // the first element to insert is the successor of the smallest list element
	while(listelement){ // safety check
		listelementValue=listelement->_value; // the value to insert into what's in front of it
		// MDH@04NOV2020: we can speed things up a little bit by comparing with the largest value so far
		//				if toinsertValue is smaller than lastinsertedValue, we have to insert it
		if(smallerthan(listelementValue,largestValue)==M_TRUE){ // listelementValue<largest value
			// NOTE we can never jump over the current largest value, so at some point the test will fail
			checklistelement=first;
			while(smallerthanorequalto(checklistelement->_value,listelementValue)==M_TRUE)checklistelement=checklistelement->_next;
			// checklistelement>listelement
			// we have to insert the value of listelement in front of checklistelement
			// we can consume checklistelement i.e. we can use it to move the values up
			Mlistelement *nextchecklistelement=checklistelement->_next;
			Mvalue *nextvaluetomoveup,*valuetomoveup=checklistelement->_value;
			checklistelement->_value=listelementValue; // with the value to move up remembered, we can safely replace its value with the value to insert (and consume checklistelement)
			while(1){
				checklistelement=checklistelement->_next;
				nextvaluetomoveup=checklistelement->_value; // remember the value that's to be replaced
				checklistelement->_value=valuetomoveup; // replace remembered value with the previous one
				if(checklistelement==listelement)break; // if we've overwritten listelement->_value (remembered in temp) we're done
				valuetomoveup=nextvaluetomoveup; // update valuetomove with what we remembered
			}
		}else{ // listelementValue>=largest value, so replaces largestValue, and there's no need to exchange any values
			largestValue=listelement->_value;
			if(report)outputValue("New largest value: '",largestValue,"'.\n");
		}
		if(listelement==last)break; // once we've inserted the last one, quit
		listelement=listelement->_next; // the list element to insert next, is the one we remembered at the beginning of the loop
	}
}
*/
// MDH@05NOV2020: changing timsort by registering the index ranges first, and writing the indices at the end
/**
 * @brief ltime sorts \p _list
 * @details returns M_LL_INVALID iff \p _list is NULL
 * @param _list 
 * @return long long M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
static long long ltimsort(Mlist* _list){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=M_LL_INVALID;
	sortstatistics.pointertests++;
	if(_list!=NULL){
		result=M_FALSE; // MDH@31MAR2023: as promised
		sortstatistics.fieldtests+=2;
		if(_list->_first!=_list->_last){ // a list with at least two elements
			// determine the index ranges
			Mindexrange indexrange={_list->_first->index,_list->_first->index};
			Mindexrange* nextindexrange=&indexrange;
			if(_list->_last->index>_list->numberOfElements){ // possibly multiple range
				Mlistelement* listelement=_list->_first;
				while(1){
					listelement=listelement->_next;
					if(NULL==listelement)break;
					if(listelement->index!=nextindexrange->last+1){ // there's a gap
						nextindexrange->_next=CALLOC_1(sizeof(Mindexrange),'~',owner);
						nextindexrange=nextindexrange->_next;
						if(NULL==nextindexrange)break; // TODO serious problem
						nextindexrange->first=listelement->index;
					}
					// update last
					nextindexrange->last=listelement->index;
				}
			}else
				nextindexrange->last=_list->_last->index;
			// sort the (fixed-size) runs with insertion sort
			unsigned long long runsize; 
			Mlistelement *runbeforefirst=NULL,*runlast=_list->_first;
			sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;
			while(1){
				sortstatistics.pointertests++;
				if(NULL==runlast)break;
				runsize=0;
				// ascertain that runlast is never NULL (as required by linsertionSort)
				while(++runsize<M_RUN_LENGTH){
					sortstatistics.fieldtests++;
					if(NULL==runlast->_next)break;
					runlast=runlast->_next;
					sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
				}
				runbeforefirst=linsertinginsertionSort(_list,runbeforefirst,runlast); // execute the run insertion sort that returns the new last element (which will exist)
				if(report)
					output("Last value: '",_list->_last->_value,"'.\n");
				runlast=runbeforefirst->_next; // now equal to the first element of the next run to insertion sort
				sortstatistics.pointerassignments+=2;sortstatistics.fieldreferences++;
			}
			/* replacing:
			Mlistelement *runlast,*runfirst=_list->_first;
			while(runfirst){
				runsize=0;
				runlast=runfirst;
				// ascertain that runlast is never NULL (as required by linsertionSort)
				while(++runsize<M_RUN_LENGTH&&runlast->_next)runlast=runlast->_next;
				linsertionSort(_list,runfirst,runlast);
				runfirst=runlast->_next;
			}
			*/
			if(report)outputList("List with sorted runs: '",_list,"'.\n");
			// the general idea of merging is to merge two successive blocks, until all blocks are merged
			// then the size of the block is doubled and all blocks are merged again
			// initialize size to the number of elements in a each block
			unsigned long long numberOfMerges,size=M_RUN_LENGTH;
			// as long as the size of a block is less than the number of elements there are blocks to merge
			Mlistelement *firstone,*lastone,*lastanother,*beforefirstone; // the one we need to pass to lmerge instead of firstone
			sortstatistics.pointerassignments+=4;
			while(size<_list->numberOfElements){
				size<<=1; // double the size
				if(report)output("Merging %llu elements each time.\n",size);
				numberOfMerges=1+(_list->numberOfElements-1)/size; // will at least equal 2
				if(report)output("Number of merges to execute: %llu.\n",numberOfMerges);
				lastanother=NULL;
				sortstatistics.pointerassignments++;
				do{
					// firstone is the successor of lastanother (if any)
					beforefirstone=lastanother;
					firstone=(beforefirstone?beforefirstone->_next:_list->_first);
					lastone=NULL;
					long long left=size; // the number of elements we need
					lastanother=firstone;
					sortstatistics.pointerassignments+=4;sortstatistics.pointerreferences+=2;sortstatistics.fieldreferences++;sortstatistics.pointertests++;
					while(--left>0){
						sortstatistics.fieldtests++;
						if(NULL==lastanother->_next)break;
						if(left*2==size){lastone=lastanother;sortstatistics.pointerassignments++;sortstatistics.pointerreferences++;}
						lastanother=lastanother->_next;
						sortstatistics.pointerassignments++;sortstatistics.fieldreferences++;
					}
					// only sort if there are two sequences
					sortstatistics.pointertests++;
					if(lastone!=NULL){
						sortstatistics.pointertests+=2;
						if(lastone!=lastanother){
							sortstatistics.pointerassignments++;
							lastanother=lmerge(_list,beforefirstone,lastone,lastanother);
							// replacing: lmerge(_list,firstone,lastone,lastanother);
						}
					}
					numberOfMerges--;
				}while(numberOfMerges>0);
				/* replacing:
				Mlistelement *beforeone,*beforeanother,*lastanother=NULL;
				do{
					beforeone=lastanother;
					long long left=size; // the number of elements we need
					do{
						lastanother=(lastanother?lastanother->_next:_list->_first);
						left--;
						if(left*2==size)beforeanother=lastanother;
					}while(left>0);
					// do we have something to merge???? (it is possible that there is an odd number of blocks)
					// assign the result of lmerge (the list element containing the maximum) to lastanother!!!
					if(beforeanother&&beforeanother!=lastanother)
						lastanother=lmerge(_list,beforeone,beforeanother,lastanother);
					numberOfMerges--;
				}while(numberOfMerges>0);
				*/
			}
			// reapply the collected indices from the index ranges
			nextindexrange=&indexrange;
			unsigned long long index=nextindexrange->first; // the fist index to assign
			Mlistelement* listelement=_list->_first;
			while(1){
				listelement->index=index;
				listelement=listelement->_next;
				if(NULL==listelement)break;
				// update the index to assign
				if(index==nextindexrange->last){
					nextindexrange=nextindexrange->_next;
					index=nextindexrange->first;
				}else
					index++;
			}
			// free all dynamically allocated index ranges
			Mindexrange* indexrangetofree=indexrange._next;
			while(indexrangetofree!=NULL){
				nextindexrange=indexrangetofree->_next;
				FREE_DISOWNED_1(indexrangetofree,'~',owner);
				indexrangetofree=nextindexrange;
			}
		}else
		if(report)outputWarning("No need to sort a list with less than 2 elements");
		result=M_TRUE;
	}
	return result;
}
// Msort is the generic entry point for sorting lists
/**
 * @brief sorts \p _tosortValue using sort method indicator \p _sortMethodValue
 * 
 * @param _tosortValue 
 * @param _sortMethodValue 
 * @return Mvalue* the wrapped boolean M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
Mvalue* Msort(Mvalue* _tosortValue,Mvalue* _sortMethodValue){
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	long long result=M_LL_INVALID;
	// we've got merge, tim and quick sort, below you can see what the default is
	char sortMethodVariant='\0',sortMethod='\0';
	if(_sortMethodValue!=NULL&&_sortMethodValue->type==VT_TEXT){sortMethod=_sortMethodValue->value._text->_c[0];if(sortMethod)sortMethodVariant=_sortMethodValue->value._text->_c[1];}
	// can either sort a list or the list elements in a map
	if(_tosortValue!=NULL){
		if(_tosortValue->type==VT_MAP){
			// will return the number of successfully sorted elements
			result=0;
			Mmapelement* mapelement=(_tosortValue->value._map!=NULL?_tosortValue->value._map->_first:NULL);
			while(mapelement!=NULL){
				Mvalue* mapelementValue=(mapelement->_variable!=NULL?mapelement->_variable->_value:NULL);
				if(mapelementValue!=NULL){
					Mvalue* mapelementValueSortResult=Msort(mapelementValue,_sortMethodValue);
					long long mapelementSortResult=(mapelementValueSortResult?mapelementValueSortResult->value._integer->ll:M_LL_INVALID);
					if(mapelementSortResult>0)result+=mapelementValueSortResult->value._integer->ll;
				}
				mapelement=mapelement->_next;
			}
		}else
		if(_tosortValue->type==VT_LIST){
			sortstatistics=(struct Msortstatistics){}; // this should work
			switch(sortMethod){
				case 'b': // "biden" sort
				case 'h':result=(sortMethodVariant?lharmonicabinarysort(_tosortValue->value._list,atoll(_sortMethodValue->value._text->_c+1)):lharmonicasort(_tosortValue->value._list));break;
				case 't':result=ltimsort(_tosortValue->value._list);break;
				// case 'm':result=lmergesort(_tosortValue->value._list);break;
				default:result=lquicksort(_tosortValue->value._list);break;
			}
			outputSortStatistics();
		}else
		if(_tosortValue->type==VT_ARRAY){
			sortstatistics=(struct Msortstatistics){}; // this should work
			switch(sortMethod){
				case 'q':result=aquicksort(_tosortValue->value._array);break;
				case 'b': // "biden" sort
				case 'h':result=aharmonicasort(_tosortValue->value._array,(sortMethodVariant=='b'?abinaryinsertmerge:(sortMethodVariant=='i'?ainsertmerge:amerge)));break;
				case 't':result=atimsort(_tosortValue->value._array,(sortMethodVariant=='b'?abinaryinsertmerge:(sortMethodVariant=='i'?ainsertmerge:amerge)));break;
				default:result=acsort(_tosortValue->value._array);break; // MDH@24DEC2020: the C sort uses the built-in qsort() method
				// case 'm':result=amergesort(_tosortValue->value._array);break;
			}
			if(sortMethod)outputSortStatistics();
		}
		if(report)
			output("Done sorting!\n");
	}
	if(report)
		output("Sort result: %lld.\n",result);
	return _getIntegerValue(result);
}
// similar to Msort but does not change the input in any way, returns NULL on failure
// MDH@24DEC2020 TODO it's advisable when sorting a list to create an array instead of a list to sort, and creating a list from the sorted array result
/**
 * @brief returns \p _tosortValue sorted using sort method \p _sortMethodValue
 * @details returns NULL on failure
 * @param _tosortValue 
 * @param _sortMethodValue 
 * @return Mvalue* \p _tosortValue sorted using sort method \p _sortMethodValue
 */
Mvalue* Msorted(Mvalue* _tosortValue,Mvalue* _sortMethodValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	Mvalue* sortedValue=NULL;
	if(_tosortValue!=NULL){
		if(_tosortValue->type==VT_MAP){
			// make a copy of the map, which means that we can sort the map elements in place
			Mmap* _tosortMap=owned_map(_getMapCopy(_tosortValue->value._map),owner);
			long long result=0;
			// will return the number of successfully sorted elements
			Mmapelement* mapelement=(_tosortMap!=NULL?_tosortMap->_first:NULL);
			while(mapelement!=NULL){
				Mvalue* mapelementValue=(mapelement->_variable!=NULL?mapelement->_variable->_value:NULL);
				if(mapelementValue!=NULL){
					Mvalue* mapelementValueSortResult=Msort(mapelementValue,_sortMethodValue);
					long long mapelementSortResult=(mapelementValueSortResult!=NULL?mapelementValueSortResult->value._integer->ll:M_LL_INVALID);
					if(mapelementSortResult<=0)break; // if failing to sort this map element value
				}
				mapelement=mapelement->_next;
			}
			if(mapelement!=NULL){ // something left unsorted
				FREE_MAP(_tosortMap,owner);
				outputError("");outputValue("Failed to sort '",mapelement->_variable->_value,"'.\n");
			}else
				sortedValue=_getValueOfMap(disowned_map(_tosortMap,owner));
		}else{
			char sortMethodVariant='\0',sortMethod='\0';
			if(_sortMethodValue!=NULL&&_sortMethodValue->type==VT_TEXT){
				sortMethod=_sortMethodValue->value._text->_c[0];
				if(sortMethod)sortMethodVariant=_sortMethodValue->value._text->_c[1];
			}
			if(_tosortValue->type==VT_LIST){
				Mlist* _tosortList=owned_list(_getListCopy(_tosortValue->value._list),owner);
				if(_tosortList!=NULL){
					sortstatistics=(struct Msortstatistics){};
					long long sortResult=M_LL_INVALID;
					switch(sortMethod){
						case 'b': // "biden" sort
						case 'h':sortResult=(sortMethodVariant?lharmonicabinarysort(_tosortList,atoll(_sortMethodValue->value._text->_c+1)):lharmonicasort(_tosortList));break;
						case 't':sortResult=ltimsort(_tosortList);break;
						// case 'm':sortResult=lmergesort(_tosortList);break;
						default:sortResult=lquicksort(_tosortList);break;
					}
					if(sortResult>0){ // _tosortList was successfully sorted
						outputSortStatistics();
						sortedValue=_getValueOfList(disowned_list(_tosortList,owner)); // NOTE will automatically
					}else
						FREE_LIST(_tosortList,owner);
					// output("List sort result: %lld.\n",sortResult); // DEBUG
				}else
					outputError("Failed to create a copy of the list to sort");
			}else
			if(_tosortValue->type==VT_ARRAY){
				Marray* _tosortArray=owned_array(_getArrayCopy(_tosortValue->value._array),owner);
				if(_tosortArray!=NULL){
					sortstatistics=(struct Msortstatistics){};
					long long sortResult=M_LL_INVALID;
					switch(sortMethod){
						case 'b': // "biden" sort
						case 'h':sortResult=aharmonicasort(_tosortArray,(sortMethodVariant=='b'?abinaryinsertmerge:(sortMethodVariant=='i'?ainsertmerge:amerge)));break;
						case 't':sortResult=atimsort(_tosortArray,(sortMethodVariant=='b'?abinaryinsertmerge:(sortMethodVariant=='i'?ainsertmerge:amerge)));break;
						// case 'm':sortResult=amergesort(_tosortArray);break;
						case 'q':sortResult=aquicksort(_tosortArray);break;
						default:sortResult=acsort(_tosortArray);break;
					}
					if(sortResult>0){ // _tosortList was successfully sorted
						if(sortMethod)outputSortStatistics();
						sortedValue=_getValueOfArray(disowned_array(_tosortArray,owner)); // NOTE will automatically
					}else
						FREE_ARRAY(_tosortArray,owner);
					// output("List sort result: %lld.\n",sortResult); // DEBUG
				}else
					outputError("Failed to create a copy of the array to sort");
			}
		}
	}
	// if(sortedValue)output("Sort done with value '%p' wrapping list '%p'!\n",sortedValue,sortedValue->value._list); // DEBUG
	return sortedValue;
}

// MDH@01NOV2020: grouping can make seperate sublists from a list either into a list or a map
/**
 * @brief groups elements of \p _listValue based on the group indicator returned by function \p functionValue
 * 
 * @param _listValue 
 * @param _functionValue 
 * @return Mvalue* the (wrapped) grouped list elements
 */
Mvalue* Mlgroup(Mvalue* _listValue,Mvalue* _functionValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_SHELL);
	if((NULL==_listValue||_listValue->type==VT_LIST)&&(NULL==_functionValue||_functionValue->type==VT_FUNCTION)){
		Mlist* list=(_listValue!=NULL?_listValue->value._list:NULL);
		if(list!=NULL){
			// if no function is specified Mlgroup essentially uses the value type to construct the groups
			Mmap* _groupMap=owned_map(__map("Mlgroup"),owner);
			if(_groupMap!=NULL){
				Mfunction* function=(_functionValue!=NULL?_functionValue->value._function:NULL);
				long long functionArgumentIndex=0;
				Mlist* _functionArgumentList=(function!=NULL?owned_list(__list("Mlgroup"),owner):NULL);
				if(_functionArgumentList!=NULL)functionArgumentIndex=appendedToList(_functionArgumentList,owner,NULL,M_LL_INVALID);
				if(NULL==function||functionArgumentIndex>0){
					Mvalue* listelementValue;
					Mlistelement* listelement=list->_first;
					while(listelement!=NULL){
						// does it make sense to group NULL values?????
						listelementValue=listelement->_value;
						char* group=NULL;
						if(listelementValue!=NULL){
							if(function!=NULL){
								if(appendedToList(_functionArgumentList,owner,listelementValue,functionArgumentIndex)>0){
									Mmap* _functionArgumentMap=_getFunctionArgumentMap(function,_functionArgumentList,owner);
									Mvalue* groupValue=getValueOfFunctionCall(function,"",_functionArgumentMap);
									FREE_MAP(_functionArgumentMap,owner);
									Mstring* _groupValueText=owned_string(_getValueText(groupValue,true),owner);
									group=string(_groupValueText);
									FREE_STRING(_groupValueText,owner);
								}
							}else // no function, all NULL values will end up in the "" map element
								group=VALUETYPENAMES[listelementValue->type];
						}else
							group="";
						// I suppose that when the function returns NULL, we can't group
						if(group!=NULL){
							if(report){outputValue("Group of '",listelementValue,"'");output(": '%s'.\n",group);}
							Mmapelement* groupMapelement=getMapelement(_groupMap,group);
							if(NULL==groupMapelement){ // the given group is not yet present
								Mlist* groupList=owned_list(__list("Mlgroup"),owner);
								if(groupList!=NULL){
									Mvalue* groupListValue=_getValueOfList(disowned_list(groupList,owner));
									if(groupListValue!=NULL){
										// NOTE unfortunately appendedToMap() will copy the list in groupListValue
										//	  TODO I have to think about whether this is correct or not
										//	  DONE it seems correct in that assignValue() will create a new
										//		   value wrapping a copy of the (currently) empty list
										//		   as a result groupListValue's reference count will still be 1
										if(appendedToMap(_groupMap,owner,group,groupListValue)!=M_TRUE){
											output(M_ERROR_PREFIX);
											outputValue("Failed to register '",listelementValue,"' in group");
											output(" '%s'.\n",group);
											// NOTE groupListValue will be freed by the gc, along with its list (groupList)
										}else{ // success
											groupMapelement=getMapelement(_groupMap,group);
											if(report)output("New group list registered.\n");
										}
									}else // groupList is not bound in a group list value, so we have to free it ourselves
										outputError("Failed to wrap a group list");
									// NOTE the following is a bit elaborate because we know that
									//	  groupListValue will NOT be referenced (count==0) when it exists
									//	  so in any normal situation, we either get the warning or the list is freed
									if(groupListValue!=NULL){
										// can't free groupList anyway because it is now owned
										if(groupListValue->count==0){
											if(Misdisowned(groupList)){
												groupListValue->value._list=NULL;
												free_list(groupList);
											}else
											if(report)
												outputWarning("Can't free the wrapped group list!");
										}else
											outputError("Can't free the group list now!");
									}else // groupList not bound to groupListValue
										FREE_LIST(groupList,owner); // groupList is most likely disowned already 
								}else
									outputError("Failed to create a group list");
							} 
							// we may safely assume that the group list is in the value of the group map element
							Mlist* groupList=(groupMapelement?groupMapelement->_variable->_value->value._list:NULL);
							if(groupList!=NULL){
								if(report)output("Number of elements in group list: %zd.\n",groupList->numberOfElements);
								long long groupListelementIndex=appendedToList(groupList,owner,listelementValue,M_LL_INVALID);
								if(groupListelementIndex<=0){
									output(M_ERROR_PREFIX);outputValue("Failed to register '",listelementValue,"'");
									output(" in group '%s'.\n",group);	
								}else
								if(report)output("Index in group list with %zd elements: %lld.\n",groupList->numberOfElements,groupListelementIndex);
							}else
								outputError("Failed to obtain the group list");							
						}else{
							output(M_WARNING_PREFIX);outputValue("'",listelementValue,"' was not grouped.\n");
						}
						listelement=listelement->_next;
					}
					return _getValueOfMap(disowned_map(_groupMap,owner));
				}else{ // we have a function, but not a valid function argument list
					if(_functionArgumentList!=NULL)FREE_LIST(_functionArgumentList,owner);
					outputError("Failed to prepare for calling the group function");
				}

			}else
				outputError("Failed to create the group map");
		}
	}else{
		if(_listValue!=NULL)outputError("First argument to the group function not a list");
		if(_functionValue!=NULL)outputError("Second argument to the group function not a function");
	}
	return NULL;
}
// MDH@03NOV2020: runs tells you how many runs there are in a given list and what length they are
//				it's more convenient to return the points where the direction changes
/**
 * @brief returns the list of run points in \p _listValue
 * 
 * @param _listValue 
 * @return Mvalue* the list of run points in \p _listValue
 */
Mvalue* Mrunpoints(Mvalue* _listValue){Mallocationowner owner=getOwner(__LINE__);
	if(_listValue!=NULL){
		if(_listValue->type==VT_LIST){
			Mlist* list=_listValue->value._list;
			Mlistelement* listelement=(list!=NULL?list->_first:NULL);
			if(listelement!=NULL){ // at least one element in the list
				Mlist* _runsList=owned_list(__list("Mrunpoints"),owner);
				if(_runsList!=NULL){
					// the first and last list element will always be in the list
					if(appendedToList(_runsList,owner,listelement->_value,listelement->index)>0){
						// if all elements are equal direction will remain 0, in which case the returned list will remain empty
						int direction,rundirection=0;
						unsigned long long valueindex;
						Mvalue *value,*nextvalue=listelement->_value;
						while(listelement->_next!=NULL){
							value=nextvalue;
							valueindex=listelement->index;
							// update direction to indicate whether the next element is down or up or equal
							listelement=listelement->_next;
							nextvalue=listelement->_value;
							// if value equals nextvalue, we simply continue, because an equal value can never end a run
							if(smallerthan(nextvalue,value)==M_TRUE)direction=-1;
							else
							if(largerthan(nextvalue,value)==M_TRUE)direction=1;
							else // never change the rundirection to 0, although it starts with 0!!
								continue;
							// NOTE an equal value (direction) 0 can never end a run!!!
							if(direction!=rundirection){ // change of direction sign
								if(rundirection!=0&&appendedToList(_runsList,owner,value,valueindex)<=0){
									outputError("Failed to update the runs list");
									break;
								}
								rundirection=direction;
							}
						}
						// add the last one (because it's can never be a direction change point)
						if(appendedToList(_runsList,owner,list->_last->_value,list->_last->index)>0)
							return _getValueOfList(disowned_list(_runsList,owner));
						outputError("Failed to add the last list element to the list of run points");
					}
					FREE_LIST(_runsList,owner);
				}
				outputError("Failed to create the runs list");
			}
		}else
		if(_listValue->type==VT_ARRAY){
			Marray* array=_listValue->value._array;
			unsigned long long arraylength=array->numberOfElements;
			if(arraylength>0){
				Mlist* _runsList=owned_list(__list("Mrunpoints"),owner);
				if(_runsList!=NULL){
					Mvalue** valueholder=array->values; // the pointer to the first Mvalue*
					Mvalue* value=*valueholder; // the first value pointed to
					// the first and last list element will always be in the list
					if(appendedToList(_runsList,owner,value,1)>0){ // the first value 
						// if all elements are equal direction will remain 0, in which case the returned list will have only a single value
						int direction,rundirection=0;
						Mvalue *nextvalue;
						// we're going to make valueindex run from 2 up until the last value, this is essentially the index of the element we're comparing with
						for(unsigned long long valueindex=2;valueindex<=arraylength;valueindex++){
							nextvalue=*(++valueholder); // valueindex should be the index (one-based) that is being used as list index
							// compare value with nextvalue
							// as we're only interested in changes of direction, we use rundirection to determine when that will be the case
							// if direction ends up nonzero we know the direction changed although it could started as zero
							direction=0;
							if(rundirection>=0&&smallerthan(nextvalue,value)==M_TRUE)direction=-1; // change from 0 or 1 to -1
							else
							if(rundirection<=0&&largerthan(nextvalue,value)==M_TRUE)direction=1; // change from 0 or -1 to 1
							// NOTE an equal value (direction) 0 can never end a run!!!
							if(direction!=0){ // the direction has changed, so we have to register the run point IFF rundirection is nonzero
								if(rundirection!=0&&appendedToList(_runsList,owner,value,valueindex)<=0){
									outputError("Failed to update the list of run points");
									break;
								}
								rundirection=direction;
							}
							value=nextvalue;
						}
						// add the last one (if not already in the list either when all values are equal)
						if(rundirection==0||appendedToList(_runsList,owner,value,arraylength)>0)
							return _getValueOfList(disowned_list(_runsList,owner));
						outputError("Failed to add the last array element to the list of run points");
					}
					FREE_LIST(_runsList,owner);
				}
				outputError("Failed to create the runs list");
			}
		}
	}
	return NULL;
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

// and finally
/**
 * @brief applies setting character \p settingCharacter
 * 
 * @param settingCharacter 
 * @return true on success
 * @return false on failure
 */
bool settingApplied(char settingCharacter){
	if(settingCharacter=='v'||settingCharacter=='V'){setVerbose(settingCharacter=='V');return true;}
	if(settingCharacter=='d'||settingCharacter=='D'){setDebugging(settingCharacter=='D');return true;}
	if(settingCharacter=='a'||settingCharacter=='A'){setAssisting(settingCharacter=='A');return true;}
	// all the rest unfortunately are interactive session characters
	return false;
}

// MDH@04MAR2020: good idea to have to plug in all callback in a call to getShellEnvironment instead of having specific setters for that
// MDH@07DEC2020: added argument locale for setting the locale
/**
 * @brief initializes the shell
 * 
 * @param settingCharacters 
 * @param locale 
 * @param moduleDebugging 
 * @param _inputCharReadFunction 
 * @param _inputInfoFunction 
 * @param _inputErrorFunction 
 * @param _outputTokenFunction 
 * @param _reoutputTokenFunction 
 * @param _updateLastTokenAutocompletionTextFunction 
 * @param _outputCommandInfoFunction 
 * @return true on success
 * @return false on failure
 */
bool shellInitialized(char const * const settingCharacters,char const * const locale,unsigned long long moduleDebugging,InputCharReadFunction _inputCharReadFunction,InputResponseFunction _inputInfoFunction,InputResponseFunction _inputErrorFunction,OutputTokenFunction _outputTokenFunction,ReoutputTokenFunction _reoutputTokenFunction,UpdateLastTokenAutocompletionTextFunction* _updateLastTokenAutocompletionTextFunction,OutputCommandInfoFunction _outputCommandInfoFunction){Mallocationowner owner=getOwner(__LINE__);

	M_MODULE_DEBUGGING=moduleDebugging; // MDH@05DEC2020

	// MDH@07DEC2020: if locale is not NULL try to set the current (overall) locale to it
	if(locale!=NULL){
		char* newlocale=setlocale(LC_ALL,locale);
		if(NULL==newlocale)
			output("%sFailed to use proposed locale '%s'!",M_ERROR_PREFIX,locale);
		else
		if(strcmp(locale,newlocale)==0)
			output("%sProposed locale '%s' not accepted. Still using '%s' as locale.\n",M_ERROR_PREFIX,locale,newlocale);
	}

	// initialize the random generator
	long long randomSeedGeneratorInitializationResult=getValueInteger(Msrand(NULL));
	if(randomSeedGeneratorInitializationResult!=M_TRUE)outputWarning("Failed to initialize the random seed generator");else outputInfo("Random generator initialized.");

	size_t numberOfSettingCharacters=(settingCharacters?strlen(settingCharacters):0);
	while(numberOfSettingCharacters>0)settingApplied(settingCharacters[--numberOfSettingCharacters]);

	// register the callbacks
	if(_inputCharReadFunction)inputCharReadFunction=_inputCharReadFunction;else outputWarning("No input character read function defined!");
	if(!_inputInfoFunction){inputInfoFunction=inputInfo;outputWarning("Using the default input info function.");}else inputInfoFunction=_inputInfoFunction;
	if(!_inputErrorFunction){inputErrorFunction=inputError;outputWarning("Using the default input error function.");}else inputErrorFunction=_inputErrorFunction;
	if(!_outputTokenFunction){outputTokenFunction=outputToken;outputWarning("Using the default output token function.");}else outputTokenFunction=_outputTokenFunction;
	if(!_reoutputTokenFunction)outputWarning("No reoutput token function.");else reoutputTokenFunction=_reoutputTokenFunction;
	if(!_updateLastTokenAutocompletionTextFunction)outputWarning("No update last token autocompletion text function.");else updateLastTokenAutocompletionTextFunction=_updateLastTokenAutocompletionTextFunction;
	if(!_outputCommandInfoFunction)outputWarning("No output command info function.");else outputCommandInfoFunction=_outputCommandInfoFunction;

	if(!inputInfoFunction)outputWarning("No input info function!");else outputInfo("Input info function set!");
	if(!inputErrorFunction)outputWarning("No input error function!");else outputInfo("Input error function set!");
	if(!inputCharReadFunction)outputWarning("No input char read function!");else outputInfo("Input char read function set!");
	if(!outputTokenFunction)outputWarning("No output token function!");else outputInfo("Output token function set!");
	if(!reoutputTokenFunction)outputWarning("No reoutput token function!");else outputInfo("Reoutput token function set!");
	if(!updateLastTokenAutocompletionTextFunction)outputWarning("No update last token auto completion text function!");else outputInfo("Update last token auto completion text function set!");
	if(!outputCommandInfoFunction)outputWarning("No output command info function!");else outputInfo("Output command info function set!");

	long long decimalprecision=getDP();
	if(decimalprecision==M_LL_INVALID)return NULL; // let's force starting with a default decimal context
	output("Default decimal precision: %llu. Call setdp() to change it.\n",decimalprecision);

	NAF_value=_getFloatValue(M_LD_NAN); // NaN is defined in Mexecution.h as 0.0/0.0 (as a constant)
	NAI_value=_getIntegerValue(M_LL_INVALID);

	// MDH@06NOV2019: NULL_value remains NULL for ever...
	UNDEFINED_value=__value("undefined");
	if(NULL==UNDEFINED_value){outputError("Failed to initialize UNDEFINED.");return false;}

	TYPES_value=_getMapValue(VT_TEXT,false,NULL);
	Mmap* TYPES_map=NULL;
	if(TYPES_value!=NULL)
		TYPES_map=TYPES_value->value._map;
	else
		outputError("Failed to initialize the TYPES constant");

	// MDH@23OCT2019: we really want NULL to be a variable with NO value, so we can actually use it to NULL a value!!
	//				therefore it shouldn't be a token value 
	/*
	NULL_value=_getValueOfToken(_getToken(NULL,TT_SQSTRING),true);
	if(NULL_value)NULL_value->value._token->text=__string("NULL");else outputBug("Failed to create NULL value!");
	*/

	// either set the DP_value to 0 (failed to get a decimal context somehow)
	/* MDH@20JUN2019: no need for DP_value anymore (as setdp() return _decimalContext->prec now): 
	_decimalContext=get_mpd_context(M_DP); // initialize the application-wide decimal context with precision M_DP
	assignValue(DP_value,_getIntegerValue(_decimalContext?_decimalContext->prec:0L));
	if(!DP_value)output("WARNING: Failed to initialize the decimal precision.");
	*/
	/* MDH@14NOV2019: using M for storing both the command (text) and the result (at that moment)
	_resultListValue=_getListValue(VT_UNDEFINED,true); // ascertain to have a list value in which the results can be stored
	// ESSENTIAL not to loose this list immediately!!!
	if(_resultListValue)incrementReferenceCount(_resultListValue);else outputWarning("Failing to create the results list. The results will not be available through the M function!");
	*/

	//if(amVerboseDebugging())
		output("Creating the root M environment.\n"); // DEBUG
	Menvironment* _Menvironment=owned_environment(_getNewEnvironment(),owner); // MDH@17JUL2019: calling the generic 'constructor' that will create a variable map for us automatically
	if(_Menvironment!=NULL){
		//if(amVerbose())
			output("M environment created.\n");
		_Menvironment->_name=owned_chars(_getChars("M"),Msubowner(owner,1)); // TODO why make a dynamic copy???
		//if(amVerbose())
			output("M environment named.\n");
		Mmap* environmentVariableMap=_Menvironment->_variableMap; // which must exist!!!
		Mfunctionmap* environmentFunctionMap=CALLOC_1(sizeof(Mfunctionmap),'W',Msubowner(owner,1));
		if(environmentFunctionMap!=NULL){

			// TODO should we allow assigning to NULL by defining NULL as a variable??????
			// MDH@29MAY2019: we've got (symbol) NULL
			// MDH@06NOV2019: the NULL constant will have value NULL forever
			if(!addVariable(_Menvironment,owner,M_NULL_VARIABLE_NAME,VT_UNDEFINED,true)||!setValue(_Menvironment,M_NULL_VARIABLE_NAME,NULL_value)){
				outputWarning("Failed to create, add or initialize constant NULL.");
			}
			// MDH@06NOV2019: whereas the UNDEFINED constant will be a non-NULL value of type VT_UNDEFINED (of which we do not need to set the value at all)
			if(!addVariable(_Menvironment,owner,M_UNDEFINED_VARIABLE_NAME,VT_UNDEFINED,true)||!setValue(_Menvironment,M_UNDEFINED_VARIABLE_NAME,UNDEFINED_value)){
				outputWarning("Failed to create, add or initialize constant UNDEFINED.");
			}
			if(NULL==NAF_value||!addVariable(_Menvironment,owner,"NAF",VT_FLOAT,true)||!setValue(_Menvironment,"NAF",NAF_value)){
				outputWarning("Failed to create, add or initialize Not-a-float constant NAF.");
				////////return false;
			}
			if(NULL==NAI_value||!addVariable(_Menvironment,owner,"NAI",VT_INTEGER,true)||!setValue(_Menvironment,"NAI",NAI_value)){
				outputWarning("Failed to create, add or initialize Not-an-integer default NAI.");
				////////return false;
			}
			// MDH@12SEP2023: if we have a types map
			if(TYPES_map!=NULL){
				if(!addVariable(_Menvironment,owner,"TYPES",VT_MAP,true)){
					outputError("Failed to register the TYPES constant");
					return false;
				}
				if(!setValue(_Menvironment,"TYPES",TYPES_value)){
					outputError("Failed to initialize the constant TYPES");
					return false;
				}
				// uoibdqftalmr#$ the mutable value type chars
				Mallocationowner valueOwner=getValueOwner();
				appendedToMap(TYPES_map,valueOwner,"undefined",_getTextValue("'u"));
				appendedToMap(TYPES_map,valueOwner,"token",_getTextValue("'o"));
				appendedToMap(TYPES_map,valueOwner,"integer",_getTextValue("'i"));
				appendedToMap(TYPES_map,valueOwner,"biginteger",_getTextValue("'b"));
				appendedToMap(TYPES_map,valueOwner,"rational",_getTextValue("'q"));
				appendedToMap(TYPES_map,valueOwner,"real",_getTextValue("'r"));
				appendedToMap(TYPES_map,valueOwner,"decimal",_getTextValue("'d"));
				appendedToMap(TYPES_map,valueOwner,"text",_getTextValue("'t"));
				appendedToMap(TYPES_map,valueOwner,"array",_getTextValue("'a"));
				appendedToMap(TYPES_map,valueOwner,"list",_getTextValue("'l"));
				appendedToMap(TYPES_map,valueOwner,"map",_getTextValue("'m"));
				appendedToMap(TYPES_map,valueOwner,"reference",_getTextValue("'r"));
				appendedToMap(TYPES_map,valueOwner,"function",_getTextValue("'f"));
				appendedToMap(TYPES_map,valueOwner,"environment",_getTextValue("'e"));
				appendedToMap(TYPES_map,valueOwner,"file",_getTextValue("'#"));
				appendedToMap(TYPES_map,valueOwner,"time",_getTextValue("'$"));
				Mlock(TYPES_value); // to lock the map itself (locking the variable does NOT suffice)
			}else
				outputError("Failed to initialize the TYPES map");

			/* MDH@13JUN2019: allow user to change the decimal precision
			if(!DP_value||!addVariable(_Menvironment,"$decimalprecision",VT_INTEGER,false)||!setValue(_Menvironment,"$decimalprecision",DP_value)){
				outputInfo("WARNING: Failed to create, add or initialize Not-an-integer default NAI.");
				////////return false;
			}*/
			// create and add PI and E constants!!!
#ifdef M_PI
			output("Long double PI is predefined in the math standard library!\n");
#else
			output("Long double PI is not predefined!\n");
#endif
			Mvalue* PI_value=NULL;
			long double acosl_1=acosl(-1);
			if(M_LD_PI!=acosl_1){
				output("%sLong double constant PI (%.*Lf) replaced by acosl(-1) (%.*Lf).\n",M_WARNING_PREFIX,33,M_LD_PI,33,acosl_1);
				PI_value=_getFloatValue(acosl_1);
			}else{
				output("Long double constant PI (%.*Lf) equals acosl(-1) (%.*Lf)!\n",33,M_LD_PI,33,acosl_1);
				PI_value=_getFloatValue(M_LD_PI);
			}
			if(NULL==PI_value){
				outputError("Failed to create PI");
				return NULL;
			}
			if(!addVariable(_Menvironment,owner,"PI",VT_FLOAT,true)){
				outputError("Failed to add PI");
				///////free_value(PI_value);
				return NULL;
			}
			if(!setValue(_Menvironment,"PI",PI_value)){
				////////free_value(PI_value);
				outputError("Failed to initialize PI");
				return NULL;
			}

#ifdef M_E
			output("Long double E is predefined in the math standard library!\n");
#else
			output("Long double E is not predefined!\n");
#endif
			Mvalue* E_value=NULL;
			long double expl1=expl(1);
			if(M_LD_E!=expl1){
				output("%sLong double constant E (%.*Lf) replaced by expl(1) (%.*Lf).\n",M_WARNING_PREFIX,33,M_LD_E,33,expl1);
				E_value=_getFloatValue(expl1);
			}else{
				output("Long double constant E (%.*Lf) equals expl(1) (%.*Lf)!\n",33,M_LD_E,33,expl1);
				E_value=_getFloatValue(M_LD_E);
			}
			if(NULL==E_value){
				///////free_value(E_value);
				outputError("Failed to create E");
				return NULL;
			}
			if(!addVariable(_Menvironment,owner,"E",VT_FLOAT,true)){
				outputError("Failed to add E");
				return NULL;
			}
			if(!setValue(_Menvironment,"E",E_value)){
				//////free_value(E_value);
				outputError("Failed to initialize E");
				return NULL;
			}

			// MDH@05DEC2020: obtain the current LC_ALL locale, and save it to the LOCALE variable
			// MDH@07DEC2020: we're going to use a map to store both the current locale setting (in property '') as well as all the fields
			Mvalue* localesettingsValue=_getValueOfMap(getLocalesettingsMap());
			if(NULL==localesettingsValue)outputWarning("Failed to obtain the current locale settings value");
			// the locale settings variable will be created as immutable, so once set it cannot be changed itself (although the map can be!!!)
			if(!addVariable(_Menvironment,owner,M_LOCALE_SETTINGS_VARIABLE_NAME,VT_MAP,true)){
				output("%sFailed to register '%s'.\n",M_ERROR_PREFIX,M_LOCALE_SETTINGS_VARIABLE_NAME);
				return NULL;
			}
			if(!setValue(_Menvironment,M_LOCALE_SETTINGS_VARIABLE_NAME,localesettingsValue)){
				//////free_value(E_value);
				output("%sFailed to initialize '%s'.\n",M_ERROR_PREFIX,M_LOCALE_SETTINGS_VARIABLE_NAME);
				return NULL;
			}

			// MDH@30SEP2020: let's add a CWD variable to contain the current working directory (if any) to makes things a little easier
			if(!addVariable(_Menvironment,owner,"CWD",VT_TEXT,true)){
				outputError("Failed to add CWD");
				return NULL;
			}
			char cwd[PATH_MAX];
			Mstring* cwd_str=owned_string(_getString("'"),owner);
			char* _cwd=getcwd(cwd,sizeof(cwd));output("Current working directory: '%s'.\n",_cwd);
			string_append(cwd_str,_cwd);
			if(string_last_char(cwd_str)!=M_PATH_SEPARATOR)string_append_char(cwd_str,M_PATH_SEPARATOR); // ascertain that CWD ends with a path separator!!!
			Mvalue* CWD_value=_getTextValue(string(cwd_str)); // I suppose we can directly use the result of getcwd() in _getString
			FREE_STRING(cwd_str,owner);
			if(!setValue(_Menvironment,"CWD",CWD_value)){
				//////free_value(E_value);
				outputError("Failed to initialize CWD");
				return NULL;
			}

			/*
			// we're going to store all commands in a list called M
			Mvalue* Mvalue=_getListValue(VT_UNDEFINED);
			if(!M_value){
				outputInfo("ERROR: Failed to create the M result list.");
				return false;
			}
			if(!addVariable(_Menvironment,"M",VT_LIST,true)){
				outputInfo("ERROR: Failed to add the M result list.");
				return false;
			}
			if(!setValue(_Menvironment,"M",M_value)){
				outputInfo("ERROR: Failed to initialize the M result list.");
				return false;
			}
			*/
			_Menvironment->_functionMap=environmentFunctionMap;

			// register if, while and for special functions
			// MDH@20DEC2020: one additional token of the if function
			if(!registerFunction(_Menvironment,owner,IFFUNCTION_NAME,Miffunction,4,(char*[]){"if condition","true clause","false clause","undefined clause"},(Mvalue*[]){NULL,NULL,NULL,NULL}))return false;
			// replacing: if(!completedValueTokenTokenTokenFunction(_Menvironment,owner,IFFUNCTION_NAME,Miffunction))return false;
			output("If function created!\n");
			// MDH@21DEC2020: all while arguments are like with do() also tokens now
			if(!registerFunction(_Menvironment,owner,WHILEFUNCTION_NAME,Mwhilefunction,2,(char*[]){"while condition","while body"},(Mvalue*[]){getValueOneOfType(VT_INTEGER),NULL}))return false;
			output("While function created!\n");
			// replacing: if(!completedTokenTokenFunction(_Menvironment,owner,WHILEFUNCTION_NAME),WHILEFUNCTION_NAME,Mwhilefunction))return false;
			
			if(!/*completedTokenListFunction*/registerFunction(_Menvironment,owner,FORFUNCTION_NAME,Mforfunction,2,(char*[]){"for condition","for body"},(Mvalue*[]){getValueOneOfType(VT_INTEGER),NULL}))return false; // MDH@23DEC2020: just like while no initial evaluation before executing
			output("For function created!\n");
			if(!/*completedTokenTokenTokenTokenTokenFunction*/registerFunction(_Menvironment,owner,FORWITHFUNCTION_NAME,Mforwithfunction,5,(char*[]){"initialization","condition","increment","body","result"},(Mvalue*[]){NULL,NULL,NULL,NULL,NULL}))return false;

			// MDH@05AUG2019: the do function has a single token to process
			if(!/*completedTokenListFunction*/registerFunction(_Menvironment,owner,DOFUNCTION_NAME,Mdofunction,2,(char*[]){"local variable map","do body"},(Mvalue*[]){_getMapValue(VT_UNDEFINED,false,NULL),NULL}))return false;
			if(!/*completedValueFunction*/registerFunction(_Menvironment,owner,EVALFUNCTION_NAME,Mevalfunction,1,(char*[]){"text to evaluate"},(Mvalue*[]){_getTextValue("\'"),NULL}))return false;
			// MDH@28OCT2020: no longer internal functions as defined in Menvironment.h/c but moved over here because they need command parsing features
			if(!/*completedStringMapTokenFunction*/registerFunction(_Menvironment,owner,DEFINEUSERFUNCTION_NAME,Mdefinefunction,3,(char*[]){"function name","argument map","function body"},(Mvalue*[]){NULL,NULL,NULL}))return false;
			if(!/*completedMapMapListFunction*/registerFunction(_Menvironment,owner,DEFINEANONYMOUSFUNCTION_NAME,Manonymousfunction,3,(char*[]){"parameter map","local variables map","body"},(Mvalue*[]){NULL,NULL,NULL}))return false;

			// MDH@22DEC2020: register the with() and endwith() function
			if(!/*completedMapFunction*/registerFunction(_Menvironment,owner,"with",Mwith,1,(char*[]){"local variables map"},(Mvalue*[]){NULL}))return false;
			// MDH@02APR2024: "end" changed to "endwith" to distinguish it effectively from "end" being used to effectively end a block of commands used as arguments to while, for, if clauses
			if(!/*completedValueFunction*/registerFunction(_Menvironment,owner,"endwith",Mendwith,1,(char*[]){"result value"},(Mvalue*[]){getValueZeroOfType(VT_INTEGER)}))return false;

			// MDH@02APR2024: for now decided to use a separate function with a different name than "endwith" that approximately does the same but is also recognized to end() a block of commands
			//                it's argument should be recognized as the result to store in the $-variable of the while of for loop (not applicable to then and else clauses as they don't require end)
			if(!/*completedValueFunction*/registerFunction(_Menvironment,owner,"end",Mend,1,(char*[]){"result value"},(Mvalue*[]){getValueZeroOfType(VT_INTEGER)}))return false;

			// // MDH@27FEB2020: Min is special as it used inputCharRead to read single characters, so it should only be available in sessions
			// if(!completedValueFunction(_Menvironment,"in"),"in",Min))return false; // moved out of registerInternalFunctions!!!!

			if(!registerInternalFunctions(_Menvironment,owner)){
				outputError("Failed to register all internal functions");
				return NULL;
			}
			/* MDH@14NOV2019: replaced by the M variable and M function
			// additional functions some of which need to know the root environment, I suppose a function should have access to its environment?????
			if(_resultListValue&&!completedIntegerFunction(_Menvironment,"M"),"M",getResult)){
				outputError("Failed to register function M (for requesting previous results)");
				return false;
			}
			*/
			/*
			if(!completedFunction(_Menvironment,"ml"),ml)){
				outputInfo("ERROR: Failed to register map list (constructor) function.");
				return false;
			}
			*/
			if(!completedIntegerFunction(_Menvironment,owner,"setdp",setdp)
				||!completedIntegerFunction(_Menvironment,owner,"getdc",getdc)
				||!completedIntegerFunction(_Menvironment,owner,"getdp",getdp)){
				outputError("Failed to register the setdp, getdc and getdp functions");
				return NULL;
			}
			// pi() functions (decimal and rational)
			if(!completedIntegerFunction(_Menvironment,owner,"pi$q",pi_q)
					||!completedIntegerFunction(_Menvironment,owner,"pi$ql",pi_ql)
					||!completedIntegerBooleanFunction(_Menvironment,owner,"pi",Mpi)){
				outputError("Failed to register the pi, pi$q and pi$ql functions");
				return NULL;
			}
			if(!completedValueValueFunction(_Menvironment,owner,"range",Mrange)){
				outputError("Failed to register the range function");
				return NULL;
			}
			// conversions (MDH@30OCT2019: real renamed to float because we actually have multiple representations of a real (like decimals and rationals))
			if(!completedValueFunction(_Menvironment,owner,"i",Mi)
					||!completedValueFunction(_Menvironment,owner,"l",Ml) // MDH@25NOV2020: conversion to a list
					||!completedValueFunction(_Menvironment,owner,"a",Ma) // MDH@25NOV2020: conversion to an array
					||!completedValueFunction(_Menvironment,owner,"m",Mm) // MDH@25NOV2020: conversion to a map
					||!completedValueFunction(_Menvironment,owner,"b",Mb)
					||!completedValueValueFunction(_Menvironment,owner,"t",Mt)
					||!completedValueFunction(_Menvironment,owner,"f",Mf)
					||!completedValueFunction(_Menvironment,owner,"q",Mq)
					||!completedValueFunction(_Menvironment,owner,"Q",MQ)
					||!completedValueValueFunction(_Menvironment,owner,"d",Md)
					||!completedValueFunction(_Menvironment,owner,"o",Mo)
					||!completedValueFunction(_Menvironment,owner,"O",MO)){
				outputError("Failed to register value type conversion functions");
				return NULL;
			}
			/* MDH@04NOV2019: moved over to Menvironment.h/c
			if(!completedValueFunction(_Menvironment,"type"),"type",Mtype)){
				outputError("Failed to register the type function");
				return false;
			}
			*/
			if(!completedValueFunction(_Menvironment,owner,"keys",Mkeys)){
				outputError("Failed to register the keys function");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"neg",Mneg)
					||!completedValueFunction(_Menvironment,owner,"bnot",Mbnot)
					||!completedValueFunction(_Menvironment,owner,"not",Mnot)){
				outputError("Failed to register all unary (neg, bnot, and not) functions");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"exists",Mexists)
					||!completedValueFunction(_Menvironment,owner,"numeric",Misnumeric)
					||!completedValueFunction(_Menvironment,owner,"list",Misalist)
					||!completedValueFunction(_Menvironment,owner,"scalar",Mscalar)
					||!completedValueFunction(_Menvironment,owner,"null",Mnull)
					||!completedValueFunction(_Menvironment,owner,"undefined",Mundefined)){
				outputError("Failed to register the exists, scalar, null and undefined functions");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"sign",Msign)){
				outputError("Failed to register the sign function");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"zero",Mzero)
					||!completedValueFunction(_Menvironment,owner,"positive",Mpositive)
					||!completedValueFunction(_Menvironment,owner,"negative",Mnegative)){
				outputError("Failed to register the zero, positive and negative functions");
				return NULL;
			}
			// MDH@06JAN2021: adding the split function!!
			if(!completedValueFunction(_Menvironment,owner,"sum",Msum)
					||!completedValueIntegerFunction(_Menvironment,owner,"setlength",Msetlen)
					||!completedValueValueValueFunction(_Menvironment,owner,"split",Msplit)
					||!completedValueFunction(_Menvironment,owner,"length",Mlen)
			){
				outputError("Failed to register the split function and the sum, length and setlength list functions");
				return NULL;
			}
			// MDH@01NOV2019: I have some generic list functions implemented
			if(!completedValueFunction(_Menvironment,owner,"empty",Mempty)||!completedValueFunction(_Menvironment,owner,"clear",Mclear)){
				outputError("Failed to register the empty and clear function");
				return false;
			}
			if(!completedListFunction(_Menvironment,owner,"statistics",Mstats)
					||!completedValueValueFunction(_Menvironment,owner,"corr",Mcorr) // MDH@05JAN2021: for those only interested in the correlation coefficient (and not simple sample statistics) 
					||!completedListFunction(_Menvironment,owner,"first",Mfirst)
					||!completedListFunction(_Menvironment,owner,"last",Mlast)
				){
				outputError("Failed to register the statistics, first and last list functions");
				return NULL;
			}

			if(!completedIntegerValueFunction(_Menvironment,owner,"array",marray)
					||!completedValueValueFunction(_Menvironment,owner,"fill",mfill)
			){
				outputError("Failed to register the array and fill array functions");
				return NULL;
			}

			if(!completedListIndexFunction(_Menvironment,owner,"removed",Mremoved)
					||!completedListValueFunction(_Menvironment,owner,"push",Mpush)
					||!completedListValueFunction(_Menvironment,owner,"append",Mpush)
					||!completedListValueFunction(_Menvironment,owner,"shove",Mshove)
					||!completedListValueFunction(_Menvironment,owner,"prepend",Mshove)
					||!completedListValueIndexFunction(_Menvironment,owner,"insert",Minsert)
					||!completedListTextFunction(_Menvironment,owner,"sort",Msort)
					||!completedListTextFunction(_Menvironment,owner,"sorted",Msorted)
					||!completedListFunction(_Menvironment,owner,"runpoints",Mrunpoints)
					||!completedListFunction(_Menvironment,owner,"pop",Mpop)
				){
				outputError("Failed to register the removed, push(=drop), shove, sort and pop functions");
				return NULL;
			}
			if(!completedListValueIntegerFunction(_Menvironment,owner,"find",Mfind)){
				outputError("Failed to register the find function");
				return NULL;
			}
			// MDH@29OCT2020: can't do without them
			if(!completedListFunctionValueFunction(_Menvironment,owner,"reduce",Mlreduce)
					||!completedListFunctionFunction(_Menvironment,owner,"map",Mlmap)
					||!completedListFunctionFunction(_Menvironment,owner,"filter",Mlfilter)
					||!completedListFunctionFunction(_Menvironment,owner,"foreach",Mlforeach)
					||!completedListFunctionFunction(_Menvironment,owner,"group",Mlgroup)
					){
				outputError("Failed to register the infamous reduce, map, filter and foreach list functions");
				return NULL;
			}

			if(!completedValueFunction(_Menvironment,owner,"tl",Mtl)){
				outputError("Failed to register the tl text function");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"fac",Mfac)
					||!completedValueFunction(_Menvironment,owner,"facd",Mfacd)){
				outputError("Failed to register the fac and facd function");
				return NULL;
			}
			if(!completedValueFunction(_Menvironment,owner,"reciprocal",Mreciprocal)
					||!completedValueFunction(_Menvironment,owner,"fibonacci",Mfibonacci)){ // MDH@10OCT2019
				outputError("Failed to register the reciprocal and fibonacci function");
				return NULL;
			}
			if(!completedValueValueFunction(_Menvironment,owner,"concat",Mconcat)){
				outputError("Failed to register the concat function");
				return NULL;
			}
			// register list conversions
			if(!completedListFunction(_Menvironment,owner,"l2m",l2m)
					||!completedListFunction(_Menvironment,owner,"l2ml",l2ml)
					||!completedListFunction(_Menvironment,owner,"ml2l",ml2l)
					||!completedListFunction(_Menvironment,owner,"ml2m",ml2m)){
				outputError("Failed to register list conversion functions");
				return NULL;
			}
			// register map conversions
			if(!completedListFunction(_Menvironment,owner,"m2ml",m2ml)
					||!completedListFunction(_Menvironment,owner,"m2l",m2l)){
				outputError("Failed to register map conversion functions");
				return NULL;
			}
			// MDH@28SEP2020: register file functions
			if(!completedValueFunction(_Menvironment,owner,"file",mfile)
				||!completedValueFunction(_Menvironment,owner,"fdelete",mfdelete)
				||!completedValueFunction(_Menvironment,owner,"files",mfiles)
				||!completedValueValueFunction(_Menvironment,owner,"fopen",mfopen)
				||!completedValueFunction(_Menvironment,owner,"fclose",mfclose)
				||!completedValueValueFunction(_Menvironment,owner,"fread",mfread)
				||!completedValueFunction(_Menvironment,owner,"freadline",mfreadline)
				||!completedValueValueFunction(_Menvironment,owner,"freadlines",mfreadlines)
				||!completedValueValueFunction(_Menvironment,owner,"fwrite",mfwrite)){
				outputError("Failed to registered the file functions");
				return NULL;
			}
			// MDH@10DEC2020: register system function(s)
			if(!registerNoArgumentFunction(_Menvironment,owner,"systemvariables",Msystemvariables)
				||!registerNoArgumentFunction(_Menvironment,owner,"clearenv",Mclearenv)
				||!completedValueFunction(_Menvironment,owner,"getenv",Mgetenv)
				||!completedValueFunction(_Menvironment,owner,"unsetenv",Munsetenv)
				||!completedValueValueFunction(_Menvironment,owner,"setenv",Msetenv)
				||!completedValueValueFunction(_Menvironment,owner,"putenv",Mputenv)
			){
				outputError("Failed to register the system environment functions");
				return NULL;				
			}
			// MDH@08DEC2020: register time functions
			if(!registerNoArgumentFunction(_Menvironment,owner,"now",Mnow)
				||!registerNoArgumentFunction(_Menvironment,owner,"gettimezone",Mgettimezone)
				||!completedValueFunction(_Menvironment,owner,"settimezone",Msettimezone)
				||!completedValueValueFunction(_Menvironment,owner,"calendartime",Mcalendartime)
				||!completedValueValueFunction(_Menvironment,owner,"time",Mparsetime)){
				outputError("Failed to register the time functions");
				return NULL;
			}
			// MDH@01MAY2023: register matrix functions
			if(!completedValueValueValueFunction(_Menvironment,owner,"matrix",Mmatrix)
					||!completedValueFunction(_Menvironment,owner,"diag",Mmatrixdiagonal)
					||!completedValueValueFunction(_Menvironment,owner,"mult",Mmatrixproduct)
					||!completedValueFunction(_Menvironment,owner,"inv",Mmatrixinverse)
					||!completedValueFunction(_Menvironment,owner,"transpose",Mmatrixtranspose)
					||!completedValueFunction(_Menvironment,owner,"det",Mmatrixdeterminant)
					||!completedValueFunction(_Menvironment,owner,"trace",Mmatrixtrace)){
				outputError("Failed to register the matrix functions");
				return NULL;
			}
		}
	}

	// if we successfully push _Menvironment (to become the current execution environment we succeeded)

	return(pushExecutionEnvironment(disowned_environment(_Menvironment,owner)));

}

// MDH@18MAR2024: putting the generic methods to deal with block commands here, although it's probably better to put 
//                them in a separate module at some point
const int8_t NUMBER_OF_BLOCK_KEYWORDS=7;

const enum BLOCK_KEYWORD_INDICES {M_KW_FOR,M_KW_WHILE,M_KW_IF,M_KW_ELIF,M_KW_ELSE,M_KW_END,M_KW_ENDALL};

const char * const BLOCK_KEYWORDS[NUMBER_OF_BLOCK_KEYWORDS]={"for","while","if","elif","else","end","endall"};

const int8_t BLOCK_FLAGS[NUMBER_OF_BLOCK_KEYWORDS]={1,1,1,3,3,2,6};

const bool BLOCK_KEYWORD_SINGLE_ARGUMENT[NUMBER_OF_BLOCK_KEYWORDS-2]={false,false,true,true,true}; // MDH@25MAR2024: single arguments are to be presented as a list and not appended as consecutive arguments
/**
 * @brief returns the block keyword id of \p keyword
 * 
 * @param keyword the text representation of a possible block keyword
 * @return int8_t the block keyword id or \p keyword
 */
int8_t getBlockKeywordId(char const * const keyword){
	output("Id of keyword '%s'",keyword);
	int8_t keywordId=NUMBER_OF_BLOCK_KEYWORDS;
	while(--keywordId>=0&&strcmp(BLOCK_KEYWORDS[keywordId],keyword));
	output(": %d.\n",keywordId);
	return keywordId;
}

/**
 * @brief adds \p command to the list of block commands in the current environment
 * 
 * @param command 
 * @return true on success
 * @return false on failure
 */
bool addBlockCommand(Mcommand const * const command,Mallocationowner owner_command){
	if(command!=NULL&&command->_firstToken!=NULL){
		// TODO the problem with the environment itself is that we do not know who owns it 
		//      unless we know every execution environment is essentially wrapped inside an Mvalue in which case we know who owns it!!
		Menvironment* environment=getExecutionEnvironment();
		if(environment!=NULL){
			Mtoken* nextInsertToken=environment->insertToken->next;
			if(environment->blockCommandsInserted){ // the placeholder token has been replaced by a command
				// insert a command separator
				Mtoken* listelementToken=owned_token(_getNewCommandToken(environment->insertToken,TT_LISTELEMENT),Msubowner(owner_command,1));
				if(listelementToken==NULL){outputError("Failed to insert the block command separator");return false;}
				listelementToken->text=owned_string(_getString(","),Msubowner(owner_command,2));
				listelementToken->significantCharacterCount=1;
				environment->insertToken->next=listelementToken;
				listelementToken->next=nextInsertToken;
				listelementToken->prev=environment->insertToken;
				environment->insertToken=listelementToken;
			}
			// connect to start of command
			environment->insertToken->next=command->_firstToken->next; // TODO insert first token as well????
			command->_firstToken->prev=environment->insertToken;
			// connect to end of command
			command->_lastToken->next=nextInsertToken;
			nextInsertToken->prev=command->_lastToken;
			// the last inserted token becomes the new insert token
			// NOTE that environment->continuationToken essentially remains the same!!!!
			environment->insertToken=command->_lastToken;
			environment->blockCommandsInserted=true;
			return true;
			// not ending the block yet but if we do we'd know where to continue searching for the next placeholder!!
			//////environment->continuationToken=nextInsertToken; // where to continue searching for the next plave holder token
			
			/*
			// ascertain to have a block command list
			//// MDH@26MAR2024 should always be true: if(NULL==environment->blockCommandList)environment->blockCommandList=owned_list(__list("block command list"),getValueDataOwner());
			if(appendedToList(environment->blockCommandList,getValueDataOwner(),_getValueOfToken(command->_firstToken->next),0)>0)
				return true;
				*/
		}
		outputError("Failed to register block command");
	}
	return false;
}

/**
 * @brief start a block of commands to embed in \p command replacing \p placeholderToken
 * 
 * @param blockKeywordId 
 * @param command
 * @param placeholderToken
 * @return true 
 * @return false 
 */
bool startBlock(Mcommand const * const command,Mtoken const * const placeholderToken){Mallocationowner owner=getOwner(__LINE__);
	// command is allowed to be NULL which means do not replace the incompleteCommand!!
	// we have to add a new execution environment
	if(placeholderToken!=NULL){
		Menvironment* _blockEnvironment=owned_environment(__environment(),owner);
		if(_blockEnvironment!=NULL){
			///////int8_t blockKeywordId=-1;
			Mtoken* startExpressionToken=placeholderToken->expr;
			Mtoken* functionNameToken=(startExpressionToken!=NULL&&startExpressionToken->type==TT_FUNCTION_CALL?startExpressionToken->prev:NULL);
			if(functionNameToken!=NULL){
				char* _functionName=_getSignificantTokenCharacters(functionNameToken);
				if(_functionName!=NULL){
					_blockEnvironment->_name=owned_chars(_getChars(_functionName),Msubowner(owner,1));
					free(_functionName);
				}
			}
			/* replacing: 
			int8_t blockKeywordId=getBlockKeywordId(_functionName);
			if(blockKeywordId>=0)_blockEnvironment->_name=owned_chars(_getChars(BLOCK_KEYWORDS[blockKeywordId]),Msubowner(owner,1));
			_blockEnvironment->blockKeywordId=blockKeywordId; // MDH@26MAR2024: remembering the block keyword id (although could have simply stored the bool from BLOCK_KEYWORD_SINGLE_ARGUMENT)
			*/
			//// NOT HERE ANYMORE!!!! _blockEnvironment->placeholderToken=placeholderToken;
			//// NOT HERE!!!! _blockEnvironment->continuationToken=placeholderToken->next;
			////////if(blockKeywordId<0)return true;
			Mtoken *prevPlaceholderToken=placeholderToken->prev,*nextPlaceholderToken=placeholderToken->next;
			// disconnect the placeholder token
			if(prevPlaceholderToken!=NULL)prevPlaceholderToken->next=nextPlaceholderToken;
			if(nextPlaceholderToken!=NULL)nextPlaceholderToken->prev=prevPlaceholderToken;
			if(pushExecutionEnvironment(disowned_environment(_blockEnvironment,owner))){
				Menvironment* environment=_blockEnvironment->_parent->value._environment;
				if(environment!=NULL){
					if(command!=NULL)environment->incompleteCommand=command; // remember the command that has to be completed NOTE when receiving NULL environment->incompleteCommand has be be left alone!!!!!
					environment->continuationToken=nextPlaceholderToken; // remember where to continue looking for placeholder tokens
					_blockEnvironment->insertToken=prevPlaceholderToken;
					_blockEnvironment->multipleCommandsAllowed=(nextPlaceholderToken!=NULL&&nextPlaceholderToken->type!=TT_LISTELEMENT);
					///////environment->blockKeywordId=blockKeywordId; // MDH@06APR2024: store the current keyword id so we can find the next one
					return true;
				}
				outputBug("Block parent environment vanished");
			}else
				outputError("Failed to activate the block command environment");
			/* replacing:
			if(_blockEnvironment->_name!=NULL){
				Mlist* _blockCommandList=owned_list(__list("block command list"),owner);
				if(_blockCommandList!=NULL){
					if(appendedToList(_blockCommandList,owner,_getValueOfToken(command->_firstToken),0)>0){
						// do NOT initialize the environment's blockCommandList until we effectively pushed the block environment
						// if I don't disown the block environment, pushExecutionEnvironment can't and won't take over the ownership
						if(pushExecutionEnvironment(disowned_environment(_blockEnvironment,owner))){
							_blockEnvironment->blockCommandList=owned_list(disowned_list(_blockCommandList,owner),getValueDataOwner());
							_blockEnvironment->insertToken=owned_token(insertToken,getValueDataOwner());
							return true;
						}
						outputError("Failed to activate the block environment");
					}
					// failed to append the block command to complete
					FREE_LIST(_blockCommandList,owner);
					// failed to initialize the pushed environment
				}
			}
			*/
			// failed to activate the block environment, so we have to free it again
			FREE_ENVIRONMENT(_blockEnvironment,owner);
		}
	}
	return false;
}
/**
 * @brief returns the popped block execution environment after completing the first block command list element
 * @details completes the first command with the rest of the commands wrapped in a list iif need be
 * @return true on success
 * @return false on failure
 */
Menvironment* endBlock(){
	Menvironment* environment=getExecutionEnvironment();
	if(environment!=NULL){
		if(environment->blockKeywordId<0)return environment;
		Mvalue* environmentValue=popExecutionEnvironment();
		if(environmentValue!=NULL)return environmentValue->value._environment;
		/*
		// we need to consume the block commands and append them to the parent
		Mlist* blockCommandList=environment->blockCommandList;
		if(blockCommandList!=NULL){
			// it would be nice to know the insert token
			// best to first complete the token sequence that we should insert into the parent's block command list
			Mtoken* completableTokens=blockCommandList->_first->_value->value._token;
			Mtoken* insertToken=environment->insertToken;
			if(completableTokens!=NULL&&insertToken!=NULL){
				Menvironment* parent=(environment->_parent!=NULL?environment->_parent->value._environment:NULL);
				if(parent!=NULL){
					///we should ascertain any parent of a block environment to have a block command list
					///if(parent->blockCommandList==NULL)parent->blockCommandList=owned_list(__list("block command list"),getValueDataOwner());
					if(parent->blockCommandList!=NULL){
						if(appendedToList(parent->blockCommandList,getValueDataOwner(),_getValueOfToken(completableTokens),0)>0){
							bool insertBlockCommandsAsList=BLOCK_KEYWORD_SINGLE_ARGUMENT[environment->blockKeywordId];
							// we should now consume the block command list (and get rid of it) at the insert token
							// let's distinguish between a single element and a compound block
							Mtoken* endOfArgumentListToken=insertToken->next;
							// open the argument list if need be
							if(insertBlockCommandsAsList){
								// insert list element separator in front of list of commands or single command to insert
								insertToken->next=NULL;
								insertToken->next=_getNewCommandToken(insertToken,TT_LISTELEMENT);
								insertToken=insertToken->next;
								insertToken->text=_getString(",");
								if(blockCommandList->numberOfElements>2){
									insertToken->next=_getNewCommandToken(insertToken,TT_LIST);
									insertToken=insertToken->next;
									insertToken->text=_getString("[");
								}
							}
							Mlistelement *blockCommandListelement=blockCommandList->_first,*nextblockCommandListelement=NULL;
							Mtoken* lastInsertedToken;
							do{
								// ASSERT blockCommandListelement must not be NULL!!!!
								nextblockCommandListelement=blockCommandListelement->_next;
								blockCommandListelement->_next=NULL; // preventing FREE_LISTELEMENT to attempt to free the successor that we still need to process
								// prefix comma as argument separator
								if(!insertBlockCommandsAsList){
									insertToken->next=NULL;
									insertToken->next=_getNewCommandToken(insertToken,TT_LISTELEMENT);
									insertToken=insertToken->next;
									insertToken->text=_getString(",");
								}
								insertToken->next=blockCommandListelement->_value->value._token; // point to the first token in the command to insert
								FREE_LISTELEMENT(blockCommandListelement,false,getValueDataOwner());
								// find the last non-NULL insert token
								lastInsertedToken=insertToken->next;
								while(lastInsertedToken->next!=NULL)lastInsertedToken=lastInsertedToken->next;
								insertToken=lastInsertedToken;
								blockCommandListelement=nextblockCommandListelement;
								if(blockCommandListelement==NULL)break; // if we've processed the last command to insert, no need to append a list element separator
								// append list element separator when we're supposed to group all commands in a list
								if(insertBlockCommandsAsList){
									insertToken->next=_getNewCommandToken(insertToken,TT_LISTELEMENT);
									insertToken=insertToken->next;
									insertToken->text=_getString(",");
								}
							}while(1);
							environment->blockCommandList->_first=NULL; // to prevent FREE_LIST by releasing the first list element again
							// close the argument list if need be
							if(insertBlockCommandsAsList){
								if(blockCommandList->numberOfElements>2){
									insertToken->next=_getNewCommandToken(insertToken,TT_END_OF_LIST);
									insertToken=insertToken->next;
									insertToken->text=_getString("]");
								}
							}
							// connect the last inserted token to the end of argument list token
							insertToken->next=endOfArgumentListToken;
							FREE_LIST(environment->blockCommandList,getValueDataOwner());
							environment->blockCommandList=NULL;
						}
						/// replacing
						//// consuming blockCommandList means updating _first as soon as we manage to append it to the parent block command list
						///Mlistelement* nextBlockCommandListElement;
						///do{
						///	nextBlockCommandListElement=blockCommandList->_first->_next;
						///	if(appendedToList(parent->blockCommandList,getValueDataOwner(),blockCommandList->_first->_value,0)<0)return false;
						///	blockCommandList->numberOfElements--;
						///	blockCommandList->_first=nextBlockCommandListElement;
						///}while(blockCommandList->_first!=NULL);
					}else
						outputBug("Missing block environment parent block command list");
					// how about extracting the completed block command before popping and destroying the block environment?????
					return popExecutionEnvironment(); // NOTE that popExecutionEnvironment() does NOT free the popped execution environment!!!!
					///
					///Menvironment* poppedBlockCommandEnvironment=getExecutionEnvironment();
					///Mtoken* completedBlockCommandToken=poppedBlockCommandEnvironment->blockCommandList->_first->_value->value._token;
					///poppedBlockCommandEnvironment->blockCommandList=NULL; // this might not be required though!!!
					
					///if(popExecutionEnvironment()){
					///	// shouldn't we append the completed block command token to the blockCommandList of the parent environment????
					///	if(appendedToList(getExecutionEnvironment()->blockCommandList,getValueDataOwner(),_getValueOfToken(completedBlockCommandToken),0)<=0)
					///		outputError("Failed to append the completed block command to the block command list");
					///	return completedBlockCommandToken;
					///}
					///outputError("Failed to pop the block command environment!");
					///
				}else
					outputBug("Block host environment vanished");
			}
			outputBug("Incomplete block command and/or insert token vanished");
		}else
			outputBug("Block environment command list vanished");
	*/
	}
	return NULL;
}

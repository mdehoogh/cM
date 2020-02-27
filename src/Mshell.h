/**
 * MDH@27FEB2020: every evaluation of an M command takes place 'inside' an M shell 
 *                which basically creates the top level M environment with all it's predefined constants and functions
 */
#include <limits.h>

#include "Menvironment.h"

// MDH@16MAY2019: not showing the error on the line above the user input line, but now below (in info color)
// MDH@22MAY2019 NOTE: const Mvalue* const is protested against in the call to _getValueText
// MDH@30OCT2019: if we let toInfoInputLine() return the number of lines it moved back we can pass that into toUserInputCursorPosition() to go down that number of lines
void inputInfo(const char* const fmt,...);
void inputError(const char* const fmt,...);

Mvalue* NULL_value=NULL;
Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount); // prototype definition of getValueOfExpression() so we can call it from getValueOfList() and getValueOfMap()

// all the available constants go here...
const char* VALUETYPENAMES[]={"unknown","token","integer","big integer","decimal","rational","float","text","list","map","reference"};
const char* const M_VARIABLE_NAME="M"; // MDH@14NOV2019: the variable to hold the list of remembered commands and the results they evaluated to
const char* const MFUNCTION_NAME="M"; // MDH@14NOV2019: the name of the function for getting previous results
const char* const IFFUNCTION_NAME="if";
const char* const WHILEFUNCTION_NAME="while";
const char* const FORFUNCTION_NAME="for";
const char* const DOFUNCTION_NAME="do"; // MDH@05AUG2019: the do function allowing the creation of variables local to the do execution
const char* const EVALFUNCTION_NAME="eval"; // MDH@28OCT2019: evaluating a text is nice
const char* const DEFINEUSERFUNCTION_NAME="function";
const char* const MUTABLEVALUETYPECHARS="uoibdqftlmr"; // the characters associated with each of the value types
const char* const IMMUTABLEVALUETYPECHARS="UOIBDQFTLMR"; // the characters associated with each of the value types
const char* const INFO_PREFIX=""; // MDH@27FEB2020: as for now NO actual info prefix text to use
const char* const ERROR_PREFIX="ERROR: "; // used in Mexecution.c as well (defined there as extern!!!)
const char* const WARNING_PREFIX="WARNING: "; // used in Mexecution.c as well (defined there as extern!!!)
const char* const BUG_PREFIX="BUG: "; // MDH@05NOV2019: for reporting bugs

const char* M_HIDDEN_VARIABLE_NAMES[]={"M","?","_"}; // MDH@14NOV2019: the variable names not to show when the variables are shown (with their current value)
const unsigned long long M_NUMBER_OF_HIDDEN_VARIABLES=3;// MDH@14NOV2019: yes, three of them

// MDH@31OCT2019: if the value of something equals the NULL value, this is the text to use to represent it, this is also the name of the NULL variable!!!
//                alternatively we could use capital letters to denote the variable, and lowercase to denote the value (which makes sense I suppose)
//                to prevent confusion it's best to use the same text for the value, otherwise they see 'null' as value and think they can use that to embed a NULL value!!!
//                OK the NULL value is displayed in the normal foreground color whereas the variable is displayed in another color (see showValueColored() for the coloring)
const char* const M_NULL_VALUE_TEXT="NULL"; // the text to represent values that are undefined...
const char* const M_NULL_VARIABLE_NAME="NULL";
const char* const M_UNDEFINED_VALUE_TEXT="UNDEFINED"; // the text to represent values that are undefined...
const char* const M_UNDEFINED_VARIABLE_NAME="UNDEFINED";

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
const long double M_LD_Q_EPS=1e-18; // this is the exact boundary to use for approximating 13/11 (which seems to be an notorious long double to approximate with rational (13/11)!!!)
const long double M_LD_PI=3.1415926535897932384626433832795L; // 31 non-zero decimal digits of PI (before the first 0)
const long double M_LD_E=2.718281828459045235360287471353L; // 30 decimal digits of E

long long M_DP=20; // the default decimal precision (initially 20) TODO should this be a constant after all?????????

const unsigned long long M_BITS_PER_ENV_LEVEL=8; // the minimum is 4 (to allow for a depth of 15 environments at the same time), the maximum is 60 of course in which case the maximum depth is 1, 8 gives a maximum depth of 7 and 256 at each level

const char M_WHITESPACE_CHARACTER=' '; // MDH@31OCT2019: let's use another character for storing whitespace in tokens (would normally be a blank)
const char M_NEWLINE_CHARACTER='\\'; // MDH@31OCT2019: the character to request a newline with!!!

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
// MDH@31OCT2019: let's use the backtick (`) as special whitespace character to use when one wants to insert a line break (i.e. continue the command on the next line)
//                although this would mean that it would show up when writing the tokens
//                we tried inserting a TT_WHITESPACE token with a backtick character (i.e. using ` as associated input character type) but ran into all kinds of problems so now we treat ` as W input character type
//                so it is appended to the current token, we only need to get it displayed in another color
//                ok, we're going to use \ for newline request character, so \ used to be % now becomes for type \ indicating a newline request (or escape character in a string!!!!)
//                switched to using the blank to indicate a newline request (using \ is a bit clumsy, backtick goes back to being the backtick, although no idea what we can use it for)
//                no we let \ be whitespace but we can turn it into a blank when it's a functional newline request
// MDH@04NOV2019: in order to be able to pass value references (i.e. variables) to a function we define @ as the redirection operator so that not the value but the value reference is returned (unresolved)
//                by defining @ as of type R we indicate that it refers to an identifier that has to be an existing variable!!!
//                                -------------------------------- !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~-
const char INPUTCHARACTERTYPES[]="iiiciiiihtniiniiiiiiiiiiiixmiiiiW!DCL%&S()*+,-.*NNNNNNNNNN:;>=>?RLLLLLLLLLLLLLLLLLLLLLLLLLL[W]%L`LLLLELLLLLLLLLLLLLLLLLLLLL{&}~b";
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
char* const NO_TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES]={"","","","","","","","","","","","","","q","q","`D","`S","","","","","","","","LEN","",""}; // MDH@30APR2019: oops one extra needed...

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
// MDH@05AUG2019: it's a pity that I need to allow a , behind a new variable in order to allow that when a do function call executes code after initializing these variables that are not yet recognized as created
//                we can solve this by remembering ALL variables when they are created in every expression that is tokenized, this would be possible by creating a tokenizing environment where we remember all created variables in in the tokenizing process
// MDH@04NOV2019: the reference token type added, so we can pass references to functions wrapped inside a value
/*
 "EXPR","UNA" ,"A","Baeru","BaErU","BAeRu","BaERu","BAeru" ,"Taeru","REF" ,"VAR"  ,"NEWVAR","L_EL","INT","REAL","DQSTRING","SQSTRING","END_DQS","END_SQS","LIST","END_L","MAP","M_V","END_M","FUNCTION","F_CALL","END_FC","CM","ERROR"},*/
const char * const TRANSITIONS[NUMBER_OF_FINISHABLE_TOKEN_TYPES][NUMBER_OF_TOKEN_TYPES]={ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*  , >?:    ] }="}, /* EXPRESSION */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; CDS% )&*  , >?:    ]{}="}, /* ONE CHARACTER UNARY !-+~ */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*  , >?:    ] }" }, /* ASSIGNMENT = */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ] }="}, /* Baeru finished bin.op. */ \
{""    ,""    ,"" ,"="    ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,"`R;!CDS%()&*+-,.>?:LEN[]{}" }, /* BaErU unfinished bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,""        ,""        ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; CDS% )&*  , >?:    ]"   }, /* BAeRu assignable repeatable */ \
{"("   ,"!-+~","" ,"="    ,""     ,""     ,""      ,"R"    ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  ,  ?:    ]"   }, /* BaERu comp. (<>) bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ]"   }, /* BAeru assignable bin.op. */ \
{"("   ,"!-+~","=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"`R; C  % )&*  , >?:    ]{}" }, /* Taeru ternary op. (? only now) */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,"LEN.",""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`R;! DS%( &*+-  >?:   [ { ="}, /* REFERENCE to an existing variable */ \
{""    ,""    ,"=",""     ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,"LEN." ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R;  DS (               {"  }, /* VARIABLE (identifier that is NOT a function) FUNCTION: some identifier not yet recognized as function name */ \
{""    ,""    ,"=",""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,"LEN."  ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,""      ,"C" ,"`R;! DS%()&*+- .>?:   [ {"  }, /* NEW_VARIABLE (variable that does not exist yet) */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,""      ,""  ,"` ; C  % )&*    >?:      }="}, /* LIST ELEMENT (similar to expression) */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,"N"  ,"."   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS (          L  [ {"  }, /* INTEGER: (signless) list of digits */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,""   ,"N"   ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS (      .   L  [ {"  }, /* REAL: part behind a decimal period */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,"D"      ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* DQSTRING: double quoted string */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,"S"      ,""    ,""     ,""   ,""   ,""     ,""        ,""      ,""      ,""  ,""                           }, /* SQSTRING: single quoted string */ \
{";"   ,""    ,"" ,"+"    ,"!="   ,"&"    ,">"     ,""     ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,"D"       ,"S"       ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS%&( * - .   LEN[ {"  }, /* END_DQSTRING: double quoted string at end of double quoted string */ \
{";"   ,""    ,"" ,"+"    ,"!="   ,"&"    ,">"     ,""     ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS%&( * - .   LEN[ {"  }, /* END_SQSTRING single quoted string at end of single quoted string */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,"]"    ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %& )*   .>?:      }="}, /* LIST: [ starts a list */ \
{";"   ,""    ,"=","?"    ,"!"    ,"&*"   ,">"     ,"-+%"  ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS  (     .   LEN  {"  }, /* END_OF_LIST: behind ] that ends a list */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,""    ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,""   ,""   ,"}"    ,""        ,""      ,")"     ,""  ,"` ; C  %& )*  ,.>?:    ]{ ="}, /* MAP: { starts a map */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,""    ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %& )*  , >?:    ] }="}, /* MAP_VALUE: : starts a map value */ \
{";"   ,""    ,"" ,"?"    ,"!="   ,"&*"   ,">"     ,"+"    ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,""   ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS% (   - .  :LEN  {"  }, /* END_OF_MAP: behind } that ends a map */ \
{""    ,""    ,"" ,""     ,""     ,""     ,""      ,""     ,""     ,""    ,""     ,""      ,""    ,""   ,""    ,""        ,""        ,""       ,""       ,""    ,""     ,""   ,""   ,""     ,""        ,"("     ,""      ,""  ,"`R;!CDS%& )*+-,.>?:   []{}="}, /* FUNCTION: some identifier recognized as function name */ \
{"("   ,"!-+~","" ,""     ,""     ,""     ,""      ,""     ,""     ,"R"   ,"LE"   ,""      ,","   ,"N"  ,"."   ,"D"       ,"S"       ,""       ,""       ,"["   ,""     ,"{"  ,""   ,""     ,""        ,""      ,")"     ,""  ,"` ; C  %&  *    >?:    ] }="}, /* FUNCTION_CALL ( following the name of a function */ \
{";"   ,""    ,"" ,"?:"   ,"!="   ,"&*"   ,">"     ,"-+%E" ,"?"    ,""    ,""     ,""      ,","   ,""   ,""    ,""        ,""        ,""       ,""       ,"["   ,"]"    ,""   ,":"  ,"}"    ,""        ,""      ,")"     ,"C" ,"`R   DS  (     .   L N  {"  }, /* END_OF_FUNCTION_CALL ) at end of last function call argument, ending a function call */ \
};

const uint8_t TOKENTYPE_IDS[NUMBER_OF_TOKEN_TYPES]={0,0b01010000,0b01000000,0b01100000,0b01100101,0b01101010,0b01100110,0b01101000,0b01110000,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,0b1000000,0b11111111};

// Decimal support
long long getDP();

Mvalue* getdc(Mvalue* value);

Mvalue* getdp(Mvalue* value);
Mvalue* setdp(Mvalue* value);

mpd_context_t* get_default_mpd_context();
// end Decimal stuff

// Mcommand stuff
// MDH@28OCT2019: because now often we need both the first and last token in a command it's probably best to combine them in a single command
typedef struct{
	Mtoken *_firstToken,*_lastToken;
	/////////////////bool identifierContinuationIsDirty; // convenient to keep it with the command itself
}Mcommand;
void free_command(Mcommand* _command);
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType/*,bool endOfInput*/); // prototype
Mcommand* _getNewCommand(bool withFirstToken);

// for appending input characters to the end of a given command (as used by Mevalfunction) and commandCharacterAccepted() in M.c
Mtoken* commandCharacterAppended(Mcommand* command,char inputChar,char *inputCharacterType,bool endOfInput);

// and finally obtaining a root environment
Menvironment* getShellEnvironment();

void outputCommandInfo(Mcommand* command);
bool isAValidCommand(Mcommand* command,bool report);
Mvalue* getCommandValue(Mcommand* command,char commandType);

// in interactive session we'll be using Muserinputline elements to keep track of where we are in the current command
typedef struct Muserinputline{
	size_t offset,index; // the number of characters on previous lines and the line index
	struct Muserinputline *_prev; // for accessing previous lines
}Muserinputline;
Muserinputline* _userinputline=NULL;
Muserinputline* __userinputline(int userInputLength); // to be called in an interactive session (or not in a non-interactive session)
size_t free_userinputline();
size_t toInfoInputLine();
void toUserInputCursorPosition(size_t linesDown);
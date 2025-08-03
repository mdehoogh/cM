/**
 * MDH@18APR2019: 
 * - moved from M.c so we can refer to Mexpression (which uses Token) in Mexecution.h
*/

#include "Msettings.h"

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

// MDH@23MAR2020: inserted TT_PROPERTY to indicate a variable that starts with M_PROPERTY_SEPARATOR_CHARACTER
// MDH@30APR2019: inserted TT_NEW_VARIABLE to indicate a variable that does not yet exist (which means it cannot be compared with, and should be assigned first)
// MDH@10APR2019: NUMBER_OF_FINISHABLE_TOKEN_TYPES defines the number of tokens that can finish, currently error and comment tokens can never end 
// MDH@31OCT2019: TT_WHITESPACE added which never should take part in any transition (so although it is a token type it is not counted in the number of (finishable) token types)
// MDH@04NOV2019: TT_REFERENCE added to be used for all references to existing variables
#define NUMBER_OF_FINISHABLE_TOKEN_TYPES 29
#define NUMBER_OF_TOKEN_TYPES NUMBER_OF_FINISHABLE_TOKEN_TYPES+2
// MDH@03MAY2019: TT_EXPRESSION is now the 'default' token type, so there's no need to set the token type on a new token
#define FOREACH_TOKENTYPE(TOKENTYPE) \
		TOKENTYPE(TT_EXPRESSION) \
		TOKENTYPE(TT_UNARY) \
		TOKENTYPE(TT_ASSIGNMENT) \
		TOKENTYPE(TT_BINARY_aeru) \
		TOKENTYPE(TT_BINARY_aErU) \
		TOKENTYPE(TT_BINARY_AeRu) \
		TOKENTYPE(TT_BINARY_aERu) \
		TOKENTYPE(TT_BINARY_Aeru) \
		TOKENTYPE(TT_TERNARY_aeru) \
		TOKENTYPE(TT_REFERENCE) \
		TOKENTYPE(TT_VARIABLE) \
		TOKENTYPE(TT_NEW_VARIABLE) \
		TOKENTYPE(TT_PROPERTY) \
		TOKENTYPE(TT_LISTELEMENT) \
		TOKENTYPE(TT_INTEGER) \
		TOKENTYPE(TT_REAL) \
		TOKENTYPE(TT_DQSTRING) \
		TOKENTYPE(TT_SQSTRING) \
		TOKENTYPE(TT_END_OF_DQSTRING) \
		TOKENTYPE(TT_END_OF_SQSTRING) \
		TOKENTYPE(TT_LIST) \
		TOKENTYPE(TT_END_OF_LIST) \
		TOKENTYPE(TT_MAP) \
		TOKENTYPE(TT_MAP_VALUE) \
		TOKENTYPE(TT_END_OF_MAP) \
		TOKENTYPE(TT_FUNCTION) \
		TOKENTYPE(TT_FUNCTION_CALL) \
		TOKENTYPE(TT_END_OF_FUNCTION_CALL) \
		TOKENTYPE(TT_PLACEHOLDER) \
		TOKENTYPE(TT_COMMENT) \
		TOKENTYPE(TT_ERROR) \
		TOKENTYPE(TT_WHITESPACE)
#define GENERATE_TOKENTYPE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
typedef enum TOKENTYPE_ENUM {
	FOREACH_TOKENTYPE(GENERATE_TOKENTYPE_ENUM)
}TokenType;
static const char* TOKENTYPE_STRING[]={
	FOREACH_TOKENTYPE(GENERATE_STRING)
};

/*
typedef struct{
	unsigned int ended:1; // one flag to indicate whether or not the Token has ended
	unsigned int complete:1; // one flag to indicate whether or not the token is complete
	unsigned int type:2; // 00=value, 01=unary operator, 02=binary operator, 03=ternary operator
	unsigned int subtype:4; // what subtype it is, i.e. the type of operator
}TokenType;
*/
typedef struct Mtoken{
	TokenType type; // actually the index into the TOKENTYPES array!!!
	uint8_t significantCharacterCount; // MDH@22MAR2019: the number of significant characters in the token (in front of any whitespace that the users add, should be set to the length of the text when that happens)
	uint16_t offset; // number of characters in front of this token in the command
	uint16_t position; // MDH@24JUN2020: keep track of the total number of lines and position on each line
	Mstring* text; // NOTE this is not an Mtext, Mstring is mutable whereas Mtext is not!!!!
	struct Mtoken* expr; // the expression this token is part of
	struct Mtoken* prev; // we need this during user input
	struct Mtoken* next;
	// MDH@07AUG2019: we will be pointing to the identifier in front of it, allowing us to determine whether a new identifier is an existing or new variable (no need to free this reference ever)
	struct Mtoken* prevIdentifier;
	long long argument; // the argument level (1 for local variables, all other values for non-local variables)
	uint64_t envid; // keep track of the special function call environment id
	long long element;
}Mtoken;

Mtoken* __token();
Mtoken* owned_token(Mtoken* _token,Mallocationowner owner_token);
Mtoken* disowned_token(Mtoken* _token,Mallocationowner owner_token);
void free_token(Mtoken* _token);
size_t checkToken(Mtoken const * const _token,Mallocationowner owner_token);
#define FREE_TOKEN(_token,owner_token) free_token(disowned_token(_token,owner_token))
/* a list of Mexpressions holds the body of an M function
typedef struct Mexpression{
    // a tokenized list of tokens, which means we have to move the definition of an Mtoken out of M.c to e.g. Mcommand or Mexpression even!!!
    Token* first;
}Mexpression;
void free_expression(Mexpression* _expression);
*/

size_t getTokenSignificantCharacterCount(Mtoken const * const token); // returns SIZE_T_MAX when invalid (no token given)
bool setTokenSignificantCharacterCount(Mtoken * const token,size_t significantCharacterCount);

// common helpers that also use the significantCharacterCount field
char* _getSignificantTokenCharacters(Mtoken const * const token); // the significant text as char*
Mstring* _getSignificantTokenText(Mtoken const * const token); // only the significant text
Mstring* _getTokenText(Mtoken const * const token); // entire token text
bool isTokenUnfinished(Mtoken const * const token);
bool isTokenFinished(Mtoken const * const token);
void finishToken(Mtoken * const token);
void unfinishToken(Mtoken * const token);

uint8_t compareTokens(Mtoken * const * token1,Mtoken * const * token2);